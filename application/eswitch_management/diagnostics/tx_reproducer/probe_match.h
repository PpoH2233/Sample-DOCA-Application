#ifndef TX_PROBE_MATCH_H
#define TX_PROBE_MATCH_H
#include <stdbool.h>
#include <string.h>
#include <doca_flow.h>

/* DOCA 3.4 implicit match: all-ones template fields take their value
 * from the entry. Constant template fields remain zero in the entry. */
static inline void probe_entry_match(struct doca_flow_match *entry,
                                     bool egress, bool hardware_arp)
{
    memset(entry, 0, sizeof(*entry));
    if (!egress && hardware_arp)
        memset(entry->outer.eth.dst_mac, 0xff, 6);
}
#endif
