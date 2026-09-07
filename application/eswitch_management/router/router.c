#include "router.h"

#include <arpa/inet.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum operation { CREATE, DELETE, SHOW, ATTACH_PORT, ATTACH_SWITCH,
  DETACH_PORT, DETACH_SWITCH, IP_ADD, IP_DEL, ROUTE_ADD, ROUTE_DEL,
  ROUTE_SHOW, MAC_SET };
struct command {
  enum operation op;
  uint16_t id, port, vswitch;
  char name[ROUTER_NAME_SIZE];
  uint32_t address, gateway;
  uint8_t prefix, mac[6];
};

static bool error(char *out, size_t size, const char *message) {
  snprintf(out, size, "ERR %s\n", message);
  return false;
}
static size_t append(char *out, size_t size, size_t used, const char *fmt, ...) {
  if (used >= size) return used;
  va_list args;
  va_start(args, fmt);
  int n = vsnprintf(out + used, size - used, fmt, args);
  va_end(args);
  return n < 0 ? size : used + (size_t)n;
}
static bool number(const char *s, uint16_t *value) {
  if (!s || !*s) return false;
  for (const char *p = s; *p; p++) if (*p < '0' || *p > '9') return false;
  errno = 0;
  char *end;
  unsigned long n = strtoul(s, &end, 10);
  if (errno || *end || n > UINT16_MAX) return false;
  *value = (uint16_t)n;
  return true;
}
static uint32_t mask(uint8_t prefix) {
  return prefix == 0 ? 0 : UINT32_MAX << (32 - prefix);
}
static bool address(const char *s, uint32_t *value) {
  struct in_addr a;
  if (inet_pton(AF_INET, s, &a) != 1) return false;
  *value = ntohl(a.s_addr);
  return true;
}
bool router_ipv4_prefix(const char *s, uint32_t *ip, uint8_t *prefix) {
  char copy[64];
  if (!s || strlen(s) >= sizeof(copy)) return false;
  strcpy(copy, s);
  char *slash = strchr(copy, '/');
  uint16_t n;
  if (!slash) return false;
  *slash++ = 0;
  if (!number(slash, &n) || n > 32 || !address(copy, ip)) return false;
  *prefix = (uint8_t)n;
  return true;
}
static bool valid_name(const char *s) {
  if (!*s || strlen(s) >= ROUTER_NAME_SIZE) return false;
  for (; *s; s++)
    if (!((*s >= 'a' && *s <= 'z') || (*s >= 'A' && *s <= 'Z') ||
          (*s >= '0' && *s <= '9') || *s == '_' || *s == '-')) return false;
  return true;
}
static bool mac_address(const char *s, uint8_t mac[6]) {
  unsigned m[6]; char extra;
  if (strlen(s) != 17 || sscanf(s, "%2x:%2x:%2x:%2x:%2x:%2x%c",
      &m[0], &m[1], &m[2], &m[3], &m[4], &m[5], &extra) != 6) return false;
  unsigned nonzero = 0;
  for (int i = 0; i < 6; i++) { mac[i] = (uint8_t)m[i]; nonzero |= m[i]; }
  return !(mac[0] & 1) && nonzero;
}

