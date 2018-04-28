#!/bin/bash
r=0

make all_unit_tests || exit $?

for test in tests/unit_test.*; do
  [ -x "$test" ] || continue

  if $test; then
    printf "\033[1;32m%s\033[0m\n" "${test##*/} passed"
  else
    r=1
    printf "\033[1;31m%s\033[0m\n" "${test##*/} failed"
  fi
done
exit $r
