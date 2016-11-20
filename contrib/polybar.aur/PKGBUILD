# Maintainer: Michael Carlberg <c@rlberg.se>
# Contributor: Michael Carlberg <c@rlberg.se>
pkgname=polybar
pkgver=2.3.1
pkgrel=1
pkgdesc="A fast and easy-to-use status bar"
arch=("i686" "x86_64")
url="https://github.com/jaagr/polybar"
license=("MIT")
depends=("libxft" "xcb-util-wm" "xcb-util-image")
optdepends=("alsa-lib: volume module support"
            "libmpdclient: mpd module support"
            "wireless_tools: network module support"
            "jsoncpp: i3 module support"
            "i3ipc-glib-git: i3 module support"
            "ttf-unifont: Font used in example config"
            "siji-git: Font used in example config")
makedepends=("cmake" "python2" "pkg-config" "boost")
conflicts=("polybar-git" "lemonbuddy-git" "lemonbuddy")
source=("${pkgname}::git+${url}.git#tag=${pkgver}")
md5sums=("SKIP")

prepare() {
  cd "$pkgname" || exit
  git submodule update --init --recursive
  mkdir build
}

build() {
  cd "${pkgname}/build" || exit
  cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr ..
  make
}

package() {
  cd "${pkgname}/build" || exit
  make DESTDIR="${pkgdir}/" install
  cd .. || exit
  install -D -m644 LICENSE "${pkgdir}/usr/share/licenses/${pkgname}/LICENSE"
}
