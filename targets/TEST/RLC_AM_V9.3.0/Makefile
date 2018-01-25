include $(OPENAIR_DIR)/common/utils/Makefile.inc

NUM_CORES=$(shell cat /proc/cpuinfo | grep processor | wc -l)

TOP_DIR      = $(OPENAIR1_DIR)
OPENAIR1_TOP = $(OPENAIR1_DIR)
OPENAIR2_TOP = $(OPENAIR2_DIR)
OPENAIR3_TOP = $(OPENAIR3_DIR)
OPENAIR3     = $(OPENAIR3_DIR)

CFLAGS += -m32 -DNB_ANTENNAS_RX=2 -DNB_ANTENNAS_TX=2 -I/usr/include/X11 

ASN1_MSG_INC = $(OPENAIR2_DIR)/RRC/LITE/MESSAGES

CFLAGS += -DOPENAIR_LTE #-DOFDMA_ULSCH -DIFFT_FPGA -DIFFT_FPGA_UE
CFLAGS += -DMAC_CONTEXT=1 -DPHY_CONTEXT=1 #-DRLC_UM_TEST_TRAFFIC=1
#CFLAGS += -DLLR8

ifndef OPENAIR2
OPENAIR2=1
endif

ifdef TRAFFIC_TM5
CFLAGS += -DRLC_UM_TEST_TRAFFIC=1 #-DFULL_BUFFER=1
endif

# activate OCG and libxml only under linux
ifeq ($(linux),1)
CFLAGS += -I/usr/include/libxml2 -L/usr/local/lib -I/usr/include/atlas -L/usr/X11R6/lib
OCG_FLAG = 1
cygwin=0
CFLAGS += -DLINUX
else
cygwin=1
OCG_FLAG = 0
endif


ifdef XFORMS
CFLAGS += -DXFORMS
endif

ifdef PRINT_STATS
CFLAGS += -DPRINT_STATS
endif

ifndef rrc_cellular
rrc_cellular = 0
rrc_cellular_eNB = 0
rrc_cellular_UE = 0
else
  ifeq ($(eNB_flag),1)
    rrc_cellular_eNB=1
  endif
  ifeq ($(UE_flag),1)
    rrc_cellular_UE=1
  endif
endif

ifdef DEBUG_PHY
CFLAGS += -DDEBUG_PHY
endif

# ln -sf EUTRA-RRC-Definitions-a20.asn EUTRA-RRC-Definitions.asn
# make cleanasn1
#make all Rel10=1
export IS_REL10=$(shell if [ -f .lock-rel10 ] ; then  echo "1" ; else  echo "0" ; fi)
export IS_REL8=$(shell if [ -f .lock-rel8 ] ; then  echo "1" ; else  echo "0" ; fi)

ifdef  Rel10
CFLAGS += -DRel10
ifeq ($(IS_REL10), 0)
 $(shell cd $(OPENAIR2_DIR)/RRC/LITE/MESSAGES/asn1c/ASN1_files/ ; rm EUTRA-RRC-Definitions.asn ; ln -s EUTRA-RRC-Definitions-a20.asn EUTRA-RRC-Definitions.asn ; cd $(OPENAIR2_DIR)/RRC/LITE/MESSAGES ; ln -s asn1-86_constants.h asn1_constants.h ; cd $(OPENAIR_TARGETS)/SIMU/USER/ ; rm -f $(ASN1_MSG_OBJS1) ; rm -f $(OPENAIR2_DIR)/RRC/LITE/MESSAGES/Makefile.inc.generated ; rm -f $(L2_OBJS) ; rm -f *.o ; rm -f oaisim ; touch .lock-rel10 ; rm .lock-rel8 ;)
