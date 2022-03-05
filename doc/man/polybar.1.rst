polybar(1)
==========

SYNOPSIS
--------
**polybar** [*OPTIONS*]... [*BAR*]

DESCRIPTION
-----------
Polybar aims to help users build beautiful and highly customizable status bars for their desktop environment, without the need of having a black belt in shell scripting.
If the *BAR* argument is not provided and the configuration file only contains one bar definition, polybar will display this bar.

OPTIONS
-------

.. program:: polybar

.. option:: -h, --help

   Display help text and exit

.. option:: -v, --version

   Display build details and exit
.. option:: -l, --log=LEVEL

   | Set the logging verbosity (default: **notice**)
   | *LEVEL* is one of: error, warning, notice, info, trace
.. option:: -q, --quiet

   Be quiet (will override -l)
.. option:: -c, --config=FILE

   Specify the path to the configuration file. By default, the configuration file is loaded from:

   * ``$XDG_CONFIG_HOME/polybar/config``
   * ``$XDG_CONFIG_HOME/polybar/config.ini``
   * ``$HOME/.config/polybar/config``
   * ``$HOME/.config/polybar/config.ini``
   * ``$XDG_CONFIG_DIRS/polybar/config.ini``
   * ``/etc/xdg/polybar/config.ini`` (only if ``XDG_CONFIG_DIRS`` is not set)
   * ``/etc/polybar/config.ini``
.. option:: -r, --reload

   Reload the application when the config file has been modified
.. option:: -d, --dump=PARAM

   Print the value of the specified parameter *PARAM* in bar section and exit
.. option:: -m, --list-monitors

   | Print list of available monitors and exit.
   | If some monitors are cloned, this will exclude all but one of them.
   | If polybar was compiled with RandR monitor support, only monitors are listed and not physical outputs.
.. option:: -M, --list-all-monitors

   | Print list of all available monitors and exit.
   | This includes cloned monitors as well as both physical outputs and RandR monitors (if supported).
   | Only the names listed here can be used as monitor names in polybar.
.. option:: -w, --print-wmname

   Print the generated *WM_NAME* and exit
.. option:: -s, --stdout

   Output the data to stdout instead of drawing it to the X window
.. option:: -p, --png=FILE

   Save png snapshot to *FILE* after running for 3 seconds

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

  :manpage:`polybar-msg`\(1),
  :manpage:`polybar`\(5)


.. only:: not man

  :doc:`polybar-msg.1`,
  :doc:`polybar.5`

| Full documentation at: <https://github.com/polybar/polybar>
| Project wiki: <https://github.com/polybar/polybar/wiki>
