#ifndef L2_PIPELINE_H
#define L2_PIPELINE_H

#include <stdbool.h>
#include <stdint.h>

#include <doca_error.h>
#include <doca_flow.h>
#include <rte_ether.h>

#include "flow_runtime.h"
#include "flow_ports.h"

struct l2_fdb_entry;

struct l2_static_rule {
  struct doca_flow_pipe_entry *entry;
  struct flow_entry_cookie cookie;
};

struct l2_pipeline {
  struct flow_runtime *runtime;
  struct switch_flow_ports *ports;
  struct doca_flow_port *switch_port;

  struct doca_flow_pipe *rss_pipe;
  struct doca_flow_pipe *destination_pipe;
  struct doca_flow_pipe *learning_clone_pipe;
  struct doca_flow_pipe *learning_dispatch_pipe;
  struct doca_flow_pipe **flood_pipes;
  struct doca_flow_pipe *flood_selector_pipe;
  struct doca_flow_pipe *source_guard_pipe;
  struct doca_flow_pipe *ingress_classifier_pipe;

  struct l2_static_rule rss_rule;
  struct l2_static_rule learning_clone_rules[2];
  struct l2_static_rule learning_dispatch_rule;
  struct l2_static_rule *flood_rules;
  uint32_t flood_rule_count;
  struct l2_static_rule *selector_rules;
  struct l2_static_rule *ingress_rules;
  bool created;
};

/*
 * Creates this hardware graph:
 *
 * ingress(root) -> source guard --hit--> destination FDB --hit--> port
 *                         | miss                 | miss
 *                         v                      v
 *                 learning dispatch      ingress-specific flood
 *                         |
 *                         v
 *                  clone {RSS, FDB}
 *
 * Example after learning 02:00:00:00:00:0a on DPDK port 1:
 *   source: (ingress=1, src=02:...:0a) -> destination pipe
 *   dest:   (ingress=0, dst=02:...:0a) -> port 1
 *   dest:   (ingress=1, dst=02:...:0a) -> DROP
 */
doca_error_t l2_pipeline_create(struct flow_runtime *runtime,
                                struct switch_flow_ports *ports,
                                struct l2_pipeline *pipeline);
void l2_pipeline_destroy(struct l2_pipeline *pipeline);

/* Dynamic hardware FDB operations, called only by the control thread. */
doca_error_t l2_pipeline_add_fdb_entry(struct l2_pipeline *pipeline,
                                       struct l2_fdb_entry *entry);
doca_error_t l2_pipeline_move_fdb_entry(struct l2_pipeline *pipeline,
                                        struct l2_fdb_entry *entry,
                                        uint16_t new_port_id);
doca_error_t l2_pipeline_remove_fdb_entry(struct l2_pipeline *pipeline,
                                          struct l2_fdb_entry *entry);
doca_error_t l2_pipeline_query_source_counter(
    const struct l2_fdb_entry *entry,
    uint64_t *packet_count);

#endif /* L2_PIPELINE_H */
