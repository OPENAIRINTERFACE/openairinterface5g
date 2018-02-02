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
###########################################
# INPUT OF THIS SCRIPT:
# THE DIRECTORY WHERE ARE LOCATED THE CONFIGURATION FILES
#########################################
# This script start  UE 
###########################################################
# Parameters
###########################################################
declare -x hard_real_time= "no"
declare -x EMULATION_DEV_INTERFACE="eth2"
declare EMULATION_MULTICAST_GROUP=1
declare MAKE_LTE_ACCESS_STRATUM_TARGET="oaisim DEBUG=1 ENABLE_ITTI=1 USE_MME=R10 NAS=1 Rel10=1 SECU=1 RRC_MSG_PRINT=1"
declare MAKE_LTE_ACCESS_STRATUM_TARGET_RT="lte-softmodem HARD_RT=1 ENABLE_ITTI=1 USE_MME=R10 DISABLE_XER_PRINT=1 SECU=1 RRC_MSG_PRINT=1 "

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

check_install_epc_software

cd $THIS_SCRIPT_PATH

EMULATION_DEV_ADDRESS=`ifconfig $EMULATION_DEV_INTERFACE | grep 'inet addr:'| grep -v '127.0.0.1' | cut -d: -f2 | awk '{ print $1}'`

#######################################################
# USIM, NVRAM files
#######################################################
export NVRAM_DIR=$THIS_SCRIPT_PATH

if [ ! -f $OPENAIR3_DIR/NAS/EURECOM-NAS/bin/ue_data ]; then
    make --directory=$OPENAIR3_DIR/NAS/EURECOM-NAS veryveryclean
    make --directory=$OPENAIR3_DIR/NAS/EURECOM-NAS PROCESS=UE
    rm .ue.nvram0
fi
if [ ! -f $OPENAIR3_DIR/NAS/EURECOM-NAS/bin/usim_data ]; then
    make --directory=$OPENAIR3_DIR/NAS/EURECOM-NAS veryveryclean
    make --directory=$OPENAIR3_DIR/NAS/EURECOM-NAS PROCESS=UE
    rm .usim.nvram0
fi
if [ ! -f .ue.nvram0 ]; then
    # generate .ue_emm.nvram0 .ue.nvram0
    $OPENAIR3_DIR/NAS/EURECOM-NAS/bin/ue_data --gen
fi

if [ ! -f .usim.nvram0 ]; then
    # generate .usim.nvram0
    $OPENAIR3_DIR/NAS/EURECOM-NAS/bin/usim_data --gen
fi
$OPENAIR3_DIR/NAS/EURECOM-NAS/bin/ue_data --print
$OPENAIR3_DIR/NAS/EURECOM-NAS/bin/usim_data --print

##################################################
# LAUNCH UE executable
##################################################
echo "Bringup UE interface"
pkill oaisim
bash_exec "rmmod $IP_DRIVER_NAME" > /dev/null 2>&1

if [ ! -d $THIS_SCRIPT_PATH/OUTPUT/$HOSTNAME ]; then 
    mkdir -m 777 -p $THIS_SCRIPT_PATH/OUTPUT/$HOSTNAME
fi

make --directory=$OPENAIR2_DIR $MAKE_IP_DRIVER_TARGET || exit 1
bash_exec "insmod  $OPENAIR2_DIR/NETWORK_DRIVER/UE_IP/$IP_DRIVER_NAME.ko"


bash_exec "ip route flush cache"
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



