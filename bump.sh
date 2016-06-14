#!/bin/sh

# shellcheck disable=SC2016
tag_prev=$(git tag -l | tail -2 | head -1)
tag_curr=$(git tag -l | tail -1)

sed -r "s/${tag_prev}/${tag_curr}/g" -i README.md CMakeLists.txt contrib/lemonbuddy.aur/PKGBUILD contrib/lemonbuddy.aur/.SRCINFO
git add README.md CMakeLists.txt contrib/lemonbuddy.aur/PKGBUILD contrib/lemonbuddy.aur/.SRCINFO
git commit -m "build: Bump version to ${tag_curr}"
git show HEAD
