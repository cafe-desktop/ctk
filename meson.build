project('ctk+-3.0', 'c',
        version: '3.25.6',
        default_options: [
          'buildtype=debugoptimized',
          'warning_level=1'
        ],
        meson_version : '>= 0.48.0',
        license: 'LGPLv2.1+')

glib_major_req = 2
glib_minor_req = 66
glib_micro_req = 8

if glib_minor_req.is_odd()
  glib_min_required = 'GLIB_VERSION_@0@_@1@'.format(glib_major_req, glib_minor_req - 1)
else
  glib_min_required = 'GLIB_VERSION_@0@_@1@'.format(glib_major_req, glib_minor_req)
endif

if glib_minor_req.is_odd()
  glib_max_allowed = 'GLIB_VERSION_@0@_@1@'.format(glib_major_req, glib_minor_req + 1)
else
  glib_max_allowed = 'GLIB_VERSION_@0@_@1@'.format(glib_major_req, glib_minor_req)
endif

glib_req           = '>= @0@.@1@.@2@'.format(glib_major_req, glib_minor_req, glib_micro_req)
pango_req          = '>= 1.41.0'
fribidi_req        = '>= 0.19.7'
atk_req            = '>= 2.35.1'
at_spi2_atk_req    = '>= 2.15.1'
cairo_req          = '>= 1.14.0'
gdk_pixbuf_req     = '>= 2.30.0'
introspection_req  = '>= 1.39.0'
wayland_proto_req  = '>= 1.17'
wayland_req        = '>= 1.14.91'
epoxy_req          = '>= 1.4'
cloudproviders_req = '>= 0.2.5'
xkbcommon_req      = '>= 0.2.0'

gnome = import('gnome')

add_project_arguments('-DG_LOG_USE_STRUCTURED=1', language: 'c')
add_project_arguments('-DGLIB_MIN_REQUIRED_VERSION=' + glib_min_required, language: 'c')
add_project_arguments('-DGLIB_MAX_ALLOWED_VERSION=' + glib_max_allowed, language: 'c')

# Making releases:
#  1. ctk_micro_version += 1;
#  2. ctk_interface_age += 1;
#  3. if any functions have been added, set ctk_interface_age to 0.
#  4. if backwards compatibility has been broken, we're in trouble
ctk_version       = meson.project_version()
ctk_major_version = ctk_version.split('.')[0].to_int()
ctk_minor_version = ctk_version.split('.')[1].to_int()
ctk_micro_version = ctk_version.split('.')[2].to_int()
ctk_interface_age = 4
add_project_arguments('-DCTK_VERSION="@0@"'.format(meson.project_version()), language: 'c')

add_project_arguments('-D_GNU_SOURCE', language: 'c')

ctk_debug_cflags = []
debug = get_option('debug')
optimization = get_option('optimization')
if debug
  ctk_debug_cflags += '-DG_ENABLE_DEBUG'
  if optimization in ['0', 'g']
    ctk_debug_cflags += '-DG_ENABLE_CONSISTENCY_CHECKS'
  endif
elif optimization in ['2', '3', 's']
  ctk_debug_cflags += '-DG_DISABLE_CAST_CHECKS'
endif

add_project_arguments(ctk_debug_cflags, language: 'c')

# Define a string for the earliest version that this release has
# backwards binary compatibility with for all interfaces a module
# might. Unless we add module-only API with lower stability
# guarantees, this should be unchanged until we break binary compat
# for CTK+.
ctk_binary_version = '3.0.0'

ctk_binary_age = 100 * ctk_minor_version + ctk_micro_version

cc = meson.get_compiler('c')

if cc.get_id() == 'msvc'
  vsver = 0
  mscver = cc.version().split('.')[0].to_int()

  # pre-Visual Studio 2015 (18.xx.xxxxx or earlier): just subtract 6 from major
  # version of cc.version() to get the Visual Studio version
  if mscver < 19
    vsver = mscver - 6
  else
    # Visual Studio 2015 and later (19.xx.xxxxx or later): look at the minor version.
    # If minor version < 10: Visual Studio 2015,
    #    10 < minor version < 20: Visual Studio 2017,
    #    20 < minor version: Visual Studio 2019
    mscsubver = cc.version().split('.')[1].to_int()
    if mscsubver < 10
      vsver = 14
    elif mscsubver < 20
      vsver = 15
    else
      vsver = 16
    endif
  endif

  ctk_soversion = 'vs@0@'.format(vsver)
else
  ctk_soversion = '0'
endif

# Maintain compatibility with the Autotools build

ctk_library_version = ''
cail_library_version = ''
if cc.get_id() == 'msvc'
  ctk_library_version = ctk_major_version.to_string()
  cail_library_version = ctk_major_version.to_string()
else
  current = ctk_binary_age - ctk_interface_age
  ctk_library_version = '@0@.@1@.@2@'.format(ctk_soversion, current, ctk_interface_age)
  cail_library_version = '0.0.0'
endif

osx_current = ctk_binary_age - ctk_interface_age + 1
ctk_osxversions = [osx_current, '@0@.@1@.0'.format(osx_current, ctk_interface_age)]

ctk_api_version = '@0@.0'.format(ctk_major_version)

