#/*
# * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
# * contributor license agreements.  See the NOTICE file distributed with
# * this work for additional information regarding copyright ownership.
# * The OpenAirInterface Software Alliance licenses this file to You under
# * the OAI Public License, Version 1.1  (the "License"); you may not use this file
# * except in compliance with the License.
# * You may obtain a copy of the License at
# *
# *      http://www.openairinterface.org/?page_id=698
# *
# * Unless required by applicable law or agreed to in writing, software
# * distributed under the License is distributed on an "AS IS" BASIS,
# * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# * See the License for the specific language governing permissions and
# * limitations under the License.
# *-------------------------------------------------------------------------------
# * For more information about the OpenAirInterface (OAI) Software Alliance:
# *      contact@openairinterface.org
# */

####################################################
#       USER SPACE CODE GENERATION
####################################################

#---------------------------------------------------
#
#---------------------------------------------------


#LM_LICENSE_FILE=
#export LM_LICENSE_FILE
include $(OPENAIR_DIR)/common/utils/Makefile.inc

KERNEL_NAME:=$(shell uname -r)
NUM_CORES=$(shell getconf _NPROCESSORS_CONF)

export SET_UM=$(shell if grep --silent User\ Mode\ Linux /proc/cpuinfo ; then echo "ARCH=um" ; else echo ""; fi)

#SUBDIRS_openair_USERRF  =   $(MAIN_RT_DIR) $(CONFIG_DIR)

#openair_USERRFDIR  = openair_RFOBJS
#openair_USERRFLIBS = $(addsuffix /${openair_USERRFDIR}/Lib.a, ${SUBDIRS_openair_USERRF})
#openair_USERRFOBJS = $(addsuffix /${openair_USERRFDIR}/*.o, ${SUBDIRS_openair_USERRF})
#openair_USERRFDEPS = $(addsuffix /${openair_USERRFDIR}/*.d, ${SUBDIRS_openair_USERRF})

mac_sim_no_netlink:
	( cd SIMULATION/USER_TOOLS/LAYER2_SIM && make mac_sim MASTER=1)

mac_sim:
	( cd SIMULATION/USER_TOOLS/LAYER2_SIM && make mac_sim MASTER=1 NETLINK=1 NO_RRM=1)

mac_sim_rrm:
	( cd SIMULATION/USER_TOOLS/LAYER2_SIM && make mac_sim_rrm MASTER=1 NETLINK=1)

mac_sim_mt_cellular:
	( cd SIMULATION/USER_TOOLS/LAYER2_SIM && make mac_sim_mt CELLULAR=1 NO_RRM=1)

mac_sim_rg_cellular:
	( cd SIMULATION/USER_TOOLS/LAYER2_SIM && make mac_sim_rg MASTER=1 CELLULAR=1 NO_RRM=1)

openair_layer2_emul.ko:
	(cd LAYER2 && $(MAKE) V=1 -C $(KERNEL_DIR)/build  M=`pwd` NO_RRM=1 PCTARGET=1 RTAI=1 MASTER=1 OPEN_AIR=1 PHYEMUL=1 && mv openair_l2.ko openair_layer2.ko)

openair_layer2_emul_rrm.ko:
	(cd LAYER2 && $(MAKE) V=1 -C $(KERNEL_DIR)/build  M=`pwd` PCTARGET=1 RTAI=1 MASTER=1 OPEN_AIR=1 PHYEMUL=1 && mv openair_l2.ko openair_layer2.ko)


openair_layer2.ko:
	(cd LAYER2 && $(MAKE) V=1 -C $(KERNEL_DIR)/build M=`pwd` NO_RRM=1 PCTARGET=1 RTAI=1 MASTER=1 OPEN_AIR=1 OPENAIR1=1 && mv openair_l2.ko openair_layer2.ko)

openair_layer2_rrm.ko:
	(cd LAYER2 && $(MAKE) V=1 -C $(KERNEL_DIR)/build  M=`pwd` PCTARGET=1 RTAI=1 MASTER=1 OPEN_AIR=1 OPENAIR1=1 && mv openair_l2.ko openair_layer2.ko)

openair_layer2_2615.ko:
	(cd LAYER2 && $(MAKE) V=1 -C $(KERNEL_DIR)/build  M=`pwd` PCTARGET=1 RTAI=1 MASTER=1 OPEN_AIR=1 OPENAIR1=1 && mv openair_l2.ko openair_layer2_2615.ko)


openair_rrc.ko:
	(cd RRC/MESH && $(MAKE) V=1 -C $(KERNEL_DIR)/build  M=`pwd` PCTARGET=1 RTAI=1 OPEN_AIR=1 MASTER=1 NO_RRM=1 && mv openair_rrc.ko openair_RRC.ko)

openair_rrc_rrm.ko:
	(cd RRC/MESH && $(MAKE) V=1 -C $(KERNEL_DIR)/build  M=`pwd` PCTARGET=1 RTAI=1 OPEN_AIR=1 MASTER=1 && mv openair_rrc.ko openair_RRC.ko)


