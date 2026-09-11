/* SPDX-License-Identifier: BSD-3-Clause
 * Device bring-up follows NVIDIA's installed DOCA 3.4 switch-mode samples.
 */
#include <stdlib.h>

#include <rte_ethdev.h>

#include <doca_argp.h>
#include <doca_dev.h>
#include <doca_flow.h>
#include <doca_log.h>

#include <flow_common.h>
#include <flow_switch_common.h>
#include <dpdk_utils.h>

#include "vf10_tx_generator.h"

DOCA_LOG_REGISTER(VF10_TX_GENERATOR::MAIN);

#define GENERATOR_PORTS 2

int main(int argc, char **argv)
{
	doca_error_t result;
	struct doca_log_backend *sdk_log = NULL;
	int exit_status = EXIT_FAILURE;
	struct application_dpdk_config dpdk_config = {
		.port_config.nb_ports = GENERATOR_PORTS,
		.port_config.nb_queues = 1,
		.port_config.switch_mode = 1,
		.port_config.enable_mbuf_metadata = 1,
	};
	struct flow_switch_ctx ctx = {0};

	if (vf10_tx_validate_environment() != 0)
		return EXIT_FAILURE;

	result = doca_log_backend_create_standard();
	if (result != DOCA_SUCCESS)
		goto out;
	result = doca_log_backend_create_with_file_sdk(stderr, &sdk_log);
	if (result != DOCA_SUCCESS)
		goto out;
	result = doca_log_backend_set_sdk_level(sdk_log, DOCA_LOG_LEVEL_WARNING);
	if (result != DOCA_SUCCESS)
		goto out;

	result = doca_argp_init(NULL, &ctx);
	if (result != DOCA_SUCCESS)
		goto out;
	result = register_doca_flow_switch_params();
	if (result != DOCA_SUCCESS)
		goto argp_cleanup;
	result = register_flow_device_no_wire_to_wire_params();
	if (result != DOCA_SUCCESS)
		goto argp_cleanup;

	doca_argp_set_dpdk_program(flow_init_dpdk);
	ctx.devs_ctx.default_dev_args = FLOW_SWITCH_DEV_ARGS;
	result = doca_argp_start(argc, argv);
	if (result != DOCA_SUCCESS) {
		DOCA_LOG_ERR("Failed to parse arguments: %s", doca_error_get_descr(result));
		goto argp_cleanup;
	}

	result = init_doca_flow_devs(&ctx.devs_ctx);
	if (result != DOCA_SUCCESS) {
		DOCA_LOG_ERR("Failed to initialize DOCA Flow devices: %s",
		             doca_error_get_descr(result));
		goto dpdk_cleanup;
	}
	if (rte_eth_dev_count_avail() != GENERATOR_PORTS) {
		DOCA_LOG_ERR("Expected one parent and one VF representor; found %u DPDK ports",
		             rte_eth_dev_count_avail());
		goto dpdk_cleanup;
	}

	result = vf10_tx_validate_ports(&ctx);
	if (result != DOCA_SUCCESS) {
		DOCA_LOG_ERR("VF10 mapping validation failed: %s",
		             doca_error_get_descr(result));
		goto dpdk_cleanup;
	}

	result = dpdk_queues_and_ports_init(&dpdk_config);
	if (result != DOCA_SUCCESS) {
		DOCA_LOG_ERR("Failed to initialize parent TX queue: %s",
		             doca_error_get_descr(result));
		goto dpdk_cleanup;
	}

	result = vf10_tx_run(1, GENERATOR_PORTS, &ctx);
	if (result == DOCA_SUCCESS)
		exit_status = EXIT_SUCCESS;
	else
		DOCA_LOG_ERR("Generator failed: %s", doca_error_get_descr(result));

	dpdk_queues_and_ports_fini(&dpdk_config);
	vf10_tx_release_pool();
dpdk_cleanup:
	dpdk_fini();
argp_cleanup:
	doca_argp_destroy();
out:
	destroy_doca_flow_devs(&ctx.devs_ctx);
	return exit_status;
}
