#!/bin/sh

msg() {
  if [ -t 1 ]; then
    printf " \033[1;32m**\033[0m %s\n" "$@"
  else
    printf "** %s\n" "$@"
  fi
}

main() {
  if [ $# -eq 0 ]; then
    set -- "$(git describe --tags --dirty=-dev)"
  fi

  GIT_TAG_NAMESPACE=$(echo "v$1" | sed "s/[^a-zA-Z0-9_]/_/g")

  msg "Current version: $1"
  sed -r "/#define GIT_TAG/s/GIT_TAG .*/GIT_TAG \"$1\"/" -i include/version.hpp
  sed -r "/#define GIT_TAG_NAMESPACE/s/GIT_TAG_NAMESPACE .*/GIT_TAG_NAMESPACE $GIT_TAG_NAMESPACE/" -i include/version.hpp

  if git diff include/version.hpp 2>/dev/null | grep -q .; then
    msg "Updated include/version.hpp"
  else
    msg "<include/version.hpp> is already up-to-date"
  fi
}

main "$@"
