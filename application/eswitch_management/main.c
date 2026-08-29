#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <rte_pause.h>

#include "../../ethernet_device_discovery/dpdk_runtime.h"
#include "../ethernet_switch/dpdk_io.h"
#include "../ethernet_switch/flow_ports.h"
#include "../ethernet_switch/flow_runtime.h"
#include "../ethernet_switch/switch_config.h"
#include "../ethernet_switch/switch_devices.h"
#include "control_server.h"
#include "eswitch_config.h"
#include "eswitch_manager.h"
#include "eswitch_pipeline.h"

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
    else
      printf("  DPDK port %u -> host=%u pf=%u vf=%u\n", port->port_id,
             port->host_index, port->pf_index, port->vf_index);
  }
  printf("\n");
}

int main(int argc, char **argv) {
  struct switch_devices devices = {0};
  struct dpdk_io io = {0};
  struct flow_runtime runtime = {0};
  struct switch_flow_ports flow_ports = {0};
  struct eswitch_pipeline pipeline = {0};
  struct eswitch_manager manager = {0};
  struct control_server control = {.listen_fd = -1, .client_fd = -1};
  const char *socket_path = getenv("ESWITCH_CONTROL_SOCKET");
  doca_error_t result;
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
  signal(SIGINT, request_stop);
  signal(SIGTERM, request_stop);

  result = dpdk_runtime_init(separator, argv);
  if (result != DOCA_SUCCESS) {
    fprintf(stderr, "Failed to initialize DPDK: %s\n",
            doca_error_get_descr(result));
    return EXIT_FAILURE;
  }
  result = switch_devices_open(argv[separator + 1], SWITCH_DPDK_DEVARGS,
                               &devices);
  if (result != DOCA_SUCCESS) {
    fprintf(stderr, "Failed to open/probe eSwitch endpoints: %s\n",
            doca_error_get_descr(result));
    goto cleanup_runtime;
  }
  print_inventory(&devices.ethernet_ports);

  result = dpdk_io_start(&devices.ethernet_ports, &io);
  if (result != DOCA_SUCCESS) {
    fprintf(stderr, "Failed to start DPDK slow path: %s\n",
            doca_error_get_descr(result));
    goto cleanup_devices;
  }
  result = flow_runtime_init(&runtime, SWITCH_FLOW_COUNTER_COUNT);
  if (result != DOCA_SUCCESS) {
    fprintf(stderr, "Failed to initialize DOCA Flow: %s\n",
            doca_error_get_descr(result));
    goto cleanup_io;
  }
  result = switch_flow_ports_start(&devices.ethernet_ports, &flow_ports);
  if (result != DOCA_SUCCESS) {
    fprintf(stderr, "Failed to start DOCA Flow ports: %s\n",
            doca_error_get_descr(result));
    goto cleanup_flow;
  }
  result = eswitch_pipeline_create(&runtime, &flow_ports, &pipeline);
  if (result != DOCA_SUCCESS) {
    fprintf(stderr, "Failed to create eSwitch pipeline: %s\n",
            doca_error_get_descr(result));
    goto cleanup_flow_ports;
  }
  result = eswitch_manager_init(&io, &flow_ports, &pipeline, &manager);
  if (result != DOCA_SUCCESS) {
    fprintf(stderr, "Failed to initialize eSwitch manager: %s\n",
            doca_error_get_descr(result));
    goto cleanup_pipeline;
  }
  result = control_server_start(&control, socket_path,
                                eswitch_manager_command, &manager);
  if (result != DOCA_SUCCESS) {
    fprintf(stderr, "Failed to open control socket %s: %s\n", socket_path,
            doca_error_get_descr(result));
    goto cleanup_manager;
  }

  printf("eSwitch Management ready: socket=%s\n", socket_path);
  printf("All ports are unassigned and DROP until vs-port-attach.\n");
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
cleanup_pipeline:
  eswitch_pipeline_destroy(&pipeline);
  eswitch_manager_release(&manager);
cleanup_flow_ports:
  cleanup_error("Failed to stop DOCA Flow ports",
                switch_flow_ports_stop(&flow_ports), &exit_status);
cleanup_flow:
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
