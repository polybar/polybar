#!/usr/bin/env bash

readonly SELF=${0##*/}
declare -rA COLORS=(
  [RED]=$'\033[0;31m'
  [GREEN]=$'\033[0;32m'
  [BLUE]=$'\033[0;34m'
  [PURPLE]=$'\033[0;35m'
  [CYAN]=$'\033[0;36m'
  [WHITE]=$'\033[0;37m'
  [YELLOW]=$'\033[0;33m'
  [BOLD]=$'\033[1m'
  [OFF]=$'\033[0m'
)

usage() {
  echo "
  Builds and installs polybar.

  ${COLORS[GREEN]}${COLORS[BOLD]}Usage:${COLORS[OFF]}
      ${COLORS[CYAN]}${SELF}${COLORS[OFF]} [options]

  ${COLORS[GREEN]}${COLORS[BOLD]}Options:${COLORS[OFF]}
      ${COLORS[GREEN]}-3, --i3${COLORS[OFF]}
          Include support for internal/i3 (requires i3); disabled by default.
      ${COLORS[GREEN]}-a, --alsa${COLORS[OFF]}
          Include support for internal/alsa (requires alsalib); disabled by default.
      ${COLORS[GREEN]}-p, --pulseaudio${COLORS[OFF]}
          Include support for internal/pulseaudio (requires libpulse); disabled by default.
      ${COLORS[GREEN]}-n, --network${COLORS[OFF]}
          Include support for internal/network (requires libnl/libiw); disabled by default.
      ${COLORS[GREEN]}-m, --mpd${COLORS[OFF]}
          Include support for internal/mpd (requires libmpdclient); disabled by default.
      ${COLORS[GREEN]}-c, --curl${COLORS[OFF]}
          Include support for internal/github (requires libcurl); disabled by default.
      ${COLORS[GREEN]}-i, --ipc${COLORS[OFF]}
          Build polybar-msg used to send ipc messages; disabled by default.
      ${COLORS[GREEN]}--all-features${COLORS[OFF]}
          Enable all abovementioned features;
          equal to -3 -a -p -n -m -c -i
      ${COLORS[GREEN]}-g, --gcc${COLORS[OFF]}
          Use GCC even if Clang is installed; disabled by default.
      ${COLORS[GREEN]}-f${COLORS[OFF]}
          Remove existing build dir; disabled by default.
      ${COLORS[GREEN]}-I, --no-install${COLORS[OFF]}
          Do not execute 'sudo make install'; enabled by default.
      ${COLORS[GREEN]}-C, --install-config${COLORS[OFF]}
          Install example configuration; disabled by default.
      ${COLORS[GREEN]}-A, --auto${COLORS[OFF]}
          Automatic, non-interactive installation; disabled by default.
          When set, script defaults options not explicitly set.
      ${COLORS[GREEN]}-h, --help${COLORS[OFF]}
          Displays this help.
"
}

msg_err() {
  echo -e "${COLORS[RED]}${COLORS[BOLD]}** ${COLORS[OFF]}$*\n"
  exit 1
}

msg() {
  echo -e "${COLORS[GREEN]}${COLORS[BOLD]}** ${COLORS[OFF]}$*\n"
}

install() {
  local p

  if [[ "$AUTO" == ON ]]; then
    [[ -z "$INSTALL" ]] && INSTALL="ON"
    [[ -z "$INSTALL_CONF" ]] && INSTALL_CONF="OFF"
  fi

  if [[ -z "$INSTALL" ]]; then
    read -r -p "$(msg "Execute 'sudo make install'? [Y/n] ")" -n 1 p && echo
    [[ "${p^^}" != "N" ]] && INSTALL="ON" || INSTALL="OFF"
  fi

  if [[ -z "$INSTALL_CONF" ]]; then
    read -r -p "$(msg "Install example configuration? [y/N]: ")" -n 1 p && echo
    [[ "${p^^}" != "Y" ]] && INSTALL_CONF="OFF" || INSTALL_CONF="ON"
  fi


  if [[ "$INSTALL" == ON ]]; then
    sudo make install || msg_err "Failed to install executables..."
  fi

  if [[ "$INSTALL_CONF" == ON ]]; then
    make userconfig || msg_err "Failed to install user configuration..."
  fi
}

set_build_opts() {
  local p

  msg "Setting build options"

  if [[ "$AUTO" == ON ]]; then
    [[ -z "$USE_GCC" ]] && USE_GCC="OFF"
    [[ -z "$ENABLE_I3" ]] && ENABLE_I3="OFF"
    [[ -z "$ENABLE_ALSA" ]] && ENABLE_ALSA="OFF"
    [[ -z "$ENABLE_PULSEAUDIO" ]] && ENABLE_PULSEAUDIO="OFF"
    [[ -z "$ENABLE_NETWORK" ]] && ENABLE_NETWORK="OFF"
    [[ -z "$ENABLE_MPD" ]] && ENABLE_MPD="OFF"
    [[ -z "$ENABLE_CURL" ]] && ENABLE_CURL="OFF"
    [[ -z "$ENABLE_IPC_MSG" ]] && ENABLE_IPC_MSG="OFF"
  fi

  if [[ -z "$USE_GCC" ]]; then
    read -r -p "$(msg "Use GCC even if Clang is installed ----------------------------- [y/N]: ")" -n 1 p && echo
    [[ "${p^^}" != "Y" ]] && USE_GCC="OFF" || USE_GCC="ON"
  fi

  if [[ -z "$ENABLE_I3" ]]; then
    read -r -p "$(msg "Include support for \"internal/i3\" (requires i3) ---------------- [y/N]: ")" -n 1 p && echo
    [[ "${p^^}" != "Y" ]] && ENABLE_I3="OFF" || ENABLE_I3="ON"
  fi

  if [[ -z "$ENABLE_ALSA" ]]; then
    read -r -p "$(msg "Include support for \"internal/alsa\" (requires alsalib) --------- [y/N]: ")" -n 1 p && echo
    [[ "${p^^}" != "Y" ]] && ENABLE_ALSA="OFF" || ENABLE_ALSA="ON"
  fi

  if [[ -z "$ENABLE_PULSEAUDIO" ]]; then
    read -r -p "$(msg "Include support for \"internal/pulseaudio\" (requires libpulse) -- [y/N]: ")" -n 1 p && echo
    [[ "${p^^}" != "Y" ]] && ENABLE_PULSEAUDIO="OFF" || ENABLE_PULSEAUDIO="ON"
  fi

  if [[ -z "$ENABLE_NETWORK" ]]; then
    read -r -p "$(msg "Include support for \"internal/network\" (requires libnl/libiw) -- [y/N]: ")" -n 1 p && echo
    [[ "${p^^}" != "Y" ]] && ENABLE_NETWORK="OFF" || ENABLE_NETWORK="ON"
  fi

  if [[ -z "$ENABLE_MPD" ]]; then
    read -r -p "$(msg "Include support for \"internal/mpd\" (requires libmpdclient) ----- [y/N]: ")" -n 1 p && echo
    [[ "${p^^}" != "Y" ]] && ENABLE_MPD="OFF" || ENABLE_MPD="ON"
  fi

  if [[ -z "$ENABLE_CURL" ]]; then
    read -r -p "$(msg "Include support for \"internal/github\" (requires libcurl) ------- [y/N]: ")" -n 1 p && echo
    [[ "${p^^}" != "Y" ]] && ENABLE_CURL="OFF" || ENABLE_CURL="ON"
  fi

  if [[ -z "$ENABLE_IPC_MSG" ]]; then
    read -r -p "$(msg "Build \"polybar-msg\" used to send ipc messages ------------------ [y/N]: ")" -n 1 p && echo
    [[ "${p^^}" != "Y" ]] && ENABLE_IPC_MSG="OFF" || ENABLE_IPC_MSG="ON"
  fi


  CXX="c++"

  if [[ "$USE_GCC" == OFF ]]; then
    if command -v clang++ >/dev/null; then
      msg "Using compiler: clang++/clang"
      CXX="clang++"
    elif command -v g++ >/dev/null; then
      msg "Using compiler: g++/gcc"
      CXX="g++"
    fi
  else
    CXX="g++"
  fi
}

