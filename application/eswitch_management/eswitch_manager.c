#include "eswitch_manager.h"

#include <errno.h>
#include <inttypes.h>
#include <netinet/in.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <rte_byteorder.h>
#include <rte_ethdev.h>
#include <rte_flow.h>
#include <rte_mbuf.h>
#include <rte_version.h>

#include "../ethernet_switch/switch_config.h"
#include "cli/eswitch_cli.h"
#include "eswitch_state.h"
#include "router/router_control.h"
#include "router/router_hw.h"
#include "router/router_arp.h"
#include "router/router_forward.h"
#include "router/router_icmp.h"
#include "l2/l2_switch.h"
#include "pipeline/tx_build.h"

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

static const struct router_interface *find_router_interface(
    const struct eswitch_manager *manager, uint16_t vr_id,
    uint16_t interface_id) {
  if (manager->router == NULL)
    return NULL;
  for (size_t i = 0; i < manager->router->interface_count; i++) {
    const struct router_interface *rif = &manager->router->interfaces[i];

    if (rif->vr_id == vr_id && rif->interface_id == interface_id)
      return rif;
  }
  return NULL;
}

static const struct router_interface *find_router_vswitch_interface(
    const struct eswitch_manager *manager, uint16_t vswitch_id,
    const uint8_t rif_mac[6]) {
  if (manager == NULL || manager->router == NULL || rif_mac == NULL)
    return NULL;
  for (size_t i = 0; i < manager->router->interface_count; i++) {
    const struct router_interface *candidate = &manager->router->interfaces[i];

    if (candidate->attachment == ROUTER_VSWITCH && candidate->has_address &&
        candidate->vswitch_id == vswitch_id &&
        memcmp(candidate->mac, rif_mac, 6) == 0)
      return candidate;
  }
  return NULL;
}

static const struct router_interface *find_router_port_interface(
    const struct eswitch_manager *manager, uint16_t port_id, uint16_t vr_id) {
  int index = find_port_index(manager, port_id);
  const struct ethernet_port *port;

  if (index < 0 || manager->router == NULL)
    return NULL;
  port = manager->ports->items[index].ethernet;
  for (size_t i = 0; i < manager->router->interface_count; i++) {
    const struct router_interface *rif = &manager->router->interfaces[i];

    if (rif->attachment == ROUTER_PORT && rif->vr_id == vr_id &&
        rif->port.host == port->host_index && rif->port.pf == port->pf_index &&
        rif->port.vf == port->vf_index)
      return rif;
  }
  return NULL;
}

static int find_router_interface_port_index(
    const struct eswitch_manager *manager, const struct router_interface *rif) {
  if (manager == NULL || rif == NULL || rif->attachment != ROUTER_PORT)
    return -1;
  for (uint16_t i = 0; i < manager->ports->count; i++) {
    const struct ethernet_port *port = manager->ports->items[i].ethernet;

    if (port->role == ETHERNET_PORT_ROLE_REPRESENTOR &&
        port->host_index == rif->port.host && port->pf_index == rif->port.pf &&
        port->vf_index == rif->port.vf)
      return i;
  }
  return -1;
}

/* Keep the hardware return context synchronized with the desired RIF MAC.
 * A MAC edit is persisted by router_control first; the next gateway packet
 * performs an idempotent bind or replaces the previous entry for this VS. */
static doca_error_t bind_sf_return_context(struct eswitch_manager *manager,
                                           uint16_t vswitch_id,
                                           const uint8_t rif_mac[6],
                                           uint16_t *context_tag) {
  const struct router_interface *rif = find_router_vswitch_interface(
      manager, vswitch_id, rif_mac);
  if (rif == NULL)
    return DOCA_ERROR_NOT_FOUND;
  doca_error_t result = eswitch_pipeline_sf_bind_vswitch(
      manager->pipeline, rif->vr_id, rif->interface_id, vswitch_id,
      rif->address, rif_mac, context_tag);

  if (result != DOCA_ERROR_BAD_STATE)
    return result;
  result = eswitch_pipeline_sf_unbind_rif(manager->pipeline,
                                          rif->interface_id);
  if (result != DOCA_SUCCESS)
    return result;
  result = eswitch_pipeline_sf_bind_vswitch(
      manager->pipeline, rif->vr_id, rif->interface_id, vswitch_id,
      rif->address, rif_mac, context_tag);
  if (result == DOCA_SUCCESS)
    printf("SF RETURN REBIND: vs=%u context-vlan=%u\n", vswitch_id,
           *context_tag);
  return result;
}

/* Directed router output already has a resolved destination port. Keep one
 * flood context per RIF for local delivery/broadcast probes, then install a
 * separate per-RIF context that jumps directly to the target egress gate. */
static doca_error_t bind_sf_directed_context(
    struct eswitch_manager *manager, uint16_t vswitch_id,
    uint16_t target_port_id, const uint8_t rif_mac[6],
    uint16_t *context_tag) {
  uint16_t flood_context_tag;
  doca_error_t result;

  result = bind_sf_return_context(manager, vswitch_id, rif_mac,
                                  &flood_context_tag);
  if (result != DOCA_SUCCESS)
    return result;
  const struct router_interface *rif = find_router_vswitch_interface(
      manager, vswitch_id, rif_mac);
  if (rif == NULL)
    return DOCA_ERROR_NOT_FOUND;
  return eswitch_pipeline_sf_bind_egress(
      manager->pipeline, rif->vr_id, rif->interface_id, vswitch_id,
      target_port_id, rif_mac, context_tag);
}


static doca_error_t manager_to_state(const struct eswitch_manager *manager,
                                     struct eswitch_state *state) {
  doca_error_t result;

  result = eswitch_state_init(ESWITCH_MAX_PERSISTED_MEMBERS, state);
  if (result != DOCA_SUCCESS)
    return result;
  for (size_t i = 0; i < ESWITCH_MAX_VSWITCHES; i++) {
    if (!manager->switches[i].exists)
      continue;
    result = eswitch_state_add_switch(state, manager->switches[i].id);
    if (result != DOCA_SUCCESS)
      return result;
  }
  for (size_t i = 0; i < ESWITCH_MAX_PERSISTED_MEMBERS; i++) {
    const struct eswitch_port_membership *configured =
        &manager->memberships[i];
    const struct ethernet_port *port;
    struct eswitch_state_member member = {0};

    if (!configured->active)
      continue;
    port = manager->ports->items[configured->port_index].ethernet;
    member.vswitch_id = configured->vswitch_id;
    member.mode = configured->mode;
    member.vlan_id = configured->vlan_id;
    member.vlan_last = configured->vlan_last;
    member.vlan_extra_id = configured->vlan_extra_id;
    member.vlan_extra_last = configured->vlan_extra_last;
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
        manager->ports->items[port_index].ethernet->port_id,
        state.members[i].mode, state.members[i].vlan_id,
        state.members[i].vlan_last, state.members[i].vlan_extra_id,
        state.members[i].vlan_extra_last);
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
                                          uint16_t port_id,
                                          enum eswitch_port_mode mode,
                                          uint16_t vlan_id,
                                          uint16_t vlan_last,
                                          uint16_t vlan_extra_id,
                                          uint16_t vlan_extra_last) {
  doca_error_t result = attach_port(manager, vswitch_id, port_id, mode,
                                    vlan_id, vlan_last, vlan_extra_id,
                                    vlan_extra_last);

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
  enum eswitch_port_mode old_mode = ESWITCH_PORT_MODE_ACCESS;
  uint16_t old_vlan = 0;
  uint16_t old_vlan_last = 0;
  uint16_t old_vlan_extra = 0;
  uint16_t old_vlan_extra_last = 0;
  int port_index;
  doca_error_t result;

  /* Validate first so an invalid CLI request cannot tear down live NAT
   * sessions. The dataplane mutation below repeats these checks. */
  if (find_vswitch(manager, vswitch_id) == NULL)
    return DOCA_ERROR_NOT_FOUND;
  port_index = find_port_index(manager, port_id);
  if (port_index < 0)
    return DOCA_ERROR_NOT_FOUND;
  if (!eswitch_port_in_vswitch(manager, vswitch_id, port_id))
    return DOCA_ERROR_INVALID_VALUE;
  for (size_t i = 0; i < ESWITCH_MAX_PERSISTED_MEMBERS; i++) {
    const struct eswitch_port_membership *member = &manager->memberships[i];
    if (member->active && member->vswitch_id == vswitch_id &&
        member->port_id == port_id) {
      old_mode = member->mode;
      old_vlan = member->vlan_id;
      old_vlan_last = member->vlan_last;
      old_vlan_extra = member->vlan_extra_id;
      old_vlan_extra_last = member->vlan_extra_last;
      break;
    }
  }

  result = eswitch_pipeline_ct_flush(manager->pipeline, 0);

  if (result == DOCA_SUCCESS) {
    router_nat_flush(manager->nat, 0);
    result = detach_port(manager, vswitch_id, port_id);
  }

  if (result == DOCA_SUCCESS) {
    doca_error_t save_result = persist_manager(manager);
    if (save_result != DOCA_SUCCESS) {
      doca_error_t rollback = attach_port(manager, vswitch_id, port_id,
                                          old_mode, old_vlan, old_vlan_last,
                                          old_vlan_extra,
                                          old_vlan_extra_last);
      return rollback == DOCA_SUCCESS ? save_result : rollback;
    }
  }
  return result;
}

