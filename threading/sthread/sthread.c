/*
 * sthread.c
 *
 * SThread, a simple threading library
 */

#define _REENTRANT

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sched.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <linux/sched.h>
#include "sthread.h"

/* make the max stack size 4MB */
#define DEF_STACK_SIZE 4194304
/* stack size for manager thread */
#define UTIL_STACK_SIZE 65536


/*
 * Atomic operations for x86 architecture.
 */
static inline int test_and_set_bit(volatile unsigned long *addr)
{
  int oldval;
  __asm__ __volatile__("xchgl %k0, %k1"
      : "=r"(oldval), "+m"(*(addr)) /* output */
      : "0"(1)            /* input */
      : "memory"  /* clobbered: changing contents of memory */
      );
  return oldval;
}
static inline void clear_bit(volatile unsigned long *addr)
{
  unsigned long oldval;
  __asm__ __volatile__("xchgl %k0, %k1"
      : "=r"(oldval), "+m"(*(addr)) /* output */
      : "0"(0)            /* input */
      : "memory"  /* clobbered: changing contents of memory */
      );
}


/*
 * struct sthread_struct encompasses all per-thread state.
 * Although sthread_t is typedef'ed to struct sthread_struct *,
 * conceptually it's an opaque handle to a thread.
 */
struct sthread_struct {
  int pid;
  sthread_main_t main;
  int running;
  int status;
  int err;
  int pipe[2];
  int stacksize;
  void *stack;
  struct sthread_struct *next;
};

#define MAIN_THREAD_INIT    \
{         \
  pid:    0,    \
  main:   NULL,   \
  running:  1,    \
  status:   0,    \
  err:    0,    \
  pipe:   { -1, -1 }, \
  stacksize:  0,    \
  stack:    NULL,   \
  next:   NULL,   \
}

#define MANAGER_THREAD_INIT   \
{         \
  pid:    0,    \
  main:   NULL,   \
  running:  0,    \
  status:   0,    \
  err:    0,    \
  pipe:   { -1, -1 }, \
  stacksize:  0,    \
  stack:    NULL,   \
  next:   &main_thread, \
}


/*
 * types for manager messages
 */

enum manager_msg_type {
  MSG_CREATE,
  MSG_THREAD_EXIT,
  MSG_ALL_EXIT,
};

union manager_msg_u {
  enum manager_msg_type type;
  struct {
    enum manager_msg_type type;
    sthread_t caller;
    sthread_main_t main;
    sthread_t thread;
    void *arg;
  } create;
  struct {
    enum manager_msg_type type;
    sthread_t caller;
  } thread_exit;
  struct {
    enum manager_msg_type type;
    sthread_t caller;
  } all_exit;
};

typedef union manager_msg_u manager_msg_t;


/* start_struct is just for passing stuff to the functions run by clone */
struct start_struct {
  void *arg;
  struct sthread_struct *thread;
};


/*
 * We keep a linked list of struct sthread_struct.  The main thread is
 * always last, and the manager thread is second to last. New threads
 * are added to the front of the list.
 */
static struct sthread_struct main_thread = MAIN_THREAD_INIT;
static struct sthread_struct manager_thread = MANAGER_THREAD_INIT;
static struct sthread_struct *list_hd = &manager_thread;
static volatile unsigned long list_lock = 0;


/*
 * Thread list management routines
 */

static void threadlist_lock(void)
{
  while (test_and_set_bit(&list_lock))
    sched_yield();
}

static void threadlist_unlock(void)
{
  clear_bit(&list_lock);
}

static struct sthread_struct * threadlist_findbypid(int pid)
{
  struct sthread_struct *p;
  threadlist_lock();
  for (p = list_hd; p; p = p->next) {
    if (p->pid == pid)
      break;
  }
  threadlist_unlock();
  return p;
}

static int threadlist_add(struct sthread_struct *t)
{
  threadlist_lock();
  t->next = list_hd;
  list_hd = t;
  threadlist_unlock();
  return 0;
}

static int threadlist_del(struct sthread_struct *t)
{
  struct sthread_struct *prev, *p;

  threadlist_lock();

  prev = NULL;
  for (p = list_hd; p != &manager_thread; prev = p, p = p->next)
    if (p->pid == t->pid)
      break;

  if (p == &manager_thread)
    abort(); /* not found in list; shouldn't happen */

  if (prev)
    prev->next = p->next;
  else
    list_hd = p->next; /* it was the first element */

  threadlist_unlock();

  return 0;
}

