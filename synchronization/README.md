
Part B: Synchronization in the Linux kernel (35 pts)

unistd.h -> Add netlock System Call Number
calls.S to add the System Call Table (didn't use the sys_ prefix to fit the net_lock.c conventions)
sched.h -> added netlock_t to the sched parameters 
syscalls.h -> added netlock to the system calls headers.
Makefile -> build netlock macho
exit.c -> release on exit
fork.c -> modify fork to create a lock
net_lock.c -> i dont know how I lost this on the .patch file.

wakelock.h -> DIDNT MEAN TO CHANGE THIS FILE



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
