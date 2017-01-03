////////////////////////////////////////////////////////////////////////
// Copyright(c) libgreen contributors.  See LICENSE file for details. //
////////////////////////////////////////////////////////////////////////


#include <green.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "configure.h"

#if GREEN_USE_UCONTEXT
#   include <ucontext.h>
#endif


// ucontext documentation suggests using SIGSTKSZ, but it seems to be too
// small on Linux and segfaults on first swapcontext.
static const int DEFAULT_STACK_SIZE = 64 * 1024;


struct green_loop {

    // Intrusive reference count (each task holds a reference, in addition to
    // all references held by clients).
    int refs;

    // Auto-incrementing task ID generator.
    int next_task_id;

    // Tick counter, used as a marker when iterating over the ready queue.
    int tick;

    // Intrusive singly-linked list of pending (ready-to-run) tasks.
    //
    // When empty, both `head` and `tail` are NULL.
    struct {
        green_task_t head;
        green_task_t tail;
    } ready_queue;

    // Platform-specific bookkeeping for context switches.
#if GREEN_USE_UCONTEXT
    ucontext_t context;
#endif
};


struct green_task {

    // Loop through which the task was spawned.
    green_loop_t loop;

    // Intrusive reference count.
    int refs;

    // Task ID.
    int id;

    // Application callback used as task entry point.  When this returns, the
    // task will be considered "stopped".
    int(*method)(green_loop_t,green_task_t,void*);

    // Application data passed to the task entry point.
    void * object;

    // Current task state.
    green_task_state_t state;

    // Tick at which the task got scheduled.
    int tick;

    // Last known location (from init or yield).
    const char * source;

    // Intrusive singly linked list (see loop->ready_queue).
    green_task_t next;

    // Platform-specific bookkeeping for context switches.
#if GREEN_USE_UCONTEXT
    ucontext_t context;

    // Hold an explicit reference to the task's stack memory.  We can't rely on
    // `coro.uc_stack.ss_sp` because the ucontext implementation is free to
    // manipulate this at will and there is no guarantee that this will be the
    // pointer we allocated.
    void * stack;
#endif
};


green_loop_t _green_loop_init(char * source)
{
    green_loop_t loop = calloc(1, sizeof(struct green_loop));
    if (loop == NULL) {
        return NULL;
    }
    loop->refs = 1;
    loop->next_task_id = 1;
    loop->tick = 0;
    loop->ready_queue.head = NULL;
    loop->ready_queue.tail = NULL;
    return loop;
}


int green_loop_acquire(green_loop_t loop)
{
    assert(loop != NULL);
    ++loop->refs;
    return GREEN_SUCCESS;
}


int green_loop_release(green_loop_t loop)
{
    assert(loop != NULL);
    assert(loop->refs > 0);
    if (--(loop->refs) > 0) {
        return GREEN_SUCCESS;
    }
    free(loop);
    return GREEN_SUCCESS;
}


static void _task(green_task_t task)
{
    assert(task != NULL);
    assert(task->loop != NULL);

    green_loop_t loop = task->loop;

    // Bump refs count to prevent teardown while task is running.
    green_loop_acquire(loop);
    green_task_acquire(task);

    fprintf(stderr, "T #%d: started.\n", task->id);

    // Call the task entry point.  We expect this to yield control to the hub
    // 0..N times during its execution (this function is re-entrant, all
    // blocked/pending/running coroutines are running it).  When this function
    // returns, the task is done.
    task->state = green_task_running;
    (*task->method)(loop, task, task->object);
    assert(loop->ready_queue.head == task);
    assert(task->state == green_task_running);

    fprintf(stderr, "T #%d: stopped.\n", task->id);

    // Update task state.
    task->state = green_task_stopped;

    // Pop ourselves off the queue.
    loop->ready_queue.head = task->next;
    task->next = NULL;

    // Release resources.
    green_task_release(task);
    green_loop_release(loop);

    // Perform context switch.
#if GREEN_USE_UCONTEXT
    // Performed automatically.  When the function returns,
    // `ucontext_t::uc_link` will be followed and we (conveniently) had this
    // point to `green_loop::context`.
#endif
}


