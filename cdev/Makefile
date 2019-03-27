export ARCH=arm
ifeq ($(KERNELRELEASE),)

ifeq ($(ARCH),arm)
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
	@#rm -rf *.o *.ko *.mod* modules.* Moduele.*

else

obj-m := vserial.o

endif
