
# Note: the list of immodules is in $(srcroot)/meson.build!

foreach l: immodules
  name = l[0]
  immod_sources = l[1]
  cond = l.get(2, true)
  cflags = l.get(3, [])

  if cond and not builtin_immodules
    shared_module('im-@0@'.format(name),
                  immod_sources,
                  c_args: common_cflags + cflags,
                  dependencies: [ libctk_dep ],
                  install_dir: '@0@/ctk-@1@/@2@/immodules/'.format(ctk_libdir, ctk_api_version, ctk_binary_version),
                  include_directories: [confinc, ctkinc],
                  install : true,
                  name_suffix: module_suffix,
                  name_prefix: '')
  endif
endforeach
