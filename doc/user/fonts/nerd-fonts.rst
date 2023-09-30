Nerd Fonts
==========

`Nerd Fonts <https://www.nerdfonts.com/>`_ (`GitHub
<https://github.com/ryanoasis/nerd-fonts/>`_) is a project that patches
together a textual font with font icons (or glyphs) from other projects (e.g.
`Font Awesome <https://github.com/FortAwesome/Font-Awesome>`_, `Material Design
Icons <https://github.com/Templarian/MaterialDesign>`_, etc.) into a single
font.

In polybar, just using nerd fonts can lead to some issues:

* Cut-off Characters
* Overlapping
* No Spacing

These look something like this:

.. image:: /_static/nerd-fonts/bad.png
   :alt: Showcase of the three issues listed above.

This behavior is intrinsic to Nerd Fonts and is described in more detail `here
<https://github.com/ryanoasis/nerd-fonts/issues/442#issuecomment-1263358904>`_.
Also see :issue:`991` for more information.

**To resolve these issues, we recommend using Nerd Fonts like this:**

The monospaced variants of the different Nerd Fonts (all characters have the
same width) don't have this issue.
However, then you often have the problem that the icons are too small and that
their size cannot be set independently of the text.

Due to that, we recommend using ``Symbols Nerd Font Mono`` (available for
`download <https://github.com/ryanoasis/nerd-fonts/releases/>`_ as
``NerdFontsSymbolsOnly.zip``).
This font only contains the nerd font icons and no text.
For the text, simply use any non-Nerd Font:

.. code-block:: ini

  font-0 = "Liberation Mono:size=20"
  font-1 = "Symbols Nerd Font Mono:size=26"

Now the icon sizes can be adjusted separately to get the best experience.
This solves all three problems shown above:

.. image:: /_static/nerd-fonts/good.png
   :alt: The same config as in the previous screenshot but using ``Symbols Nerd
         Font Mono`` for the font icons

.. note::

   In the overlap example, there is no space between the icon and text, that's
   why they're so close together.
