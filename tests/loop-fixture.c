////////////////////////////////////////////////////////////////////////
// Copyright(c) libgreen contributors.  See LICENSE file for details. //
////////////////////////////////////////////////////////////////////////

#include "loop-fixture.h"

int main(int argc, char ** argv)
{
    fprintf(stderr, "green include version: \"%s\".\n", GREEN_VERSION_STRING);
    fprintf(stderr,
            "green library version: \"%s\".\n", green_version_string());

    check_eq(green_init(), 0);
    green_loop_t loop = green_loop_init();
    check_ne(loop, NULL);
    check_eq(test(loop), 0);
    check_eq(green_loop_release(loop), 0); loop = NULL;
    check_eq(green_term(), 0);

    return EXIT_SUCCESS;
}
