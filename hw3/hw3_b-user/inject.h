#ifndef __INJECT_H_
#define __INJECT_H_

#include <sys/syscall.h>

#define __NR_inject_failure 378

inline long inject_failure(int delay) {
    return syscall(__NR_inject_failure, delay);
}

#endif  // __INJECT_H_
