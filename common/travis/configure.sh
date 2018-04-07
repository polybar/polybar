#!/bin/bash
mkdir -p "${TRAVIS_BUILD_DIR}/build"
cd "${TRAVIS_BUILD_DIR}/build" || false
cmake \
  -DCMAKE_C_COMPILER="${CC}" \
  -DCMAKE_CXX_COMPILER="${CXX}" \
  -DCMAKE_CXX_FLAGS="${CXXFLAGS}" \
  -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
  -DBUILD_TESTS:BOOL="${BUILD_TESTS:-OFF}" ..
