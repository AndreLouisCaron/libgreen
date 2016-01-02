////////////////////////////////////////////////////////////////////////
// Copyright(c) libgreen contributors.  See LICENSE file for details. //
////////////////////////////////////////////////////////////////////////

#include "loop-fixture.h"

int test(green_loop_t loop)
{
    green_poller_t poller = NULL;

    // Can't create poller without an event loop.
    poller = green_poller_init(NULL, 2);
    check_eq(poller, NULL);

    // Can't create poller that can't store any futures.
    poller = green_poller_init(loop, 0);
    check_eq(poller, NULL);

    // OK, good to go.
    poller = green_poller_init(loop, 2);
    check_ne(poller, NULL);
    check_eq(green_poller_size(poller), 2);
    check_eq(green_poller_used(poller), 0);
    check_eq(green_poller_done(poller), 0);

    // Poller is reference counted.
    check_eq(green_poller_acquire(poller), 0);
    check_eq(green_poller_release(poller), 0);

    // Methods are NULL-safe.
    check_eq(green_poller_acquire(NULL), GREEN_EINVAL);
    check_eq(green_poller_release(NULL), GREEN_EINVAL);

    // NULL poller has invalid size.
    check_eq(green_poller_size(NULL), 0);
    check_eq(green_poller_used(NULL), 0);
    check_eq(green_poller_done(NULL), 0);

    green_future_t f1 = green_future_init(loop);
    check_ne(f1, NULL);
    green_future_t f2 = green_future_init(loop);
    check_ne(f2, NULL);
    green_future_t f3 = green_future_init(loop);
    check_ne(f3, NULL);

    // Always pop NULL from NULL poller.
    check_eq(green_poller_pop(NULL), NULL);

    // Always pop NULL from empty poller.
    check_eq(green_poller_pop(poller), NULL);

    // Arguments are required.
    check_eq(green_poller_add(NULL, f1), GREEN_EINVAL);
    check_eq(green_poller_add(poller, NULL), GREEN_EINVAL);
    check_eq(green_poller_rem(NULL, f1), GREEN_EINVAL);
    check_eq(green_poller_rem(poller, NULL), GREEN_EINVAL);

    // Can't remove a future that's not in the poller.
    check_eq(green_poller_rem(poller, f3), GREEN_ENOENT);

    // Poller has a maximum size.
    check_eq(green_poller_add(poller, f1), 0);
    check_eq(green_poller_used(poller), 1);
    check_eq(green_poller_done(poller), 0);
    check_eq(green_poller_add(poller, f2), 0);
    check_eq(green_poller_used(poller), 2);
    check_eq(green_poller_done(poller), 0);
    check_eq(green_poller_add(poller, f3), GREEN_ENFILE);
    check_eq(green_poller_used(poller), 2);
    check_eq(green_poller_done(poller), 0);

    // Double-registration is prohibited.
    check_eq(green_poller_add(poller, f1), GREEN_EALREADY);
    check_eq(green_poller_add(poller, f2), GREEN_EALREADY);
    check_eq(green_poller_used(poller), 2);
    check_eq(green_poller_done(poller), 0);

    // Always pop NULL from poller unless something is ready.
    check_eq(green_poller_pop(poller), NULL);
    check_eq(green_poller_used(poller), 2);
    check_eq(green_poller_done(poller), 0);

    // When a future is completed, we can pop it.
    check_eq(green_future_set_result(f1, NULL, 0), 0);
    check_eq(green_future_result(f1, NULL, NULL), 0);
    check_eq(green_poller_used(poller), 2);
    check_eq(green_poller_done(poller), 1);
    check_eq(green_poller_pop(poller), f1);
    check_eq(green_poller_used(poller), 1);
    check_eq(green_poller_done(poller), 0);

    // We can add a completed future.
    check_eq(green_poller_add(poller, f1), 0);
    check_eq(green_poller_used(poller), 2);
    check_eq(green_poller_done(poller), 1);
    check_eq(green_poller_pop(poller), f1);

    // Always pop NULL from poller unless something is ready.
    check_eq(green_poller_pop(poller), NULL);

    // We can remove a pending future.
    check_eq(green_poller_rem(poller, f2), 0);
    check_eq(green_poller_add(poller, f2), 0);

    // When a future is completed, we can pop it.
    check_eq(green_future_set_result(f2, NULL, 0), 0);
    check_eq(green_future_result(f2, NULL, NULL), 0);
    check_eq(green_poller_used(poller), 1);
    check_eq(green_poller_done(poller), 1);

    // When the last future is completed, we can pop it.
    check_eq(green_poller_pop(poller), f2);
    check_eq(green_poller_used(poller), 0);
    check_eq(green_poller_done(poller), 0);

    // Always pop NULL from empty poller.
    check_eq(green_poller_pop(poller), NULL);

    // Can't add a future to a poller attached to a different event loop.
    green_loop_t loop2 = green_loop_init();
    green_future_t f4 = green_future_init(loop2);
    check_eq(green_poller_add(poller, f4), GREEN_EINVAL);
    check_eq(green_future_release(f4), 0); f4 = NULL;
    check_eq(green_loop_release(loop2), 0); loop2 = NULL;

    // Future will be released even if the poller holds the last reference.
    //
    // NOTE: the real assert that proves the future refernce is released
    //       correctly will be done by the memory checker.
    check_eq(green_poller_add(poller, f3), 0);

    // Done.
    check_eq(green_future_release(f3), 0); f3 = NULL;
    check_eq(green_future_release(f2), 0); f2 = NULL;
    check_eq(green_future_release(f1), 0); f1 = NULL;
    check_eq(green_poller_release(poller), 0); poller = NULL;

    return EXIT_SUCCESS;
}

#include "loop-fixture.c"
