#include "l2_switch.h"
#include "../eswitch_manager.h"
#include "../router/router_control.h"

#include <stdio.h>
#include <stdlib.h>

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

static struct eswitch_port_membership *find_membership(
    struct eswitch_manager *manager, uint16_t vswitch_id, uint16_t port_id) {
  for (size_t i = 0; i < ESWITCH_MAX_PERSISTED_MEMBERS; i++) {
    struct eswitch_port_membership *member = &manager->memberships[i];
    if (member->active && member->vswitch_id == vswitch_id &&
        member->port_id == port_id)
      return member;
  }
  return NULL;
}

bool eswitch_port_in_vswitch(const struct eswitch_manager *manager,
                             uint16_t vswitch_id, uint16_t port_id) {
  for (size_t i = 0; i < ESWITCH_MAX_PERSISTED_MEMBERS; i++) {
    const struct eswitch_port_membership *member = &manager->memberships[i];
    if (member->active && member->vswitch_id == vswitch_id &&
        member->port_id == port_id)
      return true;
  }
  return false;
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
                         uint16_t vswitch_id, uint16_t port_id,
                         enum eswitch_port_mode mode, uint16_t vlan_id,
                         uint16_t vlan_last) {
  struct managed_vswitch *vswitch = find_vswitch(manager, vswitch_id);
  struct eswitch_port_membership *membership = NULL;
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
  if ((mode == ESWITCH_PORT_MODE_TRUNK &&
       !eswitch_vlan_range_valid(vlan_id, vlan_last)) ||
      (mode == ESWITCH_PORT_MODE_ACCESS &&
       (vlan_id != 0 || vlan_last != 0)))
    return DOCA_ERROR_INVALID_VALUE;
  if (router_control_port_reserved(manager, (uint16_t)port_index))
    return DOCA_ERROR_IN_USE;
  if (eswitch_vlan_is_range(vlan_id, vlan_last) && manager->router != NULL &&
      router_switch_reserved(manager->router, vswitch_id))
    return DOCA_ERROR_IN_USE;
  size_t vlan_rule_count = mode == ESWITCH_PORT_MODE_TRUNK
                               ? eswitch_vlan_range_size(vlan_id, vlan_last)
                               : 1U;
  for (size_t i = 0; i < ESWITCH_MAX_PERSISTED_MEMBERS; i++) {
    struct eswitch_port_membership *candidate = &manager->memberships[i];
    if (!candidate->active) {
      if (membership == NULL)
        membership = candidate;
      continue;
    }
    if (candidate->mode == ESWITCH_PORT_MODE_TRUNK)
      vlan_rule_count += eswitch_vlan_range_size(candidate->vlan_id,
                                                  candidate->vlan_last);
    else
      vlan_rule_count++;
    if (candidate->vswitch_id == vswitch_id &&
        (eswitch_vlan_is_range(candidate->vlan_id, candidate->vlan_last) ||
         eswitch_vlan_is_range(vlan_id, vlan_last))) {
      /* A transparent range vSwitch keeps the original tag end to end. It
       * cannot share a broadcast domain with a pop/push or access member. */
      if (!eswitch_vlan_is_range(candidate->vlan_id, candidate->vlan_last) ||
          !eswitch_vlan_is_range(vlan_id, vlan_last))
        return DOCA_ERROR_IN_USE;
    }
    if (candidate->port_index != (uint16_t)port_index)
      continue;
    if (candidate->vswitch_id == vswitch_id ||
        candidate->mode == ESWITCH_PORT_MODE_ACCESS ||
        mode == ESWITCH_PORT_MODE_ACCESS ||
        eswitch_vlan_ranges_overlap(candidate->vlan_id,
                                    candidate->vlan_last, vlan_id,
                                    vlan_last))
      return DOCA_ERROR_IN_USE;
  }
  if (membership == NULL ||
      manager->membership_count >= ESWITCH_MAX_VLAN_MEMBERSHIPS ||
      vlan_rule_count > ESWITCH_MAX_VLAN_MEMBERSHIPS)
    return DOCA_ERROR_NO_MEMORY;

  result = eswitch_pipeline_attach_port(manager->pipeline,
                                        (uint16_t)port_index, vswitch_id,
                                        mode, vlan_id, vlan_last);
  if (result != DOCA_SUCCESS)
    return result;
  result = eswitch_pipeline_flood_add_port(manager->pipeline, vswitch_id,
                                           port_id, &vswitch->flood);
  if (result != DOCA_SUCCESS) {
    doca_error_t cleanup = eswitch_pipeline_detach_vswitch_port(
        manager->pipeline, (uint16_t)port_index, vswitch_id);
    if (cleanup == DOCA_SUCCESS)
      cleanup = eswitch_pipeline_release_vswitch_port(
          manager->pipeline, (uint16_t)port_index, vswitch_id);
    return cleanup == DOCA_SUCCESS ? result : cleanup;
  }
  *membership = (struct eswitch_port_membership){
      .vswitch_id = vswitch_id,
      .port_index = (uint16_t)port_index,
      .port_id = port_id,
      .vlan_id = vlan_id,
      .vlan_last = eswitch_vlan_range_last(vlan_id, vlan_last),
      .mode = mode,
      .active = true,
  };
  manager->membership_count++;
  if (mode == ESWITCH_PORT_MODE_ACCESS)
    manager->port_owner[port_index] = vswitch_id;
  if (eswitch_vlan_is_range(vlan_id, vlan_last))
    printf("VSWITCH ATTACH: vs=%u dpdk-port=%u mode=trunk vlan=%u-%u "
           "path=transparent\n", vswitch_id, port_id, vlan_id,
           eswitch_vlan_range_last(vlan_id, vlan_last));
  else
    printf("VSWITCH ATTACH: vs=%u dpdk-port=%u mode=%s vlan=%u\n", vswitch_id,
           port_id, mode == ESWITCH_PORT_MODE_TRUNK ? "trunk" : "access",
           vlan_id);
  return DOCA_SUCCESS;
}

