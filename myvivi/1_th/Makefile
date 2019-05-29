KERN_DIR = /usr/src/linux-headers-2.6.31-14-generic


.PHONY: clean all
all:
		make -C $(KERN_DIR) M=`pwd` modules
clean:
		make clean -C $(KERN_DIR) M=`pwd` modules

obj-m += vivi.o
