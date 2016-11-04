# Lemonbuddy

[![Build Status](https://travis-ci.org/jaagr/lemonbuddy.svg?branch=master)](https://travis-ci.org/jaagr/lemonbuddy)
[![MIT License](https://img.shields.io/github/license/mashape/apistatus.svg?maxAge=2592000?style=plastic)](https://github.com/jaagr/lemonbuddy/blob/master/LICENSE)

A fast and easy-to-use tool for creating status bars.

**Lemonbuddy** aims to help users build beautiful and highly customizable status bars
for their desktop environment, without the need of having a black belt in shell scripting.
Heres a few screenshots showing you what it can look like:

[![sample screenshot](http://i.imgur.com/xvlw9iHt.png)](http://i.imgur.com/xvlw9iH.png)
[![sample screenshot](http://i.imgur.com/cYQOuRrt.png)](http://i.imgur.com/cYQOuRr.png)
[![sample screenshot](http://i.imgur.com/A6spiZZt.png)](http://i.imgur.com/A6spiZZ.png)
[![sample screenshot](http://i.imgur.com/TY5a5r9t.png)](http://i.imgur.com/TY5a5r9.png)

Please note that the project still is in early development, so please report any
problems by [creating an issue ticket](https://github.com/jaagr/lemonbuddy/issues/new).


## Table of Contents

* [Introduction](#introduction)
* [Getting started](#getting-started)
  * [Dependencies](#dependencies)
  * [Building from source](#building-from-source)
  * [Configuration](#configuration)
  * [Running](#running)
* [License](#license)


## Introduction

The main purpose of **Lemonbuddy** is to help users create awesome status bars.
It has built-in functionality to generate content for the most commonly used widgets, such as:

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
- And more...

Each bar contains a set of modules, which in turn defines a set of formatting rules and options.
Read more about [how the configuration works](#configuration).

## Getting started

If you are using **Arch Linux**, you can install the AUR package [lemonbuddy-git](https://aur.archlinux.org/packages/lemonbuddy-git/) to get the latest version, or
[lemonbuddy](https://aur.archlinux.org/packages/lemonbuddy/) for the latest stable release.

If you create a package for any other distribution, please consider contributing the template so that we can make the application
available for more people.


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
$ pacman -S cmake python2 boost xcb-util-image xcb-util-wm libxft wireless_tools alsa-lib libmpdclient
$ apt-get install cmake cmake-data libboost-dev libfreetype6-dev libxcb1-dev libx11-xcb-dev libxcb-util0-dev libxcb-image0-dev libxcb-randr0-dev libxcb-ewmh-dev libxcb-icccm4-dev xcb-proto python-xcbgen i3-wm libiw-dev libasound2-dev libmpdclient-dev
~~~


### Building from source

Please [report any problems](https://github.com/jaagr/lemonbuddy/issues/new) you run into when building the project.

  ~~~ sh
  $ git clone --branch 2.2.2 --recursive https://github.com/jaagr/lemonbuddy
  $ mkdir lemonbuddy/build
  $ cd lemonbuddy/build
  $ cmake -DCMAKE_BUILD_TYPE=Release ..
  $ sudo make install
  ~~~


### Configuration

*Details on how to setup and configure the bar and each module have been moved to [the wiki](https://github.com/jaagr/lemonbuddy/wiki/Configuration).*

Before customizing the bar, make sure everything works as expected by trying
out one of the example configurations installed with the application.
The following code will get you started:

  ~~~ sh
  # Create the config root directory
  $ mkdir -p ${XDG_CONFIG_HOME:-$HOME/.config}/lemonbuddy
  $ cd ${XDG_CONFIG_HOME:-$HOME/.config}/lemonbuddy

  # Copy sample config for the running wm (uses a wm agnostic config as fallback)
  $ __wm=$(pgrep -l -x "(bspwm|i3)"); __prefix=$(which lemonbuddy)
  $ cp "${__prefix%%/bin*}/share/examples/lemonbuddy/config${__wm:+.${__wm##* }}" config

  # Launch the bar
  # (where "example" is the name of the bar as defined by [bar/NAME] in the config)
  $ lemonbuddy example
  ~~~

**NOTE:** If the bar output looks odd, it's probably because you're
missing he fonts defined in the config. Update the config or install the
missing fonts.


### Running

See the wiki page on [how to launch the bar when starting your WM](https://github.com/jaagr/lemonbuddy/wiki/Running-the-app).


## License

Lemonbuddy is licensed under the MIT license. [See LICENSE for more information](https://github.com/jaagr/lemonbuddy/blob/master/LICENSE).
