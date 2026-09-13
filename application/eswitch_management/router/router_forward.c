#include "router_forward.h"

#include <stdbool.h>
#include <string.h>

#define ETH_HEADER_LEN 14U
#define IPV4_MIN_HEADER_LEN 20U
#define ETHERNET_MIN_FRAME_NO_FCS 60U

static uint16_t read16(const uint8_t *p) {
  return (uint16_t)((uint16_t)p[0] << 8 | p[1]);
}

static uint32_t read32(const uint8_t *p) {
  return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 |
         (uint32_t)p[2] << 8 | p[3];
}

static void write16(uint8_t *p, uint16_t value) {
  p[0] = (uint8_t)(value >> 8);
  p[1] = (uint8_t)value;
}

static uint16_t checksum(const uint8_t *bytes, size_t length) {
  uint32_t sum = 0;

  while (length >= 2) {
    sum += read16(bytes);
    bytes += 2;
    length -= 2;
  }
  if (length != 0)
    sum += (uint16_t)bytes[0] << 8;
  while ((sum >> 16) != 0)
    sum = (sum & UINT16_MAX) + (sum >> 16);
  return (uint16_t)~sum;
}

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

enum router_ipv4_disposition router_ipv4_lookup(
    const struct router_config *config, uint16_t ingress_vswitch_id,
    const uint8_t *frame, size_t length,
    struct router_ipv4_decision *decision) {
  const struct router_interface *ingress = NULL;
  const struct router_interface *egress = NULL;
  const uint8_t *ip;
  size_t ip_header_length;
  uint16_t ip_total_length;
  uint32_t destination;
  uint8_t best_length = 0;
  bool have_route = false;
  bool best_connected = false;
  uint32_t best_next_hop = 0;

  if (decision != NULL)
    *decision = (struct router_ipv4_decision){0};
  if (config == NULL || frame == NULL || decision == NULL ||
      ingress_vswitch_id == 0 || length < ETH_HEADER_LEN + IPV4_MIN_HEADER_LEN)
    return ROUTER_IPV4_INVALID;
  if (frame[12] != 0x08 || frame[13] != 0x00)
    return ROUTER_IPV4_NOT_FOR_ROUTER;

  for (size_t i = 0; i < config->interface_count; i++) {
    const struct router_interface *candidate = &config->interfaces[i];

    if (candidate->attachment == ROUTER_VSWITCH && candidate->has_address &&
        candidate->vswitch_id == ingress_vswitch_id &&
        memcmp(frame, candidate->mac, 6) == 0) {
      ingress = candidate;
      break;
    }
  }
  if (ingress == NULL)
    return ROUTER_IPV4_NOT_FOR_ROUTER;

  ip = frame + ETH_HEADER_LEN;
  if ((ip[0] >> 4) != 4 || (ip[0] & 0x0fU) < 5)
    return ROUTER_IPV4_INVALID;
  ip_header_length = (size_t)(ip[0] & 0x0fU) * 4U;
  if (length < ETH_HEADER_LEN + ip_header_length ||
      checksum(ip, ip_header_length) != 0)
    return ROUTER_IPV4_INVALID;
  ip_total_length = read16(ip + 2);
  if (ip_total_length < ip_header_length ||
      ip_total_length > length - ETH_HEADER_LEN)
    return ROUTER_IPV4_INVALID;

  decision->vr_id = ingress->vr_id;
  decision->ingress_interface_id = ingress->interface_id;
  decision->source_ip = read32(ip + 12);
  decision->destination_ip = read32(ip + 16);
  destination = decision->destination_ip;
  if ((destination >> 28) == 0x0e || destination == UINT32_MAX)
    return decision->disposition = ROUTER_IPV4_NO_ROUTE;

  for (size_t i = 0; i < config->interface_count; i++) {
    const struct router_interface *candidate = &config->interfaces[i];

    if (candidate->vr_id == ingress->vr_id && candidate->has_address &&
        candidate->address == destination)
      return decision->disposition = ROUTER_IPV4_LOCAL;
  }
  if (ip[8] <= 1)
    return decision->disposition = ROUTER_IPV4_TTL_EXPIRED;

  /* Connected routes participate in the same LPM decision as static routes.
   * A connected route wins an equal-length tie. */
  for (size_t i = 0; i < config->interface_count; i++) {
    const struct router_interface *candidate = &config->interfaces[i];
    uint32_t mask;

    if (candidate->vr_id != ingress->vr_id || !candidate->has_address ||
        candidate->attachment != ROUTER_VSWITCH)
      continue;
    mask = prefix_mask(candidate->prefix);
    if ((destination & mask) != (candidate->address & mask))
      continue;
    if (!have_route || candidate->prefix > best_length ||
        (candidate->prefix == best_length && !best_connected)) {
      have_route = true;
      best_connected = true;
      best_length = candidate->prefix;
      best_next_hop = destination;
      egress = candidate;
    }
  }
  for (size_t i = 0; i < config->route_count; i++) {
    const struct router_route *route = &config->routes[i];
    uint32_t mask;

    if (route->vr_id != ingress->vr_id)
      continue;
    mask = prefix_mask(route->length);
    if ((destination & mask) != route->prefix ||
        (have_route && route->length <= best_length))
      continue;
    egress = interface_by_id(config, ingress->vr_id, route->interface_id);
    if (egress == NULL || !egress->has_address ||
        egress->attachment != ROUTER_VSWITCH)
      continue;
    have_route = true;
    best_connected = false;
    best_length = route->length;
    best_next_hop = route->gateway;
  }
  if (!have_route || egress == NULL)
    return decision->disposition = ROUTER_IPV4_NO_ROUTE;

  decision->egress_interface_id = egress->interface_id;
  decision->egress_vswitch_id = egress->vswitch_id;
  decision->next_hop_ip = best_next_hop;
  decision->prefix_length = best_length;
  decision->connected = best_connected;
  return decision->disposition = ROUTER_IPV4_FORWARD;
}

