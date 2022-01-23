polybar-msg(1)
==============

SYNOPSIS
--------
| **polybar-msg** [*OPTIONS*] **action** *action-string*
| **polybar-msg** [*OPTIONS*] **action** *module* *action* [*data*]
| **polybar-msg** [*OPTIONS*] **cmd** *command*

DESCRIPTION
-----------
Polybar allows external control through *actions* and *commands*.
Actions control individual modules and commands control the bar itself.

The full IPC documentation is linked at the end of this document.

The available actions depend on the target module.
For actions, the payload is either a single action string or the module name,
the action name, and the optional data string specified separately.

In order for **polybar-msg** being able to send a message to a running
**polybar** process, the bar must have IPC enabled and both **polybar-msg** and
**polybar** must run under the same user.

OPTIONS
-------

.. program:: polybar-msg

.. option:: -h, --help

   Display help text and exit

.. option:: -p PID

   Send message only to **polybar** process running under the given process ID.
   If not specified, the message is sent to all running **polybar** processes.

EXAMPLES
--------

**polybar-msg** **cmd** *quit*
  Terminate all running **polybar** instances.

**polybar-msg** **action** *mymodule* *module_hide*

**polybar-msg** **action** "*#mymodule.module_hide*"
  Hide the module named *mymodule*.
  The first variant specifies the module and action names separately, the second uses an action string.

AUTHORS
-------
| Polybar was created by Michael Carlberg and is currently maintained by Patrick Ziegler.
| Contributors can be listed on GitHub.

REPORTING BUGS
--------------
Report issues on GitHub <https://github.com/polybar/polybar>

SEE ALSO
--------
.. only:: man

  :manpage:`polybar`\(1),
  :manpage:`polybar`\(5)

  | IPC documentation: <https://polybar.rtfd.org/en/stable/user/ipc.html>


.. only:: not man

  :doc:`polybar.1`,
  :doc:`polybar.5`

  :doc:`/user/ipc`
