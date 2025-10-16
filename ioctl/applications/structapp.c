#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define MY_MAGIC 'M'
#define CMD_WRITE_VALUE _IOW(MY_MAGIC, 1, int)
#define CMD_READ_VALUE  _IOR(MY_MAGIC, 2, int)


struct mydev1{
        int a;
        int b;
};
int main()
{
    int fd, aa,bb;
    int value;
    fd = open("/dev/mychardev", O_RDWR);
    if (fd < 0) {
        perror("Failed to open /dev/mychardev");
        return -1;
    }

    while (1) {
        printf("Enter the value of aa and bb (3,3 to exit): ");
        scanf("%d%d", &aa,&bb);
        if (aa==3 && bb==3)
            break;   
        struct mydev1 news;
        news.a=aa;
        news.b=bb;
        printf("Writing value %d and %d to driver...\n", news.a, news.b);
        ioctl(fd, CMD_WRITE_VALUE, &news);

        ioctl(fd, CMD_READ_VALUE, &value);
        printf("Value read from driver: %d\n", value);
    }

    close(fd);
    printf("Program exited cleanly.\n");
    return 0;
}

