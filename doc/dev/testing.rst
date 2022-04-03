Testing
=======

Polybar uses `googletest <https://google.github.io/googletest/>`_ as its
testing and mocking framework.
Tests live in the ``tests/`` directory; they can be enabled during cmake with
``-DBUILD_TESTS=ON`` and compiled with ``make all_unit_tests``.

Each test gets its own executable in ``build/tests``, which can be executed to run
a specific test.

Running all tests is preferably done with the following command:

.. code-block:: shell

  make check

This runs all available tests and prints the output in color for failed tests
only.

Adding New Tests
----------------

All new tests need to be added to the ``tests/CMakeLists.txt`` file. Have a look
at the other unit tests in ``tests/unit_tests`` to see how to write tests for your
code.
