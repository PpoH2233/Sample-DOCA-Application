#include "eswitch_manager.h"

#include <errno.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <rte_byteorder.h>
#include <rte_ethdev.h>
#include <rte_flow.h>
#include <rte_mbuf.h>

#include "../ethernet_switch/switch_config.h"

static uint64_t monotonic_ns(void) {
  struct timespec value;
  if (clock_gettime(CLOCK_MONOTONIC, &value) != 0)
    return 0;
  return (uint64_t)value.tv_sec * 1000000000ULL + (uint64_t)value.tv_nsec;
}

static size_t append_text(char *buffer, size_t size, size_t used,
                          const char *format, ...) {
  va_list args;
  int written;

  if (used >= size)
    return used;
  va_start(args, format);
  written = vsnprintf(buffer + used, size - used, format, args);
  va_end(args);
  if (written < 0)
    return used;
  if ((size_t)written >= size - used)
    return size;
  return used + (size_t)written;
}

static struct managed_vswitch *find_vswitch(struct eswitch_manager *manager,
                                            uint16_t id) {
  for (size_t i = 0; i < ESWITCH_MAX_VSWITCHES; i++) {
    if (manager->switches[i].exists && manager->switches[i].id == id)
      return &manager->switches[i];
  }
  return NULL;
}

static int find_port_index(const struct eswitch_manager *manager,
                           uint16_t port_id) {
  for (uint16_t i = 0; i < manager->ports->count; i++) {
    if (manager->ports->items[i].ethernet->port_id == port_id)
      return i;
  }
  return -1;
}

static uint16_t collect_members(const struct eswitch_manager *manager,
                                uint16_t vswitch_id,
                                uint16_t extra_port_index,
                                uint16_t *port_ids) {
  uint16_t count = 0;

  for (uint16_t i = 0; i < manager->ports->count; i++) {
    if (manager->port_owner[i] == vswitch_id || i == extra_port_index)
      port_ids[count++] = manager->ports->items[i].ethernet->port_id;
  }
  return count;
}

static doca_error_t create_vswitch(struct eswitch_manager *manager,
                                   uint16_t id) {
  if (id == 0)
    return DOCA_ERROR_INVALID_VALUE;
  if (find_vswitch(manager, id) != NULL)
    return DOCA_ERROR_ALREADY_EXIST;
  for (size_t i = 0; i < ESWITCH_MAX_VSWITCHES; i++) {
    if (!manager->switches[i].exists) {
      manager->switches[i].exists = true;
      manager->switches[i].id = id;
      printf("VSWITCH CREATE: id=%u\n", id);
      return DOCA_SUCCESS;
    }
  }
  return DOCA_ERROR_NO_MEMORY;
}

