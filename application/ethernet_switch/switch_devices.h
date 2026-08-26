#ifndef SWITCH_DEVICES_H
#define SWITCH_DEVICES_H

#include <stddef.h>

#include <doca_dev.h>
#include <doca_error.h>

#include "../../ethernet_device_discovery/ethernet_ports.h"

/*
 * Owns every DOCA handle opened for one BlueField parent device.
 * The ethernet_ports array owns only DPDK mappings; it borrows these handles.
 */
struct switch_devices {
  struct doca_dev *parent;
  struct doca_dev_rep **representors;
  size_t representor_count;
  struct ethernet_ports ethernet_ports;
};

/* Open the parent, all network VF representors, and probe all DPDK ethdevs. */
doca_error_t switch_devices_open(const char *pci_address,
                                 const char *devargs,
                                 struct switch_devices *devices);

/* Close DPDK ports first, then representors, then the parent DOCA device. */
doca_error_t switch_devices_close(struct switch_devices *devices);

#endif /* SWITCH_DEVICES_H */
