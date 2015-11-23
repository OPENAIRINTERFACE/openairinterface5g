include $(OPENAIR_DIR)/common/utils/Makefile.inc

COMMON_UTILS_DIR = $(OPENAIR_DIR)/common/utils
TOP_DIR = $(OPENAIR1_DIR)
OPENAIR1_TOP = $(OPENAIR1_DIR)
OPENAIR2_TOP = $(OPENAIR2_DIR)
OPENAIR3 = $(OPENAIR3_DIR)

CFLAGS += -DMAX_NUM_CCs=1 -Wall -DPHYSIM -DNODE_RG -DUSER_MODE -DNB_ANTENNAS_RX=2 -DNB_ANTENNAS_TXRX=2 -DNB_ANTENNAS_TX=2 -DPHY_CONTEXT=1 -DMALLOC_CHECK_=1 -DENABLE_VCD_FIFO -DLOG_NO_THREAD # -Wno-packed-bitfield-compat

# enable C99 mode
CFLAGS += -std=gnu99

# DCI Debug
# CFLAGS += -DDEBUG_DCI_ENCODING
# TODO: only for framegen
#CFLAGS += -DOPENAIR2
#CFLAGS += -DDEBUG_PHY_PROC
#CFLAGS += -DDEBUG_GROUPHOP

ifdef OMP
CFLAGS += -DOMP -fopenmp
endif
ifdef LLR8
CFLAGS += -DLLR8
endif
CFLAGS += -DNEW_FFT

LFLAGS = -lm -lblas -lxml2 -lrt -lpthread $(LFDS_DIR)/bin/liblfds611.a

ifdef GPIB
LFLAGS += -lgpib
endif

CFLAGS += -DOPENAIR_LTE #-DOFDMA_ULSCH #-DIFFT_FPGA -DIFFT_FPGA_UE
#CFLAGS += -DTBS_FIX
CFLAGS += -DCELLULAR

ASN1_MSG_INC = $(OPENAIR2_DIR)/RRC/LITE/MESSAGES

ifdef EMOS
CFLAGS += -DEMOS
endif

ifdef DEBUG_PHY
CFLAGS += -DDEBUG_PHY
endif

ifdef MeNBMUE
CFLAGS += -DMeNBMUE
endif

ifdef MU_RECEIVER
CFLAGS += -DMU_RECEIVER
endif

ifdef ZBF_ENABLED
CFLAGS += -DNULL_SHAPE_BF_ENABLED
endif

ifdef RANDOM_BF
CFLAGS += -DRANDOM_BF
endif

ifdef PBS_SIM
CFLAGS += -DPBS_SIM
endif

ifdef XFORMS
CFLAGS += -DXFORMS
LFLAGS += -lforms
endif

ifdef SMBV
CFLAGS += -DSMBV
endif

ifdef PERFECT_CE
CFLAGS += -DPERFECT_CE
endif

ifdef BIT8_TX
CFLAGS += -DBIT8_TX
endif

CFLAGS += -DNO_RRM -DOPENAIR1 -DPHY_ABSTRACTION #-DOPENAIR2

CFLAGS += -I/usr/include/X11 -I/usr/X11R6/include

ifdef RTAI
CFLAGS += -DRTAI_ENABLED  -D__IN_RTAI__ $(shell rtai-config --lxrt-cflags)
LFLAGS += $(shell rtai-config --lxrt-ldflags) -llxrt
endif

include $(COMMON_UTILS_DIR)/Makefile.inc
include $(TOP_DIR)/PHY/Makefile.inc
include $(TOP_DIR)/SCHED/Makefile.inc
include $(TOP_DIR)/SIMULATION/Makefile.inc
include $(OPENAIR2_DIR)/LAYER2/Makefile.inc
include $(OPENAIR2_DIR)/UTIL/Makefile.inc
include $(OPENAIR2_DIR)/RRC/LITE/MESSAGES/Makefile.inc

#LOG_OBJS +=  $(LOG_DIR)/vcd_signal_dumper.o

CFLAGS += $(L2_incl) -I$(ASN1_MSG_INC) -I$(TOP_DIR) -I$(OPENAIR2_DIR) -I$(OPENAIR3) $(UTIL_incl) $(UTILS_incl)
# EXTRA_CFLAGS = 

#STATS_OBJS += $(TOP_DIR)/ARCH/CBMIMO1/DEVICE_DRIVER/cbmimo1_proc.o

#LAYER2_OBJ += $(OPENAIR2_DIR)/LAYER2/MAC/rar_tools.o
#LAYER2_OBJ = $(OPENAIR2_DIR)/LAYER2/MAC/lte_transport_init.o