static doca_error_t delete_vswitch_persisted(struct eswitch_manager *manager,
                                             uint16_t id) {
  struct eswitch_port_membership *members;
  uint16_t member_count = 0;
  doca_error_t result;

  if (find_vswitch(manager, id) == NULL)
    return DOCA_ERROR_NOT_FOUND;
  members = calloc(ESWITCH_MAX_VLAN_MEMBERSHIPS, sizeof(*members));
  if (members == NULL)
    return DOCA_ERROR_NO_MEMORY;
  for (size_t i = 0; i < ESWITCH_MAX_PERSISTED_MEMBERS; i++) {
    if (manager->memberships[i].active &&
        manager->memberships[i].vswitch_id == id)
      members[member_count++] = manager->memberships[i];
  }

  result = delete_vswitch(manager, id);
  if (result == DOCA_SUCCESS) {
    doca_error_t save_result = persist_manager(manager);
    if (save_result != DOCA_SUCCESS) {
      doca_error_t rollback = create_vswitch(manager, id);
      for (uint16_t i = 0; rollback == DOCA_SUCCESS && i < member_count; i++)
        rollback = attach_port(manager, id, members[i].port_id,
                               members[i].mode, members[i].vlan_id,
                               members[i].vlan_last,
                               members[i].vlan_extra_id,
                               members[i].vlan_extra_last);
      result = rollback == DOCA_SUCCESS ? save_result : rollback;
    }
  }
  free(members);
  return result;
}

doca_error_t eswitch_manager_init(struct dpdk_io *io,
                                  struct switch_flow_ports *ports,
                                  struct eswitch_pipeline *pipeline,
                                  struct sf_packet_io *sf_io,
                                  bool hardware_ct_supported,
                                  bool packet_debug,
                                  const char *state_path,
                                  struct eswitch_manager *manager) {
  doca_error_t result;

  if (io == NULL || ports == NULL || pipeline == NULL || sf_io == NULL ||
      state_path == NULL ||
      *state_path == '\0' || manager == NULL ||
      !io->port_started || !ports->started || !pipeline->created ||
      !sf_io->started)
    return DOCA_ERROR_INVALID_VALUE;
  if (!rte_flow_dynf_metadata_avail())
    return DOCA_ERROR_NOT_SUPPORTED;
  manager->port_owner = calloc(ports->count, sizeof(*manager->port_owner));
  if (manager->port_owner == NULL)
    return DOCA_ERROR_NO_MEMORY;
  manager->nat = calloc(1, sizeof(*manager->nat));
  if (manager->nat == NULL) {
    free(manager->port_owner);
    manager->port_owner = NULL;
    return DOCA_ERROR_NO_MEMORY;
  }
  router_nat_init(manager->nat);
  manager->io = io;
  manager->ports = ports;
  manager->pipeline = pipeline;
  manager->sf_io = sf_io;
  manager->hardware_ct_supported = hardware_ct_supported;
  manager->packet_debug = packet_debug;
  if (snprintf(manager->state_path, sizeof(manager->state_path), "%s",
               state_path) >= (int)sizeof(manager->state_path)) {
    free(manager->port_owner);
    manager->port_owner = NULL;
    free(manager->nat);
    manager->nat = NULL;
    return DOCA_ERROR_TOO_BIG;
  }
  result = eswitch_fdb_init(pipeline, SWITCH_MAX_FDB_ENTRIES,
                            SWITCH_FDB_AGING_SECONDS, &manager->fdb);
  if (result != DOCA_SUCCESS) {
    free(manager->port_owner);
    manager->port_owner = NULL;
    free(manager->nat);
    manager->nat = NULL;
    return result;
  }
  manager->started_ns = monotonic_ns();
  manager->next_aging_ns = manager->started_ns +
      (uint64_t)SWITCH_AGING_SCAN_SECONDS * 1000000000ULL;
  manager->initialized = true;
  result = restore_manager(manager);
  if (result == DOCA_SUCCESS)
    result = router_control_restore(manager);
  if (result == DOCA_SUCCESS)
    result = eswitch_manager_hw_routes_sync(manager, manager->router);
  if (result != DOCA_SUCCESS)
    fprintf(stderr, "Failed to restore eSwitch configuration %s: %s\n",
            manager->state_path, doca_error_get_descr(result));
  return result;
}

doca_error_t eswitch_manager_hw_routes_sync(
    struct eswitch_manager *manager, const struct router_config *config) {
  struct router_hw_route *routes;
  size_t route_count;
  doca_error_t result;
  uint64_t now_ns;

  if (manager == NULL || config == NULL || manager->pipeline == NULL)
    return DOCA_ERROR_INVALID_VALUE;
  if (!manager->pipeline->hardware_routing_enabled)
    return DOCA_SUCCESS;
  now_ns = monotonic_ns();
  /* Runtime packet/maintenance syncs use the active config and may be
   * throttled after an HWS resource failure. Candidate configurations from
   * the control plane still bypass this gate and validate immediately. */
  if (config == manager->router &&
      manager->pipeline->hardware_routing_degraded &&
      now_ns != 0 && now_ns < manager->next_hw_route_retry_ns)
    return DOCA_SUCCESS;
  routes = calloc(manager->pipeline->hw_route_capacity, sizeof(*routes));
  if (routes == NULL)
    return DOCA_ERROR_NO_MEMORY;
  route_count = router_hw_routes_build(config, &manager->neighbors, now_ns,
                                       routes,
                                       manager->pipeline->hw_route_capacity);
  /* A neighbor is useful only while its VF remains owned by the expected VS.
   * This closes the stale-adjacency window across port moves. */
  for (size_t i = 0; i < route_count;) {
    int index = find_port_index(manager, routes[i].target_port_id);
    if (index >= 0 && eswitch_port_in_vswitch(
                          manager, routes[i].egress_vswitch_id,
                          routes[i].target_port_id)) {
      i++;
      continue;
    }
    routes[i] = routes[--route_count];
  }
  result = eswitch_pipeline_hw_routes_sync(manager->pipeline, routes,
                                            route_count);
  free(routes);
  if (result == DOCA_SUCCESS) {
    manager->next_hw_route_retry_ns = 0;
    manager->hw_route_retry_backoff_ns = 0;
  } else if (config == manager->router && now_ns != 0) {
    if (manager->hw_route_retry_backoff_ns == 0)
      manager->hw_route_retry_backoff_ns = ESWITCH_HW_RETRY_INITIAL_NS;
    else if (manager->hw_route_retry_backoff_ns < ESWITCH_HW_RETRY_MAX_NS / 2)
      manager->hw_route_retry_backoff_ns *= 2;
    else
      manager->hw_route_retry_backoff_ns = ESWITCH_HW_RETRY_MAX_NS;
    manager->next_hw_route_retry_ns = now_ns +
                                      manager->hw_route_retry_backoff_ns;
  }
  return result;
}

static doca_error_t reply_gateway_arp(struct eswitch_manager *manager,
                                      struct rte_mbuf *request, uint16_t vs,
                                      uint16_t ingress, uint64_t now_ns) {
  uint8_t scratch[42], response[60];
  uint16_t context_tag = 0;
  bool debug = manager->packet_debug &&
               (manager->arp_seen < 3 ||
                now_ns - manager->tx_log_ns >= 1000000000ULL);
  if (manager->arp_seen == manager->tx_snapshot_seen)
    manager->tx_snapshot_ns = now_ns;
  manager->arp_seen++;
  if (debug)
    manager->tx_log_ns = now_ns;
  const uint8_t *bytes = rte_pktmbuf_read(request, 0, sizeof(scratch), scratch);
  if (!bytes || !router_arp_reply(manager->router, vs, bytes, sizeof(scratch),
                                   response, sizeof(response))) {
    manager->arp_ignored++;
    if (debug)
      printf("ARP TX SKIP: vs=%u port=%u reason=%s rx_len=%u\n", vs, ingress,
             bytes ? "not-local-gateway-request-or-invalid-arp" : "truncated-frame",
             rte_pktmbuf_pkt_len(request));
    return DOCA_ERROR_NOT_FOUND;
  }
  manager->arp_built++;
  int index = find_port_index(manager, ingress);
  if (index < 0 || ingress == UINT16_MAX ||
      !eswitch_port_in_vswitch(manager, vs, ingress) ||
      manager->sf_io == NULL || !manager->sf_io->started) {
    manager->arp_target_drops++;
    manager->arp_tx_drops++;
    if (debug)
      printf("ARP TX DROP: stage=target-validation vs=%u port=%u index=%d\n",
             vs, ingress, index);
    return DOCA_ERROR_INVALID_VALUE;
  }
  if (now_ns - manager->arp_window_ns >= 1000000000ULL) {
    manager->arp_window_ns = now_ns;
    manager->arp_window_replies = 0;
  }
  if (manager->arp_window_replies >= 100) {
    manager->arp_rate_drops++;
    if (debug) printf("ARP TX DROP: stage=rate-limit limit=100/s\n");
    return DOCA_ERROR_IN_USE;
  }
  manager->arp_window_replies++;
  doca_error_t arm_result = bind_sf_directed_context(
      manager, vs, ingress, response + 6, &context_tag);
  if (arm_result != DOCA_SUCCESS) {
    manager->arp_target_drops++; manager->arp_tx_drops++;
    fprintf(stderr,
            "ARP SF RETURN BIND FAILED: vs=%u port=%u error=%s\n",
            vs, ingress, doca_error_get_descr(arm_result));
    return arm_result;
  }
  if (debug) {
    const struct ethernet_port *target = manager->ports->items[index].ethernet;
    printf("ARP SF TX BUILD: vs=%u iface=%s sf-port=%u target=%u "
           "host=%u pf=%u vf=%u context-vlan=%u wire-src="
           "%02x:%02x:%02x:%02x:%02x:%02x guest-src="
           "%02x:%02x:%02x:%02x:%02x:%02x len=%zu\n",
           vs, manager->sf_io->interface_name, manager->pipeline->sf_port_id,
           ingress, target->host_index, target->pf_index, target->vf_index,
           context_tag, manager->sf_io->mac[0], manager->sf_io->mac[1],
           manager->sf_io->mac[2], manager->sf_io->mac[3],
           manager->sf_io->mac[4], manager->sf_io->mac[5], response[6],
           response[7], response[8], response[9], response[10], response[11],
           sizeof(response));
    printf("ARP GUEST FRAME:");
    for (size_t i = 0; i < sizeof(response); i++) printf(" %02x", response[i]);
    printf("\n");
  }
  if (sf_packet_io_send_context(manager->sf_io, response, sizeof(response),
                                context_tag) == DOCA_SUCCESS) {
    manager->arp_replies++;
    if (debug) printf("ARP SF TX SENT: vs=%u port=%u gateway=%u.%u.%u.%u\n", vs, ingress,
           response[28], response[29], response[30], response[31]);
    return DOCA_SUCCESS;
  } else {
    manager->arp_tx_drops++;
    manager->arp_sf_send_drops++;
    fprintf(stderr, "ARP TX DROP: stage=sf-send vs=%u port=%u iface=%s\n",
            vs, ingress, manager->sf_io->interface_name);
    return DOCA_ERROR_DRIVER;
  }
}

