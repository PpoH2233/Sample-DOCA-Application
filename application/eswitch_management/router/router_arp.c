#include "router_arp.h"
#include <string.h>

static uint32_t read32(const uint8_t *p) {
  return (uint32_t)p[0]<<24 | (uint32_t)p[1]<<16 | (uint32_t)p[2]<<8 | p[3];
}
static void write32(uint8_t *p, uint32_t value) {
  p[0]=(uint8_t)(value>>24);p[1]=(uint8_t)(value>>16);
  p[2]=(uint8_t)(value>>8);p[3]=(uint8_t)value;
}
static size_t reply_for_interface(const struct router_interface *rif,
                                  const uint8_t *p,size_t len,uint8_t *out,
                                  size_t capacity) {
  if (!rif || !p || !out || len<42 || capacity<60 || !rif->has_address) return 0;
  /* Ethernet, ARP Ethernet/IPv4, 6/4 address sizes, request opcode. */
  if (p[12]!=8 || p[13]!=6 || p[14]!=0 || p[15]!=1 ||
      p[16]!=8 || p[17]!=0 || p[18]!=6 || p[19]!=4 ||
      p[20]!=0 || p[21]!=1) return 0;
  uint8_t nonzero=0;
  for (unsigned i=0;i<6;i++) nonzero|=p[22+i];
  if (!nonzero || (p[22]&1) || memcmp(p+6,p+22,6)) return 0;
  if (rif->address!=read32(p+38) || !memcmp(p+22,rif->mac,6)) return 0;
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

size_t router_arp_reply(const struct router_config *config,uint16_t vs,
                        const uint8_t *p,size_t len,uint8_t *out,size_t capacity) {
  if(!config || !vs) return 0;
  for(size_t i=0;i<config->interface_count;i++) {
    const struct router_interface *rif=&config->interfaces[i];
    if(rif->attachment==ROUTER_VSWITCH && rif->vswitch_id==vs &&
       rif->has_address && p && len>=42 && rif->address==read32(p+38))
      return reply_for_interface(rif,p,len,out,capacity);
  }
  return 0;
}

size_t router_arp_reply_interface(const struct router_config *config,
                                  uint16_t interface_id,const uint8_t *p,
                                  size_t len,uint8_t *out,size_t capacity) {
  if(!config || !interface_id) return 0;
  for(size_t i=0;i<config->interface_count;i++)
    if(config->interfaces[i].interface_id==interface_id)
      return reply_for_interface(&config->interfaces[i],p,len,out,capacity);
  return 0;
}

size_t router_arp_request(const struct router_interface *rif,
                          uint32_t target_ip, uint8_t *out,
                          size_t capacity) {
  static const uint8_t broadcast[6]={255,255,255,255,255,255};
  uint32_t mask;

  if (!rif || !out || capacity<60 || !target_ip ||
      !rif->has_address ||
      (rif->mac[0]&1U)) return 0;
  mask=rif->prefix==0?0:UINT32_MAX<<(32-rif->prefix);
  if ((target_ip&mask)!=(rif->address&mask) || target_ip==rif->address)
    return 0;
  memset(out,0,60);
  memcpy(out,broadcast,6);memcpy(out+6,rif->mac,6);
  out[12]=0x08;out[13]=0x06;
  out[14]=0;out[15]=1;out[16]=0x08;out[17]=0;
  out[18]=6;out[19]=4;out[20]=0;out[21]=1;
  memcpy(out+22,rif->mac,6);write32(out+28,rif->address);
  write32(out+38,target_ip);
  return 60;
}
