obj-m += spi_mfrc522_drv.o
spi_mfrc522_drv-objs := mfrc522_drv.o MFRC522.o 

KDIR := /lib/modules/$(shell uname -r)/build
all:
	make -C $(KDIR) M=$(shell pwd) modules
dtbs:
	make $(build)=arch/$(SCRATCH)/boot/dts

clean:
	make -C $(KDIR) M=$(shell pwd) clean
