## Programming Assignment: System Call Fault Injector (60 pts)

System calls may fail, but they fail very infrequently (e.g., only when the system is under an extreme load), so applications are often not prepared to handle the failures. In this programming assignment, we will build a system call fault injector in the Linux kernel in Android. Our injector will artificially introduce faults into system calls to test application reliability.

You'll implement a function that applications can call to inject system call faults. The interface is

    int fail(int N)
When a program calls fail(N): if N > 0, then the Nth system call following this call will return an error code instead of proceeding as usual. If N is 0 and there is a fault injection session going on already for the current process, the kernel should stop this session. In all other cases (e.g., N is negative), fail should return appropriate error codes.

Some examples:

fail(2); fail(2); syscallA(); syscallB();
syscallA() should not fail; syscallB() should fail. Second fail() call renewed the session.
fail(0);
This call should fail. No session is running.
fail(1); fail(1);
Second fail() call should fail. fail() is also a system call.
fail(1); fail(0);
Second fail() call should fail. fail() is also a system call.
0. Setting up the environment

We'll need to test the injector with the programs we write, so we need to download and install the Android Software Development Kit (SDK) and Native-code Development Kit (NDK). Follow these instructions to install them.

You should also have an unlocked device to test your results. Follow these instructions to unlock your device.
Before implementing the fault injector, build an unmodified kernel first, and try to flash it to your device and boot it. Follow these instructions to build and test your kernel. If you can boot your device with the unmodified kernel you build but experience crashes once you modify the kernel, you know it is really the modifications that cause the crashes.

### 1. Bookkeeping for fault injection

If a process wants the kernel to inject a fault, you need to keep this piece of information. You also need to record how many system calls are left before injecting the fault.

__Hint__: you may store bookkeeping information within struct task_struct. In Linux, this struct is essentially the process's control block. Each process owns an instance of this struct, so it seems to ideal for us to store the relevant information.

Exercise 1. Add necessary program constructs to record the bookkeeping information for the current process.
### 2. Adding a system call

Now, you need to add a system call to implement the fault injector. Look at arch/arm/kernel/calls.S and arch/arm/kernel/sys_arm.c to see how other system calls are defined.

Name your system call sys_fail, and place the new system call after all the other system calls, so that it has the system call number 378. It is important that your system call has this exact number, because the user-space code we give you (see below) expects that your system call has this number.
Add a new file to the kernel source tree, place it at a reasonable place, and implement the new system call in the file. You should record the bookkeeping information according to the argument passed to the system call. You must also check that the argument is valid and return appropriate error codes.

> Exercise 2. Add a system call to inject faults.


### 3. Implementing fault injection.

Now that you have the fault injection information passed via sys_fail(N), you need to actually inject a fault to the Nth system call from now on. You'll do so by modifying the common entry point of system calls to (1) check whether a fault should be injected and (2) if so, return an error code without doing the actual system call. Specifically, you'll implement two functions, long should_fail() and long fail_syscall(). should_fail checks whether a fault should be injected for the current system call. If so, it returns 1; otherwise, it returns 0. It also updates the bookkeeping information accordingly. fail_syscall() injects a fault by returning an error code.

