#include "router_icmp.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static uint16_t checksum(const uint8_t *p, size_t length) {
  uint32_t sum = 0;
  while (length >= 2) {
    sum += (uint16_t)((uint16_t)p[0] << 8 | p[1]);
    p += 2;
    length -= 2;
  }
  if (length) sum += (uint16_t)p[0] << 8;
  while (sum >> 16) sum = (sum & 0xffffU) + (sum >> 16);
  return (uint16_t)~sum;
}

static void put16(uint8_t *p, uint16_t value) {
  p[0] = (uint8_t)(value >> 8); p[1] = (uint8_t)value;
}

int main(void) {
  struct router_config config;
  uint8_t request[98] = {0};
  uint8_t reply[128];
  uint8_t bad[98];
  const uint8_t vm[6] = {0xa6,0x94,0x27,0xfb,0x6c,0x38};
  const uint8_t rif[6] = {0x02,0x98,0x13,0x43,0x9d,0x40};

  router_config_init(&config);
  config.interface_count = 1;
  config.interfaces[0] = (struct router_interface){
      .vr_id=101,.vswitch_id=100,.attachment=ROUTER_VSWITCH,
      .has_address=true,.address=0xc0a80001};
  memcpy(config.interfaces[0].mac, rif, 6);

  memcpy(request,rif,6); memcpy(request+6,vm,6); request[12]=8;
  request[14]=0x45; put16(request+16,84); put16(request+18,0x1234);
  put16(request+20,0x4000); request[22]=64; request[23]=1;
  request[26]=192;request[27]=168;request[28]=0;request[29]=50;
  request[30]=192;request[31]=168;request[32]=0;request[33]=1;
  put16(request+24,checksum(request+14,20));
  request[34]=8; request[35]=0; put16(request+38,0x1234); put16(request+40,7);
  for(size_t i=42;i<98;i++) request[i]=(uint8_t)i;
  put16(request+36,checksum(request+34,64));

  assert(router_icmp_echo_reply(&config,100,request,sizeof(request),reply,sizeof(reply))==98);
  assert(!memcmp(reply,vm,6) && !memcmp(reply+6,rif,6));
  assert(reply[34]==0 && reply[35]==0);
  assert(!memcmp(reply+26,request+30,4) && !memcmp(reply+30,request+26,4));
  assert(checksum(reply+14,20)==0 && checksum(reply+34,64)==0);
  assert(!router_icmp_echo_reply(&config,200,request,sizeof(request),reply,sizeof(reply)));
  memcpy(bad,request,sizeof(bad)); bad[34]=3;
  assert(!router_icmp_echo_reply(&config,100,bad,sizeof(bad),reply,sizeof(reply)));
  memcpy(bad,request,sizeof(bad)); bad[20]=0x20;
  put16(bad+24,0); put16(bad+24,checksum(bad+14,20));
  assert(!router_icmp_echo_reply(&config,100,bad,sizeof(bad),reply,sizeof(reply)));
  assert(!router_icmp_echo_reply(&config,100,request,41,reply,sizeof(reply)));
  puts("PASS: ICMP echo reply, checksums, VR isolation and malformed packets");
  return 0;
}
