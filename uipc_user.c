//#include <stdio.h>
#include <argp.h>
#include <sys/ioctl.h> // ioctl
#include <fcntl.h> // open
#include <unistd.h> // close

#include "uipc.h"

static char doc[] =
  "UIPC - user-level helper function";

int main(int argc, char *argv[])
{
    if (argc != 2) {
        printf("Usage:\n./program ENTER|TRIGGER\n");
        return 0;
    }

    char ch;
    int cmd = -1;

    while ((ch = getopt(argc, argv, "eth")) != -1) {
        switch (ch) {
        case 'e':
            cmd = UIPC_ENTER_MONITOR_MWAIT;
            break;
        case 't':
            cmd = UIPC_TRIGGER_MONITOR;
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

    int fd = open("/dev/uipc-mwait", O_RDWR);
    if (fd < 0) {
        perror("open /dev/uipc-mwait failed.\n");
        return 1;
    }

    int res = ioctl(fd, cmd, NULL);
    if (res < 0) {
        perror("ioctl failed.\n");
        return 1;
    }

    close(fd);
    return 0;
}
