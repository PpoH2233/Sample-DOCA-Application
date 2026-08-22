#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <doca_dev.h>
#include <doca_error.h>

#include "../doca_device_discovery/open_pci_device.h"
#include "dpdk_runtime.h"
#include "ethernet_ports.h"

#define FLOW_SWITCH_DEVARGS                                                   \
  "dv_flow_en=2,fdb_def_rule_en=0,vport_match=1,"                           \
  "repr_matching_en=0,dv_xmeta_en=4"

static doca_error_t open_all_vf_representors(
    struct doca_dev *parent_device,
    struct doca_dev_rep ***opened_representors,
    size_t *opened_count) {
  struct doca_devinfo_rep **rep_info_list = NULL;
  struct doca_dev_rep **representors = NULL;
  uint32_t rep_info_count = 0;
  size_t vf_count = 0;
  size_t opened = 0;
  doca_error_t result;
  doca_error_t destroy_result;

  if (parent_device == NULL || opened_representors == NULL ||
      opened_count == NULL)
    return DOCA_ERROR_INVALID_VALUE;

  *opened_representors = NULL;
  *opened_count = 0;

  result = doca_devinfo_rep_create_list(parent_device,
                                         DOCA_DEVINFO_REP_FILTER_NET,
                                         &rep_info_list, &rep_info_count);
  if (result != DOCA_SUCCESS)
    return result;

  for (uint32_t i = 0; i < rep_info_count; i++) {
    enum doca_pci_func_type function_type;

    result = doca_devinfo_rep_get_pci_func_type(rep_info_list[i],
                                                 &function_type);
    if (result != DOCA_SUCCESS)
      goto destroy_rep_info_list;

    if (function_type == DOCA_PCI_FUNC_TYPE_VF)
      vf_count++;
  }

  if (vf_count == 0) {
    result = DOCA_ERROR_NOT_FOUND;
    goto destroy_rep_info_list;
  }

  representors = calloc(vf_count, sizeof(*representors));
  if (representors == NULL) {
    result = DOCA_ERROR_NO_MEMORY;
    goto destroy_rep_info_list;
  }

  for (uint32_t i = 0; i < rep_info_count; i++) {
    enum doca_pci_func_type function_type;

    result = doca_devinfo_rep_get_pci_func_type(rep_info_list[i],
                                                 &function_type);
    if (result != DOCA_SUCCESS)
      goto destroy_rep_info_list;

    if (function_type != DOCA_PCI_FUNC_TYPE_VF)
      continue;

    result = doca_dev_rep_open(rep_info_list[i], &representors[opened]);
    if (result != DOCA_SUCCESS)
      goto destroy_rep_info_list;

    opened++;
  }

  result = DOCA_SUCCESS;

destroy_rep_info_list:
  destroy_result = doca_devinfo_rep_destroy_list(rep_info_list);
  if (result == DOCA_SUCCESS && destroy_result != DOCA_SUCCESS)
    result = destroy_result;

  if (result != DOCA_SUCCESS) {
    for (size_t i = 0; i < opened; i++)
      (void)doca_dev_rep_close(representors[i]);
    free(representors);
    return result;
  }

  *opened_representors = representors;
  *opened_count = opened;
  return DOCA_SUCCESS;
}

static doca_error_t close_representors(struct doca_dev_rep **representors,
                                        size_t representor_count) {
  doca_error_t result = DOCA_SUCCESS;

  for (size_t i = 0; i < representor_count; i++) {
    doca_error_t close_result = doca_dev_rep_close(representors[i]);

    if (result == DOCA_SUCCESS && close_result != DOCA_SUCCESS)
      result = close_result;
  }

  free(representors);
  return result;
}

static int find_argument_separator(int argc, char **argv) {
  for (int i = 1; i < argc; i++) {
    if (argv[i][0] == '-' && argv[i][1] == '-' && argv[i][2] == '\0')
      return i;
  }

  return -1;
}

int main(int argc, char **argv) {
  struct ethernet_ports ports = {0};
  struct doca_dev_rep **representors = NULL;
  struct doca_dev *device = NULL;
  size_t representor_count = 0;
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

  result = open_all_vf_representors(device, &representors,
                                    &representor_count);
  if (result != DOCA_SUCCESS) {
    fprintf(stderr, "Failed to open VF representors: %s\n",
            doca_error_get_descr(result));
    goto device_cleanup;
  }

  printf("Opened %zu network VF representor(s)\n", representor_count);

  result = ethernet_ports_probe_representors(
      device, representors, representor_count, FLOW_SWITCH_DEVARGS, &ports);
  if (result != DOCA_SUCCESS) {
    fprintf(stderr, "Failed to probe parent and representor ports: %s\n",
            doca_error_get_descr(result));
    goto representor_cleanup;
  }

  printf("Probe created %u DPDK Ethernet port(s)\n", ports.count);

  for (uint16_t i = 0; i < ports.count; i++) {
    printf("\n=== Ethernet port %u of %u ===\n", i + 1, ports.count);

    result = ethernet_port_print_info(&ports.items[i]);
    if (result != DOCA_SUCCESS) {
      fprintf(stderr, "Failed to read DPDK port %u information: %s\n",
              ports.items[i].port_id, doca_error_get_descr(result));
      goto port_cleanup;
    }
  }

  exit_status = EXIT_SUCCESS;

port_cleanup:
  result = ethernet_ports_close(&ports);
  if (result != DOCA_SUCCESS) {
    fprintf(stderr, "Failed to close DPDK Ethernet ports: %s\n",
            doca_error_get_descr(result));
    exit_status = EXIT_FAILURE;
  }

representor_cleanup:
  result = close_representors(representors, representor_count);
  if (result != DOCA_SUCCESS) {
    fprintf(stderr, "Failed to close VF representors: %s\n",
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
