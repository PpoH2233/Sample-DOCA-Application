#include "router_nat_ct.h"

#include <string.h>

#include <doca_flow_ct.h>

#include "../eswitch_config.h"

doca_error_t router_nat_ct_init(bool requested, bool supported,
                                uint32_t capacity,
                                struct router_nat_ct_runtime *runtime) {
  struct doca_flow_ct_cfg *cfg = NULL;
  struct doca_flow_meta zone_mask = {0};
  struct doca_flow_ct_meta modify_mask = {0};
  uint32_t actions_mem_size;
  doca_error_t result;
  doca_error_t cleanup;

  if (runtime == NULL || capacity < ESWITCH_CT_MIN_CAPACITY ||
      capacity > ESWITCH_CT_MAX_CAPACITY)
    return DOCA_ERROR_INVALID_VALUE;
  *runtime = (struct router_nat_ct_runtime){
      .requested = requested,
      .supported = supported,
      .requested_capacity = capacity,
  };
  if (!requested)
    return DOCA_SUCCESS;
  if (!supported) {
    runtime->degraded = true;
    return DOCA_ERROR_NOT_SUPPORTED;
  }

  actions_mem_size = capacity * ESWITCH_CT_ACTIONS_MEM_PER_CONNECTION;
  if (actions_mem_size < ESWITCH_CT_ACTIONS_MEM_MIN_SIZE)
    actions_mem_size = ESWITCH_CT_ACTIONS_MEM_MIN_SIZE;

  result = doca_flow_ct_cfg_create(&cfg);
  if (result != DOCA_SUCCESS)
    return result;
  result = doca_flow_ct_cfg_set_flags(
      cfg, DOCA_FLOW_CT_FLAG_NO_AGING | DOCA_FLOW_CT_FLAG_NO_COUNTER);
  if (result == DOCA_SUCCESS)
    result = doca_flow_ct_cfg_set_queues(cfg, 1);
  if (result == DOCA_SUCCESS)
    result = doca_flow_ct_cfg_set_queue_depth(cfg, ESWITCH_CT_QUEUE_DEPTH);
  if (result == DOCA_SUCCESS)
    result = doca_flow_ct_cfg_set_ctrl_queues(cfg, 1);
  if (result == DOCA_SUCCESS)
    result = doca_flow_ct_cfg_set_actions_mem_size(
        cfg, actions_mem_size);

  /* meta.u32[1] carries the connection-private admission zone.
   * meta.u32[0] is reserved for the
   * post-CT adjacency selector written independently in each direction. */
  zone_mask.u32[1] = UINT32_MAX;
  modify_mask.flow.u32[0] = UINT32_MAX;
  if (result == DOCA_SUCCESS)
    result = doca_flow_ct_cfg_set_direction(
        cfg, false, false, &zone_mask, &modify_mask);
  if (result == DOCA_SUCCESS)
    result = doca_flow_ct_cfg_set_direction(
        cfg, true, false, &zone_mask, &modify_mask);
  if (result == DOCA_SUCCESS)
    result = doca_flow_ct_init(cfg);

  cleanup = doca_flow_ct_cfg_destroy(cfg);
  if (result == DOCA_SUCCESS && cleanup != DOCA_SUCCESS)
    result = cleanup;
  runtime->initialized = result == DOCA_SUCCESS;
  runtime->degraded = result != DOCA_SUCCESS;
  return result;
}

void router_nat_ct_destroy(struct router_nat_ct_runtime *runtime) {
  if (runtime == NULL)
    return;
  if (runtime->initialized)
    doca_flow_ct_destroy();
  memset(runtime, 0, sizeof(*runtime));
}
