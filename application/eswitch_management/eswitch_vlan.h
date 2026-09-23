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
  uint16_t vlan_last;
  enum eswitch_port_mode mode;
  bool active;
};

static inline bool eswitch_vlan_valid(uint16_t vlan_id) {
  return vlan_id >= ESWITCH_VLAN_MIN && vlan_id <= ESWITCH_VLAN_MAX;
}

static inline uint16_t eswitch_vlan_range_last(uint16_t first,
                                                uint16_t last) {
  return last == 0 ? first : last;
}

static inline bool eswitch_vlan_range_valid(uint16_t first, uint16_t last) {
  last = eswitch_vlan_range_last(first, last);
  return eswitch_vlan_valid(first) && eswitch_vlan_valid(last) &&
         first <= last;
}

static inline bool eswitch_vlan_is_range(uint16_t first, uint16_t last) {
  return eswitch_vlan_range_last(first, last) > first;
}

static inline uint16_t eswitch_vlan_range_size(uint16_t first,
                                                uint16_t last) {
  last = eswitch_vlan_range_last(first, last);
  return eswitch_vlan_range_valid(first, last)
             ? (uint16_t)(last - first + 1U)
             : 0;
}

static inline bool eswitch_vlan_ranges_overlap(uint16_t left_first,
                                                uint16_t left_last,
                                                uint16_t right_first,
                                                uint16_t right_last) {
  left_last = eswitch_vlan_range_last(left_first, left_last);
  right_last = eswitch_vlan_range_last(right_first, right_last);
  return left_first <= right_last && right_first <= left_last;
}

#endif /* ESWITCH_VLAN_H */
