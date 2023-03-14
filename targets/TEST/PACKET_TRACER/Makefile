include $(OPENAIR_DIR)/common/utils/Makefile.inc

TOP_DIR      = $(OPENAIR1_DIR)
OPENAIR1_TOP = $(OPENAIR1_DIR)
OPENAIR2_TOP = $(OPENAIR2_DIR)
OPENAIR3_TOP = $(OPENAIR3_DIR)
OPENAIR3     = $(OPENAIR3_DIR)

CFLAGS += -m32 -DPHYSIM -DNB_ANTENNAS_RX=2 -DNB_ANTENNAS_TX=2 -I/usr/include/X11

ASN1_MSG_INC = $(OPENAIR2_DIR)/RRC/LITE/MESSAGES

CFLAGS += -DOPENAIR_LTE #-DOFDMA_ULSCH -DIFFT_FPGA -DIFFT_FPGA_UE  
CFLAGS += -DMAC_CONTEXT=1 -DPHY_CONTEXT=1
CFLAGS += -DEMIT_ASN_DEBUG=1
ifndef OPENAIR2
OPENAIR2=1
endif

# activate OCG and libxml only under linux
ifeq ($(linux),1) 
CFLAGS += -I/usr/include/libxml2 -L/usr/local/lib -I/usr/include/atlas -L/usr/X11R6/lib 
CFLAGS += -DLINUX
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

#CFLAGS += -DPDCP_USE_NETLINK -DLINUX
#NAS_FLAG=1

#require kernel patch for oai driver to enable address autoconfiguration (IPv6 only)
ifdef ADDCONF
CFLAGS+=-DADDRCONF
endif


ifdef TRAFFIC_TM5
CFLAGS += -DRLC_UM_TEST_TRAFFIC=1 -DFULL_BUFFER=1
endif

include $(OPENAIR1_DIR)/PHY/Makefile.inc
include $(OPENAIR1_DIR)/SCHED/Makefile.inc
include $(OPENAIR2_DIR)/LAYER2/Makefile.inc
include $(OPENAIR1_DIR)/SIMULATION/ETH_TRANSPORT/Makefile.inc
include $(OPENAIR2_DIR)/RRC/LITE/MESSAGES/Makefile.inc
include $(OPENAIR2_DIR)/RRC/NAS/Makefile.inc
include $(OPENAIR2_DIR)/UTIL/Makefile.inc

ASN1_MSG_OBJS1=$(addprefix $(OPENAIR2_DIR)/RRC/LITE/MESSAGES/, $(ASN1_MSG_OBJS))
#L2_OBJS =
#EXTRA_CFLAGS =

export IS_REL10=$(shell if [ -f .lock-rel10 ] ; then  echo "1" ; else  echo "0" ; fi)
export IS_REL8=$(shell if [ -f .lock-rel8 ] ; then  echo "1" ; else  echo "0" ; fi)

ifdef Rel10
CFLAGS += -DRel10
ifeq ($(IS_REL10), 0)
 $(shell cd $(OPENAIR2_DIR)/RRC/LITE/MESSAGES/asn1c/ASN1_files/ ; rm EUTRA-RRC-Definitions.asn ; ln -s EUTRA-RRC-Definitions-a20.asn EUTRA-RRC-Definitions.asn ; cd $(OPENAIR_TARGETS)/TEST/PACKET_TRACER/ ; rm -f $(ASN1_MSG_OBJS1) ; rm -f $(OPENAIR2_DIR)/RRC/LITE/MESSAGES/Makefile.inc.generated ; rm -f $(L2_OBJS) ; touch .lock-rel10 ; rm .lock-rel8 ;)
endif
else # default is rel 8
ifeq ($(IS_REL8), 0)
 $(shell cd $(OPENAIR2_DIR)/RRC/LITE/MESSAGES/asn1c/ASN1_files/ ; rm EUTRA-RRC-Definitions.asn ; ln -s EUTRA-RRC-Definitions-86.asn EUTRA-RRC-Definitions.asn ;  cd $(OPENAIR_TARGETS)/TEST/PACKET_TRACER/ ; rm -f $(ASN1_MSG_OBJS1) ; rm -f $(OPENAIR2_DIR)/RRC/LITE/MESSAGES/Makefile.inc.generated ; rm -f $(L2_OBJS); rm -f *.o ; rm -f oaisim ; touch .lock-rel8 ; rm .lock-rel10 ; )
