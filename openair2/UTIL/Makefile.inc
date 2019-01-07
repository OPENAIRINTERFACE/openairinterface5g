MEM_DIR = $(OPENAIR2_TOP)/UTIL/MEM
LIST_DIR = $(OPENAIR2_TOP)/UTIL/LISTS
FIFO_DIR = $(OPENAIR2_TOP)/UTIL/FIFO
MATH_DIR = $(OPENAIR2_TOP)/UTIL/MATH
TIMER_DIR = $(OPENAIR2_TOP)/UTIL/TIMER
LOG_DIR=$(OPENAIR2_TOP)/UTIL/LOG
OCG_DIR=$(OPENAIR2_TOP)/UTIL/OCG
OPT_DIR=$(OPENAIR2_TOP)/UTIL/OPT
OMG_DIR=$(OPENAIR2_TOP)/UTIL/OMG
OTG_DIR=$(OPENAIR2_TOP)/UTIL/OTG
CLI_DIR=$(OPENAIR2_TOP)/UTIL/CLI
OMV_DIR=$(OPENAIR2_TOP)/UTIL/OMV
LFDS_DIR=$(OPENAIR2_TOP)/UTIL/LFDS/liblfds6.1.1/liblfds611
OSA_DIR=$(OPENAIR2_TOP)/UTIL/OSA

LIST_OBJ =  $(LIST_DIR)/list.o

FIFO_OBJ =  $(FIFO_DIR)/pad_list.o

LIST_OBJ +=  $(LIST_DIR)/list2.o

TIMER_OBJ +=  $(TIMER_DIR)/umts_timer.o

MEM_OBJ +=  $(MEM_DIR)/mem_block.o

LOG_OBJS =  $(LOG_DIR)/log.o
LOG_OBJS +=  $(LOG_DIR)/vcd_signal_dumper.o

OCG_OBJS  =  $(OCG_DIR)/OCG.o
OCG_OBJS +=  $(OCG_DIR)/OCG_create_dir.o
OCG_OBJS +=  $(OCG_DIR)/OCG_detect_file.o
OCG_OBJS +=  $(OCG_DIR)/OCG_generate_report.o
OCG_OBJS +=  $(OCG_DIR)/OCG_parse_filename.o
OCG_OBJS +=  $(OCG_DIR)/OCG_parse_XML.o
OCG_OBJS +=  $(OCG_DIR)/OCG_save_XML.o

ifdef SECU
OSA_OBJS  = $(OSA_DIR)/osa_key_deriver.o
OSA_OBJS += $(OSA_DIR)/osa_stream_eia.o
OSA_OBJS += $(OSA_DIR)/osa_stream_eea.o
OSA_OBJS += $(OSA_DIR)/osa_snow3g.o
OSA_OBJS += $(OSA_DIR)/osa_rijndael.o
endif

OPT_OBJS =  $(OPT_DIR)/probe.o

OMG_OBJS =  $(OMG_DIR)/omg.o
OMG_OBJS +=  $(OMG_DIR)/common.o
OMG_OBJS +=  $(OMG_DIR)/job.o
OMG_OBJS +=  $(OMG_DIR)/static.o
OMG_OBJS +=  $(OMG_DIR)/rwp.o
OMG_OBJS +=  $(OMG_DIR)/rwalk.o
#OMG_OBJS +=  $(OMG_DIR)/omg_hashtable.o
OMG_OBJS +=  $(OMG_DIR)/mobility_parser.o
OMG_OBJS +=  $(OMG_DIR)/trace.o
OMG_OBJS +=  $(OMG_DIR)/sumo.o
OMG_OBJS +=  $(OMG_DIR)/id_manager.o
OMG_OBJS +=  $(OMG_DIR)/client_traci_OMG.o
OMG_OBJS +=  $(OMG_DIR)/storage_traci_OMG.o
OMG_OBJS +=  $(OMG_DIR)/socket_traci_OMG.o
OMG_OBJS +=  $(OMG_DIR)/steadystaterwp.o
OMG_OBJS +=  $(OMG_DIR)/grid.o
OMG_OBJS +=  $(OMG_DIR)/trace_hashtable.o


OTG_OBJS =  $(OTG_DIR)/otg_tx.o
OTG_OBJS +=  $(OTG_DIR)/otg.o
OTG_OBJS +=  $(OTG_DIR)/otg_rx.o
OTG_OBJS +=  $(OTG_DIR)/otg_kpi.o
OTG_OBJS +=  $(OTG_DIR)/otg_form.o
OTG_OBJS +=  $(OTG_DIR)/otg_models.o

MATH_OBJS  =  $(MATH_DIR)/oml.o

CLI_OBJ  =  $(CLI_DIR)/cli_server.o
CLI_OBJ  +=  $(CLI_DIR)/cli.o
CLI_OBJ  +=  $(CLI_DIR)/cli_cmd.o

UTIL_OBJ =              \
    $(OSA_OBJS)         \
    $(FIFO_OBJ)         \
    $(LIST_OBJ)         \
    $(TIMER_OBJ)        \
    $(MEM_OBJ)          \
    $(LOG_OBJS)         \
    $(OCG_OBJS)         \
    $(MATH_OBJS)        \
    $(OTG_OBJS)         \
    $(CLI_OBJ)          \
    $(OMG_OBJS)         \
    $(OPT_OBJS)

UTIL_incl = \
    -I$(OPENAIR2_TOP)/UTIL      \
    -I$(OSA_DIR)                \
    -I$(LFDS_DIR)/inc           \
    -I$(MEM_DIR)                \
    -I$(LIST_DIR)               \
    -I$(FIFO_DIR)               \
    -I$(OCG_DIR)                \
    -I$(LOG_DIR)                \
    -I$(MATH_DIR)               \
    -I$(TIMER_DIR)              \
    -I$(OMG_DIR)                \
    -I$(OTG_DIR)                \
    -I$(CLI_DIR)                \
    -I$(OPT_DIR)                \
    -I$(OMV_DIR)
