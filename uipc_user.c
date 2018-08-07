#define _GNU_SOURCE             /* See feature_test_macros(7) */
#include <stdio.h> // printf
#include <stdlib.h> // atoi
#include <stdbool.h> // bool
//#include <argp.h>
#include <sys/ioctl.h> // ioctl
#include <fcntl.h> // open
#include <unistd.h> // close
#include <sched.h> // for sched_setaffinity

#include "uipc.h"

#define CMD 200
#define DATA 300

static bool detect_monitor_mwait(void)
{
    unsigned int eax, ebx, ecx, edx;
    uipc_cpuid(1, &eax, &ebx, &ecx, &edx);
    printf("[UIPC] processor family = %d, model = %d\n", (eax & 0xF00) >> 8, (ebx & 0xF0) >> 4); // according to CPUID manual page 21
    return ecx & MONITOR_MWAIT_FLAG;
}

/* get the smallest and largest monitor range size*/
static void get_monitor_range_size(void)
{
    unsigned int eax, ebx, ecx, edx;
    uipc_cpuid(0x5, &eax, &ebx, &ecx, &edx);
    printf("[UIPC] smallest monitor size = %d, largest monitor size = %d\n", eax & 0xFFFF, ebx & 0xffff);
}

static inline int set_sched_affinity(const char *optarg)
{
    pid_t pid = getpid();
    cpu_set_t my_set;
    int ret, affinity;

    affinity = atoi(optarg);

    CPU_ZERO(&my_set);
    CPU_SET(affinity, &my_set);

    ret = sched_setaffinity(pid, sizeof(my_set), &my_set);
    if (ret < 0) {
        perror("[UIPC] set affinity failed...\n");
        return -1;
    }

    return 0;
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        printf("Usage:\n./program ENTER|TRIGGER\n");
        return 0;
    }

    char ch;
    int cmd = -1;

    if (!detect_monitor_mwait()) {
        printf("monitor/mwait is not enabled on your machine\n");
        return 0;
    }

    get_monitor_range_size();

    //ret = sched_getaffinity(0, sizeof(my_set), &my_set);

    unsigned long arg;

    while ((ch = getopt(argc, argv, "e:b:d:h")) != -1) {
        switch (ch) {
        case 'e':
            cmd = UIPC_ENTER_MONITOR_MWAIT;
            arg = atoi(optarg);
            break;
        case 'b': // bind thread to specific core
            if (set_sched_affinity(optarg) < 0)
                cmd = -1;
            break;
        case 'd':
            cmd = UIPC_TRIGGER_MONITOR;
            arg = atoi(optarg); // the cpu to wakeup
            break;
        default:
            cmd = -1;
            break;
        }
    }
    if (cmd < 0) {
        printf("Usage: ./uipc -e|-t\n\t-e enter mwait state\n\t-t trigger monitored memory");
        return 0;
    }

    printf("choosing cpu %ld\n", arg);
    int fd = open("/dev/uipc-mwait", O_RDWR);
    if (fd < 0) {
        perror("open /dev/uipc-mwait failed.\n");
        return 1;
    }

    int res = ioctl(fd, cmd, &arg);
    if (res < 0) {
        perror("ioctl failed.\n");
        close(fd);
        return 1;
    }

    close(fd);
    return 0;
}
