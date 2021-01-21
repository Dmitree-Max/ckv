obj-m += ckv.o

KDIR = /lib/modules/`uname -r`/build

all:
	make -C $(KDIR) M=$(PWD) modules

 
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
