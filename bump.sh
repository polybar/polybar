#!/bin/sh

# Use passed argument as new tag
[ $# -eq 1 ] && {
  git tag "$@" || exit 1
}

# shellcheck disable=SC2016
tag_prev=$(git tag -l | tail -2 | head -1)
tag_curr=$(git tag -l | tail -1)

./version.sh "$tag_curr"

sed -r "s/${tag_prev}/${tag_curr}/g" -i README.md CMakeLists.txt contrib/lemonbuddy.aur/PKGBUILD contrib/lemonbuddy.aur/.SRCINFO
git add README.md CMakeLists.txt contrib/lemonbuddy.aur/PKGBUILD contrib/lemonbuddy.aur/.SRCINFO
git add -u include/version.hpp
git commit -m "build: Bump version to ${tag_curr}"

# Recreate the tag to include the last commit
[ $# -eq 1 ] && git tag -f "$@"
