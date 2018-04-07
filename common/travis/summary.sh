#!/bin/bash
echo "${CXX} --version"
eval "${CXX} --version"

echo "${CC} --version"
eval "${CC} --version"

echo "cmake --version"
cmake --version

echo "PATH=${PATH}"
echo "CXX=${CXX}"
echo "CC=${CC}"
echo "CXXFLAGS=${CXXFLAGS}"
echo "LDFLAGS=${LDFLAGS}"
echo "LD_LIBRARY_PATH=${LD_LIBRARY_PATH}"
echo "JOBS=${JOBS}"
