#include "eswitch_pipeline.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <doca_bitfield.h>

#include "../ethernet_switch/switch_config.h"
#include "eswitch_config.h"

#define ESWITCH_MAX_FLOOD_MEMBERS 254U
#define ESWITCH_METADATA_VSWITCH_MASK UINT32_C(0xffff0000)
#define ESWITCH_METADATA_PORT_MASK UINT32_C(0x0000ffff)

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

static doca_error_t create_flood_selector(struct eswitch_pipeline *pipeline) {
  struct doca_flow_pipe_cfg *cfg = NULL;
  struct doca_flow_match match = {0};
  struct doca_flow_match mask = {0};
  struct doca_flow_fwd fwd = {.type = DOCA_FLOW_FWD_CHANGEABLE};
  struct doca_flow_fwd miss = {.type = DOCA_FLOW_FWD_DROP};
  doca_error_t result;

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

static doca_error_t create_ingress_classifier(
    struct eswitch_pipeline *pipeline) {
  struct doca_flow_pipe_cfg *cfg = NULL;
  struct doca_flow_match match = {0};
  struct doca_flow_actions actions = {0};
  struct doca_flow_actions *actions_array[1] = {&actions};
  struct doca_flow_fwd fwd = {0};
  struct doca_flow_fwd miss = {.type = DOCA_FLOW_FWD_DROP};
  doca_error_t result;

  match.parser_meta.port_id = UINT16_MAX;
  actions.meta.pkt_meta = UINT32_MAX;
  fwd.type = DOCA_FLOW_FWD_PIPE;
  fwd.next_pipe = pipeline->source_guard_pipe;
  result = doca_flow_pipe_cfg_create(&cfg, pipeline->switch_port);
  if (result != DOCA_SUCCESS)
    return result;
  result = set_pipe_identity(cfg, "ESW_INGRESS_CLASSIFIER",
                             DOCA_FLOW_PIPE_BASIC, true,
                             pipeline->ports->count);
  if (result == DOCA_SUCCESS)
    result = doca_flow_pipe_cfg_set_match(cfg, &match, NULL);
  if (result == DOCA_SUCCESS)
    result = doca_flow_pipe_cfg_set_actions(cfg, actions_array, NULL, NULL, 1);
  if (result == DOCA_SUCCESS)
    result = doca_flow_pipe_create(cfg, &fwd, &miss,
                                   &pipeline->ingress_classifier_pipe);
  doca_flow_pipe_cfg_destroy(cfg);
  if (result != DOCA_SUCCESS)
    return result;

  pipeline->classifier_rules = calloc(pipeline->ports->count,
                                      sizeof(*pipeline->classifier_rules));
  if (pipeline->classifier_rules == NULL)
    return DOCA_ERROR_NO_MEMORY;
  pipeline->egress_gates = calloc(pipeline->ports->count,
                                  sizeof(*pipeline->egress_gates));
  return pipeline->egress_gates == NULL ? DOCA_ERROR_NO_MEMORY
                                         : DOCA_SUCCESS;
}

doca_error_t eswitch_pipeline_create(struct flow_runtime *runtime,
                                     struct switch_flow_ports *ports,
                                     struct eswitch_pipeline *pipeline) {
  doca_error_t result;

  if (runtime == NULL || ports == NULL || pipeline == NULL ||
      !runtime->initialized || !ports->started || ports->count < 2)
    return DOCA_ERROR_INVALID_VALUE;
  pipeline->runtime = runtime;
  pipeline->ports = ports;
  pipeline->switch_port = ports->switch_port;

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
  CREATE_STAGE("flood selector", create_flood_selector(pipeline));
  CREATE_STAGE("destination FDB", create_destination_pipe(pipeline));
  CREATE_STAGE("learning clone", create_learning_clone(pipeline));
  CREATE_STAGE("learning dispatch", create_learning_dispatch(pipeline));
  CREATE_STAGE("source guard", create_source_guard(pipeline));
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
  if (pipeline->ingress_classifier_pipe != NULL)
    doca_flow_pipe_destroy(pipeline->ingress_classifier_pipe);
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
    }
  }
  free(pipeline->classifier_rules);
  free(pipeline->egress_gates);
  *pipeline = (struct eswitch_pipeline){0};
}

