# Maintainer: Patrick Ziegler <p.ziegler96@gmail.com>
pkgname=polybar
pkgver=3.5.5
pkgrel=2
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
makedepends=("cmake" "python" "pkg-config" "python-sphinx" "python-packaging" "i3-wm")
conflicts=("polybar-git")
install="${pkgname}.install"
source=(${url}/releases/download/${pkgver}/${pkgname}-${pkgver}.tar.gz)
sha256sums=('7e625d3b6f7885587e70200fd81c2a5d3fb03f5649422de8e138747152ca0bb1')
_dir="${pkgname}-${pkgver}"

prepare() {
  mkdir -p "${_dir}/build"
}

build() {
  cd "${_dir}/build" || exit 1
  # Force cmake to use system python (to detect xcbgen)
  # We need to turn off _GLIBCXX_ASSERTIONS because of a bug in polybar:
  # https://github.com/polybar/polybar/issues/2416
  cmake -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-U_GLIBCXX_ASSERTIONS" -DPYTHON_EXECUTABLE=/usr/bin/python3 ..
  cmake --build .
}

package() {
  cmake --build "${_dir}/build" --target install -- DESTDIR="${pkgdir}"
  install -Dm644 "${_dir}/LICENSE" "${pkgdir}/usr/share/licenses/${pkgname}/LICENSE"
}
