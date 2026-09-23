#include "router_neighbor.h"

#include <string.h>

#define ARP_FRAME_LEN 42U

static uint16_t read16(const uint8_t *p) {
  return (uint16_t)((uint16_t)p[0] << 8 | p[1]);
}

static uint32_t read32(const uint8_t *p) {
  return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 |
         (uint32_t)p[2] << 8 | p[3];
}

static uint32_t prefix_mask(uint8_t length) {
  return length == 0 ? 0 : UINT32_MAX << (32 - length);
}

static struct router_neighbor *find_neighbor(struct router_neighbor_table *table,
                                              uint16_t vr_id,
                                              uint16_t interface_id,
                                              uint32_t ip) {
  for (size_t i = 0; i < ROUTER_MAX_NEIGHBORS; i++) {
    struct router_neighbor *neighbor = &table->entries[i];

    if (neighbor->used && neighbor->vr_id == vr_id &&
        neighbor->interface_id == interface_id && neighbor->ip == ip)
      return neighbor;
  }
  return NULL;
}

static struct router_neighbor *allocate_neighbor(
    struct router_neighbor_table *table, uint16_t vr_id,
    uint16_t interface_id, uint32_t ip) {
  struct router_neighbor *neighbor = find_neighbor(table, vr_id, interface_id,
                                                    ip);

  if (neighbor != NULL)
    return neighbor;
  for (size_t i = 0; i < ROUTER_MAX_NEIGHBORS; i++) {
    if (table->entries[i].used)
      continue;
    neighbor = &table->entries[i];
    *neighbor = (struct router_neighbor){.used = true,
                                         .vr_id = vr_id,
                                         .interface_id = interface_id,
                                         .ip = ip};
    table->count++;
    return neighbor;
  }
  return NULL;
}

static bool learn_on_interface(struct router_neighbor_table *table,
                               const struct router_interface *rif,
                               uint16_t ingress_port_id,
                               const uint8_t *frame, size_t length,
                               uint64_t now_ns) {
  struct router_neighbor *neighbor;
  bool changed;
  uint32_t sender_ip;
  uint32_t mask;
  uint8_t nonzero = 0;

  if (table == NULL || rif == NULL || frame == NULL ||
      !rif->has_address || length < ARP_FRAME_LEN ||
      frame[12] != 0x08 || frame[13] != 0x06 || read16(frame + 14) != 1 ||
      read16(frame + 16) != 0x0800 || frame[18] != 6 || frame[19] != 4 ||
      (read16(frame + 20) != 1 && read16(frame + 20) != 2) ||
      memcmp(frame + 6, frame + 22, 6) != 0 || (frame[22] & 1U) != 0)
    return false;
  for (size_t i = 0; i < 6; i++)
    nonzero |= frame[22 + i];
  if (nonzero == 0)
    return false;

  sender_ip = read32(frame + 28);
  if (sender_ip == 0)
    return false; /* Do not create neighbors from RFC 5227 probes. */
  mask = prefix_mask(rif->prefix);
  if ((sender_ip & mask) != (rif->address & mask) || sender_ip == rif->address)
    return false;

  neighbor = allocate_neighbor(table, rif->vr_id, rif->interface_id,
                               sender_ip);
  if (neighbor == NULL)
    return false;
  changed = !neighbor->resolved || neighbor->port_id != ingress_port_id ||
            memcmp(neighbor->mac, frame + 22, 6) != 0;
  neighbor->resolved = true;
  neighbor->port_id = ingress_port_id;
  memcpy(neighbor->mac, frame + 22, 6);
  /* A broadcast-heavy segment can repeat the same gateway ARP thousands of
   * times per second. Refresh reachability at bounded frequency instead of
   * dirtying the neighbor cache line for every duplicate. */
  if (changed || neighbor->last_seen_ns == 0 ||
      now_ns - neighbor->last_seen_ns >= ROUTER_NEIGHBOR_REFRESH_NS)
    neighbor->last_seen_ns = now_ns;
  return changed;
}

