# Include some shared directives
include $(OPENAIR_TARGETS)/Makerules

default: lte-softmodem
all: lte-softmodem rrh

include $(OPENAIR_TARGETS)/Makefile.common

KERNEL_VERSION:=$(shell echo `uname -r | cut -d. -f-2`)
KERNEL_TYPE:=$(shell echo `uname -r | cut -d. -f-3 | cut -d- -f3`)
DEADLINE_SCHEDULER_KERNEL:=$(shell if [ $(KERNEL_TYPE) = "lowlatency" ]; then echo "1" ; else echo "0" ; fi)

include $(OPENAIR2_DIR)/RRC/LITE/MESSAGES/Makefile.inc

ifdef EXMIMO
#ifdef DEADLINE_SCHEDULER # this ifdef is to be removed after the debugging
ifeq ($(DEADLINE_SCHEDULER_KERNEL),1)
CFLAGS+=-DDEADLINE_SCHEDULER
endif 
#endif 
endif

ifndef USRP
USRP=0
endif

CFLAGS += -DDRIVER2013 -I$(OPENAIR_TARGETS)/ARCH/COMMON  -I.

ifndef OPENAIR2
OPENAIR2=1
endif

ifdef DEBUG
DISABLE_XER_PRINT=0
MSG_PRINT=1
endif

ifdef Rel10
CFLAGS += -DRel10
endif

ifeq ($(EXMIMO),1)
  CFLAGS += -I$(OPENAIR_TARGETS)/ARCH/EXMIMO/USERSPACE/LIB/ -I$(OPENAIR_TARGETS)/ARCH/EXMIMO/DEFS -DENABLE_VCD_FIFO
endif

ifeq ($(ETHERNET),1)
  CFLAGS += -I$(OPENAIR_TARGETS)/ARCH/ETHERNET/USERSPACE/LIB/ -DETHERNET
endif

ifeq ($(DEBUG),1)	
CFLAGS += -g -ggdb
#CFLAGS += -DRRC_MSG_PRINT
#CFLAGS += -DPDCP_MSG_PRINT
else 
CFLAGS += -O2 
endif

ifdef ($(MSG_PRINT),1)
CFLAGS += -DRRC_MSG_PRINT
CFLAGS += -DPDCP_MSG_PRINT
endif

SRC = synctest.c condtest.c 

ifndef RTAI
RTAI=1
endif

ifeq ($(LOCALIZATION), 1)
CFLAGS += -DLOCALIZATION
endif

ifeq ($(LINUX_LIST), 1)
CFLAGS += -DLINUX_LIST
endif

ifeq ($(RTAI),1)
CFLAGS += -DENABLE_RTAI_CLOCK
CFLAGS += -DCONFIG_RTAI_LXRT_INLINE  #remend the RTAI warning
RTAI_OBJ = sched_dlsch.o sched_rx_pdsch.o rt_wrapper.o 
else #RTAI
CFLAGS += -DENABLE_USE_CPU_EXECUTION_TIME
OBJ += sched_dlsch.o sched_rx_pdsch.o rt_wrapper.o
endif

OBJ += $(OPENAIR1_DIR)/SIMULATION/TOOLS/taus.o $(OPENAIR_TARGETS)/SIMU/USER/init_lte.o 

ifeq ($(EXMIMO),1)
OBJ += $(OPENAIR_TARGETS)/ARCH/EXMIMO/USERSPACE/LIB/openair0_lib.o $(OPENAIR_TARGETS)/ARCH/EXMIMO/USERSPACE/LIB/gain_control.o
CFLAGS += -DDRIVER2013 -I$(OPENAIR_TARGETS)/ARCH/EXMIMO/USERSPACE/LIB/ -I$(OPENAIR_TARGETS)/ARCH/EXMIMO/DEFS 
endif

CFLAGS += -DENABLE_VCD_FIFO


TOP_DIR = $(OPENAIR1_DIR)
include $(OPENAIR1_DIR)/PHY/Makefile.inc
include $(OPENAIR1_DIR)/SCHED/Makefile.inc

OBJ += $(PHY_OBJS)
ifeq ($(RTAI),1)
RTAI_OBJ += $(SCHED_OBJS)
else
OBJ += $(SCHED_OBJS)
endif

