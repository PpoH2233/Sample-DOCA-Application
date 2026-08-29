#include "eswitch_pipeline.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <doca_bitfield.h>

#include "../ethernet_switch/switch_config.h"

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

  mask.meta.pkt_meta = UINT32_MAX;
  result = doca_flow_pipe_cfg_create(&cfg, pipeline->switch_port);
  if (result != DOCA_SUCCESS)
    return result;
  result = set_pipe_identity(cfg, "ESW_FLOOD_SELECTOR", DOCA_FLOW_PIPE_BASIC,
                             false, pipeline->ports->count);
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
  uint32_t capacity = SWITCH_MAX_FDB_ENTRIES * pipeline->ports->count;
  doca_error_t result;

  mask.meta.pkt_meta = UINT32_MAX;
  memset(mask.outer.eth.dst_mac, UINT8_MAX, RTE_ETHER_ADDR_LEN);
  miss.type = DOCA_FLOW_FWD_PIPE;
  miss.next_pipe = pipeline->flood_selector_pipe;
  result = doca_flow_pipe_cfg_create(&cfg, pipeline->switch_port);
  if (result != DOCA_SUCCESS)
    return result;
  result = set_pipe_identity(cfg, "ESW_DEST_FDB", DOCA_FLOW_PIPE_BASIC,
                             false, capacity);
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
  return pipeline->classifier_rules == NULL ? DOCA_ERROR_NO_MEMORY
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
  free(pipeline->classifier_rules);
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
  return process_rules(pipeline, rule, 1);
}

doca_error_t eswitch_pipeline_detach_port(struct eswitch_pipeline *pipeline,
                                          uint16_t port_index) {
  if (pipeline == NULL || !pipeline->created ||
      port_index >= pipeline->ports->count)
    return DOCA_ERROR_INVALID_VALUE;
  return remove_rule(pipeline, &pipeline->classifier_rules[port_index],
                     "detach ingress port");
}

static doca_error_t create_flood_path(struct eswitch_pipeline *pipeline,
                                      uint16_t vswitch_id,
                                      uint16_t ingress_port_id,
                                      const uint16_t *member_port_ids,
                                      uint16_t member_count,
                                      struct eswitch_flood_path *path) {
  struct doca_flow_pipe_cfg *cfg = NULL;
  struct doca_flow_fwd pipe_fwd = {.type = DOCA_FLOW_FWD_CHANGEABLE};
  struct doca_flow_match selector_match = {0};
  struct doca_flow_fwd selector_fwd = {0};
  char name[64];
  uint16_t cursor = 0;
  doca_error_t result;

  path->ingress_port_id = ingress_port_id;
  path->member_count = member_count - 1;
  selector_match.meta.pkt_meta = DOCA_HTOBE32(
      eswitch_metadata_encode(vswitch_id, ingress_port_id));

  /* A two-port vSwitch has exactly one peer for each ingress. Forwarding
   * directly avoids constructing a one-entry flooding hash pipe. Apart from
   * being cheaper, this follows the ordinary switch-mode PORT-forwarding
   * path and avoids relying on a capacity-one flooding hash map. */
  if (path->member_count == 1) {
    for (uint16_t i = 0; i < member_count; i++) {
      if (member_port_ids[i] == ingress_port_id)
        continue;
      selector_fwd.type = DOCA_FLOW_FWD_PORT;
      selector_fwd.port_id = member_port_ids[i];
      printf("FLOOD PATH: vs=%u ingress=%u -> port=%u (direct)\n",
             vswitch_id, ingress_port_id, member_port_ids[i]);
      break;
    }
  } else {
    path->members = calloc(path->member_count, sizeof(*path->members));
    if (path->members == NULL)
      return DOCA_ERROR_NO_MEMORY;

    snprintf(name, sizeof(name), "ESW_VS%u_FLOOD_FROM_%u", vswitch_id,
             ingress_port_id);
    result = doca_flow_pipe_cfg_create(&cfg, pipeline->switch_port);
    if (result != DOCA_SUCCESS)
      return result;
    result = set_pipe_identity(cfg, name, DOCA_FLOW_PIPE_HASH, false,
                               next_power_of_two(path->member_count));
    if (result == DOCA_SUCCESS)
      result = doca_flow_pipe_cfg_set_hash_map_algorithm(
          cfg, DOCA_FLOW_PIPE_HASH_MAP_ALGORITHM_FLOODING);
    if (result == DOCA_SUCCESS)
      result = doca_flow_pipe_create(cfg, &pipe_fwd, NULL, &path->pipe);
    doca_flow_pipe_cfg_destroy(cfg);
    if (result != DOCA_SUCCESS)
      return result;

    for (uint16_t i = 0; i < member_count; i++) {
      struct doca_flow_fwd fwd = {0};
      struct eswitch_rule *rule;

      if (member_port_ids[i] == ingress_port_id)
        continue;
      rule = &path->members[cursor];
      fwd.type = DOCA_FLOW_FWD_PORT;
      fwd.port_id = member_port_ids[i];
      printf("FLOOD PATH: vs=%u ingress=%u -> port=%u (group)\n",
             vswitch_id, ingress_port_id, member_port_ids[i]);
      flow_entry_cookie_prepare(&rule->cookie, "flood member",
                                DOCA_FLOW_ENTRY_OP_ADD);
      result = doca_flow_pipe_hash_add_entry(
          pipeline->runtime->queue_id, path->pipe, cursor, 0, NULL, NULL,
          &fwd, batch_flags(cursor, path->member_count), &rule->cookie,
          &rule->entry);
      if (result != DOCA_SUCCESS)
        return result;
      cursor++;
    }
    result = process_rules(pipeline, path->members, path->member_count);
    if (result != DOCA_SUCCESS)
      return result;

    selector_fwd.type = DOCA_FLOW_FWD_HASH_PIPE;
    selector_fwd.hash_pipe.pipe = path->pipe;
    selector_fwd.hash_pipe.algorithm =
        DOCA_FLOW_PIPE_HASH_MAP_ALGORITHM_FLOODING;
  }

  flow_entry_cookie_prepare(&path->selector.cookie, "flood selector",
                            DOCA_FLOW_ENTRY_OP_ADD);
  result = doca_flow_pipe_basic_add_entry(
      pipeline->runtime->queue_id, pipeline->flood_selector_pipe,
      &selector_match, 0, NULL, NULL, &selector_fwd,
      DOCA_FLOW_ENTRY_FLAGS_NO_WAIT, &path->selector.cookie,
      &path->selector.entry);
  if (result != DOCA_SUCCESS)
    return result;
  return process_rules(pipeline, &path->selector, 1);
}

