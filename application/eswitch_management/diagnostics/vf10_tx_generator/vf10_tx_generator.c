#include "vf10_tx_generator.h"

#include <errno.h>
#include <inttypes.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <rte_byteorder.h>
#include <rte_ethdev.h>
#include <rte_ether.h>
#include <rte_errno.h>
#include <rte_flow.h>
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
#define TX_QUEUE_ID 0U
#define FRAME_SIZE 60U
#define PROBE_ETHERTYPE 0x88B5U
#define DEFAULT_INTERVAL_MS 1000U
#define MAX_INTERVAL_MS 60000U

static struct rte_mempool *packet_pool;
static volatile sig_atomic_t interrupted;
static uint8_t destination_mac[RTE_ETHER_ADDR_LEN];
static uint8_t source_mac[RTE_ETHER_ADDR_LEN];
static uint32_t interval_ms = DEFAULT_INTERVAL_MS;
static uint64_t packet_limit;

static void stop_generator(int signal_number)
{
	(void)signal_number;
	interrupted = 1;
}

static int parse_mac(const char *text, uint8_t output[RTE_ETHER_ADDR_LEN])
{
	unsigned int bytes[RTE_ETHER_ADDR_LEN];
	int consumed = 0;

	if (text == NULL || strlen(text) != 17 ||
	    sscanf(text, "%2x:%2x:%2x:%2x:%2x:%2x%n",
	           &bytes[0], &bytes[1], &bytes[2], &bytes[3], &bytes[4],
	           &bytes[5], &consumed) != RTE_ETHER_ADDR_LEN || consumed != 17)
		return -1;
	for (unsigned int i = 0; i < RTE_ETHER_ADDR_LEN; ++i)
		output[i] = (uint8_t)bytes[i];
	if ((output[0] & 1U) != 0)
		return -1;
	return 0;
}