openair_rrc_2615.ko:
	(cd RRC/MESH && $(MAKE) V=1 -C $(KERNEL_DIR)/build M=`pwd` PCTARGET=1 RTAI=1 OPEN_AIR=1 MASTER=1 && mv openair_rrc.ko openair_RRC_2615.ko)

openair_rrc_emul.ko:
	(cd RRC/MESH && $(MAKE) V=1 -C $(KERNEL_DIR)/build  M=`pwd` PCTARGET=1 RTAI=1 OPEN_AIR=1 MASTER=1 PHYEMUL=1 NO_RRM=1)

openair_rrc_emul_rrm.ko:
	(cd RRC/MESH && $(MAKE) V=1 -C $(KERNEL_DIR)/build  M=`pwd` PCTARGET=1 RTAI=1 OPEN_AIR=1 MASTER=1 PHYEMUL=1)


openair_emul.ko:
	(cd SIMULATION/PHY_EMULATION/DEVICE_DRIVER/ && $(MAKE) V=1 -C $(KERNEL_DIR)/build  M=`pwd` PCTARGET=1 RTAI=1 OPEN_AIR=1 MASTER=1 NO_RRM=1)

openair_emul_rrm.ko:
	(cd SIMULATION/PHY_EMULATION/DEVICE_DRIVER/ && $(MAKE) V=1 -C $(KERNEL_DIR)/build  M=`pwd` PCTARGET=1 RTAI=1 OPEN_AIR=1 MASTER=1)



create_device:
	rm -f /dev/openair0
	mknod /dev/openair0 c 127 0
	chmod a+rw /dev/openair0


install_emul:
	make devs
	make fifos
	(insmod SIMULATION/PHY_EMULATION/DEVICE_DRIVER/openair_emul.ko)
	(insmod LAYER2/openair_layer2.ko)
	(insmod RRC/MESH/openair_rrc.ko)
	(insmod NETWORK_DRIVER/MESH/nasmesh.ko)

remove_emul:
	rmmod  nasmesh
	sleep 1
	rmmod  openair_rrc
	sleep 1
	rmmod  openair_l2
	sleep 1
	rmmod  openair_emul


clean_nasmesh:
	(cd NETWORK_DRIVER/MESH && $(MAKE) V=1 -C $(KERNEL_DIR)/build M=`pwd` clean)

nasmesh_address_fix.ko:
	(cd NETWORK_DRIVER/MESH && $(MAKE) -j$(NUM_CORES) $(SET_UM) V=1 ADDRESS_FIX=1 -C $(KERNEL_DIR)/build M=`pwd`)

nasmesh.ko:
	(cd NETWORK_DRIVER/MESH && $(MAKE) -j$(NUM_CORES) $(SET_UM) V=1 -C $(KERNEL_DIR)/build M=`pwd`)

# automatically detect the linux header files for driver compilation
nasmesh_netlink.ko:
	(cd NETWORK_DRIVER/MESH && $(MAKE) -j$(NUM_CORES) $(SET_UM) PDCP_USE_NETLINK=1 V=1 -C $(KERNEL_DIR)/build M=`pwd` modules)

naslite_netlink_ether.ko:
	(cd NETWORK_DRIVER/LITE && $(MAKE) -j$(NUM_CORES) $(SET_UM) OAI_NW_DRIVER_TYPE_ETHERNET=1 PDCP_USE_NETLINK=1 OAI_NW_DRIVER_USE_NETLINK=1 V=1 -C $(KERNEL_DIR)/build M=`pwd` modules)

oai_nw_ether.ko:
	(cd NETWORK_DRIVER/LITE && $(MAKE) -j$(NUM_CORES) $(SET_UM) OAI_NW_DRIVER_TYPE_ETHERNET=1 PDCP_USE_NETLINK=1 OAI_NW_DRIVER_USE_NETLINK=1 V=1 -C $(KERNEL_DIR)/build M=`pwd` modules)

oai_nw_drv.ko:
	(cd NETWORK_DRIVER/LITE && $(MAKE) -j$(NUM_CORES) $(SET_UM) OAI_NW_DRIVER_TYPE_ETHERNET=1 PDCP_USE_NETLINK=1 OAI_NW_DRIVER_USE_NETLINK=1 V=1 -C $(KERNEL_DIR)/build M=`pwd` modules)

nasmesh_netlink_address_fix.ko:
	(cd NETWORK_DRIVER/MESH && $(MAKE) -j$(NUM_CORES) $(SET_UM) PDCP_USE_NETLINK=1 ADDRESS_FIX=1  V=1 -C $(KERNEL_DIR)/build M=`pwd` modules)


nasmesh_netlink_loopback.ko:
	(cd NETWORK_DRIVER/MESH && $(MAKE) $(SET_UM) PDCP_USE_NETLINK=1 LOOPBACK=1 V=1 -C $(KERNEL_DIR)/build M=`pwd` modules)

