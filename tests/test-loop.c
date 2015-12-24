////////////////////////////////////////////////////////////////////////
// Copyright(c) libgreen contributors.  See LICENSE file for details. //
////////////////////////////////////////////////////////////////////////

#include "loop-fixture.h"

int test(green_loop_t loop)
{
    check_eq(green_loop_acquire(loop), 0);
    check_eq(green_loop_release(loop), 0);

    return EXIT_SUCCESS;
}

#include "loop-fixture.c"
