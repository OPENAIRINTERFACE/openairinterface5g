# Include some shared directives
include $(OPENAIR_TARGETS)/Makerules

default: oaisim

include $(OPENAIR_TARGETS)/Makefile.common

TOP_DIR               = $(OPENAIR1_DIR)
OPENAIR1_TOP          = $(OPENAIR1_DIR)
OPENAIR2_TOP          = $(OPENAIR2_DIR)
OPENAIR3_TOP          = $(OPENAIR3_DIR)
OPENAIR3              = $(OPENAIR3_DIR)

ifeq ($(DEBUG),1)	
CFLAGS += -g -ggdb -DDEBUG_PHY -DDEBUG_MEAS
CFLAGS += -DRRC_MSG_PRINT
CFLAGS += -DPDCP_MSG_PRINT
CFLAGS += -DTRACE_RLC_UM_PDU
CFLAGS += -DTRACE_RLC_AM_PDU
else
CFLAGS += -DTRACE_RLC_UM_PDU
CFLAGS += -DTRACE_RLC_AM_PDU
endif 

ifeq ($(MSG_PRINT),1)
CFLAGS += -DRRC_MSG_PRINT
CFLAGS += -DPDCP_MSG_PRINT
endif


CFLAGS += -Wpointer-sign
CFLAGS += -DNB_ANTENNAS_RX=2 -DNB_ANTENNAS_TX=2 $(CPUFLAGS) -I/usr/include/X11 #-Wno-packed-bitfield-compat

ASN1_MSG_INC = $(OPENAIR2_DIR)/RRC/LITE/MESSAGES

CFLAGS += -DOPENAIR_LTE #-DOFDMA_ULSCH -DIFFT_FPGA -DIFFT_FPGA_UE
CFLAGS += -DMAC_CONTEXT=1 -DPHY_CONTEXT=1 #-DRLC_UM_TEST_TRAFFIC=1
#CFLAGS += -DLLR8
CFLAGS += -DPACKAGE_NAME='"oaisim"'

ifndef OPENAIR2
OPENAIR2=1
endif

ifdef DEBUG
DISABLE_XER_PRINT=0
MSG_PRINT=1
endif

ifeq ($(LOCALIZATION), 1)
CFLAGS += -DLOCALIZATION
endif

ifeq ($(LINUX_LIST), 1)
CFLAGS += -DLINUX_LIST
endif

ifdef TRAFFIC_TM5
CFLAGS += -DRLC_UM_TEST_TRAFFIC=1 #-DFULL_BUFFER=1
endif

# activate OCG and libxml only under linux
ifeq ($(linux),1)
CFLAGS += $(LIBXML2_CFLAGS) -L/usr/local/lib -I/usr/include/atlas -L/usr/X11R6/lib
OCG_FLAG =1
CFLAGS += -DLINUX
endif

ifdef SMBV
CFLAGS += -DSMBV
endif

ifeq ($(XFORMS), 1)
CFLAGS += -DXFORMS
endif

ifdef DB
# Check if libmysqlclient is installed and use it if found to store simulation data for postprocessing
ENABLE_DB = $(shell if [ `dpkg -l | grep libmysqlclient -c` = "0" ]; then  echo "0" ; else  echo "1" ; fi )
ifeq ($(ENABLE_DB), 1)
CFLAGS +=-I/usr/include/mysql -L/usr/lib/mysql -DENABLE_DB_STATS
DB_LDFLAGS = -lmysqlclient
endif
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

ifdef LS
CFLAGS += -DLARGE_SCALE
endif

# ln -sf EUTRA-RRC-Definitions-a20.asn EUTRA-RRC-Definitions.asn
# make cleanasn1
#make all Rel10=1

ifdef CBA
CFLAGS += -DRel10
CFLAGS += -DCBA
$(info "Rel10=1 CBA=1")

else ifdef Rel10
CFLAGS += -DRel10
$(info "Rel10=1")
else # default is rel 8
$(info "Rel8=1")
CFLAGS += -DRel8
endif

#CFLAGS += -DMAX_NUM_CCs=1

ifeq ($(RLC_STOP_ON_LOST_PDU), 1)
CFLAGS += -DRLC_STOP_ON_LOST_PDU
endif
ifeq ($(STOP_ON_IP_TRAFFIC_OVERLOAD), 1)
CFLAGS += -DSTOP_ON_IP_TRAFFIC_OVERLOAD
endif


