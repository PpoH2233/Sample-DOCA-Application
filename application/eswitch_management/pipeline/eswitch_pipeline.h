#ifndef ESWITCH_PIPELINE_H
#define ESWITCH_PIPELINE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <doca_error.h>
#include <doca_flow.h>
#include <rte_ether.h>

#include "../../ethernet_switch/flow_ports.h"
#include "../../ethernet_switch/flow_runtime.h"
#include "../router/router_hw.h"

struct eswitch_rule {
  struct doca_flow_pipe_entry *entry;
  struct flow_entry_cookie cookie;
};

struct eswitch_egress_gate {
  struct doca_flow_pipe *pipe;
  struct eswitch_rule drop_self;
  struct eswitch_rule forward;
};

struct eswitch_flood_member {
  uint16_t port_id;
  bool active;
  struct eswitch_rule rule;
};

struct eswitch_flood_group {
  uint16_t vswitch_id;
  struct doca_flow_pipe *pipe;
  struct eswitch_rule selector;
  struct eswitch_flood_member *members;
  uint16_t member_count;
  uint16_t member_capacity;
};

#define ESWITCH_MAX_SF_RETURN_CONTEXTS 256U

struct eswitch_sf_return_context {
  uint16_t vr_id;
  uint16_t vswitch_id;
  uint16_t context_tag;
  uint16_t target_port_id;
  uint8_t rif_mac[6];
  uint32_t rif_address;
  struct eswitch_rule return_rule;
  struct eswitch_rule local_ip_rule;
  struct eswitch_rule route_selector_rule;
  struct eswitch_rule route_eligible_rule;
  bool directed;
  bool active;
};

struct eswitch_hw_route_entry {
  struct router_hw_route spec;
  struct eswitch_rule rule;
  bool active;
};

struct eswitch_pipeline {
  struct flow_runtime *runtime;
  struct switch_flow_ports *ports;
  struct doca_flow_port *switch_port;

  struct doca_flow_pipe *rss_pipe;
  struct doca_flow_pipe *flood_selector_pipe;
  struct doca_flow_pipe *destination_pipe;
  struct doca_flow_pipe *learning_clone_pipe;
  struct doca_flow_pipe *learning_dispatch_pipe;
  struct doca_flow_pipe *source_guard_pipe;
  struct doca_flow_pipe *arp_dispatch_pipe;
  struct eswitch_rule arp_dispatch_rule;
  struct doca_flow_pipe *ingress_classifier_pipe;
  struct doca_flow_pipe *sf_return_pipe;
  struct doca_flow_pipe *local_ip_pipe;
  struct doca_flow_pipe *route_control_pipe;
  struct doca_flow_pipe *route_selector_pipe;
  struct doca_flow_pipe *route_lpm_pipe;
  struct eswitch_rule route_fallback_rule;
  struct eswitch_hw_route_entry hw_routes[ROUTER_HW_MAX_ROUTES];
  size_t hw_route_count;
  uint64_t hw_route_promotions;
  uint64_t hw_route_updates;
  uint64_t hw_route_removals;
  uint64_t hw_route_failures;
  uint32_t hw_route_requested_capacity;
  uint32_t hw_route_capacity;
  bool hardware_routing_requested;
  bool hardware_routing_enabled;
  bool hardware_routing_degraded;
  struct eswitch_rule sf_root_rule;
  uint16_t sf_port_id;
  struct eswitch_sf_return_context
      sf_return_contexts[ESWITCH_MAX_SF_RETURN_CONTEXTS];

  struct eswitch_rule rss_rule;
  struct eswitch_rule learning_clone_rules[2];
  struct eswitch_rule learning_dispatch_rule;
  struct eswitch_rule *classifier_rules; /* indexed like ports->items */
  struct eswitch_egress_gate *egress_gates; /* indexed like ports->items */
  bool created;
};

struct eswitch_hw_fdb_entry {
  /* Two stable cookie addresses let MAC move install the new source guard
   * before removing the old one. DOCA retains usr_ctx for the entry lifetime. */
  struct eswitch_rule sources[2];
  uint8_t active_source;
  struct eswitch_rule destination;
  uint16_t learned_port_id;
};

uint32_t eswitch_metadata_encode(uint16_t vswitch_id, uint16_t port_id);
void eswitch_metadata_decode(uint32_t metadata, uint16_t *vswitch_id,
                            uint16_t *port_id);

/* Bind an internal SF VLAN tag to one VS and virtual RIF source identity. */
doca_error_t eswitch_pipeline_sf_bind_vswitch(
    struct eswitch_pipeline *pipeline, uint16_t vr_id, uint16_t vswitch_id,
    uint32_t rif_address, const uint8_t rif_mac[6], uint16_t *context_tag);

