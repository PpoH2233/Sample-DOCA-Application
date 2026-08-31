#ifndef ESWITCH_MANAGER_H
#define ESWITCH_MANAGER_H

#include <stdbool.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>

#include <doca_error.h>

#include "../ethernet_switch/dpdk_io.h"
#include "eswitch_config.h"
#include "eswitch_fdb.h"

struct managed_vswitch {
  bool exists;
  uint16_t id;
  struct eswitch_flood_group flood;
};

struct eswitch_manager {
  struct dpdk_io *io;
  struct switch_flow_ports *ports;
  struct eswitch_pipeline *pipeline;
  struct eswitch_fdb fdb;
  struct managed_vswitch switches[ESWITCH_MAX_VSWITCHES];
  uint16_t *port_owner; /* indexed like ports->items; 0 means available */
  uint64_t started_ns;
  uint64_t next_aging_ns;
  char state_path[PATH_MAX];
  bool initialized;
};

doca_error_t eswitch_manager_init(struct dpdk_io *io,
                                  struct switch_flow_ports *ports,
                                  struct eswitch_pipeline *pipeline,
                                  const char *state_path,
                                  struct eswitch_manager *manager);
doca_error_t eswitch_manager_poll_packets(struct eswitch_manager *manager,
                                          bool *did_work);
doca_error_t eswitch_manager_maintenance(struct eswitch_manager *manager);
doca_error_t eswitch_manager_command(const char *request, char *response,
                                     size_t response_size, void *context);
doca_error_t eswitch_manager_destroy(struct eswitch_manager *manager);
/* Emergency cleanup after a graceful hardware removal error. Call this before
 * destroying the shared pipeline so per-vSwitch pipes release references. */
void eswitch_manager_release(struct eswitch_manager *manager);

#endif /* ESWITCH_MANAGER_H */
