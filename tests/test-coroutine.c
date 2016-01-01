////////////////////////////////////////////////////////////////////////
// Copyright(c) libgreen contributors.  See LICENSE file for details. //
////////////////////////////////////////////////////////////////////////

#include "loop-fixture.h"

int mycoroutine(green_loop_t loop, void * object)
{
    fprintf(stderr, "in coroutine (%d).\n", __COUNTER__);
    if (loop == NULL) {
        return 1;
    }
    if (object != NULL) {
        return 1;
    }

    if (green_yield(loop, NULL)) {
        return 2;
    }
    fprintf(stderr, "in coroutine (%d).\n", __COUNTER__);

    return 777;
}

int test(green_loop_t loop)
{
    fprintf(stderr, "spawning coroutine.\n");

    green_coroutine_t coro = green_coroutine_init(loop, mycoroutine, NULL, 0);
    check_ne(coro, NULL);

    fprintf(stderr, "yielding to coroutine.\n");
    check_eq(green_yield(loop, coro), 0);

    // Temporarily increase ref count.
    check_eq(green_coroutine_acquire(coro), 0);
    check_eq(green_coroutine_release(coro), 0);

    fprintf(stderr, "yielding to coroutine (again).\n");
    check_eq(green_yield(loop, coro), 0);

    fprintf(stderr, "deleting coroutine.\n");
    check_eq(green_coroutine_result(coro), 777);

    check_eq(green_coroutine_release(coro), 0);
    coro = NULL;

    return EXIT_SUCCESS;
}

#include "loop-fixture.c"
