#ifndef ESW_ROUTER_H
#define ESW_ROUTER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define ROUTER_MAX_VRS 64
#define ROUTER_MAX_INTERFACES 256
#define ROUTER_MAX_ROUTES 512
#define ROUTER_MAX_LINKS 128
#define ROUTER_MAX_NAT_POLICIES ROUTER_MAX_VRS
#define ROUTER_MAX_PORT_FORWARDS 128
#define ROUTER_NAME_SIZE 32
#define ROUTER_COMMAND_SIZE 512

/* Desired configuration is independent of SDK handles and runtime port IDs.
 * No pointer into a candidate configuration may be retained by a backend. */
struct router_port_identity { uint32_t host, pf, vf; };
enum router_attachment { ROUTER_VSWITCH, ROUTER_PORT, ROUTER_LINK };
struct router_interface {
  uint16_t vr_id, interface_id;
  char name[ROUTER_NAME_SIZE];
  enum router_attachment attachment;
  uint16_t vswitch_id;
  uint16_t link_id;
  struct router_port_identity port;
  uint8_t mac[6];
  bool has_address;
  uint32_t address; /* host byte order */
  uint8_t prefix;
};
struct router_route {
  uint16_t vr_id, interface_id;
  uint32_t prefix, gateway; /* host byte order */
  uint8_t length;
};
struct router_nat_policy {
  uint16_t vr_id, interface_id;
  uint32_t public_address; /* host byte order; 0 means use RIF address */
  uint16_t port_first, port_last;
};
struct router_port_forward {
  uint16_t vr_id, rule_id, interface_id;
  uint8_t protocol; /* TCP=6 or UDP=17 */
  uint16_t public_port, private_port;
  uint32_t public_ip, private_ip; /* host byte order */
};
struct router_config {
  uint16_t vr_ids[ROUTER_MAX_VRS];
  uint16_t link_ids[ROUTER_MAX_LINKS];
  size_t vr_count, link_count, interface_count, route_count, nat_policy_count;
  size_t port_forward_count;
  uint32_t next_interface_id;
  struct router_interface interfaces[ROUTER_MAX_INTERFACES];
  struct router_route routes[ROUTER_MAX_ROUTES];
  struct router_nat_policy nat_policies[ROUTER_MAX_NAT_POLICIES];
  struct router_port_forward port_forwards[ROUTER_MAX_PORT_FORWARDS];
};
/* Callbacks validate attachments against the manager's current inventory.
 * Public ports must be representors and not owned by an L2 switch. */
struct router_inventory {
  void *context;
  bool (*port)(void *, uint16_t, struct router_port_identity *);
  bool (*switch_exists)(void *, uint16_t);
};

void router_config_init(struct router_config *config);
bool router_port_reserved(const struct router_config *,
                          const struct router_port_identity *);
bool router_switch_reserved(const struct router_config *, uint16_t);
bool router_has_vr(const struct router_config *, uint16_t);
bool router_has_link(const struct router_config *, uint16_t);
/* Return the other endpoint of a two-ended logical router link. */
const struct router_interface *router_link_peer(const struct router_config *,
                                                uint16_t interface_id);
const struct router_nat_policy *router_nat_policy_find(
    const struct router_config *, uint16_t vr_id);
uint32_t router_nat_policy_address(const struct router_config *,
                                   const struct router_nat_policy *);
bool router_port_forward_uses_interface(const struct router_config *,
                                        uint16_t interface_id);
bool router_ipv4_prefix(const char *, uint32_t *, uint8_t *);
/* Strict parser shared by eswitchctl and daemon. No mutations on failure. */
bool router_command_valid(const char *, char *, size_t);
/* Works on a transaction candidate. Caller persists before publishing it.
 * Returns false on failure, with an ERR response. changed means config changed.
 * Private VS interfaces report the Arm LPM milestone. Public interfaces are
 * steered to Arm and become active for TCP/UDP/ICMP Echo NAT when a policy
 * exists. */
bool router_command(struct router_config *, const struct router_inventory *,
                    const char *, char *, size_t, bool *changed);
bool router_config_save(const char *, const struct router_config *, char *, size_t);
bool router_config_load(const char *, struct router_config *, char *, size_t);

#endif
