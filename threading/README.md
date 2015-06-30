## Group Programming Assignment (60 pts)

This programming assignment has two parts. In Part A, you'll extend a simple user space threading library to support semaphores; in Part B, you'll implement a readers-writer lock in the linux kernel that favors writers. For part A, we will provide you the files that you can get started with (see below). For part B, you will use the same development environment as you did in HW2 by setting up your environment by following the instructions here. Make sure you start with a new kernel without your changes from HW2.

For the user threads package part of this assignment, be sure to Submit a Makefile and a README file with your code. The Makefile should have at least a default target that builds all assigned programs. The README should explain the details of your solution, describe any way in which your solution differs from what was assigned, and state any assumptions you made (5 pts). Also, use the -Wall option to gcc when compiling. You will lose points if your code produces warnings when compiled.

For the kernel part, commit your source code changes using the git add and git commit commands you learned from the previous assignment. Remember to add your new file to the code repository. Then use the git diff command to get the code change. You should compare with the original version, so please do

    git diff 365a6e06
Here 365a6e06 refers to the initial version of the kernel source before you make any modifications. Then submit the code difference in a .patch file. The file name should be hw3-group<your group>-code.patch.

Part A: Synchronization and threads in userspace (25 pts)

In this problem you will add synchronization functionality to the implementation of SThreads, a simple threading library. Get the source files found in this directory. The files provide the source, header, and Makefile for the SThreads library. The publicly available functions and datatypes are in the file sthread.h. The library is used as follows:

Threads are manipulated using variables of type sthread_t. Consider sthread_t to be an opaque data type (i.e. only functions in sthread.c get to know what it really is).

sthread_init() must be called exactly once, as the first thing in main(). It returns 0 normally, and -1 on error.

A new thread is created using the function:

int sthread_create(sthread_t *t, sthread_main_t main, void *arg).

The first argument is where the sthread_t object is returned. The second is the function the new thread should run. The third argument is passed to this function. sthread_create() returns 0 normally and -1 on error.

You are also provided with sthread_self(), which returns the sthread_t associated with the currently running thread, as well as sthread_suspend(), which puts the currently running thread to sleep, and sthread_wake(sthread_t t), which wakes up a thread, given the thread's sthread_t object. Note that for the SThreads library, first waking up a running thread and then suspending it leaves the thread in a running state.

Usage example:

```
#define _REENTRANT
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include "sthread.h"

int
threadmain(void *arg)
{
  int threadno = (int)arg;
  for (;;) {
    printf("thread %d: I'm going to sleep\n", threadno);
    sthread_suspend();
    printf("thread %d: I woke up!\n", threadno);
  }
  return 0;
}

int
main(int argc, char *argv[])
{
  sthread_t thr1, thr2;

  if (sthread_init() == -1)
    fprintf(stderr, "%s: sthread_init: %s\n", argv[0], strerror(errno));

  if (sthread_create(&thr1, threadmain, (void *)1) == -1)
    fprintf(stderr, "%s: sthread_create: %s\n", argv[0], strerror(errno));

  if (sthread_create(&thr2, threadmain, (void *)2) == -1)
    fprintf(stderr, "%s: sthread_create: %s\n", argv[0], strerror(errno));

  sleep(1);
  sthread_wake(thr1);
  sleep(1);
  sthread_wake(thr2);
  sleep(1);
  sthread_wake(thr1);
  sthread_wake(thr2);
  sleep(1);

  return 0;
}

```
Note the #define _REENTRANT at the top of the file. This is necessary in any multithreaded program, as it tells subsequent include files to use reentrant versions of library functions and thread-safe variables. For example, the errno variable, since it is a global variable, is normally not thread-safe. A function in thread A can set it on error, and thread B can set it to something else before the caller in thread A ever gets to read the value. This is an example of a race condition. If you #define _REENTRANT, though, errno is redefined to (*__errno_location()), which references a different address for each thread. An implementation of this function is provided in sthread.c.

You will also be provided with the function test_and_set(int *x), which atomically sets the integer that x points to to 1, and returns its previous value. Using sthread_suspend(), sthread_wake(), and test_and_set(), you are to implement the missing semaphore primitives in the SThreads library.

You may use test_and_set() to implement spinlocks, in which you repeatedly call test_and_set() and a no-op in a tight loop, waiting for the test result to be 0. sched_yield() is a good no-op function to use, but you don't have to worry about precisely what it does (not until the next homework, anyway!). Note that you can use spinlocks to synchronize access on your semaphores' shared data structures, but not to implement the semaphores themselves. In other words, if I call sthread_sem_down() on an unavailable semaphore (the count of the semaphore is negative or 0), I should suspend rather than spinning until the semaphore is available (the count of the semaphore is positive).

> Exercise 1. Understand the SThreads library. Run the usage example and understand why a particular sequence of output messages is generated by the execution.
Now comes the fun part. For this assignment, implement counting semaphores in the SThreads library. Put your function implementations in a single file, called sync.c, and your structure definitions in sync.h. Skeleton files are provided, but you may have to add functions and datatypes to these files. You shouldn't have to change sthread.c or sthread.h. In fact, you shouldn't even have to look at the implementations of the functions in sthread.c (but feel free if you want to, of course). Unless otherwise noted, all functions should return 0 on success and -1 on error.



