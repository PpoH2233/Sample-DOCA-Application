#ifndef ESWITCH_PIPELINE_H
#define ESWITCH_PIPELINE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <doca_error.h>
#include <doca_flow.h>
#include <rte_ether.h>

#include "../ethernet_switch/flow_ports.h"
#include "../ethernet_switch/flow_runtime.h"

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
  struct doca_flow_pipe *ingress_classifier_pipe;

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

doca_error_t eswitch_pipeline_create(struct flow_runtime *runtime,
                                     struct switch_flow_ports *ports,
                                     struct eswitch_pipeline *pipeline);
void eswitch_pipeline_destroy(struct eswitch_pipeline *pipeline);

doca_error_t eswitch_pipeline_attach_port(struct eswitch_pipeline *pipeline,
                                          uint16_t port_index,
                                          uint16_t vswitch_id);
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
