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
#include "eswitch_state.h"

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
  int port_index;
  doca_error_t result;

  if (vswitch == NULL)
    return DOCA_ERROR_NOT_FOUND;
  port_index = find_port_index(manager, port_id);
  if (port_index < 0)
    return DOCA_ERROR_NOT_FOUND;
  if (manager->port_owner[port_index] != 0)
    return DOCA_ERROR_IN_USE;

  /* Prepare the egress membership before opening ingress.  Until the
   * classifier is committed, packets cannot enter this vSwitch from port_id. */
  result = eswitch_pipeline_flood_add_port(manager->pipeline, vswitch_id,
                                           port_id, &vswitch->flood);
  if (result != DOCA_SUCCESS)
    return result;
  result = eswitch_pipeline_attach_port(manager->pipeline,
                                        (uint16_t)port_index, vswitch_id);
  if (result != DOCA_SUCCESS) {
    doca_error_t cleanup = eswitch_pipeline_flood_remove_port(
        manager->pipeline, port_id, &vswitch->flood);
    if (cleanup == DOCA_SUCCESS && vswitch->flood.member_count == 0)
      cleanup = eswitch_pipeline_destroy_flood_group(manager->pipeline,
                                                      &vswitch->flood);
    return cleanup == DOCA_SUCCESS ? result : cleanup;
  }
  manager->port_owner[port_index] = vswitch_id;
  printf("VSWITCH ATTACH: vs=%u dpdk-port=%u\n", vswitch_id, port_id);
  return DOCA_SUCCESS;
}

