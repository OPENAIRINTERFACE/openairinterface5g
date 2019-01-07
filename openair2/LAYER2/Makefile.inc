ifeq ($(OPENAIR2),1)
EXTRA_CFLAGS += -DOPENAIR2
endif
EXTRA_CFLAGS += -DASSERT
EXTRA_CFLAGS += -DNO_RRM

COMMON_DIR = $(OPENAIR2_TOP)/COMMON
L2_INTERFACE_DIR = $(OPENAIR2_TOP)/RRC/L2_INTERFACE
RLC_UM_DIR = $(OPENAIR2_TOP)/LAYER2/RLC/UM_v9.3.0
RLC_AM_DIR = $(OPENAIR2_TOP)/LAYER2/RLC/AM_v9.3.0
RLC_TM_DIR = $(OPENAIR2_TOP)/LAYER2/RLC/TM_v9.3.0
RLC_DIR = $(OPENAIR2_TOP)/LAYER2/RLC
PDCP_DIR = $(OPENAIR2_TOP)/LAYER2/PDCP_v10.1.0
PHY_INTERFACE_DIR = $(OPENAIR2_TOP)/PHY_INTERFACE
RRC_DIR = $(OPENAIR2_TOP)/RRC/LITE
L2_INTERFACE_DIR = $(OPENAIR2_TOP)/RRC/L2_INTERFACE
MAC_DIR = $(OPENAIR2_TOP)/LAYER2/MAC
RRC_CELL_DIR = $(OPENAIR2_TOP)/RRC/CELLULAR

SOURCES_L2 +=  $(OPENAIR2_TOP)/LAYER2/openair2_proc.c 

SOURCES_L2 +=  $(PDCP_DIR)/pdcp.c
SOURCES_L2 +=  $(PDCP_DIR)/pdcp_fifo.c
SOURCES_L2 +=  $(PDCP_DIR)/pdcp_sequence_manager.c
SOURCES_L2 +=  $(PDCP_DIR)/pdcp_primitives.c
SOURCES_L2 +=  $(PDCP_DIR)/pdcp_util.c
SOURCES_L2 +=  $(PDCP_DIR)/pdcp_security.c
SOURCES_L2 +=  $(PDCP_DIR)/pdcp_netlink.c

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


SOURCES_L2 +=  $(RLC_TM_DIR)/rlc_tm.c
SOURCES_L2 +=  $(RLC_TM_DIR)/rlc_tm_init.c

SOURCES_L2 +=  $(RLC_UM_DIR)/rlc_um.c
SOURCES_L2 +=  $(RLC_UM_DIR)/rlc_um_fsm.c
SOURCES_L2 +=  $(RLC_UM_DIR)/rlc_um_control_primitives.c
SOURCES_L2 +=  $(RLC_UM_DIR)/rlc_um_segment.c
SOURCES_L2 +=  $(RLC_UM_DIR)/rlc_um_reassembly.c
SOURCES_L2 +=  $(RLC_UM_DIR)/rlc_um_receiver.c
SOURCES_L2 +=  $(RLC_UM_DIR)/rlc_um_dar.c

SOURCES_L2 +=  $(RLC_DIR)/rlc_mac.c
SOURCES_L2 +=  $(RLC_DIR)/rlc.c
SOURCES_L2 +=  $(RLC_DIR)/rlc_rrc.c
SOURCES_L2 +=  $(RLC_DIR)/rlc_mpls.c

RRC_OBJS =  $(RRC_DIR)/rrc_UE.o
ifeq ($(ENABLE_RAL),1)
RRC_OBJS +=  $(RRC_DIR)/rrc_UE_ral.o
RRC_OBJS +=  $(RRC_DIR)/rrc_eNB_ral.o
endif

RRC_OBJS += $(RRC_DIR)/rrc_eNB.o
RRC_OBJS += $(RRC_DIR)/rrc_eNB_UE_context.o
RRC_OBJS += $(RRC_DIR)/rrc_eNB_S1AP.o
ifdef USE_MME
RRC_OBJS += $(RRC_DIR)/rrc_eNB_GTPV1U.o
endif
RRC_OBJS += $(RRC_DIR)/rrc_common.o
RRC_OBJS += $(RRC_DIR)/L2_interface.o

MAC_OBJS =  $(MAC_DIR)/lte_transport_init.o
MAC_OBJS += $(MAC_DIR)/main.o
MAC_OBJS += $(MAC_DIR)/ue_procedures.o
MAC_OBJS += $(MAC_DIR)/ra_procedures.o
MAC_OBJS += $(MAC_DIR)/l1_helpers.o
MAC_OBJS += $(MAC_DIR)/rar_tools.o
MAC_OBJS += $(MAC_DIR)/eNB_scheduler.o
MAC_OBJS += $(MAC_DIR)/eNB_scheduler_dlsch.o
MAC_OBJS += $(MAC_DIR)/eNB_scheduler_ulsch.o
MAC_OBJS += $(MAC_DIR)/eNB_scheduler_mch.o
MAC_OBJS += $(MAC_DIR)/eNB_scheduler_bch.o
MAC_OBJS += $(MAC_DIR)/eNB_scheduler_primitives.o
MAC_OBJS += $(MAC_DIR)/eNB_scheduler_RA.o
MAC_OBJS += $(MAC_DIR)/pre_processor.o
MAC_OBJS += $(MAC_DIR)/config.o

L2_INTERFACE_OBJS = $(L2_INTERFACE_DIR)/openair_rrc_L2_interface.o

L2_OBJS=$(addsuffix .o,$(basename $(SOURCES_L2))) $(PHY_INTERFACE_OBJS) $(MAC_OBJS) $(L2_INTERFACE_OBJS)

L2_incl =  -I$(OPENAIR2_TOP)
L2_incl =  -I$(OPENAIR_DIR)/common/utils/msc
L2_incl += -I$(COMMON_DIR)
L2_incl += -I$(RLC_DIR)
L2_incl += -I$(RLC_AM_DIR)
L2_incl += -I$(RLC_UM_DIR)
L2_incl += -I$(RLC_TM_DIR)
L2_incl += -I$(PDCP_DIR)
L2_incl += -I$(OPENAIR3)/MESH
L2_incl += -I$(OPENAIR2_TOP)/RRC/LITE/MESSAGES
L2_incl += -I$(OPENAIR3)/MESH/RRM

ifeq ($(rrc_cellular),1)
  include $(RRC_CELL_DIR)/Makefile.inc
  L2_OBJS += $(RRC_CELLULAR_OBJS)
  L2_incl += -I$(OPENAIR2_TOP)/RRC/CELLULAR
  EXTRA_CFLAGS += -DRRC_CELLULAR -DCELLULAR
else
  L2_OBJS += $(RRC_OBJS)
  L2_incl += -I$(OPENAIR2_TOP)/RRC/LITE
endif

