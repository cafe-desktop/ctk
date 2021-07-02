if get_option('gtk_doc')
    configure_file(input: 'version.xml.in', output: 'version.xml', configuration: version_conf)

    gnome.gtkdoc('cail-libcail-util3',
                 mode: 'none',
                 main_xml: 'cail-libcail-util-docs.sgml',
                 src_dir: libcailutilinc,
                 dependencies: [libctk_dep, libcailutil],
                 gobject_typesfile: join_paths(meson.current_source_dir(), 'cail-libcail-util3.types'),
                 scan_args: [
                   '--ignore-decorators=_CDK_EXTERN|G_GNUC_WARN_UNUSED_RESULT',
                 ],
                 fixxref_args: [
                   '--html-dir=@0@'.format(docpath),
                   '--extra-dir=@0@'.format(join_paths(glib_docpath, 'glib')),
                   '--extra-dir=@0@'.format(join_paths(glib_docpath, 'gobject')),
                   '--extra-dir=@0@'.format(join_paths(glib_docpath, 'gio')),
                   '--extra-dir=../ctk',
                 ],
                 install: true)
endif
