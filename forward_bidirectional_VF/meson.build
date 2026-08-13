project(
  'vf-eswitch-forward',
  'c',
  version: '0.1.0',
  license: 'BSD-3-Clause',
  meson_version: '>=0.61.2',
  # DOCA/DPDK helpers use POSIX/GNU functions such as strnlen() and usleep().
  default_options: ['c_std=gnu11', 'buildtype=debugoptimized', 'warning_level=2'],
)

add_project_arguments('-D DOCA_ALLOW_EXPERIMENTAL_API', language: 'c')

doca_dependencies = [
  dependency('doca-common'),
  dependency('doca-flow'),
  dependency('doca-dpdk-bridge'),
  dependency('doca-argp'),
  dependency('libdpdk'),
]

# Keep the DPDK compatibility defines aligned with the shipped DOCA 3.4
# flow_switch_single sample. dpdk_utils.c uses this to select the correct
# rte_ipv6_hdr field layout for the DPDK bundled with the DOCA container.
c_compiler = meson.get_compiler('c')
if c_compiler.has_header_symbol(
  'rte_pmd_mlx5.h',
  'rte_pmd_mlx5_driver_disable_steering',
  dependencies: dependency('libdpdk'),
)
  add_project_arguments('-D DOCA_BUNDLE_DPDK_FOUND', language: 'c')
  message('Found DOCA-bundled DPDK')
elif c_compiler.has_header_symbol(
  'rte_pmd_mlx5.h',
  'rte_pmd_mlx5_disable_steering',
  dependencies: dependency('libdpdk'),
)
  add_project_arguments('-D UPSTREAM_DPDK_FOUND', language: 'c')
  message('Found upstream DPDK')
else
  warning('Unable to identify the installed DPDK flavour')
endif

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
