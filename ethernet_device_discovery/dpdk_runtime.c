#include "dpdk_runtime.h"

#include <stdbool.h>
#include <stdlib.h>

#include <rte_eal.h>
static bool runtime_initialized;

doca_error_t dpdk_runtime_init(int argc, char **argv) {
  char **eal_argv;
  int eal_argc;
  int result;

  if (argc <= 0 || argv == NULL)
    return DOCA_ERROR_INVALID_VALUE;

  if (runtime_initialized)
    return DOCA_ERROR_BAD_STATE;

  eal_argc = argc + 4;
  eal_argv = calloc((size_t)eal_argc + 1, sizeof(*eal_argv));
  if (eal_argv == NULL)
    return DOCA_ERROR_NO_MEMORY;

  for (int i = 0; i < argc; i++)
    eal_argv[i] = argv[i];

  /* Keep bus support enabled, but prevent EAL from probing real devices. */
  eal_argv[argc] = "-a";
  eal_argv[argc + 1] = "pci:00:00.0";
  eal_argv[argc + 2] = "-a";
  eal_argv[argc + 3] = "auxiliary:";

  result = rte_eal_init(eal_argc, eal_argv);
  free(eal_argv);

  if (result < 0)
    return DOCA_ERROR_DRIVER;

  runtime_initialized = true;
  return DOCA_SUCCESS;
}

doca_error_t dpdk_runtime_cleanup(void) {
  if (!runtime_initialized)
    return DOCA_SUCCESS;

  if (rte_eal_cleanup() < 0)
    return DOCA_ERROR_DRIVER;

  runtime_initialized = false;
  return DOCA_SUCCESS;
}
