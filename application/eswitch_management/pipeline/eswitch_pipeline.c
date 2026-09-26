#include "eswitch_pipeline.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>

#include <doca_bitfield.h>
#include <doca_flow_ct.h>

#include "../../ethernet_switch/switch_config.h"
#include "../eswitch_config.h"

#define ESWITCH_MAX_FLOOD_MEMBERS 254U
#define ESWITCH_METADATA_VSWITCH_MASK UINT32_C(0xffff0000)
#define ESWITCH_METADATA_PORT_MASK UINT32_C(0x0000ffff)

static doca_error_t get_egress_gate(struct eswitch_pipeline *pipeline,
                                    uint16_t vswitch_id,
                                    uint16_t port_id,
                                    struct doca_flow_pipe **gate_pipe);

static struct eswitch_pipeline_membership *find_membership(
    struct eswitch_pipeline *pipeline, uint16_t vswitch_id,
    uint16_t port_id) {
  for (size_t i = 0; i < ESWITCH_MAX_VLAN_MEMBERSHIPS; i++) {
    struct eswitch_pipeline_membership *member = &pipeline->memberships[i];
    if (member->active && member->vswitch_id == vswitch_id &&
        member->port_id == port_id)
      return member;
  }
  return NULL;
}

static uint16_t ct_queue_id(const struct eswitch_pipeline *pipeline) {
  /* CT queue IDs start after the regular Flow pipe queues. */
  return (uint16_t)(pipeline->runtime->queue_id + 1U);
}

static uint32_t ipv4_prefix_mask(uint8_t length) {
  return length == 0 ? 0 : UINT32_MAX << (32 - length);
}

static doca_error_t set_pipe_identity(struct doca_flow_pipe_cfg *cfg,
                                      const char *name,
                                      enum doca_flow_pipe_type type,
                                      bool is_root, uint32_t entries) {
  doca_error_t result;

  result = doca_flow_pipe_cfg_set_name(cfg, name);
  if (result != DOCA_SUCCESS)
    return result;
  result = doca_flow_pipe_cfg_set_type(cfg, type);
  if (result != DOCA_SUCCESS)
    return result;
  result = doca_flow_pipe_cfg_set_is_root(cfg, is_root);
  if (result != DOCA_SUCCESS)
    return result;
  return doca_flow_pipe_cfg_set_nr_entries(cfg, entries);
}

static uint32_t next_power_of_two(uint32_t value) {
  uint32_t capacity = 1;

  while (capacity < value)
    capacity <<= 1;
  return capacity;
}

static uint16_t max_flood_members(const struct eswitch_pipeline *pipeline) {
  return pipeline->ports->count < ESWITCH_MAX_FLOOD_MEMBERS
             ? pipeline->ports->count
             : ESWITCH_MAX_FLOOD_MEMBERS;
}

static int find_port_index(const struct eswitch_pipeline *pipeline,
                           uint16_t port_id) {
  for (uint16_t i = 0; i < pipeline->ports->count; i++) {
    if (pipeline->ports->items[i].ethernet->port_id == port_id)
      return i;
  }
  return -1;
}

static uint32_t batch_flags(uint32_t index, uint32_t count) {
  return index + 1 == count ? DOCA_FLOW_ENTRY_FLAGS_NO_WAIT
                            : DOCA_FLOW_ENTRY_FLAGS_WAIT_FOR_BATCH;
}

static doca_error_t process_rules(struct eswitch_pipeline *pipeline,
                                  struct eswitch_rule *rules,
                                  uint32_t count) {
  struct flow_entry_cookie **cookies;
  doca_error_t result;

  if (count == 0)
    return DOCA_SUCCESS;
  cookies = calloc(count, sizeof(*cookies));
  if (cookies == NULL)
    return DOCA_ERROR_NO_MEMORY;
  for (uint32_t i = 0; i < count; i++)
    cookies[i] = &rules[i].cookie;
  result = flow_runtime_process(pipeline->runtime, pipeline->switch_port,
                                cookies, count);
  free(cookies);
  return result;
}

static doca_error_t remove_rule(struct eswitch_pipeline *pipeline,
                                struct eswitch_rule *rule,
                                const char *name) {
  struct flow_entry_cookie *cookie;
  doca_error_t result;

  if (rule->entry == NULL)
    return DOCA_SUCCESS;
  flow_entry_cookie_prepare(&rule->cookie, name, DOCA_FLOW_ENTRY_OP_DEL);
  result = doca_flow_pipe_remove_entry(pipeline->runtime->queue_id,
                                       DOCA_FLOW_ENTRY_FLAGS_NO_WAIT,
                                       rule->entry);
  if (result != DOCA_SUCCESS)
    return result;
  cookie = &rule->cookie;
  result = flow_runtime_process(pipeline->runtime, pipeline->switch_port,
                                &cookie, 1);
  if (result == DOCA_SUCCESS)
    rule->entry = NULL;
  return result;
}

uint32_t eswitch_metadata_encode(uint16_t vswitch_id, uint16_t port_id) {
  return ((uint32_t)vswitch_id << 16) | port_id;
}

void eswitch_metadata_decode(uint32_t metadata, uint16_t *vswitch_id,
                            uint16_t *port_id) {
  if (vswitch_id != NULL)
    *vswitch_id = (uint16_t)(metadata >> 16);
  if (port_id != NULL)
    *port_id = (uint16_t)metadata;
}

static doca_error_t create_rss_pipe(struct eswitch_pipeline *pipeline) {
  struct doca_flow_pipe_cfg *cfg = NULL;
  struct doca_flow_match match = {0};
  struct doca_flow_fwd fwd = {0};
  uint16_t queue = SWITCH_RX_QUEUE_ID;
  doca_error_t result;

  result = doca_flow_pipe_cfg_create(&cfg, pipeline->switch_port);
  if (result != DOCA_SUCCESS)
    return result;
  result = set_pipe_identity(cfg, "ESW_RSS", DOCA_FLOW_PIPE_BASIC, false, 1);
  if (result != DOCA_SUCCESS)
    goto out;
  result = doca_flow_pipe_cfg_set_match(cfg, &match, NULL);
  if (result != DOCA_SUCCESS)
    goto out;
  fwd.type = DOCA_FLOW_FWD_RSS;
  fwd.rss_type = DOCA_FLOW_RESOURCE_TYPE_NON_SHARED;
  fwd.rss.queues_array = &queue;
  fwd.rss.nr_queues = 1;
  fwd.rss.inner_flags = DOCA_FLOW_RSS_AUTO;
  result = doca_flow_pipe_create(cfg, &fwd, NULL, &pipeline->rss_pipe);
out:
  doca_flow_pipe_cfg_destroy(cfg);
  if (result != DOCA_SUCCESS)
    return result;

  flow_entry_cookie_prepare(&pipeline->rss_rule.cookie, "RSS catch-all",
                            DOCA_FLOW_ENTRY_OP_ADD);
  result = doca_flow_pipe_basic_add_entry(
      pipeline->runtime->queue_id, pipeline->rss_pipe, &match, 0, NULL, NULL,
      NULL, DOCA_FLOW_ENTRY_FLAGS_NO_WAIT, &pipeline->rss_rule.cookie,
      &pipeline->rss_rule.entry);
  if (result != DOCA_SUCCESS)
    return result;
  return process_rules(pipeline, &pipeline->rss_rule, 1);
}

static doca_error_t create_ct_egress_pipe(struct eswitch_pipeline *pipeline) {
  struct doca_flow_pipe_cfg *cfg = NULL;
  struct doca_flow_match match = {0};
  struct doca_flow_match mask = {0};
  struct doca_flow_actions actions = {0};
  struct doca_flow_actions *actions_array[1] = {&actions};
  struct doca_flow_action_desc desc = {0};
  struct doca_flow_action_descs descs = {.nb_action_desc = 1,
                                         .desc_array = &desc};
  struct doca_flow_action_descs *descs_array[1] = {&descs};
  struct doca_flow_fwd fwd = {.type = DOCA_FLOW_FWD_CHANGEABLE};
  struct doca_flow_fwd miss = {.type = DOCA_FLOW_FWD_PIPE,
                               .next_pipe = pipeline->rss_pipe};
  doca_error_t result;

  match.meta.u32[0] = UINT32_MAX;
  mask.meta.u32[0] = UINT32_MAX;
  memset(actions.outer.eth.src_mac, UINT8_MAX, RTE_ETHER_ADDR_LEN);
  memset(actions.outer.eth.dst_mac, UINT8_MAX, RTE_ETHER_ADDR_LEN);
  actions.outer.l3_type = DOCA_FLOW_L3_TYPE_IP4;
  actions.outer.ip4.ttl = UINT8_MAX;
  desc.type = DOCA_FLOW_ACTION_ADD;
  desc.field_op.dst.field_string = "outer.ipv4.ttl";
  desc.field_op.width = 8;

  result = doca_flow_pipe_cfg_create(&cfg, pipeline->switch_port);
  if (result != DOCA_SUCCESS)
    return result;
  result = set_pipe_identity(cfg, "ESW_CT_EGRESS", DOCA_FLOW_PIPE_BASIC,
                             false, ESWITCH_MAX_CT_ADJACENCIES);
  if (result == DOCA_SUCCESS)
    result = doca_flow_pipe_cfg_set_match(cfg, &match, &mask);
  if (result == DOCA_SUCCESS)
    result = doca_flow_pipe_cfg_set_actions(cfg, actions_array, NULL,
                                            descs_array, 1);
  if (result == DOCA_SUCCESS)
    result = doca_flow_pipe_create(cfg, &fwd, &miss,
                                   &pipeline->ct_egress_pipe);
  doca_flow_pipe_cfg_destroy(cfg);
  return result;
}

static void fill_ct_action_template(struct doca_flow_actions *action,
                                    struct doca_flow_actions *mask,
                                    enum doca_flow_l4_type_ext l4_type) {
  action->meta.u32[0] = UINT32_MAX;
  action->outer.l3_type = DOCA_FLOW_L3_TYPE_IP4;
  action->outer.ip4.src_ip = UINT32_MAX;
  action->outer.ip4.dst_ip = UINT32_MAX;
  action->outer.l4_type_ext = l4_type;
  action->outer.transport.src_port = UINT16_MAX;
  action->outer.transport.dst_port = UINT16_MAX;
  mask->meta.u32[0] = UINT32_MAX;
  mask->outer.l3_type = DOCA_FLOW_L3_TYPE_IP4;
  mask->outer.ip4.src_ip = UINT32_MAX;
  mask->outer.ip4.dst_ip = UINT32_MAX;
  mask->outer.l4_type_ext = l4_type;
  mask->outer.transport.src_port = UINT16_MAX;
  mask->outer.transport.dst_port = UINT16_MAX;
}

static doca_error_t create_ct_pipe(struct eswitch_pipeline *pipeline) {
  struct doca_flow_pipe_cfg *cfg = NULL;
  struct doca_flow_match match = {0};
  struct doca_flow_actions tcp_action = {0}, udp_action = {0};
  struct doca_flow_actions tcp_mask = {0}, udp_mask = {0};
  struct doca_flow_actions *actions[2] = {&tcp_action, &udp_action};
  struct doca_flow_actions *masks[2] = {&tcp_mask, &udp_mask};
  struct doca_flow_fwd hit = {.type = DOCA_FLOW_FWD_PIPE,
                              .next_pipe = pipeline->ct_egress_pipe};
  struct doca_flow_fwd miss = {.type = DOCA_FLOW_FWD_PIPE,
                               .next_pipe = pipeline->rss_pipe};
  doca_error_t result;

  fill_ct_action_template(&tcp_action, &tcp_mask,
                          DOCA_FLOW_L4_TYPE_EXT_TCP);
  fill_ct_action_template(&udp_action, &udp_mask,
                          DOCA_FLOW_L4_TYPE_EXT_UDP);
  result = doca_flow_pipe_cfg_create(&cfg, pipeline->switch_port);
  if (result != DOCA_SUCCESS)
    return result;
  result = set_pipe_identity(cfg, "ESW_NAT_CT", DOCA_FLOW_PIPE_CT, false,
                             pipeline->ct_capacity);
  if (result == DOCA_SUCCESS)
    result = doca_flow_pipe_cfg_set_ct_connections(
        cfg, pipeline->ct_capacity, 0, 0);
  if (result == DOCA_SUCCESS)
    result = doca_flow_pipe_cfg_set_ct_max_connections_per_zone(
        cfg, pipeline->ct_capacity);
  if (result == DOCA_SUCCESS)
    result = doca_flow_pipe_cfg_set_match(cfg, &match, NULL);
  if (result == DOCA_SUCCESS)
    result = doca_flow_pipe_cfg_set_actions(cfg, actions, masks, NULL, 2);
  if (result == DOCA_SUCCESS)
    result = doca_flow_pipe_create(cfg, &hit, &miss, &pipeline->ct_pipe);
  doca_flow_pipe_cfg_destroy(cfg);
  return result;
}

static doca_error_t create_ct_dispatch_pipe(struct eswitch_pipeline *pipeline) {
  struct doca_flow_pipe_cfg *cfg = NULL;
  struct doca_flow_fwd rss = {.type = DOCA_FLOW_FWD_PIPE,
                              .next_pipe = pipeline->rss_pipe};
  doca_error_t result;

  result = doca_flow_pipe_cfg_create(&cfg, pipeline->switch_port);
  if (result != DOCA_SUCCESS)
    return result;
  result = set_pipe_identity(cfg, "ESW_CT_DISPATCH", DOCA_FLOW_PIPE_CONTROL,
                             false, 3);
  if (result == DOCA_SUCCESS)
    result = doca_flow_pipe_create(cfg, NULL, NULL,
                                   &pipeline->ct_dispatch_pipe);
  doca_flow_pipe_cfg_destroy(cfg);
  if (result != DOCA_SUCCESS)
    return result;

  for (uint32_t i = 0; i < 3; i++) {
    struct doca_flow_match match = {0};
    /* Legacy VR-wide dispatch must not enter connection-private zones.
     * Only the scoped admission entries below can reach CT now. */
    struct doca_flow_fwd *fwd = &rss;

    if (i == 0)
      match.parser_meta.outer_l4_type = DOCA_FLOW_L4_META_TCP;
    else if (i == 1)
      match.parser_meta.outer_l4_type = DOCA_FLOW_L4_META_UDP;
    if (i < 2)
      match.parser_meta.outer_l3_type = DOCA_FLOW_L3_META_IPV4;
    flow_entry_cookie_prepare(&pipeline->ct_dispatch_rules[i].cookie,
                              i == 0 ? "CT TCP dispatch" :
                              (i == 1 ? "CT UDP dispatch" :
                                        "CT slow-path fallback"),
                              DOCA_FLOW_ENTRY_OP_ADD);
    result = doca_flow_pipe_control_add_entry(
        pipeline->runtime->queue_id, pipeline->ct_dispatch_pipe,
        i < 2 ? &match : NULL, NULL, NULL, NULL, NULL, NULL, NULL,
        i < 2 ? i : 7, fwd, &pipeline->ct_dispatch_rules[i].cookie,
        &pipeline->ct_dispatch_rules[i].entry);
    if (result != DOCA_SUCCESS)
      return result;
  }
  return process_rules(pipeline, pipeline->ct_dispatch_rules, 3);
}

static doca_error_t create_flood_selector(struct eswitch_pipeline *pipeline) {
  struct doca_flow_pipe_cfg *cfg = NULL;
  struct doca_flow_match match = {0};
  struct doca_flow_match mask = {0};
  struct doca_flow_fwd fwd = {.type = DOCA_FLOW_FWD_CHANGEABLE};
  struct doca_flow_fwd miss = {.type = DOCA_FLOW_FWD_DROP};
  doca_error_t result;

  /* With an explicit mask, an all-ones pipe value marks this field as
   * changeable. A zero value would program a constant-zero VS selector and
   * ignore the values supplied by selector entries. */
  match.meta.pkt_meta = UINT32_MAX;
  mask.meta.pkt_meta = DOCA_HTOBE32(ESWITCH_METADATA_VSWITCH_MASK);
  result = doca_flow_pipe_cfg_create(&cfg, pipeline->switch_port);
  if (result != DOCA_SUCCESS)
    return result;
  result = set_pipe_identity(cfg, "ESW_FLOOD_SELECTOR", DOCA_FLOW_PIPE_BASIC,
                             false, ESWITCH_MAX_VSWITCHES);
  if (result == DOCA_SUCCESS)
    result = doca_flow_pipe_cfg_set_match(cfg, &match, &mask);
  if (result == DOCA_SUCCESS)
    result = doca_flow_pipe_create(cfg, &fwd, &miss,
                                   &pipeline->flood_selector_pipe);
  doca_flow_pipe_cfg_destroy(cfg);
  return result;
}

static doca_error_t create_destination_pipe(struct eswitch_pipeline *pipeline) {
  struct doca_flow_pipe_cfg *cfg = NULL;
  struct doca_flow_match match = {0};
  struct doca_flow_match mask = {0};
  struct doca_flow_fwd fwd = {.type = DOCA_FLOW_FWD_CHANGEABLE};
  struct doca_flow_fwd miss = {0};
  doca_error_t result;

  match.meta.pkt_meta = UINT32_MAX;
  memset(match.outer.eth.dst_mac, UINT8_MAX, RTE_ETHER_ADDR_LEN);
  mask.meta.pkt_meta = DOCA_HTOBE32(ESWITCH_METADATA_VSWITCH_MASK);
  memset(mask.outer.eth.dst_mac, UINT8_MAX, RTE_ETHER_ADDR_LEN);
  miss.type = DOCA_FLOW_FWD_PIPE;
  miss.next_pipe = pipeline->flood_selector_pipe;
  result = doca_flow_pipe_cfg_create(&cfg, pipeline->switch_port);
  if (result != DOCA_SUCCESS)
    return result;
  result = set_pipe_identity(cfg, "ESW_DEST_FDB", DOCA_FLOW_PIPE_BASIC,
                             false, SWITCH_MAX_FDB_ENTRIES);
  if (result == DOCA_SUCCESS)
    result = doca_flow_pipe_cfg_set_match(cfg, &match, &mask);
  if (result == DOCA_SUCCESS)
    result = doca_flow_pipe_cfg_set_miss_counter(cfg, true);
  if (result == DOCA_SUCCESS)
    result = doca_flow_pipe_create(cfg, &fwd, &miss,
                                   &pipeline->destination_pipe);
  doca_flow_pipe_cfg_destroy(cfg);
  return result;
}

static doca_error_t create_learning_clone(struct eswitch_pipeline *pipeline) {
  struct doca_flow_pipe_cfg *cfg = NULL;
  struct doca_flow_fwd pipe_fwd = {.type = DOCA_FLOW_FWD_CHANGEABLE};
  struct doca_flow_fwd fwds[2] = {0};
  doca_error_t result;

  result = doca_flow_pipe_cfg_create(&cfg, pipeline->switch_port);
  if (result != DOCA_SUCCESS)
    return result;
  result = set_pipe_identity(cfg, "ESW_LEARNING_CLONE", DOCA_FLOW_PIPE_HASH,
                             false, 2);
  if (result == DOCA_SUCCESS)
    result = doca_flow_pipe_cfg_set_hash_map_algorithm(
        cfg, DOCA_FLOW_PIPE_HASH_MAP_ALGORITHM_FLOODING);
  if (result == DOCA_SUCCESS)
    result = doca_flow_pipe_create(cfg, &pipe_fwd, NULL,
                                   &pipeline->learning_clone_pipe);
  doca_flow_pipe_cfg_destroy(cfg);
  if (result != DOCA_SUCCESS)
    return result;

  fwds[0].type = DOCA_FLOW_FWD_PIPE;
  fwds[0].next_pipe = pipeline->rss_pipe;
  fwds[1].type = DOCA_FLOW_FWD_PIPE;
  fwds[1].next_pipe = pipeline->destination_pipe;
  for (uint32_t i = 0; i < 2; i++) {
    struct eswitch_rule *rule = &pipeline->learning_clone_rules[i];
    flow_entry_cookie_prepare(&rule->cookie, "learning clone",
                              DOCA_FLOW_ENTRY_OP_ADD);
    result = doca_flow_pipe_hash_add_entry(
        pipeline->runtime->queue_id, pipeline->learning_clone_pipe, i, 0,
        NULL, NULL, &fwds[i], batch_flags(i, 2), &rule->cookie, &rule->entry);
    if (result != DOCA_SUCCESS)
      return result;
  }
  return process_rules(pipeline, pipeline->learning_clone_rules, 2);
}

static doca_error_t create_learning_dispatch(
    struct eswitch_pipeline *pipeline) {
  struct doca_flow_pipe_cfg *cfg = NULL;
  struct doca_flow_match match = {0};
  struct doca_flow_fwd fwd = {0};
  doca_error_t result;

  fwd.type = DOCA_FLOW_FWD_HASH_PIPE;
  fwd.hash_pipe.pipe = pipeline->learning_clone_pipe;
  fwd.hash_pipe.algorithm = DOCA_FLOW_PIPE_HASH_MAP_ALGORITHM_FLOODING;
  result = doca_flow_pipe_cfg_create(&cfg, pipeline->switch_port);
  if (result != DOCA_SUCCESS)
    return result;
  result = set_pipe_identity(cfg, "ESW_LEARNING_DISPATCH",
                             DOCA_FLOW_PIPE_BASIC, false, 1);
  if (result == DOCA_SUCCESS)
    result = doca_flow_pipe_cfg_set_match(cfg, &match, NULL);
  if (result == DOCA_SUCCESS)
    result = doca_flow_pipe_create(cfg, &fwd, NULL,
                                   &pipeline->learning_dispatch_pipe);
  doca_flow_pipe_cfg_destroy(cfg);
  if (result != DOCA_SUCCESS)
    return result;

  flow_entry_cookie_prepare(&pipeline->learning_dispatch_rule.cookie,
                            "learning dispatch", DOCA_FLOW_ENTRY_OP_ADD);
  result = doca_flow_pipe_basic_add_entry(
      pipeline->runtime->queue_id, pipeline->learning_dispatch_pipe, &match,
      0, NULL, NULL, NULL, DOCA_FLOW_ENTRY_FLAGS_NO_WAIT,
      &pipeline->learning_dispatch_rule.cookie,
      &pipeline->learning_dispatch_rule.entry);
  if (result != DOCA_SUCCESS)
    return result;
  return process_rules(pipeline, &pipeline->learning_dispatch_rule, 1);
}

static doca_error_t create_source_guard(struct eswitch_pipeline *pipeline) {
  struct doca_flow_pipe_cfg *cfg = NULL;
  struct doca_flow_match match = {0};
  struct doca_flow_match mask = {0};
  struct doca_flow_monitor monitor = {0};
  struct doca_flow_fwd fwd = {0};
  struct doca_flow_fwd miss = {0};
  doca_error_t result;

  match.meta.pkt_meta = UINT32_MAX;
  memset(match.outer.eth.src_mac, UINT8_MAX, RTE_ETHER_ADDR_LEN);
  mask.meta.pkt_meta = UINT32_MAX;
  memset(mask.outer.eth.src_mac, UINT8_MAX, RTE_ETHER_ADDR_LEN);
  monitor.counter_type = DOCA_FLOW_RESOURCE_TYPE_NON_SHARED;
  fwd.type = DOCA_FLOW_FWD_PIPE;
  fwd.next_pipe = pipeline->destination_pipe;
  miss.type = DOCA_FLOW_FWD_PIPE;
  miss.next_pipe = pipeline->learning_dispatch_pipe;
  result = doca_flow_pipe_cfg_create(&cfg, pipeline->switch_port);
  if (result != DOCA_SUCCESS)
    return result;
  result = set_pipe_identity(cfg, "ESW_SOURCE_GUARD", DOCA_FLOW_PIPE_BASIC,
                             false, SWITCH_MAX_FDB_ENTRIES);
  if (result == DOCA_SUCCESS)
    result = doca_flow_pipe_cfg_set_match(cfg, &match, &mask);
  if (result == DOCA_SUCCESS)
    result = doca_flow_pipe_cfg_set_monitor(cfg, &monitor);
  if (result == DOCA_SUCCESS)
    result = doca_flow_pipe_create(cfg, &fwd, &miss,
                                   &pipeline->source_guard_pipe);
  doca_flow_pipe_cfg_destroy(cfg);
  return result;
}

/* ARP always produces one learning/control copy, even for known sources.
 * The original retains existing private L2 forwarding; the copy is never
 * reinjected. This reuses the existing metadata-preserving clone path. */
static doca_error_t create_arp_dispatch(struct eswitch_pipeline *pipeline) {
  struct doca_flow_pipe_cfg *cfg = NULL;
  struct doca_flow_match match = {0};
  struct doca_flow_fwd hit = {.type = DOCA_FLOW_FWD_PIPE};
  struct doca_flow_fwd miss = {.type = DOCA_FLOW_FWD_PIPE};
  struct eswitch_rule *rule = &pipeline->arp_dispatch_rule;
  doca_error_t result;
  match.outer.eth.type = DOCA_HTOBE16(0x0806);
  hit.next_pipe = pipeline->learning_dispatch_pipe;
  miss.next_pipe = pipeline->ct_admission_pipe != NULL
      ? pipeline->ct_admission_pipe : pipeline->local_ip_pipe;
  result = doca_flow_pipe_cfg_create(&cfg, pipeline->switch_port);
  if (result != DOCA_SUCCESS) return result;
  result = set_pipe_identity(cfg, "ESW_ARP_DISPATCH", DOCA_FLOW_PIPE_BASIC, false, 1);
  if (result == DOCA_SUCCESS)
    result = doca_flow_pipe_cfg_set_match(cfg, &match, NULL);
  if (result == DOCA_SUCCESS)
    result = doca_flow_pipe_create(cfg, &hit, &miss, &pipeline->arp_dispatch_pipe);
  doca_flow_pipe_cfg_destroy(cfg);
  if (result != DOCA_SUCCESS) return result;
  flow_entry_cookie_prepare(&rule->cookie, "ARP control copy", DOCA_FLOW_ENTRY_OP_ADD);
  result = doca_flow_pipe_basic_add_entry(pipeline->runtime->queue_id,
      pipeline->arp_dispatch_pipe, &match, 0, NULL, NULL, NULL,
      DOCA_FLOW_ENTRY_FLAGS_NO_WAIT, &rule->cookie, &rule->entry);
  return result == DOCA_SUCCESS ? process_rules(pipeline, rule, 1) : result;
}