endif
else # default is rel 8
ifeq ($(IS_REL8), 0)
 $(shell cd $(OPENAIR2_DIR)/RRC/LITE/MESSAGES/asn1c/ASN1_files/ ; rm EUTRA-RRC-Definitions.asn ; ln -s EUTRA-RRC-Definitions-86.asn EUTRA-RRC-Definitions.asn ;  cd $(OPENAIR2_DIR)/RRC/LITE/MESSAGES ; ln -s asn1-a20_constants.h asn1_constants.h ; cd $(OPENAIR_TARGETS)/SIMU/USER/ ; rm -f $(ASN1_MSG_OBJS1) ; rm -f $(OPENAIR2_DIR)/RRC/LITE/MESSAGES/Makefile.inc.generated ; rm -f $(L2_OBJS); rm -f *.o ; rm -f oaisim ; touch .lock-rel8 ; rm .lock-rel10 ; )
endif
endif

ifeq ($(NAS), 1)
CFLAGS += -DPDCP_USE_NETLINK -DLINUX
NAS_FLAG=1
endif

ifdef TEST_OMG
CFLAGS += -DTEST_OMG
#CFLAGS += -DDEBUG_OMG
endif
# after the execution, a profiling output "gmon.out" will be created,
#then we need to run "gprof ./oaisim > gmon.txt" in the same directory as "gmon.out"
# you could also use the script "grpof2dot.py gmon.txt" for visual diagrams
ifdef GPROF
CFLAGS += -pg
endif

#require kernel patch for oai driver to enable address autoconfiguration (IPv6 only)
ifdef ADDCONF
CFLAGS+=-DADDRCONF
endif

ifdef OAI_NW_DRIVER_TYPE_ETHERNET
CFLAGS+=-DOAI_NW_DRIVER_TYPE_ETHERNET
endif

CFLAGS += $(shell if [ `uname -o` = "Cygwin" ] ; then echo "-DCYGWIN" ;fi)

CFLAGS += -DENABLE_USE_CPU_EXECUTION_TIME

ifneq ($(USE_MME), R8)
UPDATE_RELEASE_9=1
endif



EXTRA_CFLAGS += -DOPENAIR2
EXTRA_CFLAGS += -DASSERT
EXTRA_CFLAGS += -DNO_RRM
EXTRA_CFLAGS += -DTEST_RLC_AM

COMMON_DIR = $(OPENAIR2_TOP)/COMMON
L2_INTERFACE_DIR = $(OPENAIR2_TOP)/RRC/L2_INTERFACE
RLC_UM_DIR = $(OPENAIR2_TOP)/LAYER2/RLC/UM_v9.3.0
RLC_AM_DIR = $(OPENAIR2_TOP)/LAYER2/RLC/AM_v9.3.0
RLC_TM_DIR = $(OPENAIR2_TOP)/LAYER2/RLC/TM_v9.3.0
RLC_DIR = $(OPENAIR2_TOP)/LAYER2/RLC
PDCP_DIR = $(OPENAIR2_TOP)/LAYER2/PDCP
MEM_DIR = $(OPENAIR2_TOP)/UTIL/MEM
LIST_DIR = $(OPENAIR2_TOP)/UTIL/LISTS
MATH_DIR = $(OPENAIR2_TOP)/UTIL/MATH
TIMER_DIR = $(OPENAIR2_TOP)/UTIL/TIMER
PHY_INTERFACE_DIR = $(OPENAIR2_TOP)/PHY_INTERFACE
RRC_DIR = $(OPENAIR2_TOP)/RRC/LITE
L2_INTERFACE_DIR = $(OPENAIR2_TOP)/RRC/L2_INTERFACE
MAC_DIR = $(OPENAIR2_TOP)/LAYER2/MAC
LOG_DIR=$(OPENAIR2_TOP)/UTIL/LOG
OCG_DIR=$(OPENAIR2_TOP)/UTIL/OCG
OPT_DIR=$(OPENAIR2_TOP)/UTIL/OPT


SOURCES_L2 =  $(LIST_DIR)/list.c
SOURCES_L2 +=  $(LIST_DIR)/list2.c

