;=====================================================
;
;   To learn more about how to configure Polybar
;   go to https://github.com/jaagr/polybar
;
;   The README contains alot of information
;
;=====================================================

[colors]
background = #222
background-alt = #444
foreground = #dfdfdf
foreground-alt = #55
primary = #ffb52a
secondary = #e60053
alert = #bd2c40

[global/wm]
margin-top = 5
margin-bottom = 5

[bar/example]
;monitor = ${env:MONITOR:HDMI-1}
width = 100%
height = 27
offset-x = 0
offset-y = 0

;background = ${xrdb:color9}
background = ${colors.background}
foreground = ${colors.foreground}

overline-size = 2
overline-color = #f00
underline-size = 2
underline-color = #00f

border-bottom = 2
border-bottom-color = #333

spacing = 1
padding-left = 0
padding-right = 2
module-margin-left = 1
module-margin-right = 2

font-0 = fixed:pixelsize=10;0
font-1 = unifont:size=6:heavy:fontformat=truetype;-2
font-2 = siji:pixelsize=10;0

modules-left = @MODULES_LEFT@
modules-center = @MODULES_CENTER@
modules-right = @MODULES_RIGHT@

tray-position = right
tray-padding = 2
;tray-transparent = true
;tray-background = #0063ff

;wm-restack = bspwm
;wm-restack = i3

;override-redirect = true

[module/xwindow]
type = internal/xwindow
label = %title:0:30:...%

[module/xkeyboard]
type = internal/xkeyboard
blacklist-0 = num lock

format-underline = ${colors.secondary}
format-prefix = " "
format-prefix-foreground = ${colors.foreground-alt}

label-layout = %layout%

label-indicator-padding = 2
label-indicator-background = ${colors.secondary}
label-indicator-underline = ${colors.secondary}

[module/filesystem]
type = internal/fs
interval = 25

mount-0 = /
mount-1 = /home
mount-2 = /invalid/mountpoint

label-mounted = %mountpoint%: %percentage_free%

label-unmounted = %mountpoint%: not mounted
label-unmounted-foreground = ${colors.foreground-alt}

[module/bspwm]
type = internal/bspwm
ws-icon-default = x

label-focused = %index%
label-focused-background = ${colors.background-alt}
label-focused-underline= ${colors.primary}
label-focused-padding = 2

label-occupied = %index%
label-occupied-padding = 2

label-urgent = %index%
label-urgent-background = ${colors.alert}
label-urgent-padding = 2

label-empty = %index%
label-empty-foreground = ${colors.foreground-alt}
label-empty-padding = 2

[module/i3]
type = internal/i3
format = <label-state> <label-mode>
index-sort = true

label-mode = %mode%
label-mode-padding = 2
label-mode-foreground = #000
label-mode-background = ${colors.primary}

label-focused = %index%
label-focused-background = ${module/bspwm.label-focused-background}
label-focused-underline = ${module/bspwm.label-focused-underline}
label-focused-padding = ${module/bspwm.label-focused-padding}

label-unfocused = %index%
label-unfocused-padding = ${module/bspwm.label-occupied-padding}

label-urgent = %index%!
label-urgent-background = ${colors.alert}
label-urgent-padding = ${module/bspwm.label-urgent-padding}

label-visible = %index%
label-visible-foreground = ${module/bspwm.label-empty-padding}
label-visible-padding = ${module/bspwm.label-empty-padding}

[module/mpd]
type = internal/mpd

format-online = <label-song>  <icon-prev> <icon-seekb> <icon-stop> <toggle> <icon-seekf> <icon-next>  <icon-repeat> <icon-random>

label-song-maxlen = 25
label-song-ellipsis = true

icon-prev = 
icon-seekb = 
icon-stop = 
icon-play = 
icon-pause = 
icon-next = 
icon-seekf = 

icon-random = 
icon-repeat = 

toggle-on-foreground = ${colors.primary}
toggle-off-foreground = #66

[module/backlight]
type = internal/xbacklight

format = <label> <bar>
label = BL

bar-width = 10
bar-indicator = │
bar-indicator-foreground = #ff
bar-indicator-font = 2
bar-fill = ─
bar-fill-font = 2
bar-fill-foreground = #9f78e1
bar-empty = ─
bar-empty-font = 2
bar-empty-foreground = ${colors.foreground-alt}

