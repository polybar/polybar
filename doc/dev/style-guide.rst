Style Guide
===========

There is a ``.editorconfig`` and a ``.clang-format`` file in the project root
that defines some basic guidelines, mainly relating to indentation.

Code Formatting
---------------

We use ``clang-format`` for code formatting, the style rules are defined in
``.clang-format``, before submitting a PR, make sure to run the following command
on all the C++ files you changed:

.. code-block:: shell

  clang-format -style=file -i <FILES>

**Note:** Depending on which file you change, this may produce a lot of changes
because we have not run ``clang-format`` on all files in the project. This is
fine.

Indentation
~~~~~~~~~~~

Files use 2 spaces for indentation.

Line Width
~~~~~~~~~~

Lines should not be longer than 120 characters, ``clang-format`` will enforce
this when run. However, try to keep lines under 80 characters if it seems
reasonable in the current situation.

In some cases it makes sense to have lines longer than 80 characters for
readability. But long lines can just the same be unreadable, for example if you
have long if-conditions or use complex expressions as function parameters. Make
sure you only use longer lines if keeping it under 80 would be less readable.

Comments
--------

Use Doxygen ``/** */`` comments in front of functions, methods, types, members and
classes:

.. code-block:: cpp

  /**
   * @brief Generates a config object from a config file
   *
   * For modularity the parsing and storing of the config is separated
   */
  class config_parser {
  ...
    /**
     * @brief Is used to resolve ${root...} references
     */
    string m_barname;
  ...
  }

For all other comments use ``//`` for single-line and ``/* */`` for multi-line comments.

Your comments should describe the intent and purpose of your code, not necessarily what it does.

Header Files
------------

Header files should end in ``.hpp``.

We use pragmas instead of include guards to guarantee header files are included
only once:

.. code-block:: cpp

  #pragma once