OPENAIR2_TOP = $(OPENAIR2_DIR)
include $(OPENAIR2_DIR)/LAYER2/Makefile.inc
include $(OPENAIR2_DIR)/UTIL/Makefile.inc
include $(OPENAIR2_DIR)/RRC/NAS/Makefile.inc
include $(OPENAIR2_DIR)/ENB_APP/Makefile.inc

ifeq ($(USRP),1)

include $(OPENAIR_TARGETS)/ARCH/USRP/USERSPACE/LIB/Makefile.inc
#CFLAGS += -I/opt/include/uhd -L/opt/lib -luhd -lpthread -lstdc++
CFLAGS += -I/usr/include/uhd -L/usr/lib -luhd -lpthread -lstdc++
CFLAGS += -DOAI_USRP
#LDFLAGS += -L/opt/lib -luhd -lpthread -lstdc++ 
LDFLAGS += -L/usr/lib/ -luhd -lpthread -lstdc++
endif

ifeq ($(ETHERNET),1)
include $(OPENAIR_TARGETS)/ARCH/ETHERNET/USERSPACE/LIB/Makefile.inc
LDFLAGS += -lpthread
endif

OBJ +=  $(ENB_APP_OBJS)

ifeq ($(RTAI),1)
CFLAGS += -I/usr/realtime/include -I/usr/realtime/include/asm
LOG_OBJS=
RTAI_OBJ+=$(LOG_DIR)/vcd_signal_dumper.o
RTAI_OBJ+=$(LOG_DIR)/log.o
endif


OBJ += $(LOG_OBJS) 

ifeq ($(OPENAIR2),1)
ASN1_MSG_OBJS1=$(addprefix $(OPENAIR2_DIR)/RRC/LITE/MESSAGES/, $(ASN1_MSG_OBJS))
OBJ += $(L2_OBJS) $(LIST_OBJ) $(TIMER_OBJ) $(MEM_OBJ) $(OTG_OBJS) $(MATH_OBJS)  $(OSA_OBJS) $(OPT_OBJS) 
OBJ += $(OPENAIR1_DIR)/SIMULATION/ETH_TRANSPORT/netlink_init.o
#OBJ += $(PDCP_DIR)/pdcp_thread.o
CFLAGS += -DOPENAIR2 -DNO_RRM -DPUCCH -DMAC_CONTEXT=1
endif

#ifdef ENABLE_ITTI
RTAI_OBJ += $(UTILS_OBJS)
#else
#OBJ += $(UTILS_OBJS)
#endif

ifdef SPECTRA
CFLAGS += -DSPECTRA
endif

#ifdef ENABLE_ITTI
CFLAGS += -DEXMIMO_IOT
#endif

CFLAGS += $(L2_incl) $(ENB_APP_incl) $(UTIL_incl) $(UTILS_incl)

CFLAGS += -I$(OPENAIR1_DIR) -I$(OPENAIR2_DIR)/RRC/LITE/MESSAGES	#-I$(OPENAIR3_DIR)/MESH -I$(OPENAIR3_DIR)/MESH/RRM

CFLAGS += -DNEW_FFT -DLOG_NO_THREAD

ifeq ($(XFORMS),1)
CFLAGS += -DXFORMS -I/usr/include/X11
LDFLAGS += -lforms
OBJ += $(OPENAIR1_DIR)/PHY/TOOLS/lte_phy_scope.o
OBJ += stats.o
endif

OBJ_SYNC = $(OPENAIR_TARGETS)/ARCH/EXMIMO/USERSPACE/LIB/openair0_lib.o rt_wrapper.o $(OPENAIR2_DIR)/UTIL/LOG/log.o $(OPENAIR2_DIR)/UTIL/LOG/vcd_signal_dumper.o 

ifdef SMBV
CFLAGS += -DSMBV
endif

CFLAGS += -DPHYSIM -DUSER_MODE -DPC_TARGET -DPC_DSP -DNB_ANTENNAS_RX=2 -DNB_ANTENNAS_TXRX=2 -DNB_ANTENNAS_TX=2 -DPHY_CONTEXT=1 
CFLAGS += -DOPENAIR_LTE -DENABLE_FXP -DOPENAIR1 #-DDLSCH_THREAD #-DULSCH_THREAD


ifeq ($(EXMIMO),1)
CFLAGS += -DEXMIMO
#CFLAGS += -DEXMIMO -DTIMING_ADVANCE_HW=138 #this is for ExpressMIMO 1
#CFLAGS += -DEXMIMO -DTIMING_ADVANCE_HW=45 #this is for ExpressMIMO 2
endif

