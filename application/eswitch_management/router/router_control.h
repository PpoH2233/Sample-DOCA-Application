#ifndef ROUTER_CONTROL_H
#define ROUTER_CONTROL_H
#include <doca_error.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
struct eswitch_manager;
bool router_control_port_reserved(const struct eswitch_manager *, uint16_t);
doca_error_t router_control_restore(struct eswitch_manager *);
doca_error_t router_control_command(struct eswitch_manager *, const char *, char *, size_t);
#endif
