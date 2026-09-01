#include "switch_devices.h"

#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "../../doca_device_discovery/open_pci_device.h"

static void skip_spaces(const char **cursor) {
  while (isspace((unsigned char)**cursor))
    (*cursor)++;
}

static bool parse_vf_index(const char **cursor, uint32_t *value) {
  char *end;
  unsigned long parsed;

  skip_spaces(cursor);
  if (!isdigit((unsigned char)**cursor))
    return false;
  errno = 0;
  parsed = strtoul(*cursor, &end, 10);
  if (errno != 0 || parsed > UINT32_MAX)
    return false;
  *cursor = end;
  *value = (uint32_t)parsed;
  return true;
}

static doca_error_t vf_scope_matches(const char *scope, uint32_t vf_index,
                                     bool *matches) {
  const char *cursor = scope;

  if (scope == NULL || matches == NULL || *scope == '\0')
    return DOCA_ERROR_INVALID_VALUE;
  *matches = false;
  if (strcmp(scope, "all") == 0) {
    *matches = true;
    return DOCA_SUCCESS;
  }

  while (*cursor != '\0') {
    uint32_t first;
    uint32_t last;

    if (!parse_vf_index(&cursor, &first))
      return DOCA_ERROR_INVALID_VALUE;
    skip_spaces(&cursor);
    last = first;
    if (*cursor == '-') {
      cursor++;
      if (!parse_vf_index(&cursor, &last) || last < first)
        return DOCA_ERROR_INVALID_VALUE;
      skip_spaces(&cursor);
    }
    if (vf_index >= first && vf_index <= last)
      *matches = true;
    if (*cursor == '\0')
      return DOCA_SUCCESS;
    if (*cursor != ',')
      return DOCA_ERROR_INVALID_VALUE;
    cursor++;
    skip_spaces(&cursor);
    if (*cursor == '\0')
      return DOCA_ERROR_INVALID_VALUE;
  }
  return DOCA_SUCCESS;
}

static doca_error_t open_scoped_vf_representors(
    struct doca_dev *parent,
    const char *vf_scope,
    struct doca_dev_rep ***opened_representors,
    size_t *opened_count) {
  struct doca_devinfo_rep **info_list = NULL;
  struct doca_dev_rep **representors = NULL;
  uint32_t info_count = 0;
  size_t vf_count = 0;
  size_t opened = 0;
  doca_error_t result;
  doca_error_t destroy_result;
  bool unused_match;

  result = vf_scope_matches(vf_scope, 0, &unused_match);
  if (result != DOCA_SUCCESS)
    return result;

  result = doca_devinfo_rep_create_list(parent, DOCA_DEVINFO_REP_FILTER_NET,
                                        &info_list, &info_count);
  if (result != DOCA_SUCCESS)
    return result;

  for (uint32_t i = 0; i < info_count; i++) {
    enum doca_pci_func_type type;
    uint32_t vf_index;
    bool selected;

    result = doca_devinfo_rep_get_pci_func_type(info_list[i], &type);
    if (result != DOCA_SUCCESS)
      goto destroy_list;
    if (type != DOCA_PCI_FUNC_TYPE_VF)
      continue;
    result = doca_devinfo_rep_get_vf_index(info_list[i], &vf_index);
    if (result != DOCA_SUCCESS)
      goto destroy_list;
    result = vf_scope_matches(vf_scope, vf_index, &selected);
    if (result != DOCA_SUCCESS)
      goto destroy_list;
    if (selected)
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
    uint32_t vf_index;
    bool selected;

    result = doca_devinfo_rep_get_pci_func_type(info_list[i], &type);
    if (result != DOCA_SUCCESS)
      goto destroy_list;
    if (type != DOCA_PCI_FUNC_TYPE_VF)
      continue;
    result = doca_devinfo_rep_get_vf_index(info_list[i], &vf_index);
    if (result != DOCA_SUCCESS)
      goto destroy_list;
    result = vf_scope_matches(vf_scope, vf_index, &selected);
    if (result != DOCA_SUCCESS)
      goto destroy_list;
    if (!selected)
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
  return switch_devices_open_scoped(pci_address, devargs, "all", devices);
}

doca_error_t switch_devices_open_scoped(const char *pci_address,
                                        const char *devargs,
                                        const char *vf_scope,
                                        struct switch_devices *devices) {
  doca_error_t result;

  if (pci_address == NULL || vf_scope == NULL || devices == NULL)
    return DOCA_ERROR_INVALID_VALUE;
  if (devices->parent != NULL)
    return DOCA_ERROR_BAD_STATE;

  devices->parent = open_pci_device(pci_address);
  if (devices->parent == NULL)
    return DOCA_ERROR_NOT_FOUND;

  result = open_scoped_vf_representors(
      devices->parent, vf_scope, &devices->representors,
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
