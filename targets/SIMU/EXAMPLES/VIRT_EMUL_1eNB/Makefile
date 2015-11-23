#ifdef cygwin
#cyg=cygwin
#endif

help:
	@echo "make one_eNB_two_UE_nas: compiles the code and performs a simulation with IP traffic between two virtual nodes (Linux only)"
	@echo "make one_eNB_one_UE: compiles the code and performs a small simulation (30 frames) with 1 eNB and 1 UE and checks that RLC data is transported between the two nodes after the connection setup"
	@echo "make one_eNB_four_UE: comples the code and performs a small simulation (60 frames) with 1 eNB and 4 UE and checks that all 4 UEs have established a complete connection (RRC output is traced)"
	@echo "make clean: Removes all .o files"

all: help

oaisim: 
	(cd $(OPENAIR_TARGETS)/SIMU/USER && $(MAKE) oaisim LINK_ENB_PDCP_TO_IP_DRIVER=1 OAI_NW_DRIVER_TYPE_ETHERNET=1 DEBUG=1) 
oaisim_nas: 
	(cd $(OPENAIR_TARGETS)/SIMU/USER && $(MAKE) oaisim OAI_NW_DRIVER_TYPE_ETHERNET=1 LINK_ENB_PDCP_TO_IP_DRIVER=1) 
nasmesh:
	(cd $(OPENAIR2_DIR) && make nasmesh_netlink_address_fix.ko)
	(cd $(OPENAIR2_DIR)/NAS/DRIVER/MESH/RB_TOOL && make)

one_eNB_one_UE_nas: oaisim_nas nasmesh
	./start_one_eNB_multi_UE_nas 1

one_eNB_two_UE_nas: oaisim_nas nasmesh
	./start_one_eNB_multi_UE_nas 2

one_eNB_three_UE_nas: oaisim_nas nasmesh
	./start_one_eNB_multi_UE_nas 3

one_eNB_one_UE : oaisim
	../../USER/oaisim -n30 | egrep SEND_SDU

one_eNB_four_UE : oaisim
	../../USER/oaisim -n60 -u4 | egrep "\[RRC\]"	

one_eNB_one_UE_PHY : oaisim_nas nasmesh
	./start_one_eNB_two_UE_PHY_nas 2

clean:
	(cd $(OPENAIR_TARGETS)/SIMU/USER && make clean)

echo_vars:
	echo $(OPENAIR1_DIR)
	echo $(OPENAIR2_DIR)
	echo $(OPENAIR3_DIR)
	echo $(OPENAIR_TARGETS)
