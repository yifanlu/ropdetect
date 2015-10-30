ifeq ($(KERNELRELEASE),)

KERNELDIR ?= ~/linux
PWD := $(shell pwd)
CFLAGS := -gnu99

.PHONY: build clean

build:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c
else

$(info Building with KERNELRELEASE = ${KERNELRELEASE})
obj-m :=    ropdetect.o

endif
