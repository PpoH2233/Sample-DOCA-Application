#ifndef FLOW_PORTS_H
#define FLOW_PORTS_H

#include <stdbool.h>
#include <stdint.h>

#include <doca_error.h>
#include <doca_flow.h>

#include "../../ethernet_device_discovery/ethernet_ports.h"

struct switch_flow_port {
  struct ethernet_port *ethernet;
  struct doca_flow_port *flow;
};

struct switch_flow_ports {
  struct switch_flow_port *items;
  uint16_t count;
  struct doca_flow_port *switch_port; /* Borrowed global switch-domain port. */
  bool started;
};

/* Bind every probed parent/representor ethdev to a DOCA Flow port. */
doca_error_t switch_flow_ports_start(struct ethernet_ports *ethernet_ports,
                                     struct switch_flow_ports *ports);

/* Flush and stop endpoint ports in reverse order, proxy/parent last. */
doca_error_t switch_flow_ports_stop(struct switch_flow_ports *ports);

#endif /* FLOW_PORTS_H */
