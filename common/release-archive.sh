#!/usr/bin/env bash

set -euo pipefail

git_url="https://github.com/polybar/polybar.git"
wd="$(realpath .)"

usage() {
  cat <<EOF
Usage: $0 [-h] TAG

Creates a polybar release archive for the given git tag.

-h           Print this help message
EOF
}


cleanup() {
  if [ -d "$tmp_dir" ]; then
    rm -rf "$tmp_dir"
  fi
}

if [ $# -ne 1 ] ; then
  usage
  exit 1
fi

if [ "$1" = "-h" ]; then
  usage
  exit 0
fi

version="$1"
tmp_dir="$(mktemp -d)"
archive="$wd/polybar-${version}.tar"

trap cleanup EXIT

git clone "$git_url" "$tmp_dir/polybar"

cd "$tmp_dir/polybar"

echo "Looking for tag '$version'"

if [ "$(git tag -l "$version" | wc -l)" != "1" ]; then
  echo "Tag '$version' not found"
  exit 1
fi

git checkout "$version"
git submodule update --init --recursive >/dev/null

find . -type d -name ".git" -exec rm -rf {} \+

cd "$tmp_dir"
tar cf "$archive" "polybar"
sha256sum "$archive"
