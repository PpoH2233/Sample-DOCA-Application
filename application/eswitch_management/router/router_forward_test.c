#include "router_forward.h"
#include "router_neighbor.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static uint16_t read16(const uint8_t *p) {
  return (uint16_t)((uint16_t)p[0]<<8|p[1]);
}
static void write16(uint8_t *p,uint16_t value) {
  p[0]=(uint8_t)(value>>8);p[1]=(uint8_t)value;
}
static uint16_t checksum(const uint8_t *p,size_t length) {
  uint32_t sum=0;
  while(length>=2){sum+=read16(p);p+=2;length-=2;}
  if(length)sum+=(uint16_t)p[0]<<8;
  while(sum>>16)sum=(sum&UINT16_MAX)+(sum>>16);
  return (uint16_t)~sum;
}
static void ip(uint8_t *p,uint32_t value) {
  p[0]=(uint8_t)(value>>24);p[1]=(uint8_t)(value>>16);
  p[2]=(uint8_t)(value>>8);p[3]=(uint8_t)value;
}
static void make_ipv4(uint8_t frame[98],const uint8_t gateway[6],
                      uint32_t source,uint32_t destination,uint8_t ttl) {
  static const uint8_t vm[6]={0xa6,0x94,0x27,0xfb,0x6c,0x38};
  memset(frame,0,98);memcpy(frame,gateway,6);memcpy(frame+6,vm,6);
  frame[12]=8;frame[13]=0;frame[14]=0x45;frame[16]=0;frame[17]=84;
  frame[22]=ttl;frame[23]=1;ip(frame+26,source);ip(frame+30,destination);
  write16(frame+24,checksum(frame+14,20));
}