/*
 * To override the libpthread errno
 */
int *__errno_location(void)
{
  struct sthread_struct *self = sthread_self();
  return &self->err;
}

static void _sthread_self_suspend(struct sthread_struct *self)
{
  char buf[9];
  if (self == NULL) {
    /* shouldn't ever happen */
    fprintf(stderr, "ERROR: sthread_suspend called by non-running thread!\n");
    abort();
  }
  read(self->pipe[0], buf, 9);
}

static int __attribute__ ((noreturn)) start_thread(void *arg)
{
  struct start_struct *ss = (struct start_struct *)arg;
  struct sthread_struct *self = ss->thread;
  void *main_arg = ss->arg;
  manager_msg_t msg;
  int retval;
  free(ss);

  self->running = 1;
  retval = self->main(main_arg);
  self->running = 0;
  self->status = retval;

  msg.type = MSG_THREAD_EXIT;
  msg.thread_exit.caller = self;
  write(manager_thread.pipe[1], &msg, sizeof(manager_msg_t));
  _sthread_self_suspend(self);

  _exit(0);
}


/*
 * helper routines for the thread manager
 */

static void thread_dealloc(struct sthread_struct *t)
{
  threadlist_del(t);
  munmap(t->stack, t->stacksize);
  close(t->pipe[0]);
  close(t->pipe[1]);
  free(t);
}

static void reap_children(void)
{
  pid_t pid;
  int status;
  struct sthread_struct *p;

  while ((pid = waitpid(-1, &status, WNOHANG | __WCLONE)) > 0) {
    p = threadlist_findbypid(pid);
    if (p == NULL)
      abort();
    thread_dealloc(p);
    if (WIFSIGNALED(status)) {
      /* If a thread died due to a signal, send the same signal to
   all other threads, including the main thread. */
      for (p = list_hd; p && (p != &manager_thread); p = p->next)
  kill(p->pid, WTERMSIG(status));
      kill(main_thread.pid, WTERMSIG(status));
      kill(manager_thread.pid, WTERMSIG(status));
      _exit(0);
    }
  }
}

