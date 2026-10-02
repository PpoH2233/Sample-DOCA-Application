#ifndef ROUTER_ACL_PLAN_H
#define ROUTER_ACL_PLAN_H

#include "router.h"

/* Project the ordered policy onto one supported L4 protocol. Rules for
 * other protocols cannot affect its first-match result. ICMP/other traffic
 * is dispatched to Arm, never to this TCP/UDP-only ACL. */
static inline bool router_acl_applies(const struct router_egress_rule *rule,
                                      uint8_t protocol) {
  return (protocol == 6 || protocol == 17) &&
         (rule->protocol == 0 || rule->protocol == protocol) &&
         rule->icmp_type < 0 && rule->icmp_code < 0;
}

/* Inclusive range -> disjoint ternary prefixes, valid for CONTROL (not
 * ACL range masks). Zero port_first denotes all ports in the CLI model. */
static inline uint32_t router_acl_port_block(uint32_t cursor, uint32_t last) {
  uint32_t block = cursor == 0 ? UINT32_C(65536) : cursor & (0U - cursor);
  while (block > last - cursor + 1U)
    block >>= 1;
  return block;
}

static inline size_t router_acl_port_count(const struct router_egress_rule *rule) {
  if (rule->port_first == 0)
    return 1;
  size_t count = 0;
  for (uint32_t cursor = rule->port_first; cursor <= rule->port_last;) {
    cursor += router_acl_port_block(cursor, rule->port_last);
    count++;
  }
  return count;
}

#endif
