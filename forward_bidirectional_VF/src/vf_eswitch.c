#include <signal.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include <doca_flow.h>
#include <doca_log.h>

#include <flow_common.h>
#include <flow_switch_common.h>

#include "vf_eswitch.h"

DOCA_LOG_REGISTER(VF_ESWITCH::FLOW);

#define PIPE_QUEUE 0
#define STATS_INTERVAL_SECONDS 1

static volatile sig_atomic_t force_quit;

static void request_shutdown(int signo)
{
	(void)signo;
	force_quit = 1;
}

/*
 * Create one root BASIC pipe on the switch manager port.
 *
 * parser_meta.port_id = UINT16_MAX means that the ingress logical port is
 * changeable per entry. fwd.port_id = UINT16_MAX does the same for egress.
 */
static doca_error_t create_vm_switching_pipe(struct doca_flow_pipe **pipe)
{
	struct doca_flow_port *switch_port = doca_flow_port_switch_get(NULL);
	struct doca_flow_pipe_cfg *pipe_cfg = NULL;
	struct doca_flow_match match = {0};
	struct doca_flow_monitor monitor = {0};
	struct doca_flow_fwd fwd = {0};
	struct doca_flow_fwd fwd_miss = {0};
	doca_error_t result;

	if (switch_port == NULL) {
		DOCA_LOG_ERR("Switch manager port is not available");
		return DOCA_ERROR_BAD_STATE;
	}

	match.parser_meta.port_id = UINT16_MAX;
	monitor.counter_type = DOCA_FLOW_RESOURCE_TYPE_NON_SHARED;

	fwd.type = DOCA_FLOW_FWD_PORT;
	fwd.port_id = UINT16_MAX;

	/* This application intentionally isolates the selected VF pair. */
	fwd_miss.type = DOCA_FLOW_FWD_DROP;

	result = doca_flow_pipe_cfg_create(&pipe_cfg, switch_port);
	if (result != DOCA_SUCCESS) {
		DOCA_LOG_ERR("Failed to create pipe cfg: %s", doca_error_get_descr(result));
		return result;
	}

	result = set_flow_pipe_cfg(pipe_cfg, "vm_switching", DOCA_FLOW_PIPE_BASIC, true);
	if (result != DOCA_SUCCESS)
		goto destroy_cfg;

	result = doca_flow_pipe_cfg_set_nr_entries(pipe_cfg, VF_ESWITCH_ENTRY_COUNT);
	if (result != DOCA_SUCCESS) {
		DOCA_LOG_ERR("Failed to set pipe entry capacity: %s", doca_error_get_descr(result));
		goto destroy_cfg;
	}

	result = doca_flow_pipe_cfg_set_match(pipe_cfg, &match, NULL);
	if (result != DOCA_SUCCESS) {
		DOCA_LOG_ERR("Failed to set ingress-port match: %s", doca_error_get_descr(result));
		goto destroy_cfg;
	}

	result = doca_flow_pipe_cfg_set_monitor(pipe_cfg, &monitor);
	if (result != DOCA_SUCCESS) {
		DOCA_LOG_ERR("Failed to set counter template: %s", doca_error_get_descr(result));
		goto destroy_cfg;
	}

	/* Pipe creation is the constructor-time validation boundary. */
	result = doca_flow_pipe_create(pipe_cfg, &fwd, &fwd_miss, pipe);
	if (result != DOCA_SUCCESS)
		DOCA_LOG_ERR("Failed to create vm_switching pipe: %s", doca_error_get_descr(result));

destroy_cfg:
	doca_flow_pipe_cfg_destroy(pipe_cfg);
	return result;
}

static doca_error_t add_forward_entry(struct doca_flow_pipe *pipe,
				      uint16_t ingress_port,
				      uint16_t egress_port,
				      uint32_t flags,
				      struct entries_status *status,
				      struct doca_flow_pipe_entry **entry)
{
	struct doca_flow_match match = {0};
	struct doca_flow_fwd fwd = {0};
	doca_error_t result;

	match.parser_meta.port_id = ingress_port;
	fwd.type = DOCA_FLOW_FWD_PORT;
	fwd.port_id = egress_port;

	result = doca_flow_pipe_basic_add_entry(PIPE_QUEUE,
						pipe,
						&match,
						0,
						NULL,
						NULL,
						&fwd,
						flags,
						status,
						entry);
	if (result != DOCA_SUCCESS)
		DOCA_LOG_ERR("Failed to add port %u -> port %u entry: %s",
			     ingress_port,
			     egress_port,
			     doca_error_get_descr(result));

	return result;
}