x11_enabled            = get_option('x11_backend')
wayland_enabled        = get_option('wayland_backend')
broadway_enabled       = get_option('broadway_backend')
quartz_enabled         = get_option('quartz_backend')
win32_enabled          = get_option('win32_backend')

os_unix   = false
os_linux  = false
os_win32  = false
os_darwin = false

# Some windowing system backends depend on the platform we're
# building for, so we need to ensure they are disabled; in other
# cases, they are the only windowing system available, so we need
# to ensure they are enabled
if host_machine.system() == 'darwin'
  os_darwin = true
elif host_machine.system() == 'windows'
  os_win32 = true
elif host_machine.system() == 'linux'
  os_linux = true
endif
os_unix = not os_win32

if os_darwin
  wayland_enabled = false
  x11_enabled = false
else
  quartz_enabled = false
endif

if os_win32
  wayland_enabled = false
  x11_enabled = false
else
  win32_enabled = false
endif

ctk_prefix = get_option('prefix')
ctk_includedir = join_paths(ctk_prefix, get_option('includedir'))
ctk_libdir = join_paths(ctk_prefix, get_option('libdir'))
ctk_datadir = join_paths(ctk_prefix, get_option('datadir'))
ctk_localedir = join_paths(ctk_prefix, get_option('localedir'))
ctk_sysconfdir = join_paths(ctk_prefix, get_option('sysconfdir'))
ctk_applicationsdir = join_paths(ctk_datadir, 'applications')
ctk_schemasdir = join_paths(ctk_datadir, 'glib-2.0/schemas')
ctk_appdatadir = join_paths(ctk_datadir, 'metainfo')

# multipress: is mplocaledir defined somewhere?
ctk_mplocaledir = join_paths(ctk_prefix, get_option('localedir'))

cdata = configuration_data()
cdata.set_quoted('PACKAGE_VERSION', meson.project_version())
cdata.set_quoted('CTK_LOCALEDIR', ctk_localedir)
cdata.set_quoted('CTK_DATADIR', ctk_datadir)
cdata.set_quoted('CTK_LIBDIR', ctk_libdir)
cdata.set_quoted('CTK_SYSCONFDIR', ctk_sysconfdir)
cdata.set_quoted('GETTEXT_PACKAGE', 'ctk30')
cdata.set('CTK_MAJOR_VERSION', ctk_major_version)
cdata.set('CTK_MINOR_VERSION', ctk_minor_version)
cdata.set('CTK_MICRO_VERSION', ctk_micro_version)
cdata.set('CTK_BINARY_AGE', ctk_binary_age)
cdata.set('CTK_INTERFACE_AGE', ctk_interface_age)

cdata.set_quoted('PACKAGE_URL', '')
cdata.set_quoted('PACKAGE_NAME', 'ctk+')
cdata.set_quoted('PACKAGE_TARNAME', 'ctk+')
cdata.set_quoted('PACKAGE_STRING', 'ctk+ @0@'.format(meson.project_version()))
cdata.set_quoted('PACKAGE_BUGREPORT', 'http://bugzilla.gnome.org/enter_bug.cgi?product=ctk%2B')

cdata.set('ENABLE_NLS', 1)

check_headers = [
  'crt/externs.h',
  'dlfcn.h',
  'ftw.h',
  'inttypes.h',
  'linux/memfd.h',
  'linux/input.h',
  'dev/evdev/input.h',
  'locale.h',
  'memory.h',
  'stdint.h',
  'stdlib.h',
  'string.h',
  'strings.h',
  'sys/mman.h',
  'sys/param.h',
  'sys/stat.h',
  'sys/sysinfo.h',
  'sys/systeminfo.h',
  'sys/time.h',
  'sys/types.h',
  'unistd.h',
]

foreach h : check_headers
  if cc.has_header(h)
    cdata.set('HAVE_' + h.underscorify().to_upper(), 1)
  endif
endforeach

# Maths functions might be implemented in libm
libm = cc.find_library('m', required: false)

check_functions = [
  'dcgettext',
  'getpagesize',
  'getresuid',
  'lstat',
  'mmap',
  'nearbyint',
  'posix_fallocate',
  '_lock_file',
  'flockfile',
  'mkstemp',
  'mallinfo2',
  'round',
  'rint',
  'log2',
  'exp2',
  'sincos',
  'trunc',
  'localtime_r',
]

foreach func : check_functions
  if cc.has_function(func, dependencies: libm)
    cdata.set('HAVE_' + func.underscorify().to_upper(), 1)
  endif
endforeach

cdata.set('HAVE_DECL_ISINF', cc.has_header_symbol('math.h', 'isinf') ? 1 : false)
cdata.set('HAVE_DECL_ISNAN', cc.has_header_symbol('math.h', 'isnan') ? 1 : false)

# Disable deprecation checks for all libraries we depend on on stable branches.
# This is so newer versions of those libraries don't cause more warnings with
# a stable CTK version.
# We don't ever want to turn off deprecation warnings for master however, because
# that's where we get rid of deprecated API we use.
if ctk_minor_version.is_even()
  cdata.set('GLIB_DISABLE_DEPRECATION_WARNINGS', 1)
endif