doca_error_t detach_port(struct eswitch_manager *manager,
                                uint16_t vswitch_id, uint16_t port_id) {
  struct managed_vswitch *vswitch = find_vswitch(manager, vswitch_id);
  struct eswitch_port_membership *membership;
  struct eswitch_port_membership saved_membership;
  struct router_neighbor_table *neighbor_backup = NULL;
  int port_index;
  doca_error_t result;

  if (vswitch == NULL)
    return DOCA_ERROR_NOT_FOUND;
  port_index = find_port_index(manager, port_id);
  if (port_index < 0)
    return DOCA_ERROR_NOT_FOUND;
  membership = find_membership(manager, vswitch_id, port_id);
  if (membership == NULL)
    return DOCA_ERROR_INVALID_VALUE;
  saved_membership = *membership;
  neighbor_backup = malloc(sizeof(*neighbor_backup));
  if (neighbor_backup == NULL)
    return DOCA_ERROR_NO_MEMORY;
  *neighbor_backup = manager->neighbors;

  /* Close ingress first. If a later hardware mutation fails, restore the
   * classifier and flood member while ownership is still unchanged. */
  result = eswitch_pipeline_detach_vswitch_port(
      manager->pipeline, (uint16_t)port_index, vswitch_id);
  if (result != DOCA_SUCCESS) {
    free(neighbor_backup);
    return result;
  }
  result = eswitch_pipeline_flood_remove_port(manager->pipeline, port_id,
                                              &vswitch->flood);
  if (result != DOCA_SUCCESS) {
    doca_error_t rollback = eswitch_pipeline_attach_port(
        manager->pipeline, (uint16_t)port_index, vswitch_id,
        saved_membership.mode, saved_membership.vlan_id,
        saved_membership.vlan_last);
    free(neighbor_backup);
    return rollback == DOCA_SUCCESS ? result : rollback;
  }
  result = eswitch_fdb_flush_port(&manager->fdb, vswitch_id, port_id,
                                  "port-detach");
  if (result != DOCA_SUCCESS) {
    doca_error_t rollback = eswitch_pipeline_attach_port(
        manager->pipeline, (uint16_t)port_index, vswitch_id,
        saved_membership.mode, saved_membership.vlan_id,
        saved_membership.vlan_last);
    if (rollback == DOCA_SUCCESS)
      rollback = eswitch_pipeline_flood_add_port(
          manager->pipeline, vswitch_id, port_id, &vswitch->flood);
    free(neighbor_backup);
    return rollback == DOCA_SUCCESS ? result : rollback;
  }
  /* Remove routes that reference the membership-specific egress gate before
   * destroying that gate. Releasing the gate first leaves hardware entries
   * pointing at it and produces Resource busy/stale forwarding failures. */
  if (membership->mode == ESWITCH_PORT_MODE_ACCESS)
    manager->port_owner[port_index] = 0;
  *membership = (struct eswitch_port_membership){0};
  manager->membership_count--;
  router_neighbor_invalidate_port(&manager->neighbors, port_id);
  result = eswitch_manager_hw_routes_sync(manager, manager->router);
  if (result != DOCA_SUCCESS) {
    doca_error_t rollback;
    manager->neighbors = *neighbor_backup;
    *membership = saved_membership;
    manager->membership_count++;
    if (membership->mode == ESWITCH_PORT_MODE_ACCESS)
      manager->port_owner[port_index] = vswitch_id;
    rollback = eswitch_pipeline_attach_port(
        manager->pipeline, (uint16_t)port_index, vswitch_id,
        saved_membership.mode, saved_membership.vlan_id,
        saved_membership.vlan_last);
    if (rollback == DOCA_SUCCESS)
      rollback = eswitch_pipeline_flood_add_port(
          manager->pipeline, vswitch_id, port_id, &vswitch->flood);
    free(neighbor_backup);
    return rollback == DOCA_SUCCESS ? result : rollback;
  }
  result = eswitch_pipeline_release_vswitch_port(
      manager->pipeline, (uint16_t)port_index, vswitch_id);
  if (result != DOCA_SUCCESS) {
    doca_error_t rollback;
    manager->neighbors = *neighbor_backup;
    *membership = saved_membership;
    manager->membership_count++;
    if (membership->mode == ESWITCH_PORT_MODE_ACCESS)
      manager->port_owner[port_index] = vswitch_id;
    rollback = eswitch_pipeline_attach_port(
        manager->pipeline, (uint16_t)port_index, vswitch_id,
        saved_membership.mode, saved_membership.vlan_id,
        saved_membership.vlan_last);
    if (rollback == DOCA_SUCCESS)
      rollback = eswitch_pipeline_flood_add_port(
          manager->pipeline, vswitch_id, port_id, &vswitch->flood);
    if (rollback == DOCA_SUCCESS)
      rollback = eswitch_manager_hw_routes_sync(manager, manager->router);
    free(neighbor_backup);
    return rollback == DOCA_SUCCESS ? result : rollback;
  }
  free(neighbor_backup);
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
  for (size_t i = 0; i < ESWITCH_MAX_PERSISTED_MEMBERS; i++) {
    struct eswitch_port_membership *member = &manager->memberships[i];
    if (!member->active || member->vswitch_id != id)
      continue;
    result = eswitch_pipeline_detach_vswitch_port(
        manager->pipeline, member->port_index, id);
    if (result != DOCA_SUCCESS)
      return result;
  }
  result = eswitch_pipeline_destroy_flood_group(manager->pipeline,
                                                &vswitch->flood);
  if (result != DOCA_SUCCESS)
    return result;
  for (size_t i = 0; i < ESWITCH_MAX_PERSISTED_MEMBERS; i++) {
    struct eswitch_port_membership *member = &manager->memberships[i];
    if (!member->active || member->vswitch_id != id)
      continue;
    result = eswitch_pipeline_release_vswitch_port(
        manager->pipeline, member->port_index, id);
    if (result != DOCA_SUCCESS)
      return result;
    if (member->mode == ESWITCH_PORT_MODE_ACCESS)
      manager->port_owner[member->port_index] = 0;
    *member = (struct eswitch_port_membership){0};
    manager->membership_count--;
  }
  printf("VSWITCH DELETE: id=%u\n", id);
  *vswitch = (struct managed_vswitch){0};
  return DOCA_SUCCESS;
}
