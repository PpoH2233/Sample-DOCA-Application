#ifndef L2_FDB_H
#define L2_FDB_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <doca_error.h>
#include <doca_flow.h>
#include <rte_ether.h>

#include "flow_runtime.h"

struct l2_pipeline;

enum l2_fdb_state {
  L2_FDB_PENDING,
  L2_FDB_ACTIVE,
  L2_FDB_MOVING,
  L2_FDB_DELETING,
};

struct l2_dynamic_rule {
  struct doca_flow_pipe_entry *entry;
  struct flow_entry_cookie cookie;
};

/* Heap allocated: its address and callback cookies remain stable. */
struct l2_fdb_entry {
  struct rte_ether_addr mac;
  uint16_t vlan_id;
  uint16_t learned_port_id;
  uint64_t last_seen_ns;
  uint64_t last_source_packets;
  enum l2_fdb_state state;

  struct l2_dynamic_rule source_rule;
  struct l2_dynamic_rule *destination_rules;
  uint16_t destination_rule_count;
  struct l2_fdb_entry *next;
};

struct l2_fdb {
  struct l2_pipeline *pipeline;
  struct l2_fdb_entry *head;
  size_t count;
  size_t capacity;
  uint64_t aging_ns;
};

doca_error_t l2_fdb_init(struct l2_pipeline *pipeline,
                         size_t capacity,
                         uint32_t aging_seconds,
                         struct l2_fdb *fdb);

/*
 * Learn or move one unicast source. vlan_id=0 is the untagged bridge.
 *
 * Example output:
 *   FDB learn: 02:00:00:00:00:0a -> DPDK port 1 (entries=1)
 *   FDB move:  02:00:00:00:00:0a port 1 -> port 2
 */
doca_error_t l2_fdb_learn(struct l2_fdb *fdb,
                          uint16_t vlan_id,
                          const struct rte_ether_addr *source,
                          uint16_t ingress_port_id,
                          uint64_t now_ns);

/* Query source-rule counters, refresh active entries, remove stale entries. */
doca_error_t l2_fdb_age(struct l2_fdb *fdb, uint64_t now_ns);

void l2_fdb_print(const struct l2_fdb *fdb);
doca_error_t l2_fdb_destroy(struct l2_fdb *fdb);

#endif /* L2_FDB_H */
