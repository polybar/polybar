# Changelog

All notable changes to this project will be documented in this file.
Each release should have the following subsections, if entries exist, in the
given order: `Breaking`, `Build`, `Deprecated`, `Removed`, `Added`, `Changed`,
`Fixed`, `Security`.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [3.6.1] - 2022-03-05
### Build
- Fixed compiler warning in Clang 13 ([`#2613`](https://github.com/polybar/polybar/pull/2613))
- Fixed compiler error in GCC 12 ([`#2616`](https://github.com/polybar/polybar/pull/2616), [`#2614`](https://github.com/polybar/polybar/issues/2614))
- Fixed installation of docs when some are not generated (man, html...) ([`#2612`](https://github.com/polybar/polybar/pull/2612))
- Fix `LDFLAGS` not being respected ([`#2619`](https://github.com/polybar/polybar/pull/2619))

### Fixed
- `tray-offset-x`, `tray-offset-y`, `offset-x`, and `offset-y` were mistakenly capped below at 0 ([`#2620`](https://github.com/polybar/polybar/pull/2620))
- `custom/script`: Polybar shutdown being stalled by hanging script ([`#2621`](https://github.com/polybar/polybar/pull/2621))
- `polybar-msg`: Wrong hint when using deprecated `hook` ([`#2624`](https://github.com/polybar/polybar/pull/2624))

## [3.6.0] - 2022-03-01
### Breaking
- We added the backslash escape character (\\) for configuration values. This means that the literal backslash character now has special meaning in configuration files, therefore if you want to use it in a value as a literal backslash, you need to escape it with the backslash escape character. The parser logs an error if any unescaped backslashes are found in a value. This affects you only if you are using two consecutive backslashes in a config value, which will now be interpreted as a single literal backslash. ([`#2354`](https://github.com/polybar/polybar/issues/2354))
- We rewrote our formatting tag parser. This shouldn't break anything, if you experience any problems, please let us know. The new parser now gives errors for certain invalid tags where the old parser would just silently ignore them. Adding extra text to the end of a valid tag now produces an error. For example, tags like `%{T-a}`, `%{T2abc}`, `%{rfoo}`, and others will now start producing errors. This does not affect you unless you are producing your own invalid formatting tags (for example in a script).
- For security reasons, the named pipe at `/tmp/polybar_mqueue.<PID>` had its permission bits changed from `666` to `600` to prevent sending ipc messages to polybar processes running under a different user.
- Also for security reasons, the `polybar-msg` command will now only send messages to polybar processes running under the same user. See the [IPC documentation](https://polybar.readthedocs.io/en/stable/user/ipc.html) for what exactly this means.

### Build
- New dependency: [libuv](https://github.com/libuv/libuv). At least version 1.3 is required.
- Bump the minimum cmake version to 3.5
- The `BUILD_IPC_MSG` option has been renamed to `BUILD_POLYBAR_MSG`
- Building the documentation is now enabled by default and not just when `sphinx-build` is found.
- Users can control exactly which targets should be available with the following cmake options (together with their default value):
  - `BUILD_POLYBAR=ON` - Builds the `polybar` executable
  - `BUILD_POLYBAR_MSG=ON` - Builds the `polybar-msg` executable
  - `BUILD_TESTS=OFF` - Builds the test suite
  - `BUILD_DOC=ON` - Builds the documentation
  - `BUILD_DOC_HTML=BUILD_DOC` - Builds the html documentation (depends on `BUILD_DOC`)
  - `BUILD_DOC_MAN=BUILD_DOC` - Builds the manpages (depends on `BUILD_DOC`)
  - `BUILD_CONFIG=ON` - Generates the default config
  - `BUILD_SHELL=ON` - Generates shell completion files
  - `DISABLE_ALL=OFF` - Disables all above targets by default. Individual targets can still be enabled explicitly.
- The documentation can no longer be built by directly configuring the `doc` directory.
- The `POLYBAR_FLAGS` cmake variable can be used to pass extra C++ compiler flags.
- The sample config file has been removed.
- Polybar now ships a default config that is installed to `/etc/polybar/config.ini`, it lives in `doc/config.ini`. It will also be placed in the `examples` directory in the documentation folder. ([`#2405`](https://github.com/polybar/polybar/issues/2405))
- The `userconfig` target has been removed, you can no longer use `make userconfig`. As an alternative, you can copy the default config from `/etc/polybar/config.ini`.
- The `DEBUG_SHADED` cmake variable and its associated functionality has been removed.

### Deprecated
- `[settings]`: `throttle-output` and `throttle-output-for` have been removed. The new event loop already does a similar thing where it coalesces update triggers if they happen directly after one another, leading to only a single bar update.
- When not specifying the config file with `--config`, naming your config file `config` is deprecated. Rename your config file to `config.ini`.
- Directly writing ipc messages to `/tmp/polybar_mqueue.<PID>` is deprecated, users should always use `polybar-msg`. As a consequence the message format used for IPC is deprecated as well.
- `polybar-msg hook` is deprecated in favor of using the hook action. `polybar-msg` will tell you the correct command to use.

### Added
- Add repeat interval for script failure (`interval-fail`) and `exec-if` (`interval-if`) ([`#943`](https://github.com/polybar/polybar/issues/943), [`#2606`](https://github.com/polybar/polybar/issues/2606))
- Support `px` and `pt` units everyhwere where before only a number of spaces or pixels could be specified. ([`#2578`](https://github.com/polybar/polybar/pull/2578), [`#1651`](https://github.com/polybar/polybar/issues/1651), [`#951`](https://github.com/polybar/polybar/issues/951))
- `internal/alsa`: Right and middle click settings. ([`#2566`](https://github.com/polybar/polybar/issues/2566), [`#2573`](https://github.com/polybar/polybar/pull/2573))
- `internal/network`:
  - New token `%mac%` shows MAC address of selected interface ([`#2568`](https://github.com/polybar/polybar/issues/2568), [`#2569`](https://github.com/polybar/polybar/pull/2569))
  - New token `%netspeed%` that provides the total speed of the internet (up + down speed) ([`#2590`](https://github.com/polybar/polybar/pull/2590), [`#1083`](https://github.com/polybar/polybar/issues/1083))
  - `speed-unit = B/s` can be used to customize how network speeds are displayed. ([`#2068`](https://github.com/polybar/polybar/pull/2068))
  - `interface-type` may be used in place of `interface` to automatically select a network interface ([`#2025`](https://github.com/polybar/polybar/pull/2025), [`#339`](https://github.com/polybar/polybar/issues/339))
- Polybar can now read config files from stdin: `polybar -c /dev/stdin`. ([`#2545`](https://github.com/polybar/polybar/pull/2545))
- `custom/script`:
  - Setting environment variables using `env-*` config option. ([`#2090`](https://github.com/polybar/polybar/issues/2090), [`#2512`](https://github.com/polybar/polybar/pull/2512))
  - Add formatting for script failure (`format-fail`, `label-fail`) ([`#2588`](https://github.com/polybar/polybar/issues/2588), [`#2596`](https://github.com/polybar/polybar/pull/2596))
- Support for ramp weights. ([`#1750`](https://github.com/polybar/polybar/issues/1750), [`#2505`](https://github.com/polybar/polybar/pull/2505))
- `internal/memory`: New tokens `%used%`, `%free%`, `%total%`, `%swap_total%`, `%swap_free%`, and `%swap_used%` that automatically switch between MiB and GiB when below or above 1GiB. ([`#2472`](https://github.com/polybar/polybar/issues/2472), [`#2488`](https://github.com/polybar/polybar/pull/2488))
- `internal/i3`: `show-urgent` option to always show urgent windows when `pin-workspace` is active ([`#2374`](https://github.com/polybar/polybar/issues/2374), [`#2378`](https://github.com/polybar/polybar/pull/2378))
- `internal/xworkspaces`:
  - `reverse-scroll` can be used to reverse the scroll direction when cycling through desktops. ([`#2365`](https://github.com/polybar/polybar/pull/2365))
  - `%nwin%` can be used to display the number of open windows per workspace ([`#604`](https://github.com/polybar/polybar/issues/604), [`#2329`](https://github.com/polybar/polybar/pull/2329))
- Initial support for the backslash escape character (\\) in configs. ([`#2354`](https://github.com/polybar/polybar/issues/2354), [`#2361`](https://github.com/polybar/polybar/pull/2361))
- Warn states for the cpu, memory, fs, and battery modules. ([`#570`](https://github.com/polybar/polybar/issues/570), [`#956`](https://github.com/polybar/polybar/issues/956), [`#1871`](https://github.com/polybar/polybar/issues/1871), [`#2141`](https://github.com/polybar/polybar/issues/2141), [`#2199`](https://github.com/polybar/polybar/pull/2199))
  - `internal/battery`: `format-low`, `label-low`, `animation-low`, `low-at = 10`.
  - `internal/cpu`: `format-warn`, `label-warn`, `warn-percentage = 80`
  - `internal/fs`: `format-warn`, `label-warn`, `warn-percentage = 90`
  - `internal/memory`: `format-warn`, `label-warn`, `warn-percentage = 90`
- `radius` now affects the bar border as well ([`#1566`](https://github.com/polybar/polybar/issues/1566), [`#2359`](https://github.com/polybar/polybar/pull/2359))
- Per-corner radius with `radius-{bottom,top}-{left,right}` ([`#2294`](https://github.com/polybar/polybar/issues/2294), [`#2297`](https://github.com/polybar/polybar/pull/2297))
- `internal/xkeyboard`:
  - `%variant%` token to display the keyboard layout variant ([`#316`](https://github.com/polybar/polybar/issues/316), [`#2163`](https://github.com/polybar/polybar/pull/2163))
  - Allow matching of variants in `layout-icon` ([`#2414`](https://github.com/polybar/polybar/issues/2414), [`#2521`](https://github.com/polybar/polybar/pull/2521))
- Config option to hide a certain module (`hidden = false`) ([`#2108`](https://github.com/polybar/polybar/issues/2108), [`#2342`](https://github.com/polybar/polybar/pull/2342))
- Actions to control visibility of modules (`module_toggle`, `module_show`, and `module_hide`) ([`#2108`](https://github.com/polybar/polybar/issues/2108), [`#2426`](https://github.com/polybar/polybar/pull/2426))
- `internal/backlight`: `use-actual-brightness` option to use the `actual_brightness` file to get the brightness ([`#2380`](https://github.com/polybar/polybar/pull/2380))
- `wm-restack = generic` option that lowers polybar to the bottom of the window stack. Fixes the issue where the bar is being drawn on top of fullscreen windows in xmonad. ([`#2205`](https://github.com/polybar/polybar/issues/2205), [`#2404`](https://github.com/polybar/polybar/pull/2404))
- `internal/bspwm`: `occupied-scroll = true` option allows scrolling through occupied desktops only. ([`#2427`](https://github.com/polybar/polybar/issues/2427), [`#2428`](https://github.com/polybar/polybar/pull/2428))
- `custom/ipc`:
  - `send` action to send arbitrary strings to be displayed in the module. ([`#2455`](https://github.com/polybar/polybar/issues/2455), [`#2463`](https://github.com/polybar/polybar/pull/2463))
  - `hook`, `next`, `prev`, `reset` actions to control the module through actions instead of the deprecated hook messages ([`#2464`](https://github.com/polybar/polybar/issues/2464), [`#2528`](https://github.com/polybar/polybar/pull/2528))
- Added `double-click-interval` setting to the bar section to control the time interval in which a double-click is recognized. Defaults to 400 (ms) ([`#1441`](https://github.com/polybar/polybar/issues/1441), [`#2510`](https://github.com/polybar/polybar/pull/2510))
- Added a new `tray-foreground` setting to give hints to tray icons about what color they should be. ([`#2235`](https://github.com/polybar/polybar/issues/2235), [`#2552`](https://github.com/polybar/polybar/pull/2552))
- `polybar-msg`:
  - For module actions, you can now also specify the module name, action name, and optional data as separate arguments. ([`#2539`](https://github.com/polybar/polybar/pull/2539))
  - Added man page: `man 1 polybar-msg` ([`#2539`](https://github.com/polybar/polybar/pull/2539))

### Changed
- Polybar now also reads `config.ini` when searching for config files. ([`#2323`](https://github.com/polybar/polybar/issues/2323), [`#2324`](https://github.com/polybar/polybar/pull/2324))
- Polybar additionally searches in `XDG_CONFIG_DIRS/polybar/config.ini` (or `/etc/xdg/polybar/config.ini` if it is not set) and `/etc/polybar/config.ini` for config files. ([`#2016`](https://github.com/polybar/polybar/issues/2016), [`#2511`](https://github.com/polybar/polybar/pull/2511))
- We rewrote polybar's main event loop. This shouldn't change any behavior for the user, but be on the lookout for X events, click events, or ipc messages not arriving and the bar not shutting down/restarting properly and let us know if you find any issues. ([`#2384`](https://github.com/polybar/polybar/pull/2384))
- Slight changes to the value ranges the different ramp levels are responsible for in the cpu, memory, fs, and battery modules. The first level is used for everything at and below the start of the value range and the last level for everything at and above the end of the value range. The other levels are evenly distributed over the value range as before. The value range is bounded by the new warning thresholds. ([`#2199`](https://github.com/polybar/polybar/pull/2199))
- `custom/script`: `interval` now defaults to 0 if `tail = true` as per the documentation. ([`#2240`](https://github.com/polybar/polybar/pull/2240))
- `internal/network`: Increased precision for upload and download speeds: 0 decimal places for KB/s (as before), 1 for MB/s and 2 for GB/s. ([`#2054`](https://github.com/polybar/polybar/pull/2054))
- Clicks arriving in close succession, no longer get dropped. Before polybar would drop any click that arrived within 5ms of the previous one. ([`#2510`](https://github.com/polybar/polybar/pull/2510))
- Increased the double click interval from 150ms to 400ms. ([`#2510`](https://github.com/polybar/polybar/pull/2510))
- Stop ignoring actions if they arrive while the previous one hasn't been processed yet. ([`#2469`](https://github.com/polybar/polybar/issues/2469), [`#2517`](https://github.com/polybar/polybar/pull/2517))
- Polybar can now be run without passing the bar name as argument given that the configuration file only defines one bar ([`#2525`](https://github.com/polybar/polybar/issues/2525), [`#2526`](https://github.com/polybar/polybar/pull/2526))
- `include-directory` and `include-file` now support relative paths. The paths are relative to the folder of the file where those directives appear. ([`#2523`](https://github.com/polybar/polybar/issues/2523), [`#2535`](https://github.com/polybar/polybar/issues/2535))
- `custom/ipc`: Empty output strings are no longer formatted. This prevents extraneous spaces and separators from appearing in the bar when the output of an ipc module is empty. ([`#2549`](https://github.com/polybar/polybar/pull/2549))

### Fixed
- Broken positioning in Openbox when the bar is hidden and shown again ([`#2021`](https://github.com/polybar/polybar/issues/2021), [`#2600`](https://github.com/polybar/polybar/pull/2600))
- Handling of action blocks that contain negative offsets ([`#1814`](https://github.com/polybar/polybar/issues/1814), [`#2601`](https://github.com/polybar/polybar/pull/2601))
- `polybar -m` used to show both physical outputs and RandR monitors, even if the outputs were covered by monitors. ([`#2481`](https://github.com/polybar/polybar/issues/2481), [`#2485`](https://github.com/polybar/polybar/pull/2485))
- Parser error if click command contained `}` ([`#2040`](https://github.com/polybar/polybar/issues/2040), [`#2303`](https://github.com/polybar/polybar/pull/2303))
- Some modules stop updating when system time moves backwards. ([`#857`](https://github.com/polybar/polybar/issues/857), [`#1932`](https://github.com/polybar/polybar/issues/1932), [`#2559`](https://github.com/polybar/polybar/pull/2559))
- `custom/script`: Concurrency issues with fast-updating tailed scripts. ([`#1978`](https://github.com/polybar/polybar/issues/1978), [`#2518`](https://github.com/polybar/polybar/pull/2518))
- `internal/alsa`: Slight imprecision when calculating percentages. This caused the volume reported to be off by one. ([`#2399`](https://github.com/polybar/polybar/issues/2399), [`#2401`](https://github.com/polybar/polybar/pull/2401))
- `internal/backlight`: With amdgpu backlights, the brightness indicator was slightly behind. ([`#2367`](https://github.com/polybar/polybar/issues/2367), [`#2380`](https://github.com/polybar/polybar/pull/2380))
- `internal/bspwm`: Warning message regarding T@ ([`#2371`](https://github.com/polybar/polybar/issues/2371), [`#2439`](https://github.com/polybar/polybar/pull/2439))
- `internal/xkeyboard`: Trailing space after the layout label when indicators are empty and made sure right amount of spacing is added between the indicator labels ([`#2292`](https://github.com/polybar/polybar/issues/2292), [`#2306`](https://github.com/polybar/polybar/pull/2306))
- `internal/xworkspaces`:
  - Broken scroll-wrapping and order of workspaces when scrolling ([`#2491`](https://github.com/polybar/polybar/issues/2491), [`#2492`](https://github.com/polybar/polybar/pull/2492))
  - Module would error if WM was not full started up. ([`#1915`](https://github.com/polybar/polybar/issues/1915), [`#2429`](https://github.com/polybar/polybar/pull/2429))
  - Make the urgent hint persistent ([`#1081`](https://github.com/polybar/polybar/issues/1081), [`#2340`](https://github.com/polybar/polybar/pull/2340))
  - Crash when the WM sets -1 for `_NET_WM_DESKTOP` ([`#2352`](https://github.com/polybar/polybar/issues/2352), [`#2353`](https://github.com/polybar/polybar/issues/2353))
- `internal/network`: The module now properly supports 'altnames' for interfaces. ([`#2540`](https://github.com/polybar/polybar/pull/2540))
- `internal/battery`: More accurate battery state ([`#2563`](https://github.com/polybar/polybar/issues/2563), [`#2556`](https://github.com/polybar/polybar/pull/2556))
- Offset tag does not respect current background color ([`#2578`](https://github.com/polybar/polybar/pull/2578), [`#1700`](https://github.com/polybar/polybar/issues/1700))
- Crash when negative margin or padding was specified ([`#2578`](https://github.com/polybar/polybar/pull/2578), [`#1265`](https://github.com/polybar/polybar/issues/1265))

## [3.5.7] - 2021-09-21
### Fixed
- The tray mistakenly removed tray icons that did not support XEMBED
  ([`#2479`](https://github.com/polybar/polybar/issues/2479),
  [`#2442`](https://github.com/polybar/polybar/issues/2442))
- `custom/ipc`: Only the first appearance of the `%pid%` token was replaced
  ([`#2500`](https://github.com/polybar/polybar/issues/2500))

## [3.5.6] - 2021-05-24
### Build
- Support building documentation on sphinx 4.0 ([`#2424`](https://github.com/polybar/polybar/issues/2424))
### Fixed
- Tray icons sometimes appears outside of bar ([`#2430`](https://github.com/polybar/polybar/issues/2430), [`#1679`](https://github.com/polybar/polybar/issues/1679))
- Crash in the i3 module ([`#2416`](https://github.com/polybar/polybar/issues/2416))

## [3.5.5] - 2021-03-01
### Build
- Support older python sphinx versions again ([`#2356`](https://github.com/polybar/polybar/issues/2356))

## [3.5.4] - 2021-01-07
### Fixed
- Wrong text displayed if module text ends with `}` ([`#2331`](https://github.com/polybar/polybar/issues/2331))

## [3.5.3] - 2020-12-23
### Build
- Don't use `git` when building documentation ([`#2309`](https://github.com/polybar/polybar/issues/2309))
### Fixed
- Empty color values are no longer treated as invalid and no longer produce an error.

[Unreleased]: https://github.com/polybar/polybar/compare/3.6.1...HEAD
[3.6.1]: https://github.com/polybar/polybar/releases/tag/3.6.1
[3.6.0]: https://github.com/polybar/polybar/releases/tag/3.6.0
[3.5.7]: https://github.com/polybar/polybar/releases/tag/3.5.7
[3.5.6]: https://github.com/polybar/polybar/releases/tag/3.5.6
[3.5.5]: https://github.com/polybar/polybar/releases/tag/3.5.5
[3.5.4]: https://github.com/polybar/polybar/releases/tag/3.5.4
[3.5.3]: https://github.com/polybar/polybar/releases/tag/3.5.3
