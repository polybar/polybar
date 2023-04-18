# -*- coding: utf-8 -*-
#
# Configuration file for the Sphinx documentation builder.
#
# This file does only contain a selection of the most common options. For a
# full list see the documentation:
# https://www.sphinx-doc.org/en/master/config

# -- Path setup --------------------------------------------------------------

# If extensions (or modules to document with autodoc) are in another directory,
# add these directories to sys.path here. If the directory is relative to the
# documentation root, use os.path.abspath to make it absolute, like shown here.
#
import os
from pathlib import Path
import datetime
import sphinx
import packaging.version

from sphinx.util.docfields import Field
from sphinx.locale import _


def get_version(root_path):
    """
    Reads the polybar version from the version.txt at the root of the repo.
    """
    path = Path(root_path) / "version.txt"
    with open(path, "r") as f:
        for line in f.readlines():
            if not line.startswith("#"):
                # NB: we can't parse it yet since sphinx could import
                # pkg_resources later on and it could patch packaging.version
                return line

    raise RuntimeError("No version found in {}".format(path))

# -- Project information -----------------------------------------------------


project = 'Polybar User Manual'
copyright = '2016-{}, Michael Carlberg & contributors'.format(
    datetime.datetime.now().year
)
author = 'Polybar Team'

# is whether we are on readthedocs.io
on_rtd = os.environ.get('READTHEDOCS', None) == 'True'

if on_rtd:
    # On readthedocs, cmake isn't run so the version string isn't available
    version = os.environ.get('READTHEDOCS_VERSION', None)
else:
    # The short X.Y version
    version = '@APP_VERSION@'

# The full version, including alpha/beta/rc tags
release = version

# Set path to documentation
if on_rtd:
    # On readthedocs conf.py is already in the doc folder
    doc_path = '.'
else:
    # In all other builds conf.py is configured with cmake and put into the
    # build folder.
    doc_path = '@doc_path@'

# The version from the version.txt file. Since we are not always first
# configured by cmake, we don't necessarily have access to the current version
# number
version_txt = get_version(Path(doc_path).absolute().parent)

# -- General configuration ---------------------------------------------------

# If your documentation needs a minimal Sphinx version, state it here.
#
# needs_sphinx = '1.0'

# Add any Sphinx extension module names here, as strings. They can be
# extensions coming with Sphinx (named 'sphinx.ext.*') or your custom
# ones.
extensions = [
    "sphinx.ext.extlinks",
]

# Add any paths that contain templates here, relative to this directory.
templates_path = [doc_path + '/_templates']

# The suffix(es) of source filenames.
# You can specify multiple suffix as a list of string:
#
# source_suffix = ['.rst', '.md']
source_suffix = '.rst'

# The master toctree document.
master_doc = 'index'

# The language for content autogenerated by Sphinx. Refer to documentation
# for a list of supported languages.
#
# This is also used if you do content translation via gettext catalogs.
# Usually you set "language" from the command line for these cases.
# language = None

# List of patterns, relative to source directory, that match files and
# directories to ignore when looking for source files.
# This pattern also affects html_static_path and html_extra_path.
exclude_patterns = []

# The name of the Pygments (syntax highlighting) style to use.
pygments_style = None

highlight_language = 'none'

smartquotes = False

# Quickly link to issues or PRs using :issue:`...` or :pull:`...`
extlinks = {
    "issue": ("https://github.com/polybar/polybar/issues/%s", "#"),
    "pull": ("https://github.com/polybar/polybar/pull/%s", "PR #"),
}

extlinks_detect_hardcoded_links = True


# -- Options for HTML output -------------------------------------------------

# Theme options are theme-specific and customize the look and feel of a theme
# further.  For a list of options available for each theme, see the
# documentation.
#
html_theme_options = {}

# The theme to use for HTML and HTML Help pages.  See the documentation for
# a list of builtin themes.
#
if on_rtd or os.environ.get('USE_RTD_THEME', '0') == '1':
    html_theme = 'sphinx_rtd_theme'
    html_theme_options['collapse_navigation'] = False
    html_theme_options['style_external_links'] = True
else:
    html_theme = 'alabaster'
    html_theme_options['description'] = version

# Add any paths that contain custom static files (such as style sheets) here,
# relative to this directory. They are copied after the builtin static files,
# so a file named "default.css" will overwrite the builtin "default.css".
html_static_path = [doc_path + '/_static']

# Custom sidebar templates, must be a dictionary that maps document names
# to template names.
#
# The default sidebars (for documents that don't match any pattern) are
# defined by theme itself.  Builtin themes are using these templates by
# default: ``['localtoc.html', 'relations.html', 'sourcelink.html',
# 'searchbox.html']``.
#
# html_sidebars = {}


