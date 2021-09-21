# Changelog

All notable changes to this project will be documented in this file.
Each release should have the following subsections, if entries exist, in the
given order: `Breaking`, `Build`, `Deprecated`, `Removed`, `Added`, `Changed`,
`Fixed`, `Security`.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]
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

[Unreleased]: https://github.com/polybar/polybar/compare/3.5.7...HEAD
[3.5.7]: https://github.com/polybar/polybar/releases/tag/3.5.7
[3.5.6]: https://github.com/polybar/polybar/releases/tag/3.5.6
[3.5.5]: https://github.com/polybar/polybar/releases/tag/3.5.5
[3.5.4]: https://github.com/polybar/polybar/releases/tag/3.5.4
[3.5.3]: https://github.com/polybar/polybar/releases/tag/3.5.3