main() {
  [[ -d ./.git ]] && {
    msg "Fetching submodules"
    git submodule update --init --recursive || msg_err "Failed to clone submodules"
  }

  [[ -d ./build ]] && {
    if [[ "$REMOVE_BUILD_DIR" == ON ]]; then
      msg "Removing existing build dir (-f)"
      rm -rf ./build >/dev/null || msg_err "Failed to remove existing build dir"
    else
      msg "A build dir already exists (pass -f to replace)"
    fi
  }

  mkdir -p ./build || msg_err "Failed to create build dir"
  cd ./build || msg_err "Failed to enter build dir"

  set_build_opts

  msg "Executing cmake command"
  cmake                                       \
    -DCMAKE_CXX_COMPILER="${CXX}"             \
    -DENABLE_ALSA:BOOL="${ENABLE_ALSA}"       \
    -DENABLE_PULSEAUDIO:BOOL="${ENABLE_PULSEAUDIO}"\
    -DENABLE_I3:BOOL="${ENABLE_I3}"           \
    -DENABLE_MPD:BOOL="${ENABLE_MPD}"         \
    -DENABLE_NETWORK:BOOL="${ENABLE_NETWORK}" \
    -DENABLE_CURL:BOOL="${ENABLE_CURL}"       \
    -DBUILD_IPC_MSG:BOOL="${ENABLE_IPC_MSG}"   \
    .. || msg_err "Failed to generate build... read output to get a hint of what went wrong"

  msg "Building project"
  make || msg_err "Failed to build project"
  install
  msg "Build complete!"

  exit 0
}


#################
###### Entry
#################
while [[ "$1" == -* ]]; do
  case "$1" in
    -3|--i3)
      ENABLE_I3=ON; shift ;;
    -a|--alsa)
      ENABLE_ALSA=ON; shift ;;
    -p|--pulseaudio)
      ENABLE_PULSEAUDIO=ON; shift ;;
    -n|--network)
      ENABLE_NETWORK=ON; shift ;;
    -m|--mpd)
      ENABLE_MPD=ON; shift ;;
    -c|--curl)
      ENABLE_CURL=ON; shift ;;
    -i|--ipc)
      ENABLE_IPC_MSG=ON; shift ;;
    --all-features)
      ENABLE_I3=ON
      ENABLE_ALSA=ON
      ENABLE_PULSEAUDIO=ON
      ENABLE_NETWORK=ON
      ENABLE_MPD=ON
      ENABLE_CURL=ON
      ENABLE_IPC_MSG=ON
      shift ;;

    -g|--gcc)
      USE_GCC=ON; shift ;;
    -f)
      REMOVE_BUILD_DIR=ON; shift ;;
    -I|--no-install)
      INSTALL=OFF; shift ;;
    -C|--install-config)
      INSTALL_CONF=ON; shift ;;
    -A|--auto)
      AUTO=ON; shift ;;
    -h|--help)
      usage
      exit 0
      ;;
    --) shift; break ;;
    *)
      usage
      [[ "$1" =~ ^-[0-9a-zA-Z]{2,}$ ]] && msg_err "don't combine options: ie do [-c -i] instead of [-ci]" || msg_err "unknown option [$1]"
      ;;
  esac
done

main

