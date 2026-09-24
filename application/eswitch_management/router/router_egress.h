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

#endif
