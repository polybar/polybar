Polybar Manual
==============

The official polybar documentation lives here.

The html documentation and man pages are built automatically when you build with cmake (cmake creates the custom
target `doc`).

## Preview Locally
The documentation uses [Sphinx](https://www.sphinx-doc.org/en/stable/) to generate the documentation, so you will need to
have that installed.

If you build polybar normally while having Sphinx installed during configuration, the documentation will be enabled and
built as well. Building the documentation can be disabled by passing `-DBUILD_DOC=OFF` to `cmake`.

Alternatively the documentation can be built without the rest of polybar, for that run `cmake` only on the `doc`
directory. For example, create a `build` directory in `doc` and then run `cmake ..` in there.

Once configured, all of the documentation can be generated with `make doc` or use `make doc_html` or `make doc_man` to
only generate the html documentation or the man pages respectively.

The HTML documentation is in `doc/html/index.html` in your build directory and the man pages are in `doc/man`.
