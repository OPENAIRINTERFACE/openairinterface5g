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
# file start_ue.bash
# brief
# author Lionel Gauthier
# company Eurecom
# email: lionel.gauthier@eurecom.fr
#
# OAI NETWORKING--------------------------------
declare -x EMULATION_DEV_INTERFACE="eth4"
declare -x IP_DRIVER_NAME="oai_nw_drv"
declare -x LTEIF="oai0"
declare -x UE_IPv4="10.0.0.11"
declare -x UE_IPv6="9998::11"
declare -x UE_IPv6_CIDR=$UE_IPv6"/64"
declare -x UE_IPv4_CIDR=$UE_IPv4"/24"
declare -a NAS_IMEI=( 3 9 1 8 3 6 7 3 0 2 0 0 0 0 )
#------------------------------------------------
LOG_FILE="/tmp/oai_sim_ue.log"

###########################################################
IPTABLES=/sbin/iptables
THIS_SCRIPT_PATH=$(dirname $(readlink -f $0))
declare -x OPENAIR_DIR=""
declare -x OPENAIR1_DIR=""
declare -x OPENAIR2_DIR=""
declare -x OPENAIR3_DIR=""
declare -x OPENAIR_TARGETS=""
###########################################################

###########################################################
source $THIS_SCRIPT_PATH/utils.bash
source $THIS_SCRIPT_PATH/build_all.bash
###########################################################


##################################################
# LAUNCH UE
##################################################

echo "Bringup UE interface"
pkill oaisim             > /dev/null 2>&1
pkill oaisim             > /dev/null 2>&1
rmmod -f $IP_DRIVER_NAME > /dev/null 2>&1

bash_exec "insmod  $OPENAIR2_DIR/NAS/DRIVER/LITE/$IP_DRIVER_NAME.ko oai_nw_drv_IMEI=${NAS_IMEI[0]},${NAS_IMEI[1]},${NAS_IMEI[2]},${NAS_IMEI[3]},${NAS_IMEI[4]},${NAS_IMEI[5]},${NAS_IMEI[6]},${NAS_IMEI[7]},${NAS_IMEI[8]},${NAS_IMEI[9]},${NAS_IMEI[10]},${NAS_IMEI[11]},${NAS_IMEI[12]},${NAS_IMEI[13]}"
bash_exec "ip route flush cache"
bash_exec "ip link set $LTEIF up"
sleep 1
bash_exec "ip addr add dev $LTEIF $UE_IPv4_CIDR"
bash_exec "ip addr add dev $LTEIF $UE_IPv6_CIDR"
sleep 1
bash_exec "sysctl -w net.ipv4.conf.all.log_martians=1"
assert "  `sysctl -n net.ipv4.conf.all.log_martians` -eq 1" $LINENO
bash_exec "sysctl -w net.ipv4.conf.all.rp_filter=0"
assert "  `sysctl -n net.ipv4.conf.all.rp_filter` -eq 0" $LINENO
bash_exec "ip route flush cache"

# please add table 200 lte in file /etc/iproute2/rt_tables
fgrep lte /etc/iproute2/rt_tables > /dev/null 2>&1
if [ $? -ne 0 ]; then
    echo "200 lte " >> /etc/iproute2/rt_tables
fi
ip rule add fwmark 5  table lte
ip -4 route add default dev $LTEIF table lte
ip -6 route add default dev $LTEIF table lte
ip route add 239.0.0.160/28 dev $EMULATION_DEV_INTERFACE

NOW=$(date +"%Y-%m-%d.%Hh_%Mm_%Ss")
rotate_log_file $LOG_FILE


#xterm -hold -e gdb --args 
$OPENAIR_TARGETS/SIMU/USER/oaisim -a  -Q3 -s15 -K $LOG_FILE -l9 -u1 -b0 -M1 -p2 -g1 -D $EMULATION_DEV_INTERFACE  
             
wait_process_started oaisim


# start MIH-USER


sleep 100000