static doca_error_t bind_route_egress(
    struct eswitch_manager *manager, const struct router_interface *egress,
    uint16_t target_port_id, uint16_t *context_tag);

static void send_route_neighbor_probe(
    struct eswitch_manager *manager, const struct router_interface *egress,
    uint32_t target_ip, uint64_t now_ns) {
  uint8_t request[60];
  uint16_t context_tag = 0;
  doca_error_t result;

  if (!router_neighbor_should_probe(&manager->neighbors, egress->vr_id,
                                    egress->interface_id, target_ip, now_ns))
    return;
  if (router_arp_request(egress, target_ip, request, sizeof(request)) == 0) {
    manager->route_tx_drops++;
    return;
  }
  if (egress->attachment == ROUTER_VSWITCH) {
    result = bind_sf_return_context(manager, egress->vswitch_id, egress->mac,
                                    &context_tag);
  } else {
    int port_index = find_router_interface_port_index(manager, egress);

    if (port_index < 0) {
      manager->route_tx_drops++;
      return;
    }
    result = bind_route_egress(
        manager, egress,
        manager->ports->items[port_index].ethernet->port_id, &context_tag);
  }
  if (result == DOCA_SUCCESS)
    result = sf_packet_io_send_context(manager->sf_io, request,
                                       sizeof(request), context_tag);
  if (result != DOCA_SUCCESS) {
    manager->route_tx_drops++;
    fprintf(stderr, "ROUTE ARP PROBE DROP: vr=%u rif=%u error=%s\n",
            egress->vr_id, egress->interface_id,
            doca_error_get_descr(result));
    return;
  }
  manager->route_arp_probes++;
  if (manager->packet_debug)
    printf("ROUTE ARP PROBE: vr=%u rif=%u attachment=%s vs=%u "
           "target=%u.%u.%u.%u context-vlan=%u\n",
           egress->vr_id, egress->interface_id,
           egress->attachment == ROUTER_PORT ? "port" : "vs",
           egress->vswitch_id,
           (target_ip >> 24) & 0xffU, (target_ip >> 16) & 0xffU,
           (target_ip >> 8) & 0xffU, target_ip & 0xffU, context_tag);
}

static bool validate_route_neighbor(
    const struct eswitch_manager *manager,
    const struct router_interface *egress,
    const struct router_neighbor *neighbor) {
  int port_index;

  if (manager == NULL || egress == NULL || neighbor == NULL)
    return false;
  port_index = find_port_index(manager, neighbor->port_id);
  if (port_index < 0)
    return false;
  if (egress->attachment == ROUTER_VSWITCH)
    return eswitch_port_in_vswitch(manager, egress->vswitch_id,
                                   neighbor->port_id);

  return port_index == find_router_interface_port_index(manager, egress);
}

static doca_error_t bind_route_egress(
    struct eswitch_manager *manager, const struct router_interface *egress,
    uint16_t target_port_id, uint16_t *context_tag) {
  doca_error_t result;

  if (egress->attachment == ROUTER_VSWITCH)
    return bind_sf_directed_context(manager, egress->vswitch_id,
                                    target_port_id, egress->mac, context_tag);

  result = eswitch_pipeline_sf_bind_egress(
      manager->pipeline, egress->vr_id, egress->interface_id, egress->vr_id,
      target_port_id, egress->mac, context_tag);
  if (result != DOCA_ERROR_BAD_STATE)
    return result;
  result = eswitch_pipeline_sf_unbind_egress(
      manager->pipeline, egress->interface_id, egress->vr_id,
      target_port_id);
  if (result != DOCA_SUCCESS)
    return result;
  return eswitch_pipeline_sf_bind_egress(
      manager->pipeline, egress->vr_id, egress->interface_id, egress->vr_id,
      target_port_id, egress->mac, context_tag);
}

static void reply_uplink_arp(struct eswitch_manager *manager,
                             struct rte_mbuf *request,
                             const struct router_interface *ingress,
                             uint16_t ingress_port, uint64_t now_ns) {
  uint8_t scratch[42];
  uint8_t response[60];
  const uint8_t *bytes;
  uint16_t context_tag = 0;
  bool debug;
  doca_error_t result;

  debug = manager->arp_seen < 3 ||
          now_ns - manager->tx_log_ns >= 1000000000ULL;
  if (manager->arp_seen == manager->tx_snapshot_seen)
    manager->tx_snapshot_ns = now_ns;
  manager->arp_seen++;
  if (debug)
    manager->tx_log_ns = now_ns;

  bytes = rte_pktmbuf_read(request, 0, sizeof(scratch), scratch);
  if (bytes == NULL ||
      router_arp_reply_interface(manager->router, ingress->interface_id,
                                 bytes, sizeof(scratch), response,
                                 sizeof(response)) == 0) {
    manager->arp_ignored++;
    return;
  }
  manager->arp_built++;
  if (manager->sf_io == NULL || !manager->sf_io->started ||
      find_port_index(manager, ingress_port) !=
          find_router_interface_port_index(manager, ingress)) {
    manager->arp_target_drops++;
    manager->arp_tx_drops++;
    return;
  }
  if (now_ns - manager->arp_window_ns >= 1000000000ULL) {
    manager->arp_window_ns = now_ns;
    manager->arp_window_replies = 0;
  }
  if (manager->arp_window_replies >= 100) {
    manager->arp_rate_drops++;
    return;
  }
  manager->arp_window_replies++;

  result = bind_route_egress(manager, ingress, ingress_port, &context_tag);
  if (result == DOCA_SUCCESS)
    result = sf_packet_io_send_context(manager->sf_io, response,
                                       sizeof(response), context_tag);
  if (result != DOCA_SUCCESS) {
    manager->arp_sf_send_drops++;
    manager->arp_tx_drops++;
    fprintf(stderr,
            "UPLINK ARP TX DROP: vr=%u rif=%u port=%u error=%s\n",
            ingress->vr_id, ingress->interface_id, ingress_port,
            doca_error_get_descr(result));
    return;
  }
  manager->arp_replies++;
  if (debug && manager->packet_debug) {
    printf("UPLINK ARP TX: vr=%u rif=%u port=%u address=%u.%u.%u.%u "
           "context-vlan=%u\n",
           ingress->vr_id, ingress->interface_id, ingress_port,
           (ingress->address >> 24) & 0xffU,
           (ingress->address >> 16) & 0xffU,
           (ingress->address >> 8) & 0xffU, ingress->address & 0xffU,
           context_tag);
  }
}

/* A logical router link is deliberately an Arm-only adjacency in phase 1.
 * Each traversal performs a fresh LPM lookup in the peer VR and therefore
 * preserves router hop semantics (TTL is decremented once per VR). */
#define ROUTER_LINK_MAX_HOPS 8U

static void route_arm_frame(struct eswitch_manager *manager,
                            const uint8_t *frame, size_t length,
                            const struct router_interface *ingress,
                            uint16_t ingress_port, uint8_t link_hops,
                            uint16_t expected_egress_interface,
                            uint64_t now_ns);

