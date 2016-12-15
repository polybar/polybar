#!/bin/bash
export DEPS_DIR="${TRAVIS_BUILD_DIR}/deps"
export LLVM_ROOT="${DEPS_DIR}/llvm-${LLVM_VERSION}"

mkdir -p "${DEPS_DIR}"
mkdir -p "${LLVM_ROOT}"

# Update compiler flags
if [ "${CXX:0:7}" = "clang++" ]; then
  export CXX="clang++-3.8"
  export CC="clang-3.8"
  export CXXFLAGS="${CXXFLAGS} -Qunused-arguments"
elif [ "${CXX}" = "g++" ]; then
  export CXX="g++-5"
  export CC="gcc-5"
fi
