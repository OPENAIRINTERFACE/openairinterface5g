# NASMESH Driver makefile
#
include $(OPENAIR_DIR)/common/utils/Makefile.inc
NAS_UPDIR	:= $(shell /bin/pwd)

####################################################
#      NASMESH compilation flags
####################################################
#RTAI=1

####################################################
#  D E B U G   F L A G S
####################################################

####################################################
#      EXTRA COMPILER FLAGS
####################################################
#EXTRA_CFLAGS = -fno-common -fno-stack-protector -mpreferred-stack-boundary=4 $(if $(SET_X64),-DARCH_64,) $(if $(SET_X64),-mcmodel=kernel,) $(if $(SET_X64),-m64,) 
EXTRA_CFLAGS += -I$(OPENAIR2_DIR)
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
ifdef NAS_DEBUG_RECEIVE
EXTRA_CFLAGS += -DNAS_DEBUG_RECEIVE
endif

ifdef NAS_DEBUG_SEND
EXTRA_CFLAGS += -DNAS_DEBUG_SEND
endif

ccflags-y := $(CFLAGS) $(EXTRA_CFLAGS)
CFLAGS=

####################################################
#      LOADABLE MODULE GOALS
####################################################
obj-m += nasmesh.o
nasmesh-objs += device.o
nasmesh-objs += common.o
nasmesh-objs += ioctl.o
nasmesh-objs += classifier.o
nasmesh-objs += tool.o
nasmesh-objs += mesh.o
ifdef PDCP_USE_NETLINK
nasmesh-objs += netlink.o
endif

####################################################
#      REVOIR LE CLEAN
####################################################

#netlink.ko:
#	make PDCP_USE_NETLINK=1 V=1 -C $(KERNEL_DIR) M=`pwd` modules

#nasmesh.ko:
#	make  V=1 -C $(KERNEL_DIR) M=`pwd` modules

clean:
	rm -f $(nasmesh-objs) $(obj-m)
#	make -C $(KERNEL_DIR) modules