ifeq ($(USE_3GPP_ADDR_AS_LINK_ADDR), 1)
CFLAGS += -DUSE_3GPP_ADDR_AS_LINK_ADDR
endif
endif



ifeq ($(LINK_ENB_PDCP_TO_IP_DRIVER), 1)
CFLAGS += -DPDCP_USE_NETLINK -DLINUX
NAS_FLAG=1
endif

ifeq ($(LINK_ENB_PDCP_TO_GTPV1U), 1)
CFLAGS += -DLINK_ENB_PDCP_TO_GTPV1U 
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

CFLAGS += -DENABLE_USE_CPU_EXECUTION_TIME
ifndef DISABLE_XER_PRINT
CFLAGS += -DXER_PRINT
endif

ifdef SECU
ifeq ($(NETTLE_FOUND), 0)
$(warning "Nettle library >= 2.5 is not installed on your system, nettle-dev lib needed, continuing with security disabled")
SECU=0
else
ifeq ($(OPENSSL_FOUND), 0)
$(warning "openssl library is not installed on your system, openssl lib needed, continuing with security disabled")
SECU=0
else
CFLAGS += -DENABLE_SECURITY
LIBS += $(OPENSSL_LIBS) $(NETTLE_LIBS)
endif
endif
endif

include $(OPENAIR1_DIR)/PHY/Makefile.inc
include $(OPENAIR1_DIR)/SCHED/Makefile.inc
include $(OPENAIR2_DIR)/RRC/LITE/MESSAGES/Makefile.inc
include $(OPENAIR2_DIR)/LAYER2/Makefile.inc
include $(OPENAIR1_DIR)/SIMULATION/ETH_TRANSPORT/Makefile.inc
include $(OPENAIR2_DIR)/RRC/NAS/Makefile.inc
include $(OPENAIR2_DIR)/UTIL/Makefile.inc
include $(OPENAIR3_DIR)/RAL-LTE/Makefile.inc
include $(OPENAIR2_DIR)/ENB_APP/Makefile.inc


INCLUDES += -I$(TOP_DIR)
INCLUDES += $(L2_incl)
INCLUDES += $(ENB_APP_incl)
INCLUDES += $(UTIL_incl)
INCLUDES += $(UTILS_incl)
INCLUDES += -I$(ASN1_MSG_INC)
INCLUDES += -I$(COMMON_UTILS_DIR)/collection

SIMULATION_OBJS  = $(TOP_DIR)/SIMULATION/TOOLS/gauss.o
SIMULATION_OBJS += $(TOP_DIR)/SIMULATION/TOOLS/random_channel.o
SIMULATION_OBJS += $(TOP_DIR)/SIMULATION/TOOLS/rangen_double.o
SIMULATION_OBJS += $(TOP_DIR)/SIMULATION/TOOLS/taus.o
SIMULATION_OBJS += $(TOP_DIR)/SIMULATION/TOOLS/multipath_channel.o
SIMULATION_OBJS += $(TOP_DIR)/SIMULATION/TOOLS/abstraction.o
SIMULATION_OBJS += $(TOP_DIR)/SIMULATION/RF/rf.o
SIMULATION_OBJS += $(TOP_DIR)/SIMULATION/RF/adc.o
SIMULATION_OBJS += $(TOP_DIR)/SIMULATION/RF/dac.o

ASN1_RRC_MSG_OBJS1=$(addprefix $(OPENAIR2_DIR)/RRC/LITE/MESSAGES/, $(ASN1_MSG_OBJS))

OAISIM_OBJS_COMMON  = $(OPENAIR_TARGETS)/SIMU/USER/channel_sim.o
OAISIM_OBJS_COMMON += $(OPENAIR_TARGETS)/SIMU/USER/init_lte.o
OAISIM_OBJS_COMMON += $(OPENAIR_TARGETS)/SIMU/USER/oaisim_config.o
OAISIM_OBJS_COMMON += $(OPENAIR_TARGETS)/SIMU/USER/sinr_sim.o
OAISIM_OBJS_COMMON += $(OPENAIR_TARGETS)/SIMU/USER/cor_SF_sim.o
OAISIM_OBJS_COMMON += $(OPENAIR_TARGETS)/SIMU/USER/oaisim_functions.o
OAISIM_OBJS_COMMON += $(OPENAIR_TARGETS)/SIMU/USER/event_handler.o

