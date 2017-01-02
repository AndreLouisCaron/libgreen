libgreen -- Python-style asyncio in C
=====================================

Introduction
------------

``libgreen`` is a coroutine-based library for friendly asynchronous I/O in C.

API
---

.. _loop:

Loop
~~~~

The event loop is the central hub for coordination between a set of task_
instances running in the same OS thread.

.. c:type:: green_loop_t

   This is an opaque pointer type to a reference-counted object.

.. c:function:: green_loop_t green_loop_init()

   Create a new event loop.

   Your application can create as many event loops as it wishes, but it make no
   sense to have more than one per thread at any given time since tasks in
   different loops cannot resume each other.

   Use :c:func:`green_task_init` to launch new tasks in the new event
   loop.

   :return: A new event loop.

.. c:function:: int green_loop_acquire(green_loop_t loop)

   Increase the reference count.

   :return: Zero if the function succeeds.

.. c:function:: int green_loop_release(green_loop_t loop)

   Decrease the reference count and destroy the object if necessary.

   :return: Zero if the function succeeds.


.. _task:

Task
~~~~

.. c:type:: green_task_t

   This is an opaque pointer type to a reference-counted object.

.. c:function:: green_task_t green_task_spawn(green_loop_t loop, int(*method)(green_loop_t,green_task_t,void*), void * object, size_t stack_size)

   Call ``method(loop, task, object)`` in a new coroutine.

   :arg loop: Loop that owns the current task.
   :arg method: Pointer to application callback that will be called inside a
      new task.
   :arg object: Pointer to application data that will be passed uninterpreted
      to ``method``.
   :arg stack_size: Size of the stack in bytes.  When zero, a default and
      possibly system-specific stack size is selected.
   :return: A new task.

   .. note:: This function is implemented as a macro.

.. c:function:: int green_yield(green_loop_t loop)

   Suspend the current task, re-schedule it to run in the next loop tick.

   :arg loop: Loop that owns the current task.
   :return: Zero if the function succeeds.

   .. note:: This function is implemented as a macro.

.. c:function:: green_task_state_t green_task_state(green_state_t task)

   Query the current state of a task.

   :arg task: The task.
   :return: The task's current state.

.. c:function:: int green_task_acquire(green_task_t task)

   Increase the reference count.

   :return: Zero if the function succeeds.

.. c:function:: int green_task_release(green_task_t task)

   Decrease the reference count and destroy the object if necessary.

   :return: Zero if the function succeeds.

Error codes
~~~~~~~~~~~

.. c:macro:: GREEN_SUCCESS

   The call completed successfully.  This value is guaranteed to be zero.

.. c:macro:: GREEN_ENOMEM

   Could not allocate enough memory for the requested operation.

Indices and tables
==================

* :ref:`genindex`
* :ref:`search`
