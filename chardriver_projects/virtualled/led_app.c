#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#define VLED_MAGIC 'L'
#define VLED_TOGGLE _IO(VLED_MAGIC, 1)

int main() {
    int fd = open("/dev/vled", O_RDWR);
    if (fd < 0) { perror("open"); return 1; }

    int state = 1;
    write(fd, &state, sizeof(int)); // Turn ON
    read(fd, &state, sizeof(int));
    printf("LED state after write: %d\n", state);

    ioctl(fd, VLED_TOGGLE); // Toggle
    read(fd, &state, sizeof(int));
    printf("LED state after toggle: %d\n", state);

    close(fd);
    return 0;
}

