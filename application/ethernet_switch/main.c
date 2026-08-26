#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#include <doca_error.h>

#include "../../ethernet_device_discovery/dpdk_runtime.h"
#include "dpdk_io.h"
#include "flow_ports.h"
#include "flow_runtime.h"
#include "l2_fdb.h"
#include "l2_pipeline.h"
#include "slow_path.h"
#include "switch_config.h"
#include "switch_devices.h"

static volatile sig_atomic_t stop_requested;

static void request_stop(int signal_number) {
  (void)signal_number;
  stop_requested = 1;
}

static int find_argument_separator(int argc, char **argv) {
  for (int i = 1; i < argc; i++) {
    if (argv[i][0] == '-' && argv[i][1] == '-' && argv[i][2] == '\0')
      return i;
  }
  return -1;
}

static void print_port_map(const struct ethernet_ports *ports) {
  printf("\nSwitch endpoints:\n");
  for (uint16_t i = 0; i < ports->count; i++) {
    const struct ethernet_port *port = &ports->items[i];

    if (port->role == ETHERNET_PORT_ROLE_PARENT) {
      printf("  DPDK port %u -> uplink/parent\n", port->port_id);
    } else {
      printf("  DPDK port %u -> host=%u pf=%u vf=%u representor\n",
             port->port_id, port->host_index, port->pf_index, port->vf_index);
    }
  }
  printf("\n");
}

static void report_cleanup_error(const char *operation, doca_error_t result,
                                 int *exit_status) {
  if (result == DOCA_SUCCESS)
    return;
  fprintf(stderr, "%s: %s\n", operation, doca_error_get_descr(result));
  *exit_status = EXIT_FAILURE;
}

int main(int argc, char **argv) {
  struct switch_devices devices = {0};
  struct dpdk_io io = {0};
  struct flow_runtime flow_runtime = {0};
  struct switch_flow_ports flow_ports = {0};
  struct l2_pipeline pipeline = {0};
  struct l2_fdb fdb = {0};
  struct slow_path slow_path = {0};
  doca_error_t result;
  int separator;
  int exit_status = EXIT_FAILURE;

  separator = find_argument_separator(argc, argv);
  if (separator < 0 || separator + 2 != argc) {
    fprintf(stderr, "Usage: %s [EAL options] -- <DEVICE_PCI>\n", argv[0]);
    fprintf(stderr, "Example: %s -l 0 -- 03:00.0\n", argv[0]);
    return EXIT_FAILURE;
  }

  signal(SIGINT, request_stop);
  signal(SIGTERM, request_stop);

  /*
   * Example startup order:
   *   EAL -> DOCA devices -> DPDK ethdevs -> queues -> DOCA Flow
   *       -> Flow ports -> hardware pipes -> software FDB -> RX loop
   */
  result = dpdk_runtime_init(separator, argv);
  if (result != DOCA_SUCCESS) {
    fprintf(stderr, "Failed to initialize DPDK: %s\n",
            doca_error_get_descr(result));
    return EXIT_FAILURE;
  }
  // probe all doca_rep device
  result =
      switch_devices_open(argv[separator + 1], SWITCH_DPDK_DEVARGS, &devices);
  if (result != DOCA_SUCCESS) {
    fprintf(stderr, "Failed to open and probe switch endpoints: %s\n",
            doca_error_get_descr(result));
    goto cleanup_runtime;
  }
  printf("Opened %zu VF representor(s); probe created %u DPDK ports\n",
         devices.representor_count, devices.ethernet_ports.count);
  print_port_map(&devices.ethernet_ports);

  result = dpdk_io_start(&devices.ethernet_ports, &io);
  if (result != DOCA_SUCCESS) {
    fprintf(stderr, "Failed to configure the DPDK slow path: %s\n",
            doca_error_get_descr(result));
    goto cleanup_devices;
  }

  result = flow_runtime_init(&flow_runtime, SWITCH_MAX_FDB_ENTRIES + 64);
  if (result != DOCA_SUCCESS) {
    fprintf(stderr, "Failed to initialize DOCA Flow: %s\n",
            doca_error_get_descr(result));
    goto cleanup_io;
  }

  result = switch_flow_ports_start(&devices.ethernet_ports, &flow_ports);
  if (result != DOCA_SUCCESS) {
    fprintf(stderr, "Failed to start DOCA Flow ports: %s\n",
            doca_error_get_descr(result));
    goto cleanup_flow_runtime;
  }

  result = l2_pipeline_create(&flow_runtime, &flow_ports, &pipeline);
  if (result != DOCA_SUCCESS) {
    fprintf(stderr, "Failed to create the L2 hardware pipeline: %s\n",
            doca_error_get_descr(result));
    goto cleanup_flow_ports;
  }
  printf("L2 hardware pipeline installed\n");

  result = l2_fdb_init(&pipeline, SWITCH_MAX_FDB_ENTRIES,
                       SWITCH_FDB_AGING_SECONDS, &fdb);
  if (result != DOCA_SUCCESS) {
    fprintf(stderr, "Failed to initialize the software FDB: %s\n",
            doca_error_get_descr(result));
    goto cleanup_pipeline;
  }

  result = slow_path_init(&io, &flow_ports, &fdb, &slow_path);
  if (result != DOCA_SUCCESS) {
    fprintf(stderr, "Failed to initialize the learning slow path: %s\n",
            doca_error_get_descr(result));
    goto cleanup_fdb;
  }

  result = slow_path_run(&slow_path, &stop_requested);
  if (result != DOCA_SUCCESS) {
    fprintf(stderr, "Learning slow path failed: %s\n",
            doca_error_get_descr(result));
    goto cleanup_fdb;
  }
  exit_status = EXIT_SUCCESS;

cleanup_fdb:
  l2_fdb_print(&fdb);
  report_cleanup_error("Failed to remove FDB entries", l2_fdb_destroy(&fdb),
                       &exit_status);
cleanup_pipeline:
  l2_pipeline_destroy(&pipeline);
cleanup_flow_ports:
  report_cleanup_error("Failed to stop DOCA Flow ports",
                       switch_flow_ports_stop(&flow_ports), &exit_status);
cleanup_flow_runtime:
  report_cleanup_error("Failed to destroy DOCA Flow",
                       flow_runtime_destroy(&flow_runtime), &exit_status);
cleanup_io:
  report_cleanup_error("Failed to stop the DPDK parent port", dpdk_io_stop(&io),
                       &exit_status);
cleanup_devices:
  report_cleanup_error("Failed to close switch devices",
                       switch_devices_close(&devices), &exit_status);
  dpdk_io_release(&io);
cleanup_runtime:
  report_cleanup_error("Failed to clean up DPDK", dpdk_runtime_cleanup(),
                       &exit_status);
  return exit_status;
}
