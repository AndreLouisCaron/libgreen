////////////////////////////////////////////////////////////////////////
// Copyright(c) libgreen contributors.  See LICENSE file for details. //
////////////////////////////////////////////////////////////////////////

#include <green.h>
#include "check.h"

int main()
{
    check_eq(0, green_init());
    check_eq(0, green_term());
}