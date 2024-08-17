# Maintainer: Patrick Ziegler <p.ziegler96@gmail.com>
_pkgname=polybar
pkgname="${_pkgname}-git"
pkgver=3.7.2
pkgrel=1
pkgdesc="A fast and easy-to-use status bar"
# aarch64 is not officially supported by polybar, it is only listed here for convenience
arch=("i686" "x86_64" "aarch64")
url="https://github.com/polybar/polybar"
license=("MIT")
depends=("libuv" "cairo" "xcb-util-image" "xcb-util-wm" "xcb-util-xrm"
         "xcb-util-cursor" "alsa-lib" "libpulse" "libmpdclient" "libnl"
         "jsoncpp" "curl")
optdepends=("i3-wm: i3 module support")
makedepends=("cmake" "git" "python" "pkg-config" "python-sphinx"
             "python-packaging" "i3-wm")
backup=("etc/polybar/config.ini")
provides=("polybar")
conflicts=("polybar")
source=("${_pkgname}::git+${url}.git")
sha256sums=("SKIP")

pkgver() {
  git -C "${_pkgname}" describe --long --tags | sed "s/-/.r/;s/-/./g"
}

prepare() {
  git -C "${_pkgname}" submodule update --init --recursive
  mkdir -p "${_pkgname}/build"
}

build() {
  cd "${_pkgname}/build" || exit 1
  # Force cmake to use system python (to detect xcbgen)
  cmake -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Release -DPYTHON_EXECUTABLE=/usr/bin/python3 ..
  cmake --build .
}

package() {
  cmake --build "${_pkgname}/build" --target install -- DESTDIR="${pkgdir}"
  install -Dm644 "${_pkgname}/LICENSE" "${pkgdir}/usr/share/licenses/${_pkgname}/LICENSE"
}