int main(void) {
  struct router_config config;
  struct router_ipv4_decision decision;
  struct router_neighbor_table neighbors={0};
  uint8_t frame[98],output[98],arp[60]={0};
  const uint8_t mac_a[6]={2,0,0,0x65,0,1};
  const uint8_t mac_b[6]={2,0,0,0x65,0,2};
  const uint8_t vm_b[6]={0x52,0x54,0,0x12,0x34,0x56};

  router_config_init(&config);config.vr_ids[0]=101;config.vr_count=1;
  config.interface_count=3;
  config.interfaces[0]=(struct router_interface){.vr_id=101,.interface_id=1,
    .attachment=ROUTER_VSWITCH,.vswitch_id=100,.has_address=true,
    .address=0xc0a80001,.prefix=24};memcpy(config.interfaces[0].mac,mac_a,6);
  config.interfaces[1]=(struct router_interface){.vr_id=101,.interface_id=2,
    .attachment=ROUTER_VSWITCH,.vswitch_id=200,.has_address=true,
    .address=0xc0a80101,.prefix=24};memcpy(config.interfaces[1].mac,mac_b,6);
  config.interfaces[2]=(struct router_interface){.vr_id=101,.interface_id=3,
    .attachment=ROUTER_PORT,.has_address=true,.address=0xc8140004,
    .prefix=16};memcpy(config.interfaces[2].mac,mac_b,6);

  make_ipv4(frame,mac_a,0xc0a80032,0xc0a80132,64);
  assert(router_ipv4_lookup(&config,100,frame,sizeof(frame),&decision)==
         ROUTER_IPV4_FORWARD);
  assert(decision.vr_id==101 && decision.ingress_interface_id==1 &&
         decision.egress_interface_id==2 && decision.egress_vswitch_id==200 &&
         decision.next_hop_ip==0xc0a80132 && decision.prefix_length==24 &&
         decision.connected);

  config.routes[0]=(struct router_route){.vr_id=101,.interface_id=2,
    .prefix=0xc0a80100,.gateway=0xc0a801fe,.length=25};config.route_count=1;
  assert(router_ipv4_lookup(&config,100,frame,sizeof(frame),&decision)==
         ROUTER_IPV4_FORWARD);
  assert(!decision.connected && decision.prefix_length==25 &&
         decision.next_hop_ip==0xc0a801fe);
  config.route_count=0;

  config.routes[0]=(struct router_route){.vr_id=101,.interface_id=3,
    .prefix=0,.gateway=0xc8140001,.length=0};config.route_count=1;
  make_ipv4(frame,mac_a,0xc0a80032,0x08080808,64);
  assert(router_ipv4_lookup(&config,100,frame,sizeof(frame),&decision)==
         ROUTER_IPV4_FORWARD);
  assert(decision.egress_interface_id==3 && decision.egress_vswitch_id==0 &&
         decision.next_hop_ip==0xc8140001 && decision.prefix_length==0 &&
         !decision.connected);
  make_ipv4(frame,mac_b,0x08080808,0xc0a80032,64);
  assert(router_ipv4_lookup_interface(&config,3,frame,sizeof(frame),&decision)==
         ROUTER_IPV4_FORWARD);
  assert(decision.ingress_interface_id==3 && decision.egress_interface_id==1 &&
         decision.egress_vswitch_id==100 && decision.connected);
  config.route_count=0;

  make_ipv4(frame,mac_a,0xc0a80032,0xc0a80001,64);
  assert(router_ipv4_lookup(&config,100,frame,sizeof(frame),&decision)==
         ROUTER_IPV4_LOCAL);
  make_ipv4(frame,mac_a,0xc0a80032,0xcb007101,64);
  assert(router_ipv4_lookup(&config,100,frame,sizeof(frame),&decision)==
         ROUTER_IPV4_NO_ROUTE);
  make_ipv4(frame,mac_a,0xc0a80032,0xc0a80132,1);
  assert(router_ipv4_lookup(&config,100,frame,sizeof(frame),&decision)==
         ROUTER_IPV4_TTL_EXPIRED);
  make_ipv4(frame,mac_b,0xc0a80032,0xc0a80132,64);
  assert(router_ipv4_lookup(&config,100,frame,sizeof(frame),&decision)==
         ROUTER_IPV4_NOT_FOR_ROUTER);
  make_ipv4(frame,mac_a,0xc0a80032,0xc0a80132,64);frame[24]^=1;
  assert(router_ipv4_lookup(&config,100,frame,sizeof(frame),&decision)==
         ROUTER_IPV4_INVALID);frame[24]^=1;

  assert(router_ipv4_rewrite(frame,sizeof(frame),mac_b,vm_b,output,
                             sizeof(output))==98);
  assert(!memcmp(output,vm_b,6) && !memcmp(output+6,mac_b,6));
  assert(output[22]==63 && checksum(output+14,20)==0);
  assert(!memcmp(output+34,frame+34,64));

  memset(arp,0,sizeof(arp));memset(arp,0xff,6);
  memcpy(arp+6,vm_b,6);arp[12]=8;arp[13]=6;arp[14]=0;arp[15]=1;
  arp[16]=8;arp[17]=0;arp[18]=6;arp[19]=4;arp[20]=0;arp[21]=2;
  memcpy(arp+22,vm_b,6);ip(arp+28,0xc0a80132);memcpy(arp+32,mac_b,6);
  ip(arp+38,0xc0a80101);
  assert(router_neighbor_learn_arp(&neighbors,&config,200,7,arp,42,
                                   UINT64_C(2000000000)));
  const struct router_neighbor *neighbor=router_neighbor_lookup(
      &neighbors,101,2,0xc0a80132,UINT64_C(3000000000));
  assert(neighbor && neighbor->port_id==7 && !memcmp(neighbor->mac,vm_b,6));
  assert(!router_neighbor_learn_arp(&neighbors,&config,200,7,arp,42,
                                    UINT64_C(4000000000)));
  neighbor=router_neighbor_lookup(&neighbors,101,2,0xc0a80132,
                                  UINT64_C(303000000000));
  assert(neighbor && neighbor->port_id==7 && !memcmp(neighbor->mac,vm_b,6));
  assert(router_neighbor_should_probe(&neighbors,101,2,0xc0a80140,
                                      UINT64_C(3000000000)));
  assert(!router_neighbor_should_probe(&neighbors,101,2,0xc0a80140,
                                       UINT64_C(3500000000)));
  assert(router_neighbor_should_probe(&neighbors,101,2,0xc0a80140,
                                      UINT64_C(4100000000)));
  {
    const uint8_t public_neighbor_mac[6]={0x02,0xaa,0xbb,0xcc,0xdd,0xee};

    memcpy(arp+6,public_neighbor_mac,6);memcpy(arp+22,public_neighbor_mac,6);
    ip(arp+28,0xc8140001);memcpy(arp+32,config.interfaces[2].mac,6);
    ip(arp+38,config.interfaces[2].address);
    assert(router_neighbor_learn_arp_interface(
        &neighbors,&config,3,9,arp,42,UINT64_C(5000000000)));
    assert(!router_neighbor_learn_arp_interface(
        &neighbors,&config,3,9,arp,42,UINT64_C(6000000000)));
    neighbor=router_neighbor_lookup(&neighbors,101,3,0xc8140001,
                                    UINT64_C(6000000000));
    assert(neighbor && neighbor->port_id==9 &&
           !memcmp(neighbor->mac,public_neighbor_mac,6));
  }
  router_neighbor_age(&neighbors,UINT64_C(400000000000));
  assert(neighbors.count==0);

  /* A logical link performs one lookup/rewrite in each VR. */
  {
    struct router_config linked;
    struct router_ipv4_decision first,second;
    uint8_t hop1[98],hop2[98];
    const uint8_t r1_lan[6]={2,0,0,1,0,1};
    const uint8_t r1_link[6]={2,0,0,1,0,2};
    const uint8_t r2_link[6]={2,0,0,2,0,1};
    const uint8_t r2_lan[6]={2,0,0,2,0,2};
    router_config_init(&linked);
    linked.vr_ids[0]=1;linked.vr_ids[1]=2;linked.vr_count=2;
    linked.link_ids[0]=10;linked.link_count=1;linked.interface_count=4;
    linked.interfaces[0]=(struct router_interface){.vr_id=1,.interface_id=10,
      .attachment=ROUTER_VSWITCH,.vswitch_id=100,.has_address=true,
      .address=0xc0a86401,.prefix=24};
    linked.interfaces[1]=(struct router_interface){.vr_id=1,.interface_id=11,
      .attachment=ROUTER_LINK,.link_id=10,.has_address=true,
      .address=0x0a0a0a01,.prefix=30};
    linked.interfaces[2]=(struct router_interface){.vr_id=2,.interface_id=12,
      .attachment=ROUTER_LINK,.link_id=10,.has_address=true,
      .address=0x0a0a0a02,.prefix=30};
    linked.interfaces[3]=(struct router_interface){.vr_id=2,.interface_id=13,
      .attachment=ROUTER_VSWITCH,.vswitch_id=200,.has_address=true,
      .address=0xc0a8c801,.prefix=24};
    memcpy(linked.interfaces[0].mac,r1_lan,6);
    memcpy(linked.interfaces[1].mac,r1_link,6);
    memcpy(linked.interfaces[2].mac,r2_link,6);
    memcpy(linked.interfaces[3].mac,r2_lan,6);
    linked.routes[0]=(struct router_route){.vr_id=1,.interface_id=11,
      .prefix=0xc0a8c800,.gateway=0x0a0a0a02,.length=24};
    linked.route_count=1;
    make_ipv4(frame,r1_lan,0xc0a86432,0xc0a8c80a,64);
    assert(router_ipv4_lookup_interface(&linked,10,frame,sizeof(frame),&first)==
           ROUTER_IPV4_FORWARD && first.egress_interface_id==11);
    assert(router_link_peer(&linked,11)==&linked.interfaces[2]);
    assert(router_ipv4_rewrite(frame,sizeof(frame),r1_link,r2_link,hop1,
                               sizeof(hop1))==98 && hop1[22]==63);
    assert(router_ipv4_lookup_interface(&linked,12,hop1,sizeof(hop1),&second)==
           ROUTER_IPV4_FORWARD && second.egress_interface_id==13);
    assert(router_ipv4_rewrite(hop1,sizeof(hop1),r2_lan,vm_b,hop2,
                               sizeof(hop2))==98 && hop2[22]==62);
  }
  puts("PASS: VR LPM, connected/static selection, TTL/checksum rewrite and ARP neighbors");
  return 0;
}