/* IPv4 addressed to an owned RIF is local control traffic. The per-RIF entry
 * restores VR isolation with the VS metadata and delivers only that traffic
 * to Arm; every miss continues through the normal L2 source guard. */
static doca_error_t create_local_ip(struct eswitch_pipeline *pipeline) {
  struct doca_flow_pipe_cfg *cfg = NULL;
  struct doca_flow_match match = {0};
  struct doca_flow_match mask = {0};
  struct doca_flow_monitor monitor = {0};
  struct doca_flow_fwd hit = {.type = DOCA_FLOW_FWD_PIPE,
                              .next_pipe = pipeline->rss_pipe};
  struct doca_flow_fwd miss = {.type = DOCA_FLOW_FWD_PIPE,
      .next_pipe = pipeline->egress_acl_selector_pipe != NULL
          ? pipeline->egress_acl_selector_pipe : pipeline->source_guard_pipe};
  doca_error_t result;

  match.meta.pkt_meta = UINT32_MAX;
  memset(match.outer.eth.dst_mac, UINT8_MAX, RTE_ETHER_ADDR_LEN);
  match.outer.eth.type = DOCA_HTOBE16(RTE_ETHER_TYPE_IPV4);
  if (pipeline->egress_acl_selector_pipe != NULL)
    match.outer.ip4.dst_ip = UINT32_MAX;
  mask.meta.pkt_meta = DOCA_HTOBE32(ESWITCH_METADATA_VSWITCH_MASK);
  memset(mask.outer.eth.dst_mac, UINT8_MAX, RTE_ETHER_ADDR_LEN);
  mask.outer.eth.type = UINT16_MAX;
  if (pipeline->egress_acl_selector_pipe != NULL)
    mask.outer.ip4.dst_ip = UINT32_MAX;
  monitor.counter_type = DOCA_FLOW_RESOURCE_TYPE_NON_SHARED;

  result = doca_flow_pipe_cfg_create(&cfg, pipeline->switch_port);
  if (result != DOCA_SUCCESS)
    return result;
  result = set_pipe_identity(cfg, "ESW_LOCAL_IP", DOCA_FLOW_PIPE_BASIC,
                             false, ESWITCH_MAX_SF_RETURN_CONTEXTS);
  if (result == DOCA_SUCCESS)
    result = doca_flow_pipe_cfg_set_match(cfg, &match, &mask);
  if (result == DOCA_SUCCESS)
    result = doca_flow_pipe_cfg_set_monitor(cfg, &monitor);
  if (result == DOCA_SUCCESS)
    result = doca_flow_pipe_create(cfg, &hit, &miss,
                                   &pipeline->local_ip_pipe);
  doca_flow_pipe_cfg_destroy(cfg);
  return result;
}

/* TTL is an unsigned byte: exact exceptions 0 and 1 are equivalent to >1,
 * without allocating a hardware comparison resource. Only valid IPv4 reaches
 * this stage; its catchall is not reachable from unauthenticated traffic. */
static doca_error_t create_ct_ttl(struct eswitch_pipeline *pipeline,
                                  const char **stage) {
  struct doca_flow_pipe_cfg *cfg = NULL;
  struct doca_flow_fwd miss = {.type = DOCA_FLOW_FWD_PIPE,
                              .next_pipe = pipeline->rss_pipe};
  struct doca_flow_fwd hit = {.type = DOCA_FLOW_FWD_PIPE,
                             .next_pipe = pipeline->ct_pipe};
  *stage = "ttl-pipe-config";
  doca_error_t result = doca_flow_pipe_cfg_create(&cfg, pipeline->switch_port);
  if (result != DOCA_SUCCESS)
    return result;
  result = set_pipe_identity(cfg, "ESW_CT_TTL", DOCA_FLOW_PIPE_CONTROL, false, 3);
  *stage = "ttl-pipe-create";
  if (result == DOCA_SUCCESS)
    result = doca_flow_pipe_create(cfg, NULL, NULL, &pipeline->ct_ttl_pipe);
  doca_flow_pipe_cfg_destroy(cfg);
  for (unsigned i = 0; result == DOCA_SUCCESS && i < 3; i++) {
    struct doca_flow_match match = {0}, mask = {0};
    match.outer.l3_type = DOCA_FLOW_L3_TYPE_IP4;
    match.outer.ip4.ttl = (uint8_t)i;
    mask.outer.ip4.ttl = UINT8_MAX;
    flow_entry_cookie_prepare(&pipeline->ct_ttl_rules[i].cookie,
                              "CT TTL guard", DOCA_FLOW_ENTRY_OP_ADD);
    *stage = i < 2 ? "ttl-exception-add" : "ttl-pass-add";
    result = doca_flow_pipe_control_add_entry(
        pipeline->runtime->queue_id, pipeline->ct_ttl_pipe,
        i < 2 ? &match : NULL, i < 2 ? &mask : NULL,
        NULL, NULL, NULL, NULL, NULL, i < 2 ? 0 : 7,
        i < 2 ? &miss : &hit, &pipeline->ct_ttl_rules[i].cookie,
        &pipeline->ct_ttl_rules[i].entry);
  }
  if (result == DOCA_SUCCESS) {
    *stage = "ttl-process";
    result = process_rules(pipeline, pipeline->ct_ttl_rules, 3);
  }
  return result;
}

/* CT is reachable only through an authorized tuple AND a valid IPv4 header.
 * Invalid authorized traffic returns to Arm, preserving policy revalidation. */
static doca_error_t create_ct_guard(struct eswitch_pipeline *pipeline,
                                    const char **stage) {
  struct doca_flow_pipe_cfg *cfg = NULL;
  struct doca_flow_match match = {0}, mask = {0};
  struct doca_flow_fwd hit = {.type = DOCA_FLOW_FWD_PIPE,
                             .next_pipe = pipeline->ct_ttl_pipe};
  struct doca_flow_fwd miss = {.type = DOCA_FLOW_FWD_PIPE,
                              .next_pipe = pipeline->rss_pipe};
  *stage = "guard-pipe-config";
  doca_error_t result = doca_flow_pipe_cfg_create(&cfg, pipeline->switch_port);
  if (result != DOCA_SUCCESS)
    return result;
  result = set_pipe_identity(cfg, "ESW_CT_IPV4_GUARD", DOCA_FLOW_PIPE_CONTROL,
                             false, 2);
  *stage = "guard-pipe-create";
  if (result == DOCA_SUCCESS)
    result = doca_flow_pipe_create(cfg, NULL, NULL, &pipeline->ct_guard_pipe);
  doca_flow_pipe_cfg_destroy(cfg);
  if (result != DOCA_SUCCESS)
    return result;
  match.parser_meta.outer_l3_type = DOCA_FLOW_L3_META_IPV4;
  mask.parser_meta.outer_l3_type = UINT32_MAX;
  mask.parser_meta.outer_ip_fragmented = UINT8_MAX;
  match.parser_meta.outer_l3_ok = 1;
  mask.parser_meta.outer_l3_ok = UINT8_MAX;
  match.parser_meta.outer_ip4_checksum_ok = 1;
  mask.parser_meta.outer_ip4_checksum_ok = UINT8_MAX;
  match.outer.l3_type = DOCA_FLOW_L3_TYPE_IP4;
  match.outer.ip4.version_ihl = 0x45;
  mask.outer.ip4.version_ihl = UINT8_MAX;
  flow_entry_cookie_prepare(&pipeline->ct_guard_rules[0].cookie,
                            "CT IPv4 guard", DOCA_FLOW_ENTRY_OP_ADD);
  *stage = "guard-entry-add";
  result = doca_flow_pipe_control_add_entry(
      pipeline->runtime->queue_id, pipeline->ct_guard_pipe,
      &match, &mask, NULL, NULL, NULL, NULL, NULL, 0, &hit,
      &pipeline->ct_guard_rules[0].cookie, &pipeline->ct_guard_rules[0].entry);
  if (result == DOCA_SUCCESS) {
    flow_entry_cookie_prepare(&pipeline->ct_guard_rules[1].cookie,
                              "CT IPv4 guard miss", DOCA_FLOW_ENTRY_OP_ADD);
    *stage = "guard-miss-add";
    result = doca_flow_pipe_control_add_entry(
        pipeline->runtime->queue_id, pipeline->ct_guard_pipe,
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, 7, &miss,
        &pipeline->ct_guard_rules[1].cookie, &pipeline->ct_guard_rules[1].entry);
  }
  if (result == DOCA_SUCCESS) {
    *stage = "guard-process";
    result = process_rules(pipeline, pipeline->ct_guard_rules, 2);
  }
  return result;
}

static doca_error_t create_ct_admission(struct eswitch_pipeline *pipeline,
                                        const char **stage) {
  struct doca_flow_pipe_cfg *cfg = NULL;
  *stage = "admission-pipe-config";
  doca_error_t result = doca_flow_pipe_cfg_create(&cfg, pipeline->switch_port);
  if (result != DOCA_SUCCESS)
    return result;
  result = set_pipe_identity(cfg, "ESW_CT_AUTHORIZED", DOCA_FLOW_PIPE_CONTROL,
                             false, 2 * pipeline->ct_capacity + 1);
  *stage = "admission-pipe-create";
  if (result == DOCA_SUCCESS)
    result = doca_flow_pipe_create(cfg, NULL, NULL,
                                   &pipeline->ct_admission_pipe);
  doca_flow_pipe_cfg_destroy(cfg);
  if (result == DOCA_SUCCESS) {
    struct doca_flow_fwd miss = {.type = DOCA_FLOW_FWD_PIPE,
                                .next_pipe = pipeline->local_ip_pipe};
    struct eswitch_rule *fallback = &pipeline->ct_admission_miss;
    flow_entry_cookie_prepare(&fallback->cookie, "CT admission miss",
                              DOCA_FLOW_ENTRY_OP_ADD);
    *stage = "admission-miss-add";
    result = doca_flow_pipe_control_add_entry(
        pipeline->runtime->queue_id, pipeline->ct_admission_pipe,
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, 7, &miss,
        &fallback->cookie, &fallback->entry);
    if (result == DOCA_SUCCESS) {
      *stage = "admission-process";
      result = process_rules(pipeline, fallback, 1);
    }
  }
  return result;
}

static doca_error_t create_route_lpm(struct eswitch_pipeline *pipeline) {
  struct doca_flow_pipe_cfg *cfg = NULL;
  struct doca_flow_match match = {0};
  struct doca_flow_match mask = {0};
  struct doca_flow_actions actions = {0};
  struct doca_flow_actions *actions_array[1] = {&actions};
  struct doca_flow_action_desc desc = {0};
  struct doca_flow_action_descs descs = {.nb_action_desc = 1,
                                         .desc_array = &desc};
  struct doca_flow_action_descs *descs_array[1] = {&descs};
  struct doca_flow_fwd fwd = {.type = DOCA_FLOW_FWD_CHANGEABLE};
  struct doca_flow_fwd miss = {.type = DOCA_FLOW_FWD_PIPE,
                               .next_pipe = pipeline->hardware_ct_enabled
                                   ? pipeline->ct_dispatch_pipe
                                   : pipeline->rss_pipe};
  doca_error_t result;

  match.outer.l3_type = DOCA_FLOW_L3_TYPE_IP4;
  match.outer.ip4.dst_ip = UINT32_MAX;
  /* DOCA Flow LPM permits its exact-match companion key in meta.u32[1].
   * meta.u32[0] is not a supported LPM EM field on BF3/DOCA 3.4. */
  mask.meta.u32[1] = UINT32_MAX;
  memset(actions.outer.eth.src_mac, UINT8_MAX, RTE_ETHER_ADDR_LEN);
  memset(actions.outer.eth.dst_mac, UINT8_MAX, RTE_ETHER_ADDR_LEN);
  actions.outer.l3_type = DOCA_FLOW_L3_TYPE_IP4;
  actions.outer.ip4.ttl = UINT8_MAX; /* -1 for DOCA_FLOW_ACTION_ADD */
  desc.type = DOCA_FLOW_ACTION_ADD;
  desc.field_op.dst.field_string = "outer.ipv4.ttl";
  desc.field_op.dst.bit_offset = 0;
  desc.field_op.width = 8;

  for (;;) {
    result = doca_flow_pipe_cfg_create(&cfg, pipeline->switch_port);
    if (result != DOCA_SUCCESS)
      return result;
    result = set_pipe_identity(cfg, "ESW_ROUTER_LPM", DOCA_FLOW_PIPE_LPM,
                               false, pipeline->hw_route_capacity);
    if (result == DOCA_SUCCESS)
      result = doca_flow_pipe_cfg_set_match(cfg, &match, &mask);
    if (result == DOCA_SUCCESS)
      result = doca_flow_pipe_cfg_set_actions(cfg, actions_array, NULL,
                                              descs_array, 1);
    if (result == DOCA_SUCCESS)
      result = doca_flow_pipe_cfg_set_miss_counter(cfg, true);
    if (result == DOCA_SUCCESS)
      result = doca_flow_pipe_create(cfg, &fwd, &miss,
                                     &pipeline->route_lpm_pipe);
    doca_flow_pipe_cfg_destroy(cfg);
    cfg = NULL;
    if (result == DOCA_SUCCESS) {
      printf("Hardware IPv4 LPM admitted capacity=%u\n",
             pipeline->hw_route_capacity);
      return DOCA_SUCCESS;
    }
    if (pipeline->route_lpm_pipe != NULL) {
      doca_flow_pipe_destroy(pipeline->route_lpm_pipe);
      pipeline->route_lpm_pipe = NULL;
    }
    if (result != DOCA_ERROR_NO_MEMORY ||
        pipeline->hw_route_capacity <= ESWITCH_HW_ROUTE_MIN_CAPACITY)
      return result;
    pipeline->hw_route_capacity >>= 1;
    fprintf(stderr, "Hardware IPv4 LPM resource retry: capacity=%u\n",
            pipeline->hw_route_capacity);
  }
}

static doca_error_t create_route_control(struct eswitch_pipeline *pipeline) {
  struct doca_flow_pipe_cfg *cfg = NULL;
  struct doca_flow_fwd fwd = {.type = DOCA_FLOW_FWD_PIPE,
                              .next_pipe = pipeline->hardware_ct_enabled
                                  ? pipeline->ct_dispatch_pipe
                                  : pipeline->rss_pipe};
  doca_error_t result;

  result = doca_flow_pipe_cfg_create(&cfg, pipeline->switch_port);
  if (result != DOCA_SUCCESS)
    return result;
  result = set_pipe_identity(cfg, "ESW_ROUTER_ELIGIBLE",
                             DOCA_FLOW_PIPE_CONTROL, false,
                             ROUTER_MAX_INTERFACES + 1);
  if (result == DOCA_SUCCESS)
    result = doca_flow_pipe_create(cfg, NULL, NULL,
                                   &pipeline->route_control_pipe);
  doca_flow_pipe_cfg_destroy(cfg);
  if (result != DOCA_SUCCESS)
    return result;

  flow_entry_cookie_prepare(&pipeline->route_fallback_rule.cookie,
                            "router slow-path fallback",
                            DOCA_FLOW_ENTRY_OP_ADD);
  result = doca_flow_pipe_control_add_entry(
      pipeline->runtime->queue_id, pipeline->route_control_pipe,
      NULL, NULL, NULL, NULL, NULL, NULL, NULL, 7, &fwd,
      &pipeline->route_fallback_rule.cookie,
      &pipeline->route_fallback_rule.entry);
  if (result != DOCA_SUCCESS)
    return result;
  return process_rules(pipeline, &pipeline->route_fallback_rule, 1);
}

static doca_error_t create_route_selector(struct eswitch_pipeline *pipeline) {
  struct doca_flow_pipe_cfg *cfg = NULL;
  struct doca_flow_match match = {0};
  struct doca_flow_match mask = {0};
  struct doca_flow_fwd hit = {.type = DOCA_FLOW_FWD_PIPE,
                              .next_pipe = pipeline->hardware_routing_enabled
                                  ? pipeline->route_control_pipe
                                  : pipeline->rss_pipe};
  struct doca_flow_fwd miss = {.type = DOCA_FLOW_FWD_PIPE,
                               .next_pipe = pipeline->source_guard_pipe};
  doca_error_t result;

  match.meta.pkt_meta = UINT32_MAX;
  memset(match.outer.eth.dst_mac, UINT8_MAX, RTE_ETHER_ADDR_LEN);
  match.outer.eth.type = DOCA_HTOBE16(RTE_ETHER_TYPE_IPV4);
  mask.meta.pkt_meta = DOCA_HTOBE32(ESWITCH_METADATA_VSWITCH_MASK);
  memset(mask.outer.eth.dst_mac, UINT8_MAX, RTE_ETHER_ADDR_LEN);
  mask.outer.eth.type = UINT16_MAX;
  result = doca_flow_pipe_cfg_create(&cfg, pipeline->switch_port);
  if (result != DOCA_SUCCESS)
    return result;
  result = set_pipe_identity(cfg, "ESW_ROUTER_SELECTOR",
                             DOCA_FLOW_PIPE_BASIC, false,
                             ROUTER_MAX_INTERFACES);
  if (result == DOCA_SUCCESS)
    result = doca_flow_pipe_cfg_set_match(cfg, &match, &mask);
  if (result == DOCA_SUCCESS)
    result = doca_flow_pipe_create(cfg, &hit, &miss,
                                   &pipeline->route_selector_pipe);
  doca_flow_pipe_cfg_destroy(cfg);
  return result;
}

/* Only packets addressed to a guest RIF reach this selector: local RIF IPs
 * were consumed by ESW_LOCAL_IP first. A selector miss retains the original
 * routing/L2 path, so an unconfigured guest policy changes nothing. */
static doca_error_t create_egress_acl_selector(
    struct eswitch_pipeline *pipeline) {
  struct doca_flow_pipe_cfg *cfg = NULL;
  struct doca_flow_match match = {0}, mask = {0};
  struct doca_flow_fwd hit = {.type = DOCA_FLOW_FWD_CHANGEABLE};
  struct doca_flow_fwd miss = {.type = DOCA_FLOW_FWD_PIPE,
                               .next_pipe = pipeline->route_selector_pipe};
  doca_error_t result;

  match.meta.pkt_meta = UINT32_MAX;
  memset(match.outer.eth.dst_mac, UINT8_MAX, RTE_ETHER_ADDR_LEN);
  match.outer.eth.type = DOCA_HTOBE16(RTE_ETHER_TYPE_IPV4);
  mask.meta.pkt_meta = DOCA_HTOBE32(ESWITCH_METADATA_VSWITCH_MASK);
  memset(mask.outer.eth.dst_mac, UINT8_MAX, RTE_ETHER_ADDR_LEN);
  mask.outer.eth.type = UINT16_MAX;
  result = doca_flow_pipe_cfg_create(&cfg, pipeline->switch_port);
  if (result != DOCA_SUCCESS)
    return result;
  result = set_pipe_identity(cfg, "ESW_GUEST_EGRESS_SELECT",
                             DOCA_FLOW_PIPE_BASIC, false,
                             ROUTER_MAX_EGRESS_POLICIES + 1U);
  if (result == DOCA_SUCCESS)
    result = doca_flow_pipe_cfg_set_match(cfg, &match, &mask);
  if (result == DOCA_SUCCESS)
    result = doca_flow_pipe_create(cfg, &hit, &miss,
                                   &pipeline->egress_acl_selector_pipe);
  doca_flow_pipe_cfg_destroy(cfg);
  return result;
}

static const struct router_interface *acl_rif(
    const struct router_config *config, uint16_t interface_id) {
  for (size_t i = 0; i < config->interface_count; i++)
    if (config->interfaces[i].interface_id == interface_id)
      return &config->interfaces[i];
  return NULL;
}

static int acl_rule_order(const void *a, const void *b) {
  const struct router_egress_rule *const *left = a, *const *right = b;
  return (int)(*left)->rule_id - (int)(*right)->rule_id;
}

static uint64_t acl_hash_word(uint64_t hash, uint64_t value) {
  /* Hash individual fields, not padding in persisted structs. */
  for (unsigned int i = 0; i < 8; i++) {
    hash ^= (uint8_t)(value >> (i * 8));
    hash *= UINT64_C(1099511628211);
  }
  return hash;
}

static uint64_t acl_fingerprint(
    const struct router_config *config,
    const struct router_egress_policy *policy,
    const struct router_egress_rule *const *rules, size_t count) {
  uint64_t hash = UINT64_C(14695981039346656037);

  hash = acl_hash_word(hash, policy->default_allow);
  hash = acl_hash_word(hash, count);
  for (size_t i = 0; i < config->port_forward_count; i++)
    if (config->port_forwards[i].vr_id == policy->vr_id)
      hash = acl_hash_word(hash, UINT64_C(0x50465245504c59));
  for (size_t i = 0; i < config->interface_count; i++) {
    const struct router_interface *rif = &config->interfaces[i];
    if (rif->vr_id != policy->vr_id || !rif->has_address)
      continue;
    hash = acl_hash_word(hash, rif->interface_id);
    hash = acl_hash_word(hash, rif->address);
  }
  for (size_t i = 0; i < count; i++) {
    const struct router_egress_rule *rule = rules[i];
    hash = acl_hash_word(hash, rule->rule_id);
    hash = acl_hash_word(hash, rule->source);
    hash = acl_hash_word(hash, rule->source_prefix);
    hash = acl_hash_word(hash, rule->destination);
    hash = acl_hash_word(hash, rule->destination_prefix);
    hash = acl_hash_word(hash, rule->protocol);
    hash = acl_hash_word(hash, rule->port_first);
    hash = acl_hash_word(hash, rule->port_last);
    hash = acl_hash_word(hash, (uint16_t)rule->icmp_type);
    hash = acl_hash_word(hash, (uint16_t)rule->icmp_code);
    hash = acl_hash_word(hash, rule->allow);
  }
  return hash;
}

static struct eswitch_egress_acl *acl_slot(
    struct eswitch_pipeline *pipeline, uint16_t interface_id) {
  struct eswitch_egress_acl *free_slot = NULL;

  for (size_t i = 0; i < ROUTER_MAX_EGRESS_POLICIES; i++) {
    struct eswitch_egress_acl *slot = &pipeline->egress_acls[i];
    if (slot->active && slot->interface_id == interface_id)
      return slot;
    if (!slot->active && free_slot == NULL)
      free_slot = slot;
  }
  return free_slot;
}

static doca_error_t acl_select_target(struct eswitch_pipeline *pipeline,
                                      struct eswitch_egress_acl *slot,
                                      const struct router_interface *rif,
                                      struct doca_flow_pipe *target);

static bool acl_can_offload(const struct router_config *config,
                            const struct router_egress_policy *policy,
                            const struct router_egress_rule *const *rules,
                            size_t count) {
  /* The DOCA 3.4 ACL sample programs TCP/UDP five-tuples with an explicit
   * L4 parser type. Keep other protocols on the authoritative Arm path until
   * their ACL entry shapes are validated on the target hardware. */
  (void)config;
  (void)policy;
  for (size_t i = 0; i < count; i++)
    if ((rules[i]->protocol != IPPROTO_TCP &&
         rules[i]->protocol != IPPROTO_UDP) ||
        rules[i]->icmp_type >= 0 || rules[i]->icmp_code >= 0)
      return false;
  return true;
}

