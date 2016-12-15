#!/bin/bash
if [ -z "$(ls -A "${DEPS_DIR}/cmake/bin" 2>/dev/null)" ]; then
  mkdir -p cmake && travis_retry wget --no-check-certificate --quiet -O - "${CMAKE_URL}" | tar --strip-components=1 -xz -C cmake
fi

export PATH="${DEPS_DIR}/cmake/bin:${PATH}"
