#include "vf10_rx_monitor.h"

#include <errno.h>
#include <inttypes.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <rte_byteorder.h>
#include <rte_common.h>
#include <rte_ethdev.h>
#include <rte_ether.h>
#include <rte_mbuf.h>

#include <doca_dev.h>
#include <doca_dpdk.h>
#include <doca_flow.h>

#include <flow_common.h>
#include <flow_switch_common.h>

#define TARGET_HOST 1U
#define TARGET_PF 0U
#define TARGET_VF 10U
#define PARENT_PORT_ID 0U
#define REPRESENTOR_PORT_ID 1U
#define RX_QUEUE_ID 0U
#define RX_BURST_SIZE 32U
#define DEFAULT_SECONDS 30U
#define MAX_SECONDS 3600U
#define DEFAULT_MAX_PACKETS 1000U

static volatile sig_atomic_t interrupted;
static uint32_t monitor_seconds = DEFAULT_SECONDS;
static uint32_t max_packets = DEFAULT_MAX_PACKETS;

static void stop_monitor(int signal_number)
{
	(void)signal_number;
	interrupted = 1;
}

static int read_u32_environment(const char *name, uint32_t default_value,
				uint32_t maximum, uint32_t *value)
{
	const char *text = getenv(name);
	char *end = NULL;
	unsigned long parsed;

	if (text == NULL || *text == '\0') {
		*value = default_value;
		return 0;
	}
	errno = 0;
	parsed = strtoul(text, &end, 10);
	if (errno != 0 || *text == '-' || end == text || *end != '\0' ||
	    parsed == 0 || parsed > maximum) {
		fprintf(stderr, "CONFIG ERROR: %s must be in range 1..%u\n", name, maximum);
		return -1;
	}
	*value = (uint32_t)parsed;
	return 0;
}

int vf10_monitor_validate_environment(void)
{
	if (read_u32_environment("VF10_MONITOR_SECONDS", DEFAULT_SECONDS,
	                         MAX_SECONDS, &monitor_seconds) != 0)
		return -1;
	if (read_u32_environment("VF10_MONITOR_MAX_PACKETS", DEFAULT_MAX_PACKETS,
	                         UINT32_MAX, &max_packets) != 0)
		return -1;
	return 0;
}

doca_error_t vf10_monitor_validate_ports(struct flow_switch_ctx *ctx)
{
	struct doca_dev *parent = NULL;
	struct doca_dev_rep *representor = NULL;
	struct doca_devinfo_rep *info;
	enum doca_pci_func_type type;
	uint32_t host = 0, pf = 0, vf = 0;
	doca_error_t result;
	doca_error_t close_result;

	if (!ctx->is_expert || ctx->devs_ctx.nb_devs != 1 ||
	    ctx->devs_ctx.devs_manager[0].nb_reps != 1 ||
	    rte_eth_dev_count_avail() != 2) {
		fprintf(stderr, "PORT ERROR: require --expert-mode and exactly one parent + one representor\n");
		return DOCA_ERROR_INVALID_VALUE;
	}

	result = doca_dpdk_port_as_dev(PARENT_PORT_ID, &parent);
	if (result != DOCA_SUCCESS || parent != ctx->devs_ctx.devs_manager[0].doca_dev) {
		fprintf(stderr, "PORT ERROR: DPDK port 0 is not the selected parent DOCA device\n");
		return DOCA_ERROR_BAD_STATE;
	}
	result = doca_dpdk_open_dev_rep_by_port_id(REPRESENTOR_PORT_ID, parent,
	                                           &representor);
	if (result != DOCA_SUCCESS)
		return result;
	info = doca_dev_rep_as_devinfo(representor);
	result = doca_devinfo_rep_get_pci_func_type(info, &type);
	if (result == DOCA_SUCCESS && type != DOCA_PCI_FUNC_TYPE_VF)
		result = DOCA_ERROR_INVALID_VALUE;
	if (result == DOCA_SUCCESS)
		result = doca_devinfo_rep_get_host_index(info, &host);
	if (result == DOCA_SUCCESS)
		result = doca_devinfo_rep_get_pf_index(info, &pf);
	if (result == DOCA_SUCCESS)
		result = doca_devinfo_rep_get_vf_index(info, &vf);

	fprintf(stderr, "PORT MAP: parent=0 representor=1 host=%u pf=%u vf=%u expected=1/0/10\n",
	        host, pf, vf);
	if (result == DOCA_SUCCESS &&
	    (host != TARGET_HOST || pf != TARGET_PF || vf != TARGET_VF))
		result = DOCA_ERROR_INVALID_VALUE;

	close_result = doca_dev_rep_close(representor);
	return result == DOCA_SUCCESS ? close_result : result;
}

