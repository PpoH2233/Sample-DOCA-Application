#ifndef ESWITCH_TX_PLAN_A_H
#define ESWITCH_TX_PLAN_A_H
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* Canonical decimal VS and lowercase colon-separated VM MAC; absent or
 * malformed selectors fail closed. No implicit first-ARP target selection. */
static inline bool tx_plan_a_selected(const char *selected_vs,
    const char *selected_vm, uint16_t vs, const uint8_t *vm) {
  char vs_text[6], vm_text[18];
  if (!selected_vs || !selected_vm || !vm || vs == 0) return false;
  snprintf(vs_text, sizeof(vs_text), "%u", vs);
  snprintf(vm_text, sizeof(vm_text), "%02x:%02x:%02x:%02x:%02x:%02x",
           vm[0], vm[1], vm[2], vm[3], vm[4], vm[5]);
  return !strcmp(selected_vs, vs_text) && !strcmp(selected_vm, vm_text);
}
#endif
