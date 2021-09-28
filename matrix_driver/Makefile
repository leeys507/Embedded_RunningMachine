MODULE_NAME = matrix_driver
APP_NAME = test_matrix
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
	cp -f $(MODULE_NAME).ko /media/pi/root1
	cp -f $(APP_NAME) /media/pi/root1
clean:
	rm -f *.ko
	rm -f *.o
	rm -f *.mod.*
	rm -f $(APP_NAME)
	rm -f modules.order
	rm -f Module.symvers
	rm -rf .tmp_versions