static void print_packet(const struct rte_mbuf *packet, uint64_t sequence)
{
	uint8_t scratch[64];
	uint32_t captured = RTE_MIN((uint32_t)sizeof(scratch), packet->pkt_len);
	const uint8_t *bytes = rte_pktmbuf_read(packet, 0, captured, scratch);

	if (bytes == NULL || captured < sizeof(struct rte_ether_hdr)) {
		fprintf(stdout, "PACKET seq=%" PRIu64 " len=%u segments=%u truncated-header\n",
		        sequence, packet->pkt_len, packet->nb_segs);
		return;
	}
	const struct rte_ether_hdr *ether = (const struct rte_ether_hdr *)bytes;
	fprintf(stdout,
	        "PACKET seq=%" PRIu64 " len=%u segments=%u mbuf_port=%u "
	        "src=" RTE_ETHER_ADDR_PRT_FMT " dst=" RTE_ETHER_ADDR_PRT_FMT " ethertype=0x%04x flags=0x%016" PRIx64 "\n",
	        sequence, packet->pkt_len, packet->nb_segs, packet->port,
	        RTE_ETHER_ADDR_BYTES(&ether->src_addr),
	        RTE_ETHER_ADDR_BYTES(&ether->dst_addr),
	        rte_be_to_cpu_16(ether->ether_type), (uint64_t)packet->ol_flags);
	fflush(stdout);
}

static doca_error_t create_vf10_rss_pipe(struct doca_flow_port *switch_port,
					struct entries_status *status,
					struct doca_flow_pipe_entry **entry)
{
	struct doca_flow_pipe_cfg *cfg = NULL;
	struct doca_flow_pipe *pipe = NULL;
	struct doca_flow_match match = {0};
	struct doca_flow_monitor monitor = {0};
	struct doca_flow_fwd forward = {0};
	struct doca_flow_fwd miss = {0};
	uint16_t rss_queue = RX_QUEUE_ID;
	doca_error_t result;

	/* Port 1 was reverse-mapped and validated as c1pf0vf10 above. */
	match.parser_meta.port_id = REPRESENTOR_PORT_ID;
	monitor.counter_type = DOCA_FLOW_RESOURCE_TYPE_NON_SHARED;
	forward.type = DOCA_FLOW_FWD_RSS;
	forward.rss_type = DOCA_FLOW_RESOURCE_TYPE_NON_SHARED;
	forward.rss.queues_array = &rss_queue;
	forward.rss.nr_queues = 1;
	forward.rss.inner_flags = DOCA_FLOW_RSS_AUTO;
	miss.type = DOCA_FLOW_FWD_DROP;

	result = doca_flow_pipe_cfg_create(&cfg, switch_port);
	if (result != DOCA_SUCCESS)
		return result;
	result = set_flow_pipe_cfg(cfg, "VF10_RX_MONITOR", DOCA_FLOW_PIPE_BASIC, true);
	if (result == DOCA_SUCCESS)
		result = doca_flow_pipe_cfg_set_nr_entries(cfg, 1);
	if (result == DOCA_SUCCESS)
		result = doca_flow_pipe_cfg_set_match(cfg, &match, NULL);
	if (result == DOCA_SUCCESS)
		result = doca_flow_pipe_cfg_set_monitor(cfg, &monitor);
	if (result == DOCA_SUCCESS)
		result = doca_flow_pipe_create(cfg, &forward, &miss, &pipe);
	doca_flow_pipe_cfg_destroy(cfg);
	if (result != DOCA_SUCCESS)
		return result;

	return doca_flow_pipe_basic_add_entry(0, pipe, &match, 0, NULL, &monitor,
	                                      NULL, DOCA_FLOW_ENTRY_FLAGS_NO_WAIT,
	                                      status, entry);
}

static uint64_t monotonic_seconds(void)
{
	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);
	return (uint64_t)now.tv_sec;
}

