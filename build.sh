#!/usr/bin/env bash

function msg_err {
  printf "\033[41;30m err \033[0m %s\n" "$@"
  exit 1
}

function msg {
  printf "\033[36;1m info \033[0m%s\n" "$@"
}

function main
{
  [[ -d ./build ]] && msg_err "A build directory already exists"
  [[ -d ./.git ]] && {
    git submodule update --init --recursive || msg_err "Failed to clone submodules"
  }

  mkdir ./build || msg_err "Failed to create build dir"
  cd ./build || msg_err "Failed to enter build dir"

  cmake -DCMAKE_INSTALL_PREFIX=/usr .. || \
    msg_err "Failed to generate build... read output to get a hint of what went wrong"

  make || msg_err "Failed to build project"

  echo -e "\n"

  read -N1 -p "Do you want to execute \"sudo make install\"? [Y/n] " -r choice
  echo

  if [[ "${choice^^}" == "Y" ]]; then
    sudo make install || msg_err "Failed to install executables..."
  fi
}

main "$@"
