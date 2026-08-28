#include "l2_pipeline.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <doca_bitfield.h>

#include "l2_fdb.h"
#include "switch_config.h"

static doca_error_t set_pipe_identity(struct doca_flow_pipe_cfg *cfg,
                                      const char *name,
                                      enum doca_flow_pipe_type type,
                                      bool is_root,
                                      uint32_t entries) {
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

static uint32_t entry_flags(uint32_t index, uint32_t count) {
  return index + 1 == count ? DOCA_FLOW_ENTRY_FLAGS_NO_WAIT
                            : DOCA_FLOW_ENTRY_FLAGS_WAIT_FOR_BATCH;
}

static uint32_t next_power_of_two(uint32_t value) {
  uint32_t capacity = 1;

  while (capacity < value)
    capacity <<= 1;
  return capacity;
}

static doca_error_t report_pipeline_stage(const char *stage,
                                          doca_error_t result) {
  if (result != DOCA_SUCCESS)
    fprintf(stderr, "L2 pipeline stage '%s' failed: %s\n", stage,
            doca_error_get_descr(result));
  return result;
}

static doca_error_t report_flow_api(const char *api, doca_error_t result) {
  if (result != DOCA_SUCCESS)
    fprintf(stderr, "  %s failed: %s\n", api,
            doca_error_get_descr(result));
  return result;
}

static doca_error_t process_rules(struct l2_pipeline *pipeline,
                                  struct l2_static_rule *rules,
                                  uint32_t count) {
  struct flow_entry_cookie **cookies;
  doca_error_t result;

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

static doca_error_t create_rss_pipe(struct l2_pipeline *pipeline) {
  struct doca_flow_pipe_cfg *cfg = NULL;
  struct doca_flow_match match = {0};
  struct doca_flow_fwd fwd = {0};
  uint16_t queue = SWITCH_RX_QUEUE_ID;
  doca_error_t result;

  result = report_flow_api(
      "doca_flow_pipe_cfg_create(L2_RSS_PIPE)",
      doca_flow_pipe_cfg_create(&cfg, pipeline->switch_port));
  if (result != DOCA_SUCCESS)
    return result;
  result = report_flow_api(
      "configure L2_RSS_PIPE",
      set_pipe_identity(cfg, "L2_RSS_PIPE", DOCA_FLOW_PIPE_BASIC, false, 1));
  if (result != DOCA_SUCCESS)
    goto destroy_cfg;
  result = report_flow_api("doca_flow_pipe_cfg_set_match(L2_RSS_PIPE)",
                           doca_flow_pipe_cfg_set_match(cfg, &match, NULL));
  if (result != DOCA_SUCCESS)
    goto destroy_cfg;

  fwd.type = DOCA_FLOW_FWD_RSS;
  fwd.rss_type = DOCA_FLOW_RESOURCE_TYPE_NON_SHARED;
  fwd.rss.queues_array = &queue;
  fwd.rss.nr_queues = 1;
  fwd.rss.inner_flags = DOCA_FLOW_RSS_AUTO;
  result = report_flow_api(
      "doca_flow_pipe_create(L2_RSS_PIPE)",
      doca_flow_pipe_create(cfg, &fwd, NULL, &pipeline->rss_pipe));

destroy_cfg:
  doca_flow_pipe_cfg_destroy(cfg);
  if (result != DOCA_SUCCESS)
    return result;

  flow_entry_cookie_prepare(&pipeline->rss_rule.cookie, "rss catch-all",
                            DOCA_FLOW_ENTRY_OP_ADD);
  result = report_flow_api(
      "doca_flow_pipe_basic_add_entry(L2_RSS_PIPE)",
      doca_flow_pipe_basic_add_entry(
          pipeline->runtime->queue_id, pipeline->rss_pipe, &match, 0, NULL,
          NULL, NULL, DOCA_FLOW_ENTRY_FLAGS_NO_WAIT,
          &pipeline->rss_rule.cookie, &pipeline->rss_rule.entry));
  if (result != DOCA_SUCCESS)
    return result;
  return report_flow_api("doca_flow_entries_process(L2_RSS_PIPE)",
                         process_rules(pipeline, &pipeline->rss_rule, 1));
}

static doca_error_t create_one_flood_pipe(struct l2_pipeline *pipeline,
                                          uint16_t ingress_index,
                                          uint32_t *rule_cursor) {
  struct doca_flow_pipe_cfg *cfg = NULL;
  struct doca_flow_fwd pipe_fwd = {.type = DOCA_FLOW_FWD_CHANGEABLE};
  uint32_t member_count = pipeline->ports->count - 1;
  uint32_t pipe_capacity = next_power_of_two(member_count);
  uint32_t member_index = 0;
  uint32_t first_rule = *rule_cursor;
  char name[64];
  doca_error_t result;

  snprintf(name, sizeof(name), "L2_FLOOD_FROM_PORT_%u",
           pipeline->ports->items[ingress_index].ethernet->port_id);
  result = doca_flow_pipe_cfg_create(&cfg, pipeline->switch_port);
  if (result != DOCA_SUCCESS)
    return result;
  /* Hash-pipe capacity must be a power of two; only real egress members get
   * entries, so an ingress with six peers uses capacity eight and six rules. */
  result = set_pipe_identity(cfg, name, DOCA_FLOW_PIPE_HASH, false,
                             pipe_capacity);
  if (result != DOCA_SUCCESS)
    goto destroy_cfg;
  result = doca_flow_pipe_cfg_set_hash_map_algorithm(
      cfg, DOCA_FLOW_PIPE_HASH_MAP_ALGORITHM_FLOODING);
  if (result != DOCA_SUCCESS)
    goto destroy_cfg;
  result = doca_flow_pipe_create(cfg, &pipe_fwd, NULL,
                                 &pipeline->flood_pipes[ingress_index]);

destroy_cfg:
  doca_flow_pipe_cfg_destroy(cfg);
  if (result != DOCA_SUCCESS)
    return result;

  for (uint16_t egress_index = 0; egress_index < pipeline->ports->count;
       egress_index++) {
    struct l2_static_rule *rule;
    struct doca_flow_fwd fwd = {0};

    if (egress_index == ingress_index)
      continue;

    rule = &pipeline->flood_rules[*rule_cursor];
    flow_entry_cookie_prepare(&rule->cookie, "flood member",
                              DOCA_FLOW_ENTRY_OP_ADD);
    fwd.type = DOCA_FLOW_FWD_PORT;
    fwd.port_id = pipeline->ports->items[egress_index].ethernet->port_id;

    result = doca_flow_pipe_hash_add_entry(
        pipeline->runtime->queue_id,
        pipeline->flood_pipes[ingress_index], member_index, 0, NULL, NULL,
        &fwd, entry_flags(member_index, member_count), &rule->cookie,
        &rule->entry);
    if (result != DOCA_SUCCESS)
      return result;

    member_index++;
    (*rule_cursor)++;
  }

  return process_rules(pipeline, &pipeline->flood_rules[first_rule],
                       member_count);
}

static doca_error_t create_flood_pipes(struct l2_pipeline *pipeline) {
  uint32_t rule_cursor = 0;
  doca_error_t result;

  pipeline->flood_pipes = calloc(pipeline->ports->count,
                                 sizeof(*pipeline->flood_pipes));
  pipeline->flood_rule_count =
      pipeline->ports->count * (pipeline->ports->count - 1);
  pipeline->flood_rules = calloc(pipeline->flood_rule_count,
                                 sizeof(*pipeline->flood_rules));
  if (pipeline->flood_pipes == NULL || pipeline->flood_rules == NULL)
    return DOCA_ERROR_NO_MEMORY;

  for (uint16_t i = 0; i < pipeline->ports->count; i++) {
    result = create_one_flood_pipe(pipeline, i, &rule_cursor);
    if (result != DOCA_SUCCESS)
      return result;
  }
  return DOCA_SUCCESS;
}

static doca_error_t create_flood_selector_pipe(struct l2_pipeline *pipeline) {
  struct doca_flow_pipe_cfg *cfg = NULL;
  struct doca_flow_match match = {0};
  struct doca_flow_match mask = {0};
  struct doca_flow_fwd pipe_fwd = {.type = DOCA_FLOW_FWD_CHANGEABLE};
  doca_error_t result;

  mask.meta.pkt_meta = UINT32_MAX;
  result = doca_flow_pipe_cfg_create(&cfg, pipeline->switch_port);
  if (result != DOCA_SUCCESS)
    return result;
  result = set_pipe_identity(cfg, "L2_FLOOD_SELECTOR",
                             DOCA_FLOW_PIPE_BASIC, false,
                             pipeline->ports->count);
  if (result != DOCA_SUCCESS)
    goto destroy_cfg;
  result = doca_flow_pipe_cfg_set_match(cfg, &match, &mask);
  if (result != DOCA_SUCCESS)
    goto destroy_cfg;
  result = doca_flow_pipe_create(cfg, &pipe_fwd, NULL,
                                 &pipeline->flood_selector_pipe);

destroy_cfg:
  doca_flow_pipe_cfg_destroy(cfg);
  if (result != DOCA_SUCCESS)
    return result;

  pipeline->selector_rules = calloc(pipeline->ports->count,
                                    sizeof(*pipeline->selector_rules));
  if (pipeline->selector_rules == NULL)
    return DOCA_ERROR_NO_MEMORY;

  for (uint16_t i = 0; i < pipeline->ports->count; i++) {
    struct doca_flow_fwd fwd = {0};
    struct l2_static_rule *rule = &pipeline->selector_rules[i];

    memset(&match, 0, sizeof(match));
    match.meta.pkt_meta = DOCA_HTOBE32(
        pipeline->ports->items[i].ethernet->port_id);
    fwd.type = DOCA_FLOW_FWD_HASH_PIPE;
    fwd.hash_pipe.pipe = pipeline->flood_pipes[i];
    fwd.hash_pipe.algorithm = DOCA_FLOW_PIPE_HASH_MAP_ALGORITHM_FLOODING;
    flow_entry_cookie_prepare(&rule->cookie, "flood selector",
                              DOCA_FLOW_ENTRY_OP_ADD);

    result = doca_flow_pipe_basic_add_entry(
        pipeline->runtime->queue_id, pipeline->flood_selector_pipe, &match, 0,
        NULL, NULL, &fwd, entry_flags(i, pipeline->ports->count),
        &rule->cookie, &rule->entry);
    if (result != DOCA_SUCCESS)
      return result;
  }

  return process_rules(pipeline, pipeline->selector_rules,
                       pipeline->ports->count);
}

static doca_error_t create_destination_pipe(struct l2_pipeline *pipeline) {
  struct doca_flow_pipe_cfg *cfg = NULL;
  struct doca_flow_match match = {0};
  struct doca_flow_match mask = {0};
  struct doca_flow_fwd fwd = {.type = DOCA_FLOW_FWD_CHANGEABLE};
  struct doca_flow_fwd miss = {0};
  uint32_t capacity = SWITCH_MAX_FDB_ENTRIES * pipeline->ports->count;
  doca_error_t result;

  mask.meta.pkt_meta = UINT32_MAX;
  memset(mask.outer.eth.dst_mac, UINT8_MAX,
         sizeof(mask.outer.eth.dst_mac));
  miss.type = DOCA_FLOW_FWD_PIPE;
  miss.next_pipe = pipeline->flood_selector_pipe;

  result = doca_flow_pipe_cfg_create(&cfg, pipeline->switch_port);
  if (result != DOCA_SUCCESS)
    return result;
  result = set_pipe_identity(cfg, "L2_DESTINATION_FDB",
                             DOCA_FLOW_PIPE_BASIC, false, capacity);
  if (result != DOCA_SUCCESS)
    goto destroy_cfg;
  result = doca_flow_pipe_cfg_set_match(cfg, &match, &mask);
  if (result != DOCA_SUCCESS)
    goto destroy_cfg;
  result = doca_flow_pipe_create(cfg, &fwd, &miss,
                                 &pipeline->destination_pipe);

destroy_cfg:
  doca_flow_pipe_cfg_destroy(cfg);
  return result;
}

static doca_error_t create_learning_clone_pipe(struct l2_pipeline *pipeline) {
  struct doca_flow_pipe_cfg *cfg = NULL;
  struct doca_flow_fwd pipe_fwd = {.type = DOCA_FLOW_FWD_CHANGEABLE};
  struct doca_flow_fwd fwds[2] = {0};
  doca_error_t result;

  result = doca_flow_pipe_cfg_create(&cfg, pipeline->switch_port);
  if (result != DOCA_SUCCESS)
    return result;
  result = set_pipe_identity(cfg, "L2_LEARNING_CLONE",
                             DOCA_FLOW_PIPE_HASH, false, 2);
  if (result != DOCA_SUCCESS)
    goto destroy_cfg;
  result = doca_flow_pipe_cfg_set_hash_map_algorithm(
      cfg, DOCA_FLOW_PIPE_HASH_MAP_ALGORITHM_FLOODING);
  if (result != DOCA_SUCCESS)
    goto destroy_cfg;
  result = doca_flow_pipe_create(cfg, &pipe_fwd, NULL,
                                 &pipeline->learning_clone_pipe);

destroy_cfg:
  doca_flow_pipe_cfg_destroy(cfg);
  if (result != DOCA_SUCCESS)
    return result;

  fwds[0].type = DOCA_FLOW_FWD_PIPE;
  fwds[0].next_pipe = pipeline->rss_pipe;
  fwds[1].type = DOCA_FLOW_FWD_PIPE;
  fwds[1].next_pipe = pipeline->destination_pipe;

  for (uint32_t i = 0; i < 2; i++) {
    struct l2_static_rule *rule = &pipeline->learning_clone_rules[i];

    flow_entry_cookie_prepare(&rule->cookie,
                              i == 0 ? "learning RSS copy"
                                     : "learning forwarding copy",
                              DOCA_FLOW_ENTRY_OP_ADD);
    result = doca_flow_pipe_hash_add_entry(
        pipeline->runtime->queue_id, pipeline->learning_clone_pipe, i, 0,
        NULL, NULL, &fwds[i], entry_flags(i, 2), &rule->cookie,
        &rule->entry);
    if (result != DOCA_SUCCESS)
      return result;
  }

  return process_rules(pipeline, pipeline->learning_clone_rules, 2);
}

/*
 * DOCA Flow 3.4 accepts HASH_PIPE forwarding on a pipe or entry, but not as
 * the miss forwarding of a BASIC pipe.  This one-entry BASIC pipe adapts the
 * source-guard miss path to the flooding hash pipe used for packet cloning.
 */
static doca_error_t create_learning_dispatch_pipe(
    struct l2_pipeline *pipeline) {
  struct doca_flow_pipe_cfg *cfg = NULL;
  struct doca_flow_match match = {0};
  struct doca_flow_fwd fwd = {0};
  doca_error_t result;

  fwd.type = DOCA_FLOW_FWD_HASH_PIPE;
  fwd.hash_pipe.pipe = pipeline->learning_clone_pipe;
  fwd.hash_pipe.algorithm = DOCA_FLOW_PIPE_HASH_MAP_ALGORITHM_FLOODING;

  result = report_flow_api(
      "doca_flow_pipe_cfg_create(L2_LEARNING_DISPATCH)",
      doca_flow_pipe_cfg_create(&cfg, pipeline->switch_port));
  if (result != DOCA_SUCCESS)
    return result;
  result = report_flow_api(
      "configure L2_LEARNING_DISPATCH",
      set_pipe_identity(cfg, "L2_LEARNING_DISPATCH", DOCA_FLOW_PIPE_BASIC,
                        false, 1));
  if (result != DOCA_SUCCESS)
    goto destroy_cfg;
  result = report_flow_api(
      "doca_flow_pipe_cfg_set_match(L2_LEARNING_DISPATCH)",
      doca_flow_pipe_cfg_set_match(cfg, &match, NULL));
  if (result != DOCA_SUCCESS)
    goto destroy_cfg;
  result = report_flow_api(
      "doca_flow_pipe_create(L2_LEARNING_DISPATCH)",
      doca_flow_pipe_create(cfg, &fwd, NULL,
                            &pipeline->learning_dispatch_pipe));

destroy_cfg:
  doca_flow_pipe_cfg_destroy(cfg);
  if (result != DOCA_SUCCESS)
    return result;

  flow_entry_cookie_prepare(&pipeline->learning_dispatch_rule.cookie,
                            "learning dispatch", DOCA_FLOW_ENTRY_OP_ADD);
  result = report_flow_api(
      "doca_flow_pipe_basic_add_entry(L2_LEARNING_DISPATCH)",
      doca_flow_pipe_basic_add_entry(
          pipeline->runtime->queue_id, pipeline->learning_dispatch_pipe,
          &match, 0, NULL, NULL, NULL, DOCA_FLOW_ENTRY_FLAGS_NO_WAIT,
          &pipeline->learning_dispatch_rule.cookie,
          &pipeline->learning_dispatch_rule.entry));
  if (result != DOCA_SUCCESS)
    return result;

  return report_flow_api(
      "doca_flow_entries_process(L2_LEARNING_DISPATCH)",
      process_rules(pipeline, &pipeline->learning_dispatch_rule, 1));
}

static doca_error_t create_source_guard_pipe(struct l2_pipeline *pipeline) {
  struct doca_flow_pipe_cfg *cfg = NULL;
  struct doca_flow_match match = {0};
  struct doca_flow_match mask = {0};
  struct doca_flow_monitor monitor = {0};
  struct doca_flow_fwd fwd = {0};
  struct doca_flow_fwd miss = {0};
  doca_error_t result;

  mask.meta.pkt_meta = UINT32_MAX;
  memset(mask.outer.eth.src_mac, UINT8_MAX,
         sizeof(mask.outer.eth.src_mac));
  monitor.counter_type = DOCA_FLOW_RESOURCE_TYPE_NON_SHARED;
  fwd.type = DOCA_FLOW_FWD_PIPE;
  fwd.next_pipe = pipeline->destination_pipe;
  miss.type = DOCA_FLOW_FWD_PIPE;
  miss.next_pipe = pipeline->learning_dispatch_pipe;

  result = report_flow_api(
      "doca_flow_pipe_cfg_create(L2_SOURCE_GUARD)",
      doca_flow_pipe_cfg_create(&cfg, pipeline->switch_port));
  if (result != DOCA_SUCCESS)
    return result;
  result = report_flow_api(
      "configure L2_SOURCE_GUARD",
      set_pipe_identity(cfg, "L2_SOURCE_GUARD", DOCA_FLOW_PIPE_BASIC, false,
                        SWITCH_MAX_FDB_ENTRIES));
  if (result != DOCA_SUCCESS)
    goto destroy_cfg;
  result = report_flow_api(
      "doca_flow_pipe_cfg_set_match(L2_SOURCE_GUARD)",
      doca_flow_pipe_cfg_set_match(cfg, &match, &mask));
  if (result != DOCA_SUCCESS)
    goto destroy_cfg;
  result = report_flow_api(
      "doca_flow_pipe_cfg_set_monitor(L2_SOURCE_GUARD)",
      doca_flow_pipe_cfg_set_monitor(cfg, &monitor));
  if (result != DOCA_SUCCESS)
    goto destroy_cfg;
  result = report_flow_api(
      "doca_flow_pipe_create(L2_SOURCE_GUARD)",
      doca_flow_pipe_create(cfg, &fwd, &miss,
                            &pipeline->source_guard_pipe));

destroy_cfg:
  doca_flow_pipe_cfg_destroy(cfg);
  return result;
}

static doca_error_t create_ingress_classifier(struct l2_pipeline *pipeline) {
  struct doca_flow_pipe_cfg *cfg = NULL;
  struct doca_flow_match match = {0};
  struct doca_flow_actions actions_template = {0};
  struct doca_flow_actions *actions_array[1];
  struct doca_flow_fwd fwd = {0};
  struct doca_flow_fwd miss = {.type = DOCA_FLOW_FWD_DROP};
  doca_error_t result;

  match.parser_meta.port_id = UINT16_MAX;
  actions_template.meta.pkt_meta = UINT32_MAX;
  actions_array[0] = &actions_template;
  fwd.type = DOCA_FLOW_FWD_PIPE;
  fwd.next_pipe = pipeline->source_guard_pipe;

  result = doca_flow_pipe_cfg_create(&cfg, pipeline->switch_port);
  if (result != DOCA_SUCCESS)
    return result;
  result = set_pipe_identity(cfg, "L2_INGRESS_CLASSIFIER",
                             DOCA_FLOW_PIPE_BASIC, true,
                             pipeline->ports->count);
  if (result != DOCA_SUCCESS)
    goto destroy_cfg;
  result = doca_flow_pipe_cfg_set_match(cfg, &match, NULL);
  if (result != DOCA_SUCCESS)
    goto destroy_cfg;
  result = doca_flow_pipe_cfg_set_actions(cfg, actions_array, NULL, NULL, 1);
  if (result != DOCA_SUCCESS)
    goto destroy_cfg;
  result = doca_flow_pipe_create(cfg, &fwd, &miss,
                                 &pipeline->ingress_classifier_pipe);

destroy_cfg:
  doca_flow_pipe_cfg_destroy(cfg);
  if (result != DOCA_SUCCESS)
    return result;

  pipeline->ingress_rules = calloc(pipeline->ports->count,
                                   sizeof(*pipeline->ingress_rules));
  if (pipeline->ingress_rules == NULL)
    return DOCA_ERROR_NO_MEMORY;

  for (uint16_t i = 0; i < pipeline->ports->count; i++) {
    struct doca_flow_actions actions = {0};
    struct l2_static_rule *rule = &pipeline->ingress_rules[i];
    uint16_t port_id = pipeline->ports->items[i].ethernet->port_id;

    memset(&match, 0, sizeof(match));
    match.parser_meta.port_id = port_id;
    actions.meta.pkt_meta = DOCA_HTOBE32(port_id);
    flow_entry_cookie_prepare(&rule->cookie, "ingress classifier",
                              DOCA_FLOW_ENTRY_OP_ADD);

    result = doca_flow_pipe_basic_add_entry(
        pipeline->runtime->queue_id, pipeline->ingress_classifier_pipe,
        &match, 0, &actions, NULL, NULL,
        entry_flags(i, pipeline->ports->count), &rule->cookie,
        &rule->entry);
    if (result != DOCA_SUCCESS)
      return result;
  }

  return process_rules(pipeline, pipeline->ingress_rules,
                       pipeline->ports->count);
}

doca_error_t l2_pipeline_create(struct flow_runtime *runtime,
                                struct switch_flow_ports *ports,
                                struct l2_pipeline *pipeline) {
  doca_error_t result;

  if (runtime == NULL || ports == NULL || pipeline == NULL ||
      !runtime->initialized || !ports->started || ports->count < 2)
    return DOCA_ERROR_INVALID_VALUE;

  pipeline->runtime = runtime;
  pipeline->ports = ports;
  pipeline->switch_port = ports->switch_port;

  printf("Creating L2 pipeline stage: RSS slow path\n");
  result = report_pipeline_stage("RSS slow path", create_rss_pipe(pipeline));
  if (result != DOCA_SUCCESS)
    goto fail;
  printf("Creating L2 pipeline stage: flood groups\n");
  result = report_pipeline_stage("flood groups",
                                 create_flood_pipes(pipeline));
  if (result != DOCA_SUCCESS)
    goto fail;
  printf("Creating L2 pipeline stage: flood selector\n");
  result = report_pipeline_stage("flood selector",
                                 create_flood_selector_pipe(pipeline));
  if (result != DOCA_SUCCESS)
    goto fail;
  printf("Creating L2 pipeline stage: destination FDB\n");
  result = report_pipeline_stage("destination FDB",
                                 create_destination_pipe(pipeline));
  if (result != DOCA_SUCCESS)
    goto fail;
  printf("Creating L2 pipeline stage: learning clone\n");
  result = report_pipeline_stage("learning clone",
                                 create_learning_clone_pipe(pipeline));
  if (result != DOCA_SUCCESS)
    goto fail;
  printf("Creating L2 pipeline stage: learning dispatch\n");
  result = report_pipeline_stage("learning dispatch",
                                 create_learning_dispatch_pipe(pipeline));
  if (result != DOCA_SUCCESS)
    goto fail;
  printf("Creating L2 pipeline stage: source guard\n");
  result = report_pipeline_stage("source guard",
                                 create_source_guard_pipe(pipeline));
  if (result != DOCA_SUCCESS)
    goto fail;
  printf("Creating L2 pipeline stage: ingress classifier\n");
  result = report_pipeline_stage("ingress classifier",
                                 create_ingress_classifier(pipeline));
  if (result != DOCA_SUCCESS)
    goto fail;

  pipeline->created = true;
  return DOCA_SUCCESS;

fail:
  l2_pipeline_destroy(pipeline);
  return result;
}

void l2_pipeline_destroy(struct l2_pipeline *pipeline) {
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
  if (pipeline->flood_pipes != NULL) {
    for (uint16_t i = 0; i < pipeline->ports->count; i++) {
      if (pipeline->flood_pipes[i] != NULL)
        doca_flow_pipe_destroy(pipeline->flood_pipes[i]);
    }
  }
  if (pipeline->rss_pipe != NULL)
    doca_flow_pipe_destroy(pipeline->rss_pipe);

  free(pipeline->flood_pipes);
  free(pipeline->flood_rules);
  free(pipeline->selector_rules);
  free(pipeline->ingress_rules);
  *pipeline = (struct l2_pipeline){0};
}

static void fill_source_match(struct doca_flow_match *match,
                              uint16_t port_id,
                              const struct rte_ether_addr *mac) {
  memset(match, 0, sizeof(*match));
  match->meta.pkt_meta = DOCA_HTOBE32(port_id);
  memcpy(match->outer.eth.src_mac, mac->addr_bytes,
         RTE_ETHER_ADDR_LEN);
}

static void fill_destination_match(struct doca_flow_match *match,
                                   uint16_t port_id,
                                   const struct rte_ether_addr *mac) {
  memset(match, 0, sizeof(*match));
  match->meta.pkt_meta = DOCA_HTOBE32(port_id);
  memcpy(match->outer.eth.dst_mac, mac->addr_bytes,
         RTE_ETHER_ADDR_LEN);
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

doca_error_t l2_pipeline_add_fdb_entry(struct l2_pipeline *pipeline,
                                       struct l2_fdb_entry *entry) {
  struct flow_entry_cookie **cookies;
  struct doca_flow_match match;
  uint32_t operation_count;
  doca_error_t result;

  if (pipeline == NULL || entry == NULL || !pipeline->created)
    return DOCA_ERROR_INVALID_VALUE;

  entry->destination_rule_count = pipeline->ports->count;
  entry->destination_rules = calloc(entry->destination_rule_count,
                                    sizeof(*entry->destination_rules));
  cookies = calloc(entry->destination_rule_count + 1, sizeof(*cookies));
  if (entry->destination_rules == NULL || cookies == NULL) {
    free(cookies);
    return DOCA_ERROR_NO_MEMORY;
  }

  operation_count = entry->destination_rule_count + 1;
  for (uint16_t i = 0; i < entry->destination_rule_count; i++) {
    struct doca_flow_fwd fwd;
    struct l2_dynamic_rule *rule = &entry->destination_rules[i];
    uint16_t ingress = pipeline->ports->items[i].ethernet->port_id;

    fill_destination_match(&match, ingress, &entry->mac);
    fill_destination_fwd(ingress, entry->learned_port_id, &fwd);
    flow_entry_cookie_prepare(&rule->cookie, "dynamic destination",
                              DOCA_FLOW_ENTRY_OP_ADD);
    cookies[i] = &rule->cookie;
    result = doca_flow_pipe_basic_add_entry(
        pipeline->runtime->queue_id, pipeline->destination_pipe, &match, 0,
        NULL, NULL, &fwd, DOCA_FLOW_ENTRY_FLAGS_WAIT_FOR_BATCH,
        &rule->cookie, &rule->entry);
    if (result != DOCA_SUCCESS)
      goto out;
  }

  fill_source_match(&match, entry->learned_port_id, &entry->mac);
  flow_entry_cookie_prepare(&entry->source_rule.cookie, "dynamic source",
                            DOCA_FLOW_ENTRY_OP_ADD);
  cookies[entry->destination_rule_count] = &entry->source_rule.cookie;
  result = doca_flow_pipe_basic_add_entry(
      pipeline->runtime->queue_id, pipeline->source_guard_pipe, &match, 0,
      NULL, NULL, NULL, DOCA_FLOW_ENTRY_FLAGS_NO_WAIT,
      &entry->source_rule.cookie, &entry->source_rule.entry);
  if (result != DOCA_SUCCESS)
    goto out;

  result = flow_runtime_process(pipeline->runtime, pipeline->switch_port,
                                cookies, operation_count);

out:
  free(cookies);
  return result;
}

doca_error_t l2_pipeline_move_fdb_entry(struct l2_pipeline *pipeline,
                                        struct l2_fdb_entry *entry,
                                        uint16_t new_port_id) {
  struct flow_entry_cookie **cookies;
  struct doca_flow_pipe_entry *old_source;
  struct doca_flow_pipe_entry *new_source = NULL;
  struct doca_flow_match match;
  uint32_t operation_count;
  doca_error_t result;

  if (pipeline == NULL || entry == NULL || !pipeline->created)
    return DOCA_ERROR_INVALID_VALUE;

  operation_count = entry->destination_rule_count + 1;
  cookies = calloc(operation_count, sizeof(*cookies));
  if (cookies == NULL)
    return DOCA_ERROR_NO_MEMORY;

  for (uint16_t i = 0; i < entry->destination_rule_count; i++) {
    struct doca_flow_fwd fwd;
    struct l2_dynamic_rule *rule = &entry->destination_rules[i];
    uint16_t ingress = pipeline->ports->items[i].ethernet->port_id;

    fill_destination_fwd(ingress, new_port_id, &fwd);
    flow_entry_cookie_prepare(&rule->cookie, "move destination",
                              DOCA_FLOW_ENTRY_OP_UPD);
    cookies[i] = &rule->cookie;
    result = doca_flow_pipe_basic_update_entry(
        pipeline->runtime->queue_id, pipeline->destination_pipe, 0, NULL,
        NULL, &fwd, DOCA_FLOW_ENTRY_FLAGS_WAIT_FOR_BATCH, rule->entry);
    if (result != DOCA_SUCCESS)
      goto out;
  }

  fill_source_match(&match, new_port_id, &entry->mac);
  flow_entry_cookie_prepare(&entry->source_rule.cookie, "move new source",
                            DOCA_FLOW_ENTRY_OP_ADD);
  cookies[entry->destination_rule_count] = &entry->source_rule.cookie;
  result = doca_flow_pipe_basic_add_entry(
      pipeline->runtime->queue_id, pipeline->source_guard_pipe, &match, 0,
      NULL, NULL, NULL, DOCA_FLOW_ENTRY_FLAGS_NO_WAIT,
      &entry->source_rule.cookie, &new_source);
  if (result != DOCA_SUCCESS)
    goto out;

  result = flow_runtime_process(pipeline->runtime, pipeline->switch_port,
                                cookies, operation_count);
  if (result != DOCA_SUCCESS)
    goto out;

  old_source = entry->source_rule.entry;
  entry->source_rule.entry = new_source;
  flow_entry_cookie_prepare(&entry->source_rule.cookie, "remove old source",
                            DOCA_FLOW_ENTRY_OP_DEL);
  result = doca_flow_pipe_remove_entry(
      pipeline->runtime->queue_id, DOCA_FLOW_ENTRY_FLAGS_NO_WAIT, old_source);
  if (result == DOCA_SUCCESS) {
    struct flow_entry_cookie *remove_cookie = &entry->source_rule.cookie;
    result = flow_runtime_process(pipeline->runtime, pipeline->switch_port,
                                  &remove_cookie, 1);
  }

out:
  free(cookies);
  return result;
}

doca_error_t l2_pipeline_remove_fdb_entry(struct l2_pipeline *pipeline,
                                          struct l2_fdb_entry *entry) {
  struct flow_entry_cookie *cookie;
  doca_error_t result;

  if (pipeline == NULL || entry == NULL || !pipeline->created)
    return DOCA_ERROR_INVALID_VALUE;

  /*
   * Dynamic rules were installed destination-first and source-last.  Remove
   * them in reverse order so a packet cannot keep hitting SOURCE_GUARD while
   * its destination rules are being dismantled.
   *
   * Entry deletion is a latency-sensitive control-path operation.  The DOCA
   * switch sample uses NO_WAIT for removal; batching deletes from different
   * pipes can complete with DOCA_FLOW_ENTRY_STATUS_ERROR on the HWS switch
   * port.  Process one deletion at a time so a successful handle can be
   * cleared immediately and will never be submitted a second time.
   */
  if (entry->source_rule.entry != NULL) {
    flow_entry_cookie_prepare(&entry->source_rule.cookie, "remove source",
                              DOCA_FLOW_ENTRY_OP_DEL);
    result = doca_flow_pipe_remove_entry(
        pipeline->runtime->queue_id, DOCA_FLOW_ENTRY_FLAGS_NO_WAIT,
        entry->source_rule.entry);
    if (result != DOCA_SUCCESS)
      return result;

    cookie = &entry->source_rule.cookie;
    result = flow_runtime_process(pipeline->runtime, pipeline->switch_port,
                                  &cookie, 1);
    if (result != DOCA_SUCCESS)
      return result;
    entry->source_rule.entry = NULL;
  }

  for (uint16_t i = 0; i < entry->destination_rule_count; i++) {
    struct l2_dynamic_rule *rule = &entry->destination_rules[i];

    if (rule->entry == NULL)
      continue;

    flow_entry_cookie_prepare(&rule->cookie, "remove destination",
                              DOCA_FLOW_ENTRY_OP_DEL);
    result = doca_flow_pipe_remove_entry(
        pipeline->runtime->queue_id, DOCA_FLOW_ENTRY_FLAGS_NO_WAIT,
        rule->entry);
    if (result != DOCA_SUCCESS)
      return result;

    cookie = &rule->cookie;
    result = flow_runtime_process(pipeline->runtime, pipeline->switch_port,
                                  &cookie, 1);
    if (result != DOCA_SUCCESS)
      return result;
    rule->entry = NULL;
  }

  return DOCA_SUCCESS;
}

doca_error_t l2_pipeline_query_source_counter(
    const struct l2_fdb_entry *entry,
    uint64_t *packet_count) {
  struct doca_flow_resource_query query = {0};
  doca_error_t result;

  if (entry == NULL || packet_count == NULL ||
      entry->source_rule.entry == NULL)
    return DOCA_ERROR_INVALID_VALUE;

  result = doca_flow_resource_query_entry(entry->source_rule.entry, &query);
  if (result == DOCA_SUCCESS)
    *packet_count = query.counter.total_pkts;
  return result;
}