static doca_error_t acl_build_generation(
    struct eswitch_pipeline *pipeline,
    const struct router_config *config,
    const struct router_egress_policy *policy,
    const struct router_egress_rule *const *rules, size_t count,
    struct doca_flow_pipe **pipe, struct eswitch_rule **entries) {
  struct doca_flow_pipe_cfg *cfg = NULL;
  struct doca_flow_match template = {0};
  struct doca_flow_actions actions = {0};
  struct doca_flow_actions *actions_array[1] = {&actions};
  struct doca_flow_fwd hit = {.type = DOCA_FLOW_FWD_CHANGEABLE};
  struct doca_flow_fwd miss = {0};
  size_t local_count = 0, total, position = 0;
  const char *stage = "pipe-config-create";
  doca_error_t result;

  *pipe = NULL;
  *entries = NULL;
  /* A default-allow miss already forwards local destinations to Arm. Avoid
   * consuming ACL entries for redundant local-RIF exceptions; in particular,
   * some HWS configurations reject an IP-only ACL entry at insertion time.
   * Default-deny still needs explicit exceptions and safely falls back to
   * Arm if the hardware cannot install them. */
  if (!policy->default_allow)
    for (size_t i = 0; i < config->interface_count; i++)
      if (config->interfaces[i].vr_id == policy->vr_id &&
          config->interfaces[i].has_address)
        local_count++;
  total = local_count + count;
  template.parser_meta.outer_l3_type = DOCA_FLOW_L3_META_IPV4;
  template.outer.l3_type = DOCA_FLOW_L3_TYPE_IP4;
  template.outer.ip4.src_ip = UINT32_MAX;
  template.outer.ip4.dst_ip = UINT32_MAX;
  /* Match the shipped DOCA 3.4 flow_acl template for a populated TCP/UDP
   * ACL. The ACL entry supplies its own TCP or UDP parser type and ports. */
  if (count != 0) {
    template.outer.l4_type_ext = DOCA_FLOW_L4_TYPE_EXT_TCP;
    template.outer.tcp.l4_port.src_port = UINT16_MAX;
    template.outer.tcp.l4_port.dst_port = UINT16_MAX;
  }
  miss.type = policy->default_allow ? DOCA_FLOW_FWD_PIPE
                                    : DOCA_FLOW_FWD_DROP;
  if (policy->default_allow)
    miss.next_pipe = pipeline->rss_pipe;
  result = doca_flow_pipe_cfg_create(&cfg, pipeline->switch_port);
  if (result != DOCA_SUCCESS) {
    fprintf(stderr, "Guest egress ACL build failed: vr=%u rif=%u stage=%s error=%s\n",
            policy->vr_id, policy->interface_id, stage,
            doca_error_get_descr(result));
    return result;
  }
  stage = "pipe-config-identity";
  result = set_pipe_identity(cfg, "ESW_GUEST_EGRESS_ACL",
                             DOCA_FLOW_PIPE_ACL, false,
                             total ? (uint32_t)total : 1U);
  if (result == DOCA_SUCCESS) {
    stage = "pipe-config-match";
    result = doca_flow_pipe_cfg_set_match(cfg, &template, NULL);
  }
  if (result == DOCA_SUCCESS) {
    stage = "pipe-config-actions";
    result = doca_flow_pipe_cfg_set_actions(cfg, actions_array, NULL, NULL, 1);
  }
  if (result == DOCA_SUCCESS) {
    stage = "pipe-create";
    result = doca_flow_pipe_create(cfg, &hit, &miss, pipe);
  }
  doca_flow_pipe_cfg_destroy(cfg);
  if (result != DOCA_SUCCESS) {
    fprintf(stderr, "Guest egress ACL build failed: vr=%u rif=%u stage=%s entries=%zu error=%s\n",
            policy->vr_id, policy->interface_id, stage, total,
            doca_error_get_descr(result));
    return result;
  }
  if (total == 0)
    return DOCA_SUCCESS;
  *entries = calloc(total, sizeof(**entries));
  if (*entries == NULL) {
    fprintf(stderr, "Guest egress ACL build failed: vr=%u rif=%u stage=entry-storage entries=%zu error=Memory allocation failure\n",
            policy->vr_id, policy->interface_id, total);
    doca_flow_pipe_destroy(*pipe);
    *pipe = NULL;
    return DOCA_ERROR_NO_MEMORY;
  }
  /* Arm exempts every local IP of this VR, not just the ingress RIF IP.
   * Insert these before user rules so a default-deny cannot intercept them. */
  for (size_t i = 0; i < config->interface_count; i++) {
    const struct router_interface *local = &config->interfaces[i];
    struct doca_flow_match match = {0}, mask = {0};
    struct doca_flow_fwd fwd = {.type = DOCA_FLOW_FWD_PIPE,
                                .next_pipe = pipeline->rss_pipe};
    if (policy->default_allow || local->vr_id != policy->vr_id ||
        !local->has_address)
      continue;
    match.outer.l3_type = DOCA_FLOW_L3_TYPE_IP4;
    match.outer.ip4.dst_ip = DOCA_HTOBE32(local->address);
    mask.outer.ip4.dst_ip = UINT32_MAX;
    flow_entry_cookie_prepare(&(*entries)[position].cookie,
                              "guest egress local-RIF bypass",
                              DOCA_FLOW_ENTRY_OP_ADD);
    stage = "local-RIF-entry-add";
    result = doca_flow_pipe_acl_add_entry(
        pipeline->runtime->queue_id, *pipe, &match, &mask, 0, NULL,
        (uint32_t)position, &fwd,
        batch_flags((uint32_t)position, (uint32_t)total),
        &(*entries)[position].cookie, &(*entries)[position].entry);
    if (result != DOCA_SUCCESS)
      goto build_done;
    position++;
  }
  for (size_t i = 0; i < count; i++, position++) {
    const struct router_egress_rule *rule = rules[i];
    struct doca_flow_match match = {0}, mask = {0};
    struct doca_flow_fwd fwd = {0};

    match.outer.l3_type = DOCA_FLOW_L3_TYPE_IP4;
    match.outer.ip4.src_ip = DOCA_HTOBE32(rule->source);
    match.outer.ip4.dst_ip = DOCA_HTOBE32(rule->destination);
    mask.outer.ip4.src_ip = DOCA_HTOBE32(
        ipv4_prefix_mask(rule->source_prefix));
    mask.outer.ip4.dst_ip = DOCA_HTOBE32(
        ipv4_prefix_mask(rule->destination_prefix));
    if (rule->protocol == IPPROTO_TCP) {
      match.parser_meta.outer_l4_type = DOCA_FLOW_L4_META_TCP;
      match.outer.l4_type_ext = DOCA_FLOW_L4_TYPE_EXT_TCP;
    } else {
      match.parser_meta.outer_l4_type = DOCA_FLOW_L4_META_UDP;
      match.outer.l4_type_ext = DOCA_FLOW_L4_TYPE_EXT_UDP;
    }
    mask.parser_meta.outer_l4_type = UINT32_MAX;
    if (rule->port_first != 0) {
      /* ACL interprets the mask port as the inclusive range end. */
      if (rule->protocol == IPPROTO_TCP) {
        match.outer.tcp.l4_port.dst_port = DOCA_HTOBE16(rule->port_first);
        mask.outer.tcp.l4_port.dst_port = DOCA_HTOBE16(rule->port_last);
      } else {
        match.outer.udp.l4_port.dst_port = DOCA_HTOBE16(rule->port_first);
        mask.outer.udp.l4_port.dst_port = DOCA_HTOBE16(rule->port_last);
      }
    }
    fwd.type = rule->allow ? DOCA_FLOW_FWD_PIPE : DOCA_FLOW_FWD_DROP;
    if (rule->allow)
      fwd.next_pipe = pipeline->rss_pipe;
    flow_entry_cookie_prepare(&(*entries)[position].cookie,
                              "guest egress ACL rule", DOCA_FLOW_ENTRY_OP_ADD);
    stage = "policy-rule-entry-add";
    result = doca_flow_pipe_acl_add_entry(
        pipeline->runtime->queue_id, *pipe, &match, &mask, 0, NULL,
        (uint32_t)position, &fwd,
        batch_flags((uint32_t)position, (uint32_t)total),
        &(*entries)[position].cookie, &(*entries)[position].entry);
    if (result != DOCA_SUCCESS)
      break;
  }
build_done:
  if (result == DOCA_SUCCESS) {
    stage = "entries-process";
    result = process_rules(pipeline, *entries, (uint32_t)total);
  }
  if (result != DOCA_SUCCESS) {
    fprintf(stderr, "Guest egress ACL build failed: vr=%u rif=%u stage=%s entry=%zu/%zu error=%s\n",
            policy->vr_id, policy->interface_id, stage, position, total,
            doca_error_get_descr(result));
    doca_flow_pipe_destroy(*pipe);
    free(*entries);
    *pipe = NULL;
    *entries = NULL;
  }
  return result;
}

/* These are exact session exceptions, not port-range policy rules. A control
 * pipe avoids the ACL template's protocol/range/action constraints. Hits
 * still go to Arm for session validation; misses go to the policy ACL. */
static doca_error_t acl_build_pf_reply_pipe(
    struct eswitch_pipeline *pipeline, struct doca_flow_pipe *acl_pipe,
    struct eswitch_rule *miss_rule,
    struct doca_flow_pipe **reply_pipe) {
  struct doca_flow_pipe_cfg *cfg = NULL;
  struct doca_flow_fwd miss = {.type = DOCA_FLOW_FWD_PIPE,
                               .next_pipe = acl_pipe};
  doca_error_t result;

  *reply_pipe = NULL;
  result = doca_flow_pipe_cfg_create(&cfg, pipeline->switch_port);
  if (result != DOCA_SUCCESS)
    return result;
  result = set_pipe_identity(cfg, "ESW_PF_REPLY_EXACT", DOCA_FLOW_PIPE_CONTROL,
                             false, ESWITCH_MAX_PF_REPLY_EXCEPTIONS + 1);
  if (result == DOCA_SUCCESS)
    result = doca_flow_pipe_create(cfg, NULL, NULL, reply_pipe);
  doca_flow_pipe_cfg_destroy(cfg);
  if (result == DOCA_SUCCESS) {
    flow_entry_cookie_prepare(&miss_rule->cookie, "PF exception policy miss",
                              DOCA_FLOW_ENTRY_OP_ADD);
    result = doca_flow_pipe_control_add_entry(
        pipeline->runtime->queue_id, *reply_pipe, NULL, NULL, NULL, NULL,
        NULL, NULL, NULL, 7, &miss, &miss_rule->cookie, &miss_rule->entry);
    if (result == DOCA_SUCCESS)
      result = process_rules(pipeline, miss_rule, 1);
  }
  return result;
}

static bool acl_pf_reply_same_session(
    const struct eswitch_pf_reply_exception *entry,
    const struct router_nat_session *session) {
  return session != NULL && entry->session == session && session->used &&
         session->port_forward && entry->vr_id == session->vr_id &&
         entry->protocol == session->protocol &&
         entry->inside_ip == session->inside_ip &&
         entry->inside_port == session->inside_port &&
         entry->remote_ip == session->remote_ip &&
         entry->remote_port == session->remote_port;
}

doca_error_t eswitch_pipeline_egress_acl_pf_reply_prune(
    struct eswitch_pipeline *pipeline) {
  if (pipeline == NULL)
    return DOCA_ERROR_INVALID_VALUE;
  for (size_t i = 0; i < ROUTER_MAX_EGRESS_POLICIES; i++) {
    struct eswitch_egress_acl *slot = &pipeline->egress_acls[i];
    if (slot->pf_replies == NULL)
      continue;
    for (size_t j = 0; j < ESWITCH_MAX_PF_REPLY_EXCEPTIONS; j++) {
      struct eswitch_pf_reply_exception *entry = &slot->pf_replies[j];
      doca_error_t result;
      if (entry->rule.entry == NULL ||
          acl_pf_reply_same_session(entry, entry->session))
        continue;
      result = remove_rule(pipeline, &entry->rule,
                           "remove stale port-forward reply exception");
      if (result != DOCA_SUCCESS)
        return result;
      *entry = (struct eswitch_pf_reply_exception){0};
      slot->pf_reply_count--;
    }
  }
  return DOCA_SUCCESS;
}

doca_error_t eswitch_pipeline_egress_acl_pf_reply_flush(
    struct eswitch_pipeline *pipeline) {
  if (pipeline == NULL)
    return DOCA_ERROR_INVALID_VALUE;
  for (size_t i = 0; i < ROUTER_MAX_EGRESS_POLICIES; i++) {
    struct eswitch_egress_acl *slot = &pipeline->egress_acls[i];
    if (slot->pf_replies == NULL)
      continue;
    for (size_t j = 0; j < ESWITCH_MAX_PF_REPLY_EXCEPTIONS; j++) {
      struct eswitch_pf_reply_exception *entry = &slot->pf_replies[j];
      doca_error_t result;
      if (entry->rule.entry == NULL)
        continue;
      result = remove_rule(pipeline, &entry->rule,
                           "flush port-forward reply exception");
      if (result != DOCA_SUCCESS)
        return result;
      *entry = (struct eswitch_pf_reply_exception){0};
    }
    slot->pf_reply_count = 0;
  }
  return DOCA_SUCCESS;
}

static doca_error_t acl_pf_reply_fallback(
    struct eswitch_pipeline *pipeline, const struct router_config *config,
    struct eswitch_egress_acl *slot) {
  const struct router_interface *rif = acl_rif(config, slot->interface_id);
  doca_error_t result;

  if (rif == NULL)
    return DOCA_ERROR_BAD_STATE;
  /* Swap to Arm before destroying either pipe. This also protects existing
   * sessions when the exception table is full or a hardware add fails. */
  result = acl_select_target(pipeline, slot, rif, pipeline->rss_pipe);
  if (result != DOCA_SUCCESS)
    return result;
  if (slot->pf_reply_pipe != NULL)
    doca_flow_pipe_destroy(slot->pf_reply_pipe);
  if (slot->pipe != NULL)
    doca_flow_pipe_destroy(slot->pipe);
  free(slot->pf_replies);
  free(slot->rules);
  slot->pf_reply_pipe = NULL;
  slot->pipe = NULL;
  slot->pf_replies = NULL;
  slot->rules = NULL;
  slot->pf_reply_count = 0;
  slot->rule_count = 0;
  slot->fallback_arm = true;
  pipeline->egress_acl_pf_reply_fallbacks++;
  return DOCA_SUCCESS;
}

doca_error_t eswitch_pipeline_egress_acl_pf_reply_add(
    struct eswitch_pipeline *pipeline, const struct router_config *config,
    uint16_t guest_interface_id, const struct router_nat_session *session) {
  struct eswitch_egress_acl *slot = NULL;
  struct eswitch_pf_reply_exception *free_entry = NULL;
  struct doca_flow_match match = {0}, mask = {0};
  struct doca_flow_fwd fwd = {.type = DOCA_FLOW_FWD_PIPE};
  const char *stage = "validate-session";
  doca_error_t result;

  if (pipeline == NULL || config == NULL || session == NULL ||
      !session->used || !session->port_forward ||
      (session->protocol != IPPROTO_TCP && session->protocol != IPPROTO_UDP))
    return DOCA_ERROR_INVALID_VALUE;
  for (size_t i = 0; i < ROUTER_MAX_EGRESS_POLICIES; i++)
    if (pipeline->egress_acls[i].active &&
        pipeline->egress_acls[i].interface_id == guest_interface_id &&
        pipeline->egress_acls[i].vr_id == session->vr_id) {
      slot = &pipeline->egress_acls[i];
      break;
    }
  if (slot == NULL || slot->fallback_arm || slot->pf_reply_pipe == NULL)
    return DOCA_SUCCESS;
  if (session->inside_port == 0 || session->remote_port == 0) {
    result = DOCA_ERROR_NOT_SUPPORTED;
    goto fallback;
  }
  fwd.next_pipe = pipeline->rss_pipe;
  stage = "prune-stale-entries";
  result = eswitch_pipeline_egress_acl_pf_reply_prune(pipeline);
  if (result != DOCA_SUCCESS)
    goto fallback;
  for (size_t i = 0; i < ESWITCH_MAX_PF_REPLY_EXCEPTIONS; i++) {
    struct eswitch_pf_reply_exception *entry = &slot->pf_replies[i];
    if (entry->rule.entry != NULL &&
        acl_pf_reply_same_session(entry, session))
      return DOCA_SUCCESS;
    if (entry->rule.entry == NULL && free_entry == NULL)
      free_entry = entry;
  }
  if (free_entry == NULL) {
    stage = "entry-capacity";
    result = DOCA_ERROR_NO_MEMORY;
    goto fallback;
  }
  match.outer.l3_type = DOCA_FLOW_L3_TYPE_IP4;
  match.parser_meta.outer_l3_type = DOCA_FLOW_L3_META_IPV4;
  mask.parser_meta.outer_l3_type = UINT32_MAX;
  match.outer.ip4.src_ip = DOCA_HTOBE32(session->inside_ip);
  match.outer.ip4.dst_ip = DOCA_HTOBE32(session->remote_ip);
  match.meta.pkt_meta = DOCA_HTOBE32(
      eswitch_metadata_encode(slot->vswitch_id, session->inside.port_id));
  mask.meta.pkt_meta = UINT32_MAX;
  memcpy(match.outer.eth.src_mac, session->inside.mac, RTE_ETHER_ADDR_LEN);
  memset(mask.outer.eth.src_mac, UINT8_MAX, RTE_ETHER_ADDR_LEN);
  mask.outer.ip4.src_ip = UINT32_MAX;
  mask.outer.ip4.dst_ip = UINT32_MAX;
  mask.parser_meta.outer_l4_type = UINT32_MAX;
  /* Control entries use ordinary bit masks, not ACL range upper bounds. */
  if (session->protocol == IPPROTO_TCP) {
    match.parser_meta.outer_l4_type = DOCA_FLOW_L4_META_TCP;
    match.outer.l4_type_ext = DOCA_FLOW_L4_TYPE_EXT_TCP;
    match.outer.tcp.l4_port.src_port = DOCA_HTOBE16(session->inside_port);
    match.outer.tcp.l4_port.dst_port = DOCA_HTOBE16(session->remote_port);
    mask.outer.tcp.l4_port.src_port = UINT16_MAX;
    mask.outer.tcp.l4_port.dst_port = UINT16_MAX;
  } else {
    match.parser_meta.outer_l4_type = DOCA_FLOW_L4_META_UDP;
    match.outer.l4_type_ext = DOCA_FLOW_L4_TYPE_EXT_UDP;
    match.outer.udp.l4_port.src_port = DOCA_HTOBE16(session->inside_port);
    match.outer.udp.l4_port.dst_port = DOCA_HTOBE16(session->remote_port);
    mask.outer.udp.l4_port.src_port = UINT16_MAX;
    mask.outer.udp.l4_port.dst_port = UINT16_MAX;
  }
  flow_entry_cookie_prepare(&free_entry->rule.cookie,
                            "port-forward reply exception", DOCA_FLOW_ENTRY_OP_ADD);
  stage = "entry-add";
  result = doca_flow_pipe_control_add_entry(
      pipeline->runtime->queue_id, slot->pf_reply_pipe, &match, &mask, 0,
      NULL, NULL, NULL, NULL, 0, &fwd,
      &free_entry->rule.cookie, &free_entry->rule.entry);
  if (result == DOCA_SUCCESS) {
    stage = "entries-process";
    result = process_rules(pipeline, &free_entry->rule, 1);
  }
  if (result != DOCA_SUCCESS)
    goto fallback;
  free_entry->session = session;
  free_entry->vr_id = session->vr_id;
  free_entry->protocol = session->protocol;
  free_entry->inside_ip = session->inside_ip;
  free_entry->inside_port = session->inside_port;
  free_entry->remote_ip = session->remote_ip;
  free_entry->remote_port = session->remote_port;
  slot->pf_reply_count++;
  return DOCA_SUCCESS;
fallback:
  pipeline->egress_acl_failures++;
  fprintf(stderr, "Port-forward reply ACL fallback: vr=%u rif=%u stage=%s protocol=%u error=%s\n",
          session->vr_id, guest_interface_id, stage,
          (unsigned int)session->protocol,
          doca_error_get_descr(result));
  return acl_pf_reply_fallback(pipeline, config, slot);
}

static doca_error_t acl_select_target(struct eswitch_pipeline *pipeline,
                                      struct eswitch_egress_acl *slot,
                                      const struct router_interface *rif,
                                      struct doca_flow_pipe *target) {
  struct doca_flow_fwd fwd = {.type = DOCA_FLOW_FWD_PIPE,
                              .next_pipe = target};
  bool adding = slot->selector.entry == NULL;
  doca_error_t result;

  if (!adding) {
    flow_entry_cookie_prepare(&slot->selector.cookie,
                              "swap guest egress ACL", DOCA_FLOW_ENTRY_OP_UPD);
    result = doca_flow_pipe_basic_update_entry(
        pipeline->runtime->queue_id, pipeline->egress_acl_selector_pipe,
        0, NULL, NULL, &fwd, DOCA_FLOW_ENTRY_FLAGS_NO_WAIT,
        slot->selector.entry);
  } else {
    struct doca_flow_match match = {0};
    match.meta.pkt_meta = DOCA_HTOBE32((uint32_t)rif->vswitch_id << 16);
    memcpy(match.outer.eth.dst_mac, rif->mac, RTE_ETHER_ADDR_LEN);
    match.outer.eth.type = DOCA_HTOBE16(RTE_ETHER_TYPE_IPV4);
    flow_entry_cookie_prepare(&slot->selector.cookie,
                              "select guest egress ACL", DOCA_FLOW_ENTRY_OP_ADD);
    result = doca_flow_pipe_basic_add_entry(
        pipeline->runtime->queue_id, pipeline->egress_acl_selector_pipe,
        &match, 0, NULL, NULL, &fwd, DOCA_FLOW_ENTRY_FLAGS_NO_WAIT,
        &slot->selector.cookie, &slot->selector.entry);
  }
  if (result != DOCA_SUCCESS)
    return result;
  result = process_rules(pipeline, &slot->selector, 1);
  if (result != DOCA_SUCCESS && adding)
    (void)remove_rule(pipeline, &slot->selector,
                      "rollback guest egress ACL selector");
  return result;
}

doca_error_t eswitch_pipeline_egress_acl_sync(
    struct eswitch_pipeline *pipeline, const struct router_config *config) {
  if (pipeline == NULL || config == NULL || !pipeline->created)
    return DOCA_ERROR_INVALID_VALUE;
  if (pipeline->egress_acl_selector_pipe == NULL)
    return DOCA_SUCCESS; /* Existing Arm policy remains authoritative. */

  for (size_t p = 0; p < config->egress_policy_count; p++) {
    const struct router_egress_policy *policy = &config->egress_policies[p];
    const struct router_interface *rif = acl_rif(config, policy->interface_id);
    const struct router_egress_rule *ordered[ROUTER_MAX_EGRESS_RULES];
    struct eswitch_egress_acl *slot = acl_slot(pipeline, policy->interface_id);
    struct doca_flow_pipe *next_pipe = NULL, *old_pipe;
    struct doca_flow_pipe *next_pf_pipe = NULL, *old_pf_pipe;
    struct eswitch_rule *next_rules = NULL, *old_rules;
    struct eswitch_pf_reply_exception *next_pf_replies = NULL;
    struct eswitch_pf_reply_exception *old_pf_replies;
    uint64_t fingerprint;
    size_t count = 0;
    bool fallback, has_pf = false;
    doca_error_t result;

    if (rif == NULL || rif->attachment != ROUTER_VSWITCH || slot == NULL)
      return DOCA_ERROR_BAD_STATE;
    /* An address may be staged after policy creation. No RIF data-plane
     * context exists yet, so there is nothing to select or offload. */
    if (!rif->has_address)
      continue;
    for (size_t i = 0; i < config->egress_rule_count; i++)
      if (config->egress_rules[i].vr_id == policy->vr_id &&
          config->egress_rules[i].interface_id == policy->interface_id)
        ordered[count++] = &config->egress_rules[i];
    qsort(ordered, count, sizeof(ordered[0]), acl_rule_order);
    for (size_t i = 0; i < config->port_forward_count; i++)
      if (config->port_forwards[i].vr_id == policy->vr_id)
        has_pf = true;
    fingerprint = acl_fingerprint(config, policy, ordered, count);
    fallback = !acl_can_offload(config, policy, ordered, count);
    if (slot->active && slot->vr_id == policy->vr_id &&
        slot->vswitch_id == rif->vswitch_id &&
        memcmp(slot->rif_mac, rif->mac, 6) == 0 &&
        slot->fingerprint == fingerprint &&
        slot->fallback_arm == fallback)
      continue;
    if (!fallback) {
      result = acl_build_generation(pipeline, config, policy, ordered, count,
                                    &next_pipe, &next_rules);
      if (result == DOCA_SUCCESS && has_pf) {
        /* The extra slot owns the miss cookie; prune/flush iterate only
         * session slots and never remove the policy fallback. */
        next_pf_replies = calloc(ESWITCH_MAX_PF_REPLY_EXCEPTIONS + 1,
                                 sizeof(*next_pf_replies));
        if (next_pf_replies == NULL)
          result = DOCA_ERROR_NO_MEMORY;
        else
          result = acl_build_pf_reply_pipe(pipeline, next_pipe,
              &next_pf_replies[ESWITCH_MAX_PF_REPLY_EXCEPTIONS].rule,
              &next_pf_pipe);
      }
      if (result != DOCA_SUCCESS) {
        pipeline->egress_acl_failures++;
        fprintf(stderr, "Guest egress ACL fallback: vr=%u rif=%u error=%s\n",
                policy->vr_id, policy->interface_id,
                doca_error_get_descr(result));
        if (next_pf_pipe != NULL)
          doca_flow_pipe_destroy(next_pf_pipe);
        if (next_pipe != NULL)
          doca_flow_pipe_destroy(next_pipe);
        free(next_pf_replies);
        free(next_rules);
        next_pf_pipe = NULL;
        next_pipe = NULL;
        next_pf_replies = NULL;
        next_rules = NULL;
        fallback = true;
      }
    }
    result = acl_select_target(pipeline, slot, rif,
                               fallback ? pipeline->rss_pipe :
                               (next_pf_pipe != NULL ? next_pf_pipe : next_pipe));
    if (result != DOCA_SUCCESS) {
      if (next_pf_pipe != NULL)
        doca_flow_pipe_destroy(next_pf_pipe);
      if (next_pipe != NULL)
        doca_flow_pipe_destroy(next_pipe);
      free(next_pf_replies);
      free(next_rules);
      return result;
    }
    old_pipe = slot->pipe;
    old_pf_pipe = slot->pf_reply_pipe;
    old_rules = slot->rules;
    old_pf_replies = slot->pf_replies;
    slot->vr_id = policy->vr_id;
    slot->interface_id = policy->interface_id;
    slot->vswitch_id = rif->vswitch_id;
    memcpy(slot->rif_mac, rif->mac, 6);
    slot->fingerprint = fingerprint;
    slot->pipe = next_pipe;
    slot->pf_reply_pipe = next_pf_pipe;
    slot->rules = next_rules;
    slot->pf_replies = next_pf_replies;
    slot->pf_reply_count = 0;
    slot->rule_count = fallback ? 0 : count;
    slot->fallback_arm = fallback;
    slot->active = true;
    if (old_pf_pipe != NULL)
      doca_flow_pipe_destroy(old_pf_pipe);
    if (old_pipe != NULL)
      doca_flow_pipe_destroy(old_pipe);
    free(old_pf_replies);
    free(old_rules);
  }
  for (size_t s = 0; s < ROUTER_MAX_EGRESS_POLICIES; s++) {
    struct eswitch_egress_acl *slot = &pipeline->egress_acls[s];
    bool keep = false;
    doca_error_t result;

    if (!slot->active)
      continue;
    for (size_t p = 0; p < config->egress_policy_count; p++)
      if (config->egress_policies[p].interface_id == slot->interface_id)
        keep = true;
    if (keep)
      continue;
    result = remove_rule(pipeline, &slot->selector,
                         "remove guest egress selector");
    if (result != DOCA_SUCCESS)
      return result;
    if (slot->pf_reply_pipe != NULL)
      doca_flow_pipe_destroy(slot->pf_reply_pipe);
    if (slot->pipe != NULL)
      doca_flow_pipe_destroy(slot->pipe);
    free(slot->rules);
    free(slot->pf_replies);
    *slot = (struct eswitch_egress_acl){0};
  }
  return DOCA_SUCCESS;
}

