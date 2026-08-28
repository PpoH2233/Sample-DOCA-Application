#include "l2_fdb.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "l2_pipeline.h"

static bool mac_is_equal(const struct rte_ether_addr *left,
                         const struct rte_ether_addr *right) {
  return rte_is_same_ether_addr(left, right) != 0;
}

static void print_mac(const struct rte_ether_addr *mac) {
  printf("%02" PRIx8 ":%02" PRIx8 ":%02" PRIx8 ":%02" PRIx8
         ":%02" PRIx8 ":%02" PRIx8,
         mac->addr_bytes[0], mac->addr_bytes[1], mac->addr_bytes[2],
         mac->addr_bytes[3], mac->addr_bytes[4], mac->addr_bytes[5]);
}

static void free_entry(struct l2_fdb_entry *entry) {
  free(entry->destination_rules);
  free(entry);
}

static void print_fdb_delete(const struct l2_fdb_entry *entry,
                             const char *reason) {
  printf("FDB DELETE: mac=");
  print_mac(&entry->mac);
  printf(" port=%u reason=%s rules=%u\n", entry->learned_port_id, reason,
         entry->destination_rule_count + 1);
}

static struct l2_fdb_entry *find_entry(struct l2_fdb *fdb,
                                       uint16_t vlan_id,
                                       const struct rte_ether_addr *mac) {
  for (struct l2_fdb_entry *entry = fdb->head; entry != NULL;
       entry = entry->next) {
    if (entry->vlan_id == vlan_id && mac_is_equal(&entry->mac, mac))
      return entry;
  }
  return NULL;
}

doca_error_t l2_fdb_init(struct l2_pipeline *pipeline,
                         size_t capacity,
                         uint32_t aging_seconds,
                         struct l2_fdb *fdb) {
  if (pipeline == NULL || fdb == NULL || !pipeline->created ||
      capacity == 0 || aging_seconds == 0)
    return DOCA_ERROR_INVALID_VALUE;
  if (fdb->pipeline != NULL)
    return DOCA_ERROR_BAD_STATE;

  fdb->pipeline = pipeline;
  fdb->capacity = capacity;
  fdb->aging_ns = (uint64_t)aging_seconds * 1000000000ULL;
  return DOCA_SUCCESS;
}

doca_error_t l2_fdb_learn(struct l2_fdb *fdb,
                          uint16_t vlan_id,
                          const struct rte_ether_addr *source,
                          uint16_t ingress_port_id,
                          uint64_t now_ns) {
  struct l2_fdb_entry *entry;
  doca_error_t result;

  if (fdb == NULL || fdb->pipeline == NULL || source == NULL)
    return DOCA_ERROR_INVALID_VALUE;
  if (!rte_is_valid_assigned_ether_addr(source))
    return DOCA_SUCCESS;

  entry = find_entry(fdb, vlan_id, source);
  if (entry == NULL) {
    if (fdb->count == fdb->capacity)
      return DOCA_ERROR_NO_MEMORY;

    entry = calloc(1, sizeof(*entry));
    if (entry == NULL)
      return DOCA_ERROR_NO_MEMORY;

    rte_ether_addr_copy(source, &entry->mac);
    entry->vlan_id = vlan_id;
    entry->learned_port_id = ingress_port_id;
    entry->last_seen_ns = now_ns;
    entry->state = L2_FDB_PENDING;

    result = l2_pipeline_add_fdb_entry(fdb->pipeline, entry);
    if (result != DOCA_SUCCESS) {
      free(entry->destination_rules);
      free(entry);
      return result;
    }

    entry->state = L2_FDB_ACTIVE;
    entry->next = fdb->head;
    fdb->head = entry;
    fdb->count++;

    printf("FDB ADD: mac=");
    print_mac(source);
    printf(" port=%u rules=%u entries=%zu\n", ingress_port_id,
           entry->destination_rule_count + 1, fdb->count);
    return DOCA_SUCCESS;
  }

  /* Aging owns this record until every hardware handle is gone. */
  if (entry->state == L2_FDB_DELETING)
    return DOCA_SUCCESS;

  if (entry->learned_port_id == ingress_port_id) {
    entry->last_seen_ns = now_ns;
    return DOCA_SUCCESS;
  }

  entry->state = L2_FDB_MOVING;
  result = l2_pipeline_move_fdb_entry(fdb->pipeline, entry,
                                      ingress_port_id);
  if (result != DOCA_SUCCESS) {
    entry->state = L2_FDB_ACTIVE;
    return result;
  }

  printf("FDB UPDATE: mac=");
  print_mac(source);
  printf(" port=%u->%u rules=%u\n", entry->learned_port_id,
         ingress_port_id, entry->destination_rule_count + 1);
  entry->learned_port_id = ingress_port_id;
  entry->last_seen_ns = now_ns;
  entry->last_source_packets = 0;
  entry->state = L2_FDB_ACTIVE;
  return DOCA_SUCCESS;
}