OAISIM_OBJS = $(OPENAIR_TARGETS)/SIMU/USER/oaisim.o

ifeq ($(XFORMS), 1)	
TOOLS_OBJS += $(OPENAIR1_DIR)/PHY/TOOLS/lte_phy_scope.o
endif

ifdef PAD
CFLAGS += -DPAD
endif

OAISIM_PAD_OBJS =  $(OPENAIR_TARGETS)/SIMU/USER/discrete_event_generator.o
OAISIM_PAD_OBJS += $(OPENAIR_TARGETS)/SIMU/USER/threadpool.o
OAISIM_PAD_OBJS += $(OPENAIR_TARGETS)/SIMU/USER/oaisim_pad.o
#deactivate the log offloading
ifndef LOG_THREAD
CFLAGS +=-DLOG_NO_THREAD
endif

ifdef PROC
INT_OBJS += $(OPENAIR_TARGETS)/SIMU/PROC/channel_sim_proc.o
INT_OBJS += $(OPENAIR_TARGETS)/SIMU/PROC/Process.o
CFLAGS += -DPROC
endif

CFLAGS += -DENABLE_VCD_FIFO
CFLAGS += -DENABLE_NEW_MULTICAST
ifdef NAS
CFLAGS += -DPDCP_USE_NETLINK_QUEUES
endif
# CFLAGS += -DENABLE_LOG_FIFO

# Check if libpgm is installed and use it if found instead of the unreliable
# multicast
ifeq ($(PGM_FOUND), 1)
CFLAGS		+= $(PGM_CFLAGS)
LIBS		+= $(PGM_LIBS)
endif

OBJ += $(PHY_OBJS) $(SIMULATION_OBJS) $(ETHERNET_TRANSPORT_OBJS) $(TOOLS_OBJS) $(SCHED_OBJS) $(STATS_OBJS) $(NAS_OBJS)
OBJ += $(INT_OBJS) $(UTIL_OBJ) $(UTILS_OBJS) $(OAISIM_OBJS_COMMON) $(ENB_APP_OBJS)
ifeq ($(OPENAIR2),1)
OBJ += $(L2_OBJS)
endif

LFDS_OBJ_DIR		 = $(subst $(OPENAIR_DIR),$(OBJS_DIR),$(LFDS_DIR))
LFDS_LIB		 = $(LFDS_OBJ_DIR)/bin/liblfds611.a
LIBS 			+= $(LFDS_LIB)
SHARED_DEPENDENCIES 	+= $(LFDS_LIB)

all: oaisim 

pad: oaisim_pad

userclean: cleanall oaisim

objsdir:
	@if [ ! -d $(OBJS_DIR) ]; then mkdir -p $(OBJS_DIR); fi

printvars:
	@echo rrc_cellular variable is $(rrc_cellular)
	@echo EXTRA_CFLAGS = $(EXTRA_CFLAGS)
	@echo OAI Config generator is $(OCG_FLAG)
	@echo OAI NETLINK FLAG is $(NAS_FLAG)
	@echo SSE3 proc is $(SSE3PROC)
	@echo L2 objs are $(L2_OBJS)
	@echo eNB_flag is $(eNB_flag)
	@echo UE_flag is $(UE_flag)
	@echo S1AP objs: $(S1AP_BUILT_OBJS)
	@echo X2AP objs: $(X2AP_BUILT_OBJS)
	@echo CFLAGS: $(CFLAGS)
	@echo Enable PGM: $(PGM_FOUND)

##UTIL_YAC_C = $(OPENAIR2_TOP)/UTIL/enb_parser.o
##$(UTIL_LEX_C) = $(subst $(OPENAIR_DIR), $(OBJS_DIR), $(OBJ) $(OAISIM_OBJS) $(ASN1_RRC_MSG_OBJS1))


OAISIM_COMPILED_OBJECTS = $(subst $(OPENAIR_DIR), $(OBJS_DIR), $(OBJ) $(OAISIM_OBJS) $(ASN1_RRC_MSG_OBJS1))

-include $(ITTI_MESSAGES_XML:.xml=.d)
-include $(OAISIM_PAD_OBJS:.o=.d)
-include $(OAISIM_COMPILED_OBJECTS:.o=.d)

