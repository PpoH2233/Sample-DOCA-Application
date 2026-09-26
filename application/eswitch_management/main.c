#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <rte_pause.h>
#include <doca_dev.h>
#include <doca_flow_ct.h>

#include "../../ethernet_device_discovery/dpdk_runtime.h"
#include "../ethernet_switch/dpdk_io.h"
#include "../ethernet_switch/flow_ports.h"
#include "../ethernet_switch/flow_runtime.h"
#include "../ethernet_switch/switch_config.h"
#include "../ethernet_switch/switch_devices.h"
#include "control/control_server.h"
#include "eswitch_config.h"
#include "eswitch_build_config.h"
#include "eswitch_manager.h"
#include "pipeline/eswitch_pipeline.h"
#include "pipeline/tx_build.h"
#include "router/sf_packet_io.h"
#include "router/router_nat_ct.h"

static volatile sig_atomic_t stop_requested;

static void request_stop(int signal_number) {
  (void)signal_number;
  stop_requested = 1;
}

static int find_separator(int argc, char **argv) {
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--") == 0)
      return i;
  }
  return -1;
}

static void cleanup_error(const char *operation, doca_error_t result,
                          int *exit_status) {
  if (result == DOCA_SUCCESS)
    return;
  fprintf(stderr, "%s: %s\n", operation, doca_error_get_descr(result));
  *exit_status = EXIT_FAILURE;
}

static void print_inventory(const struct ethernet_ports *ports) {
  printf("\neSwitch endpoint inventory (all start unassigned/DROP):\n");
  for (uint16_t i = 0; i < ports->count; i++) {
    const struct ethernet_port *port = &ports->items[i];
    if (port->role == ETHERNET_PORT_ROLE_PARENT)
      printf("  DPDK port %u -> uplink/parent\n", port->port_id);
    else if (port->role == ETHERNET_PORT_ROLE_SF_REPRESENTOR)
      printf("  DPDK port %u -> Arm system SF (reserved)\n", port->port_id);
    else
      printf("  DPDK port %u -> host=%u pf=%u vf=%u\n", port->port_id,
             port->host_index, port->pf_index, port->vf_index);
  }
  printf("\n");
}

static bool env_enabled(const char *name) {
  const char *value = getenv(name);
  return value != NULL && (strcmp(value, "1") == 0 ||
      strcmp(value, "true") == 0 || strcmp(value, "on") == 0);
}

static bool parse_hw_route_capacity(uint32_t *capacity) {
  const char *value = getenv("ESWITCH_HW_ROUTE_CAPACITY");
  char *end = NULL;
  unsigned long parsed;

  if (capacity == NULL)
    return false;
  if (value == NULL || *value == '\0') {
    *capacity = ESWITCH_HW_ROUTE_DEFAULT_CAPACITY;
    return true;
  }
  parsed = strtoul(value, &end, 10);
  if (*value == '-' || end == value || *end != '\0' ||
      parsed < ESWITCH_HW_ROUTE_MIN_CAPACITY ||
      parsed > ROUTER_HW_MAX_ROUTES || (parsed & (parsed - 1)) != 0)
    return false;
  *capacity = (uint32_t)parsed;
  return true;
}

static bool parse_ct_capacity(uint32_t *capacity) {
  const char *value = getenv("ESWITCH_HW_CT_CAPACITY");
  char *end = NULL;
  unsigned long parsed;

  if (capacity == NULL)
    return false;
  if (value == NULL || *value == '\0') {
    *capacity = ESWITCH_CT_DEFAULT_CAPACITY;
    return true;
  }
  parsed = strtoul(value, &end, 10);
  if (*value == '-' || end == value || *end != '\0' ||
      parsed < ESWITCH_CT_MIN_CAPACITY || parsed > ESWITCH_CT_MAX_CAPACITY ||
      (parsed & (parsed - 1)) != 0)
    return false;
  *capacity = (uint32_t)parsed;
  return true;
}

static bool parse_u32_env(const char *name, uint32_t default_value,
                          uint32_t maximum, uint32_t *value) {
  const char *text = getenv(name);
  char *end = NULL;
  unsigned long parsed;

  if (value == NULL)
    return false;
  if (text == NULL || *text == '\0') {
    *value = default_value;
    return true;
  }
  parsed = strtoul(text, &end, 10);
  if (*text == '-' || end == text || *end != '\0' || parsed > maximum)
    return false;
  *value = (uint32_t)parsed;
  return true;
}