static doca_error_t attach_port(struct eswitch_manager *manager,
                                uint16_t vswitch_id, uint16_t port_id) {
  struct managed_vswitch *vswitch = find_vswitch(manager, vswitch_id);
  struct eswitch_flood_group replacement = {0};
  uint16_t *members;
  uint16_t member_count;
  int port_index;
  doca_error_t result;

  if (vswitch == NULL)
    return DOCA_ERROR_NOT_FOUND;
  port_index = find_port_index(manager, port_id);
  if (port_index < 0)
    return DOCA_ERROR_NOT_FOUND;
  if (manager->port_owner[port_index] != 0)
    return DOCA_ERROR_IN_USE;
  members = calloc(manager->ports->count, sizeof(*members));
  if (members == NULL)
    return DOCA_ERROR_NO_MEMORY;
  member_count = collect_members(manager, vswitch_id, (uint16_t)port_index,
                                 members);

  /* Membership changes invalidate destination rules expanded per ingress. */
  result = eswitch_fdb_flush_vswitch(&manager->fdb, vswitch_id,
                                     "membership-change");
  if (result != DOCA_SUCCESS)
    goto out;
  result = eswitch_pipeline_destroy_flood_group(manager->pipeline,
                                                &vswitch->flood);
  if (result != DOCA_SUCCESS)
    goto out;
  result = eswitch_pipeline_build_flood_group(
      manager->pipeline, vswitch_id, members, member_count, &replacement);
  if (result != DOCA_SUCCESS) {
    if (replacement.paths != NULL) {
      vswitch->flood = replacement;
      manager->port_owner[port_index] = vswitch_id;
      goto out;
    }
    goto restore_old_flood;
  }
  result = eswitch_pipeline_attach_port(manager->pipeline,
                                        (uint16_t)port_index, vswitch_id);
  if (result != DOCA_SUCCESS) {
    doca_error_t cleanup = eswitch_pipeline_destroy_flood_group(
        manager->pipeline, &replacement);
    if (cleanup != DOCA_SUCCESS) {
      vswitch->flood = replacement;
      manager->port_owner[port_index] = vswitch_id;
      result = cleanup;
      goto out;
    }
    goto restore_old_flood;
  }
  vswitch->flood = replacement;
  manager->port_owner[port_index] = vswitch_id;
  printf("VSWITCH ATTACH: vs=%u dpdk-port=%u\n", vswitch_id, port_id);
  free(members);
  return DOCA_SUCCESS;

restore_old_flood:
  member_count = collect_members(manager, vswitch_id, UINT16_MAX, members);
  (void)eswitch_pipeline_build_flood_group(
      manager->pipeline, vswitch_id, members, member_count, &vswitch->flood);
out:
  free(members);
  return result;
}

static doca_error_t delete_vswitch(struct eswitch_manager *manager,
                                   uint16_t id) {
  struct managed_vswitch *vswitch = find_vswitch(manager, id);
  doca_error_t result;

  if (vswitch == NULL)
    return DOCA_ERROR_NOT_FOUND;
  result = eswitch_fdb_flush_vswitch(&manager->fdb, id, "vs-delete");
  if (result != DOCA_SUCCESS)
    return result;
  for (uint16_t i = 0; i < manager->ports->count; i++) {
    if (manager->port_owner[i] != id)
      continue;
    result = eswitch_pipeline_detach_port(manager->pipeline, i);
    if (result != DOCA_SUCCESS)
      return result;
    manager->port_owner[i] = 0;
  }
  result = eswitch_pipeline_destroy_flood_group(manager->pipeline,
                                                &vswitch->flood);
  if (result != DOCA_SUCCESS)
    return result;
  printf("VSWITCH DELETE: id=%u\n", id);
  *vswitch = (struct managed_vswitch){0};
  return DOCA_SUCCESS;
}

doca_error_t eswitch_manager_init(struct dpdk_io *io,
                                  struct switch_flow_ports *ports,
                                  struct eswitch_pipeline *pipeline,
                                  struct eswitch_manager *manager) {
  doca_error_t result;

  if (io == NULL || ports == NULL || pipeline == NULL || manager == NULL ||
      !io->port_started || !ports->started || !pipeline->created)
    return DOCA_ERROR_INVALID_VALUE;
  if (!rte_flow_dynf_metadata_avail())
    return DOCA_ERROR_NOT_SUPPORTED;
  manager->port_owner = calloc(ports->count, sizeof(*manager->port_owner));
  if (manager->port_owner == NULL)
    return DOCA_ERROR_NO_MEMORY;
  manager->io = io;
  manager->ports = ports;
  manager->pipeline = pipeline;
  result = eswitch_fdb_init(pipeline, SWITCH_MAX_FDB_ENTRIES,
                            SWITCH_FDB_AGING_SECONDS, &manager->fdb);
  if (result != DOCA_SUCCESS) {
    free(manager->port_owner);
    manager->port_owner = NULL;
    return result;
  }
  manager->started_ns = monotonic_ns();
  manager->next_aging_ns = manager->started_ns +
      (uint64_t)SWITCH_AGING_SCAN_SECONDS * 1000000000ULL;
  manager->initialized = true;
  return DOCA_SUCCESS;
}

