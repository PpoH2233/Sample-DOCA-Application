#ifndef ETHERNET_PORTS_H
#define ETHERNET_PORTS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <doca_dev.h>
#include <doca_error.h>

enum ethernet_port_role {
  ETHERNET_PORT_ROLE_PARENT,
  ETHERNET_PORT_ROLE_REPRESENTOR,
};

/* A DPDK Ethernet port created from an opened DOCA device. */
struct ethernet_port {
  struct doca_dev *device;             /* Borrowed parent device. */
  struct doca_dev_rep *representor;    /* Borrowed; NULL for parent port. */
  uint16_t port_id;
  enum ethernet_port_role role;
  uint32_t host_index;
  uint32_t pf_index;
  uint32_t vf_index;
  bool is_probed;
};

/* Parent port plus all VF representor ports created by one probe operation. */
struct ethernet_ports {
  struct doca_dev *parent_device; /* Borrowed; caller owns this handle. */
  struct ethernet_port *items;    /* Allocated and owned by this module. */
  uint16_t count;
  bool is_probed;
};

/*
 * Probe device through the DOCA-DPDK bridge and resolve its first port ID.
 * DPDK EAL must already be initialized by dpdk_runtime_init().
 */
doca_error_t ethernet_port_probe(struct doca_dev *device,
                                 const char *devargs,
                                 struct ethernet_port *port);

/*
 * Probe a parent device and its opened VF representors in one operation.
 * The result maps every DPDK port ID to either the parent or one input VF.
 * DPDK EAL must be initialized and ports must be zero-initialized.
 */
doca_error_t ethernet_ports_probe_representors(
    struct doca_dev *parent_device,
    struct doca_dev_rep **representors,
    size_t representor_count,
    const char *devargs,
    struct ethernet_ports *ports);

/* Find the parent port without relying on the order of ports->items. */
struct ethernet_port *
ethernet_ports_find_parent(struct ethernet_ports *ports);

/* Find one VF representor by its complete host/PF/VF identity. */
struct ethernet_port *ethernet_ports_find_vf(struct ethernet_ports *ports,
                                             uint32_t host_index,
                                             uint32_t pf_index,
                                             uint32_t vf_index);

/* Print basic information about a successfully probed Ethernet port. */
doca_error_t ethernet_port_print_info(const struct ethernet_port *port);

/* Close the DPDK Ethernet port. The borrowed doca_dev is not closed. */
doca_error_t ethernet_port_close(struct ethernet_port *port);

/* Close all DPDK ports and release the mapping array, not DOCA handles. */
doca_error_t ethernet_ports_close(struct ethernet_ports *ports);

#endif /* ETHERNET_PORTS_H */