if [ x$hard_real_time != "xyes" ]; then

    make --directory=$OPENAIR_TARGETS/SIMU/USER $MAKE_LTE_ACCESS_STRATUM_TARGET -j`grep -c ^processor /proc/cpuinfo ` || exit 1
    ip route add 239.0.0.160/28 dev $EMULATION_DEV_INTERFACE

    ITTI_LOG_FILE=$THIS_SCRIPT_PATH/OUTPUT/$HOSTNAME/itti_ue.$HOSTNAME.log
    #rotate_log_file $ITTI_LOG_FILE
    
    STDOUT_LOG_FILE=$THIS_SCRIPT_PATH/OUTPUT/$HOSTNAME/stdout_ue.$HOSTNAME.log
    #rotate_log_file $STDOUT_LOG_FILE
    #rotate_log_file $STDOUT_LOG_FILE.filtered
    
    PCAP_LOG_FILE=$THIS_SCRIPT_PATH/OUTPUT/$HOSTNAME/tshark_ue.$HOSTNAME.pcap
    #rotate_log_file $PCAP_LOG_FILE
    
    cd $THIS_SCRIPT_PATH

    nohup xterm -e $OPENAIR3_DIR/NAS/EURECOM-NAS/bin/UserProcess &

    cp $OPENAIR_TARGETS/SIMU/USER/oaisim  $OPENAIR_TARGETS/SIMU/USER/oaisim_ue
    gdb --args $OPENAIR_TARGETS/SIMU/USER/oaisim_ue -a -l9 -u1 -b0 -M1 -p2 -g$EMULATION_MULTICAST_GROUP -D $EMULATION_DEV_ADDRESS -K $ITTI_LOG_FILE  2>&1 | tee $STDOUT_LOG_FILE 
    
else 
    #make --directory=$OPENAIR_TARGETS/RTAI/USER drivers  || exit 1
    # 2 lines below replace the line above
    cd $OPENAIR_TARGETS/ARCH/EXMIMO/DRIVER/eurecom && make clean && make   || exit 1
    cd $OPENAIR_TARGETS/ARCH/EXMIMO/USERSPACE/OAI_FW_INIT && make clean && make   || exit 1
    cd $THIS_SCRIPT_PATH

    make --directory=$OPENAIR_TARGETS/RT/USER $MAKE_LTE_ACCESS_STRATUM_TARGET_RT -j`grep -c ^processor /proc/cpuinfo ` || exit 1

    ITTI_LOG_FILE=$THIS_SCRIPT_PATH/OUTPUT/$HOSTNAME/itti_ue_rf.$HOSTNAME.log
    #rotate_log_file $ITTI_LOG_FILE
    
    STDOUT_LOG_FILE=$THIS_SCRIPT_PATH/OUTPUT/$HOSTNAME/stdout_ue_rf.$HOSTNAME.log
    #rotate_log_file $STDOUT_LOG_FILE
    #rotate_log_file $STDOUT_LOG_FILE.filtered
    
    PCAP_LOG_FILE=$THIS_SCRIPT_PATH/OUTPUT/$HOSTNAME/tshark_ue_rf.$HOSTNAME.pcap
    #rotate_log_file $PCAP_LOG_FILE
    
    if [ ! -f /tmp/init_rt_done.tmp ]; then
        echo_warning "STARTING REAL TIME (RTAI)"
        insmod /usr/realtime/modules/rtai_hal.ko     > /dev/null 2>&1
        insmod /usr/realtime/modules/rtai_sched.ko   > /dev/null 2>&1
        insmod /usr/realtime/modules/rtai_sem.ko     > /dev/null 2>&1
        insmod /usr/realtime/modules/rtai_fifos.ko   > /dev/null 2>&1
        insmod /usr/realtime/modules/rtai_mbx.ko     > /dev/null 2>&1
        touch /tmp/init_rt_done.tmp
        chmod 666 /tmp/init_rt_done.tmp
    else
        echo_warning "REAL TIME FOUND STARTED (RTAI)"
    fi
    
    cd $OPENAIR_TARGETS/RT/USER
    bash ./init_exmimo2.sh
    echo_warning "STARTING SOFTMODEM..."
    ./lte-softmodem -K $ITTI_LOG_FILE -O $CONFIG_FILE_ENB 2>&1
    #cat /dev/rtf62 > $STDOUT_LOG_FILE
    cd $THIS_SCRIPT_PATH
fi


cat $STDOUT_LOG_FILE |  grep -v '[PHY]' | grep -v '[MAC]' | grep -v '[EMU]' | \
                        grep -v '[OCM]' | grep -v '[OMG]' | \
                        grep -v 'RLC not configured' | grep -v 'check if serving becomes' | \
                        grep -v 'mac_rrc_data_req'   | grep -v 'BCCH request =>' > $STDOUT_LOG_FILE.filtered
