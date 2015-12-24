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


.. _loop:

Loop
~~~~

The event loop is the central hub for coordination between a set of coroutine_
instances running in the same OS thread.

.. c:type:: green_loop_t

   This is an opaque pointer type to a reference-counted object.

.. c:function:: green_loop_t green_loop_init()

   Create a new event loop.

   Your application can create as many event loops as it wishes, but it make no
   sense to have more than one per thread at any given time since coroutines in
   different loops cannot resume each other.

   Use :c:func:`green_coroutine_init` to launch new coroutines in the new event
   loop.

   :return: A new event loop.

.. c:function:: int green_loop_acquire(green_loop_t loop)

   Increase the reference count.

   :return: Zero if the function succeeds.

.. c:function:: int green_loop_release(green_loop_t loop)

   Decrease the reference count and destroy the object if necessary.

   :return: Zero if the function succeeds.


.. _coroutine:

Coroutine
~~~~~~~~~

.. c:type:: green_coroutine_t

   This is an opaque pointer type to a reference-counted object.

.. c:function:: green_coroutine_t green_coroutine_init(green_loop_t loop, int(*method)(void*,void*), void * object, size_t stack_size)

   Call ``method(loop, object)`` in a new coroutine.

   :arg loop: Loop that owns the current coroutine.
   :arg method: Pointer to application callback that will be called inside a
      new coroutine.
   :arg object: Pointer to application data that will be passed uninterpreted
      to ``method``.
   :arg stack_size: Size of the stack in bytes.  When zero, a default and
      possibly system-specific stack size is selected.
   :return: A new coroutine.

   .. note:: This function is implemented as a macro.

.. c:function:: int green_yield(green_loop_t loop, green_coroutine_t coro)

   Block until any other coroutine yields back.

   :arg loop: Loop that owns the current coroutine (and ``coro``).
   :arg coro: Coroutine to which control should be yielded.  When ``NULL``,
      control is returned to the loop.
   :return: Zero if the function succeeds.

   .. note:: This function is implemented as a macro.

.. c:function:: green_future_t green_coroutine_join(green_coroutine_t coro)

   :arg coro: Coroutine whose completion you are interested in.
   :return: A future that will be completed when ``coro`` returns.

   .. note:: Cancelling this future does **not** cancel the coroutine.

.. c:function:: int green_coroutine_acquire(green_coroutine_t coro)

   Increase the reference count.

   :return: Zero if the function succeeds.

.. c:function:: int green_coroutine_release(green_coroutine_t coro)

   Decrease the reference count and destroy the object if necessary.

   :return: Zero if the function succeeds.


Indices and tables
==================

* :ref:`genindex`
* :ref:`search`
