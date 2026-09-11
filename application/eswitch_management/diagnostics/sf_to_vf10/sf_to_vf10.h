#ifndef SF_TO_VF10_H
#define SF_TO_VF10_H

#include <stdint.h>

#include <doca_error.h>

struct flow_switch_ctx;

struct sf_vf_port_map {
	uint16_t parent_port;
	uint16_t sf_port;
	uint16_t vf_port;
};

int sf_to_vf10_validate_environment(void);
doca_error_t sf_to_vf10_validate_ports(struct flow_switch_ctx *ctx,
				       struct sf_vf_port_map *map);
doca_error_t sf_to_vf10_run(int nb_queues, int nb_ports,
			    struct flow_switch_ctx *ctx,
			    const struct sf_vf_port_map *map);

#endif
