////////////////////////////////////////////////////////////////////////
// Copyright(c) libgreen contributors.  See LICENSE file for details. //
////////////////////////////////////////////////////////////////////////


// Required for using ucontext.
#define _XOPEN_SOURCE 500


#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>


ucontext_t hub;
ucontext_t coro;
int coro_status = 0;
void * P = NULL;
int stack_size = 64 * 1024;  // SIGSTKSZ is too small on linux.


// NOTE: according to the ucontext specification, we can only pass int
//       arguments to the coroutine.  Use a pair of ints in case `sizeof(void*)
//       != sizeof(int)`.
typedef union {
    int i[2];
    void * p;
} coroutine_args;


void coroutine(int i0, int i1)
{
    // Decode pointer argument.
    coroutine_args args;
    args.i[0] = i0;
    args.i[1] = i1;
    void * p = args.p;

    int i = 0;

    fprintf(stderr, "coro: enter.\n");
    if (p != P) {
        fprintf(stderr, "coro: p != P\n");
        coro_status = 1;
        return;
    }

    fprintf(stderr, "coro: yield.\n");
    i++;
    if (swapcontext(&coro, &hub) < 0) {
        fprintf(stderr, "coro: swapcontext()\n");
        coro_status = 1;
        return;
    }

    fprintf(stderr, "coro: check.\n");
    if (i != 1) {
        fprintf(stderr, "coro: stack corruption detected.\n");
        coro_status = 1;
        return;
    }

    fprintf(stderr, "coro: leave.\n");
}


int main()
{
    int i = 0;

    fprintf(stderr, "main: enter.\n");

    // Store the pointer we pass as an argument to the coroutine to make sure
    // the coroutine can check that it is passed safely.
    P = &hub;

    // Encode pointer argument.
    coroutine_args args;
    args.p = &hub;

    // Spawn the coroutine (delayed start).
    fprintf(stderr, "main: spawn.\n");
    if (getcontext(&coro) < 0) {
        fprintf(stderr, "main: getcontext()\n");
        return EXIT_FAILURE;
    }
    void * stack = malloc(stack_size);
    if (stack == NULL) {
        fprintf(stderr, "main: malloc()\n");
        return EXIT_FAILURE;
    }
    coro.uc_link = &hub;
    coro.uc_stack.ss_sp = stack;
    coro.uc_stack.ss_size = stack_size;
    makecontext(&coro, (void(*)())coroutine, 2, args.i[0], args.i[1]);

    fprintf(stderr, "main: stack @ %p.\n", coro.uc_stack.ss_sp);

    // Yield to the coroutine until it yields back.
    i++;
    fprintf(stderr, "main: yield (1).\n");
    if (swapcontext(&hub, &coro) < 0) {
        fprintf(stderr, "main: swapcontext()\n");
        return EXIT_FAILURE;
    }
    if (coro_status != 0) {
        fprintf(stderr, "main: coroutine error\n");
        return EXIT_FAILURE;
    }

    // Check for stack corruption.
    fprintf(stderr, "main: check.\n");
    if (i != 1) {
        fprintf(stderr, "main: stack corruption detected.\n");
        return EXIT_FAILURE;
    }

    // Yield back to the coroutine to let it finish.
    i++;
    fprintf(stderr, "main: yield (2).\n");
    if (swapcontext(&hub, &coro) < 0) {
        fprintf(stderr, "main: swapcontext()\n");
        return EXIT_FAILURE;
    }
    if (coro_status != 0) {
        fprintf(stderr, "main: coroutine error\n");
        return EXIT_FAILURE;
    }

    // Check for stack corruption.
    fprintf(stderr, "main: check.\n");
    if (i != 2) {
        fprintf(stderr, "main: stack corruption detected.\n");
        return EXIT_FAILURE;
    }

    // Release the coroutine's stack.
    //
    // NOTE: we can't rely on `coro.uc_stack.ss_sp` because the ucontext
    //       implementation is free to manipulate this at will and there is no
    //       guarantee that this will be the pointer we allocated.
    fprintf(stderr, "main: cleanup.\n");
    fprintf(stderr, "main: stack @ %p.\n", coro.uc_stack.ss_sp);
    free(stack);
    coro.uc_stack.ss_sp = NULL;

    fprintf(stderr, "main: leave.\n");
    return EXIT_SUCCESS;
}
