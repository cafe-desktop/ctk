ctk_tests_export_dynamic_ldflag = []

if cc.get_id() != 'msvc' and cc.get_id() != 'clang'
  ctk_tests_export_dynamic_ldflag = ['-Wl,--export-dynamic']
endif

tests = [
  ['accel'],
  ['accessible'],
  ['action'],
  ['adjustment'],
  ['bitmask', ['../../ctk/ctkallocatedbitmask.c'], ['-DCTK_COMPILATION', '-UG_ENABLE_DEBUG']],
  ['builder', [], [], ctk_tests_export_dynamic_ldflag],
  ['builderparser'],
  ['cellarea'],
  ['check-icon-names'],
  ['check-cursor-names'],
  ['cssprovider'],
  ['defaultvalue'],
  ['entry'],
  ['firefox-stylecontext'],
  ['floating'],
  ['focus'],
  ['gestures'],
  ['grid'],
  ['ctkmenu'],
  ['icontheme'],
  ['keyhash', ['../../ctk/ctkkeyhash.c', ctkresources, '../../ctk/ctkprivate.c'], ctk_cargs],
  ['listbox'],
  ['notify'],
  ['no-ctk-init'],
  ['object'],
  ['papersize'],
  ['rbtree', ['../../ctk/ctkrbtree.c'], ['-DCTK_COMPILATION', '-UG_ENABLE_DEBUG']],
  ['recentmanager'],
  ['regression-tests'],
  ['scrolledwindow'],
  ['spinbutton'],
  ['stylecontext'],
  ['templates'],
  ['textbuffer'],
  ['textiter'],
  ['treemodel', ['treemodel.c', 'liststore.c', 'treestore.c', 'filtermodel.c',
                 'modelrefcount.c', 'sortmodel.c', 'ctktreemodelrefcount.c']],
  ['treepath'],
  ['treeview'],
  ['typename'],
  ['window'],
  ['displayclose'],
  ['revealer-size'],
]

# Tests that are expected to fail
xfail = [
]

is_debug = get_option('buildtype').startswith('debug')

test_cargs = []

if os_unix
  # tests += [['defaultvalue']]  # disabled in Makefile.am as well
  test_cargs += ['-DHAVE_UNIX_PRINT_WIDGETS']
endif

if x11_enabled
  tests += [
    ['clipboard'],
    ['objects-finalize', ['../../cdk/x11/cdkdisplaymanager-x11.c'], ['-DCDK_COMPILATION', '-UG_ENABLE_DEBUG']],
  ]
endif

foreach t : tests
  test_name = t.get(0)
  test_srcs = ['@0@.c'.format(test_name)] + t.get(1, [])
  test_extra_cargs = t.get(2, [])
  test_extra_ldflags = t.get(3, [])

  test_exe = executable(test_name, test_srcs,
    c_args : test_cargs + test_extra_cargs,
    link_args : test_extra_ldflags,
    dependencies : libctk_dep,
    install: get_option('installed_tests'),
    install_dir: installed_test_bindir)

  expect_fail = xfail.contains(test_name)

  test(test_name, test_exe,
       args: [ '--tap', '-k' ],
       env: [ 'GIO_USE_VOLUME_MONITOR=unix',
              'GSETTINGS_BACKEND=memory',
              'CTK_CSD=1',
              'G_ENABLE_DIAGNOSTIC=0',
              'GSK_RENDERER=cairo',
              'G_TEST_SRCDIR=@0@'.format(meson.current_source_dir()),
              'G_TEST_BUILDDIR=@0@'.format(meson.current_build_dir()),
              'GSETTINGS_SCHEMA_DIR=@0@'.format(ctk_schema_build_dir),
              'CTK_TEST_MESON=1',
            ],
       suite: 'ctk',
       should_fail: expect_fail,
  )
endforeach

# FIXME: if objc autotestkeywords_CPPFLAGS += -DHAVE_OBJC=1 -x objective-c++
if add_languages('cpp', required: false)
  test_exe = executable('autotestkeywords',
                        'autotestkeywords.cc',
                        c_args : test_cargs + ['-Idummy-headers'],
                        dependencies : libctk_dep,
                        install: get_option('installed_tests'),
                        install_dir: installed_test_bindir)
  test('c++ keywords', test_exe,
       args: [ '--tap', '-k' ],
       env: [ 'GIO_USE_VOLUME_MONITOR=unix',
              'GSETTINGS_BACKEND=memory',
              'CTK_CSD=1',
              'G_ENABLE_DIAGNOSTIC=0',
              'G_TEST_SRCDIR=@0@'.format(meson.current_source_dir()),
              'G_TEST_BUILDDIR=@0@'.format(meson.current_build_dir()),
              'GSETTINGS_SCHEMA_DIR=@0@'.format(ctk_schema_build_dir),
            ],
       suite: 'ctk')

  if get_option('installed_tests')
    conf = configuration_data()
    conf.set('testexecdir', installed_test_bindir)
    conf.set('test', 'autotestkeywords')
    configure_file(input: 'ctk.test.in',
                   output: 'autotestkeywords.test',
                   configuration: conf,
                   install_dir: installed_test_datadir)
  endif
endif


if get_option('installed_tests')
  foreach t : tests
    test_name = t.get(0)
    conf = configuration_data()
    conf.set('testexecdir', installed_test_bindir)
    conf.set('test', test_name)
    configure_file(input: 'ctk.test.in',
                   output: '@0@.test'.format(test_name),
                   configuration: conf,
                   install_dir: installed_test_datadir)
  endforeach

  install_subdir('icons', install_dir: installed_test_bindir)
  install_subdir('icons2', install_dir: installed_test_bindir)
  install_subdir('ui', install_dir: installed_test_bindir)
endif