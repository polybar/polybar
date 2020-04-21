polybar(1)
==========

SYNOPSIS
--------
**polybar** [*OPTIONS*]... *BAR*

DESCRIPTION
-----------
Polybar aims to help users build beautiful and highly customizable status bars for their desktop environment, without the need of having a black belt in shell scripting.

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

   |
   | **$XDG_CONFIG_HOME/polybar/config**
   | **$HOME/.config/polybar/config**
.. option:: -r, --reload

   Reload the application when the config file has been modified
.. option:: -d, --dump=PARAM

   Print the value of the specified parameter *PARAM* in bar section and exit
.. option:: -m, --list-monitors

   Print list of available monitors and exit

   If some monitors are cloned, this will exclude all but one of them
.. option:: -M, --list-all-monitors

   Print list of available monitors and exit

   This will also include all cloned monitors.
.. option:: -w, --print-wmname

   Print the generated *WM_NAME* and exit
.. option:: -s, --stdout

   Output the data to stdout instead of drawing it to the X window
.. option:: -p, --png=FILE

   Save png snapshot to *FILE* after running for 3 seconds

AUTHOR
------
| Michael Carlberg <c@rlberg.se>
| Contributors can be listed on GitHub.

REPORTING BUGS
--------------
Report issues on GitHub <https://github.com/polybar/polybar>

SEE ALSO
--------
| Full documentation at: <https://github.com/polybar/polybar>
| Project wiki: <https://github.com/polybar/polybar/wiki>

.. only:: man

  :manpage:`polybar(5)`

.. only:: not man

  :doc:`polybar.5`
