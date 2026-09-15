#ifndef ESWITCH_CONFIG_H
#define ESWITCH_CONFIG_H

#include <stdint.h>

#define ESWITCH_SOCKET_PATH "/run/eswitch-management/control.sock"
#define ESWITCH_STATE_PATH "/var/lib/eswitch-management/eswitch.conf"
#define ESWITCH_MAX_VSWITCHES 64U
#define ESWITCH_MAX_PERSISTED_MEMBERS 4096U
#define ESWITCH_RESPONSE_SIZE (128U * 1024U)
#define ESWITCH_HW_ROUTE_DEFAULT_CAPACITY 1024U
#define ESWITCH_HW_ROUTE_MIN_CAPACITY 64U
#define ESWITCH_HW_ACTIONS_MEM_MIN_SIZE (512U * 1024U)
#define ESWITCH_SF_ACTION_ENTRIES_PER_CONTEXT 2U

/* 0 means "not assigned" and is never a valid virtual-switch ID. */
#define ESWITCH_MIN_VSWITCH_ID 1U
#define ESWITCH_MAX_VSWITCH_ID UINT16_MAX

#endif /* ESWITCH_CONFIG_H */
