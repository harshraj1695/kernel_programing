#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

int main()
{
    int fd;
    char write_msg[100];
    char read_buf[100];
    int counter = 0;

    fd = open("/dev/mynode1", O_RDWR);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    while (1) {

        snprintf(write_msg, sizeof(write_msg),
                 "Message %d to node0 hii harsh you have done it ", counter++);

        write(fd, write_msg, strlen(write_msg));
        printf("WROTE: %s\n", write_msg);

        lseek(fd, 0, SEEK_SET);

        memset(read_buf, 0, sizeof(read_buf));
        int n = read(fd, read_buf, sizeof(read_buf));

        if (n > 0)
            printf("READ : %s\n", read_buf);
        else
            printf("READ : (empty)\n");

        sleep(1);
    }

    close(fd);
    return 0;
}

