#ifndef ESWITCH_STATE_H
#define ESWITCH_STATE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <doca_error.h>

#include "eswitch_config.h"

enum eswitch_state_port_kind {
  ESWITCH_STATE_PORT_PARENT,
  ESWITCH_STATE_PORT_REPRESENTOR,
};

struct eswitch_state_member {
  uint16_t vswitch_id;
  enum eswitch_state_port_kind kind;
  uint32_t host_index;
  uint32_t pf_index;
  uint32_t vf_index;
};

struct eswitch_state {
  uint16_t switch_ids[ESWITCH_MAX_VSWITCHES];
  size_t switch_count;
  struct eswitch_state_member *members;
  size_t member_count;
  size_t member_capacity;
};

doca_error_t eswitch_state_init(size_t member_capacity,
                                struct eswitch_state *state);
void eswitch_state_destroy(struct eswitch_state *state);
doca_error_t eswitch_state_add_switch(struct eswitch_state *state,
                                      uint16_t vswitch_id);
doca_error_t eswitch_state_add_member(
    struct eswitch_state *state, const struct eswitch_state_member *member);

/* ENOENT is not an error: exists is false and state remains empty. */
doca_error_t eswitch_state_load(const char *path, struct eswitch_state *state,
                                bool *exists);

/* Writes path.tmp.<pid>, fsyncs it, renames it over path, then fsyncs the
 * containing directory. The old file therefore survives an interrupted write. */
doca_error_t eswitch_state_save(const char *path,
                                const struct eswitch_state *state);

#endif /* ESWITCH_STATE_H */