static doca_error_t add_route_selector_rule(
    struct eswitch_pipeline *pipeline,
    struct eswitch_sf_return_context *context) {
  struct doca_flow_match match = {0};
  doca_error_t result;

  match.meta.pkt_meta = DOCA_HTOBE32((uint32_t)context->vswitch_id << 16);
  memcpy(match.outer.eth.dst_mac, context->rif_mac, RTE_ETHER_ADDR_LEN);
  match.outer.eth.type = DOCA_HTOBE16(RTE_ETHER_TYPE_IPV4);
  flow_entry_cookie_prepare(&context->route_selector_rule.cookie,
                            "private router MAC selector",
                            DOCA_FLOW_ENTRY_OP_ADD);
  result = doca_flow_pipe_basic_add_entry(
      pipeline->runtime->queue_id, pipeline->route_selector_pipe, &match, 0,
      NULL, NULL, NULL, DOCA_FLOW_ENTRY_FLAGS_NO_WAIT,
      &context->route_selector_rule.cookie,
      &context->route_selector_rule.entry);
  if (result != DOCA_SUCCESS)
    return result;
  return process_rules(pipeline, &context->route_selector_rule, 1);
}

static doca_error_t add_route_eligible_rule(
    struct eswitch_pipeline *pipeline,
    struct eswitch_sf_return_context *context) {
  struct doca_flow_match match = {0};
  struct doca_flow_match mask = {0};
  struct doca_flow_match_condition condition = {0};
  struct doca_flow_actions actions = {0};
  struct doca_flow_fwd fwd = {.type = DOCA_FLOW_FWD_PIPE,
                              .next_pipe = pipeline->route_lpm_pipe};
  doca_error_t result;

  match.meta.pkt_meta = DOCA_HTOBE32((uint32_t)context->vswitch_id << 16);
  mask.meta.pkt_meta = DOCA_HTOBE32(ESWITCH_METADATA_VSWITCH_MASK);
  memset(mask.outer.eth.dst_mac, UINT8_MAX, RTE_ETHER_ADDR_LEN);
  memcpy(match.outer.eth.dst_mac, context->rif_mac, RTE_ETHER_ADDR_LEN);
  match.outer.eth.type = DOCA_HTOBE16(RTE_ETHER_TYPE_IPV4);
  mask.outer.eth.type = UINT16_MAX;
  match.outer.ip4.version_ihl = 0x45;
  mask.outer.ip4.version_ihl = UINT8_MAX;
  match.parser_meta.outer_l3_type = DOCA_FLOW_L3_META_IPV4;
  mask.parser_meta.outer_l3_type = UINT32_MAX;
  match.parser_meta.outer_ip_fragmented = 0;
  mask.parser_meta.outer_ip_fragmented = UINT8_MAX;
  match.parser_meta.outer_l3_ok = 1;
  mask.parser_meta.outer_l3_ok = UINT8_MAX;
  match.parser_meta.outer_ip4_checksum_ok = 1;
  mask.parser_meta.outer_ip4_checksum_ok = UINT8_MAX;
  match.outer.ip4.ttl = 1;
  condition.operation = DOCA_FLOW_COMPARE_GT;
  condition.field_op.a.field_string = "outer.ipv4.ttl";
  condition.field_op.a.bit_offset = 0;
  condition.field_op.b.field_string = NULL;
  condition.field_op.b.bit_offset = 0;
  condition.field_op.width = 8;
  actions.meta.u32[1] = DOCA_HTOBE32(context->vr_id);

  flow_entry_cookie_prepare(&context->route_eligible_rule.cookie,
                            "private IPv4 hardware eligibility",
                            DOCA_FLOW_ENTRY_OP_ADD);
  result = doca_flow_pipe_control_add_entry(
      pipeline->runtime->queue_id, pipeline->route_control_pipe,
      &match, &mask, &condition, &actions, NULL, NULL, NULL, 0, &fwd,
      &context->route_eligible_rule.cookie,
      &context->route_eligible_rule.entry);
  if (result != DOCA_SUCCESS)
    return result;
  return process_rules(pipeline, &context->route_eligible_rule, 1);
}

static doca_error_t create_ingress_classifier(
    struct eswitch_pipeline *pipeline) {
  struct doca_flow_pipe_cfg *cfg = NULL;
  struct doca_flow_match match = {0};
  struct doca_flow_match match_mask = {0};
  struct doca_flow_actions actions = {0};
  struct doca_flow_monitor monitor = {0};
  struct doca_flow_fwd fwd = {0};
  doca_error_t result;

  monitor.counter_type = DOCA_FLOW_RESOURCE_TYPE_NON_SHARED;
  result = doca_flow_pipe_cfg_create(&cfg, pipeline->switch_port);
  if (result != DOCA_SUCCESS)
    return result;
  result = set_pipe_identity(cfg, "ESW_INGRESS_CLASSIFIER",
                             DOCA_FLOW_PIPE_CONTROL, true,
                             ESWITCH_MAX_VLAN_MEMBERSHIPS +
                                 pipeline->ports->count);
  if (result == DOCA_SUCCESS)
    result = doca_flow_pipe_create(cfg, NULL, NULL,
                                   &pipeline->ingress_classifier_pipe);
  doca_flow_pipe_cfg_destroy(cfg);
  if (result != DOCA_SUCCESS)
    return result;

  pipeline->classifier_rules = calloc(pipeline->ports->count,
                                      sizeof(*pipeline->classifier_rules));
  if (pipeline->classifier_rules == NULL)
    return DOCA_ERROR_NO_MEMORY;
  pipeline->uplink_arp_meter_rules = calloc(
      pipeline->ports->count, sizeof(*pipeline->uplink_arp_meter_rules));
  if (pipeline->uplink_arp_meter_rules == NULL)
    return DOCA_ERROR_NO_MEMORY;
  pipeline->uplink_catchall_rules = calloc(
      pipeline->ports->count, sizeof(*pipeline->uplink_catchall_rules));
  if (pipeline->uplink_catchall_rules == NULL)
    return DOCA_ERROR_NO_MEMORY;
  pipeline->egress_gates = calloc(pipeline->ports->count,
                                  sizeof(*pipeline->egress_gates));
  if (pipeline->egress_gates == NULL)
    return DOCA_ERROR_NO_MEMORY;

  for (uint16_t i = 0; i < pipeline->ports->count; i++) {
    const struct ethernet_port *port = pipeline->ports->items[i].ethernet;
    struct eswitch_rule *rule = &pipeline->sf_root_rule;

    if (port->role != ETHERNET_PORT_ROLE_SF_REPRESENTOR)
      continue;
    if (rule->entry != NULL)
      return DOCA_ERROR_BAD_STATE;
    memset(&match, 0, sizeof(match));
    memset(&match_mask, 0, sizeof(match_mask));
    memset(&actions, 0, sizeof(actions));
    match.parser_meta.port_id = port->port_id;
    match_mask.parser_meta.port_id = UINT16_MAX;
    actions.meta.pkt_meta = 0;
    fwd.type = DOCA_FLOW_FWD_PIPE;
    fwd.next_pipe = pipeline->sf_return_pipe;
    flow_entry_cookie_prepare(&rule->cookie, "attach system SF",
                              DOCA_FLOW_ENTRY_OP_ADD);
    result = doca_flow_pipe_control_add_entry(
        pipeline->runtime->queue_id, pipeline->ingress_classifier_pipe,
        &match, &match_mask, NULL, &actions, NULL, NULL, &monitor, 0, &fwd,
        &rule->cookie, &rule->entry);
    if (result != DOCA_SUCCESS)
      return result;
    result = process_rules(pipeline, rule, 1);
    if (result != DOCA_SUCCESS)
      return result;
    pipeline->sf_port_id = port->port_id;
  }
  return pipeline->sf_root_rule.entry == NULL ? DOCA_ERROR_NOT_FOUND
                                               : DOCA_SUCCESS;
}

/* DOCA Flow 3.4 exposes Ethernet and meter-color fields but not ARP opcode or
 * target protocol address. Preserve every unicast ARP reply and rate-limit
 * only broadcast ARP before it reaches the Arm slow path. The software ARP
 * parser remains the exact TPA/opcode authority for the admitted packets. */
static doca_error_t create_uplink_arp_classifier(
    struct eswitch_pipeline *pipeline) {
  struct doca_flow_pipe_cfg *cfg = NULL;
  struct doca_flow_match match = {0};
  struct doca_flow_match mask = {0};
  struct doca_flow_fwd rss = {.type = DOCA_FLOW_FWD_PIPE,
                              .next_pipe = pipeline->rss_pipe};
  struct doca_flow_fwd drop = {.type = DOCA_FLOW_FWD_DROP};
  const enum doca_flow_meter_color colors[2] = {
      DOCA_FLOW_METER_COLOR_GREEN, DOCA_FLOW_METER_COLOR_YELLOW};
  doca_error_t result;

  match.parser_meta.meter_color =
      (enum doca_flow_meter_color)UINT32_MAX;
  mask.parser_meta.meter_color =
      (enum doca_flow_meter_color)UINT32_MAX;
  result = doca_flow_pipe_cfg_create(&cfg, pipeline->switch_port);
  if (result != DOCA_SUCCESS)
    return result;
  result = set_pipe_identity(cfg, "ESW_UPLINK_ARP_COLOR",
                             DOCA_FLOW_PIPE_BASIC, false, 2);
  if (result == DOCA_SUCCESS)
    result = doca_flow_pipe_cfg_set_match(cfg, &match, &mask);
  if (result == DOCA_SUCCESS)
    result = doca_flow_pipe_cfg_set_miss_counter(cfg, true);
  if (result == DOCA_SUCCESS)
    result = doca_flow_pipe_create(cfg, &rss, &drop,
                                   &pipeline->uplink_arp_color_pipe);
  doca_flow_pipe_cfg_destroy(cfg);
  if (result != DOCA_SUCCESS)
    return result;

  for (uint32_t i = 0; i < 2; i++) {
    struct eswitch_rule *rule = &pipeline->uplink_arp_color_rules[i];

    memset(&match, 0, sizeof(match));
    match.parser_meta.meter_color = colors[i];
    flow_entry_cookie_prepare(&rule->cookie,
                              i == 0 ? "uplink ARP meter green"
                                     : "uplink ARP meter yellow",
                              DOCA_FLOW_ENTRY_OP_ADD);
    result = doca_flow_pipe_basic_add_entry(
        pipeline->runtime->queue_id, pipeline->uplink_arp_color_pipe,
        &match, 0, NULL, NULL, NULL, batch_flags(i, 2),
        &rule->cookie, &rule->entry);
    if (result != DOCA_SUCCESS)
      return result;
  }
  result = process_rules(pipeline, pipeline->uplink_arp_color_rules, 2);
  if (result != DOCA_SUCCESS)
    return result;

  result = doca_flow_pipe_cfg_create(&cfg, pipeline->switch_port);
  if (result != DOCA_SUCCESS)
    return result;
  result = set_pipe_identity(cfg, "ESW_UPLINK_DISPATCH",
                             DOCA_FLOW_PIPE_CONTROL, false,
                             pipeline->ports->count * 2U);
  if (result == DOCA_SUCCESS)
    result = doca_flow_pipe_create(cfg, NULL, NULL,
                                   &pipeline->uplink_dispatch_pipe);
  doca_flow_pipe_cfg_destroy(cfg);
  return result;
}

doca_error_t eswitch_pipeline_uplink_arp_drop_query(
    const struct eswitch_pipeline *pipeline, uint64_t *packets) {
  struct doca_flow_resource_query query = {0};
  doca_error_t result;

  if (pipeline == NULL || packets == NULL)
    return DOCA_ERROR_INVALID_VALUE;
  if (!pipeline->uplink_arp_filter_enabled ||
      pipeline->uplink_arp_color_pipe == NULL)
    return DOCA_ERROR_NOT_FOUND;
  result = doca_flow_resource_query_pipe_miss(
      pipeline->uplink_arp_color_pipe, &query);
  if (result == DOCA_SUCCESS)
    *packets = query.counter.total_pkts;
  return result;
}

/* Packets created by Arm enter through the actual SF MAC with an internal VLAN
 * context. Hardware removes that private tag and restores the virtual RIF
 * source MAC. A directed context jumps to one resolved egress gate; a flood
 * context uses the normal VS FDB/flood path. Unknown tags fail closed. */
static doca_error_t create_sf_return(struct eswitch_pipeline *pipeline) {
  struct doca_flow_pipe_cfg *cfg = NULL;
  struct doca_flow_match match = {0};
  struct doca_flow_actions actions = {0};
  struct doca_flow_actions *actions_array[1] = {&actions};
  struct doca_flow_monitor monitor = {0};
  struct doca_flow_fwd fwd = {.type = DOCA_FLOW_FWD_CHANGEABLE};
  struct doca_flow_fwd miss = {.type = DOCA_FLOW_FWD_DROP};
  doca_error_t result;

  /* eth_vlan[] is ignored unless the corresponding valid-header bit is set.
   * Without it every context entry has an effective wildcard match; the most
   * recently installed entry then captures packets for the older tags. */
  match.outer.l2_valid_headers = DOCA_FLOW_L2_VALID_HEADER_VLAN_0;
  match.outer.eth_vlan[0].tci = UINT16_MAX;
  actions.pop_vlan = true;
  memset(actions.outer.eth.src_mac, UINT8_MAX, RTE_ETHER_ADDR_LEN);
  actions.meta.pkt_meta = UINT32_MAX;
  monitor.counter_type = DOCA_FLOW_RESOURCE_TYPE_NON_SHARED;
  result = doca_flow_pipe_cfg_create(&cfg, pipeline->switch_port);
  if (result != DOCA_SUCCESS)
    return result;
  result = set_pipe_identity(cfg, "ESW_SF_RETURN", DOCA_FLOW_PIPE_BASIC,
                             false, ESWITCH_MAX_SF_RETURN_CONTEXTS);
  if (result == DOCA_SUCCESS)
    result = doca_flow_pipe_cfg_set_match(cfg, &match, NULL);
  if (result == DOCA_SUCCESS)
    result = doca_flow_pipe_cfg_set_actions(cfg, actions_array, NULL, NULL, 1);
  if (result == DOCA_SUCCESS)
    result = doca_flow_pipe_cfg_set_monitor(cfg, &monitor);
  if (result == DOCA_SUCCESS)
    result = doca_flow_pipe_create(cfg, &fwd, &miss,
                                   &pipeline->sf_return_pipe);
  doca_flow_pipe_cfg_destroy(cfg);
  return result;
}

static doca_error_t bind_sf_return_context(
    struct eswitch_pipeline *pipeline, uint16_t vr_id, uint16_t interface_id,
    uint16_t vswitch_id, uint32_t rif_address, uint16_t target_port_id,
    bool directed, const uint8_t rif_mac[6], uint16_t *context_tag) {
  struct eswitch_sf_return_context *free_context = NULL;
  struct doca_flow_match return_match = {0};
  struct doca_flow_match local_match = {0};
  struct doca_flow_actions actions = {0};
  struct doca_flow_fwd fwd = {0};
  struct doca_flow_pipe *gate_pipe = NULL;
  doca_error_t result;

  if (pipeline == NULL || !pipeline->created || vr_id == 0 ||
      interface_id == 0 ||
      vswitch_id == 0 ||
      rif_mac == NULL || context_tag == NULL || (rif_mac[0] & 1U) != 0)
    return DOCA_ERROR_INVALID_VALUE;
  for (size_t i = 0; i < ESWITCH_MAX_SF_RETURN_CONTEXTS; i++) {
    struct eswitch_sf_return_context *context =
        &pipeline->sf_return_contexts[i];
    if (!context->active) {
      if (free_context == NULL)
        free_context = context;
      continue;
    }
    if (context->interface_id == interface_id &&
        context->directed == directed &&
        (!directed || context->target_port_id == target_port_id)) {
      if (context->vr_id != vr_id || context->vswitch_id != vswitch_id ||
          (!directed && context->rif_address != rif_address) ||
          memcmp(context->rif_mac, rif_mac, 6) != 0)
        return DOCA_ERROR_BAD_STATE;
      *context_tag = context->context_tag;
      return DOCA_SUCCESS;
    }
  }
  if (free_context == NULL)
    return DOCA_ERROR_NO_MEMORY;

  if (directed) {
    result = get_egress_gate(pipeline, vswitch_id, target_port_id, &gate_pipe);
    if (result != DOCA_SUCCESS) {
      fprintf(stderr, "SF RETURN RESOURCE ERROR: stage=egress-gate vs=%u "
                      "target=%u error=%s\n",
              vswitch_id, target_port_id, doca_error_get_descr(result));
      return result;
    }
    fwd.type = DOCA_FLOW_FWD_PIPE;
    fwd.next_pipe = gate_pipe;
  } else {
    fwd.type = DOCA_FLOW_FWD_PIPE;
    fwd.next_pipe = pipeline->destination_pipe;
  }

  *context_tag = (uint16_t)(free_context - pipeline->sf_return_contexts) + 1;
  return_match.outer.l2_valid_headers =
      DOCA_FLOW_L2_VALID_HEADER_VLAN_0;
  return_match.outer.eth_vlan[0].tci = DOCA_HTOBE16(*context_tag);
  memcpy(actions.outer.eth.src_mac, rif_mac, 6);
  actions.meta.pkt_meta = DOCA_HTOBE32(
      eswitch_metadata_encode(vswitch_id, pipeline->sf_port_id));
  flow_entry_cookie_prepare(&free_context->return_rule.cookie,
                            "bind SF return context", DOCA_FLOW_ENTRY_OP_ADD);
  result = doca_flow_pipe_basic_add_entry(
      pipeline->runtime->queue_id, pipeline->sf_return_pipe, &return_match, 0,
      &actions, NULL, &fwd, DOCA_FLOW_ENTRY_FLAGS_NO_WAIT,
      &free_context->return_rule.cookie, &free_context->return_rule.entry);
  if (result != DOCA_SUCCESS) {
    fprintf(stderr, "SF RETURN RESOURCE ERROR: stage=entry-add vs=%u "
                    "mode=%s target=%u error=%s\n",
            vswitch_id, directed ? "directed" : "flood",
            directed ? target_port_id : UINT16_MAX,
            doca_error_get_descr(result));
    return result;
  }
  result = process_rules(pipeline, &free_context->return_rule, 1);
  if (result != DOCA_SUCCESS) {
    doca_error_t original_error = result;
    fprintf(stderr, "SF RETURN RESOURCE ERROR: stage=entry-commit vs=%u "
                    "mode=%s target=%u error=%s\n",
            vswitch_id, directed ? "directed" : "flood",
            directed ? target_port_id : UINT16_MAX,
            doca_error_get_descr(result));
    doca_error_t cleanup = remove_rule(pipeline, &free_context->return_rule,
                                       "rollback SF return context");
    return cleanup == DOCA_SUCCESS ? original_error : cleanup;
  }

  if (!directed) {
    local_match.meta.pkt_meta =
        DOCA_HTOBE32((uint32_t)vswitch_id << 16);
    memcpy(local_match.outer.eth.dst_mac, rif_mac, 6);
    if (pipeline->egress_acl_selector_pipe != NULL)
      local_match.outer.ip4.dst_ip = DOCA_HTOBE32(rif_address);
    flow_entry_cookie_prepare(&free_context->local_ip_rule.cookie,
                              "bind local RIF IPv4", DOCA_FLOW_ENTRY_OP_ADD);
    result = doca_flow_pipe_basic_add_entry(
        pipeline->runtime->queue_id, pipeline->local_ip_pipe, &local_match, 0,
        NULL, NULL, NULL, DOCA_FLOW_ENTRY_FLAGS_NO_WAIT,
        &free_context->local_ip_rule.cookie,
        &free_context->local_ip_rule.entry);
    if (result == DOCA_SUCCESS)
      result = process_rules(pipeline, &free_context->local_ip_rule, 1);
    if (result != DOCA_SUCCESS) {
      doca_error_t original_error = result;
      fprintf(stderr, "SF RETURN RESOURCE ERROR: stage=local-ip vs=%u "
                      "mode=%s target=%u error=%s\n",
              vswitch_id, directed ? "directed" : "flood",
              directed ? target_port_id : UINT16_MAX,
              doca_error_get_descr(result));
      doca_error_t cleanup = remove_rule(pipeline, &free_context->local_ip_rule,
                                         "rollback local RIF IPv4");
      if (cleanup == DOCA_SUCCESS)
        cleanup = remove_rule(pipeline, &free_context->return_rule,
                              "rollback SF return context");
      return cleanup == DOCA_SUCCESS ? original_error : cleanup;
    }
    if (pipeline->route_selector_pipe != NULL) {
      free_context->vr_id = vr_id;
      free_context->interface_id = interface_id;
      free_context->vswitch_id = vswitch_id;
      free_context->rif_address = rif_address;
      memcpy(free_context->rif_mac, rif_mac, 6);
      /* Install the RIF selector first. Without LPM, its hit goes to RSS;
       * with LPM, route-control fallback still reaches RSS when optional
       * eligibility cannot be allocated. */
      result = add_route_selector_rule(pipeline, free_context);
      if (result != DOCA_SUCCESS) {
        doca_error_t original_error = result;
        fprintf(stderr, "SF RETURN RESOURCE ERROR: stage=route-selector "
                        "vs=%u error=%s\n",
                vswitch_id, doca_error_get_descr(result));
        doca_error_t cleanup = remove_rule(
            pipeline, &free_context->local_ip_rule,
            "rollback local RIF IPv4");
        if (cleanup == DOCA_SUCCESS)
          cleanup = remove_rule(pipeline, &free_context->return_rule,
                                "rollback SF return context");
        return cleanup == DOCA_SUCCESS ? original_error : cleanup;
      }
      if (pipeline->hardware_routing_enabled)
        result = add_route_eligible_rule(pipeline, free_context);
      if (result != DOCA_SUCCESS) {
        /* Hardware routing is optional. Preserve SF return and local-IP
         * entries, and preserve the selector so its fallback sends this RIF
         * to RSS. A later route sync retries only the missing eligibility
         * entry. */
        pipeline->hw_route_failures++;
        pipeline->hardware_routing_degraded = true;
        if (pipeline->hardware_ct_enabled)
          pipeline->hardware_ct_degraded = true;
        fprintf(stderr, "HARDWARE ROUTING DEGRADED: stage=route-eligible "
                        "vs=%u fallback=arm error=%s\n",
                vswitch_id, doca_error_get_descr(result));
      }
    }
  }

  free_context->vr_id = vr_id;
  free_context->interface_id = interface_id;
  free_context->vswitch_id = vswitch_id;
  free_context->rif_address = rif_address;
  free_context->context_tag = *context_tag;
  free_context->target_port_id = target_port_id;
  memcpy(free_context->rif_mac, rif_mac, 6);
  free_context->directed = directed;
  free_context->active = true;
  printf("SF RETURN BIND: vr=%u rif-id=%u vs=%u context-vlan=%u "
         "mode=%s target=%u rif="
         "%02x:%02x:%02x:%02x:%02x:%02x sf-port=%u\n",
         vr_id, interface_id, vswitch_id, *context_tag,
         directed ? "directed" : "flood",
         directed ? target_port_id : UINT16_MAX,
         rif_mac[0], rif_mac[1], rif_mac[2], rif_mac[3],
         rif_mac[4], rif_mac[5], pipeline->sf_port_id);
  return DOCA_SUCCESS;
}

doca_error_t eswitch_pipeline_sf_bind_vswitch(
    struct eswitch_pipeline *pipeline, uint16_t vr_id, uint16_t interface_id,
    uint16_t vswitch_id, uint32_t rif_address, const uint8_t rif_mac[6],
    uint16_t *context_tag) {
  return bind_sf_return_context(pipeline, vr_id, interface_id, vswitch_id,
                                rif_address, UINT16_MAX, false, rif_mac,
                                context_tag);
}

