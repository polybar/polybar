# Maintainer: Michael Carlberg <c@rlberg.se>
# Contributor: Michael Carlberg <c@rlberg.se>
pkgname=polybar
pkgver=3.3.1
pkgrel=1
pkgdesc="A fast and easy-to-use status bar"
arch=("i686" "x86_64")
url="https://github.com/polybar/polybar"
license=("MIT")
depends=("cairo" "xcb-util-image" "xcb-util-wm" "xcb-util-xrm" "xcb-util-cursor")
optdepends=("alsa-lib: alsa module support"
            "pulseaudio: pulseaudio module support"
            "libmpdclient: mpd module support"
            "libnl: network module support"
            "wireless_tools: network module support (legacy)"
            "jsoncpp: i3 module support"
            "i3-wm: i3 module support"
            "ttf-unifont: Font used in example config"
            "siji-git: Font used in example config"
            "xorg-fonts-misc: Font used in example config"
            "curl: github module support")
makedepends=("cmake" "git" "python" "python2" "pkg-config")
conflicts=("polybar-git")
install="${pkgname}.install"
source=(${url}/releases/download/${pkgver}/polybar-${pkgver}.tar)
sha256sums=('b0982a729f99f658d77969fd582d074843a57982b5e7c6b2b6efee5480b2a81c')

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
