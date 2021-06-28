
add_languages(['objc'])

cdk_quartz_sources = files(
  'CdkQuartzView.c',
  'CdkQuartzNSWindow.c',
  'cdkcursor-quartz.c',
  'cdkdevice-core-quartz.c',
  'cdkdevicemanager-core-quartz.c',
  'cdkdisplay-quartz.c',
  'cdkdisplaymanager-quartz.c',
  'cdkdnd-quartz.c',
  'cdkevents-quartz.c',
  'cdkeventloop-quartz.c',
  'cdkglcontext-quartz.c',
  'cdkglobals-quartz.c',
  'cdkkeys-quartz.c',
  'cdkmonitor-quartz.c',
  'cdkproperty-quartz.c',
  'cdkscreen-quartz.c',
  'cdkselection-quartz.c',
  'cdktestutils-quartz.c',
  'cdkutils-quartz.c',
  'cdkvisual-quartz.c',
  'cdkwindow-quartz.c',
)

cdk_quartz_public_headers = files(
  'cdkquartzcursor.h',
  'cdkquartzdevice-core.h',
  'cdkquartzdevicemanager-core.h',
  'cdkquartzdisplay.h',
  'cdkquartzdisplaymanager.h',
  'cdkquartzdnd.h',
  'cdkquartzkeys.h',
  'cdkquartzmonitor.h',
  'cdkquartzscreen.h',
  'cdkquartzutils.h',
  'cdkquartzvisual.h',
  'cdkquartzwindow.h',
)

install_headers(cdk_quartz_public_headers, subdir: 'ctk-3.0/cdk/quartz')
install_headers('cdkquartz.h', subdir: 'ctk-3.0/cdk')

core_graphics_dep = dependency('appleframeworks', modules : 'CoreGraphics', required : true)
appkit_dep = dependency('appleframeworks', modules : 'AppKit', required : true)
cocoa_dep = dependency('appleframeworks', modules : 'Cocoa', required : true)
carbon_dep = dependency('appleframeworks', modules : 'Carbon', required : true)

cdk_quartz_deps = [ core_graphics_dep, appkit_dep, cocoa_dep, carbon_dep ]

libcdk_quartz = static_library('cdk-quartz',
  cdk_quartz_sources, cdkconfig, cdkenum_h,
  include_directories: [confinc, cdkinc],
  c_args: ['-DGDK_COMPILATION', '-DG_LOG_DOMAIN="Cdk"', '-xobjective-c'],
  dependencies: [cdk_deps, cdk_quartz_deps])
