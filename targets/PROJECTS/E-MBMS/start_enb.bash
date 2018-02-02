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
# file start_enb.bash
# brief
# author Lionel Gauthier
# company Eurecom
# email: lionel.gauthier@eurecom.fr
#
# OAI NETWORKING--------------------------------
declare -x EMULATION_DEV_INTERFACE="eth0"
declare -x IP_DRIVER_NAME="oai_nw_drv"
declare -x LTEIF="oai0"
declare -x ENB_IPv4="10.0.1.1"
declare -x UE_IPv4="10.0.1.11"
declare -x ENB_IPv6="9998::1"
declare -x UE_IPv6="9998::11"
declare -x ENB_IPv6_CIDR=$ENB_IPv6"/64"
declare -x ENB_IPv4_CIDR=$ENB_IPv4"/24"
declare -a NAS_IMEI=( 3 9 1 8 3 6 6 2 0 0 0 0 0 0 )
declare -x DEFAULT_RB_ID=3
declare -x MBMS_RB_ID=225
#------------------------------------------------
LOG_FILE="/tmp/oai_sim_enb.log"


###########################################################
IPTABLES=/sbin/iptables
declare -x OPENAIR_DIR=""
declare -x OPENAIR1_DIR=""
declare -x OPENAIR2_DIR=""
declare -x OPENAIR3_DIR=""
declare -x OPENAIR_TARGETS=""
#######################################
#######################################

###########################################################
THIS_SCRIPT_PATH=$(dirname $(readlink -f $0))
source $THIS_SCRIPT_PATH/utils.bash
###########################################################

set_openair
cecho "OPENAIR_DIR     = $OPENAIR_DIR" $green
cecho "OPENAIR1_DIR    = $OPENAIR1_DIR" $green
cecho "OPENAIR2_DIR    = $OPENAIR2_DIR" $green
cecho "OPENAIR3_DIR    = $OPENAIR3_DIR" $green
cecho "OPENAIR_TARGETS = $OPENAIR_TARGETS" $green

bash_exec "/sbin/iptables  -t mangle -F"
bash_exec "/sbin/iptables  -t nat -F"
bash_exec "/sbin/iptables  -t raw -F"
bash_exec "/sbin/iptables  -t filter -F"
bash_exec "/sbin/ip6tables -t mangle -F"
bash_exec "/sbin/ip6tables -t filter -F"
bash_exec "/sbin/ip6tables -t raw -F"

##################################################
# LAUNCH eNB  executable
##################################################
source $THIS_SCRIPT_PATH/build_all.bash

echo "Bringup eNB interface"
pkill oaisim             > /dev/null 2>&1
pkill oaisim             > /dev/null 2>&1
rmmod -f $IP_DRIVER_NAME > /dev/null 2>&1

bash_exec "insmod  $OPENAIR2_DIR/NAS/DRIVER/LITE/$IP_DRIVER_NAME.ko oai_nw_drv_IMEI=${NAS_IMEI[0]},${NAS_IMEI[1]},${NAS_IMEI[2]},${NAS_IMEI[3]},${NAS_IMEI[4]},${NAS_IMEI[5]},${NAS_IMEI[6]},${NAS_IMEI[7]},${NAS_IMEI[8]},${NAS_IMEI[9]},${NAS_IMEI[10]},${NAS_IMEI[11]},${NAS_IMEI[12]},${NAS_IMEI[13]}"
bash_exec "ip route flush cache"
bash_exec "ip link set $LTEIF up"
sleep 1
bash_exec "ip addr add dev $LTEIF $ENB_IPv4_CIDR"
bash_exec "ip addr add dev $LTEIF $ENB_IPv6_CIDR"
sleep 1
bash_exec "sysctl -w net.ipv4.conf.all.log_martians=1"
assert "  `sysctl -n net.ipv4.conf.all.log_martians` -eq 1" $LINENO
bash_exec "sysctl -w net.ipv4.conf.all.rp_filter=0"
assert "  `sysctl -n net.ipv4.conf.all.rp_filter` -eq 0" $LINENO
bash_exec "ip route flush cache"

# please add table 200 lte in /etc/iproute2/rt_tables
fgrep lte /etc/iproute2/rt_tables > /dev/null 2>&1
if [ $? -ne 0 ]; then
    echo "200 lte " >> /etc/iproute2/rt_tables
fi
ip rule  add fwmark $DEFAULT_RB_ID  table lte
ip rule  add fwmark $MBMS_RB_ID     table lte
ip route add default dev $LTEIF     table lte
ip route add 239.0.0.160/28 dev $EMULATION_DEV_INTERFACE

/sbin/ebtables -t nat -A POSTROUTING -p arp  -j mark --mark-set $DEFAULT_RB_ID

#/sbin/ip6tables -A OUTPUT -t mangle -o oai0 -m pkttype --pkt-type multicast -j MARK --set-mark $DEFAULT_RB_ID
#/sbin/iptables  -A OUTPUT -t mangle -o oai0 -m pkttype --pkt-type broadcast -j MARK --set-mark $DEFAULT_RB_ID
#/sbin/iptables  -A OUTPUT -t mangle -o oai0 -m pkttype --pkt-type multicast -j MARK --set-mark $DEFAULT_RB_ID

#/sbin/ip6tables -A POSTROUTING -t mangle -o oai0 -m pkttype --pkt-type multicast -j MARK --set-mark $DEFAULT_RB_ID
#/sbin/iptables  -A POSTROUTING -t mangle -o oai0 -m pkttype --pkt-type broadcast -j MARK --set-mark $DEFAULT_RB_ID
#/sbin/iptables  -A POSTROUTING -t mangle -o oai0 -m pkttype --pkt-type multicast -j MARK --set-mark $DEFAULT_RB_ID

#All other traffic is sent on the RAB you want (mark = RAB ID)
/sbin/ip6tables -A POSTROUTING -t mangle -o oai0                               -j MARK --set-mark $MBMS_RB_ID

#/sbin/ip6tables -A POSTROUTING -t mangle -o oai0 -m pkttype --pkt-type unicast -j MARK --set-mark $DEFAULT_RB_ID
#/sbin/ip6tables -A OUTPUT      -t mangle -o oai0 -m pkttype --pkt-type unicast -j MARK --set-mark $DEFAULT_RB_ID
#/sbin/iptables  -A POSTROUTING -t mangle -o oai0 -m pkttype --pkt-type unicast -j MARK --set-mark $DEFAULT_RB_ID
#/sbin/iptables  -A OUTPUT      -t mangle -o oai0 -m pkttype --pkt-type unicast -j MARK --set-mark $DEFAULT_RB_ID

rotate_log_file $LOG_FILE

#xterm -hold -e gdb --args
#$OPENAIR_TARGETS/SIMU/USER/oaisim -a  -Q3 -s15 -K $LOG_FILE -l9 -u0 -b1 -M0 -p2  -g1 -D $EMULATION_DEV_INTERFACE -O $THIS_SCRIPT_PATH/enb.conf &
#$OPENAIR_TARGETS/SIMU/USER/oaisim -a -l3 -u0 -b1 -M0 -p2  -g1 -D $EMULATION_DEV_INTERFACE -O $THIS_SCRIPT_PATH/enb.conf &
$OPENAIR_TARGETS/SIMU/USER/oaisim -a -Q3 -l7 -u0 -b1 -M0 -p2  -g1 -D 192.168.55.51 -O $THIS_SCRIPT_PATH/enb.conf | grep "PDCP\|RLC\|RRC" &


wait_process_started oaisim


sleep 100000


