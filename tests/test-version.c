////////////////////////////////////////////////////////////////////////
// Copyright(c) libgreen contributors.  See LICENSE file for details. //
////////////////////////////////////////////////////////////////////////

#include <green.h>
#include "check.h"

// Make sure *compile-time* testing is supported.
#if GREEN_VERSION != GREEN_MAKE_VERSION(GREEN_MAJOR, GREEN_MINOR, GREEN_PATCH)
#   error Something is off.
#endif

int main()
{
    fprintf(stderr, "Library version: \"%s\".\n", GREEN_VERSION_STRING);

    // Compile-time and run-time checks should be equal since we're not
    // building in a compatibility configuration.
    check_eq(green_version(), GREEN_VERSION);
    check_str_eq(green_version_string(), GREEN_VERSION_STRING);

    // Make sure ordering make sense.
    check_eq(GREEN_MAKE_VERSION(1, 0, 0),
             GREEN_MAKE_VERSION(1, 0, 0));
    check_lt(GREEN_MAKE_VERSION(1, 0, 0),
             GREEN_MAKE_VERSION(1, 0, 1));
    check_lt(GREEN_MAKE_VERSION(1, 0, 1),
             GREEN_MAKE_VERSION(1, 1, 0));
    check_lt(GREEN_MAKE_VERSION(1, 0, 0),
             GREEN_MAKE_VERSION(1, 1, 0));
    check_lt(GREEN_MAKE_VERSION(1, 0, 0),
             GREEN_MAKE_VERSION(2, 0, 0));
    check_lt(GREEN_MAKE_VERSION(1, 1, 0),
             GREEN_MAKE_VERSION(2, 0, 0));

}
