////////////////////////////////////////////////////////////////////////
// Copyright(c) libgreen contributors.  See LICENSE file for details. //
////////////////////////////////////////////////////////////////////////

#ifndef _GREEN_H__
#define _GREEN_H__

#include <stddef.h>

// Library version.
#define GREEN_MAJOR 0
#define GREEN_MINOR 1
#define GREEN_PATCH 0

// Compile-time version check helper.
#define GREEN_MAKE_VERSION(major, minor, patch) \
    ((major * 10000) + (minor * 100) + patch)

// Helper macro.
#define _GREEN_STRING(x) #x
#define GREEN_STRING(x) _GREEN_STRING(x)

// API version.
#define GREEN_VERSION \
    GREEN_MAKE_VERSION(GREEN_MAJOR, GREEN_MINOR, GREEN_PATCH)
#define GREEN_VERSION_STRING \
    GREEN_STRING(GREEN_MAJOR) "." \
    GREEN_STRING(GREEN_MINOR) "." \
    GREEN_STRING(GREEN_PATCH)

// Error codes.
#define GREEN_SUCCESS 0
#define GREEN_EINVAL 1
#define GREEN_ENOMEM 2
#define GREEN_EBUSY 3
#define GREEN_ECANCELED 4
#define GREEN_EALREADY 5
#define GREEN_ENOENT 6
#define GREEN_ENFILE 7
#define GREEN_EBADFD 8

// Lib version.
int green_version();
const char * green_version_string();

// Library setup and teardown.
int _green_init(int major, int minor);
#define green_init() \
    _green_init(GREEN_MAJOR, GREEN_MINOR)
int green_term();

// Loop setup and teardown.
typedef struct green_loop * green_loop_t;
green_loop_t green_loop_init();
int green_loop_acquire(green_loop_t loop);
int green_loop_release(green_loop_t loop);

// Coroutine methods.
typedef struct green_coroutine * green_coroutine_t;

green_coroutine_t _green_coroutine_init(
    green_loop_t loop, int(*method)(green_loop_t,void*),
    void * object, size_t stack_size, const char * source
);
#define green_coroutine_init(loop, method, object, stack_size) \
    _green_coroutine_init(loop, method, object, stack_size, \
                          __FILE__ ":" GREEN_STRING(__LINE__))

int _green_yield(green_loop_t loop, green_coroutine_t coro,
                 const char * source);
#define green_yield(loop, coro) \
    _green_yield(loop, coro, __FILE__ ":" GREEN_STRING(__LINE__))
int green_coroutine_result(green_coroutine_t coro);

int green_coroutine_acquire(green_coroutine_t coro);
int green_coroutine_release(green_coroutine_t coro);

// Future.
typedef struct green_future * green_future_t;
green_future_t green_future_init(green_loop_t loop);
int green_future_done(green_future_t future);
int green_future_canceled(green_future_t future);
int green_future_set_result(green_future_t future, void * p, int i);
int green_future_result(green_future_t future, void ** p, int * i);
int green_future_cancel(green_future_t future);

int green_future_acquire(green_future_t future);
int green_future_release(green_future_t future);

// Poller.
typedef struct green_poller * green_poller_t;
green_poller_t green_poller_init(green_loop_t loop, size_t size);
size_t green_poller_size(green_poller_t poller);
size_t green_poller_used(green_poller_t poller);
size_t green_poller_done(green_poller_t poller);
int green_poller_add(green_poller_t poller, green_future_t future);
int green_poller_rem(green_poller_t poller, green_future_t future);
green_future_t green_poller_pop(green_poller_t poller);

int green_poller_acquire(green_poller_t poller);
int green_poller_release(green_poller_t poller);

green_future_t _green_select(green_poller_t poller, const char * source);
#define green_select(poller) \
    green_select_ex(poller, timeout, __FILE__ ":" GREEN_STRING(__LINE__))

#endif // _GREEN_H__
