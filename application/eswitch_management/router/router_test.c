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
  command("vr ip del --id 100 --interface p1 --address 200.20.0.4/16",false);
  command("vr switch-detach --id 100 --interface p1",false);
  command("vr port-detach --id 100 --interface p1",false);
  command("vr show-interface --id 100",true);
  assert(strstr(response,"vf=11") && strstr(response,"PENDING_DATAPLANE"));
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
  assert(loaded.interface_count==config.interface_count && loaded.next_interface_id==config.next_interface_id);
  for(size_t i=0;i<config.interface_count;i++)
    assert(!memcmp(&loaded.interfaces[i],&config.interfaces[i],sizeof(config.interfaces[i])));
  for(size_t i=0;i<config.route_count;i++)
    assert(!memcmp(&loaded.routes[i],&config.routes[i],sizeof(config.routes[i])));
  /* Malformed restore must not publish partial configuration. */
  FILE *file=fopen(path,"a");assert(file);assert(fputs("vr delete --id 101\n",file)>=0);assert(fclose(file)==0);
  struct router_config before=loaded;
  assert(!router_config_load(path,&loaded,response,sizeof(response)));
  assert(!memcmp(&before,&loaded,sizeof(loaded)));
  assert(unlink(path)==0);assert(rmdir(dir)==0);

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
