#!/usr/bin/env sh

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

# shellcheck disable=SC2094
lemonbuddy "$@" -p "$pipe" 2>"$logfile" | $lemonbar >"$pipe" & pid=$!

sigaction() {
  printf "\r"
  kill -TERM $pid 2>/dev/null
  echo "Waiting for processes to terminate..."
}

trap sigaction TERM INT PIPE CHLD

while true; do
  wait || break
done
