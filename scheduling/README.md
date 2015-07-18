### Part A: Implementing MyCFS (30 pts)

In the first part of this assignment, you will implement the fair
scheduling algorithm as we discussed in class. The runqueue should be
one red-black tree indexed by the virtual runtime. The schedule latency
should be 10ms. For simplicity, assume that all processes using MyCFS
are equally-weighted, so you don't have to worry about their nice
levels.

You will add a new scheduler class called mycfs_sched_class. Its
priority should be lower than fair_sched_class and higher than
idle_sched_class. Put your scheduler implementation in
kernel/sched/mycfs.c.

MyCFS should work alongside existing Linux schedulers. Therefore, you
should add a new scheduling policy, SCHED_MYCFS. The value of
SCHED_MYCFS should be 6. By default, processes are not using MyCFS, but
you can use the sched_setscheduler() system call to change the
scheduling policy of a process.

MyCFS should utilize all cores on the Nexus 7. You should have a MyCFS
runqueue for each core. However, to make things easier, you don't have
to worry about load balancing across cores. Processes on one core are
never migrated to another core. If a process forks, the child should
stay on the same core as the parent.

You may start with reading Documentation/scheduler/sched-design-CFS.txt.
Even though you can look at the current Linux implementation of CFS,
please refrain from copying the CFS code since it's way too complex for
what you're requested to implement. Write your own code and be prepared
to clearly explain how your code works during the demo grading.

Hints: You may find printk() and /proc/kmsg extremely useful. If you
ever encounter a kernel panic (hopefully not), you can view previous
messages at /proc/last_kmsg.

### Part B: Adding CPU usage limit to MyCFS (30 pts)

You might have the experience that sometimes an application eats up all
the CPU and makes your Nexus 7 as hot as a frying pan. In the second
part of this assignment, you will make MyCFS be able to limit the
maximum CPU usage of individual processes.

Let us define CPU usage by the amount of CPU time a process consumes
within each period of 100ms. You will add a new system call

    int sched_setlimit(pid_t pid, int limit);
    
to specify the CPU usage limit. For example, limit=40 means the process
can only consume 40ms of CPU time within each 100ms period. Calling it
with limit=0 means clearing the CPU usage limit for the process. The
number of this system call should be 378.

To implement this feature, your MyCFS should schedule processes normally
until a process hits its CPU usage limit. After that, MyCFS should skip
that process (but continue scheduling other processes that haven't
reached their limits yet) for the rest of the 100ms period. The same
logic applies for multiple limited processes as well.

You don't have to interrupt a process within a scheduler tick. In the
demo, it's fine if your CPU usage limit has an error within 10ms.

If a process forks, the child should inherit the CPU usage limit of the
parent.

If a process switches from another scheduler to MyCFS, its CPU usage
limit should be cleared.