static uint32_t actions_mem_size(bool hardware_routing_enabled,
                                 uint32_t route_capacity,
                                 bool hardware_ct_enabled,
                                 bool uplink_arp_meter_enabled,
                                 uint32_t acl_action_entries) {
  uint32_t required = SWITCH_ACTIONS_MEM_SIZE;
  uint32_t rounded = 1;

  /* The private LPM and dynamic SF return contexts share the parent port's
   * action pool. Each context can require a return rewrite and an eligibility
   * metadata write, so reserving only for LPM entries lets the LPM pipe start
   * successfully but makes the first gateway ARP bind fail with NO_MEMORY. */
  if (hardware_routing_enabled) {
    uint32_t action_entries = route_capacity +
        ESWITCH_MAX_SF_RETURN_CONTEXTS *
            ESWITCH_SF_ACTION_ENTRIES_PER_CONTEXT;
    required += action_entries * DOCA_FLOW_MAX_ENTRY_ACTIONS_MEM_SIZE +
                4096U;
  }
  /* Meter actions and their color dispatch entries share the switch-port
   * action pool even when private LPM promotion is disabled. */
  if (uplink_arp_meter_enabled)
    required += ESWITCH_MAX_VSWITCHES *
                    DOCA_FLOW_MAX_ENTRY_ACTIONS_MEM_SIZE +
                4096U;
  /* Every trunk membership may need one VLAN-push action in its egress gate.
   * Reserve this up front: growing the action pool after a Flow port starts is
   * impossible, and otherwise the first p0 trunk can fail despite the ingress
   * classifier having been installed successfully. */
  required += ESWITCH_MAX_VLAN_MEMBERSHIPS *
                  DOCA_FLOW_MAX_ENTRY_ACTIONS_MEM_SIZE +
              4096U;
  /* ACL pipes allocate action resources when the pipe is created, before
   * any policy entries are installed. Reserve independently of the VLAN
   * budget; otherwise even a two-entry ACL may fail at pipe-create after
   * the base switch pipes have consumed the parent port's action pool. */
  required += acl_action_entries *
                  ESWITCH_ACL_ACTION_MEM_UNITS_PER_ENTRY *
                  DOCA_FLOW_MAX_ENTRY_ACTIONS_MEM_SIZE;
  /* CT owns its L3/L4 action memory, while the post-CT adjacency pipe uses
   * the parent switch-port pool for L2 and TTL rewrites. */
  if (hardware_ct_enabled)
    required += ESWITCH_MAX_CT_ADJACENCIES *
                    DOCA_FLOW_MAX_ENTRY_ACTIONS_MEM_SIZE +
                4096U;
  while (rounded < required)
    rounded <<= 1;
  if (hardware_routing_enabled &&
      rounded < ESWITCH_HW_ACTIONS_MEM_MIN_SIZE)
    rounded = ESWITCH_HW_ACTIONS_MEM_MIN_SIZE;
  return rounded;
}

