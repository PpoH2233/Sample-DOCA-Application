#ifndef VF10_TX_GENERATOR_H
#define VF10_TX_GENERATOR_H

#include <doca_error.h>

struct flow_switch_ctx;

int vf10_tx_validate_environment(void);
doca_error_t vf10_tx_validate_ports(struct flow_switch_ctx *ctx);
doca_error_t vf10_tx_run(int nb_queues, int nb_ports,
                         struct flow_switch_ctx *ctx);
void vf10_tx_release_pool(void);

#endif
