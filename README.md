# Polybar

[![Build Status](https://travis-ci.org/jaagr/polybar.svg?branch=master)](https://travis-ci.org/jaagr/polybar)
[![MIT License](https://img.shields.io/github/license/mashape/apistatus.svg?maxAge=2592000?style=plastic)](https://github.com/jaagr/polybar/blob/master/LICENSE)

A fast and easy-to-use tool for creating status bars.

**Polybar** aims to help users build beautiful and highly customizable status bars
for their desktop environment, without the need of having a black belt in shell scripting.
Heres a few screenshots showing you what it can look like:

[![sample screenshot](http://i.imgur.com/xvlw9iHt.png)](http://i.imgur.com/xvlw9iH.png)
[![sample screenshot](http://i.imgur.com/cYQOuRrt.png)](http://i.imgur.com/cYQOuRr.png)
[![sample screenshot](http://i.imgur.com/A6spiZZt.png)](http://i.imgur.com/A6spiZZ.png)
[![sample screenshot](http://i.imgur.com/TY5a5r9t.png)](http://i.imgur.com/TY5a5r9.png)

Please note that the project still is in early development, so please report any
problems by [creating an issue ticket](https://github.com/jaagr/polybar/issues/new).


## Table of Contents

* [Introduction](#introduction)
* [Getting started](#getting-started)
  * [Dependencies](#dependencies)
  * [Building from source](#building-from-source)
  * [Configuration](#configuration)
  * [Running](#running)
* [License](#license)


## Introduction

The main purpose of **Polybar** is to help users create awesome status bars.
It has built-in functionality to generate content for the most commonly used widgets, such as:

- Window title
- Playback controls and status display for [MPD](https://www.musicpd.org/) using [libmpdclient](https://www.musicpd.org/libs/libmpdclient/)
- [ALSA](http://www.alsa-project.org/main/index.php/Main_Page) volume controls
- Workspace and desktop panel for [bspwm](https://github.com/baskerville/bspwm) and [i3](https://github.com/i3/i3)
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

[See the wiki for more details](https://github.com/jaagr/polybar/wiki).


## Getting started

If you are using **Arch Linux**, you can install the AUR package [polybar-git](https://aur.archlinux.org/packages/polybar-git/) to get the latest version, or
[polybar](https://aur.archlinux.org/packages/polybar/) for the latest stable release. If you are using **Void Linux**
you can install the package [polybar](https://github.com/voidlinux/void-packages/blob/master/srcpkgs/polybar/template) available in the official xbps repository.

If you create a package for any other distribution, please consider contributing the template.


### Dependencies

A compiler with C++14 support ([clang-3.4+](http://llvm.org/releases/download.html), [gcc-5.1+](https://gcc.gnu.org/releases.html)).
- cmake
- boost
- xcb-util-wm
- xcb-util-image
- libXft
- python2

Optional dependencies for module support:

- jsoncpp (required by `internal/i3`)
  - Because some distributions only offer pre-historic releases of jsoncpp,
    a local copy of the library source (jsoncpp-1.7.7) will be built and linked
    statically as a fallback if a valid version is not found.
- wireless_tools (required by `internal/network`)
- alsa-lib (required by `internal/volume`)
- libmpdclient (required by `internal/mpd`)

~~~ sh
# required
$ apt-get install cmake cmake-data libboost-dev libfontconfig1-dev libfreetype6-dev libghc-x11-xft-dev libx11-xcb-dev libxcb1-dev libxcb-ewmh-dev libxcb-icccm4-dev libxcb-image0-dev libxcb-randr0-dev libxcb-util0-dev libxcb-xkb-dev pkg-config python-xcbgen xcb-proto
# optional
$ apt-get install i3-wm libasound2-dev libmpdclient-dev libiw-dev
~~~


### Building from source

Please [report any problems](https://github.com/jaagr/polybar/issues/new) you run into when building the project.

  ~~~ sh
  $ git clone --branch 2.3.12 --recursive https://github.com/jaagr/polybar
  $ mkdir polybar/build
  $ cd polybar/build
  $ cmake -DCMAKE_BUILD_TYPE=Release ..
  $ sudo make install
  ~~~

There's also a helper script available in the root folder:

  ~~~ sh
  $ ./build.sh
  ~~~


### Configuration

Details on how to setup and configure the bar and each module have been moved to [the wiki](https://github.com/jaagr/polybar/wiki/Configuration).

  ~~~ sh
  # Install the example configuration
  $ make userconfig
  # Launch the example bar
  $ polybar example
  ~~~

**NOTE:** If the bar output looks odd, it's probably because you're
missing the fonts defined in the config. Update the config or install the
missing fonts.


### Running

[See the wiki for details on how to run polybar](https://github.com/jaagr/polybar/wiki).


## License

Polybar is licensed under the MIT license. [See LICENSE for more information](https://github.com/jaagr/polybar/blob/master/LICENSE).