int main(int argc, char **argv) {
  struct switch_devices devices = {0};
  struct dpdk_io io = {0};
  struct flow_runtime runtime = {0};
  struct switch_flow_ports flow_ports = {0};
  struct eswitch_pipeline pipeline = {0};
  struct eswitch_manager manager = {0};
  struct sf_packet_io sf_io = {.fd = -1};
  struct control_server control = {.listen_fd = -1, .client_fd = -1};
  struct router_nat_ct_runtime ct_runtime = {0};
  const char *socket_path = getenv("ESWITCH_CONTROL_SOCKET");
  const char *state_path = getenv("ESWITCH_STATE_FILE");
  const char *vf_scope = getenv("ESWITCH_VF_SCOPE");
  const char *sf_interface = getenv("ESWITCH_SF_IFACE");
  doca_error_t result;
  doca_error_t ct_capability;
  bool hardware_ct_supported = false;
  bool hardware_routing_enabled = env_enabled("ESWITCH_HW_ROUTING");
  bool hardware_ct_requested = env_enabled("ESWITCH_HW_CT");
  bool packet_debug = env_enabled("ESWITCH_PACKET_DEBUG");
  uint32_t hardware_route_capacity;
  uint32_t hardware_ct_capacity;
  uint32_t uplink_arp_pps;
  uint32_t uplink_arp_burst;
  uint32_t acl_action_entries;
  uint32_t flow_actions_mem_size;
  uint32_t legacy_actions_mem_size;
  int separator;
  int exit_status = EXIT_FAILURE;

  separator = find_separator(argc, argv);
  if (separator < 0 || separator + 2 != argc) {
    fprintf(stderr, "Usage: %s [EAL options] -- <DEVICE_PCI>\n", argv[0]);
    fprintf(stderr, "Example: %s -l 0 -- 03:00.0\n", argv[0]);
    return EXIT_FAILURE;
  }
  if (socket_path == NULL || *socket_path == '\0')
    socket_path = ESWITCH_SOCKET_PATH;
  if (state_path == NULL || *state_path == '\0')
    state_path = ESWITCH_STATE_PATH;
  if (vf_scope == NULL || *vf_scope == '\0')
    vf_scope = ESWITCH_DEFAULT_VF_SCOPE;
  if (sf_interface == NULL || *sf_interface == '\0')
    sf_interface = ESWITCH_DEFAULT_SF_INTERFACE;
  if (!parse_hw_route_capacity(&hardware_route_capacity)) {
    fprintf(stderr, "ESWITCH_HW_ROUTE_CAPACITY must be a power of two "
                    "between %u and %u\n",
            ESWITCH_HW_ROUTE_MIN_CAPACITY, ROUTER_HW_MAX_ROUTES);
    return EXIT_FAILURE;
  }
  if (!parse_ct_capacity(&hardware_ct_capacity)) {
    fprintf(stderr, "ESWITCH_HW_CT_CAPACITY must be a power of two "
                    "between %u and %u\n",
            ESWITCH_CT_MIN_CAPACITY, ESWITCH_CT_MAX_CAPACITY);
    return EXIT_FAILURE;
  }
  if (!parse_u32_env("ESWITCH_ACL_ACTION_ENTRIES",
                     ESWITCH_ACL_ACTION_DEFAULT_ENTRIES,
                     ESWITCH_ACL_ACTION_MAX_ENTRIES, &acl_action_entries) ||
      acl_action_entries == 0) {
    fprintf(stderr, "ESWITCH_ACL_ACTION_ENTRIES must be 1..%u\n",
            ESWITCH_ACL_ACTION_MAX_ENTRIES);
    return EXIT_FAILURE;
  }
  if (!parse_u32_env("ESWITCH_UPLINK_ARP_PPS",
                     ESWITCH_UPLINK_ARP_DEFAULT_PPS,
                     ESWITCH_UPLINK_ARP_MAX_PPS, &uplink_arp_pps) ||
      !parse_u32_env("ESWITCH_UPLINK_ARP_BURST",
                     ESWITCH_UPLINK_ARP_DEFAULT_BURST,
                     ESWITCH_UPLINK_ARP_MAX_PPS, &uplink_arp_burst) ||
      (uplink_arp_pps != 0 && uplink_arp_burst == 0)) {
    fprintf(stderr, "ESWITCH_UPLINK_ARP_PPS and "
                    "ESWITCH_UPLINK_ARP_BURST must be 0..%u; burst must "
                    "be non-zero when rate limiting is enabled\n",
            ESWITCH_UPLINK_ARP_MAX_PPS);
    return EXIT_FAILURE;
  }
  flow_actions_mem_size = actions_mem_size(hardware_routing_enabled,
                                            hardware_route_capacity,
                                            hardware_ct_requested,
                                            uplink_arp_pps != 0,
                                            acl_action_entries);
  legacy_actions_mem_size = actions_mem_size(hardware_routing_enabled,
                                              hardware_route_capacity,
                                              hardware_ct_requested,
                                              uplink_arp_pps != 0, 0);
  signal(SIGINT, request_stop);
  signal(SIGTERM, request_stop);

  result = dpdk_runtime_init(separator, argv);
  if (result != DOCA_SUCCESS) {
    fprintf(stderr, "Failed to initialize DPDK: %s\n",
            doca_error_get_descr(result));
    return EXIT_FAILURE;
  }
  printf("VF probe scope: %s\n", vf_scope);
  result = switch_devices_open_scoped_with_sf(
      argv[separator + 1], SWITCH_DPDK_DEVARGS, vf_scope, &devices);
  if (result != DOCA_SUCCESS) {
    fprintf(stderr,
            "Failed to open/probe eSwitch endpoints with VF scope '%s': %s\n",
            vf_scope, doca_error_get_descr(result));
    goto cleanup_runtime;
  }
  print_inventory(&devices.ethernet_ports);

  /* Runtime capability is authoritative. CT remains disabled until the
   * uplink pipeline can install both directions atomically. */
  ct_capability = doca_flow_ct_cap_is_dev_supported(
      doca_dev_as_devinfo(devices.parent));
  hardware_ct_supported = ct_capability == DOCA_SUCCESS;
  printf("DOCA FLOW CT CAPABILITY: state=%s result=%s\n",
         hardware_ct_supported ? "supported" : "unsupported",
         doca_error_get_descr(ct_capability));

  result = dpdk_io_start(&devices.ethernet_ports, &io);
  if (result != DOCA_SUCCESS) {
    fprintf(stderr, "Failed to start DPDK slow path: %s\n",
            doca_error_get_descr(result));
    goto cleanup_devices;
  }
  printf("TX BUILD: revision=%s compiled=%s %s\n",
         ESWITCH_TX_REVISION, __DATE__, __TIME__);
  printf("TX CONFIG: path=arm-sf-return mode=%s context=rif-mac-to-vswitch "
         "packet-debug=%s\n", ESWITCH_TX_FLOW_MODE,
         packet_debug ? "enabled" : "disabled");
  result = flow_runtime_init_with_mode(&runtime, SWITCH_FLOW_COUNTER_COUNT,
                                       ESWITCH_TX_FLOW_MODE);
  if (result != DOCA_SUCCESS) {
    fprintf(stderr, "Failed to initialize DOCA Flow: %s\n",
            doca_error_get_descr(result));
    goto cleanup_io;
  }
  result = router_nat_ct_init(hardware_ct_requested, hardware_ct_supported,
                              hardware_ct_capacity, &ct_runtime);
  if (result != DOCA_SUCCESS) {
    fprintf(stderr, "DOCA Flow CT unavailable (%s); continuing with Arm "
                    "NAT slow path\n", doca_error_get_descr(result));
  }
  result = switch_flow_ports_start_with_actions_mem(
      &devices.ethernet_ports, flow_actions_mem_size,
      uplink_arp_pps == 0 ? 0 : ESWITCH_MAX_VSWITCHES, &flow_ports);
  if (result == DOCA_ERROR_NO_MEMORY &&
      flow_actions_mem_size > legacy_actions_mem_size) {
    fprintf(stderr, "ACL action-memory reserve unavailable; retrying Flow "
                    "ports with previous budget %u (ACL may fall back to Arm)\n",
            legacy_actions_mem_size);
    flow_actions_mem_size = legacy_actions_mem_size;
    acl_action_entries = 0;
    result = switch_flow_ports_start_with_actions_mem(
        &devices.ethernet_ports, flow_actions_mem_size,
        uplink_arp_pps == 0 ? 0 : ESWITCH_MAX_VSWITCHES, &flow_ports);
  }
  if (result != DOCA_SUCCESS) {
    fprintf(stderr, "Failed to start DOCA Flow ports: %s\n",
            doca_error_get_descr(result));
    goto cleanup_ct;
  }
  /* Follow flow_switch_to_wire: obtain switch domain from the actual parent
   * Flow handle, not the process-global NULL lookup. Parent is item 0,
   * independent of its dynamically discovered DPDK port ID. */
  flow_ports.switch_port = doca_flow_port_switch_get(flow_ports.items[0].flow);
  if (flow_ports.switch_port == NULL) {
    result = DOCA_ERROR_NOT_FOUND;
    fprintf(stderr, "TX DOMAIN ERROR: parent switch handle unavailable\n");
    goto cleanup_flow_ports;
  }
  printf("TX DOMAIN: parent=%u lookup=explicit-parent revision=%s\n",
         flow_ports.items[0].ethernet->port_id, ESWITCH_TX_REVISION);
  printf("HARDWARE ROUTING: configured=%s requested-capacity=%u "
         "actions-mem=%u acl-action-entries=%u scope=private-vs-ipv4\n",
         hardware_routing_enabled ? "enabled" : "disabled",
         hardware_route_capacity, flow_actions_mem_size,
         acl_action_entries);
  printf("HARDWARE CT: configured=%s requested-capacity=%u "
         "initialized=%s protocols=tcp,udp miss=arm\n",
         hardware_ct_requested ? "enabled" : "disabled",
         hardware_ct_capacity,
         ct_runtime.initialized ? "yes" : "no");
  printf("UPLINK ARP CLASSIFIER: configured=%s rate=%u-pps burst=%u "
         "scope=broadcast-arp\n",
         uplink_arp_pps == 0 ? "disabled" : "enabled", uplink_arp_pps,
         uplink_arp_burst);
  result = eswitch_pipeline_create(&runtime, &flow_ports,
                                   hardware_routing_enabled,
                                   hardware_route_capacity,
                                   ct_runtime.initialized,
                                   hardware_ct_capacity, uplink_arp_pps,
                                   uplink_arp_burst, &pipeline);
  if (result != DOCA_SUCCESS) {
    fprintf(stderr, "Failed to create eSwitch pipeline: %s\n",
            doca_error_get_descr(result));
    goto cleanup_flow_ports;
  }
  pipeline.hardware_ct_requested = hardware_ct_requested;
  if (hardware_ct_requested &&
      (!pipeline.hardware_ct_enabled || pipeline.ct_admission_pipe == NULL))
    pipeline.hardware_ct_degraded = true;
  result = sf_packet_io_start(sf_interface, &sf_io);
  if (result != DOCA_SUCCESS) {
    fprintf(stderr, "Failed to start Arm SF packet I/O on %s: %s\n",
            sf_interface, doca_error_get_descr(result));
    goto cleanup_pipeline;
  }
  result = eswitch_manager_init(&io, &flow_ports, &pipeline, &sf_io,
                                hardware_ct_supported,
                                packet_debug,
                                state_path, &manager);
  if (result != DOCA_SUCCESS) {
    fprintf(stderr, "Failed to initialize eSwitch manager: %s\n",
            doca_error_get_descr(result));
    goto cleanup_sf_io;
  }
  result = control_server_start(&control, socket_path,
                                eswitch_manager_command, &manager);
  if (result != DOCA_SUCCESS) {
    fprintf(stderr, "Failed to open control socket %s: %s\n", socket_path,
            doca_error_get_descr(result));
    goto cleanup_manager;
  }

  printf("eSwitch Management ready: socket=%s\n", socket_path);
  printf("All ports are unassigned and DROP until: "
         "eswitchctl vs port attach --id <id> --port <port>\n");
  while (!stop_requested) {
    bool command_work = false;
    bool packet_work = false;

    result = control_server_poll(&control, &command_work);
    if (result != DOCA_SUCCESS) {
      fprintf(stderr, "Control socket failure: %s\n",
              doca_error_get_descr(result));
      break;
    }
    result = eswitch_manager_poll_packets(&manager, &packet_work);
    if (result != DOCA_SUCCESS) {
      fprintf(stderr, "Learning slow path failure: %s\n",
              doca_error_get_descr(result));
      break;
    }
    result = eswitch_manager_maintenance(&manager);
    if (result != DOCA_SUCCESS)
      fprintf(stderr, "FDB maintenance will retry: %s\n",
              doca_error_get_descr(result));
    if (!command_work && !packet_work)
      rte_pause();
  }
  if (stop_requested)
    exit_status = EXIT_SUCCESS;

  control_server_stop(&control);
cleanup_manager:
  cleanup_error("Failed to remove managed vSwitch state",
                eswitch_manager_destroy(&manager), &exit_status);
cleanup_sf_io:
  sf_packet_io_stop(&sf_io);
cleanup_pipeline:
  /* If graceful manager cleanup failed, destroy its per-vSwitch HASH pipes
   * before destroying the shared pipes/gates they reference. */
  eswitch_manager_release(&manager);
  eswitch_pipeline_destroy(&pipeline);
cleanup_flow_ports:
  cleanup_error("Failed to stop DOCA Flow ports",
                switch_flow_ports_stop(&flow_ports), &exit_status);
cleanup_ct:
  router_nat_ct_destroy(&ct_runtime);
  cleanup_error("Failed to destroy DOCA Flow", flow_runtime_destroy(&runtime),
                &exit_status);
cleanup_io:
  cleanup_error("Failed to stop DPDK parent port", dpdk_io_stop(&io),
                &exit_status);
cleanup_devices:
  cleanup_error("Failed to close eSwitch devices",
                switch_devices_close(&devices), &exit_status);
  dpdk_io_release(&io);
cleanup_runtime:
  cleanup_error("Failed to clean up DPDK", dpdk_runtime_cleanup(),
                &exit_status);
  return exit_status;
}
