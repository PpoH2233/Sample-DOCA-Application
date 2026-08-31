#include "eswitch_state.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(void) {
  char directory[] = "/tmp/eswitch-state-test.XXXXXX";
  char path[256];
  struct eswitch_state written = {0};
  struct eswitch_state loaded = {0};
  struct eswitch_state_member parent = {
      .vswitch_id = 100,
      .kind = ESWITCH_STATE_PORT_PARENT,
  };
  struct eswitch_state_member representor = {
      .vswitch_id = 100,
      .kind = ESWITCH_STATE_PORT_REPRESENTOR,
      .host_index = 1,
      .pf_index = 0,
      .vf_index = 3,
  };
  bool exists = false;

  assert(mkdtemp(directory) != NULL);
  assert(snprintf(path, sizeof(path), "%s/eswitch.conf", directory) <
         (int)sizeof(path));
  assert(eswitch_state_init(8, &written) == DOCA_SUCCESS);
  assert(eswitch_state_add_switch(&written, 100) == DOCA_SUCCESS);
  assert(eswitch_state_add_switch(&written, 200) == DOCA_SUCCESS);
  assert(eswitch_state_add_member(&written, &parent) == DOCA_SUCCESS);
  assert(eswitch_state_add_member(&written, &representor) == DOCA_SUCCESS);
  assert(eswitch_state_save(path, &written) == DOCA_SUCCESS);

  assert(eswitch_state_init(8, &loaded) == DOCA_SUCCESS);
  assert(eswitch_state_load(path, &loaded, &exists) == DOCA_SUCCESS);
  assert(exists);
  assert(loaded.switch_count == 2);
  assert(loaded.switch_ids[0] == 100 && loaded.switch_ids[1] == 200);
  assert(loaded.member_count == 2);
  assert(loaded.members[0].kind == ESWITCH_STATE_PORT_PARENT);
  assert(loaded.members[1].kind == ESWITCH_STATE_PORT_REPRESENTOR);
  assert(loaded.members[1].host_index == 1);
  assert(loaded.members[1].pf_index == 0);
  assert(loaded.members[1].vf_index == 3);

  eswitch_state_destroy(&loaded);
  eswitch_state_destroy(&written);
  assert(unlink(path) == 0);
  assert(rmdir(directory) == 0);
  puts("eswitch_state_test: PASS");
  return EXIT_SUCCESS;
}
