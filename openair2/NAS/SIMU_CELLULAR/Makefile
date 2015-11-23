# lines starting with the pound sign are comments.
#
# These things are options that you might need
# to tweak.


# You can modify the below as well, but probably
# won't need to.

include $(OPENAIR_DIR)/common/utils/Makefile.inc
#UPDIR	:= /homes/mwett/rh_mw/RHODOS
COMMON_DIR = $(OPENAIR2_DIR)/COMMON
RRC_CELL_DIR = $(OPENAIR2_DIR)/RRC/CELLULAR
NAS_SIMU_DIR = $(OPENAIR2_DIR)/NAS/SIMU_CELLULAR
#ASN1_DIR = $(OPENAIR2_DIR)/RRC/LITE/MESSAGES
ASN1_DIR = $(OPENAIR2_DIR)

#UPDIR	:= $(EURECOM_SRC_PATH)
#GRAAL_DIR  =$(UPDIR)/non_access_stratum/driver/nasmt
#RRC_DIR =$(UPDIR)/access_stratum/l3/rrc

#CFLAGS += -Wall -g -DDEBUG_RRC_STATE -DUSER_MODE -DNODE_RG -DNODE_MT
CFLAGS += -std=gnu99 -I$(KERNEL_DIR)  -DUSER_MODE -DNB_ANTENNAS_RX=1

ifdef CELL_RRM
CFLAGS += -DNODE_RG
OBJ +=rrm_fifo_standalone.o nasrg_simu_meas.o
else
ifdef MASTER
CFLAGS += -DNODE_RG
OBJ +=nasrg_simu_main.o nasrg_simu_control.o nasrg_simu_rrm_fifo.o nasrg_simu_meas.o
else
CFLAGS += -DNODE_MT
OBJ +=nasmt_simu_main.o nasmt_simu_control.o
endif
endif

ifdef CELLULAR
CFLAGS += -DCELLULAR -DNO_RRM -I$(COMMON_DIR) -I$(RRC_CELL_DIR) -I$(NAS_SIMU_DIR) -I$(ASN1_DIR)
endif


$(OBJ) : %.o : %.c
	$(CC) $(CFLAGS) -c -o $@ $< -pthread

all: nasmt_simu nasrg_simu

nasmt_sim : $(OBJ)
	$(CC)  -o nasmt_simu $(OBJ) $(CFLAGS) -lm -lrt -pthread
	mv nasmt_simu $(OPENAIR_TARGETS)/SIMU/USER

nasrg_sim : $(OBJ)
	$(CC)  -o nasrg_simu $(OBJ) $(CFLAGS) -lm -lrt -pthread
	mv nasrg_simu $(OPENAIR_TARGETS)/SIMU/USER

cell_rrm : $(OBJ)
	$(CC)  -o cell_rrm $(OBJ) $(CFLAGS) -lm -lrt -pthread
	mv cell_rrm $(OPENAIR_TARGETS)/SIMU/USER

clean:
	-rm $(OBJ) $(EXECUTABLE)

