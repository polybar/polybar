set(SETTING_ALSA_SOUNDCARD "default"
  CACHE STRING "Name of the ALSA soundcard driver")
set(SETTING_BSPWM_SOCKET_PATH "/tmp/bspwm_0_0-socket"
  CACHE STRING "Path to bspwm socket")
set(SETTING_BSPWM_STATUS_PREFIX "W"
  CACHE STRING "Prefix prepended to the bspwm status line")
set(SETTING_CONNECTION_TEST_IP "8.8.8.8"
  CACHE STRING "Address to ping when testing network connection")
set(SETTING_PATH_ADAPTER "/sys/class/power_supply/%adapter%"
  CACHE STRING "Path to adapter")
set(SETTING_PATH_BACKLIGHT "/sys/class/backlight/%card%"
  CACHE STRING "Path to backlight sysfs folder")
set(SETTING_PATH_BATTERY "/sys/class/power_supply/%battery%"
  CACHE STRING "Path to battery")
set(SETTING_PATH_CPU_INFO "/proc/stat"
  CACHE STRING "Path to file containing cpu info")
set(SETTING_PATH_MEMORY_INFO "/proc/meminfo"
  CACHE STRING "Path to file containing memory info")
set(SETTING_PATH_MESSAGING_FIFO "/tmp/polybar_mqueue.%pid%"
  CACHE STRING "Path to file containing the current temperature")
set(SETTING_PATH_TEMPERATURE_INFO "/sys/class/thermal/thermal_zone%zone%/temp"
  CACHE STRING "Path to file containing the current temperature")
set(SETTING_PATH_THERMAL_ZONE_WILDCARD "/sys/class/thermal/thermal_zone*"
  CACHE STRING "Wildcard path to different thermal zones")
