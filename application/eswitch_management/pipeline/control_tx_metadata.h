#ifndef ESWITCH_CONTROL_TX_METADATA_H
#define ESWITCH_CONTROL_TX_METADATA_H

#include <stdint.h>

/* Host order in mbufs, big endian in DOCA Flow matches. RX low bits hold
 * a valid ingress port, never UINT16_MAX. No vSwitch ID is reserved. */
static inline uint32_t eswitch_control_tx_metadata(uint16_t port_id) {
  return ((uint32_t)port_id << 16) | UINT32_C(0xffff);
}

#endif