static doca_error_t process_packet(struct eswitch_manager *manager,
                                   struct rte_mbuf *packet,
                                   uint64_t now_ns) {
  struct rte_ether_hdr header_copy;
  const struct rte_ether_hdr *header;
  uint16_t vswitch_id;
  uint16_t port_id;
  uint16_t *members;
  uint16_t member_count;
  uint32_t metadata = *RTE_FLOW_DYNF_METADATA(packet);
  int port_index;
  doca_error_t result;

  eswitch_metadata_decode(metadata, &vswitch_id, &port_id);
  port_index = find_port_index(manager, port_id);
  if (vswitch_id == 0 || port_index < 0 ||
      manager->port_owner[port_index] != vswitch_id) {
    fprintf(stderr,
            "Discarding ARM copy with stale/invalid metadata: value=%" PRIu32
            "\n",
            metadata);
    return DOCA_SUCCESS;
  }
  header = rte_pktmbuf_read(packet, 0, sizeof(header_copy), &header_copy);
  if (header == NULL)
    return DOCA_ERROR_INVALID_VALUE;
  if (header->ether_type == rte_cpu_to_be_16(RTE_ETHER_TYPE_VLAN) ||
      header->ether_type == rte_cpu_to_be_16(RTE_ETHER_TYPE_QINQ))
    return DOCA_SUCCESS; /* v1 is one untagged bridge domain per vSwitch. */

  printf("ARM RX: vs=%u port=%u len=%u src=%02x:%02x:%02x:%02x:%02x:%02x "
         "dst=%02x:%02x:%02x:%02x:%02x:%02x\n",
         vswitch_id, port_id, rte_pktmbuf_pkt_len(packet),
         header->src_addr.addr_bytes[0], header->src_addr.addr_bytes[1],
         header->src_addr.addr_bytes[2], header->src_addr.addr_bytes[3],
         header->src_addr.addr_bytes[4], header->src_addr.addr_bytes[5],
         header->dst_addr.addr_bytes[0], header->dst_addr.addr_bytes[1],
         header->dst_addr.addr_bytes[2], header->dst_addr.addr_bytes[3],
         header->dst_addr.addr_bytes[4], header->dst_addr.addr_bytes[5]);

  members = calloc(manager->ports->count, sizeof(*members));
  if (members == NULL)
    return DOCA_ERROR_NO_MEMORY;
  member_count = collect_members(manager, vswitch_id, UINT16_MAX, members);
  result = eswitch_fdb_learn(&manager->fdb, vswitch_id, 0,
                             &header->src_addr, port_id, members,
                             member_count, now_ns);
  free(members);
  return result;
}

doca_error_t eswitch_manager_poll_packets(struct eswitch_manager *manager,
                                          bool *did_work) {
  struct rte_mbuf *packets[SWITCH_PACKET_BURST];
  uint64_t now_ns;
  uint16_t received;

  if (manager == NULL || !manager->initialized || did_work == NULL)
    return DOCA_ERROR_INVALID_VALUE;
  now_ns = monotonic_ns();
  received = rte_eth_rx_burst(manager->io->parent_port_id,
                              SWITCH_RX_QUEUE_ID, packets,
                              SWITCH_PACKET_BURST);
  *did_work = received != 0;
  for (uint16_t i = 0; i < received; i++) {
    doca_error_t result = process_packet(manager, packets[i], now_ns);
    rte_pktmbuf_free(packets[i]);
    if (result != DOCA_SUCCESS)
      return result;
  }
  return DOCA_SUCCESS;
}

doca_error_t eswitch_manager_maintenance(struct eswitch_manager *manager) {
  uint64_t now_ns;

  if (manager == NULL || !manager->initialized)
    return DOCA_ERROR_INVALID_VALUE;
  now_ns = monotonic_ns();
  if (now_ns < manager->next_aging_ns)
    return DOCA_SUCCESS;
  manager->next_aging_ns = now_ns +
      (uint64_t)SWITCH_AGING_SCAN_SECONDS * 1000000000ULL;
  return eswitch_fdb_age(&manager->fdb, now_ns);
}

