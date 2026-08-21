#include <stdio.h>
#include <stdlib.h>

#include <doca_error.h>

#include "../doca_device_discovery/open_pci_device.h"
#include "dpdk_runtime.h"
#include "ethernet_ports.h"

#define FLOW_SWITCH_DEVARGS                                                   \
  "dv_flow_en=2,fdb_def_rule_en=0,dv_xmeta_en=4"

static int find_argument_separator(int argc, char **argv) {
  for (int i = 1; i < argc; i++) {
    if (argv[i][0] == '-' && argv[i][1] == '-' && argv[i][2] == '\0')
      return i;
  }

  return -1;
}

int main(int argc, char **argv) {
  struct ethernet_port port = {0};
  struct doca_dev *device = NULL;
  doca_error_t result;
  int separator;
  int exit_status = EXIT_FAILURE;

  separator = find_argument_separator(argc, argv);
  if (separator < 0 || separator + 2 != argc) {
    fprintf(stderr, "Usage: %s [EAL options] -- <DEVICE_PCI>\n", argv[0]);
    return EXIT_FAILURE;
  }

  result = dpdk_runtime_init(separator, argv);
  if (result != DOCA_SUCCESS) {
    fprintf(stderr, "Failed to initialize DPDK: %s\n",
            doca_error_get_descr(result));
    return EXIT_FAILURE;
  }

  device = open_pci_device(argv[separator + 1]);
  if (device == NULL) {
    fprintf(stderr, "Failed to open DOCA device %s\n", argv[separator + 1]);
    goto runtime_cleanup;
  }

  result = ethernet_port_probe(device, FLOW_SWITCH_DEVARGS, &port);
  if (result != DOCA_SUCCESS) {
    fprintf(stderr, "Failed to probe DPDK Ethernet port: %s\n",
            doca_error_get_descr(result));
    goto device_cleanup;
  }

  result = ethernet_port_print_info(&port);
  if (result != DOCA_SUCCESS) {
    fprintf(stderr, "Failed to read DPDK Ethernet port information: %s\n",
            doca_error_get_descr(result));
    goto port_cleanup;
  }

  exit_status = EXIT_SUCCESS;

port_cleanup:
  result = ethernet_port_close(&port);
  if (result != DOCA_SUCCESS) {
    fprintf(stderr, "Failed to close DPDK Ethernet port: %s\n",
            doca_error_get_descr(result));
    exit_status = EXIT_FAILURE;
  }

device_cleanup:
  result = doca_dev_close(device);
  if (result != DOCA_SUCCESS) {
    fprintf(stderr, "Failed to close DOCA device: %s\n",
            doca_error_get_descr(result));
    exit_status = EXIT_FAILURE;
  }

runtime_cleanup:
  result = dpdk_runtime_cleanup();
  if (result != DOCA_SUCCESS) {
    fprintf(stderr, "Failed to clean up DPDK: %s\n",
            doca_error_get_descr(result));
    exit_status = EXIT_FAILURE;
  }

  return exit_status;
}
