all: oaisim nasmesh

oaisim: 
	(cd $(OPENAIR_TARGETS)/SIMU/USER && make LINK_ENB_PDCP_TO_IP_DRIVER=1 -j2 )
nasmesh:
	(cd $(OPENAIR2_DIR) && make nasmesh_netlink.ko)
	(cd $(OPENAIR2_DIR)/NAS/DRIVER/MESH/RB_TOOL/ && make)

one_eNB_one_UE: oaisim nasmesh

userclean: clean oaisim nasmesh	

clean: 
	(cd $(OPENAIR_TARGETS)/SIMU/USER && make cleanall)
