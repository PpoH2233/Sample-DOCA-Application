#include "router.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static unsigned checks;
static bool port(void *context,uint16_t id,struct router_port_identity *p) {
  (void)context;
  /* Deliberately different DPDK IDs and VF indices. */
  if(id!=7 && id!=8) return false;
  *p=(struct router_port_identity){0,0,id==7?11:12}; return true;
}
static bool vs(void *context,uint16_t id) {(void)context;return id==200 || id==201;}
static struct router_inventory inventory={NULL,port,vs};
static struct router_config config;
static char response[16384];
static void command(const char *request,bool expected) {
  struct router_config before=config;
  bool changed=false;
  bool ok=router_command(&config,&inventory,request,response,sizeof(response),&changed);
  if(ok!=expected) fprintf(stderr,"%s: %s",request,response);
  assert(ok==expected);
  if(!ok) {assert(!changed);assert(!memcmp(&before,&config,sizeof(config)));}
  checks++;
}
int main(void) {
  router_config_init(&config);

  /* Canonical resource-first grammar. This section is torn down completely so
   * the legacy-alias section below starts from the same inventory. */
  command("vr create --id 300",true);
  command("vr port attach --id 300 --port 7 --name up0",true);
  command("vr switch attach --id 300 --switch-id 200 --name lan0",true);
  command("vr interface set --id 300 --interface lan0 --mac 02:aa:bb:cc:dd:ee",true);
  command("vr ip add --id 300 --interface lan0 --address 10.10.0.1/24",true);
  command("vr ip add --id 300 --interface up0 --address 203.0.113.2/24",true);
  command("vr route add --id 300 --prefix 0.0.0.0/0 --via 203.0.113.1 --interface up0",true);
  command("vr nat enable --id 300 --interface up0 --address interface --port-range 20000-60999",true);
  command("vr show --id 300",true);
  assert(strstr(response,"up0") && strstr(response,"lan0") &&
         strstr(response,"ACTIVE_ARM_NAT") && strstr(response,"ACTIVE_ARM_LPM"));
  command("vr route show --id 300",true);
  assert(strstr(response,"connected 10.10.0.0/24") &&
         strstr(response,"static 0.0.0.0/0"));
  /* Canonical nested grammar is strict about its options and actions. */
  command("vr port attach --id 300 --port 8 --interface p9",false);
  command("vr port detach --id 300 --name up0",false);
  command("vr port connect --id 300 --port 8 --name p9",false);
  command("vr switch connect --id 300 --switch-id 201 --name p9",false);
  command("vr port",false);
  command("vr switch",false);
  command("vr port attach --id 300 --port 8",false);
  command("vr switch attach --id 300 --name p9",false);
  command("vr port detach --id 300 --interface lan0",false);
  command("vr switch detach --id 300 --interface up0",false);
  /* Canonical and legacy aliases address the same objects. */
  command("vr show-interface --id 300",true);
  command("vr port-detach --id 300 --interface up0",false);
  command("vr nat disable --id 300",true);
  command("vr route del --id 300 --prefix 0.0.0.0/0",true);
  command("vr ip del --id 300 --interface up0 --address 203.0.113.2/24",true);
  command("vr port detach --id 300 --interface up0",true);
  command("vr ip del --id 300 --interface lan0 --address 10.10.0.1/24",true);
  command("vr switch detach --id 300 --interface lan0",true);
  command("vr delete --id 300",true);
  assert(!router_has_vr(&config,300));
  assert(!router_switch_reserved(&config,200));

  /* Deprecated flat aliases remain fully supported. */
  command("vr create --id 100",true);
  command("vr create --id 100",false);
  command("vr create --id 0",false);
  command("vr create --id -1",false);
  command("vr create --id 65536",false);
  command("vr create --id 101 --id 102",false);
  command("vr create --id 101 --unknown value",false);
  command("vr create --id 101",true);
  command("vr port-attach --id 100 --port 11 --name p1",false);
  command("vr port-attach --id 100 --port 7 --name p1",true);
  command("vr port-attach --id 101 --port 7 --name p1",false);
  command("vr port-attach --id 100 --port 8 --name p3",false);
  command("vr port-attach --id 101 --port 8 --name p1",true);
  command("vr switch-attach --id 100 --switch-id 200 --name p2",true);
  command("vr switch-attach --id 101 --switch-id 200 --name p2",false);
  command("vr switch-attach --id 101 --switch-id 201 --name p2",true);
  command("vr delete --id 100",false);
  command("vr ip add --id 100 --interface p2 --address 192.168.0.0/24",false);
  command("vr ip add --id 100 --interface p2 --address 192.168.0.255/24",false);
  command("vr ip add --id 100 --interface p2 --address 192.168.0.1/33",false);
  command("vr ip add --id 100 --interface p2 --address 192.168.0.1/24",true);
  command("vr ip add --id 101 --interface p2 --address 192.168.0.1/24",true);
  command("vr ip add --id 100 --interface p1 --address 192.168.0.2/24",false);
  command("vr ip add --id 100 --interface p1 --address 200.20.0.4/16",true);
  command("vr interface set --id 100 --interface p1 --mac ff:ff:ff:ff:ff:ff",false);
  command("vr interface set --id 100 --interface p1 --mac 02:11:22:33:44:55",true);
  command("vr route add --id 100 --prefix 0.0.0.0/0 --via 200.21.0.1 --interface p1",false);
  command("vr route add --id 100 --prefix 10.0.0.1/8 --via 200.20.0.1 --interface p1",false);
  command("vr route add --id 100 --prefix 0.0.0.0/0 --via 200.20.0.1 --interface p1",true);
  command("vr route add --id 100 --prefix 0.0.0.0/0 --via 200.20.0.2 --interface p1",false);
  command("vr nat enable --id 100 --interface p2 --address interface --port-range 20000-60999",false);
  command("vr nat enable --id 100 --interface p1 --address 200.20.0.5 --port-range 20000-60999",false);
  command("vr nat enable --id 100 --interface p1 --address interface --port-range 1-65535",false);
  command("vr nat enable --id 100 --interface p1 --address interface --port-range 20000-60999",true);
  command("vr nat enable --id 100 --interface p1 --address interface --port-range 20000-60999",false);
  command("vr nat show --id 100",true);
  assert(strstr(response,"nat=enabled") && strstr(response,"address=200.20.0.4") &&
         strstr(response,"ports=20000-60999"));
  command("vr route del --id 100 --prefix 0.0.0.0/0",false);
  command("vr ip del --id 100 --interface p1 --address 200.20.0.4/16",false);
  command("vr switch-detach --id 100 --interface p1",false);
  command("vr port-detach --id 100 --interface p1",false);
  command("vr show-interface --id 100",true);
  assert(strstr(response,"vf=11") && strstr(response,"ACTIVE_ARM_NAT") &&
         strstr(response,"ACTIVE_ARM_LPM"));
  command("vr route show --id 100",true);
  assert(strstr(response,"connected 192.168.0.0/24") && strstr(response,"static 0.0.0.0/0"));
  command("vr nat add --id 100",false);
  command("vr interface set --id 100 --interface p1 --admin-state up",false);

  char dir[]="/tmp/eswitch-router-test.XXXXXX",path[256];
  assert(mkdtemp(dir));snprintf(path,sizeof(path),"%s/router.conf",dir);
  assert(router_config_save(path,&config,response,sizeof(response)));
  struct router_config loaded;router_config_init(&loaded);
  assert(router_config_load(path,&loaded,response,sizeof(response)));
  assert(loaded.vr_count==config.vr_count && loaded.route_count==config.route_count);
  assert(loaded.nat_policy_count==config.nat_policy_count);
  assert(loaded.interface_count==config.interface_count && loaded.next_interface_id==config.next_interface_id);
  for(size_t i=0;i<config.interface_count;i++)
    assert(!memcmp(&loaded.interfaces[i],&config.interfaces[i],sizeof(config.interfaces[i])));
  for(size_t i=0;i<config.route_count;i++)
    assert(!memcmp(&loaded.routes[i],&config.routes[i],sizeof(config.routes[i])));
  for(size_t i=0;i<config.nat_policy_count;i++)
    assert(!memcmp(&loaded.nat_policies[i],&config.nat_policies[i],sizeof(config.nat_policies[i])));
  /* Malformed restore must not publish partial configuration. */
  FILE *file=fopen(path,"a");assert(file);assert(fputs("vr delete --id 101\n",file)>=0);assert(fclose(file)==0);
  struct router_config before=loaded;
  assert(!router_config_load(path,&loaded,response,sizeof(response)));
  assert(!memcmp(&before,&loaded,sizeof(loaded)));
  assert(unlink(path)==0);

  /* Newly saved state uses the canonical VR grammar. */
  assert(router_config_save(path,&config,response,sizeof(response)));
  file=fopen(path,"r");assert(file);
  char saved[8192]={0};
  size_t saved_length=fread(saved,1,sizeof(saved)-1,file);
  assert(saved_length>0 && feof(file));assert(fclose(file)==0);
  assert(strstr(saved,"vr port attach --id ") && strstr(saved,"vr switch attach --id "));
  assert(!strstr(saved,"port-attach") && !strstr(saved,"switch-attach"));

  /* Legacy state written by a version-1 daemon still reloads. */
  char legacy_path[256];
  snprintf(legacy_path,sizeof(legacy_path),"%s/router-legacy.conf",dir);
  file=fopen(legacy_path,"w");assert(file);
  assert(fputs("router-state 1\nnext 3\n"
               "vr create --id 400\n"
               "identity 1 0 0 11\n"
               "vr port-attach --id 400 --name up0 --port 0\n"
               "vr interface set --id 400 --interface up0 --mac 02:00:01:90:00:01\n"
               "vr ip add --id 400 --interface up0 --address 203.0.113.9/24\n"
               "identity 2 0 0 0\n"
               "vr switch-attach --id 400 --name lan0 --switch-id 200\n"
               "vr interface set --id 400 --interface lan0 --mac 02:00:01:90:00:02\n",
               file)>=0);
  assert(fclose(file)==0);
  struct router_config legacy;router_config_init(&legacy);
  assert(router_config_load(legacy_path,&legacy,response,sizeof(response)));
  assert(legacy.vr_count==1 && legacy.interface_count==2);
  assert(legacy.next_interface_id==3);
  assert(legacy.interfaces[0].attachment==ROUTER_PORT &&
         legacy.interfaces[0].port.vf==11 &&
         !strcmp(legacy.interfaces[0].name,"up0"));
  assert(legacy.interfaces[1].attachment==ROUTER_VSWITCH &&
         legacy.interfaces[1].vswitch_id==200 &&
         !strcmp(legacy.interfaces[1].name,"lan0"));
  /* Legacy state is re-saved in canonical form without changing the model. */
  assert(router_config_save(legacy_path,&legacy,response,sizeof(response)));
  struct router_config rewritten;router_config_init(&rewritten);
  assert(router_config_load(legacy_path,&rewritten,response,sizeof(response)));
  assert(!memcmp(&rewritten,&legacy,sizeof(legacy)));
  assert(unlink(legacy_path)==0);
  assert(unlink(path)==0);assert(rmdir(dir)==0);

  command("vr nat disable --id 100",true);
  command("vr nat show --id 100",true);
  assert(strstr(response,"nat=disabled"));
  command("vr route del --id 100 --prefix 0.0.0.0/0",true);
  command("vr ip del --id 100 --interface p1 --address 200.20.0.4/16",true);
  command("vr port-detach --id 100 --interface p1",true);
  command("vr ip del --id 100 --interface p2 --address 192.168.0.1/24",true);
  command("vr switch-detach --id 100 --interface p2",true);
  command("vr delete --id 100",true);
  assert(router_has_vr(&config,101));
  assert(!router_switch_reserved(&config,200));
  struct router_port_identity released={0,0,11};
  assert(!router_port_reserved(&config,&released));
  printf("PASS: %u command cases plus persistence, isolation and rollback checks\n",checks);
  return 0;
}
