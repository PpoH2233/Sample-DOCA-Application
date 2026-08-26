project(
  'doca-learning-ethernet-switch',
  'c',
  version: '0.1.0',
  meson_version: '>=0.61.2',
  default_options: ['c_std=gnu11', 'buildtype=debug', 'warning_level=2'],
)

add_project_arguments('-D DOCA_ALLOW_EXPERIMENTAL_API', language: 'c')

dependencies = [
  dependency('doca-common', required: true),
  dependency('doca-dpdk-bridge', required: true),
  dependency('doca-flow', required: true),
  dependency('libdpdk', required: true),
]

include_dirs = include_directories(
  '.',
  '../../ethernet_device_discovery',
  '../../doca_device_discovery',
)

executable(
  'ethernet_switch',
  [
    'main.c',
    'switch_devices.c',
    'dpdk_io.c',
    'flow_runtime.c',
    'flow_ports.c',
    'l2_pipeline.c',
    'l2_fdb.c',
    'slow_path.c',
    '../../ethernet_device_discovery/dpdk_runtime.c',
    '../../ethernet_device_discovery/ethernet_ports.c',
    '../../ethernet_device_discovery/ethernet_ports_probe_representors.c',
    '../../doca_device_discovery/open_pci_device.c',
  ],
  dependencies: dependencies,
  include_directories: include_dirs,
  install: false,
)
