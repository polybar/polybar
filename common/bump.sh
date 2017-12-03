#!/bin/sh

main() {
  if [ $# -eq 0 ]; then
    version=$(git describe --tags --abbrev=0)
    set -- "${version%.*}.$((${version##*.}+1))"
  fi

  git tag "$@" || exit 1

  tag_curr="$(git tag --sort=version:refname | tail -1)"
  tag_prev="$(git tag --sort=version:refname | tail -2 | head -1)"

  if [ -x ./common/version.sh ]; then
    ./common/version.sh "$tag_curr"
  fi

  sed -r "s/${tag_prev}/${tag_curr}/g" -i \
    README.md CMakeLists.txt \
    contrib/polybar.aur/PKGBUILD \
    contrib/polybar-git.aur/PKGBUILD

  git add -u README.md CMakeLists.txt \
    contrib/polybar.aur/PKGBUILD \
    contrib/polybar-git.aur/PKGBUILD \
    include/version.hpp

  git commit -m "build: Bump version to ${tag_curr}"

  # Recreate the tag to include the last commit
  [ $# -eq 1 ] && git tag -f "$@"
}

main "$@"
