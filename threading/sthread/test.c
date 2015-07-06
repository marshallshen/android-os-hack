#define _REENTRANT
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include "sthread.h"

int threadmain(void *arg)
{
  int threadno = (int)arg;
  for (;;) {
    printf("thread %d: I'm going to sleep\n", threadno);
    sthread_suspend();
    printf("thread %d: I woke up!\n", threadno);
  }
  return 0;
}

int main(int argc, char *argv[])
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
