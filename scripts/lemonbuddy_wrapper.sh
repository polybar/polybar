#!/usr/bin/env bash

#set -eux
set -eu
set -o pipefail

[ $# -eq 0 ] && {
  echo "No bar specified" ; exit 1
}

command -v lemonbar >/dev/null || {
  echo "Lemonbar is not installed" ; exit 1
}

command -v lemonbuddy >/dev/null || {
  echo "Lemonbuddy is not installed" ; exit 1
}

lemonbar="$(lemonbuddy "$@" -x)"
wmname="$(lemonbuddy "$@" -w)"
logfile="${XDG_CACHE_HOME:-$HOME/.cache}/lemonbuddy/${wmname}.log"
logdir="$(dirname "$logfile")"
pipe="$(mktemp -u /tmp/lemonbuddy.in.XXXXX)"

[ -d "$logdir" ] || mkdir -p "$logdir"

mkfifo "$pipe"

cleanup_proc() {
  pid=$1
  kill -0 "$pid" 2>/dev/null && {
    echo "$pid is running (sending term signal)..."
    kill -TERM "$pid" 2>/dev/null
  }
}

# shellcheck disable=SC2094
{ lemonbuddy "$@" -p "$pipe" 2>"$logfile"; kill -TERM $$ 2>/dev/null; } | $lemonbar >"$pipe" &

trap 'cleanup_proc $!' TERM INT

while kill -0 $! 2>/dev/null; do
  sleep 0.5s
done

[ -e "$pipe" ] && rm "$pipe"

kill 0; wait
