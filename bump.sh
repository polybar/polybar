#!/bin/sh

# Use passed argument as new tag
if [ $# -eq 0 ]; then
  version=$(git describe --tags --abbrev=0)
  patch=${version##*.}
  set -- "${version%.*}.$((patch+1))"
fi

git tag "$@" || exit 1

# shellcheck disable=SC2016
tag_curr=$(git describe --tags --abbrev=0)
tag_curr_patch="${tag_curr##*.}"
tag_prev="${tag_curr%.*}.$((tag_curr_patch-1))"

./version.sh "$tag_curr"

sed -r "s/${tag_prev}/${tag_curr}/g" -i \
  README.md CMakeLists.txt \
  contrib/lemonbuddy.aur/PKGBUILD contrib/lemonbuddy.aur/.SRCINFO \
  contrib/lemonbuddy-git.aur/PKGBUILD contrib/lemonbuddy-git.aur/.SRCINFO

git add -u README.md CMakeLists.txt \
  contrib/lemonbuddy.aur/PKGBUILD contrib/lemonbuddy.aur/.SRCINFO \
  contrib/lemonbuddy-git.aur/PKGBUILD contrib/lemonbuddy-git.aur/.SRCINFO \
  include/version.hpp

git commit -m "build: Bump version to ${tag_curr}"

# Recreate the tag to include the last commit
[ $# -eq 1 ] && git tag -f "$@"
