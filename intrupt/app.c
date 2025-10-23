#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define MAGIC 'M'
#define CMD_PROCESS _IOWR(MAGIC, 1, int)

int main()
{
    int fd, value;

    fd = open("/dev/simplewq", O_RDWR);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    printf("Enter a number: ");
    scanf("%d", &value);

    ioctl(fd, CMD_PROCESS, &value);

    printf("Result from kernel: %d\n", value);

    close(fd);
    return 0;
}