doca_error_t eswitch_pipeline_build_flood_group(
    struct eswitch_pipeline *pipeline, uint16_t vswitch_id,
    const uint16_t *member_port_ids, uint16_t member_count,
    struct eswitch_flood_group *group) {
  doca_error_t result;

  if (pipeline == NULL || group == NULL || !pipeline->created ||
      vswitch_id == 0 || (member_count != 0 && member_port_ids == NULL))
    return DOCA_ERROR_INVALID_VALUE;
  if (group->paths != NULL)
    return DOCA_ERROR_BAD_STATE;

  group->vswitch_id = vswitch_id;
  if (member_count < 2)
    return DOCA_SUCCESS;
  group->paths = calloc(member_count, sizeof(*group->paths));
  if (group->paths == NULL)
    return DOCA_ERROR_NO_MEMORY;
  group->path_count = member_count;

  for (uint16_t i = 0; i < member_count; i++) {
    result = create_flood_path(pipeline, vswitch_id, member_port_ids[i],
                               member_port_ids, member_count,
                               &group->paths[i]);
    if (result != DOCA_SUCCESS) {
      doca_error_t cleanup =
          eswitch_pipeline_destroy_flood_group(pipeline, group);
      return cleanup == DOCA_SUCCESS ? result : cleanup;
    }
  }
  return DOCA_SUCCESS;
}

