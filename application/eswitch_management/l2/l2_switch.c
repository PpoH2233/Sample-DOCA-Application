#include "l2_switch.h"
#include "../eswitch_manager.h"
#include "../router/router_control.h"

#include <stdio.h>

static struct managed_vswitch *find_vswitch(struct eswitch_manager *manager,
                                            uint16_t id) {
  for (size_t i = 0; i < ESWITCH_MAX_VSWITCHES; i++) {
    if (manager->switches[i].exists && manager->switches[i].id == id)
      return &manager->switches[i];
  }
  return NULL;
}

static int find_port_index(const struct eswitch_manager *manager,
                           uint16_t port_id) {
  for (uint16_t i = 0; i < manager->ports->count; i++) {
    if (manager->ports->items[i].ethernet->port_id == port_id)
      return i;
  }
  return -1;
}

doca_error_t create_vswitch(struct eswitch_manager *manager,
                                   uint16_t id) {
  if (id == 0)
    return DOCA_ERROR_INVALID_VALUE;
  if (find_vswitch(manager, id) != NULL)
    return DOCA_ERROR_ALREADY_EXIST;
  for (size_t i = 0; i < ESWITCH_MAX_VSWITCHES; i++) {
    if (!manager->switches[i].exists) {
      manager->switches[i].exists = true;
      manager->switches[i].id = id;
      printf("VSWITCH CREATE: id=%u\n", id);
      return DOCA_SUCCESS;
    }
  }
  return DOCA_ERROR_NO_MEMORY;
}

doca_error_t attach_port(struct eswitch_manager *manager,
                                uint16_t vswitch_id, uint16_t port_id) {
  struct managed_vswitch *vswitch = find_vswitch(manager, vswitch_id);
  int port_index;
  doca_error_t result;

  if (vswitch == NULL)
    return DOCA_ERROR_NOT_FOUND;
  port_index = find_port_index(manager, port_id);
  if (port_index < 0)
    return DOCA_ERROR_NOT_FOUND;
  if (manager->ports->items[port_index].ethernet->role ==
      ETHERNET_PORT_ROLE_SF_REPRESENTOR)
    return DOCA_ERROR_NOT_SUPPORTED;
  if (manager->port_owner[port_index] != 0 ||
      router_control_port_reserved(manager, (uint16_t)port_index))
    return DOCA_ERROR_IN_USE;

  /* Prepare the egress membership before opening ingress.  Until the
   * classifier is committed, packets cannot enter this vSwitch from port_id. */
  result = eswitch_pipeline_flood_add_port(manager->pipeline, vswitch_id,
                                           port_id, &vswitch->flood);
  if (result != DOCA_SUCCESS)
    return result;
  result = eswitch_pipeline_attach_port(manager->pipeline,
                                        (uint16_t)port_index, vswitch_id);
  if (result != DOCA_SUCCESS) {
    doca_error_t cleanup = eswitch_pipeline_flood_remove_port(
        manager->pipeline, port_id, &vswitch->flood);
    if (cleanup == DOCA_SUCCESS && vswitch->flood.member_count == 0)
      cleanup = eswitch_pipeline_destroy_flood_group(manager->pipeline,
                                                      &vswitch->flood);
    return cleanup == DOCA_SUCCESS ? result : cleanup;
  }
  manager->port_owner[port_index] = vswitch_id;
  printf("VSWITCH ATTACH: vs=%u dpdk-port=%u\n", vswitch_id, port_id);
  return DOCA_SUCCESS;
}

doca_error_t detach_port(struct eswitch_manager *manager,
                                uint16_t vswitch_id, uint16_t port_id) {
  struct managed_vswitch *vswitch = find_vswitch(manager, vswitch_id);
  int port_index;
  doca_error_t result;

  if (vswitch == NULL)
    return DOCA_ERROR_NOT_FOUND;
  port_index = find_port_index(manager, port_id);
  if (port_index < 0)
    return DOCA_ERROR_NOT_FOUND;
  if (manager->port_owner[port_index] != vswitch_id)
    return DOCA_ERROR_INVALID_VALUE;

  /* Close ingress first. If a later hardware mutation fails, restore the
   * classifier and flood member while ownership is still unchanged. */
  result = eswitch_pipeline_detach_port(manager->pipeline,
                                        (uint16_t)port_index);
  if (result != DOCA_SUCCESS)
    return result;
  result = eswitch_pipeline_flood_remove_port(manager->pipeline, port_id,
                                              &vswitch->flood);
  if (result != DOCA_SUCCESS) {
    doca_error_t rollback = eswitch_pipeline_attach_port(
        manager->pipeline, (uint16_t)port_index, vswitch_id);
    return rollback == DOCA_SUCCESS ? result : rollback;
  }
  result = eswitch_fdb_flush_port(&manager->fdb, vswitch_id, port_id,
                                  "port-detach");
  if (result != DOCA_SUCCESS) {
    doca_error_t rollback = eswitch_pipeline_flood_add_port(
        manager->pipeline, vswitch_id, port_id, &vswitch->flood);
    if (rollback == DOCA_SUCCESS)
      rollback = eswitch_pipeline_attach_port(
          manager->pipeline, (uint16_t)port_index, vswitch_id);
    return rollback == DOCA_SUCCESS ? result : rollback;
  }
  manager->port_owner[port_index] = 0;
  printf("VSWITCH DETACH: vs=%u dpdk-port=%u\n", vswitch_id, port_id);
  return DOCA_SUCCESS;
}

doca_error_t delete_vswitch(struct eswitch_manager *manager,
                                   uint16_t id) {
  struct managed_vswitch *vswitch = find_vswitch(manager, id);
  doca_error_t result;

  if (vswitch == NULL)
    return DOCA_ERROR_NOT_FOUND;
  result = eswitch_pipeline_sf_unbind_vswitch(manager->pipeline, id);
  if (result != DOCA_SUCCESS)
    return result;
  result = eswitch_fdb_flush_vswitch(&manager->fdb, id, "vs-delete");
  if (result != DOCA_SUCCESS)
    return result;
  for (uint16_t i = 0; i < manager->ports->count; i++) {
    if (manager->port_owner[i] != id)
      continue;
    result = eswitch_pipeline_detach_port(manager->pipeline, i);
    if (result != DOCA_SUCCESS)
      return result;
    manager->port_owner[i] = 0;
  }
  result = eswitch_pipeline_destroy_flood_group(manager->pipeline,
                                                &vswitch->flood);
  if (result != DOCA_SUCCESS)
    return result;
  printf("VSWITCH DELETE: id=%u\n", id);
  *vswitch = (struct managed_vswitch){0};
  return DOCA_SUCCESS;
}
