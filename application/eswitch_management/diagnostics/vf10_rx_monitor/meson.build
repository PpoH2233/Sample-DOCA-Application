project('vf10-rx-monitor', 'c', version: '0.1.0',
  meson_version: '>=0.61.2',
  default_options: ['c_std=gnu11', 'buildtype=debug', 'warning_level=2'])

sdk = get_option('sdk_root')
samples = sdk / 'samples'
flow = samples / 'doca_flow'
common = sdk / 'applications/common'

deps = [
  dependency('doca-common'),
  dependency('doca-argp'),
  dependency('doca-flow'),
  dependency('doca-dpdk-bridge'),
  dependency('libdpdk'),
]

add_project_arguments('-DDOCA_ALLOW_EXPERIMENTAL_API', language: 'c')

executable('vf10-rx-monitor',
  [
    'main.c',
    'vf10_rx_monitor.c',
    flow / 'flow_common.c',
    flow / 'flow_switch_common.c',
    samples / 'common.c',
    common / 'dpdk_utils.c',
    common / 'utils.c',
  ],
  include_directories: include_directories(samples, flow, common),
  dependencies: deps,
  install: false,
)
