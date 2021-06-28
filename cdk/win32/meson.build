cdk_win32_sources = files(
  'cdkcursor-win32.c',
  'cdkdevicemanager-win32.c',
  'cdkdevice-virtual.c',
  'cdkdevice-win32.c',
  'cdkdevice-wintab.c',
  'cdkdisplay-win32.c',
  'cdkdisplaymanager-win32.c',
  'cdkdnd-win32.c',
  'cdkevents-win32.c',
  'cdkgeometry-win32.c',
  'cdkglcontext-win32.c',
  'cdkglobals-win32.c',
  'cdkkeys-win32.c',
  'cdkmain-win32.c',
  'cdkmonitor-win32.c',
  'cdkproperty-win32.c',
  'cdkscreen-win32.c',
  'cdkselection-win32.c',
  'cdktestutils-win32.c',
  'cdkwin32id.c',
  'cdkwin32langnotification.c',
  'cdkwindow-win32.c',
)

cdk_win32_public_headers = files(
  'cdkwin32cursor.h',
  'cdkwin32display.h',
  'cdkwin32displaymanager.h',
  'cdkwin32dnd.h',
  'cdkwin32glcontext.h',
  'cdkwin32keys.h',
  'cdkwin32misc.h',
  'cdkwin32monitor.h',
  'cdkwin32screen.h',
  'cdkwin32window.h',
)

CDK_WIN32_CFLAGS = ['-DCDK_COMPILATION', '-DG_LOG_DOMAIN="Cdk"', '-DINSIDE_CDK_WIN32']

if win32_has_egl
  CDK_WIN32_CFLAGS += '-DCDK_WIN32_ENABLE_EGL'
endif


install_headers(cdk_win32_public_headers, subdir: 'ctk-3.0/cdk/win32')
install_headers('cdkwin32.h', subdir: 'ctk-3.0/cdk')

cdk_win32_deps = [ # FIXME
    pangowin32_dep
]

libcdk_win32 = static_library('cdk-win32',
  cdk_win32_sources, cdkconfig, cdkenum_h,
  include_directories: [confinc, cdkinc],
  c_args: CDK_WIN32_CFLAGS,
  dependencies: [cdk_deps, cdk_win32_deps])
