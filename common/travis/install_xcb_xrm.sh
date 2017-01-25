#!/bin/bash
if [ -z "$(ls -A "${DEPS_DIR}/xcb-util-xrm" 2>/dev/null)" ]; then
  git clone --recursive https://github.com/Airblader/xcb-util-xrm
  cd xcb-util-xrm && {
    ./autogen.sh --prefix=/usr --libdir=/usr/lib
    make
    sudo make install
  }
fi