static void do_create(struct sthread_struct *caller,
    struct sthread_struct *t, sthread_main_t main, void *arg)
{
  struct sthread_struct *self = &manager_thread;
  struct start_struct *ss;
  caller->status = 0;
  caller->err = 0;
  t->main = main;
  t->running = 0;
  t->status = 0;
  t->stacksize = DEF_STACK_SIZE;
  t->stack = mmap(NULL, t->stacksize, PROT_READ | PROT_WRITE | PROT_EXEC,
      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (t->stack == MAP_FAILED) {
    caller->err = self->err;
    goto create_out;
  }
  if (pipe(t->pipe) == -1) {
    caller->err = self->err;
    goto create_freestack;
  }
  /* We can't keep ss on the stack, since we may return before our
   * clone is done using it.  We let the clone free ss when it's
   * done with it. */
  ss = (struct start_struct *)malloc(sizeof(struct start_struct));
  if (ss == NULL) {
    caller->err = self->err;
    goto create_closepipe;
  }
  if (threadlist_add(t) == -1) {
    caller->err = self->err;
    goto create_freess;
  }
  ss->thread = t;
  ss->arg = arg;
  threadlist_lock();
  t->pid = clone(start_thread, t->stack + t->stacksize,
     CLONE_VM | CLONE_FS | CLONE_FILES | CLONE_SIGHAND, ss);
  threadlist_unlock();
  if (t->pid == -1) {
    caller->err = self->err;
    goto create_listdel;
  }
  return;

  /* unwind state on error */
 create_listdel:
  threadlist_del(t);
 create_freess:
  free(ss);
 create_closepipe:
  close(t->pipe[0]);
  close(t->pipe[1]);
 create_freestack:
  free(t->stack);
 create_out:
  caller->status = -1;
}

/*
 * main routine for the manager thread
 */
static int __attribute__ ((noreturn)) manager_main(void *arg)
{
  manager_msg_t msg;
  struct sthread_struct *self = &manager_thread;
  struct sthread_struct *p, *t;

  while (manager_thread.running) {

    /* Check for termination of the main thread */
    if (getppid() == 1) {
      for (p = list_hd; p && (p != &manager_thread); p = p->next)
  kill(p->pid, SIGKILL);
      _exit(0);
    }

    reap_children();

    read(manager_thread.pipe[0], &msg, sizeof(manager_msg_t));

    switch (msg.type) {

    case MSG_CREATE:
      do_create(msg.create.caller, msg.create.thread,
    msg.create.main, msg.create.arg);
      sthread_wake(msg.create.caller);
      break;


    case MSG_THREAD_EXIT:
      t = msg.thread_exit.caller;
      sthread_wake(t);
      if (waitpid(t->pid, NULL, __WCLONE) < 1)
  fprintf(stderr, "sthread manager: waitpid: %s\n", strerror(self->err));
      thread_dealloc(t);
      break;


    case MSG_ALL_EXIT:
      t = msg.create.caller;
      sthread_wake(msg.create.caller);
      /* kill everyone but ourselves and the sender of the message */
      for (p = list_hd; p; p = p->next)
  if (p != self && p != t)
    kill(p->pid, SIGUSR2);
      for (p = list_hd; p; p = p->next)
  if (p != self && p != t) {
    waitpid(p->pid, NULL, __WCLONE);
    thread_dealloc(p);
  }
      manager_thread.running = 0;
      break;
    }
  }

  close(manager_thread.pipe[1]);
  close(manager_thread.pipe[0]);

  _exit(0);
}


/*
 * Initialization & cleanup routines
 */

static int init_main_thread(void)
{
  main_thread.pid = getpid();
  if (pipe(main_thread.pipe) == -1)
    return -1;
  return 0;
}

static int init_manager_thread(void)
{
  int errno_save;
  manager_thread.main = NULL;
  manager_thread.running = 1;
  manager_thread.status = 0;
  manager_thread.stacksize = UTIL_STACK_SIZE;
  manager_thread.stack = mmap(NULL, UTIL_STACK_SIZE,
            PROT_READ | PROT_WRITE | PROT_EXEC,
            MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (manager_thread.stack == MAP_FAILED)
    return -1;
  /* the manager uses its pipe to get messages, not to suspend itself */
  if (pipe(manager_thread.pipe) == -1) {
    free(manager_thread.stack);
    return -1;
  }
  manager_thread.pid = clone(manager_main,
           manager_thread.stack + manager_thread.stacksize,
           CLONE_VM | CLONE_FS | CLONE_FILES | CLONE_SIGHAND,
           NULL);
  if (manager_thread.pid == -1) {
    errno_save = errno;
    close(manager_thread.pipe[0]);
    close(manager_thread.pipe[1]);
    free(manager_thread.stack);
    errno = errno_save;
    return -1;
  }
  return 0;
}

static void handle_cancel(int sig)
{
  _exit(0);
}

static void cleanup(void)
{
  manager_msg_t msg;
  struct sthread_struct *self = sthread_self();

  msg.type = MSG_ALL_EXIT;
  msg.all_exit.caller = self;
  write(manager_thread.pipe[1], &msg, sizeof(manager_msg_t));
  _sthread_self_suspend(self);
}


/********************************
 *
 * EXTERNALLY EXPORTED FUNCTIONS
 *
 ********************************/

int sthread_init(void)
{
  struct sigaction sa;

  if (init_main_thread() == -1)
    return -1;
  if (init_manager_thread() == -1)
    return -1;

  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sa.sa_handler = handle_cancel;
  sigaction(SIGUSR2, &sa, NULL);

  atexit(cleanup);
  return 0;
}

int sthread_create(sthread_t *t, sthread_main_t main_routine, void *arg)
{
  manager_msg_t msg;
  struct sthread_struct *self = sthread_self();
  struct sthread_struct *newthr;
  newthr = (struct sthread_struct *)malloc(sizeof(struct sthread_struct));
  if (newthr == NULL)
    return -1;
  msg.type = MSG_CREATE;
  msg.create.caller = self;
  msg.create.main = main_routine;
  msg.create.thread = newthr;
  msg.create.arg = arg;
  write(manager_thread.pipe[1], &msg, sizeof(manager_msg_t));
  _sthread_self_suspend(self);
  if (self->status)
    free(newthr);
  else
    *t = (sthread_t)newthr;
  return self->status;
}

sthread_t sthread_self(void)
{
  return (sthread_t)threadlist_findbypid(getpid());
}

void sthread_suspend(void)
{
  _sthread_self_suspend(sthread_self());
}

void sthread_wake(sthread_t t)
{
  write(t->pipe[1], "wake up!", 9);
}

int test_and_set(volatile unsigned long *x)
{
  return test_and_set_bit(x);
}
