#!/bin/bash
cd "${TRAVIS_BUILD_DIR}/build" && make -j"${JOBS}"
