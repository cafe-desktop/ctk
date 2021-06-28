cdk_broadway_sources = files(
  'cdkcursor-broadway.c',
  'cdkdevice-broadway.c',
  'cdkdevicemanager-broadway.c',
  'cdkdisplay-broadway.c',
  'cdkdnd-broadway.c',
  'cdkeventsource.c',
  'cdkglobals-broadway.c',
  'cdkkeys-broadway.c',
  'cdkmonitor-broadway.c',
  'cdkproperty-broadway.c',
  'cdkscreen-broadway.c',
  'cdkselection-broadway.c',
  'cdktestutils-broadway.c',
  'cdkvisual-broadway.c',
  'cdkwindow-broadway.c',
  'cdkbroadway-server.c',
)

cdk_broadway_public_headers = files(
  'cdkbroadwaydisplay.h',
  'cdkbroadwaywindow.h',
  'cdkbroadwaycursor.h',
  'cdkbroadwaymonitor.h',
  'cdkbroadwayvisual.h',
)

install_headers(cdk_broadway_public_headers, subdir: 'ctk-3.0/cdk/broadway/')
install_headers('cdkbroadway.h', subdir: 'ctk-3.0/cdk/')

cdk_broadway_deps = [shmlib]

gen_c_array = find_program('gen-c-array.py')

clienthtml_h = custom_target('clienthtml.h',
  input : 'client.html',
  output : 'clienthtml.h',
  command : [
    gen_c_array,
    '--array-name=client_html',
    '--output=@OUTPUT@',
    '@INPUT@',
  ],
)

broadwayjs_h = custom_target('broadwayjs.h',
  input : ['broadway.js', 'rawinflate.min.js'],
  output : 'broadwayjs.h',
  command : [
    gen_c_array,
    '--array-name=broadway_js',
    '--output=@OUTPUT@',
    '@INPUT0@','@INPUT1@',
  ],
)

libcdk_broadway = static_library('cdk-broadway',
  clienthtml_h, broadwayjs_h,
  cdk_broadway_sources, cdkconfig, cdkenum_h,
  include_directories: [confinc, cdkinc],
  c_args: [
    '-DGDK_COMPILATION',
    '-DG_LOG_DOMAIN="Gdk"',
  ] + common_cflags,
  link_args: common_ldflags,
  dependencies: [cdk_deps, cdk_broadway_deps])

# ctk4-broadwayd

broadwayd_syslib = os_win32 ? cc.find_library('ws2_32') : shmlib

executable('broadwayd',
  clienthtml_h, broadwayjs_h,
  'broadwayd.c', 'broadway-server.c', 'broadway-buffer.c', 'broadway-output.c',
  include_directories: [confinc, cdkinc, include_directories('.')],
  c_args: ['-DGDK_COMPILATION', '-DG_LOG_DOMAIN="Gdk"', ],
  dependencies : [broadwayd_syslib, cdk_deps],
  install : true)