endif
endif
export IS_REL10=$(shell if [ -f .lock-rel10 ] ; then  echo "1" ; else  echo "0" ; fi)
export IS_REL8=$(shell if [ -f .lock-rel8 ] ; then  echo "1" ; else  echo "0" ; fi)

PT_OBJS = pt.o 

SIMULATION_OBJS += $(TOP_DIR)/SIMULATION/TOOLS/taus.o  
SIMULATION_OBJS += $(TOP_DIR)/SIMULATION/TOOLS/rangen_double.o  

OBJ = $(PHY_OBJS) $(SIMULATION_OBJS) $(SCHED_OBJS) $(L2_OBJS) $(TOOLS_OBJS) $(STATS_OBJS) $(ASN1_MSG_OBJS1) $(NAS_OBJS) $(INT_OBJS) $(UTIL_OBJ)

OBJ += $(OPENAIR_TARGETS)/SIMU/USER/init_lte.o

all: pt

printvars:
	@echo LINUX variable is $(linux)
	@echo rrc_cellular variable is $(rrc_cellular)
	@echo EXTRA_CFLAGS = $(EXTRA_CFLAGS)
	@echo OAI NETLINK FLAG is $(NAS_FLAG)
	@echo SSE3 proc is $(SSE3PROC)
	@echo IS_REL_8 is $(IS_REL_8)
	@echo IS_REL_10 is $(IS_REL_10)

$(OBJ): %.o : %.c
	@echo Compiling $<
	$(CC) -c $(CFLAGS) $(EXTRA_CFLAGS) -DPHY_CONTEXT=1 -I$(TOP_DIR) $(L2_incl) $(UTIL_incl) -I$(ASN1_MSG_INC) -o $@ $<

pt: $(OBJ) pt.c  
	@echo "Compiling pt.c ..."
	@$(CC)  pt.c -I$(TOP_DIR) $(L2_incl) $(UTIL_incl) -I$(ASN1_MSG_INC) -o pt $(CFLAGS) $(EXTRA_CFLAGS) $(OBJ) -lm  -lblas -lpthread -lxml2 -lX11 -lXpm -lrt  -lforms -llapack_atlas

ifeq ($(rrc_cellular_eNB),1)
	mv oaisim oaisim_eNB
endif
ifeq ($(rrc_cellular_UE),1)
	mv oaisim oaisim_UE
endif

nasmesh_fix:
	(cd $(OPENAIR2_DIR) && make nasmesh_netlink_address_fix.ko)
	(sudo insmod $(OPENAIR2_DIR)/NAS/DRIVER/MESH/nasmesh.ko)
nasmesh_nl:
	(cd $(OPENAIR2_DIR) && make nasmesh_netlink.ko)	
	(sudo insmod $(OPENAIR2_DIR)/NAS/DRIVER/MESH/nasmesh.ko)
rb_tool:
	(cd $(OPENAIR2_DIR)/NAS/DRIVER/MESH/RB_TOOL && make)

nasmesh_install:
	(sudo rmmod nasmesh)
	(sudo insmod $(OPENAIR2_DIR)/NAS/DRIVER/MESH/nasmesh.ko)
clean:
	rm -f oaisim
	rm -f $(OBJ)
	rm -f *.o
	rm -f *.exe*

cleanl1:
	rm -f oaisim
	rm -f $(PHY_OBJS) $(SCHED_OBJS)
	rm -f *.o
	rm -f *.exe

cleanl2:
	rm -f $(L2_OBJS)

cleanasn1:
	rm -f $(ASN1_MSG_OBJS1)
	rm -f $(OPENAIR2_DIR)/RRC/LITE/MESSAGES/Makefile.inc.generated

print:
	@echo $(CFLAGS)
	@echo $(EXTRA_CFLAGS)
	@echo $(OBJ)

showcflags:
	@echo $(CFLAGS)