ifeq ($(HARD_RT),1)
CFLAGS += -DHARD_RT
endif

ifeq ($(EMOS),1)
CFLAGS += -D_FILE_OFFSET_BITS=64 #-DEMOS -DEMOS_CHANNEL
LDFLAGS += -lgps
endif


ifeq ($(LINK_ENB_PDCP_TO_GTPV1U), 1)
CFLAGS += -DLINK_ENB_PDCP_TO_GTPV1U 
endif

ifeq ($(LINK_ENB_PDCP_TO_IP_DRIVER), 1)
CFLAGS += -DPDCP_USE_NETLINK -DLINUX
OBJ += $(NAS_OBJS)
NAS_FLAG=1
endif

LDFLAGS += -lpthread -lm -lforms -lconfig -lrt
ifeq ($(RTAI),1)
RTAI_CFLAGS += $(shell rtai-config --lxrt-cflags) -DRTAI
LDFLAGS += $(shell rtai-config --lxrt-ldflags)
endif

#ifeq ($(USRP),1)
#CFLAGS += -I/opt/uhd/include -L/opt/uhd/lib -luhd -lpthread -lstdc++
#CFLAGS += -DOAI_USRP
#endif
#CFLAGS += -I/usr/include/c++/4.6 -I/usr/include/c++/4.6/x86_64-linux-gnu -I/usr/include/rtai/

LFDS_OBJ_DIR		 = $(subst $(OPENAIR_DIR),$(OBJS_DIR),$(LFDS_DIR))
LFDS_LIB		 = $(LFDS_OBJ_DIR)/bin/liblfds611.a
LIBS 			+= $(LFDS_LIB)
SHARED_DEPENDENCIES 	+= $(LFDS_LIB)

-include $(OBJ:.o=.d)
-include $(ASN1_MSG_OBJS1:.o=.d)
-include $(RTAI_OBJ:.o=.d)
-include lte-softmodem.d
-include lte-ue.d
-include rrh.d

$(LFDS_LIB):
	@if [ ! -d $(LFDS_OBJ_DIR)/bin ]; then mkdir -p $(LFDS_OBJ_DIR)/bin; fi;
	@if [ ! -d $(LFDS_OBJ_DIR)/obj ]; then mkdir -p $(LFDS_OBJ_DIR)/obj; fi;
	$(MAKE) -C $(LFDS_DIR) -f makefile.linux OUTDIR=$(LFDS_OBJ_DIR)

ifeq ($(USRP),1)
$(USRP_OBJ):$(USRP_FILE_OBJ)
	@echo Compiling $<
	@$(CXX) -c  -g -ggdb $(USRP_CFLAGS) $(USRP_FILE_OBJ) -o $(USRP_OBJ)
endif

ifeq ($(ETHERNET),1)
$(ETHERNET_OBJ):$(ETHERNET_FILE_OBJ)
	@echo Compiling $<
	@$(CC) -c -g -ggdb $(ETHERNET_CFLAGS) $(ETHERNET_FILE_OBJ) -o $(ETHERNET_OBJ)
endif

ifeq ($(RTAI),1)
$(RTAI_OBJ) lte-softmodem.o lte-ue.o: %.o : %.c
else
$(RTAI_OBJ): %.o : %.c
endif
	@echo Compiling $< ...
	@$(CC) -c $(CFLAGS) $(EXTRA_CFLAGS) $(RTAI_CFLAGS) -o $@ $<
	@$(CC) -MM $(CFLAGS) $(EXTRA_CFLAGS) $(RTAI_CFLAGS) $< > $*.d
	@mv -f $*.d $*.d.tmp
	@sed -e 's|.*:|$*.o:|' < $*.d.tmp > $*.d
	@sed -e 's/.*://' -e 's/\\$$//' < $*.d.tmp | fmt -1 | \
	sed -e 's/^ *//' -e 's/$$/:/' >> $*.d
	@rm -f $*.d.tmp

ifdef ENABLE_ITTI
$(OBJ) $(RTAI_OBJ): $(ITTI_MESSAGES_H)
endif

ifeq ($(RTAI),1)
$(OBJ) $(ASN1_MSG_OBJS1): %.o : %.c
else
$(OBJ) $(ASN1_MSG_OBJS1) lte-softmodem.o lte-ue.o: %.o : %.c
endif

