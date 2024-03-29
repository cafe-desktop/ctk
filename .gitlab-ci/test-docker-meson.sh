#!/bin/bash

set -e

mkdir -p _ccache
export CCACHE_BASEDIR="$(pwd)"
export CCACHE_DIR="${CCACHE_BASEDIR}/_ccache"

export PATH="${HOME}/.local/bin:${PATH}"
python3 -m pip install --user meson==0.49.2

meson \
    -Dinstalled_tests=true \
    -Dbroadway_backend=true \
    -Dx11_backend=true \
    -Dwayland_backend=true \
    -Dxinerama=yes \
    -Dprint_backends="file,lpr,test,cups" \
    ${EXTRA_MESON_FLAGS:-} \
    _build

cd _build
ninja

xvfb-run -a -s "-screen 0 1024x768x24" \
    meson test \
        --timeout-multiplier 4 \
        --print-errorlogs \
        --suite=ctk+-3.0 \
        --no-suite=ctk+-3.0:a11y
