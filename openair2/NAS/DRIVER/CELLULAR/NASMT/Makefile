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
#      NASMTCELL extra compilation flags
####################################################
#RTAI=1
RRC_DIR := $(OPENAIR2_DIR)/RRC/CELLULAR
EXTRA_CFLAGS += -I$(RRC_DIR) -DNODE_MT

####################################################
#      LOADABLE MODULE GOALS
####################################################
obj-m += nascellmt.o
nascellmt-objs += nasmt_device.o
nascellmt-objs += nasmt_common.o
nascellmt-objs += nasmt_iocontrol.o
nascellmt-objs += nasmt_classifier.o
nascellmt-objs += nasmt_tool.o
nascellmt-objs += nasmt_ascontrol.o
ifdef PDCP_USE_NETLINK
nascellmt-objs += nasmt_netlink.o
endif

####################################################
#      REVOIR LE CLEAN
####################################################

#netlink.ko:
	#make $(x)$(y) PDCP_USE_NETLINK=1 V=1 -C $(KERNEL_DIR) M=`pwd` modules

#nasmesh.ko:
#	make  V=1 -C  $(KERNEL_DIR) M=`pwd` modules
print:
	@echo subversion : $(SUBVERSION)
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
