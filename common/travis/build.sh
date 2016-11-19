#!/bin/sh
cd "${TRAVIS_BUILD_DIR}/build" && make -j"${JOBS}"
