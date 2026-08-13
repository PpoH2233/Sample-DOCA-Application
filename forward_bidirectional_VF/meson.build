project(
  'vf-eswitch-forward',
  'c',
  version: '0.1.0',
  license: 'BSD-3-Clause',
  meson_version: '>=0.61.2',
  default_options: ['c_std=c11', 'buildtype=debugoptimized', 'warning_level=2'],
)

add_project_arguments('-D DOCA_ALLOW_EXPERIMENTAL_API', language: 'c')

doca_dependencies = [
  dependency('doca-common'),
  dependency('doca-flow'),
  dependency('doca-dpdk-bridge'),
  dependency('doca-argp'),
  dependency('libdpdk'),
]

vendor_sources = files(
  'vendor/doca/common.c',
  'vendor/doca/flow_common.c',
  'vendor/doca/flow_switch_common.c',
  'vendor/doca/dpdk_utils.c',
)

include_dirs = include_directories(
  'include',
  'vendor/doca',
)

executable(
  'vf-eswitch-forward',
  files('src/main.c', 'src/vf_eswitch.c') + vendor_sources,
  dependencies: doca_dependencies,
  include_directories: include_dirs,
  c_args: ['-Wno-missing-braces'],
  install: true,
)
