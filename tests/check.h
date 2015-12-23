////////////////////////////////////////////////////////////////////////
// Copyright(c) libgreen contributors.  See LICENSE file for details. //
////////////////////////////////////////////////////////////////////////

#ifndef _CHECK_H__
#define _CHECK_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define check(exp)                              \
    do { \
        if (!(exp)) { \
            fprintf(stderr, "Test failed at %s:%d: '%s'.\n", \
                    __FILE__, __LINE__, #exp); \
            abort(); \
        } \
    } while(0)

#define check_eq(lhs, rhs) check((lhs) == (rhs))
#define check_ne(lhs, rhs) check((lhs) != (rhs))
#define check_lt(lhs, rhs) check((lhs) <  (rhs))
#define check_le(lhs, rhs) check((lhs) <= (rhs))
#define check_gt(lhs, rhs) check((lhs) >  (rhs))
#define check_ge(lhs, rhs) check((lhs) >= (rhs))

#define check_str_eq(lhs, rhs) check(strcmp(lhs, rhs) == 0)

#endif // _CHECK_H__
