#!/bin/bash
if [ -z "$(ls -A "${DEPS_DIR}/cmake/bin" 2>/dev/null)" ]; then
  mkdir -p "${DEPS_DIR}/cmake"
  cd "${DEPS_DIR}/cmake" || exit 1
  travis_retry wget --no-check-certificate -O - "${CMAKE_URL}" | tar --strip-components=1 -xz
fi

export PATH="${DEPS_DIR}/cmake/bin:${PATH}"
