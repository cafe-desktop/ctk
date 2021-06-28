cdk_wayland_sources = files(
  'cdkapplaunchcontext-wayland.c',
  'cdkcursor-wayland.c',
  'cdkdevice-wayland.c',
  'cdkdisplay-wayland.c',
  'cdkdnd-wayland.c',
  'cdkeventsource.c',
  'cdkkeys-wayland.c',
  'cdkglcontext-wayland.c',
  'cdkmonitor-wayland.c',
  'cdkscreen-wayland.c',
  'cdkselection-wayland.c',
  'cdkwindow-wayland.c',
  'wm-button-layout-translation.c',
)

cdk_wayland_public_headers = files(
  'cdkwaylanddevice.h',
  'cdkwaylanddisplay.h',
  'cdkwaylandglcontext.h',
  'cdkwaylandmonitor.h',
  'cdkwaylandselection.h',
  'cdkwaylandwindow.h',
)

install_headers(cdk_wayland_public_headers, subdir: 'ctk-3.0/cdk/wayland')
install_headers('cdkwayland.h', subdir: 'ctk-3.0/cdk')

cdk_wayland_deps = [
  shmlib,
  xkbdep,
  wlclientdep,
  wlprotocolsdep,
  wlcursordep,
  wlegldep,
]

# wayland protocols
proto_dir = dependency('wayland-protocols').get_pkgconfig_variable('pkgdatadir')
assert(proto_dir != '', 'Could not get pkgdatadir from wayland-protocols.pc')

wayland_scanner = find_program('wayland-scanner')

# Format:
#  - protocol name
#  - protocol stability ('private', 'stable' or 'unstable')
#  - protocol version (if stability is 'unstable')
proto_sources = [
  ['ctk-shell', 'private', ],
  ['ctk-primary-selection', 'private', ],
  ['pointer-gestures', 'unstable', 'v1', ],
  ['xdg-shell', 'unstable', 'v6', ],
  ['xdg-shell', 'stable', ],
  ['xdg-foreign', 'unstable', 'v1', ],
  ['tablet', 'unstable', 'v2', ],
  ['keyboard-shortcuts-inhibit', 'unstable', 'v1', ],
  ['server-decoration', 'private' ],
  ['xdg-output', 'unstable', 'v1', ],
  ['primary-selection', 'unstable', 'v1', ],
]

cdk_wayland_gen_headers = []

foreach p: proto_sources
  proto_name = p.get(0)
  proto_stability = p.get(1)

  if proto_stability == 'stable'
    output_base = proto_name
    input = join_paths(proto_dir, '@0@/@1@/@2@.xml'.format(proto_stability, proto_name, output_base))
  elif proto_stability == 'private'
    output_base = proto_name
    input = 'protocol/@0@.xml'.format(proto_name)
  else
    proto_version = p.get(2)
    output_base = '@0@-@1@-@2@'.format(proto_name, proto_stability, proto_version)
    input = join_paths(proto_dir, '@0@/@1@/@2@.xml'.format(proto_stability, proto_name, output_base))
  endif

  cdk_wayland_gen_headers += custom_target('@0@ client header'.format(output_base),
                                           input: input,
                                           output: '@0@-client-protocol.h'.format(output_base),
                                           command: [
                                             wayland_scanner,
                                             'client-header',
                                             '@INPUT@', '@OUTPUT@',
                                           ])

  cdk_wayland_sources += custom_target('@0@ source'.format(output_base),
                                       input: input,
                                       output: '@0@-protocol.c'.format(output_base),
                                       command: [
                                         wayland_scanner,
                                         'private-code',
                                         '@INPUT@', '@OUTPUT@',
                                       ])
endforeach

libcdk_wayland = static_library('cdk-wayland',
                                cdk_wayland_sources, cdk_wayland_gen_headers, cdkconfig, cdkenum_h,
                                include_directories: [ confinc, cdkinc, ],
                                c_args: [
                                  '-DGDK_COMPILATION',
                                  '-DG_LOG_DOMAIN="Gdk"',
                                ] + common_cflags,
                                link_args: common_ldflags,
                                dependencies: [ cdk_deps, cdk_wayland_deps, ])
