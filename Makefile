obj-m += foo_spi_driver.o 

KDIR := /lib/modules/$(shell uname -r)/build
all:
	make -C $(KDIR) M=$(shell pwd) modules
dtbs:
	make $(build)=arch/$(SCRATCH)/boot/dts

clean:
	make -C $(KDIR) M=$(shell pwd) clean
