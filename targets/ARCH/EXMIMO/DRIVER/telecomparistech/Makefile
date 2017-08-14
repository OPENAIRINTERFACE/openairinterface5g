obj-m += em1.o
em1-objs = em1_drv.o em1_fifos.o em1_rw.o em1_dev.o em1_ioctl.o em1_mmap.o

default:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
