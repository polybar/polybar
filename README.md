<p align="center">
	<img src="banner.png" alt="Polybar">
</p>

<p align="center">
A fast and easy-to-use tool for creating status bars.
</p>

<p align="center">
<a href="https://github.com/polybar/polybar/releases"><img src="https://img.shields.io/github/release/polybar/polybar.svg"></a>
<a href="https://travis-ci.com/polybar/polybar"><img src="https://travis-ci.com/polybar/polybar.svg?branch=master"></a>
<a href="https://polybar.readthedocs.io"><img src="https://readthedocs.org/projects/polybar/badge/?version=latest"></a>
<a href="https://codecov.io/gh/polybar/polybar/branch/master"><img src="https://codecov.io/gh/polybar/polybar/branch/master/graph/badge.svg"></a>
<a href="https://github.com/polybar/polybar/blob/master/LICENSE"><img src="https://img.shields.io/github/license/polybar/polybar.svg"></a>
</p>

**Polybar** aims to help users build beautiful and highly customizable status bars
for their desktop environment, without the need of having a black belt in shell scripting.
Here are a few screenshots showing you what it can look like:

[![sample screenshot](http://i.imgur.com/xvlw9iHt.png)](http://i.imgur.com/xvlw9iH.png)
[![sample screenshot](http://i.imgur.com/cYQOuRrt.png)](http://i.imgur.com/cYQOuRr.png)
[![sample screenshot](http://i.imgur.com/A6spiZZt.png)](http://i.imgur.com/A6spiZZ.png)
[![sample screenshot](http://i.imgur.com/TY5a5r9t.png)](http://i.imgur.com/TY5a5r9.png)

You can find polybar configs for these example images (and other configs) [here](https://github.com/jaagr/dots/tree/master/.local/etc/themer/themes).


**If you need help**, check out the [Support](SUPPORT.md) page.

Please report any bugs you find by [creating an issue ticket](https://github.com/polybar/polybar/issues/new/choose) here on GitHub.
Make sure you include steps on how to reproduce it.


## Table of Contents

* [Introduction](#introduction)
* [Getting Help](#getting-help)
* [Getting started](#getting-started)
  * [Dependencies](#dependencies)
  * [Building from source](#building-from-source)
  * [Configuration](#configuration)
  * [Running](#running)
* [Community](#community)
* [Contributors](#contributors)
* [License](#license)


## Introduction

The main purpose of **Polybar** is to help users create awesome status bars.
It has built-in functionality to display information about the most commonly used services.
Some of the services included so far:

- Systray icons
- Window title
- Playback controls and status display for [MPD](https://www.musicpd.org/) using [libmpdclient](https://www.musicpd.org/libs/libmpdclient/)
- [ALSA](http://www.alsa-project.org/main/index.php/Main_Page) volume controls
- Workspace and desktop panel for [bspwm](https://github.com/baskerville/bspwm) and [i3](https://github.com/i3/i3)
- Workspace module for [EWMH compliant](https://specifications.freedesktop.org/wm-spec/wm-spec-1.3.html#idm140130320786080) window managers
- Keyboard layout and indicator status
- CPU and memory load indicator
- Battery display
- Network connection details
- Backlight level
- Date and time label
- Time-based shell script execution
- Command output tailing
- User-defined menu tree
- Inter-process messaging
- And more...

[See the wiki for more details](https://github.com/polybar/polybar/wiki).

## Getting Help

If you find yourself stuck, have a look at our [Support](SUPPORT.md) page for resources where you can find help.

## Getting started

<a href="https://repology.org/metapackage/polybar">
    <img src="https://repology.org/badge/vertical-allrepos/polybar.svg" alt="Packaging status" align="right">
</a>

Polybar was already packaged for the distros listed below.
If you can't find your distro here, you will have to [build from source](#building-from-source).

If you create a package for any other distribution, please consider contributing the template.

If you are using **Arch Linux**, you can install the AUR package [polybar-git](https://aur.archlinux.org/packages/polybar-git/) to get the latest version, or
[polybar](https://aur.archlinux.org/packages/polybar/) for the latest stable release.

If you are using **Void Linux**, you can install [polybar](https://github.com/void-linux/void-packages/blob/master/srcpkgs/polybar/template) using `xbps-install -S polybar`.

If you are using **NixOS**, polybar is available in both the stable and unstable channels and can be installed with the command `nix-env -iA nixos.polybar`.

If you are using **Slackware**, polybar is available from the [SlackBuilds](https://slackbuilds.org/repository/14.2/desktop/polybar/) repository.

If you are using **Source Mage GNU/Linux**, polybar spell is available in test grimoire and can be installed via `cast polybar`.

If you are using **openSUSE**, polybar is available from [OBS](https://build.opensuse.org/package/show/X11:Utilities/polybar/) repository. Package is available for openSUSE Leap 15 and Tumbleweed.

If you are using **FreeBSD**, [polybar](https://svnweb.freebsd.org/ports/head/x11/polybar/) can be installed using `pkg install polybar`. Make sure you are using the `latest` package branch.

If you are using **Gentoo**, both release and git-master versions are available in the [main](https://packages.gentoo.org/packages/x11-misc/polybar) repository.

### Dependencies

A compiler with C++14 support ([clang-3.4+](http://llvm.org/releases/download.html), [gcc-5.1+](https://gcc.gnu.org/releases.html)), [cmake 3.1+](https://cmake.org/download/), [git](https://git-scm.com/downloads)
- `cairo`
- `libxcb`
- `python`
- `xcb-proto`
- `xcb-util-image`
- `xcb-util-wm`

**Optional dependencies:**
- `xcb-util-cursor` *required for the `cursor-click` and `cursor-scroll` settings*
- `xcb-util-xrm` *required for accessing X resources with `${xrdb:...}`*

**Optional dependencies for extended module support:**
- `xcb-xkb` *required by `internal/xkeyboard`*
- `alsa-lib` *required by `internal/alsa`*
- `libpulse` *required by `internal/pulseaudio`*
- `i3-wm` *required by `internal/i3`*
- `jsoncpp` *required by `internal/i3`*
- `libmpdclient` *required by `internal/mpd`*
- `libcurl` *required by `internal/github`*
- `libnl-genl` or `wireless_tools` *required by `internal/network`*

Find a more complete list on the [dedicated wiki page](https://github.com/polybar/polybar/wiki/Compiling).


### Building from source

Please [report any problems](https://github.com/polybar/polybar/issues/new/choose) you run into when building the project.

Download the `polybar-<version>.tar` for the version you want to build from the
[release page](https://github.com/polybar/polybar/releases), extract it with
`tar xvf polybar-<version>.tar` and go into the extracted folder. There, run
the following commands:

```sh
$ mkdir build
$ cd build
$ cmake ..
$ make -j$(nproc)
$ sudo make install
```

There's also a helper script available in the root folder:

  ~~~ sh
  $ ./build.sh
  ~~~

For more info, have a look at the [Compiling wiki page](https://github.com/polybar/polybar/wiki/Compiling).

### Configuration

Details on how to setup and configure the bar and each module have been moved to [the wiki](https://github.com/polybar/polybar/wiki/Configuration).

#### Install the example configuration
Run the following inside the build directory:
~~~ sh
$ make userconfig
~~~
Or you can copy the example config from `/usr/share/doc/polybar/config` or ` /usr/local/share/doc/polybar/config` (depending on your install parameters)

#### Launch the example bar
  ~~~ sh
  $ polybar example
  ~~~

### Running

[See the wiki for details on how to run polybar](https://github.com/polybar/polybar/wiki).

## Community
Want to get in touch?

* We have our own subreddit at [r/polybar](https://www.reddit.com/r/polybar).
* Chat with us in the `#polybar` IRC channel on the `chat.freenode.net` server.

## Contributors

### Owner
* Michael Carlberg [**@jaagr**](http://github.com/jaagr/)

### Maintainers
* [**@NBonaparte**](https://github.com/NBonaparte)
* Chase Geigle [**@skystrife**](https://github.com/skystrife)
* Patrick Ziegler [**@patrick96**](https://github.com/patrick96)

### Logo Design by
* [**@Tobaloidee**](https://github.com/Tobaloidee)


### [All Contributors](https://github.com/polybar/polybar/graphs/contributors)

## License

Polybar is licensed under the MIT license. [See LICENSE for more information](https://github.com/polybar/polybar/blob/master/LICENSE).
