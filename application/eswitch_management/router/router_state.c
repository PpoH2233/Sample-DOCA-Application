#include "router.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Versioned text, using stable host/PF/VF identity, never a runtime DPDK ID.
 * Load replays the public model validator into an isolated candidate. */
static bool fail(char *out,size_t size,const char *s) {
  snprintf(out,size,"ERR router state: %s\n",s); return false;
}
static const char *ipstr(uint32_t ip,char out[INET_ADDRSTRLEN]) {
  struct in_addr a={.s_addr=htonl(ip)}; return inet_ntop(AF_INET,&a,out,INET_ADDRSTRLEN);
}
bool router_config_save(const char *path,const struct router_config *c,char *out,size_t size) {
  char tmp[PATH_MAX];
  if(snprintf(tmp,sizeof(tmp),"%s.tmp.XXXXXX",path)>=(int)sizeof(tmp)) return fail(out,size,"path too long");
  int fd=mkstemp(tmp);
  if(fd<0) return fail(out,size,strerror(errno));
  FILE *f=fdopen(fd,"w");
  if(!f) {close(fd); unlink(tmp); return fail(out,size,strerror(errno));}
  fprintf(f,"router-state 1\nnext %u\n",c->next_interface_id);
  for(size_t i=0;i<c->vr_count;i++) fprintf(f,"vr create --id %u\n",c->vr_ids[i]);
  for(size_t i=0;i<c->interface_count;i++) {
    const struct router_interface *r=&c->interfaces[i]; char ip[INET_ADDRSTRLEN];
    fprintf(f,"identity %u %u %u %u\n",r->interface_id,r->port.host,r->port.pf,r->port.vf);
    if(r->attachment==ROUTER_PORT)
      fprintf(f,"vr port-attach --id %u --name %s --port 0\n",r->vr_id,r->name);
    else fprintf(f,"vr switch-attach --id %u --name %s --switch-id %u\n",r->vr_id,r->name,r->vswitch_id);
    fprintf(f,"vr interface set --id %u --interface %s --mac %02x:%02x:%02x:%02x:%02x:%02x\n",
      r->vr_id,r->name,r->mac[0],r->mac[1],r->mac[2],r->mac[3],r->mac[4],r->mac[5]);
    if(r->has_address) fprintf(f,"vr ip add --id %u --interface %s --address %s/%u\n",
      r->vr_id,r->name,ipstr(r->address,ip),r->prefix);
  }
  for(size_t i=0;i<c->route_count;i++) {
    const struct router_route *r=&c->routes[i]; char ip[INET_ADDRSTRLEN],gw[INET_ADDRSTRLEN];
    const char *name=NULL;
    for(size_t j=0;j<c->interface_count;j++) if(c->interfaces[j].interface_id==r->interface_id) name=c->interfaces[j].name;
    if(!name) {fclose(f);unlink(tmp);return fail(out,size,"dangling route reference");}
    fprintf(f,"vr route add --id %u --interface %s --prefix %s/%u --via %s\n",
      r->vr_id,name,ipstr(r->prefix,ip),r->length,ipstr(r->gateway,gw));
  }
  bool ok=!ferror(f) && fflush(f)==0 && fsync(fd)==0;
  if(fclose(f)!=0) ok=false;
  if(!ok || rename(tmp,path)!=0) {unlink(tmp);return fail(out,size,strerror(errno));}
  char parent[PATH_MAX]; snprintf(parent,sizeof(parent),"%s",path);
  char *slash=strrchr(parent,'/');
  if(!slash) strcpy(parent,".");
  else if(slash==parent) slash[1]=0;
  else *slash=0;
  fd=open(parent,O_RDONLY|O_DIRECTORY);
  if(fd<0 || fsync(fd)!=0) fprintf(stderr,"Warning: router config renamed but directory fsync failed: %s\n",path);
  if(fd>=0) close(fd);
  return true; /* rename is commit point; never roll back only RAM afterwards */
}
static bool replay_port(void *ctx,uint16_t port,struct router_port_identity *out) {
  if(port!=0) return false;
  *out=*(const struct router_port_identity *)ctx; return true;
}
static bool replay_switch(void *ctx,uint16_t id) { (void)ctx; return id!=0; }
static bool uints(char *line,const char *tag,uint32_t *values,size_t count) {
  char *save=NULL,*t=strtok_r(line," \t\r\n",&save);
  if(!t || strcmp(t,tag)) return false;
  for(size_t i=0;i<count;i++) {
    t=strtok_r(NULL," \t\r\n",&save);
    if(!t || !*t) return false;
    for(char *p=t;*p;p++) if(*p<'0'||*p>'9') return false;
    errno=0; char *end; unsigned long long n=strtoull(t,&end,10);
    if(errno || *end || n>UINT32_MAX) return false;
    values[i]=(uint32_t)n;
  }
  return strtok_r(NULL," \t\r\n",&save)==NULL;
}
bool router_config_load(const char *path,struct router_config *config,char *out,size_t size) {
  FILE *f=fopen(path,"r");
  if(!f) return errno==ENOENT ? true : fail(out,size,strerror(errno));
  struct router_config *c=malloc(sizeof(*c));
  if(!c) {fclose(f);return fail(out,size,"out of memory");}
  router_config_init(c);
  char line[ROUTER_COMMAND_SIZE], copy[ROUTER_COMMAND_SIZE];
  uint32_t next=0, identity=0;
  struct router_port_identity port={0};
  struct router_inventory inv={.context=&port,.port=replay_port,.switch_exists=replay_switch};
  bool ok=fgets(line,sizeof(line),f) && !strcmp(line,"router-state 1\n");
  if(ok) ok=fgets(line,sizeof(line),f) && uints(line,"next",&next,1) && next>0 && next<=65536;
  while(ok && fgets(line,sizeof(line),f)) {
    if(!strchr(line,'\n')) {ok=false;break;}
    if(!strncmp(line,"identity ",9)) {
      uint32_t vals[4];
      if(identity || !uints(line,"identity",vals,4) || !vals[0] || vals[0]>=next) {ok=false;break;}
      identity=vals[0]; port=(struct router_port_identity){vals[1],vals[2],vals[3]};
      for(size_t i=0;i<c->interface_count;i++) if(c->interfaces[i].interface_id==identity) ok=false;
      continue;
    }
    bool attachment=!strncmp(line,"vr port-attach ",15) || !strncmp(line,"vr switch-attach ",17);
    bool permitted=attachment || !strncmp(line,"vr create ",10) ||
      !strncmp(line,"vr interface set ",17) || !strncmp(line,"vr ip add ",10) || !strncmp(line,"vr route add ",13);
    if(!permitted || (attachment!=(identity!=0))) {ok=false;break;}
    if(attachment) c->next_interface_id=identity;
    bool changed=false;
    snprintf(copy,sizeof(copy),"%s",line);
    ok=router_command(c,&inv,copy,out,size,&changed) && changed;
    if(attachment) identity=0;
  }
  if(ferror(f) || identity) ok=false;
  fclose(f);
  if(ok) {c->next_interface_id=next;*config=*c;}
  free(c);
  return ok ? true : fail(out,size,"invalid configuration (nothing loaded)");
}