static bool parse_u16(const char *text, uint16_t *value) {
  char *end = NULL;
  unsigned long parsed;

  if (text == NULL || *text == '\0')
    return false;
  errno = 0;
  parsed = strtoul(text, &end, 0);
  if (errno != 0 || *end != '\0' || parsed > UINT16_MAX)
    return false;
  *value = (uint16_t)parsed;
  return true;
}

static size_t format_status(const struct eswitch_manager *manager,
                            char *response, size_t size) {
  size_t used = 0;
  size_t switch_count = 0;
  size_t assigned_count = 0;
  uint64_t uptime = (monotonic_ns() - manager->started_ns) / 1000000000ULL;

  for (size_t i = 0; i < ESWITCH_MAX_VSWITCHES; i++)
    switch_count += manager->switches[i].exists ? 1U : 0U;
  for (uint16_t i = 0; i < manager->ports->count; i++)
    assigned_count += manager->port_owner[i] != 0 ? 1U : 0U;
  used = append_text(response, size, used, "OK\n");
  used = append_text(response, size, used,
                     "service=eSwitch Management state=running uptime=%" PRIu64
                     "s\n",
                     uptime);
  used = append_text(response, size, used,
                     "ports=%u assigned=%zu available=%zu vswitches=%zu "
                     "fdb=%zu\n",
                     manager->ports->count, assigned_count,
                     manager->ports->count - assigned_count, switch_count,
                     manager->fdb.count);
  return used;
}

static size_t format_vswitches(const struct eswitch_manager *manager,
                               char *response, size_t size) {
  size_t used = append_text(response, size, 0, "OK\n");
  size_t shown = 0;

  for (size_t s = 0; s < ESWITCH_MAX_VSWITCHES; s++) {
    const struct managed_vswitch *vs = &manager->switches[s];
    bool first = true;
    if (!vs->exists)
      continue;
    used = append_text(response, size, used, "vs=%u ports=[", vs->id);
    for (uint16_t i = 0; i < manager->ports->count; i++) {
      if (manager->port_owner[i] != vs->id)
        continue;
      used = append_text(response, size, used, "%s%u", first ? "" : ",",
                         manager->ports->items[i].ethernet->port_id);
      first = false;
    }
    used = append_text(response, size, used, "]\n");
    shown++;
  }
  if (shown == 0)
    used = append_text(response, size, used, "(empty)\n");
  return used;
}

static size_t format_available_ports(const struct eswitch_manager *manager,
                                     char *response, size_t size) {
  size_t used = append_text(response, size, 0, "OK\n");
  size_t shown = 0;

  for (uint16_t i = 0; i < manager->ports->count; i++) {
    const struct ethernet_port *port = manager->ports->items[i].ethernet;
    if (manager->port_owner[i] != 0)
      continue;
    if (port->role == ETHERNET_PORT_ROLE_PARENT) {
      used = append_text(response, size, used,
                         "DPDK port %u (uplink/parent)\n", port->port_id);
    } else {
      /* The Arm representor does not expose the host netdev name. This stable
       * host/PF/VF identity identifies the VF visible on the x86 host. */
      used = append_text(response, size, used,
                         "DPDK port %u (host=%u pf=%u vf=%u)\n",
                         port->port_id, port->host_index, port->pf_index,
                         port->vf_index);
    }
    shown++;
  }
  if (shown == 0)
    used = append_text(response, size, used, "(none)\n");
  return used;
}

