deprecated_cdk_sources = files('deprecated/cdkcolor.c')
cdk_sources = files(
  'cdk-private.c',
  'cdk.c',
  'cdkapplaunchcontext.c',
  'cdkcairo.c',
  'cdkcursor.c',
  'cdkdeprecated.c',
  'cdkdevice.c',
  'cdkdevicemanager.c',
  'cdkdevicepad.c',
  'cdkdisplay.c',
  'cdkdisplaymanager.c',
  'cdkdnd.c',
  'cdkevents.c',
  'cdkframetimings.c',
  'cdkgl.c',
  'cdkglcontext.c',
  'cdkglobals.c',
  'cdkkeys.c',
  'cdkkeyuni.c',
  'cdkoffscreenwindow.c',
  'cdkframeclock.c',
  'cdkframeclockidle.c',
  'cdkpango.c',
  'cdkpixbuf-drawable.c',
  'cdkprofiler.c',
  'cdkproperty.c',
  'cdkrectangle.c',
  'cdkrgba.c',
  'cdkscreen.c',
  'cdkselection.c',
  'cdkvisual.c',
  'cdkwindow.c',
  'cdkwindowimpl.c',
  'cdkseat.c',
  'cdkseatdefault.c',
  'cdkdevicetool.c',
  'cdkdrawingcontext.c',
  'cdkmonitor.c'
)

cdk_gir_public_headers = files(
  'cdk.h',
  'cdk-autocleanup.h',
  'cdkapplaunchcontext.h',
  'cdkcairo.h',
  'cdkcursor.h',
  'cdkdevice.h',
  'cdkdevicetool.h',
  'cdkdevicemanager.h',
  'cdkdevicepad.h',
  'cdkdisplay.h',
  'cdkdisplaymanager.h',
  'cdkdnd.h',
  'cdkdrawingcontext.h',
  'cdkevents.h',
  'cdkframetimings.h',
  'cdkglcontext.h',
  'cdkkeys.h',
  'cdkkeysyms.h',
  'cdkmain.h',
  'cdkmonitor.h',
  'cdkpango.h',
  'cdkframeclock.h',
  'cdkpixbuf.h',
  'cdkprivate.h',
  'cdkproperty.h',
  'cdkrectangle.h',
  'cdkrgba.h',
  'cdkscreen.h',
  'cdkseat.h',
  'cdkselection.h',
  'cdktestutils.h',
  'cdkthreads.h',
  'cdktypes.h',
  'cdkvisual.h',
  'cdkwindow.h',
)
cdk_nogir_public_headers = [files('cdkkeysyms-compat.h')]
cdk_public_headers = cdk_gir_public_headers + cdk_nogir_public_headers
install_headers(cdk_public_headers, subdir : 'ctk-3.0/cdk')

deprecated_h_sources = files('deprecated/cdkcolor.h')
install_headers(deprecated_h_sources, subdir : 'ctk-3.0/cdk/deprecated')

# This variable is unused, but added just for completeness
cdk_private_headers = files(
  'cdkseatdefaultprivate.h',
  'cdkdevicetoolprivate.h',
  'cdkdrawingcontextprivate.h',
  'cdkmonitorprivate.h',
  'cdkprivate.h',
)

cdk_wayland_sources = files(
  'wayland/cdkapplaunchcontext-wayland.c',
  'wayland/cdkcursor-wayland.c',
  'wayland/cdkdevice-wayland.c',
  'wayland/cdkdisplay-wayland.c',
  'wayland/cdkdnd-wayland.c',
  'wayland/cdkeventsource.c',
  'wayland/cdkglcontext-wayland.c',
  'wayland/cdkkeys-wayland.c',
  'wayland/cdkmonitor-wayland.c',
  'wayland/cdkscreen-wayland.c',
  'wayland/cdkselection-wayland.c',
  'wayland/cdkwindow-wayland.c',
  'wayland/wm-button-layout-translation.c',
)

cdk_broadway_sources = files([
  'broadway/cdkdisplay-broadway.c',
  'broadway/cdkscreen-broadway.c',
  'broadway/broadway-output.c',
  'broadway/broadway-server.c',
  'broadway/cdkbroadway-server.c',
  'broadway/cdkcursor-broadway.c',
  'broadway/cdkvisual-broadway.c',
  'broadway/cdkselection-broadway.c',
  'broadway/cdkwindow-broadway.c',
  'broadway/cdkmonitor-broadway.c',
  'broadway/cdkkeys-broadway.c',
  'broadway/cdkglobals-broadway.c',
  'broadway/cdkeventsource.c',
  'broadway/cdkdnd-broadway.c',
  'broadway/broadwayd.c',
  'broadway/cdkdevicemanager-broadway.c',
  'broadway/cdkdevice-broadway.c',
  'broadway/broadway-buffer.c',
  'broadway/cdktestutils-broadway.c',
  'broadway/cdkproperty-broadway.c'
])