# -- Options for HTMLHelp output ---------------------------------------------

# Output file base name for HTML help builder.
htmlhelp_basename = 'polybardoc'


# -- Options for LaTeX output ------------------------------------------------

latex_elements = {
    # The paper size ('letterpaper' or 'a4paper').
    #
    # 'papersize': 'letterpaper',

    # The font size ('10pt', '11pt' or '12pt').
    #
    # 'pointsize': '10pt',

    # Additional stuff for the LaTeX preamble.
    #
    # 'preamble': '',

    # Latex figure (float) alignment
    #
    # 'figure_align': 'htbp',
}

# Grouping the document tree into LaTeX files. List of tuples
# (source start file, target name, title,
#  author, documentclass [howto, manual, or own class]).
latex_documents = [
    (master_doc, 'polybar.tex', 'polybar Documentation',
     'Polybar Team', 'manual'),
]


# -- Options for manual page output ------------------------------------------

# One entry per manual page. List of tuples
# (source start file, name, description, authors, manual section).
man_pages = [
    (
        'man/polybar.1', 'polybar',
        'A fast and easy-to-use tool status bar', [], 1
    ),
    (
        'man/polybar-msg.1', 'polybar-msg',
        'Send IPC messages to polybar', [], 1
    ),
    (
        'man/polybar.5', 'polybar',
        'configuration file for polybar(1)', [], 5
    )
]

man_make_section_directory = False


# -- Options for Texinfo output ----------------------------------------------

# Grouping the document tree into Texinfo files. List of tuples
# (source start file, target name, title, author,
#  dir menu entry, description, category)
texinfo_documents = [
    (master_doc, 'polybar', 'polybar Documentation',
     author, 'polybar', 'One line description of project.',
     'Miscellaneous'),
]


# -- Options for Epub output -------------------------------------------------

# Bibliographic Dublin Core info.
epub_title = project

# The unique identifier of the text. This can be a ISBN number
# or the project homepage.
#
# epub_identifier = ''

# A unique identification for the text.
#
# epub_uid = ''

# A list of files that should not be packed into the epub file.
epub_exclude_files = ['search.html']

# The 'versionadded' and 'versionchanged' directives are overridden.
suppress_warnings = ['app.add_directive']


def setup(app):

    # Adds a new directive for document a polybar config setting
    # Inside goes the description of the option as well as custom roles to
    # document the type, default value, etc:
    # .. poly-setting:: NAME
    #
    #   Description
    #   :type: ...
    #   :default: ...
    app.add_object_type(
        'poly-setting',
        'poly-setting',
        objname='configuration value',
        indextemplate='pair: %s; configuration value',
        doc_field_types=[
            Field('type',
                  label=_("Type"),
                  names=['type'],
                  has_arg=False,
                  ),
            Field('tags',
                  label=_("Available Tags"),
                  names=['tags'],
                  has_arg=False,
                  ),
            Field('tokens',
                  label=_("Supported Tokens"),
                  names=['tokens'],
                  has_arg=False,
                  ),
            Field('default',
                  label=_("Default Value"),
                  names=['default'],
                  has_arg=False,
                  ),
        ]
    )

    try:
        inject_version_directives(app)
    except NameError:
        # Function was not defined because sphinx version was too low
        pass


# It is not exactly clear in which version the VersionChange class was
# introduced, but we know it is available in at least 1.8.5.
# This feature is mainly needed for the online docs on readthedocs for the docs
# built from master, documentation built for proper releases should not even
# mention unreleased changes. Because of that it's not that important that this
# is added to local builds.
if packaging.version.parse(sphinx.__version__) >= packaging.version.parse("1.8.5"):

    from typing import List
    from docutils.nodes import Node
    from sphinx.domains.changeset import VersionChange

    def inject_version_directives(app):
        app.add_directive('deprecated', VersionDirective)
        app.add_directive('versionadded', VersionDirective)
        app.add_directive('versionchanged', VersionDirective)

    class VersionDirective(VersionChange):
        """
        Overwrites the Sphinx directive for versionchanged, versionadded, and
        deprecated and adds an unreleased tag to versions that are not yet
        released
        """

        def run(self) -> List[Node]:
            directive_version = packaging.version.parse(self.arguments[0])
            parsed_version_txt = packaging.version.parse(version_txt)

            if directive_version > parsed_version_txt:
                self.arguments[0] += " (unreleased)"

            return super().run()
