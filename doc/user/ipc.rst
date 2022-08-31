Inter-process-messaging
=======================

Polybar supports controlling parts of the bar and its modules from the outside
through inter-process-messaging (IPC).

IPC is disabled by default and can be enabled by setting ``enable-ipc = true``
in the bar section.

By default polybar ships with the ``polybar-msg`` tool that is needed to send
messages to polybar.

.. note:: Starting with version 3.6.0, the underlying IPC mechanism has been
          completely changed.

          Writing directly to the named pipe to send IPC messages has been
          deprecated, ``polybar-msg`` should be used exclusively
          Everything you could do by directly writing to the named pipe, you
          can also do using ``polybar-msg``.
          In addition, hook messages are also deprecated; they are replaced by
          actions on the :ref:`ipc module <actions-ipc>`.

          Unless noted otherwise, everything in this guide is still valid for
          older versions.

Sending Messages
----------------

``polybar-msg`` can be called on the commandline like this:

.. code-block:: shell

  polybar-msg [-p <pid>] <type> <payload>

If the ``-p`` argument is specified, the message is only sent to the running
polybar instance with the given process ID.
Otherwise, the message is sent to all running polybar processes that have IPC
enabled.

.. note:: IPC messages are only sent to polybar instances running under the
          same user as ``polybar-msg`` is running as.

          Concretely, ``polybar`` and ``polybar-msg`` use the
          ``$XDG_RUNTIME_DIR`` environment variable in accordance with the `XDG
          Base Directory Specification`_ to determine where to find the socket
          to communicate.

          If ``polybar`` and ``polybar-msg`` don't have the same value for
          ``$XDG_RUNTIME_DIR``, they will likely not be able to communicate.
          The variable may not be set if you use ``su`` or ``sudo`` to execute
          ``polybar-msg`` as a different user, often a full user session is
          required.

          .. _XDG Base Directory Specification: https://specifications.freedesktop.org/basedir-spec/basedir-spec-latest.html

The ``<type>`` argument is either :ref:`action <ipc-actions>` or
:ref:`cmd <ipc-commands>`.
The allowed values for ``<payload>`` depend on the type.

Message Types
-------------

.. _ipc-commands:

Commands
^^^^^^^^

Using ``cmd`` for ``<type>``, you can control certain aspects of the bar.

Available values for ``<payload>`` are:

* ``quit``: Terminates the bar
* ``restart``: Restarts the bar in-place
* ``hide``: Hides the bar
* ``show``: Makes the bar visible again, if it was hidden
* ``toggle``: Toggles between the hidden and visible state.

.. _ipc-actions:

Module Actions
^^^^^^^^^^^^^^

For the ``<type>`` ``action``, ``polybar-msg`` can execute
:doc:`module actions <actions>` in the bar.

An action consists of the name of the target module, the name of the action and an optional data string:

::

  #<modulename>.<actionname>[.<data>]

More information about action strings and available actions can be found in
:doc:`actions`

For example, if you have a date module named ``date``, you can toggle between
the regular and alternative label with:

.. code-block:: shell

  polybar-msg action "#date.toggle"

As an example for an action with data, say you have a menu module named
``powermenu``, you can open the menu level 0 using:

.. code-block:: shell

  polybar-msg action "#powermenu.open.0"


.. note::

  For convenience, ``polybar-msg`` also allows you to pass the module name,
  action name, and data as separate arguments:

  .. code-block:: shell

    polybar-msg action date toggle
    polybar-msg action powermenu open 0

  .. versionadded:: 3.6.0
