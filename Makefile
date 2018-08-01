# 内核模块的Makefile（模块源码在内核源码外，且内核先编译）
# 1、找内核的Makefile
# 2、内核的Makefile找内核模块的Makeifle
# 内核模块的Makeifle定义要编译对象
ifneq ($(KERNELRELEASE),)
	obj-m += uipc.o
else
        # dirty workaround for now
	obj-m += uipc.o
endif

KERNELDIR :=  /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)
USER_SOURCE := uipc_user.c

all:
	$(CC) -o uipc $(USER_SOURCE)
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
clean:
	rm uipc
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean
