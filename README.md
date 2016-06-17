###### Did the bar stop working after your last update? [Read this](https://github.com/jaagr/lemonbuddy/wiki/pre-1.0:-Patching-your-configuration-file) to find out how to patch your config!
---

Lemonbuddy
==========

[![Build Status](https://travis-ci.org/jaagr/lemonbuddy.svg?branch=master)](https://travis-ci.org/jaagr/lemonbuddy)
[![MIT License](https://img.shields.io/github/license/mashape/apistatus.svg?maxAge=2592000?style=plastic)](https://github.com/jaagr/lemonbuddy/blob/master/LICENSE)

A fast and easy-to-use tool for [Lemonbar](https://github.com/LemonBoy/bar/).

**Lemonbuddy** aims to help users build beautiful and highly customizable status bars
without messing with named pipes, MacGyver-like scripting or non-blocking
loops lobotomizing your CPU.

Please note that the project is still in early development, so please
report any anomalies by [creating an issue ticket](https://github.com/jaagr/lemonbuddy/issues)
here on GitHub. I welcome pull requests with fixes or improvements.

Here are two screenshots showing you what it could look like (make sure to view them in full scale):

[![bspwm workspace](http://i.imgur.com/xvlw9iHm.png)](http://i.imgur.com/xvlw9iH.png)
[![dracula workspace](http://i.imgur.com/cYQOuRrm.png)](http://i.imgur.com/cYQOuRr.png)


## Installation

###  Arch Linux
Install the AUR package [lemonbuddy-git](https://aur.archlinux.org/packages/lemonbuddy-git/) to get the latest version, or
[lemonbuddy](https://aur.archlinux.org/packages/lemonbuddy/) for the latest stable release.

###  Void Linux
A package will be written for XBPS so stay tuned.

### Dependencies:

A C++ compiler with C++14 support. For example `clang`.

- lemonbar (patched with xft support)
- cmake
- boost
- libx11
- libxrandr
- wireless_tools _(optional: used by the network module)_
- alsa-lib _(optional: used by the volume module)_
- libmpdclient _(optional: used by the mpd module)_
- libsigc++ _(optional: used by the i3 module)_

> **NOTE:** The application has only been tested against the `single-mon` fork.
> If you have trouble running it with your current version, install the one
> included in `contrib/lemonbar-sm-git`.
> Plans are to make `lemonbar` an internal module.

**Install dependencies using pacman:**
~~~ sh
$ sudo pacman -S cmake boost libx11 libxrandr wireless_tools alsa-lib libmpdclient libsigc++
$ yaourt ttf-font-awesome
~~~

**Install dependencies using xbps-install:**
~~~ sh
$ sudo xbps-install cmake alsa-lib-devel boost-devel i3-devel libX11-devel libXrandr-devel libmpdclient-devel libsigc++-devel wireless_tools-devel
$ sudo xbps-install font-awesome
~~~

**Install dependencies using apt-get:**

> **NOTE:** To get support for the mpd and i3 modules, the `universe` repository
> needs to be added to the list of sources in `/etc/apt/sources.list`.
>
> Packages in the `universe` repository: `libmpdclient-dev` `fonts-font-awesome`

~~~ sh
$ sudo apt-get install cmake libxrandr-dev libboost-dev libiw-dev libasound2-dev libmpdclient-dev libsigc++-2.0-dev
$ sudo apt-get install fonts-font-awesome
~~~


### Building from source

> **NOTE:** In Void Linux you will need to install `git-perl` to get support for submodules.

  ~~~ sh
  $ git clone --branch 1.2.0 --recursive https://github.com/jaagr/lemonbuddy.git
  $ mkdir lemonbuddy/build
  $ cd lemonbuddy/build
  $ cmake ..
  # Optionally list and edit build settings before compiling
  # $ make edit_cache
  $ sudo make install
  ~~~


## Running

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
  $ lemonbuddy_wrapper example
  ~~~

> **NOTE:** If you are running i3 or bspwm and you don't see the workspace icons
> it probably depends on the font. Install `font-awesome` and relaunch the bar.
> ...or replace the icons in the config.

**It is recommended** to always use `lemonbuddy_wrapper` when launching the bars.

`lemonbuddy_wrapper` is just a simple shell script that takes care
of redirecting the in-/output streams between `lemonbuddy` and `lemonbar`.

If you handle the in-/output stream redirection's manually, the internal command
handlers (e.g. mpd or volume controls) might stop working. It won't change the
output of the bar but you will miss out on the internal API calls, which is one
of the main advantages of using the application.

The `lemonbuddy_wrapper` will be deprecated once `lemonbar` is integrated
into the project.


## Launching the bar in your wm's bootstrap routine

When using the wrapper to start the bar in in your wm's autostart routine, make sure to include
a kill directive before launching the bar. This is done to make sure that any previously spawned
processes gets terminated before before we launch the new ones.  For example in
`$HOME/.config/bspwmrc`:

  ~~~ sh
  # Terminate already running bar instances
  lemonbuddy_terminate noconfirm

  # Launch bar1 and bar2
  lemonbuddy_wrapper bar1 &
  lemonbuddy_wrapper bar2 &

  echo "Bars launched..."
  ~~~

If you are using i3, see [this issue](https://github.com/jaagr/lemonbuddy/issues/21) on how to display the bar.

## Configuration

The configuration syntax is very much **WIP**. If you have any feedback or suggestions on how to improve it,
please [create an issue ticket](https://github.com/jaagr/lemonbuddy/issues), or a pull request.

When working with unicode symbols, remember that fonts render them differently. Changing font
can change the quality of your generated output drastically. One must-have font
is [Unifont](http://unifoundry.com/unifont.html), which has great unicode coverage.

Also try different icon fonts, such as [Font Awesome](http://fontawesome.io/icons/) and [Material Icons](https://design.google.com/icons/).

The values used in the examples below are to be considered placeholder values, and
the resulting output might not be award-winning.

### Syntax and DSL

The configuration syntax is based on the `ini` file format.

  ~~~ ini
  [section/name]
  str = My string
  ; Hint: Quote the value to keep the spaces
  str = "   My string"
  bool = true
  bool = on
  int = 10
  float = 10.0

  ; Values for a defined bar can be referenced using:
  key = ${bar/top.foreground}

  ; Values for the current bar can be referenced using:
  key = ${BAR.foreground}

  ; Other values can be referenced using:
  key = ${section.key}

  ;format[-NAME] = "<TAGS...>"
  ;format[-NAME]-spacing = N (unit: whitespaces)
  ;format[-NAME]-padding = N (unit: whitespaces)
  ;format[-NAME]-margin = N  (unit: whitespaces)
  ;format[-NAME]-offset = N  (unit: pixels)
  ;format[-NAME]-foreground = #aarrggbb
  ;format[-NAME]-background = #aarrggbb
  ;format[-NAME]-underline = #aarrggbb
  ;format[-NAME]-overline = #aarrggbb
  ;
  ;label-NAME[-(foreground|background|(under|over)line|font|padding)] = ?
  ;icon-NAME[-(foreground|background|(under|over)line|font|padding)] = ?
  ;ramp-NAME-[0-9]+[-(foreground|background|(under|over)line|font|padding)] = ?
  ;animation-NAME-[0-9]+[-(foreground|background|(under|over)line|font|padding)] = ?
  ;
  ;bar-NAME-width = N (unit: characters)
  ;bar-NAME-format = (tokens: %fill% %indicator% %empty%)
  ;bar-NAME-foreground-[0-9]+ = #aarrggbb
  ;bar-NAME-indicator[-(foreground|background|(under|over)line|font|padding)] =
  ;bar-NAME-fill[-(foreground|background|(under|over)line|font|padding)] =
  ;bar-NAME-empty[-(foreground|background|(under|over)line|font|padding)] =
  ~~~

`🟊 = module is still flagged as work in progress`


### Application settings
  ~~~ ini
  [settings]
  ; Limit the amount of events sent to lemonbar within a set timeframe:
  ; - "Allow <throttle_limit> updates within <throttle_ms> of time"
  ; Default values:
  ;throttle_limit = 5
  ;throttle_ms = 50
  ~~~


### Bar settings
  ~~~ ini
  [bar/example]
  ; Use the following command to list available outputs:
  ; $ xrandr -q | grep " connected" | cut -d ' ' -f1
  monitor = HDMI1

  ; Omit the % to specify the width in pixels
  width = 100%
  height = 30

  ; Offset value defined in pixels
  ;offset_x = 0
  ;offset_y = 0

  ; Put the bar at the bottom of the screen
  bottom = true

  ; Weather to force docking mode or not
  dock = false

  ; This value is used as a multiplier when adding spaces between elements
  spacing = 3

  ; Height of under-/overline
  lineheight = 14

  ; Colors
  background = #222222
  foreground = #eefafafa
  linecolor = ${bar/example.background}

  ; Amount of spaces to add at the start/end of the whole bar
  padding_left = 5
  padding_right = 2

  ; Amount of spaces to add before/after each module
  module_margin_left = 3
  module_margin_right = 3

  ; Fonts are defined using: <FontName>;<Offset>
  font-0 = NotoSans-Regular:size=8;0
  font-1 = MaterialIcons:size=10;0
  font-2 = Termsynu:size=8;-1
  font-3 = FontAwesome:size=10;0

  ; The separator will be inserted between the output of each module
  ;separator = |

  ; This value is used by Lemonbar and it specifies the clickable
  ; areas available -> %{A:action:}...%{A}
  clickareas = 30

  ; Value to be used to set the WM_NAME atom
  ; This defaults to "lemonbuddy-[BAR]_[MONITOR]"
  ;wm_name = mybar

  ; Locale used to localize module output (for example date)
  ;locale = sv_SE.UTF-8

  ; Define what modules to output
  modules-left = cpu ram
  modules-center = label
  modules-right = clock
  ~~~


### Module `internal/backlight`
  ~~~ ini
  [module/backlight]
  type = internal/backlight

  ; Use the following command to list available cards:
  ; $ ls -1 /sys/class/backlight/
  card = intel_backlight
  ~~~

##### Extra formatting (example)
  ~~~ ini
  ; Available tags:
  ;   <label> (default)
  ;   <ramp>
  ;   <bar>
  format = <ramp> <bar>

  ; Available tokens:
  ;   %percentage% (default)
  label = %percentage%

  ramp-0 = 🌕
  ramp-1 = 🌔
  ramp-2 = 🌓
  ramp-3 = 🌒
  ramp-4 = 🌑

  bar-width = 10
  bar-indicator = |
  bar-fill = ─
  bar-empty = ─
  ~~~


### Module `internal/battery`

  ~~~ ini
  [module/battery]
  type = internal/battery

  ; This is useful in case the battery never reports 100% charge
  ;full_at = 99

  ; Use the following command to list batteries and adapters:
  ; $ ls -1 /sys/class/power_supply/
  ;battery = BAT0
  ;adapter = ADP1

  ; Seconds between reading battery capacity.
  ; If set to 0, polling will be disabled.
  ;poll_interval = 3
  ~~~

##### Extra formatting (example)
  ~~~ ini
  ; Available tags:
  ;   <label-charging> (default)
  ;   <bar-capaity>
  ;   <ramp-capacity>
  ;   <animation-charging>
  format-charging = <animation-charging> <label-charging>

  ; Available tags:
  ;   <label-discharging> (default)
  ;   <bar-capaity>
  ;   <ramp-capacity>
  format-discharging = <ramp-capacity> <label-discharging>

  ; Available tags:
  ;   <label-full> (default)
  ;   <bar-capaity>
  ;   <ramp-capacity>
  ;format-full = <ramp-capacity> <label-full>

  ; Available tokens:
  ;   %percentage% (default)
  label-charging = Charging %percentage%

  ; Available tokens:
  ;   %percentage% (default)
  label-discharging = Discharging %percentage%

  ; Available tokens:
  ;   %percentage% (default)
  label-full = Fully charged

  ramp-capacity-0 = 
  ramp-capacity-1 = 
  ramp-capacity-2 = 
  ramp-capacity-3 = 
  ramp-capacity-4 = 

  bar-capacity-width = 10

  animation-charging-0 = 
  animation-charging-1 = 
  animation-charging-2 = 
  animation-charging-3 = 
  animation-charging-4 = 
  animation-charging-framerate_ms = 750
  ~~~


### Module `internal/bspwm`
  ~~~ ini
  [module/bspwm]
  type = internal/bspwm
  ~~~

##### Extra formatting (example)
  ~~~ ini
  ; workspace_icon-[0-9]+ = label;icon
  workspace_icon-0 = code;♚
  workspace_icon-1 = office;♛
  workspace_icon-2 = graphics;♜
  workspace_icon-3 = mail;♝
  workspace_icon-4 = web;♞
  workspace_icon-default = ♟

  ; Available tags:
  ;   <label-state> (default) - gets replaced with <label-(active|urgent|occupied|empty)>
  ;   <label-mode> - gets replaced with <label-(monocle|tiled|fullscreen|floating|locked|sticky|private)>
  format = <label-state> <label-mode>

  ; If any values for label-dimmed-N area defined, the workspace/mode colors will get overridden
  ; with those values if the monitor is out of focus
  label-dimmed-foreground = #555
  label-dimmed-underline = ${BAR.background}

  ; Available tokens:
  ;   %name%
  ;   %icon%
  ;   %index%
  ; Default: %icon%  %name%
  label-active = %icon%
  label-active-foreground = #ffffff
  label-active-background = #3f3f3f
  label-active-underline = #fba922

  ; Available tokens:
  ;   %name%
  ;   %icon%
  ;   %index%
  ; Default: %icon%  %name%
  label-occupied = %icon%
  label-occupied-underline = #555555

  ; Available tokens:
  ;   %name%
  ;   %icon%
  ;   %index%
  ; Default: %icon%  %name%
  label-urgent = %icon%
  label-urgent-foreground = #000000
  label-urgent-background = #bd2c40
  label-urgent-underline = #9b0a20

  ; Available tokens:
  ;   %name%
  ;   %icon%
  ;   %index%
  ; Default: %icon%  %name%
  label-empty = %icon%
  label-empty-foreground = #55ffffff

  ; Available tokens:
  ;   None
  label-monocle = 
  ;label-tiled = 
  ;label-fullscreen = 
  ;label-floating = 
  label-locked = 
  label-locked-foreground = #bd2c40
  label-sticky = 
  label-sticky-foreground = #fba922
  label-private = 
  label-private-foreground = #bd2c40
  ~~~


### Module `internal/cpu`
  ~~~ ini
  [module/cpu]
  type = internal/cpu

  ; Seconds to sleep between updates
  ;interval = 0.5
  ~~~

##### Extra formatting (example)
  ~~~ ini
  ; Available tags:
  ;   <label> (default)
  ;   <bar-load>
  ;   <ramp-load>
  ;   <ramp-load_per_core>
  format = <label> <ramp-load_per_core>

  ; Available tokens:
  ;   %percentage% (default) - total cpu load
  label = CPU %percentage%

  ramp-load_per_core-0 = ▁
  ramp-load_per_core-1 = ▂
  ramp-load_per_core-2 = ▃
  ramp-load_per_core-3 = ▄
  ramp-load_per_core-4 = ▅
  ramp-load_per_core-5 = ▆
  ramp-load_per_core-6 = ▇
  ramp-load_per_core-7 = █
  ~~~


### Module `internal/date`
  ~~~ ini
  [module/date]
  type = internal/date

  ; Seconds to sleep between updates
  ;interval = 1.0

  ; see "man date" for details on how to format the date string
  ; NOTE: if you want to use lemonbar tags here you need to use %%{...}
  date = %Y-%m-%d% %H:%M

  ; if date_detailed is defined, clicking the area will toggle between formats
  date_detailed = %%{F#888}%A, %d %B %Y  %%{F#fff}%H:%M%%{F#666}:%%{F#fba922}%S%%{F-}
  ~~~

##### Extra formatting (example)
  ~~~ ini
  ; Available tags:
  ;   <date> (default)
  format = 🕓 <date>
  format-background = #55ff3399
  format-foreground = #fff
  ~~~


### 🟊 Module `internal/i3`

Requires the project to be built with support for i3. For more information,
see [the dependency section](#user-content-dependencies).

The module is still marked as WIP since it needs more testing. If you notice any
anomalies, please [create an issue](https://github.com/jaagr/lemonbuddy/issues).

See [the bspwm module](#user-content-dependencies) for details on `label-dimmed`.

  ~~~ ini
  [module/i3]
  type = internal/i3
  ~~~

##### Extra formatting (example)
  ~~~ ini
  ; workspace_icon-[0-9]+ = label;icon
  workspace_icon-0 = 1;♚
  workspace_icon-1 = 2;♛
  workspace_icon-2 = 3;♜
  workspace_icon-3 = 4;♝
  workspace_icon-4 = 5;♞
  workspace_icon-default = ♟

  ; Available tags:
  ;   <label-state> (default) - gets replaced with <label-(focused|unfocused|visible|urgent)>
  ;format = <label-state>

  ; Available tokens:
  ;   %name%
  ;   %icon%
  ;   %index%
  ; Default: %icon%  %name%
  label-focused = %icon%
  label-focused-foreground = #ffffff
  label-focused-background = #3f3f3f
  label-focused-underline = #fba922
  label-focused-padding = 4

  ; Available tokens:
  ;   %name%
  ;   %icon%
  ;   %index%
  ; Default: %icon%  %name%
  label-unfocused = %icon%
  label-unfocused-padding = 4

  ; Available tokens:
  ;   %name%
  ;   %icon%
  ;   %index%
  ; Default: %icon%  %name%
  label-visible = %icon%
  label-visible-underline = #555555
  label-visible-padding = 4

  ; Available tokens:
  ;   %name%
  ;   %icon%
  ;   %index%
  ; Default: %icon%  %name%
  label-urgent = %icon%
  label-urgent-foreground = #000000
  label-urgent-background = #bd2c40
  label-urgent-padding = 4
  ~~~


### Module `internal/memory`
  ~~~ ini
  [module/memory]
  type = internal/memory

  ; Seconds to sleep between updates
  ;interval = 1.0
  ~~~

##### Extra formatting (example)
  ~~~ ini
  ; Available tags:
  ;   <label> (default)
  ;   <bar-used>
  ;   <bar-free>
  format = <label> <bar-used>

  ; Available tokens:
  ;   %percentage_used% (default)
  ;   %percentage_free%
  ;   %gb_used%
  ;   %gb_free%
  ;   %gb_total%
  ;   %mb_used%
  ;   %mb_free%
  ;   %mb_total%
  label = RAM %percentage_used%

  bar-used-width = 50
  bar-used-foreground-0 = #55aa55
  bar-used-foreground-1 = #557755
  bar-used-foreground-2 = #f5a70a
  bar-used-foreground-3 = #ff5555
  bar-used-fill = ▐
  bar-used-empty = ▐
  bar-used-empty-foreground = #444444
  ~~~


### Module `internal/mpd`
  ~~~ ini
  [module/mpd]
  type = internal/mpd

  ;host = 127.0.0.1
  ;port = 6600
  ;password = mypassword

  ; Seconds to sleep between progressbar/song timer sync
  ;interval = 0.5
  ~~~

##### Extra formatting (example)
  ~~~ ini
  ; Available tags:
  ;   <label-song> (default)
  ;   <label-time>
  ;   <bar-progress>
  ;   <toggle> - gets replaced with <icon-(pause|play)>
  ;   <icon-random>
  ;   <icon-repeat>
  ;   <icon-repeatone>
  ;   <icon-prev>
  ;   <icon-stop>
  ;   <icon-play>
  ;   <icon-pause>
  ;   <icon-next>
  format-online = <icon-prev> <icon-stop> <toggle> <icon-next>  <icon-repeat> <icon-random>  <bar-progress> <label-time>  <label-song>

  ; Available tags:
  ;   <label-offline>
  ;format-offline = <label-offline>

  ; Available tokens:
  ;   %artist%
  ;   %album%
  ;   %title%
  ; Default: %artist% - %title%
  ;label-song = 𝄞 %artist% - %title%

  ; Available tokens:
  ;   %elapsed%
  ;   %total%
  ; Default: %elapsed% / %total%
  ;label-time = %elapsed% / %total%

  ; Available tokens:
  ;   None
  label-offline = 🎜 mpd is offline

  icon-play = ⏵
  icon-pause = ⏸
  icon-stop = ⏹
  icon-prev = ⏮
  icon-next = ⏭
  icon-random = 🔀
  icon-repeat = 🔁
  ;icon-repeatone = 🔂

  ; Used to display the state of random/repeat/repeatone
  toggle_on-foreground = #ff
  toggle_off-foreground = #55

  bar-progress-width = 45
  bar-progress-indicator = |
  bar-progress-fill = ─
  bar-progress-empty = ─
  ~~~


### Module `internal/network`

 **NOTE:** If you use both a wired and a wireless network, just add 2 module definitions.
 For example:

> ~~~ ini
> [module/wired-network]
> type = internal/network
> interface = eth1
>
> [module/wireless-network]
> type = internal/network
> interface = wlan1
> ~~~

  ~~~ ini
  [module/network]
  type = internal/network
  interface = wlan1

  ; Seconds to sleep between updates
  interval = 3.0

  ; Test connectivity every Nth update
  ; A value of 0 disables the feature
  ; Recommended minimum value: round(10 / interval)
  ;   - which would test the connection approx. every 10th sec.
  ; Default: 0
  ;ping_interval = 3
  ~~~

##### Extra formatting (example)
  ~~~ ini
  ; Available tags:
  ;   <label-connected> (default)
  ;   <ramp-signal>
  format-connected = <ramp-signal> <label-connected>

  ; Available tags:
  ;   <label-disconnected> (default)
  ;format-disconnected = <label-disconnected>

  ; Available tags:
  ;   <label-connected> (default)
  ;   <label-packetloss>
  ;   <animation-packetloss>
  format-packetloss = <animation-packetloss> <label-connected>

  ; Available tokens:
  ;   %ifname%    [wireless+wired]
  ;   %local_ip%  [wireless+wired]
  ;   %essid%     [wireless]
  ;   %signal%    [wireless]
  ;   %linkspeed% [wired]
  ; Default: %ifname% %local_ip%
  label-connected = %essid%
  label-connected-foreground = #eefafafa

  ; Available tokens:
  ;   %ifname%    [wireless+wired]
  ; Default: (none)
  ;label-disconnected = not connected
  ;label-disconnected-foreground = #66ffffff

  ; Available tokens:
  ;   %ifname%    [wireless+wired]
  ;   %local_ip%  [wireless+wired]
  ;   %essid%     [wireless]
  ;   %signal%    [wireless]
  ;   %linkspeed% [wired]
  ; Default: (none)
  ;label-packetloss = %essid%
  ;label-packetloss-foreground = #eefafafa

  ramp-signal-0 = 😱
  ramp-signal-1 = 😠
  ramp-signal-2 = 😒
  ramp-signal-3 = 😊
  ramp-signal-4 = 😃
  ramp-signal-5 = 😈

  animation-packetloss-0 = ⚠
  animation-packetloss-0-foreground = #ffa64c
  animation-packetloss-1 = 📶
  animation-packetloss-1-foreground = #000000
  animation-packetloss-framerate_ms = 500
  ~~~


### 🟊 Module `internal/volume`

  This module is still WIP.

  TODO: Add custom format for when the headphones are plugged in.

  ~~~ ini
  [module/volume]
  type = internal/volume
  ;master_mixer = Master

  ; Use the following command to list available mixer controls:
  ; $ amixer scontrols | sed -nr "s/.*'([[:alnum:]]+)'.*/\1/p"
  speaker_mixer = Speaker
  headphone_mixer = Headphone

  ; NOTE: This is required if headphone_mixer is defined
  ; Use the following command to list available device controls
  ; $ amixer controls | sed -r "/CARD/\!d; s/.*=([0-9]+).*name='([^']+)'.*/printf '%3.0f: %s\n' '\1' '\2'/e" | sort
  headphone_control_numid = 9
  ~~~

##### Extra formatting (example)
  ~~~ ini
  ; Available tags:
  ;   <label-volume> (default)
  ;   <ramp-volume>
  ;   <bar-volume>
  format-volume = <ramp-volume> <label-volume>

  ; Available tags:
  ;   <label-muted> (default)
  ;   <ramp-volume>
  ;   <bar-volume>
  ;format-muted = <label-muted>

  ; Available tokens:
  ;   %percentage% (default)
  ;label-volume = %percentage%

  ; Available tokens:
  ;   %percentage% (default)
  label-muted = 🔇 muted
  label-muted-foreground = #66

  ramp-volume-0 = 🔈
  ramp-volume-1 = 🔉
  ramp-volume-2 = 🔊
  ~~~


### Module `custom/menu`
  ~~~ ini
  [module/menu-apps]
  type = custom/menu

  ; "menu-LEVEL-N" has the same properties as "label-NAME" with
  ; the additional "exec" property
  ;
  ; Available exec commands:
  ;   menu_open-LEVEL
  ;   menu_close
  ; Other commands will be executed using "/usr/bin/env sh -c $COMMAND"

  menu-0-0 = Browsers
  menu-0-0-exec = menu_open-1
  menu-0-0-foreground = #fba922
  menu-0-2 = Multimedia
  menu-0-2-exec = menu_open-3
  menu-0-2-foreground = #fba922

  menu-1-0 = Firefox
  menu-1-0-exec = firefox &
  menu-1-0-foreground = #fba922
  menu-1-1 = Chromium
  menu-1-1-exec = chromium &
  menu-1-1-foreground = #fba922

  menu-2-0 = Gimp
  menu-2-0-foreground = #fba922
  menu-2-0-exec = gimp &
  menu-2-1 = Scrot
  menu-2-1-exec = scrot &
  menu-2-1-foreground = #fba922
  ~~~

##### Extra formatting (example)
  ~~~ ini
  ; Available tags:
  ;   <label-toggle> (default) - gets replaced with <label-(open|close)>
  ;   <menu> (default)
  ;f-ormat = <label-toggle> <menu>

  label-open = Apps
  label-close = x
  ~~~


### Module `custom/script`
  ~~~ ini
  [module/pkgupdates-available]
  type = custom/script

  ; Available tokens:
  ;   %counter%
  ; The "exec" command will be executed using "/usr/bin/env sh -c [command]"
  exec = count=$(sudo pacman -Syup --noprogressbar 2>/dev/null | sed '/Starting full/,~1!d;/Starting full/d' | wc -l); [ $count -gt 0 ] && echo "Updates available: $count"
  ;exec = count=$(echo n | sudo xbps-install -Su >/dev/null 2>&1; sudo xbps-install -Sun | wc -l); [ $count -gt 0 ] && echo "Updates available: $count"

  ; Should we keep listening for output from the command?
  ;tail = false

  ; Seconds to sleep between updates
  ; Will be ignored if `tail = true`
  ; Default: 1
  interval = 90
  ~~~

##### Extra formatting (example)
  ~~~ ini
  ; Available tags:
  ;   <output> (default)
  ;format = <output>
  format-background = #999
  format-foreground = #000
  format-padding = 4

  ; Available tokens:
  ;   %counter%
  ;
  ; "click-(left|middle|right)" will be executed using "/usr/bin/env sh -c [command]"
  click-left = echo left %counter%
  click-middle = echo middle %counter%
  click-right = echo right %counter%

  ; Available tokens:
  ;   %counter%
  ;
  ; "scroll-(up|down)" will be executed using "/usr/bin/env sh -c [command]"
  scroll-up = echo scroll up %counter%
  scroll-down = echo scroll down %counter%
  ~~~

##### Useful example
  Show title of the currently focused window.

  ~~~ ini
  [module/xtitle]
  type = custom/script
  exec = xtitle -s
  tail = true
  ~~~

### Module `custom/text`
  ~~~ ini
  [module/my-text-label]
  type = custom/text
  content = Some random label
  ~~~

##### Extra formatting (example)
  ~~~ ini
  ; "content" has the same properties as "format-NAME"
  content-background = #000
  content-foreground = #fff
  content-padding = 4

  ; "click-(left|middle|right)" will be executed using "/usr/bin/env sh -c $COMMAND"
  click-left = echo left
  click-middle = echo middle
  click-right = echo right

  ; "scroll-(up|down)" will be executed using "/usr/bin/env sh -c $COMMAND"
  scroll-up = echo scroll up
  scroll-down = echo scroll down
  ~~~


## License

Lemonbuddy is licensed under the MIT license. [See LICENSE for more information](https://github.com/jaagr/lemonbuddy/blob/master/LICENSE).