size_t router_ipv4_rewrite(const uint8_t *frame, size_t length,
                           const uint8_t source_mac[6],
                           const uint8_t destination_mac[6],
                           uint8_t *output, size_t capacity) {
  const uint8_t *ip;
  uint8_t *output_ip;
  size_t ip_header_length;
  size_t frame_length;
  uint16_t ip_total_length;

  if (frame == NULL || source_mac == NULL || destination_mac == NULL ||
      output == NULL || length < ETH_HEADER_LEN + IPV4_MIN_HEADER_LEN ||
      frame[12] != 0x08 || frame[13] != 0x00)
    return 0;
  ip = frame + ETH_HEADER_LEN;
  if ((ip[0] >> 4) != 4 || (ip[0] & 0x0fU) < 5 || ip[8] <= 1)
    return 0;
  ip_header_length = (size_t)(ip[0] & 0x0fU) * 4U;
  if (length < ETH_HEADER_LEN + ip_header_length ||
      checksum(ip, ip_header_length) != 0)
    return 0;
  ip_total_length = read16(ip + 2);
  if (ip_total_length < ip_header_length ||
      ip_total_length > length - ETH_HEADER_LEN)
    return 0;
  frame_length = ETH_HEADER_LEN + ip_total_length;
  if (frame_length < ETHERNET_MIN_FRAME_NO_FCS)
    frame_length = ETHERNET_MIN_FRAME_NO_FCS;
  if (capacity < frame_length)
    return 0;

  memset(output, 0, frame_length);
  memcpy(output, destination_mac, 6);
  memcpy(output + 6, source_mac, 6);
  output[12] = 0x08;
  output[13] = 0x00;
  memcpy(output + ETH_HEADER_LEN, ip, ip_total_length);
  output_ip = output + ETH_HEADER_LEN;
  output_ip[8]--;
  output_ip[10] = 0;
  output_ip[11] = 0;
  write16(output_ip + 10, checksum(output_ip, ip_header_length));
  return frame_length;
}
