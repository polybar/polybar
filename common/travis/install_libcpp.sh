#!/bin/sh
if [ "${CXX}" = "clang++" ]; then
  if [ -z "$(ls -A "${LLVM_ROOT}/install/include" 2>/dev/null)" ]; then
    mkdir -p "${LLVM_ROOT}" "${LLVM_ROOT}/build" "${LLVM_ROOT}/projects/libcxx" "${LLVM_ROOT}/projects/libcxxabi"

    travis_retry wget --quiet -O - "${LLVM_URL}" | tar --strip-components=1 -xJ -C "${LLVM_ROOT}"
    travis_retry wget --quiet -O - "${LIBCXX_URL}" | tar --strip-components=1 -xJ -C "${LLVM_ROOT}/projects/libcxx"
    travis_retry wget --quiet -O - "${LIBCXXABI_URL}" | tar --strip-components=1 -xJ -C "${LLVM_ROOT}/projects/libcxxabi"

    (cd "${LLVM_ROOT}/build" && cmake .. -DCMAKE_CXX_COMPILER=clang++ && make cxxabi cxx -j2)
    (cd "${LLVM_ROOT}/build/projects/libcxx" && make install)
    (cd "${LLVM_ROOT}/build/projects/libcxxabi" && make install)
  fi

  export CXXFLAGS="${CXXFLAGS} -I${LLVM_ROOT}/install/include"
  export CXXFLAGS="${CXXFLAGS} -I${LLVM_ROOT}/install/include/c++/v1"
  export CXXFLAGS="${CXXFLAGS} -stdlib=libc++"
  export LDFLAGS="${LDFLAGS} -L${LLVM_ROOT}/install/lib"
  export LDFLAGS="${LDFLAGS} -lc++"
  export LDFLAGS="${LDFLAGS} -lc++abi"
  export LD_LIBRARY_PATH="${LD_LIBRARY_PATH}:${LLVM_ROOT}/install/lib"
fi
