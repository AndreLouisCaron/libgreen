////////////////////////////////////////////////////////////////////////
// Copyright(c) libgreen contributors.  See LICENSE file for details. //
////////////////////////////////////////////////////////////////////////

#include "loop-fixture.h"

int test(green_loop_t loop)
{
    green_future_t f = NULL;

    // Poller is required.
    f = green_future_init(NULL);
    check_eq(f, NULL);

    // Common case.
    f = green_future_init(loop);
    check_ne(f, NULL);

    void * p = NULL;
    int i = 0;

    // Future is required.
    check_eq(green_future_result(NULL, &p, &i), GREEN_EINVAL);
    check_eq(p, NULL);
    check_eq(i, 0);

    // Future is required.
    check_eq(green_future_set_result(NULL, p, i), GREEN_EINVAL);

    // Future is required.
    check_eq(green_future_acquire(NULL), GREEN_EINVAL);
    check_eq(green_future_release(NULL), GREEN_EINVAL);

    // Check result before completion is not supported.
    check_eq(green_future_result(f, &p, &i), GREEN_EBUSY);

    // Function is NULL-proof.
    check_ne(green_future_done(NULL), 0);

    // Future is not done until set_result is called.
    check_eq(green_future_done(f), 0);

    // Once complete, we should be able to get the result.
    check_eq(green_future_set_result(f, &i, 7), 0);
    check_eq(green_future_result(f, &p, &i), 0);
    check_eq(p, &i);
    check_eq(i, 7);

    // After set_result, future is done.
    check_ne(green_future_done(f), 0);

    // We can get a sub-set of the result.
    p = NULL;
    i = 0;
    check_eq(green_future_result(f, &p, NULL), 0);
    check_eq(p, &i);
    check_eq(i, 0);

    // We can get a sub-set of the result.
    p = NULL;
    i = 0;
    check_eq(green_future_result(f, NULL, &i), 0);
    check_eq(p, NULL);
    check_eq(i, 7);

    // Setting the result again is prohibited.
    check_eq(green_future_set_result(f, &i, 7), GREEN_EBADFD);

    // Let's get us another future.
    check_eq(green_future_release(f), 0); f = NULL;
    f = green_future_init(loop);
    check_ne(f, NULL);

    // Future is required.
    check_eq(green_future_cancel(NULL), GREEN_EINVAL);

    // Once canceled, the future should be done.
    p = NULL;
    i = 0;
    check_eq(green_future_cancel(f), 0);
    check_eq(green_future_result(f, &p, &i), GREEN_EBADFD);
    check_ne(green_future_done(f), 0);
    check_eq(p, NULL);
    check_eq(i, 0);

    // Cannot be canceled twice.
    check_eq(green_future_cancel(f), GREEN_EBADFD);

    // Cannot complete the future after it has been canceled.
    check_eq(green_future_set_result(f, NULL, 0), GREEN_ECANCELED);

    // After set_result, future is done.
    check_ne(green_future_done(f), 0);

    // Done.
    check_eq(green_future_release(f), 0); f = NULL;

    return EXIT_SUCCESS;
}

#include "loop-fixture.c"
