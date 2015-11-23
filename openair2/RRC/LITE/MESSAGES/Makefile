include $(OPENAIR_DIR)/common/utils/Makefile.inc
OPENAIR2_TOP = ../../..
include $(OPENAIR2_DIR)/RRC/LITE/MESSAGES/Makefile.inc


EXTRA_CFLAGS += -I$(OPENAIR1_DIR) -I/usr/realtime/include -D__need_Emath -D__NO_ASSERT_H__ 

ifdef RTAI
# Get the RTAI variables
CCC = $(shell rtai-config --cc)
RTAI_SUBVERSION=$(shell rtai-config --version | sed -e 's/^..\(.\).*$$/\1/')
IS_RTAI_SUBVERSION_LESS_THAN_FIVE=$(shell if [ $(RTAI_SUBVERSION) -lt 5 ] ; then echo true ; fi)

EXTRA_CFLAGS += -D__IN_RTAI__ $(shell rtai-config --module-cflags) -g -march=pentium4 -DNODE_RG -DBIGPHYSAREA -DRTAI -DRTAI_ENABLED -fno-stack-protector -DHW_PREFIX_REMOVAL -Wall -D__KERNEL__ -DMODULE -D_LOOSE_KERNEL_NAMES -DWIDENS_DLC -I$(KERNEL_DIR)/build/include -I$(KERNEL_DIR)/build/include/asm/mach-default -include $(KERNEL_DIR)/build/include/linux/autoconf.h $(if $(IS_RTAI_SUBVERSION_LESS_THAN_FIVE),-DRTAI_ISNT_POSIX,)
endif


obj-m = asn1_msg_kern.o
asn1_msg_kern-objs = $(ASN1_CONVERTER_OBJS) $(ASN1_MODULE_OBJS) bsearch.o rtai_mem.o

clean:
	rm -rf $(asn1_msg_kern-objs) $(obj-m)

test:
	echo $(asn1_msg_kern-objs) $(obj-m)

regen: regenerate-from-asn1-source

regenerate-from-asn1-source:
	asn1c -gen-PER -fcompound-names -fnative-types -fskeletons-copy asn1c/ASN1_files/EUTRA-RRC-Definitions.asn

