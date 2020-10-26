Actions
=======

.. versionadded:: 3.5.0

"Actions" are used to trigger certain behavior in modules.
For example, when you click on your volume module (pulseaudio or alsa), polybar
internally sends an action to that module that tells it to mute/unmute the
audio.

These actions are not only used internally, but users can also send these
actions to polybar through `Inter Process Communication
<https://github.com/polybar/polybar/wiki/Inter-process-messaging>`_ (IPC) to
trigger certain behavior in polybar modules.

Action String Format
--------------------

An action string follows the following format:

::

  #NAME.ACTION[.DATA]

Where ``NAME`` is the name of the target module (not the type!) and ``ACTION``
is the name of the action in that module. ``DATA`` is optional data attached to
an action (for example to say which menu level should be opened).

For example the
`date module <https://github.com/polybar/polybar/wiki/Module:-date>`_ supports
the ``toggle`` action to toggle between the regular and the alternative time and
date format.
If you have the following date module:

.. code-block:: ini

  [module/mydate]
  type = internal/date
  ...

The action string for toggling between the date formats would look like this:

::

  #mydate.toggle

As an example for an action string with additional data, take the menu module:

.. code-block:: ini

  [module/powermenu]
  type = custom/menu
  menu-0-0 = Poweroff
  menu-0-0-exec = poweroff
  menu-0-1 = Suspend
  menu-0-1-exec = systemctl suspend

The action name to open a certain menu level is ``open``, so to open level 0
(`menu-0`), the action string additionally has the level attached to it:

::

  #powermenu.open.0

Triggering Actions
------------------

Most modules already use action strings to trigger actions when you click on or
scroll over a module.
But in some cases you may want or need to manually send action strings to
polybar to trigger a certain behavior.

