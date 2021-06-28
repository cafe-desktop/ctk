if get_option('gtk_doc')
    glib_prefix = dependency('glib-2.0').get_pkgconfig_variable('prefix')
    glib_docpath = join_paths(glib_prefix, 'share', 'gtk-doc', 'html')

    cairo_prefix = dependency('cairo-gobject').get_pkgconfig_variable('prefix')
    cairo_docpath = join_paths(cairo_prefix, 'share', 'gtk-doc', 'html', 'cairo')

    cdkpixbuf_prefix = dependency('cdk-pixbuf-2.0').get_pkgconfig_variable('prefix')
    cdkpixbuf_docpath = join_paths(cdkpixbuf_prefix, 'share', 'gtk-doc', 'html', 'cdk-pixbuf')

    docpath = join_paths(ctk_datadir, 'gtk-doc', 'html')

    version_conf = configuration_data()
    version_conf.set('CTK_VERSION', meson.project_version())

    src_dir_conf = configuration_data()
    src_dir_conf.set('abs_top_srcdir', meson.source_root())
endif

subdir('cdk')
subdir('ctk')
subdir('libgail-util')
