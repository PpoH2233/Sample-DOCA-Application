#include "router_hw.h"

#include <string.h>

static uint32_t prefix_mask(uint8_t length) {
  return length == 0 ? 0 : UINT32_MAX << (32 - length);
}

static const struct router_interface *interface_by_id(
    const struct router_config *config, uint16_t vr_id, uint16_t interface_id) {
  for (size_t i = 0; i < config->interface_count; i++) {
    const struct router_interface *rif = &config->interfaces[i];
    if (rif->vr_id == vr_id && rif->interface_id == interface_id)
      return rif;
  }
  return NULL;
}

static bool static_route_wins(const struct router_config *config,
                              const struct router_interface *connected,
                              uint32_t destination) {
  for (size_t i = 0; i < config->route_count; i++) {
    const struct router_route *route = &config->routes[i];
    if (route->vr_id == connected->vr_id &&
        route->length > connected->prefix &&
        (destination & prefix_mask(route->length)) == route->prefix)
      return true;
  }
  return false;
}

static bool append_route(struct router_hw_route *routes, size_t capacity,
                         size_t *count, const struct router_interface *egress,
                         const struct router_neighbor *neighbor,
                         uint32_t prefix, uint8_t length) {
  struct router_hw_route *route;

  if (*count >= capacity)
    return false;
  /* Keep the software connected-route tie-break rule. */
  for (size_t i = 0; i < *count; i++) {
    if (routes[i].vr_id == egress->vr_id && routes[i].prefix == prefix &&
        routes[i].length == length)
      return true;
  }
  route = &routes[(*count)++];
  *route = (struct router_hw_route){
      .vr_id = egress->vr_id,
      .egress_interface_id = egress->interface_id,
      .egress_vswitch_id = egress->vswitch_id,
      .target_port_id = neighbor->port_id,
      .prefix = prefix,
      .length = length,
  };
  memcpy(route->source_mac, egress->mac, sizeof(route->source_mac));
  memcpy(route->destination_mac, neighbor->mac,
         sizeof(route->destination_mac));
  return true;
}

size_t router_hw_routes_build(const struct router_config *config,
                              const struct router_neighbor_table *neighbors,
                              uint64_t now_ns,
                              struct router_hw_route *routes,
                              size_t capacity) {
  size_t count = 0;

  if (config == NULL || neighbors == NULL || routes == NULL || capacity == 0)
    return 0;

  /* Connected routes require a resolved destination adjacency, therefore they
   * are promoted as /32 entries rather than as the whole connected prefix. */
  for (size_t i = 0; i < ROUTER_MAX_NEIGHBORS; i++) {
    const struct router_neighbor *neighbor = &neighbors->entries[i];
    const struct router_interface *egress;

    if (!neighbor->used || !neighbor->resolved ||
        now_ns - neighbor->last_seen_ns > ROUTER_NEIGHBOR_REACHABLE_NS)
      continue;
    egress = interface_by_id(config, neighbor->vr_id,
                             neighbor->interface_id);
    if (egress == NULL || egress->attachment != ROUTER_VSWITCH ||
        !egress->has_address || static_route_wins(config, egress, neighbor->ip))
      continue;
    if (!append_route(routes, capacity, &count, egress, neighbor,
                      neighbor->ip, 32))
      return count;
  }

  /* A static prefix is safe to promote only after its configured next hop is
   * resolved on a private vSwitch RIF. Default/public/NAT paths stay on Arm. */
  for (size_t i = 0; i < config->route_count; i++) {
    const struct router_route *configured = &config->routes[i];
    const struct router_interface *egress = interface_by_id(
        config, configured->vr_id, configured->interface_id);
    const struct router_neighbor *neighbor;

    if (egress == NULL || egress->attachment != ROUTER_VSWITCH ||
        !egress->has_address)
      continue;
    neighbor = router_neighbor_lookup(neighbors, configured->vr_id,
                                      configured->interface_id,
                                      configured->gateway, now_ns);
    if (neighbor == NULL)
      continue;
    if (!append_route(routes, capacity, &count, egress, neighbor,
                      configured->prefix, configured->length))
      return count;
  }
  return count;
}

static bool route_spec_equal(const struct router_hw_route *left,
                             const struct router_hw_route *right) {
  return left->vr_id == right->vr_id &&
         left->egress_interface_id == right->egress_interface_id &&
         left->egress_vswitch_id == right->egress_vswitch_id &&
         left->target_port_id == right->target_port_id &&
         left->prefix == right->prefix && left->length == right->length &&
         memcmp(left->source_mac, right->source_mac,
                sizeof(left->source_mac)) == 0 &&
         memcmp(left->destination_mac, right->destination_mac,
                sizeof(left->destination_mac)) == 0;
}

bool router_hw_route_plans_equal(
    const struct router_config *left, const struct router_config *right,
    const struct router_neighbor_table *neighbors, uint64_t now_ns) {
  struct router_hw_route left_routes[ROUTER_HW_MAX_ROUTES];
  struct router_hw_route right_routes[ROUTER_HW_MAX_ROUTES];
  bool matched[ROUTER_HW_MAX_ROUTES] = {0};
  size_t left_count, right_count;

  if (left == NULL || right == NULL || neighbors == NULL)
    return false;
  left_count = router_hw_routes_build(left, neighbors, now_ns, left_routes,
                                      ROUTER_HW_MAX_ROUTES);
  right_count = router_hw_routes_build(right, neighbors, now_ns, right_routes,
                                       ROUTER_HW_MAX_ROUTES);
  if (left_count != right_count)
    return false;
  /* Treat the route plan as a set. Config serialization order is not a
   * dataplane change and must not force an HWS transaction. */
  for (size_t i = 0; i < left_count; i++) {
    bool found = false;
    for (size_t j = 0; j < right_count; j++) {
      if (!matched[j] && route_spec_equal(&left_routes[i],
                                           &right_routes[j])) {
        matched[j] = true;
        found = true;
        break;
      }
    }
    if (!found)
      return false;
  }
  return true;
}
