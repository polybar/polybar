Actions
=======

.. versionadded:: 3.5.0

"Actions" are used to trigger certain behavior in modules.
For example, when you click on your volume module (pulseaudio or alsa), polybar
internally sends an action to that module that tells it to mute/unmute the
audio.

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

Finally, polybar's
`Inter Process Messaging <https://github.com/polybar/polybar/wiki/Inter-process-messaging>`_
(IPC) can also be used to trigger actions:

.. code-block:: bash

  polybar-msg action "#mydate.toggle"

.. note::

  The quotes around the action string are necessary, otherwise your shell will
  interpret the ``#`` as the beginning of the comment and ignore the rest of the
  line.

Supported Actions
-----------------


Legacy Action Names
-------------------

Before actions included the name of the module it should be sent to, action
strings only included information about the module type.
This meant for bars that contained multiple different modules of the same type,
actions for these modules were sometimes processed by the wrong module with the
same type.

Migration to New Action Strings
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

+-------------------------+------------------+---------------+
|Module Type              |Legacy Action Name|New Action Name|
+=========================+==================+===============+
|``internal/date``        |``datetoggle``    |``toggle``     |
+-------------------------+------------------+---------------+
|``internal/alsa``        |``volup``         |``inc``        |
|                         +------------------+---------------+
|                         |``voldown``       |``dec``        |
|                         +------------------+---------------+
|                         |``volmute``       |``toggle``     |
+-------------------------+------------------+---------------+
|``internal/pulseaudio``  |                  |               |
+-------------------------+------------------+---------------+
|``internal/xbacklight``  |                  |               |
+-------------------------+------------------+---------------+
|``internal/backlight``   |                  |               |
+-------------------------+------------------+---------------+
|``internal/xkeyboard``   |                  |               |
+-------------------------+------------------+---------------+
|``internal/mpd``         |                  |               |
+-------------------------+------------------+---------------+
|``internal/xworkspaces`` |                  |               |
+-------------------------+------------------+---------------+
|``internal/bspwm``       |                  |               |
+-------------------------+------------------+---------------+
|``internal/i3``          |                  |               |
+-------------------------+------------------+---------------+
|``custom/menu``          |                  |               |
+-------------------------+------------------+---------------+
