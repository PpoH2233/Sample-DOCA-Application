#ifndef ESW_ROUTER_FORWARD_H
#define ESW_ROUTER_FORWARD_H

#include <stddef.h>
#include <stdint.h>

#include "router.h"

enum router_ipv4_disposition {
  ROUTER_IPV4_NOT_FOR_ROUTER = 0,
  ROUTER_IPV4_INVALID,
  ROUTER_IPV4_LOCAL,
  ROUTER_IPV4_TTL_EXPIRED,
  ROUTER_IPV4_NO_ROUTE,
  ROUTER_IPV4_FORWARD,
};

struct router_ipv4_decision {
  enum router_ipv4_disposition disposition;
  uint16_t vr_id;
  uint16_t ingress_interface_id;
  uint16_t egress_interface_id;
  uint16_t egress_vswitch_id;
  uint32_t source_ip;
  uint32_t destination_ip;
  uint32_t next_hop_ip;
  uint8_t prefix_length;
  bool connected;
};

/* Parse an untagged IPv4 frame addressed to the ingress RIF and perform a
 * longest-prefix lookup over connected and configured static routes. */
enum router_ipv4_disposition router_ipv4_lookup(
    const struct router_config *config, uint16_t ingress_vswitch_id,
    const uint8_t *frame, size_t length, struct router_ipv4_decision *decision);

/* Same lookup for traffic arriving on a router-owned public port. This is
 * used after reverse NAT changes the destination back to an inside address. */
enum router_ipv4_disposition router_ipv4_lookup_interface(
    const struct router_config *config, uint16_t ingress_interface_id,
    const uint8_t *frame, size_t length, struct router_ipv4_decision *decision);

/* Apply the IPv4 forwarding mutations after neighbor resolution. */
size_t router_ipv4_rewrite(const uint8_t *frame, size_t length,
                           const uint8_t source_mac[6],
                           const uint8_t destination_mac[6],
                           uint8_t *output, size_t capacity);

#endif /* ESW_ROUTER_FORWARD_H */
