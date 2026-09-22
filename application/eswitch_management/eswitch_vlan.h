#ifndef ESWITCH_VLAN_H
#define ESWITCH_VLAN_H

#include <stdbool.h>
#include <stdint.h>

#define ESWITCH_VLAN_MIN 1U
#define ESWITCH_VLAN_MAX 4094U

enum eswitch_port_mode {
  ESWITCH_PORT_MODE_ACCESS = 0,
  ESWITCH_PORT_MODE_TRUNK = 1,
};

struct eswitch_port_membership {
  uint16_t vswitch_id;
  uint16_t port_index;
  uint16_t port_id;
  uint16_t vlan_id;
  enum eswitch_port_mode mode;
  bool active;
};

static inline bool eswitch_vlan_valid(uint16_t vlan_id) {
  return vlan_id >= ESWITCH_VLAN_MIN && vlan_id <= ESWITCH_VLAN_MAX;
}

#endif /* ESWITCH_VLAN_H */
