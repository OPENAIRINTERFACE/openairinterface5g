#!/bin/bash
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
################################################################################
# file start_enb_and_ue.bash
# brief
# author Lionel Gauthier
# company Eurecom
# email: lionel.gauthier@eurecom.fr
###########################################
# INPUT OF THIS SCRIPT:
# THE DIRECTORY WHERE ARE LOCATED THE CONFIGURATION FILES
#########################################
# This script start  ENB+UE (all in one executable, on one host)
# MME+SP-GW executable have to be launched by your own (start_epc.bash) before this script is invoked.
#


###########################################################
# Parameters
###########################################################
declare MAKE_LTE_ACCESS_STRATUM_TARGET="oaisim LIBCONFIG_LONG=1 DEBUG=1 ENABLE_ITTI=1 USE_MME=R10 LINK_ENB_PDCP_TO_GTPV1U=1 NAS=1 SECU=1 Rel10=1 TRACE_RLC_AM_PDU=1 TRACE_RLC_UM_PDU=1"
declare MAKE_IP_DRIVER_TARGET="ue_ip.ko"
declare IP_DRIVER_NAME="ue_ip"
declare LTEIF="oip1"
declare UE_IPv4="10.0.0.8"
declare UE_IPv6="2001:1::8"
declare UE_IPv6_CIDR=$UE_IPv6"/64"
declare UE_IPv4_CIDR=$UE_IPv4"/24"

