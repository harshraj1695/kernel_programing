obj-m += my_rtl8139.o

# This MUST match the folder name we found earlier
KDIR := /home/harshraj1695/buildroot/output/build/linux-6.18.7
PWD := $(shell pwd)

# This tells the compiler to use the Buildroot tools, not your host tools
CROSS_COMPILE := /home/harshraj1695/buildroot/output/host/bin/x86_64-buildroot-linux-gnu-

all:
	$(MAKE) -C $(KDIR) M=$(PWD) ARCH=x86_64 CROSS_COMPILE=$(CROSS_COMPILE) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
