////////////////////////////////////////////////////////////////////////
// Copyright(c) libgreen contributors.  See LICENSE file for details. //
////////////////////////////////////////////////////////////////////////

#include <green.h>
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

    int refs;
    int coroutines;
    int nextcoroid;

#if GREEN_USE_UCONTEXT
    ucontext_t context;
#endif

    green_coroutine_t currentcoro;
};

typedef enum green_coroutine_state {
    pending,
    running,
    blocked,
    stopped,
} green_coroutine_state_t;

struct green_coroutine {

    green_loop_t loop;
    int refs;
    int id;

    int(*method)(green_loop_t,void*);
    void * object;

    green_coroutine_state_t state;
    int result;

#if GREEN_USE_UCONTEXT
    ucontext_t context;
    void * stack;
#endif

    // Last known location (from init or yield).
    const char * source;
};


typedef enum green_future_state {

    green_future_pending,
    green_future_aborted,
    green_future_complete,

} green_future_state_t;

struct green_future {

    green_loop_t loop;
    int refs;

    struct {
        void * p;
        int i;
    } result;

    green_future_state_t state;

    // Intrusive set.
    green_poller_t poller;
    int slot;
};

struct green_poller {

    green_loop_t loop;
    int refs;

    // Intrusive set.
    green_future_t * futures;
    size_t used;
    size_t size;
    size_t busy;
};

#define green_panic()                           \
    do {                                        \
        fflush(stderr);                         \
        abort();                                \
    } while (0)

