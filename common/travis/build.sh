#!/bin/bash
cd "${TRAVIS_BUILD_DIR}/build" || false
make || exit $?
