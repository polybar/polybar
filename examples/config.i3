;=====================================================
;
;   To learn more about how to configure Lemonbuddy
;   go to https://github.com/jaagr/lemonbuddy
;
;   The README contains alot of information
;
;=====================================================

[bar/example]
;monitor = eDP-1
dock = false
width = 100%
height = 27

background = #ee222222
foreground = #ccfafafa
linecolor = #666

border-bottom = 2
border-bottom-color = #333

spacing = 1
lineheight = 1

padding-left = 0
padding-right = 2

module-margin-left = 1
module-margin-right = 2

font-0 = fixed:size=8;1

modules-left = i3
modules-right = xbacklight date

tray-position = right
tray-background = #33ffffff
tray-padding = 4


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
label-visible-foreground = #444
label-visible-padding = 2


[module/xbacklight]
type = internal/xbacklight

format = <label> <bar>

label = Brightness %percentage%

bar-width = 15
bar-indicator = │
bar-indicator-foreground = #bbffffff
bar-fill = ─
bar-fill-foreground = #9f78e1
bar-empty = ─
bar-empty-foreground = #44


[module/date]
type = internal/date
date = %Y-%m-%d %%{F#e60053}%H:%M%%{F#cc}
interval = 5

; vim:ft=dosini
