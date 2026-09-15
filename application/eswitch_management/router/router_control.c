#include "../eswitch_manager.h"
#include "router_control.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool router_control_port_reserved(const struct eswitch_manager *m,uint16_t index) {
  if(!m->router || index>=m->ports->count) return false;
  const struct ethernet_port *p=m->ports->items[index].ethernet;
  if(p->role!=ETHERNET_PORT_ROLE_REPRESENTOR) return false;
  struct router_port_identity id={p->host_index,p->pf_index,p->vf_index};
  return router_port_reserved(m->router,&id);
}
static bool inventory_port(void *context,uint16_t port,struct router_port_identity *id) {
  struct eswitch_manager *m=context;
  for(uint16_t i=0;i<m->ports->count;i++) {
    const struct ethernet_port *p=m->ports->items[i].ethernet;
    if(p->port_id!=port) continue;
    if(p->role!=ETHERNET_PORT_ROLE_REPRESENTOR || m->port_owner[i]) return false;
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
static int interface_port_index(const struct eswitch_manager *m,
                                const struct router_interface *rif) {
  if(!m || !rif || rif->attachment!=ROUTER_PORT) return -1;
  for(uint16_t i=0;i<m->ports->count;i++) {
    const struct ethernet_port *p=m->ports->items[i].ethernet;
    if(p->role==ETHERNET_PORT_ROLE_REPRESENTOR &&
       p->host_index==rif->port.host && p->pf_index==rif->port.pf &&
       p->vf_index==rif->port.vf) return i;
  }
  return -1;
}
static const struct router_interface *port_interface_at(
    const struct router_config *config,const struct eswitch_manager *m,
    uint16_t index) {
  if(!config || !m || index>=m->ports->count) return NULL;
  const struct ethernet_port *p=m->ports->items[index].ethernet;
  for(size_t i=0;i<config->interface_count;i++) {
    const struct router_interface *rif=&config->interfaces[i];
    if(rif->attachment==ROUTER_PORT && rif->port.host==p->host_index &&
       rif->port.pf==p->pf_index && rif->port.vf==p->vf_index) return rif;
  }
  return NULL;
}
static bool state_path(const struct eswitch_manager *m,char *out,size_t size) {
  return snprintf(out,size,"%s.router",m->state_path)<(int)size;
}

static const struct router_interface *interface_by_id(
    const struct router_config *config,uint16_t interface_id) {
  for(size_t i=0;i<config->interface_count;i++)
    if(config->interfaces[i].interface_id==interface_id)
      return &config->interfaces[i];
  return NULL;
}

/* Remove selectors for a changed private RIF before publishing its new MAC or
 * address. A later ARP request recreates the local and hardware-eligibility
 * entries from the committed configuration. */
static doca_error_t invalidate_changed_private_rifs(
    struct eswitch_manager *m,const struct router_config *candidate) {
  for(size_t i=0;i<m->router->interface_count;i++) {
    const struct router_interface *before=&m->router->interfaces[i];
    const struct router_interface *after;
    if(before->attachment!=ROUTER_VSWITCH) continue;
    after=interface_by_id(candidate,before->interface_id);
    if(after!=NULL && after->attachment==ROUTER_VSWITCH &&
       after->vr_id==before->vr_id && after->vswitch_id==before->vswitch_id &&
       after->has_address==before->has_address &&
       after->address==before->address && after->prefix==before->prefix &&
       memcmp(after->mac,before->mac,6)==0)
      continue;
    doca_error_t result=eswitch_pipeline_sf_unbind_vswitch(
        m->pipeline,before->vswitch_id);
    if(result!=DOCA_SUCCESS) return result;
  }
  return DOCA_SUCCESS;
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
      if(p->role==ETHERNET_PORT_ROLE_REPRESENTOR && p->host_index==rif->port.host &&
        p->pf_index==rif->port.pf && p->vf_index==rif->port.vf && !m->port_owner[i]) found=true;
    }
    if(!found) {
      fprintf(stderr,"Router restore: VR %u interface %s attachment unavailable or outside probe scope\n",rif->vr_id,rif->name);
      return DOCA_ERROR_NOT_FOUND;
    }
  }
  for(size_t r=0;r<m->router->interface_count;r++) {
    const struct router_interface *rif=&m->router->interfaces[r];
    int index=interface_port_index(m,rif);
    if(rif->attachment!=ROUTER_PORT) continue;
    if(index<0) return DOCA_ERROR_NOT_FOUND;
    doca_error_t result=eswitch_pipeline_attach_router_port(
        m->pipeline,(uint16_t)index,rif->vr_id);
    if(result!=DOCA_SUCCESS) return result;
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
    int removed=-1,added=-1;
    uint16_t removed_vr=0,added_vr=0;
    bool removed_detached=false,added_attached=false;

    /* Any router mutation can invalidate a CT zone, adjacency, or NAT
     * tuple. Remove hardware entries before changing reachable dataplane
     * objects, then remove their software owners. */
    {
      doca_error_t result=eswitch_pipeline_ct_flush(m->pipeline,0);
      if(result!=DOCA_SUCCESS) {
        snprintf(out,size,"ERR hardware CT flush failed: %s\n",
                 doca_error_get_descr(result));
        ok=false;
      } else {
        router_nat_flush(m->nat,0);
      }
    }
    for(uint16_t i=0;i<m->ports->count;i++) {
      const struct router_interface *before=port_interface_at(m->router,m,i);
      const struct router_interface *after=port_interface_at(candidate,m,i);
      if(before && !after) {removed=i;removed_vr=before->vr_id;}
      if(!before && after) {added=i;added_vr=after->vr_id;}
    }
    if(ok && removed>=0) {
      doca_error_t result=eswitch_pipeline_detach_port(m->pipeline,(uint16_t)removed);
      if(result!=DOCA_SUCCESS) {
        snprintf(out,size,"ERR router uplink detach failed: %s\n",doca_error_get_descr(result));
        ok=false;
      } else removed_detached=true;
    }
    if(ok && added>=0) {
      doca_error_t result=eswitch_pipeline_attach_router_port(
          m->pipeline,(uint16_t)added,added_vr);
      if(result!=DOCA_SUCCESS) {
        snprintf(out,size,"ERR router uplink attach failed: %s\n",doca_error_get_descr(result));
        ok=false;
      } else added_attached=true;
    }
    if(ok) {
      doca_error_t result=invalidate_changed_private_rifs(m,candidate);
      if(result!=DOCA_SUCCESS) {
        snprintf(out,size,"ERR private RIF invalidation failed: %s\n",
                 doca_error_get_descr(result));
        ok=false;
      }
    }
    if(ok) {
      doca_error_t result=eswitch_manager_hw_routes_sync(m,candidate);
      if(result!=DOCA_SUCCESS) {
        snprintf(out,size,"ERR hardware route transaction failed: %s\n",
                 doca_error_get_descr(result));
        ok=false;
      }
    }
    if(ok) {
      if(!state_path(m,path,sizeof(path))) {
        snprintf(out,size,"ERR router state path too long\n");ok=false;
      } else ok=router_config_save(path,candidate,out,size);
    }
    if(ok) *m->router=*candidate;
    else {
      (void)eswitch_manager_hw_routes_sync(m,m->router);
      if(added_attached) (void)eswitch_pipeline_detach_port(m->pipeline,(uint16_t)added);
      if(removed_detached) (void)eswitch_pipeline_attach_router_port(
          m->pipeline,(uint16_t)removed,removed_vr);
    }
  }
  free(candidate);
  return ok ? DOCA_SUCCESS : DOCA_ERROR_INVALID_VALUE;
}
