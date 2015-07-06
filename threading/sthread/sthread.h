/*
 * sthread.h
 *
 * SThread, a simple threading library
 */

#ifndef _STHREAD_H_
#define _STHREAD_H_

typedef int (*sthread_main_t)(void *);
typedef struct sthread_struct *sthread_t;
#define NO_THREAD ((sthread_t)NULL)

#include "sync.h"

/*
 * Initialize the sthread system.  Must be called once, before any
 * other sthread routines.
 */
int sthread_init(void);

/*
 * Creates and runs a new thread, which starts running at the given
 * main_routine.
 */
int sthread_create(sthread_t *t, sthread_main_t main_routine, void *arg);

/*
 * Returns the thread object for the thread calling it.
 */
sthread_t sthread_self(void);

/*
 * Suspend the calling thread.
 */
void sthread_suspend(void);

/*
 * Wake the previously suspended thread t.
 */
void sthread_wake(sthread_t t);

/*
 * Atomically tests and sets the given integer to 1.  Returns the
 * previous value.
 */
int test_and_set(volatile unsigned long *x);

#endif
