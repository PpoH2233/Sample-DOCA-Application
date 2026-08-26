#include "switch_devices.h"

#include <stdint.h>
#include <stdlib.h>

#include "../../doca_device_discovery/open_pci_device.h"

static doca_error_t open_all_vf_representors(
    struct doca_dev *parent,
    struct doca_dev_rep ***opened_representors,
    size_t *opened_count) {
  struct doca_devinfo_rep **info_list = NULL;
  struct doca_dev_rep **representors = NULL;
  uint32_t info_count = 0;
  size_t vf_count = 0;
  size_t opened = 0;
  doca_error_t result;
  doca_error_t destroy_result;

  result = doca_devinfo_rep_create_list(parent, DOCA_DEVINFO_REP_FILTER_NET,
                                        &info_list, &info_count);
  if (result != DOCA_SUCCESS)
    return result;

  for (uint32_t i = 0; i < info_count; i++) {
    enum doca_pci_func_type type;

    result = doca_devinfo_rep_get_pci_func_type(info_list[i], &type);
    if (result != DOCA_SUCCESS)
      goto destroy_list;
    if (type == DOCA_PCI_FUNC_TYPE_VF)
      vf_count++;
  }

  if (vf_count == 0) {
    result = DOCA_ERROR_NOT_FOUND;
    goto destroy_list;
  }

  representors = calloc(vf_count, sizeof(*representors));
  if (representors == NULL) {
    result = DOCA_ERROR_NO_MEMORY;
    goto destroy_list;
  }

  for (uint32_t i = 0; i < info_count; i++) {
    enum doca_pci_func_type type;

    result = doca_devinfo_rep_get_pci_func_type(info_list[i], &type);
    if (result != DOCA_SUCCESS)
      goto destroy_list;
    if (type != DOCA_PCI_FUNC_TYPE_VF)
      continue;

    result = doca_dev_rep_open(info_list[i], &representors[opened]);
    if (result != DOCA_SUCCESS)
      goto destroy_list;
    opened++;
  }

destroy_list:
  destroy_result = doca_devinfo_rep_destroy_list(info_list);
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

doca_error_t switch_devices_open(const char *pci_address,
                                 const char *devargs,
                                 struct switch_devices *devices) {
  doca_error_t result;

  if (pci_address == NULL || devices == NULL)
    return DOCA_ERROR_INVALID_VALUE;
  if (devices->parent != NULL)
    return DOCA_ERROR_BAD_STATE;

  devices->parent = open_pci_device(pci_address);
  if (devices->parent == NULL)
    return DOCA_ERROR_NOT_FOUND;

  result = open_all_vf_representors(devices->parent,
                                    &devices->representors,
                                    &devices->representor_count);
  if (result != DOCA_SUCCESS)
    goto close_parent;

  result = ethernet_ports_probe_representors(
      devices->parent, devices->representors, devices->representor_count,
      devargs, &devices->ethernet_ports);
  if (result != DOCA_SUCCESS)
    goto close_representors;

  return DOCA_SUCCESS;

close_representors:
  for (size_t i = 0; i < devices->representor_count; i++)
    (void)doca_dev_rep_close(devices->representors[i]);
  free(devices->representors);
  devices->representors = NULL;
  devices->representor_count = 0;
close_parent:
  (void)doca_dev_close(devices->parent);
  devices->parent = NULL;
  return result;
}

doca_error_t switch_devices_close(struct switch_devices *devices) {
  doca_error_t result = DOCA_SUCCESS;
  doca_error_t current;

  if (devices == NULL)
    return DOCA_ERROR_INVALID_VALUE;

  current = ethernet_ports_close(&devices->ethernet_ports);
  if (current != DOCA_SUCCESS)
    result = current;

  for (size_t i = 0; i < devices->representor_count; i++) {
    current = doca_dev_rep_close(devices->representors[i]);
    if (result == DOCA_SUCCESS && current != DOCA_SUCCESS)
      result = current;
  }
  free(devices->representors);

  if (devices->parent != NULL) {
    current = doca_dev_close(devices->parent);
    if (result == DOCA_SUCCESS && current != DOCA_SUCCESS)
      result = current;
  }

  *devices = (struct switch_devices){0};
  return result;
}
