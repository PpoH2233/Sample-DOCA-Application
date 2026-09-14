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
  struct router_nat_inside vm1={.vswitch_id=100,.interface_id=1,.port_id=4,
    .mac={0xa6,0x94,0x27,0xfb,0x6c,0x38}};
  struct router_nat_inside vm2={.vswitch_id=200,.interface_id=2,.port_id=3,
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
  free(table);
  puts("PASS: TCP/UDP/ICMP Echo NAT, reverse lookup, checksums, isolation and aging");
  return 0;
}