ifdef ENABLE_ITTI
$(OAISIM_COMPILED_OBJECTS): $(ITTI_MESSAGES_H)

oaisim_pad.o: $(ITTI_MESSAGES_H)
endif

$(OAISIM_COMPILED_OBJECTS): | objsdir

.SECONDEXPANSION:
$(OAISIM_COMPILED_OBJECTS) $(OAISIM_PAD_OBJS): %.o : $$(subst $(OBJS_DIR), $(OPENAIR_DIR), $$*.c)
	@echo Compiling $<
	@if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi;
	@$(CC) -c $(CFLAGS) $(EXTRA_CFLAGS) -DPHY_CONTEXT=1 $(INCLUDES) -o $@ $<
	@$(CC) -MM $(CFLAGS) $(EXTRA_CFLAGS) -DPHY_CONTEXT=1 $(INCLUDES) $< > $*.d
	@mv -f $*.d $*.d.tmp
	@sed -e 's|.*:|$*.o:|' < $*.d.tmp > $*.d
	@sed -e 's/.*://' -e 's/\\$$//' < $*.d.tmp | fmt -1 | \
	sed -e 's/^ *//' -e 's/$$/:/' >> $*.d
	@rm -f $*.d.tmp

$(LFDS_LIB):
	@if [ ! -d $(LFDS_OBJ_DIR)/bin ]; then mkdir -p $(LFDS_OBJ_DIR)/bin; fi;
	@if [ ! -d $(LFDS_OBJ_DIR)/obj ]; then mkdir -p $(LFDS_OBJ_DIR)/obj; fi;
	$(MAKE) -C $(LFDS_DIR) -f makefile.linux OUTDIR=$(LFDS_OBJ_DIR)

oaisim_pad: $(OBJ) $(OAISIM_PAD_OBJS) $(ASN1_RRC_MSG_OBJS1)
	@echo "Linking oaisim_pad ..."
	@$(MPICC) $(INCLUDES) $(S1AP_Incl) $(X2AP_Incl) -o oaisim_pad $(CFLAGS) $(EXTRA_CFLAGS) $(OBJ) $(OAISIM_PAD_OBJS) $(ASN1_RRC_MSG_OBJS1) \
	-lpthread -llapack_atlas -lforms -lrt -lconfig $(LIBS) $(DB_LDFLAGS)

oaisim: $(OAISIM_COMPILED_OBJECTS) $(SHARED_DEPENDENCIES)
	@echo "Linking oaisim ..."
	@$(CC) $(INCLUDES) $(S1AP_Incl) $(X2AP_Incl) -o oaisim $(CFLAGS) $(EXTRA_CFLAGS) $(OAISIM_COMPILED_OBJECTS) \
	-lpthread -llapack_atlas -lforms -lrt -lconfig $(LIBS) $(DB_LDFLAGS)

ifeq ($(rrc_cellular_eNB),1)
	mv oaisim oaisim_eNB
endif
ifeq ($(rrc_cellular_UE),1)
	mv oaisim oaisim_UE
endif

oaisimCROWN: $(OBJ) $(ASN1_RRC_MSG_OBJS1) oaisimCROWN.c
	@echo "Compiling oaisimCROWN.c ..."
	@$(CC) oaisimCROWN.c $(INCLUDES) $(S1AP_Incl) $(X2AP_Incl) -o oaisimCROWN $(CFLAGS) $(EXTRA_CFLAGS) $(OBJ) $(ASN1_RRC_MSG_OBJS1) -lm -lblas -lpthread -llapack_atlas -lforms $(LIBXML2_LIBS) -lX11 -lXpm -lrt -lconfig 

oai_nw_drv:
	(cd $(OPENAIR2_DIR) && $(MAKE) oai_nw_drv.ko)
	(cp $(OPENAIR2_DIR)/NAS/DRIVER/LITE/oai_nw_drv.ko . )
oai_nw_drv_clean:
	(cd $(OPENAIR2_DIR)/NAS/DRIVER/LITE && $(MAKE) clean)
nasmesh_fix:
	(cd $(OPENAIR2_DIR)/NETWORK_DRIVER/MESH/RB_TOOL && $(MAKE) clean && $(MAKE))
	(cd $(OPENAIR2_DIR) && $(MAKE) clean && $(MAKE) nasmesh_netlink_address_fix.ko)
