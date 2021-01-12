#!/usr/bin/env bash

set -eo pipefail

set -x

"${CXX}" --version
cmake --version

set +x

echo "PATH=${PATH}"
echo "CXX=${CXX}"
echo "CXXFLAGS=${CXXFLAGS}"
echo "LDFLAGS=${LDFLAGS}"
echo "LD_LIBRARY_PATH=${LD_LIBRARY_PATH}"
echo "MAKEFLAGS=${MAKEFLAGS}"
echo "POLYBAR_BUILD_TYPE=${POLYBAR_BUILD_TYPE}"
echo "CMAKE_BUILD_TYPE=${BUILD_TYPE}"
