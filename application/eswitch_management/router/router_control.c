#include "../eswitch_manager.h"
#include "router_control.h"

#include <stdio.h>
#include <stdlib.h>

bool router_control_port_reserved(const struct eswitch_manager *m,uint16_t index) {
  if(!m->router || index>=m->ports->count) return false;
  const struct ethernet_port *p=m->ports->items[index].ethernet;
  if(p->role==ETHERNET_PORT_ROLE_PARENT) return false;
  struct router_port_identity id={p->host_index,p->pf_index,p->vf_index};
  return router_port_reserved(m->router,&id);
}
static bool inventory_port(void *context,uint16_t port,struct router_port_identity *id) {
  struct eswitch_manager *m=context;
  for(uint16_t i=0;i<m->ports->count;i++) {
    const struct ethernet_port *p=m->ports->items[i].ethernet;
    if(p->port_id!=port) continue;
    if(p->role==ETHERNET_PORT_ROLE_PARENT || m->port_owner[i]) return false;
    *id=(struct router_port_identity){p->host_index,p->pf_index,p->vf_index};
    return true;
  }
  return false;
}
static bool inventory_switch(void *context,uint16_t id) {
  struct eswitch_manager *m=context;
  for(size_t i=0;i<ESWITCH_MAX_VSWITCHES;i++)
    if(m->switches[i].exists && m->switches[i].id==id) return true;
  return false;
}
static bool state_path(const struct eswitch_manager *m,char *out,size_t size) {
  return snprintf(out,size,"%s.router",m->state_path)<(int)size;
}
doca_error_t router_control_restore(struct eswitch_manager *m) {
  char path[PATH_MAX],error[256];
  if(!state_path(m,path,sizeof(path))) return DOCA_ERROR_TOO_BIG;
  m->router=malloc(sizeof(*m->router));
  if(!m->router) return DOCA_ERROR_NO_MEMORY;
  router_config_init(m->router);
  if(!router_config_load(path,m->router,error,sizeof(error))) {
    fprintf(stderr,"%s",error); return DOCA_ERROR_INVALID_VALUE;
  }
  for(size_t r=0;r<m->router->interface_count;r++) {
    const struct router_interface *rif=&m->router->interfaces[r];
    bool found=false;
    if(rif->attachment==ROUTER_VSWITCH) found=inventory_switch(m,rif->vswitch_id);
    else for(uint16_t i=0;i<m->ports->count;i++) {
      const struct ethernet_port *p=m->ports->items[i].ethernet;
      if(p->role!=ETHERNET_PORT_ROLE_PARENT && p->host_index==rif->port.host &&
        p->pf_index==rif->port.pf && p->vf_index==rif->port.vf && !m->port_owner[i]) found=true;
    }
    if(!found) {
      fprintf(stderr,"Router restore: VR %u interface %s attachment unavailable or outside probe scope\n",rif->vr_id,rif->name);
      return DOCA_ERROR_NOT_FOUND;
    }
  }
  return DOCA_SUCCESS;
}
doca_error_t router_control_command(struct eswitch_manager *m,const char *request,char *out,size_t size) {
  if (!m->router) {
    snprintf(out,size,"ERR router control is not initialized\n");
    return DOCA_ERROR_BAD_STATE;
  }
  struct router_config *candidate=malloc(sizeof(*candidate));
  if(!candidate) {snprintf(out,size,"ERR out of memory\n");return DOCA_ERROR_NO_MEMORY;}
  *candidate=*m->router;
  struct router_inventory inventory={m,inventory_port,inventory_switch};
  bool changed=false;
  bool ok=router_command(candidate,&inventory,request,out,size,&changed);
  if(ok && changed) {
    char path[PATH_MAX];
    if(!state_path(m,path,sizeof(path))) {
      snprintf(out,size,"ERR router state path too long\n");ok=false;
    } else ok=router_config_save(path,candidate,out,size);
    if(ok) *m->router=*candidate;
  }
  free(candidate);
  return ok ? DOCA_SUCCESS : DOCA_ERROR_INVALID_VALUE;
}