doca_error_t eswitch_pipeline_sf_bind_egress(
    struct eswitch_pipeline *pipeline, uint16_t vr_id, uint16_t interface_id,
    uint16_t vswitch_id, uint16_t target_port_id,
    const uint8_t rif_mac[6], uint16_t *context_tag) {
  return bind_sf_return_context(pipeline, vr_id, interface_id, vswitch_id, 0,
                                target_port_id, true, rif_mac, context_tag);
}

doca_error_t eswitch_pipeline_sf_unbind_egress(
    struct eswitch_pipeline *pipeline, uint16_t interface_id,
    uint16_t domain_id, uint16_t target_port_id) {
  if (pipeline == NULL || !pipeline->created || interface_id == 0 ||
      domain_id == 0)
    return DOCA_ERROR_INVALID_VALUE;
  for (size_t i = 0; i < ESWITCH_MAX_SF_RETURN_CONTEXTS; i++) {
    struct eswitch_sf_return_context *context =
        &pipeline->sf_return_contexts[i];
    doca_error_t result;

    if (!context->active || !context->directed ||
        context->interface_id != interface_id ||
        context->vswitch_id != domain_id ||
        context->target_port_id != target_port_id)
      continue;
    result = remove_rule(pipeline, &context->return_rule,
                         "unbind directed SF return context");
    if (result != DOCA_SUCCESS)
      return result;
    printf("SF RETURN UNBIND: domain=%u context-vlan=%u mode=directed "
           "target=%u\n",
           domain_id, context->context_tag, target_port_id);
    *context = (struct eswitch_sf_return_context){0};
    return DOCA_SUCCESS;
  }
  return DOCA_SUCCESS;
}

static doca_error_t remove_sf_return_context(
    struct eswitch_pipeline *pipeline,
    struct eswitch_sf_return_context *context) {
  doca_error_t result;

  if (context->route_selector_rule.entry != NULL) {
    result = remove_rule(pipeline, &context->route_selector_rule,
                         "unbind hardware router selector");
    if (result != DOCA_SUCCESS)
      return result;
  }
  if (context->route_eligible_rule.entry != NULL) {
    result = remove_rule(pipeline, &context->route_eligible_rule,
                         "unbind hardware route eligibility");
    if (result != DOCA_SUCCESS)
      return result;
  }
  if (context->local_ip_rule.entry != NULL) {
    result = remove_rule(pipeline, &context->local_ip_rule,
                         "unbind local RIF IPv4");
    if (result != DOCA_SUCCESS)
      return result;
  }
  result = remove_rule(pipeline, &context->return_rule,
                       "unbind SF return context");
  if (result != DOCA_SUCCESS)
    return result;
  printf("SF RETURN UNBIND: vr=%u rif-id=%u vs=%u context-vlan=%u "
         "mode=%s target=%u\n",
         context->vr_id, context->interface_id, context->vswitch_id,
         context->context_tag, context->directed ? "directed" : "flood",
         context->directed ? context->target_port_id : UINT16_MAX);
  *context = (struct eswitch_sf_return_context){0};
  return DOCA_SUCCESS;
}

doca_error_t eswitch_pipeline_sf_unbind_rif(
    struct eswitch_pipeline *pipeline, uint16_t interface_id) {
  if (pipeline == NULL || !pipeline->created || interface_id == 0)
    return DOCA_ERROR_INVALID_VALUE;
  for (size_t i = 0; i < ROUTER_MAX_EGRESS_POLICIES; i++) {
    struct eswitch_egress_acl *slot = &pipeline->egress_acls[i];
    doca_error_t result;
    if (!slot->active || slot->interface_id != interface_id)
      continue;
    result = remove_rule(pipeline, &slot->selector,
                         "unbind guest egress ACL selector");
    if (result != DOCA_SUCCESS)
      return result;
    if (slot->pf_reply_pipe != NULL)
      doca_flow_pipe_destroy(slot->pf_reply_pipe);
    if (slot->pipe != NULL)
      doca_flow_pipe_destroy(slot->pipe);
    free(slot->pf_replies);
    free(slot->rules);
    *slot = (struct eswitch_egress_acl){0};
  }
  for (size_t i = 0; i < ESWITCH_MAX_SF_RETURN_CONTEXTS; i++) {
    struct eswitch_sf_return_context *context =
        &pipeline->sf_return_contexts[i];
    doca_error_t result;

    if (!context->active || context->interface_id != interface_id)
      continue;
    result = remove_sf_return_context(pipeline, context);
    if (result != DOCA_SUCCESS)
      return result;
  }
  return DOCA_SUCCESS;
}

doca_error_t eswitch_pipeline_sf_unbind_vswitch(
    struct eswitch_pipeline *pipeline, uint16_t vswitch_id) {
  if (pipeline == NULL || !pipeline->created || vswitch_id == 0)
    return DOCA_ERROR_INVALID_VALUE;
  for (size_t i = 0; i < ESWITCH_MAX_SF_RETURN_CONTEXTS; i++) {
    struct eswitch_sf_return_context *context =
        &pipeline->sf_return_contexts[i];
    if (!context->active || context->vswitch_id != vswitch_id)
      continue;
    doca_error_t result = remove_sf_return_context(pipeline, context);
    if (result != DOCA_SUCCESS)
      return result;
  }
  return DOCA_SUCCESS;
}

doca_error_t eswitch_pipeline_sf_query_counters(
    const struct eswitch_pipeline *pipeline, uint64_t *ingress_packets,
    uint64_t *context_packets, uint64_t *local_ip_packets) {
  struct doca_flow_resource_query query = {0};
  doca_error_t result;

  if (pipeline == NULL || ingress_packets == NULL ||
      context_packets == NULL || local_ip_packets == NULL ||
      pipeline->sf_root_rule.entry == NULL)
    return DOCA_ERROR_INVALID_VALUE;

  result = doca_flow_resource_query_entry(pipeline->sf_root_rule.entry,
                                          &query);
  if (result != DOCA_SUCCESS)
    return result;
  *ingress_packets = query.counter.total_pkts;
  *context_packets = 0;
  *local_ip_packets = 0;

  for (size_t i = 0; i < ESWITCH_MAX_SF_RETURN_CONTEXTS; i++) {
    const struct eswitch_sf_return_context *context =
        &pipeline->sf_return_contexts[i];

    if (!context->active)
      continue;
    memset(&query, 0, sizeof(query));
    result = doca_flow_resource_query_entry(context->return_rule.entry,
                                            &query);
    if (result != DOCA_SUCCESS)
      return result;
    *context_packets += query.counter.total_pkts;
    if (context->local_ip_rule.entry != NULL) {
      memset(&query, 0, sizeof(query));
      result = doca_flow_resource_query_entry(context->local_ip_rule.entry,
                                              &query);
      if (result != DOCA_SUCCESS)
        return result;
      *local_ip_packets += query.counter.total_pkts;
    }
  }
  return DOCA_SUCCESS;
}

doca_error_t eswitch_pipeline_sf_context_query(
    const struct eswitch_sf_return_context *context, uint64_t *packets) {
  struct doca_flow_resource_query query = {0};
  doca_error_t result;

  if (context == NULL || packets == NULL || !context->active ||
      context->return_rule.entry == NULL)
    return DOCA_ERROR_INVALID_VALUE;
  result = doca_flow_resource_query_entry(context->return_rule.entry, &query);
  if (result != DOCA_SUCCESS)
    return result;
  *packets = query.counter.total_pkts;
  return DOCA_SUCCESS;
}

doca_error_t eswitch_pipeline_destination_miss_query(
    const struct eswitch_pipeline *pipeline, uint64_t *packets) {
  struct doca_flow_resource_query query = {0};
  doca_error_t result;

  if (pipeline == NULL || packets == NULL || !pipeline->created ||
      pipeline->destination_pipe == NULL)
    return DOCA_ERROR_INVALID_VALUE;

  result = doca_flow_resource_query_pipe_miss(pipeline->destination_pipe,
                                              &query);
  if (result != DOCA_SUCCESS)
    return result;
  *packets = query.counter.total_pkts;
  return DOCA_SUCCESS;
}

doca_error_t eswitch_pipeline_egress_query(
    const struct eswitch_pipeline *pipeline, uint16_t port_id,
    uint64_t *forward_packets, uint64_t *split_horizon_drops) {
  struct doca_flow_resource_query query = {0};
  const struct eswitch_egress_gate *gate;
  bool found = false;
  int port_index;
  doca_error_t result;

  if (pipeline == NULL || forward_packets == NULL ||
      split_horizon_drops == NULL || !pipeline->created)
    return DOCA_ERROR_INVALID_VALUE;
  port_index = find_port_index(pipeline, port_id);
  if (port_index < 0)
    return DOCA_ERROR_NOT_FOUND;
  *forward_packets = 0;
  *split_horizon_drops = 0;
  for (size_t i = 0; i <= ESWITCH_MAX_VLAN_MEMBERSHIPS; i++) {
    if (i == ESWITCH_MAX_VLAN_MEMBERSHIPS) {
      gate = &pipeline->egress_gates[port_index];
    } else {
      const struct eswitch_pipeline_membership *member =
          &pipeline->memberships[i];
      if (!member->active || member->port_id != port_id)
        continue;
      gate = &member->egress;
    }
    if (gate->pipe == NULL || gate->drop_self.entry == NULL ||
        (gate->forward.entry == NULL && gate->range_forward_count == 0))
      continue;
    found = true;
    if (gate->forward.entry != NULL) {
      memset(&query, 0, sizeof(query));
      result = doca_flow_resource_query_entry(gate->forward.entry, &query);
      if (result != DOCA_SUCCESS)
        return result;
      *forward_packets += query.counter.total_pkts;
    }
    for (uint16_t j = 0; j < gate->range_forward_count; j++) {
      memset(&query, 0, sizeof(query));
      result = doca_flow_resource_query_entry(
          gate->range_forwards[j].entry, &query);
      if (result != DOCA_SUCCESS)
        return result;
      *forward_packets += query.counter.total_pkts;
    }
    memset(&query, 0, sizeof(query));
    result = doca_flow_resource_query_entry(gate->drop_self.entry, &query);
    if (result != DOCA_SUCCESS)
      return result;
    *split_horizon_drops += query.counter.total_pkts;
  }
  return found ? DOCA_SUCCESS : DOCA_ERROR_NOT_FOUND;
}

static doca_error_t ct_bind_adjacency(
    struct eswitch_pipeline *pipeline, uint16_t target_vswitch,
    uint16_t target_port,
    const uint8_t source_mac[6], const uint8_t destination_mac[6],
    uint32_t *adjacency_id) {
  struct eswitch_ct_adjacency *free_entry = NULL;
  struct doca_flow_pipe *gate = NULL;
  struct doca_flow_match match = {0};
  struct doca_flow_actions actions = {0};
  struct doca_flow_fwd fwd = {0};
  doca_error_t result;

  for (size_t i = 0; i < ESWITCH_MAX_CT_ADJACENCIES; i++) {
    struct eswitch_ct_adjacency *entry = &pipeline->ct_adjacencies[i];

    if (!entry->active) {
      if (entry->rule.entry == NULL && free_entry == NULL)
        free_entry = entry;
      continue;
    }
    if (entry->vswitch_id == target_vswitch &&
        entry->target_port_id == target_port &&
        memcmp(entry->source_mac, source_mac, 6) == 0 &&
        memcmp(entry->destination_mac, destination_mac, 6) == 0) {
      *adjacency_id = entry->id;
      return DOCA_SUCCESS;
    }
  }
  if (free_entry == NULL)
    return DOCA_ERROR_FULL;
  result = get_egress_gate(pipeline, target_vswitch, target_port, &gate);
  if (result != DOCA_SUCCESS)
    return result;

  free_entry->id = (uint32_t)(free_entry - pipeline->ct_adjacencies) + 1U;
  free_entry->vswitch_id = target_vswitch;
  free_entry->target_port_id = target_port;
  memcpy(free_entry->source_mac, source_mac, 6);
  memcpy(free_entry->destination_mac, destination_mac, 6);
  match.meta.u32[0] = DOCA_HTOBE32(free_entry->id);
  memcpy(actions.outer.eth.src_mac, source_mac, 6);
  memcpy(actions.outer.eth.dst_mac, destination_mac, 6);
  actions.outer.l3_type = DOCA_FLOW_L3_TYPE_IP4;
  actions.outer.ip4.ttl = UINT8_MAX;
  fwd.type = DOCA_FLOW_FWD_PIPE;
  fwd.next_pipe = gate;
  flow_entry_cookie_prepare(&free_entry->rule.cookie, "CT adjacency",
                            DOCA_FLOW_ENTRY_OP_ADD);
  result = doca_flow_pipe_basic_add_entry(
      pipeline->runtime->queue_id, pipeline->ct_egress_pipe, &match, 0,
      &actions, NULL, &fwd, DOCA_FLOW_ENTRY_FLAGS_NO_WAIT,
      &free_entry->rule.cookie, &free_entry->rule.entry);
  if (result == DOCA_SUCCESS)
    result = process_rules(pipeline, &free_entry->rule, 1);
  if (result != DOCA_SUCCESS) {
    doca_error_t cleanup = remove_rule(pipeline, &free_entry->rule,
                                       "rollback CT adjacency");
    if (cleanup == DOCA_SUCCESS)
      memset(free_entry, 0, sizeof(*free_entry));
    return result;
  }
  free_entry->active = true;
  *adjacency_id = free_entry->id;
  return DOCA_SUCCESS;
}

static struct eswitch_ct_session *ct_session_slot(
    struct eswitch_pipeline *pipeline,
    const struct router_nat_session *software) {
  struct eswitch_ct_session *free_entry = NULL;

  for (size_t i = 0; i < ROUTER_NAT_MAX_SESSIONS; i++) {
    struct eswitch_ct_session *entry = &pipeline->ct_sessions[i];

    if (entry->active && entry->software == software)
      return entry;
    if (!entry->active && entry->entry == NULL && free_entry == NULL)
      free_entry = entry;
  }
  return free_entry;
}

static doca_error_t ct_admit_direction(
    struct eswitch_pipeline *pipeline, struct eswitch_ct_session *hardware,
    unsigned direction, uint16_t vs, uint16_t port, const uint8_t rif_mac[6],
    const uint8_t peer_mac[6], uint32_t source_ip, uint32_t destination_ip,
    uint16_t source_port, uint16_t destination_port, const char **stage) {
  const struct router_nat_session *session = hardware->software;
  struct doca_flow_match match = {0}, mask = {0};
  struct doca_flow_actions actions = {0};
  struct doca_flow_fwd fwd = {.type = DOCA_FLOW_FWD_PIPE,
                             .next_pipe = pipeline->ct_guard_pipe};
  struct eswitch_rule *rule = &hardware->admission[direction];
  doca_error_t result;

  match.meta.pkt_meta = DOCA_HTOBE32(eswitch_metadata_encode(vs, port));
  mask.meta.pkt_meta = UINT32_MAX;
  memcpy(match.outer.eth.dst_mac, rif_mac, 6);
  memcpy(match.outer.eth.src_mac, peer_mac, 6);
  memset(mask.outer.eth.dst_mac, UINT8_MAX, 6);
  memset(mask.outer.eth.src_mac, UINT8_MAX, 6);
  match.parser_meta.outer_l3_type = DOCA_FLOW_L3_META_IPV4;
  mask.parser_meta.outer_l3_type = UINT32_MAX;
  match.parser_meta.outer_l4_type = session->protocol == IPPROTO_TCP
      ? DOCA_FLOW_L4_META_TCP : DOCA_FLOW_L4_META_UDP;
  mask.parser_meta.outer_l4_type = UINT32_MAX;
  match.outer.l3_type = DOCA_FLOW_L3_TYPE_IP4;
  match.outer.ip4.src_ip = DOCA_HTOBE32(source_ip);
  match.outer.ip4.dst_ip = DOCA_HTOBE32(destination_ip);
  mask.outer.ip4.src_ip = mask.outer.ip4.dst_ip = UINT32_MAX;
  if (session->protocol == IPPROTO_TCP) {
    match.outer.l4_type_ext = DOCA_FLOW_L4_TYPE_EXT_TCP;
    match.outer.tcp.l4_port.src_port = DOCA_HTOBE16(source_port);
    match.outer.tcp.l4_port.dst_port = DOCA_HTOBE16(destination_port);
    mask.outer.tcp.l4_port.src_port = UINT16_MAX;
    mask.outer.tcp.l4_port.dst_port = UINT16_MAX;
  } else {
    match.outer.l4_type_ext = DOCA_FLOW_L4_TYPE_EXT_UDP;
    match.outer.udp.l4_port.src_port = DOCA_HTOBE16(source_port);
    match.outer.udp.l4_port.dst_port = DOCA_HTOBE16(destination_port);
    mask.outer.udp.l4_port.src_port = UINT16_MAX;
    mask.outer.udp.l4_port.dst_port = UINT16_MAX;
  }
  /* Each connection owns a zone. Overlapping guest tuples on different
   * RIFs cannot hit another connection's VR-wide CT key. */
  actions.meta.u32[1] = DOCA_HTOBE32(
      (uint32_t)(hardware - pipeline->ct_sessions) + 1);
  flow_entry_cookie_prepare(&rule->cookie, "authorize NAT CT tuple",
                            DOCA_FLOW_ENTRY_OP_ADD);
  *stage = direction == 0 ? "origin-admission-add" : "reply-admission-add";
  result = doca_flow_pipe_control_add_entry(
      pipeline->runtime->queue_id, pipeline->ct_admission_pipe,
      &match, &mask, NULL, &actions, NULL, NULL, NULL, 0, &fwd,
      &rule->cookie, &rule->entry);
  if (result == DOCA_SUCCESS) {
    *stage = direction == 0 ? "origin-admission-process" : "reply-admission-process";
    result = process_rules(pipeline, rule, 1);
  }
  return result;
}

doca_error_t eswitch_pipeline_ct_promote(
    struct eswitch_pipeline *pipeline,
    const struct router_nat_session *session,
    uint16_t origin_target_vswitch, uint16_t origin_target_port,
    const uint8_t origin_source_mac[6],
    const uint8_t origin_destination_mac[6],
    uint16_t reply_target_vswitch, uint16_t reply_target_port,
    const uint8_t reply_source_mac[6],
    const uint8_t reply_destination_mac[6]) {
  struct eswitch_ct_session *hardware;
  struct doca_flow_ct_match origin = {0}, reply = {0};
  struct doca_flow_ct_actions origin_action = {0}, reply_action = {0};
  uint32_t origin_adjacency, reply_adjacency;
  uint32_t prepare_flags = DOCA_FLOW_CT_ENTRY_FLAGS_ALLOC_ON_MISS;
  uint32_t entry_flags = DOCA_FLOW_CT_ENTRY_FLAGS_NO_WAIT |
      DOCA_FLOW_CT_ENTRY_FLAGS_DIR_ORIGIN |
      DOCA_FLOW_CT_ENTRY_FLAGS_DIR_REPLY;
  bool found = false;
  const char *stage = "session-slot";
  doca_error_t result;

  if (pipeline == NULL || session == NULL || !session->used ||
      origin_source_mac == NULL || origin_destination_mac == NULL ||
      reply_source_mac == NULL || reply_destination_mac == NULL)
    return DOCA_ERROR_INVALID_VALUE;
  if (!pipeline->hardware_ct_enabled || pipeline->ct_pipe == NULL ||
      pipeline->ct_admission_pipe == NULL || pipeline->ct_guard_pipe == NULL ||
      pipeline->ct_ttl_pipe == NULL ||
      origin_target_vswitch == 0 ||
      reply_target_vswitch == 0)
    return DOCA_ERROR_NOT_SUPPORTED;
  if (session->protocol != IPPROTO_TCP && session->protocol != IPPROTO_UDP)
    return DOCA_ERROR_NOT_SUPPORTED;
  hardware = ct_session_slot(pipeline, session);
  if (hardware != NULL && hardware->active)
    return DOCA_SUCCESS;
  if (!offload_retry_ready(&pipeline->ct_retry, session->last_seen_ns)) {
    pipeline->ct_retry_suppressed++;
    /* Deferred, not another failed hardware operation. */
    return DOCA_SUCCESS;
  }
  if (hardware == NULL) {
    result = DOCA_ERROR_FULL;
    goto fail;
  }
  stage = "ct-capacity";
  if (pipeline->ct_active >= pipeline->ct_capacity) {
    result = DOCA_ERROR_FULL;
    goto fail;
  }

  stage = "origin-adjacency";
  result = ct_bind_adjacency(pipeline, origin_target_vswitch,
                             origin_target_port,
                             origin_source_mac, origin_destination_mac,
                             &origin_adjacency);
  if (result == DOCA_SUCCESS) {
    stage = "reply-adjacency";
    result = ct_bind_adjacency(pipeline, reply_target_vswitch,
                               reply_target_port,
                               reply_source_mac, reply_destination_mac,
                               &reply_adjacency);
  }
  if (result != DOCA_SUCCESS)
    goto fail;

  origin.ipv4.src_ip = DOCA_HTOBE32(session->inside_ip);
  origin.ipv4.dst_ip = DOCA_HTOBE32(session->remote_ip);
  origin.ipv4.l4_port.src_port = DOCA_HTOBE16(session->inside_port);
  origin.ipv4.l4_port.dst_port = DOCA_HTOBE16(session->remote_port);
  origin.ipv4.next_proto = session->protocol;
  origin.ipv4.metadata = DOCA_HTOBE32(
      (uint32_t)(hardware - pipeline->ct_sessions) + 1);
  reply.ipv4.src_ip = DOCA_HTOBE32(session->remote_ip);
  reply.ipv4.dst_ip = DOCA_HTOBE32(session->public_ip);
  reply.ipv4.l4_port.src_port = DOCA_HTOBE16(session->remote_port);
  reply.ipv4.l4_port.dst_port = DOCA_HTOBE16(session->public_port);
  reply.ipv4.next_proto = session->protocol;
  reply.ipv4.metadata = origin.ipv4.metadata;

  origin_action.resource_type = DOCA_FLOW_RESOURCE_TYPE_NON_SHARED;
  origin_action.data.action_idx = session->protocol == IPPROTO_TCP ? 0 : 1;
  origin_action.data.meta.flow.u32[0] = DOCA_HTOBE32(origin_adjacency);
  origin_action.data.ip4.src_ip = DOCA_HTOBE32(session->public_ip);
  origin_action.data.ip4.dst_ip = DOCA_HTOBE32(session->remote_ip);
  origin_action.data.l4_port.src_port = DOCA_HTOBE16(session->public_port);
  origin_action.data.l4_port.dst_port = DOCA_HTOBE16(session->remote_port);
  reply_action.resource_type = DOCA_FLOW_RESOURCE_TYPE_NON_SHARED;
  reply_action.data.action_idx = origin_action.data.action_idx;
  reply_action.data.meta.flow.u32[0] = DOCA_HTOBE32(reply_adjacency);
  reply_action.data.ip4.src_ip = DOCA_HTOBE32(session->remote_ip);
  reply_action.data.ip4.dst_ip = DOCA_HTOBE32(session->inside_ip);
  reply_action.data.l4_port.src_port = DOCA_HTOBE16(session->remote_port);
  reply_action.data.l4_port.dst_port = DOCA_HTOBE16(session->inside_port);

  hardware->software = session;
  flow_entry_cookie_prepare(&hardware->cookie, "NAT CT connection",
                            DOCA_FLOW_ENTRY_OP_ADD);
  stage = "ct-prepare";
  result = doca_flow_ct_entry_prepare(
      ct_queue_id(pipeline), pipeline->ct_pipe, prepare_flags, &origin, 0,
      &reply, 0, &hardware->entry, &found);
  if (result != DOCA_SUCCESS)
    goto fail_reset;
  if (!found) {
    stage = "ct-add";
    result = doca_flow_ct_add_entry(
        ct_queue_id(pipeline), pipeline->ct_pipe, entry_flags, &origin,
        &reply, &origin_action, &reply_action, NULL, NULL, 0,
        &hardware->cookie, hardware->entry);
    if (result != DOCA_SUCCESS) {
      doca_error_t rollback = doca_flow_ct_entry_prepare_rollback(
          ct_queue_id(pipeline), pipeline->ct_pipe, hardware->entry);
      if (rollback == DOCA_SUCCESS)
        hardware->entry = NULL;
      goto fail_reset;
    }
    stage = "ct-process";
    result = doca_flow_ct_entries_process(
        pipeline->switch_port, ct_queue_id(pipeline),
        ESWITCH_CT_QUEUE_DEPTH, ESWITCH_CT_QUEUE_DEPTH, NULL);
    if (result != DOCA_SUCCESS || !hardware->cookie.completed ||
        hardware->cookie.last_status != DOCA_FLOW_ENTRY_STATUS_SUCCESS) {
      if (result == DOCA_SUCCESS)
        result = DOCA_ERROR_BAD_STATE;
      goto fail_reset;
    }
  }
  hardware->active = true;
  router_nat_session_set_hardware_active(session, true);
  pipeline->ct_active++;
  hardware->lease_until_ns = session->last_seen_ns + UINT64_C(30000000000);
  stage = "origin-admission";
  result = ct_admit_direction(pipeline, hardware, 0,
      reply_target_vswitch, reply_target_port, reply_source_mac,
      reply_destination_mac, session->inside_ip, session->remote_ip,
      session->inside_port, session->remote_port, &stage);
  if (result == DOCA_SUCCESS) {
    stage = "reply-admission";
    result = ct_admit_direction(pipeline, hardware, 1,
        origin_target_vswitch, origin_target_port, origin_source_mac,
        origin_destination_mac, session->remote_ip, session->public_ip,
        session->remote_port, session->public_port, &stage);
  }
  if (result != DOCA_SUCCESS) {
    /* Keep the software owner pinned until hardware deletion succeeds. */
    (void)eswitch_pipeline_ct_flush(pipeline, session->vr_id);
    goto fail;
  }
  pipeline->ct_promotions++;
  offload_retry_reset(&pipeline->ct_retry);
  return DOCA_SUCCESS;

fail_reset:
  if (hardware->entry != NULL) {
    /* An unsuccessful completion does not prove that no hardware exists.
     * Retain its handle and software owner until a confirmed removal. */
    hardware->active = true;
    hardware->lease_until_ns = session->last_seen_ns;
    router_nat_session_set_hardware_active(session, true);
    pipeline->ct_active++;
    (void)eswitch_pipeline_ct_flush(pipeline, session->vr_id);
  } else {
    hardware->software = NULL;
  }
fail:
  pipeline->ct_failures++;
  if (result == DOCA_ERROR_FULL)
    pipeline->ct_full++;
  if (result == DOCA_ERROR_NO_MEMORY)
    pipeline->ct_no_memory++;
  pipeline->ct_last_failure_stage = stage;
  pipeline->ct_last_failure = result;
  offload_retry_failed(&pipeline->ct_retry, session->last_seen_ns);
  fprintf(stderr, "NAT CT PROMOTION FAILED: stage=%s vr=%u proto=%u "
                  "inside-port=%u public-port=%u active=%zu capacity=%u "
                  "error=%s retry-ms=%u fallback=arm\n",
          stage, session->vr_id, session->protocol, session->inside_port,
          session->public_port, pipeline->ct_active, pipeline->ct_capacity,
          doca_error_get_descr(result), pipeline->ct_retry.delay_ms);
  pipeline->hardware_ct_degraded = true;
  return result;
}

