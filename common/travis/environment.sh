#!/bin/bash
# Update compiler flags
if [ "${CXX}" = "clang++" ]; then
  export CXXFLAGS="${CXXFLAGS} -Qunused-arguments"
elif [ "${CXX}" = "g++" ]; then
  export CXX="g++-5"
  export CC="gcc-5"
fi
