# Maintainer: Michael Carlberg <c@rlberg.se>
# Contributor: Michael Carlberg <c@rlberg.se>
pkgname=polybar
pkgver=3.0.5
pkgrel=4
pkgdesc="A fast and easy-to-use status bar"
arch=("i686" "x86_64")
url="https://github.com/jaagr/polybar"
license=("MIT")
depends=("cairo" "xcb-util-image" "xcb-util-wm" "xcb-util-xrm")
optdepends=("alsa-lib: volume module support"
            "libmpdclient: mpd module support"
            "wireless_tools: network module support"
            "jsoncpp: i3 module support"
            "i3ipc-glib-git: i3 module support"
            "ttf-unifont: Font used in example config"
            "siji-git: Font used in example config"
            "curl: github module support")
makedepends=("clang" "cmake" "git" "python" "python2" "pkg-config")
conflicts=("polybar-git")
install="${pkgname}.install"
source=("${pkgname}::git+${url}.git#tag=${pkgver}")
md5sums=("SKIP")

prepare() {
  git -C "${pkgname}" submodule update --init --recursive
  git -C "${pkgname}" cherry-pick -n d35abc7620c8f06618b4708d9a969dfa2f309e96
  mkdir -p "${pkgname}/build"
}

build() {
  cd "${pkgname}/build" || exit 1
  cmake -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_C_COMPILER="clang" -DCMAKE_CXX_COMPILER="clang++" ..
  cmake --build .
}

package() {
  cmake --build "${pkgname}/build" --target install -- DESTDIR="${pkgdir}"
  install -Dm644 "${pkgname}/LICENSE" "${pkgdir}/usr/share/licenses/${pkgname}/LICENSE"
}