static bool route_across_link(struct eswitch_manager *manager,
                              const uint8_t *frame, size_t length,
                              const struct router_interface *egress,
                              uint32_t next_hop_ip, uint8_t link_hops,
                              uint64_t now_ns) {
  const struct router_interface *peer;
  size_t capacity=length<60?60:length;
  uint8_t *output;
  size_t output_length;

  if(link_hops>=ROUTER_LINK_MAX_HOPS) {
    manager->route_ttl_expired++;
    manager->router_link_drops++;
    return false;
  }
  peer=router_link_peer(manager->router,egress->interface_id);
  if(peer==NULL || !egress->has_address || !peer->has_address ||
     next_hop_ip!=peer->address) {
    manager->route_neighbor_misses++;
    manager->router_link_drops++;
    return false;
  }
  output=malloc(capacity);
  if(output==NULL) {manager->route_tx_drops++;manager->router_link_drops++;return false;}
  output_length=router_ipv4_rewrite(frame,length,egress->mac,peer->mac,
                                    output,capacity);
  if(output_length==0) {
    manager->route_invalid++;
    manager->router_link_drops++;
    free(output);
    return false;
  }
  if(manager->packet_debug)
    printf("ROUTER LINK TX: link=%u from-vr=%u to-vr=%u next-hop="
           "%u.%u.%u.%u len=%zu\n",egress->link_id,egress->vr_id,
           peer->vr_id,(next_hop_ip>>24)&0xffU,(next_hop_ip>>16)&0xffU,
           (next_hop_ip>>8)&0xffU,next_hop_ip&0xffU,output_length);
  route_arm_frame(manager,output,output_length,peer,0,link_hops+1,0,now_ns);
  manager->router_link_forwards++;
  free(output);
  return true;
}

static void route_arm_local_reply(struct eswitch_manager *manager,
                                  const uint8_t *frame, size_t length,
                                  const struct router_interface *ingress,
                                  uint16_t ingress_port,uint8_t link_hops,
                                  uint64_t now_ns) {
  size_t capacity=length<60?60:length;
  uint8_t *response=malloc(capacity);
  size_t response_length;
  uint16_t context_tag=0;
  doca_error_t result;

  manager->icmp_seen++;
  if(response==NULL) {manager->icmp_tx_drops++;return;}
  response_length=router_icmp_echo_reply_interface(
      manager->router,ingress->interface_id,frame,length,response,capacity);
  if(response_length==0) {manager->icmp_ignored++;free(response);return;}
  manager->icmp_built++;
  if(ingress->attachment==ROUTER_LINK) {
    const struct router_interface *peer=router_link_peer(
        manager->router,ingress->interface_id);
    if(peer==NULL || link_hops>=ROUTER_LINK_MAX_HOPS) {
      manager->icmp_tx_drops++;
    } else {
      route_arm_frame(manager,response,response_length,peer,0,link_hops+1,0,
                      now_ns);
      manager->router_link_forwards++;
      manager->icmp_replies++;
    }
    free(response);
    return;
  }
  int ingress_index=find_port_index(manager,ingress_port);
  if(ingress->attachment!=ROUTER_VSWITCH || ingress_index<0 ||
     !eswitch_port_in_vswitch(manager,ingress->vswitch_id,ingress_port)) {
    manager->icmp_tx_drops++;
    free(response);
    return;
  }
  result=bind_sf_directed_context(manager,ingress->vswitch_id,ingress_port,
                                  ingress->mac,&context_tag);
  if(result==DOCA_SUCCESS)
    result=sf_packet_io_send_context(manager->sf_io,response,response_length,
                                     context_tag);
  if(result==DOCA_SUCCESS) manager->icmp_replies++;
  else manager->icmp_tx_drops++;
  free(response);
}

static void route_arm_frame(struct eswitch_manager *manager,
                            const uint8_t *frame, size_t length,
                            const struct router_interface *ingress,
                            uint16_t ingress_port, uint8_t link_hops,
                            uint16_t expected_egress_interface,
                            uint64_t now_ns) {
  struct router_ipv4_decision decision;
  const struct router_interface *egress;
  const struct router_neighbor *neighbor;
  const struct router_nat_policy *nat_policy = NULL;
  const struct router_nat_session *nat_session = NULL;
  struct router_nat_inside inside = {0};
  size_t capacity = length < 60 ? 60 : length;
  uint8_t *translated = NULL;
  uint8_t *output = NULL;
  const uint8_t *forward_frame;
  size_t output_length;
  uint16_t context_tag = 0;
  doca_error_t result;
  enum router_ipv4_disposition disposition;
  enum router_nat_result nat_result;

  disposition = router_ipv4_lookup_interface(manager->router,
      ingress->interface_id,frame,length,&decision);
  if (disposition == ROUTER_IPV4_NOT_FOR_ROUTER)
    goto out;
  if (disposition == ROUTER_IPV4_LOCAL) {
    route_arm_local_reply(manager,frame,length,ingress,ingress_port,
                          link_hops,now_ns);
    goto out;
  }
  manager->routed_seen++;
  if (disposition == ROUTER_IPV4_INVALID) {
    manager->route_invalid++;
    goto out;
  }
  if (disposition == ROUTER_IPV4_TTL_EXPIRED) {
    manager->route_ttl_expired++;
    goto out;
  }
  if (disposition == ROUTER_IPV4_NO_ROUTE) {
    manager->route_no_route++;
    goto out;
  }

  egress = find_router_interface(manager, decision.vr_id,
                                 decision.egress_interface_id);
  if (egress == NULL) {
    manager->route_no_route++;
    goto out;
  }
  if(expected_egress_interface!=0 &&
     decision.egress_interface_id!=expected_egress_interface) {
    manager->route_no_route++;
    goto out;
  }
  if(egress->attachment==ROUTER_LINK) {
    (void)route_across_link(manager,frame,length,egress,decision.next_hop_ip,
                            link_hops,now_ns);
    goto out;
  }
  neighbor = router_neighbor_lookup(
      &manager->neighbors, decision.vr_id, decision.egress_interface_id,
      decision.next_hop_ip, now_ns);
  if (neighbor == NULL) {
    manager->route_neighbor_misses++;
    send_route_neighbor_probe(manager, egress, decision.next_hop_ip, now_ns);
    goto out;
  }
  if (!validate_route_neighbor(manager, egress, neighbor)) {
    manager->route_neighbor_misses++;
    send_route_neighbor_probe(manager, egress, decision.next_hop_ip, now_ns);
    goto out;
  }

  forward_frame = frame;
  nat_policy = router_nat_policy_find(manager->router, decision.vr_id);
  if (egress->attachment == ROUTER_PORT ||
      (nat_policy != NULL &&
       nat_policy->interface_id == egress->interface_id)) {
    if (nat_policy == NULL || nat_policy->interface_id != egress->interface_id) {
      manager->route_tx_drops++;
      fprintf(stderr,
              "NAT OUT DROP: vr=%u egress-rif=%u reason=no-active-policy\n",
              decision.vr_id, decision.egress_interface_id);
      goto out;
    }
    translated = malloc(capacity);
    if (translated == NULL) {
      manager->route_tx_drops++;
      goto out;
    }
    inside.vswitch_id = ingress->attachment==ROUTER_VSWITCH ?
                        ingress->vswitch_id : 0;
    inside.interface_id = decision.ingress_interface_id;
    inside.port_id = ingress->attachment==ROUTER_VSWITCH ? ingress_port : 0;
    memcpy(inside.mac, frame + 6, sizeof(inside.mac));
    nat_result = router_nat_outbound(
        manager->nat, nat_policy,
        router_nat_policy_address(manager->router, nat_policy), &inside,
        frame, length, now_ns, translated, capacity, &nat_session);
    if (nat_result != ROUTER_NAT_TRANSLATED) {
      if (nat_result == ROUTER_NAT_INVALID)
        manager->route_invalid++;
      manager->route_tx_drops++;
      fprintf(stderr,
              "NAT OUT DROP: vr=%u egress-rif=%u result=%u\n",
              decision.vr_id, decision.egress_interface_id,
              (unsigned)nat_result);
      goto out;
    }
    forward_frame = translated;
  }

  output = malloc(capacity);
  if (output == NULL) {
    manager->route_tx_drops++;
    goto out;
  }
  output_length = router_ipv4_rewrite(forward_frame, length, egress->mac,
                                      neighbor->mac, output, capacity);
  if (output_length == 0) {
    manager->route_invalid++;
    goto out;
  }
  result = bind_route_egress(manager, egress, neighbor->port_id, &context_tag);
  if (result == DOCA_SUCCESS)
    result = sf_packet_io_send_context(manager->sf_io, output, output_length,
                                       context_tag);
  if (result != DOCA_SUCCESS) {
    manager->route_tx_drops++;
    fprintf(stderr, "ROUTE TX DROP: vr=%u rif=%u vs=%u error=%s\n",
            decision.vr_id, decision.egress_interface_id,
            decision.egress_vswitch_id, doca_error_get_descr(result));
    goto out;
  }
  manager->routed_forwarded++;
  if (manager->packet_debug)
    printf("ROUTE TX: vr=%u ingress-rif=%u egress-rif=%u attachment=%s "
         "egress-vs=%u prefix=/%u next-hop=%u.%u.%u.%u "
         "context-vlan=%u len=%zu\n",
         decision.vr_id, decision.ingress_interface_id,
         decision.egress_interface_id,
         egress->attachment == ROUTER_PORT ? "port" : "vs",
         decision.egress_vswitch_id,
         decision.prefix_length, (decision.next_hop_ip >> 24) & 0xffU,
         (decision.next_hop_ip >> 16) & 0xffU,
         (decision.next_hop_ip >> 8) & 0xffU,
         decision.next_hop_ip & 0xffU, context_tag, output_length);
  if (nat_session != NULL && manager->packet_debug) {
    printf("NAT OUT: vr=%u proto=%u inside=%u.%u.%u.%u:%u "
           "public=%u.%u.%u.%u:%u remote=%u.%u.%u.%u:%u\n",
           nat_session->vr_id, nat_session->protocol,
           (nat_session->inside_ip >> 24) & 0xffU,
           (nat_session->inside_ip >> 16) & 0xffU,
           (nat_session->inside_ip >> 8) & 0xffU,
           nat_session->inside_ip & 0xffU, nat_session->inside_port,
           (nat_session->public_ip >> 24) & 0xffU,
           (nat_session->public_ip >> 16) & 0xffU,
           (nat_session->public_ip >> 8) & 0xffU,
           nat_session->public_ip & 0xffU, nat_session->public_port,
           (nat_session->remote_ip >> 24) & 0xffU,
           (nat_session->remote_ip >> 16) & 0xffU,
           (nat_session->remote_ip >> 8) & 0xffU,
           nat_session->remote_ip & 0xffU, nat_session->remote_port);
  }
  if (nat_session != NULL && ingress->attachment==ROUTER_VSWITCH &&
      manager->pipeline->hardware_ct_enabled &&
      manager->pipeline->hardware_routing_enabled &&
      (nat_session->protocol == IPPROTO_TCP ||
       nat_session->protocol == IPPROTO_UDP)) {
    const struct router_interface *inside_rif = find_router_interface(
        manager, decision.vr_id, decision.ingress_interface_id);

    if (inside_rif != NULL) {
      result = eswitch_pipeline_ct_promote(
          manager->pipeline, nat_session,
          egress->attachment == ROUTER_VSWITCH ? egress->vswitch_id : 0,
          neighbor->port_id, egress->mac, neighbor->mac,
          inside_rif->vswitch_id, nat_session->inside.port_id, inside_rif->mac,
          nat_session->inside.mac);
      if (result != DOCA_SUCCESS && manager->packet_debug)
        fprintf(stderr, "NAT CT PROMOTION DEFERRED: vr=%u proto=%u "
                        "error=%s fallback=arm\n",
                nat_session->vr_id, nat_session->protocol,
                doca_error_get_descr(result));
    }
  }
out:
  free(output);
  free(translated);
}

