////////////////////////////////////////////////////////////////////////
// Copyright(c) libgreen contributors.  See LICENSE file for details. //
////////////////////////////////////////////////////////////////////////

#include <green.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
        return 1;
    }
    if (minor > GREEN_MINOR) {
        return 1;
    }
    return 0;
}

int green_term()
{
    return 0;
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
    return 0;
}

int green_loop_release(green_loop_t loop)
{
    green_assert(loop != NULL);
    green_assert(loop->refs > 0);

    if (--(loop->refs) > 0) {
        return 0;
    }

    green_assert(loop != NULL);

    green_assert(loop->coroutines == 0);
    green_free(loop);

    return 0;
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

    return 0;
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
    return 0;
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
    return 0;
}
