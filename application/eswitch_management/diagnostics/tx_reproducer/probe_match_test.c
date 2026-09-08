#include <stdio.h>
#include "probe_match.h"

int main(void)
{
    struct doca_flow_match entry, expected;
    for (int egress = 0; egress <= 1; ++egress) {
        for (int arp = 0; arp <= 1; ++arp) {
            memset(&entry, 0xa5, sizeof(entry));
            memset(&expected, 0, sizeof(expected));
            if (!egress && arp)
                memset(expected.outer.eth.dst_mac, 0xff, 6);
            probe_entry_match(&entry, egress, arp);
            if (memcmp(&entry, &expected, sizeof(entry)) != 0) {
                fprintf(stderr, "entry match regression: egress=%d arp=%d\n", egress, arp);
                return 1;
            }
        }
    }
    puts("entry match: broadcast set per entry; other modes stay zero");
    return 0;
}
