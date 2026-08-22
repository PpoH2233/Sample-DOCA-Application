#include "ethernet_ports.h"

#include <stdio.h>

#include <doca_dpdk.h>
#include <rte_ether.h>
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
  port->representor = NULL;
  port->role = ETHERNET_PORT_ROLE_PARENT;
  port->is_probed = true;
  return DOCA_SUCCESS;
}

doca_error_t ethernet_port_print_info(const struct ethernet_port *port) {
  struct rte_eth_dev_info device_info = {0};
  struct rte_ether_addr mac_address = {0};
  uint16_t mtu = 0;

  if (port == NULL || !port->is_probed)
    return DOCA_ERROR_BAD_STATE;

  if (rte_eth_dev_info_get(port->port_id, &device_info) != 0)
    return DOCA_ERROR_DRIVER;

  printf("Role: %s\n", port->role == ETHERNET_PORT_ROLE_PARENT
                            ? "parent"
                            : "VF representor");
  printf("DPDK port ID: %u\n", port->port_id);
  printf("Driver: %s\n",
         device_info.driver_name == NULL ? "unknown" : device_info.driver_name);
  printf("NUMA socket: %d\n", rte_eth_dev_socket_id(port->port_id));
  printf("Queues: configured rx=%u tx=%u, maximum rx=%u tx=%u\n",
         device_info.nb_rx_queues, device_info.nb_tx_queues,
         device_info.max_rx_queues, device_info.max_tx_queues);
  printf("MTU range: %u-%u\n", device_info.min_mtu, device_info.max_mtu);
  printf("eSwitch: name=%s domain=%u port=%u\n",
         device_info.switch_info.name == NULL ? "unknown"
                                              : device_info.switch_info.name,
         device_info.switch_info.domain_id, device_info.switch_info.port_id);

  if (rte_eth_macaddr_get(port->port_id, &mac_address) == 0) {
    printf("MAC address: " RTE_ETHER_ADDR_PRT_FMT "\n",
           RTE_ETHER_ADDR_BYTES(&mac_address));
  } else {
    printf("MAC address: unavailable\n");
  }

  if (rte_eth_dev_get_mtu(port->port_id, &mtu) == 0)
    printf("Current MTU: %u\n", mtu);
  else
    printf("Current MTU: unavailable\n");

  if (port->role == ETHERNET_PORT_ROLE_REPRESENTOR) {
    char vuid[DOCA_DEVINFO_VUID_SIZE] = {0};
    struct doca_devinfo_rep *rep_info =
        doca_dev_rep_as_devinfo(port->representor);

    printf("VF topology: host=%u pf=%u vf=%u\n", port->host_index,
           port->pf_index, port->vf_index);

    if (rep_info != NULL &&
        doca_devinfo_rep_get_vuid(rep_info, vuid, sizeof(vuid)) ==
            DOCA_SUCCESS)
      printf("Representor VUID: %s\n", vuid);
    else
      printf("Representor VUID: unavailable\n");
  }

  return DOCA_SUCCESS;
}

doca_error_t ethernet_port_close(struct ethernet_port *port) {
  if (port == NULL)
    return DOCA_ERROR_INVALID_VALUE;

  if (!port->is_probed)
    return DOCA_SUCCESS;

  if (rte_eth_dev_close(port->port_id) != 0)
    return DOCA_ERROR_DRIVER;

  *port = (struct ethernet_port){0};
  return DOCA_SUCCESS;
}