The prototypes for the five functions you must implement are found in sync.h, and are all named sthread_sem_*. You must define struct sthread_sem_struct. sthread_sem_init(sthread_sem_t *sem, int count) and sthread_sem_destroy() should be used to initialize and free resources related to this struct. The argument int count in sthread_sem_init(sthread_sem_t *sem, int count) indicates the count of the semaphore. The value of int count could be any positive number.

sthread_sem_down() will decrement the semaphore by 1 if the value of which is greater than 0 (these two steps must be atomic), or it will block until another thread releases the semaphore and wakes it up. sthread_sem_up() will increment the value of semaphore by 1 if nobody is being blocked on it; if there are threads waiting for the semaphore, it should wake up one of the waiting threads; these two steps must also be atomic. sthread_sem_try_down() should obtain the semaphore and return 0 if the semaphore is available, otherwise return non-zero immediately. This function does not cause the caller to block.



> Exercise 2. Implement semaphores in the SThreads library.
Note that your implementation should not allow starvation of any thread waiting for the semaphore. That is, if two threads are repeatedly obtaining and releasing a semaphore, that should not prevent a third thread from eventually getting the semaphore. (Of course, if one thread never release the semaphore, the others will starve, but that's a programming error, i.e. not our problem!)



> Exercise 3. Write a few testcases to test your semaphore implementation.
Part B: Synchronization in the Linux kernel (35 pts)

Energy consumption is a critical issue when designing mobile applications. Data transfer over cellular radio networks is one of the most energy intensive operations on mobile devices, and so cellular protocols such as 3G (HSPA) and LTE are designed to allow phones to keep their radio transmitters and receivers turned off when they are not needed.

When the battery life of a mobile device is below a certain critical level, we might want to restrict which processes can access the network. If non-critical applications (e.g. social network updates) can be blocked from accessing the network when power supply is limited, the mobile device can have a potentially longer operation span. This functionality can be achieved if every process that wishes to use the network needs to acquire a read lock, and a process that monitors the device battery life can obtain an exclusive lock if the battery life is below a critical level. When such a process acquires an exclusive lock, no other process can acquire a regular lock, which blocks subsequent process from accessing the network.

For this problem, you will implement a regular-exclusive (reader-writer) "net-lock" kernel synchronization primitive that prioritizes exclusive lock holders over regular lock holders. The locking primitive should be implemented in kernel/netlock.c file, and should export the following system calls to acquire and release the net lock:

     /* Syscall 378. Acquire netlock. type indicates
        whether a regular or exclusive lock is needed. Returns 0 on success 
  and -1 on failure.  
      */
     int netlock_acquire(netlock_t type);

     /* Syscall 379. Release netlock. Return 0 on success and -1 on failure.  
      */
     int netlock_release(void);

     enum __netlock_t {
        NET_LOCK_N, /* Placeholder for no lock */
  NET_LOCK_R, /* Indicates regular lock */
  NET_LOCK_E, /* Indicates exclusive lock */
     } ;
     typedef enum __netlock_t netlock_t;
  
The specification for this part is as follows. You may assume that all processes that intend to use the network must call netlock_acquire in regular (read) mode. The calls to acquire the lock in regular mode should succeed immediately as long as no process is holding an exclusive (write) lock or is waiting for an exclusive lock. Multiple processes may request the lock in regular mode, and all of them should be granted the lock if this condition is met. If a process is currently holding or waiting for an exclusive lock, subsequent calls to netlock_acquire in regular mode must block. All processes waiting for regular locks must be granted the lock once the condition for granting the regular locks is fulfilled.

Only one process may hold an exclusive lock at any given time. When a process requests an exclusive lock, it must wait until processes currently holding regular or exclusive locks release the locks using the netlock_release system call. Processes requesting an exclusive lock get priority over processes waiting to get regular locks. If multiple processes request exclusive locks, they should be granted locks in the order of their request.

Read LKD Ch. 4, pg 58-61 to learn about wait queues. Read LKD Ch. 9 and Ch. 10 to understand Linux synchronization methods. Note that unlike the userspace semaphores you implemented for question 1, the synchronization primitives in Linux generally do not have owners and are not recursive. No owners means that there's nothing stopping any piece of code from unlocking every lock in the kernel (and bringing the kernel to a fiery screeching halt, probably). Not recursive means that the owner of a lock (even if there was such a thing) can't obtain the lock twice.



> Exercise 4. Implement net-lock in Linux. For the purpose of this assignment, your implementation shouldn't use the Linux kernel's reader-writer spin-locks or reader-writer semaphores.
Note that your implementation must favor writers over readers: whenever there is a thread waiting to acquire the lock in write mode, you should let this thread acquire the lock as soon as possible, To illustrate this point, consider the following example. Suppose the lock is held in read mode and a thread A is waiting to acquire the lock in write mode. Another thread B comes, trying to acquire the lock in read mode. In an implementation that favors readers over writers, thread B can immediately acquire the lock in read mode and proceed. However, in your implementation, thread B must wait and thread A should grab the lock first.



> Exercise 5. Write a few testcases to test your net-lock implementation. Your testcases should run in user-space (see the testcase of the last programming assignment).
Once the net-lock is implemented, we can modify the Linux kernel's network-related code to allow only processes holding the net-lock to send or receive packets. We do not require you to implement this functionality because understanding and modifying network-related code is outside the scope of this assignment. However, we welcome you to give it a try. If you figure out how to implement this functionality, be sure to let us know by emailing your patch or design to the course staff mailing list.
