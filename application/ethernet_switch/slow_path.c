#include "slow_path.h"

#include <inttypes.h>
#include <stdio.h>
#include <time.h>

#include <doca_bitfield.h>
#include <rte_byteorder.h>
#include <rte_ethdev.h>
#include <rte_ether.h>
#include <rte_flow.h>
#include <rte_mbuf.h>
#include <rte_pause.h>

#include "switch_config.h"

static uint64_t monotonic_time_ns(void) {
  struct timespec value;

  if (clock_gettime(CLOCK_MONOTONIC, &value) != 0)
    return 0;
  return (uint64_t)value.tv_sec * 1000000000ULL +
         (uint64_t)value.tv_nsec;
}

static const struct switch_flow_port *find_ingress_port(
    const struct switch_flow_ports *ports,
    uint16_t port_id) {
  for (uint16_t i = 0; i < ports->count; i++) {
    if (ports->items[i].ethernet->port_id == port_id)
      return &ports->items[i];
  }
  return NULL;
}

static doca_error_t read_ingress_port_id(
    const struct rte_mbuf *packet,
    const struct switch_flow_ports *ports,
    uint16_t *port_id) {
  uint32_t metadata;

  if ((packet->ol_flags & RTE_MBUF_DYNFLAG_RX_METADATA) == 0)
    return DOCA_ERROR_NOT_FOUND;

  metadata = DOCA_BETOH32(*RTE_FLOW_DYNF_METADATA(packet));
  if (metadata > UINT16_MAX ||
      find_ingress_port(ports, (uint16_t)metadata) == NULL)
    return DOCA_ERROR_INVALID_VALUE;

  *port_id = (uint16_t)metadata;
  return DOCA_SUCCESS;
}

static doca_error_t read_ethernet_header(
    const struct rte_mbuf *packet,
    struct rte_ether_hdr *header,
    uint16_t *vlan_id) {
  const struct rte_ether_hdr *packet_header;
  uint16_t ether_type;

  packet_header = rte_pktmbuf_read(packet, 0, sizeof(*header), header);
  if (packet_header == NULL)
    return DOCA_ERROR_INVALID_VALUE;
  if (packet_header != header)
    *header = *packet_header;

  *vlan_id = 0;
  ether_type = rte_be_to_cpu_16(header->ether_type);
  if (ether_type == RTE_ETHER_TYPE_VLAN ||
      ether_type == RTE_ETHER_TYPE_QINQ) {
    struct rte_vlan_hdr vlan_copy;
    const struct rte_vlan_hdr *vlan;

    vlan = rte_pktmbuf_read(packet, sizeof(*header), sizeof(*vlan),
                            &vlan_copy);
    if (vlan == NULL)
      return DOCA_ERROR_INVALID_VALUE;
    *vlan_id = rte_be_to_cpu_16(vlan->vlan_tci) & 0x0fff;
  }

  return DOCA_SUCCESS;
}

static doca_error_t process_learning_packet(struct slow_path *slow_path,
                                            struct rte_mbuf *packet,
                                            uint64_t now_ns) {
  struct rte_ether_hdr ethernet;
  uint16_t ingress_port_id;
  uint16_t vlan_id;
  doca_error_t result;

  result = read_ingress_port_id(packet, slow_path->ports,
                                &ingress_port_id);
  if (result != DOCA_SUCCESS) {
    fprintf(stderr, "Learning copy has no valid ingress metadata: %s\n",
            doca_error_get_descr(result));
    return result;
  }

  result = read_ethernet_header(packet, &ethernet, &vlan_id);
  if (result != DOCA_SUCCESS)
    return result;

  /*
   * Version 1 intentionally has one untagged bridge domain.  Matching VLAN in
   * software while omitting it from the hardware FDB would mix two VLANs.
   */
  if (vlan_id != 0) {
    fprintf(stderr, "Ignoring VLAN %u learning copy (untagged only)\n",
            vlan_id);
    return DOCA_SUCCESS;
  }

  if (!rte_is_valid_assigned_ether_addr(&ethernet.src_addr))
    return DOCA_SUCCESS;

  return l2_fdb_learn(slow_path->fdb, vlan_id, &ethernet.src_addr,
                      ingress_port_id, now_ns);
}

doca_error_t slow_path_init(struct dpdk_io *io,
                            struct switch_flow_ports *ports,
                            struct l2_fdb *fdb,
                            struct slow_path *slow_path) {
  if (io == NULL || ports == NULL || fdb == NULL || slow_path == NULL ||
      !io->port_started || !io->metadata_registered || !ports->started ||
      fdb->pipeline == NULL)
    return DOCA_ERROR_INVALID_VALUE;
  if (!rte_flow_dynf_metadata_avail())
    return DOCA_ERROR_NOT_SUPPORTED;

  slow_path->io = io;
  slow_path->ports = ports;
  slow_path->fdb = fdb;
  slow_path->next_aging_scan_ns = monotonic_time_ns() +
      (uint64_t)SWITCH_AGING_SCAN_SECONDS * 1000000000ULL;
  return DOCA_SUCCESS;
}

doca_error_t slow_path_run(struct slow_path *slow_path,
                           const volatile sig_atomic_t *stop_requested) {
  struct rte_mbuf *packets[SWITCH_PACKET_BURST];

  if (slow_path == NULL || slow_path->io == NULL ||
      stop_requested == NULL)
    return DOCA_ERROR_INVALID_VALUE;

  printf("Learning loop started; press Ctrl-C to stop\n");
  while (*stop_requested == 0) {
    uint64_t now_ns = monotonic_time_ns();
    uint16_t received = rte_eth_rx_burst(
        slow_path->io->parent_port_id, SWITCH_RX_QUEUE_ID, packets,
        SWITCH_PACKET_BURST);

    for (uint16_t i = 0; i < received; i++) {
      doca_error_t result =
          process_learning_packet(slow_path, packets[i], now_ns);

      rte_pktmbuf_free(packets[i]);
      if (result != DOCA_SUCCESS)
        return result;
    }

    if (now_ns >= slow_path->next_aging_scan_ns) {
      doca_error_t result = l2_fdb_age(slow_path->fdb, now_ns);

      if (result != DOCA_SUCCESS)
        return result;
      slow_path->next_aging_scan_ns = now_ns +
          (uint64_t)SWITCH_AGING_SCAN_SECONDS * 1000000000ULL;
    }

    if (received == 0)
      rte_pause();
  }

  printf("Learning loop stopped\n");
  return DOCA_SUCCESS;
}
