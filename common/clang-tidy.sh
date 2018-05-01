#!/bin/sh

main() {
  if [ $# -lt 2 ]; then
    echo "$0 build_path [-fix] DIR..." 1>&2
    exit 1
  fi

  args="-p $1"; shift

  if [ "$1" = "-fix" ]; then
    args="${args} -fix"; shift
  fi

  # Search paths
  search="${*:-.}"

  echo "$0 in $search"

  # shellcheck disable=2086
  find $search -iname "*.cpp"                                          \
    -exec printf "\\033[32;1m** \\033[0mProcessing %s\\n" {} \; \
    -exec clang-tidy $args {} \;
}

main "$@"
