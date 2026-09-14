#include "router_hw.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void add_rif(struct router_config *config, uint16_t vr, uint16_t id,
                    uint16_t vs, uint32_t address, uint8_t prefix,
                    uint8_t mac_last) {
  struct router_interface *rif = &config->interfaces[config->interface_count++];
  *rif = (struct router_interface){.vr_id = vr, .interface_id = id,
      .attachment = ROUTER_VSWITCH, .vswitch_id = vs, .has_address = true,
      .address = address, .prefix = prefix};
  rif->mac[0] = 2;
  rif->mac[5] = mac_last;
}

static void add_neighbor(struct router_neighbor_table *table, size_t slot,
                         uint16_t vr, uint16_t rif, uint16_t port,
                         uint32_t ip, uint8_t mac_last, uint64_t seen) {
  struct router_neighbor *neighbor = &table->entries[slot];
  *neighbor = (struct router_neighbor){.used = true, .resolved = true,
      .vr_id = vr, .interface_id = rif, .port_id = port, .ip = ip,
      .last_seen_ns = seen};
  neighbor->mac[0] = 2;
  neighbor->mac[5] = mac_last;
  table->count++;
}

int main(void) {
  struct router_config config;
  struct router_neighbor_table neighbors = {0};
  struct router_hw_route routes[ROUTER_HW_MAX_ROUTES];
  const uint64_t now = UINT64_C(1000000000);
  size_t count;

  router_config_init(&config);
  add_rif(&config, 101, 1, 100, 0xc0a80001U, 24, 1);
  add_rif(&config, 101, 2, 200, 0xc0a80a01U, 24, 2);
  add_rif(&config, 202, 3, 300, 0x0a000001U, 24, 3);
  add_neighbor(&neighbors, 0, 101, 1, 4, 0xc0a80032U, 0x32, now);
  add_neighbor(&neighbors, 1, 101, 2, 3, 0xc0a80a0aU, 0x0a, now);
  add_neighbor(&neighbors, 2, 202, 3, 8, 0x0a000032U, 0x50, now);

  count = router_hw_routes_build(&config, &neighbors, now, routes,
                                 ROUTER_HW_MAX_ROUTES);
  assert(count == 3);
  assert(routes[0].vr_id == 101 && routes[0].prefix == 0xc0a80032U);
  assert(routes[1].vr_id == 101 && routes[1].target_port_id == 3);
  assert(routes[2].vr_id == 202 && routes[2].target_port_id == 8);

  /* A private static route is promoted only after its gateway resolves. */
  config.routes[config.route_count++] = (struct router_route){
      .vr_id = 101, .interface_id = 2, .prefix = 0xac100000U,
      .gateway = 0xc0a80a0aU, .length = 16};
  count = router_hw_routes_build(&config, &neighbors, now, routes,
                                 ROUTER_HW_MAX_ROUTES);
  assert(count == 4);
  assert(routes[3].prefix == 0xac100000U && routes[3].length == 16);

  /* A more-specific static route wins over the connected subnet exactly as
   * the software LPM does; do not also install a destination /32. */
  config.routes[config.route_count++] = (struct router_route){
      .vr_id = 101, .interface_id = 1, .prefix = 0xc0a80a00U,
      .gateway = 0xc0a80032U, .length = 25};
  count = router_hw_routes_build(&config, &neighbors, now, routes,
                                 ROUTER_HW_MAX_ROUTES);
  assert(count == 4);
  bool found_static = false;
  bool found_wrong_connected = false;
  for (size_t i = 0; i < count; i++) {
    found_static |= routes[i].prefix == 0xc0a80a00U &&
                    routes[i].length == 25 && routes[i].target_port_id == 4;
    found_wrong_connected |= routes[i].prefix == 0xc0a80a0aU &&
                             routes[i].length == 32;
  }
  assert(found_static && !found_wrong_connected);

  /* Neighbor invalidation is what makes a VF detach fail closed. */
  router_neighbor_invalidate_port(&neighbors, 4);
  count = router_hw_routes_build(&config, &neighbors, now, routes,
                                 ROUTER_HW_MAX_ROUTES);
  assert(count == 2);
  add_neighbor(&neighbors, 0, 101, 1, 4, 0xc0a80032U, 0x33, now);
  count = router_hw_routes_build(&config, &neighbors, now, routes,
                                 ROUTER_HW_MAX_ROUTES);
  found_static = false;
  for (size_t i = 0; i < count; i++)
    if (routes[i].prefix == 0xc0a80a00U && routes[i].length == 25) {
      found_static = true;
      assert(routes[i].destination_mac[5] == 0x33);
    }
  assert(found_static);

  /* Public links and expired adjacencies never enter the hardware plan. */
  config.interfaces[1].attachment = ROUTER_PORT;
  count = router_hw_routes_build(&config, &neighbors, now, routes,
                                 ROUTER_HW_MAX_ROUTES);
  assert(count == 3);
  count = router_hw_routes_build(&config, &neighbors,
      now + ROUTER_NEIGHBOR_REACHABLE_NS + 1, routes, ROUTER_HW_MAX_ROUTES);
  assert(count == 0);

  puts("PASS: hardware route eligibility, VR isolation, private adjacency and aging");
  return 0;
}