static void route_uplink_ipv4_packet(
    struct eswitch_manager *manager, struct rte_mbuf *packet,
    const struct router_interface *ingress, uint16_t ingress_port,
    uint64_t now_ns);

static void route_ipv4_packet(struct eswitch_manager *manager,
                              struct rte_mbuf *packet, uint16_t ingress_vs,
                              uint16_t ingress_port, uint64_t now_ns) {
  const struct router_interface *ingress=NULL;
  size_t length=rte_pktmbuf_pkt_len(packet);
  uint8_t *scratch=malloc(length);
  const uint8_t *frame;
  if(scratch==NULL) {manager->route_tx_drops++;return;}
  frame=rte_pktmbuf_read(packet,0,length,scratch);
  if(frame==NULL) {manager->route_invalid++;free(scratch);return;}
  for(size_t i=0;i<manager->router->interface_count;i++) {
    const struct router_interface *candidate=&manager->router->interfaces[i];
    if(candidate->attachment==ROUTER_VSWITCH &&
       candidate->vswitch_id==ingress_vs && candidate->has_address &&
       !memcmp(frame,candidate->mac,6)) {ingress=candidate;break;}
  }
  if(ingress!=NULL) {
    const struct router_nat_policy *policy =
        router_nat_policy_find(manager->router, ingress->vr_id);
    if (policy != NULL && policy->interface_id == ingress->interface_id) {
      /* A packet arriving on the public NAT interface is always consumed by
       * reverse NAT.  A reverse miss is a firewall drop, not permission to
       * fall through into ordinary VR routing. */
      route_uplink_ipv4_packet(manager, packet, ingress, ingress_port,
                               now_ns);
      free(scratch);
      return;
    }
    route_arm_frame(manager,frame,length,ingress,ingress_port,0,0,now_ns);
  }
  free(scratch);
}

static void route_uplink_ipv4_packet(
    struct eswitch_manager *manager, struct rte_mbuf *packet,
    const struct router_interface *ingress, uint16_t ingress_port,
    uint64_t now_ns) {
  const struct router_nat_policy *policy;
  const struct router_nat_session *session = NULL;
  size_t length = rte_pktmbuf_pkt_len(packet);
  size_t capacity = length < 60 ? 60 : length;
  uint8_t *scratch = NULL;
  uint8_t *translated = NULL;
  const uint8_t *frame;
  enum router_nat_result nat_result;

  policy = router_nat_policy_find(manager->router, ingress->vr_id);
  if (policy == NULL || policy->interface_id != ingress->interface_id)
    return;
  scratch = malloc(length);
  translated = malloc(capacity);
  if (scratch == NULL || translated == NULL) {
    manager->route_tx_drops++;
    goto out;
  }
  frame = rte_pktmbuf_read(packet, 0, length, scratch);
  if (frame == NULL) {
    manager->route_invalid++;
    goto out;
  }
  nat_result = router_nat_inbound(manager->nat, ingress->vr_id, frame, length,
                                  now_ns, translated, capacity, &session);
  if (nat_result != ROUTER_NAT_TRANSLATED) {
    if (nat_result == ROUTER_NAT_INVALID)
      manager->route_invalid++;
    goto out;
  }

  if(session==NULL) goto out;
  route_arm_frame(manager,translated,length,ingress,ingress_port,0,
                  session->inside.interface_id,now_ns);
  if (manager->packet_debug)
    printf("NAT IN: vr=%u ingress-port=%u public=%u.%u.%u.%u:%u "
         "inside=%u.%u.%u.%u:%u return-rif=%u\n",
         ingress->vr_id, ingress_port,
         (session->public_ip >> 24) & 0xffU,
         (session->public_ip >> 16) & 0xffU,
         (session->public_ip >> 8) & 0xffU, session->public_ip & 0xffU,
         session->public_port, (session->inside_ip >> 24) & 0xffU,
         (session->inside_ip >> 16) & 0xffU,
         (session->inside_ip >> 8) & 0xffU, session->inside_ip & 0xffU,
         session->inside_port,session->inside.interface_id);
  free(translated);
  free(scratch);
  return;
out:
  free(translated);
  free(scratch);
}

