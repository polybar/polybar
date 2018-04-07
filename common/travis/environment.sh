#!/bin/bash
# Update compiler flags
if [ "${CXX:0:7}" = "clang++" ]; then
  export CXX="clang++-3.8"
  export CC="clang-3.8"
  export CXXFLAGS="${CXXFLAGS} -Qunused-arguments"
elif [ "${CXX:0:3}" = "g++" ]; then
  export CXX="g++-5"
  export CC="gcc-5"
fi
