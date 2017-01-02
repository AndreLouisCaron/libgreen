////////////////////////////////////////////////////////////////////////
// Copyright(c) libgreen contributors.  See LICENSE file for details. //
////////////////////////////////////////////////////////////////////////

#ifndef _GREEN_H__
#define _GREEN_H__

#include <stddef.h>

// Helper macro.
#define _GREEN_STRING(x) #x
#define GREEN_STRING(x) _GREEN_STRING(x)

// Error codes.
#define GREEN_SUCCESS 0
#define GREEN_ENOMEM 1

// Loop setup and teardown.
typedef struct green_loop * green_loop_t;
green_loop_t _green_loop_init(char * source);
#define green_loop_init() \
    _green_loop_init(__FILE__ ":" GREEN_STRING(__LINE__))
int green_loop_acquire(green_loop_t loop);
int green_loop_release(green_loop_t loop);
int _green_tick(green_loop_t loop,
                const char * source);
#define green_tick(loop) \
    _green_tick(loop, __FILE__ ":" GREEN_STRING(__LINE__))
int _green_yield(green_loop_t loop,
                 const char * source);
#define green_yield(loop) \
    _green_yield(loop, __FILE__ ":" GREEN_STRING(__LINE__))

// Task methods.
typedef struct green_task * green_task_t;

typedef enum green_task_state {
    green_task_blocked,  // waiting for something to complete.
    green_task_pending,  // ready to run in next tick.
    green_task_running,  // currently running.
    green_task_stopped,  // will never run again.
} green_task_state_t;

green_task_t _green_spawn(
    green_loop_t loop,
    int(*method)(green_loop_t,green_task_t,void*),
    void * object, size_t stack_size, const char * source
);
#define green_spawn(loop, method, object, stack_size) \
    _green_spawn(loop, method, object, stack_size, \
                 __FILE__ ":" GREEN_STRING(__LINE__))
green_task_state_t green_task_state(green_task_t task);
int green_task_acquire(green_task_t task);
int green_task_release(green_task_t task);

#endif // _GREEN_H__
