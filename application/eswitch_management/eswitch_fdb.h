#ifndef ESWITCH_FDB_H
#define ESWITCH_FDB_H

#include <stddef.h>
#include <stdint.h>

#include <doca_error.h>
#include <rte_ether.h>

#include "eswitch_pipeline.h"

enum eswitch_fdb_entry_state {
  ESWITCH_FDB_ENTRY_ACTIVE,
  ESWITCH_FDB_ENTRY_QUERY_RETRY,
  ESWITCH_FDB_ENTRY_DELETE_PENDING,
};

struct eswitch_fdb_entry {
  uint16_t vswitch_id;
  uint16_t vlan_id;
  struct rte_ether_addr mac;
  uint16_t learned_port_id;
  uint64_t last_seen_ns;
  uint64_t last_source_packets;
  uint64_t next_retry_ns;
  uint32_t query_retries;
  uint32_t delete_retries;
  enum eswitch_fdb_entry_state state;
  struct eswitch_hw_fdb_entry hardware;
  struct eswitch_fdb_entry *next;
};

struct eswitch_fdb {
  struct eswitch_pipeline *pipeline;
  struct eswitch_fdb_entry *head;
  size_t count;
  size_t capacity;
  uint64_t aging_ns;
};

doca_error_t eswitch_fdb_init(struct eswitch_pipeline *pipeline,
                              size_t capacity, uint32_t aging_seconds,
                              struct eswitch_fdb *fdb);
doca_error_t eswitch_fdb_learn(struct eswitch_fdb *fdb,
                               uint16_t vswitch_id, uint16_t vlan_id,
                               const struct rte_ether_addr *source,
                               uint16_t ingress_port_id, uint64_t now_ns);
doca_error_t eswitch_fdb_age(struct eswitch_fdb *fdb, uint64_t now_ns);
doca_error_t eswitch_fdb_flush_port(struct eswitch_fdb *fdb,
                                    uint16_t vswitch_id, uint16_t port_id,
                                    const char *reason);
doca_error_t eswitch_fdb_flush_vswitch(struct eswitch_fdb *fdb,
                                       uint16_t vswitch_id,
                                       const char *reason);
doca_error_t eswitch_fdb_destroy(struct eswitch_fdb *fdb);

/* Appends either every entry (vswitch_id=0) or one vSwitch to text. */
size_t eswitch_fdb_format(const struct eswitch_fdb *fdb,
                          uint16_t vswitch_id, char *buffer,
                          size_t buffer_size);

#endif /* ESWITCH_FDB_H */