[module/cpu]
type = internal/cpu
interval = 2
format-prefix = " "
format-prefix-foreground = ${colors.foreground-alt}
format-underline = #f90000
label = %percentage%

[module/memory]
type = internal/memory
interval = 2
format-prefix = " "
format-prefix-foreground = ${colors.foreground-alt}
format-underline = #4bffdc
label = %percentage_used%

[module/wlan]
type = internal/network
interface = @INTERFACE_WLAN@
interval = 3.0

format-connected = <ramp-signal> <label-connected>
format-connected-underline = #9f78e1
format-disconnected-underline = ${self.format-connected-underline}

label-connected = %essid%
label-disconnected = %ifname% disconnected
label-disconnected-foreground = ${colors.foreground-alt}

ramp-signal-0 = 
ramp-signal-1 = 
ramp-signal-2 = 
ramp-signal-3 = 
ramp-signal-4 = 
ramp-signal-foreground = ${colors.foreground-alt}

[module/eth]
type = internal/network
interface = @INTERFACE_ETH@
interval = 3.0

format-connected-underline = #55aa55
format-connected-prefix = " "
format-connected-foreground-foreground = ${colors.foreground-alt}
label-connected = %local_ip%

format-disconnected-underline = ${self.format-connected-underline}
label-disconnected = %ifname% disconnected
label-disconnected-foreground = ${colors.foreground-alt}

[module/date]
type = internal/date
date = %H:%M
interval = 5
format-prefix = " "
format-prefix-foreground = ${colors.foreground-alt}
format-underline = #0a6cf5

[module/volume]
type = internal/volume

format-volume = <label-volume> <bar-volume>
label-volume = VOL
label-volume-foreground = ${root.foreground}

format-muted-prefix = " "
format-muted-foreground = ${colors.foreground-alt}
label-muted = sound muted

bar-volume-width = 10
bar-volume-foreground-0 = #55aa55
bar-volume-foreground-1 = #55aa55
bar-volume-foreground-2 = #55aa55
bar-volume-foreground-3 = #55aa55
bar-volume-foreground-4 = #55aa55
bar-volume-foreground-5 = #f5a70a
bar-volume-foreground-6 = #ff5555
bar-volume-gradient = false
bar-volume-indicator = │
bar-volume-indicator-font = 2
bar-volume-indicator-foreground = #ff
bar-volume-fill = ─
bar-volume-fill-font = 2
bar-volume-empty = ─
bar-volume-empty-font = 2
bar-volume-empty-foreground = ${colors.foreground-alt}

[module/battery]
type = internal/battery
battery = @BATTERY@
adapter = @ADAPTER@
full-at = 98

format-charging = <animation-charging> <label-charging>
format-charging-underline = #ffb52a

format-discharging = <ramp-capacity> <label-discharging>
format-discharging-underline = ${self.format-charging-underline}

format-full-prefix = " "
format-full-prefix-foreground = ${colors.foreground-alt}
format-full-underline = ${self.format-charging-underline}

ramp-capacity-0 = 
ramp-capacity-1 = 
ramp-capacity-2 = 
ramp-capacity-foreground = ${colors.foreground-alt}

animation-charging-0 = 
animation-charging-1 = 
animation-charging-2 = 
animation-charging-foreground = ${colors.foreground-alt}
animation-charging-framerate = 750

[module/temperature]
type = internal/temperature
thermal-zone = 0
warn-temperature = 60

format = <ramp> <label>
format-underline = #f50a4d
format-warn = <ramp> <label-warn>
format-warn-underline = ${self.format-underline}

label = %temperature%
label-warn = %temperature%
label-warn-foreground = ${colors.secondary}

ramp-0 = 
ramp-1 = 
ramp-2 = 
ramp-foreground = ${colors.foreground-alt}

[module/powermenu]
type = custom/menu

label-open =  power
label-open-foreground = ${colors.secondary}
label-close =  cancel
label-close-foreground = ${colors.secondary}
label-separator = |
label-separator-foreground = ${colors.foreground-alt}

menu-0-0 = reboot
menu-0-0-exec = menu-open-1
menu-0-1 = power off
menu-0-1-exec = menu-open-2

menu-1-0 = cancel
menu-1-0-exec = menu-open-0
menu-1-1 = reboot
menu-1-1-exec = sudo reboot

menu-2-0 = power off
menu-2-0-exec = sudo poweroff
menu-2-1 = cancel
menu-2-1-exec = menu-open-0

; vim:ft=dosini
