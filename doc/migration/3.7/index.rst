Migrating From Version 3.6 to 3.7
=================================

Text Module (``custom/text``)
-----------------------------

Using ``content`` to specify the text of the module is deprecated in favor of
using the same concepts as all other modules (formats and labels).

For example, the following text module:

.. code-block:: dosini

  [module/text]
  type = custom/text
  content = Hello World
  content-foreground = #ff0000

Should now look like this:

.. code-block:: dosini

  [module/text]
  type = custom/text
  label = Hello World
  label-foreground = #ff0000

Because it is set to its default value, the ``format`` setting can also be
completely left out.

In general, all properties on ``content`` also apply the same on ``label``
(e.g. ``background``, ``font``), except for ``offset``,
``prefix``, ``suffix`` (and their sub-properties).
Those three properties have to instead be applied to ``format`` (e.g.
``format-offset``).

System Tray
-----------

.. include:: tray.rst
