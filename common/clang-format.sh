#!/bin/sh

main() {
  if [ $# -lt 1 ]; then
    printf "%s DIR...\n" "$0" 1>&2
    exit 1
  fi
  search="${*:-.}"

  [ -d "$search" ] || search="$(dirname "$search")"

  # shellcheck disable=2086
  find $search -regex ".*.[c|h]pp"                           \
    -exec printf "\033[32;1m** \033[0mFormatting %s\n" {} \; \
    -exec clang-format -style=file -i {} \;
}

main "$@"
