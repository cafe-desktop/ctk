#!/usr/bin/env python3

import os
import shutil
import sys
import subprocess

if 'DESTDIR' not in os.environ:
    ctk_api_version = sys.argv[1]
    ctk_abi_version = sys.argv[2]
    ctk_bindir = sys.argv[3]
    ctk_libdir = sys.argv[4]
    ctk_datadir = sys.argv[5]
    ctk_query_immodules = os.path.join(ctk_bindir, 'ctk-query-immodules-' + ctk_api_version)
    ctk_update_icon_cache = os.path.join(ctk_bindir, 'ctk-update-icon-cache')

    ctk_moduledir = os.path.join(ctk_libdir, 'ctk-' + ctk_api_version, ctk_abi_version)
    ctk_immodule_dir = os.path.join(ctk_moduledir, 'immodules')
    ctk_printmodule_dir = os.path.join(ctk_moduledir, 'printbackends')

    if os.name == 'nt':
        for lib in ['gdk', 'ctk', 'gailutil']:
            # Make copy for MSVC-built .lib files, e.g. xxx-3.lib->xxx-3.0.lib
            installed_lib = os.path.join(ctk_libdir, lib + '-' + ctk_api_version.split('.')[0] + '.lib')
            installed_lib_dst = os.path.join(ctk_libdir, lib + '-' + ctk_api_version + '.lib')
            if os.path.isfile(installed_lib):
                shutil.copyfile(installed_lib, installed_lib_dst)

    print('Compiling GSettings schemas...')
    glib_compile_schemas = subprocess.check_output(['pkg-config',
                                                   '--variable=glib_compile_schemas',
                                                   'gio-2.0']).strip()
    if not os.path.exists(glib_compile_schemas):
        # pkg-config variables only available since GLib 2.62.0.
        glib_compile_schemas = 'glib-compile-schemas'
    subprocess.call([glib_compile_schemas,
                    os.path.join(ctk_datadir, 'glib-2.0', 'schemas')])

    print('Updating icon cache...')
    subprocess.call([ctk_update_icon_cache, '-q', '-t' ,'-f',
                    os.path.join(ctk_datadir, 'icons', 'hicolor')])

    print('Updating module cache for input methods...')
    os.makedirs(ctk_immodule_dir, exist_ok=True)
    immodule_cache_file = open(os.path.join(ctk_moduledir, 'immodules.cache'), 'w')
    subprocess.call([ctk_query_immodules], stdout=immodule_cache_file)
    immodule_cache_file.close()

    # Untested!
    print('Updating module cache for print backends...')
    os.makedirs(ctk_printmodule_dir, exist_ok=True)
    gio_querymodules = subprocess.check_output(['pkg-config',
                                                '--variable=gio_querymodules',
                                                'gio-2.0']).strip()
    if not os.path.exists(gio_querymodules):
        # pkg-config variables only available since GLib 2.62.0.
        gio_querymodules = 'gio-querymodules'
    subprocess.call([gio_querymodules, ctk_printmodule_dir])
