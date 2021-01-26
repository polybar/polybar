#!/bin/sh

main() {
  if [ $# -lt 1 ]; then
    echo "$0 DIR..." 1>&2
    exit 1
  fi

  # Search paths
  search="${*:-.}"

  echo "$0 in $search"

  # shellcheck disable=2086
  find $search -regex ".*.[c|h]pp"                           \
    -exec printf "\\033[32;1m** \\033[0mFormatting %s\\n" {} \; \
    -exec clang-format -style=file -i {} \;
}

main "$@"
