cailutil_sources = files(
  'cailmisc.c',
  'cailtextutil.c',
)

cailutil_public_headers = files(
  'cailmisc.h',
  'cailtextutil.h',
  'cail-util.h',
  )

install_headers(cailutil_public_headers, subdir : 'cail-3.0/libcail-util')

cailutil_link_args = common_ldflags

if cc.get_id() == 'msvc'
  cailutil_link_args += '-def:@0@/cailutil.def'.format(meson.current_source_dir())
endif

libcailutil = library('cailutil-3',
                      soversion: ctk_soversion,
                      version: cail_library_version,
                      darwin_versions: ['1', '1.0'],
                      sources: [cailutil_sources],
                      dependencies: libctk_dep,
                      include_directories: [confinc, ctkinc],
                      c_args: [
                        '-DCDK_DISABLE_DEPRECATED',
                        '-DCTK_DISABLE_DEPRECATED',
                      ] + common_cflags,
                      link_args: cailutil_link_args,
                      install: true)