green_task_t _green_spawn(green_loop_t loop,
                          int(*method)(green_loop_t,green_task_t,void*),
                          void * object, size_t stack_size,
                          const char * source)
{
    assert(loop != NULL);
    assert(method != NULL);
    assert(source != NULL);

    if (stack_size == 0) {
        stack_size = DEFAULT_STACK_SIZE;
    }

    green_task_t task = calloc(1, sizeof(struct green_task));
    if (loop == NULL) {
        return NULL;
    }

    task->refs = 1;
    task->loop = loop;
    task->id = loop->next_task_id++;
    task->method = method;
    task->object = object;
    task->state = green_task_pending;
    task->source = source;
    task->next = NULL;

    fprintf(stderr,
            "loop: spawned T #%d @ \"%s\".\n", task->id, task->source);

#if GREEN_USE_UCONTEXT
    // NOTE: man pages says to check getcontext for -1 and check errno, but no
    //       error codes are defined.  Since there is no way to test this
    //       because we don't know how to trigger any errors, just assert on
    //       it and deal with it if we ever hit the assertion in practice.
    int rc = getcontext(&task->context);
    assert(rc == 0);
    task->stack = calloc(stack_size, 1);
    if (task->stack == NULL) {
        return GREEN_ENOMEM;
    }
    task->context.uc_stack.ss_sp = task->stack;
    task->context.uc_stack.ss_size = stack_size;
    task->context.uc_link = &loop->context;
    makecontext(&task->context, (void(*)())_task, 1, task);
#endif

    // Queue for execution (FIFO).
    if (loop->ready_queue.head == NULL) {
        fprintf(stderr, "loop: queueing T #%d as head.\n", task->id);
        loop->ready_queue.tail = loop->ready_queue.head = task;
    }
    else {
        fprintf(stderr, "loop: queueing T #%d as tail.\n", task->id);
        loop->ready_queue.tail = loop->ready_queue.tail->next = task;
    }
    task->tick = loop->tick;

    return task;
}


int _green_tick(green_loop_t loop, const char * source)
{
    assert(loop != NULL);
    assert(source != NULL);

    // Ensure next time we run whatever was (re-)scheduled in this tick.
    int tick = loop->tick++;

    while ((loop->ready_queue.head != NULL) &&
           (loop->ready_queue.head->tick <= tick))
    {
        // Grab next task.
        green_task_t task = loop->ready_queue.head;
        assert(task->state == green_task_pending);
        fprintf(stderr,
                "loop: yielding to T #%d @ \"%s\".\n",
                task->id, task->source);

        // Perform context switch.
        task->state = green_task_running;
#if GREEN_USE_UCONTEXT
        swapcontext(&loop->context, &task->context);
#endif

        // Once we get control back, the task should be blocked on I/O, pending
        // for execution or stopped (ready for termination).
        assert(task->state != green_task_running);
        assert(task->source != NULL);

        // Yield point should have popped us off the queue.
        assert((loop->ready_queue.head != task) || (task->tick > tick));

        fprintf(stderr,
                "loop: yielded from T #%d @ \"%s\".\n",
                task->id, task->source);
    }

    return GREEN_SUCCESS;
}


int _green_yield(green_loop_t loop, const char * source)
{
    assert(loop != NULL);
    assert(loop->ready_queue.head != NULL);
    assert(loop->ready_queue.tail != NULL);
    assert(source != NULL);

    green_task_t task = loop->ready_queue.head;
    assert(task != NULL);
    assert(task->state == green_task_running);

    // Update task state.
    task->state = green_task_pending;
    task->source = source;

    fprintf(stderr,
            "T #%d: yielding back to event loop from \"%s\".\n",
            task->id, task->source);

    // Pop ourselves off the queue.
    loop->ready_queue.head = task->next;
    if (loop->ready_queue.head == NULL) {
        loop->ready_queue.tail = NULL;
    }
    task->next = NULL;

    // Queue for execution at next tick (FIFO).
    if (loop->ready_queue.head == NULL) {
        fprintf(stderr, "T #%d: re-queueing as head.\n", task->id);
        loop->ready_queue.tail = loop->ready_queue.head = task;
    }
    else {
        fprintf(stderr, "T #%d: re-queueing as tail.\n", task->id);
        loop->ready_queue.tail = loop->ready_queue.tail->next = task;
    }
    task->tick = loop->tick;

    fprintf(stderr, "T #%d: pausing.\n", task->id);

    // Perform context switch.
#if GREEN_USE_UCONTEXT
    swapcontext(&task->context, &loop->context);
#endif

    fprintf(stderr, "T #%d: resumed.\n", task->id);

    return GREEN_SUCCESS;
}


green_task_state_t green_task_state(green_task_t task)
{
    assert(task != NULL);
    return task->state;
}


int green_task_acquire(green_task_t task)
{
    assert(task != NULL);
    ++task->refs;
    return GREEN_SUCCESS;
}


int green_task_release(green_task_t task)
{
    assert(task != NULL);
    assert(task->loop != NULL);
    if (--task->refs == 0) {
#if GREEN_USE_UCONTEXT
        assert(task->stack != NULL);
        free(task->stack);
        task->stack = NULL;
#endif
        free(task);
    }
    return GREEN_SUCCESS;
}
