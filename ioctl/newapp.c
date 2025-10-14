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

    while (1) {
        printf("Enter the value (3 to exit): ");
        scanf("%d", &value);

        if (value == 3)
            break;   

        printf("Writing value %d to driver...\n", value);
        ioctl(fd, CMD_WRITE_VALUE, &value);

        ioctl(fd, CMD_READ_VALUE, &value);
        printf("Value read from driver: %d\n", value);
    }

    close(fd);
    printf("Program exited cleanly.\n");
    return 0;
}

