#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define MY_MAGIC 'M'
#define CMD_WRITE_VALUE _IOW(MY_MAGIC, 1, int)
#define CMD_READ_VALUE  _IOR(MY_MAGIC, 2, int)

int main()
{
    int fd, value;

    fd = open("/dev/mychardev", O_RDWR);
    if (fd < 0) {
        perror("Failed to open /dev/mychardev");
        return -1;
    }

    value = 42;
    printf("Writing value %d to driver...\n", value);
    ioctl(fd, CMD_WRITE_VALUE, &value);

    value = 0;
    ioctl(fd, CMD_READ_VALUE, &value);
    printf("Value read from driver: %d\n", value);

    close(fd);
    return 0;
}

