#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <infiniband/verbs.h>

#include <rte_dev.h>
#include <rte_ethdev.h>

#include <doca_argp.h>
#include <doca_flow.h>
#include <doca_log.h>
#include <doca_rdma_bridge.h>

#include <dpdk_utils.h>
#include <flow_common.h>
#include <flow_switch_common.h>

#include "vf_eswitch.h"

DOCA_LOG_REGISTER(VF_ESWITCH::MAIN);

#define MAX_DEVARGS_LEN 1024
#define NB_DPDK_PF_PORTS 1

struct port_dev_mapping {
	struct doca_dev *dev;
	bool valid;
};

static struct port_dev_mapping port_to_dev[RTE_MAX_ETHPORTS];

static doca_error_t add_port_dev_mapping(uint16_t port_id, struct doca_dev *dev)
{
	if (port_id >= RTE_MAX_ETHPORTS)
		return DOCA_ERROR_INVALID_VALUE;

	port_to_dev[port_id].dev = dev;
	port_to_dev[port_id].valid = true;
	return DOCA_SUCCESS;
}

static int find_port_by_pci_addr(const char *pci_addr, struct doca_dev *dev)
{
	struct rte_eth_dev_info dev_info;
	const struct rte_devargs *devargs;
	uint16_t port_id;

	for (port_id = 0; port_id < RTE_MAX_ETHPORTS; port_id++) {
		if (!rte_eth_dev_is_valid_port(port_id))
			continue;
		if (rte_eth_dev_info_get(port_id, &dev_info) < 0 || dev_info.device == NULL)
			continue;

		devargs = rte_dev_devargs(dev_info.device);
		if (devargs != NULL && strcmp(devargs->name, pci_addr) == 0) {
			if (add_port_dev_mapping(port_id, dev) != DOCA_SUCCESS)
				return -1;
			return port_id;
		}
	}

	return -1;
}

/*
 * DOCA 3.4 switch mode probes only the PF proxy as a DPDK port. VF
 * representors are opened as doca_dev_rep objects and must not be configured
 * with rte_eth_dev_configure()/rte_eth_dev_start().
 */
static doca_error_t probe_pf_proxy(const char *pci_addr, const char *base_devargs, struct doca_dev *dev)
{
	struct ibv_pd *pd = NULL;
	char devargs[MAX_DEVARGS_LEN] = {0};
	doca_error_t result;
	int dup_cmd_fd;
	int port_id;
	int len;

	result = doca_rdma_bridge_get_dev_pd(dev, &pd);
	if (result != DOCA_SUCCESS) {
		DOCA_LOG_ERR("Failed to get PD for %s: %s", pci_addr, doca_error_get_descr(result));
		return result;
	}

	dup_cmd_fd = dup(pd->context->cmd_fd);
	if (dup_cmd_fd < 0)
		return DOCA_ERROR_OPERATING_SYSTEM;

	len = snprintf(devargs,
		       sizeof(devargs),
		       "%s,pd_handle=%u,cmd_fd=%d,%s",
		       pci_addr,
		       pd->handle,
		       dup_cmd_fd,
		       base_devargs);
	if (len < 0 || len >= (int)sizeof(devargs)) {
		close(dup_cmd_fd);
		return DOCA_ERROR_INVALID_VALUE;
	}

	DOCA_LOG_INFO("Probing PF proxy with devargs: %s", devargs);
	if (rte_dev_probe(devargs) != 0) {
		close(dup_cmd_fd);
		return DOCA_ERROR_DRIVER;
	}

	port_id = find_port_by_pci_addr(pci_addr, dev);
	if (port_id < 0) {
		(void)rte_eal_hotplug_remove("pci", pci_addr);
		return DOCA_ERROR_NOT_FOUND;
	}

	DOCA_LOG_INFO("PF proxy is DPDK port %d", port_id);
	return DOCA_SUCCESS;
}

static doca_error_t probe_pf_from_argp(struct flow_switch_ctx *ctx)
{
	struct doca_dev *dev;
	char pci_addr[DOCA_DEVINFO_PCI_ADDR_SIZE] = {0};
	doca_error_t result;

	if (ctx->devs_ctx.nb_devs != 1 || ctx->devs_ctx.devs_manager[0].doca_dev == NULL)
		return DOCA_ERROR_INVALID_VALUE;

	dev = ctx->devs_ctx.devs_manager[0].doca_dev;
	result = doca_devinfo_get_pci_addr_str(doca_dev_as_devinfo(dev), pci_addr);
	if (result != DOCA_SUCCESS)
		return result;

	return probe_pf_proxy(pci_addr, FLOW_SWITCH_DEV_ARGS, dev);
}

