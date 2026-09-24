#include "router_forward.h"
#include "router_icmp.h"
#include "router_nat.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint16_t read16(const uint8_t *p) {
  return (uint16_t)((uint16_t)p[0] << 8 | p[1]);
}

static uint32_t read32(const uint8_t *p) {
  return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 |
         (uint32_t)p[2] << 8 | p[3];
}

static void write16(uint8_t *p, uint16_t value) {
  p[0] = (uint8_t)(value >> 8);
  p[1] = (uint8_t)value;
}

static void write32(uint8_t *p, uint32_t value) {
  p[0] = (uint8_t)(value >> 24);
  p[1] = (uint8_t)(value >> 16);
  p[2] = (uint8_t)(value >> 8);
  p[3] = (uint8_t)value;
}

static uint16_t checksum(const uint8_t *p, size_t length) {
  uint32_t sum = 0;

  while (length >= 2) {
    sum += read16(p);
    p += 2;
    length -= 2;
  }
  if (length != 0)
    sum += (uint16_t)*p << 8;
  while (sum >> 16)
    sum = (sum & UINT16_MAX) + (sum >> 16);
  return (uint16_t)~sum;
}

static size_t make_echo_request(uint8_t frame[98]) {
  memset(frame, 0, 98);
  memset(frame, 0xaa, 6);
  memset(frame + 6, 0xbb, 6);
  frame[12] = 0x08;
  frame[13] = 0x00;
  frame[14] = 0x45;
  write16(frame + 16, 84);
  write16(frame + 18, 0x1234);
  write16(frame + 20, 0x4000);
  frame[22] = 64;
  frame[23] = 1;
  write32(frame + 26, 0xc0a8640aU); /* 192.168.100.10 */
  write32(frame + 30, 0xa1f6060fU); /* 161.246.6.15 */
  write16(frame + 24, checksum(frame + 14, 20));
  frame[34] = 8;
  write16(frame + 38, 0x4321);
  write16(frame + 40, 1);
  for (size_t i = 42; i < 98; i++)
    frame[i] = (uint8_t)i;
  write16(frame + 36, checksum(frame + 34, 64));
  return 98;
}

int main(void) {
  struct router_config config;
  struct router_nat_table *nat = calloc(1, sizeof(*nat));
  struct router_nat_policy policy = {
      .vr_id = 1, .interface_id = 2,
      .port_first = 20000, .port_last = 60999};
  struct router_nat_inside inside = {
      .vswitch_id = 100, .interface_id = 1, .port_id = 1,
      .mac = {0xa6, 0x94, 0x27, 0xfb, 0x6c, 0x38}};
  const struct router_nat_session *session = NULL;
  const struct router_nat_session *reverse_session = NULL;
  uint8_t request[98], translated[128], wire_request[128];
  uint8_t wire_reply[128], reverse[128];
  const uint8_t vr1_mac[6] = {0x02, 0, 0, 1, 0, 2};
  const uint8_t vr2_mac[6] = {0x02, 0, 0, 2, 0, 5};
  size_t length;

  assert(nat != NULL);
  router_config_init(&config);
  router_nat_init(nat);
  config.interface_count = 2;
  config.interfaces[0] = (struct router_interface){
      .vr_id = 1, .interface_id = 2, .vswitch_id = 999,
      .attachment = ROUTER_VSWITCH, .has_address = true,
      .address = 0xa1f60626U, .prefix = 16}; /* 161.246.6.38 */
  config.interfaces[1] = (struct router_interface){
      .vr_id = 2, .interface_id = 5, .vswitch_id = 999,
      .attachment = ROUTER_VSWITCH, .has_address = true,
      .address = 0xa1f6060fU, .prefix = 16}; /* 161.246.6.15 */
  memcpy(config.interfaces[0].mac, vr1_mac, sizeof(vr1_mac));
  memcpy(config.interfaces[1].mac, vr2_mac, sizeof(vr2_mac));

  length = make_echo_request(request);
  assert(router_nat_outbound(nat, &policy, config.interfaces[0].address,
                             &inside, request, length, 1, translated,
                             sizeof(translated), &session) ==
         ROUTER_NAT_TRANSLATED);
  assert(session != NULL && read32(translated + 26) == 0xa1f60626U);
  assert(router_ipv4_rewrite(translated, length, vr1_mac, vr2_mac,
                             wire_request, sizeof(wire_request)) == length);
  assert(router_icmp_echo_reply_interface(
             &config, 5, wire_request, length, wire_reply,
             sizeof(wire_reply)) == length);
  assert(read32(wire_reply + 26) == 0xa1f6060fU &&
         read32(wire_reply + 30) == 0xa1f60626U);
  assert(router_nat_inbound(nat, 1, wire_reply, length, 2, reverse,
                            sizeof(reverse), &reverse_session) ==
         ROUTER_NAT_TRANSLATED);
  assert(reverse_session == session && read32(reverse + 30) == 0xc0a8640aU &&
         read16(reverse + 38) == 0x4321 && checksum(reverse + 14, 20) == 0 &&
         checksum(reverse + 34, 64) == 0);

  free(nat);
  puts("PASS: shared-WAN RIF ICMP crosses outbound NAT and reverse NAT");
  return 0;
}
