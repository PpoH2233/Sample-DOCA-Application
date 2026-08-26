#ifndef FLOW_RUNTIME_H
#define FLOW_RUNTIME_H

#include <stdbool.h>
#include <stdint.h>

#include <doca_error.h>
#include <doca_flow.h>

struct flow_runtime {
  uint16_t queue_id;
  bool initialized;
};

/* Stable user context stored with a Flow entry for its entire lifetime. */
struct flow_entry_cookie {
  const char *name;
  enum doca_flow_entry_op last_op;
  enum doca_flow_entry_status last_status;
  bool completed;
};

doca_error_t flow_runtime_init(struct flow_runtime *runtime,
                               uint32_t counter_count);
doca_error_t flow_runtime_destroy(struct flow_runtime *runtime);

void flow_entry_cookie_prepare(struct flow_entry_cookie *cookie,
                               const char *name,
                               enum doca_flow_entry_op operation);

/* Wait for exactly expected operations and reject callback-level failures. */
doca_error_t flow_runtime_process(struct flow_runtime *runtime,
                                  struct doca_flow_port *switch_port,
                                  struct flow_entry_cookie **cookies,
                                  uint32_t expected);

#endif /* FLOW_RUNTIME_H */
