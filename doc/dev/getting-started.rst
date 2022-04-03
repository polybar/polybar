Getting Started
===============

Setting up polybar for development is basically the same process as `compiling
it from source <https://github.com/polybar/polybar/wiki/Compiling>`_.
However, we recommend using the ``Debug`` or ``Sanitize`` cmake build type when
configuring the project:

.. code-block:: shell

  cmake -DCMAKE_BUILD_TYPE=Debug ..
  # Or
  cmake -DCMAKE_BUILD_TYPE=Sanitize ..

This will give you debug symbols in the executable and the ``Sanitize`` build
type will also enable the ``AddressSanitizer`` and
``UndefinedBehaviorSanitizer``, which can give you very useful information
about crashes and undefined behavior at runtime.

Editors
-------

Since this is a cmake project, most IDEs will have built-in support or a plugin
to automatically setup this project.

In addition, the ``cmake`` command creates a ``compile_commands.json`` file in
the build folder, which can be used by many `language servers
<https://microsoft.github.io/language-server-protocol/>`_.
If you are using a C++ language server in your editor, it should be as easy as
symlinking the ``compile_commands.json`` into the repo root directory:

.. code-block:: shell

  ln -s build/compile_commands.json .

Distro-Specific Setup
---------------------

The wiki contains user-contributed `setup tips
<https://github.com/polybar/polybar/wiki/Distro-Specific-Setup>`_ for some
distros.
