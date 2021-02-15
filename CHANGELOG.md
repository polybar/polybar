# Changelog
All notable changes to this project will be documented in this file.
Each release should have the following subsections, if entries exist, in the
given order: `Breaking`, `Build`, `Deprecated`, `Removed`, `Added`, `Changed`,
`Fixed`, `Security`.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]
### Breaking
- We added the backslash escape character (\\) for configuration values. This
  means that the literal backslash character now has special meaning in
  configuration files, therefore if you want to use it in a value as a literal
  backslash, you need to escape it with the backslash escape character. The
  parser logs an error if any unescaped backslashes are found in a value. This
  affects you only if you are using two consecutive backslashes in a value,
  which will now be interpreted as a single literal backslash.
  [`#2354`](https://github.com/polybar/polybar/issues/2354)
- We rewrote our tag parser. This shouldn't break anything, if you experience
  any problems, please let us know.
  The new parser now gives errors for certain invalid tags where the old parser
  would just silently ignore them. Adding extra text to the end of a valid tag
  now produces an error. For example, tags like `%{T-a}`, `%{T2abc}`, `%{rfoo}`,
  and others will now start producing errors.
  This does not affect you unless you are producing your own formatting tags
  (for example in a script) and you are using one of these invalid tags.

### Build
- Bump the minimum cmake version to 3.5
- The `BUILD_IPC_MSG` option has been renamed to `BUILD_POLYBAR_MSG`
- Building the documentation is now enabled by default and not just when
  `sphinx-build` is found.
- Users can control exactly which targets should be available with the following
  cmake options (together with their default value):
  - `BUILD_POLYBAR=ON` - Builds the `polybar` executable
  - `BUILD_POLYBAR_MSG=ON` - Builds the `polybar-msg` executable
  - `BUILD_TESTS=OFF` - Builds the test suite
  - `BUILD_DOC=ON` - Builds the documentation
  - `BUILD_DOC_HTML=BUILD_DOC` - Builds the html documentation (depends on `BUILD_DOC`)
  - `BUILD_DOC_MAN=BUILD_DOC` - Builds the manpages (depends on `BUILD_DOC`)
  - `BUILD_CONFIG=ON` - Generates sample config
  - `BUILD_SHELL=ON` - Generates shell completion files
  - `DISABLE_ALL=OFF` - Disables all above targets by default. Individual
    targets can still be enabled explicitly.
- The documentation can no longer be built by directly configuring the `doc`
  directory.
- The sample config file is now placed in the `generated-sources` folder inside
  whatever folder you invoked `cmake` from instead of in the root folder of the
  repository.

### Added
- `internal/xworkspaces`: `reverse-scroll` can be used to reverse the scroll
  direction when cycling through desktops.
- The backslash escape character (\\).
  [`#2354`](https://github.com/polybar/polybar/issues/2354)
- Warn states for the cpu, memory, fs, and battery modules.
  ([`#570`](https://github.com/polybar/polybar/issues/570),
  [`#956`](https://github.com/polybar/polybar/issues/956),
  [`#1871`](https://github.com/polybar/polybar/issues/1871),
  [`#2141`](https://github.com/polybar/polybar/issues/2141))
  - `internal/battery`: `format-low`, `label-low`, `animation-low`, `low-at = 10`.
  - `internal/cpu`: `format-warn`, `label-warn`, `warn-percentage = 80`
  - `internal/fs`: `format-warn`, `label-warn`, `warn-percentage = 90`
  - `internal/memory`: `format-warn`, `label-warn`, `warn-percentage = 90`
- Per-corner corner radius with `radius-{bottom,top}-{left,right}`
  ([`#2294`](https://github.com/polybar/polybar/issues/2294))
- `internal/network`: `speed-unit = B/s` can be used to customize how network
  speeds are displayed.
- `internal/xkeyboard`: `%variant%` can be used to parse the layout variant
  ([`#316`](https://github.com/polybar/polybar/issues/316))
- Added .ini extension check to the default config search.
  ([`#2323`](https://github.com/polybar/polybar/issues/2323))
- IPC commands to change visibility of modules
  (`hide.<name>`, `show.<name>`, and `toggle.<name>`)
  ([`#2108`](https://github.com/polybar/polybar/issues/2108))
- Config option to hide a certain module
  (`hidden = false`)
  ([`#2108`](https://github.com/polybar/polybar/issues/2108))
- `internal/xworkspaces`: Make the urgent hint persistent
  ([`#1081`](https://github.com/polybar/polybar/issues/1081))
- `internal/network`: `interface-type` may be used in place of `interface` to
  automatically select a network interface
  ([`#2025`](https://github.com/polybar/polybar/pull/2025))
- `internal/xworkspaces`: `%nwin%` can be used to display the number of open
  windows per workspace
  ([`#604`](https://github.com/polybar/polybar/issues/604))
- `internal/backlight`: added `use-actual-brightness` option

### Changed
- Slight changes to the value ranges the different ramp levels are responsible
  for in the cpu, memory, fs, and battery modules. The first and last level are
  only used for everything at or below and at and above the edges of the value
  range, respectively. The other levels are evenly distributed over the value
  range as before.
- `custom/script`: `interval` now defaults to 0 if `tail = true` as per the
  documentation.
- `internal/network`:
  - Increased precision for upload and download speeds: 0 decimal places for
    KB/s (as before), 1 for MB/s and 2 for GB/s.

### Fixed
- Trailing space after the layout label when indicators are empty and made sure right amount
  of spacing is added between the indicator labels, in the xkeyboard module.
  ([`#2292`](https://github.com/polybar/polybar/issues/2292))
- Parser error if click command contained `}`
  ([`#2040`](https://github.com/polybar/polybar/issues/2040))
- With amdgpu backlights, the brightness indicator was slightly behind.
  ([`#2367](https://github.com/polybar/polybar/issues/2367))

## [3.5.4] - 2021-01-07
### Fixed
- Wrong text displayed if module text ends with `}` ([`#2331`](https://github.com/polybar/polybar/issues/2331))

## [3.5.3] - 2020-12-23
### Build
- Don't use `git` when building documentation ([`#2309`](https://github.com/polybar/polybar/issues/2309))
### Fixed
- Empty color values are no longer treated as invalid and no longer produce an error.

[Unreleased]: https://github.com/polybar/polybar/compare/3.5.4...HEAD
[3.5.4]: https://github.com/polybar/polybar/releases/tag/3.5.4
[3.5.3]: https://github.com/polybar/polybar/releases/tag/3.5.3
