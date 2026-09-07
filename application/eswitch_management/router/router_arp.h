#ifndef ROUTER_ARP_H
#define ROUTER_ARP_H
#include "router.h"
/* Build a padded Ethernet/IPv4 ARP reply for an owned private gateway.
 * Input/output are wire bytes, no packed/unaligned structure access.
 * Returns 60 on reply, zero for malformed, non-request or unowned target. */
size_t router_arp_reply(const struct router_config *, uint16_t vswitch_id,
                        const uint8_t *, size_t, uint8_t *, size_t);
#endif