cdk_gresource_xml = configure_file(output : 'cdk.gresource.xml',
  input : 'gen-cdk-gresources-xml.py',
  command : [find_program('gen-cdk-gresources-xml.py'),
             meson.current_source_dir(), '@OUTPUT@'])

cdkresources = gnome.compile_resources('cdkresources',
  cdk_gresource_xml,
  source_dir: '.',
  c_name: '_cdk',
  extra_args: '--manual-register'
)

cdk_enums = gnome.mkenums('cdkenumtypes',
  sources: cdk_public_headers,
  c_template : 'cdkenumtypes.c.template',
  h_template : 'cdkenumtypes.h.template',
  install_dir: join_paths(ctk_includedir, 'ctk-3.0/cdk'),
  install_header : true)

cdkenum_h = cdk_enums[1]

cdk_marshalers = gnome.genmarshal('cdkmarshalers',
  sources : 'cdkmarshalers.list',
  prefix : '_cdk_marshal',
  valist_marshallers : true)

cdkmarshal_h = cdk_marshalers[1]

cdkconfig_cdata = configuration_data()
cdkconfig_cdata.set('CDK_WINDOWING_X11', x11_enabled)
cdkconfig_cdata.set('CDK_WINDOWING_WAYLAND', wayland_enabled)
cdkconfig_cdata.set('CDK_WINDOWING_WIN32', win32_enabled)
cdkconfig_cdata.set('CDK_WINDOWING_BROADWAY', broadway_enabled)
cdkconfig_cdata.set('CDK_WINDOWING_QUARTZ', quartz_enabled)

cdkconfig = configure_file(
  input  : 'cdkconfig.h.meson',
  output : 'cdkconfig.h',
  configuration : cdkconfig_cdata,
  install_dir: join_paths(ctk_includedir, 'ctk-3.0/cdk'))


cdkversion_cdata = configuration_data()
cdkversion_cdata.set('CTK_MAJOR_VERSION', ctk_major_version)
cdkversion_cdata.set('CTK_MINOR_VERSION', ctk_minor_version)
cdkversion_cdata.set('CTK_MICRO_VERSION', ctk_micro_version)
cdkversion_cdata.set('CTK_API_VERSION', ctk_api_version)
cdkversion_cdata.set('CTK_BINARY_VERSION', ctk_binary_version)
cdkversion_cdata.set('CTK_VERSION', meson.project_version())
cdkversion_cdata.set('LT_CURRENT_MINUS_AGE', ctk_soversion)

cdkversionmacros = configure_file(
  input  : 'cdkversionmacros.h.in',
  output : 'cdkversionmacros.h',
  configuration: cdkversion_cdata,
  install_dir: join_paths(ctk_includedir, 'ctk-3.0/cdk'))

cdkinc = include_directories('.')
cdkx11_inc = include_directories('x11')
cdkwayland_inc = include_directories('wayland')

wlinc = include_directories('.')

cdk_gen_headers = [cdkenum_h, cdkmarshal_h, cdkconfig, cdkversionmacros]

cdk_deps = [
  libm,
  pixbuf_dep,
  cairo_dep,
  pango_dep,
  fribidi_dep,
  cairogobj_dep,
  glib_dep,
  epoxy_dep,
  fontconfig_dep,
  platform_gio_dep,
  pangocairo_dep
]

if win32_enabled
  cdk_deps += [cc.find_library('advapi32'),
               cc.find_library('comctl32'),
               cc.find_library('dwmapi'),
               cc.find_library('imm32'),
               cc.find_library('setupapi'),
               cc.find_library('winmm')]

  cdk_rc = configure_file(
    input: 'win32/rc/cdk.rc.in',
    output: 'cdk.rc',
    configuration: cdkversion_cdata,
  )
  cdk_res = import('windows').compile_resources(cdk_rc, include_directories: include_directories('win32/rc'))
  cdk_sources += cdk_res
endif

if profiler_enabled
  if profiler_dep.found()
    cdk_deps += [profiler_dep]
  endif
endif

cdk_sources = [
  # Generated
  cdkconfig,
  cdk_enums,
  cdk_marshalers,
  cdkresources,
  cdkversionmacros,
  # Static
  cdk_sources,
  deprecated_cdk_sources,
]

if wayland_enabled or broadway_enabled
  if cc.has_function('shm_open')
    shmlib = []
  elif cc.has_function('shm_open', args : '-lrt')
    shmlib = cc.find_library('rt')
  else
    shmlib = []
  endif
endif

