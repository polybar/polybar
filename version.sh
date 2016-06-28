#!/bin/sh

msg() {
  echo " \033[1;32m**\033[0m" "$@"
}

main() {
  version="$(git describe --tags --dirty=-dev)"

  msg "Current version: ${version}"

  sed -r "/#define GIT_TAG/s/GIT_TAG .*/GIT_TAG \"${version}\"/" -i include/version.hpp

  if [ "$(git diff include/version.hpp 2>/dev/null)" ]; then
    msg "Updated include/version.hpp"
  else
    msg "<include/version.hpp> is already up-to-date"
  fi
}

main "$@"
