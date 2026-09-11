#include "sf_to_vf10.h"

#include <arpa/inet.h>
#include <errno.h>
#include <inttypes.h>
#include <net/if.h>
#include <netpacket/packet.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#include <rte_byteorder.h>
#include <rte_ethdev.h>
#include <rte_ether.h>

#include <doca_dev.h>
#include <doca_dpdk.h>
#include <doca_flow.h>

#include <flow_common.h>
#include <flow_switch_common.h>

#define TARGET_HOST 1U
#define TARGET_PF 0U
#define TARGET_VF 10U
#define FRAME_SIZE 60U
#define PROBE_ETHERTYPE 0x88B5U
#define DEFAULT_PACKET_COUNT 10U
#define MAX_PACKET_COUNT 100000U
#define DEFAULT_INTERVAL_MS 500U
#define MAX_INTERVAL_MS 60000U

static char sf_interface[IFNAMSIZ] = "enp3s0f0s0";
static uint8_t destination_mac[RTE_ETHER_ADDR_LEN];
static uint32_t packet_count = DEFAULT_PACKET_COUNT;
static uint32_t interval_ms = DEFAULT_INTERVAL_MS;

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
	return (output[0] & 1U) == 0 ? 0 : -1;
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
	    parsed == 0 || parsed > maximum)
		return -1;
	*value = (uint32_t)parsed;
	return 0;
}

int sf_to_vf10_validate_environment(void)
{
	const char *iface = getenv("SF_TX_IFACE");

	if (iface != NULL && *iface != '\0') {
		if (strlen(iface) >= sizeof(sf_interface)) {
			fprintf(stderr, "CONFIG ERROR: SF_TX_IFACE is too long\n");
			return -1;
		}
		strcpy(sf_interface, iface);
	}
	if (parse_mac(getenv("SF_TX_DEST_MAC"), destination_mac) != 0) {
		fprintf(stderr, "CONFIG ERROR: SF_TX_DEST_MAC must be a unicast MAC\n");
		return -1;
	}
	if (parse_u32("SF_TX_PACKET_COUNT", DEFAULT_PACKET_COUNT,
	              MAX_PACKET_COUNT, &packet_count) != 0 ||
	    parse_u32("SF_TX_INTERVAL_MS", DEFAULT_INTERVAL_MS,
	              MAX_INTERVAL_MS, &interval_ms) != 0) {
		fprintf(stderr, "CONFIG ERROR: invalid packet count or interval\n");
		return -1;
	}
	return 0;
}

doca_error_t sf_to_vf10_validate_ports(struct flow_switch_ctx *ctx,
				       struct sf_vf_port_map *map)
{
	struct doca_dev *parent = NULL;
	unsigned int sf_count = 0, vf_count = 0;
	doca_error_t result;

	if (ctx == NULL || map == NULL || !ctx->is_expert ||
	    ctx->devs_ctx.nb_devs != 1 ||
	    ctx->devs_ctx.devs_manager[0].nb_reps != 2 ||
	    rte_eth_dev_count_avail() != 3) {
		fprintf(stderr, "PORT ERROR: require --expert-mode and exactly one SF + c1pf0vf10\n");
		return DOCA_ERROR_INVALID_VALUE;
	}
	result = doca_dpdk_port_as_dev(0, &parent);
	if (result != DOCA_SUCCESS ||
	    parent != ctx->devs_ctx.devs_manager[0].doca_dev)
		return DOCA_ERROR_BAD_STATE;
	map->parent_port = 0;

	for (uint16_t port_id = 1; port_id < 3; ++port_id) {
		struct doca_dev_rep *rep = NULL;
		struct doca_devinfo_rep *info;
		enum doca_pci_func_type type;
		doca_error_t close_result;

		result = doca_dpdk_open_dev_rep_by_port_id(port_id, parent, &rep);
		if (result != DOCA_SUCCESS)
			return result;
		info = doca_dev_rep_as_devinfo(rep);
		result = doca_devinfo_rep_get_pci_func_type(info, &type);
		if (result == DOCA_SUCCESS && type == DOCA_PCI_FUNC_TYPE_SF) {
			map->sf_port = port_id;
			++sf_count;
			fprintf(stderr, "PORT MAP: SF representor logical_port=%u\n", port_id);
		} else if (result == DOCA_SUCCESS && type == DOCA_PCI_FUNC_TYPE_VF) {
			uint32_t host = 0, pf = 0, vf = 0;

			result = doca_devinfo_rep_get_host_index(info, &host);
			if (result == DOCA_SUCCESS)
				result = doca_devinfo_rep_get_pf_index(info, &pf);
			if (result == DOCA_SUCCESS)
				result = doca_devinfo_rep_get_vf_index(info, &vf);
			fprintf(stderr,
			        "PORT MAP: VF representor logical_port=%u host=%u pf=%u vf=%u\n",
			        port_id, host, pf, vf);
			if (result == DOCA_SUCCESS &&
			    (host != TARGET_HOST || pf != TARGET_PF || vf != TARGET_VF))
				result = DOCA_ERROR_INVALID_VALUE;
			if (result == DOCA_SUCCESS) {
				map->vf_port = port_id;
				++vf_count;
			}
		} else if (result == DOCA_SUCCESS) {
			result = DOCA_ERROR_NOT_SUPPORTED;
		}
		close_result = doca_dev_rep_close(rep);
		if (result == DOCA_SUCCESS)
			result = close_result;
		if (result != DOCA_SUCCESS)
			return result;
	}
	if (sf_count != 1 || vf_count != 1)
		return DOCA_ERROR_INVALID_VALUE;
	fprintf(stderr, "PORT MAP: parent=%u sf=%u vf10=%u\n",
	        map->parent_port, map->sf_port, map->vf_port);
	return DOCA_SUCCESS;
}

