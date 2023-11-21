<p align="center">
  <img src="doc/_static/banner.png#gh-light-mode-only" alt="Polybar">
  <img src="doc/_static/banner-dark-mode.png#gh-dark-mode-only" alt="Polybar">
</p>

<p align="center">
A fast and easy-to-use tool for creating status bars.
</p>

<p align="center">
<a href="https://github.com/polybar/polybar/releases"><img src="https://img.shields.io/github/release/polybar/polybar.svg"></a>
<a href="https://github.com/polybar/polybar/releases"><img alt="GitHub All Releases" src="https://img.shields.io/github/downloads/polybar/polybar/total" /></a>
<a href="https://github.com/polybar/polybar/actions?query=workflow%3ACI"><img src="https://github.com/polybar/polybar/workflows/CI/badge.svg"></a>
<a href="https://github.com/polybar/polybar/actions?query=workflow%3A%22Release+Workflow%22"><img src="https://github.com/polybar/polybar/workflows/Release%20Workflow/badge.svg"></a>
<a href="https://polybar.readthedocs.io"><img src="https://readthedocs.org/projects/polybar/badge/?version=latest"></a>
<a href="https://gitter.im/polybar/polybar"><img src="https://badges.gitter.im/polybar/polybar.svg"></a>
<a href="https://codecov.io/gh/polybar/polybar/branch/master"><img src="https://codecov.io/gh/polybar/polybar/branch/master/graph/badge.svg"></a>
<a href="https://github.com/polybar/polybar/blob/master/LICENSE"><img src="https://img.shields.io/github/license/polybar/polybar.svg"></a>
<a href="https://www.codetriage.com/polybar/polybar"><img src="https://www.codetriage.com/polybar/polybar/badges/users.svg"></a>
<a href="https://opencollective.com/polybar"><img src="https://opencollective.com/polybar/tiers/badge.svg"></a>
</p>