doca_error_t eswitch_pipeline_ct_flush(struct eswitch_pipeline *pipeline,
                                       uint16_t vr_id) {
  doca_error_t first_error = DOCA_SUCCESS;

  if (pipeline == NULL)
    return DOCA_ERROR_INVALID_VALUE;
  if (!pipeline->hardware_ct_enabled || pipeline->ct_pipe == NULL)
    return DOCA_SUCCESS;
  for (size_t i = 0; i < ROUTER_NAT_MAX_SESSIONS; i++) {
    struct eswitch_ct_session *session = &pipeline->ct_sessions[i];
    doca_error_t result;

    if (session->entry == NULL ||
        (vr_id != 0 && session->software != NULL &&
         session->software->vr_id != vr_id))
      continue;
    /* Close admission first; a concurrent packet can only miss to Arm. */
    result = remove_rule(pipeline, &session->admission[0], "revoke CT origin");
    if (result == DOCA_SUCCESS)
      result = remove_rule(pipeline, &session->admission[1], "revoke CT reply");
    if (result != DOCA_SUCCESS) {
      if (first_error == DOCA_SUCCESS)
        first_error = result;
      continue;
    }
    flow_entry_cookie_prepare(&session->cookie, "remove NAT CT connection",
                              DOCA_FLOW_ENTRY_OP_DEL);
    result = doca_flow_ct_rm_entry(
        ct_queue_id(pipeline), pipeline->ct_pipe,
        DOCA_FLOW_CT_ENTRY_FLAGS_NO_WAIT, session->entry);
    if (result == DOCA_SUCCESS)
      result = doca_flow_ct_entries_process(
          pipeline->switch_port, ct_queue_id(pipeline),
          ESWITCH_CT_QUEUE_DEPTH, ESWITCH_CT_QUEUE_DEPTH, NULL);
    if (result == DOCA_SUCCESS &&
        (!session->cookie.completed ||
         session->cookie.last_op != DOCA_FLOW_ENTRY_OP_DEL ||
         session->cookie.last_status != DOCA_FLOW_ENTRY_STATUS_SUCCESS))
      result = DOCA_ERROR_BAD_STATE;
    if (result != DOCA_SUCCESS) {
      if (first_error == DOCA_SUCCESS)
        first_error = result;
      continue;
    }
    router_nat_session_set_hardware_active(session->software, false);
    *session = (struct eswitch_ct_session){0};
    if (pipeline->ct_active != 0)
      pipeline->ct_active--;
  }
  /* Adjacencies are shared and intentionally have no per-VR ownership. Only
   * a global flush can safely reclaim them, after every CT reference is gone. */
  if (vr_id == 0 && first_error == DOCA_SUCCESS) {
    for (size_t i = 0; i < ESWITCH_MAX_CT_ADJACENCIES; i++) {
      struct eswitch_ct_adjacency *adjacency = &pipeline->ct_adjacencies[i];
      doca_error_t result;

      if (adjacency->rule.entry == NULL)
        continue;
      result = remove_rule(pipeline, &adjacency->rule,
                           "remove CT adjacency");
      if (result != DOCA_SUCCESS) {
        first_error = result;
        break;
      }
      *adjacency = (struct eswitch_ct_adjacency){0};
    }
  }
  return first_error;
}

doca_error_t eswitch_pipeline_ct_expire(struct eswitch_pipeline *pipeline,
                                       uint64_t now_ns) {
  /* NO_AGING/NO_COUNTER is the existing CT contract. A bounded lease avoids
   * indefinitely pinning NAT ports. Batch revoke is conservative until
   * hardware activity-based aging is implemented and verified on BF3. */
  for (size_t i = 0; i < ROUTER_NAT_MAX_SESSIONS; i++)
    if (pipeline->ct_sessions[i].active &&
        now_ns >= pipeline->ct_sessions[i].lease_until_ns)
      return eswitch_pipeline_ct_flush(pipeline, 0);
  return DOCA_SUCCESS;
}

doca_error_t eswitch_pipeline_create(struct flow_runtime *runtime,
                                     struct switch_flow_ports *ports,
                                     bool hardware_routing_enabled,
                                     uint32_t hardware_route_capacity,
                                     bool hardware_ct_enabled,
                                     uint32_t hardware_ct_capacity,
                                     uint32_t uplink_arp_pps,
                                     uint32_t uplink_arp_burst,
                                     struct eswitch_pipeline *pipeline) {
  doca_error_t result;

  if (runtime == NULL || ports == NULL || pipeline == NULL ||
      !runtime->initialized || !ports->started || ports->count < 2)
    return DOCA_ERROR_INVALID_VALUE;
  if (hardware_routing_enabled &&
      (hardware_route_capacity < ESWITCH_HW_ROUTE_MIN_CAPACITY ||
       hardware_route_capacity > ROUTER_HW_MAX_ROUTES ||
       (hardware_route_capacity & (hardware_route_capacity - 1)) != 0))
    return DOCA_ERROR_INVALID_VALUE;
  pipeline->runtime = runtime;
  pipeline->ports = ports;
  pipeline->switch_port = ports->switch_port;
  pipeline->hardware_routing_requested = hardware_routing_enabled;
  pipeline->hardware_routing_enabled = hardware_routing_enabled;
  pipeline->hw_route_requested_capacity = hardware_route_capacity;
  pipeline->hw_route_capacity = hardware_route_capacity;
  pipeline->hardware_ct_requested = hardware_ct_enabled;
  pipeline->hardware_ct_enabled = hardware_ct_enabled;
  pipeline->ct_capacity = hardware_ct_capacity;
  pipeline->uplink_arp_pps = uplink_arp_pps;
  pipeline->uplink_arp_burst = uplink_arp_burst;
  pipeline->uplink_arp_filter_requested = uplink_arp_pps != 0;

#define CREATE_STAGE(label, call)                                             \
  do {                                                                        \
    printf("Creating eSwitch stage: %s\n", label);                            \
    result = (call);                                                          \
    if (result != DOCA_SUCCESS) {                                             \
      fprintf(stderr, "eSwitch stage '%s' failed: %s\n", label,               \
              doca_error_get_descr(result));                                  \
      goto fail;                                                              \
    }                                                                         \
  } while (0)

  CREATE_STAGE("RSS slow path", create_rss_pipe(pipeline));
  if (pipeline->hardware_ct_enabled) {
    printf("Creating eSwitch stage: CT egress adjacency\n");
    result = create_ct_egress_pipe(pipeline);
    if (result == DOCA_SUCCESS) {
      printf("Creating eSwitch stage: NAT connection tracking\n");
      result = create_ct_pipe(pipeline);
    }
    if (result == DOCA_SUCCESS) {
      printf("Creating eSwitch stage: CT protocol dispatch\n");
      result = create_ct_dispatch_pipe(pipeline);
    }
    if (result != DOCA_SUCCESS) {
      fprintf(stderr, "Hardware CT pipeline unavailable (%s); continuing "
                      "with Arm NAT slow path\n",
              doca_error_get_descr(result));
      if (pipeline->ct_dispatch_pipe != NULL)
        doca_flow_pipe_destroy(pipeline->ct_dispatch_pipe);
      if (pipeline->ct_pipe != NULL)
        doca_flow_pipe_destroy(pipeline->ct_pipe);
      if (pipeline->ct_egress_pipe != NULL)
        doca_flow_pipe_destroy(pipeline->ct_egress_pipe);
      pipeline->ct_dispatch_pipe = NULL;
      pipeline->ct_pipe = NULL;
      pipeline->ct_egress_pipe = NULL;
      pipeline->hardware_ct_enabled = false;
      pipeline->hardware_ct_degraded = true;
      pipeline->ct_failures++;
    }
  }
  CREATE_STAGE("flood selector", create_flood_selector(pipeline));
  CREATE_STAGE("destination FDB", create_destination_pipe(pipeline));
  CREATE_STAGE("SF return classifier", create_sf_return(pipeline));
  CREATE_STAGE("learning clone", create_learning_clone(pipeline));
  CREATE_STAGE("learning dispatch", create_learning_dispatch(pipeline));
  CREATE_STAGE("source guard", create_source_guard(pipeline));
  if (pipeline->hardware_routing_enabled) {
    printf("Creating eSwitch stage: router IPv4 LPM\n");
    result = create_route_lpm(pipeline);
    if (result == DOCA_SUCCESS) {
      printf("Creating eSwitch stage: router eligibility\n");
      result = create_route_control(pipeline);
    }
    if (result != DOCA_SUCCESS) {
      fprintf(stderr, "Hardware routing unavailable (%s); continuing with "
                      "Arm slow path\n",
              doca_error_get_descr(result));
      if (pipeline->route_control_pipe != NULL)
        doca_flow_pipe_destroy(pipeline->route_control_pipe);
      if (pipeline->route_lpm_pipe != NULL)
        doca_flow_pipe_destroy(pipeline->route_lpm_pipe);
      pipeline->route_control_pipe = NULL;
      pipeline->route_lpm_pipe = NULL;
      pipeline->hardware_routing_enabled = false;
      pipeline->hardware_routing_degraded = true;
      pipeline->hw_route_failures++;
      if (pipeline->hardware_ct_enabled)
        pipeline->hardware_ct_degraded = true;
    }
  }
  /* The ACL selector is useful even when LPM is disabled. Its route-selector
   * hit then goes to RSS, preserving the existing Arm routing semantics. */
  printf("Creating eSwitch stage: router MAC selector\n");
  result = create_route_selector(pipeline);
  if (result == DOCA_SUCCESS) {
    printf("Creating eSwitch stage: guest egress ACL selector\n");
    result = create_egress_acl_selector(pipeline);
  }
  if (result != DOCA_SUCCESS) {
    fprintf(stderr, "Guest egress ACL selector unavailable (%s); "
                    "continuing with Arm slow path\n",
            doca_error_get_descr(result));
    if (pipeline->egress_acl_selector_pipe != NULL)
      doca_flow_pipe_destroy(pipeline->egress_acl_selector_pipe);
    if (pipeline->route_selector_pipe != NULL)
      doca_flow_pipe_destroy(pipeline->route_selector_pipe);
    pipeline->egress_acl_selector_pipe = NULL;
    pipeline->route_selector_pipe = NULL;
    pipeline->egress_acl_failures++;
    if (pipeline->hardware_routing_enabled) {
      if (pipeline->route_control_pipe != NULL)
        doca_flow_pipe_destroy(pipeline->route_control_pipe);
      if (pipeline->route_lpm_pipe != NULL)
        doca_flow_pipe_destroy(pipeline->route_lpm_pipe);
      pipeline->route_control_pipe = NULL;
      pipeline->route_lpm_pipe = NULL;
      pipeline->hardware_routing_enabled = false;
      pipeline->hardware_routing_degraded = true;
      pipeline->hw_route_failures++;
    }
  }
  CREATE_STAGE("local IPv4 delivery", create_local_ip(pipeline));
  if (pipeline->hardware_ct_enabled) {
    const char *stage = "ttl";
    result = create_ct_ttl(pipeline, &stage);
    if (result == DOCA_SUCCESS)
      result = create_ct_guard(pipeline, &stage);
    if (result == DOCA_SUCCESS)
      result = create_ct_admission(pipeline, &stage);
    if (result != DOCA_SUCCESS) {
      if (pipeline->ct_admission_pipe != NULL)
        doca_flow_pipe_destroy(pipeline->ct_admission_pipe);
      pipeline->ct_admission_pipe = NULL;
      if (pipeline->ct_guard_pipe != NULL)
        doca_flow_pipe_destroy(pipeline->ct_guard_pipe);
      pipeline->ct_guard_pipe = NULL;
      if (pipeline->ct_ttl_pipe != NULL)
        doca_flow_pipe_destroy(pipeline->ct_ttl_pipe);
      pipeline->ct_ttl_pipe = NULL;
      pipeline->hardware_ct_degraded = true;
      pipeline->ct_last_failure_stage = stage;
      pipeline->ct_last_failure = result;
      fprintf(stderr, "CT admission unavailable: stage=%s; keeping Arm authorization: %s\n",
              stage, doca_error_get_descr(result));
    } else {
      printf("CT authorization ready: exact-session -> IPv4 guard -> TTL exceptions -> CT\n");
    }
  }
  CREATE_STAGE("ARP dispatch", create_arp_dispatch(pipeline));
  if (pipeline->uplink_arp_filter_requested) {
    printf("Creating eSwitch stage: uplink ARP classifier\n");
    result = create_uplink_arp_classifier(pipeline);
    if (result == DOCA_SUCCESS) {
      pipeline->uplink_arp_filter_enabled = true;
    } else {
      fprintf(stderr, "Uplink ARP classifier unavailable (%s); continuing "
                      "with exact Arm ARP filtering\n",
              doca_error_get_descr(result));
      if (pipeline->uplink_dispatch_pipe != NULL)
        doca_flow_pipe_destroy(pipeline->uplink_dispatch_pipe);
      if (pipeline->uplink_arp_color_pipe != NULL)
        doca_flow_pipe_destroy(pipeline->uplink_arp_color_pipe);
      pipeline->uplink_dispatch_pipe = NULL;
      pipeline->uplink_arp_color_pipe = NULL;
      memset(pipeline->uplink_arp_color_rules, 0,
             sizeof(pipeline->uplink_arp_color_rules));
      pipeline->uplink_arp_filter_degraded = true;
    }
  }
  CREATE_STAGE("ingress classifier", create_ingress_classifier(pipeline));
#undef CREATE_STAGE

  pipeline->created = true;
  return DOCA_SUCCESS;
fail:
  eswitch_pipeline_destroy(pipeline);
  return result;
}

void eswitch_pipeline_destroy(struct eswitch_pipeline *pipeline) {
  if (pipeline == NULL)
    return;
  if (eswitch_pipeline_ct_flush(pipeline, 0) != DOCA_SUCCESS)
    fprintf(stderr, "Failed to flush all NAT CT connections\n");
  if (pipeline->ingress_classifier_pipe != NULL)
    doca_flow_pipe_destroy(pipeline->ingress_classifier_pipe);
  if (pipeline->uplink_dispatch_pipe != NULL)
    doca_flow_pipe_destroy(pipeline->uplink_dispatch_pipe);
  if (pipeline->uplink_arp_color_pipe != NULL)
    doca_flow_pipe_destroy(pipeline->uplink_arp_color_pipe);
  if (pipeline->sf_return_pipe != NULL)
    doca_flow_pipe_destroy(pipeline->sf_return_pipe);
  if (pipeline->arp_dispatch_pipe != NULL)
    doca_flow_pipe_destroy(pipeline->arp_dispatch_pipe);
  if (pipeline->ct_admission_pipe != NULL)
    doca_flow_pipe_destroy(pipeline->ct_admission_pipe);
  if (pipeline->ct_guard_pipe != NULL)
    doca_flow_pipe_destroy(pipeline->ct_guard_pipe);
  if (pipeline->ct_ttl_pipe != NULL)
    doca_flow_pipe_destroy(pipeline->ct_ttl_pipe);
  if (pipeline->local_ip_pipe != NULL)
    doca_flow_pipe_destroy(pipeline->local_ip_pipe);
  for (size_t i = 0; i < ROUTER_MAX_EGRESS_POLICIES; i++) {
    if (pipeline->egress_acls[i].pf_reply_pipe != NULL)
      doca_flow_pipe_destroy(pipeline->egress_acls[i].pf_reply_pipe);
    if (pipeline->egress_acls[i].pipe != NULL)
      doca_flow_pipe_destroy(pipeline->egress_acls[i].pipe);
    free(pipeline->egress_acls[i].pf_replies);
    free(pipeline->egress_acls[i].rules);
  }
  if (pipeline->egress_acl_selector_pipe != NULL)
    doca_flow_pipe_destroy(pipeline->egress_acl_selector_pipe);
  if (pipeline->route_selector_pipe != NULL)
    doca_flow_pipe_destroy(pipeline->route_selector_pipe);
  if (pipeline->route_control_pipe != NULL)
    doca_flow_pipe_destroy(pipeline->route_control_pipe);
  if (pipeline->route_lpm_pipe != NULL)
    doca_flow_pipe_destroy(pipeline->route_lpm_pipe);
  if (pipeline->ct_dispatch_pipe != NULL)
    doca_flow_pipe_destroy(pipeline->ct_dispatch_pipe);
  if (pipeline->ct_pipe != NULL)
    doca_flow_pipe_destroy(pipeline->ct_pipe);
  if (pipeline->ct_egress_pipe != NULL)
    doca_flow_pipe_destroy(pipeline->ct_egress_pipe);
  if (pipeline->source_guard_pipe != NULL)
    doca_flow_pipe_destroy(pipeline->source_guard_pipe);
  if (pipeline->learning_dispatch_pipe != NULL)
    doca_flow_pipe_destroy(pipeline->learning_dispatch_pipe);
  if (pipeline->learning_clone_pipe != NULL)
    doca_flow_pipe_destroy(pipeline->learning_clone_pipe);
  if (pipeline->destination_pipe != NULL)
    doca_flow_pipe_destroy(pipeline->destination_pipe);
  if (pipeline->flood_selector_pipe != NULL)
    doca_flow_pipe_destroy(pipeline->flood_selector_pipe);
  if (pipeline->rss_pipe != NULL)
    doca_flow_pipe_destroy(pipeline->rss_pipe);
  if (pipeline->egress_gates != NULL) {
    for (uint16_t i = 0; i < pipeline->ports->count; i++) {
      if (pipeline->egress_gates[i].pipe != NULL)
        doca_flow_pipe_destroy(pipeline->egress_gates[i].pipe);
      free(pipeline->egress_gates[i].range_forwards);
    }
  }
  for (size_t i = 0; i < ESWITCH_MAX_VLAN_MEMBERSHIPS; i++) {
    if (pipeline->memberships[i].egress.pipe != NULL)
      doca_flow_pipe_destroy(pipeline->memberships[i].egress.pipe);
    free(pipeline->memberships[i].egress.range_forwards);
    free(pipeline->memberships[i].range_ingress);
  }
  free(pipeline->classifier_rules);
  free(pipeline->uplink_arp_meter_rules);
  free(pipeline->uplink_catchall_rules);
  free(pipeline->egress_gates);
  *pipeline = (struct eswitch_pipeline){0};
}

doca_error_t eswitch_pipeline_attach_port(struct eswitch_pipeline *pipeline,
                                          uint16_t port_index,
                                          uint16_t vswitch_id,
                                          enum eswitch_port_mode mode,
                                          uint16_t vlan_id,
                                          uint16_t vlan_last,
                                          uint16_t vlan_extra_id,
                                          uint16_t vlan_extra_last) {
  struct doca_flow_match match = {0};
  struct doca_flow_match mask = {0};
  struct doca_flow_actions actions = {0};
  struct doca_flow_monitor monitor = {
      .counter_type = DOCA_FLOW_RESOURCE_TYPE_NON_SHARED};
  struct doca_flow_fwd fwd = {.type = DOCA_FLOW_FWD_PIPE,
                              .next_pipe = pipeline->arp_dispatch_pipe};
  struct eswitch_pipeline_membership *member = NULL;
  struct eswitch_rule *rules;
  uint16_t rule_count;
  bool transparent_trunk;
  bool vlan_access;
  bool restore = false;
  uint16_t port_id;
  size_t configured_rule_count = 0;
  doca_error_t result;

  if (pipeline == NULL || !pipeline->created ||
      port_index >= pipeline->ports->count || vswitch_id == 0)
    return DOCA_ERROR_INVALID_VALUE;
  if ((mode == ESWITCH_PORT_MODE_TRUNK &&
       !eswitch_vlan_allowlist_valid(vlan_id, vlan_last, vlan_extra_id,
                                     vlan_extra_last)) ||
      (mode == ESWITCH_PORT_MODE_ACCESS &&
       ((vlan_id == 0) != (vlan_last == 0) || vlan_id != vlan_last ||
        (vlan_id != 0 && !eswitch_vlan_valid(vlan_id)) ||
        vlan_extra_id != 0 || vlan_extra_last != 0)))
    return DOCA_ERROR_INVALID_VALUE;
  if (pipeline->ports->items[port_index].ethernet->role ==
      ETHERNET_PORT_ROLE_SF_REPRESENTOR)
    return DOCA_ERROR_NOT_SUPPORTED;
  port_id = pipeline->ports->items[port_index].ethernet->port_id;
  vlan_last = eswitch_vlan_range_last(vlan_id, vlan_last);
  vlan_extra_last = eswitch_vlan_extra_present(vlan_extra_id, vlan_extra_last)
                        ? eswitch_vlan_range_last(vlan_extra_id,
                                                  vlan_extra_last)
                        : 0;
  transparent_trunk = eswitch_vlan_is_transparent_trunk(
      mode, vlan_id, vlan_last, vlan_extra_id, vlan_extra_last);
  vlan_access = mode == ESWITCH_PORT_MODE_ACCESS && vlan_id != 0;
  rule_count = transparent_trunk
                   ? eswitch_vlan_allowlist_size(
                         vlan_id, vlan_last, vlan_extra_id, vlan_extra_last)
                   : 1U;
  for (size_t i = 0; i < ESWITCH_MAX_VLAN_MEMBERSHIPS; i++) {
    struct eswitch_pipeline_membership *candidate =
        &pipeline->memberships[i];
    if (!candidate->active) {
      if (member == NULL)
        member = candidate;
      continue;
    }
    configured_rule_count += candidate->mode == ESWITCH_PORT_MODE_TRUNK
                                 ? eswitch_vlan_allowlist_size(
                                       candidate->vlan_id,
                                       candidate->vlan_last,
                                       candidate->vlan_extra_id,
                                       candidate->vlan_extra_last)
                                 : 1U;
    if (candidate->vswitch_id == vswitch_id &&
        eswitch_vlan_uses_tagged_domain(
            candidate->mode, candidate->vlan_id, candidate->vlan_last,
            candidate->vlan_extra_id, candidate->vlan_extra_last) !=
            eswitch_vlan_uses_tagged_domain(mode, vlan_id, vlan_last,
                                            vlan_extra_id, vlan_extra_last))
      return DOCA_ERROR_IN_USE;
    if (candidate->port_index != port_index)
      continue;
    if (candidate->vswitch_id == vswitch_id) {
      if (candidate->mode != mode || candidate->vlan_id != vlan_id ||
          candidate->vlan_last != vlan_last ||
          candidate->vlan_extra_id != vlan_extra_id ||
          candidate->vlan_extra_last != vlan_extra_last ||
          candidate->ingress.entry != NULL)
        return DOCA_ERROR_ALREADY_EXIST;
      for (uint16_t j = 0; j < candidate->range_ingress_count; j++)
        if (candidate->range_ingress[j].entry != NULL)
          return DOCA_ERROR_ALREADY_EXIST;
      member = candidate;
      restore = true;
      break;
    }
    if (candidate->mode == ESWITCH_PORT_MODE_ACCESS ||
        mode == ESWITCH_PORT_MODE_ACCESS ||
        eswitch_vlan_allowlists_overlap(
            candidate->vlan_id, candidate->vlan_last,
            candidate->vlan_extra_id, candidate->vlan_extra_last, vlan_id,
            vlan_last, vlan_extra_id, vlan_extra_last))
      return DOCA_ERROR_IN_USE;
  }
  if (member == NULL ||
      (!restore && configured_rule_count + rule_count >
                       ESWITCH_MAX_VLAN_MEMBERSHIPS))
    return DOCA_ERROR_NO_MEMORY;
  if (!restore)
    *member = (struct eswitch_pipeline_membership){
        .vswitch_id = vswitch_id,
        .port_index = port_index,
        .port_id = port_id,
        .vlan_id = vlan_id,
        .vlan_last = vlan_last,
        .vlan_extra_id = vlan_extra_id,
        .vlan_extra_last = vlan_extra_last,
        .mode = mode,
    };
  if (transparent_trunk && member->range_ingress == NULL) {
    member->range_ingress = calloc(rule_count, sizeof(*member->range_ingress));
    if (member->range_ingress == NULL)
      return DOCA_ERROR_NO_MEMORY;
    member->range_ingress_count = rule_count;
  }
  rules = transparent_trunk ? member->range_ingress : &member->ingress;
  actions.meta.pkt_meta =
      DOCA_HTOBE32(eswitch_metadata_encode(vswitch_id, port_id));
  actions.pop_vlan = mode == ESWITCH_PORT_MODE_TRUNK && !transparent_trunk;
  if (vlan_access) {
    actions.has_push = true;
    actions.push.type = DOCA_FLOW_PUSH_ACTION_VLAN;
    actions.push.vlan.eth_type = DOCA_HTOBE16(RTE_ETHER_TYPE_VLAN);
    actions.push.vlan.vlan_hdr.tci = DOCA_HTOBE16(vlan_id);
  }
  for (uint16_t i = 0; i < rule_count; i++) {
    struct eswitch_rule *rule = &rules[i];
    uint16_t match_vlan = transparent_trunk
                              ? eswitch_vlan_allowlist_at(
                                    vlan_id, vlan_last, vlan_extra_id,
                                    vlan_extra_last, i)
                              : vlan_id;

    memset(&match, 0, sizeof(match));
    memset(&mask, 0, sizeof(mask));
    match.parser_meta.port_id = port_id;
    mask.parser_meta.port_id = UINT16_MAX;
    match.outer.l2_valid_headers = mode == ESWITCH_PORT_MODE_TRUNK
                                       ? DOCA_FLOW_L2_VALID_HEADER_VLAN_0
                                       : 0;
    match.outer.eth_vlan[0].tci = mode == ESWITCH_PORT_MODE_TRUNK
                                      ? DOCA_HTOBE16(match_vlan)
                                      : 0;
    mask.outer.l2_valid_headers = UINT16_MAX;
    mask.outer.eth_vlan[0].tci = mode == ESWITCH_PORT_MODE_TRUNK
                                     ? DOCA_HTOBE16(0x0fffU)
                                     : 0;
    flow_entry_cookie_prepare(&rule->cookie,
                              transparent_trunk ? "attach trunk list ingress"
                                                : "attach ingress port",
                              DOCA_FLOW_ENTRY_OP_ADD);
    result = doca_flow_pipe_control_add_entry(
        pipeline->runtime->queue_id, pipeline->ingress_classifier_pipe,
        &match, &mask, NULL, &actions, NULL, NULL,
        transparent_trunk ? NULL : &monitor, 0, &fwd,
        &rule->cookie, &rule->entry);
    if (result != DOCA_SUCCESS)
      goto rollback;
  }
  result = process_rules(pipeline, rules, rule_count);
  if (result != DOCA_SUCCESS) {
    goto rollback;
  }
  member->active = true;
  return DOCA_SUCCESS;

rollback:
  {
    doca_error_t original_error = result;
    doca_error_t cleanup = DOCA_SUCCESS;
    for (uint16_t i = 0; i < rule_count; i++) {
      doca_error_t one = remove_rule(pipeline, &rules[i],
                                     "rollback ingress attach");
      if (cleanup == DOCA_SUCCESS && one != DOCA_SUCCESS)
        cleanup = one;
    }
    if (transparent_trunk && cleanup == DOCA_SUCCESS) {
      free(member->range_ingress);
      member->range_ingress = NULL;
      member->range_ingress_count = 0;
    }
    if (!restore && cleanup == DOCA_SUCCESS)
      *member = (struct eswitch_pipeline_membership){0};
    return cleanup == DOCA_SUCCESS ? original_error : cleanup;
  }
}

