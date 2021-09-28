MODULE_NAME = sound_sensor_driver
KDIR = /root/raspberry/linux
#TOOLCHAIN = /root/raspberry/tools/arm-bcm2708/gcc-linaro-arm-linux-gnueabihf-raspbian/bin/arm-linux-gnueabihf-
TOOLCHAIN = arm-linux-gnueabihf-
CC = $(TOOLCHAIN)gcc
TARGET = arm
PWD := $(shell pwd)
obj-m := $(MODULE_NAME).o
driver:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) ARCH=$(TARGET) CROSS_COMPILE=$(TOOLCHAIN) modules
install:
	cp -f $(MODULE_NAME).ko /media/21411844/root6
clean:
	rm -f *.ko
	rm -f *.o
	rm -f *.mod.*
	rm -f .$(MODULE_NAME).*
	rm -f modules.order
	rm -f Module.symvers
	rm -rf .tmp_versions
