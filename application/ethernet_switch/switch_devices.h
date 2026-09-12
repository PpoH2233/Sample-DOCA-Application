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

/* Open the parent and scoped external-host VF representors. */
doca_error_t switch_devices_open(const char *pci_address,
                                 const char *devargs,
                                 struct switch_devices *devices);

/* Open VF representors selected by vf_scope.
 * vf_scope accepts "all" or a comma-separated list of indexes/ranges such as
 * "0-6,10-20". The VF index is the value reported by DOCA for each
 * representor. The parent ethdev is always included. */
doca_error_t switch_devices_open_scoped(const char *pci_address,
                                        const char *devargs,
                                        const char *vf_scope,
                                        struct switch_devices *devices);

/* eSwitch-management variant: also requires and opens one Arm system SF. */
doca_error_t switch_devices_open_scoped_with_sf(
    const char *pci_address, const char *devargs, const char *vf_scope,
    struct switch_devices *devices);

/* Close DPDK ports first, then representors, then the parent DOCA device. */
doca_error_t switch_devices_close(struct switch_devices *devices);

#endif /* SWITCH_DEVICES_H */
