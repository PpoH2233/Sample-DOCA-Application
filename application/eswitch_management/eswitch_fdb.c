#include "eswitch_fdb.h"

#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../ethernet_switch/switch_config.h"

#define NSEC_PER_SEC UINT64_C(1000000000)

static bool mac_equal(const struct rte_ether_addr *left,
                      const struct rte_ether_addr *right) {
  return rte_is_same_ether_addr(left, right) != 0;
}

static uint64_t monotonic_ns(void) {
  struct timespec now;

  if (clock_gettime(CLOCK_MONOTONIC, &now) != 0)
    return 0;
  return (uint64_t)now.tv_sec * NSEC_PER_SEC + (uint64_t)now.tv_nsec;
}

static const char *entry_state_name(enum eswitch_fdb_entry_state state) {
  switch (state) {
  case ESWITCH_FDB_ENTRY_ACTIVE:
    return "ACTIVE";
  case ESWITCH_FDB_ENTRY_QUERY_RETRY:
    return "QUERY_RETRY";
  case ESWITCH_FDB_ENTRY_DELETE_PENDING:
    return "DELETE_PENDING";
  }
  return "UNKNOWN";
}

static uint64_t retry_delay_ns(uint32_t retries) {
  uint32_t seconds;

  if (retries == 0)
    return 0;
  seconds = retries >= 6 ? SWITCH_FDB_RETRY_MAX_SECONDS
                         : (UINT32_C(1) << (retries - 1));
  if (seconds > SWITCH_FDB_RETRY_MAX_SECONDS)
    seconds = SWITCH_FDB_RETRY_MAX_SECONDS;
  return (uint64_t)seconds * NSEC_PER_SEC;
}

/* Log the first failures and then exponentially less often. The retry itself
 * still follows next_retry_ns; this only controls stderr volume. */
static bool should_log_retry(uint32_t retries) {
  return retries <= 3 || (retries & (retries - 1)) == 0;
}

static void log_retry(const char *operation,
                      const struct eswitch_fdb_entry *entry,
                      doca_error_t error, uint32_t retries) {
  const uint8_t *mac = entry->mac.addr_bytes;

  if (!should_log_retry(retries))
    return;
  fprintf(stderr,
          "FDB RETRY: operation=%s vs=%u "
          "mac=%02x:%02x:%02x:%02x:%02x:%02x port=%u error=%s retry=%u\n",
          operation, entry->vswitch_id, mac[0], mac[1], mac[2], mac[3],
          mac[4], mac[5], entry->learned_port_id,
          doca_error_get_descr(error), retries);
}

static struct eswitch_fdb_entry *find_entry(
    struct eswitch_fdb *fdb, uint16_t vswitch_id, uint16_t vlan_id,
    const struct rte_ether_addr *mac) {
  for (struct eswitch_fdb_entry *entry = fdb->head; entry != NULL;
       entry = entry->next) {
    if (entry->vswitch_id == vswitch_id && entry->vlan_id == vlan_id &&
        mac_equal(&entry->mac, mac))
      return entry;
  }
  return NULL;
}

static doca_error_t remove_at(struct eswitch_fdb *fdb,
                              struct eswitch_fdb_entry **cursor,
                              const char *reason);

