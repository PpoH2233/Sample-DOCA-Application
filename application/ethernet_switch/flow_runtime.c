#include "flow_runtime.h"

#include <stdio.h>
#include <string.h>

#include "switch_config.h"

static void entry_process_callback(struct doca_flow_pipe_entry *entry,
                                   uint16_t pipe_queue,
                                   enum doca_flow_entry_status status,
                                   enum doca_flow_entry_op operation,
                                   void *user_context) {
  struct flow_entry_cookie *cookie = user_context;

  (void)entry;
  (void)pipe_queue;
  if (cookie == NULL)
    return;

  cookie->last_op = operation;
  cookie->last_status = status;
  cookie->completed = true;

  if (status != DOCA_FLOW_ENTRY_STATUS_SUCCESS)
    fprintf(stderr, "Flow operation failed: %s (op=%d status=%d)\n",
            cookie->name == NULL ? "unnamed entry" : cookie->name,
            operation, status);
}

void flow_entry_cookie_prepare(struct flow_entry_cookie *cookie,
                               const char *name,
                               enum doca_flow_entry_op operation) {
  cookie->name = name;
  cookie->last_op = operation;
  cookie->last_status = DOCA_FLOW_ENTRY_STATUS_IN_PROCESS;
  cookie->completed = false;
}

doca_error_t flow_runtime_init(struct flow_runtime *runtime,
                               uint32_t counter_count) {
  struct doca_flow_cfg *cfg = NULL;
  doca_error_t result;
  doca_error_t destroy_result;

  if (runtime == NULL || counter_count == 0)
    return DOCA_ERROR_INVALID_VALUE;
  if (runtime->initialized)
    return DOCA_ERROR_BAD_STATE;

  result = doca_flow_cfg_create(&cfg);
  if (result != DOCA_SUCCESS)
    return result;

  result = doca_flow_cfg_set_pipe_queues(cfg, 1);
  if (result != DOCA_SUCCESS)
    goto destroy_cfg;
  result = doca_flow_cfg_set_mode_args(cfg, SWITCH_FLOW_MODE_ARGS);
  if (result != DOCA_SUCCESS)
    goto destroy_cfg;
  result = doca_flow_cfg_set_nr_counters(cfg, counter_count);
  if (result != DOCA_SUCCESS)
    goto destroy_cfg;
  result = doca_flow_cfg_set_cb_entry_process(cfg, entry_process_callback);
  if (result != DOCA_SUCCESS)
    goto destroy_cfg;

  result = doca_flow_init(cfg);
  if (result == DOCA_SUCCESS) {
    runtime->queue_id = SWITCH_FLOW_QUEUE_ID;
    runtime->initialized = true;
  }

destroy_cfg:
  destroy_result = doca_flow_cfg_destroy(cfg);
  if (result == DOCA_SUCCESS && destroy_result != DOCA_SUCCESS)
    result = destroy_result;
  return result;
}

doca_error_t flow_runtime_destroy(struct flow_runtime *runtime) {
  if (runtime == NULL)
    return DOCA_ERROR_INVALID_VALUE;
  if (!runtime->initialized)
    return DOCA_SUCCESS;

  /* DOCA 3.4 declares doca_flow_destroy() as void. */
  doca_flow_destroy();
  *runtime = (struct flow_runtime){0};
  return DOCA_SUCCESS;
}

doca_error_t flow_runtime_process(struct flow_runtime *runtime,
                                  struct doca_flow_port *switch_port,
                                  struct flow_entry_cookie **cookies,
                                  uint32_t expected) {
  doca_error_t result;

  if (runtime == NULL || switch_port == NULL ||
      (expected != 0 && cookies == NULL))
    return DOCA_ERROR_INVALID_VALUE;
  if (!runtime->initialized)
    return DOCA_ERROR_BAD_STATE;
  if (expected == 0)
    return DOCA_SUCCESS;

  result = doca_flow_entries_process(switch_port, runtime->queue_id,
                                     SWITCH_FLOW_TIMEOUT_US, expected);
  if (result != DOCA_SUCCESS)
    return result;

  for (uint32_t i = 0; i < expected; i++) {
    if (cookies[i] == NULL || !cookies[i]->completed ||
        cookies[i]->last_status != DOCA_FLOW_ENTRY_STATUS_SUCCESS)
      return DOCA_ERROR_BAD_STATE;
  }

  return DOCA_SUCCESS;
}
