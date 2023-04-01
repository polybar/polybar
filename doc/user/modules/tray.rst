.. include:: defs.rst

Tray Module
===========

.. versionadded:: 3.7.0

The tray module displays system tray application icons on the bar.

This module is a bit different from the other modules.
The tray icons (also called clients) are individual windows managed by their
respective application (e.g. the Dropbox tray icon is created and managed by
the Dropbox application).
Polybar is only responsible for embedding the windows in the bar and
positioning them correctly.

.. note::

   Only a single instance of this module can be active at the same time (across
   all polybar instances).

   The way the `system tray protocol <systray-spec_>`_ works, at most one tray
   can exist at any time.
   Polybar will produce a warning if additional tray instances are created.

For transparent background colors, the tray will use pseudo-transparency, true
transparency is not possible for the tray icons.

TODO mention the ``type`` setting somewhere

Formats
-------

The module only has a single format:

.. setting:: format

  :type: |type-format|
  :tags: ``<tray>``: Shows tray icons
  :default: ``<tray>``

Settings
--------

.. setting:: tray-spacing

  Space added between tray icons

  :type: |type-extent|, non-negative
  :default: ``0px``

.. setting:: tray-padding

  Space added before and after each tray icon

  :type: |type-extent|, non-negative
  :default: ``0px``

.. setting:: tray-size

  Size of individual tray icons

  Relative to bar height

  :type: |type-pwo|, non-negative
  :default: 66%

.. setting:: tray-background

  Background color of tray icons

  .. note::
    This only affects the color of the individual icons and not the space in
    between, changing this setting will likely not look good.

  :type: |type-color|
  :default: ``${root.background}``

.. setting:: tray-foreground

  Tray icon color hint

  This serves as a hint to the tray icon application what color to use for the
  icon.

  This is not guaranteed to have any effect (likely only in GTK3) because it
  targets a non-standard part of the `system tray protocol <systray-spec_>`_ by
  setting the ``_NET_SYSTEM_TRAY_COLORS`` atom on the tray window.

  :type: |type-color|
  :default: ``${tray-foreground}``


Example
-------

::

  [module/tray]
  type = internal/tray

  format-margin = 8px
  tray-spacing = 8px

References
----------

* `System Tray Protocol Specification <systray-spec_>`_
* `XEmbed Protocol Specification <xembed_>`_

.. _systray-spec: https://specifications.freedesktop.org/systemtray-spec/systemtray-spec-latest.html
.. _xembed: https://specifications.freedesktop.org/xembed-spec/xembed-spec-latest.html