# Compiler flags
if cc.get_id() == 'msvc'
  # Compiler options taken from msvc_recommended_pragmas.h
  # in GLib, based on _Win32_Programming_ by Rector and Newcomer
  test_cflags = [
    '-FImsvc_recommended_pragmas.h',
    '-D_USE_MATH_DEFINES',
    '-utf-8',
  ]
  add_project_arguments(cc.get_supported_arguments(test_cflags), language: 'c')
elif cc.get_id() == 'gcc' or cc.get_id() == 'clang'
  test_cflags = [
    '-fno-strict-aliasing',
    '-Wpointer-arith',
    '-Wimplicit-function-declaration',
    '-Wformat=2',
    '-Wformat-security',
    '-Wnested-externs',
    '-Wold-style-definition',
    '-Wundef',
    '-Wunused',
    '-Wcast-align',
    '-Wmissing-noreturn',
    '-Wmissing-format-attribute',
    '-Wmissing-include-dirs',
    '-Wlogical-op',
    '-Wignored-qualifiers',
    '-Wno-discarded-qualifiers',
    '-Werror=implicit',
    '-Werror=nonnull',
    '-Werror=init-self',
    '-Werror=main',
    '-Werror=missing-braces',
    '-Werror=sequence-point',
    '-Werror=return-type',
    '-Werror=trigraphs',
    '-Werror=array-bounds',
    '-Werror=write-strings',
    '-Werror=address',
    '-Werror=int-to-pointer-cast',
    '-Werror=pointer-to-int-cast',
    '-Werror=empty-body',
    '-Werror=write-strings',
  ]
else
  test_cflags = []
endif

common_cflags = cc.get_supported_arguments(test_cflags)

# Symbol visibility
if get_option('default_library') != 'static'
  if os_win32
    cdata.set('DLL_EXPORT', true)
    if cc.get_id() == 'msvc'
      cdata.set('_CDK_EXTERN', '__declspec(dllexport) extern')
    else
      cdata.set('_CDK_EXTERN', '__attribute__((visibility("default"))) __declspec(dllexport) extern')
      common_cflags += ['-fvisibility=hidden']
    endif
  else
    cdata.set('_CDK_EXTERN', '__attribute__((visibility("default"))) extern')
    common_cflags += ['-fvisibility=hidden']
  endif
endif

common_ldflags = []

if os_unix and not os_darwin
  foreach ldflag: [ '-Wl,-Bsymbolic', '-Wl,-z,relro', '-Wl,-z,now', ]
    if cc.links('int main () { return 0; }', name: ldflag, args: ldflag)
      common_ldflags += [ ldflag ]
    endif
  endforeach
endif

confinc = include_directories('.')
cdkinc = include_directories('cdk')
ctkinc = include_directories('ctk')
libcailutilinc = include_directories('libcail-util')
testinc = include_directories('tests')

# Dependencies
glib_dep       = dependency('glib-2.0', version: glib_req,
                            fallback : ['glib', 'libglib_dep'])
gobject_dep    = dependency('gobject-2.0', version: glib_req,
                            fallback : ['glib', 'libgobject_dep'])
if os_win32
  giowin32_dep = dependency('gio-windows-2.0', version: glib_req, required: win32_enabled,
                            fallback : ['glib', 'libgio_dep'])
endif
if os_unix
  giounix_dep  = dependency('gio-unix-2.0', version: glib_req, required: false,
                            fallback : ['glib', 'libgio_dep'])
endif
gmodule_dep    = dependency('gmodule-2.0', version: glib_req,
                            fallback : ['glib', 'libgmodule_dep'])

# Update when we can find depedencies in a declarative manner
cairo_dep      = dependency('cairo', version: cairo_req, required: cc.get_id() != 'msvc')
cairogobj_dep  = dependency('cairo-gobject', version: cairo_req, required: cc.get_id() != 'msvc')

# Remove when we can find depedencies in a declarative manner
if not cairo_dep.found() and cc.get_id() == 'msvc'
  if cc.has_header('cairo.h') and cc.has_header('cairo-win32.h')
    cairo_dep = cc.find_library('cairo', required: false)
  endif
endif

if not cairo_dep.found()
  cairo_dep      = dependency('cairo', version: cairo_req,
                              fallback : ['cairo', 'libcairo_dep'])
endif

# Remove when we can find depedencies in a declarative manner
if not cairogobj_dep.found() and cc.get_id() == 'msvc'
  if cc.has_header('cairo-gobject.h')
    cairogobj_dep = cc.find_library('cairo-gobject', required: false)
  endif
endif

if not cairogobj_dep.found()
  cairogobj_dep  = dependency('cairo-gobject', version: cairo_req,
                              fallback : ['cairo', 'libcairogobject_dep'])
endif

pango_dep      = dependency('pango', version: pango_req,
                            fallback : ['pango', 'libpango_dep'])
fribidi_dep    = dependency('fribidi', version: fribidi_req,
                            fallback : ['fribidi', 'libfribidi_dep'])

# Require PangoFT2 if on X11 or wayland
require_pangoft2 = wayland_enabled or x11_enabled
if require_pangoft2
  pangoft_dep    = dependency('pangoft2', fallback : ['pango', 'libpangoft2_dep'])
else
  pangoft_dep    = dependency('pangoft2', required: false)
