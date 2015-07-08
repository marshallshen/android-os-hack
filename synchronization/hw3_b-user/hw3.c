`#include <stdio.h>
#include <sys/utsname.h>
#include <inject.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <math.h>
#include <linux/netlock.h>

#define NUM_THREADS 2
/*#define TOTAL_CALLS 20 */

/*
    enum __netlock_t {
    NET_LOCK_N, /* Placeholder for no lock
    NET_LOCK_R, /* Indicates regular lock
    NET_LOCK_E, /* Indicates exclusive lock
     } ;
     typedef enum __netlock_t netlock_t;
*/

    /*
    struct thread_info {       Used as argument to thread_start()
    pthread_t thread_id;        ID returned by pthread_create()
    int       thread_num;       Application-defined thread #
    char     *argv_string;      From command-line argument
    };
    */

    
    /* First Idea (failed to properly get it going)*/
    /* create a pipe to create extra fork() and then call netlock. Similar to linux fork man page example*/

    /*
    int ret;
    pid_t testpid;
    int pipefiledes[2];

    testpid = fork();
    if(testpid==-1) {
            perror("fork");
            exit(EXIT_FAILURE);
    }

    if(testpid==0) {

    }
    */

    /* pthreads */

    /*
    Method to test
    You may assume that all processes that intend to use the network must call netlock_acquire in regular (read) mode. 
     The calls to acquire the lock in regular mode should succeed immediately as long as no process is holding an exclusive
     (write) lock or is waiting for an exclusive lock. Multiple processes may request the lock in regular mode, 
     and all of them should be granted the lock if this condition is met. If a process is currently holding or waiting 
     for an exclusive lock, subsequent calls to netlock_acquire in regular mode must block. All processes waiting for regular 
     locks must be granted the lock once the condition for granting the regular locks is fulfilled. 
     https://computing.llnl.gov/tutorials/pthreads/
     */
void *TestNetLock(void *threadid)
{
    int ret;
    int procid=(int)threadid;
    if((ret= netlock_acquire(NET_LOCK_R))!=0) 
    {
        printf("regular/read lock not given\n");
        printf("process id %d\n", procid); 
    }
    else
    {   
        printf("regular/read lock not given\n");
        printf("process id %d\n", procid);
    }
    if((ret= netlock_release())!=0) 
    {
        printf("lock not released\n");
        printf("process id %d\n", procid); 
    }
    else
    {   
        printf("lock released\n");
        printf("process id %d\n", procid);
    }
    if((ret= netlock_acquire(NET_LOCK_E))!=0) 
    {
        printf("exclusive/write lock not given\n");
        printf("process id %d\n", procid);
    }
    else
    {   
        printf("exclusive/write lock given\n");
        printf("process id %d\n", procid);
    }
    if((ret= netlock_release())!=0) 
    {
        printf("lock not released\n");
        printf("process id %d\n", procid);
    }
    else
    {   
        printf("lock released\n");
        printf("process id %d\n", procid);
    }



}

int main() {
    pthread_t threads[NUM_THREADS];
    long k;
    int tempthread;

    for(k=0; k<NUM_THREADS; k++){
        tempthread = pthread_create(&threads[k], NULL, TestNetLock, (void *)k);
        if(tempthread)
        {
            printf("Error during create thread");
            exit(-1);
        }
    }

   pthread_exit(NULL);

    /*
    if (ret != 0) {
        printf("INJECT_FAILURE ret: %d\n", ret);
        printf("Error! Fail to inject failures!\n");
        return 1;
    }
    int i;
    struct utsname name;
    int rets[TOTAL_CALLS];
    for (i = 0; i < TOTAL_CALLS; i++) {
        rets[i] = uname(&name);
    }
    for (i = 0; i < TOTAL_CALLS; i++) {
        printf("syscall %d: ret = %d\n", i, rets[i]);
    }
    */
    /* return 0; */


}
