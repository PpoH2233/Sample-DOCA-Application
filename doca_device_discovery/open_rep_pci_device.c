#include "open_rep_pci_device.h"

#include <stdint.h>
#include <stdio.h>

#include <doca_error.h>

struct doca_dev_rep *open_rep_pci_device(struct doca_dev *parent_device,
                                         const char *pci_address) {
  const struct doca_devinfo *parent_info;
  struct doca_devinfo_rep **rep_list = NULL;
  struct doca_dev_rep *representor = NULL;
  uint32_t rep_count = 0;
  uint8_t filter_supported = 0;
  doca_error_t result;

  if (parent_device == NULL || pci_address == NULL)
    return NULL;

  parent_info = doca_dev_as_devinfo(parent_device);
  if (parent_info == NULL)
    return NULL;

  result = doca_devinfo_rep_cap_is_filter_net_supported(
      parent_info, &filter_supported);
  if (result != DOCA_SUCCESS || filter_supported == 0) {
    fprintf(stderr, "Network representor discovery is not supported\n");
    return NULL;
  }

  result = doca_devinfo_rep_create_list(parent_device,
                                         DOCA_DEVINFO_REP_FILTER_NET,
                                         &rep_list, &rep_count);
  if (result != DOCA_SUCCESS) {
    fprintf(stderr, "Failed to create representor list: %s\n",
            doca_error_get_descr(result));
    return NULL;
  }

  for (uint32_t i = 0; i < rep_count; i++) {
    uint8_t is_match = 0;

    result = doca_devinfo_rep_is_equal_pci_addr(rep_list[i], pci_address,
                                                 &is_match);
    if (result != DOCA_SUCCESS || is_match == 0)
      continue;

    result = doca_dev_rep_open(rep_list[i], &representor);
    if (result != DOCA_SUCCESS) {
      fprintf(stderr, "Failed to open representor %s: %s\n", pci_address,
              doca_error_get_descr(result));
    }
    break;
  }

  result = doca_devinfo_rep_destroy_list(rep_list);
  if (result != DOCA_SUCCESS) {
    fprintf(stderr, "Failed to destroy representor list: %s\n",
            doca_error_get_descr(result));
    if (representor != NULL) {
      (void)doca_dev_rep_close(representor);
      representor = NULL;
    }
  }

  return representor;
}
