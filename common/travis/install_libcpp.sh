#!/bin/bash
if [ "${CXX:0:7}" = "clang++" ]; then
  if [ -z "$(ls -A "${LLVM_ROOT}/install/include" 2>/dev/null)" ]; then
    mkdir -p "${LLVM_ROOT}" "${LLVM_ROOT}/build" "${LLVM_ROOT}/projects/libcxx" "${LLVM_ROOT}/projects/libcxxabi"

    travis_retry wget --quiet -O - "${LLVM_URL}" | tar --strip-components=1 -xJ -C "${LLVM_ROOT}"
    travis_retry wget --quiet -O - "${LIBCXX_URL}" | tar --strip-components=1 -xJ -C "${LLVM_ROOT}/projects/libcxx"
    travis_retry wget --quiet -O - "${LIBCXXABI_URL}" | tar --strip-components=1 -xJ -C "${LLVM_ROOT}/projects/libcxxabi"

    (cd "${LLVM_ROOT}/build" && cmake .. -DCMAKE_CXX_COMPILER="$CXX" -DCMAKE_C_COMPILER="$CC" -DCMAKE_INSTALL_PREFIX="${LLVM_ROOT}/install" -DCMAKE_BUILD_TYPE=$BUILD_TYPE)
    (cd "${LLVM_ROOT}/build/projects/libcxx" && make install)
    (cd "${LLVM_ROOT}/build/projects/libcxxabi" && make install)
  fi

  export CXXFLAGS="${CXXFLAGS} -I ${LLVM_ROOT}/install/include/c++/v1"
  export LDFLAGS="${LDFLAGS} -L ${LLVM_ROOT}/install/lib -lc++ -lc++abi"
  export LD_LIBRARY_PATH="${LD_LIBRARY_PATH}:${LLVM_ROOT}/install/lib"
fi
