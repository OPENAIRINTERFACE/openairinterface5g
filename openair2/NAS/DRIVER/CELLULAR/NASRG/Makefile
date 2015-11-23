# NAS CELLULAR Driver makefile
#
include $(OPENAIR_DIR)/common/utils/Makefile.inc

NAS_UPDIR	:= $(shell /bin/pwd)

####################################################
#  D E B U G   F L A G S
####################################################

####################################################
#      EXTRA COMPILER FLAGS
####################################################
EXTRA_CFLAGS = -fno-common -fno-stack-protector -mpreferred-stack-boundary=4 $(if $(SET_X64),-DARCH_64,) $(if $(SET_X64),-mcmodel=kernel,) $(if $(SET_X64),-m64,) 

ifdef ADDRCONF
EXTRA_CFLAGS += -DADDRCONF
endif

ifdef NAS_DRIVER_TYPE_ETHERNET
EXTRA_CFLAGS += -DNAS_DRIVER_TYPE_ETHERNET 
endif


ifdef PDCP_USE_NETLINK
EXTRA_CFLAGS += -DPDCP_USE_NETLINK
else
EXTRA_CFLAGS += $(shell rtai-config --module-cflags) -DRTAI -D__IN_RTAI__ -Wall
endif

ifdef LOOPBACK
EXTRA_CFLAGS += -DLOOPBACK_TEST
endif

ifdef ADDRESS_FIX
EXTRA_CFLAGS += -DNAS_ADDRESS_FIX
endif

####################################################
#      NASRGCELL extra compilation flags
####################################################
#RTAI=1
RRC_DIR := $(OPENAIR2_DIR)/RRC/CELLULAR
EXTRA_CFLAGS += -I$(RRC_DIR) -DNODE_RG

####################################################
#      LOADABLE MODULE GOALS
####################################################
obj-m += nascellrg.o
nascellrg-objs += nasrg_device.o
nascellrg-objs += nasrg_common.o
nascellrg-objs += nasrg_iocontrol.o
nascellrg-objs += nasrg_classifier.o
nascellrg-objs += nasrg_tool.o
nascellrg-objs += nasrg_ascontrol.o
ifdef PDCP_USE_NETLINK
nascellrg-objs += nasrg_netlink.o
endif

####################################################
#      REVOIR LE CLEAN
####################################################

#netlink.ko:
	#make $(x)$(y) PDCP_USE_NETLINK=1 V=1 -C $(KERNEL_DIR) M=`pwd` modules

#nasmesh.ko:
#	make  V=1 -C $(KERNEL_DIR) M=`pwd` modules
print:
	@echo RRC_DIR $(RRC_DIR)
clean:
	rm -f *.ko
	rm -f .*.ko.cmd
	rm -f .*.o.cmd
	rm -f *.o
	rm -f *.mod.c
	find . -name *.ko     -delete
	find . -name *.o      -delete
	find . -name *.mod.c  -delete
