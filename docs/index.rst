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

.. c:function:: int green_coroutine_acquire(green_coroutine_t coro)

   Increase the reference count.

   :return: Zero if the function succeeds.

.. c:function:: int green_coroutine_release(green_coroutine_t coro)

   Decrease the reference count and destroy the object if necessary.

   :return: Zero if the function succeeds.


.. _future:

Future
~~~~~~

The future is the foundation of ``libgreen``.  It represents the promise of
completion of an asynchronous operation.  Combined with the poller_ and
:c:func:`green_select`, it can be used to multiplex multiple asynchronous
operations in the same coroutine_.

.. c:type:: green_future_t

   This is an opaque pointer type to a reference-counted object.

.. c:function:: green_future_t green_future_init(green_hub_t hub)

   Create a custom future.  When your asynchronous operation completes, call
   :c:func:`green_future_set_result` to mark it as complete and unblock a
   coroutine if one is waiting on this future.

   :arg hub: Hub to which the future will be attached.  The coroutine that
      completes this future **MUST** be running from this hub.
   :return: A future that you can complete whenever you wish.

.. c:function:: int green_future_set_result(green_future_t future, void * p, int i)

   Mark the future as completed.  If any coroutine is currently blocking on
   :c:func:`green_select` with a poller in which this future is registered,
   then that coroutine will be unblocked and resumed soon.

   :arg future: Future to complete.
   :arg p: Pointer result.  Will be returned by :c:func:`green_future_result`.
   :arg i: Integer result.  Will be returned by :c:func:`green_future_result`.
   :return: Zero if the function succeeds.

.. c:function:: int green_future_done(green_future_t future)

   Check if the future is completd or canceled.

   :arg future: Future to check for completion.
   :return: Non-zero if the future is completed or cancelled.  Zero if the
      future is still pending.

.. c:function:: int green_future_canceled(green_future_t future)

   Check if the future is canceled.

   :arg future: Future to check for cancellation.
   :return: Non-zero if the future is cancelled.  Zero if the future is still
      pending or completed.

.. c:function:: int green_future_result(green_future_t future, void ** p, int * i)

   Return the result that was stored when the future was completed.

   :arg future: Future to check for result after completion.
   :arg p: Pointer into which the value passed to
      :c:func:`green_future_set_result` will be stored.
   :arg p: Integer into which the value passed to
      :c:func:`green_future_set_result` will be stored.
   :return: Zero if the function succeeds, :c:macro:`GREEN_EBUSY` if the future
      is pending, :c:macro:`GREEN_EBADFD` if the future is canceled.

.. c:function:: int green_future_cancel(green_future_t future)

   Since there is no reason to keep around a cancelled future, canceling a
   future automatically decrements the reference count and there is no need to
   call :c:func:`green_future_release` after canceling the future.

   .. attention:: Cancellation is usually asynchronous and may not be natively
      supported by all asynchronous operations or by all platforms for a given
      asynchronous operation.  The basic cancellation guarantee is that the
      future will never be returned by :c:func:`green_select`, but the future
      may not be deleted until it is actually completed.

   :arg future: The future to cancel.
   :return: Zero on success.

.. c:function:: int green_future_acquire(green_future_t future)

   Increase the reference count.

   :return: Zero if the function succeeds.

.. c:function:: int green_future_release(green_future_t future)

   Decrease the reference count and destroy the object if necessary.

   :return: Zero if the function succeeds.


.. _poller:

Poller
~~~~~~

The poller is a specialized container that stores a set of future_ refrences in
way that allows very efficient dispatch of future completion and implicit
unblocking of a coroutine_ currently waiting on a completed future.

When you initiate a new asynchronous operation, call :c:func:`green_poller_add`
to add the future_ to the poller.  When your coroutine is ready, call
:c:func:`green_select` to block until one or more such futures complete.

The poller is similar to the ``fd_set`` that is used with ``select()``, but
implemented in a way that allows ``O(1)`` dispatch.

.. attention:: Pollers **MUST NOT** be shared between coroutines.  Any
   modification of a poller on which another coroutine is blocking via
   :c:func:`green_select` may result in undefined behavior.

