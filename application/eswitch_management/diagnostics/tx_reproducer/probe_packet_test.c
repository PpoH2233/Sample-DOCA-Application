#include <assert.h>
#include "probe_packet.h"

int main(void)
{
    uint8_t vm[6], gw[6], ip[4], gateway[4], f[60];
    assert(probe_mac("7e:83:a5:77:11:06", vm) == 0);
    assert(probe_mac("02:00:00:65:00:01", gw) == 0);
    assert(inet_pton(AF_INET, "192.168.0.10", ip) == 1);
    assert(inet_pton(AF_INET, "192.168.0.1", gateway) == 1);
    probe_arp(f, vm, gw, ip, gateway);
    const uint8_t expected[60] = {
        0x7e,0x83,0xa5,0x77,0x11,0x06,0x02,0x00,0x00,0x65,0x00,0x01,
        0x08,0x06,0x00,0x01,0x08,0x00,0x06,0x04,0x00,0x02,
        0x02,0x00,0x00,0x65,0x00,0x01,0xc0,0xa8,0x00,0x01,
        0x7e,0x83,0xa5,0x77,0x11,0x06,0xc0,0xa8,0x00,0x0a
    };
    assert(memcmp(f, expected, sizeof(f)) == 0);
    const char *bad[] = {NULL, "", "7e:83:a5:77:11:06x", "7e:83:a5:77:11:0g",
        "ff:ff:ff:ff:ff:ff", "01:00:00:00:00:01", "00:00:00:00:00:00",
        "+2:00:00:00:00:01", " 2:00:00:00:00:01", "2:00:00:00:00:01"};
    for (unsigned int i = 0; i < sizeof(bad)/sizeof(bad[0]); ++i)
        assert(probe_mac(bad[i], vm) != 0);
    puts("probe packet: exact 60-byte ARP reply and invalid MAC tests passed");
    return 0;
}
