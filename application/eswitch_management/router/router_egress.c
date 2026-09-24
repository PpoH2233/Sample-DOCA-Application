#include "router_egress.h"

#include <string.h>

#define ETH_LEN 14U
#define IPV4_MIN_LEN 20U

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

enum router_egress_verdict router_egress_check(
    const struct router_config *config, const struct router_interface *ingress,
    const uint8_t *frame, size_t length) {
  const struct router_egress_policy *policy;
  const struct router_egress_rule *best = NULL;
  const uint8_t *ip, *l4;
  uint32_t source, destination;
  uint16_t total_length, fragment, destination_port = 0;
  size_t ip_header_length, l4_length;
  uint8_t protocol;

  if (config == NULL || ingress == NULL || frame == NULL ||
      ingress->attachment != ROUTER_VSWITCH ||
      (policy = router_egress_policy_find(config, ingress->vr_id,
                                           ingress->interface_id)) == NULL ||
      length < ETH_LEN || memcmp(frame, ingress->mac, 6) != 0 ||
      frame[12] != 0x08 || frame[13] != 0x00)
    return ROUTER_EGRESS_NOT_APPLICABLE;
  if (length < ETH_LEN + IPV4_MIN_LEN)
    return ROUTER_EGRESS_DENY;
  ip = frame + ETH_LEN;
  if ((ip[0] >> 4) != 4 || (ip[0] & 0x0fU) < 5)
    return ROUTER_EGRESS_DENY;
  ip_header_length = (size_t)(ip[0] & 0x0fU) * 4U;
  if (length < ETH_LEN + ip_header_length)
    return ROUTER_EGRESS_DENY;
  total_length = read16(ip + 2);
  if (total_length < ip_header_length || total_length > length - ETH_LEN)
    return ROUTER_EGRESS_DENY;
  destination = read32(ip + 16);
  /* Local VR services are not guest-network egress traffic. */
  for (size_t i = 0; i < config->interface_count; i++) {
    const struct router_interface *candidate = &config->interfaces[i];
    if (candidate->vr_id == ingress->vr_id && candidate->has_address &&
        candidate->address == destination)
      return ROUTER_EGRESS_NOT_APPLICABLE;
  }
  /* A later fragment has no L4 header; allowing it under a port rule would
   * be a firewall bypass. Until fragment tracking exists, fail closed. */
  fragment = read16(ip + 6);
  if ((fragment & 0x3fffU) != 0)
    return ROUTER_EGRESS_DENY;
  source = read32(ip + 12);
  protocol = ip[9];
  l4 = ip + ip_header_length;
  l4_length = total_length - ip_header_length;
  if ((protocol == 6 && l4_length < 20) ||
      (protocol == 17 && l4_length < 8) ||
      (protocol == 1 && l4_length < 4))
    return ROUTER_EGRESS_DENY;
  if (protocol == 6 || protocol == 17)
    destination_port = read16(l4 + 2);
  for (size_t i = 0; i < config->egress_rule_count; i++) {
    const struct router_egress_rule *rule = &config->egress_rules[i];
    if (rule->vr_id != ingress->vr_id ||
        rule->interface_id != ingress->interface_id ||
        (rule->protocol != 0 && rule->protocol != protocol) ||
        (source & prefix_mask(rule->source_prefix)) != rule->source ||
        (destination & prefix_mask(rule->destination_prefix)) !=
            rule->destination ||
        (rule->port_first && ((protocol != 6 && protocol != 17) ||
         destination_port < rule->port_first ||
         destination_port > rule->port_last)) ||
        (rule->icmp_type >= 0 &&
         (protocol != 1 || l4_length < 2 || l4[0] != rule->icmp_type)) ||
        (rule->icmp_code >= 0 &&
         (protocol != 1 || l4_length < 2 || l4[1] != rule->icmp_code)))
      continue;
    if (best == NULL || rule->rule_id < best->rule_id)
      best = rule;
  }
  if (best != NULL)
    return best->allow ? ROUTER_EGRESS_ALLOW : ROUTER_EGRESS_DENY;
  return policy->default_allow ? ROUTER_EGRESS_ALLOW : ROUTER_EGRESS_DENY;
}
