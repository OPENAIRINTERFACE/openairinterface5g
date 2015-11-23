all: oaisim naslite_netlink_ether

oaisim:
	(cd $(OPENAIR_TARGETS)/SIMU/USER && make oaisim OAI_NW_DRIVER_TYPE_ETHERNET=1 PDCP_USE_NETLINK=1)
naslite_netlink_ether:
	(cd $(OPENAIR2_DIR) && make naslite_netlink_ether.ko)
	(cd $(OPENAIR2_DIR)/NAS/DRIVER/LITE/RB_TOOL/ && make)

remserial:
	(cd $(OPENAIR3_DIR)/UTILS/REMSERIAL-1.4.UDP && make all)
	(cp -upv $(OPENAIR3_DIR)/UTILS/REMSERIAL-1.4.UDP/remserial /usr/bin/remserial.udp)

one_eNB_one_UE: oaisim naslite_netlink_ether

userclean: clean oaisim naslite_netlink_ether

clean:
	(cd $(OPENAIR_TARGETS)/SIMU/USER && make clean)
	(cd $(OPENAIR2_DIR)/NAS/DRIVER/LITE && make clean)
	(cd $(OPENAIR2_DIR) && make clean)