rrh.o: %.o : %.c

	@echo Compiling $< ...
	@$(CC) -c $(CFLAGS) $(EXTRA_CFLAGS) -o $@ $<
	@$(CC) -MM $(CFLAGS) $(EXTRA_CFLAGS) $< > $*.d
	@mv -f $*.d $*.d.tmp
	@sed -e 's|.*:|$*.o:|' < $*.d.tmp > $*.d
	@sed -e 's/.*://' -e 's/\\$$//' < $*.d.tmp | fmt -1 | \
	sed -e 's/^ *//' -e 's/$$/:/' >> $*.d
	@rm -f $*.d.tmp

OBJ_EMOS = $(OPENAIR_TARGETS)/ARCH/EXMIMO/USERSPACE/LIB/openair0_lib.o $(OPENAIR_TARGETS)/ARCH/EXMIMO/USERSPACE/LIB/gain_control.o rt_wrapper.o $(OPENAIR2_DIR)/UTIL/LOG/log.o $(OPENAIR2_DIR)/UTIL/LOG/vcd_signal_dumper.o $(OPENAIR1_DIR)/PHY/TOOLS/signal_energy.o $(OPENAIR1_DIR)/PHY/TOOLS/dB_routines.o
ifeq ($(XFORMS),1)
OBJ_EMOS+=lte_scope.o
endif

condtest: condtest.c 
	$(CC) $(CFLAGS) $(LDFLAGS) condtest.c -o condtest

synctest: $(OBJ_SYNC) $(SHARED_DEPENDENCIES) synctest.c
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) $(RTAI_CFLAGS) $(OBJ_SYNC) -o synctest synctest.c $(LDFLAGS) $(LIBS)

sleeptest: rt_wrapper.o sleeptest.c
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) $(RTAI_CFLAGS) rt_wrapper.o -o sleeptest sleeptest.c $(LDFLAGS) 

lte-softmodem: $(OBJ) $(USRP_OBJ) $(ETHERNET_OBJ) $(ASN1_MSG_OBJS1) $(RTAI_OBJ) lte-ue.o lte-softmodem.o $(SHARED_DEPENDENCIES)
	@echo Linking $@
	@$(CC) $(CFLAGS) $(EXTRA_CFLAGS) $(OBJ) $(USRP_OBJ) $(ETHERNET_OBJ) $(RTAI_OBJ) $(ASN1_MSG_OBJS1) lte-ue.o lte-softmodem.o -o lte-softmodem $(LDFLAGS) $(LIBS)

rrh: rrh.o
	@$(CC) $(CFLAGS) $(EXTRA_CFLAGS) rrh.o -o rrh -lpthread -lrt

emos-raw: $(SHARED_DEPENDENCIES) $(OBJ_EMOS) emos-raw.c
	@$(CC) $(CFLAGS) $(EXTRA_CFLAGS) $(RTAI_CFLAGS) $(OBJ_EMOS) -o emos-raw emos-raw.c $(LDFLAGS) $(LIBS)


synctest_eNB: synctest
	cp synctest synctest_eNB

synctest_UE: synctest
	cp synctest synctest_UE

drivers:
#	cd $(OPENAIR2_DIR) && make clean && make nasmesh_netlink.ko
#	cd $(OPENAIR2_DIR)/NAS/DRIVER/MESH/RB_TOOL && make clean && make
	cd $(OPENAIR_TARGETS)/ARCH/EXMIMO/DRIVER/eurecom && make clean && make 
	cd $(OPENAIR_TARGETS)/ARCH/EXMIMO/USERSPACE/OAI_FW_INIT && make clean && make 
	cp $(OPENAIR_TARGETS)/ARCH/EXMIMO/USERSPACE/OAI_FW_INIT/updatefw $(OPENAIR_TARGETS)/bin/.

run: condtest
	rtai-load condtest --verbose

run_eNB_test: 
	rtai-load eNB_test --verbose

run_eNB: 
	rtai-load eNB --verbose

run_UE: 
	rtai-load UE --verbose

run_UE0:
	rtai-load UE0 --verbose

run_UE0_smbv:
	rtai-load UE0_smbv --verbose

run_UE850:
	rtai-load UE850 --verbose

run_eNB850:
	rtai-load eNB850 --verbose

run_UE0prach:
	rtai-load UE0prach --verbose