# drivers cellular version
nascellmt_netlink.ko:
	(cd NAS/DRIVER/CELLULAR/NASMT && $(MAKE) -j$(NUM_CORES) $(SET_UM) NAS_DRIVER_TYPE_ETHERNET=1 PDCP_USE_NETLINK=1 V=1 -C $(KERNEL_DIR)/build M=`pwd` modules)

nascellmt_clean.ko:
	(cd NAS/DRIVER/CELLULAR/NASMT && $(MAKE) -j$(NUM_CORES) $(SET_UM) NAS_DRIVER_TYPE_ETHERNET=1 PDCP_USE_NETLINK=1 V=1 -C $(KERNEL_DIR)/build M=`pwd` clean)

nascellrg_netlink.ko:
	(cd NAS/DRIVER/CELLULAR/NASRG && $(MAKE) -j$(NUM_CORES) $(SET_UM) NAS_DRIVER_TYPE_ETHERNET=1 PDCP_USE_NETLINK=1 V=1 -C $(KERNEL_DIR)/build M=`pwd` modules)

nascellrg_clean.ko:
	(cd NAS/DRIVER/CELLULAR/NASRG && $(MAKE) -j$(NUM_CORES) $(SET_UM) NAS_DRIVER_TYPE_ETHERNET=1 PDCP_USE_NETLINK=1 V=1 -C $(KERNEL_DIR)/build M=`pwd` clean)

# Driver for UE using LTE core network (MME, S+P-GW)
ue_ip.ko:
	(cd NETWORK_DRIVER/UE_IP && $(MAKE) -j$(NUM_CORES) $(SET_UM) PDCP_USE_NETLINK=1 OAI_NW_DRIVER_USE_NETLINK=1 V=1 -C $(KERNEL_DIR)/build M=`pwd` modules)


rt_emul:
	(cd SIMULATION/USER_TOOLS/RT_EMUL_LAUNCHER && $(MAKE))

bypass_phy:
	(cd SIMULATION/PHY_EMULATION/TRANSPORT/KERNEL_INTERFACE && $(MAKE))

rb_tool:
	(cd NETWORK_DRIVER/MESH/RB_TOOL && $(MAKE))


fifos:
	@for i in `seq 0 64`;\
	do \
	have_rtfX=`ls /dev/ |grep -c rtf$$i`;\
	if [ "$$have_rtfX" -eq 0 ] ;then \
	mknod -m 666 /dev/rtf$$i c 150 $$i; \
	fi;\
	done

#	cp arch/openair_CardBus_MIMO1/device_driver/openair_rf.ko $(KERNEL_DIR)/openair/
#	depmod -aq

devs:
	if ! test -a /dev/openair0;\
	then \
	mknod /dev/openair0 c 127 0;\
	chmod a+rw /dev/openair0;\
	fi;

load_sounder_ue: install_cbmimo1_ublaze
	insmod mac/chansounder/openair_chansounder_ue.ko

load_sounder_nodeb: install_platon
	(cd openair_rf && ./openair_rf_platon 1917600 0 ../arch/platon/rbt_files/DAQ_B.rbt 30p 15p 10p 25p && ./openair_rf_platon 1917600 0 ../arch/platon/rbt_files/daq_platon_11_2005.rbt 30p 15p 10p 25p)


stop_sounder:
	rmmod openair_chansounder
	rmmod openair_rf

stop_sounder_nodeb:
	(cd openair_rf && ./openair_rf_platon 1907600 4)
	rmmod openair_chansounder
	rmmod openair_rf


emul_rt:
	(cd SIMULATION/USER_TOOLS/RT_EMUL_LAUNCHER && $(MAKE))

clean:
	(cd NETWORK_DRIVER/MESH && $(MAKE) clean)
	(cd NETWORK_DRIVER/LITE && $(MAKE) clean)
	rm -rf NETWORK_DRIVER/MESH/RB_TOOL/rb_tool
	rm -rf NETWORK_DRIVER/LITE/RB_TOOL/rb_tool

cleanall:
	find -name *.o -delete -print
	find -name *.ko -delete -print
	find -name *~ -delete -print
	find -name *.bak -delete -print
	rm -rf NETWORK_DRIVER/MESH/RB_TOOL/rb_tool
	rm -rf NETWORK_DRIVER/LITE/RB_TOOL/rb_tool


cleansvn:
	rm -rf `find -name *.svn* -print` ; \
	find -name *.svn* -delete -print

tgz:
	mkdir /tmp/openair2_tmp; \
	$(MAKE) cleanall; \
	cp -r . /tmp/openair2_tmp; \
	(cd /tmp/openair2_tmp && $(MAKE) cleansvn && cd .. && tar czfv openair2.tgz openair2_tmp) ; \
	cp /tmp/openair2.tgz . ;\
	rm -rf /tmp/openair2_tmp

print:
	@echo $(IS_KERNEL_OPENAIRINTERFACE)
	@echo $(KERNEL_DIR)
	@echo $(IS_LINUX)
