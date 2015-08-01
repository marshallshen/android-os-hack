## Hack on [Android Kernel 3.4](http://androidxref.com/kernel_3.4/)

### HW5

Programming Assignment: Per-app Memory Quota (60 pts)

In this assignment, you will implement a kernel feature that gives each app a physical memory quota, so that a buggy or malicious app cannot exhaust the system's physical memory.

Let's first study some background. A system may become out-of-memory (OOM) when a buggy or malicious process uses a lot of physical memory or when the system has too many processes running. To avoid crashing the whole system including all processes, Linux implements an OOM killer to selectively kill some processes and reclaim the physical memory allocated to these processes. The OOM killer selects processes to kill using a variety of heuristics, typically targeting the process that (1) uses a large amount of physical memory and (2) is not important.

Although this OOM killer works reasonably well, it has one security flaw: it ignores which users created the processes. A malicious user can thus game the OOM killer by creating a large number of processes. While the aggregated amount of memory allocated to this user's processes is huge, each process owns only a small amount of memory, and the OOM killer may not notice these processes.

This security flaw has implications on Android as well. In Android, each app is represented as a user. When an app runs, it can create more than one processes. Thus, a malicious app can create a large number of processes, causing other apps to be killed.

In this assignment, you'll fix the security flaw in the OOM killer by implementing per-app memory quota. Specifically, you'll (1) implement a new system call to set per-user memory quota (recall each Android app is represented as a unique user) and (2) modify the OOM killer in the Android kernel to kill a process owned by a user when the user's processes run out of the user's memory quota.

Part A: Implementing System Call set_mlimit() (10 pts)

In the first part of this assignment, you will implement a system call with following signature:

        int set_mlimit(uid_t uid, long mem_max);
    
The system call has two parameters:
uid: the id of the user that we want to set memory quota for
mem_max: maximum amount of physical memory the user can use (in bytes)

You should save the memory quota (i.e., mem_max) information in proper data structures so that the OOM killer can use this information to determine when a user has run out of her quota.

Part B: Modifying the OOM Killer (50 pts)

In the second part, you'll modify the original OOM killer to kill a process owned by a user when the total amount of physical memory allocated to all processes of the user exceeds the user's memory quota. To determine the total amount of physical memory allocated to a user, use the Resident Set Size (RSS) which tracks the amount of physical memory currently allocated to a process. RSS is stored in struct mm_struct. Among all processes belonging to the user, your OOM killer should kill the process that has the highest RSS.

To understand how the kernel allocates physical memory to a process, read through function __alloc_pages_nodemask() in mm/page_alloc.c. This function also shows how the OOM Killer is triggered when the memory runs out.

To keep your modified OOM killer simple, you are allowed to reuse the original OOM Killer's code when appropriate.

Hint: how to test your code on Android

1. The Android system represents each app as a unique user. To test your code, you may use "adb root" to acquire the root access, then type "adb shell" to login, and use "su" command to switch to a specific user. For example, if currently an app with username u0_a70 is running, you can type "su 10070" (10070 is the uid of user u0_a70) in adb shell to switch to that user.

2. After switching to a specific user, you can use your newly added system call set_mlimt to set a memory quota for that user. If your quota is smaller than the RSS of the currently running app (e.g., with username u0_a70), one of the app's processes should be killed upon the next physical memory allocation request.

3. To help you debug, we have created a test program hw5_test.c. It takes two or more parameters. The first parameter specifies the username of the user you want to set memory quota for. The second parameter specifies the memory quota. Each parameter from the third to the last causes the test program to fork a process and allocate the specified amount of memory. For example, by running

		./hw5_test u0_a70 100000000 60000000
	
you set a 100MB memory quota for username u0_a70 (i.e., uid 10070) and fork a new process which requests 60MB memory. If the user already has an existing process which uses 50MB memory, the total memory of all processes of that user is larger than the quota, so your OOM killer should kill one process. It should kill the process forked by the command line because this process has a largest RSS than the existing process. If the existing process uses 80MB memory already, the memory quota for u0_a70 also runs out and the existing process should be killed instead.

