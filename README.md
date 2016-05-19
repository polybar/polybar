Lemonbuddy
==========

A fast and easy-to-use tool for [Lemonbar](https://github.com/LemonBoy/bar/).

**Lemonbuddy** aims to help users' build beautiful and highly customizable status bars
without messing with named pipes, MacGyver-like scripting or non-blocking
loops lobotomizing your CPU.

Please note that the project hasn't been tested, other then by myself, so
bugs and various bumps is to be expected. Please report any issues here on
github.


## Installation

Package builds will be released for **AUR** and **XBPS** which will make it
alot easier to install. Until then you could build it from source.

### Dependencies:

- boost-libs
- libx11
- libxrandr
- wireless_tools
- If building with **ENABLE_ALSA** (default)
  - alsa-lib
- If building with **ENABLE_MPD** (default)
  - libmpdclient
- If building with **ENABLE_I3**
  - libsigc++
  - i3-wm

**With Pacman you can install the packages using:**
~~~ sh
$ pacman -S boost-libs libx11 libxrandr wireless_tools alsa-lib libmpdclient libsigc++ i3-wm
$ "i3ipc-glib-git is located in the AUR"
~~~

**With XBPS you can install the packages using:**
~~~ sh
$ xbps-install -S alsa-lib-devel boost-devel i3-devel i3ipc-glib-devel libX11-devel libXrandr-devel libmpdclient-devel libsigc++-devel wireless_tools-devel
~~~~

<br>

## Building from source

#### Automatic installation using ./build.sh

If you haven't worked with builds before you could try to run the following
command chain:

~~~ sh
$ git clone --branch %VERSION% --recursive https://github.com/jaagr/lemonbuddy.git
$ cd lemonbuddy
$ ./build.sh
~~~

NOTE: **git-perl** is required for submodules to work in **Void Linux**

---

#### It is of course recommended that you control the build process yourself.

  ~~~ sh
  $ git clone --branch %VERSION% --recursive https://github.com/jaagr/lemonbuddy.git
  $ mkdir lemonbuddy/build
  $ cd lemonbuddy/build
  $ cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr ..
  # List and edit build variables
  $ make edit_cache
  $ sudo make install
  ~~~
---

<br>

## License

> The MIT License (MIT)
> Copyright (c) 2016 Michael Carlberg
>
> Permission is hereby granted, free of charge, to any person obtaining a copy of
> this software and associated documentation files (the "Software"), to deal in
> the Software without restriction, including without limitation the rights to
> use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
> the Software, and to permit persons to whom the Software is furnished to do so,
> subject to the following conditions:
>
> The above copyright notice and this permission notice shall be included in all
> copies or substantial portions of the Software.
>
> THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
> IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
> FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
> COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
> IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
> CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
