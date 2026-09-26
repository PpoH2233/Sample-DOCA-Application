#include "router_nat.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint16_t read16(const uint8_t *p)
{
	return (uint16_t)((uint16_t)p[0] << 8 | p[1]);
}

static uint32_t read32(const uint8_t *p)
{
	return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 |
	       (uint32_t)p[2] << 8 | p[3];
}

static void write16(uint8_t *p, uint16_t v)
{
	p[0] = (uint8_t)(v >> 8);
	p[1] = (uint8_t)v;
}

static void write32(uint8_t *p, uint32_t v)
{
	p[0] = (uint8_t)(v >> 24);
	p[1] = (uint8_t)(v >> 16);
	p[2] = (uint8_t)(v >> 8);
	p[3] = (uint8_t)v;
}

static uint32_t add(uint32_t sum, const uint8_t *p, size_t n)
{
	while (n >= 2) {
		sum += read16(p);
		p += 2;
		n -= 2;
	}
	if (n != 0)
		sum += (uint16_t)*p << 8;
	return sum;
}

static uint16_t finish(uint32_t sum)
{
	while ((sum >> 16) != 0)
		sum = (sum & UINT16_MAX) + (sum >> 16);
	return (uint16_t)~sum;
}

static uint16_t checksum(const uint8_t *p, size_t n)
{
	return finish(add(0, p, n));
}

static uint16_t l4_checksum(const uint8_t *ip)
{
	uint16_t total = read16(ip + 2);
	uint16_t ihl = (uint16_t)(ip[0] & 15U) * 4U;
	uint16_t l4len = total - ihl;
	uint8_t pseudo[4] = {0, ip[9], (uint8_t)(l4len >> 8), (uint8_t)l4len};
	uint32_t sum = add(0, ip + 12, 8);

	sum = add(sum, pseudo, 4);
	sum = add(sum, ip + ihl, l4len);
	return finish(sum);
}

static size_t make_icmp_echo(uint8_t *frame, uint8_t type, uint32_t src,
                             uint32_t dst, uint16_t identifier,
                             uint16_t sequence)
{
	const size_t icmp_length = 24;
	const size_t total = 20 + icmp_length;
	const size_t length = 14 + total;
	uint8_t *icmp = frame + 34;

	memset(frame, 0, 128);
	memset(frame, 0xaa, 6);
	memset(frame + 6, 0xbb, 6);
	frame[12] = 8;
	frame[13] = 0;
	frame[14] = 0x45;
	write16(frame + 16, (uint16_t)total);
	frame[22] = 64;
	frame[23] = 1;
	write32(frame + 26, src);
	write32(frame + 30, dst);
	icmp[0] = type;
	icmp[1] = 0;
	write16(icmp + 4, identifier);
	write16(icmp + 6, sequence);
	memcpy(icmp + 8, "BF3-ICMP-NAT-OK", 15);
	write16(icmp + 2, checksum(icmp, icmp_length));
	write16(frame + 24, checksum(frame + 14, 20));
	return length;
}

static size_t make_packet(uint8_t *frame,uint8_t protocol,uint32_t src,
                          uint16_t sport,uint32_t dst,uint16_t dport) {
  size_t l4len=protocol==6?20:8,total=20+l4len,length=14+total;
  memset(frame,0,128);memset(frame,0xaa,6);memset(frame+6,0xbb,6);
  frame[12]=8;frame[13]=0;frame[14]=0x45;write16(frame+16,(uint16_t)total);
  frame[22]=64;frame[23]=protocol;write32(frame+26,src);write32(frame+30,dst);
  uint8_t *l4=frame+34;write16(l4,sport);write16(l4+2,dport);
  if(protocol==6){l4[12]=0x50;l4[13]=2;write16(l4+14,65535);}
  else write16(l4+4,(uint16_t)l4len);
  write16(frame+24,checksum(frame+14,20));
  uint16_t c=l4_checksum(frame+14);write16(l4+(protocol==6?16:6),c?c:UINT16_MAX);
  return length;
}

