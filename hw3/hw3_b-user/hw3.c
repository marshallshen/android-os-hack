#include <stdio.h>
#include <sys/utsname.h>
#include "inject.h"

#define TOTAL_CALLS 20

int main() {
    int ret = inject_failure(10);
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
    return 0;
}
