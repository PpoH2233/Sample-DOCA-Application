#ifndef ESW_ROUTER_NEIGHBOR_H
#define ESW_ROUTER_NEIGHBOR_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "router.h"

#define ROUTER_MAX_NEIGHBORS 512U
#define ROUTER_NEIGHBOR_REACHABLE_NS UINT64_C(300000000000)
#define ROUTER_NEIGHBOR_PROBE_NS UINT64_C(1000000000)

struct router_neighbor {
  bool used;
  bool resolved;
  uint16_t vr_id;
  uint16_t interface_id;
  uint16_t port_id;
  uint32_t ip;
  uint8_t mac[6];
  uint64_t last_seen_ns;
  uint64_t last_probe_ns;
};

struct router_neighbor_table {
  struct router_neighbor entries[ROUTER_MAX_NEIGHBORS];
  size_t count;
};

/* Learn an on-link sender from a validated Ethernet/IPv4 ARP packet. */
bool router_neighbor_learn_arp(struct router_neighbor_table *table,
                               const struct router_config *config,
                               uint16_t ingress_vswitch_id,
                               uint16_t ingress_port_id,
                               const uint8_t *frame, size_t length,
                               uint64_t now_ns);

bool router_neighbor_learn_arp_interface(
    struct router_neighbor_table *table, const struct router_config *config,
    uint16_t ingress_interface_id, uint16_t ingress_port_id,
    const uint8_t *frame, size_t length, uint64_t now_ns);

const struct router_neighbor *router_neighbor_lookup(
    const struct router_neighbor_table *table, uint16_t vr_id,
    uint16_t interface_id, uint32_t ip, uint64_t now_ns);

/* Returns true once per probe interval and creates a pending slot if needed. */
bool router_neighbor_should_probe(struct router_neighbor_table *table,
                                  uint16_t vr_id, uint16_t interface_id,
                                  uint32_t ip, uint64_t now_ns);

void router_neighbor_age(struct router_neighbor_table *table, uint64_t now_ns);

#endif /* ESW_ROUTER_NEIGHBOR_H */
