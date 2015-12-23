libgreen -- Python-style asyncio in C
=====================================

Introduction
------------

``libgreen`` is a coroutine-based library for friendly asynchronous I/O in C.

API
---

Library version
~~~~~~~~~~~~~~~

The library follows `semantic versioning`_.  The (:c:macro:`GREEN_MINOR`,
:c:macro:`GREEN_MINOR`) pair identifes the API contract version while
:c:macro:`GREEN_PATCH` is incremented for internal changes that fix bugs.

This library's API offers several facilities for using the library version at
compile-time (for conditional compilation) and at run-time (usally for
diagnostics).

.. _`semantic versioning`: http://semver.org/

.. c:macro:: GREEN_MAJOR

   The library's API version.  This value is increased with each breaking API
   change.

   See :c:macro:`GREEN_VERSION` and :c:func:`GREEN_MAKE_VERSION` for version
   testing.

.. c:macro:: GREEN_MINOR

   The library's feature version.  This value is increased when adding new
   features that do not break existing APIs.

   See :c:macro:`GREEN_VERSION` and :c:func:`GREEN_MAKE_VERSION` for version
   testing.

.. c:macro:: GREEN_PATCH

   The library's patch version.  This value is increased for releases that
   contain only internal changes, typically to fix bugs without changing the
   API.

   .. attention:: This library treats a discrepancy between the documentation
      and the behavior as a bug in the code.  If your application relies on a
      bug in the library, it may be surprised by an apparent change in
      behavior.

.. c:macro:: GREEN_VERSION

   A single number that encodes :c:macro:`GREEN_MAJOR`, :c:macro:`GREEN_MINOR`
   and :c:macro:`GREEN_PATCH` for compile-time version testing.

   This is typically used to compare against a call to
   :c:macro:`GREEN_MAKE_VERSION`.

   Here is how to use this macro to handle multiple contract versions in the
   same library:

   .. code-block:: c
      :caption: Compile-time workaround for API break

      #if GREEN_VERSION < GREEN_MAKE_VERSION(1, 0, 0)
          // Workaround for pre-1.0 versions.
      #endif

   Here is how to use this macro to handle presence and absence of a specific
   feature:

   .. code-block:: c
      :caption: Compile-time feature testing

      #define HAVE_GREEN_XYZ \
          (GREEN_VERSION >= GREEN_MAKE_VERSION(1, 1, 0))

      // ...

      #if HAVE_GREEN_XYZ
          // Use XYZ.
      #endif

   See :c:func:`green_version` to see which version your application is
   currently running against.

.. c:function:: GREEN_MAKE_VERSION(major, minor, patch)

   Build a value which can be used to compare against :c:macro:`GREEN_VERSION`
   or the result of :c:func:`green_version`.

   See :c:macro:`GREEN_VERSION` for examples on how to use this macro.

.. c:function:: int green_version()

   See :c:macro:`GREEN_VERSION` to see which version your application was
   compiled against.

.. c:macro:: GREEN_VERSION_STRING

   Build a dotted version string that can be used for display.

   See :c:func:`green_version_string` to see which version your application is
   currently running against.

.. c:function:: const char * green_version_string()

   Build a dotted version string that can be used for display.

   See :c:func:`GREEN_VERSION_STRING` to see which version your application was
   compiled against.

Library setup
~~~~~~~~~~~~~

.. c:function:: int green_init()

   :return: Zero if the function succeeds.

   .. note:: This function is implemented as a macro.

.. c:function:: int green_term()

   :return: Zero if the function succeeds.


Indices and tables
==================

* :ref:`genindex`
* :ref:`search`
