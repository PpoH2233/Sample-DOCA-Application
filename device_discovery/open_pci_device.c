#include "open_pci_device.h"

#include <stdint.h>
#include <stdio.h>

#include <doca_error.h>

struct doca_dev *open_pci_device(const char *pci_address) {
  struct doca_devinfo **device_list = NULL;
  struct doca_dev *device = NULL;
  uint32_t device_count = 0;
  doca_error_t result;

  if (pci_address == NULL)
    return NULL;

  result = doca_devinfo_create_list(&device_list, &device_count);
  if (result != DOCA_SUCCESS) {
    fprintf(stderr, "Failed to create device list: %s\n",
            doca_error_get_descr(result));
    return NULL;
  }

  for (uint32_t i = 0; i < device_count; i++) {
    uint8_t is_match = 0;

    result = doca_devinfo_is_equal_pci_addr(device_list[i], pci_address,
                                             &is_match);
    if (result != DOCA_SUCCESS || is_match == 0)
      continue;

    result = doca_dev_open(device_list[i], &device);
    if (result != DOCA_SUCCESS) {
      fprintf(stderr, "Failed to open device %s: %s\n", pci_address,
              doca_error_get_descr(result));
    }
    break;
  }

  result = doca_devinfo_destroy_list(device_list);
  if (result != DOCA_SUCCESS) {
    fprintf(stderr, "Failed to destroy device list: %s\n",
            doca_error_get_descr(result));
    if (device != NULL) {
      (void)doca_dev_close(device);
      device = NULL;
    }
  }

  return device;
}
