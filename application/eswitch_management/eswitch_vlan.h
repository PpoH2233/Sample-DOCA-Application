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
  uint16_t vlan_extra_id;
  uint16_t vlan_extra_last;
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

static inline bool eswitch_vlan_extra_present(uint16_t first,
                                               uint16_t last) {
  return first != 0 || last != 0;
}

static inline bool eswitch_vlan_allowlist_valid(uint16_t first,
                                                uint16_t last,
                                                uint16_t extra_first,
                                                uint16_t extra_last) {
  if (!eswitch_vlan_range_valid(first, last))
    return false;
  if (!eswitch_vlan_extra_present(extra_first, extra_last))
    return true;
  return eswitch_vlan_range_valid(extra_first, extra_last) &&
         !eswitch_vlan_ranges_overlap(first, last, extra_first, extra_last);
}

static inline uint16_t eswitch_vlan_allowlist_size(uint16_t first,
                                                   uint16_t last,
                                                   uint16_t extra_first,
                                                   uint16_t extra_last) {
  uint16_t size;

  if (!eswitch_vlan_allowlist_valid(first, last, extra_first, extra_last))
    return 0;
  size = eswitch_vlan_range_size(first, last);
  if (eswitch_vlan_extra_present(extra_first, extra_last))
    size = (uint16_t)(size +
                      eswitch_vlan_range_size(extra_first, extra_last));
  return size;
}

static inline uint16_t eswitch_vlan_allowlist_at(uint16_t first,
                                                 uint16_t last,
                                                 uint16_t extra_first,
                                                 uint16_t extra_last,
                                                 uint16_t index) {
  uint16_t primary_size = eswitch_vlan_range_size(first, last);

  if (index < primary_size)
    return (uint16_t)(first + index);
  index = (uint16_t)(index - primary_size);
  if (eswitch_vlan_extra_present(extra_first, extra_last) &&
      index < eswitch_vlan_range_size(extra_first, extra_last))
    return (uint16_t)(extra_first + index);
  return 0;
}

static inline bool eswitch_vlan_allowlists_overlap(
    uint16_t left_first, uint16_t left_last, uint16_t left_extra_first,
    uint16_t left_extra_last, uint16_t right_first, uint16_t right_last,
    uint16_t right_extra_first, uint16_t right_extra_last) {
  if (eswitch_vlan_ranges_overlap(left_first, left_last, right_first,
                                  right_last))
    return true;
  if (eswitch_vlan_extra_present(left_extra_first, left_extra_last) &&
      eswitch_vlan_ranges_overlap(left_extra_first, left_extra_last,
                                  right_first, right_last))
    return true;
  if (eswitch_vlan_extra_present(right_extra_first, right_extra_last) &&
      eswitch_vlan_ranges_overlap(left_first, left_last, right_extra_first,
                                  right_extra_last))
    return true;
  return eswitch_vlan_extra_present(left_extra_first, left_extra_last) &&
         eswitch_vlan_extra_present(right_extra_first, right_extra_last) &&
         eswitch_vlan_ranges_overlap(left_extra_first, left_extra_last,
                                     right_extra_first, right_extra_last);
}

static inline bool eswitch_vlan_is_transparent_trunk(
    enum eswitch_port_mode mode, uint16_t first, uint16_t last,
    uint16_t extra_first, uint16_t extra_last) {
  return mode == ESWITCH_PORT_MODE_TRUNK &&
         (eswitch_vlan_is_range(first, last) ||
          eswitch_vlan_extra_present(extra_first, extra_last));
}

static inline bool eswitch_vlan_uses_tagged_domain(
    enum eswitch_port_mode mode, uint16_t first, uint16_t last,
    uint16_t extra_first, uint16_t extra_last) {
  return eswitch_vlan_is_transparent_trunk(mode, first, last, extra_first,
                                           extra_last) ||
         (mode == ESWITCH_PORT_MODE_ACCESS && first != 0);
}

#endif /* ESWITCH_VLAN_H */
