#include "control_tx_metadata.h"
#include <assert.h>
#include <stdio.h>

int main(void) {
  assert(eswitch_control_tx_metadata(2) == UINT32_C(0x0002ffff));
  for (uint32_t port = 0; port < UINT16_MAX; port++) {
    uint32_t tx = eswitch_control_tx_metadata((uint16_t)port);
    assert((tx >> 16) == port);
    assert((tx & UINT16_MAX) == UINT16_MAX);
    /* Every valid RX port differs from the TX namespace, regardless of VS. */
    assert((tx & UINT16_MAX) != port);
    if (port != 0)
      assert(tx != eswitch_control_tx_metadata((uint16_t)(port - 1)));
  }
  for (uint32_t vs = 0; vs <= UINT16_MAX; vs++) {
    uint32_t rx = (vs << 16) | 2;
    assert(rx != eswitch_control_tx_metadata(2));
  }
  puts("PASS: TX metadata encoding, unique targets, disjoint RX namespace");
  return 0;
}
