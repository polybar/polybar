modules = {
    i3_test = {
        type = "internal/i3",
        -- this syntax is needed for key containing '-' or reserved word
        [ "wrapping-scroll" ] = true,
        format = "<label-state> <label-mode>"
    },
    mydate = {
        type = "internal/date",
        internal = "5",
        date = " %Y-%m-%d",
        time = "%H:%M",
        ["time-alt"] = "%H:%M:%S",
        ["format-underline"] = "#0a6cf5",
        label = "%date% %time%"
    },
    fs1 = {
        type = "internal/fs",
        interval = "25",
        mount = {"/"},
        ["label-mounted"] = "%{F#0a81f5}%mountpoint%%{F-}: %percentage_used%%",
        ["label-unmounted"] = "%mountpoint% not mounted",
        ["label-unmounted-foreground"] = "#555555"
    },
    fs2 = {
        type = "internal/fs",
        interval = 25,
        mount = {"/home"},
        ["label-mounted"] = "%{F#0a81f5}%mountpoint%%{F-}: %percentage_used%%",
        ["label-unmounted"] = "%mountpoint% not mounted",
        ["label-unmounted-foreground"] = "#555555"
    }
}

bars = {
    bar_test = {
        monitor = "eDP-1",
        width = "100%",
        height = "18",
        radius = 4.0,
        ["fixed-center"] = false,
--        background = {"#222222", "#444444"},
        background = "#222222",
        foreground = "#dfdfdf",
        font = {"fixed:pixelsize=10;1"},
        ["enable-ipc"] = true,
        ["modules-center"] = "i3_test mydate fs1 fs2"
    }
}

