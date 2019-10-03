#!/bin/bash
mkdir -p "${TRAVIS_BUILD_DIR}/build"
cd "${TRAVIS_BUILD_DIR}/build" || false

FLAGS=""

# Disable all extra modules and X extensions for minimal builds
# Most of these should already be turned off because their libraries are not
# installed, but some may not be
if [ "$POLYBAR_BUILD_TYPE" == "minimal" ]; then
  FLAGS=(
  "-DENABLE_PULSEAUDIO=OFF"
  "-DENABLE_NETWORK=OFF"
  "-DENABLE_MPD=OFF"
  "-DENABLE_CURL=OFF"
  "-DENABLE_ALSA=OFF"
  "-DENABLE_I3=OFF"
  "-DWITH_XRM=OFF"
  "-DWITH_XKB=OFF"
  "-DWITH_XRANDR_MONITORS=OFF"
  "-DWITH_XCURSOR=OFF"
  "-DWITH_XRANDR=ON"
  )
fi

cmake \
  -DCMAKE_CXX_COMPILER="${CXX}" \
  -DCMAKE_CXX_FLAGS="${CXXFLAGS} -Werror" \
  -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
  -DBUILD_TESTS:BOOL="${BUILD_TESTS:-OFF}" \
  -DBUILD_DOC:BOOL="${BUILD_DOC:-OFF}" \
  "${FLAGS[@]}" ..