static doca_error_t create_sf_to_vf_pipe(struct doca_flow_port *switch_port,
					 uint16_t sf_port, uint16_t vf_port,
					 struct entries_status *status,
					 struct doca_flow_pipe_entry **entry)
{
	struct doca_flow_pipe_cfg *cfg = NULL;
	struct doca_flow_pipe *pipe = NULL;
	struct doca_flow_match match = {0};
	struct doca_flow_monitor monitor = {0};
	struct doca_flow_fwd forward = {0};
	struct doca_flow_fwd miss = {0};
	doca_error_t result;

	match.parser_meta.port_id = sf_port;
	match.outer.eth.type = rte_cpu_to_be_16(PROBE_ETHERTYPE);
	monitor.counter_type = DOCA_FLOW_RESOURCE_TYPE_NON_SHARED;
	forward.type = DOCA_FLOW_FWD_PORT;
	forward.port_id = vf_port;
	miss.type = DOCA_FLOW_FWD_DROP;
	result = doca_flow_pipe_cfg_create(&cfg, switch_port);
	if (result != DOCA_SUCCESS)
		return result;
	result = set_flow_pipe_cfg(cfg, "SF_TO_VF10", DOCA_FLOW_PIPE_BASIC, true);
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
	return doca_flow_pipe_basic_add_entry(0, pipe, &match, 0, NULL,
	                                      &monitor, NULL,
	                                      DOCA_FLOW_ENTRY_FLAGS_NO_WAIT,
	                                      status, entry);
}

static int open_sf_socket(uint8_t source_mac[RTE_ETHER_ADDR_LEN],
			  struct sockaddr_ll *destination)
{
	struct ifreq request = {0};
	int fd = socket(AF_PACKET, SOCK_RAW, htons(PROBE_ETHERTYPE));

	if (fd < 0)
		return -1;
	strncpy(request.ifr_name, sf_interface, IFNAMSIZ - 1);
	if (ioctl(fd, SIOCGIFFLAGS, &request) < 0 ||
	    (request.ifr_flags & IFF_UP) == 0) {
		fprintf(stderr, "SF ERROR: interface %s must exist and be UP\n",
		        sf_interface);
		close(fd);
		errno = ENETDOWN;
		return -1;
	}
	if (ioctl(fd, SIOCGIFHWADDR, &request) < 0) {
		close(fd);
		return -1;
	}
	memcpy(source_mac, request.ifr_hwaddr.sa_data, RTE_ETHER_ADDR_LEN);
	*destination = (struct sockaddr_ll){
		.sll_family = AF_PACKET,
		.sll_protocol = htons(PROBE_ETHERTYPE),
		.sll_ifindex = (int)if_nametoindex(sf_interface),
		.sll_halen = RTE_ETHER_ADDR_LEN,
	};
	memcpy(destination->sll_addr, destination_mac, RTE_ETHER_ADDR_LEN);
	if (destination->sll_ifindex == 0) {
		close(fd);
		errno = ENODEV;
		return -1;
	}
	return fd;
}

static void build_frame(uint8_t frame[FRAME_SIZE],
			const uint8_t source_mac[RTE_ETHER_ADDR_LEN],
			uint64_t sequence)
{
	uint64_t sequence_be = rte_cpu_to_be_64(sequence);

	memset(frame, 0, FRAME_SIZE);
	memcpy(frame, destination_mac, RTE_ETHER_ADDR_LEN);
	memcpy(frame + RTE_ETHER_ADDR_LEN, source_mac, RTE_ETHER_ADDR_LEN);
	frame[12] = (uint8_t)(PROBE_ETHERTYPE >> 8);
	frame[13] = (uint8_t)PROBE_ETHERTYPE;
	memcpy(frame + 14, "BF3-SF-TO-VF10", 14);
	memcpy(frame + 30, &sequence_be, sizeof(sequence_be));
}

