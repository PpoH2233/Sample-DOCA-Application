#ifndef ESW_ROUTER_NAT_H
#define ESW_ROUTER_NAT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "router.h"

#define ROUTER_NAT_MAX_SESSIONS 4096U
#define ROUTER_NAT_TCP_IDLE_NS UINT64_C(300000000000)
#define ROUTER_NAT_UDP_IDLE_NS UINT64_C(60000000000)

enum router_nat_result {
  ROUTER_NAT_NOT_APPLICABLE = 0,
  ROUTER_NAT_TRANSLATED,
  ROUTER_NAT_UNSUPPORTED,
  ROUTER_NAT_INVALID,
  ROUTER_NAT_FULL,
};

struct router_nat_inside {
  uint16_t vswitch_id;
  uint16_t interface_id;
  uint16_t port_id;
  uint8_t mac[6];
};

struct router_nat_session {
  bool used;
  uint16_t vr_id;
  uint8_t protocol;
  uint32_t inside_ip;
  uint16_t inside_port;
  uint32_t remote_ip;
  uint16_t remote_port;
  uint32_t public_ip;
  uint16_t public_port;
  struct router_nat_inside inside;
  uint64_t created_ns;
  uint64_t last_seen_ns;
  uint64_t original_packets;
  uint64_t reply_packets;
};

struct router_nat_stats {
  uint64_t outbound_packets;
  uint64_t inbound_packets;
  uint64_t sessions_created;
  uint64_t sessions_aged;
  uint64_t port_allocation_failures;
  uint64_t reverse_misses;
  uint64_t unsupported_packets;
  uint64_t invalid_packets;
};

struct router_nat_table {
  struct router_nat_session entries[ROUTER_NAT_MAX_SESSIONS];
  size_t count;
  uint16_t next_port;
  struct router_nat_stats stats;
};

void router_nat_init(struct router_nat_table *table);

/* Translate one untagged IPv4 TCP/UDP packet. Both functions copy input to
 * output and recompute IPv4 and L4 checksums. All IP/port values stored in
 * sessions are host byte order. IPv4 fragments fail closed in this MVP. */
enum router_nat_result router_nat_outbound(
    struct router_nat_table *table, const struct router_nat_policy *policy,
    uint32_t public_ip, const struct router_nat_inside *inside,
    const uint8_t *frame, size_t length, uint64_t now_ns,
    uint8_t *output, size_t capacity,
    const struct router_nat_session **session);

enum router_nat_result router_nat_inbound(
    struct router_nat_table *table, uint16_t vr_id,
    const uint8_t *frame, size_t length, uint64_t now_ns,
    uint8_t *output, size_t capacity,
    const struct router_nat_session **session);

void router_nat_age(struct router_nat_table *table, uint64_t now_ns);
void router_nat_flush(struct router_nat_table *table, uint16_t vr_id);

#endif /* ESW_ROUTER_NAT_H */
