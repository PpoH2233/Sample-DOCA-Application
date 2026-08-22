project(
  'ethernet-device-discovery',
  'c',
  version: '0.1.0',
  meson_version: '>=0.61.2',
  default_options: ['c_std=gnu11', 'buildtype=debug', 'warning_level=2'],
)

add_project_arguments('-D DOCA_ALLOW_EXPERIMENTAL_API', language: 'c')

dependencies = [
  dependency('doca-common', required: true),
  dependency('doca-dpdk-bridge', required: true),
  dependency('libdpdk', required: true),
]

executable(
  'ethernet_device_discovery',
  [
    'main.c',
    'dpdk_runtime.c',
    'ethernet_ports.c',
    'ethernet_ports_probe_representors.c',
    '../doca_device_discovery/open_pci_device.c',
  ],
  dependencies: dependencies,
)