endif

if pangoft_dep.found()
  # Need at least 2.7.1 for FT_Get_Var_Design_Coordinates()
  # We get the dependency itself from pango, but pango doesn't care
  # about ft2 version, so an extra check is needed.

  # Update once Meson can have deps declared in a declarative manner
  ft2_dep      = dependency('freetype2', version: '>= 2.7.1', required: false)

  if not ft2_dep.found() and cc.get_id() == 'msvc'
    if cc.has_header('ft2build.h') and cc.has_header('freetype/ftmm.h')
      ft_lib = cc.find_library('freetype', required: false)
      if ft_lib.found()
        if cc.has_function('FT_Get_Var_Design_Coordinates', dependencies: ft_lib,
                           prefix: '''#include <ft2build.h>
                                      #include FT_MULTIPLE_MASTERS_H''')
          ft2_dep = ft_lib
        endif
      endif
    endif
  endif

  if not ft2_dep.found()
    ft2_dep      = dependency('freetype2', version: '>= 2.7.1', fallback : ['freetype2', 'freetype_dep'])
  endif
endif

if win32_enabled
  # for CTK_IM_CONTEXT_IME
  pangowin32_dep  = dependency('pangowin32', fallback : ['pango', 'libpangowin32_dep'])
endif

pangocairo_dep = dependency('pangocairo', version: cairo_req,
                            fallback : ['pango', 'libpangocairo_dep'])
pixbuf_dep     = dependency('gdk-pixbuf-2.0', version: gdk_pixbuf_req,
                            fallback : ['gdk-pixbuf', 'gdkpixbuf_dep'])
epoxy_dep      = dependency('epoxy', version: epoxy_req,
                            fallback: ['libepoxy', 'libepoxy_dep'])
atk_dep        = dependency('atk', version: atk_req,
                            fallback : ['atk', 'libatk_dep'])

# Update once Meson can have deps declared in a declarative manner or can
# find deps properly with CMake again
harfbuzz_dep   = dependency('harfbuzz', version: '>= 0.9', required: false)
xkbdep         = dependency('xkbcommon', version: xkbcommon_req, required: wayland_enabled)
iso_codes_dep  = dependency('iso-codes', required: false)

fontconfig_dep = [] # only used in x11 backend
atkbridge_dep  = [] # only used in x11 backend

if os_win32
  platform_gio_dep = giowin32_dep
endif
if os_unix
  platform_gio_dep = giounix_dep
endif

# Remove once Meson is fixed to find deps properly with CMake or
# gains the ability to declare deps in a declarative manner
# Fallback for HarfBuzz
if cc.get_id() == 'msvc' and not harfbuzz_dep.found()
  if cc.has_header('hb.h')
    harfbuzz_dep = cc.find_library('harfbuzz', required : false)
  endif
endif

require_harfbuzz = wayland_enabled or x11_enabled
if require_harfbuzz and not harfbuzz_dep.found()
  harfbuzz_dep   = dependency('harfbuzz', version: '>= 0.9', required: false,
                              fallback: ['harfbuzz', 'libharfbuzz_dep'])
endif

tracker3_enabled = get_option('tracker3')
if tracker3_enabled
  tracker3_dep = dependency('tracker-sparql-3.0', required: false)
  if tracker3_dep.found()
    cdata.set('HAVE_TRACKER3', tracker3_dep.found())
  else
    error('Tracker3 not found, but was explicitly requested.')
  endif
endif

if iso_codes_dep.found()
  cdata.set_quoted('ISO_CODES_PREFIX', iso_codes_dep.get_pkgconfig_variable('prefix'))
else
  cdata.set_quoted('ISO_CODES_PREFIX', get_option('prefix'))
endif

pc_cdk_extra_libs = []

cairo_found_type = cairo_dep.type_name()
if cairo_found_type == 'internal'
  cairo_features = subproject('cairo').get_variable('built_features')
endif

cairogobj_found_type = cairogobj_dep.type_name()

cairo_backends = []
foreach backend: [ ['cairo-xlib', cairo_req, x11_enabled],
                   ['cairo-win32', cairo_req, win32_enabled],
                   ['cairo-quartz', cairo_req, quartz_enabled],
                   ['cairo', cairo_req, broadway_enabled or wayland_enabled], ]
  backend_enabled = backend.get(2)
  cairo_backend_req = backend.get(1)
  cairo_backend = backend.get(0)
  if backend_enabled
    if cairo_found_type == 'pkgconfig'
      if dependency(cairo_backend, version: cairo_backend_req, required : cc.get_id() != 'msvc').found()
        cairo_backends += cairo_backend
      endif
    elif cairo_found_type == 'internal'
      foreach f: cairo_features
        if f['name'] == cairo_backend
          cairo_backends += cairo_backend
        endif
      endforeach
    endif
  endif
endforeach

cairo_pkg_found = false
cairogobj_pkg_found = false

if cairo_found_type == 'internal' or cairo_found_type == 'pkgconfig'
  cairo_pkg_found = true
endif
if cairogobj_found_type == 'internal' or cairogobj_found_type == 'pkgconfig'
  cairogobj_pkg_found = true
endif

cairo_libs = []

