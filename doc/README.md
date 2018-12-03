Polybar Manual
==============

The official polybar documentation lives here.

The html documentation and man pages are built automatically when you build with cmake (cmake creates the custom
target `doc`).

## Preview Locally
The documentation uses [Sphinx](http://www.sphinx-doc.org/en/stable/) with the "Read The Docs" theme
[`sphinx_rtd_theme`](https://github.com/rtfd/sphinx_rtd_theme/) to generate the documentation, so you will need to
have those installed.

You can then run `make html` inside this folder and sphinx will generate the html documentation inside `doc/build/html`.
Open `doc/build/html/index.html` to read the documentation in the browser.

With `make man` it will generate the man pages in the `doc/build/man` folder.
