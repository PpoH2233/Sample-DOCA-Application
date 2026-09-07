#include "router_arp.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void) {
  struct router_config c;
  router_config_init(&c);
  c.interface_count=2;
  c.interfaces[0]=(struct router_interface){.vr_id=101,.vswitch_id=100,
    .attachment=ROUTER_VSWITCH,.has_address=true,.address=0xc0a80001,
    .mac={2,0,0,0x65,0,1}};
  c.interfaces[1]=c.interfaces[0];
  c.interfaces[1].vr_id=102;c.interfaces[1].vswitch_id=200;c.interfaces[1].mac[3]=0x66;
  uint8_t req[60]={
    255,255,255,255,255,255, 0x7e,0x83,0xa5,0x77,0x11,6, 8,6,
    0,1,8,0,6,4,0,1, 0x7e,0x83,0xa5,0x77,0x11,6, 192,168,0,10,
    0,0,0,0,0,0, 192,168,0,1};
  uint8_t reply[60],bad[60];
  assert(router_arp_reply(&c,100,req,42,reply,60)==60);
  assert(!memcmp(reply,req+6,6));
  assert(!memcmp(reply+6,c.interfaces[0].mac,6));
  assert(reply[20]==0 && reply[21]==2);
  assert(!memcmp(reply+22,c.interfaces[0].mac,6));
  assert(!memcmp(reply+28,req+38,4));
  assert(!memcmp(reply+32,req+22,6));
  assert(!memcmp(reply+38,req+28,4));
  for(unsigned i=42;i<60;i++) assert(reply[i]==0);
  assert(router_arp_reply(&c,200,req,60,reply,60)==60);
  assert(reply[9]==0x66); /* same IP in another VR selects its own MAC */
  assert(!router_arp_reply(&c,300,req,60,reply,60));
  for(size_t n=0;n<42;n++) assert(!router_arp_reply(&c,100,req,n,reply,60));
  assert(!router_arp_reply(&c,100,req,60,reply,59));
  unsigned offsets[]={12,13,14,15,16,17,18,19,20,21,38};
  for(size_t i=0;i<sizeof(offsets)/sizeof(offsets[0]);i++) {
    memcpy(bad,req,60);bad[offsets[i]]^=1;
    assert(!router_arp_reply(&c,100,bad,60,reply,60));
  }
  memcpy(bad,req,60);bad[6]^=2; /* Ethernet/ARP source mismatch */
  assert(!router_arp_reply(&c,100,bad,60,reply,60));
  memcpy(bad,req,60);bad[6]|=1;bad[22]|=1;
  assert(!router_arp_reply(&c,100,bad,60,reply,60));
  memcpy(bad,req,60);memset(bad+6,0,6);memset(bad+22,0,6);
  assert(!router_arp_reply(&c,100,bad,60,reply,60));
  memcpy(bad,req,60);memset(bad+28,0,4); /* address conflict detection probe */
  assert(router_arp_reply(&c,100,bad,60,reply,60)==60);
  memcpy(bad,req,60);memcpy(bad+28,req+38,4);
  assert(!router_arp_reply(&c,100,bad,60,reply,60));
  memcpy(bad,req,60);memcpy(bad,c.interfaces[0].mac,6); /* unicast refresh */
  assert(router_arp_reply(&c,100,bad,60,reply,60)==60);
  bad[5]^=0x20;
  assert(!router_arp_reply(&c,100,bad,60,reply,60));
  c.interfaces[0].has_address=false;
  assert(!router_arp_reply(&c,100,req,60,reply,60));
  c.interfaces[0].has_address=true;c.interfaces[0].attachment=ROUTER_PORT;
  assert(!router_arp_reply(&c,100,req,60,reply,60));
  puts("PASS: ARP wire reply, VR isolation, malformed/truncated packets, probes and address removal");
  return 0;
}