doca_error_t vf10_monitor_run(int nb_queues, int nb_ports,
			      struct flow_switch_ctx *ctx)
{
	struct flow_resources resources = {0};
	uint32_t shared[SHARED_RESOURCE_NUM_VALUES] = {0};
	uint32_t actions[2] = {ACTIONS_MEM_SIZE(2), ACTIONS_MEM_SIZE(2)};
	struct doca_flow_port *ports[2] = {0};
	struct doca_flow_port *switch_port;
	struct doca_flow_pipe_entry *entry = NULL;
	struct entries_status status = {0};
	struct doca_flow_resource_query query = {0};
	struct rte_eth_stats stats_before = {0}, stats_after = {0};
	uint64_t received = 0;
	uint64_t deadline;
	doca_error_t result;

	if (nb_queues != 1 || nb_ports != 2)
		return DOCA_ERROR_INVALID_VALUE;

	resources.mode = DOCA_FLOW_RESOURCE_MODE_PORT;
	resources.nr_counters = 4;
	resources.nr_rss = 1;
	result = init_doca_flow(nb_queues, "switch,hws,hairpinq_num=4,expert",
	                        &resources, shared);
	if (result != DOCA_SUCCESS)
		return result;
	result = init_doca_flow_switch_ports(ctx->devs_ctx.devs_manager,
	                                     ctx->devs_ctx.nb_devs, ports,
	                                     nb_ports, actions, &resources);
	if (result != DOCA_SUCCESS)
		goto destroy_flow;
	switch_port = doca_flow_port_switch_get(ports[PARENT_PORT_ID]);
	if (switch_port == NULL) {
		result = DOCA_ERROR_BAD_STATE;
		goto stop_ports;
	}

	result = create_vf10_rss_pipe(switch_port, &status, &entry);
	if (result == DOCA_SUCCESS)
		result = doca_flow_entries_process(switch_port, 0,
		                                   DEFAULT_TIMEOUT_US, 1);
	if (result != DOCA_SUCCESS)
		goto stop_ports;
	if (status.failure || status.nb_processed != 1) {
		fprintf(stderr, "ENTRY ERROR: processed=%d expected=1 failure=%d\n",
		        status.nb_processed, status.failure);
		result = DOCA_ERROR_BAD_STATE;
		goto stop_ports;
	}

	if (rte_eth_stats_get(PARENT_PORT_ID, &stats_before) != 0) {
		result = DOCA_ERROR_BAD_STATE;
		goto stop_ports;
	}
	signal(SIGINT, stop_monitor);
	signal(SIGTERM, stop_monitor);
	deadline = monotonic_seconds() + monitor_seconds;
	fprintf(stderr,
	        "MONITOR READY: source=host1/pf0/vf10 logical_port=1 parent_rx=0/0 "
	        "seconds=%u max_packets=%u\n",
	        monitor_seconds, max_packets);

	while (!interrupted && monotonic_seconds() < deadline && received < max_packets) {
		struct rte_mbuf *packets[RX_BURST_SIZE];
		uint16_t count = rte_eth_rx_burst(PARENT_PORT_ID, RX_QUEUE_ID,
		                                  packets, RX_BURST_SIZE);
		if (count == 0) {
			struct timespec pause = {.tv_nsec = 1000000};
			nanosleep(&pause, NULL);
			continue;
		}
		for (uint16_t i = 0; i < count; ++i) {
			++received;
			print_packet(packets[i], received);
			rte_pktmbuf_free(packets[i]);
		}
	}

	result = doca_flow_resource_query_entry(entry, &query);
	if (result != DOCA_SUCCESS)
		goto stop_ports;
	if (rte_eth_stats_get(PARENT_PORT_ID, &stats_after) != 0) {
		result = DOCA_ERROR_BAD_STATE;
		goto stop_ports;
	}
	fprintf(stderr,
	        "MONITOR SUMMARY: flow_selected=%" PRIu64 " software_received=%" PRIu64
	        " parent_ipackets_delta=%" PRIu64 " parent_imissed_delta=%" PRIu64
	        " interrupted=%d\n",
	        query.counter.total_pkts, received,
	        stats_after.ipackets - stats_before.ipackets,
	        stats_after.imissed - stats_before.imissed, interrupted != 0);

	if (received == 0 || query.counter.total_pkts == 0) {
		fprintf(stderr, "MONITOR CHECK FAILED: no VF10 packet traversed both Flow and software RX\n");
		result = DOCA_ERROR_BAD_STATE;
	}

stop_ports: {
	doca_error_t stop_result = stop_doca_flow_ports(nb_ports, ports);
	if (result == DOCA_SUCCESS)
		result = stop_result;
}
destroy_flow:
	doca_flow_destroy();
	return result;
}
