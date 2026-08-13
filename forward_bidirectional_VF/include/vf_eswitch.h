#ifndef VF_ESWITCH_H
#define VF_ESWITCH_H

#include <doca_error.h>

#include <flow_switch_common.h>

/*
 * A switch-mode DOCA Flow application has three logical ports:
 *   port 0: PF proxy / switch manager
 *   port 1: the first VF representor supplied with -r
 *   port 2: the second VF representor supplied with -r
 */
#define VF_ESWITCH_FLOW_PORT_COUNT 3
#define VF_ESWITCH_ENTRY_COUNT 2

doca_error_t vf_eswitch_run(int nb_queues, struct flow_switch_ctx *ctx);

#endif /* VF_ESWITCH_H */
