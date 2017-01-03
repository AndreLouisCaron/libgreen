////////////////////////////////////////////////////////////////////////
// Copyright(c) libgreen contributors.  See LICENSE file for details. //
////////////////////////////////////////////////////////////////////////

#include "loop-fixture.h"

int main(int argc, char ** argv)
{
    green_loop_t loop = green_loop_init();
    check_ne(loop, NULL);
    check_eq(test(loop), 0);
    check_eq(green_loop_release(loop), 0); loop = NULL;

    return EXIT_SUCCESS;
}
