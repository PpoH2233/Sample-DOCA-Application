#include "router_arp.h"
#include <string.h>

static uint32_t read32(const uint8_t *p) {
  return (uint32_t)p[0]<<24 | (uint32_t)p[1]<<16 | (uint32_t)p[2]<<8 | p[3];
}
size_t router_arp_reply(const struct router_config *config,uint16_t vs,
                        const uint8_t *p,size_t len,uint8_t *out,size_t capacity) {
  if (!config || !p || !out || len<42 || capacity<60 || !vs) return 0;
  /* Ethernet, ARP Ethernet/IPv4, 6/4 address sizes, request opcode. */
  if (p[12]!=8 || p[13]!=6 || p[14]!=0 || p[15]!=1 ||
      p[16]!=8 || p[17]!=0 || p[18]!=6 || p[19]!=4 ||
      p[20]!=0 || p[21]!=1) return 0;
  uint8_t nonzero=0;
  for (unsigned i=0;i<6;i++) nonzero|=p[22+i];
  if (!nonzero || (p[22]&1) || memcmp(p+6,p+22,6)) return 0;
  const struct router_interface *rif=NULL;
  for (size_t i=0;i<config->interface_count;i++) {
    const struct router_interface *r=&config->interfaces[i];
    if (r->attachment==ROUTER_VSWITCH && r->vswitch_id==vs &&
        r->has_address && r->address==read32(p+38)) {rif=r;break;}
  }
  if (!rif || !memcmp(p+22,rif->mac,6)) return 0;
  static const uint8_t broadcast[6]={255,255,255,255,255,255};
  if (memcmp(p,broadcast,6) && memcmp(p,rif->mac,6)) return 0;
  /* Reject a sender claiming the gateway IP; allow RFC 5227 probes (SPA=0). */
  if (read32(p+28)==rif->address) return 0;
  memset(out,0,60);
  memcpy(out,p+22,6); memcpy(out+6,rif->mac,6);
  memcpy(out+12,p+12,10); out[21]=2;
  memcpy(out+22,rif->mac,6); memcpy(out+28,p+38,4);
  memcpy(out+32,p+22,6); memcpy(out+38,p+28,4);
  return 60;
}
