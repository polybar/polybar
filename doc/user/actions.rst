Actions
=======

.. versionadded:: 3.5.0

.. contents:: Table of Contents
   :local:

"Actions" are used to trigger certain behavior in modules.
For example, when you click on your volume module (pulseaudio or alsa), polybar
internally sends an action to that module that tells it to mute/unmute the
audio.

These actions are not only used internally, but users can also send these
actions to polybar through `Inter Process Communication
<https://github.com/polybar/polybar/wiki/Inter-process-messaging>`_ (IPC) to
trigger certain behavior in polybar modules.

.. _action-string-format:

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

Note that we use the name of the module (``mydate``) and not the type.

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

Available Actions
-----------------

The following modules have actions available. Most of them are already used by
the module by default for click and scroll events.

All Modules
^^^^^^^^^^^

These actions are available to all modules and are prefixed with ``module_``.

:``module_show``, ``module_hide``:
  Shows/Hides a module. The module is still running in the background when
  hidden, it is just not drawn. The starting state can be configured with the
  `hidden` configuration option.

  .. versionadded:: 3.6.0

:``module_toggle``:
  Toggles the visibility of a module.

  .. versionadded:: 3.6.0

internal/date
^^^^^^^^^^^^^

:``toggle``:
  Toggles the date/time format between ``date``/``time`` and
  ``date-alt``/``time-alt``

internal/alsa
^^^^^^^^^^^^^

:``inc``, ``dec``:
  Increases/Decreases the volume by ``interval`` percentage points, where
  ``interval`` is the config setting in the module. Volume changed like this
  will never go above 100%.

  if ``unmute-on-scroll`` is turned on, the sound will also be unmuted when
  this action is called.

:``toggle``:
  Toggles between muted and unmuted.

internal/pulseaudio
^^^^^^^^^^^^^^^^^^^

:``inc``, ``dec``:
  Increases/Decreases the volume by ``interval`` percentage points, where
  ``interval`` is the config setting in the module. Volume changed like this
  will never go above ~153% (if ``use-ui-max`` is set to ``true``) or 100% (if
  not).

  if ``unmute-on-scroll`` is turned on, the sound will also be unmuted when
  this action is called.

:``toggle``:
  Toggles between muted and unmuted.

internal/xbacklight
^^^^^^^^^^^^^^^^^^^

:``inc``, ``dec``:
  Increases/Decreases screen brightness 5 percentage points.

internal/backlight
^^^^^^^^^^^^^^^^^^

:``inc``, ``dec``:
  Increases/Decreases screen brightness 5 percentage points.

internal/xkeyboard
^^^^^^^^^^^^^^^^^^

:``switch``:
  Cycles through configured keyboard layouts.

internal/mpd
^^^^^^^^^^^^

:``play``: Starts playing the current song.
:``pause``: Pauses the current song.
:``stop``: Stops playing.
:``prev``: Starts playing the previous song.
:``next``: Starts playing the next song.
:``repeat``: Toggles repeat mode.
:``single``: Toggles single mode.
:``random``: Toggles random mode.
:``consume``: Toggles consume mode.
:``seek``: *(Has Data)* Seeks inside the current song.

           The data must be of the form ``[+-]N``, where ``N`` is a number
           between 0 and 100.

           If either ``+`` or ``-`` is used, it will seek forward or backward
           from the current position by ``N%`` (relative to the length of the
           song).
           Otherwise it will seek to ``N%`` of the current song.

internal/xworkspaces
^^^^^^^^^^^^^^^^^^^^

:``focus``: *(Has Data)* Switches to the given workspace.

            The data is the index of the workspace that should be selected.
:``next``: Switches to the next workspace. The behavior of this action is
           affected by the ``pin-workspaces`` setting.
:``prev``: Switches to the previous workspace. The behavior of this action is
           affected by the ``pin-workspaces`` setting.

internal/bspwm
^^^^^^^^^^^^^^

:``focus``: *(Has Data)* Switches to the given workspace.

            The data has the form ``N+M``, where ``N`` is the index of the
            monitor and ``M`` the index of the workspace on that monitor.
            Both indices are 0-based and correspond to the position the monitor
            and workspace appear in the output of ``bspc subscribe report``.
:``next``: Switches to the next workspace. The behavior of this action is
           affected by the ``pin-workspaces`` setting.
:``prev``: Switches to the previous workspace. The behavior of this action is
           affected by the ``pin-workspaces`` setting.


internal/i3
^^^^^^^^^^^

:``focus``: *(Has Data)* Switches to the given workspace.

            The data is the name of the workspace defined in the i3 config.