bool router_neighbor_learn_arp(struct router_neighbor_table *table,
                               const struct router_config *config,
                               uint16_t ingress_vswitch_id,
                               uint16_t ingress_port_id,
                               const uint8_t *frame, size_t length,
                               uint64_t now_ns) {
  bool changed=false;
  uint32_t target_ip;

  if (!config || !ingress_vswitch_id || !frame || length<ARP_FRAME_LEN)
    return false;
  target_ip=read32(frame+38);
  for(size_t i=0;i<config->interface_count;i++) {
    const struct router_interface *rif=&config->interfaces[i];
    if(rif->attachment==ROUTER_VSWITCH &&
       rif->vswitch_id==ingress_vswitch_id && rif->has_address &&
       rif->address==target_ip)
      changed|=learn_on_interface(table,rif,ingress_port_id,frame,length,
                                  now_ns);
  }
  return changed;
}

bool router_neighbor_learn_arp_interface(
    struct router_neighbor_table *table, const struct router_config *config,
    uint16_t ingress_interface_id, uint16_t ingress_port_id,
    const uint8_t *frame, size_t length, uint64_t now_ns) {
  if(!config || !ingress_interface_id) return false;
  for(size_t i=0;i<config->interface_count;i++)
    if(config->interfaces[i].interface_id==ingress_interface_id)
      return learn_on_interface(table,&config->interfaces[i],ingress_port_id,
                                frame,length,now_ns);
  return false;
}

const struct router_neighbor *router_neighbor_lookup(
    const struct router_neighbor_table *table, uint16_t vr_id,
    uint16_t interface_id, uint32_t ip, uint64_t now_ns) {
  if (table == NULL)
    return NULL;
  for (size_t i = 0; i < ROUTER_MAX_NEIGHBORS; i++) {
    const struct router_neighbor *neighbor = &table->entries[i];

    if (neighbor->used && neighbor->resolved && neighbor->vr_id == vr_id &&
        neighbor->interface_id == interface_id && neighbor->ip == ip &&
        now_ns - neighbor->last_seen_ns <= ROUTER_NEIGHBOR_REACHABLE_NS)
      return neighbor;
  }
  return NULL;
}

bool router_neighbor_should_probe(struct router_neighbor_table *table,
                                  uint16_t vr_id, uint16_t interface_id,
                                  uint32_t ip, uint64_t now_ns) {
  struct router_neighbor *neighbor;

  if (table == NULL || vr_id == 0 || interface_id == 0 || ip == 0)
    return false;
  neighbor = allocate_neighbor(table, vr_id, interface_id, ip);
  if (neighbor == NULL)
    return false;
  if (neighbor->last_probe_ns != 0 &&
      now_ns - neighbor->last_probe_ns < ROUTER_NEIGHBOR_PROBE_NS)
    return false;
  neighbor->last_probe_ns = now_ns;
  return true;
}

bool router_neighbor_needs_refresh(const struct router_neighbor_table *table,
                                   uint16_t vr_id, uint16_t interface_id,
                                   uint32_t ip, uint64_t now_ns) {
  if (table == NULL || vr_id == 0 || interface_id == 0 || ip == 0)
    return false;
  for (size_t i = 0; i < ROUTER_MAX_NEIGHBORS; i++) {
    const struct router_neighbor *neighbor = &table->entries[i];

    if (!neighbor->used || neighbor->vr_id != vr_id ||
        neighbor->interface_id != interface_id || neighbor->ip != ip)
      continue;
    return !neighbor->resolved || neighbor->last_seen_ns == 0 ||
           now_ns - neighbor->last_seen_ns >= ROUTER_NEIGHBOR_REFRESH_DUE_NS;
  }
  return true;
}

void router_neighbor_age(struct router_neighbor_table *table, uint64_t now_ns) {
  if (table == NULL)
    return;
  for (size_t i = 0; i < ROUTER_MAX_NEIGHBORS; i++) {
    struct router_neighbor *neighbor = &table->entries[i];
    uint64_t reference;

    if (!neighbor->used)
      continue;
    reference = neighbor->resolved ? neighbor->last_seen_ns
                                   : neighbor->last_probe_ns;
    if (now_ns - reference <= ROUTER_NEIGHBOR_REACHABLE_NS)
      continue;
    *neighbor = (struct router_neighbor){0};
    table->count--;
  }
}

void router_neighbor_invalidate_port(struct router_neighbor_table *table,
                                     uint16_t port_id) {
  if (table == NULL)
    return;
  for (size_t i = 0; i < ROUTER_MAX_NEIGHBORS; i++) {
    if (!table->entries[i].used || table->entries[i].port_id != port_id)
      continue;
    table->entries[i] = (struct router_neighbor){0};
    table->count--;
  }
}