static doca_error_t process_packet(struct eswitch_manager *manager,
                                   struct rte_mbuf *packet,
                                   uint64_t now_ns) {
  struct rte_ether_hdr header_copy;
  const struct rte_ether_hdr *header;
  const struct router_interface *uplink = NULL;
  uint16_t domain_id;
  uint16_t port_id;
  uint32_t metadata = *RTE_FLOW_DYNF_METADATA(packet);
  int port_index;
  doca_error_t result;

  eswitch_metadata_decode(metadata, &domain_id, &port_id);
  port_index = find_port_index(manager, port_id);
  if (port_index >= 0 &&
      !eswitch_port_in_vswitch(manager, domain_id, port_id))
    uplink = find_router_port_interface(manager, port_id, domain_id);
  if (domain_id == 0 || port_index < 0 ||
      (!eswitch_port_in_vswitch(manager, domain_id, port_id) &&
       uplink == NULL)) {
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

  if (uplink != NULL) {
    if (manager->packet_debug)
      printf("ARM RX: vr=%u uplink-rif=%u port=%u len=%u "
           "src=%02x:%02x:%02x:%02x:%02x:%02x "
           "dst=%02x:%02x:%02x:%02x:%02x:%02x\n",
           domain_id, uplink->interface_id, port_id,
           rte_pktmbuf_pkt_len(packet), header->src_addr.addr_bytes[0],
           header->src_addr.addr_bytes[1], header->src_addr.addr_bytes[2],
           header->src_addr.addr_bytes[3], header->src_addr.addr_bytes[4],
           header->src_addr.addr_bytes[5], header->dst_addr.addr_bytes[0],
           header->dst_addr.addr_bytes[1], header->dst_addr.addr_bytes[2],
           header->dst_addr.addr_bytes[3], header->dst_addr.addr_bytes[4],
           header->dst_addr.addr_bytes[5]);
    if (header->ether_type == rte_cpu_to_be_16(RTE_ETHER_TYPE_ARP)) {
      uint8_t arp_scratch[42];
      const uint8_t *arp = rte_pktmbuf_read(packet, 0, sizeof(arp_scratch),
                                            arp_scratch);

      if (arp != NULL && router_neighbor_learn_arp_interface(
                             &manager->neighbors, manager->router,
                             uplink->interface_id, port_id, arp,
                             sizeof(arp_scratch), now_ns)) {
        if (manager->packet_debug)
          printf("UPLINK NEIGHBOR LEARN: vr=%u rif=%u port=%u "
               "ip=%u.%u.%u.%u mac="
               "%02x:%02x:%02x:%02x:%02x:%02x\n",
               uplink->vr_id, uplink->interface_id, port_id, arp[28], arp[29],
               arp[30], arp[31], arp[22], arp[23], arp[24], arp[25], arp[26],
               arp[27]);
        result = eswitch_manager_hw_routes_sync(manager, manager->router);
        if (result != DOCA_SUCCESS)
          fprintf(stderr, "Hardware route sync after uplink ARP failed: %s\n",
                  doca_error_get_descr(result));
      }
      reply_uplink_arp(manager, packet, uplink, port_id, now_ns);
    } else if (header->ether_type ==
               rte_cpu_to_be_16(RTE_ETHER_TYPE_IPV4)) {
      route_uplink_ipv4_packet(manager, packet, uplink, port_id, now_ns);
    }
    return DOCA_SUCCESS;
  }

  /* Reserve configured RIF MACs against dynamic source learning. */
  if (manager->router) for (size_t i = 0; i < manager->router->interface_count; i++) {
    const struct router_interface *rif = &manager->router->interfaces[i];
    if (rif->attachment == ROUTER_VSWITCH && rif->vswitch_id == domain_id &&
        memcmp(header->src_addr.addr_bytes, rif->mac, 6) == 0)
      return DOCA_SUCCESS;
  }
  if (manager->packet_debug)
    printf("ARM RX: vs=%u port=%u len=%u src=%02x:%02x:%02x:%02x:%02x:%02x "
         "dst=%02x:%02x:%02x:%02x:%02x:%02x\n",
         domain_id, port_id, rte_pktmbuf_pkt_len(packet),
         header->src_addr.addr_bytes[0], header->src_addr.addr_bytes[1],
         header->src_addr.addr_bytes[2], header->src_addr.addr_bytes[3],
         header->src_addr.addr_bytes[4], header->src_addr.addr_bytes[5],
         header->dst_addr.addr_bytes[0], header->dst_addr.addr_bytes[1],
         header->dst_addr.addr_bytes[2], header->dst_addr.addr_bytes[3],
         header->dst_addr.addr_bytes[4], header->dst_addr.addr_bytes[5]);

  result = eswitch_fdb_learn(&manager->fdb, domain_id, 0,
                             &header->src_addr, port_id, now_ns);
  if (result != DOCA_SUCCESS)
    return result;
  /* Learn every validated source for ordinary L2 switching and aging. Router
   * replies use a directed SF context and therefore do not depend on this FDB
   * entry being visible before transmission. */
  if (header->ether_type == rte_cpu_to_be_16(RTE_ETHER_TYPE_ARP)) {
    uint8_t arp_scratch[42];
    bool neighbor_changed = false;
    doca_error_t arp_reply_result;
    const uint8_t *arp = rte_pktmbuf_read(packet, 0, sizeof(arp_scratch),
                                          arp_scratch);
    if (arp != NULL)
      neighbor_changed = router_neighbor_learn_arp(
          &manager->neighbors, manager->router, domain_id, port_id, arp,
          sizeof(arp_scratch), now_ns);
    if (neighbor_changed && manager->packet_debug) {
      printf("ROUTE NEIGHBOR LEARN: vs=%u port=%u ip=%u.%u.%u.%u mac="
             "%02x:%02x:%02x:%02x:%02x:%02x\n",
             domain_id, port_id, arp[28], arp[29], arp[30], arp[31], arp[22],
             arp[23], arp[24], arp[25], arp[26], arp[27]);
    }

    /* Neighbor resolution is the dependency of all later IPv4 traffic. Bind
     * the SF return context and enqueue the gateway ARP reply before spending
     * hardware resources or time on an optional LPM promotion. The neighbor
     * is already present in the Arm table, so the sync below still sees it. */
    arp_reply_result = reply_gateway_arp(manager, packet, domain_id, port_id,
                                         now_ns);
    /* A successful gateway reply also retries a promotion deferred by an
     * earlier resource failure, even when the neighbor tuple is unchanged. */
    if ((neighbor_changed || arp_reply_result == DOCA_SUCCESS) &&
        (arp_reply_result == DOCA_SUCCESS ||
         arp_reply_result == DOCA_ERROR_NOT_FOUND)) {
      result = eswitch_manager_hw_routes_sync(manager, manager->router);
      if (result != DOCA_SUCCESS)
        fprintf(stderr, "Hardware route sync after private ARP failed: %s\n",
                doca_error_get_descr(result));
    } else if (neighbor_changed) {
      fprintf(stderr,
              "Hardware route promotion deferred: vs=%u port=%u "
              "arp-return-error=%s\n",
              domain_id, port_id, doca_error_get_descr(arp_reply_result));
    }
  } else if (header->ether_type == rte_cpu_to_be_16(RTE_ETHER_TYPE_IPV4)) {
    route_ipv4_packet(manager, packet, domain_id, port_id, now_ns);
  }
  return DOCA_SUCCESS;
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
    if (result != DOCA_SUCCESS) {
      for (uint16_t remaining = i + 1; remaining < received; remaining++)
        rte_pktmbuf_free(packets[remaining]);
      return result;
    }
  }
  return DOCA_SUCCESS;
}

static size_t format_status(const struct eswitch_manager *manager,
                            char *response, size_t size);
static size_t format_tx_debug(const struct eswitch_manager *manager,
                              char *response, size_t size);

doca_error_t eswitch_manager_maintenance(struct eswitch_manager *manager) {
  uint64_t now_ns;

  if (manager == NULL || !manager->initialized)
    return DOCA_ERROR_INVALID_VALUE;
  now_ns = monotonic_ns();
  if (manager->packet_debug &&
      manager->arp_seen != manager->tx_snapshot_seen &&
      now_ns - manager->tx_snapshot_ns >= 5000000000ULL) {
    char diagnostic[8192];
    format_tx_debug(manager, diagnostic, sizeof(diagnostic));
    printf("SF TX DEBUG SNAPSHOT (cumulative; send success is not guest receipt):\n%s",
           diagnostic);
    manager->tx_snapshot_seen = manager->arp_seen;
    manager->tx_snapshot_ns = now_ns;
  }
  if (now_ns < manager->next_aging_ns)
    return DOCA_SUCCESS;
  manager->next_aging_ns = now_ns +
      (uint64_t)SWITCH_AGING_SCAN_SECONDS * 1000000000ULL;
  router_neighbor_age(&manager->neighbors, now_ns);
  router_nat_age(manager->nat, now_ns);
  {
    doca_error_t result = eswitch_manager_hw_routes_sync(manager,
                                                          manager->router);
    /* Hardware route promotion is optional and already has exponential
     * backoff. Never let its resource pressure suppress FDB aging or make the
     * main loop mislabel the failure as FDB maintenance. */
    if (result != DOCA_SUCCESS && manager->packet_debug)
      fprintf(stderr, "Optional hardware route maintenance deferred: %s\n",
              doca_error_get_descr(result));
  }
  return eswitch_fdb_age(&manager->fdb, now_ns);
}

