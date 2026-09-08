/*
 * Copyright (c) 2023-2026 NVIDIA CORPORATION AND AFFILIATES.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted
 * provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice, this list of
 *       conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of
 *       conditions and the following disclaimer in the documentation and/or other materials
 *       provided with the distribution.
 *     * Neither the name of the NVIDIA CORPORATION nor the names of its contributors may be used
 *       to endorse or promote products derived from this software without specific prior written
 *       permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL NVIDIA CORPORATION BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TOR (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <stdlib.h>

#include <rte_ethdev.h>

#include <doca_argp.h>
#include <doca_dev.h>
#include <doca_flow.h>
#include <doca_log.h>
#include <doca_ctx.h>

#include <flow_common.h>
#include <flow_switch_common.h>

#include <dpdk_utils.h>

DOCA_LOG_REGISTER(FLOW_SWITCH_TO_WIRE::MAIN);

#define SWITCH_TO_WIRE_PORTS 2

/* Sample's Logic */
doca_error_t flow_switch_to_wire(int nb_queues, int nb_ports, struct flow_switch_ctx *ctx);

/*
 * Sample main function
 *
 * @argc [in]: command line arguments size
 * @argv [in]: array of command line arguments
 * @return: EXIT_SUCCESS on success and EXIT_FAILURE otherwise
 */
int tx_probe_validate_config(void);
void tx_probe_release_pool(void);
doca_error_t tx_probe_validate_ports(struct flow_switch_ctx *ctx);

int main(int argc, char **argv)
{
	if (tx_probe_validate_config() != 0)
		return EXIT_FAILURE;
	doca_error_t result;
	struct doca_log_backend *sdk_log;
	int exit_status = EXIT_FAILURE;
	struct application_dpdk_config dpdk_config = {
		.port_config.nb_ports = SWITCH_TO_WIRE_PORTS,
		.port_config.nb_queues = 1,
		.port_config.switch_mode = 1,
		/* Keep sample queue initialization; generated mbufs have no TX metadata. */
		.port_config.enable_mbuf_metadata = 1,
	};
	struct flow_switch_ctx ctx = {0};
	uint16_t nr_ports;

	/* Register a logger backend */
	result = doca_log_backend_create_standard();
	if (result != DOCA_SUCCESS)
		goto sample_exit;

	/* Register a logger backend for internal SDK errors and warnings */
	result = doca_log_backend_create_with_file_sdk(stderr, &sdk_log);
	if (result != DOCA_SUCCESS)
		goto sample_exit;
	result = doca_log_backend_set_sdk_level(sdk_log, DOCA_LOG_LEVEL_WARNING);
	if (result != DOCA_SUCCESS)
		goto sample_exit;

	result = doca_argp_init(NULL, &ctx);
	if (result != DOCA_SUCCESS) {
		DOCA_LOG_ERR("Failed to init ARGP resources: %s", doca_error_get_descr(result));
		goto sample_exit;
	}
	result = register_doca_flow_switch_params();
	if (result != DOCA_SUCCESS) {
		DOCA_LOG_ERR("Failed to register flow param: %s", doca_error_get_descr(result));
		goto argp_cleanup;
	}
	/*
	 * Enable --no-wire2wire when the test environment has no wire-to-wire
	 * traffic path (e.g. only Wire-to-VF or VF-to-Wire). This skips
	 * the pre-egress loopback and pre-wire direction checks, reducing
	 * latency for ingress-to-egress and forward-to-port forwarding.
	 */
	result = register_flow_device_no_wire_to_wire_params();
	if (result != DOCA_SUCCESS) {
		DOCA_LOG_ERR("Failed to register no wire_to_wire param: %s", doca_error_get_descr(result));
		goto argp_cleanup;
	}

	doca_argp_set_dpdk_program(flow_init_dpdk);
	ctx.devs_ctx.default_dev_args = FLOW_SWITCH_DEV_ARGS;

	result = doca_argp_start(argc, argv);
	if (result != DOCA_SUCCESS) {
		DOCA_LOG_ERR("Failed to parse sample input: %s", doca_error_get_descr(result));
		goto argp_cleanup;
	}

	result = init_doca_flow_devs(&ctx.devs_ctx);
	if (result != DOCA_SUCCESS) {
		DOCA_LOG_ERR("Failed to init flow switch common: %s", doca_error_get_descr(result));
		goto dpdk_cleanup;
	}

	nr_ports = rte_eth_dev_count_avail();
	if (nr_ports != SWITCH_TO_WIRE_PORTS) {
		DOCA_LOG_ERR("Failed to init - lack of ports, probed:%d, needed:%d", nr_ports, SWITCH_TO_WIRE_PORTS);
		goto dpdk_cleanup;
	}

	result = tx_probe_validate_ports(&ctx);
	if (result != DOCA_SUCCESS) {
		DOCA_LOG_ERR("TX probe port validation failed: %s", doca_error_get_descr(result));
		goto dpdk_cleanup;
	}

	/* update queues and ports */
	result = dpdk_queues_and_ports_init(&dpdk_config);
	if (result != DOCA_SUCCESS) {
		DOCA_LOG_ERR("Failed to update ports and queues");
		goto dpdk_cleanup;
	}

	/* run sample */
	result = flow_switch_to_wire(dpdk_config.port_config.nb_queues, SWITCH_TO_WIRE_PORTS, &ctx);
	if (result != DOCA_SUCCESS) {
		DOCA_LOG_ERR("flow_switch_to_wire() encountered an error: %s", doca_error_get_descr(result));
		goto dpdk_ports_queues_cleanup;
	}

	exit_status = EXIT_SUCCESS;

dpdk_ports_queues_cleanup:
	dpdk_queues_and_ports_fini(&dpdk_config);
	tx_probe_release_pool();
dpdk_cleanup:
	dpdk_fini();
argp_cleanup:
	doca_argp_destroy();
sample_exit:
	destroy_doca_flow_devs(&ctx.devs_ctx);
	if (exit_status == EXIT_SUCCESS)
		DOCA_LOG_INFO("TX probe finished: see EGRESS counters; guest delivery requires capture");
	else
		DOCA_LOG_INFO("Sample finished with errors");
	return exit_status;
}
