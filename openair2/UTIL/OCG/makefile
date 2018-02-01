include $(OPENAIR_DIR)/common/utils/Makefile.inc
LIB = -lxml2

TOP_DIR      = ../..
OPENAIR1_TOP = ../..
OPENAIR2_TOP = ../../../openair2
OPENAIR3_TOP = ../../../openair3
OPENAIR3     = $(OPENAIR3_DIR)

CFLAGS += -DPHYSIM -DNODE_RG -DNB_ANTENNAS_RX=2 -DNB_ANTENNAS_TX=2 -DMAX_MODULES=1 -I/usr/include/X11 #-Wno-packed-bitfield-compat


CFLAGS += -m32 -DOPENAIR_LTE -DOPENAIR2 #-DOFDMA_ULSCH -DIFFT_FPGA -DIFFT_FPGA_UE 

#CFLAGS += -DXFORMS

ifdef DEBUG_PHY
CFLAGS += -DDEBUG_PHY
endif

ifdef PDCP_USE_NETLINK
CFLAGS += -DPDCP_USE_NETLINK -DLINUX -DDEBUG_CONTROL
endif

#ifdef OCG
CFLAGS += -I/usr/include/libxml2 -lxml2
#endif

include $(OPENAIR1_DIR)/PHY/Makefile.inc
include $(OPENAIR1_DIR)/SCHED/Makefile.inc
include $(OPENAIR2_DIR)/LAYER2/Makefile.inc
include $(OPENAIR1_DIR)/SIMULATION/ETH_TRANSPORT/Makefile.inc

SIMULATION_OBJS  = $(TOP_DIR)/SIMULATION/TOOLS/gauss.o  
SIMULATION_OBJS += $(TOP_DIR)/SIMULATION/TOOLS/random_channel.o  
SIMULATION_OBJS += $(TOP_DIR)/SIMULATION/TOOLS/rangen_double.o  
SIMULATION_OBJS += $(TOP_DIR)/SIMULATION/TOOLS/taus.o  
SIMULATION_OBJS += $(TOP_DIR)/SIMULATION/TOOLS/multipath_channel.o
SIMULATION_OBJS += $(TOP_DIR)/SIMULATION/RF/rf.o
SIMULATION_OBJS += $(TOP_DIR)/SIMULATION/RF/adc.o
SIMULATION_OBJS += $(TOP_DIR)/SIMULATION/RF/dac.o

OBJ = $(PHY_OBJS) $(SIMULATION_OBJS) $(ETHERNET_TRANSPORT_OBJS) $(TOOLS_OBJS) $(SCHED_OBJS) $(STATS_OBJS)

all: physim


$(OBJ) : %.o : %.c
	$(CC) -c $(CFLAGS) $(EXTRA_CFLAGS) -DPHY_CONTEXT=1 -I$(TOP_DIR) $(L2_incl) -o $@ $<

$(L2_OBJS) : %.o : %.c
	$(CC) -c $(CFLAGS) $(EXTRA_CFLAGS) -DMAC_CONTEXT=1 -DPHY_CONTEXT=1 -I$(TOP_DIR) $(L2_incl) -o $@ $<

physim : $(OBJ) $(L2_OBJS) OCG.c OCG_get_opt.c OCG_detect_file.c OCG_parse_filename.c OCG_create_dir.c OCG_parse_XML.c OCG_save_XML.c OCG_config_mobi.c OCG_generate_report.c 
	$(CC)  OCG.c OCG_get_opt.c OCG_detect_file.c OCG_parse_filename.c OCG_create_dir.c OCG_parse_XML.c OCG_save_XML.c OCG_config_mobi.c OCG_generate_report.c -I$(TOP_DIR) $(L2_incl) -o physim $(CFLAGS) $(EXTRA_CFLAGS) $(OBJ) $(L2_OBJS) -lm -lforms -L/usr/local/lib -lforms -lX11 -L/usr/X11R6/lib -lXpm -lblas -lpthread

clean:
	rm -f physim
	rm -f $(OBJ)
	rm -f $(L2_OBJS)
	rm -f *.o
	rm -f *.exe*

cleanl1:
	rm -f physim
	rm -f $(OBJ)
	rm -f *.o
	rm -f *.exe
cleanl2:
	rm -f $(L2_OBJS)

	 