static size_t format_status(const struct eswitch_manager *manager,
                            char *response, size_t size) {
  size_t used = 0;
  size_t switch_count = 0;
  size_t assigned_count = 0;
  size_t assignable_port_count = 0;
  size_t sf_return_count = 0;
  size_t sf_directed_count = 0;
  uint64_t sf_ingress_hits = 0;
  uint64_t sf_context_hits = 0;
  uint64_t local_ip_hits = 0;
  uint64_t hw_lpm_misses = 0;
  uint64_t uplink_arp_hw_drops = 0;
  uint64_t now_ns = monotonic_ns();
  uint64_t hw_retry_in_ms = manager->next_hw_route_retry_ns > now_ns
      ? (manager->next_hw_route_retry_ns - now_ns) / UINT64_C(1000000)
      : 0;
  doca_error_t sf_counter_result;
  doca_error_t hw_counter_result;
  doca_error_t uplink_arp_counter_result;
  uint64_t uptime = (now_ns - manager->started_ns) / 1000000000ULL;

  for (size_t i = 0; i < ESWITCH_MAX_VSWITCHES; i++)
    switch_count += manager->switches[i].exists ? 1U : 0U;
  for (uint16_t i = 0; i < manager->ports->count; i++) {
    const struct ethernet_port *port = manager->ports->items[i].ethernet;
    if (port->role == ETHERNET_PORT_ROLE_SF_REPRESENTOR)
      continue;
    assignable_port_count++;
    bool member = false;
    for (size_t j = 0; j < ESWITCH_MAX_PERSISTED_MEMBERS; j++)
      member |= manager->memberships[j].active &&
                manager->memberships[j].port_index == i;
    assigned_count += (member || router_control_port_reserved(manager, i))
                          ? 1U : 0U;
  }
  for (size_t i = 0; i < ESWITCH_MAX_SF_RETURN_CONTEXTS; i++) {
    const struct eswitch_sf_return_context *context =
        &manager->pipeline->sf_return_contexts[i];

    sf_return_count += context->active ? 1U : 0U;
    sf_directed_count += context->active && context->directed ? 1U : 0U;
  }
  sf_counter_result = eswitch_pipeline_sf_query_counters(
      manager->pipeline, &sf_ingress_hits, &sf_context_hits,
      &local_ip_hits);
  hw_counter_result = eswitch_pipeline_hw_route_stats(manager->pipeline,
                                                       &hw_lpm_misses);
  uplink_arp_counter_result = eswitch_pipeline_uplink_arp_drop_query(
      manager->pipeline, &uplink_arp_hw_drops);
  used = append_text(response, size, used, "OK\n");
  used = append_text(response, size, used,
                     "service=eSwitch Management state=running uptime=%" PRIu64
                     "s\n",
                     uptime);
  used = append_text(response, size, used, "config=%s\n",
                     manager->state_path);
  used = append_text(response, size, used,
      "tx_revision=%s tx_mode=%s sf_iface=%s sf_port=%u\n",
      ESWITCH_TX_REVISION, ESWITCH_TX_FLOW_MODE,
      manager->sf_io->interface_name, manager->pipeline->sf_port_id);
  used = append_text(response, size, used,
                     "packet_debug=%s\n",
                     manager->packet_debug ? "enabled" : "disabled");
  used = append_text(response, size, used,
                     "ports=%u assignable=%zu assigned=%zu available=%zu "
                     "vswitches=%zu fdb=%zu\n",
                     manager->ports->count, assignable_port_count,
                     assigned_count, assignable_port_count - assigned_count,
                     switch_count,
                     manager->fdb.count);
  used = append_text(response, size, used,
                     "routers=%zu router_links=%zu "
                     "router_dataplane=%s\n",
                     manager->router ? manager->router->vr_count : 0,
                     manager->router ? manager->router->link_count : 0,
                     manager->pipeline->hardware_routing_enabled
                         ? "DOCA_FLOW_LPM_PRIVATE_PLUS_ARM_ROUTER_LINK_NAT44"
                         : "GATEWAY_ARP_ICMP_ARM_LPM_ROUTER_LINK_NAT44");
  used = append_text(response, size, used,
      "hw_routing_configured=%s hw_state=%s hw_scope=private-vs-ipv4 "
      "hw_requested_capacity=%u hw_capacity=%u hw_routes=%zu "
      "promotions=%" PRIu64 " updates=%" PRIu64
      " removals=%" PRIu64 " failures=%" PRIu64
      " lpm_misses=%" PRIu64 " counter_state=%s\n",
      manager->pipeline->hardware_routing_requested ? "enabled" : "disabled",
      !manager->pipeline->hardware_routing_requested ? "off" :
          (!manager->pipeline->hardware_routing_enabled ? "fallback-arm" :
           (manager->pipeline->hardware_routing_degraded ? "degraded" :
                                                          "ready")),
      manager->pipeline->hw_route_requested_capacity,
      manager->pipeline->hw_route_capacity,
      manager->pipeline->hw_route_count,
      manager->pipeline->hw_route_promotions,
      manager->pipeline->hw_route_updates,
      manager->pipeline->hw_route_removals,
      manager->pipeline->hw_route_failures, hw_lpm_misses,
      !manager->pipeline->hardware_routing_enabled ? "off" :
          (hw_counter_result == DOCA_SUCCESS ? "ready" : "error"));
  used = append_text(response, size, used,
      "hw_retry_backoff_ms=%" PRIu64 " hw_retry_in_ms=%" PRIu64 "\n",
      manager->hw_route_retry_backoff_ns / UINT64_C(1000000),
      hw_retry_in_ms);
  used = append_text(response, size, used,
      "uplink_arp_classifier=%s scope=broadcast-arp rate_pps=%u burst=%u "
      "hw_drops=%" PRIu64 " counter_state=%s exact_target_validation=arm\n",
      !manager->pipeline->uplink_arp_filter_requested ? "off" :
          (manager->pipeline->uplink_arp_filter_enabled &&
                   !manager->pipeline->uplink_arp_filter_degraded
               ? "ready"
               : "fallback-arm"),
      manager->pipeline->uplink_arp_pps,
      manager->pipeline->uplink_arp_burst, uplink_arp_hw_drops,
      !manager->pipeline->uplink_arp_filter_enabled ? "off" :
          (uplink_arp_counter_result == DOCA_SUCCESS ? "ready" : "error"));
  used = append_text(response, size, used,
      "nat_dataplane=%s hw_ct_capability=%s hw_ct_state=%s "
      "hw_ct_capacity=%u hw_ct_active=%zu hw_ct_promotions=%" PRIu64
      " hw_ct_failures=%" PRIu64 " hw_ct_full=%" PRIu64 "\n",
      manager->pipeline->hardware_ct_enabled
          ? "DOCA_FLOW_CT_TCP_UDP_PLUS_ARM_ICMP_SLOWPATH"
          : "ARM_NAPT_TCP_UDP_ICMP_ECHO",
      manager->hardware_ct_supported ? "supported" : "unsupported",
      !manager->pipeline->hardware_ct_requested ? "off" :
          (!manager->pipeline->hardware_ct_enabled ? "fallback-arm" :
           (manager->pipeline->hardware_ct_degraded ? "degraded" :
                                                      "ready")),
      manager->pipeline->ct_capacity, manager->pipeline->ct_active,
      manager->pipeline->ct_promotions, manager->pipeline->ct_failures,
      manager->pipeline->ct_full);
  used = append_text(response, size, used,
      "nat_policies=%zu nat_sessions=%zu nat_out=%" PRIu64
      " nat_in=%" PRIu64 " nat_reverse_misses=%" PRIu64
      " nat_created=%" PRIu64 " nat_aged=%" PRIu64
      " nat_unsupported=%" PRIu64 " nat_invalid=%" PRIu64
      " nat_port_alloc_failures=%" PRIu64 "\n",
      manager->router ? manager->router->nat_policy_count : 0,
      manager->nat ? manager->nat->count : 0,
      manager->nat ? manager->nat->stats.outbound_packets : 0,
      manager->nat ? manager->nat->stats.inbound_packets : 0,
      manager->nat ? manager->nat->stats.reverse_misses : 0,
      manager->nat ? manager->nat->stats.sessions_created : 0,
      manager->nat ? manager->nat->stats.sessions_aged : 0,
      manager->nat ? manager->nat->stats.unsupported_packets : 0,
      manager->nat ? manager->nat->stats.invalid_packets : 0,
      manager->nat ? manager->nat->stats.port_allocation_failures : 0);
  used = append_text(response, size, used,
      "nat_icmp_echo_out=%" PRIu64 " nat_icmp_echo_in=%" PRIu64 "\n",
      manager->nat ? manager->nat->stats.icmp_echo_outbound_packets : 0,
      manager->nat ? manager->nat->stats.icmp_echo_inbound_packets : 0);
  used = append_text(response, size, used,
      "private_gateway_arp=enabled arp_sf_tx_sent=%" PRIu64
      " arp_tx_drops=%" PRIu64 " arp_rate_drops=%" PRIu64 "\n",
      manager->arp_replies, manager->arp_tx_drops, manager->arp_rate_drops);
  used = append_text(response, size, used,
      "arp_seen=%" PRIu64 " arp_ignored=%" PRIu64 " arp_built=%" PRIu64
      " target_drops=%" PRIu64 " sf_send_drops=%" PRIu64 "\n",
      manager->arp_seen, manager->arp_ignored, manager->arp_built,
      manager->arp_target_drops, manager->arp_sf_send_drops);
  used = append_text(response, size, used,
      "gateway_icmp=enabled icmp_seen=%" PRIu64
      " icmp_ignored=%" PRIu64 " icmp_built=%" PRIu64
      " icmp_sf_tx_sent=%" PRIu64 " icmp_tx_drops=%" PRIu64 "\n",
      manager->icmp_seen, manager->icmp_ignored, manager->icmp_built,
      manager->icmp_replies, manager->icmp_tx_drops);
  used = append_text(response, size, used,
      "ipv4_routing=arm-lpm routed_seen=%" PRIu64
      " routed_forwarded=%" PRIu64 " no_route=%" PRIu64
      " ttl_expired=%" PRIu64 " invalid=%" PRIu64 "\n",
      manager->routed_seen, manager->routed_forwarded,
      manager->route_no_route, manager->route_ttl_expired,
      manager->route_invalid);
  used = append_text(response, size, used,
      "router_link_dataplane=arm forwards=%" PRIu64 " drops=%" PRIu64
      " max_hops=%u\n", manager->router_link_forwards,
      manager->router_link_drops, ROUTER_LINK_MAX_HOPS);
  used = append_text(response, size, used,
      "neighbors=%zu neighbor_misses=%" PRIu64 " arp_probes=%" PRIu64
      " route_tx_drops=%" PRIu64 "\n",
      manager->neighbors.count, manager->route_neighbor_misses,
      manager->route_arp_probes, manager->route_tx_drops);
  used = append_text(response, size, used,
      "sf_return_contexts=%zu directed=%zu flood=%zu "
      "source_identity=context-vlan "
      "sf_wire_source=actual-sf rewrite_source=rif-mac "
      "unicast_destination=direct-egress broadcast_destination=vs-flood\n",
      sf_return_count, sf_directed_count,
      sf_return_count - sf_directed_count);
  if (sf_counter_result == DOCA_SUCCESS)
    used = append_text(response, size, used,
        "sf_counter_state=ready sf_ingress_hits=%" PRIu64
        " sf_context_hits=%" PRIu64 " local_ip_hits=%" PRIu64 "\n",
        sf_ingress_hits, sf_context_hits, local_ip_hits);
  else
    used = append_text(response, size, used,
        "sf_counter_state=error error=%s\n",
        doca_error_get_descr(sf_counter_result));
  return used;
}