static doca_error_t validate_two_vf_topology(const struct flow_switch_ctx *ctx)
{
	if (ctx->devs_ctx.nb_devs != 1) {
		DOCA_LOG_ERR("Expected one PF, received %u device groups", ctx->devs_ctx.nb_devs);
		return DOCA_ERROR_INVALID_VALUE;
	}

	if (ctx->devs_ctx.devs_manager[0].nb_reps != 2) {
		DOCA_LOG_ERR("Expected exactly two VF representors, received %u",
			     ctx->devs_ctx.devs_manager[0].nb_reps);
		return DOCA_ERROR_INVALID_VALUE;
	}

	DOCA_LOG_INFO("VF-A is logical port 1; VF-B is logical port 2");
	return DOCA_SUCCESS;
}

static void remove_probed_pf(void)
{
	struct rte_eth_dev_info dev_info = {0};
	uint16_t port_id;

	for (port_id = 0; port_id < RTE_MAX_ETHPORTS; port_id++) {
		if (port_to_dev[port_id].valid)
			break;
	}
	if (port_id == RTE_MAX_ETHPORTS || !rte_eth_dev_is_valid_port(port_id))
		return;
	if (rte_eth_dev_info_get(port_id, &dev_info) != 0 || dev_info.device == NULL)
		return;

	(void)rte_eth_dev_stop(port_id);
	(void)rte_eth_dev_close(port_id);
	if (rte_dev_remove(dev_info.device) != 0) {
		DOCA_LOG_WARN("Failed to remove PF proxy DPDK port %u", port_id);
		return;
	}

	port_to_dev[port_id].dev = NULL;
	port_to_dev[port_id].valid = false;
}

int main(int argc, char **argv)
{
	struct application_dpdk_config dpdk_config = {
		.port_config.nb_ports = NB_DPDK_PF_PORTS,
		.port_config.nb_queues = 1,
		.port_config.switch_mode = 1,
	};
	struct flow_switch_ctx ctx = {0};
	struct doca_log_backend *sdk_log = NULL;
	doca_error_t result;
	int exit_status = EXIT_FAILURE;

	result = doca_log_backend_create_standard();
	if (result != DOCA_SUCCESS)
		goto exit;

	result = doca_log_backend_create_with_file_sdk(stderr, &sdk_log);
	if (result != DOCA_SUCCESS)
		goto exit;
	result = doca_log_backend_set_sdk_level(sdk_log, DOCA_LOG_LEVEL_WARNING);
	if (result != DOCA_SUCCESS)
		goto exit;

	result = doca_argp_init(NULL, &ctx);
	if (result != DOCA_SUCCESS)
		goto exit;

	result = register_doca_flow_switch_params();
	if (result != DOCA_SUCCESS)
		goto destroy_argp;

	/* EAL starts without auto-probing mlx5; DOCA performs the real probe. */
	doca_argp_set_dpdk_program(flow_init_dpdk);
	ctx.devs_ctx.default_dev_args = FLOW_SWITCH_DEV_ARGS;

	result = doca_argp_start(argc, argv);
	if (result != DOCA_SUCCESS) {
		DOCA_LOG_ERR("Failed to parse arguments: %s", doca_error_get_descr(result));
		goto destroy_argp;
	}

	result = validate_two_vf_topology(&ctx);
	if (result != DOCA_SUCCESS)
		goto finish_dpdk;

	result = probe_pf_from_argp(&ctx);
	if (result != DOCA_SUCCESS) {
		DOCA_LOG_ERR("Failed to probe PF proxy: %s", doca_error_get_descr(result));
		goto finish_dpdk;
	}

	result = dpdk_queues_and_ports_init(&dpdk_config);
	if (result != DOCA_SUCCESS) {
		DOCA_LOG_ERR("Failed to initialize the PF proxy queue: %s", doca_error_get_descr(result));
		goto remove_pf;
	}

	result = vf_eswitch_run(dpdk_config.port_config.nb_queues, &ctx);
	if (result == DOCA_SUCCESS)
		exit_status = EXIT_SUCCESS;
	else
		DOCA_LOG_ERR("VF switch failed: %s", doca_error_get_descr(result));

	/* Match the DOCA 3.4 sample: detach the hot-plugged PF before DPDK queue cleanup. */
	remove_probed_pf();
	dpdk_queues_and_ports_fini(&dpdk_config);
	goto finish_dpdk;
remove_pf:
	remove_probed_pf();
finish_dpdk:
	dpdk_fini();
destroy_argp:
	doca_argp_destroy();
exit:
	destroy_doca_flow_devs(&ctx.devs_ctx);
	return exit_status;
}
