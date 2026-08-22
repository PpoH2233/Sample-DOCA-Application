#include "ethernet_ports.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <doca_dpdk.h>
#include <rte_ethdev.h>

struct vf_identity {
  char vuid[DOCA_DEVINFO_VUID_SIZE];
  uint32_t host_index;
  uint32_t pf_index;
  uint32_t vf_index;
};

static doca_error_t get_vf_identity(struct doca_dev_rep *representor,
                                    struct vf_identity *identity) {
  struct doca_devinfo_rep *rep_info;
  enum doca_pci_func_type function_type;
  doca_error_t result;

  if (representor == NULL || identity == NULL)
    return DOCA_ERROR_INVALID_VALUE;

  rep_info = doca_dev_rep_as_devinfo(representor);
  if (rep_info == NULL)
    return DOCA_ERROR_INVALID_VALUE;

  result = doca_devinfo_rep_get_pci_func_type(rep_info, &function_type);
  if (result != DOCA_SUCCESS)
    return result;

  if (function_type != DOCA_PCI_FUNC_TYPE_VF)
    return DOCA_ERROR_NOT_SUPPORTED;

  result = doca_devinfo_rep_get_vuid(rep_info, identity->vuid,
                                     sizeof(identity->vuid));
  if (result != DOCA_SUCCESS)
    return result;

  result = doca_devinfo_rep_get_host_index(rep_info, &identity->host_index);
  if (result != DOCA_SUCCESS)
    return result;

  result = doca_devinfo_rep_get_pf_index(rep_info, &identity->pf_index);
  if (result != DOCA_SUCCESS)
    return result;

  return doca_devinfo_rep_get_vf_index(rep_info, &identity->vf_index);
}

static bool vf_identity_is_equal(const struct vf_identity *left,
                                 const struct vf_identity *right) {
  return strcmp(left->vuid, right->vuid) == 0 &&
         left->host_index == right->host_index &&
         left->pf_index == right->pf_index &&
         left->vf_index == right->vf_index;
}

static void close_dpdk_ports(const uint16_t *port_ids, uint16_t port_count) {
  for (uint16_t i = 0; i < port_count; i++) {
    if (rte_eth_dev_is_valid_port(port_ids[i]))
      (void)rte_eth_dev_close(port_ids[i]);
  }
}

static doca_error_t map_representor_port(
    uint16_t port_id,
    struct doca_dev *parent_device,
    struct doca_dev_rep **representors,
    const struct vf_identity *input_identities,
    bool *is_mapped,
    size_t representor_count,
    struct ethernet_port *port,
    bool *is_representor) {
  struct doca_dev_rep *mapped_representor = NULL;
  struct vf_identity mapped_identity = {0};
  doca_error_t result;
  size_t match_index;

  *is_representor = false;

  result = doca_dpdk_open_dev_rep_by_port_id(port_id, parent_device,
                                              &mapped_representor);
  if (result == DOCA_ERROR_NOT_FOUND)
    return DOCA_SUCCESS;

  if (result != DOCA_SUCCESS)
    return result;

  *is_representor = true;

  result = get_vf_identity(mapped_representor, &mapped_identity);
  if (result != DOCA_SUCCESS)
    goto close_mapped_representor;

  for (match_index = 0; match_index < representor_count; match_index++) {
    if (!is_mapped[match_index] &&
        vf_identity_is_equal(&mapped_identity, &input_identities[match_index]))
      break;
  }

  if (match_index == representor_count) {
    result = DOCA_ERROR_NOT_FOUND;
    goto close_mapped_representor;
  }

  port->device = parent_device;
  port->representor = representors[match_index];
  port->port_id = port_id;
  port->role = ETHERNET_PORT_ROLE_REPRESENTOR;
  port->host_index = mapped_identity.host_index;
  port->pf_index = mapped_identity.pf_index;
  port->vf_index = mapped_identity.vf_index;
  port->is_probed = true;
  is_mapped[match_index] = true;

close_mapped_representor:
  {
    doca_error_t close_result = doca_dev_rep_close(mapped_representor);

    if (result == DOCA_SUCCESS && close_result != DOCA_SUCCESS)
      result = close_result;
  }

  return result;
}

