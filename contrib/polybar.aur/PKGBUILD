# Maintainer: Patrick Ziegler <p.ziegler96@gmail.com>
pkgname=polybar
pkgver=3.4.3
pkgrel=1
pkgdesc="A fast and easy-to-use status bar"
arch=("i686" "x86_64")
url="https://github.com/polybar/polybar"
license=("MIT")
depends=("cairo" "xcb-util-image" "xcb-util-wm" "xcb-util-xrm" "xcb-util-cursor"
         "alsa-lib" "libpulse" "libmpdclient" "libnl" "jsoncpp" "curl")
optdepends=("i3-wm: i3 module support"
            "ttf-unifont: Font used in example config"
            "siji-git: Font used in example config"
            "xorg-fonts-misc: Font used in example config")
makedepends=("cmake" "git" "python" "pkg-config" "python-sphinx" "i3-wm")
conflicts=("polybar-git")
install="${pkgname}.install"
source=(${url}/releases/download/${pkgver}/polybar-${pkgver}.tar)
sha256sums=('d4ed121c1d3960493f8268f966d65a94d94c4646a4abb131687e37b63616822f')

prepare() {
  mkdir -p "${pkgname}/build"
}

build() {
  cd "${pkgname}/build" || exit 1
  cmake -DCMAKE_INSTALL_PREFIX=/usr ..
  cmake --build .
}

package() {
  cmake --build "${pkgname}/build" --target install -- DESTDIR="${pkgdir}"
  install -Dm644 "${pkgname}/LICENSE" "${pkgdir}/usr/share/licenses/${pkgname}/LICENSE"
}
