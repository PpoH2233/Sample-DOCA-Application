#ifndef VF10_RX_MONITOR_H
#define VF10_RX_MONITOR_H

#include <doca_error.h>

struct flow_switch_ctx;

int vf10_monitor_validate_environment(void);
doca_error_t vf10_monitor_validate_ports(struct flow_switch_ctx *ctx);
doca_error_t vf10_monitor_run(int nb_queues, int nb_ports,
                              struct flow_switch_ctx *ctx);

#endif
