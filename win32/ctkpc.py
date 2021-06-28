#!/usr/bin/python
#
# Utility script to generate .pc files for CTK+
# for Visual Studio builds, to be used for
# building introspection files

# Author: Fan, Chun-wei
# Date: April 26, 2016

import os
import sys
import argparse

from replace import replace_multi, replace
from pc_base import BasePCItems

def main(argv):
    base_pc = BasePCItems()

    cdk_parser = argparse.ArgumentParser(description='Setup basic .pc file info')
    cdk_parser.add_argument('--broadway',
                              action='store_const',
                              const=1,
                              help='GDK with Broadway backend')
    cdk_parser.add_argument('--host',
                            required=True,
                            help='Build type')
    base_pc.setup(argv, cdk_parser)

    atk_min_ver = '2.15.1'
    cairo_min_ver = '1.14.0'
    cdk_pixbuf_min_ver = '2.30.0'
    cdk_win32_sys_libs = '-lgdi32 -limm32 -lshell32 -lole32 -lwinmm -ldwmapi'
    cairo_libs = '-lcairo-gobject -lcairo '
    glib_min_ver = '2.45.8'

    cdk_backends = 'win32'
    gio_package = 'gio-2.0 >= ' + glib_min_ver
    broadway_extra_libs = ''

    cdk_args = cdk_parser.parse_args()
    if getattr(cdk_args, 'broadway', None) is 1:
        # On Visual Studio, we link to zlib1.lib
        broadway_extra_libs = ' -lzlib1'
        cdk_backends += ' broadway'

    pkg_replace_items = {'@CTK_API_VERSION@': '3.0',
                         '@GDK_BACKENDS@': cdk_backends}

    pkg_required_packages = 'cdk-pixbuf-2.0 >= ' + cdk_pixbuf_min_ver

    cdk_pc_replace_items = {'@GDK_PACKAGES@': gio_package + ' ' + \
                                              'pangowin32 pangocairo' + ' ' + \
                                              pkg_required_packages,
                            '@GDK_PRIVATE_PACKAGES@': gio_package,
                            '@GDK_EXTRA_LIBS@': cairo_libs + cdk_win32_sys_libs + broadway_extra_libs,
                            '@GDK_EXTRA_CFLAGS@': '',
                            'cdk-3': 'cdk-3.0'}

    ctk_pc_replace_items = {'@host@': cdk_args.host,
                            '@CTK_BINARY_VERSION@': '3.0.0',
                            '@CTK_PACKAGES@': 'atk >= ' + atk_min_ver + ' ' + \
                                              pkg_required_packages + ' ' + \
                                              gio_package,
                            '@CTK_PRIVATE_PACKAGES@': 'atk',
                            '@CTK_EXTRA_CFLAGS@': '',
                            '@CTK_EXTRA_LIBS@': '',
                            '@CTK_EXTRA_CFLAGS@': '',
                            'ctk-3': 'ctk-3.0'}

    gail_pc_replace_items = {'gailutil-3': 'gailutil-3.0'}

    pkg_replace_items.update(base_pc.base_replace_items)
    cdk_pc_replace_items.update(pkg_replace_items)
    ctk_pc_replace_items.update(pkg_replace_items)
    gail_pc_replace_items.update(base_pc.base_replace_items)

    # Generate cdk-3.0.pc
    replace_multi(base_pc.top_srcdir + '/cdk-3.0.pc.in',
                  base_pc.srcdir + '/cdk-3.0.pc',
                  cdk_pc_replace_items)

    # Generate ctk+-3.0.pc
    replace_multi(base_pc.top_srcdir + '/ctk+-3.0.pc.in',
                  base_pc.srcdir + '/ctk+-3.0.pc',
                  ctk_pc_replace_items)

    # Generate gail-3.0.pc
    replace_multi(base_pc.top_srcdir + '/gail-3.0.pc.in',
                  base_pc.srcdir + '/gail-3.0.pc',
                  gail_pc_replace_items)

if __name__ == '__main__':
    sys.exit(main(sys.argv))
