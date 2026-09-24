#ifndef ESWITCH_MANAGER_H
#define ESWITCH_MANAGER_H

#include <stdbool.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>

#include <doca_error.h>

#include "../ethernet_switch/dpdk_io.h"
#include "eswitch_config.h"
#include "eswitch_vlan.h"
#include "l2/eswitch_fdb.h"
#include "router/router.h"
#include "router/router_neighbor.h"
#include "router/router_pending.h"
#include "router/router_nat.h"
#include "router/sf_packet_io.h"

struct managed_vswitch {
  bool exists;
  uint16_t id;
  struct eswitch_flood_group flood;
};

struct eswitch_manager {
  struct dpdk_io *io;
  struct switch_flow_ports *ports;
  struct eswitch_pipeline *pipeline;
  struct sf_packet_io *sf_io;
  struct eswitch_fdb fdb;
  struct router_config *router;
  uint64_t arp_replies, arp_tx_drops, arp_rate_drops;
  uint64_t arp_window_ns;
  unsigned arp_window_replies;
  uint64_t arp_seen, arp_ignored, arp_built;
  uint64_t arp_target_drops, arp_sf_send_drops;
  uint64_t icmp_seen, icmp_ignored, icmp_built, icmp_replies;
  uint64_t icmp_tx_drops;
  struct router_neighbor_table neighbors;
  struct router_pending_queue pending;
  struct router_nat_table *nat;
  uint64_t nat_local_fallbacks, nat_fail_closed_drops;
  bool hardware_ct_supported;
  bool packet_debug;
  uint64_t routed_seen, routed_forwarded, route_no_route;
  uint64_t egress_checked, egress_allowed, egress_denied;
  uint64_t egress_established_replies;
  uint64_t route_ttl_expired, route_invalid, route_neighbor_misses;
  uint64_t route_arp_probes, route_proactive_probes, route_tx_drops;
  uint64_t router_link_forwards, router_link_drops;
  uint64_t shared_vswitch_forwards, shared_vswitch_drops;
  uint64_t tx_log_ns, tx_snapshot_ns, tx_snapshot_seen;
  uint64_t next_hw_route_retry_ns, hw_route_retry_backoff_ns;
  struct managed_vswitch switches[ESWITCH_MAX_VSWITCHES];
  struct eswitch_port_membership memberships[ESWITCH_MAX_PERSISTED_MEMBERS];
  size_t membership_count;
  uint16_t *port_owner; /* indexed like ports->items; 0 means available */
  uint64_t started_ns;
  uint64_t next_aging_ns;
  char state_path[PATH_MAX];
  bool initialized;
};

doca_error_t eswitch_manager_init(struct dpdk_io *io,
                                  struct switch_flow_ports *ports,
                                  struct eswitch_pipeline *pipeline,
                                  struct sf_packet_io *sf_io,
                                  bool hardware_ct_supported,
                                  bool packet_debug,
                                  const char *state_path,
                                  struct eswitch_manager *manager);
doca_error_t eswitch_manager_poll_packets(struct eswitch_manager *manager,
                                          bool *did_work);
doca_error_t eswitch_manager_maintenance(struct eswitch_manager *manager);
doca_error_t eswitch_manager_hw_routes_sync(
    struct eswitch_manager *manager, const struct router_config *config);
/* Arm all static SF return contexts, then probe configured next hops whose
 * neighbor state is absent or nearing expiry. Safe to call repeatedly. */
doca_error_t eswitch_manager_router_prepare(
    struct eswitch_manager *manager, const struct router_config *config,
    uint64_t now_ns);
doca_error_t eswitch_manager_router_prearm(
    struct eswitch_manager *manager, const struct router_config *config);
doca_error_t eswitch_manager_command(const char *request, char *response,
                                     size_t response_size, void *context);
doca_error_t eswitch_manager_destroy(struct eswitch_manager *manager);
/* Emergency cleanup after a graceful hardware removal error. Call this before
 * destroying the shared pipeline so per-vSwitch pipes release references. */
void eswitch_manager_release(struct eswitch_manager *manager);

#endif /* ESWITCH_MANAGER_H */