###########################################################
THIS_SCRIPT_PATH=$(dirname $(readlink -f $0))
. $THIS_SCRIPT_PATH/utils.bash
. $THIS_SCRIPT_PATH/interfaces.bash
. $THIS_SCRIPT_PATH/networks.bash
###########################################################
if [ $# -eq 1 ]; then
    declare -x CONFIG_FILE_DIR=$1
    if [ ! -d $CONFIG_FILE_DIR ]; then
        echo_error "ERROR while invoking this script, as first argument to this script you have to provide the path to a directory (./CONF/VLAN.VIRTUAL.$HOSTNAME for example) containing valid epc and enb config files"
        exit 1
    fi
else
    echo_error "ERROR while invoking this script, as first argument to this script  you have to provide the path to a directory  (./CONF/VLAN.VIRTUAL.$HOSTNAME for example) containing valid epc and enb config files"
    exit 1
fi


#check_install_epc_software



cd $THIS_SCRIPT_PATH
#######################################################
# FIND CONFIG FILE
#######################################################
SEARCHED_CONFIG_FILE_ENB="enb*.conf"
CONFIG_FILE_ENB=`find $CONFIG_FILE_DIR -iname $SEARCHED_CONFIG_FILE_ENB`
if [ -f $CONFIG_FILE_ENB ]; then
    echo_warning "eNB config file found is now $CONFIG_FILE_ENB"
else
    echo_error "eNB config file not found, exiting"
    exit 1
fi

#######################################################
# SOURCE CONFIG FILE
#######################################################
rm -f /tmp/source.txt
VARIABLES="
           ENB_INTERFACE_NAME_FOR_S1_MME\|\
           ENB_IPV4_ADDRESS_FOR_S1_MME\|\
           ENB_INTERFACE_NAME_FOR_S1U\|\
           ENB_IPV4_ADDRESS_FOR_S1U"

VARIABLES=$(echo $VARIABLES | sed -e 's/\\r//g')
VARIABLES=$(echo $VARIABLES | tr -d ' ')
cat $CONFIG_FILE_ENB | grep -w "$VARIABLES"| tr -d " " | tr -d ";" > /tmp/source.txt
source /tmp/source.txt

declare ENB_IPV4_NETMASK_FOR_S1_MME=$(       echo $ENB_IPV4_ADDRESS_FOR_S1_MME        | cut -f2 -d '/')
declare ENB_IPV4_NETMASK_FOR_S1U=$(          echo $ENB_IPV4_ADDRESS_FOR_S1U        | cut -f2 -d '/')

ENB_IPV4_ADDRESS_FOR_S1_MME=$(               echo $ENB_IPV4_ADDRESS_FOR_S1_MME        | cut -f1 -d '/')
ENB_IPV4_ADDRESS_FOR_S1U=$(                  echo $ENB_IPV4_ADDRESS_FOR_S1U           | cut -f1 -d '/')


is_vlan_interface $ENB_INTERFACE_NAME_FOR_S1_MME \
                  $ENB_INTERFACE_NAME_FOR_S1U
if [ $? -eq 1 ]; then
    echo_success "Found VLAN network configuration"
    clean_enb_vlan_network
    build_enb_vlan_network
    test_enb_vlan_network
fi 


#######################################################
# USIM, NVRAM files
#######################################################
export NVRAM_DIR=$THIS_SCRIPT_PATH

if [ ! -f $OPENAIR3_DIR/NAS/EURECOM-NAS/bin/ue_data ] || [ ! -f $OPENAIR3_DIR/NAS/EURECOM-NAS/bin/usim_data ]; then
    if [ ! -f /tmp/nas_cleaned ]; then
        echo_success "make --directory=$OPENAIR3_DIR/NAS/EURECOM-NAS veryveryclean"
        make --directory=$OPENAIR3_DIR/NAS/EURECOM-NAS veryveryclean
    fi
    echo_success "make --directory=$OPENAIR3_DIR/NAS/EURECOM-NAS -f Makefile PROCESS=UE all"
    make  -f Makefile --debug=b --directory=$OPENAIR3_DIR/NAS/EURECOM-NAS PROCESS=UE all
    rm .ue.nvram0
    rm .usim.nvram0
    touch /tmp/nas_cleaned
fi

if [ ! -f .ue.nvram0 ]; then
    echo_success "generate .ue_emm.nvram0 .ue.nvram0"
    $OPENAIR3_DIR/NAS/EURECOM-NAS/bin/ue_data --gen
fi

if [ ! -f .usim.nvram0 ]; then
    echo_success "generate .usim.nvram0"
    $OPENAIR3_DIR/NAS/EURECOM-NAS/bin/usim_data --gen
fi
$OPENAIR3_DIR/NAS/EURECOM-NAS/bin/ue_data --print
$OPENAIR3_DIR/NAS/EURECOM-NAS/bin/usim_data --print

##################################################
# LAUNCH eNB + UE executable
##################################################
pkill oaisim
bash_exec "rmmod $IP_DRIVER_NAME" > /dev/null 2>&1

echo_success "make $MAKE_IP_DRIVER_TARGET $MAKE_LTE_ACCESS_STRATUM_TARGET ....." 
#bash_exec "make --directory=$OPENAIR2_DIR $MAKE_IP_DRIVER_TARGET "
make --directory=$OPENAIR2_DIR $MAKE_IP_DRIVER_TARGET || exit 1


echo_success "make --debug=b --directory=$OPENAIR_TARGETS/SIMU/USER $MAKE_LTE_ACCESS_STRATUM_TARGET || exit 1" 
#make --directory=$OPENAIR_TARGETS/SIMU/USER $MAKE_LTE_ACCESS_STRATUM_TARGET -j`grep -c ^processor /proc/cpuinfo ` || exit 1
make --debug=b --directory=$OPENAIR_TARGETS/SIMU/USER $MAKE_LTE_ACCESS_STRATUM_TARGET || exit 1



echo_success "Bringup UE interface..."
bash_exec "insmod  $OPENAIR2_DIR/NETWORK_DRIVER/UE_IP/$IP_DRIVER_NAME.ko"

bash_exec "ip route flush cache"

#bash_exec "ip link set $LTEIF up"
sleep 1
#bash_exec "ip addr add dev $LTEIF $UE_IPv4_CIDR"
#bash_exec "ip addr add dev $LTEIF $UE_IPv6_CIDR"

sleep 1

bash_exec "sysctl -w net.ipv4.conf.all.log_martians=1"
assert "  `sysctl -n net.ipv4.conf.all.log_martians` -eq 1" $LINENO

echo "   Disabling reverse path filtering"
bash_exec "sysctl -w net.ipv4.conf.all.rp_filter=0"
assert "  `sysctl -n net.ipv4.conf.all.rp_filter` -eq 0" $LINENO


bash_exec "ip route flush cache"

# Check table 200 lte in /etc/iproute2/rt_tables
fgrep lte /etc/iproute2/rt_tables  > /dev/null 
if [ $? -ne 0 ]; then
    echo "200 lte " >> /etc/iproute2/rt_tables
fi
ip rule add fwmark 5 table lte
ip route add default dev $LTEIF table lte

echo_warning "Be careful of the lines below, for a specific virtual box setting" 
bash_exec "/sbin/iptables  -t mangle -F"
bash_exec "/sbin/iptables  -t nat -F"
bash_exec "/sbin/iptables  -t raw -F"
bash_exec "/sbin/iptables  -t filter -F"
bash_exec "/sbin/ip6tables -t mangle -F"
bash_exec "/sbin/ip6tables -t filter -F"
bash_exec "/sbin/ip6tables -t raw -F"
bash_exec "iptables -A OUTPUT  -o eth0 -p sctp  -j DROP"
bash_exec "iptables -A INPUT   -i eth0 -p sctp  -j DROP"
bash_exec "iptables -A FORWARD -i eth0 -p sctp  -j DROP"
bash_exec "iptables -A OUTPUT  -o eth1 -p sctp  -j DROP"
bash_exec "iptables -A INPUT   -i eth1 -p sctp  -j DROP"
bash_exec "iptables -A FORWARD -i eth1 -p sctp  -j DROP"

cd $THIS_SCRIPT_PATH/OUTPUT/$HOSTNAME
ITTI_LOG_FILE=itti_enb_ue.$HOSTNAME.log
#rotate_log_file $ITTI_LOG_FILE

STDOUT_LOG_FILE=stdout_enb_ue.$HOSTNAME.log
#rotate_log_file $STDOUT_LOG_FILE

rotate_log_file $STDOUT_LOG_FILE.filtered
#rotate_log_file tshark_enb_ue.$HOSTNAME.pcap

cd $THIS_SCRIPT_PATH

#nohup tshark -i $ENB_INTERFACE_NAME_FOR_S1_MME -i $ENB_INTERFACE_NAME_FOR_S1U -w OUTPUT/$HOSTNAME/tshark_enb_ue.$HOSTNAME.pcap &

# To start NAS connectivity: AT+CFUN=1
# nohup xterm -hold -e $OPENAIR3_DIR/NAS/EURECOM-NAS/bin/UserProcess &

export MALLOC_TRACE=/tmp/malloc_trace.txt
touch /tmp/malloc_trace.txt
#--malloc-trace-enabled 
#-a
gdb --args $OPENAIR_TARGETS/SIMU/USER/oaisim -u1 -l9 -K OUTPUT/$HOSTNAME/$ITTI_LOG_FILE --enb-conf $CONFIG_FILE_ENB 2>&1 | tee OUTPUT/$HOSTNAME/$STDOUT_LOG_FILE 
# > OUTPUT/$HOSTNAME/$STDOUT_LOG_FILE 
# | tee OUTPUT/$HOSTNAME/$STDOUT_LOG_FILE 

#pkill tshark
echo_warning "oaisim ended, filtering stdout from oaisim"
cat OUTPUT/$HOSTNAME/$STDOUT_LOG_FILE | grep 'RRC\|S1AP\|SCTP\|NAS' | grep -v 'RRC_MAC_IN_SYNC_IND' | grep -v 'serving becomes better than threshold' > OUTPUT/$HOSTNAME/$STDOUT_LOG_FILE.filtered
