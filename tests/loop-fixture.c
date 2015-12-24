////////////////////////////////////////////////////////////////////////
// Copyright(c) libgreen contributors.  See LICENSE file for details. //
////////////////////////////////////////////////////////////////////////

#include "loop-fixture.h"

int main(int argc, char ** argv)
{
    fprintf(stderr, "green include version: \"%s\".\n", GREEN_VERSION_STRING);
    fprintf(stderr,
            "green library version: \"%s\".\n", green_version_string());

    if (green_init()) {
        fprintf(stderr, "green_init()\n");
        return EXIT_FAILURE;
    }
    green_loop_t loop = green_loop_init();
    if (loop == NULL) {
        fprintf(stderr, "green_loop_init()\n");
        return EXIT_FAILURE;
    }

    if (test(loop) != 0) {
        fprintf(stderr, "green_test()\n");
        return EXIT_FAILURE;
    }

    if (green_loop_release(loop)) {
        fprintf(stderr, "green_loop_release()\n");
        return EXIT_FAILURE;
    }
    loop = NULL;
    if (green_term()) {
        fprintf(stderr, "green_term()\n");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
