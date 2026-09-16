#ifndef ESW_ROUTER_HW_H
#define ESW_ROUTER_HW_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "router.h"
#include "router_neighbor.h"

#define ROUTER_HW_MAX_ROUTES (ROUTER_MAX_NEIGHBORS + ROUTER_MAX_ROUTES)

/* A resolved, hardware-safe route. Public port-links are deliberately absent:
 * packets that may require NAT remain on the Arm dataplane. */
struct router_hw_route {
  uint16_t vr_id;
  uint16_t egress_interface_id;
  uint16_t egress_vswitch_id;
  uint16_t target_port_id;
  uint32_t prefix; /* host byte order */
  uint8_t length;
  uint8_t source_mac[6];
  uint8_t destination_mac[6];
};

size_t router_hw_routes_build(const struct router_config *config,
                              const struct router_neighbor_table *neighbors,
                              uint64_t now_ns,
                              struct router_hw_route *routes,
                              size_t capacity);

/* Return true when two control-plane configurations produce exactly the same
 * hardware-safe route set for the current adjacency table. Arm-only routes
 * (uplink/NAT and router-link) are intentionally invisible to this result. */
bool router_hw_route_plans_equal(
    const struct router_config *left, const struct router_config *right,
    const struct router_neighbor_table *neighbors, uint64_t now_ns);

#endif /* ESW_ROUTER_HW_H */
