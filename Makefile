obj-m += hello_world.o

KERNELDIR := <change_me_to_linux_3.9.11_in_output_build_of_buildroot>
 
all:
	make ARCH=avr32 CROSS_COMPILE=avr32-linux- -C $(KERNELDIR) M=$(shell pwd) modules
 
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
