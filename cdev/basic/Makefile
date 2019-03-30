export ARCH=arm
ifeq ($(KERNELRELEASE),)

ifeq ($(ARCH),arm)
	# change these variable's value
	KERNEL_DIR ?= /home/topeet/itop4412_kernel_4_14_2_bsp/linux-4.14.2_iTop-4412_scp 
	ROOTFS	?= `pwd` 
else
	KERNEL_DIR ?= /lib/modules/$(shell uname -r)/build
endif

PWD := $(shell pwd)

modules:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) modules
module_install:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) INSTALL_MOD_PATH=$(ROOTFS) module_install

.PHONY:
	clean

clean:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) clean
	@# rm -rf *.o *.ko *.mod* modules.* Moduele.*

else

# change it's value
obj-m := vserial.o

# if there are multi files related to this $(obj-m):
# 	1. uncomment this line;
# 	2. change *.o to real files related to this ko

# hello-objs = *.o

endif