static int parse_u32(const char *name, uint32_t default_value,
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

static int parse_u64_limit(const char *name, uint64_t *value)
{
	const char *text = getenv(name);
	char *end = NULL;
	unsigned long long parsed;

	if (text == NULL || *text == '\0') {
		*value = 0; /* Unlimited until SIGINT/SIGTERM. */
		return 0;
	}
	errno = 0;
	parsed = strtoull(text, &end, 10);
	if (errno != 0 || *text == '-' || end == text || *end != '\0') {
		fprintf(stderr, "CONFIG ERROR: %s must be zero or a positive integer\n", name);
		return -1;
	}
	*value = (uint64_t)parsed;
	return 0;
}

int vf10_tx_validate_environment(void)
{
	if (parse_mac(getenv("VF10_TX_DEST_MAC"), destination_mac) != 0 ||
	    parse_mac(getenv("VF10_TX_SOURCE_MAC"), source_mac) != 0) {
		fprintf(stderr, "CONFIG ERROR: VF10_TX_DEST_MAC and VF10_TX_SOURCE_MAC must be unicast MAC addresses\n");
		return -1;
	}
	if (parse_u32("VF10_TX_INTERVAL_MS", DEFAULT_INTERVAL_MS,
	              MAX_INTERVAL_MS, &interval_ms) != 0)
		return -1;
	return parse_u64_limit("VF10_TX_PACKET_LIMIT", &packet_limit);
}

doca_error_t vf10_tx_validate_ports(struct flow_switch_ctx *ctx)
{
	struct doca_dev *parent = NULL;
	struct doca_dev_rep *representor = NULL;
	struct doca_devinfo_rep *info;
	enum doca_pci_func_type type;
	uint32_t host = 0, pf = 0, vf = 0;
	doca_error_t result;
	doca_error_t close_result;

	/* Non-expert switch mode is mandatory for direct destination metadata. */
	if (ctx->is_expert || ctx->devs_ctx.nb_devs != 1 ||
	    ctx->devs_ctx.devs_manager[0].nb_reps != 1 ||
	    rte_eth_dev_count_avail() != 2) {
		fprintf(stderr, "PORT ERROR: omit --expert-mode and open exactly one parent + one representor\n");
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

	fprintf(stderr, "PORT MAP: parent=0 destination=1 host=%u pf=%u vf=%u expected=1/0/10\n",
	        host, pf, vf);
	if (result == DOCA_SUCCESS &&
	    (host != TARGET_HOST || pf != TARGET_PF || vf != TARGET_VF))
		result = DOCA_ERROR_INVALID_VALUE;
	close_result = doca_dev_rep_close(representor);
	return result == DOCA_SUCCESS ? close_result : result;
}

static void build_frame(struct rte_mbuf *packet, uint64_t sequence)
{
	uint8_t *frame = (uint8_t *)rte_pktmbuf_append(packet, FRAME_SIZE);
	uint64_t sequence_be = rte_cpu_to_be_64(sequence);
	struct timespec now;
	uint64_t timestamp_be;

	memset(frame, 0, FRAME_SIZE);
	memcpy(frame, destination_mac, RTE_ETHER_ADDR_LEN);
	memcpy(frame + RTE_ETHER_ADDR_LEN, source_mac, RTE_ETHER_ADDR_LEN);
	frame[12] = (uint8_t)(PROBE_ETHERTYPE >> 8);
	frame[13] = (uint8_t)PROBE_ETHERTYPE;
	memcpy(frame + 14, "BF3-VF10-TX", 11);
	memcpy(frame + 26, &sequence_be, sizeof(sequence_be));
	clock_gettime(CLOCK_MONOTONIC, &now);
	timestamp_be = rte_cpu_to_be_64((uint64_t)now.tv_sec * 1000000000ULL +
	                                (uint64_t)now.tv_nsec);
	memcpy(frame + 34, &timestamp_be, sizeof(timestamp_be));
}

static void sleep_interval(void)
{
	struct timespec pause = {
		.tv_sec = interval_ms / 1000U,
		.tv_nsec = (long)(interval_ms % 1000U) * 1000000L,
	};
	while (!interrupted && nanosleep(&pause, &pause) != 0 && errno == EINTR)
		;
}

void vf10_tx_release_pool(void)
{
	if (packet_pool != NULL)
		rte_mempool_free(packet_pool);
	packet_pool = NULL;
}

doca_error_t vf10_tx_run(int nb_queues, int nb_ports,
			 struct flow_switch_ctx *ctx)
{
	struct flow_resources resources = {0};
	uint32_t shared[SHARED_RESOURCE_NUM_VALUES] = {0};
	uint32_t actions[2] = {ACTIONS_MEM_SIZE(1), ACTIONS_MEM_SIZE(1)};
	struct doca_flow_port *ports[2] = {0};
	struct rte_eth_stats before = {0}, after = {0};
	uint64_t attempted = 0, accepted = 0;
	doca_error_t result;

	if (nb_queues != 1 || nb_ports != 2)
		return DOCA_ERROR_INVALID_VALUE;

	resources.mode = DOCA_FLOW_RESOURCE_MODE_PORT;
	result = init_doca_flow(nb_queues, "switch,hws,hairpinq_num=4",
	                        &resources, shared);
	if (result != DOCA_SUCCESS)
		return result;
	result = init_doca_flow_switch_ports(ctx->devs_ctx.devs_manager,
	                                     ctx->devs_ctx.nb_devs, ports,
	                                     nb_ports, actions, &resources);
	if (result != DOCA_SUCCESS)
		goto destroy_flow;

	packet_pool = rte_pktmbuf_pool_create("vf10_tx_generator", 255, 0, 0,
	                                     RTE_MBUF_DEFAULT_BUF_SIZE,
	                                     rte_socket_id());
	if (packet_pool == NULL) {
		fprintf(stderr, "POOL ERROR: %s\n", rte_strerror(rte_errno));
		result = DOCA_ERROR_NO_MEMORY;
		goto stop_ports;
	}
	if (rte_eth_stats_get(PARENT_PORT_ID, &before) != 0) {
		result = DOCA_ERROR_BAD_STATE;
		goto stop_ports;
	}

	signal(SIGINT, stop_generator);
	signal(SIGTERM, stop_generator);
	fprintf(stderr,
	        "GENERATOR READY: path=non-expert-tx-metadata parent_tx=0/0 "
	        "destination_port=1 vf=10 ethertype=0x%04x interval_ms=%u limit=%" PRIu64 "\n",
	        PROBE_ETHERTYPE, interval_ms, packet_limit);

	while (!interrupted && (packet_limit == 0 || attempted < packet_limit)) {
		struct rte_mbuf *packet = rte_pktmbuf_alloc(packet_pool);
		uint16_t sent;

		if (packet == NULL) {
			result = DOCA_ERROR_NO_MEMORY;
			goto stop_ports;
		}
		if (rte_pktmbuf_tailroom(packet) < FRAME_SIZE) {
			rte_pktmbuf_free(packet);
			result = DOCA_ERROR_NO_MEMORY;
			goto stop_ports;
		}
		build_frame(packet, attempted + 1);
		packet->port = PARENT_PORT_ID;
		rte_flow_dynf_metadata_set(packet, REPRESENTOR_PORT_ID);
		packet->ol_flags |= RTE_MBUF_DYNFLAG_TX_METADATA;
		++attempted;
		sent = rte_eth_tx_burst(PARENT_PORT_ID, TX_QUEUE_ID, &packet, 1);
		if (sent == 0)
			rte_pktmbuf_free(packet);
		else
			++accepted;
		fprintf(stdout, "TX seq=%" PRIu64 " accepted=%u destination_port=1\n",
		        attempted, sent);
		fflush(stdout);
		sleep_interval();
	}

	/* Allow accepted descriptors to drain before reading port statistics. */
	for (unsigned int i = 0; i < 3 && !interrupted; ++i)
		sleep_interval();
	if (rte_eth_stats_get(PARENT_PORT_ID, &after) != 0) {
		result = DOCA_ERROR_BAD_STATE;
		goto stop_ports;
	}
	fprintf(stderr,
	        "GENERATOR SUMMARY: attempted=%" PRIu64 " accepted=%" PRIu64
	        " parent_opackets_delta=%" PRIu64 " parent_oerrors_delta=%" PRIu64
	        " interrupted=%d guest_delivery=VERIFY_WITH_TCPDUMP\n",
	        attempted, accepted, after.opackets - before.opackets,
	        after.oerrors - before.oerrors, interrupted != 0);
	result = accepted == 0 ? DOCA_ERROR_BAD_STATE : DOCA_SUCCESS;

stop_ports: {
	doca_error_t stop_result = stop_doca_flow_ports(nb_ports, ports);
	if (result == DOCA_SUCCESS)
		result = stop_result;
}
destroy_flow:
	doca_flow_destroy();
	return result;
}
