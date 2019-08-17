export ARCH=arm
ifeq ($(KERNELRELEASE),)

ifeq ($(ARCH),arm)
	# change these variable's value
	KERNEL_DIR ?= /home/jason/arm-devlop/itop4412_linux_official
	ROOTFS	?= /home/jason/rootfs 
else
	KERNEL_DIR ?= /lib/modules/$(shell uname -r)/build
endif

PWD := $(shell pwd)

modules:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) modules
modules_install:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) INSTALL_MOD_PATH=$(ROOTFS) modules_install

.PHONY:
	clean

clean:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) clean
	rm -rf a.out test
	@# rm -rf *.o *.ko *.mod* modules.* Moduele.*

else

# change it's value
obj-m := led_dev.o led_driver.o

# if there are multi files related to this $(obj-m):
# 	1. uncomment this line;
# 	2. change *.o to real files related to this ko

# hello-objs = *.o

endif

test: test.c
	arm-linux-gnueabihf-gcc $^ -o $@