static doca_error_t attach_uplink_arp_meter(
    struct eswitch_pipeline *pipeline, uint16_t port_index,
    uint32_t metadata) {
  struct doca_flow_match match = {0};
  struct doca_flow_match mask = {0};
  struct doca_flow_monitor monitor = {0};
  struct doca_flow_fwd meter_fwd = {
      .type = DOCA_FLOW_FWD_PIPE,
      .next_pipe = pipeline->uplink_arp_color_pipe};
  struct doca_flow_fwd rss_fwd = {
      .type = DOCA_FLOW_FWD_PIPE,
      .next_pipe = pipeline->hardware_ct_enabled
          ? pipeline->ct_dispatch_pipe : pipeline->rss_pipe};
  struct eswitch_rule *meter = &pipeline->uplink_arp_meter_rules[port_index];
  struct eswitch_rule *catchall = &pipeline->uplink_catchall_rules[port_index];
  doca_error_t result;

  match.meta.pkt_meta = DOCA_HTOBE32(metadata);
  mask.meta.pkt_meta = UINT32_MAX;
  match.outer.eth.type = DOCA_HTOBE16(RTE_ETHER_TYPE_ARP);
  mask.outer.eth.type = UINT16_MAX;
  memset(match.outer.eth.dst_mac, UINT8_MAX, RTE_ETHER_ADDR_LEN);
  memset(mask.outer.eth.dst_mac, UINT8_MAX, RTE_ETHER_ADDR_LEN);
  monitor.meter_type = DOCA_FLOW_RESOURCE_TYPE_NON_SHARED;
  monitor.non_shared_meter.limit_type =
      DOCA_FLOW_METER_LIMIT_TYPE_PACKETS;
  monitor.non_shared_meter.cir = pipeline->uplink_arp_pps;
  monitor.non_shared_meter.cbs = pipeline->uplink_arp_burst;
  flow_entry_cookie_prepare(&meter->cookie, "meter uplink broadcast ARP",
                            DOCA_FLOW_ENTRY_OP_ADD);
  result = doca_flow_pipe_control_add_entry(
      pipeline->runtime->queue_id, pipeline->uplink_dispatch_pipe,
      &match, &mask, NULL, NULL, NULL, NULL, &monitor, 0, &meter_fwd,
      &meter->cookie, &meter->entry);
  if (result != DOCA_SUCCESS)
    return result;
  result = process_rules(pipeline, meter, 1);
  if (result != DOCA_SUCCESS) {
    doca_error_t original_error = result;
    doca_error_t cleanup = remove_rule(pipeline, meter,
                                       "rollback failed uplink ARP meter");
    return cleanup == DOCA_SUCCESS ? original_error : cleanup;
  }

  memset(&match, 0, sizeof(match));
  memset(&mask, 0, sizeof(mask));
  match.meta.pkt_meta = DOCA_HTOBE32(metadata);
  mask.meta.pkt_meta = UINT32_MAX;
  flow_entry_cookie_prepare(&catchall->cookie, "uplink non-noise catch-all",
                            DOCA_FLOW_ENTRY_OP_ADD);
  result = doca_flow_pipe_control_add_entry(
      pipeline->runtime->queue_id, pipeline->uplink_dispatch_pipe,
      &match, &mask, NULL, NULL, NULL, NULL, NULL, 7, &rss_fwd,
      &catchall->cookie, &catchall->entry);
  if (result == DOCA_SUCCESS)
    result = process_rules(pipeline, catchall, 1);
  if (result != DOCA_SUCCESS) {
    doca_error_t cleanup = remove_rule(pipeline, catchall,
                                       "rollback uplink catch-all");
    if (cleanup == DOCA_SUCCESS)
      cleanup = remove_rule(pipeline, meter,
                            "rollback uplink ARP meter");
    return cleanup == DOCA_SUCCESS ? result : cleanup;
  }
  return DOCA_SUCCESS;
}

static doca_error_t detach_uplink_arp_meter(
    struct eswitch_pipeline *pipeline, uint16_t port_index) {
  doca_error_t result;

  result = remove_rule(pipeline, &pipeline->uplink_catchall_rules[port_index],
                       "detach uplink catch-all");
  if (result != DOCA_SUCCESS)
    return result;
  return remove_rule(pipeline, &pipeline->uplink_arp_meter_rules[port_index],
                     "detach uplink ARP meter");
}

doca_error_t eswitch_pipeline_attach_router_port(
    struct eswitch_pipeline *pipeline, uint16_t port_index, uint16_t vr_id) {
  struct doca_flow_match match = {0};
  struct doca_flow_match mask = {0};
  struct doca_flow_actions actions = {0};
  struct doca_flow_monitor monitor = {
      .counter_type = DOCA_FLOW_RESOURCE_TYPE_NON_SHARED};
  struct doca_flow_fwd fwd = {.type = DOCA_FLOW_FWD_PIPE};
  struct eswitch_rule *rule;
  uint16_t port_id;
  bool metered = false;
  doca_error_t result;

  if (pipeline == NULL || !pipeline->created ||
      port_index >= pipeline->ports->count || vr_id == 0)
    return DOCA_ERROR_INVALID_VALUE;
  if (pipeline->ports->items[port_index].ethernet->role !=
      ETHERNET_PORT_ROLE_REPRESENTOR)
    return DOCA_ERROR_NOT_SUPPORTED;
  rule = &pipeline->classifier_rules[port_index];
  if (rule->entry != NULL)
    return DOCA_ERROR_BAD_STATE;
  port_id = pipeline->ports->items[port_index].ethernet->port_id;
  match.parser_meta.port_id = port_id;
  mask.parser_meta.port_id = UINT16_MAX;
  actions.meta.pkt_meta = DOCA_HTOBE32(eswitch_metadata_encode(vr_id, port_id));
  actions.meta.u32[1] = DOCA_HTOBE32(vr_id);
  if (pipeline->uplink_arp_filter_enabled) {
    result = attach_uplink_arp_meter(
        pipeline, port_index, eswitch_metadata_encode(vr_id, port_id));
    if (result == DOCA_SUCCESS) {
      metered = true;
    } else {
      pipeline->uplink_arp_filter_degraded = true;
      fprintf(stderr, "UPLINK ARP CLASSIFIER DEGRADED: vr=%u port=%u "
                      "fallback=arm error=%s\n",
              vr_id, port_id, doca_error_get_descr(result));
    }
  }
  fwd.next_pipe = metered ? pipeline->uplink_dispatch_pipe
                          : (pipeline->hardware_ct_enabled
                                 ? pipeline->ct_dispatch_pipe
                                 : pipeline->rss_pipe);
  flow_entry_cookie_prepare(&rule->cookie, "attach router uplink",
                            DOCA_FLOW_ENTRY_OP_ADD);
  result = doca_flow_pipe_control_add_entry(
      pipeline->runtime->queue_id, pipeline->ingress_classifier_pipe, &match,
      &mask, NULL, &actions, NULL, NULL, &monitor, 0, &fwd, &rule->cookie,
      &rule->entry);
  if (result != DOCA_SUCCESS) {
    if (metered)
      (void)detach_uplink_arp_meter(pipeline, port_index);
    return result;
  }
  result = process_rules(pipeline, rule, 1);
  if (result != DOCA_SUCCESS) {
    doca_error_t original_error = result;
    doca_error_t cleanup = remove_rule(pipeline, rule,
                                       "rollback router uplink attach");
    if (cleanup == DOCA_SUCCESS && metered)
      cleanup = detach_uplink_arp_meter(pipeline, port_index);
    return cleanup == DOCA_SUCCESS ? original_error : cleanup;
  }
  printf("ROUTER UPLINK ATTACH: vr=%u port=%u path=%s\n", vr_id, port_id,
         metered ? "ARP-METER->RSS" : "RSS");
  return DOCA_SUCCESS;
}

doca_error_t eswitch_pipeline_detach_port(struct eswitch_pipeline *pipeline,
                                          uint16_t port_index) {
  if (pipeline == NULL || !pipeline->created ||
      port_index >= pipeline->ports->count)
    return DOCA_ERROR_INVALID_VALUE;
  {
    doca_error_t result = remove_rule(
        pipeline, &pipeline->classifier_rules[port_index],
        "detach ingress port");
    if (result != DOCA_SUCCESS)
      return result;
  }
  return detach_uplink_arp_meter(pipeline, port_index);
}

doca_error_t eswitch_pipeline_detach_vswitch_port(
    struct eswitch_pipeline *pipeline, uint16_t port_index,
    uint16_t vswitch_id) {
  struct eswitch_pipeline_membership *member;
  uint16_t port_id;
  doca_error_t result;

  if (pipeline == NULL || !pipeline->created ||
      port_index >= pipeline->ports->count || vswitch_id == 0)
    return DOCA_ERROR_INVALID_VALUE;
  port_id = pipeline->ports->items[port_index].ethernet->port_id;
  member = find_membership(pipeline, vswitch_id, port_id);
  if (member == NULL)
    return DOCA_ERROR_NOT_FOUND;
  if (member->range_ingress != NULL) {
    for (uint16_t i = 0; i < member->range_ingress_count; i++) {
      result = remove_rule(pipeline, &member->range_ingress[i],
                           "detach vSwitch trunk range ingress");
      if (result != DOCA_SUCCESS)
        return result;
    }
    return DOCA_SUCCESS;
  }
  result = remove_rule(pipeline, &member->ingress,
                       "detach vSwitch ingress membership");
  return result;
}

doca_error_t eswitch_pipeline_release_vswitch_port(
    struct eswitch_pipeline *pipeline, uint16_t port_index,
    uint16_t vswitch_id) {
  struct eswitch_pipeline_membership *member;
  uint16_t port_id;

  if (pipeline == NULL || !pipeline->created ||
      port_index >= pipeline->ports->count || vswitch_id == 0)
    return DOCA_ERROR_INVALID_VALUE;
  port_id = pipeline->ports->items[port_index].ethernet->port_id;
  member = find_membership(pipeline, vswitch_id, port_id);
  if (member == NULL || member->ingress.entry != NULL)
    return DOCA_ERROR_BAD_STATE;
  for (uint16_t i = 0; i < member->range_ingress_count; i++)
    if (member->range_ingress[i].entry != NULL)
      return DOCA_ERROR_BAD_STATE;
  if (member->egress.pipe != NULL) {
    doca_flow_pipe_destroy(member->egress.pipe);
    free(member->egress.range_forwards);
    member->egress = (struct eswitch_egress_gate){0};
  }
  free(member->range_ingress);
  *member = (struct eswitch_pipeline_membership){0};
  return DOCA_SUCCESS;
}

static bool hw_route_same_key(const struct router_hw_route *left,
                              const struct router_hw_route *right) {
  return left->vr_id == right->vr_id && left->prefix == right->prefix &&
         left->length == right->length;
}

static bool hw_route_same_action(const struct router_hw_route *left,
                                 const struct router_hw_route *right) {
  return left->egress_interface_id == right->egress_interface_id &&
         left->egress_vswitch_id == right->egress_vswitch_id &&
         left->target_port_id == right->target_port_id &&
         memcmp(left->source_mac, right->source_mac, 6) == 0 &&
         memcmp(left->destination_mac, right->destination_mac, 6) == 0;
}

static void fill_hw_route_action(const struct router_hw_route *spec,
                                 struct doca_flow_actions *actions) {
  *actions = (struct doca_flow_actions){0};
  memcpy(actions->outer.eth.src_mac, spec->source_mac, 6);
  memcpy(actions->outer.eth.dst_mac, spec->destination_mac, 6);
  actions->outer.l3_type = DOCA_FLOW_L3_TYPE_IP4;
  actions->outer.ip4.ttl = UINT8_MAX;
}

static doca_error_t add_hw_route(struct eswitch_pipeline *pipeline,
                                 struct eswitch_hw_route_entry *record,
                                 const struct router_hw_route *spec) {
  struct doca_flow_match match = {0};
  struct doca_flow_match match_mask = {0};
  struct doca_flow_actions actions;
  struct doca_flow_fwd fwd = {.type = DOCA_FLOW_FWD_PIPE};
  struct doca_flow_pipe *gate = NULL;
  doca_error_t result;

  result = get_egress_gate(pipeline, spec->egress_vswitch_id,
                           spec->target_port_id, &gate);
  if (result != DOCA_SUCCESS)
    return result;
  match.meta.u32[1] = DOCA_HTOBE32(spec->vr_id);
  /* The IP mask is variable per prefix, while the VR dimension must always
   * remain an exact part of every LPM key. */
  match_mask.meta.u32[1] = UINT32_MAX;
  match.outer.ip4.dst_ip = DOCA_HTOBE32(spec->prefix);
  match_mask.outer.ip4.dst_ip = DOCA_HTOBE32(
      ipv4_prefix_mask(spec->length));
  fill_hw_route_action(spec, &actions);
  fwd.next_pipe = gate;
  flow_entry_cookie_prepare(&record->rule.cookie, "hardware IPv4 route",
                            DOCA_FLOW_ENTRY_OP_ADD);
  result = doca_flow_pipe_lpm_add_entry(
      pipeline->runtime->queue_id, pipeline->route_lpm_pipe, &match,
      &match_mask, 0, &actions, NULL, &fwd,
      DOCA_FLOW_ENTRY_FLAGS_NO_WAIT, &record->rule.cookie,
      &record->rule.entry);
  if (result != DOCA_SUCCESS)
    return result;
  result = process_rules(pipeline, &record->rule, 1);
  if (result != DOCA_SUCCESS) {
    (void)remove_rule(pipeline, &record->rule,
                      "rollback hardware IPv4 route");
    return result;
  }
  record->spec = *spec;
  record->active = true;
  pipeline->hw_route_count++;
  pipeline->hw_route_promotions++;
  return DOCA_SUCCESS;
}

static doca_error_t update_hw_route(struct eswitch_pipeline *pipeline,
                                    struct eswitch_hw_route_entry *record,
                                    const struct router_hw_route *spec) {
  struct doca_flow_actions actions;
  struct doca_flow_fwd fwd = {.type = DOCA_FLOW_FWD_PIPE};
  struct doca_flow_pipe *gate = NULL;
  struct flow_entry_cookie *cookie = &record->rule.cookie;
  doca_error_t result;

  result = get_egress_gate(pipeline, spec->egress_vswitch_id,
                           spec->target_port_id, &gate);
  if (result != DOCA_SUCCESS)
    return result;
  fill_hw_route_action(spec, &actions);
  fwd.next_pipe = gate;
  flow_entry_cookie_prepare(cookie, "update hardware IPv4 adjacency",
                            DOCA_FLOW_ENTRY_OP_UPD);
  result = doca_flow_pipe_lpm_update_entry(
      pipeline->runtime->queue_id, pipeline->route_lpm_pipe, 0, &actions,
      NULL, &fwd, DOCA_FLOW_ENTRY_FLAGS_NO_WAIT, record->rule.entry);
  if (result != DOCA_SUCCESS)
    return result;
  result = flow_runtime_process(pipeline->runtime, pipeline->switch_port,
                                &cookie, 1);
  if (result == DOCA_SUCCESS) {
    record->spec = *spec;
    pipeline->hw_route_updates++;
  }
  return result;
}

doca_error_t eswitch_pipeline_hw_routes_sync(
    struct eswitch_pipeline *pipeline, const struct router_hw_route *routes,
    size_t route_count) {
  bool keep[ROUTER_HW_MAX_ROUTES] = {0};
  doca_error_t first_error = DOCA_SUCCESS;

  if (pipeline == NULL || !pipeline->created ||
      (route_count != 0 && routes == NULL) ||
      route_count > pipeline->hw_route_capacity)
    return DOCA_ERROR_INVALID_VALUE;
  if (!pipeline->hardware_routing_enabled)
    return DOCA_SUCCESS;

  for (size_t desired = 0; desired < route_count; desired++) {
    struct eswitch_hw_route_entry *free_record = NULL;
    struct eswitch_hw_route_entry *record = NULL;
    size_t slot = 0;

    for (size_t i = 0; i < ROUTER_HW_MAX_ROUTES; i++) {
      if (!pipeline->hw_routes[i].active) {
        if (free_record == NULL) {
          free_record = &pipeline->hw_routes[i];
          slot = i;
        }
      } else if (hw_route_same_key(&pipeline->hw_routes[i].spec,
                                   &routes[desired])) {
        record = &pipeline->hw_routes[i];
        slot = i;
        break;
      }
    }
    if (record != NULL) {
      keep[slot] = true;
      if (!hw_route_same_action(&record->spec, &routes[desired])) {
        doca_error_t result = update_hw_route(pipeline, record,
                                               &routes[desired]);
        if (result != DOCA_SUCCESS) {
          /* Never retain a known-stale MAC/port after an adjacency update
           * failure. Removing the old entry turns the packet into an LPM miss
           * and therefore restores the software slow path. */
          doca_error_t cleanup = remove_rule(
              pipeline, &record->rule,
              "fail closed stale hardware IPv4 adjacency");
          if (cleanup == DOCA_SUCCESS) {
            *record = (struct eswitch_hw_route_entry){0};
            pipeline->hw_route_count--;
            pipeline->hw_route_removals++;
          }
          if (first_error == DOCA_SUCCESS)
            first_error = cleanup == DOCA_SUCCESS ? result : cleanup;
        }
      }
      continue;
    }
    if (free_record == NULL) {
      if (first_error == DOCA_SUCCESS)
        first_error = DOCA_ERROR_NO_MEMORY;
      continue;
    }
    {
      doca_error_t result = add_hw_route(pipeline, free_record,
                                         &routes[desired]);
      if (result == DOCA_SUCCESS)
        keep[slot] = true;
      else if (first_error == DOCA_SUCCESS)
        first_error = result;
    }
  }

  for (size_t i = 0; i < ROUTER_HW_MAX_ROUTES; i++) {
    struct eswitch_hw_route_entry *record = &pipeline->hw_routes[i];
    if (!record->active || keep[i])
      continue;
    {
      doca_error_t result = remove_rule(pipeline, &record->rule,
                                        "remove stale hardware IPv4 route");
      if (result == DOCA_SUCCESS) {
        *record = (struct eswitch_hw_route_entry){0};
        pipeline->hw_route_count--;
        pipeline->hw_route_removals++;
      } else if (first_error == DOCA_SUCCESS) {
        first_error = result;
      }
    }
  }
  if (first_error != DOCA_SUCCESS) {
    pipeline->hw_route_failures++;
    pipeline->hardware_routing_degraded = true;
    if (pipeline->hardware_ct_enabled)
      pipeline->hardware_ct_degraded = true;
    /* Keep the router-MAC selector installed: with no eligibility entry the
     * control pipe's fallback sends router traffic to RSS. This disables
     * acceleration without diverting ordinary L2 IPv4 away from its FDB. */
    for (size_t i = 0; i < ESWITCH_MAX_SF_RETURN_CONTEXTS; i++) {
      struct eswitch_sf_return_context *context =
          &pipeline->sf_return_contexts[i];
      if (!context->active || context->directed ||
          context->route_eligible_rule.entry == NULL)
        continue;
      {
        doca_error_t cleanup = remove_rule(
            pipeline, &context->route_eligible_rule,
            "disable hardware routing after sync failure");
        if (cleanup != DOCA_SUCCESS)
          first_error = cleanup;
      }
    }
  } else {
    /* Recover automatically after a transient programming failure. */
    for (size_t i = 0; i < ESWITCH_MAX_SF_RETURN_CONTEXTS; i++) {
      struct eswitch_sf_return_context *context =
          &pipeline->sf_return_contexts[i];
      if (!context->active || context->directed ||
          context->route_eligible_rule.entry != NULL)
        continue;
      {
        doca_error_t result = add_route_eligible_rule(pipeline, context);
        if (result != DOCA_SUCCESS) {
          pipeline->hw_route_failures++;
          pipeline->hardware_routing_degraded = true;
          return result;
        }
      }
    }
    pipeline->hardware_routing_degraded = false;
    if (pipeline->hardware_ct_enabled && pipeline->ct_failures == 0)
      pipeline->hardware_ct_degraded = false;
  }
  return first_error;
}

doca_error_t eswitch_pipeline_hw_route_stats(
    const struct eswitch_pipeline *pipeline, uint64_t *lpm_misses) {
  struct doca_flow_resource_query query = {0};
  doca_error_t result;

  if (pipeline == NULL || lpm_misses == NULL)
    return DOCA_ERROR_INVALID_VALUE;
  *lpm_misses = 0;
  if (!pipeline->hardware_routing_enabled)
    return DOCA_SUCCESS;
  result = doca_flow_resource_query_pipe_miss(pipeline->route_lpm_pipe,
                                               &query);
  if (result == DOCA_SUCCESS)
    *lpm_misses = query.counter.total_pkts;
  return result;
}

