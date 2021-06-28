#!/usr/bin/python3

# Generate cdk/cdkversionmacros.h

# Author: Fan, Chun-wei
# Date: July 25, 2019

import os
import sys
import argparse

from replace import replace_multi, replace

def main(argv):
    srcdir = os.path.dirname(__file__)
    top_srcdir = os.path.join(srcdir, os.pardir)
    parser = argparse.ArgumentParser(description='Generate cdkversionmacros.h')
    parser.add_argument('--version', help='Version of the package',
                        required=True)
    args = parser.parse_args()
    cdk_sourcedir = os.path.join(top_srcdir, 'cdk')
    version_parts = args.version.split('.')

    cdkversionmacro_replace_items = {'@CTK_MAJOR_VERSION@': version_parts[0],
                                     '@CTK_MINOR_VERSION@': version_parts[1],
                                     '@CTK_MICRO_VERSION@': version_parts[2]}

    replace_multi(os.path.join(cdk_sourcedir, 'cdkversionmacros.h.in'),
                  os.path.join(cdk_sourcedir, 'cdkversionmacros.h'),
                  cdkversionmacro_replace_items)

if __name__ == '__main__':
    sys.exit(main(sys.argv))