doca_error_t ethernet_ports_probe_representors(
    struct doca_dev *parent_device,
    struct doca_dev_rep **representors,
    size_t representor_count,
    const char *devargs,
    struct ethernet_ports *ports) {
  struct vf_identity *input_identities = NULL;
  struct ethernet_port *mapped_ports = NULL;
  uint16_t *port_ids = NULL;
  bool *is_mapped = NULL;
  uint16_t port_count = 0;
  uint16_t parent_count = 0;
  uint8_t capability_supported = 0;
  bool probe_succeeded = false;
  doca_error_t result;

  if (parent_device == NULL || representors == NULL ||
      representor_count == 0 || ports == NULL)
    return DOCA_ERROR_INVALID_VALUE;

  if (ports->is_probed || ports->items != NULL)
    return DOCA_ERROR_BAD_STATE;

  if (representor_count + 1 > RTE_MAX_ETHPORTS ||
      representor_count + 1 > UINT16_MAX)
    return DOCA_ERROR_TOO_BIG;

  result = doca_dpdk_cap_is_rep_port_supported(
      doca_dev_as_devinfo(parent_device), &capability_supported);
  if (result != DOCA_SUCCESS) {
    fprintf(stderr, "Representor probe capability query failed: %s\n",
            doca_error_get_descr(result));
    return result;
  }

  if (capability_supported == 0) {
    fprintf(stderr, "Parent device does not support DPDK representor probe\n");
    return DOCA_ERROR_NOT_SUPPORTED;
  }

  input_identities = calloc(representor_count, sizeof(*input_identities));
  is_mapped = calloc(representor_count, sizeof(*is_mapped));
  port_ids = calloc(RTE_MAX_ETHPORTS, sizeof(*port_ids));
  if (input_identities == NULL || is_mapped == NULL || port_ids == NULL) {
    result = DOCA_ERROR_NO_MEMORY;
    goto cleanup;
  }

  for (size_t i = 0; i < representor_count; i++) {
    uint16_t dpdk_vf_index;

    result = get_vf_identity(representors[i], &input_identities[i]);
    if (result != DOCA_SUCCESS) {
      fprintf(stderr, "Failed to read identity of representor[%zu]: %s\n", i,
              doca_error_get_descr(result));
      goto cleanup;
    }

    result = doca_dpdk_get_rep_vf_index(
        doca_dev_rep_as_devinfo(representors[i]), &dpdk_vf_index);
    if (result != DOCA_SUCCESS) {
      fprintf(stderr,
              "Representor[%zu] host=%u pf=%u vf=%u cannot be used as a "
              "DPDK VF representor: %s\n",
              i, input_identities[i].host_index,
              input_identities[i].pf_index, input_identities[i].vf_index,
              doca_error_get_descr(result));
      goto cleanup;
    }

    printf("Input representor[%zu]: host=%u pf=%u vf=%u dpdk-vf=%u "
           "vuid=%s\n",
           i, input_identities[i].host_index, input_identities[i].pf_index,
           input_identities[i].vf_index, dpdk_vf_index,
           input_identities[i].vuid);

    for (size_t previous = 0; previous < i; previous++) {
      if (vf_identity_is_equal(&input_identities[i],
                               &input_identities[previous])) {
        fprintf(stderr,
                "Representor[%zu] duplicates representor[%zu] "
                "(host=%u pf=%u vf=%u vuid=%s)\n",
                i, previous, input_identities[i].host_index,
                input_identities[i].pf_index, input_identities[i].vf_index,
                input_identities[i].vuid);
        result = DOCA_ERROR_INVALID_VALUE;
        goto cleanup;
      }
    }
  }

  if (devargs == NULL)
    devargs = "";

  result = doca_dpdk_port_probe_with_representors(
      parent_device, devargs, representors, representor_count);
  if (result != DOCA_SUCCESS) {
    fprintf(stderr,
            "doca_dpdk_port_probe_with_representors rejected %zu "
            "representor(s), devargs=\"%s\": %s\n",
            representor_count, devargs, doca_error_get_descr(result));
    goto cleanup;
  }

  probe_succeeded = true;

  result = doca_dpdk_get_port_ids(parent_device, port_ids, RTE_MAX_ETHPORTS,
                                  &port_count);
  if (result != DOCA_SUCCESS)
    goto cleanup;

  if (port_count != representor_count + 1) {
    result = DOCA_ERROR_BAD_STATE;
    goto cleanup;
  }

  mapped_ports = calloc(port_count, sizeof(*mapped_ports));
  if (mapped_ports == NULL) {
    result = DOCA_ERROR_NO_MEMORY;
    goto cleanup;
  }

  for (uint16_t i = 0; i < port_count; i++) {
    struct doca_dev *mapped_device = NULL;
    bool is_representor;

    if (!rte_eth_dev_is_valid_port(port_ids[i])) {
      result = DOCA_ERROR_NOT_FOUND;
      goto cleanup;
    }

    result = map_representor_port(
        port_ids[i], parent_device, representors, input_identities, is_mapped,
        representor_count, &mapped_ports[i], &is_representor);
    if (result != DOCA_SUCCESS)
      goto cleanup;

    if (is_representor)
      continue;

    result = doca_dpdk_port_as_dev(port_ids[i], &mapped_device);
    if (result != DOCA_SUCCESS || mapped_device != parent_device) {
      if (result == DOCA_SUCCESS)
        result = DOCA_ERROR_NOT_FOUND;
      goto cleanup;
    }

    mapped_ports[i].device = parent_device;
    mapped_ports[i].representor = NULL;
    mapped_ports[i].port_id = port_ids[i];
    mapped_ports[i].role = ETHERNET_PORT_ROLE_PARENT;
    mapped_ports[i].is_probed = true;
    parent_count++;
  }

  if (parent_count != 1) {
    result = DOCA_ERROR_BAD_STATE;
    goto cleanup;
  }

  for (size_t i = 0; i < representor_count; i++) {
    if (!is_mapped[i]) {
      result = DOCA_ERROR_NOT_FOUND;
      goto cleanup;
    }
  }

  ports->parent_device = parent_device;
  ports->items = mapped_ports;
  ports->count = port_count;
  ports->is_probed = true;
  mapped_ports = NULL;
  result = DOCA_SUCCESS;

cleanup:
  if (result != DOCA_SUCCESS && probe_succeeded)
    close_dpdk_ports(port_ids, port_count);

  free(mapped_ports);
  free(port_ids);
  free(is_mapped);
  free(input_identities);
  return result;
}

doca_error_t ethernet_ports_close(struct ethernet_ports *ports) {
  doca_error_t result = DOCA_SUCCESS;

  if (ports == NULL)
    return DOCA_ERROR_INVALID_VALUE;

  if (!ports->is_probed)
    return DOCA_SUCCESS;

  for (uint16_t i = 0; i < ports->count; i++) {
    if (rte_eth_dev_is_valid_port(ports->items[i].port_id) &&
        rte_eth_dev_close(ports->items[i].port_id) != 0)
      result = DOCA_ERROR_DRIVER;
  }

  free(ports->items);
  *ports = (struct ethernet_ports){0};
  return result;
}