static doca_error_t create_egress_gate_for(
    struct eswitch_pipeline *pipeline, struct eswitch_egress_gate *gate,
    uint16_t port_index, uint16_t vswitch_id,
    enum eswitch_port_mode mode, uint16_t vlan_id, uint16_t vlan_last,
    uint16_t vlan_extra_id, uint16_t vlan_extra_last) {
  struct doca_flow_pipe_cfg *cfg = NULL;
  struct doca_flow_match match = {0};
  struct doca_flow_match mask = {0};
  struct doca_flow_fwd fwd = {0};
  struct doca_flow_actions actions = {0};
  struct doca_flow_monitor monitor = {
      .counter_type = DOCA_FLOW_RESOURCE_TYPE_NON_SHARED};
  char name[64];
  uint16_t port_id = pipeline->ports->items[port_index].ethernet->port_id;
  bool transparent_trunk = eswitch_vlan_is_transparent_trunk(
      mode, vlan_id, vlan_last, vlan_extra_id, vlan_extra_last);
  bool vlan_access = mode == ESWITCH_PORT_MODE_ACCESS && vlan_id != 0;
  uint16_t range_count = transparent_trunk
                             ? eswitch_vlan_allowlist_size(
                                   vlan_id, vlan_last, vlan_extra_id,
                                   vlan_extra_last)
                             : 0;
  doca_error_t result;

  if (gate->pipe != NULL)
    return DOCA_SUCCESS;

  snprintf(name, sizeof(name), "ESW_EGRESS_%u_%u", vswitch_id, port_id);
  result = doca_flow_pipe_cfg_create(&cfg, pipeline->switch_port);
  if (result != DOCA_SUCCESS)
    return result;
  result = set_pipe_identity(cfg, name, DOCA_FLOW_PIPE_CONTROL, false,
                             transparent_trunk ? (uint32_t)range_count + 1U
                                               : 2U);
  if (result == DOCA_SUCCESS)
    result = doca_flow_pipe_create(cfg, NULL, NULL, &gate->pipe);
  doca_flow_pipe_cfg_destroy(cfg);
  if (result != DOCA_SUCCESS)
    return result;

  /* pkt_meta keeps the original ingress in its low 16 bits.  The exact
   * high-priority rule provides split-horizon filtering for this egress. */
  match.meta.pkt_meta = DOCA_HTOBE32(port_id);
  mask.meta.pkt_meta = DOCA_HTOBE32(ESWITCH_METADATA_PORT_MASK);
  fwd.type = DOCA_FLOW_FWD_DROP;
  flow_entry_cookie_prepare(&gate->drop_self.cookie, "egress self-drop",
                            DOCA_FLOW_ENTRY_OP_ADD);
  result = doca_flow_pipe_control_add_entry(
      pipeline->runtime->queue_id, gate->pipe, &match, &mask, NULL, NULL,
      NULL, NULL, &monitor, 0, &fwd, &gate->drop_self.cookie,
      &gate->drop_self.entry);
  if (result != DOCA_SUCCESS)
    goto fail;
  result = process_rules(pipeline, &gate->drop_self, 1);
  if (result != DOCA_SUCCESS)
    goto fail;

  memset(&match, 0, sizeof(match));
  memset(&mask, 0, sizeof(mask));
  memset(&fwd, 0, sizeof(fwd));
  fwd.type = DOCA_FLOW_FWD_PORT;
  fwd.port_id = port_id;
  if (transparent_trunk) {
    gate->range_forwards = calloc(range_count,
                                  sizeof(*gate->range_forwards));
    if (gate->range_forwards == NULL) {
      result = DOCA_ERROR_NO_MEMORY;
      goto fail;
    }
    gate->range_forward_count = range_count;
    match.outer.l2_valid_headers = DOCA_FLOW_L2_VALID_HEADER_VLAN_0;
    mask.outer.l2_valid_headers = UINT16_MAX;
    mask.outer.eth_vlan[0].tci = DOCA_HTOBE16(0x0fffU);
    for (uint16_t i = 0; i < range_count; i++) {
      struct eswitch_rule *rule = &gate->range_forwards[i];
      match.outer.eth_vlan[0].tci = DOCA_HTOBE16(eswitch_vlan_allowlist_at(
          vlan_id, vlan_last, vlan_extra_id, vlan_extra_last, i));
      flow_entry_cookie_prepare(&rule->cookie, "egress trunk list forward",
                                DOCA_FLOW_ENTRY_OP_ADD);
      result = doca_flow_pipe_control_add_entry(
          pipeline->runtime->queue_id, gate->pipe, &match, &mask, NULL, NULL,
          NULL, NULL, &monitor, 1, &fwd, &rule->cookie, &rule->entry);
      if (result != DOCA_SUCCESS)
        goto fail;
    }
    result = process_rules(pipeline, gate->range_forwards, range_count);
    if (result != DOCA_SUCCESS)
      goto fail;
    printf("EGRESS GATE CREATE: vs=%u port=%u mode=trunk "
           "allowed-vlans=%u path=transparent\n", vswitch_id, port_id,
           range_count);
    return DOCA_SUCCESS;
  }
  if (mode == ESWITCH_PORT_MODE_TRUNK) {
    match.outer.l2_valid_headers = 0;
    mask.outer.l2_valid_headers = UINT16_MAX;
    actions.has_push = true;
    actions.push.type = DOCA_FLOW_PUSH_ACTION_VLAN;
    actions.push.vlan.eth_type = DOCA_HTOBE16(RTE_ETHER_TYPE_VLAN);
    actions.push.vlan.vlan_hdr.tci = DOCA_HTOBE16(vlan_id);
  } else if (vlan_access) {
    match.outer.l2_valid_headers = DOCA_FLOW_L2_VALID_HEADER_VLAN_0;
    mask.outer.l2_valid_headers = UINT16_MAX;
    match.outer.eth_vlan[0].tci = DOCA_HTOBE16(vlan_id);
    mask.outer.eth_vlan[0].tci = DOCA_HTOBE16(0x0fffU);
    actions.pop_vlan = true;
  }
  flow_entry_cookie_prepare(&gate->forward.cookie, "egress forward",
                            DOCA_FLOW_ENTRY_OP_ADD);
  result = doca_flow_pipe_control_add_entry(
      pipeline->runtime->queue_id, gate->pipe, &match,
      mode == ESWITCH_PORT_MODE_TRUNK || vlan_access ? &mask : NULL, NULL,
      mode == ESWITCH_PORT_MODE_TRUNK || vlan_access ? &actions : NULL,
      NULL, NULL, &monitor, 1, &fwd, &gate->forward.cookie,
      &gate->forward.entry);
  if (result != DOCA_SUCCESS)
    goto fail;
  result = process_rules(pipeline, &gate->forward, 1);
  if (result != DOCA_SUCCESS)
    goto fail;

  printf("EGRESS GATE CREATE: vs=%u port=%u mode=%s vlan=%u\n", vswitch_id,
         port_id, mode == ESWITCH_PORT_MODE_TRUNK ? "trunk" : "access",
         vlan_id);
  return DOCA_SUCCESS;

fail:
  doca_flow_pipe_destroy(gate->pipe);
  free(gate->range_forwards);
  *gate = (struct eswitch_egress_gate){0};
  return result;
}

static doca_error_t create_legacy_egress_gate(
    struct eswitch_pipeline *pipeline, uint16_t port_index) {
  return create_egress_gate_for(pipeline, &pipeline->egress_gates[port_index],
                                port_index, 0, ESWITCH_PORT_MODE_ACCESS, 0,
                                0, 0, 0);
}

static doca_error_t get_egress_gate(struct eswitch_pipeline *pipeline,
                                    uint16_t vswitch_id,
                                    uint16_t port_id,
                                    struct doca_flow_pipe **gate_pipe) {
  int port_index;
  doca_error_t result;
  struct eswitch_pipeline_membership *member;

  if (pipeline == NULL || gate_pipe == NULL)
    return DOCA_ERROR_INVALID_VALUE;
  port_index = find_port_index(pipeline, port_id);
  if (port_index < 0)
    return DOCA_ERROR_NOT_FOUND;
  member = vswitch_id == 0 ? NULL :
      find_membership(pipeline, vswitch_id, port_id);
  if (member != NULL) {
    result = create_egress_gate_for(pipeline, &member->egress,
                                    (uint16_t)port_index, vswitch_id,
                                    member->mode, member->vlan_id,
                                    member->vlan_last, member->vlan_extra_id,
                                    member->vlan_extra_last);
    if (result == DOCA_SUCCESS)
      *gate_pipe = member->egress.pipe;
    return result;
  }
  /* Legacy direct-port consumers (router port-link and CT adjacency) have no
   * VLAN context. Never guess a trunk VLAN for them. */
  result = create_legacy_egress_gate(pipeline, (uint16_t)port_index);
  if (result == DOCA_SUCCESS)
    *gate_pipe = pipeline->egress_gates[port_index].pipe;
  return result;
}

static doca_error_t create_flood_group(struct eswitch_pipeline *pipeline,
                                       uint16_t vswitch_id,
                                       struct eswitch_flood_group *group) {
  struct doca_flow_pipe_cfg *cfg = NULL;
  struct doca_flow_fwd pipe_fwd = {.type = DOCA_FLOW_FWD_CHANGEABLE};
  struct doca_flow_match selector_match = {0};
  struct doca_flow_fwd selector_fwd = {0};
  char name[64];
  uint16_t member_capacity = max_flood_members(pipeline);
  uint32_t pipe_capacity = next_power_of_two(member_capacity);
  doca_error_t result;

  if (pipe_capacity < 2)
    pipe_capacity = 2;
  group->members = calloc(member_capacity, sizeof(*group->members));
  if (group->members == NULL)
    return DOCA_ERROR_NO_MEMORY;
  group->vswitch_id = vswitch_id;
  group->member_capacity = member_capacity;

  snprintf(name, sizeof(name), "ESW_VS%u_FLOOD", vswitch_id);
  result = doca_flow_pipe_cfg_create(&cfg, pipeline->switch_port);
  if (result != DOCA_SUCCESS)
    goto fail;
  result = set_pipe_identity(cfg, name, DOCA_FLOW_PIPE_HASH, false,
                             pipe_capacity);
  if (result == DOCA_SUCCESS)
    result = doca_flow_pipe_cfg_set_hash_map_algorithm(
        cfg, DOCA_FLOW_PIPE_HASH_MAP_ALGORITHM_FLOODING);
  if (result == DOCA_SUCCESS)
    result = doca_flow_pipe_create(cfg, &pipe_fwd, NULL, &group->pipe);
  doca_flow_pipe_cfg_destroy(cfg);
  cfg = NULL;
  if (result != DOCA_SUCCESS)
    goto fail;

  selector_match.meta.pkt_meta = DOCA_HTOBE32((uint32_t)vswitch_id << 16);
  selector_fwd.type = DOCA_FLOW_FWD_HASH_PIPE;
  selector_fwd.hash_pipe.pipe = group->pipe;
  selector_fwd.hash_pipe.algorithm =
      DOCA_FLOW_PIPE_HASH_MAP_ALGORITHM_FLOODING;
  flow_entry_cookie_prepare(&group->selector.cookie, "flood selector",
                            DOCA_FLOW_ENTRY_OP_ADD);
  result = doca_flow_pipe_basic_add_entry(
      pipeline->runtime->queue_id, pipeline->flood_selector_pipe,
      &selector_match, 0, NULL, NULL, &selector_fwd,
      DOCA_FLOW_ENTRY_FLAGS_NO_WAIT, &group->selector.cookie,
      &group->selector.entry);
  if (result != DOCA_SUCCESS)
    goto fail;
  result = process_rules(pipeline, &group->selector, 1);
  if (result != DOCA_SUCCESS)
    goto fail;

  printf("FLOOD GROUP CREATE: vs=%u capacity=%u\n", vswitch_id,
         member_capacity);
  return DOCA_SUCCESS;

fail:
  if (group->selector.entry != NULL) {
    doca_error_t cleanup = remove_rule(pipeline, &group->selector,
                                       "rollback flood selector");
    if (cleanup != DOCA_SUCCESS)
      return cleanup;
  }
  if (cfg != NULL)
    doca_flow_pipe_cfg_destroy(cfg);
  if (group->pipe != NULL)
    doca_flow_pipe_destroy(group->pipe);
  free(group->members);
  *group = (struct eswitch_flood_group){0};
  return result;
}

doca_error_t eswitch_pipeline_flood_add_port(
    struct eswitch_pipeline *pipeline, uint16_t vswitch_id, uint16_t port_id,
    struct eswitch_flood_group *group) {
  struct doca_flow_pipe *gate_pipe = NULL;
  struct eswitch_flood_member *member = NULL;
  struct doca_flow_fwd fwd = {0};
  uint16_t slot;
  bool created = false;
  doca_error_t result;

  if (pipeline == NULL || group == NULL || !pipeline->created ||
      vswitch_id == 0)
    return DOCA_ERROR_INVALID_VALUE;
  if (group->pipe != NULL && group->vswitch_id != vswitch_id)
    return DOCA_ERROR_BAD_STATE;
  for (slot = 0; slot < group->member_capacity; slot++) {
    if (group->members[slot].active &&
        group->members[slot].port_id == port_id)
      return DOCA_ERROR_ALREADY_EXIST;
  }

  result = get_egress_gate(pipeline, vswitch_id, port_id, &gate_pipe);
  if (result != DOCA_SUCCESS)
    return result;
  if (group->pipe == NULL) {
    result = create_flood_group(pipeline, vswitch_id, group);
    if (result != DOCA_SUCCESS)
      return result;
    created = true;
  }
  if (group->member_count >= group->member_capacity) {
    result = DOCA_ERROR_NO_MEMORY;
    goto fail;
  }
  for (slot = 0; slot < group->member_capacity; slot++) {
    if (!group->members[slot].active &&
        group->members[slot].rule.entry == NULL) {
      member = &group->members[slot];
      break;
    }
  }
  if (member == NULL) {
    result = DOCA_ERROR_BAD_STATE;
    goto fail;
  }

  fwd.type = DOCA_FLOW_FWD_PIPE;
  fwd.next_pipe = gate_pipe;
  flow_entry_cookie_prepare(&member->rule.cookie, "add flood member",
                            DOCA_FLOW_ENTRY_OP_ADD);
  result = doca_flow_pipe_hash_add_entry(
      pipeline->runtime->queue_id, group->pipe, slot, 0, NULL, NULL, &fwd,
      DOCA_FLOW_ENTRY_FLAGS_NO_WAIT, &member->rule.cookie,
      &member->rule.entry);
  if (result != DOCA_SUCCESS)
    goto fail;
  result = process_rules(pipeline, &member->rule, 1);
  if (result != DOCA_SUCCESS)
    goto fail;

  member->port_id = port_id;
  member->active = true;
  group->member_count++;
  printf("FLOOD MEMBER ADD: vs=%u port=%u slot=%u\n", vswitch_id, port_id,
         slot);
  return DOCA_SUCCESS;

fail:
  if (member != NULL && member->rule.entry != NULL) {
    doca_error_t cleanup = remove_rule(pipeline, &member->rule,
                                       "rollback flood member");
    if (cleanup != DOCA_SUCCESS)
      return cleanup;
  }
  if (created) {
    doca_error_t cleanup =
        eswitch_pipeline_destroy_flood_group(pipeline, group);
    if (cleanup != DOCA_SUCCESS)
      return cleanup;
  }
  return result;
}

doca_error_t eswitch_pipeline_flood_remove_port(
    struct eswitch_pipeline *pipeline, uint16_t port_id,
    struct eswitch_flood_group *group) {
  struct eswitch_flood_member *member = NULL;
  uint16_t slot;
  doca_error_t result;

  if (pipeline == NULL || group == NULL || !pipeline->created ||
      group->pipe == NULL)
    return DOCA_ERROR_INVALID_VALUE;
  for (slot = 0; slot < group->member_capacity; slot++) {
    if (group->members[slot].active &&
        group->members[slot].port_id == port_id) {
      member = &group->members[slot];
      break;
    }
  }
  if (member == NULL)
    return DOCA_ERROR_NOT_FOUND;

  result = remove_rule(pipeline, &member->rule, "remove flood member");
  if (result != DOCA_SUCCESS)
    return result;
  *member = (struct eswitch_flood_member){0};
  group->member_count--;
  printf("FLOOD MEMBER REMOVE: vs=%u port=%u slot=%u\n", group->vswitch_id,
         port_id, slot);
  return DOCA_SUCCESS;
}

doca_error_t eswitch_pipeline_destroy_flood_group(
    struct eswitch_pipeline *pipeline, struct eswitch_flood_group *group) {
  doca_error_t result;

  if (pipeline == NULL || group == NULL)
    return DOCA_ERROR_INVALID_VALUE;
  if (group->pipe == NULL) {
    free(group->members);
    *group = (struct eswitch_flood_group){0};
    return DOCA_SUCCESS;
  }
  result = remove_rule(pipeline, &group->selector, "remove flood selector");
  if (result != DOCA_SUCCESS)
    return result;
  doca_flow_pipe_destroy(group->pipe);
  free(group->members);
  *group = (struct eswitch_flood_group){0};
  return DOCA_SUCCESS;
}

static void fill_source_match(struct doca_flow_match *match,
                              uint16_t vswitch_id, uint16_t port_id,
                              const struct rte_ether_addr *mac) {
  memset(match, 0, sizeof(*match));
  match->meta.pkt_meta = DOCA_HTOBE32(
      eswitch_metadata_encode(vswitch_id, port_id));
  memcpy(match->outer.eth.src_mac, mac->addr_bytes, RTE_ETHER_ADDR_LEN);
}

static void fill_destination_match(struct doca_flow_match *match,
                                   uint16_t vswitch_id,
                                   const struct rte_ether_addr *mac) {
  memset(match, 0, sizeof(*match));
  match->meta.pkt_meta = DOCA_HTOBE32((uint32_t)vswitch_id << 16);
  memcpy(match->outer.eth.dst_mac, mac->addr_bytes, RTE_ETHER_ADDR_LEN);
}

static doca_error_t fill_destination_fwd(struct eswitch_pipeline *pipeline,
                                         uint16_t vswitch_id,
                                         uint16_t learned_port_id,
                                         struct doca_flow_fwd *fwd) {
  struct doca_flow_pipe *gate_pipe = NULL;
  doca_error_t result;

  memset(fwd, 0, sizeof(*fwd));
  result = get_egress_gate(pipeline, vswitch_id, learned_port_id, &gate_pipe);
  if (result != DOCA_SUCCESS)
    return result;
  fwd->type = DOCA_FLOW_FWD_PIPE;
  fwd->next_pipe = gate_pipe;
  return DOCA_SUCCESS;
}

doca_error_t eswitch_pipeline_fdb_add(
    struct eswitch_pipeline *pipeline, uint16_t vswitch_id,
    const struct rte_ether_addr *mac, uint16_t learned_port_id,
    struct eswitch_hw_fdb_entry *hardware) {
  struct doca_flow_match match;
  struct doca_flow_fwd fwd;
  struct eswitch_rule *source_rule;
  doca_error_t original_error;
  doca_error_t result;

  if (pipeline == NULL || !pipeline->created || vswitch_id == 0 ||
      mac == NULL || hardware == NULL)
    return DOCA_ERROR_INVALID_VALUE;
  if (hardware->destination.entry != NULL ||
      hardware->sources[0].entry != NULL ||
      hardware->sources[1].entry != NULL)
    return DOCA_ERROR_BAD_STATE;

  fill_destination_match(&match, vswitch_id, mac);
  result = fill_destination_fwd(pipeline, vswitch_id, learned_port_id, &fwd);
  if (result != DOCA_SUCCESS)
    return result;
  flow_entry_cookie_prepare(&hardware->destination.cookie,
                            "add destination FDB", DOCA_FLOW_ENTRY_OP_ADD);
  result = doca_flow_pipe_basic_add_entry(
      pipeline->runtime->queue_id, pipeline->destination_pipe, &match, 0,
      NULL, NULL, &fwd, DOCA_FLOW_ENTRY_FLAGS_NO_WAIT,
      &hardware->destination.cookie, &hardware->destination.entry);
  if (result != DOCA_SUCCESS)
    goto rollback;
  result = process_rules(pipeline, &hardware->destination, 1);
  if (result != DOCA_SUCCESS)
    goto rollback;

  fill_source_match(&match, vswitch_id, learned_port_id, mac);
  source_rule = &hardware->sources[0];
  flow_entry_cookie_prepare(&source_rule->cookie, "add source guard",
                            DOCA_FLOW_ENTRY_OP_ADD);
  result = doca_flow_pipe_basic_add_entry(
      pipeline->runtime->queue_id, pipeline->source_guard_pipe, &match, 0,
      NULL, NULL, NULL, DOCA_FLOW_ENTRY_FLAGS_NO_WAIT,
      &source_rule->cookie, &source_rule->entry);
  if (result != DOCA_SUCCESS)
    goto rollback;
  result = process_rules(pipeline, source_rule, 1);
  if (result != DOCA_SUCCESS)
    goto rollback;
  hardware->active_source = 0;
  hardware->learned_port_id = learned_port_id;
  return DOCA_SUCCESS;

rollback:
  original_error = result;
  result = eswitch_pipeline_fdb_remove(pipeline, hardware);
  return result == DOCA_SUCCESS ? original_error : result;
}

doca_error_t eswitch_pipeline_fdb_move(
    struct eswitch_pipeline *pipeline, uint16_t vswitch_id,
    const struct rte_ether_addr *mac, uint16_t old_port_id,
    uint16_t new_port_id, struct eswitch_hw_fdb_entry *hardware) {
  struct eswitch_rule *new_source;
  struct eswitch_rule *old_source;
  uint8_t new_source_index;
  struct doca_flow_match match;
  struct doca_flow_fwd fwd;
  struct doca_flow_fwd old_fwd;
  doca_error_t original_error;
  doca_error_t result;

  if (pipeline == NULL || hardware == NULL || mac == NULL ||
      hardware->destination.entry == NULL ||
      hardware->active_source > 1 ||
      hardware->learned_port_id != old_port_id)
    return DOCA_ERROR_INVALID_VALUE;
  result = fill_destination_fwd(pipeline, vswitch_id, new_port_id, &fwd);
  if (result != DOCA_SUCCESS)
    return result;
  old_source = &hardware->sources[hardware->active_source];
  new_source_index = (uint8_t)(1U - hardware->active_source);
  new_source = &hardware->sources[new_source_index];
  if (old_source->entry == NULL || new_source->entry != NULL)
    return DOCA_ERROR_BAD_STATE;

  /* Install the new source guard before redirecting known unicast.  Each
   * hardware operation is completed separately so every failure has a
   * deterministic rollback path. */
  fill_source_match(&match, vswitch_id, new_port_id, mac);
  flow_entry_cookie_prepare(&new_source->cookie, "add moved source guard",
                            DOCA_FLOW_ENTRY_OP_ADD);
  result = doca_flow_pipe_basic_add_entry(
      pipeline->runtime->queue_id, pipeline->source_guard_pipe, &match, 0,
      NULL, NULL, NULL, DOCA_FLOW_ENTRY_FLAGS_NO_WAIT,
      &new_source->cookie, &new_source->entry);
  if (result != DOCA_SUCCESS)
    return result;
  result = process_rules(pipeline, new_source, 1);
  if (result != DOCA_SUCCESS)
    goto rollback_new_source;

  flow_entry_cookie_prepare(&hardware->destination.cookie,
                            "move destination FDB", DOCA_FLOW_ENTRY_OP_UPD);
  result = doca_flow_pipe_basic_update_entry(
      pipeline->runtime->queue_id, pipeline->destination_pipe, 0, NULL,
      NULL, &fwd, DOCA_FLOW_ENTRY_FLAGS_NO_WAIT,
      hardware->destination.entry);
  if (result != DOCA_SUCCESS)
    goto rollback_new_source;
  result = process_rules(pipeline, &hardware->destination, 1);
  if (result != DOCA_SUCCESS)
    goto rollback_destination;

  result = remove_rule(pipeline, old_source, "remove old source guard");
  if (result != DOCA_SUCCESS)
    goto rollback_destination;

  hardware->active_source = new_source_index;
  hardware->learned_port_id = new_port_id;
  return DOCA_SUCCESS;

rollback_destination:
  original_error = result;
  result = fill_destination_fwd(pipeline, vswitch_id, old_port_id, &old_fwd);
  if (result == DOCA_SUCCESS) {
    flow_entry_cookie_prepare(&hardware->destination.cookie,
                              "rollback destination FDB",
                              DOCA_FLOW_ENTRY_OP_UPD);
    result = doca_flow_pipe_basic_update_entry(
        pipeline->runtime->queue_id, pipeline->destination_pipe, 0, NULL,
        NULL, &old_fwd, DOCA_FLOW_ENTRY_FLAGS_NO_WAIT,
        hardware->destination.entry);
    if (result == DOCA_SUCCESS)
      result = process_rules(pipeline, &hardware->destination, 1);
  }
  if (result != DOCA_SUCCESS)
    return result;
  result = remove_rule(pipeline, new_source,
                       "rollback moved source guard");
  return result == DOCA_SUCCESS ? original_error : result;

rollback_new_source:
  original_error = result;
  result = remove_rule(pipeline, new_source,
                       "rollback moved source guard");
  return result == DOCA_SUCCESS ? original_error : result;
}

doca_error_t eswitch_pipeline_fdb_remove(
    struct eswitch_pipeline *pipeline,
    struct eswitch_hw_fdb_entry *hardware) {
  doca_error_t result;

  if (pipeline == NULL || hardware == NULL)
    return DOCA_ERROR_INVALID_VALUE;
  result = remove_rule(pipeline, &hardware->destination,
                       "remove destination FDB");
  if (result != DOCA_SUCCESS)
    return result;
  for (uint8_t i = 0; i < 2; i++) {
    result = remove_rule(pipeline, &hardware->sources[i],
                         "remove source guard");
    if (result != DOCA_SUCCESS)
      return result;
  }
  *hardware = (struct eswitch_hw_fdb_entry){0};
  return DOCA_SUCCESS;
}

doca_error_t eswitch_pipeline_fdb_query(
    const struct eswitch_hw_fdb_entry *hardware, uint64_t *packet_count) {
  struct doca_flow_resource_query query = {0};
  doca_error_t result;

  if (hardware == NULL || packet_count == NULL ||
      hardware->active_source > 1 ||
      hardware->sources[hardware->active_source].entry == NULL)
    return DOCA_ERROR_INVALID_VALUE;
  result = doca_flow_resource_query_entry(
      hardware->sources[hardware->active_source].entry, &query);
  if (result == DOCA_SUCCESS)
    *packet_count = query.counter.total_pkts;
  return result;
}
