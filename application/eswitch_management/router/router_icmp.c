#include "router_icmp.h"

#include <string.h>

#define ETH_HEADER_LEN 14U
#define IPV4_MIN_HEADER_LEN 20U
#define ICMP_HEADER_LEN 8U
#define ETHERNET_MIN_FRAME_NO_FCS 60U

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

static uint16_t checksum(const uint8_t *bytes, size_t length) {
  uint32_t sum = 0;

  while (length >= 2) {
    sum += read16(bytes);
    bytes += 2;
    length -= 2;
  }
  if (length != 0)
    sum += (uint16_t)bytes[0] << 8;
  while ((sum >> 16) != 0)
    sum = (sum & UINT16_MAX) + (sum >> 16);
  return (uint16_t)~sum;
}

size_t router_icmp_echo_reply(const struct router_config *config,
                              uint16_t vswitch_id, const uint8_t *packet,
                              size_t length, uint8_t *output,
                              size_t capacity) {
  const struct router_interface *rif = NULL;
  const uint8_t *ip;
  const uint8_t *icmp;
  uint8_t *output_ip;
  uint8_t *output_icmp;
  size_t ip_header_length;
  size_t frame_length;
  uint16_t ip_total_length;
  uint16_t fragment;
  size_t icmp_length;

  if (config == NULL || packet == NULL || output == NULL || vswitch_id == 0 ||
      length < ETH_HEADER_LEN + IPV4_MIN_HEADER_LEN + ICMP_HEADER_LEN)
    return 0;
  if (packet[12] != 0x08 || packet[13] != 0x00 ||
      (packet[6] & 1U) != 0)
    return 0;

  ip = packet + ETH_HEADER_LEN;
  if ((ip[0] >> 4) != 4 || (ip[0] & 0x0fU) < 5)
    return 0;
  ip_header_length = (size_t)(ip[0] & 0x0fU) * 4U;
  if (length < ETH_HEADER_LEN + ip_header_length + ICMP_HEADER_LEN ||
      ip[9] != 1 || checksum(ip, ip_header_length) != 0)
    return 0;

  ip_total_length = read16(ip + 2);
  if (ip_total_length < ip_header_length + ICMP_HEADER_LEN ||
      ip_total_length > length - ETH_HEADER_LEN)
    return 0;
  fragment = read16(ip + 6);
  if ((fragment & 0x3fffU) != 0) /* Reject MF and every non-zero offset. */
    return 0;

  for (size_t i = 0; i < config->interface_count; i++) {
    const struct router_interface *candidate = &config->interfaces[i];

    if (candidate->attachment == ROUTER_VSWITCH &&
        candidate->vswitch_id == vswitch_id && candidate->has_address &&
        candidate->address == read32(ip + 16) &&
        memcmp(packet, candidate->mac, 6) == 0) {
      rif = candidate;
      break;
    }
  }
  if (rif == NULL || read32(ip + 12) == rif->address)
    return 0;

  icmp = ip + ip_header_length;
  icmp_length = ip_total_length - ip_header_length;
  if (icmp[0] != 8 || icmp[1] != 0 || checksum(icmp, icmp_length) != 0)
    return 0;

  frame_length = ETH_HEADER_LEN + ip_total_length;
  if (frame_length < ETHERNET_MIN_FRAME_NO_FCS)
    frame_length = ETHERNET_MIN_FRAME_NO_FCS;
  if (capacity < frame_length)
    return 0;

  memset(output, 0, frame_length);
  memcpy(output, packet + 6, 6);
  memcpy(output + 6, rif->mac, 6);
  output[12] = 0x08;
  output[13] = 0x00;
  memcpy(output + ETH_HEADER_LEN, ip, ip_total_length);

  output_ip = output + ETH_HEADER_LEN;
  output_ip[8] = 64;
  write32(output_ip + 12, rif->address);
  memcpy(output_ip + 16, ip + 12, 4);
  output_ip[10] = 0;
  output_ip[11] = 0;
  write16(output_ip + 10, checksum(output_ip, ip_header_length));

  output_icmp = output_ip + ip_header_length;
  output_icmp[0] = 0;
  output_icmp[1] = 0;
  output_icmp[2] = 0;
  output_icmp[3] = 0;
  write16(output_icmp + 2, checksum(output_icmp, icmp_length));
  return frame_length;
}