int main(void) {
  struct router_nat_table *table=calloc(1,sizeof(*table));
  struct router_nat_policy policy={.vr_id=101,.interface_id=3,
    .port_first=20000,.port_last=20002};
  struct router_nat_inside vm1={.vr_id=101,.vswitch_id=100,.interface_id=1,.port_id=4,
    .mac={0xa6,0x94,0x27,0xfb,0x6c,0x38}};
  struct router_nat_inside vm2={.vr_id=101,.vswitch_id=200,.interface_id=2,.port_id=3,
    .mac={0x52,0x54,0,0x12,0x34,0x56}};
  const struct router_nat_session *s1,*s2,*reply;
  const struct router_nat_session *icmp_session;
  const struct router_nat_session *icmp_session2;
  uint8_t original[128],translated[128],reverse[128];
  const uint32_t public_ip=0xc8140004,remote_ip=0x08080808;
  size_t length;

  assert(table);router_nat_init(table);
  length=make_packet(original,6,0xc0a80032,51000,remote_ip,443);
  assert(router_nat_outbound(table,&policy,public_ip,&vm1,original,length,1,
                             translated,sizeof(translated),&s1)==ROUTER_NAT_TRANSLATED);
  assert(s1 && s1->public_port==20000 && table->count==1);
  assert(read32(translated+26)==public_ip && read16(translated+34)==20000);
  assert(checksum(translated+14,20)==0 && l4_checksum(translated+14)==0);

  /* An identical tuple reuses its mapping; a second inside tuple cannot
   * collide even when it uses the same private source port. */
  assert(router_nat_outbound(table,&policy,public_ip,&vm1,original,length,2,
                             translated,sizeof(translated),&reply)==ROUTER_NAT_TRANSLATED);
  assert(reply==s1 && table->count==1);
  vm1.port_id=7;vm1.mac[5]=0x39;
  assert(router_nat_outbound(table,&policy,public_ip,&vm1,original,length,2,
                             translated,sizeof(translated),&reply)==ROUTER_NAT_TRANSLATED);
  assert(reply==s1 && reply->inside.port_id==7 && reply->inside.mac[5]==0x39);
  length=make_packet(original,6,0xc0a80a0a,51000,remote_ip,443);
  assert(router_nat_outbound(table,&policy,public_ip,&vm2,original,length,3,
                             translated,sizeof(translated),&s2)==ROUTER_NAT_TRANSLATED);
  assert(s2 && s2->public_port==20001 && table->count==2);

  length=make_packet(original,6,remote_ip,443,public_ip,s1->public_port);
  assert(router_nat_inbound(table,102,original,length,4,reverse,sizeof(reverse),
                            &reply)==ROUTER_NAT_NOT_APPLICABLE);
  assert(router_nat_inbound(table,101,original,length,4,reverse,sizeof(reverse),
                            &reply)==ROUTER_NAT_TRANSLATED);
  assert(reply==s1 && read32(reverse+30)==0xc0a80032 &&
         read16(reverse+36)==51000 && reply->inside.port_id==7 &&
         !memcmp(reply->inside.mac,vm1.mac,6));
  assert(checksum(reverse+14,20)==0 && l4_checksum(reverse+14)==0);

  write16(original+36,29999);write16(original+24,0);write16(original+24,checksum(original+14,20));
  original[50]=0;original[51]=0;write16(original+50,l4_checksum(original+14));
  assert(router_nat_inbound(table,101,original,length,5,reverse,sizeof(reverse),
                            &reply)==ROUTER_NAT_NOT_APPLICABLE);

  length=make_packet(original,17,0xc0a80032,53000,remote_ip,53);
  assert(router_nat_outbound(table,&policy,public_ip,&vm1,original,length,6,
                             translated,sizeof(translated),&reply)==ROUTER_NAT_TRANSLATED);
  assert(l4_checksum(translated+14)==0);
  original[40]=0;original[41]=0; /* IPv4 UDP zero checksum stays disabled. */
  assert(router_nat_outbound(table,&policy,public_ip,&vm1,original,length,6,
                             translated,sizeof(translated),&reply)==ROUTER_NAT_TRANSLATED);
  assert(read16(translated+40)==0);
  original[20]=0x20; /* More-fragments bit. */
  original[24]=0;original[25]=0;write16(original+24,checksum(original+14,20));
  assert(router_nat_outbound(table,&policy,public_ip,&vm1,original,length,7,
                             translated,sizeof(translated),&reply)==ROUTER_NAT_UNSUPPORTED);

  length=make_packet(original,6,0xc0a80032,52000,remote_ip,443);
  original[46]^=1; /* Invalid TCP checksum must not be repaired and forwarded. */
  assert(router_nat_outbound(table,&policy,public_ip,&vm1,original,length,8,
                             translated,sizeof(translated),&reply)==ROUTER_NAT_INVALID);

  /* ICMP Echo uses its Identifier as the translated endpoint. Sequence and
   * payload remain unchanged in both directions. */
  length=make_icmp_echo(original,8,0xc0a80032,remote_ip,0x1234,7);
  assert(router_nat_outbound(table,&policy,public_ip,&vm1,original,length,9,
                             translated,sizeof(translated),&icmp_session)==
         ROUTER_NAT_TRANSLATED);
  assert(icmp_session && icmp_session->protocol==1 &&
         icmp_session->inside_port==0x1234 && icmp_session->remote_port==0 &&
         icmp_session->public_port>=policy.port_first &&
         icmp_session->public_port<=policy.port_last && table->count==4);
  assert(read32(translated+26)==public_ip &&
         read16(translated+38)==icmp_session->public_port &&
         read16(translated+40)==7 && checksum(translated+34,length-34)==0 &&
         checksum(translated+14,20)==0);

  /* Two inside hosts may use the same Echo Identifier concurrently. */
  length=make_icmp_echo(original,8,0xc0a80a0a,remote_ip,0x1234,7);
  assert(router_nat_outbound(table,&policy,public_ip,&vm2,original,length,9,
                             translated,sizeof(translated),&icmp_session2)==
         ROUTER_NAT_TRANSLATED);
  assert(icmp_session2 && icmp_session2->public_port!=icmp_session->public_port &&
         table->count==5);

  /* An Echo Request arriving at the public RIF is not a reverse-NAT reply.
   * The manager may pass this result to its local-RIF ICMP classifier. */
  length=make_icmp_echo(original,8,remote_ip,public_ip,0x4321,7);
  assert(router_nat_inbound(table,101,original,length,10,reverse,
                            sizeof(reverse),&reply)==ROUTER_NAT_UNSUPPORTED);

  length=make_icmp_echo(original,0,remote_ip,public_ip,
                        icmp_session->public_port,7);
  assert(router_nat_inbound(table,101,original,length,10,reverse,
                            sizeof(reverse),&reply)==ROUTER_NAT_TRANSLATED);
  assert(reply==icmp_session && read32(reverse+30)==0xc0a80032 &&
         read16(reverse+38)==0x1234 && read16(reverse+40)==7 &&
         checksum(reverse+34,length-34)==0 && checksum(reverse+14,20)==0);
  assert(table->stats.icmp_echo_outbound_packets==2 &&
         table->stats.icmp_echo_inbound_packets==1);

  length=make_icmp_echo(original,0,0x01010101,public_ip,
                        icmp_session->public_port,7);
  assert(router_nat_inbound(table,101,original,length,11,reverse,
                            sizeof(reverse),&reply)==ROUTER_NAT_NOT_APPLICABLE);
  length=make_icmp_echo(original,0,0xc0a80032,remote_ip,0x1234,8);
  assert(router_nat_outbound(table,&policy,public_ip,&vm1,original,length,12,
                             translated,sizeof(translated),&reply)==
         ROUTER_NAT_UNSUPPORTED);
  length=make_icmp_echo(original,8,0xc0a80032,remote_ip,0x1234,8);
  original[length-1]^=1;
  assert(router_nat_outbound(table,&policy,public_ip,&vm1,original,length,13,
                             translated,sizeof(translated),&reply)==
         ROUTER_NAT_INVALID);

  router_nat_age(table,ROUTER_NAT_UDP_IDLE_NS+10);
  assert(table->count==2); /* UDP and ICMP expire before TCP sessions. */
  router_nat_age(table,ROUTER_NAT_TCP_IDLE_NS+ROUTER_NAT_UDP_IDLE_NS+20);
  assert(table->count==0 && table->stats.sessions_aged==5);

  /* A global control-plane flush must preserve hardware-owned entries until
   * the CT pipe removes them, then remove their software owners. */
  {
    struct router_interface guest = {.vr_id=1, .interface_id=1,
        .attachment=ROUTER_VSWITCH, .vswitch_id=100};
    struct router_interface wan = {.vr_id=1, .interface_id=2,
        .attachment=ROUTER_VSWITCH, .vswitch_id=999};
    struct router_nat_session authorized = {.used=true, .vr_id=1,
        .protocol=6, .public_interface_id=2,
        .inside={.interface_id=1, .vswitch_id=100, .port_id=1,
                 .mac={2,0,0,0,0,1}}};
    const uint8_t wrong_mac[6] = {2,0,0,0,0,2};
    assert(router_nat_session_offload_eligible(&authorized,&guest,&wan,1,
                                              authorized.inside.mac,true));
    authorized.port_forward=true; /* Established PF reply is also eligible. */
    assert(router_nat_session_offload_eligible(&authorized,&guest,&wan,1,
                                              authorized.inside.mac,true));
    assert(!router_nat_session_offload_eligible(&authorized,&guest,&wan,1,
                                               authorized.inside.mac,false));
    assert(!router_nat_session_offload_eligible(&authorized,&guest,&wan,2,
                                               authorized.inside.mac,true));
    assert(!router_nat_session_offload_eligible(&authorized,&guest,&wan,1,
                                               wrong_mac,true));
    guest.interface_id=3;
    assert(!router_nat_session_offload_eligible(&authorized,&guest,&wan,1,
                                               authorized.inside.mac,true));
    guest.interface_id=1; wan.vr_id=2;
    assert(!router_nat_session_offload_eligible(&authorized,&guest,&wan,1,
                                               authorized.inside.mac,true));
    wan.vr_id=1; wan.attachment=ROUTER_PORT;
    assert(!router_nat_session_offload_eligible(&authorized,&guest,&wan,1,
                                               authorized.inside.mac,true));
    wan.attachment=ROUTER_VSWITCH; authorized.protocol=1;
    assert(!router_nat_session_offload_eligible(&authorized,&guest,&wan,1,
                                               authorized.inside.mac,true));
  }
  length=make_packet(original,6,0xc0a80032,51001,remote_ip,443);
  assert(router_nat_outbound(table,&policy,public_ip,&vm1,original,length,1,
                             translated,sizeof(translated),&s1)==
         ROUTER_NAT_TRANSLATED);
  length=make_packet(original,17,0xc0a80a0a,53001,remote_ip,53);
  assert(router_nat_outbound(table,&policy,public_ip,&vm2,original,length,1,
                             translated,sizeof(translated),&s2)==
         ROUTER_NAT_TRANSLATED);
  router_nat_session_set_hardware_active(s1,true);
  router_nat_flush(table,0);
  assert(table->count==1 && s1->used && s1->hardware_active);
  router_nat_session_set_hardware_active(s1,false);
  router_nat_flush(table,0);
  assert(table->count==0);

  /* A downstream VR is represented by the NAT owner's logical-link RIF.
   * Reverse NAT retains that RIF so the manager can run LPM through the peer
   * VR instead of assuming a directly attached inside VF. */
  {
    struct router_nat_inside link_inside={.interface_id=11};
    length=make_packet(original,6,0xc0a8c80a,41000,remote_ip,443);
    assert(router_nat_outbound(table,&policy,public_ip,&link_inside,original,
                               length,1,translated,sizeof(translated),&s1)==
           ROUTER_NAT_TRANSLATED);
    length=make_packet(original,6,remote_ip,443,public_ip,s1->public_port);
    assert(router_nat_inbound(table,101,original,length,2,reverse,
                              sizeof(reverse),&reply)==ROUTER_NAT_TRANSLATED);
    assert(reply->inside.interface_id==11 && reply->inside.vswitch_id==0 &&
           reply->inside.port_id==0 && read32(reverse+30)==0xc0a8c80a);
    router_nat_flush(table,101);
  }

  /* Fill every slot, exercise hash collisions in both directions, flush and
   * reuse indices. Each reverse lookup must retain the exact inside owner. */
  policy.port_first=20000;
  policy.port_last=30000;
  for (unsigned round=0;round<2;round++) {
    for (unsigned i=0;i<ROUTER_NAT_MAX_SESSIONS;i++) {
      length=make_packet(original,17,0x0a000001+i,12345,remote_ip,53);
      assert(router_nat_outbound(table,&policy,public_ip,&vm1,original,length,1,
                                 translated,sizeof(translated),&reply)==
             ROUTER_NAT_TRANSLATED);
    }
    assert(table->count==ROUTER_NAT_MAX_SESSIONS);
    for (unsigned i=0;i<ROUTER_NAT_MAX_SESSIONS;i++) {
      length=make_packet(original,17,0x0a000001+i,12345,remote_ip,53);
      assert(router_nat_outbound(table,&policy,public_ip,&vm1,original,length,2,
                                 translated,sizeof(translated),&reply)==
             ROUTER_NAT_TRANSLATED);
      uint16_t port=reply->public_port;
      length=make_packet(original,17,remote_ip,53,public_ip,port);
      assert(router_nat_inbound(table,101,original,length,3,reverse,
                                sizeof(reverse),&reply)==ROUTER_NAT_TRANSLATED);
      assert(read32(reverse+30)==0x0a000001+i);
    }
    router_nat_flush(table,101);
    assert(table->count==0);
    for (unsigned index=0;index<3;index++)
      for (unsigned bucket=0;bucket<ROUTER_NAT_BUCKETS;bucket++)
        assert(table->buckets[index][bucket]==0);
  }
  /* A forwarding rule reserves its public tuple even before the first
   * inbound flow, and the reply must use the fixed public port, not PAT. */
  {
    struct router_config config={0};
    const uint32_t private_ip=0xc0a80032;
    policy.port_first=20000;
    policy.port_last=20001;
    config.port_forward_count=2;
    config.port_forwards[0]=(struct router_port_forward){
      .vr_id=101,.rule_id=7,.interface_id=3,.protocol=6,
      .public_ip=public_ip,.public_port=20000,.public_port_last=20000,
      .private_ip=private_ip,.private_port=22,.private_port_last=22};
    config.port_forwards[1]=(struct router_port_forward){
      .vr_id=101,.rule_id=8,.interface_id=3,.protocol=17,
      .public_ip=public_ip,.public_port=20000,.public_port_last=20000,
      .private_ip=private_ip,.private_port=53,.private_port_last=53};
    router_nat_init(table);
    router_nat_set_port_forwards(table,&config);
    length=make_packet(original,6,private_ip,52000,remote_ip,443);
    assert(router_nat_outbound(table,&policy,public_ip,&vm1,original,length,1,
                               translated,sizeof(translated),&s1)==ROUTER_NAT_TRANSLATED);
    assert(s1->public_port==20001);
    length=make_packet(original,6,remote_ip,51000,public_ip,20000);
    assert(router_nat_port_forward_inbound(table,&config,102,3,original,length,2,
                reverse,sizeof(reverse),&reply)==ROUTER_NAT_NOT_APPLICABLE);
    assert(router_nat_port_forward_inbound(table,&config,101,9,original,length,2,
                reverse,sizeof(reverse),&reply)==ROUTER_NAT_NOT_APPLICABLE);
    assert(router_nat_inbound(table,101,original,length,2,reverse,
                              sizeof(reverse),&reply)==ROUTER_NAT_NOT_APPLICABLE);
    assert(router_nat_port_forward_inbound(table,&config,101,3,original,length,2,
                reverse,sizeof(reverse),&reply)==ROUTER_NAT_TRANSLATED);
    assert(reply && reply->port_forward && reply->port_forward_rule_id==7);
    assert(read32(reverse+30)==private_ip && read16(reverse+36)==22);
    assert(checksum(reverse+14,20)==0 && l4_checksum(reverse+14)==0);
    length=make_packet(original,6,private_ip,22,remote_ip,51000);
    {
      const struct router_interface guest={.vr_id=101,.has_address=true,
          .address=0xc0a80001,.prefix=24};
      struct router_interface other=guest;
      assert(router_nat_is_port_forward_reply(table,&guest,original,length));
      other.address=0xc0a80101;
      assert(!router_nat_is_port_forward_reply(table,&other,original,length));
      original[33]^=1;
      assert(!router_nat_is_port_forward_reply(table,&guest,original,length));
      original[33]^=1;
    }
    assert(router_nat_outbound(table,&policy,public_ip,&vm1,original,length,3,
                translated,sizeof(translated),&reply)==ROUTER_NAT_TRANSLATED);
    assert(reply->port_forward && read32(translated+26)==public_ip &&
           read16(translated+34)==20000);
    assert(checksum(translated+14,20)==0 && l4_checksum(translated+14)==0);
    assert(table->stats.port_forward_sessions_created==1 &&
           table->stats.port_forward_inbound_packets==1 &&
           table->stats.port_forward_outbound_packets==1);
    router_nat_flush(table,0);
    assert(table->count==0);
    /* PF reply can also be translated when no SNAT policy is enabled. */
    length=make_packet(original,6,remote_ip,51000,public_ip,20000);
    assert(router_nat_port_forward_inbound(table,&config,101,3,original,length,4,
                reverse,sizeof(reverse),&reply)==ROUTER_NAT_TRANSLATED);
    length=make_packet(original,6,private_ip,22,remote_ip,51000);
    assert(router_nat_outbound(table,NULL,0,&vm1,original,length,5,
                translated,sizeof(translated),&reply)==ROUTER_NAT_TRANSLATED);
    assert(read16(translated+34)==20000);
    router_nat_flush(table,0);
    length=make_packet(original,17,remote_ip,54000,public_ip,20000);
    assert(router_nat_port_forward_inbound(table,&config,101,3,original,length,6,
                reverse,sizeof(reverse),&reply)==ROUTER_NAT_TRANSLATED);
    assert(reply->port_forward_rule_id==8 && read16(reverse+36)==53 &&
           l4_checksum(reverse+14)==0);
    length=make_packet(original,17,private_ip,53,remote_ip,54000);
    assert(router_nat_outbound(table,&policy,public_ip,&vm1,original,length,7,
                translated,sizeof(translated),&reply)==ROUTER_NAT_TRANSLATED);
    assert(read16(translated+34)==20000 && l4_checksum(translated+14)==0);
    router_nat_port_forward_reject(table,reply);
    assert(table->count==0);
    length=make_packet(original,17,remote_ip,54000,public_ip,20000);
    assert(router_nat_inbound(table,101,original,length,8,reverse,
                              sizeof(reverse),&reply)==ROUTER_NAT_NOT_APPLICABLE);
    router_nat_flush(table,0);
    config.port_forward_count=4;
    config.port_forwards[2]=(struct router_port_forward){
      .vr_id=101,.rule_id=9,.interface_id=3,.protocol=6,
      .public_ip=public_ip,.public_port=21000,.public_port_last=21003,
      .private_ip=private_ip,.private_port=8100,.private_port_last=8103};
    config.port_forwards[3]=(struct router_port_forward){
      .vr_id=101,.rule_id=10,.interface_id=3,.protocol=17,
      .public_ip=public_ip,.public_port=22000,.public_port_last=22001,
      .private_ip=private_ip,.private_port=5300,.private_port_last=5301};
    router_nat_set_port_forwards(table,&config);
    policy.port_first=21000;
    policy.port_last=21004;
    length=make_packet(original,6,private_ip,52000,remote_ip,443);
    assert(router_nat_outbound(table,&policy,public_ip,&vm1,original,length,9,
                translated,sizeof(translated),&s1)==ROUTER_NAT_TRANSLATED);
    assert(s1->public_port==21004);
    for (uint16_t offset=0;offset<4;offset++) {
      uint16_t public_port=(uint16_t)(21000+offset);
      uint16_t private_port=(uint16_t)(8100+offset);
      length=make_packet(original,6,remote_ip,(uint16_t)(51000+offset),
                         public_ip,public_port);
      assert(router_nat_port_forward_inbound(table,&config,101,3,original,length,
                  10+offset,reverse,sizeof(reverse),&reply)==ROUTER_NAT_TRANSLATED);
      assert(reply->public_port==public_port && reply->inside_port==private_port);
      assert(read16(reverse+36)==private_port &&
             checksum(reverse+14,20)==0 && l4_checksum(reverse+14)==0);
      length=make_packet(original,6,private_ip,private_port,remote_ip,
                         (uint16_t)(51000+offset));
      assert(router_nat_outbound(table,&policy,public_ip,&vm1,original,length,
                  20+offset,translated,sizeof(translated),&reply)==ROUTER_NAT_TRANSLATED);
      assert(read16(translated+34)==public_port && l4_checksum(translated+14)==0);
    }
    length=make_packet(original,6,remote_ip,52000,public_ip,20999);
    assert(router_nat_port_forward_inbound(table,&config,101,3,original,length,30,
                reverse,sizeof(reverse),&reply)==ROUTER_NAT_NOT_APPLICABLE);
    length=make_packet(original,6,remote_ip,52000,public_ip,21005);
    assert(router_nat_port_forward_inbound(table,&config,101,3,original,length,31,
                reverse,sizeof(reverse),&reply)==ROUTER_NAT_NOT_APPLICABLE);
    length=make_packet(original,17,remote_ip,54000,public_ip,22001);
    assert(router_nat_port_forward_inbound(table,&config,101,3,original,length,32,
                reverse,sizeof(reverse),&reply)==ROUTER_NAT_TRANSLATED);
    assert(reply->inside_port==5301 && read16(reverse+36)==5301 &&
           l4_checksum(reverse+14)==0);
    router_nat_flush(table,0);
  }
  free(table);
  puts("PASS: TCP/UDP/ICMP Echo NAT, port forwarding, reverse lookup, checksums, isolation and aging");
  return 0;
}
