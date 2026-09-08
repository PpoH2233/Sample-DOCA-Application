#ifndef TX_PROBE_PACKET_H
#define TX_PROBE_PACKET_H

#include <arpa/inet.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* Wire bytes only: no struct padding, host byte order or DPDK dependency. */
static inline int probe_mac(const char *s, uint8_t mac[6])
{
    unsigned int b[6];
    int n = 0;
    if (!s || strlen(s) != 17 ||
        sscanf(s, "%2x:%2x:%2x:%2x:%2x:%2x%n",
               &b[0], &b[1], &b[2], &b[3], &b[4], &b[5], &n) != 6 || n != 17)
        return -1;
    for (int i = 0; i < 6; ++i) {
        for (int j = 0; j < 2; ++j) {
            char c = s[3*i+j];
            if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
                  (c >= 'A' && c <= 'F'))) return -1;
        }
        mac[i] = (uint8_t)b[i];
    }
    static const uint8_t zero[6] = {0};
    return (mac[0] & 1) || memcmp(mac, zero, 6) == 0 ? -1 : 0;
}

static inline void probe_arp(uint8_t frame[60], const uint8_t vm[6],
                             const uint8_t gateway[6], const uint8_t vm_ip[4],
                             const uint8_t gateway_ip[4])
{
    static const uint8_t arp_header[10] =
        {0x08, 0x06, 0x00, 0x01, 0x08, 0x00, 0x06, 0x04, 0x00, 0x02};
    memset(frame, 0, 60);
    memcpy(frame, vm, 6);
    memcpy(frame + 6, gateway, 6);
    memcpy(frame + 12, arp_header, 10);
    memcpy(frame + 22, gateway, 6);
    memcpy(frame + 28, gateway_ip, 4);
    memcpy(frame + 32, vm, 6);
    memcpy(frame + 38, vm_ip, 4);
}
#endif