static void log_entry(const char *operation,
                      const struct eswitch_fdb_entry *entry,
                      uint16_t old_port) {
  const uint8_t *mac = entry->mac.addr_bytes;

  printf("FDB %s: vs=%u mac=%02x:%02x:%02x:%02x:%02x:%02x ", operation,
         entry->vswitch_id, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  if (old_port != UINT16_MAX)
    printf("port=%u->%u\n", old_port, entry->learned_port_id);
  else
    printf("port=%u\n", entry->learned_port_id);
}

doca_error_t eswitch_fdb_init(struct eswitch_pipeline *pipeline,
                              size_t capacity, uint32_t aging_seconds,
                              struct eswitch_fdb *fdb) {
  if (pipeline == NULL || !pipeline->created || fdb == NULL || capacity == 0 ||
      aging_seconds == 0)
    return DOCA_ERROR_INVALID_VALUE;
  fdb->pipeline = pipeline;
  fdb->capacity = capacity;
  fdb->aging_ns = (uint64_t)aging_seconds * 1000000000ULL;
  return DOCA_SUCCESS;
}

doca_error_t eswitch_fdb_learn(struct eswitch_fdb *fdb,
                               uint16_t vswitch_id, uint16_t vlan_id,
                               const struct rte_ether_addr *source,
                               uint16_t ingress_port_id, uint64_t now_ns) {
  struct eswitch_fdb_entry *entry;
  doca_error_t result;

  if (fdb == NULL || fdb->pipeline == NULL || vswitch_id == 0 ||
      source == NULL)
    return DOCA_ERROR_INVALID_VALUE;
  if (!rte_is_valid_assigned_ether_addr(source))
    return DOCA_SUCCESS;
  entry = find_entry(fdb, vswitch_id, vlan_id, source);
  if (entry == NULL) {
    if (fdb->count >= fdb->capacity)
      return DOCA_ERROR_NO_MEMORY;
    entry = calloc(1, sizeof(*entry));
    if (entry == NULL)
      return DOCA_ERROR_NO_MEMORY;
    entry->vswitch_id = vswitch_id;
    entry->vlan_id = vlan_id;
    rte_ether_addr_copy(source, &entry->mac);
    entry->learned_port_id = ingress_port_id;
    entry->last_seen_ns = now_ns;
    entry->state = ESWITCH_FDB_ENTRY_ACTIVE;
    result = eswitch_pipeline_fdb_add(fdb->pipeline, vswitch_id, source,
                                      ingress_port_id, &entry->hardware);
    if (result != DOCA_SUCCESS) {
      if (entry->hardware.destination.entry == NULL &&
          entry->hardware.sources[0].entry == NULL &&
          entry->hardware.sources[1].entry == NULL) {
        free(entry);
      } else {
        /* Keep callback cookies alive if rollback itself failed. The daemon
         * will stop and eswitch_fdb_destroy() will retry the handles. */
        entry->next = fdb->head;
        fdb->head = entry;
        fdb->count++;
      }
      return result;
    }
    entry->next = fdb->head;
    fdb->head = entry;
    fdb->count++;
    log_entry("ADD", entry, UINT16_MAX);
    return DOCA_SUCCESS;
  }

  if (entry->learned_port_id == ingress_port_id) {
    entry->last_seen_ns = now_ns;
    return DOCA_SUCCESS;
  }
  {
    uint16_t old_port = entry->learned_port_id;
    result = eswitch_pipeline_fdb_move(fdb->pipeline, vswitch_id, source,
                                       old_port, ingress_port_id,
                                       &entry->hardware);
    if (result != DOCA_SUCCESS)
      return result;
    entry->learned_port_id = ingress_port_id;
    entry->last_seen_ns = now_ns;
    entry->last_source_packets = 0;
    log_entry("MOVE", entry, old_port);
  }
  return DOCA_SUCCESS;
}

doca_error_t eswitch_fdb_flush_port(struct eswitch_fdb *fdb,
                                    uint16_t vswitch_id, uint16_t port_id,
                                    const char *reason) {
  struct eswitch_fdb_entry **cursor;
  doca_error_t first_error = DOCA_SUCCESS;

  if (fdb == NULL || fdb->pipeline == NULL || vswitch_id == 0)
    return DOCA_ERROR_INVALID_VALUE;
  cursor = &fdb->head;
  while (*cursor != NULL) {
    struct eswitch_fdb_entry *entry = *cursor;
    doca_error_t result;

    if (entry->vswitch_id != vswitch_id ||
        entry->learned_port_id != port_id) {
      cursor = &entry->next;
      continue;
    }
    result = remove_at(fdb, cursor, reason == NULL ? "port-flush" : reason);
    if (result != DOCA_SUCCESS) {
      if (first_error == DOCA_SUCCESS)
        first_error = result;
      cursor = &entry->next;
    }
  }
  return first_error;
}

static doca_error_t remove_at(struct eswitch_fdb *fdb,
                              struct eswitch_fdb_entry **cursor,
                              const char *reason) {
  struct eswitch_fdb_entry *entry = *cursor;
  doca_error_t result;

  result = eswitch_pipeline_fdb_remove(fdb->pipeline, &entry->hardware);
  if (result != DOCA_SUCCESS)
    return result;
  printf("FDB DELETE: vs=%u port=%u reason=%s\n", entry->vswitch_id,
         entry->learned_port_id, reason);
  *cursor = entry->next;
  free(entry);
  fdb->count--;
  return DOCA_SUCCESS;
}

doca_error_t eswitch_fdb_age(struct eswitch_fdb *fdb, uint64_t now_ns) {
  struct eswitch_fdb_entry **cursor;
  doca_error_t first_error = DOCA_SUCCESS;

  if (fdb == NULL || fdb->pipeline == NULL)
    return DOCA_ERROR_INVALID_VALUE;
  cursor = &fdb->head;
  while (*cursor != NULL) {
    struct eswitch_fdb_entry *entry = *cursor;
    uint64_t packets = 0;
    doca_error_t result;

    if (entry->next_retry_ns != 0 && now_ns < entry->next_retry_ns) {
      cursor = &entry->next;
      continue;
    }

    if (entry->state == ESWITCH_FDB_ENTRY_DELETE_PENDING) {
      result = remove_at(fdb, cursor, "aged-retry");
      if (result == DOCA_SUCCESS)
        continue;
      entry->delete_retries++;
      entry->next_retry_ns = now_ns + retry_delay_ns(entry->delete_retries);
      log_retry("remove", entry, result, entry->delete_retries);
      if (result != DOCA_ERROR_IN_USE && first_error == DOCA_SUCCESS)
        first_error = result;
      cursor = &entry->next;
      continue;
    }

    result = eswitch_pipeline_fdb_query(&entry->hardware, &packets);
    if (result != DOCA_SUCCESS) {
      uint64_t failure_grace_ns =
          (uint64_t)SWITCH_FDB_QUERY_FAILURE_GRACE_SECONDS * NSEC_PER_SEC;

      entry->state = ESWITCH_FDB_ENTRY_QUERY_RETRY;
      entry->query_retries++;
      entry->next_retry_ns = now_ns + retry_delay_ns(entry->query_retries);
      log_retry("query", entry, result, entry->query_retries);

      /* A permanently busy counter must not pin an entry forever. Wait for
       * the normal aging interval plus a safety grace, then retire it. If it
       * was still active, its next packet simply takes the learning path. */
      if (now_ns - entry->last_seen_ns >= fdb->aging_ns + failure_grace_ns) {
        entry->state = ESWITCH_FDB_ENTRY_DELETE_PENDING;
        entry->next_retry_ns = now_ns;
        fprintf(stderr,
                "FDB DELETE PENDING: vs=%u port=%u reason=query-unavailable\n",
                entry->vswitch_id, entry->learned_port_id);
      }
      if (result != DOCA_ERROR_IN_USE && first_error == DOCA_SUCCESS)
        first_error = result;
      cursor = &entry->next;
      continue;
    }
    entry->state = ESWITCH_FDB_ENTRY_ACTIVE;
    entry->query_retries = 0;
    entry->next_retry_ns = 0;
    if (packets != entry->last_source_packets) {
      entry->last_source_packets = packets;
      entry->last_seen_ns = now_ns;
      cursor = &entry->next;
      continue;
    }
    if (now_ns - entry->last_seen_ns < fdb->aging_ns) {
      cursor = &entry->next;
      continue;
    }
    result = remove_at(fdb, cursor, "aged");
    if (result != DOCA_SUCCESS) {
      entry->state = ESWITCH_FDB_ENTRY_DELETE_PENDING;
      entry->delete_retries++;
      entry->next_retry_ns = now_ns + retry_delay_ns(entry->delete_retries);
      log_retry("remove", entry, result, entry->delete_retries);
      if (result != DOCA_ERROR_IN_USE && first_error == DOCA_SUCCESS)
        first_error = result;
      cursor = &entry->next;
    }
  }
  return first_error;
}

doca_error_t eswitch_fdb_flush_vswitch(struct eswitch_fdb *fdb,
                                       uint16_t vswitch_id,
                                       const char *reason) {
  struct eswitch_fdb_entry **cursor;
  doca_error_t first_error = DOCA_SUCCESS;

  if (fdb == NULL || fdb->pipeline == NULL || vswitch_id == 0)
    return DOCA_ERROR_INVALID_VALUE;
  cursor = &fdb->head;
  while (*cursor != NULL) {
    struct eswitch_fdb_entry *entry = *cursor;
    doca_error_t result;

    if (entry->vswitch_id != vswitch_id) {
      cursor = &entry->next;
      continue;
    }
    result = remove_at(fdb, cursor, reason == NULL ? "flush" : reason);
    if (result != DOCA_SUCCESS) {
      if (first_error == DOCA_SUCCESS)
        first_error = result;
      cursor = &entry->next;
    }
  }
  return first_error;
}

doca_error_t eswitch_fdb_destroy(struct eswitch_fdb *fdb) {
  doca_error_t first_error = DOCA_SUCCESS;

  if (fdb == NULL)
    return DOCA_ERROR_INVALID_VALUE;
  while (fdb->head != NULL) {
    doca_error_t result = remove_at(fdb, &fdb->head, "shutdown");
    if (result != DOCA_SUCCESS) {
      if (first_error == DOCA_SUCCESS)
        first_error = result;
      /* Hardware pipeline destruction will release any remaining handles. */
      break;
    }
  }
  if (first_error == DOCA_SUCCESS)
    *fdb = (struct eswitch_fdb){0};
  return first_error;
}

static size_t append_text(char *buffer, size_t size, size_t used,
                          const char *format, ...) {
  va_list args;
  int written;

  if (used >= size)
    return used;
  va_start(args, format);
  written = vsnprintf(buffer + used, size - used, format, args);
  va_end(args);
  if (written < 0)
    return used;
  if ((size_t)written >= size - used)
    return size;
  return used + (size_t)written;
}

size_t eswitch_fdb_format(const struct eswitch_fdb *fdb,
                          uint16_t vswitch_id, char *buffer,
                          size_t buffer_size) {
  size_t used = 0;
  size_t shown = 0;
  uint64_t now_ns = monotonic_ns();

  if (fdb == NULL || buffer == NULL || buffer_size == 0)
    return 0;
  used = append_text(buffer, buffer_size, used,
                     "FDB entries: total=%zu filter-vs=%s\n", fdb->count,
                     vswitch_id == 0 ? "all" : "selected");
  for (const struct eswitch_fdb_entry *entry = fdb->head; entry != NULL;
       entry = entry->next) {
    const uint8_t *m = entry->mac.addr_bytes;
    if (vswitch_id != 0 && entry->vswitch_id != vswitch_id)
      continue;
    used = append_text(
        buffer, buffer_size, used,
        "vs=%u mac=%02x:%02x:%02x:%02x:%02x:%02x port=%u packets=%" PRIu64
        " age=%" PRIu64 "s state=%s query-retries=%u delete-retries=%u\n",
        entry->vswitch_id, m[0], m[1], m[2], m[3], m[4], m[5],
        entry->learned_port_id, entry->last_source_packets,
        now_ns >= entry->last_seen_ns
            ? (now_ns - entry->last_seen_ns) / NSEC_PER_SEC
            : 0,
        entry_state_name(entry->state), entry->query_retries,
        entry->delete_retries);
    shown++;
  }
  if (shown == 0)
    used = append_text(buffer, buffer_size, used, "(empty)\n");
  return used;
}
