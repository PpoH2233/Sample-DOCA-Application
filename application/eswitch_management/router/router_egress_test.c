#include "router_egress.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static struct router_config config;
static char response[16384];

static bool switch_exists(void *context, uint16_t id) {
  (void)context;
  return id == 100 || id == 200;
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
  puts("egress policy tests passed");
  return 0;
}
