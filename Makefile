obj-m += ckv.o
ckv-objs := ./ckv_src.o ./commands.o ./string_functions.o ./structures.o ./list_interactions.o ./buffer_interactions.o

KDIR = /lib/modules/`uname -r`/build

all:
	make -C $(KDIR) M=$(PWD) modules

 
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