nasmesh_fix_uninstall:	
	(sudo rmmod nasmesh.ko)
nasmesh_fix_install:	
	(sudo insmod $(OPENAIR2_DIR)/NETWORK_DRIVER/MESH/nasmesh.ko)	
nasmesh_fix_reinstall:
	(sudo rmmod nasmesh.ko)	
	(sudo insmod $(OPENAIR2_DIR)/NETWORK_DRIVER/MESH/nasmesh.ko)
nasmesh_nl:
	(cd $(OPENAIR2_DIR)/NETWORK_RIVER/MESH/RB_TOOL && $(MAKE))
	(cd $(OPENAIR2_DIR) && $(MAKE) nasmesh_netlink.ko)
	(sudo insmod $(OPENAIR2_DIR)/NETWORK_DRIVER/MESH/nasmesh.ko)
rb_tool:
	(cd $(OPENAIR2_DIR)/NETWORK_DRIVER/MESH/RB_TOOL && $(MAKE))

nasmesh_install:
	(sudo rmmod nasmesh)
	(sudo insmod $(OPENAIR2_DIR)/NAS/DRIVER/MESH/nasmesh.ko)
nasmesh_clean:
	(cd $(OPENAIR2_DIR) && $(MAKE) clean)
nas_sim_mt_cellular:
	( cd $(OPENAIR2_DIR)/NAS/SIMU_CELLULAR && make nasmt_sim CELLULAR=1 NO_RRM=1)

nas_sim_rg_cellular:
	( cd $(OPENAIR2_DIR)/NAS/SIMU_CELLULAR && make nasrg_sim MASTER=1 CELLULAR=1 NO_RRM=1)

rrm_std_cellular:
	( cd $(OPENAIR2_DIR)/NAS/SIMU_CELLULAR && make cell_rrm CELL_RRM=1 CELLULAR=1 NO_RRM=1)

cleanall: common-cleanall clean cleanasn1

cleanalmostall: clean
	rm -f $(ASN1_RRC_MSG_OBJS1)

clean: common-clean
	@$(RM_F_V) oaisim
	@$(RM_F_V) oaisim_pad
# 	@$(RM_F_V) $(ITTI_MESSAGES_H) $(ITTI_MESSAGES_XML) $(ITTI_MESSAGES_XML:.xml=.d)
	@$(RM_F_V) $(OAISIM_PAD_OBJS)
	@$(RM_F_V) $(OAISIM_PAD_OBJS:.o=.d)
	@$(RM_F_V) $(OAISIM_COMPILED_OBJECTS)
	@$(RM_F_V) $(OAISIM_COMPILED_OBJECTS:.o=.d)
	@$(RM_F_V) *.exe*
	@$(RM_F_V) $(ASN1_MSG_INC)/asn1_msg.o

cleanl1:
	@$(RM_F_V) LOG_THREAD oaisim
	@$(RM_F_V) $(PHY_OBJS) $(SCHED_OBJS)
	@$(RM_F_V) *.o
	@$(RM_F_V) *.exe

cleanl2:
	@$(RM_F_V) $(L2_OBJS)
	@$(RM_F_V) $(ASN1_MSG_INC)/asn1_msg.o

cleanasn1:
	@$(RM_F_V) $(ASN1_RRC_MSG_OBJS1)
	$(shell cd $(OPENAIR2_DIR)/RRC/LITE/MESSAGES ; rm -f $(ASN_MODULE_SOURCES) $(ASN_MODULE_HEADERS) *.o *.d)
	@$(RM_F_V) $(OPENAIR2_DIR)/RRC/LITE/MESSAGES/Makefile.am.sample
	@$(RM_F_V) $(OPENAIR2_DIR)/RRC/LITE/MESSAGES/Makefile.inc.generated
	@$(RM_F_V) $(OPENAIR2_DIR)/RRC/LITE/MESSAGES/asn1c/ASN1_files/.lock-rel*

