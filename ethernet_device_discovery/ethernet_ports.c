#include "ethernet_ports.h"

#include <stdio.h>

#include <doca_dpdk.h>
#include <rte_ethdev.h>

doca_error_t ethernet_port_probe(struct doca_dev *device,
                                 const char *devargs,
                                 struct ethernet_port *port) {
  doca_error_t result;

  if (device == NULL || port == NULL)
    return DOCA_ERROR_INVALID_VALUE;

  if (port->is_probed)
    return DOCA_ERROR_BAD_STATE;

  if (devargs == NULL)
    devargs = "";

  result = doca_dpdk_port_probe(device, devargs);
  if (result != DOCA_SUCCESS)
    return result;

  result = doca_dpdk_get_first_port_id(device, &port->port_id);
  if (result != DOCA_SUCCESS)
    return result;

  if (!rte_eth_dev_is_valid_port(port->port_id))
    return DOCA_ERROR_NOT_FOUND;

  port->device = device;
  port->is_probed = true;
  return DOCA_SUCCESS;
}

doca_error_t ethernet_port_print_info(const struct ethernet_port *port) {
  struct rte_eth_dev_info device_info = {0};

  if (port == NULL || !port->is_probed)
    return DOCA_ERROR_BAD_STATE;

  if (rte_eth_dev_info_get(port->port_id, &device_info) != 0)
    return DOCA_ERROR_DRIVER;

  printf("DPDK port ID: %u\n", port->port_id);
  printf("Driver: %s\n",
         device_info.driver_name == NULL ? "unknown" : device_info.driver_name);

  return DOCA_SUCCESS;
}

doca_error_t ethernet_port_close(struct ethernet_port *port) {
  if (port == NULL)
    return DOCA_ERROR_INVALID_VALUE;

  if (!port->is_probed)
    return DOCA_SUCCESS;

  if (rte_eth_dev_close(port->port_id) != 0)
    return DOCA_ERROR_DRIVER;

  port->device = NULL;
  port->port_id = 0;
  port->is_probed = false;
  return DOCA_SUCCESS;
}
