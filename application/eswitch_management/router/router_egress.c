#include "router_egress.h"

#include <string.h>
#include <stdlib.h>
#include <netinet/in.h>

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

static enum router_egress_verdict firewall_check(
    const struct router_config *config, const struct router_interface *ingress,
    const uint8_t *frame, size_t length, bool public_ingress) {
  const struct router_egress_policy *policy;
  const struct router_egress_rule *best = NULL;
  const uint8_t *ip, *l4;
  uint32_t source, destination;
  uint16_t total_length, fragment, destination_port = 0;
  size_t ip_header_length, l4_length;
  uint8_t protocol;

  if (config == NULL || ingress == NULL || frame == NULL)
    return ROUTER_EGRESS_NOT_APPLICABLE;
  const struct router_egress_policy *policies = public_ingress ? config->ingress_policies : config->egress_policies;
  const struct router_egress_rule *rules = public_ingress ? config->ingress_rules : config->egress_rules;
  size_t policy_count = public_ingress ? config->ingress_policy_count : config->egress_policy_count;
  size_t rule_count = public_ingress ? config->ingress_rule_count : config->egress_rule_count;
  policy = NULL;
  for (size_t i=0;i<policy_count;i++)
    if(policies[i].vr_id==ingress->vr_id && policies[i].interface_id==ingress->interface_id)
      policy=&policies[i];
  if ((!public_ingress && ingress->attachment != ROUTER_VSWITCH) || policy == NULL ||
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
  for (size_t i = 0; !public_ingress && i < config->interface_count; i++) {
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
  for (size_t i = 0; i < rule_count; i++) {
    const struct router_egress_rule *rule = &rules[i];
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

enum router_egress_verdict router_egress_check(
    const struct router_config *config, const struct router_interface *ingress,
    const uint8_t *frame, size_t length) {
  return firewall_check(config, ingress, frame, length, false);
}
enum router_egress_verdict router_ingress_check(
    const struct router_config *config, const struct router_interface *ingress,
    const uint8_t *frame, size_t length) {
  return firewall_check(config, ingress, frame, length, true);
}

size_t router_ingress_l4_deny_ranges(
    const struct router_config *config, const struct router_interface *rif,
    uint8_t protocol, struct router_tcp_port_range *ranges, size_t capacity) {
  uint8_t *excluded;
  bool default_deny = false;
  size_t count = 0;
  if (config == NULL || rif == NULL || ranges == NULL || capacity == 0 ||
      rif->attachment != ROUTER_VSWITCH || !rif->has_address ||
      (protocol != IPPROTO_TCP && protocol != IPPROTO_UDP))
    return 0;
  for (size_t i = 0; i < config->ingress_policy_count; i++)
    if (config->ingress_policies[i].vr_id == rif->vr_id &&
        config->ingress_policies[i].interface_id == rif->interface_id)
      default_deny = !config->ingress_policies[i].default_allow;
  if (!default_deny)
    return 0;
  excluded = calloc(65536, 1);
  if (excluded == NULL)
    return 0;
  /* Exclude every potentially allowed destination port, even if source,
   * destination or rule priority would narrow it. This over-approximates
   * allows, so no packet allowed by Arm can be dropped here. */
  for (size_t i = 0; i < config->ingress_rule_count; i++) {
    const struct router_egress_rule *rule = &config->ingress_rules[i];
    if (rule->vr_id != rif->vr_id ||
        rule->interface_id != rif->interface_id || !rule->allow ||
        rule->icmp_type >= 0 || rule->icmp_code >= 0 ||
        (rule->protocol != 0 && rule->protocol != protocol))
      continue;
    uint16_t first = rule->port_first ? rule->port_first : 0;
    uint16_t last = rule->port_first ? rule->port_last : UINT16_MAX;
    for (uint32_t port = first; port <= last; port++)
      excluded[port] = 1;
  }
  /* Arm reverse-NAT lookup precedes ingress policy. Exempt the entire NAT
   * allocation range, including Arm-only and not-yet-promoted sessions. */
  for (size_t i = 0; i < config->nat_policy_count; i++) {
    const struct router_nat_policy *nat = &config->nat_policies[i];
    if (nat->vr_id != rif->vr_id || nat->interface_id != rif->interface_id)
      continue;
    for (uint32_t port = nat->port_first; port <= nat->port_last; port++)
      excluded[port] = 1;
  }
  for (uint32_t port = 0; port <= UINT16_MAX;) {
    uint32_t first;
    if (excluded[port]) { port++; continue; }
    first = port;
    while (port <= UINT16_MAX && !excluded[port]) port++;
    if (count == capacity) { free(excluded); return 0; }
    ranges[count++] = (struct router_tcp_port_range){first, port - 1};
  }
  free(excluded);
  return count;
}

size_t router_ingress_tcp_deny_ranges(
    const struct router_config *config, const struct router_interface *rif,
    struct router_tcp_port_range *ranges, size_t capacity) {
  return router_ingress_l4_deny_ranges(config, rif, IPPROTO_TCP,
                                       ranges, capacity);
}

static bool append_u8_prefixes(struct router_icmp_deny_match *matches,
                               size_t capacity, size_t *count,
                               uint8_t first, uint8_t last,
                               bool type_dimension, uint8_t fixed) {
  uint32_t cursor = first;
  while (cursor <= last) {
    uint32_t block = cursor == 0 ? 256U : cursor & (0U - cursor);
    uint32_t remaining = (uint32_t)last - cursor + 1U;
    uint8_t mask;
    while (block > remaining)
      block >>= 1;
    if (*count == capacity)
      return false;
    mask = (uint8_t)~(block - 1U);
    if (type_dimension) {
      matches[*count] = (struct router_icmp_deny_match){
          .type = (uint8_t)cursor, .type_mask = mask};
    } else {
      matches[*count] = (struct router_icmp_deny_match){
          .type = fixed, .type_mask = UINT8_MAX,
          .code = (uint8_t)cursor, .code_mask = mask};
    }
    (*count)++;
    cursor += block;
  }
  return true;
}

size_t router_ingress_icmp_deny_matches(
    const struct router_config *config, const struct router_interface *rif,
    struct router_icmp_deny_match *matches, size_t capacity) {
  bool allowed[256][256] = {{false}};
  bool default_deny = false;
  bool full_deny[256] = {false};
  size_t count = 0;

  if (config == NULL || rif == NULL || matches == NULL || capacity == 0 ||
      rif->attachment != ROUTER_VSWITCH || !rif->has_address)
    return 0;
  for (size_t i = 0; i < config->ingress_policy_count; i++)
    if (config->ingress_policies[i].vr_id == rif->vr_id &&
        config->ingress_policies[i].interface_id == rif->interface_id)
      default_deny = !config->ingress_policies[i].default_allow;
  if (!default_deny)
    return 0;
  /* Ignore CIDR and priority restrictions when collecting possible allows.
   * This can preserve extra traffic for Arm, but can never over-drop. */
  for (size_t i = 0; i < config->ingress_rule_count; i++) {
    const struct router_egress_rule *rule = &config->ingress_rules[i];
    uint16_t type_first, type_last, code_first, code_last;
    if (rule->vr_id != rif->vr_id ||
        rule->interface_id != rif->interface_id || !rule->allow ||
        rule->port_first != 0 ||
        (rule->protocol != 0 && rule->protocol != IPPROTO_ICMP))
      continue;
    type_first = rule->icmp_type >= 0 ? (uint8_t)rule->icmp_type : 0;
    type_last = rule->icmp_type >= 0 ? (uint8_t)rule->icmp_type : UINT8_MAX;
    code_first = rule->icmp_code >= 0 ? (uint8_t)rule->icmp_code : 0;
    code_last = rule->icmp_code >= 0 ? (uint8_t)rule->icmp_code : UINT8_MAX;
    for (uint16_t type = type_first; type <= type_last; type++)
      for (uint16_t code = code_first; code <= code_last; code++)
        allowed[type][code] = true;
  }
  for (uint16_t type = 0; type <= UINT8_MAX; type++) {
    full_deny[type] = true;
    for (uint16_t code = 0; code <= UINT8_MAX; code++)
      if (allowed[type][code]) {
        full_deny[type] = false;
        break;
      }
  }
  for (uint16_t type = 0; type <= UINT8_MAX;) {
    if (full_deny[type]) {
      uint16_t first = type;
      while (type <= UINT8_MAX && full_deny[type])
        type++;
      if (!append_u8_prefixes(matches, capacity, &count,
                              (uint8_t)first, (uint8_t)(type - 1), true, 0))
        return 0;
      continue;
    }
    for (uint16_t code = 0; code <= UINT8_MAX;) {
      uint16_t first;
      if (allowed[type][code]) {
        code++;
        continue;
      }
      first = code;
      while (code <= UINT8_MAX && !allowed[type][code])
        code++;
      if (!append_u8_prefixes(matches, capacity, &count,
                              (uint8_t)first, (uint8_t)(code - 1), false,
                              (uint8_t)type))
        return 0;
    }
    type++;
  }
  return count;
}
