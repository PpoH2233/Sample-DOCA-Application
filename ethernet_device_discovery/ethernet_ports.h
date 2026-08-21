#ifndef ETHERNET_PORTS_H
#define ETHERNET_PORTS_H

#include <stdbool.h>
#include <stdint.h>

#include <doca_dev.h>
#include <doca_error.h>

/* A DPDK Ethernet port created from an opened DOCA device. */
struct ethernet_port {
  struct doca_dev *device; /* Borrowed; the caller owns this handle. */
  uint16_t port_id;
  bool is_probed;
};

/*
 * Probe device through the DOCA-DPDK bridge and resolve its first port ID.
 * DPDK EAL must already be initialized by dpdk_runtime_init().
 */
doca_error_t ethernet_port_probe(struct doca_dev *device,
                                 const char *devargs,
                                 struct ethernet_port *port);

/* Print basic information about a successfully probed Ethernet port. */
doca_error_t ethernet_port_print_info(const struct ethernet_port *port);

/* Close the DPDK Ethernet port. The borrowed doca_dev is not closed. */
doca_error_t ethernet_port_close(struct ethernet_port *port);

#endif /* ETHERNET_PORTS_H */