doca_error_t eswitch_pipeline_destroy_flood_group(
    struct eswitch_pipeline *pipeline, struct eswitch_flood_group *group) {
  doca_error_t first_error = DOCA_SUCCESS;

  if (pipeline == NULL || group == NULL)
    return DOCA_ERROR_INVALID_VALUE;
  for (uint16_t i = 0; i < group->path_count; i++) {
    struct eswitch_flood_path *path = &group->paths[i];
    doca_error_t result = remove_rule(pipeline, &path->selector,
                                      "remove flood selector");
    if (first_error == DOCA_SUCCESS && result != DOCA_SUCCESS)
      first_error = result;
  }
  if (first_error != DOCA_SUCCESS)
    return first_error;

  for (uint16_t i = 0; i < group->path_count; i++) {
    if (group->paths[i].pipe != NULL)
      doca_flow_pipe_destroy(group->paths[i].pipe);
    free(group->paths[i].members);
  }
  free(group->paths);
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
                                   uint16_t vswitch_id, uint16_t port_id,
                                   const struct rte_ether_addr *mac) {
  memset(match, 0, sizeof(*match));
  match->meta.pkt_meta = DOCA_HTOBE32(
      eswitch_metadata_encode(vswitch_id, port_id));
  memcpy(match->outer.eth.dst_mac, mac->addr_bytes, RTE_ETHER_ADDR_LEN);
}

static void fill_destination_fwd(uint16_t ingress_port_id,
                                 uint16_t learned_port_id,
                                 struct doca_flow_fwd *fwd) {
  memset(fwd, 0, sizeof(*fwd));
  if (ingress_port_id == learned_port_id) {
    fwd->type = DOCA_FLOW_FWD_DROP;
  } else {
    fwd->type = DOCA_FLOW_FWD_PORT;
    fwd->port_id = learned_port_id;
  }
}

doca_error_t eswitch_pipeline_fdb_add(
    struct eswitch_pipeline *pipeline, uint16_t vswitch_id,
    const struct rte_ether_addr *mac, uint16_t learned_port_id,
    const uint16_t *member_port_ids, uint16_t member_count,
    struct eswitch_hw_fdb_entry *hardware) {
  struct doca_flow_match match;
  doca_error_t original_error;
  doca_error_t result;

  if (pipeline == NULL || !pipeline->created || vswitch_id == 0 ||
      mac == NULL || member_port_ids == NULL || member_count == 0 ||
      hardware == NULL)
    return DOCA_ERROR_INVALID_VALUE;
  hardware->destination_count = member_count;
  hardware->destinations =
      calloc(member_count, sizeof(*hardware->destinations));
  if (hardware->destinations == NULL)
    return DOCA_ERROR_NO_MEMORY;

  /* Dynamic learning is not latency-critical. Process one entry at a time so
   * every successful handle can be rolled back deterministically on error. */
  for (uint16_t i = 0; i < member_count; i++) {
    struct doca_flow_fwd fwd;
    struct eswitch_rule *rule = &hardware->destinations[i];

    fill_destination_match(&match, vswitch_id, member_port_ids[i], mac);
    fill_destination_fwd(member_port_ids[i], learned_port_id, &fwd);
    flow_entry_cookie_prepare(&rule->cookie, "add destination FDB",
                              DOCA_FLOW_ENTRY_OP_ADD);
    result = doca_flow_pipe_basic_add_entry(
        pipeline->runtime->queue_id, pipeline->destination_pipe, &match, 0,
        NULL, NULL, &fwd, DOCA_FLOW_ENTRY_FLAGS_NO_WAIT, &rule->cookie,
        &rule->entry);
    if (result != DOCA_SUCCESS)
      goto rollback;
    result = process_rules(pipeline, rule, 1);
    if (result != DOCA_SUCCESS)
      goto rollback;
  }

  fill_source_match(&match, vswitch_id, learned_port_id, mac);
  flow_entry_cookie_prepare(&hardware->source.cookie, "add source guard",
                            DOCA_FLOW_ENTRY_OP_ADD);
  result = doca_flow_pipe_basic_add_entry(
      pipeline->runtime->queue_id, pipeline->source_guard_pipe, &match, 0,
      NULL, NULL, NULL, DOCA_FLOW_ENTRY_FLAGS_NO_WAIT,
      &hardware->source.cookie, &hardware->source.entry);
  if (result != DOCA_SUCCESS)
    goto rollback;
  result = process_rules(pipeline, &hardware->source, 1);
  if (result != DOCA_SUCCESS)
    goto rollback;
  return DOCA_SUCCESS;

rollback:
  original_error = result;
  result = eswitch_pipeline_fdb_remove(pipeline, hardware);
  return result == DOCA_SUCCESS ? original_error : result;
}

