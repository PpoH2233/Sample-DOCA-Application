#include "open_rep_vf_device.h"

#include <stdint.h>
#include <stdio.h>

#include <doca_error.h>

static int is_target_vf(const struct doca_devinfo_rep *rep_info,
                        uint32_t target_host,
                        uint32_t target_pf,
                        uint32_t target_vf) {
  enum doca_pci_func_type function_type;
  uint32_t host_index;
  uint32_t pf_index;
  uint32_t vf_index;

  if (doca_devinfo_rep_get_pci_func_type(rep_info, &function_type) !=
          DOCA_SUCCESS ||
      function_type != DOCA_PCI_FUNC_TYPE_VF)
    return 0;

  if (doca_devinfo_rep_get_host_index(rep_info, &host_index) != DOCA_SUCCESS ||
      doca_devinfo_rep_get_pf_index(rep_info, &pf_index) != DOCA_SUCCESS ||
      doca_devinfo_rep_get_vf_index(rep_info, &vf_index) != DOCA_SUCCESS)
    return 0;

  return host_index == target_host && pf_index == target_pf &&
         vf_index == target_vf;
}

struct doca_dev_rep *open_rep_vf_device(struct doca_dev *parent_device,
                                        uint32_t host_index,
                                        uint32_t pf_index,
                                        uint32_t vf_index) {
  const struct doca_devinfo *parent_info;
  struct doca_devinfo_rep **rep_list = NULL;
  struct doca_dev_rep *representor = NULL;
  uint32_t rep_count = 0;
  uint8_t filter_supported = 0;
  doca_error_t result;

  if (parent_device == NULL)
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
    if (!is_target_vf(rep_list[i], host_index, pf_index, vf_index))
      continue;

    result = doca_dev_rep_open(rep_list[i], &representor);
    if (result != DOCA_SUCCESS) {
      fprintf(stderr, "Failed to open VF representor: %s\n",
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

  if (representor == NULL) {
    fprintf(stderr, "VF representor host=%u pf=%u vf=%u was not opened\n",
            host_index, pf_index, vf_index);
  }

  return representor;
}