:``next``: Switches to the next workspace. The behavior of this action is
           affected by the ``pin-workspaces`` setting.
:``prev``: Switches to the previous workspace. The behavior of this action is
           affected by the ``pin-workspaces`` setting.

custom/menu
^^^^^^^^^^^

:``open``: *(Has Data)* Opens the given menu level

           The data is a single number specifying which menu level should be
           opened.
:``close``: Closes the menu
:``exec``: *(Has Data)* Executes the command at the given menu element.

           The data has the form ``N-M`` and the action will execute the command
           in ``menu-N-M-exec``.


.. _actions-ipc:

custom/ipc
^^^^^^^^^^

.. versionadded:: 3.6.0

:``send``: *(Has Data)* Replace the contents of the module with the data passed in this action.
:``hook``: *(Has Data)* Trigger the given hook.

           The data is the 0-based index of the hook to trigger.
:``next``: Switches to the next hook and wrap around when the last hook was displayed.
:``prev``: Switches to the previous hook and wrap around when the first hook was displayed.
:``reset``: Reset the module to its startup state: either empty or according to the ``initial`` setting.


Deprecated Action Names
-----------------------

.. deprecated:: 3.5.0

In earlier versions (< 3.5.0) action strings only included information about the
module type.
This meant in bars that contained multiple different modules of the same type,
actions for these modules were sometimes processed by the wrong module with the
same type.

Since version 3.5.0, this no longer happens. However, this also means we had to
change what actions are recognized by polybar modules.

If you explicitly use any polybar action names in your config or any of your
scripts, you are advised to change them, as they may stop working at some point
in the future.
For now polybar still supports the old action names, will convert them to the
appropriate new action name, and will print a warning to help you find old
action names in your config.

If you use the `menu module
<https://github.com/polybar/polybar/wiki/Module:-menu>`_, you most likely use
old action names to open and close the menu (for example ``menu-open-1`` or
``menu-close``).
The ``i3wm-wsnext``, ``i3wm-wsprev``, ``bspwm-desknext``, and ``bspwm-deskprev``
actions, to switch workspaces in i3 and bspwm, may also appear in your config.

Migration
^^^^^^^^^

Updating your config to use the new action names is quite straightforward.

For each action name, consult the table below to find the new action name.
Afterwards build the complete action string as described in
:ref:`action-string-format`.

Please see :ref:`below <menu-example>` for an example of migrating a typical menu module.

+-------------------------+-----------------------+---------------+
|Module Type              |Deprecated Action Name |New Action Name|
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

   Some deprecated action names are suffixed with ``N``, this means that that
   action has some additional data (represented by that ``N``), in the new
   action names this data will appear in exactly the same way, after a period.

.. _menu-example:

Menu Module Example
"""""""""""""""""""

The menu module is the only module where we have to explicitly use actions for
it to work. Because of this, almost everyone will need to update their menu
module to use the new action format.

Below you can see an example of a menu module:

.. code-block:: ini

  [module/apps]
  type = custom/menu

  label-open = Apps

  menu-0-0 = Browsers
  menu-0-0-exec = menu-open-1
  menu-0-1 = Multimedia
  menu-0-1-exec = menu-open-2

  menu-1-0 = Firefox
  menu-1-0-exec = firefox
  menu-1-1 = Chromium
  menu-1-1-exec = chromium

  menu-2-0 = Gimp
  menu-2-0-exec = gimp
  menu-2-1 = Scrot
  menu-2-1-exec = scrot

This module uses two actions: ``menu-open-1`` and ``menu-open-2``.
These are actions with data, the data specifies which level of the menu should
be opened.

Looking at the table, we see that the new action name for ``menu-open-N`` is
``open.N``, where ``.N`` is the data attached to the action.
Putting this together with the name of the module gives us ``#apps.open.1`` and
``#apps.open.2`` as action strings.
Since your menu module likely has a different name, your action strings will
likely not use ``apps``, but the name of your module.

.. code-block:: ini

  [module/apps]
  type = custom/menu

  label-open = Apps

  menu-0-0 = Browsers
  menu-0-0-exec = #apps.open.1
  menu-0-1 = Multimedia
  menu-0-1-exec = #apps.open.2

  menu-1-0 = Firefox
  menu-1-0-exec = firefox
  menu-1-1 = Chromium
  menu-1-1-exec = chromium

  menu-2-0 = Gimp
  menu-2-0-exec = gimp
  menu-2-1 = Scrot
  menu-2-1-exec = scrot
