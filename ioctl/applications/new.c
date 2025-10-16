#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdlib.h>

#define DEVICE_NAME "/dev/mychardev"
#define MY_MAGIC 'M'
#define CMD_WRITE_VALUE _IOW(MY_MAGIC,1,int)
#define CMD_READ_VALUE _IOR(MY_MAGIC,2,int)

int main(){
        int fd, value;
        fd=open(DEVICE_NAME,O_RDWR);
        if(fd<0){
        perror("open");
        exit(1);
        }
        while(1){
                printf("Enter the value (0 to exit)");
                scanf("%d", &value);
                if(value==0){
                        printf("closing file ");
                        break;
                }
                printf("Writing to kernel ...\n");
                ioctl(fd,CMD_WRITE_VALUE,&value);
                
                
                ioctl(fd,CMD_READ_VALUE,&value);
                printf(" Reading from user..%d\n",value);
                
        }
        close(fd);
        return 0;
}
