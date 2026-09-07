#ifndef ESW_ROUTER_H
#define ESW_ROUTER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define ROUTER_MAX_VRS 64
#define ROUTER_MAX_INTERFACES 256
#define ROUTER_MAX_ROUTES 512
#define ROUTER_NAME_SIZE 32
#define ROUTER_COMMAND_SIZE 512

/* Desired configuration is independent of SDK handles and runtime port IDs.
 * No pointer into a candidate configuration may be retained by a backend. */
struct router_port_identity { uint32_t host, pf, vf; };
enum router_attachment { ROUTER_VSWITCH, ROUTER_PORT };
struct router_interface {
  uint16_t vr_id, interface_id;
  char name[ROUTER_NAME_SIZE];
  enum router_attachment attachment;
  uint16_t vswitch_id;
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
struct router_config {
  uint16_t vr_ids[ROUTER_MAX_VRS];
  size_t vr_count, interface_count, route_count;
  uint32_t next_interface_id;
  struct router_interface interfaces[ROUTER_MAX_INTERFACES];
  struct router_route routes[ROUTER_MAX_ROUTES];
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
bool router_ipv4_prefix(const char *, uint32_t *, uint8_t *);
/* Strict parser shared by eswitchctl and daemon. No mutations on failure. */
bool router_command_valid(const char *, char *, size_t);
/* Works on a transaction candidate. Caller persists before publishing it.
 * Returns false on failure, with an ERR response. changed means config changed.
 * Configured interfaces are deliberately NOT reported as hardware-ready. */
bool router_command(struct router_config *, const struct router_inventory *,
                    const char *, char *, size_t, bool *changed);
bool router_config_save(const char *, const struct router_config *, char *, size_t);
bool router_config_load(const char *, struct router_config *, char *, size_t);

#endif
