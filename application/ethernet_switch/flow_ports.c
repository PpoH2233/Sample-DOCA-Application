#include "flow_ports.h"

#include <stdlib.h>

#include "switch_config.h"

static doca_error_t start_one_port(struct ethernet_port *ethernet,
                                   struct doca_flow_port **flow_port) {
  struct doca_flow_port_cfg *cfg = NULL;
  doca_error_t result;
  doca_error_t destroy_result;

  result = doca_flow_port_cfg_create(&cfg);
  if (result != DOCA_SUCCESS)
    return result;

  result = doca_flow_port_cfg_set_port_id(cfg, ethernet->port_id);
  if (result != DOCA_SUCCESS)
    goto destroy_cfg;

  /*
   * The classifier writes pkt_meta, so DOCA Flow 3.4 requires actions memory
   * to be reserved before the port starts. Tune this development value with
   * the DOCA Flow Tune tool once the pipeline is stable.
   */
  result = doca_flow_port_cfg_set_actions_mem_size(
      cfg, SWITCH_ACTIONS_MEM_SIZE);
  if (result != DOCA_SUCCESS)
    goto destroy_cfg;

  if (ethernet->role == ETHERNET_PORT_ROLE_PARENT)
    result = doca_flow_port_cfg_set_dev(cfg, ethernet->device);
  else
    result = doca_flow_port_cfg_set_dev_rep(cfg, ethernet->representor);
  if (result != DOCA_SUCCESS)
    goto destroy_cfg;

  result = doca_flow_port_start(cfg, flow_port);

destroy_cfg:
  destroy_result = doca_flow_port_cfg_destroy(cfg);
  if (result == DOCA_SUCCESS && destroy_result != DOCA_SUCCESS)
    result = destroy_result;
  return result;
}

doca_error_t switch_flow_ports_start(struct ethernet_ports *ethernet_ports,
                                     struct switch_flow_ports *ports) {
  doca_error_t result;

  if (ethernet_ports == NULL || ports == NULL ||
      !ethernet_ports->is_probed || ethernet_ports->count == 0)
    return DOCA_ERROR_INVALID_VALUE;
  if (ports->started)
    return DOCA_ERROR_BAD_STATE;

  ports->items = calloc(ethernet_ports->count, sizeof(*ports->items));
  if (ports->items == NULL)
    return DOCA_ERROR_NO_MEMORY;
  ports->count = ethernet_ports->count;

  for (uint16_t i = 0; i < ports->count; i++) {
    ports->items[i].ethernet = &ethernet_ports->items[i];
    result = start_one_port(ports->items[i].ethernet,
                            &ports->items[i].flow);
    if (result != DOCA_SUCCESS) {
      ports->count = i;
      (void)switch_flow_ports_stop(ports);
      return result;
    }
  }

  ports->switch_port = doca_flow_port_switch_get(NULL);
  if (ports->switch_port == NULL) {
    (void)switch_flow_ports_stop(ports);
    return DOCA_ERROR_NOT_FOUND;
  }

  ports->started = true;
  return DOCA_SUCCESS;
}

doca_error_t switch_flow_ports_stop(struct switch_flow_ports *ports) {
  doca_error_t result = DOCA_SUCCESS;

  if (ports == NULL)
    return DOCA_ERROR_INVALID_VALUE;

  for (uint16_t i = ports->count; i > 0; i--) {
    struct doca_flow_port *port = ports->items[i - 1].flow;
    doca_error_t current;

    if (port == NULL)
      continue;
    doca_flow_port_pipes_flush(port);
    current = doca_flow_port_stop(port);
    if (result == DOCA_SUCCESS && current != DOCA_SUCCESS)
      result = current;
  }

  free(ports->items);
  *ports = (struct switch_flow_ports){0};
  return result;
}