cdata.set('HAVE_HARFBUZZ', harfbuzz_dep.found() ? 1 : false)
cdata.set('HAVE_PANGOFT', pangoft_dep.found() ? 1 : false)

atk_pkgs = ['atk']

wayland_pkgs = []
if wayland_enabled
  wlclientdep    = dependency('wayland-client', version:  wayland_req)
  wlprotocolsdep = dependency('wayland-protocols', version: wayland_proto_req)
  wlcursordep    = dependency('wayland-cursor', version: wayland_req)
  wlegldep       = dependency('wayland-egl')

  wayland_pkgs = [
    'wayland-client', wayland_req,
    'wayland-protocols', wayland_proto_req,
    'xkbcommon', xkbcommon_req,
    'wayland-cursor', wayland_req,
    'wayland-egl',
  ]
endif

x11_pkgs = []
if x11_enabled
  xrandr_dep     = dependency('xrandr', version: '>= 1.2.99')
  xrandr15_dep   = dependency('xrandr', version: '>= 1.5', required: false)
  x11_dep        = dependency('x11')
  xrender_dep    = dependency('xrender')
  xi_dep         = dependency('xi')
  xext_dep       = dependency('xext')
  xcursor_dep    = dependency('xcursor', required: false)
  xdamage_dep    = dependency('xdamage', required: false)
  xfixes_dep     = dependency('xfixes', required: false)
  xcomposite_dep = dependency('xcomposite', required: false)
  fontconfig_dep = dependency('fontconfig', fallback: ['fontconfig', 'fontconfig_dep'])
  atkbridge_dep  = dependency('atk-bridge-2.0', version: at_spi2_atk_req)

  x11_pkgs = ['fontconfig', 'x11', 'xext', 'xi', 'xrandr']

  if xcursor_dep.found()
    x11_pkgs += ['xcursor']
  endif
  if xfixes_dep.found()
    x11_pkgs += ['xfixes']
  endif
  if xcomposite_dep.found()
    x11_pkgs += ['xcomposite']
  endif
  if xdamage_dep.found()
    x11_pkgs += ['xdamage']
  endif

  atk_pkgs += ['atk-bridge-2.0']

  cdata.set('HAVE_XDAMAGE', xdamage_dep.found() ? 1 : false)
  cdata.set('HAVE_XCURSOR', xcursor_dep.found() ? 1 : false)
  cdata.set('HAVE_XCOMPOSITE', xcomposite_dep.found() ? 1 : false)
  cdata.set('HAVE_XFIXES', xfixes_dep.found() ? 1 : false)

  if cc.has_function('XkbQueryExtension', dependencies: x11_dep,
                     prefix : '#include <X11/XKBlib.h>')
    cdata.set('HAVE_XKB', 1)
  endif

  if cc.has_function('XSyncQueryExtension', dependencies: xext_dep,
                     prefix: '''#include <X11/Xlib.h>
                                #include <X11/extensions/sync.h>''')
    cdata.set('HAVE_XSYNC', 1)
  endif

  if cc.has_function('XGetEventData', dependencies: x11_dep)
    cdata.set('HAVE_XGENERICEVENTS', 1)
  endif

  if xi_dep.found() and cc.has_header('X11/extensions/XInput2.h', dependencies: xi_dep)
    cdata.set('XINPUT_2', 1)
    # Note that we also check that the XIScrollClassInfo struct is defined,
    # because at least Ubuntu Oneiric seems to have XIAllowTouchEvents(),
    # but not the XIScrollClassInfo struct
    has_allow_touch_evens = cc.has_function('XIAllowTouchEvents', dependencies: xi_dep)
    has_scroll_class_info = cc.has_member('XIScrollClassInfo', 'number', dependencies: xi_dep,
                                          prefix: '''#include <X11/Xlib.h>
                                                     #include <X11/extensions/XInput2.h>''')
    if has_allow_touch_evens and has_scroll_class_info
      cdata.set('XINPUT_2_2', 1)
    endif
  endif

  enable_xinerama = get_option('xinerama')
  if enable_xinerama != 'no'
    want_xinerama = enable_xinerama == 'yes'
    xinerama_dep = dependency('xinerama', required: want_xinerama)
    if xinerama_dep.found() and cc.has_header_symbol('X11/extensions/Xinerama.h', 'XineramaQueryExtension', dependencies: xinerama_dep)
      cdata.set('HAVE_XFREE_XINERAMA', 1)
      cdata.set('HAVE_XINERAMA', 1)
      x11_pkgs += ['xinerama']
    endif
  else
    xinerama_dep = []
  endif

  cdata.set('HAVE_RANDR', xrandr_dep.found() ? 1 : false)
  cdata.set('HAVE_RANDR15', xrandr15_dep.found() ? 1 : false)
endif

if broadway_enabled
  pc_cdk_extra_libs += ['-lz']
endif

if quartz_enabled
  pc_cdk_extra_libs += ['-Wl,-framework,Cocoa', '-Wl,-framework,Carbon', '-Wl,-framework,CoreGraphics']
endif

