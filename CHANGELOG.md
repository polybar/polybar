# Changelog
All notable changes to this project will be documented in this file.
Each release should have the following subsections, if entries exist, in the
given order: `Breaking`, `Build`, `Deprecated`, `Removed`, `Added`, `Changed`,
`Fixed`, `Security`.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]
### Breaking
- We rewrote our tag parser. This shouldn't break anything, if you experience
  any problems, please let us know.
  The new parser now gives errors for certain invalid tags where the old parser
  would just silently ignore them. Adding extra text to the end of a valid tag
  now produces an error. For example, tags like `%{T-a}`, `%{T2abc}`, `%{rfoo}`,
  and others will now start producing errors.
  This does not affect you unless you are producing your own formatting tags
  (for example in a script) and you are using one of these invalid tags.

### Added
- Warn states for the cpu, memory, fs, and battery modules.
  ([`#570`](https://github.com/polybar/polybar/issues/570),
  [`#956`](https://github.com/polybar/polybar/issues/956),
  [`#1871`](https://github.com/polybar/polybar/issues/1871),
  [`#2141`](https://github.com/polybar/polybar/issues/2141))
  - `internal/battery`: `format-low`, `label-low`, `animation-low`, `low-at =
    10`.
  - `internal/cpu`: `format-warn`, `label-warn`, `warn-percentage = 80`
  - `internal/fs`: `format-warn`, `label-warn`, `warn-percentage = 90`
  - `internal/memory`: `format-warn`, `label-warn`, `warn-percentage = 90`
- Per-corner corner radius with `radius-{bottom,top}-{left,right}`
  ([`#2294`](https://github.com/polybar/polybar/issues/2294))
- `internal/network`: `speed-unit = B/s` can be used to customize how network
  speeds are displayed.
- `internal/xkeyboard`: `%variant%` can be used to parse the layout variant
  ([`#316`](https://github.com/polybar/polybar/issues/316))

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

[Unreleased]: https://github.com/polybar/polybar/compare/3.5.2...HEAD
