obj-m += simple2_pseduo.o

# Add debug flag
EXTRA_CFLAGS += -g

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