doca_error_t eswitch_pipeline_attach_port(struct eswitch_pipeline *pipeline,
                                          uint16_t port_index,
                                          uint16_t vswitch_id) {
  struct doca_flow_match match = {0};
  struct doca_flow_actions actions = {0};
  struct eswitch_rule *rule;
  uint16_t port_id;
  doca_error_t result;

  if (pipeline == NULL || !pipeline->created ||
      port_index >= pipeline->ports->count || vswitch_id == 0)
    return DOCA_ERROR_INVALID_VALUE;
  rule = &pipeline->classifier_rules[port_index];
  if (rule->entry != NULL)
    return DOCA_ERROR_BAD_STATE;
  port_id = pipeline->ports->items[port_index].ethernet->port_id;
  match.parser_meta.port_id = port_id;
  actions.meta.pkt_meta = DOCA_HTOBE32(
      eswitch_metadata_encode(vswitch_id, port_id));
  flow_entry_cookie_prepare(&rule->cookie, "attach ingress port",
                            DOCA_FLOW_ENTRY_OP_ADD);
  result = doca_flow_pipe_basic_add_entry(
      pipeline->runtime->queue_id, pipeline->ingress_classifier_pipe, &match,
      0, &actions, NULL, NULL, DOCA_FLOW_ENTRY_FLAGS_NO_WAIT, &rule->cookie,
      &rule->entry);
  if (result != DOCA_SUCCESS)
    return result;
  result = process_rules(pipeline, rule, 1);
  if (result != DOCA_SUCCESS) {
    doca_error_t original_error = result;
    doca_error_t cleanup = remove_rule(pipeline, rule,
                                       "rollback ingress attach");
    return cleanup == DOCA_SUCCESS ? original_error : cleanup;
  }
  return DOCA_SUCCESS;
}

doca_error_t eswitch_pipeline_detach_port(struct eswitch_pipeline *pipeline,
                                          uint16_t port_index) {
  if (pipeline == NULL || !pipeline->created ||
      port_index >= pipeline->ports->count)
    return DOCA_ERROR_INVALID_VALUE;
  return remove_rule(pipeline, &pipeline->classifier_rules[port_index],
                     "detach ingress port");
}

static doca_error_t create_egress_gate(struct eswitch_pipeline *pipeline,
                                       uint16_t port_index) {
  struct eswitch_egress_gate *gate = &pipeline->egress_gates[port_index];
  struct doca_flow_pipe_cfg *cfg = NULL;
  struct doca_flow_match match = {0};
  struct doca_flow_match mask = {0};
  struct doca_flow_fwd fwd = {0};
  char name[64];
  uint16_t port_id = pipeline->ports->items[port_index].ethernet->port_id;
  doca_error_t result;

  if (gate->pipe != NULL)
    return DOCA_SUCCESS;

  snprintf(name, sizeof(name), "ESW_EGRESS_GATE_%u", port_id);
  result = doca_flow_pipe_cfg_create(&cfg, pipeline->switch_port);
  if (result != DOCA_SUCCESS)
    return result;
  result = set_pipe_identity(cfg, name, DOCA_FLOW_PIPE_CONTROL, false, 2);
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
      NULL, NULL, NULL, 0, &fwd, &gate->drop_self.cookie,
      &gate->drop_self.entry);
  if (result != DOCA_SUCCESS)
    goto fail;
  result = process_rules(pipeline, &gate->drop_self, 1);
  if (result != DOCA_SUCCESS)
    goto fail;

  memset(&match, 0, sizeof(match));
  memset(&fwd, 0, sizeof(fwd));
  fwd.type = DOCA_FLOW_FWD_PORT;
  fwd.port_id = port_id;
  flow_entry_cookie_prepare(&gate->forward.cookie, "egress forward",
                            DOCA_FLOW_ENTRY_OP_ADD);
  result = doca_flow_pipe_control_add_entry(
      pipeline->runtime->queue_id, gate->pipe, &match, NULL, NULL, NULL,
      NULL, NULL, NULL, 1, &fwd, &gate->forward.cookie,
      &gate->forward.entry);
  if (result != DOCA_SUCCESS)
    goto fail;
  result = process_rules(pipeline, &gate->forward, 1);
  if (result != DOCA_SUCCESS)
    goto fail;

  printf("EGRESS GATE CREATE: port=%u\n", port_id);
  return DOCA_SUCCESS;

fail:
  doca_flow_pipe_destroy(gate->pipe);
  *gate = (struct eswitch_egress_gate){0};
  return result;
}

static doca_error_t get_egress_gate(struct eswitch_pipeline *pipeline,
                                    uint16_t port_id,
                                    struct doca_flow_pipe **gate_pipe) {
  int port_index;
  doca_error_t result;

  if (pipeline == NULL || gate_pipe == NULL)
    return DOCA_ERROR_INVALID_VALUE;
  port_index = find_port_index(pipeline, port_id);
  if (port_index < 0)
    return DOCA_ERROR_NOT_FOUND;
  result = create_egress_gate(pipeline, (uint16_t)port_index);
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

  result = get_egress_gate(pipeline, port_id, &gate_pipe);
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
                                         uint16_t learned_port_id,
                                         struct doca_flow_fwd *fwd) {
  struct doca_flow_pipe *gate_pipe = NULL;
  doca_error_t result;

  memset(fwd, 0, sizeof(*fwd));
  result = get_egress_gate(pipeline, learned_port_id, &gate_pipe);
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
  result = fill_destination_fwd(pipeline, learned_port_id, &fwd);
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
  result = fill_destination_fwd(pipeline, new_port_id, &fwd);
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
  result = fill_destination_fwd(pipeline, old_port_id, &old_fwd);
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