static doca_error_t detach_port(struct eswitch_manager *manager,
                                uint16_t vswitch_id, uint16_t port_id) {
  struct managed_vswitch *vswitch = find_vswitch(manager, vswitch_id);
  int port_index;
  doca_error_t result;

  if (vswitch == NULL)
    return DOCA_ERROR_NOT_FOUND;
  port_index = find_port_index(manager, port_id);
  if (port_index < 0)
    return DOCA_ERROR_NOT_FOUND;
  if (manager->port_owner[port_index] != vswitch_id)
    return DOCA_ERROR_INVALID_VALUE;

  /* Close ingress first. If a later hardware mutation fails, restore the
   * classifier and flood member while ownership is still unchanged. */
  result = eswitch_pipeline_detach_port(manager->pipeline,
                                        (uint16_t)port_index);
  if (result != DOCA_SUCCESS)
    return result;
  result = eswitch_pipeline_flood_remove_port(manager->pipeline, port_id,
                                              &vswitch->flood);
  if (result != DOCA_SUCCESS) {
    doca_error_t rollback = eswitch_pipeline_attach_port(
        manager->pipeline, (uint16_t)port_index, vswitch_id);
    return rollback == DOCA_SUCCESS ? result : rollback;
  }
  result = eswitch_fdb_flush_port(&manager->fdb, vswitch_id, port_id,
                                  "port-detach");
  if (result != DOCA_SUCCESS) {
    doca_error_t rollback = eswitch_pipeline_flood_add_port(
        manager->pipeline, vswitch_id, port_id, &vswitch->flood);
    if (rollback == DOCA_SUCCESS)
      rollback = eswitch_pipeline_attach_port(
          manager->pipeline, (uint16_t)port_index, vswitch_id);
    return rollback == DOCA_SUCCESS ? result : rollback;
  }
  manager->port_owner[port_index] = 0;
  printf("VSWITCH DETACH: vs=%u dpdk-port=%u\n", vswitch_id, port_id);
  return DOCA_SUCCESS;
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

static doca_error_t manager_to_state(const struct eswitch_manager *manager,
                                     struct eswitch_state *state) {
  doca_error_t result;

  result = eswitch_state_init(manager->ports->count, state);
  if (result != DOCA_SUCCESS)
    return result;
  for (size_t i = 0; i < ESWITCH_MAX_VSWITCHES; i++) {
    if (!manager->switches[i].exists)
      continue;
    result = eswitch_state_add_switch(state, manager->switches[i].id);
    if (result != DOCA_SUCCESS)
      return result;
  }
  for (uint16_t i = 0; i < manager->ports->count; i++) {
    const struct ethernet_port *port = manager->ports->items[i].ethernet;
    struct eswitch_state_member member = {0};

    if (manager->port_owner[i] == 0)
      continue;
    member.vswitch_id = manager->port_owner[i];
    if (port->role == ETHERNET_PORT_ROLE_PARENT) {
      member.kind = ESWITCH_STATE_PORT_PARENT;
    } else {
      member.kind = ESWITCH_STATE_PORT_REPRESENTOR;
      member.host_index = port->host_index;
      member.pf_index = port->pf_index;
      member.vf_index = port->vf_index;
    }
    result = eswitch_state_add_member(state, &member);
    if (result != DOCA_SUCCESS)
      return result;
  }
  return DOCA_SUCCESS;
}

static doca_error_t persist_manager(const struct eswitch_manager *manager) {
  struct eswitch_state state = {0};
  doca_error_t result;

  result = manager_to_state(manager, &state);
  if (result == DOCA_SUCCESS)
    result = eswitch_state_save(manager->state_path, &state);
  eswitch_state_destroy(&state);
  if (result == DOCA_SUCCESS)
    printf("CONFIG SAVE: path=%s\n", manager->state_path);
  return result;
}

static int find_state_member_port(const struct eswitch_manager *manager,
                                  const struct eswitch_state_member *member) {
  for (uint16_t i = 0; i < manager->ports->count; i++) {
    const struct ethernet_port *port = manager->ports->items[i].ethernet;

    if (member->kind == ESWITCH_STATE_PORT_PARENT &&
        port->role == ETHERNET_PORT_ROLE_PARENT)
      return i;
    if (member->kind == ESWITCH_STATE_PORT_REPRESENTOR &&
        port->role == ETHERNET_PORT_ROLE_REPRESENTOR &&
        port->host_index == member->host_index &&
        port->pf_index == member->pf_index &&
        port->vf_index == member->vf_index)
      return i;
  }
  return -1;
}

static doca_error_t restore_manager(struct eswitch_manager *manager) {
  struct eswitch_state state = {0};
  bool exists = false;
  doca_error_t result;

  result = eswitch_state_init(ESWITCH_MAX_PERSISTED_MEMBERS, &state);
  if (result != DOCA_SUCCESS)
    return result;
  result = eswitch_state_load(manager->state_path, &state, &exists);
  if (result != DOCA_SUCCESS)
    goto out;
  if (!exists) {
    result = persist_manager(manager);
    goto out;
  }
  for (size_t i = 0; i < state.switch_count; i++) {
    result = create_vswitch(manager, state.switch_ids[i]);
    if (result != DOCA_SUCCESS)
      goto out;
  }
  for (size_t i = 0; i < state.member_count; i++) {
    int port_index = find_state_member_port(manager, &state.members[i]);

    if (port_index < 0) {
      const struct eswitch_state_member *member = &state.members[i];
      if (member->kind == ESWITCH_STATE_PORT_PARENT) {
        fprintf(stderr, "Configured parent port is not available\n");
      } else {
        fprintf(stderr,
                "Configured representor is not available: "
                "host=%u pf=%u vf=%u\n",
                member->host_index, member->pf_index, member->vf_index);
      }
      result = DOCA_ERROR_NOT_FOUND;
      goto out;
    }
    result = attach_port(
        manager, state.members[i].vswitch_id,
        manager->ports->items[port_index].ethernet->port_id);
    if (result != DOCA_SUCCESS)
      goto out;
  }
  printf("CONFIG RESTORE: path=%s vswitches=%zu members=%zu\n",
         manager->state_path, state.switch_count, state.member_count);

out:
  eswitch_state_destroy(&state);
  return result;
}

static doca_error_t create_vswitch_persisted(struct eswitch_manager *manager,
                                             uint16_t id) {
  doca_error_t result = create_vswitch(manager, id);

  if (result == DOCA_SUCCESS) {
    doca_error_t save_result = persist_manager(manager);
    if (save_result != DOCA_SUCCESS) {
      doca_error_t rollback = delete_vswitch(manager, id);
      return rollback == DOCA_SUCCESS ? save_result : rollback;
    }
  }
  return result;
}

static doca_error_t attach_port_persisted(struct eswitch_manager *manager,
                                          uint16_t vswitch_id,
                                          uint16_t port_id) {
  doca_error_t result = attach_port(manager, vswitch_id, port_id);

  if (result == DOCA_SUCCESS) {
    doca_error_t save_result = persist_manager(manager);
    if (save_result != DOCA_SUCCESS) {
      doca_error_t rollback = detach_port(manager, vswitch_id, port_id);
      return rollback == DOCA_SUCCESS ? save_result : rollback;
    }
  }
  return result;
}

static doca_error_t detach_port_persisted(struct eswitch_manager *manager,
                                          uint16_t vswitch_id,
                                          uint16_t port_id) {
  doca_error_t result = detach_port(manager, vswitch_id, port_id);

  if (result == DOCA_SUCCESS) {
    doca_error_t save_result = persist_manager(manager);
    if (save_result != DOCA_SUCCESS) {
      doca_error_t rollback = attach_port(manager, vswitch_id, port_id);
      return rollback == DOCA_SUCCESS ? save_result : rollback;
    }
  }
  return result;
}

static doca_error_t delete_vswitch_persisted(struct eswitch_manager *manager,
                                             uint16_t id) {
  uint16_t *member_ports;
  uint16_t member_count = 0;
  doca_error_t result;

  if (find_vswitch(manager, id) == NULL)
    return DOCA_ERROR_NOT_FOUND;
  member_ports = calloc(manager->ports->count, sizeof(*member_ports));
  if (member_ports == NULL)
    return DOCA_ERROR_NO_MEMORY;
  for (uint16_t i = 0; i < manager->ports->count; i++) {
    if (manager->port_owner[i] == id)
      member_ports[member_count++] =
          manager->ports->items[i].ethernet->port_id;
  }

  result = delete_vswitch(manager, id);
  if (result == DOCA_SUCCESS) {
    doca_error_t save_result = persist_manager(manager);
    if (save_result != DOCA_SUCCESS) {
      doca_error_t rollback = create_vswitch(manager, id);
      for (uint16_t i = 0; rollback == DOCA_SUCCESS && i < member_count; i++)
        rollback = attach_port(manager, id, member_ports[i]);
      result = rollback == DOCA_SUCCESS ? save_result : rollback;
    }
  }
  free(member_ports);
  return result;
}

doca_error_t eswitch_manager_init(struct dpdk_io *io,
                                  struct switch_flow_ports *ports,
                                  struct eswitch_pipeline *pipeline,
                                  const char *state_path,
                                  struct eswitch_manager *manager) {
  doca_error_t result;

  if (io == NULL || ports == NULL || pipeline == NULL || state_path == NULL ||
      *state_path == '\0' || manager == NULL ||
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
  if (snprintf(manager->state_path, sizeof(manager->state_path), "%s",
               state_path) >= (int)sizeof(manager->state_path)) {
    free(manager->port_owner);
    manager->port_owner = NULL;
    return DOCA_ERROR_TOO_BIG;
  }
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
  result = restore_manager(manager);
  if (result != DOCA_SUCCESS)
    fprintf(stderr, "Failed to restore eSwitch configuration %s: %s\n",
            manager->state_path, doca_error_get_descr(result));
  return result;
}

static doca_error_t process_packet(struct eswitch_manager *manager,
                                   struct rte_mbuf *packet,
                                   uint64_t now_ns) {
  struct rte_ether_hdr header_copy;
  const struct rte_ether_hdr *header;
  uint16_t vswitch_id;
  uint16_t port_id;
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

  result = eswitch_fdb_learn(&manager->fdb, vswitch_id, 0,
                             &header->src_addr, port_id, now_ns);
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

static bool parse_id_arguments(char **arguments, size_t count,
                               bool required, uint16_t *id) {
  if (count == 0) {
    if (required)
      return false;
    *id = 0;
    return true;
  }
  if (count == 1)
    return parse_u16(arguments[0], id); /* Legacy positional form. */
  return count == 2 && strcmp(arguments[0], "--id") == 0 &&
         parse_u16(arguments[1], id);
}

static bool parse_port_arguments(char **arguments, size_t count,
                                 uint16_t *id, uint16_t *port_id) {
  bool found_id = false;
  bool found_port = false;

  if (count == 2)
    return parse_u16(arguments[0], id) &&
           parse_u16(arguments[1], port_id); /* Legacy positional form. */
  if (count != 4)
    return false;
  for (size_t i = 0; i < count; i += 2) {
    if (strcmp(arguments[i], "--id") == 0 && !found_id) {
      found_id = parse_u16(arguments[i + 1], id);
      if (!found_id)
        return false;
    } else if (strcmp(arguments[i], "--port") == 0 && !found_port) {
      found_port = parse_u16(arguments[i + 1], port_id);
      if (!found_port)
        return false;
    } else {
      return false;
    }
  }
  return found_id && found_port;
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
  used = append_text(response, size, used, "config=%s\n",
                     manager->state_path);
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
  char *arguments[8];
  size_t argument_count = 0;
  uint16_t first;
  uint16_t second;
  doca_error_t result = DOCA_SUCCESS;
  bool syntax_error = false;

  if (request == NULL || response == NULL || response_size == 0 ||
      manager == NULL || !manager->initialized)
    return DOCA_ERROR_INVALID_VALUE;
  snprintf(command, sizeof(command), "%s", request);
  command[strcspn(command, "\r\n")] = '\0';
  verb = strtok_r(command, " \t", &save);
  while (argument_count < sizeof(arguments) / sizeof(arguments[0])) {
    char *argument = strtok_r(NULL, " \t", &save);
    if (argument == NULL)
      break;
    arguments[argument_count++] = argument;
  }
  if (strtok_r(NULL, " \t", &save) != NULL)
    verb = NULL;
  if (verb == NULL) {
    result = DOCA_ERROR_INVALID_VALUE;
  } else if (strcmp(verb, "status") == 0 && argument_count == 0) {
    (void)format_status(manager, response, response_size);
    return DOCA_SUCCESS;
  } else if (strcmp(verb, "vs-list") == 0 && argument_count == 0) {
    (void)format_vswitches(manager, response, response_size);
    return DOCA_SUCCESS;
  } else if (strcmp(verb, "list-port-available") == 0 &&
             argument_count == 0) {
    (void)format_available_ports(manager, response, response_size);
    return DOCA_SUCCESS;
  } else if (strcmp(verb, "show-fdb") == 0 &&
             parse_id_arguments(arguments, argument_count, false, &first)) {
    snprintf(response, response_size, "OK\n");
    eswitch_fdb_format(&manager->fdb, first,
                       response + strlen(response),
                       response_size - strlen(response));
    return DOCA_SUCCESS;
  } else if (strcmp(verb, "vs-create") == 0 &&
             parse_id_arguments(arguments, argument_count, true, &first)) {
    result = create_vswitch_persisted(manager, first);
  } else if (strcmp(verb, "vs-delete") == 0 &&
             parse_id_arguments(arguments, argument_count, true, &first)) {
    result = delete_vswitch_persisted(manager, first);
  } else if (strcmp(verb, "vs-port-attach") == 0 &&
             parse_port_arguments(arguments, argument_count, &first,
                                  &second)) {
    result = attach_port_persisted(manager, first, second);
  } else if (strcmp(verb, "vs-port-detach") == 0 &&
             parse_port_arguments(arguments, argument_count, &first,
                                  &second)) {
    result = detach_port_persisted(manager, first, second);
  } else {
    result = DOCA_ERROR_INVALID_VALUE;
    syntax_error = true;
  }

  if (result == DOCA_SUCCESS) {
    snprintf(response, response_size, "OK\n");
    return DOCA_SUCCESS;
  }
  snprintf(response, response_size, "ERR code=%d message=%s\n", result,
           doca_error_get_descr(result));
  if (syntax_error) {
    size_t used = strlen(response);

    if (verb != NULL && strcmp(verb, "vs-create") == 0)
      append_text(response, response_size, used,
                  "Usage: vs-create --id <id>\n");
    else if (verb != NULL && strcmp(verb, "vs-delete") == 0)
      append_text(response, response_size, used,
                  "Usage: vs-delete --id <id>\n");
    else if (verb != NULL && strcmp(verb, "vs-port-attach") == 0)
      append_text(response, response_size, used,
                  "Usage: vs-port-attach --id <id> --port <port-id>\n");
    else if (verb != NULL && strcmp(verb, "vs-port-detach") == 0)
      append_text(response, response_size, used,
                  "Usage: vs-port-detach --id <id> --port <port-id>\n");
    else if (verb != NULL && strcmp(verb, "show-fdb") == 0)
      append_text(response, response_size, used,
                  "Usage: show-fdb [--id <id>]\n");
    else
      append_text(response, response_size, used,
                  "Run: eswitchctl --help\n");
  }
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
    if (group->pipe != NULL)
      doca_flow_pipe_destroy(group->pipe);
    free(group->members);
  }
  while (manager->fdb.head != NULL) {
    struct eswitch_fdb_entry *entry = manager->fdb.head;
    manager->fdb.head = entry->next;
    free(entry);
  }
  free(manager->port_owner);
  *manager = (struct eswitch_manager){0};
}
