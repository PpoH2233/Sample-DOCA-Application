#ifndef ROUTER_ICMP_H
#define ROUTER_ICMP_H

#include "router.h"

/* Build an Ethernet/IPv4 ICMP echo reply for an owned private gateway.
 * Input/output are wire bytes. Returns the padded frame length on success and
 * zero for malformed, fragmented, non-echo or non-local packets. */
size_t router_icmp_echo_reply(const struct router_config *config,
                              uint16_t vswitch_id, const uint8_t *packet,
                              size_t length, uint8_t *output,
                              size_t capacity);

#endif /* ROUTER_ICMP_H */
