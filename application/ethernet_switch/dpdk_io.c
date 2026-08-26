#include "dpdk_io.h"

#include <stdio.h>

#include <rte_ethdev.h>
#include <rte_flow.h>

#include "switch_config.h"

doca_error_t dpdk_io_start(struct ethernet_ports *ports,
                           struct dpdk_io *io) {
  struct ethernet_port *parent;
  struct rte_eth_dev_info info = {0};
  struct rte_eth_conf port_conf = {0};
  struct rte_eth_rxconf rx_conf;
  struct rte_eth_txconf tx_conf;
  uint16_t rx_desc = SWITCH_RX_DESC;
  uint16_t tx_desc = SWITCH_TX_DESC;
  int socket_id;
  int result;

  if (ports == NULL || io == NULL)
    return DOCA_ERROR_INVALID_VALUE;
  if (io->mbuf_pool != NULL || io->port_started)
    return DOCA_ERROR_BAD_STATE;

  parent = ethernet_ports_find_parent(ports);
  if (parent == NULL)
    return DOCA_ERROR_NOT_FOUND;

  result = rte_flow_dynf_metadata_register();
  if (result < 0) {
    fprintf(stderr, "Failed to register DPDK packet metadata: %d\n", result);
    return DOCA_ERROR_DRIVER;
  }
  io->metadata_registered = true;

  io->mbuf_pool = rte_pktmbuf_pool_create(
      "ethernet_switch_mbuf_pool", SWITCH_MBUF_COUNT, SWITCH_MBUF_CACHE, 0,
      RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());
  if (io->mbuf_pool == NULL)
    return DOCA_ERROR_NO_MEMORY;

  io->parent_port_id = parent->port_id;
  result = rte_eth_dev_info_get(io->parent_port_id, &info);
  if (result != 0)
    goto driver_error;

  result = rte_eth_dev_configure(io->parent_port_id, 1, 1, &port_conf);
  if (result != 0)
    goto driver_error;

  result = rte_eth_dev_adjust_nb_rx_tx_desc(io->parent_port_id,
                                             &rx_desc, &tx_desc);
  if (result != 0)
    goto driver_error;

  socket_id = rte_eth_dev_socket_id(io->parent_port_id);
  if (socket_id < 0)
    socket_id = rte_socket_id();

  rx_conf = info.default_rxconf;
  result = rte_eth_rx_queue_setup(io->parent_port_id, SWITCH_RX_QUEUE_ID,
                                  rx_desc, socket_id, &rx_conf,
                                  io->mbuf_pool);
  if (result != 0)
    goto driver_error;

  tx_conf = info.default_txconf;
  result = rte_eth_tx_queue_setup(io->parent_port_id, SWITCH_TX_QUEUE_ID,
                                  tx_desc, socket_id, &tx_conf);
  if (result != 0)
    goto driver_error;

  result = rte_eth_dev_start(io->parent_port_id);
  if (result != 0)
    goto driver_error;

  io->port_started = true;
  printf("DPDK slow path: parent port %u, RX queue 0, TX queue 0\n",
         io->parent_port_id);
  return DOCA_SUCCESS;

driver_error:
  /*
   * A configured RX queue may still hold a reference to this pool.  main.c
   * closes the ethdev first and calls dpdk_io_release() afterwards.
   */
  return DOCA_ERROR_DRIVER;
}

doca_error_t dpdk_io_stop(struct dpdk_io *io) {
  if (io == NULL)
    return DOCA_ERROR_INVALID_VALUE;
  if (!io->port_started)
    return DOCA_SUCCESS;

  if (rte_eth_dev_stop(io->parent_port_id) != 0)
    return DOCA_ERROR_DRIVER;

  io->port_started = false;
  return DOCA_SUCCESS;
}

void dpdk_io_release(struct dpdk_io *io) {
  if (io == NULL)
    return;
  if (io->mbuf_pool != NULL)
    rte_mempool_free(io->mbuf_pool);
  *io = (struct dpdk_io){0};
}