#define _green_assert(exp, file, line)                                  \
    do {                                                                \
        if (!(exp)) {                                                   \
            fprintf(stderr, "Assertion failed: \"%s\". at %s:%d\n", #exp, file, line); \
            green_panic();                                              \
        }                                                               \
    } while (0)
#define green_assert(exp) _green_assert(exp, __FILE__, __LINE__)

void * green_malloc(int size)
{
    // TODO: insert block header for tracking.
    void * p = malloc(size);
    green_assert(p != NULL);
    memset(p, 0, size);
    return p;
}

void green_free(void * p)
{
    // TODO: account for block header.
    free(p);
}

int green_version()
{
    return GREEN_VERSION;
}

const char * green_version_string()
{
    return GREEN_VERSION_STRING;
}

int _green_init(int major, int minor)
{
    if (major != GREEN_MAJOR) {
        return GREEN_EINVAL;
    }
    if (minor > GREEN_MINOR) {
        return GREEN_EINVAL;
    }
    return GREEN_SUCCESS;
}

int green_term()
{
    return GREEN_SUCCESS;
}

green_loop_t green_loop_init()
{
    green_loop_t loop = green_malloc(sizeof(struct green_loop));
    loop->refs = 1;
    loop->coroutines = 0;
    loop->nextcoroid = 1;
    loop->currentcoro = NULL;

    return loop;
}

int green_loop_acquire(green_loop_t loop)
{
    green_assert(loop != NULL);
    ++loop->refs;
    return GREEN_SUCCESS;
}

int green_loop_release(green_loop_t loop)
{
    green_assert(loop != NULL);
    green_assert(loop->refs > 0);

    if (--(loop->refs) > 0) {
        return GREEN_SUCCESS;
    }

    green_assert(loop != NULL);

    green_assert(loop->coroutines == 0);
    green_free(loop);

    return GREEN_SUCCESS;
}

static void _coroutine(green_coroutine_t coro)
{
    green_assert(coro != NULL);
    green_assert(coro->loop != NULL);
    green_assert(coro->loop->currentcoro == coro);

    // Bump refs count to prevent teardown while coroutine is running.
    ++coro->loop->refs;
    ++coro->refs;

    coro->state = running;
    coro->result = (*coro->method)(coro->loop, coro->object);
    green_assert(coro->loop->currentcoro == coro);
    green_assert(coro->state == running);

    coro->state = stopped;
    coro->loop->currentcoro = NULL;

    // Coroutine is done, release artificial ref counts.
    //
    // TODO: handle case where this is last ref to coroutine!
    --coro->refs;
    --coro->loop->refs;
}

green_coroutine_t _green_coroutine_init(green_loop_t loop,
                                        int(*method)(green_loop_t,void*),
                                        void * object, size_t stack_size,
                                        const char * source)
{
    green_assert(loop != NULL);
    green_assert(method != NULL);
    green_assert(source != NULL);

    if (stack_size == 0) {
        stack_size = DEFAULT_STACK_SIZE;
    }

    green_coroutine_t coro = green_malloc(sizeof(struct green_coroutine));

    coro->refs = 1;
    coro->loop = loop;
    coro->id = loop->nextcoroid++;
    coro->method = method;
    coro->object = object;
    coro->state = pending;
    coro->result = -1;
    coro->source = source;

#if GREEN_USE_UCONTEXT
    // NOTE: man pages says to check getcontext for -1 and check errno, but no
    //       error codes are defined.  Since there is no way to test this
    //       because we don't know how to trigger any errors, just assert on
    //       it and deal with it if we ever hit the assertion in practice.
    int rc = getcontext(&coro->context);
    green_assert(rc == 0);
    coro->stack = green_malloc(stack_size);
    coro->context.uc_stack.ss_sp = coro->stack;
    coro->context.uc_stack.ss_size = stack_size;
    coro->context.uc_link = &loop->context;
    makecontext(&coro->context, (void(*)())_coroutine, 1, coro);
#endif

    loop->coroutines++;

    return coro;
}

int _green_yield(green_loop_t loop, green_coroutine_t coro, const char * source)
{
    green_assert(loop != NULL);
    if (loop->currentcoro) {
        green_assert(loop->currentcoro->state == running);
    }
    if (coro) {
        green_assert((coro->state == blocked) || (coro->state == pending));
    }

    if (coro) {
        fprintf(stderr, "Yielding to coroutine %d.\n", coro->id);
        green_assert(loop->currentcoro == NULL);
        loop->currentcoro = coro;
        loop->currentcoro->state = running;
#if GREEN_USE_UCONTEXT
        swapcontext(&loop->context, &coro->context);
#endif
        green_assert(loop->currentcoro == NULL);
        if (coro->state != stopped) {
            coro->state = blocked;
        }
    }
    else {
        fprintf(stderr, "Yielding to event loop.\n");
        green_assert(loop->currentcoro != NULL);
        loop->currentcoro->state = blocked;
        coro = loop->currentcoro;
        loop->currentcoro = NULL;
#if GREEN_USE_UCONTEXT
        swapcontext(&coro->context, &loop->context);
#endif
        green_assert(loop->currentcoro != NULL);
        loop->currentcoro->state = running;
    }

    return GREEN_SUCCESS;
}

int green_coroutine_result(green_coroutine_t coro)
{
    green_assert(coro != NULL);
    green_assert(coro->state == stopped);
    return coro->result;
}

int green_coroutine_acquire(green_coroutine_t coro)
{
    green_assert(coro != NULL);
    ++coro->refs;
    return GREEN_SUCCESS;
}

int green_coroutine_release(green_coroutine_t coro)
{
    green_assert(coro != NULL);
    green_assert(coro->loop != NULL);
    if (--coro->refs == 0) {
#if GREEN_USE_UCONTEXT
        green_assert(coro->stack != NULL);
        green_free(coro->stack);
        coro->stack = NULL;
#endif
        --coro->loop->coroutines;
        green_free(coro);
    }
    return GREEN_SUCCESS;
}

static void green_poller_dump(green_poller_t poller)
{
    static const char* states[] = {
        "pending",
        "aborted",
        "complete",
    };
    fprintf(stderr, "poller @ 0x%p, size=%zu, used=%zu, busy=%zu:\n",
            poller, poller->size, poller->used, poller->busy);
    int w = (int)ceil(log10((double)poller->size));
    for (size_t i = 0; (i < poller->size); ++i) {
        green_future_t f = poller->futures[i];
        if (i == 0) {
            fprintf(stderr, "  --busy--\n");
        }
        if (i == poller->busy) {
            fprintf(stderr, "  --done--\n");
        }
        if (i == poller->used) {
            fprintf(stderr, "  --free--\n");
        }
        if (f) {
            fprintf(stderr, "  %*zu => (%*d) %p (%s)\n",
                    w, i, w, f->slot, f, states[f->state]);
        }
        else {
            fprintf(stderr, "  %*zu => null\n", w, i);
        }
    }
    fprintf(stderr, "  --------\n");
}

static void green_poller_swap(green_poller_t poller, int lhs, int rhs)
{
    fprintf(stderr, "swapping %d and %d.\n", lhs, rhs);
    if (lhs == rhs) {
        return;
    }
    green_poller_dump(poller);
    green_future_t f1 = poller->futures[lhs];
    green_future_t f2 = poller->futures[rhs];
    green_assert(f1->slot == lhs);
    green_assert(f2->slot == rhs);
    poller->futures[lhs] = f2, f2->slot = lhs;
    poller->futures[rhs] = f1, f1->slot = rhs;
    green_poller_dump(poller);
}

green_poller_t green_poller_init(green_loop_t loop, size_t size)
{
    if ((loop == NULL) || (size == 0)) {
        return NULL;
    }
    green_assert(loop->refs > 0);

    green_poller_t poller = green_malloc(sizeof(struct green_poller));
    poller->loop = loop;
    poller->refs = 1;
    poller->futures = green_malloc(size * sizeof(green_future_t));
    poller->size = size;

    return poller;
}

size_t green_poller_size(green_poller_t poller)
{
    if (poller == NULL) {
        return 0;
    }
    green_assert(poller->loop != NULL);
    green_assert(poller->size >= poller->used);
    green_assert(poller->used >= poller->busy);
    return poller->size;
}

size_t green_poller_used(green_poller_t poller)
{
    if (poller == NULL) {
        return 0;
    }
    green_assert(poller->loop != NULL);
    green_assert(poller->size >= poller->used);
    green_assert(poller->used >= poller->busy);
    return poller->used;
}

size_t green_poller_done(green_poller_t poller)
{
    if (poller == NULL) {
        return 0;
    }
    green_assert(poller->loop != NULL);
    green_assert(poller->size >= poller->used);
    green_assert(poller->used >= poller->busy);
    return poller->used - poller->busy;
}

int green_poller_acquire(green_poller_t poller)
{
    if (poller == NULL) {
        return GREEN_EINVAL;
    }
    green_assert(poller->refs > 0);
    ++poller->refs;
    return GREEN_SUCCESS;
}

int green_poller_release(green_poller_t poller)
{
    fprintf(stderr, "green_poller_release()\n");

    if (poller == NULL) {
        return GREEN_EINVAL;
    }
    green_assert(poller->refs > 0);
    if (--poller->refs == 0) {
        for (int i = 0; i < poller->used; ++i) {
            poller->futures[i]->poller = NULL;
            poller->futures[i]->slot = -1;
            green_future_release(poller->futures[i]);
            poller->futures[i] = NULL;
        }
        green_free(poller->futures);
        poller->futures = NULL;
        poller->loop = NULL;
        green_free(poller);
    }
    return GREEN_SUCCESS;
}

int green_poller_add(green_poller_t poller, green_future_t future)
{
    fprintf(stderr, "green_poller_add()\n");

    // Check for required arguments.
    if ((poller == NULL) || (future == NULL)) {
        return GREEN_EINVAL;
    }

    // Event loop must match.
    if (future->loop != poller->loop) {
        return GREEN_EINVAL;
    }

    // Future cannot be registered twice.
    if (future->poller != NULL) {
        return GREEN_EALREADY;
    }

    // Cannot exceed maximum poller size.
    if (poller->used == poller->size) {
        return GREEN_ENFILE;
    }

    green_assert(poller->refs > 0);
    green_assert(future->refs > 0);
    green_assert(future->slot < 0);
    green_assert(poller->used >= 0);
    green_assert(poller->used <= poller->size);

    green_poller_dump(poller);

    green_future_acquire(future);
    poller->futures[poller->used] = future;
    future->slot = poller->used++;
    future->poller = poller;
    if (future->state == green_future_pending) {
        green_poller_swap(poller, future->slot, poller->busy++);
    }

    return GREEN_SUCCESS;
}

int green_poller_rem(green_poller_t poller, green_future_t future)
{
    fprintf(stderr, "green_poller_rem()\n");
    if ((poller == NULL) || (future == NULL)) {
        return GREEN_EINVAL;
    }
    if (future->poller != poller) {
        return GREEN_ENOENT;
    }

    green_assert(poller->refs > 0);
    green_assert(future->refs > 0);
    green_assert(poller->loop == future->loop);
    green_assert(future->poller == poller);
    green_assert(future->slot >= 0);

    if (future->slot >= poller->busy) {
        green_assert(future->state != green_future_pending);
        green_poller_swap(poller, future->slot, poller->used-1);
    }
    else {
        green_assert(future->state == green_future_pending);
        green_poller_swap(poller, future->slot, --poller->busy);
        green_poller_swap(poller, poller->busy, poller->used-1);
    }
    future->slot = -1;
    future->poller = NULL;
    poller->futures[--poller->used] = NULL;
    green_future_release(future);
    return GREEN_SUCCESS;
}

green_future_t green_poller_pop(green_poller_t poller)
{
    fprintf(stderr, "green_poller_pop()\n");

    if (poller == NULL) {
        return NULL;
    }
    green_poller_dump(poller);
    if ((poller->used == 0) || (poller->busy == poller->used)) {
        return NULL;
    }
    green_future_t f = poller->futures[poller->busy];
    green_assert(green_poller_rem(poller, f) == 0);
    return f;
}

green_future_t green_future_init(green_loop_t loop)
{
    if (loop == NULL) {
        return NULL;
    }
    green_future_t future = green_malloc(sizeof(struct green_future));
    future->loop = loop;
    future->state = green_future_pending;
    future->refs = 1;
    future->slot = -1;
    return future;
}

int green_future_acquire(green_future_t future)
{
    if (future == NULL) {
        return GREEN_EINVAL;
    }
    green_assert(future != NULL);
    green_assert(future->refs > 0);
    ++future->refs;
    return GREEN_SUCCESS;
}

int green_future_release(green_future_t future)
{
    if (future == NULL) {
        return GREEN_EINVAL;
    }
    fprintf(stderr, "green_future_release(%p, %d)\n", future, future->refs);
    // NOTE: async operations hold an implicit ref count, so there is no risk
    //       of async operations dereferencing a dangling pointer when
    //       attempting to resolve the future.
    green_assert(future != NULL);
    green_assert(future->refs > 0);
    if (--future->refs == 0) {
        green_assert(future->poller == NULL);
        green_free(future);
    }
    return GREEN_SUCCESS;
}

int green_future_done(green_future_t future)
{
    if (future == NULL) {
        return GREEN_EINVAL;
    }
    green_assert(future->refs > 0);
    return (future->state == green_future_complete) ||
           (future->state == green_future_aborted);
}

int green_future_set_result(green_future_t future, void * p, int i)
{
    if (future == NULL) {
        return GREEN_EINVAL;
    }
    green_assert(future->refs > 0);

    if (future->state == green_future_aborted) {
        return GREEN_ECANCELED;
    }
    if (future->state == green_future_complete) {
        return GREEN_EBADFD;
    }

    // Store result.
    future->result.i = i;
    future->result.p = p;

    // Mark as complete.
    future->state = green_future_complete;

    // Restore poller invariant.
    if (future->poller) {
        green_poller_swap(future->poller,
                          future->slot, --future->poller->busy);

        // TODO: resume coroutine blocked on poller, if any.
    }

    return GREEN_SUCCESS;
}

int green_future_result(green_future_t future, void ** p, int * i)
{
    if (future == NULL) {
        return GREEN_EINVAL;
    }
    green_assert(future->refs > 0);

    if (future->state == green_future_pending) {
        return GREEN_EBUSY;
    }
    if (future->state == green_future_aborted) {
        return GREEN_EBADFD;
    }
    if (p) {
        *p = future->result.p;
    }
    if (i) {
        *i = future->result.i;
    }
    return GREEN_SUCCESS;
}

int green_future_cancel(green_future_t future)
{
    // NOTE: when the async operation completes, the attempt to resolve the
    //       future will may omit to post a completion notification if it is
    //       possible to do so.  However, it's also possible that the
    //       completion notification is already in flight.  In that case, the
    //       coroutine that wakes up should also avoid to return control to
    //       application code as a result of the completion notification for a
    //       canceled future.
    if (future == NULL) {
        return GREEN_EINVAL;
    }
    green_assert(future->refs > 0);
    if (future->state != green_future_pending) {
        return GREEN_EBADFD;
    }
    future->state = green_future_aborted;
    return GREEN_SUCCESS;
}
