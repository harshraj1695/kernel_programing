#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

struct student {
    int a;
    int b;
};

int main() {
    int fd = open("/dev/mychardev", O_RDWR);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    struct student s1 = {10, 20};
    write(fd, &s1, sizeof(s1));

    struct student s2;
    read(fd, &s2, sizeof(s2));

    printf("Read from kernel: a=%d, b=%d\n", s2.a, s2.b);

    close(fd);
    return 0;
}