OBJ = $(PHY_OBJS) $(SIMULATION_OBJS) $(TOOLS_OBJS) $(SCHED_OBJS) $(LAYER2_OBJ) $(LOG_OBJS) $(ITTI_DIR)/backtrace.o
ifdef GPIB
OBJ += LTE_Configuration.o
endif
#OBJ2 = $(PHY_OBJS) $(SIMULATION_OBJS) $(TOOLS_OBJS)  

ifdef XFORMS
OBJ += $(OPENAIR1_DIR)/PHY/TOOLS/lte_phy_scope.o
endif

ifdef STATIC
LFLAGS += -static -L/usr/lib/libblas
endif

all: dlsim pbchsim pdcchsim ulsim pucchsim prachsim mbmssim scansim

$(LFDS_DIR)/bin/liblfds611.a:
	$(MAKE) -C $(LFDS_DIR) -f makefile.linux

test: $(SIMULATION_OBJS) $(TOOLS_OBJS) $(TOP_DIR)/PHY/INIT/lte_init.o test.c
	$(CC)  test.c -I$(TOP_DIR) -o test $(CFLAGS) $(SIMULATION_OBJS) $(TOOLS_OBJS) -lm 

OBJ += $(LIST_OBJ) 
OBJ += $(MEM_OBJ)
$(OBJ) : %.o : %.c
	@echo Compiling $< ...
	@$(CC) -c $(CFLAGS) -o $@ $<

dlsim : $(OBJ) dlsim.c $(LFDS_DIR)/bin/liblfds611.a
	@echo "Compiling dlsim.c ..."
	@$(CC) dlsim.c  -o dlsim $(CFLAGS) $(OBJ) $(LFLAGS)

dlsim2 : $(OBJ) dlsim2.c
	@echo "Compiling dlsim2.c ..."
	@$(CC) dlsim2.c  -o dlsim2 $(CFLAGS) $(OBJ) $(LFLAGS)

framegen: $(OBJ) framegen.c
	@echo "Compiling framegen.c"
	@$(CC) framegen.c -o framegen $(CFLAGS) $(OBJ) $(LFLAGS) 

pbchsim : $(OBJ) pbchsim.c $(LFDS_DIR)/bin/liblfds611.a
	@echo "Compiling pbchsim.c"
	@$(CC) pbchsim.c  -o pbchsim $(CFLAGS) $(OBJ) $(LFLAGS) 

scansim : $(OBJ) scansim.c $(LFDS_DIR)/bin/liblfds611.a
	@echo "Compiling scansim.c"
	@$(CC) scansim.c -o scansim $(CFLAGS) $(OBJ) $(LFLAGS)

mbmssim : $(OBJ) mbmssim.c $(LFDS_DIR)/bin/liblfds611.a
	@echo "Compiling mbmssim.c"
	@$(CC) mbmssim.c  -o mbmssim $(CFLAGS) $(OBJ) $(LFLAGS) 

pdcchsim : $(OBJ) pdcchsim.c $(LFDS_DIR)/bin/liblfds611.a
	@echo "Compiling pdcchsim.c"
	@$(CC) pdcchsim.c  -o pdcchsim $(CFLAGS) $(OBJ) $(LFLAGS) 

pucchsim : $(OBJ) pucchsim.c $(LFDS_DIR)/bin/liblfds611.a
	@echo "Compiling pucchsim.c"
	@$(CC) pucchsim.c  -o pucchsim $(CFLAGS) $(OBJ) $(LFLAGS) 

prachsim : $(OBJ) prachsim.c $(LFDS_DIR)/bin/liblfds611.a
	@echo "Compiling prachsim.c"
	@$(CC) prachsim.c  -o prachsim $(CFLAGS) $(OBJ) $(LFLAGS) 

ulsim : $(OBJ) ulsim.c $(LFDS_DIR)/bin/liblfds611.a #ulsim_form.c
	@echo "Compiling [ulsim.c]"
	@$(CC) ulsim.c -o ulsim $(CFLAGS) $(OBJ) $(LFLAGS) 

syncsim : $(OBJ) syncsim.c $(LFDS_DIR)/bin/liblfds611.a
	@echo "Compiling syncsim.c"
	@$(CC) syncsim.c  -o syncsim $(CFLAGS) $(OBJ) $(LFLAGS) 

clean :
	rm -f $(OBJ)
	rm -f *.o

cleanall : clean
	rm -f dlsim pbchsim pdcchsim ulsim pucchsim mbmssim prachsim 
	rm -f *.exe*

showflags :
	@echo $(CFLAGS)
	@echo $(LFLAGS)

showobj:
	@echo $(SCHED_OBJS)

cppcheck:
	@echo "cppcheck ..."
	cppcheck $(INCLUDES) ${OBJ:.o=.c} ${OAISIM_OBJS,.o=.c} ${ASN1_RRC_MSG_OBJS1,.o=.c}
