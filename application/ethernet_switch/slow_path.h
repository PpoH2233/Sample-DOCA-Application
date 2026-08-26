#ifndef SLOW_PATH_H
#define SLOW_PATH_H

#include <signal.h>
#include <stdint.h>

#include <doca_error.h>

#include "dpdk_io.h"
#include "flow_ports.h"
#include "l2_fdb.h"

struct slow_path {
  struct dpdk_io *io;
  struct switch_flow_ports *ports;
  struct l2_fdb *fdb;
  uint64_t next_aging_scan_ns;
};

doca_error_t slow_path_init(struct dpdk_io *io,
                            struct switch_flow_ports *ports,
                            struct l2_fdb *fdb,
                            struct slow_path *slow_path);

/* Poll until *stop_requested becomes non-zero. */
doca_error_t slow_path_run(struct slow_path *slow_path,
                           const volatile sig_atomic_t *stop_requested);

#endif /* SLOW_PATH_H */
