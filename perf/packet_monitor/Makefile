obj-m += packet_monitor.o

# Add debug symbols (-g) and disable some optimizations for better tracing
ccflags-y := -g -O1

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean