
all: oaisim naslite_netlink_ether

oaisim:
	(cd $(OPENAIR_TARGETS)/SIMU/USER && $(MAKE) oaisim OAI_NW_DRIVER_TYPE_ETHERNET=1 LINK_ENB_PDCP_TO_IP_DRIVER=1)

naslite_netlink_ether:
	(cd $(OPENAIR2_DIR) && $(MAKE) naslite_netlink_ether.ko)
	(cd $(OPENAIR2_DIR)/NAS/DRIVER/LITE/RB_TOOL/ && $(MAKE))

userclean: clean oaisim naslite_netlink_ether

one_eNB_one_UE_nas: oaisim naslite_netlink_ether
	/bin/bash ./start_one_eNB_multi_UE_nas 1

one_eNB_two_UE_nas: oaisim naslite_netlink_ether
	/bin/bash ./start_one_eNB_multi_UE_nas 2

one_eNB_three_UE_nas: oaisim naslite_netlink_ether
	/bin/bash ./start_one_eNB_multi_UE_nas 3

clean:
	(cd $(OPENAIR_TARGETS)/SIMU/USER && $(MAKE) clean)
	(cd $(OPENAIR2_DIR)/NAS/DRIVER/LITE && $(MAKE) clean)

echo_vars:
	echo $(OPENAIR1_DIR)
	echo $(OPENAIR2_DIR)
	echo $(OPENAIR3_DIR)
	echo $(OPENAIR_TARGETS)

