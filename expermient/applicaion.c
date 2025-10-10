#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define DEVICE "/dev/mychardev"

int main() {
    int fd;
    char write_buf[100];
    char read_buf[100];
    ssize_t ret;

    // Open the device file
    fd = open(DEVICE, O_RDWR);
    if (fd < 0) {
        perror("Failed to open device");
        return EXIT_FAILURE;
    }

    printf("Enter data to write to kernel: ");
    fgets(write_buf, sizeof(write_buf), stdin);
    write_buf[strcspn(write_buf, "\n")] = '\0';  // remove newline

    // Write data to kernel buffer
    ret = write(fd, write_buf, strlen(write_buf));
    if (ret < 0) {
        perror("Write failed");
        close(fd);
        return EXIT_FAILURE;
    }
    printf("Data written: %s\n", write_buf);

    // Read data from kernel buffer
    ret = read(fd, read_buf, sizeof(read_buf));
    if (ret < 0) {
        perror("Read failed");
        close(fd);
        return EXIT_FAILURE;
    }

    read_buf[ret] = '\0';
    printf("Data read from kernel: %s\n", read_buf);

    // Close the device
    close(fd);
    return 0;
}

