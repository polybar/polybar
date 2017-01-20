#!/bin/bash
if [ -z "$(ls -A "${DEPS_DIR}/xcb-util-xrm" 2>/dev/null)" ]; then
  git clone --recursive https://github.com/Airblader/xcb-util-xrm
  cd xcb-util-xrm || exit 1
  ./autogen.sh
fi

cd "${DEPS_DIR}/xcb-util-xrm" || exit 1
make install
