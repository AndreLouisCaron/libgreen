////////////////////////////////////////////////////////////////////////
// Copyright(c) libgreen contributors.  See LICENSE file for details. //
////////////////////////////////////////////////////////////////////////

#include <green.h>

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