extra_demo_ldflags = []
if win32_enabled
  pc_cdk_extra_libs += ['-lgdi32', '-limm32', '-lshell32', '-lole32']
  if cc.get_id() == 'msvc'
    # Since the demo programs are now built as pure GUI programs, we
    # need to pass in /entry:mainCRTStartup so that they will properly
    # link on Visual Studio builds
    extra_demo_ldflags = ['/entry:mainCRTStartup']
  else
    pc_cdk_extra_libs += ['-Wl,-luuid']
  endif
  pc_cdk_extra_libs += ['-lwinmm', '-ldwmapi', '-lsetupapi', '-lcfgmgr32']

  # Check whether libepoxy is built with EGL support on Windows
  win32_has_egl = epoxy_dep.get_variable(
    pkgconfig: 'epoxy_has_egl',
    internal: 'epoxy_has_egl',
    default_value: '0') == '1'
endif

# IMModules stuff-unfortunately we need to put items here
# as they can be built as modules or built directly into CTK
builtin_modules_opt = get_option('builtin_immodules')
if builtin_modules_opt == 'auto'
  if os_win32
    # Current MSVC projects build all immodules directly into CTK by default,
    # as does the mingw autotools build
    message('IMModules are built into CTK for MSVC builds by default')
    builtin_immodules = true
  else
    builtin_immodules = false
  endif
elif builtin_modules_opt == 'yes'
  builtin_immodules = true
else
  builtin_immodules = false
endif

proto_sources = [
  'text-input-unstable-v3',
  'ctk-text-input',
]
proto_sources_outputs = []

foreach proto_name: proto_sources
  if wayland_enabled

    wayland_scanner = find_program('wayland-scanner')
    input_path = join_paths('modules', 'input', proto_name + '.xml')
    sources = []
    sources += custom_target('@0@ client header'.format(proto_name),
                             input: input_path,
                             output: '@0@-client-protocol.h'.format(proto_name),
                             command: [
                               wayland_scanner,
                               'client-header',
                               '@INPUT@', '@OUTPUT@',
                            ])

    sources += custom_target('@0@ source'.format(proto_name),
                             input: input_path,
                             output: '@0@-protocol.c'.format(proto_name),
                             command: [
                               wayland_scanner,
                               'private-code',
                               '@INPUT@', '@OUTPUT@',
                             ])
    proto_sources_outputs += [sources]
  else
    proto_sources_outputs += [[]]
  endif
endforeach

# Unfortunately it seems that we need to include the input module sources here, and build
# them in $(buildrootdir) if we are to build them included in CTK...
# Make sure the source listings are the same as in modules/input/Makefile.am
immodule_srcdir = 'modules/input/'
immodules = [
  [ 'am-et', files([ immodule_srcdir + 'imam-et.c' ]) ],
  [ 'cedilla', files([ immodule_srcdir + 'imcedilla.c' ]) ],
  [ 'cyrillic-translit', files([ immodule_srcdir + 'imcyrillic-translit.c' ]) ],
  [ 'inuktitut', files([ immodule_srcdir + 'iminuktitut.c' ]) ],
  [ 'ipa', files([ immodule_srcdir + 'imipa.c' ]) ],
  [ 'multipress', files( [ immodule_srcdir + 'ctkimcontextmultipress.c', immodule_srcdir + 'immultipress.c' ]), true,
                    ['-DMULTIPRESS_LOCALEDIR="@0@"'.format(ctk_mplocaledir),
                     '-DMULTIPRESS_CONFDIR="@0@/ctk-3.0"'.format(ctk_sysconfdir)] ],
  [ 'thai', files([ immodule_srcdir + 'thai-charprop.c', immodule_srcdir + 'ctkimcontextthai.c', immodule_srcdir + 'imthai.c' ]) ],
  [ 'ti-er', files([ immodule_srcdir + 'imti-er.c' ]) ],
  [ 'ti-et', files([ immodule_srcdir + 'imti-et.c' ]) ],
  [ 'viqr', files([ immodule_srcdir + 'imviqr.c' ]) ],
  [ 'broadway', files([ immodule_srcdir + 'imbroadway.c' ]), broadway_enabled ],
  [ 'ime', files([ immodule_srcdir + 'ctkimcontextime.c', immodule_srcdir + 'imime.c' ]), win32_enabled ],
  [ 'quartz', files([ immodule_srcdir + 'imquartz.c' ]), quartz_enabled, [ '-xobjective-c' ] ],
  [ 'wayland', files([ immodule_srcdir + 'imwayland.c' ]) + proto_sources_outputs[0], wayland_enabled ],
  [ 'waylandctk', files([ immodule_srcdir + 'imwaylandctk.c'])  + proto_sources_outputs[1], wayland_enabled ],
  [ 'xim', files([ immodule_srcdir + 'ctkimcontextxim.c', immodule_srcdir + 'imxim.c' ]), x11_enabled ],
]

install_data(join_paths(immodule_srcdir, 'im-multipress.conf'), install_dir: join_paths(ctk_sysconfdir, 'ctk-3.0'))

# Check for bind_textdomain_codeset, including -lintl if GLib brings it in by
# doing the same check as glib. We can't check that by linking to glib because
# it might be a subproject and hence not built yet.
if cc.has_function('ngettext')
  libintl_dep = []
  cdata.set('HAVE_BIND_TEXTDOMAIN_CODESET', 1)
