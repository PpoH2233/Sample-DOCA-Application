#include "router_egress.h"
#include "router_acl_plan.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>

static struct router_config config;
static char response[16384];

static bool hardware_denies(const struct router_tcp_port_range *ranges,
                            size_t count, uint16_t port) {
  for (size_t i = 0; i < count; i++)
    if (port >= ranges[i].first && port <= ranges[i].last) return true;
  return false;
}

static bool hardware_denies_icmp(
    const struct router_icmp_deny_match *matches, size_t count,
    uint8_t type, uint8_t code) {
  for (size_t i = 0; i < count; i++)
    if ((type & matches[i].type_mask) ==
            (matches[i].type & matches[i].type_mask) &&
        (code & matches[i].code_mask) ==
            (matches[i].code & matches[i].code_mask))
      return true;
  return false;
}

static bool switch_exists(void *context, uint16_t id) {
  (void)context;
  return id == 100 || id == 200;
}

static void test_control_port_prefixes(void) {
  const uint16_t ranges[][2] = {{1, 65535}, {80, 80}, {443, 443},
      {2222, 2223}, {1000, 2017}, {65535, 65535}};
  struct router_egress_rule rule = {0};
  assert(router_acl_port_count(&rule) == 1);
  assert(router_acl_port_block(0, 65535) == 65536);
  for (size_t i = 0; i < sizeof(ranges) / sizeof(ranges[0]); i++) {
    rule.port_first = ranges[i][0];
    rule.port_last = ranges[i][1];
    size_t blocks = 0;
    for (uint32_t port = 0; port <= 65535U; port++) {
      unsigned int matches = 0;
      for (uint32_t cursor = rule.port_first; cursor <= rule.port_last;) {
        uint32_t block = router_acl_port_block(cursor, rule.port_last);
        uint16_t mask = (uint16_t)~(block - 1U);
        matches += ((uint16_t)port & mask) == ((uint16_t)cursor & mask);
        if (port == 0) blocks++;
        cursor += block;
      }
      assert(matches == (port >= rule.port_first && port <= rule.port_last));
    }
    assert(blocks == router_acl_port_count(&rule));
  }
}

static void command(const char *request, bool expected) {
  struct router_inventory inventory = {.switch_exists = switch_exists};
  bool changed = false;
  bool ok = router_command(&config, &inventory, request, response,
                           sizeof(response), &changed);
  if (ok != expected)
    fprintf(stderr, "%s: %s", request, response);
  assert(ok == expected);
}

static void packet(uint8_t frame[54], uint16_t destination_port) {
  memset(frame, 0, 54);
  memcpy(frame, config.interfaces[0].mac, 6);
  frame[6] = 0x02;
  frame[12] = 0x08; frame[13] = 0x00;
  frame[14] = 0x45;
  frame[16] = 0; frame[17] = 40;
  frame[23] = 6;
  frame[26] = 10; frame[27] = 0; frame[28] = 0; frame[29] = 50;
  frame[30] = 203; frame[31] = 0; frame[32] = 113; frame[33] = 10;
  frame[34] = 0x30; frame[35] = 0x39;
  frame[36] = (uint8_t)(destination_port >> 8);
  frame[37] = (uint8_t)destination_port;
}

