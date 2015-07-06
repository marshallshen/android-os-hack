/**
 * Test for semaphore counter of threads
 */

#define _REENTRANT
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include "sthread.h"


static sthread_sem_t sem;

int threadmain(void *arg)
{
  int i = 0;
  for( i=0; i< 20; i++ )
  {
    sleep(1);
    sthread_sem_down( &sem );
    sleep(1);
    sthread_sem_up( &sem );
  }
  return 0;
}

int main(int argc, char *argv[])
{
  srand( time(0) );
  sthread_t thr1, thr2;

  if (sthread_init() == -1)
    fprintf(stderr, "%s: sthread_init: %s\n", argv[0], strerror(errno));

  if( sthread_sem_init( &sem, 1 ) == -1 )
  {
    fprintf( stderr, "sem init error\n" );
  }

  if (sthread_create(&thr1, threadmain, (void *)1) == -1)
  {
    fprintf(stderr, "%s: sthread_create: %s\n", argv[0], strerror(errno));
  }

  if (sthread_create(&thr2, threadmain, (void *)2) == -1)
  {
    fprintf(stderr, "%s: sthread_create: %s\n", argv[0], strerror(errno));
  }

  if (sthread_create(&thr2, threadmain, (void *)3) == -1)
  {
    fprintf(stderr, "%s: sthread_create: %s\n", argv[0], strerror(errno));
  }

  sleep( 10 );

  if( sthread_sem_destroy( &sem ) == -1 )
  {
    fprintf( stderr, "sem destroy error\n" );
  }

  return 0;
}
