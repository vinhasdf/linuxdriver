#if KERNELRELEASE is defined, we're been invoked from kernel build system and can use it language
ifneq ($(KERNELRELEASE),)
	echo "hai"
	obj-m := hello.o
else
	KERNELDIR ?= /lib/modules/$(shell uname -r)/build
	PWD := $(shell pwd)
	echo "mot"
default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
endif