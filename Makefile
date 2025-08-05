obj-m += easymoney.o
easymoney-objs := krootkit.o 

EXTRA_CFLAGS = -std=gnu99 -I$(PWD)/../include

KDIR ?= /lib/modules/$(shell uname -r)/build

ifeq ($(DEBUG), 1)
	EXTRA_CFLAGS += -DDEBUG
endif

all:
	make -C $(KDIR) M=$(PWD) modules

clean:
	make -C $(KDIR) M=$(PWD) clean
