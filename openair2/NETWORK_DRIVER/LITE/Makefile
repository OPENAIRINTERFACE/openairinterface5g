# NASMESH Driver makefile
#
include $(OPENAIR_DIR)/common/utils/Makefile.inc
NAS_UPDIR	:= $(shell /bin/pwd)

KERNEL_ARCH:=$(shell echo `uname -m`)
SET_X64:=$(shell if [ $(KERNEL_ARCH) = 'x86_64' ]; then echo true ; fi)

####################################################
#      EXTRA COMPILER FLAGS
####################################################
EXTRA_CFLAGS = -I$(OPENAIR2_DIR)/COMMON -fno-common -fno-stack-protector -mpreferred-stack-boundary=4 $(if $(SET_X64),-DARCH_64,) $(if $(SET_X64),-mcmodel=kernel,) 

ifdef ADDRCONF
EXTRA_CFLAGS += -DADDRCONF
endif

ifdef OAI_NW_DRIVER_TYPE_ETHERNET
EXTRA_CFLAGS += -DOAI_NW_DRIVER_TYPE_ETHERNET
endif

ifdef OAI_NW_DRIVER_USE_NETLINK
EXTRA_CFLAGS += -DOAI_NW_DRIVER_USE_NETLINK
else
EXTRA_CFLAGS += $(shell rtai-config --module-cflags) -DRTAI -D__IN_RTAI__ -Wall
endif

ccflags-y= $(CFLAGS) $(EXTRA_CFLAGS)
CFLAGS=

ifdef LOOPBACK
EXTRA_CFLAGS += -DLOOPBACK_TEST
endif

ifdef ADDRESS_FIX
EXTRA_CFLAGS += -DNAS_ADDRESS_FIX
endif

####################################################
#      LOADABLE MODULE GOALS
####################################################
obj-m += oai_nw_drv.o
oai_nw_drv-objs += device.o
oai_nw_drv-objs += common.o
oai_nw_drv-objs += ioctl.o
oai_nw_drv-objs += classifier.o
oai_nw_drv-objs += tool.o
ifdef OAI_NW_DRIVER_USE_NETLINK
oai_nw_drv-objs += netlink.o
endif

####################################################
#      REVOIR LE CLEAN
####################################################

#netlink.ko:
	#make $(x)$(y) PDCP_USE_NETLINK=1 V=1 -C $(KERNEL_DIR) M=`pwd` modules

#oai_nw_drv.ko:
#	make  V=1 -C $(KERNEL_DIR) M=`pwd` modules
print:
	@echo subversion : $(SUBVERSION)
clean:
	rm -f *.ko
	rm -f .*.ko.cmd
	rm -f .*.o.cmd
	rm -f *.o
	rm -f *.mod.c
	find . -name *.ko     -delete
	find . -name .*.o     -delete
	find . -name *.o      -delete
	find . -name *.mod.c  -delete