/* Bind an SF context directly to one known egress port. */
doca_error_t eswitch_pipeline_sf_bind_egress(
    struct eswitch_pipeline *pipeline, uint16_t vswitch_id,
    uint16_t target_port_id, const uint8_t rif_mac[6],
    uint16_t *context_tag);

/* Remove one directed context without disturbing the other egresses that use
 * the same metadata domain. This is used when a public RIF MAC changes. */
doca_error_t eswitch_pipeline_sf_unbind_egress(
    struct eswitch_pipeline *pipeline, uint16_t domain_id,
    uint16_t target_port_id);

/* Remove a previously installed SF return context. This is idempotent. */
doca_error_t eswitch_pipeline_sf_unbind_vswitch(
    struct eswitch_pipeline *pipeline, uint16_t vswitch_id);

/* Query cumulative SF root and active context-entry hits. */
doca_error_t eswitch_pipeline_sf_query_counters(
    const struct eswitch_pipeline *pipeline, uint64_t *ingress_packets,
    uint64_t *context_packets, uint64_t *local_ip_packets);

/* Query one active SF context entry. */
doca_error_t eswitch_pipeline_sf_context_query(
    const struct eswitch_sf_return_context *context, uint64_t *packets);

/* Return-path diagnostics. Counters are cumulative hardware values. */
doca_error_t eswitch_pipeline_destination_miss_query(
    const struct eswitch_pipeline *pipeline, uint64_t *packets);
doca_error_t eswitch_pipeline_egress_query(
    const struct eswitch_pipeline *pipeline, uint16_t port_id,
    uint64_t *forward_packets, uint64_t *split_horizon_drops);

doca_error_t eswitch_pipeline_create(struct flow_runtime *runtime,
                                     struct switch_flow_ports *ports,
                                     bool hardware_routing_enabled,
                                     uint32_t hardware_route_capacity,
                                     struct eswitch_pipeline *pipeline);
void eswitch_pipeline_destroy(struct eswitch_pipeline *pipeline);

doca_error_t eswitch_pipeline_attach_port(struct eswitch_pipeline *pipeline,
                                          uint16_t port_index,
                                          uint16_t vswitch_id);
/* Router-owned uplinks bypass L2 learning and go directly to the Arm RSS
 * slow path. The high metadata half carries the VR id for ingress isolation. */
doca_error_t eswitch_pipeline_attach_router_port(
    struct eswitch_pipeline *pipeline, uint16_t port_index, uint16_t vr_id);

/* Incrementally reconcile resolved private routes with the LPM table. */
doca_error_t eswitch_pipeline_hw_routes_sync(
    struct eswitch_pipeline *pipeline, const struct router_hw_route *routes,
    size_t route_count);
doca_error_t eswitch_pipeline_hw_route_stats(
    const struct eswitch_pipeline *pipeline, uint64_t *lpm_misses);
doca_error_t eswitch_pipeline_detach_port(struct eswitch_pipeline *pipeline,
                                          uint16_t port_index);

/* One flooding hash pipe per vSwitch; membership changes are incremental. */
doca_error_t eswitch_pipeline_flood_add_port(
    struct eswitch_pipeline *pipeline, uint16_t vswitch_id, uint16_t port_id,
    struct eswitch_flood_group *group);
doca_error_t eswitch_pipeline_flood_remove_port(
    struct eswitch_pipeline *pipeline, uint16_t port_id,
    struct eswitch_flood_group *group);
doca_error_t eswitch_pipeline_destroy_flood_group(
    struct eswitch_pipeline *pipeline, struct eswitch_flood_group *group);

doca_error_t eswitch_pipeline_fdb_add(
    struct eswitch_pipeline *pipeline, uint16_t vswitch_id,
    const struct rte_ether_addr *mac, uint16_t learned_port_id,
    struct eswitch_hw_fdb_entry *hardware);
doca_error_t eswitch_pipeline_fdb_move(
    struct eswitch_pipeline *pipeline, uint16_t vswitch_id,
    const struct rte_ether_addr *mac, uint16_t old_port_id,
    uint16_t new_port_id, struct eswitch_hw_fdb_entry *hardware);
doca_error_t eswitch_pipeline_fdb_remove(
    struct eswitch_pipeline *pipeline,
    struct eswitch_hw_fdb_entry *hardware);
doca_error_t eswitch_pipeline_fdb_query(
    const struct eswitch_hw_fdb_entry *hardware, uint64_t *packet_count);

#endif /* ESWITCH_PIPELINE_H */
