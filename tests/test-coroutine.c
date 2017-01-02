////////////////////////////////////////////////////////////////////////
// Copyright(c) libgreen contributors.  See LICENSE file for details. //
////////////////////////////////////////////////////////////////////////


#include "loop-fixture.h"


#define N 10
#define K 10


int I = 0;


int mytask(green_loop_t loop, green_task_t task, void * object)
{
    check_ne(loop, NULL);
    check_ne(object, NULL);

    int n = *(int*)object;

    // Yield a few times.
    for (int i = 0; i < K; ++i)
    {
        // IMPORTANT: this check verifies the FIFO scheduling property!
        check_eq((I++ % N), n - 1);

        check_eq(green_yield(loop), 0);
        check_eq(green_task_state(task), green_task_running);
    }

    return 0;
}


int test(green_loop_t loop)
{
    // Create N tasks.
    fprintf(stderr, "spawning tasks.\n");
    fprintf(stderr, "---------------\n");
    int n[N];
    green_task_t tasks[N];
    for (int i = 0; (i < N); ++i) {
        n[i] = i + 1;
        tasks[i] = green_spawn(loop, mytask, &n[i], 0);
        check_ne(tasks[i], NULL);
        check_eq(green_task_state(tasks[i]), green_task_pending);
    }
    fprintf(stderr, "\n");

    // Resume each task K times.
    for (int i = 0; i <= K; ++i) {
        fprintf(stderr, "tick.\n");
        fprintf(stderr, "---\n");
        check_eq(green_tick(loop), 0);
        fprintf(stderr, "\n");
    }

    // Release all task objects.
    fprintf(stderr, "deleting tasks.\n");
    fprintf(stderr, "---------------\n");
    for (int i = 0; (i < N); ++i) {
        check_eq(green_task_state(tasks[i]), green_task_stopped);
        check_eq(green_task_release(tasks[i]), 0);
        tasks[i] = NULL;
    }
    fprintf(stderr, "\n");

    return EXIT_SUCCESS;
}


#include "loop-fixture.c"