run_UE1prach:
	rtai-load UE1prach --verbose

run_UE2prach:
	rtai-load UE2prach --verbose

run_UE0noL2:
	rtai-load UE0noL2 -- verbose

run_UE1noL2: 
	rtai-load UE1noL2 --verbose

run_UE2noL2: 
	rtai-load UE2noL2 --verbose

run_UE0calib:
	rtai-load UE0calib --verbose

run_UE0calibmed:
	rtai-load UE0calibmed --verbose

run_UE0calibbyp:
	rtai-load UE0calibbyp --verbose

run_UE1: 
	rtai-load UE1 --verbose

run_UE2: 
	rtai-load UE2 --verbose

run_eNB0:
	rtai-load eNB0 --verbose

run_eNB1:
	rtai-load eNB1 --verbose

run_eNB2:
	rtai-load eNB2 --verbose

clean: cleanmodem common-clean

cleanmodem:
	@$(RM_F_V) $(OBJ) $(RTAI_OBJ) $(OBJ_EMOS) $(OBJ_SYNC) $(USRP_OBJ) $(ETHERNET_OBJ)
	@$(RM_F_V) $(OBJ:.o=.d) $(RTAI_OBJ:.o=.d) $(OBJ_EMOS:.o=.d) $(OBJ_SYNC:.o=.d)
	@$(RM_F_V) $(OPENAIR2_DIR)/RRC/LITE/MESSAGES/asn1_msg.o $(OPENAIR2_DIR)/RRC/LITE/MESSAGES/asn1_msg.d
	@$(RM_F_V) lte-ue.o lte-ue.d rrh.o rrh.d lte-softmodem.o lte-softmodem.d
	@$(RM_F_V) rrh.o lte-ue.o lte-softmodem.o

cleanasn1:
	rm -f $(ASN1_MSG_OBJS1)
	$(shell cd $(OPENAIR2_DIR)/RRC/LITE/MESSAGES ; rm -f $(ASN_MODULE_SOURCES) $(ASN_MODULE_HEADERS) *.o *.d )
	rm -f $(OPENAIR2_DIR)/RRC/LITE/MESSAGES/Makefile.am.sample
	rm -f $(OPENAIR2_DIR)/RRC/LITE/MESSAGES/Makefile.inc.generated
	rm -f $(OPENAIR2_DIR)/RRC/LITE/MESSAGES/asn1c/ASN1_files/.lock-rel*

cleancell:
	rm -f $(OPENAIR2_DIR)/RRC/CELLULAR/*.o
	rm -f $(OPENAIR2_DIR)/RRC/CELLULAR/*.d
	rm -f $(OPENAIR2_DIR)/RRC/L2_INTERFACE/*.o
	rm -f $(OPENAIR2_DIR)/RRC/L2_INTERFACE/*.d
	rm -f $(OPENAIR2_DIR)/NAS/SIMU_CELLULAR/*.o
	rm -f $(OPENAIR2_DIR)/NAS/SIMU_CELLULAR/*.d

cleanalmostall: cleanmodem 
	rm -f $(ASN1_MSG_OBJS1)
	rm -rf condtest synctest lte-softmodem rrh
	rm -rf synctest_eNB synctest_UE

cleanall: common-cleanall clean cleanasn1
	rm -rf condtest synctest lte-softmodem rrh
	rm -rf synctest_eNB synctest_UE

show:
	@echo $(CFLAGS)
	@echo $(EXTRA_CFLAGS)
	@echo $(OBJ) $(RTAI_OBJ) 
	@echo $(USRP_OBJ)
	@echo $(ETHERNET_OBJ)
	@echo $(ETHERNET_FILE_OBJ)
	@echo IS_REL8 is $(IS_REL8)
	@echo IS_REL10 is $(IS_REL10)
	@echo openssl $(OPENSSL_FOUND) 
	@echo nettle $(NETTLE_FOUND)
	@echo lowlatency kernel: $(DEADLINE_SCHEDULER_KERNEL)

beautiful:
	astyle --style=gnu -s2 $(SRC)

fifos:
	@for i in `seq 0 64`;\
	do \
	have_rtfX=`ls /dev/ |grep -c rtf$$i`;\
	if [ "$$have_rtfX" -eq 0 ] ;then \
	mknod -m 666 /dev/rtf$$i c 150 $$i; \
	fi;\
	done