doca_error_t sf_to_vf10_run(int nb_queues, int nb_ports,
			    struct flow_switch_ctx *ctx,
			    const struct sf_vf_port_map *map)
{
	struct flow_resources resources = {0};
	uint32_t shared[SHARED_RESOURCE_NUM_VALUES] = {0};
	uint32_t actions[3] = {
		ACTIONS_MEM_SIZE(2), ACTIONS_MEM_SIZE(2), ACTIONS_MEM_SIZE(2),
	};
	struct doca_flow_port *ports[3] = {0};
	struct doca_flow_port *switch_port;
	struct doca_flow_pipe_entry *entry = NULL;
	struct entries_status status = {0};
	struct doca_flow_resource_query query = {0};
	struct sockaddr_ll socket_destination = {0};
	uint8_t source_mac[RTE_ETHER_ADDR_LEN] = {0};
	uint32_t sent = 0;
	doca_error_t result;
	int socket_fd = -1;

	if (nb_queues != 1 || nb_ports != 3 || ctx == NULL || map == NULL)
		return DOCA_ERROR_INVALID_VALUE;
	resources.mode = DOCA_FLOW_RESOURCE_MODE_PORT;
	resources.nr_counters = 4;
	result = init_doca_flow(nb_queues, "switch,hws,hairpinq_num=4,expert",
	                        &resources, shared);
	if (result != DOCA_SUCCESS)
		return result;
	result = init_doca_flow_switch_ports(ctx->devs_ctx.devs_manager,
	                                     ctx->devs_ctx.nb_devs, ports,
	                                     nb_ports, actions, &resources);
	if (result != DOCA_SUCCESS)
		goto destroy_flow;
	switch_port = doca_flow_port_switch_get(ports[map->parent_port]);
	if (switch_port == NULL) {
		result = DOCA_ERROR_BAD_STATE;
		goto stop_ports;
	}
	result = create_sf_to_vf_pipe(switch_port, map->sf_port, map->vf_port,
	                              &status, &entry);
	if (result == DOCA_SUCCESS)
		result = doca_flow_entries_process(switch_port, 0,
		                                   DEFAULT_TIMEOUT_US, 1);
	if (result != DOCA_SUCCESS)
		goto stop_ports;
	if (status.failure || status.nb_processed != 1) {
		result = DOCA_ERROR_BAD_STATE;
		goto stop_ports;
	}
	socket_fd = open_sf_socket(source_mac, &socket_destination);
	if (socket_fd < 0) {
		fprintf(stderr, "SF ERROR: cannot open %s: %s\n",
		        sf_interface, strerror(errno));
		result = DOCA_ERROR_DRIVER;
		goto stop_ports;
	}
	fprintf(stderr,
	        "SF TX READY: iface=%s ifindex=%d "
	        "source=%02x:%02x:%02x:%02x:%02x:%02x "
	        "destination=%02x:%02x:%02x:%02x:%02x:%02x "
	        "sf_port=%u vf10_port=%u ethertype=0x%04x count=%u\n",
	        sf_interface, socket_destination.sll_ifindex,
	        source_mac[0], source_mac[1], source_mac[2], source_mac[3],
	        source_mac[4], source_mac[5],
	        destination_mac[0], destination_mac[1], destination_mac[2],
	        destination_mac[3], destination_mac[4], destination_mac[5],
	        map->sf_port, map->vf_port, PROBE_ETHERTYPE, packet_count);
	for (uint32_t i = 0; i < packet_count; ++i) {
		uint8_t frame[FRAME_SIZE];
		struct timespec pause = {
			.tv_sec = interval_ms / 1000U,
			.tv_nsec = (long)(interval_ms % 1000U) * 1000000L,
		};
		ssize_t written;

		build_frame(frame, source_mac, (uint64_t)i + 1U);
		written = sendto(socket_fd, frame, sizeof(frame), 0,
		                 (const struct sockaddr *)&socket_destination,
		                 sizeof(socket_destination));
		if (written == (ssize_t)sizeof(frame))
			++sent;
		else
			fprintf(stderr, "SF TX ERROR: seq=%u result=%zd errno=%d\n",
			        i + 1U, written, errno);
		fprintf(stdout, "SF TX seq=%u accepted=%d\n",
		        i + 1U, written == (ssize_t)sizeof(frame));
		fflush(stdout);
		nanosleep(&pause, NULL);
	}
	close(socket_fd);
	socket_fd = -1;
	sleep(2);
	result = doca_flow_resource_query_entry(entry, &query);
	if (result != DOCA_SUCCESS)
		goto stop_ports;
	fprintf(stderr,
	        "SF->VF10 SUMMARY: socket_sent=%u flow_hits=%" PRIu64
	        " flow_bytes=%" PRIu64 " guest_delivery=VERIFY_WITH_TCPDUMP\n",
	        sent, query.counter.total_pkts, query.counter.total_bytes);
	if (sent != packet_count || query.counter.total_pkts != sent) {
		fprintf(stderr,
		        "SF->VF10 CHECK FAILED: expected sent=%u and equal Flow hits\n",
		        packet_count);
		result = DOCA_ERROR_BAD_STATE;
	}

stop_ports:
	if (socket_fd >= 0)
		close(socket_fd);
	{
		doca_error_t stop_result = stop_doca_flow_ports(nb_ports, ports);
		if (result == DOCA_SUCCESS)
			result = stop_result;
	}
destroy_flow:
	doca_flow_destroy();
	return result;
}
