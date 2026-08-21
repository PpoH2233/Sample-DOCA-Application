#ifndef DPDK_RUNTIME_H
#define DPDK_RUNTIME_H

#include <doca_error.h>

/*
 * Initialize DPDK EAL without auto-probing real PCI or auxiliary devices.
 * Call this once before using doca_dpdk_port_probe().
 */
doca_error_t dpdk_runtime_init(int argc, char **argv);

/* Release the process-wide DPDK EAL resources. */
doca_error_t dpdk_runtime_cleanup(void);

#endif /* DPDK_RUNTIME_H */
