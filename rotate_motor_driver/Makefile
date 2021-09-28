MODULE_NAME = rotate_driver
APP_NAME = test_rotate
KDIR = /root/raspberry/linux
#TOOLCHAIN = /root/raspberry/tools/arm-bcm2708/gcc-linaro-arm-linux-gnueabihf-raspbian/bin/arm-linux-gnueabihf-
TOOLCHAIN = arm-linux-gnueabihf-
CC = $(TOOLCHAIN)gcc
TARGET = arm
PWD := $(shell pwd)
obj-m := $(MODULE_NAME).o
all: app driver
driver:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) ARCH=$(TARGET) CROSS_COMPILE=$(TOOLCHAIN) modules
app:
	$(CC) -o $(APP_NAME) $(APP_NAME).c
install:
	cp -f $(MODULE_NAME).ko /media/21411844/root6
	cp -f $(APP_NAME) /media/21411844/root6
clean:
	rm -f *.ko
	rm -f *.o
	rm -f *.mod.*
	rm -f $(APP_NAME)
	rm -f modules.order
	rm -f Module.symvers
	rm -rf .tmp_versions
