# Maintainer: Patrick Ziegler <p.ziegler96@gmail.com>
pkgname=polybar
pkgver=3.5.2
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
source=(${url}/releases/download/${pkgver}/${pkgname}-${pkgver}.tar.gz)
sha256sums=('e411d9c091d0d4b5e8fbb44969e45c537d8c45900ecb3e90c49a248b885fe110')
_dir="${pkgname}-${pkgver}"

prepare() {
  mkdir -p "${_dir}/build"
}

build() {
  cd "${_dir}/build" || exit 1
  # Force cmake to use system python (to detect xcbgen)
  cmake -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Release -DPYTHON_EXECUTABLE=/usr/bin/python3 ..
  cmake --build .
}

package() {
  cmake --build "${_dir}/build" --target install -- DESTDIR="${pkgdir}"
  install -Dm644 "${_dir}/LICENSE" "${pkgdir}/usr/share/licenses/${pkgname}/LICENSE"
}