Everywhere where you can specify a command to run on click or scroll, you can
also specify an action string.
For example, in the bar section, you can specify a command that is triggered
when you click anywhere on the bar (where there isn't another click action):

.. code-block:: ini

  [bar/mybar]
  ...
  click-left = #mydate.toggle
  ...

This will then trigger the ``toggle`` action on the ``mydate`` module when you
click anywhere on the bar.

Similarly, we can use action strings in ``%{A}``
`formatting tags <https://github.com/polybar/polybar/wiki/Formatting#action-a>`_
just as we would regular commands:

::

  %{A1:firefox:}%{A3:#mydate.toggle:}Opens firefox on left-click and toggles the
  date on right-click %{A}%{A}

Finally, polybar's `Inter Process Communication
<https://github.com/polybar/polybar/wiki/Inter-process-messaging>`_ (IPC) can
also be used to trigger actions:

.. code-block:: bash

  polybar-msg action "#mydate.toggle"

.. note::

  The quotes around the action string are necessary, otherwise your shell may
  interpret the ``#`` as the beginning of the comment and ignore the rest of the
  line.

Supported Actions
-----------------

.. TODO Finish table. Maybe one section per module type (table not really convenient)

+-------------------------+---------------+----+---------------------+
|Module Type              |New Action Name|Data|Description          |
+=========================+===============+====+=====================+
|``internal/date``        |``toggle``     |no  ||date_toggle_desc|   |
+-------------------------+---------------+----+---------------------+
|``internal/alsa``        |``inc``        |no  |                     |
|                         +---------------+----+---------------------+
|                         |``dec``        |no  |                     |
|                         +---------------+----+---------------------+
|                         |``toggle``     |no  |                     |
+-------------------------+---------------+----+---------------------+
|``internal/pulseaudio``  |``inc``        |no  |                     |
|                         +---------------+----+---------------------+
|                         |``dec``        |no  |                     |
|                         +---------------+----+---------------------+
|                         |``toggle``     |no  |                     |
+-------------------------+---------------+----+---------------------+
|``internal/xbacklight``  |``inc``        |no  |                     |
|                         +---------------+----+---------------------+
|                         |``dec``        |no  |                     |
+-------------------------+---------------+----+---------------------+
|``internal/backlight``   |``inc``        |no  |                     |
|                         +---------------+----+---------------------+
|                         |``dec``        |no  |                     |
+-------------------------+---------------+----+---------------------+
|``internal/xkeyboard``   |``switch``     |no  |                     |
+-------------------------+---------------+----+---------------------+
|``internal/mpd``         |``play``       |no  |                     |
|                         +---------------+----+---------------------+
|                         |``pause``      |no  |                     |
|                         +---------------+----+---------------------+
|                         |``stop``       |no  |                     |
|                         +---------------+----+---------------------+
|                         |``prev``       |no  |                     |
|                         +---------------+----+---------------------+
|                         |``next``       |no  |                     |
|                         +---------------+----+---------------------+
|                         |``repeat``     |no  |                     |
|                         +---------------+----+---------------------+
|                         |``single``     |no  |                     |
|                         +---------------+----+---------------------+
|                         |``random``     |no  |                     |
|                         +---------------+----+---------------------+
|                         |``consume``    |no  |                     |
|                         +---------------+----+---------------------+
|                         |``seek``       |yes |                     |
+-------------------------+---------------+----+---------------------+
|``internal/xworkspaces`` |``focus``      |yes |                     |
|                         +---------------+----+---------------------+
|                         |``next``       |no  |                     |
|                         +---------------+----+---------------------+
|                         |``prev``       |no  |                     |
+-------------------------+---------------+----+---------------------+
|``internal/bspwm``       |``focus``      |yes |                     |
|                         +---------------+----+---------------------+
|                         |``next``       |no  |                     |
|                         +---------------+----+---------------------+
|                         |``prev``       |no  |                     |
+-------------------------+---------------+----+---------------------+
|``internal/i3``          |``focus``      |yes |                     |
|                         +---------------+----+---------------------+
|                         |``next``       |no  |                     |
|                         +---------------+----+---------------------+
|                         |``prev``       |no  |                     |
+-------------------------+---------------+----+---------------------+
|``custom/menu``          |``open``       |yes |                     |
|                         +---------------+----+---------------------+
|                         |``close``      |no  |                     |
|                         +---------------+----+---------------------+
|                         |``exec``       |yes |                     |
+-------------------------+---------------+----+---------------------+

.. |date_toggle_desc| replace:: Toggles the date/time format between ``date``/``time`` and ``date-alt``/``time-alt``.

Legacy Action Names
-------------------

In earlier versions (< 3.5.0) action strings only included information about the
module type.
This meant in bars that contained multiple different modules of the same type,
actions for these modules were sometimes processed by the wrong module with the
same type.

Since version 3.5.0, this no longer happens. However, this also means we had to
change what actions are recognized by polybar modules.

If you explicitly use any polybar action names in your config or any of your
scripts, you are advised to change them, as they may stop working at some point.
For now polybar still supports the old action names, will convert them to the
appropriate new action name, and will print a warning to help you find old
action names in your config.

If you use the `menu module
<https://github.com/polybar/polybar/wiki/Module:-menu>`_, you most likely use
old action names to open and close the menu (for example ``menu-open-1`` or
``menu-close``).
The ``i3wm-wsnext``, ``i3wm-wsprev``, ``bspwm-desknext``, and ``bspwm-deskprev``
actions, to switch workspaces in i3 and bspwm, may also appear in your config.

Migration to New Action Strings
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

+-------------------------+-----------------------+---------------+
|Module Type              |Legacy Action Name     |New Action Name|
+=========================+=======================+===============+
|``internal/date``        |``datetoggle``         |``toggle``     |
+-------------------------+-----------------------+---------------+
|``internal/alsa``        |``volup``              |``inc``        |
|                         +-----------------------+---------------+
|                         |``voldown``            |``dec``        |
|                         +-----------------------+---------------+
|                         |``volmute``            |``toggle``     |
+-------------------------+-----------------------+---------------+
|``internal/pulseaudio``  |``pa_volup``           |``inc``        |
|                         +-----------------------+---------------+
|                         |``pa_voldown``         |``dec``        |
|                         +-----------------------+---------------+
|                         |``pa_volmute``         |``toggle``     |
+-------------------------+-----------------------+---------------+
|``internal/xbacklight``  |``xbacklight+``        |``inc``        |
|                         +-----------------------+---------------+
|                         |``xbacklight-``        |``dec``        |
+-------------------------+-----------------------+---------------+
|``internal/backlight``   |``backlight+``         |``inc``        |
|                         +-----------------------+---------------+
|                         |``backlight-``         |``dec``        |
+-------------------------+-----------------------+---------------+
|``internal/xkeyboard``   |``xkeyboard/switch``   |``switch``     |
+-------------------------+-----------------------+---------------+
|``internal/mpd``         |``mpdplay``            |``play``       |
|                         +-----------------------+---------------+
|                         |``mpdpause``           |``pause``      |
|                         +-----------------------+---------------+
|                         |``mpdstop``            |``stop``       |
|                         +-----------------------+---------------+
|                         |``mpdprev``            |``prev``       |
|                         +-----------------------+---------------+
|                         |``mpdnext``            |``next``       |
|                         +-----------------------+---------------+
|                         |``mpdrepeat``          |``repeat``     |
|                         +-----------------------+---------------+
|                         |``mpdsingle``          |``single``     |
|                         +-----------------------+---------------+
|                         |``mpdrandom``          |``random``     |
|                         +-----------------------+---------------+
|                         |``mpdconsume``         |``consume``    |
|                         +-----------------------+---------------+
|                         |``mpdseekN``           |``seek.N``     |
+-------------------------+-----------------------+---------------+
|``internal/xworkspaces`` |``xworkspaces-focus=N``|``focus.N``    |
|                         +-----------------------+---------------+
|                         |``xworkspaces-next``   |``next``       |
|                         +-----------------------+---------------+
|                         |``xworkspaces-prev``   |``prev``       |
+-------------------------+-----------------------+---------------+
|``internal/bspwm``       |``bspwm-deskfocusN``   |``focus.N``    |
|                         +-----------------------+---------------+
|                         |``bspwm-desknext``     |``next``       |
|                         +-----------------------+---------------+
|                         |``bspwm-deskprev``     |``prev``       |
+-------------------------+-----------------------+---------------+
|``internal/i3``          |``i3wm-wsfocus-N``     |``focus.N``    |
|                         +-----------------------+---------------+
|                         |``i3-wsnext``          |``next``       |
|                         +-----------------------+---------------+
|                         |``i3-wsprev``          |``prev``       |
+-------------------------+-----------------------+---------------+
|``custom/menu``          |``menu-open-N``        |``open.N``     |
|                         +-----------------------+---------------+
|                         |``menu-close``         |``close``      |
+-------------------------+-----------------------+---------------+

.. note::

   Some legacy action names are suffixed with ``N``, this means that action has
   some additional data (represented by that ``N``), in the new action names,
   this data will appear in exactly the same way, after a period.

.. TODO show how to migrate