cleancell:
	@$(RM_F_V) $(OPENAIR2_DIR)/RRC/CELLULAR/*.o
	@$(RM_F_V) $(OPENAIR2_DIR)/RRC/CELLULAR/*.d
	@$(RM_F_V) $(OPENAIR2_DIR)/RRC/L2_INTERFACE/*.o
	@$(RM_F_V) $(OPENAIR2_DIR)/RRC/L2_INTERFACE/*.d
	@$(RM_F_V) $(OPENAIR2_DIR)/NAS/SIMU_CELLULAR/*.o
	@$(RM_F_V) $(OPENAIR2_DIR)/NAS/SIMU_CELLULAR/*.d

cleanlfds:
	$(MAKE) -C $(LFDS_DIR) -f makefile.linux clean

cppcheck:
	@echo "cppcheck oaisim ..."
	cppcheck $(INCLUDES) ${OBJ:.o=.c} ${OAISIM_OBJS,.o=.c} ${ASN1_RRC_MSG_OBJS1,.o=.c}
print:
	@echo "OBJ "				$(OBJ)
	@echo "OAISIM_OBJS "		$(OAISIM_OBJS)
	@echo "OAISIM_PAD_OBJS "	$(OAISIM_PAD_OBJS)
	@echo "OCG_FLAG are "		$(OCG_FLAG)
	@echo "CFLAGS are "			$(CFLAGS)
	@echo "EXTRA_CFLAGS are"	$(EXTRA_CFLAGS)
	@echo "COMMON_CFLAGS are"	$(COMMON_CFLAGS)
	@echo "L2 includes are"		$(L2_incl)
	@echo "INCLUDES are"		$(INCLUDES)
	@echo "S1AP Includes are"	$(S1AP_Incl)
	@echo "X2AP Includes are"	$(X2AP_Incl)
	@echo "TOP_DIR Include is"	I$(TOP_DIR)
	@echo "UTIL includes are"	$(UTIL_incl)
	@echo "UTILS includes are"	$(UTILS_incl)
	@echo "IS_REL8 is "			$(IS_REL8)
	@echo "IS_REL10 is "		$(IS_REL10)
	@echo "ENABLE_DB is "		$(ENABLE_DB)
	@echo "Libs are"			$(LIBS)
	@echo "NAS"			$(NAS)	

showcflags:
	@echo oaisim cflags: $(CFLAGS)
	@echo oaisim includes: $(INCLUDES)
	@if [ -d $(S1AP_DIR) ]; then $(MAKE) -C $(S1AP_DIR) -f Makefile.eNB showcflags; fi
	@if [ -d $(X2AP_DIR) ]; then $(MAKE) -C $(X2AP_DIR) -f Makefile.inc showcflags; fi

otg_all: otg_latency otg_gp

# need to check if the file exists before calling otgplot
otg_latency:
	export TITLE="Application OWD(ms)"
	cp /tmp/otg_latency.dat /tmp/onewaydelay.dat
	$(OPENAIR2_DIR)/UTIL/OTG/OTGplot "/tmp/otg_latency.dat" "[$(COL1):$(COL2)]"
otg_latency_bg:
	export TITLE="Background OWD(ms)"
	$(OPENAIR2_DIR)/UTIL/OTG/OTGplot "/tmp/otg_latency_bg.dat" "[$(COL1):$(COL2)]"
otg_gp:
	export TITLE="Application Goodput(Kb/s)"
	cp /tmp/otg_goodput.dat /tmp/goodput.dat
	$(OPENAIR2_DIR)/UTIL/OTG/OTGplot "/tmp/goodput.dat" "[$(COL1):$(COL2)]"
otg_gp_bg:
	export TITLE="Background Goodput(Kb/s)"
	$(OPENAIR2_DIR)/UTIL/OTG/OTGplot "/tmp/otg_goodput_bg.dat" "[$(COL1):$(COL2)]"
otg_jitter:
	export TITLE="Aplication Jitter(ms)"
	$(OPENAIR2_DIR)/UTIL/OTG/OTGplot "/tmp/otg_jitter.dat" "[$(COL1):$(COL2)]"

pexpect = $(shell if [ `dpkg -l | grep pexpect -c` = "0" ]; then  echo "0" ; else  echo "1" ; fi )

pre-ci:
	@echo "Pre-commit check"

ifeq 	($(pexpect), 1)
ifndef	PW
	@echo "Usage: make pre-ci PW=mypassword"
	@echo "Password is used to access the local host through ssh."
	python $(OPENAIR_TARGETS)/TEST/OAI/test01.py
else
	python $(OPENAIR_TARGETS)/TEST/OAI/test01.py -w $(PW)
endif
else
	@echo "Python pexpect package is missing, please install"
endif