static size_t format_tx_debug(const struct eswitch_manager *manager,
                              char *response, size_t size) {
  size_t used = format_status(manager, response, size);
  uint64_t destination_misses = 0;
  doca_error_t result = eswitch_pipeline_destination_miss_query(
      manager->pipeline, &destination_misses);

  if (result == DOCA_SUCCESS)
    used = append_text(response, size, used,
        "return_path_counter_state=ready destination_fdb_misses=%" PRIu64
        "\n", destination_misses);
  else
    used = append_text(response, size, used,
        "return_path_counter_state=error error=%s\n",
        doca_error_get_descr(result));

  for (size_t i = 0; i < ESWITCH_MAX_SF_RETURN_CONTEXTS; i++) {
    const struct eswitch_sf_return_context *context =
        &manager->pipeline->sf_return_contexts[i];
    uint64_t context_hits = 0;

    if (!context->active)
      continue;
    result = eswitch_pipeline_sf_context_query(context, &context_hits);
    if (result != DOCA_SUCCESS) {
      used = append_text(response, size, used,
          "sf_context_tag=%u counter_state=error error=%s\n",
          context->context_tag, doca_error_get_descr(result));
      continue;
    }
    used = append_text(response, size, used,
        "sf_context_tag=%u vr=%u rif=%u vs=%u mode=%s target=%u "
        "hits=%" PRIu64 "\n",
        context->context_tag, context->vr_id, context->interface_id,
        context->vswitch_id,
        context->directed ? "directed" : "flood",
        context->directed ? context->target_port_id : UINT16_MAX,
        context_hits);
  }

  for (uint16_t i = 0; i < manager->ports->count; i++) {
    const struct ethernet_port *port = manager->ports->items[i].ethernet;
    uint64_t forward_hits = 0;
    uint64_t split_horizon_drops = 0;

    result = eswitch_pipeline_egress_query(
        manager->pipeline, port->port_id, &forward_hits,
        &split_horizon_drops);
    if (result == DOCA_ERROR_NOT_FOUND)
      continue;
    if (result != DOCA_SUCCESS) {
      used = append_text(response, size, used,
          "egress_port=%u counter_state=error error=%s\n", port->port_id,
          doca_error_get_descr(result));
      continue;
    }
    used = append_text(response, size, used,
        "egress_port=%u owner_vs=%u forward_hits=%" PRIu64
        " split_horizon_drops=%" PRIu64 "\n",
        port->port_id, manager->port_owner[i], forward_hits,
        split_horizon_drops);
  }
  return used;
}

static size_t format_vswitches(const struct eswitch_manager *manager,
                               char *response, size_t size, uint16_t filter) {
  size_t used = append_text(response, size, 0, "OK\n");
  size_t shown = 0;

  for (size_t s = 0; s < ESWITCH_MAX_VSWITCHES; s++) {
    const struct managed_vswitch *vs = &manager->switches[s];
    bool first = true;
    if (!vs->exists)
      continue;
    if (filter != 0 && vs->id != filter)
      continue;
    used = append_text(response, size, used, "vs=%u ports=[", vs->id);
    for (size_t i = 0; i < ESWITCH_MAX_PERSISTED_MEMBERS; i++) {
      const struct eswitch_port_membership *member = &manager->memberships[i];
      if (!member->active || member->vswitch_id != vs->id)
        continue;
      if (member->mode == ESWITCH_PORT_MODE_TRUNK) {
        if (member->vlan_extra_id != 0)
          used = member->vlan_id == member->vlan_last
                     ? append_text(response, size, used,
                                   "%s%u:trunk/vlan=%u+%u-%u",
                                   first ? "" : ",", member->port_id,
                                   member->vlan_id, member->vlan_extra_id,
                                   member->vlan_extra_last)
                     : append_text(response, size, used,
                                   "%s%u:trunk/vlan=%u-%u+%u-%u",
                                   first ? "" : ",", member->port_id,
                                   member->vlan_id, member->vlan_last,
                                   member->vlan_extra_id,
                                   member->vlan_extra_last);
        else if (eswitch_vlan_is_range(member->vlan_id, member->vlan_last))
          used = append_text(response, size, used,
                             "%s%u:trunk/vlan=%u-%u", first ? "" : ",",
                             member->port_id, member->vlan_id,
                             member->vlan_last);
        else
          used = append_text(response, size, used,
                             "%s%u:trunk/vlan=%u", first ? "" : ",",
                             member->port_id, member->vlan_id);
      } else if (member->vlan_id != 0) {
        used = append_text(response, size, used, "%s%u:access/vlan=%u",
                           first ? "" : ",", member->port_id,
                           member->vlan_id);
      } else {
        used = append_text(response, size, used, "%s%u:access",
                           first ? "" : ",", member->port_id);
      }
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
    if (port->role == ETHERNET_PORT_ROLE_SF_REPRESENTOR)
      continue;
    bool access_member = false;
    bool trunk_member = false;
    for (size_t j = 0; j < ESWITCH_MAX_PERSISTED_MEMBERS; j++) {
      const struct eswitch_port_membership *member = &manager->memberships[j];
      if (!member->active || member->port_index != i)
        continue;
      access_member |= member->mode == ESWITCH_PORT_MODE_ACCESS;
      trunk_member |= member->mode == ESWITCH_PORT_MODE_TRUNK;
    }
    /* A parent port remains selectable after its first trunk membership: the
     * same physical trunk can carry another VLAN into another VS. Access
     * ports and VF representors remain exclusive. */
    if (access_member || router_control_port_reserved(manager, i) ||
        (trunk_member && port->role != ETHERNET_PORT_ROLE_PARENT))
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
  struct eswitch_cli_command parsed = {0};
  doca_error_t result = DOCA_SUCCESS;
  bool syntax_error = false;

  if (request == NULL || response == NULL || response_size == 0 ||
      manager == NULL || !manager->initialized)
    return DOCA_ERROR_INVALID_VALUE;
  /* The router group owns its own grammar and its own transaction. */
  if (eswitch_cli_is_router_line(request))
    return router_control_command(manager, request, response, response_size);
  /* Raw socket clients reach the same grammar as eswitchctl: canonical
   * resource-first forms plus the deprecated flat aliases. */
  if (!eswitch_cli_parse_line(request, &parsed)) {
    result = DOCA_ERROR_INVALID_VALUE;
    syntax_error = true;
  } else if (parsed.verb == ESWITCH_CLI_STATUS) {
    (void)format_status(manager, response, response_size);
    return DOCA_SUCCESS;
  } else if (parsed.verb == ESWITCH_CLI_TX_DEBUG) {
    (void)format_tx_debug(manager, response, response_size);
    return DOCA_SUCCESS;
  } else if (parsed.verb == ESWITCH_CLI_PORT_SHOW) {
    (void)format_available_ports(manager, response, response_size);
    return DOCA_SUCCESS;
  } else if (parsed.verb == ESWITCH_CLI_VS_SHOW) {
    /* An explicit filter for an absent vSwitch is a lookup miss, not an empty
     * collection, so API clients can map it to 404. */
    if (parsed.id != 0 && find_vswitch(manager, parsed.id) == NULL) {
      result = DOCA_ERROR_NOT_FOUND;
    } else {
      (void)format_vswitches(manager, response, response_size, parsed.id);
      return DOCA_SUCCESS;
    }
  } else if (parsed.verb == ESWITCH_CLI_FDB_SHOW) {
    snprintf(response, response_size, "OK\n");
    eswitch_fdb_format(&manager->fdb, parsed.id,
                       response + strlen(response),
                       response_size - strlen(response));
    return DOCA_SUCCESS;
  } else if (parsed.verb == ESWITCH_CLI_VS_CREATE) {
    result = create_vswitch_persisted(manager, parsed.id);
  } else if (parsed.verb == ESWITCH_CLI_VS_DELETE) {
    if (manager->router && router_switch_reserved(manager->router, parsed.id)) {
      snprintf(response, response_size,
               "ERR vSwitch is attached to a VR; detach it first with: "
               "vr switch detach --id <vr-id> --interface <name>\n");
      return DOCA_ERROR_IN_USE;
    }
    result = delete_vswitch_persisted(manager, parsed.id);
  } else if (parsed.verb == ESWITCH_CLI_VS_PORT_ATTACH) {
    result = attach_port_persisted(manager, parsed.id, parsed.port_id,
                                   parsed.port_mode, parsed.vlan_id,
                                   parsed.vlan_last, parsed.vlan_extra_id,
                                   parsed.vlan_extra_last);
  } else if (parsed.verb == ESWITCH_CLI_VS_PORT_DETACH) {
    result = detach_port_persisted(manager, parsed.id, parsed.port_id);
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
  if (syntax_error)
    append_text(response, response_size, strlen(response), "%s",
                eswitch_cli_usage_for_line(request));
  return result;
}

doca_error_t eswitch_manager_destroy(struct eswitch_manager *manager) {
  doca_error_t first_error = DOCA_SUCCESS;

  if (manager == NULL)
    return DOCA_ERROR_INVALID_VALUE;
  if (!manager->initialized) {
    free(manager->router);
    free(manager->port_owner);
    free(manager->nat);
    *manager = (struct eswitch_manager){0};
    return DOCA_SUCCESS;
  }
  first_error = eswitch_pipeline_ct_flush(manager->pipeline, 0);
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
    free(manager->router);
    free(manager->port_owner);
    free(manager->nat);
    *manager = (struct eswitch_manager){0};
  }
  return first_error;
}

void eswitch_manager_release(struct eswitch_manager *manager) {
  if (manager == NULL)
    return;
  if (manager->pipeline != NULL) {
    for (size_t i = 0; i < ROUTER_NAT_MAX_SESSIONS; i++)
      manager->pipeline->ct_sessions[i].software = NULL;
  }
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
  free(manager->router);
  free(manager->nat);
  *manager = (struct eswitch_manager){0};
}
