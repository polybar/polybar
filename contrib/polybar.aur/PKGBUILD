# Maintainer: Patrick Ziegler <p.ziegler96@gmail.com>
pkgname=polybar
pkgver=3.4.1
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
makedepends=("cmake" "git" "python" "pkg-config" "xorgproto" "python-sphinx" "i3-wm")
conflicts=("polybar-git")
install="${pkgname}.install"
source=(${url}/releases/download/${pkgver}/polybar-${pkgver}.tar)
sha256sums=('9e37fa48a1027881f14546f2b4f6ace4c91d09a20a293685f845da9cbaedc4eb')

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