else
  libintl_dep = cc.find_library('intl', required : false)
  if cc.has_function('bind_textdomain_codeset', dependencies: libintl_dep)
    cdata.set('HAVE_BIND_TEXTDOMAIN_CODESET', 1)
  else
    # Don't use subproject('proxy-libintl').get_variable('intl_dep') because that
    # makes the dependency unconditional. This way, people have the option of
    # either not providing the subproject or disabling it entirely with
    # --wrap-mode=nodownload or nofallback.
    libintl_dep = dependency('', required : false,
                             fallback: ['proxy-libintl', 'intl_dep'])
    if libintl_dep.found()
      cdata.set('HAVE_BIND_TEXTDOMAIN_CODESET', 1)
    endif
  endif
endif

if os_win32
  if cc.has_header_symbol('windows.h', 'IPrintDialogCallback')
    cdata.set('HAVE_IPRINTDIALOGCALLBACK', 1)
  endif

  dvot_size = cc.sizeof('DISPLAYCONFIG_VIDEO_OUTPUT_TECHNOLOGY', prefix : '''
    #define _WIN32_WINNT 0x601
    #include <windows.h>
  ''')
  cdata.set('SIZEOF_DISPLAYCONFIG_VIDEO_OUTPUT_TECHNOLOGY', dvot_size)

  getdevprop_code = '''
    #define _WIN32_WINNT 0x0600
    #include <windows.h>
    #include <devpropdef.h>
    #include <setupapi.h>

    int main(int argc, char *argv[]) {
      return SetupDiGetDevicePropertyW(NULL, NULL, NULL, NULL, NULL, 0, NULL, 0);
    }
  '''
  result = cc.links(getdevprop_code, args: ['-lsetupapi'], name: 'has SetupDiGetDevicePropertyW')
  cdata.set('HAVE_SETUP_DI_GET_DEVICE_PROPERTY_W', result ? 1 : false)
endif

have_gio_unix = false
if os_unix
  have_gio_unix = giounix_dep.found()
  cdata.set('HAVE_GIO_UNIX', have_gio_unix ? 1 : false)
endif

cloudproviders_enabled = get_option('cloudproviders')
cloudproviders_packages = []
if cloudproviders_enabled
  cloudproviders_dep = dependency('cloudproviders', version: cloudproviders_req, required: true)
  if cloudproviders_dep.found()
    cloudproviders_packages += ['cloudproviders', cloudproviders_req]
    cdata.set('HAVE_CLOUDPROVIDERS', cloudproviders_dep.found() ? 1 : false)
  else
    error('Cloudproviders support not found, but was explicitly requested.')
  endif
endif

profiler_enabled = get_option('profiler')
if profiler_enabled
  profiler_dep = dependency('sysprof-capture-3', static: true, required: true)
  if profiler_dep.found()
    cdata.set('HAVE_SYSPROF_CAPTURE', profiler_dep.found())
  else
    error('Profiler support not found, but was explicitly requested.')
  endif
endif

module_suffix = []
if os_darwin
  module_suffix = 'so'
endif

build_gir = get_option('introspection')
subdir('cdk')
subdir('ctk')
subdir('modules')
subdir('libcail-util')
if get_option('demos')
  subdir('demos')
endif
if get_option('tests')
  subdir('tests')
  subdir('testsuite')
endif
if get_option('examples')
  subdir('examples')
endif

# config.h
configure_file(input: 'config.h.meson',
               output: 'config.h',
               configuration: cdata)

# pkg-config files - bit of a mess all of this
pkgconf = configuration_data()

pkgconf.set('prefix', get_option('prefix'))
pkgconf.set('exec_prefix', '${prefix}')
pkgconf.set('libdir', '${prefix}' / get_option('libdir'))
pkgconf.set('includedir', '${prefix}' / get_option('includedir'))
pkgconf.set('CTK_API_VERSION', ctk_api_version)
pkgconf.set('VERSION', meson.project_version())
pkgconf.set('CTK_BINARY_VERSION', ctk_binary_version)
pkgconf.set('host', '@0@-@1@'.format(host_machine.cpu_family(), host_machine.system())) # FIXME

# Requires
pango_pkgname = win32_enabled ? 'pangowin32' : 'pango'
cdk_packages = ' '.join([ pango_pkgname, pango_req,
                       'pangocairo', pango_req,
                       'gdk-pixbuf-2.0', gdk_pixbuf_req ])

cairo_packages = ''

if cairo_pkg_found
  cairo_packages += ' '.join([ ' cairo', cairo_req ])
elif cc.get_id() == 'msvc' and cairo_dep.found()
  cairo_libs += '-lcairo'
endif
if cairogobj_pkg_found
  cairo_packages += ' '.join([ ' cairo-gobject', cairo_req ])
elif cc.get_id() == 'msvc' and cairogobj_dep.found()
  cairo_libs += '-lcairo-gobject'
endif

cdk_packages += cairo_packages
pkgconf.set('CDK_PACKAGES', cdk_packages)

if have_gio_unix
  gio_packages = ['gio-unix-2.0', glib_req]
else
  gio_packages = ['gio-2.0', glib_req]
endif

pkgconf.set('CDK_PRIVATE_PACKAGES',
            ' '.join(gio_packages + x11_pkgs + wayland_pkgs + cairo_backends +
                     ['epoxy', epoxy_req] + cloudproviders_packages +
                     ['fribidi', fribidi_req]))