**[Documentation](https://github.com/polybar/polybar/wiki/) | [Installation](#installation) | [Support](SUPPORT.md) | [Donate](#donations)**

**Polybar** aims to help users build beautiful and highly customizable status bars
for their desktop environment, without the need of having a black belt in shell scripting.

![default configuration screenshot](doc/_static/default.png)

## Table of Contents

* [Introduction](#introduction)
* [Getting Help](#getting-help)
* [Contributing](#contributing)
* [Getting started](#getting-started)
  * [Installation](#installation)
  * [First Steps](#first-steps)
* [Community](#community)
* [Contributors](#contributors)
* [Donations](#donations)
  * [Sponsors](#sponsors)
  * [Backers](#backers)
* [License](#license)
* [Signatures](#signatures)

## Introduction

The main purpose of **Polybar** is to help users create awesome status bars.
It has built-in functionality to display information about the most commonly used services.
Some of the services included so far:

- Systray icons
- Window title
- Playback controls and status display for [MPD](https://www.musicpd.org/) using [libmpdclient](https://www.musicpd.org/libs/libmpdclient/)
- [ALSA](https://www.alsa-project.org/main/index.php/Main_Page) and [PulseAudio](https://www.freedesktop.org/wiki/Software/PulseAudio/) volume controls
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

## Contributing

Read our [contributing guidelines](CONTRIBUTING.md) for how to get started with contributing to polybar.

## Getting started

### Installation

<a href="https://repology.org/metapackage/polybar">
    <img src="https://repology.org/badge/vertical-allrepos/polybar.svg" alt="Packaging status" align="right">
</a>

Polybar is already available in the package manager for many repositories.
We list some of the more prominent ones here.
Also click the [image on the
right](https://repology.org/project/polybar/versions) to see a more complete
list of available polybar packages.

If you are using **Debian** (bullseye/11/stable) or later, you can install [polybar](https://tracker.debian.org/pkg/polybar)
using `sudo apt install polybar`. Newer releases of polybar are sometimes provided in the [backports](https://wiki.debian.org/Backports)
repository for stable users, you need to enable [backports](https://wiki.debian.org/Backports) and then install using
`sudo apt -t bullseye-backports install polybar`.

If you are using **Ubuntu** 20.10 (Groovy Gorilla) or later, you can install polybar
using `sudo apt install polybar`.

If you are using **Arch Linux**, you can install
[polybar](https://archlinux.org/packages/extra/x86_64/polybar/) to get the
latest stable release using `sudo pacman -S polybar`. The latest unstable
changes are also available in the
[`polybar-git`](https://aur.archlinux.org/packages/polybar-git) package in the
AUR.

If you are using **Manjaro**, you can install [polybar](https://software.manjaro.org/package/polybar) to get the latest stable release using `sudo pacman -S polybar`.

If you are using **Void Linux**, you can install [polybar](https://github.com/void-linux/void-packages/blob/master/srcpkgs/polybar/template) using `xbps-install -S polybar`.

If you are using **NixOS**, polybar is available in both the stable and unstable channels and can be installed with the command `nix-env -iA nixos.polybar`.

If you are using **Slackware**, polybar is available from the [SlackBuilds](https://slackbuilds.org/repository/14.2/desktop/polybar/) repository.

If you are using **Source Mage GNU/Linux**, polybar spell is available in test grimoire and can be installed via `cast polybar`.

If you are using **openSUSE Leap** or **openSUSE Tumbleweed**, polybar is available from the
[official
repositories](https://build.opensuse.org/package/show/X11:Utilities/polybar)
and can be installed via `zypper install polybar`.
The package is available for openSUSE Leap 15.3 and above.

If you are using **FreeBSD**, [polybar](https://www.freshports.org/x11/polybar) can be installed using `pkg install polybar`. Make sure you are using the `latest` package branch.

If you are using **Gentoo**, both release and git-master versions are available in the [main](https://packages.gentoo.org/packages/x11-misc/polybar) repository.

If you are using **Fedora**, you can install [polybar](https://src.fedoraproject.org/rpms/polybar) using `sudo dnf install polybar`.

If you can't find your distro here, you will have to [build from source](https://github.com/polybar/polybar/wiki/Compiling).

### First Steps
[See the wiki for details on how to run and configure polybar](https://github.com/polybar/polybar/wiki).

## Community
Want to get in touch?

* Visit our [Discussion page](https://github.com/polybar/polybar/discussions)
* Join our Gitter room at [`gitter.im/polybar/polybar`](https://gitter.im/polybar/polybar)
* We have our own subreddit at [`r/polybar`](https://www.reddit.com/r/polybar)
* Chat with us in the `#polybar` IRC channel on the [`irc.libera.chat:6697`](https://libera.chat/) server

## Contributors

### Maintainers
* Patrick Ziegler [**@patrick96**](https://github.com/patrick96)

### Owner
* Michael Carlberg [**@jaagr**](https://github.com/jaagr/)

### Former Maintainers
* [**@Lomadriel**](https://github.com/Lomadriel)
* [**@NBonaparte**](https://github.com/NBonaparte)
* Chase Geigle [**@skystrife**](https://github.com/skystrife)

### Logo Design by
* [**@Tobaloidee**](https://github.com/Tobaloidee)


### [All Contributors](https://github.com/polybar/polybar/graphs/contributors)

## Donations

Polybar accepts donations through [open collective](https://opencollective.com/polybar).

[Become a backer](https://opencollective.com/polybar) and support polybar!
### Sponsors

<a href="https://opencollective.com/polybar/sponsor/0/website?requireActive=false" target="_blank"><img src="https://opencollective.com/polybar/sponsor/0/avatar.svg?requireActive=false"></a>
<a href="https://opencollective.com/polybar/sponsor/1/website?requireActive=false" target="_blank"><img src="https://opencollective.com/polybar/sponsor/1/avatar.svg?requireActive=false"></a>
<a href="https://opencollective.com/polybar/sponsor/2/website?requireActive=false" target="_blank"><img src="https://opencollective.com/polybar/sponsor/2/avatar.svg?requireActive=false"></a>
<a href="https://opencollective.com/polybar/sponsor/3/website?requireActive=false" target="_blank"><img src="https://opencollective.com/polybar/sponsor/3/avatar.svg?requireActive=false"></a>
<a href="https://opencollective.com/polybar/sponsor/4/website?requireActive=false" target="_blank"><img src="https://opencollective.com/polybar/sponsor/4/avatar.svg?requireActive=false"></a>
<a href="https://opencollective.com/polybar/sponsor/5/website?requireActive=false" target="_blank"><img src="https://opencollective.com/polybar/sponsor/5/avatar.svg?requireActive=false"></a>
<a href="https://opencollective.com/polybar/sponsor/6/website?requireActive=false" target="_blank"><img src="https://opencollective.com/polybar/sponsor/6/avatar.svg?requireActive=false"></a>
<a href="https://opencollective.com/polybar/sponsor/7/website?requireActive=false" target="_blank"><img src="https://opencollective.com/polybar/sponsor/7/avatar.svg?requireActive=false"></a>
<a href="https://opencollective.com/polybar/sponsor/8/website?requireActive=false" target="_blank"><img src="https://opencollective.com/polybar/sponsor/8/avatar.svg?requireActive=false"></a>
<a href="https://opencollective.com/polybar/sponsor/9/website?requireActive=false" target="_blank"><img src="https://opencollective.com/polybar/sponsor/9/avatar.svg?requireActive=false"></a>
<a href="https://opencollective.com/polybar/sponsor/10/website?requireActive=false" target="_blank"><img src="https://opencollective.com/polybar/sponsor/10/avatar.svg?requireActive=false"></a>
<a href="https://opencollective.com/polybar/sponsor/11/website?requireActive=false" target="_blank"><img src="https://opencollective.com/polybar/sponsor/11/avatar.svg?requireActive=false"></a>
<a href="https://opencollective.com/polybar/sponsor/12/website?requireActive=false" target="_blank"><img src="https://opencollective.com/polybar/sponsor/12/avatar.svg?requireActive=false"></a>
<a href="https://opencollective.com/polybar/sponsor/13/website?requireActive=false" target="_blank"><img src="https://opencollective.com/polybar/sponsor/13/avatar.svg?requireActive=false"></a>
<a href="https://opencollective.com/polybar/sponsor/14/website?requireActive=false" target="_blank"><img src="https://opencollective.com/polybar/sponsor/14/avatar.svg?requireActive=false"></a>
<a href="https://opencollective.com/polybar/sponsor/15/website?requireActive=false" target="_blank"><img src="https://opencollective.com/polybar/sponsor/15/avatar.svg?requireActive=false"></a>
<a href="https://opencollective.com/polybar/sponsor/16/website?requireActive=false" target="_blank"><img src="https://opencollective.com/polybar/sponsor/16/avatar.svg?requireActive=false"></a>
<a href="https://opencollective.com/polybar/sponsor/17/website?requireActive=false" target="_blank"><img src="https://opencollective.com/polybar/sponsor/17/avatar.svg?requireActive=false"></a>
<a href="https://opencollective.com/polybar/sponsor/18/website?requireActive=false" target="_blank"><img src="https://opencollective.com/polybar/sponsor/18/avatar.svg?requireActive=false"></a>
<a href="https://opencollective.com/polybar/sponsor/19/website?requireActive=false" target="_blank"><img src="https://opencollective.com/polybar/sponsor/19/avatar.svg?requireActive=false"></a>
<a href="https://opencollective.com/polybar/sponsor/20/website?requireActive=false" target="_blank"><img src="https://opencollective.com/polybar/sponsor/20/avatar.svg?requireActive=false"></a>
<a href="https://opencollective.com/polybar/sponsor/21/website?requireActive=false" target="_blank"><img src="https://opencollective.com/polybar/sponsor/21/avatar.svg?requireActive=false"></a>
<a href="https://opencollective.com/polybar/sponsor/22/website?requireActive=false" target="_blank"><img src="https://opencollective.com/polybar/sponsor/22/avatar.svg?requireActive=false"></a>
<a href="https://opencollective.com/polybar/sponsor/23/website?requireActive=false" target="_blank"><img src="https://opencollective.com/polybar/sponsor/23/avatar.svg?requireActive=false"></a>
<a href="https://opencollective.com/polybar/sponsor/24/website?requireActive=false" target="_blank"><img src="https://opencollective.com/polybar/sponsor/24/avatar.svg?requireActive=false"></a>
<a href="https://opencollective.com/polybar/sponsor/25/website?requireActive=false" target="_blank"><img src="https://opencollective.com/polybar/sponsor/25/avatar.svg?requireActive=false"></a>
<a href="https://opencollective.com/polybar/sponsor/26/website?requireActive=false" target="_blank"><img src="https://opencollective.com/polybar/sponsor/26/avatar.svg?requireActive=false"></a>
<a href="https://opencollective.com/polybar/sponsor/27/website?requireActive=false" target="_blank"><img src="https://opencollective.com/polybar/sponsor/27/avatar.svg?requireActive=false"></a>
<a href="https://opencollective.com/polybar/sponsor/28/website?requireActive=false" target="_blank"><img src="https://opencollective.com/polybar/sponsor/28/avatar.svg?requireActive=false"></a>
<a href="https://opencollective.com/polybar/sponsor/29/website?requireActive=false" target="_blank"><img src="https://opencollective.com/polybar/sponsor/29/avatar.svg?requireActive=false"></a>

### Backers

<a href="https://opencollective.com/polybar/backer/0/website?requireActive=false" target="_blank"><img src="https://opencollective.com/polybar/backer/0/avatar.svg?requireActive=false"></a>
<a href="https://opencollective.com/polybar/backer/1/website?requireActive=false" target="_blank"><img src="https://opencollective.com/polybar/backer/1/avatar.svg?requireActive=false"></a>
<a href="https://opencollective.com/polybar/backer/2/website?requireActive=false" target="_blank"><img src="https://opencollective.com/polybar/backer/2/avatar.svg?requireActive=false"></a>
<a href="https://opencollective.com/polybar/backer/3/website?requireActive=false" target="_blank"><img src="https://opencollective.com/polybar/backer/3/avatar.svg?requireActive=false"></a>
<a href="https://opencollective.com/polybar/backer/4/website?requireActive=false" target="_blank"><img src="https://opencollective.com/polybar/backer/4/avatar.svg?requireActive=false"></a>
<a href="https://opencollective.com/polybar/backer/5/website?requireActive=false" target="_blank"><img src="https://opencollective.com/polybar/backer/5/avatar.svg?requireActive=false"></a>
<a href="https://opencollective.com/polybar/backer/6/website?requireActive=false" target="_blank"><img src="https://opencollective.com/polybar/backer/6/avatar.svg?requireActive=false"></a>
<a href="https://opencollective.com/polybar/backer/7/website?requireActive=false" target="_blank"><img src="https://opencollective.com/polybar/backer/7/avatar.svg?requireActive=false"></a>
<a href="https://opencollective.com/polybar/backer/8/website?requireActive=false" target="_blank"><img src="https://opencollective.com/polybar/backer/8/avatar.svg?requireActive=false"></a>
<a href="https://opencollective.com/polybar/backer/9/website?requireActive=false" target="_blank"><img src="https://opencollective.com/polybar/backer/9/avatar.svg?requireActive=false"></a>
<a href="https://opencollective.com/polybar/backer/10/website?requireActive=false" target="_blank"><img src="https://opencollective.com/polybar/backer/10/avatar.svg?requireActive=false"></a>
<a href="https://opencollective.com/polybar/backer/11/website?requireActive=false" target="_blank"><img src="https://opencollective.com/polybar/backer/11/avatar.svg?requireActive=false"></a>
<a href="https://opencollective.com/polybar/backer/12/website?requireActive=false" target="_blank"><img src="https://opencollective.com/polybar/backer/12/avatar.svg?requireActive=false"></a>
<a href="https://opencollective.com/polybar/backer/13/website?requireActive=false" target="_blank"><img src="https://opencollective.com/polybar/backer/13/avatar.svg?requireActive=false"></a>
<a href="https://opencollective.com/polybar/backer/14/website?requireActive=false" target="_blank"><img src="https://opencollective.com/polybar/backer/14/avatar.svg?requireActive=false"></a>
<a href="https://opencollective.com/polybar/backer/15/website?requireActive=false" target="_blank"><img src="https://opencollective.com/polybar/backer/15/avatar.svg?requireActive=false"></a>
<a href="https://opencollective.com/polybar/backer/16/website?requireActive=false" target="_blank"><img src="https://opencollective.com/polybar/backer/16/avatar.svg?requireActive=false"></a>
<a href="https://opencollective.com/polybar/backer/17/website?requireActive=false" target="_blank"><img src="https://opencollective.com/polybar/backer/17/avatar.svg?requireActive=false"></a>
<a href="https://opencollective.com/polybar/backer/18/website?requireActive=false" target="_blank"><img src="https://opencollective.com/polybar/backer/18/avatar.svg?requireActive=false"></a>
<a href="https://opencollective.com/polybar/backer/19/website?requireActive=false" target="_blank"><img src="https://opencollective.com/polybar/backer/19/avatar.svg?requireActive=false"></a>
<a href="https://opencollective.com/polybar/backer/20/website?requireActive=false" target="_blank"><img src="https://opencollective.com/polybar/backer/20/avatar.svg?requireActive=false"></a>
<a href="https://opencollective.com/polybar/backer/21/website?requireActive=false" target="_blank"><img src="https://opencollective.com/polybar/backer/21/avatar.svg?requireActive=false"></a>
<a href="https://opencollective.com/polybar/backer/22/website?requireActive=false" target="_blank"><img src="https://opencollective.com/polybar/backer/22/avatar.svg?requireActive=false"></a>
<a href="https://opencollective.com/polybar/backer/23/website?requireActive=false" target="_blank"><img src="https://opencollective.com/polybar/backer/23/avatar.svg?requireActive=false"></a>
<a href="https://opencollective.com/polybar/backer/24/website?requireActive=false" target="_blank"><img src="https://opencollective.com/polybar/backer/24/avatar.svg?requireActive=false"></a>
<a href="https://opencollective.com/polybar/backer/25/website?requireActive=false" target="_blank"><img src="https://opencollective.com/polybar/backer/25/avatar.svg?requireActive=false"></a>
<a href="https://opencollective.com/polybar/backer/26/website?requireActive=false" target="_blank"><img src="https://opencollective.com/polybar/backer/26/avatar.svg?requireActive=false"></a>
<a href="https://opencollective.com/polybar/backer/27/website?requireActive=false" target="_blank"><img src="https://opencollective.com/polybar/backer/27/avatar.svg?requireActive=false"></a>
<a href="https://opencollective.com/polybar/backer/28/website?requireActive=false" target="_blank"><img src="https://opencollective.com/polybar/backer/28/avatar.svg?requireActive=false"></a>
<a href="https://opencollective.com/polybar/backer/29/website?requireActive=false" target="_blank"><img src="https://opencollective.com/polybar/backer/29/avatar.svg?requireActive=false"></a>

## License

Polybar is licensed under the MIT license. [See LICENSE for more information](https://github.com/polybar/polybar/blob/master/LICENSE).

## Signatures

Release archives and tags are signed by a maintainer using GPG. Currently
everything is signed by [Patrick Ziegler](https://www.patrickziegler.ch/gpg)
with fingerprint `1D5791352D51A228D4DDDBA4521E5E03AEBCA1A7`
