project('device-discovery', 'c', default_options: ['c_std=c11'])

doca_common = dependency('doca-common', required: true)

executable(
  'device_discovery',
  [
    'main.c',
    'open_pci_device.c',
    'open_rep_pci_device.c',
    'open_rep_vf_device.c',
  ],
  dependencies: doca_common,
)