static bool parse(const char *request, struct command *c, char *out, size_t size) {
  char text[ROUTER_COMMAND_SIZE], *tokens[32], *save = NULL;
  size_t count = 0, start = 2;
  unsigned allowed = 0, required = 0, found = 0;
  enum { ID=1, NAME=2, PORT=4, SWITCH=8, ADDRESS=16, PREFIX=32, VIA=64, MAC=128 };
  if (!request || strlen(request) >= sizeof(text))
    return error(out, size, "command too long");
  strcpy(text, request);
  for (char *t = strtok_r(text, " \t\r\n", &save); t;
       t = strtok_r(NULL, " \t\r\n", &save)) {
    if (count == 32) return error(out, size, "too many arguments");
    tokens[count++] = t;
  }
  if (count < 2 || strcmp(tokens[0], "vr"))
    return error(out, size, "expected vr <operation>");
  const char *verb = tokens[1];
  if (!strcmp(verb,"create")) { c->op=CREATE; required=ID; }
  else if (!strcmp(verb,"delete")) { c->op=DELETE; required=ID; }
  else if (!strcmp(verb,"show") || !strcmp(verb,"show-interface")) {
    c->op=SHOW; required=ID;
  } else if (!strcmp(verb,"port-attach")) {
    c->op=ATTACH_PORT; required=ID|NAME|PORT;
  } else if (!strcmp(verb,"switch-attach")) {
    c->op=ATTACH_SWITCH; required=ID|NAME|SWITCH;
  } else if (!strcmp(verb,"port-detach") || !strcmp(verb,"switch-detach")) {
    c->op=!strcmp(verb,"port-detach") ? DETACH_PORT : DETACH_SWITCH;
    required=ID|NAME;
  } else if (count > 2 && !strcmp(verb,"ip")) {
    start=3; required=ID|NAME|ADDRESS;
    if (!strcmp(tokens[2],"add")) c->op=IP_ADD;
    else if (!strcmp(tokens[2],"del")) c->op=IP_DEL;
    else return error(out,size,"expected ip add|del");
  } else if (count > 2 && !strcmp(verb,"route")) {
    start=3;
    if (!strcmp(tokens[2],"add")) { c->op=ROUTE_ADD; required=ID|NAME|PREFIX|VIA; }
    else if (!strcmp(tokens[2],"del")) { c->op=ROUTE_DEL; required=ID|PREFIX; }
    else if (!strcmp(tokens[2],"show")) { c->op=ROUTE_SHOW; required=ID; }
    else return error(out,size,"expected route add|del|show");
  } else if (count > 2 && !strcmp(verb,"interface") && !strcmp(tokens[2],"set")) {
    start=3; c->op=MAC_SET; required=ID|NAME|MAC;
  } else return error(out,size,"unsupported vr operation (see router/README.md)");
  allowed=required;
  if ((count-start)%2) return error(out,size,"options require values");
  for (size_t i=start; i<count; i+=2) {
    const char *k=tokens[i], *v=tokens[i+1];
    unsigned bit=0; bool valid=false;
    if (!strcmp(k,"--id")) { bit=ID; valid=number(v,&c->id) && c->id; }
    else if (!strcmp(k,"--name") && (c->op==ATTACH_PORT || c->op==ATTACH_SWITCH)) {
      bit=NAME; valid=valid_name(v); if(valid) strcpy(c->name,v);
    } else if (!strcmp(k,"--interface") && c->op!=ATTACH_PORT && c->op!=ATTACH_SWITCH) {
      bit=NAME; valid=valid_name(v); if(valid) strcpy(c->name,v);
    } else if (!strcmp(k,"--port")) { bit=PORT; valid=number(v,&c->port); }
    else if (!strcmp(k,"--switch-id")) { bit=SWITCH; valid=number(v,&c->vswitch) && c->vswitch; }
    else if (!strcmp(k,"--address")) { bit=ADDRESS; valid=router_ipv4_prefix(v,&c->address,&c->prefix); }
    else if (!strcmp(k,"--prefix")) { bit=PREFIX; valid=router_ipv4_prefix(v,&c->address,&c->prefix); }
    else if (!strcmp(k,"--via")) { bit=VIA; valid=address(v,&c->gateway); }
    else if (!strcmp(k,"--mac")) { bit=MAC; valid=mac_address(v,c->mac); }
    if (!bit || !(allowed&bit) || (found&bit) || !valid)
      return error(out,size,"unknown, duplicate or invalid option");
    found|=bit;
  }
  if (found!=required) return error(out,size,"missing required option");
  if ((c->op==ROUTE_ADD || c->op==ROUTE_DEL) && (c->address & mask(c->prefix))!=c->address)
    return error(out,size,"route prefix must have zero host bits");
  return true;
}
bool router_command_valid(const char *s, char *out, size_t size) {
  struct command c={0}; return parse(s,&c,out,size);
}
void router_config_init(struct router_config *c) {
  memset(c,0,sizeof(*c)); c->next_interface_id=1;
}
bool router_has_vr(const struct router_config *c, uint16_t id) {
  for(size_t i=0;i<c->vr_count;i++) if(c->vr_ids[i]==id) return true;
  return false;
}
static bool same_port(const struct router_port_identity *a,const struct router_port_identity *b) {
  return a->host==b->host && a->pf==b->pf && a->vf==b->vf;
}
bool router_port_reserved(const struct router_config *c,const struct router_port_identity *p) {
  for(size_t i=0;i<c->interface_count;i++)
    if(c->interfaces[i].attachment==ROUTER_PORT && same_port(&c->interfaces[i].port,p)) return true;
  return false;
}
bool router_switch_reserved(const struct router_config *c,uint16_t vs) {
  for(size_t i=0;i<c->interface_count;i++)
    if(c->interfaces[i].attachment==ROUTER_VSWITCH && c->interfaces[i].vswitch_id==vs) return true;
  return false;
}
static struct router_interface *interface(struct router_config *c,uint16_t vr,const char *name) {
  for(size_t i=0;i<c->interface_count;i++)
    if(c->interfaces[i].vr_id==vr && !strcmp(c->interfaces[i].name,name)) return &c->interfaces[i];
  return NULL;
}
static const char *iptext(uint32_t ip,char text[INET_ADDRSTRLEN]) {
  struct in_addr a={.s_addr=htonl(ip)};
  return inet_ntop(AF_INET,&a,text,INET_ADDRSTRLEN);
}
static bool usable_address(uint32_t ip,uint8_t prefix) {
  if (!ip || (ip>>24)==127 || (ip>>24)==0 || (ip>>28)>=14 || prefix==0) return false;
  if(prefix<31) {
    uint32_t host=ip&~mask(prefix);
    if(!host || host==~mask(prefix)) return false;
  }
  return true;
}
static bool routes_reference(const struct router_config *c,uint16_t rif) {
  for(size_t i=0;i<c->route_count;i++) if(c->routes[i].interface_id==rif) return true;
  return false;
}
bool router_command(struct router_config *c,const struct router_inventory *inv,
                    const char *request,char *out,size_t size,bool *changed) {
  struct command q={0}; *changed=false;
  if(!parse(request,&q,out,size)) return false;
  bool exists=router_has_vr(c,q.id);
  if(q.op==CREATE) {
    if(exists) return error(out,size,"VR already exists");
    if(c->vr_count==ROUTER_MAX_VRS) return error(out,size,"VR capacity reached");
    c->vr_ids[c->vr_count++]=q.id;
  } else {
    if(!exists) return error(out,size,"VR not found");
    struct router_interface *rif=interface(c,q.id,q.name);
    if(q.op==SHOW || q.op==ROUTE_SHOW) {
      size_t used=append(out,size,0,"OK vr=%u dataplane=NOT_IMPLEMENTED\n",q.id);
      for(size_t i=0;i<c->interface_count;i++) {
        const struct router_interface *r=&c->interfaces[i]; char ip[INET_ADDRSTRLEN];
        if(r->vr_id!=q.id) continue;
        if(q.op==SHOW) {
          used=append(out,size,used,"%s ifindex=%u type=%s ",r->name,r->interface_id,
            r->attachment==ROUTER_PORT?"port-link":"vs-link");
          if(r->attachment==ROUTER_PORT)
            used=append(out,size,used,"host=%u pf=%u vf=%u ",r->port.host,r->port.pf,r->port.vf);
          else used=append(out,size,used,"switch=%u ",r->vswitch_id);
          used=append(out,size,used,"mac=%02x:%02x:%02x:%02x:%02x:%02x ",
            r->mac[0],r->mac[1],r->mac[2],r->mac[3],r->mac[4],r->mac[5]);
          if(r->has_address) used=append(out,size,used,"address=%s/%u status=PENDING_DATAPLANE arp=%s\n",iptext(r->address,ip),r->prefix,
            r->attachment==ROUTER_VSWITCH ? "PRIVATE_GATEWAY_ENABLED" : "NOT_IMPLEMENTED");
          else used=append(out,size,used,"address=- status=NO_ADDRESS\n");
        } else if(r->has_address)
          used=append(out,size,used,"connected %s/%u interface=%s\n",iptext(r->address&mask(r->prefix),ip),r->prefix,r->name);
      }
      if(q.op==ROUTE_SHOW) for(size_t i=0;i<c->route_count;i++) {
        const struct router_route *r=&c->routes[i]; char ip[INET_ADDRSTRLEN],gw[INET_ADDRSTRLEN];
        if(r->vr_id==q.id) used=append(out,size,used,"static %s/%u via=%s ifindex=%u\n",iptext(r->prefix,ip),r->length,iptext(r->gateway,gw),r->interface_id);
      }
      if(used>=size) return error(out,size,"response too large");
      return true;
    } else if(q.op==DELETE) {
      for(size_t i=0;i<c->interface_count;i++) if(c->interfaces[i].vr_id==q.id)
        return error(out,size,"detach interfaces before deleting VR");
      for(size_t i=0;i<c->vr_count;i++) if(c->vr_ids[i]==q.id) {
        memmove(&c->vr_ids[i],&c->vr_ids[i+1],(--c->vr_count-i)*sizeof(c->vr_ids[0])); break;
      }
    } else if(q.op==ATTACH_PORT || q.op==ATTACH_SWITCH) {
      if(rif) return error(out,size,"interface name already exists in VR");
      if(c->interface_count==ROUTER_MAX_INTERFACES || c->next_interface_id>UINT16_MAX)
        return error(out,size,"interface capacity reached");
      struct router_interface r={.vr_id=q.id,.interface_id=(uint16_t)c->next_interface_id};
      strcpy(r.name,q.name);
      if(q.op==ATTACH_PORT) {
        r.attachment=ROUTER_PORT;
        if(!inv || !inv->port || !inv->port(inv->context,q.port,&r.port))
          return error(out,size,"port must be an available, in-scope VF representor");
        if(router_port_reserved(c,&r.port)) return error(out,size,"port already attached to a VR");
        for(size_t i=0;i<c->interface_count;i++)
          if(c->interfaces[i].vr_id==q.id && c->interfaces[i].attachment==ROUTER_PORT)
            return error(out,size,"MVP permits one public uplink per VR");
      } else {
        r.attachment=ROUTER_VSWITCH; r.vswitch_id=q.vswitch;
        if(!inv || !inv->switch_exists || !inv->switch_exists(inv->context,q.vswitch))
          return error(out,size,"vSwitch not found");
        if(router_switch_reserved(c,q.vswitch)) return error(out,size,"vSwitch already attached to a VR");
      }
      r.mac[0]=2; r.mac[1]=0; r.mac[2]=(uint8_t)(q.id>>8); r.mac[3]=(uint8_t)q.id;
      r.mac[4]=(uint8_t)(r.interface_id>>8); r.mac[5]=(uint8_t)r.interface_id;
      c->interfaces[c->interface_count++]=r; c->next_interface_id++;
    } else if(q.op==ROUTE_DEL) {
      size_t i;
      for(i=0;i<c->route_count;i++) if(c->routes[i].vr_id==q.id && c->routes[i].prefix==q.address && c->routes[i].length==q.prefix) break;
      if(i==c->route_count) return error(out,size,"static route not found (connected routes follow interface IP)");
      memmove(&c->routes[i],&c->routes[i+1],(--c->route_count-i)*sizeof(c->routes[0]));
    } else {
      if(!rif) return error(out,size,"interface not found in VR");
      if(q.op==DETACH_PORT || q.op==DETACH_SWITCH) {
        if((q.op==DETACH_PORT)!=(rif->attachment==ROUTER_PORT)) return error(out,size,"attachment type mismatch");
        if(rif->has_address || routes_reference(c,rif->interface_id)) return error(out,size,"remove IP and route dependencies before detach");
        size_t i=(size_t)(rif-c->interfaces);
        memmove(rif,rif+1,(--c->interface_count-i)*sizeof(*rif));
      } else if(q.op==IP_ADD) {
        if(rif->has_address) return error(out,size,"one IPv4 address per interface in MVP; delete existing address first");
        if(!usable_address(q.address,q.prefix)) return error(out,size,"invalid unicast interface address");
        for(size_t i=0;i<c->interface_count;i++) {
          const struct router_interface *r=&c->interfaces[i];
          uint8_t len=r->prefix<q.prefix?r->prefix:q.prefix;
          if(r->vr_id==q.id && r->has_address && (r->address&mask(len))==(q.address&mask(len)))
            return error(out,size,"overlapping interface subnets within VR are not supported");
        }
        rif->has_address=true; rif->address=q.address; rif->prefix=q.prefix;
      } else if(q.op==IP_DEL) {
        if(!rif->has_address || rif->address!=q.address || rif->prefix!=q.prefix) return error(out,size,"address not found");
        if(routes_reference(c,rif->interface_id)) return error(out,size,"remove dependent static routes first");
        rif->has_address=false; rif->address=0; rif->prefix=0;
      } else if(q.op==MAC_SET) {
        for(size_t i=0;i<c->interface_count;i++)
          if(&c->interfaces[i]!=rif && !memcmp(c->interfaces[i].mac,q.mac,6))
            return error(out,size,"MAC already assigned to another RIF");
        memcpy(rif->mac,q.mac,6);
      } else if(q.op==ROUTE_ADD) {
        if(!rif->has_address || !usable_address(q.gateway,rif->prefix) || q.gateway==rif->address ||
            (q.gateway&mask(rif->prefix))!=(rif->address&mask(rif->prefix)))
          return error(out,size,"gateway must be an on-link neighbor of addressed interface");
        if(c->route_count==ROUTER_MAX_ROUTES) return error(out,size,"route capacity reached");
        for(size_t i=0;i<c->route_count;i++) if(c->routes[i].vr_id==q.id && c->routes[i].prefix==q.address && c->routes[i].length==q.prefix)
          return error(out,size,"static route already exists");
        c->routes[c->route_count++]=(struct router_route){.vr_id=q.id,.interface_id=rif->interface_id,
          .prefix=q.address,.gateway=q.gateway,.length=q.prefix};
      }
    }
  }
  *changed=true;
  snprintf(out,size,"OK configuration staged; dataplane=NOT_IMPLEMENTED\n");
  return true;
}