.. c:type:: green_poller_t

   This is an opaque pointer type to a reference-counted object.

   Use :c:func:`green_poller_release` when you are done with such a pointer.
   If you need to grab extra references, call :c:func:`green_poller_acquire`.
   Make sure you call :c:func:`green_poller_release` once for each call to
   :c:func:`green_poller_acquire`.

.. c:function:: green_poller_t green_poller_init(green_hub_t hub, size_t size)

   Create a new poller for use with :c:func:`green_select`.

   :arg hub: Hub to which the current coroutine is attached.
   :arg size: Maximum number of futures you intend on adding to this poller.
   :return: A new poller that can be passed to :c:func:`green_select`.  Call
      :c:func:`green_poller_release` when you are done with this poller.

.. c:function:: size_t green_poller_size(green_poller_t poller)

   Get the total number of slots in the future.

   :arg poller: Poller to check for maximum size.
   :return: The maximum number of futures that can be stored in the poller.

.. c:function:: size_t green_poller_used(green_poller_t poller)

   Get the number of slots currently used by futures (pending and completed).

   :arg poller: Poller to check for current size.
   :return: The current number of futures stored in the poller.

.. c:function:: size_t green_poller_done(green_poller_t poller)

   Get the number of slots currently used by completed futures.  This number
   indicates the number of times you can call :c:func:`green_poller_pop` before
   it returns ``NULL``.

   :arg poller: Poller to check for completed futures.
   :return: The number of completed futures currently stored in the poller.

.. c:function:: int green_poller_add(green_poller_t poller, green_future_t future)

   Add a future to the set.

   It is legal to add a completed future to a poller.  In that case, the next
   call to :c:func:`green_select` with that poller will not block.  This is
   especially convenient to handle synchronous completion of some asynchronous
   operations as it removes the need for an alternate code path to handle
   synchronous completion -- especially for operations that may be synchronous
   only on some platforms.

   :arg poller: Poller to which the future should be added.
   :arg future: Future that should be added to the poller.
   :return: Zero if the function succeeds.

.. c:function:: int green_poller_rem(green_poller_t poller, green_future_t future)

   Remove a future from the poller without fulfilling or cancelling it.

   Futures are automatically removed from the poller when fulfilled or
   cancelled.  However, the implicit removal of a cancelled future may be
   asynchronous.  If you cancel a future and then need the slot immediately to
   add a new future, you can call this to free the slot immediately.

   :arg poller: Poller from which the future should be removed.
   :arg future: Future that should be removed from the poller.
   :return: Zero if the function succeeds.

.. c:function:: green_future_t green_poller_pop(green_poller_t poller)

   Grab the next completed future from the poller.  You can call
   :c:func:`green_poller_done` to determine how many times you can call this
   function before it returns ``NULL``.

   :arg poller: Poller from which a completed future should be removed.
   :return: A completed future.  If ``poller`` contains no completed futures,
      ``NULL`` is returned.

.. c:function:: int green_poller_acquire(green_poller_t poller)

   Increase the reference count.

   :return: Zero if the function succeeds.

.. c:function:: int green_poller_release(green_poller_t poller)

   Decrease the reference count and destroy the object if necessary.

   :return: Zero if the function succeeds.

.. c:function:: green_future_t green_select(green_poller_t poller)

   Block until any of the futures registered in ``poller`` are completed.  If
   the poller contains any completed futures, the function returns immediately.

   :arg poller: The poller in which all futures that can unblock the current
      coroutine are registered.
   :return: A completed future if the function succeeds, else ``NULL``.

   .. note:: This function is implemented as a macro.

Error codes
~~~~~~~~~~~

.. c:macro:: GREEN_SUCCESS

   The call completed successfully.  This value is guaranteed to be zero.

.. c:macro:: GREEN_EBUSY

   The future is still pending.

.. c:macro:: GREEN_ENOMEM

   Could not allocate enough memory.

.. c:macro:: GREEN_EBADFD

   The future is in an invalid state.

.. c:macro:: GREEN_ECANCELED

   Cannot complete the future because it is already canceled.

.. c:macro:: GREEN_EALREADY

   Cannot add the future to the poller because the future is alredy in a
   poller.

.. c:macro:: GREEN_ENOENT

   Cannot remove the future from the poller because it was not found inside the
   poller.

.. c:macro:: GREEN_ENFILE

   Cannot add the future to the poller because the poller is already full.

Indices and tables
==================

* :ref:`genindex`
* :ref:`search`
