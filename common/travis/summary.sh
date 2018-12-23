#!/bin/bash
echo "${CXX} --version"
eval "${CXX} --version"

echo "cmake --version"
cmake --version

echo "PATH=${PATH}"
echo "CXX=${CXX}"
echo "CXXFLAGS=${CXXFLAGS}"
echo "LDFLAGS=${LDFLAGS}"
echo "LD_LIBRARY_PATH=${LD_LIBRARY_PATH}"
echo "JOBS=${JOBS}"
