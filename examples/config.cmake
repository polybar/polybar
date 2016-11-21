;=====================================================
;
;   To learn more about how to configure Polybar
;   go to https://github.com/jaagr/polybar
;
;   The README contains alot of information
;
;=====================================================

[global/wm]
margin-top = 5
margin-bottom = 5


[bar/example]
;monitor = ${env:MONITOR:HDMI-1}
dock = false
width = 100%
height = 27
offset-x = 0
offset-y = 0

;background = ${xrdb:color9}
background = #ee222222
foreground = #dfdfdf

;lineheight = 1
;linecolor = #555
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


[module/xwindow]
type = internal/xwindow
label = %title%
label-maxlen = 30


[module/filesystem]
type = internal/fs
interval = 25

mount-0 = /
mount-1 = /home
mount-2 = /invalid/mountpoint

label-mounted = %mountpoint%: %percentage_free%

label-unmounted = %mountpoint%: not mounted
label-unmounted-foreground = #55



[module/bspwm]
type = internal/bspwm
ws-icon-default = x

label-active = %index%
label-active-background = #ee333333
label-active-underline= #cc333333
label-active-padding = 2

label-occupied = %index%
label-occupied-padding = 2

label-urgent = %index%
label-urgent-background = #bd2c40
label-urgent-padding = 2

label-empty = %index%
label-empty-foreground = #55
label-empty-padding = 2


[module/i3]
type = internal/i3
format = <label-state>
index-sort = true

label-focused = %index%
label-focused-background = #ee333333
label-focused-underline= #cc333333
label-focused-padding = 2

label-unfocused = %index%
label-unfocused-padding = 2

label-urgent = %index%!
label-urgent-background = #bd2c40
label-urgent-padding = 2

label-visible = %index%
label-visible-foreground = #55
label-visible-padding = 2


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

toggle-on-foreground = #e60053
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
bar-empty-foreground = #55


[module/cpu]
type = internal/cpu
interval = 2
label = %{F#666}%{F#cc} %percentage%
label-underline = #f90000


[module/memory]
type = internal/memory
interval = 2
label = %{F#665}%{F#cc} %percentage_used%
label-underline = #4bffdc


[module/wlan]
type = internal/network
interface = @INTERFACE_WLAN@
interval = 3.0

format-connected = <ramp-signal> <label-connected>
format-connected-underline = #9f78e1
format-disconnected-underline = ${self.format-connected-underline}

label-connected = %essid%
label-disconnected = %ifname% disconnected
label-disconnected-foreground = #55

ramp-signal-0 = 
ramp-signal-1 = 
ramp-signal-2 = 
ramp-signal-3 = 
ramp-signal-4 = 
ramp-signal-foreground = #55


[module/eth]
type = internal/network
interface = @INTERFACE_ETH@
interval = 3.0

format-connected-underline = #55aa55
format-disconnected-underline = ${self.format-connected-underline}

label-connected = %{F#55}%{F#ff} %local_ip%
label-disconnected = %ifname% disconnected
label-disconnected-foreground = #55


[module/date]
type = internal/date
date = %%{F#55}%%{F#ff} %H:%M
date-alt = %%{F#55}%{F#ff} %Y-%m-%d  %%{F#55}%%{F#ff} %H:%M
interval = 5
format-underline = #0a6cf5


[module/volume]
type = internal/volume

format-volume = <label-volume> <bar-volume>

label-volume = VOL
label-volume-foreground = ${root.foreground}

label-muted =  sound muted
label-muted-foreground = #55

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
bar-volume-empty-foreground = #55


[module/battery]
type = internal/battery
battery = @BATTERY@
adapter = @ADAPTER@
full-at = 98

format-charging = <animation-charging> <label-charging>
format-charging-underline = #ffb52a
format-discharging = <ramp-capacity> <label-discharging>
format-discharging-underline = ${self.format-charging-underline}
format-full = %{F#55}%{F#ff}  <label-full>
format-full-underline = ${self.format-charging-underline}

ramp-capacity-0 = 
ramp-capacity-1 = 
ramp-capacity-2 = 
ramp-capacity-foreground = #55

animation-charging-0 = 
animation-charging-1 = 
animation-charging-2 = 
animation-charging-foreground = #55
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
label-warn-foreground = #e60053

ramp-0 = 
ramp-1 = 
ramp-2 = 
ramp-foreground = #55


[module/powermenu]
type = custom/menu

label-open =  power
label-open-foreground = #e60053
label-close =  cancel
label-close-foreground = #e60053
label-separator = |
label-separator-foreground = #55

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
