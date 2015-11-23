ifeq ($(KERNELRELEASE),)

KERNELDIR ?= ~/linux
PWD := $(shell pwd)

.PHONY: build user clean

build:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

user: ropdetect

ropdetect: ropdetect_user.o
	$(CC) $(LDFLAGS) -o $@ $?

ropsimulate: ropsimulate.c
	$(CC) -static -O0 -march=armv7-a -mthumb-interwork -mthumb -fno-pic -fno-pie -o $@ $?

clean:
	rm -rf ropdetect ropsimulate *.o *~ core .depend .*.cmd *.ko *.mod.c
else

$(info Building with KERNELRELEASE = ${KERNELRELEASE})
obj-m :=    ropdetect.o
CFLAGS_ropdetect.o := -std=gnu99 -DEXCLUSIVE_CPU_ACCESS

endif
