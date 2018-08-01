//#include <stdio.h>
#include <argp.h>
#include <sys/ioctl.h> // ioctl
#include <fcntl.h> // open
#include <unistd.h> // close

int main(int argc, char *argv[])
{
    if (argc != 2) {
        printf("Usage:\n./program ENTER|TRIGGER");
        return 0;
    }

    int fd = open("/dev/uipc-mwait", O_RDWR);
    if (fd < 0) {
        perror("open /dev/uipc-mwait failed.\n");
        return 1;
    }
    close(fd);
    return 0;
}