cdk_backends = []
cdk_backends_gen_headers = []  # non-public generated headers
foreach backend : ['broadway', 'quartz', 'wayland', 'win32', 'x11']
  if get_variable('@0@_enabled'.format(backend))
    subdir(backend)
    cdk_deps += get_variable('cdk_@0@_deps'.format(backend))
    cdk_backends += get_variable('libcdk_@0@'.format(backend))
    # Special-case this for now to work around Meson bug with get_variable()
    # fallback being an empty array, or any array (#1481)
    if backend == 'wayland'
      cdk_backends_gen_headers += get_variable('cdk_@0@_gen_headers'.format(backend))
    endif
    if backend == 'quartz'
      common_cflags += ['-xobjective-c']
    endif
  endif
endforeach

if cdk_backends.length() == 0
  error('No backends enabled')
endif

cdk_link_args = common_ldflags

# Somehow cdk_win32_display_manager_get_type is not being exported as it
# seems that it was optimized out by the MSVC linker.  Force-export it here.
if cc.get_id() == 'msvc'
  cdk_link_args += [ '-export:cdk_win32_display_manager_get_type' ]
endif

libcdk = library('cdk-3',
                 soversion: ctk_soversion,
                 version: ctk_library_version,
                 sources: [cdk_sources, cdk_backends_gen_headers, cdkconfig],
                 dependencies: cdk_deps,
                 include_directories: [confinc, cdkx11_inc, wlinc],
                 c_args: [
                   '-DCDK_COMPILATION',
                   '-DG_LOG_DOMAIN="Cdk"',
                 ] + common_cflags,
                 link_whole: cdk_backends,
                 link_args: cdk_link_args,
                 darwin_versions : ctk_osxversions,
                 install: true)

cdk_dep_sources = [cdkconfig, cdkenum_h]
# Introspection
if build_gir
  gir_args = ['--quiet']

  # We may build some of the dependencies as sub-projects; this means
  # that we need to depend on the built introspection data, instead of
  # the installed one
  cdk_gir_inc = [ 'cairo-1.0', 'Gio-2.0', ]

  if pixbuf_dep.type_name() == 'internal'
    cdk_gir_inc += subproject('cdk-pixbuf').get_variable('cdkpixbuf_gir').get(0)
  else
    cdk_gir_inc += 'GdkPixbuf-2.0'
  endif

  if pango_dep.type_name() == 'internal'
    cdk_gir_inc += subproject('pango').get_variable('pango_gir').get(0)
  else
    cdk_gir_inc += 'Pango-1.0'
  endif

  cdk_gir_sources = cdk_gir_public_headers + deprecated_h_sources
  cdk_gir_sources += cdk_sources + cdk_enums + [cdkconfig]
  cdk_gir = gnome.generate_gir(libcdk,
                               sources: cdk_gir_sources,
                               namespace: 'Cdk',
                               nsversion: ctk_api_version,
                               identifier_prefix: 'Cdk',
                               symbol_prefix: 'cdk',
                               export_packages: 'cdk-3.0',
                               includes: cdk_gir_inc,
                               header: 'cdk/cdk.h',
                               install: true,
                               extra_args: gir_args + [
                                 '-DCDK_COMPILATION',
                               ])
  cdk_gir_dep = declare_dependency(sources: cdk_gir)
  cdk_dep_sources += cdk_gir

  if x11_enabled
    cdk_x11_gir = gnome.generate_gir(libcdk,
                                     sources: cdk_x11_public_headers + cdk_x11_sources,
                                     namespace: 'CdkX11',
                                     nsversion: ctk_api_version,
                                     identifier_prefix: 'Cdk',
                                     symbol_prefix: 'cdk',
                                     export_packages: 'cdk-x11-3.0',
                                     includes: cdk_gir_inc + [ cdk_gir[0], 'xlib-2.0', ],
                                     install: true,
                                     dependencies: cdk_gir_dep,
                                     header: 'cdk/cdkx.h',
                                     extra_args: gir_args + [
                                       '-DCDK_COMPILATION',
                                     ])
    cdk_dep_sources += cdk_x11_gir
  endif

  if win32_enabled
    cdk_win32_gir = gnome.generate_gir(libcdk,
                                     sources: cdk_win32_public_headers + cdk_win32_sources,
                                     namespace: 'CdkWin32',
                                     nsversion: ctk_api_version,
                                     identifier_prefix: 'Cdk',
                                     symbol_prefix: 'cdk',
                                     export_packages: 'cdk-win32-3.0',
                                     includes: cdk_gir_inc + [cdk_gir[0]],
                                     install: true,
                                     dependencies: cdk_gir_dep,
                                     header: 'cdk/cdkwin32.h',
                                     extra_args: gir_args + [
                                       '-DCDK_COMPILATION',
                                     ])
    cdk_dep_sources += cdk_win32_gir
  endif
endif

libcdk_dep = declare_dependency(
  link_with: libcdk,
  sources: cdk_dep_sources,
  include_directories: [confinc, cdkx11_inc, wlinc],
  dependencies: cdk_deps)
