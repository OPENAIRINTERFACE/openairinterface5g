include $(OPENAIR_DIR)/common/utils/Makefile.inc

COMMON_UTILS_DIR = $(OPENAIR_DIR)/common/utils
TOP_DIR = $(OPENAIR1_DIR)
OPENAIR1_TOP = $(OPENAIR1_DIR)
OPENAIR2_TOP = $(OPENAIR2_DIR)
OPENAIR3 = $(OPENAIR3_DIR)


CFLAGS += -DNODE_RG -DNO_UL_REF -DNB_ANTENNAS_RX=2 -DNB_ANTENNAS_TX=2 -DOPENAIR_LTE -DMAX_NUM_CCs=1

ASN1_MSG_INC = $(OPENAIR2_DIR)/RRC/LITE/MESSAGES

ifdef LLR8
CFLAGS += -DLLR8
endif

ifeq ($(turbo_flag),1)
  CFLAGS += -DTURBO_S
endif

include $(TOP_DIR)/PHY/Makefile.inc
#include $(TOP_DIR)/SCHED/Makefile.inc
SCHED_OBJS = $(TOP_DIR)/SCHED/phy_procedures_lte_common.o
include $(TOP_DIR)/SIMULATION/Makefile.inc
include $(TOP_DIR)/../openair2/LAYER2/Makefile.inc
include $(OPENAIR2_DIR)/RRC/LITE/MESSAGES/Makefile.inc
include $(OPENAIR2_DIR)/LAYER2/Makefile.inc
include $(OPENAIR2_DIR)/UTIL/Makefile.inc

LAYER2_OBJ = $(OPENAIR2_DIR)/LAYER2/MAC/lte_transport_init.o

OBJ = $(PHY_OBJS) $(SIMULATION_OBJS) $(TOOLS_OBJS) $(SCHED_OBJS) $(LAYER2_OBJ) $(LOG_OBJS) #$(ASN1_MSG_OBJS) 

CFLAGS += -I$(TOP_DIR) $(L2_incl) -I$(ASN1_MSG_INC) -I$(OPENAIR3) $(UTIL_incl) $(UTILS_incl)
CFLAGS += -DNO_RRM

all: ltetest

#$(OBJ) : %.o : %.c 
#	$(CC) -c -O2 $(CFLAGS) -Wall -I$(TOP_DIR) -I$(TOP_DIR)/PHY/CODING -I$(OPENAIR0_DIR)/express-mimo/simulator/baseband/C/channel_decoder -o $@ $<

$(OBJ) : %.o : %.c
	$(CC) -c $(CFLAGS) -Wall -I$(TOP_DIR) -I$(TOP_DIR)/PHY/CODING -I$(OPENAIR0_DIR)/express-mimo/simulator/baseband/C/channel_decoder -o $@ $<

ltetest: $(OBJ) ltetest.c
	$(CC) ltetest.c -O2 -I$(TOP_DIR) -I$(TOP_DIR)/PHY/CODING -I$(OPENAIR0_DIR)/express-mimo/simulator/baseband/C/channel_decoder -o ltetest $(CFLAGS) $(OBJ) -lm -DOPENAIR_LTE -lblas -lrt

pdcch_test: $(OBJ) pdcch_test.c
	$(CC) pdcch_test.c -I$(TOP_DIR) -o pdcch_test $(CFLAGS) $(PHY_OBJS) -lm -lblas
viterbi_test: $(OBJ) viterbi_test.c
	$(CC) viterbi_test.c -I$(TOP_DIR) -o viterbi_test $(CFLAGS) $(PHY_OBJS) -lm -lblas

clean: 
	rm $(OBJ)
	rm -f ltetest
	rm -f *.m