We have provided you stub code in assembly at the entry point of all system calls to call into the two functions you implement, but you need to modify arch/arm/kernel/entry-common.S to add the assembly stub code. First, find this part of the assembly code in arch/arm/kernel/entry-common.S:

     tst    r10, #_TIF_SYSCALL_WORK        @ are we tracing syscalls?
     bne    __sys_trace

     cmp    scno, #NR_syscalls        @ check upper syscall limit
     adr    lr, BSYM(ret_fast_syscall)    @ return address
     ldrcc    pc, [tbl, scno, lsl #2]        @ call sys_* routine
Here, the kernel first checks if it is tracing system calls. If not, it compares the system call number (scno) with the total number of system calls (NR_syscalls). Then the kernel sets the return address to ret_fast_syscall which handles some cleanup work after a system call is executed. Finally, the kernel does a conditional branch. If the system call number is smaller than the total number of system calls(cmp and "cc" in ldrcc), the kernel loads the address of the system call routine into pc(ldrcc pc), which effectively causes the processor to jump to the corresponding system call routine.

Before the cmp instruction, add the following assembly code before the cmp statement:

       bl      should_fail

       /* back up the return value into r10 */
       mov     r10, r0

       /* restore caller-saved registers */
       add     r0, sp, #S_R0 + S_OFF
       ldmia   r0, {r0 - r3}

       /* check the return value stored in r10 */
       cmp     r10, #0
       beq     no_failure

	   /* fail the system call */
       adr     lr,     BSYM(ret_fast_syscall)  @ return address
       ldr     pc, =fail_syscall

no_failure:

> Exercise 3. Modify the assembly code in arch/arm/kernel/entry-common.S to add the stub assembly code.
The stub code calls should_fail to see if current system call should be failed. The result is stored in register r0 per the GCC calling convention on ARM. The stub code backs up the return value to r10. Since should_fail may modify registers, the stub code restores them registers from the stack. Then, it compares the result of should_fail with 0. If it is 0, current system call should not be failed. Otherwise, it jumps to fail_syscall to fail current the system call.

> Exercise 4. Which registers are modified by the should_fail call? Why do we only restore registers r0 - r3? How about r4 - r11? Check the ARM Software Development Toolkit Reference Guide to understand the caller and callee saved registers in ARM.


### 4. Determining when to fail the system call

Now you need to implement should_fail to tell whether the current system call should be failed. The prototype is:

    long should_fail(void)
Implement it in the same file you added previously. It should return 0 to indicate that current system call should not be failed, and 1 otherwise. It should also update bookkeeping information.

> Exercise 5. Implement should_fail to inform the kernel when to fail the current system call.


### 5. Failing a system call

Now you need to implement fail_syscall. Implement fail_syscall in the same file. Its prototype is:

    long fail_syscall(void)
It is similar to other system call routines, and its return value should indicate that the system call failed. Pick an appropriate error code to return.

> Exercise 6. Implement fail_syscall to fail system calls.


### 6. Testing failure injection

Boot the device with your modified kernel. Now you have a new system call, and you need a user space program to call it.

Download http://www.cs.columbia.edu/~junfeng/13fa-w4118/hw/hw2-user.tar.gz, extract, run
    make test
to test your system call. The result should be:

```
syscall 0: ret = 0
syscall 1: ret = 0
syscall 2: ret = 0
syscall 3: ret = 0
syscall 4: ret = 0
syscall 5: ret = 0
syscall 6: ret = 0
syscall 7: ret = 0
syscall 8: ret = 0
syscall 9: ret = -1
syscall 10: ret = 0
syscall 11: ret = 0
syscall 12: ret = 0
syscall 13: ret = 0
syscall 14: ret = 0
syscall 15: ret = 0
syscall 16: ret = 0
syscall 17: ret = 0
syscall 18: ret = 0
syscall 19: ret = 0
```
> Exercise 7. Call the system call and check results.

> Exercise 8. Read hw2.c. What will happen if we print the return of each system call using printf right after each system call? Why?

> Exercise 9. Read inject.h. If you did not add the system call as the last system call, what may happen?


You also need to write a few more testcases and test the code by yourself because the TAs may grade your solution with other grading scripts.

7. Submit

Commit your source code changes using the git add and git commit commands you learned from the previous assignment. Remember to add your new file to the code repository. Then use the git diff command to get the code change. You should compare with the original version, so please do

    git diff 365a6e06
Here 365a6e06 refers to the initial version of the kernel source before you make any modifications. Then submit the code difference in a .patch file. The file name should be hw2-<your uni>-code.patch. Note that you only need to submit this patch; you don't need to submit the answers to the other exercises.
