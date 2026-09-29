#ifndef ESW_ROUTER_EGRESS_H
#define ESW_ROUTER_EGRESS_H

#include "router.h"

enum router_egress_verdict {
  ROUTER_EGRESS_NOT_APPLICABLE,
  ROUTER_EGRESS_ALLOW,
  ROUTER_EGRESS_DENY,
};

/* Inspect the original, pre-NAT guest frame. Only an addressed guest RIF
 * with a configured policy is in scope; router-local IPv4 stays reachable. */
enum router_egress_verdict router_egress_check(
    const struct router_config *config, const struct router_interface *ingress,
    const uint8_t *frame, size_t length);
enum router_egress_verdict router_ingress_check(
    const struct router_config *config, const struct router_interface *ingress,
    const uint8_t *frame, size_t length);

struct router_tcp_port_range { uint16_t first, last; };
/* Conservative subset of default-deny ingress policy: TCP destination ports
 * that cannot match an allow rule or an outbound NAT reverse session. Zero
 * means no safe hardware offload, never permission to drop everything. */
size_t router_ingress_tcp_deny_ranges(
    const struct router_config *config, const struct router_interface *rif,
    struct router_tcp_port_range *ranges, size_t capacity);

#endif
