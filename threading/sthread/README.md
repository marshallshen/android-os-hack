## A simple thread library

### Test Setup

Make sure to run the following to understand simple threading:

```
gcc -o test test.o sthread.o
./test
```

### Exercise 1: Understand SThread

We ran multiple experiements and saw some interesting patterns:

```
mshen@ubuntu:~/Desktop/android-os-hack/threading/sthread$ ./test
thread 2: I'm going to sleep
thread 1: I'm going to sleep
thread 1: I woke up!
thread 1: I'm going to sleep
thread 2: I woke up!
thread 2: I'm going to sleep
thread 2: I woke up!
thread 2: I'm going to sleep
thread 1: I woke up!
thread 1: I'm going to sleep

mshen@ubuntu:~/Desktop/android-os-hack/threading/sthread$ ./test
thread 1: I'm going to sleep
thread 1: I'm going to sleep
thread 2: I'm going to sleep
thread 1: I woke up!
thread 1: I'm going to sleep
thread 2: I woke up!
thread 2: I'm going to sleep
thread 2: I woke up!
thread 2: I'm going to sleep
thread 1: I woke up!
thread 1: I'm going to sleep

mshen@ubuntu:~/Desktop/android-os-hack/threading/sthread$ ./test
thread 2: I'm going to sleep
thread 1: I'm going to sleep
thread 1: I woke up!
thread 1: I'm going to sleep
thread 2: I woke up!
thread 2: I'm going to sleep
thread 2: I woke up!
thread 2: I'm going to sleep
thread 1: I woke up!
thread 1: I'm going to sleep
```
### Observation

`sleep(1)` randomly puts one of the active threads in sleep. If
`sthread_wake` is triggered on a sleeping thread, that sleeping thread
will become active and print out message "I woke up"! If `sthread_wake`
is triggered on a non-sleepling thread, it's an no-op.