ctk_packages = ' '.join([
    atk_dep.name(), atk_req,
    cairo_packages,
    pixbuf_dep.name(), gdk_pixbuf_req,
    'gio-2.0', glib_req,
])
pkgconf.set('CTK_PACKAGES', ctk_packages)

# Requires.private
pc_cdk_extra_libs += cairo_libs

ctk_private_packages = atk_pkgs + wayland_pkgs + ['epoxy', epoxy_req, 'fribidi', fribidi_req]
if wayland_enabled or x11_enabled
  ctk_private_packages += ['pangoft2']
endif
ctk_private_packages += gio_packages

pkgconf.set('CTK_PRIVATE_PACKAGES', ' '.join(ctk_private_packages))

pkgconf.set('CDK_EXTRA_LIBS', ' '.join(pc_cdk_extra_libs))
pkgconf.set('GSK_EXTRA_LIBS', '')
pkgconf.set('CTK_EXTRA_LIBS', '')

pkgconf.set('CDK_EXTRA_CFLAGS', '')
pkgconf.set('GSK_EXTRA_CFLAGS', '')
pkgconf.set('CTK_EXTRA_CFLAGS', '')

pkg_install_dir = join_paths(get_option('libdir'), 'pkgconfig')

ctk_pcs = ['ctk+-3.0.pc']
cdk_pcs = ['cdk-3.0.pc']

pkg_targets = ''
disabled_backends = []
foreach backend: [ 'broadway', 'quartz', 'wayland', 'win32', 'x11']
  if get_variable('@0@_enabled'.format(backend))
    ctk_pcs += ['ctk+-@0@-3.0.pc'.format(backend)]
    cdk_pcs += ['cdk-@0@-3.0.pc'.format(backend)]
    pkg_targets += ' ' + backend
  else
    disabled_backends += [backend]
  endif
endforeach
pkgconf.set('CDK_BACKENDS', pkg_targets.strip())

configure_file(input: 'cail-3.0.pc.in',
               output: 'cail-3.0.pc',
               configuration: pkgconf,
               install_dir: pkg_install_dir)

foreach pkg: cdk_pcs
  configure_file(input: 'cdk-3.0.pc.in',
                 output: pkg,
                 configuration: pkgconf,
                 install_dir: pkg_install_dir)
endforeach

foreach pkg: ctk_pcs
  configure_file(input: 'ctk+-3.0.pc.in',
                 output: pkg,
                 configuration: pkgconf,
                 install_dir: pkg_install_dir)
endforeach

if os_unix
  configure_file(input: 'ctk+-unix-print-3.0.pc.in',
                 output: 'ctk+-unix-print-3.0.pc',
                 configuration: pkgconf,
                 install_dir: pkg_install_dir)
endif

subdir('po')
subdir('po-properties')
subdir('docs/tools')
subdir('docs/reference')

install_data('m4macros/ctk-3.0.m4', install_dir: join_paths(get_option('datadir'), 'aclocal'))

# Keep this in sync with post-install.py expected arguments
meson.add_install_script('build-aux/meson/post-install.py',
                         ctk_api_version,
                         ctk_binary_version,
                         join_paths(ctk_prefix, get_option('bindir')),
                         ctk_libdir,
                         ctk_datadir)

if host_machine.system() != 'windows'
  # Install Valgrind suppression files (except on Windows,
  # as Valgrind is currently not supported on Windows)
  install_data('ctk.supp',
    install_dir : join_paths(ctk_datadir, 'ctk-3.0', 'valgrind'))
endif

test(
  'version-check',
  find_program('check-version.py'),
  args: [
    join_paths(meson.current_source_dir(), 'configure.ac'),
    join_paths(meson.current_source_dir(), 'meson.build'),
  ],
  suite: 'ctk',
)

summary = [
  '',
  '------',
  'CTK+ @0@ (@1@)'.format(ctk_version, ctk_api_version),
  '',
  '  Display backends: @0@ [disabled: @1@]'.format(pkg_targets.strip(), ' '.join(disabled_backends)),
  '    Print backends: @0@ [disabled: @1@]'.format(' '.join(print_backends), ' '.join(disabled_print_backends)),
  '     Cloud support: @0@'.format(get_option('cloudproviders')),
  '    Colord support: @0@'.format(get_option('colord')),
  '          Profiler: @0@'.format(get_option('profiler')),
  '     Introspection: @0@'.format(get_option('introspection')),
  '     Documentation: @0@'.format(get_option('gtk_doc')),
  '         Man pages: @0@'.format(get_option('man')),
  '       Build tests: @0@'.format(get_option('tests')),
  '   Installed tests: @0@'.format(get_option('installed_tests')),
  '             Demos: @0@'.format(get_option('demos')),
  '          Examples: @0@'.format(get_option('examples')),
  'Directories:',
  '            prefix: @0@'.format(ctk_prefix),
  '        includedir: @0@'.format(ctk_includedir),
  '            libdir: @0@'.format(ctk_libdir),
  '           datadir: @0@'.format(ctk_datadir),
  '------',
  ''
]

message('\n'.join(summary))
