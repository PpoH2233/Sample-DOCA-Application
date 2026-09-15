#ifndef ESW_ROUTER_NAT_CT_H
#define ESW_ROUTER_NAT_CT_H

#include <stdbool.h>
#include <stdint.h>

#include <doca_error.h>

struct router_nat_ct_runtime {
  uint32_t requested_capacity;
  bool requested;
  bool supported;
  bool initialized;
  bool degraded;
};

/* CT is process-global in DOCA 3.4.  This call belongs after doca_flow_init()
 * and before the first doca_flow_port_start(). */
doca_error_t router_nat_ct_init(bool requested, bool supported,
                                uint32_t capacity,
                                struct router_nat_ct_runtime *runtime);
void router_nat_ct_destroy(struct router_nat_ct_runtime *runtime);

#endif
