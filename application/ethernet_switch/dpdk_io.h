#ifndef DPDK_IO_H
#define DPDK_IO_H

#include <stdbool.h>
#include <stdint.h>

#include <doca_error.h>
#include <rte_mempool.h>

#include "../../ethernet_device_discovery/ethernet_ports.h"

/* CPU slow-path resources. Known-unicast traffic does not use these queues. */
struct dpdk_io {
  uint16_t parent_port_id;
  struct rte_mempool *mbuf_pool;
  bool metadata_registered;
  bool port_started;
};

/* Register pkt_meta, allocate mbufs, configure one RX/TX queue, start parent. */
doca_error_t dpdk_io_start(struct ethernet_ports *ports,
                           struct dpdk_io *io);

/* Stop the parent ethdev. The mempool is intentionally kept until close. */
doca_error_t dpdk_io_stop(struct dpdk_io *io);

/* Free the mempool after switch_devices_close() has closed all ethdevs. */
void dpdk_io_release(struct dpdk_io *io);

#endif /* DPDK_IO_H */
