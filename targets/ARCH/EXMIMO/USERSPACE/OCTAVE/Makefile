CC = gcc
CFLAGS = -DEXMIMO -g 
XTRA_CFLAGS = -msse -msse2 -mssse3 -fPIC

#ifndef OPENAIR0_DIR
#  $(error Environment variable OPENAIR0_DIR must be set!)
#else
#  CFLAGS += -I$(OPENAIR0_DIR)/express-mimo/software/pcie
#endif


GCCVERSION = $(shell gcc --version | grep ^gcc | sed 's/^.* //g')
ifeq "$(GCCVERSION)"  "4.4.3"
    CFLAGS += -Wno-packed-bitfield-compat
endif
ifeq "$(GCCVERSION)" "4.5.2"
    CFLAGS += -Wno-packed-bitfield-compat
endif
ifeq "$(GCCVERSION)" "4.6.1"
    CFLAGS += -Wno-packed-bitfield-compat
endif


OPENAIRTARGETS_DIR ?=../../../..

OPENAIROBJS += $(OPENAIRTARGETS_DIR)/ARCH/EXMIMO/USERSPACE/LIB/openair0_lib.o
CFLAGS += -I$(OPENAIRTARGETS_DIR)/ARCH/EXMIMO/USERSPACE/LIB -I$(OPENAIRTARGETS_DIR)/ARCH/EXMIMO/DEFS -I$(OPENAIRTARGETS_DIR)/ARCH/COMMON

#OPENAIROBJS = $(OPENAIR1_DIR)/PHY/CONFIG/openair_readconfigfile.o $(OPENAIR1_DIR)/PHY/CONFIG/openair_configure.o 

OCTAVEOBJS =  oarf_config_exmimo.oct oarf_get_frame.oct oarf_stop.oct oarf_send_frame.oct oarf_get_num_detected_cards.oct oarf_stop_without_reset.oct

all: $(OCTAVEOBJS) gpib_send.oct 

oarf: $(OCTAVEOBJS)

$(OPENAIROBJS) : %.o : %.c
	gcc -c $(CFLAGS) $(XTRA_CFLAGS) -o $@ $<

test:
	echo $(OPENAIROBJS)

$(OCTAVEOBJS) : %.oct : %.cc $(OPENAIROBJS) 
	mkoctfile -o $@ $(CFLAGS) -lm $(OPENAIROBJS) $<

gpib_send.oct:	gpib_send.cc
	mkoctfile -I/usr/include/gpib -I/usr/local/include/gpib -lgpib gpib_send.cc

clean:
	rm -f gpib_send.oct $(OCTAVEOBJS) $(OPENAIROBJS) *.o octave-core
