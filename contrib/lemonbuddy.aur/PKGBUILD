# Maintainer: Michael Carlberg <c@rlberg.se>
# Contributor: Michael Carlberg <c@rlberg.se>
pkgname=lemonbuddy
pkgver=1.2.2
pkgrel=1
pkgdesc="A fast and easy-to-use tool for Lemonbar"
arch=("i686" "x86_64")
url="https://github.com/jaagr/lemonbuddy"
license=("MIT")
depends=("bash" "libxrandr")
optdepends=("alsa-lib: volume module support"
            "libmpdclient: mpd module support"
            "wireless_tools: network module support"
            "libsigc++: i3 module support"
            "i3ipc-glib-git: i3 module support")
makedepends=("cmake" "pkg-config" "clang" "glibc" "boost")
conflicts=("lemonbuddy-git")
source=("${pkgname}::git+${url}.git#tag=${pkgver}")
md5sums=("SKIP")

prepare() {
  cd "$pkgname" || exit
  git submodule update --init --recursive
  mkdir build
}

build() {
  cd "${pkgname}/build" || exit
  cmake -DCMAKE_INSTALL_PREFIX=/usr ..
  make
}

package() {
  cd "${pkgname}/build" || exit
  make DESTDIR="${pkgdir}/" install
  cd .. || exit
  install -D -m644 LICENSE "${pkgdir}/usr/share/licenses/${pkgname}/LICENSE"
}