int main(void) {
  test_control_port_prefixes();
  struct router_egress_rule projected = {.protocol = 0, .icmp_type = -1, .icmp_code = -1};
  assert(router_acl_applies(&projected, 6));
  assert(router_acl_applies(&projected, 17));
  assert(!router_acl_applies(&projected, 1));
  projected.protocol = 6;
  assert(router_acl_applies(&projected, 6));
  assert(!router_acl_applies(&projected, 17));
  projected.protocol = 1;
  assert(!router_acl_applies(&projected, 6));
  assert(!router_acl_applies(&projected, 17));
  projected.protocol = 17;
  assert(router_acl_applies(&projected, 17));
  projected.icmp_type = 8;
  assert(!router_acl_applies(&projected, 17));
  uint8_t frame[54];
  const struct router_interface *guest;
  char path[] = "/tmp/eswitch-egress-test-XXXXXX";
  struct router_config loaded;
  int fd;

  router_config_init(&config);
  command("vr create --id 1", true);
  command("vr switch attach --id 1 --switch-id 100 --name guest", true);
  command("vr ip add --id 1 --interface guest --address 10.0.0.1/24", true);
  guest = &config.interfaces[0];
  packet(frame, 443);
  assert(router_egress_check(&config, guest, frame, sizeof(frame)) ==
         ROUTER_EGRESS_NOT_APPLICABLE);
  command("vr egress policy set --id 1 --interface guest --default deny", true);
  command("vr port-forward add --id 1 --rule-id 50 --interface guest "
          "--protocol tcp --public-port 8443 --private-ip 10.0.0.50 "
          "--private-port 443", false);
  assert(router_egress_check(&config, guest, frame, sizeof(frame)) ==
         ROUTER_EGRESS_DENY);
  {
    struct router_interface other=*guest;
    other.interface_id=99; other.vswitch_id=200;
    assert(router_egress_check(&config,&other,frame,sizeof(frame))==
           ROUTER_EGRESS_NOT_APPLICABLE);
  }
  command("vr egress rule add --id 1 --interface guest --rule-id 20 "
          "--action allow --protocol tcp --source 10.0.0.0/24 "
          "--destination 203.0.113.0/24 --port-range 443", true);
  assert(router_egress_check(&config, guest, frame, sizeof(frame)) ==
         ROUTER_EGRESS_ALLOW);
  packet(frame, 22);
  assert(router_egress_check(&config, guest, frame, sizeof(frame)) ==
         ROUTER_EGRESS_DENY);
  packet(frame, 443);
  frame[30] = 198; frame[31] = 51; frame[32] = 100;
  assert(router_egress_check(&config, guest, frame, sizeof(frame)) ==
         ROUTER_EGRESS_DENY);
  frame[30] = 10; frame[31] = 0; frame[32] = 0; frame[33] = 1;
  assert(router_egress_check(&config, guest, frame, sizeof(frame)) ==
         ROUTER_EGRESS_NOT_APPLICABLE);
  packet(frame, 443);
  frame[20] = 0x20; /* More fragments; no tracked L4 context. */
  assert(router_egress_check(&config, guest, frame, sizeof(frame)) ==
         ROUTER_EGRESS_DENY);
  packet(frame, 443);
  command("vr egress rule add --id 1 --interface guest --rule-id 10 "
          "--action deny --protocol tcp --destination 203.0.113.10/32", true);
  assert(router_egress_check(&config, guest, frame, sizeof(frame)) ==
         ROUTER_EGRESS_DENY); /* Lower rule ID wins. */
  command("vr egress rule show --id 1 --interface guest --rule-id 10", true);
  assert(strstr(response, "rule=10 action=deny") != NULL);
  command("vr egress rule add --id 1 --interface guest --rule-id 30 "
          "--action allow --protocol icmp --icmp-type 8 --icmp-code 0", true);
  packet(frame, 443);
  frame[17]=28; frame[23]=1; frame[34]=8; frame[35]=0;
  assert(router_egress_check(&config, guest, frame, sizeof(frame)) ==
         ROUTER_EGRESS_ALLOW);
  frame[34]=3;
  assert(router_egress_check(&config, guest, frame, sizeof(frame)) ==
         ROUTER_EGRESS_DENY);
  packet(frame, 443);
  command("vr egress rule add --id 1 --interface guest --rule-id 11 "
          "--action allow --protocol icmp --port-range 80", false);
  command("vr egress rule add --id 1 --interface guest --rule-id 11 "
          "--action allow --protocol tcp --source 10.0.0.50/24", false);
  command("vr switch detach --id 1 --interface guest", false);
  fd = mkstemp(path);
  assert(fd >= 0);
  close(fd);
  assert(router_config_save(path, &config, response, sizeof(response)));
  router_config_init(&loaded);
  assert(router_config_load(path, &loaded, response, sizeof(response)));
  assert(loaded.egress_policy_count == 1 && loaded.egress_rule_count == 3);
  assert(router_egress_check(&loaded, &loaded.interfaces[0], frame,
                             sizeof(frame)) == ROUTER_EGRESS_DENY);
  unlink(path);
  command("vr egress policy delete --id 1 --interface guest", false);
  command("vr egress rule delete --id 1 --interface guest --rule-id 10", true);
  command("vr egress rule delete --id 1 --interface guest --rule-id 20", true);
  command("vr egress rule delete --id 1 --interface guest --rule-id 30", true);
  command("vr egress policy delete --id 1 --interface guest", true);
  assert(router_egress_check(&config, guest, frame, sizeof(frame)) ==
         ROUTER_EGRESS_NOT_APPLICABLE);
  command("vr ingress policy set --id 1 --interface guest --default deny", true);
  packet(frame, 2222);
  assert(router_ingress_check(&config,guest,frame,sizeof(frame))==ROUTER_EGRESS_DENY);
  assert(router_egress_check(&config,guest,frame,sizeof(frame))==ROUTER_EGRESS_NOT_APPLICABLE);
  command("vr ingress rule add --id 1 --interface guest --rule-id 100 --action allow --protocol tcp --port-range 2222-2223",true);
  {
    struct router_tcp_port_range ranges[8];
    size_t count = router_ingress_tcp_deny_ranges(&config, guest, ranges, 8);
    assert(count == 2);
    assert(hardware_denies(ranges, count, 22));
    assert(!hardware_denies(ranges, count, 2222));
    assert(!hardware_denies(ranges, count, 2223));
    assert(hardware_denies(ranges, count, 2224));
    assert(hardware_denies(ranges, count, 0));
    config.nat_policies[0] = (struct router_nat_policy){
        .vr_id=guest->vr_id, .interface_id=guest->interface_id,
        .port_first=20000, .port_last=60999};
    config.nat_policy_count=1;
    count = router_ingress_tcp_deny_ranges(&config, guest, ranges, 8);
    assert(count == 3);
    assert(!hardware_denies(ranges, count, 40000));
    assert(hardware_denies(ranges, count, 61000));
    config.nat_policy_count=0;
    config.ingress_rules[config.ingress_rule_count] =
        (struct router_egress_rule){.vr_id=guest->vr_id,
            .interface_id=guest->interface_id, .rule_id=101,
            .protocol=0, .allow=true, .icmp_type=-1, .icmp_code=-1};
    config.ingress_rule_count++;
    assert(router_ingress_tcp_deny_ranges(&config, guest, ranges, 8) == 0);
    config.ingress_rule_count--;
  }
  assert(router_ingress_check(&config,guest,frame,sizeof(frame))==ROUTER_EGRESS_ALLOW);
  packet(frame,2224);
  assert(router_ingress_check(&config,guest,frame,sizeof(frame))==ROUTER_EGRESS_DENY);
  command("vr ingress rule add --id 1 --interface guest --rule-id 110 --action allow --protocol udp --port-range 53",true);
  {
    struct router_tcp_port_range ranges[8];
    size_t count = router_ingress_l4_deny_ranges(
        &config, guest, IPPROTO_UDP, ranges, 8);
    assert(count == 2);
    assert(!hardware_denies(ranges, count, 53));
    assert(hardware_denies(ranges, count, 52));
    assert(hardware_denies(ranges, count, 54));
  }
  packet(frame,53); frame[23]=17;
  assert(router_ingress_check(&config,guest,frame,sizeof(frame))==ROUTER_EGRESS_ALLOW);
  command("vr ingress rule add --id 1 --interface guest --rule-id 120 --action allow --protocol icmp --icmp-type 8 --icmp-code 0",true);
  {
    struct router_icmp_deny_match matches[256];
    size_t count = router_ingress_icmp_deny_matches(
        &config, guest, matches, 256);
    assert(count != 0);
    assert(!hardware_denies_icmp(matches, count, 8, 0));
    assert(hardware_denies_icmp(matches, count, 8, 1));
    assert(hardware_denies_icmp(matches, count, 3, 0));
  }
  frame[23]=1; frame[34]=8; frame[35]=0;
  assert(router_ingress_check(&config,guest,frame,sizeof(frame))==ROUTER_EGRESS_ALLOW);
  frame[35]=1;
  assert(router_ingress_check(&config,guest,frame,sizeof(frame))==ROUTER_EGRESS_DENY);
  frame[35]=0; frame[20]=0x20;
  assert(router_ingress_check(&config,guest,frame,sizeof(frame))==ROUTER_EGRESS_DENY);
  frame[20]=0;
  struct router_interface unrelated=*guest;
  unrelated.interface_id++;
  assert(router_ingress_check(&config,&unrelated,frame,sizeof(frame))==ROUTER_EGRESS_NOT_APPLICABLE);
  command("vr ingress rule add --id 1 --interface guest --rule-id 50 --action deny --protocol icmp",true);
  assert(router_ingress_check(&config,guest,frame,sizeof(frame))==ROUTER_EGRESS_DENY);
  command("vr ingress rule delete --id 1 --interface guest --rule-id 50",true);
  command("vr ingress rule add --id 1 --interface guest --rule-id 51 --action allow --protocol tcp --icmp-type 8",false);
  command("vr ingress policy show --id 1 --interface guest",true);
  assert(strstr(response,"ingress interface=")!=NULL);
  command("vr ingress rule show --id 1 --interface guest",true);
  char ingress_path[]="/tmp/eswitch-ingress-test-XXXXXX";
  fd=mkstemp(ingress_path); assert(fd>=0); close(fd);
  assert(router_config_save(ingress_path,&config,response,sizeof(response)));
  assert(router_config_load(ingress_path,&loaded,response,sizeof(response)));
  assert(loaded.ingress_policy_count==1 && loaded.ingress_rule_count==3);
  assert(router_ingress_check(&loaded,&loaded.interfaces[0],frame,sizeof(frame))==ROUTER_EGRESS_ALLOW);
  unlink(ingress_path);
  command("vr ingress policy delete --id 1 --interface guest",false);
  command("vr ingress rule delete --id 1 --interface guest --rule-id 100",true);
  command("vr ingress rule delete --id 1 --interface guest --rule-id 110",true);
  command("vr ingress rule delete --id 1 --interface guest --rule-id 120",true);
  command("vr ingress policy delete --id 1 --interface guest",true);
  assert(router_ingress_check(&config,guest,frame,sizeof(frame))==ROUTER_EGRESS_NOT_APPLICABLE);
  puts("egress and ingress policy tests passed");
  return 0;
}
