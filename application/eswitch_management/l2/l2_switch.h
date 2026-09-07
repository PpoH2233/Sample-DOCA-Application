#ifndef L2_SWITCH_H
#define L2_SWITCH_H
#include <stdint.h>
#include <doca_error.h>
struct eswitch_manager;
doca_error_t create_vswitch(struct eswitch_manager *, uint16_t);
doca_error_t attach_port(struct eswitch_manager *, uint16_t, uint16_t);
doca_error_t detach_port(struct eswitch_manager *, uint16_t, uint16_t);
doca_error_t delete_vswitch(struct eswitch_manager *, uint16_t);
#endif