SOURCES_L2 +=  $(RLC_AM_DIR)/rlc_am.c
SOURCES_L2 +=  $(RLC_AM_DIR)/rlc_am_init.c
SOURCES_L2 +=  $(RLC_AM_DIR)/rlc_am_timer_poll_retransmit.c
SOURCES_L2 +=  $(RLC_AM_DIR)/rlc_am_timer_reordering.c
SOURCES_L2 +=  $(RLC_AM_DIR)/rlc_am_timer_status_prohibit.c
SOURCES_L2 +=  $(RLC_AM_DIR)/rlc_am_segment.c
SOURCES_L2 +=  $(RLC_AM_DIR)/rlc_am_segments_holes.c
SOURCES_L2 +=  $(RLC_AM_DIR)/rlc_am_in_sdu.c
SOURCES_L2 +=  $(RLC_AM_DIR)/rlc_am_receiver.c
SOURCES_L2 +=  $(RLC_AM_DIR)/rlc_am_retransmit.c
SOURCES_L2 +=  $(RLC_AM_DIR)/rlc_am_windows.c
SOURCES_L2 +=  $(RLC_AM_DIR)/rlc_am_rx_list.c
SOURCES_L2 +=  $(RLC_AM_DIR)/rlc_am_reassembly.c
SOURCES_L2 +=  $(RLC_AM_DIR)/rlc_am_status_report.c
SOURCES_L2 +=  $(RLC_AM_DIR)/rlc_am_test.c

SOURCES_L2 +=  $(MEM_DIR)/mem_block.c

LOG_OBJS =  $(LOG_DIR)/log.o 
LOG_OBJS +=  $(LOG_DIR)/vcd_signal_dumper.o
OPT_OBJS =  $(OPT_DIR)/probe.o

L2_OBJS=$(addsuffix .o,$(basename $(SOURCES_L2))) $(PHY_INTERFACE_OBJS) $(MAC_OBJS) $(L2_INTERFACE_OBJS) $(RRC_OBJS) $(LOG_OBJS) $(OPT_OBJS)


L2_incl = -I$(OPENAIR2_TOP)  -I$(COMMON_DIR) -I$(RLC_DIR) -I$(RLC_AM_DIR) -I$(RLC_UM_DIR)  -I$(RLC_TM_DIR) -I$(PDCP_DIR) -I$(MEM_DIR) -I$(LIST_DIR)  -I$(LOG_DIR) -I$(MATH_DIR) -I$(TIMER_DIR) -I$(OPENAIR3)/MESH -I$(OPENAIR2_TOP)/RRC/LITE -I$(OPENAIR2_TOP)/RRC/LITE/MESSAGES -I$(OPENAIR3)/MESH/RRM -I$(OPT_DIR)





ASN1_MSG_OBJS2 := $(foreach file, $(ASN1_MSG_OBJS), $(OPENAIR2_DIR)/RRC/LITE/MESSAGES/$(file))

OBJ = $(PHY_OBJS) $(SIMULATION_OBJS)  $(SCHED_OBJS) $(STATS_OBJS) $(ASN1_MSG_OBJS2)

all: testam


$(OBJ) : %.o : %.c
	$(CC) -c $(CFLAGS) $(EXTRA_CFLAGS) -DPHY_CONTEXT=1 -I$(TOP_DIR) $(L2_incl) -I$(ASN1_MSG_INC) -o $@ $<

$(L2_OBJS) : %.o : %.c
	$(CC) -c $(CFLAGS) $(EXTRA_CFLAGS) -DMAC_CONTEXT=1 -DPHY_CONTEXT=1 -I$(TOP_DIR) $(L2_incl) -I$(ASN1_MSG_INC) -o $@ $<

testam : $(OBJ) $(L2_OBJS) oaisim.c
	echo CYGWIN variable is $(cygwin)
	$(CC)  oaisim.c -I$(TOP_DIR) $(L2_incl) -I$(ASN1_MSG_INC) -o testam $(CFLAGS) $(EXTRA_CFLAGS) $(OBJ) $(L2_OBJS) -lm -lrt


clean:
	rm -f testam
	rm -f $(OBJ)
	rm -f $(L2_OBJS)
	rm -f *.o
	rm -f *.exe*