doca_error_t eswitch_pipeline_fdb_move(
    struct eswitch_pipeline *pipeline, uint16_t vswitch_id,
    const struct rte_ether_addr *mac, uint16_t old_port_id,
    uint16_t new_port_id, const uint16_t *member_port_ids,
    uint16_t member_count, struct eswitch_hw_fdb_entry *hardware) {
  struct doca_flow_pipe_entry *old_source;
  struct doca_flow_pipe_entry *new_source = NULL;
  struct flow_entry_cookie **cookies;
  struct doca_flow_match match;
  doca_error_t result;

  (void)old_port_id;
  if (pipeline == NULL || hardware == NULL || mac == NULL ||
      member_port_ids == NULL || member_count != hardware->destination_count)
    return DOCA_ERROR_INVALID_VALUE;
  cookies = calloc(member_count + 1, sizeof(*cookies));
  if (cookies == NULL)
    return DOCA_ERROR_NO_MEMORY;

  for (uint16_t i = 0; i < member_count; i++) {
    struct doca_flow_fwd fwd;
    struct eswitch_rule *rule = &hardware->destinations[i];

    fill_destination_fwd(member_port_ids[i], new_port_id, &fwd);
    flow_entry_cookie_prepare(&rule->cookie, "move destination FDB",
                              DOCA_FLOW_ENTRY_OP_UPD);
    cookies[i] = &rule->cookie;
    result = doca_flow_pipe_basic_update_entry(
        pipeline->runtime->queue_id, pipeline->destination_pipe, 0, NULL,
        NULL, &fwd, DOCA_FLOW_ENTRY_FLAGS_WAIT_FOR_BATCH, rule->entry);
    if (result != DOCA_SUCCESS)
      goto out;
  }
  fill_source_match(&match, vswitch_id, new_port_id, mac);
  flow_entry_cookie_prepare(&hardware->source.cookie, "move source guard",
                            DOCA_FLOW_ENTRY_OP_ADD);
  cookies[member_count] = &hardware->source.cookie;
  result = doca_flow_pipe_basic_add_entry(
      pipeline->runtime->queue_id, pipeline->source_guard_pipe, &match, 0,
      NULL, NULL, NULL, DOCA_FLOW_ENTRY_FLAGS_NO_WAIT,
      &hardware->source.cookie, &new_source);
  if (result != DOCA_SUCCESS)
    goto out;
  result = flow_runtime_process(pipeline->runtime, pipeline->switch_port,
                                cookies, member_count + 1);
  if (result != DOCA_SUCCESS)
    goto out;

  old_source = hardware->source.entry;
  hardware->source.entry = new_source;
  flow_entry_cookie_prepare(&hardware->source.cookie, "remove moved source",
                            DOCA_FLOW_ENTRY_OP_DEL);
  result = doca_flow_pipe_remove_entry(pipeline->runtime->queue_id,
                                       DOCA_FLOW_ENTRY_FLAGS_NO_WAIT,
                                       old_source);
  if (result == DOCA_SUCCESS) {
    struct flow_entry_cookie *cookie = &hardware->source.cookie;
    result = flow_runtime_process(pipeline->runtime, pipeline->switch_port,
                                  &cookie, 1);
  }
out:
  free(cookies);
  return result;
}

doca_error_t eswitch_pipeline_fdb_remove(
    struct eswitch_pipeline *pipeline,
    struct eswitch_hw_fdb_entry *hardware) {
  doca_error_t result;

  if (pipeline == NULL || hardware == NULL)
    return DOCA_ERROR_INVALID_VALUE;
  result = remove_rule(pipeline, &hardware->source, "remove source guard");
  if (result != DOCA_SUCCESS)
    return result;
  for (uint16_t i = 0; i < hardware->destination_count; i++) {
    result = remove_rule(pipeline, &hardware->destinations[i],
                         "remove destination FDB");
    if (result != DOCA_SUCCESS)
      return result;
  }
  free(hardware->destinations);
  *hardware = (struct eswitch_hw_fdb_entry){0};
  return DOCA_SUCCESS;
}

doca_error_t eswitch_pipeline_fdb_query(
    const struct eswitch_hw_fdb_entry *hardware, uint64_t *packet_count) {
  struct doca_flow_resource_query query = {0};
  doca_error_t result;

  if (hardware == NULL || packet_count == NULL ||
      hardware->source.entry == NULL)
    return DOCA_ERROR_INVALID_VALUE;
  result = doca_flow_resource_query_entry(hardware->source.entry, &query);
  if (result == DOCA_SUCCESS)
    *packet_count = query.counter.total_pkts;
  return result;
}
