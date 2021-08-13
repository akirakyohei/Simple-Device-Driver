CFILES = simple_device_driver.c

obj-m := simple_device_driver.o
simple_device_driver-obj :=$(CFILES:.c=.o)
ccflags-y += -std=gnu99 -Wall -Wno-declaration-after-statement
all:
	make -C /lib/modules/$(shell uname -r)/build M =$(shell pwd) modules
clean:
	make -C /lib/modules/$(shell uname -r)/build M =$(shell pwd) clean