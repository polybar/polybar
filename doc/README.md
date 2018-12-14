Polybar Manual
==============

The official polybar documentation lives here.

The html documentation and man pages are built automatically when you build with cmake (cmake creates the custom
target `doc`).

## Preview Locally
The documentation uses [Sphinx](http://www.sphinx-doc.org/en/stable/) to generate the documentation, so you will need to
have that installed.

To generate the documentation you first need to configure polybar the same as when you compile it (`cmake ..` in `build`
folder).
After that you can run `make doc` to generate all of the documentation or `make doc_html` or `make doc_man` to only
generate the html documentation or the man pages.

Open `build/doc/html/index.html` to read the documentation in the browser.

The manual pages are placed in `build/doc/man`.
