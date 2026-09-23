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
      .mode = ESWITCH_PORT_MODE_TRUNK,
      .vlan_id = 6,
      .vlan_last = 6,
      .vlan_extra_id = 800,
      .vlan_extra_last = 899,
  };
  struct eswitch_state_member representor = {
      .vswitch_id = 100,
      .kind = ESWITCH_STATE_PORT_REPRESENTOR,
      .host_index = 1,
      .pf_index = 0,
      .vf_index = 3,
      .mode = ESWITCH_PORT_MODE_ACCESS,
      .vlan_id = 6,
      .vlan_last = 6,
  };
  struct eswitch_state_member conflicting_parent = {
      .vswitch_id = 200,
      .kind = ESWITCH_STATE_PORT_PARENT,
      .mode = ESWITCH_PORT_MODE_TRUNK,
      .vlan_id = 6,
      .vlan_last = 6,
  };
  struct eswitch_state_member mixed_domain_representor = {
      .vswitch_id = 100,
      .kind = ESWITCH_STATE_PORT_REPRESENTOR,
      .host_index = 1,
      .pf_index = 0,
      .vf_index = 4,
      .mode = ESWITCH_PORT_MODE_ACCESS,
  };
  struct eswitch_state_member range_representor = {
      .vswitch_id = 300,
      .kind = ESWITCH_STATE_PORT_REPRESENTOR,
      .host_index = 1,
      .pf_index = 0,
      .vf_index = 4,
      .mode = ESWITCH_PORT_MODE_TRUNK,
      .vlan_id = 800,
      .vlan_last = 899,
  };
  bool exists = false;

  assert(mkdtemp(directory) != NULL);
  assert(snprintf(path, sizeof(path), "%s/eswitch.conf", directory) <
         (int)sizeof(path));
  assert(eswitch_state_init(8, &written) == DOCA_SUCCESS);
  assert(eswitch_state_add_switch(&written, 100) == DOCA_SUCCESS);
  assert(eswitch_state_add_switch(&written, 200) == DOCA_SUCCESS);
  assert(eswitch_state_add_switch(&written, 300) == DOCA_SUCCESS);
  assert(eswitch_state_add_member(&written, &parent) == DOCA_SUCCESS);
  assert(eswitch_state_add_member(&written, &representor) == DOCA_SUCCESS);
  assert(eswitch_state_add_member(&written, &conflicting_parent) !=
         DOCA_SUCCESS);
  assert(eswitch_state_add_member(&written, &mixed_domain_representor) !=
         DOCA_SUCCESS);
  assert(eswitch_state_add_member(&written, &range_representor) ==
         DOCA_SUCCESS);
  assert(eswitch_state_save(path, &written) == DOCA_SUCCESS);

  assert(eswitch_state_init(8, &loaded) == DOCA_SUCCESS);
  assert(eswitch_state_load(path, &loaded, &exists) == DOCA_SUCCESS);
  assert(exists);
  assert(loaded.switch_count == 3);
  assert(loaded.switch_ids[0] == 100 && loaded.switch_ids[1] == 200);
  assert(loaded.member_count == 3);
  assert(loaded.members[0].kind == ESWITCH_STATE_PORT_PARENT);
  assert(loaded.members[0].mode == ESWITCH_PORT_MODE_TRUNK);
  assert(loaded.members[0].vlan_id == 6);
  assert(loaded.members[0].vlan_last == 6);
  assert(loaded.members[0].vlan_extra_id == 800);
  assert(loaded.members[0].vlan_extra_last == 899);
  assert(loaded.members[1].kind == ESWITCH_STATE_PORT_REPRESENTOR);
  assert(loaded.members[1].mode == ESWITCH_PORT_MODE_ACCESS);
  assert(loaded.members[1].vlan_id == 6);
  assert(loaded.members[1].vlan_last == 6);
  assert(loaded.members[1].host_index == 1);
  assert(loaded.members[1].pf_index == 0);
  assert(loaded.members[1].vf_index == 3);
  assert(loaded.members[2].vlan_id == 800);
  assert(loaded.members[2].vlan_last == 899);

  eswitch_state_destroy(&loaded);
  eswitch_state_destroy(&written);

  /* Version 2 stored one VLAN value. Loading normalizes it to an exact
   * first==last interval before the next version-4 save. */
  {
    FILE *legacy = fopen(path, "w");
    assert(legacy != NULL);
    assert(fputs("version 2\nvswitch 10\nmember 10 parent trunk 6\n",
                 legacy) >= 0);
    assert(fclose(legacy) == 0);
  }
  exists = false;
  assert(eswitch_state_init(8, &loaded) == DOCA_SUCCESS);
  assert(eswitch_state_load(path, &loaded, &exists) == DOCA_SUCCESS);
  assert(exists && loaded.member_count == 1);
  assert(loaded.members[0].vlan_id == 6);
  assert(loaded.members[0].vlan_last == 6);
  eswitch_state_destroy(&loaded);

  /* Version 3 stored one inclusive interval and is migrated with an empty
   * second interval. */
  {
    FILE *legacy = fopen(path, "w");
    assert(legacy != NULL);
    assert(fputs("version 3\nvswitch 10\n"
                 "member 10 parent trunk 800 899\n",
                 legacy) >= 0);
    assert(fclose(legacy) == 0);
  }
  exists = false;
  assert(eswitch_state_init(8, &loaded) == DOCA_SUCCESS);
  assert(eswitch_state_load(path, &loaded, &exists) == DOCA_SUCCESS);
  assert(exists && loaded.member_count == 1);
  assert(loaded.members[0].vlan_id == 800);
  assert(loaded.members[0].vlan_last == 899);
  assert(loaded.members[0].vlan_extra_id == 0);
  assert(loaded.members[0].vlan_extra_last == 0);
  eswitch_state_destroy(&loaded);

  assert(unlink(path) == 0);
  assert(rmdir(directory) == 0);
  puts("eswitch_state_test: PASS");
  return EXIT_SUCCESS;
}