doca_error_t eswitch_manager_command(const char *request, char *response,
                                     size_t response_size, void *context) {
  struct eswitch_manager *manager = context;
  char command[256];
  char *save = NULL;
  char *verb;
  char *arg1;
  char *arg2;
  uint16_t first;
  uint16_t second;
  doca_error_t result = DOCA_SUCCESS;

  if (request == NULL || response == NULL || response_size == 0 ||
      manager == NULL || !manager->initialized)
    return DOCA_ERROR_INVALID_VALUE;
  snprintf(command, sizeof(command), "%s", request);
  command[strcspn(command, "\r\n")] = '\0';
  verb = strtok_r(command, " \t", &save);
  arg1 = strtok_r(NULL, " \t", &save);
  arg2 = strtok_r(NULL, " \t", &save);
  if (verb == NULL) {
    result = DOCA_ERROR_INVALID_VALUE;
  } else if (strcmp(verb, "status") == 0 && arg1 == NULL) {
    (void)format_status(manager, response, response_size);
    return DOCA_SUCCESS;
  } else if (strcmp(verb, "vs-list") == 0 && arg1 == NULL) {
    (void)format_vswitches(manager, response, response_size);
    return DOCA_SUCCESS;
  } else if ((strcmp(verb, "list-port-avaiable") == 0 ||
              strcmp(verb, "list-port-available") == 0) && arg1 == NULL) {
    (void)format_available_ports(manager, response, response_size);
    return DOCA_SUCCESS;
  } else if (strcmp(verb, "show_fdb") == 0 && arg2 == NULL) {
    if (arg1 == NULL) {
      first = 0;
    } else if (!parse_u16(arg1, &first)) {
      result = DOCA_ERROR_INVALID_VALUE;
      goto error;
    }
    snprintf(response, response_size, "OK\n");
    eswitch_fdb_format(&manager->fdb, first,
                       response + strlen(response),
                       response_size - strlen(response));
    return DOCA_SUCCESS;
  } else if (strcmp(verb, "vs-create") == 0 && arg2 == NULL &&
             parse_u16(arg1, &first)) {
    result = create_vswitch(manager, first);
  } else if (strcmp(verb, "vs-delete") == 0 && arg2 == NULL &&
             parse_u16(arg1, &first)) {
    result = delete_vswitch(manager, first);
  } else if (strcmp(verb, "vs-port-attach") == 0 &&
             parse_u16(arg1, &first) && parse_u16(arg2, &second) &&
             strtok_r(NULL, " \t", &save) == NULL) {
    result = attach_port(manager, first, second);
  } else {
    result = DOCA_ERROR_INVALID_VALUE;
  }

  if (result == DOCA_SUCCESS) {
    snprintf(response, response_size, "OK\n");
    return DOCA_SUCCESS;
  }
error:
  snprintf(response, response_size, "ERR code=%d message=%s\n", result,
           doca_error_get_descr(result));
  return result;
}

doca_error_t eswitch_manager_destroy(struct eswitch_manager *manager) {
  doca_error_t first_error = DOCA_SUCCESS;

  if (manager == NULL)
    return DOCA_ERROR_INVALID_VALUE;
  if (!manager->initialized) {
    free(manager->port_owner);
    *manager = (struct eswitch_manager){0};
    return DOCA_SUCCESS;
  }
  for (size_t i = 0; i < ESWITCH_MAX_VSWITCHES; i++) {
    doca_error_t result;
    if (!manager->switches[i].exists)
      continue;
    result = delete_vswitch(manager, manager->switches[i].id);
    if (first_error == DOCA_SUCCESS && result != DOCA_SUCCESS)
      first_error = result;
  }
  if (first_error == DOCA_SUCCESS) {
    doca_error_t result = eswitch_fdb_destroy(&manager->fdb);
    if (result != DOCA_SUCCESS)
      first_error = result;
  }
  if (first_error == DOCA_SUCCESS) {
    free(manager->port_owner);
    *manager = (struct eswitch_manager){0};
  }
  return first_error;
}

void eswitch_manager_release(struct eswitch_manager *manager) {
  if (manager == NULL)
    return;
  for (size_t i = 0; i < ESWITCH_MAX_VSWITCHES; i++) {
    struct eswitch_flood_group *group = &manager->switches[i].flood;
    for (uint16_t p = 0; p < group->path_count; p++) {
      if (group->paths[p].pipe != NULL)
        doca_flow_pipe_destroy(group->paths[p].pipe);
      free(group->paths[p].members);
    }
    free(group->paths);
  }
  while (manager->fdb.head != NULL) {
    struct eswitch_fdb_entry *entry = manager->fdb.head;
    manager->fdb.head = entry->next;
    free(entry->hardware.destinations);
    free(entry);
  }
  free(manager->port_owner);
  *manager = (struct eswitch_manager){0};
}