static doca_error_t add_bidirectional_entries(struct doca_flow_pipe *pipe,
					      struct entries_status *status,
					      struct doca_flow_pipe_entry *entries[])
{
	doca_error_t result;

	/* First -r argument: logical port 1. Second -r argument: logical port 2. */
	result = add_forward_entry(pipe,
				   1,
				   2,
				   DOCA_FLOW_ENTRY_FLAGS_WAIT_FOR_BATCH,
				   status,
				   &entries[0]);
	if (result != DOCA_SUCCESS)
		return result;

	return add_forward_entry(pipe,
				 2,
				 1,
				 DOCA_FLOW_ENTRY_FLAGS_NO_WAIT,
				 status,
				 &entries[1]);
}

static void print_entry_counter(const char *direction, struct doca_flow_pipe_entry *entry)
{
	struct doca_flow_resource_query query = {0};
	doca_error_t result;

	result = doca_flow_resource_query_entry(entry, &query);
	if (result != DOCA_SUCCESS) {
		DOCA_LOG_WARN("Failed to query %s counter: %s", direction, doca_error_get_descr(result));
		return;
	}

	DOCA_LOG_INFO("%s packets=%llu bytes=%llu",
		      direction,
		      (unsigned long long)query.counter.total_pkts,
		      (unsigned long long)query.counter.total_bytes);
}

static void run_until_stopped(struct doca_flow_pipe_entry *entries[])
{
	DOCA_LOG_INFO("Datapath is armed. Press Ctrl-C or send SIGTERM to stop");

	while (!force_quit) {
		sleep(STATS_INTERVAL_SECONDS);
		print_entry_counter("VF-A -> VF-B", entries[0]);
		print_entry_counter("VF-B -> VF-A", entries[1]);
	}
}

doca_error_t vf_eswitch_run(int nb_queues, struct flow_switch_ctx *ctx)
{
	struct flow_resources resources = {0};
	uint32_t nr_shared_resources[SHARED_RESOURCE_NUM_VALUES] = {0};
	struct doca_flow_port *ports[VF_ESWITCH_FLOW_PORT_COUNT] = {0};
	uint32_t actions_mem_size[VF_ESWITCH_FLOW_PORT_COUNT];
	struct doca_flow_pipe_entry *entries[VF_ESWITCH_ENTRY_COUNT] = {0};
	struct doca_flow_pipe *pipe = NULL;
	struct entries_status status = {0};
	doca_error_t result;
	doca_error_t stop_result;

	force_quit = 0;
	signal(SIGINT, request_shutdown);
	signal(SIGTERM, request_shutdown);

	/* Two NON_SHARED counters: one for each direction. */
	resources.mode = DOCA_FLOW_RESOURCE_MODE_PORT;
	resources.nr_counters = VF_ESWITCH_ENTRY_COUNT;

	result = init_doca_flow(nb_queues, "switch,hws", &resources, nr_shared_resources);
	if (result != DOCA_SUCCESS) {
		DOCA_LOG_ERR("Failed to initialize DOCA Flow: %s", doca_error_get_descr(result));
		return result;
	}

	ARRAY_INIT(actions_mem_size, ACTIONS_MEM_SIZE(VF_ESWITCH_ENTRY_COUNT));
	result = init_doca_flow_switch_ports(ctx->devs_ctx.devs_manager,
					     ctx->devs_ctx.nb_devs,
					     ports,
					     VF_ESWITCH_FLOW_PORT_COUNT,
					     actions_mem_size,
					     &resources);
	if (result != DOCA_SUCCESS) {
		DOCA_LOG_ERR("Failed to start switch ports: %s", doca_error_get_descr(result));
		doca_flow_destroy();
		return result;
	}

	result = create_vm_switching_pipe(&pipe);
	if (result != DOCA_SUCCESS)
		goto stop_ports;

	result = add_bidirectional_entries(pipe, &status, entries);
	if (result != DOCA_SUCCESS)
		goto destroy_pipe;

	result = flow_process_entries(doca_flow_port_switch_get(NULL), &status, VF_ESWITCH_ENTRY_COUNT);
	if (result != DOCA_SUCCESS)
		goto destroy_pipe;

	DOCA_LOG_INFO("Installed rule: logical port 1 -> logical port 2");
	DOCA_LOG_INFO("Installed rule: logical port 2 -> logical port 1");
	run_until_stopped(entries);

destroy_pipe:
	if (pipe != NULL)
		doca_flow_pipe_destroy(pipe);
stop_ports:
	stop_result = stop_doca_flow_ports(VF_ESWITCH_FLOW_PORT_COUNT, ports);
	if (result == DOCA_SUCCESS && stop_result != DOCA_SUCCESS)
		result = stop_result;
	doca_flow_destroy();
	return result;
}
