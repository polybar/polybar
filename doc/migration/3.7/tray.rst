..
  File included directly in other pages describing migrating to the new tray module

Polybar version 3.7 introduced the new tray module and deprecated the legacy
tray implementation which uses ``tray-position`` to position the tray.
You should switch over to the tray module as soon as possible.

The legacy tray was configured in the `bar section
<https://github.com/polybar/polybar/wiki/Configuration#bar-settings>`_, the
setting for the module live in that module's section of the config file.
The settings in the bar section don't always directly correspond to an
equivalent setting in the module section for the new tray module.

The following lists how each old setting in the bar section should be migrated:

``tray-position``
  The tray is now positioned as a module and so its positioning is done by
  placing it where you want it to appear in one of the three module lists
  ``modules-left``, ``modules-center``, ``modules-right``.

``tray-detached``
  This setting does not have an equivalent, detaching the tray is no longer
  possible.

``tray-maxsize``
  The :poly-setting:`tray-size` setting now determines the size of tray icons.

``tray-transparent``
  Was already deprecated and does not exist in the tray module.
  Transparency is enabled automatically if a transparent background is used.

``tray-background``
  Also exists in the module section (see :poly-setting:`tray-background`). Now,
  the setting only applies to the icons themselves and no longer to the space
  around them.

``tray-foreground``
  Also exists in the module section with the same functionality (see
  :poly-setting:`tray-foreground`).

``tray-offset-x``, ``tray-offset-y``
  Has no direct equivalent in the module settings. Horizontally, the tray can
  be moved in the same way other module content can be moved; by reordering the
  modules or using things like ``format-offset``, ``format-margin``, or
  ``format-padding``.
  The tray can't be moved vertically.

  In any case, the tray can no longer be moved outside of the bar window.

``tray-padding``
  Spacing between tray icons works a bit different now and needs to be
  completely reconfigured (see :poly-setting:`tray-padding` and
  :poly-setting:`tray-spacing`).

``tray-scale``
  No longer exist. The size of the icons is solely determined by
  :poly-setting:`tray-size`.