doca_error_t l2_fdb_age(struct l2_fdb *fdb, uint64_t now_ns) {
  struct l2_fdb_entry **cursor;
  doca_error_t first_error = DOCA_SUCCESS;

  if (fdb == NULL || fdb->pipeline == NULL)
    return DOCA_ERROR_INVALID_VALUE;

  cursor = &fdb->head;
  while (*cursor != NULL) {
    struct l2_fdb_entry *entry = *cursor;
    uint64_t packet_count = 0;
    doca_error_t result;

    if (entry->state != L2_FDB_DELETING) {
      result = l2_pipeline_query_source_counter(entry, &packet_count);
      if (result != DOCA_SUCCESS) {
        if (first_error == DOCA_SUCCESS)
          first_error = result;
        cursor = &entry->next;
        continue;
      }

      if (packet_count != entry->last_source_packets) {
        entry->last_source_packets = packet_count;
        entry->last_seen_ns = now_ns;
        cursor = &entry->next;
        continue;
      }

      if (now_ns - entry->last_seen_ns < fdb->aging_ns) {
        cursor = &entry->next;
        continue;
      }

      entry->state = L2_FDB_DELETING;
    }

    result = l2_pipeline_remove_fdb_entry(fdb->pipeline, entry);
    if (result != DOCA_SUCCESS) {
      /* Keep DELETING so the next maintenance pass resumes at the first
       * non-NULL hardware handle instead of querying a removed source rule. */
      if (first_error == DOCA_SUCCESS)
        first_error = result;
      cursor = &entry->next;
      continue;
    }

    print_fdb_delete(entry, "aged");
    *cursor = entry->next;
    free_entry(entry);
    fdb->count--;
  }

  return first_error;
}

void l2_fdb_print(const struct l2_fdb *fdb) {
  if (fdb == NULL)
    return;

  printf("\nSoftware FDB (%zu/%zu entries):\n", fdb->count,
         fdb->capacity);
  for (const struct l2_fdb_entry *entry = fdb->head; entry != NULL;
       entry = entry->next) {
    printf("  mac=");
    print_mac(&entry->mac);
    printf(" port=%u packets=%" PRIu64 " state=%d\n",
           entry->learned_port_id, entry->last_source_packets,
           entry->state);
  }
}

doca_error_t l2_fdb_destroy(struct l2_fdb *fdb) {
  struct l2_fdb_entry **cursor;
  doca_error_t first_error = DOCA_SUCCESS;

  if (fdb == NULL)
    return DOCA_ERROR_INVALID_VALUE;

  cursor = &fdb->head;
  while (*cursor != NULL) {
    struct l2_fdb_entry *entry = *cursor;

    if (fdb->pipeline != NULL && fdb->pipeline->created &&
        (entry->source_rule.entry != NULL ||
         entry->destination_rules != NULL)) {
      doca_error_t result =
          l2_pipeline_remove_fdb_entry(fdb->pipeline, entry);
      if (result != DOCA_SUCCESS) {
        if (first_error == DOCA_SUCCESS)
          first_error = result;
        cursor = &entry->next;
        continue;
      }
    }

    print_fdb_delete(entry, "shutdown");
    *cursor = entry->next;
    free_entry(entry);
    fdb->count--;
  }

  if (fdb->head == NULL)
    *fdb = (struct l2_fdb){0};
  return first_error;
}

void l2_fdb_release(struct l2_fdb *fdb) {
  if (fdb == NULL)
    return;

  while (fdb->head != NULL) {
    struct l2_fdb_entry *entry = fdb->head;

    fdb->head = entry->next;
    free_entry(entry);
  }
  *fdb = (struct l2_fdb){0};
}
