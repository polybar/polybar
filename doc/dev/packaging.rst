Packaging Polybar
=================

Do you want to package polybar for a distro? Great! Read this page to get
started.

First Steps
-----------

Before you get started, have a look at the `Packaging Label
<https://github.com/polybar/polybar/issues?q=label%3APackaging>`_ on our GitHub
repo and `Repology <https://repology.org/project/polybar/versions>`_ to see if
polybar is already packaged for that distro or if there are efforts to do so.

Even if a package already exists, it might still make sense for you to package
polybar in some cases. Some of these cases are:

- The existing package is out-of-date and the packager is no longer able/willing
  to continue maintaining the package (or they are simply not reachable
  anymore).
- The existing package exist in some non-official repository and you are able to
  introduce the package into the official package repository for the
  distro/package manager. For example if there is a PPA providing polybar for
  Ubuntu and you can add polybar to the official Ubuntu repositories, please do
  :)

The list above is not exhaustive, if you are unsure, feel free to ask in a new
GitHub issue or on `Gitter <https://gitter.im/polybar>`_. Please also ask if you
run into any polybar related issues while packaging.

Packaging
---------

If you haven't already, carefully read the `Compiling
<https://github.com/polybar/polybar/wiki/Compiling>`_ wiki page to make sure you
fully understand all the dependencies involved and how to build polybar
manually.

We can't really tell you how to create a package for your distro, you need to
figure that out yourself. But we can give you some guidance on building polybar
for a package

Gathering the Source Code
^^^^^^^^^^^^^^^^^^^^^^^^^

Unless you are creating a package that tracks the ``master`` branch, don't clone
the git repository. We provide a tarball with all the required source code on
our `Release Page <https://github.com/polybar/polybar/releases>`_, use that in
your build.

Configuring and Compiling
^^^^^^^^^^^^^^^^^^^^^^^^^

.. note::

  Do not use the ``build.sh`` script for building polybar for your package. The
  usage and flags of the script may change without notice and we don't consider
  that a breaking change.

You can mostly follow the instructions on the `wiki
<https://github.com/polybar/polybar/wiki/Compiling#compiling>`_ for how to
compile polybar, but there are some additional ``cmake`` arguments you might
want to use:

- ``-DCMAKE_BUILD_TYPE=Release``: As of writing this is already the default, but
  use it just to be on the safe side.
- ``-DCMAKE_INSTALL_PREFIX=/usr``: Without this all the polybar files will be
  installed under ``/usr/local``. However, for packages it is often recommended
  they directly install to ``/usr``. So this flag will install polybar to
  ``/usr/bin/polybar`` instead of ``/usr/local/bin/polybar``. The packaging
  guidelines for your distro may disagree with this, in that case be sure to
  follow your distro's guidelines.

Instead of ``sudo make install``, you will most likely want to use
``DESTDIR=<dir> make install``. That way the files will be installed into
``<dir>`` instead of your filesystem root.

Finishing Up
------------

Finally, subscribe to our :issue:`GitHub thread for package maintainers <1971>`
to get notified about new releases and changes to how polybar is built.
If you want to, you can also open a PR to add your package to the `Getting
Started <https://github.com/polybar/polybar#getting-started>`_ section of our
README.

Thank you very much for maintaining a polybar package! 🎉
