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

function test_usage {
    echo "OAI CI VM script"
    echo "   Original Author: Raphael Defosseux"
    echo "   Requirements:"
    echo "     -- uvtool uvtool-libvirt apt-cacher"
    echo "     -- $VM_OSREL image already synced"
    echo "   Default:"
    echo "     -- eNB with USRP"
    echo ""
    echo "Usage:"
    echo "------"
    echo "    oai-ci-vm-tool test [OPTIONS]"
    echo ""
    command_options_usage
}

function start_basic_sim_enb {
    local LOC_VM_IP_ADDR=$2
    local LOC_EPC_IP_ADDR=$3
    local LOC_LOG_FILE=$4
    local LOC_NB_RBS=$5
    local LOC_CONF_FILE=$6
    local LOC_FLEXRAN_CTL_IP_ADRR=$7
    echo "cd /home/ubuntu/tmp" > $1
    echo "echo \"sudo apt-get --yes --quiet install daemon \"" >> $1
    echo "sudo apt-get --yes install daemon >> /home/ubuntu/tmp/cmake_targets/log/daemon-install.txt 2>&1" >> $1
    echo "echo \"export ENODEB=1\"" >> $1
    echo "export ENODEB=1" >> $1
    echo "echo \"source oaienv\"" >> $1
    echo "source oaienv" >> $1
    echo "cd ci-scripts/conf_files/" >> $1
    echo "cp $LOC_CONF_FILE ci-$LOC_CONF_FILE" >> $1
    echo "sed -i -e 's#N_RB_DL.*=.*;#N_RB_DL                                         = $LOC_NB_RBS;#' -e 's#CI_MME_IP_ADDR#$LOC_EPC_IP_ADDR#' -e 's#CI_ENB_IP_ADDR#$LOC_VM_IP_ADDR#' ci-$LOC_CONF_FILE" >> $1
    if [[ $LOC_FLEXRAN_CTL_IP_ADRR =~ .*none.* ]]
    then
        echo "sed -i -e 's#CI_FLEXRAN_CTL_IP_ADDR#127.0.0.1#' ci-$LOC_CONF_FILE" >> $1
    else
        echo "sed -i -e 's#FLEXRAN_ENABLED        = .*no.*;#FLEXRAN_ENABLED        = \"yes\";#' -e 's#CI_FLEXRAN_CTL_IP_ADDR#$LOC_FLEXRAN_CTL_IP_ADRR#' ci-$LOC_CONF_FILE" >> $1
    fi
    echo "echo \"grep N_RB_DL ci-$LOC_CONF_FILE\"" >> $1
    echo "grep N_RB_DL ci-$LOC_CONF_FILE | sed -e 's#N_RB_DL.*=#N_RB_DL =#'" >> $1
    echo "echo \"cd /home/ubuntu/tmp/cmake_targets/ran_build/build/\"" >> $1
    echo "sudo chmod 777 /home/ubuntu/tmp/cmake_targets/ran_build/" >> $1
    echo "sudo chmod 777 /home/ubuntu/tmp/cmake_targets/ran_build/build/" >> $1
    echo "cd /home/ubuntu/tmp/cmake_targets/ran_build/build/" >> $1
    echo "echo \"ulimit -c unlimited && ./lte-softmodem -O /home/ubuntu/tmp/ci-scripts/conf_files/ci-$LOC_CONF_FILE --log_config.global_log_options level,nocolor --basicsim\" > ./my-lte-softmodem-run.sh " >> $1
    echo "chmod 775 ./my-lte-softmodem-run.sh" >> $1
    echo "cat ./my-lte-softmodem-run.sh" >> $1
    echo "if [ -e /home/ubuntu/tmp/cmake_targets/log/$LOC_LOG_FILE ]; then sudo sudo rm -f /home/ubuntu/tmp/cmake_targets/log/$LOC_LOG_FILE; fi" >> $1
    echo "sudo -E daemon --inherit --unsafe --name=enb_daemon --chdir=/home/ubuntu/tmp/cmake_targets/ran_build/build -o /home/ubuntu/tmp/cmake_targets/log/$LOC_LOG_FILE ./my-lte-softmodem-run.sh" >> $1

    ssh -T -o StrictHostKeyChecking=no ubuntu@$LOC_VM_IP_ADDR < $1
    rm $1
    sleep 5

    local i="0"
    echo "egrep -c \"got sync\" /home/ubuntu/tmp/cmake_targets/log/$LOC_LOG_FILE" > $1
    while [ $i -lt 10 ]
    do
        CONNECTED=`ssh -T -o StrictHostKeyChecking=no ubuntu@$LOC_VM_IP_ADDR < $1`
        if [ $CONNECTED -ne 0 ]
        then
            i="100"
        else
            sleep 5
            i=$[$i+1]
        fi
    done
    ENB_SYNC=0
    echo "echo \"free -m\"" > $1
    echo "free -m" >> $1
    ssh -T -o StrictHostKeyChecking=no ubuntu@$LOC_VM_IP_ADDR < $1
    rm $1
    if [ $i -lt 50 ]
    then
        ENB_SYNC=0
        echo "Basic-Sim eNB: eNB did NOT got sync"
    else
        echo "Basic-Sim eNB: eNB GOT SYNC --> waiting for UE to connect"
    fi
    sleep 5
}

function start_basic_sim_ue {
    local LOC_UE_LOG_FILE=$3
    local LOC_NB_RBS=$4
    local LOC_FREQUENCY=$5
    echo "echo \"cd /home/ubuntu/tmp/cmake_targets/ran_build/build/\"" > $1
    echo "sudo chmod 777 /home/ubuntu/tmp/cmake_targets/ran_build/build/" >> $1
    echo "cd /home/ubuntu/tmp/cmake_targets/ran_build/build" >> $1
    echo "echo \"./lte-uesoftmodem -C ${LOC_FREQUENCY}000000 -r $LOC_NB_RBS  --log_config.global_log_options nocolor,level --basicsim\" > ./my-lte-uesoftmodem-run.sh" >> $1
    echo "chmod 775 ./my-lte-uesoftmodem-run.sh" >> $1
    echo "cat ./my-lte-uesoftmodem-run.sh" >> $1
    echo "if [ -e /home/ubuntu/tmp/cmake_targets/log/$LOC_UE_LOG_FILE ]; then sudo sudo rm -f /home/ubuntu/tmp/cmake_targets/log/$LOC_UE_LOG_FILE; fi" >> $1
    echo "sudo -E daemon --inherit --unsafe --name=ue_daemon --chdir=/home/ubuntu/tmp/cmake_targets/ran_build/build -o /home/ubuntu/tmp/cmake_targets/log/$LOC_UE_LOG_FILE ./my-lte-uesoftmodem-run.sh" >> $1

    ssh -T -o StrictHostKeyChecking=no ubuntu@$2 < $1
    rm $1

    local i="0"
    echo "ifconfig oaitun_ue1 | egrep -c \"inet addr\"" > $1
    while [ $i -lt 10 ]
    do
        sleep 5
        CONNECTED=`ssh -T -o StrictHostKeyChecking=no ubuntu@$2 < $1`
        if [ $CONNECTED -eq 1 ]
        then
            i="100"
        else
            i=$[$i+1]
        fi
    done
    UE_SYNC=1
    rm $1
    if [ $i -lt 50 ]
    then
        UE_SYNC=0
        echo "Basic-Sim UE: oaitun_ue1 is DOWN and/or NOT CONFIGURED"
    else
        echo "Basic-Sim UE: oaitun_ue1 is UP and CONFIGURED"
    fi
    i="0"
    echo "egrep -c \"Generating RRCConnectionReconfigurationComplete\" /home/ubuntu/tmp/cmake_targets/log/$LOC_UE_LOG_FILE" > $1
    while [ $i -lt 10 ]
    do
        CONNECTED=`ssh -T -o StrictHostKeyChecking=no ubuntu@$2 < $1`
        if [ $CONNECTED -ne 0 ]
        then
            i="100"
        else
            sleep 5
            i=$[$i+1]
        fi
    done
    echo "echo \"free -m\"" > $1
    echo "free -m" >> $1
    ssh -T -o StrictHostKeyChecking=no ubuntu@$2 < $1
    rm $1
    if [ $i -lt 50 ]
    then
        UE_SYNC=0
        echo "Basic-Sim UE: UE did NOT complete RRC Connection"
    else
        echo "Basic-Sim UE: UE did COMPLETE RRC Connection"
    fi
}

function get_ue_ip_addr {
    local LOC_IF_ID=$3
    echo "ifconfig oaitun_ue${LOC_IF_ID} | egrep \"inet addr\" | sed -e 's#^.*inet addr:##' -e 's#  P-t-P:.*\$##'" > $1
    UE_IP_ADDR=`ssh -T -o StrictHostKeyChecking=no ubuntu@$2 < $1`
    echo "Test UE${LOC_IF_ID} IP Address is : $UE_IP_ADDR"
    rm $1
}

function get_ue_mbms_ip_addr {
    local LOC_IF_ID=$3
    echo "ifconfig oaitun_uem${LOC_IF_ID} | egrep \"inet addr\" | sed -e 's#^.*inet addr:##' -e 's#  P-t-P:.*\$##'" > $1
    UE_IP_ADDR=`ssh -T -o StrictHostKeyChecking=no ubuntu@$2 < $1`
    echo "Test UE${LOC_IF_ID} MBMS IP Address is : $UE_IP_ADDR"
    rm $1
}

function get_enb_noS1_ip_addr {
    echo "ifconfig oaitun_enb1 | egrep \"inet addr\" | sed -e 's#^.*inet addr:##' -e 's#  P-t-P:.*\$##'" > $1
    ENB_IP_ADDR=`ssh -T -o StrictHostKeyChecking=no ubuntu@$2 < $1`
    echo "Test eNB IP Address is : $ENB_IP_ADDR"
    rm $1
}

function get_enb_mbms_noS1_ip_addr {
    echo "ifconfig oaitun_enm1 | egrep \"inet addr\" | sed -e 's#^.*inet addr:##' -e 's#  P-t-P:.*\$##'" > $1
    ENB_IP_ADDR=`ssh -T -o StrictHostKeyChecking=no ubuntu@$2 < $1`
    echo "Test eNB MBMS IP Address is : $ENB_IP_ADDR"
    rm $1
}

function ping_ue_ip_addr {
    local LOC_FG_OR_BG=$5
    echo "echo \"COMMAND IS: ping -c 20 $3\" > $4" > $1
    echo "echo \"ping -c 20 $3\"" >> $1
    if [ $LOC_FG_OR_BG -eq 0 ]
    then
        echo "ping -c 20 $UE_IP_ADDR >> $4 2>&1" >> $1
        echo "tail -3 $4" >> $1
    else
        echo "nohup ping -c 20 $UE_IP_ADDR >> $4 &" >> $1
    fi
    ssh -T -o StrictHostKeyChecking=no ubuntu@$2 < $1
    rm -f $1
}

function ping_epc_ip_addr {
    local LOC_IF_ID=$5
    local LOC_FG_OR_BG=$6
    echo "echo \"COMMAND IS: ping -I oaitun_ue${LOC_IF_ID} -c 20 $3\" > $4" > $1
    echo "echo \"ping -I oaitun_ue${LOC_IF_ID} -c 20 $3\"" >> $1
    if [ $LOC_FG_OR_BG -eq 0 ]
    then
        echo "ping -I oaitun_ue${LOC_IF_ID} -c 20 $3 >> $4 2>&1" >> $1
        echo "tail -3 $4" >> $1
    else
        echo "nohup ping -I oaitun_ue${LOC_IF_ID} -c 20 $3 >> $4 &" >> $1
    fi
    ssh -T -o StrictHostKeyChecking=no ubuntu@$2 < $1
    rm -f $1
}

function ping_enb_ip_addr {
    local LOC_FG_OR_BG=$5
    echo "echo \"COMMAND IS: ping -I oaitun_enb1 -c 20 $3\" > $4" > $1
    echo "echo \"ping -I oaitun_enb1 -c 20 $3\"" >> $1
    if [ $LOC_FG_OR_BG -eq 0 ]
    then
        echo "ping -I oaitun_enb1 -c 20 $3 >> $4 2>&1" >> $1
        echo "tail -3 $4" >> $1
    else
        echo "nohup ping -I oaitun_enb1 -c 20 $3 >> $4 &" >> $1
    fi
    ssh -T -o StrictHostKeyChecking=no ubuntu@$2 < $1
    rm -f $1
}

function check_ping_result {
    local LOC_PING_FILE=$1
    local LOC_NB_PINGS=$2
    if [ -f $LOC_PING_FILE ]
    then
        local FILE_COMPLETE=`egrep -c "ping statistics" $LOC_PING_FILE`
        if [ $FILE_COMPLETE -eq 0 ]
        then
            PING_STATUS=-1
            echo "ping file incomplete"
        else
            local ALL_PACKET_RECEIVED=`egrep -c "$LOC_NB_PINGS received" $LOC_PING_FILE`
            if [ $ALL_PACKET_RECEIVED -eq 1 ]
            then
                echo "got all ping packets"
            else
                LOC_NB_PINGS=$[$2-1]
                ALL_PACKET_RECEIVED=`egrep -c "$LOC_NB_PINGS received" $LOC_PING_FILE`
                if [ $ALL_PACKET_RECEIVED -eq 1 ]
                then
                    echo "got almost all ping packets"
                else
                    echo "got NOT all ping packets"
                    PING_STATUS=-1
                fi
            fi
        fi
    else
        echo "ping file not present"
        PING_STATUS=-1
    fi
}


function check_sa_result {
    local LOC_GNB_LOG=$1
    local LOC_UE_LOG=$2

    #if log files exist
    if [ -f $LOC_GNB_LOG ] && [ -f $LOC_UE_LOG ]
    then

        #gNB  SA test
        #console check
        echo "Checking gNB Log for SA success"
        egrep "Received rrcSetupComplete" $1
        egrep "Received Ack of RA-Msg4\. CBRA procedure succeeded" $1

        #script check
        local RRC_CHECK=`egrep -c "Received rrcSetupComplete" $1`        
        local CBRA_CHECK=`egrep -c "Received Ack of RA-Msg4\. CBRA procedure succeeded" $1`

        #UE SA test
        #console check
        echo 'Checking UE Log for SA success'
        egrep "SIB1 decoded" $2
        #script check
        local SIB1_CHECK=`egrep -c "SIB1 decoded" $2`

        #generate status
        if [ $RRC_CHECK -eq 0 ] || [ $CBRA_CHECK -eq 0 ] || [ $SIB1_CHECK -eq 0 ]
        then
            SA_STATUS=-1
            echo "SA test FAILED, could not find the markers"
        fi
    #case where log files do not exist
    else
        echo "SA test log files not present"
        SA_STATUS=-1        
    fi

}


function check_ra_result {
    local LOC_GNB_LOG=$1
    local LOC_UE_LOG=$2

    #if log files exist
    if [ -f $LOC_GNB_LOG ] && [ -f $LOC_UE_LOG ]
    then

        #gNB RA test
        #console check
        echo "Checking gNB Log for RA success"
        egrep "\[RAPROC\] PUSCH with TC_RNTI (.+) received correctly" $1
        #script check
        local GNB_COMPLETE=`egrep -c "\[RAPROC\] PUSCH with TC_RNTI (.+) received correctly" $1`

        #UE RA test
        #console check
        echo 'Checking UE Log for RA success'
        egrep "\[RAPROC\] RA procedure succeeded" $2
        #script check
        local UE_COMPLETE=`egrep -c "\[RAPROC\] RA procedure succeeded" $2`

        #generate status
        if [ $GNB_COMPLETE -eq 0 ] || [ $UE_COMPLETE -eq 0 ]
        then
            RA_STATUS=-1
            echo "RA test FAILED, could not find the markers"
        fi
    #case where log files do not exist
    else
        echo "RA test log files not present"
        RA_STATUS=-1        
    fi

}

# In DL: iperf server should be on UE side
#                     -B oaitun_ue{j}-IP-Addr
#        iperf client should be on EPC (S1) or eNB (noS1) side
#                     -c oaitun_ue{j}-IP-Addr -B oaitun_enb1-IP-Addr (in noS1)
# In UL: iperf server should be on EPC (S1) or eNB (noS1) side
#                     -B oaitun_enb1-IP-Addr
#        iperf client should be on UE side
#                     -c oaitun_enb1-IP-Addr -B oaitun_ue{j}-IP-Addr (in noS1)
function generic_iperf {
    local LOC_ISERVER_CMD=$1
    local LOC_ISERVER_IP=$2
    local LOC_ISERVER_BOND_IP=$3
    local LOC_ICLIENT_CMD=$4
    local LOC_ICLIENT_IP=$5
    local LOC_ICLIENT_BOND_IP=$6
    local LOC_REQ_BANDWIDTH=$7
    local LOC_BASE_LOG_FILE=$8
    local LOC_PORT_ID=$[$9+5001]
    local LOC_FG_OR_BG=${10}
    # By default the requested bandwidth is in Mbits/sec
    if [[ $LOC_REQ_BANDWIDTH =~ .*K.* ]]
    then
        local IPERF_FORMAT="-fk"
        local FORMATTED_REQ_BW=$LOC_REQ_BANDWIDTH
    else
        local IPERF_FORMAT="-fm"
        local FORMATTED_REQ_BW=${LOC_REQ_BANDWIDTH}"M"
    fi
    # Starting Iperf Server
    echo "iperf -B ${LOC_ISERVER_BOND_IP} -u -s -i 1 ${IPERF_FORMAT} -p ${LOC_PORT_ID}"
    echo "nohup iperf -B ${LOC_ISERVER_BOND_IP} -u -s -i 1 ${IPERF_FORMAT} -p ${LOC_PORT_ID} > ${LOC_BASE_LOG_FILE}_server.txt 2>&1 &" > ${LOC_ISERVER_CMD}
    ssh -T -o StrictHostKeyChecking=no ubuntu@${LOC_ISERVER_IP} < ${LOC_ISERVER_CMD}
    rm ${LOC_ISERVER_CMD}

    # Starting Iperf Client
    echo "iperf -c ${LOC_ISERVER_BOND_IP} -u -t 30 -b ${FORMATTED_REQ_BW} -i 1 ${IPERF_FORMAT} -B ${LOC_ICLIENT_BOND_IP} -p ${LOC_PORT_ID}"
    echo "echo \"COMMAND IS: iperf -c ${LOC_ISERVER_BOND_IP} -u -t 30 -b ${FORMATTED_REQ_BW} -i 1 ${IPERF_FORMAT} -B ${LOC_ICLIENT_BOND_IP} -p ${LOC_PORT_ID}\" > ${LOC_BASE_LOG_FILE}_client.txt" > ${LOC_ICLIENT_CMD}
    if [ $LOC_FG_OR_BG -eq 0 ]
    then
        echo "iperf -c ${LOC_ISERVER_BOND_IP} -u -t 30 -b ${FORMATTED_REQ_BW} -i 1 ${IPERF_FORMAT} -B ${LOC_ICLIENT_BOND_IP} -p ${LOC_PORT_ID} >> ${LOC_BASE_LOG_FILE}_client.txt 2>&1" >> ${LOC_ICLIENT_CMD}
        echo "tail -3 ${LOC_BASE_LOG_FILE}_client.txt | grep -v datagram" >> ${LOC_ICLIENT_CMD}
    else
        echo "nohup iperf -c ${LOC_ISERVER_BOND_IP} -u -t 30 -b ${FORMATTED_REQ_BW} -i 1 ${IPERF_FORMAT} -B ${LOC_ICLIENT_BOND_IP} -p ${LOC_PORT_ID} >> ${LOC_BASE_LOG_FILE}_client.txt 2>&1 &" >> ${LOC_ICLIENT_CMD}
    fi
    ssh -T -o StrictHostKeyChecking=no ubuntu@${LOC_ICLIENT_IP} < ${LOC_ICLIENT_CMD}
    rm -f ${LOC_ICLIENT_CMD}

    # Stopping Iperf Server
    if [ $LOC_FG_OR_BG -eq 0 ]
    then
        echo "killall --signal SIGKILL iperf"
        echo "killall --signal SIGKILL iperf" > ${LOC_ISERVER_CMD}
        ssh -T -o StrictHostKeyChecking=no ubuntu@${LOC_ISERVER_IP} < ${LOC_ISERVER_CMD}
        rm ${LOC_ISERVER_CMD}
    fi
}

function check_iperf {
    local LOC_BASE_LOG=$1
    local LOC_REQ_BW=`echo $2 | sed -e "s#K##"`
    local LOC_REQ_BW_MINUS_ONE=`echo "$LOC_REQ_BW - 1" | bc -l`
    local LOC_REQ_BW_MINUS_TWO=`echo "$LOC_REQ_BW - 2" | bc -l`
    local LOC_REQ_BW_MINUS_THREE=`echo "$LOC_REQ_BW - 3" | bc -l`
    local LOC_IS_DL=`echo $LOC_BASE_LOG | grep -c _dl`
    local LOC_IS_BASIC_SIM=`echo $LOC_BASE_LOG | grep -c basic_sim`
    local LOC_IS_RF_SIM=`echo $LOC_BASE_LOG | grep -c rf_sim`
    local LOC_IS_NR=`echo $LOC_BASE_LOG | grep -c _106prb`
    if [ -f ${LOC_BASE_LOG}_client.txt ]
    then
        local FILE_COMPLETE=`egrep -c "Server Report" ${LOC_BASE_LOG}_client.txt`
        if [ $FILE_COMPLETE -eq 0 ]
        then
            IPERF_STATUS=-1
            echo "File Report not found"
        else
            if [ `egrep -c "Mbits/sec" ${LOC_BASE_LOG}_client.txt` -ne 0 ]
            then
                local EFFECTIVE_BANDWIDTH=`tail -n3 ${LOC_BASE_LOG}_client.txt | egrep "Mbits/sec" | sed -e "s#^.*MBytes *##" -e "s#sec.*#sec#"`
                local BW_PREFIX="M"
            fi
            if [ `egrep -c "Kbits/sec" ${LOC_BASE_LOG}_client.txt` -ne 0 ]
            then
                local EFFECTIVE_BANDWIDTH=`tail -n3 ${LOC_BASE_LOG}_client.txt | egrep "Kbits/sec" | sed -e "s#^.*KBytes *##" -e "s#sec.*#sec#"`
                local BW_PREFIX="K"
            fi
            if [ $LOC_IS_DL -eq 1 ] && [ $LOC_IS_BASIC_SIM -eq 1 ]
            then
                if [[ $EFFECTIVE_BANDWIDTH =~ .*${LOC_REQ_BW}.*${BW_PREFIX}bits.* ]] || [[ $EFFECTIVE_BANDWIDTH =~ .*${LOC_REQ_BW_MINUS_ONE}.*${BW_PREFIX}bits.* ]] || [[ $EFFECTIVE_BANDWIDTH =~ .*${LOC_REQ_BW_MINUS_TWO}.*${BW_PREFIX}bits.* ]] || [[ $EFFECTIVE_BANDWIDTH =~ .*${LOC_REQ_BW_MINUS_THREE}.*${BW_PREFIX}bits.* ]]
                then
                    echo "got requested DL bandwidth: $EFFECTIVE_BANDWIDTH"
                else
                    echo "got LESS than requested DL bandwidth: $EFFECTIVE_BANDWIDTH"
                    IPERF_STATUS=-1
                fi
            else
                if [[ $EFFECTIVE_BANDWIDTH =~ .*${LOC_REQ_BW}.*${BW_PREFIX}bits.* ]] || [[ $EFFECTIVE_BANDWIDTH =~ .*${LOC_REQ_BW_MINUS_ONE}.*${BW_PREFIX}bits.* ]]
                then
                    if [ $LOC_IS_DL -eq 1 ]
                    then
                        echo "got requested DL bandwidth: $EFFECTIVE_BANDWIDTH"
                    else
                        echo "got requested UL bandwidth: $EFFECTIVE_BANDWIDTH"
                    fi
                else
                    echo "got LESS than requested DL bandwidth: $EFFECTIVE_BANDWIDTH"
                    IPERF_STATUS=-1
                fi
            fi
        fi
    else
        IPERF_STATUS=-1
        echo "File not found"
    fi
}

function terminate_enb_ue_basic_sim {
    # mode = 0 : eNB + UE or gNB and NR-UE
    # mode = 1 : eNB or gNB
    # mode = 2 : UE or NR-UE
    local LOC_MODE=$3
    echo "NB_OAI_PROCESSES=\`ps -aux | grep modem | grep -v grep | grep -c softmodem\`" > $1
    if [ $LOC_MODE -eq 0 ] || [ $LOC_MODE -eq 1 ]
    then
        echo "if [ \$NB_OAI_PROCESSES -ne 0 ]; then echo \"sudo killall -r --signal SIGINT .*-softmodem\"; fi" >> $1
        echo "if [ \$NB_OAI_PROCESSES -ne 0 ]; then sudo killall -r --signal SIGINT .*-softmodem; fi" >> $1
        echo "if [ \$NB_OAI_PROCESSES -ne 0 ]; then sleep 5; fi" >> $1
        echo "echo \"ps -aux | grep softmodem\"" >> $1
        echo "ps -aux | grep softmodem | grep -v grep" >> $1
        echo "NB_OAI_PROCESSES=\`ps -aux | grep modem | grep -v grep | grep -c softmodem\`" >> $1
        echo "if [ \$NB_OAI_PROCESSES -ne 0 ]; then echo \"sudo killall -r --signal SIGKILL .*-softmodem\"; fi" >> $1
        echo "if [ \$NB_OAI_PROCESSES -ne 0 ]; then sudo killall -r --signal SIGKILL .*-softmodem; fi" >> $1
    fi
    if [ $LOC_MODE -eq 0 ] || [ $LOC_MODE -eq 2 ]
    then
        echo "if [ \$NB_OAI_PROCESSES -ne 0 ]; then echo \"sudo killall -r --signal SIGKILL .*-uesoftmodem\"; fi" >> $1
        echo "if [ \$NB_OAI_PROCESSES -ne 0 ]; then sudo killall -r --signal SIGKILL .*-uesoftmodem; fi" >> $1
        echo "if [ \$NB_OAI_PROCESSES -ne 0 ]; then sleep 5; fi" >> $1
    fi
    echo "echo \"ps -aux | grep softmodem\"" >> $1
    echo "ps -aux | grep softmodem | grep -v grep" >> $1
    ssh -T -o StrictHostKeyChecking=no ubuntu@$2 < $1
    rm -f $1
}

function recover_core_dump {
    local IS_SEG_FAULT=`egrep -ic "segmentation fault" $3`
    if [ $IS_SEG_FAULT -ne 0 ]
    then
        local TC=`echo $3 | sed -e "s#^.*enb_##" -e "s#Hz.*#Hz#"`
        echo "Segmentation fault detected on enb -> recovering core dump"
        echo "cd /home/ubuntu/tmp/cmake_targets/ran_build/build/" > $1
        echo "sync" >> $1
        echo "sudo tar -cjhf basic-simulator-enb-core-${TC}.bz2 core lte-softmodem *.so ci-lte-basic-sim.conf my-lte-softmodem-run.sh" >> $1
        echo "sudo rm core" >> $1
        echo "rm ci-lte-basic-sim.conf" >> $1
        echo "sync" >> $1
        ssh -T -o StrictHostKeyChecking=no ubuntu@$2 < $1
        scp -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/ran_build/build/basic-simulator-enb-core-${TC}.bz2 $4
        rm -f $1
    fi
}

function full_terminate {
    echo "############################################################"
    echo "Terminate enb/ue simulators"
    echo "############################################################"
    terminate_enb_ue_basic_sim $VM_CMDS $VM_IP_ADDR 0
    scp -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/$CURRENT_ENB_LOG_FILE $ARCHIVES_LOC
    scp -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/$CURRENT_UE_LOG_FILE $ARCHIVES_LOC
    recover_core_dump $VM_CMDS $VM_IP_ADDR $ARCHIVES_LOC/$CURRENT_ENB_LOG_FILE $ARCHIVES_LOC
    sleep 10
}

function full_basic_sim_destroy {
    if [ $KEEP_VM_ALIVE -eq 0 ]
    then
        echo "############################################################"
        echo "Destroying VMs"
        echo "############################################################"
        uvt-kvm destroy $VM_NAME
        ssh-keygen -R $VM_IP_ADDR
        if [ -e $JENKINS_WKSP/flexran/flexran_build_complete.txt ]
        then
            uvt-kvm destroy $FLEXRAN_CTL_VM_NAME
            ssh-keygen -R $FLEXRAN_CTL_VM_IP_ADDR
        fi
    fi
}

function install_epc_on_vm {
    local LOC_EPC_VM_NAME=$1
    local LOC_EPC_VM_CMDS=$2

    if [ -d /opt/ltebox-archives/ ]
    then
        # Checking if all ltebox archives are available to run ltebx epc on a brand new VM
        if [ -f /opt/ltebox-archives/ltebox_2.2.70_16_04_amd64.deb ] && [ -f /opt/ltebox-archives/etc-conf.zip ] && [ -f /opt/ltebox-archives/hss-sim-develop.zip ]
        then
            echo "############################################################"
            echo "Test EPC on VM ($EPC_VM_NAME) will be using ltebox"
            echo "############################################################"
            LTEBOX=1
        fi
    fi
    # Here we could have other types of EPC detection

    # Do we need to start the EPC VM
    echo "EPC_VM_CMD_FILE     = $LOC_EPC_VM_CMDS"
    IS_EPC_VM_ALIVE=`uvt-kvm list | grep -c $LOC_EPC_VM_NAME`
    if [ $IS_EPC_VM_ALIVE -eq 0 ]
    then
        echo "############################################################"
        echo "Creating test EPC VM ($LOC_EPC_VM_NAME) on Ubuntu Cloud Image base"
        echo "############################################################"
        acquire_vm_create_lock
        uvt-kvm create $LOC_EPC_VM_NAME release=$VM_OSREL --unsafe-caching
        echo "Waiting for VM to be started"
        uvt-kvm wait $LOC_EPC_VM_NAME --insecure
        release_vm_create_lock
    else
        echo "Waiting for VM to be started"
        uvt-kvm wait $LOC_EPC_VM_NAME --insecure
    fi

    local LOC_EPC_VM_IP_ADDR=`uvt-kvm ip $LOC_EPC_VM_NAME`

    echo "$LOC_EPC_VM_NAME has for IP addr = $LOC_EPC_VM_IP_ADDR"
    [ -f /etc/apt/apt.conf.d/01proxy ] && scp -o StrictHostKeyChecking=no /etc/apt/apt.conf.d/01proxy ubuntu@$LOC_EPC_VM_IP_ADDR:/home/ubuntu

    # ltebox specific actions (install and start)
    LTE_BOX_TO_INSTALL=1
    if [ $LTEBOX -eq 1 ]
    then
        echo "ls -ls /opt/ltebox/tools/start_ltebox" > $LOC_EPC_VM_CMDS
        RESPONSE=`ssh -T -o StrictHostKeyChecking=no ubuntu@$LOC_EPC_VM_IP_ADDR < $LOC_EPC_VM_CMDS`
        NB_EXES=`echo $RESPONSE | grep -c ltebox`
        if [ $NB_EXES -eq 1 ]; then LTE_BOX_TO_INSTALL=0; fi
    fi

    if [ $LTEBOX -eq 1 ] && [ $LTE_BOX_TO_INSTALL -eq 1 ]
    then
        echo "############################################################"
        echo "Copying ltebox archives into EPC VM ($LOC_EPC_VM_NAME)" 
        echo "############################################################"
        scp -o StrictHostKeyChecking=no /opt/ltebox-archives/ltebox_2.2.70_16_04_amd64.deb ubuntu@$LOC_EPC_VM_IP_ADDR:/home/ubuntu
        scp -o StrictHostKeyChecking=no /opt/ltebox-archives/etc-conf.zip ubuntu@$LOC_EPC_VM_IP_ADDR:/home/ubuntu
        scp -o StrictHostKeyChecking=no /opt/ltebox-archives/hss-sim-develop.zip ubuntu@$LOC_EPC_VM_IP_ADDR:/home/ubuntu

        echo "############################################################"
        echo "Install EPC on EPC VM ($LOC_EPC_VM_NAME)"
        echo "############################################################"
        echo "[ -f 01proxy ] && sudo cp 01proxy /etc/apt/apt.conf.d/" > $LOC_EPC_VM_CMDS
        echo "touch /home/ubuntu/.hushlogin" >> $LOC_EPC_VM_CMDS
        echo "echo \"sudo apt-get --yes --quiet install zip openjdk-8-jre libconfuse-dev libreadline-dev liblog4c-dev libgcrypt-dev libsctp-dev python2.7 python2.7-dev iperf\"" >> $LOC_EPC_VM_CMDS
        echo "sudo apt-get update > zip-install.txt 2>&1" >> $LOC_EPC_VM_CMDS
        echo "sudo apt-get --yes install zip openjdk-8-jre libconfuse-dev libreadline-dev liblog4c-dev libgcrypt-dev libsctp-dev python2.7 python2.7-dev iperf >> zip-install.txt 2>&1" >> $LOC_EPC_VM_CMDS

        # Installing HSS
        echo "echo \"cd /opt\"" >> $LOC_EPC_VM_CMDS
        echo "cd /opt" >> $LOC_EPC_VM_CMDS
        echo "echo \"sudo unzip -qq /home/ubuntu/hss-sim-develop.zip\"" >> $LOC_EPC_VM_CMDS
        echo "sudo unzip -qq /home/ubuntu/hss-sim-develop.zip" >> $LOC_EPC_VM_CMDS
        echo "echo \"cd /opt/hss_sim0609\"" >> $LOC_EPC_VM_CMDS
        echo "cd /opt/hss_sim0609" >> $LOC_EPC_VM_CMDS

        # Installing ltebox
        echo "echo \"cd /home/ubuntu\"" >> $LOC_EPC_VM_CMDS
        echo "cd /home/ubuntu" >> $LOC_EPC_VM_CMDS
        echo "echo \"sudo dpkg -i ltebox_2.2.70_16_04_amd64.deb \"" >> $LOC_EPC_VM_CMDS
        echo "sudo dpkg -i ltebox_2.2.70_16_04_amd64.deb >> zip-install.txt 2>&1" >> $LOC_EPC_VM_CMDS

        echo "echo \"cd /opt/ltebox/etc/\"" >> $LOC_EPC_VM_CMDS
        echo "cd /opt/ltebox/etc/" >> $LOC_EPC_VM_CMDS
        echo "echo \"sudo unzip -qq -o /home/ubuntu/etc-conf.zip\"" >> $LOC_EPC_VM_CMDS
        echo "sudo unzip -qq -o /home/ubuntu/etc-conf.zip" >> $LOC_EPC_VM_CMDS
        echo "sudo sed -i  -e 's#EPC_VM_IP_ADDRESS#$LOC_EPC_VM_IP_ADDR#' gw.conf" >> $LOC_EPC_VM_CMDS
        echo "sudo sed -i  -e 's#EPC_VM_IP_ADDRESS#$LOC_EPC_VM_IP_ADDR#' mme.conf" >> $LOC_EPC_VM_CMDS

        ssh -T -o StrictHostKeyChecking=no ubuntu@$LOC_EPC_VM_IP_ADDR < $LOC_EPC_VM_CMDS
        rm -f $LOC_EPC_VM_CMDS
    fi
}

function add_user_to_epc_lists {
    local LOC_EPC_VM_CMDS=$1
    local LOC_EPC_VM_IP_ADDR=$2
    local LOC_NB_USERS=$3
    if [ $LTEBOX -eq 1 ]
    then
        scp -o StrictHostKeyChecking=no $JENKINS_WKSP/ci-scripts/add_user_to_subscriber_list.awk ubuntu@$LOC_EPC_VM_IP_ADDR:/home/ubuntu/
        echo "cd /opt/hss_sim0609" > $LOC_EPC_VM_CMDS
        echo "if [ -e subscriber.data.orig ]; then sudo mv subscriber.data.orig subscriber.data; fi" >> $1
        echo "if [ -e profile.data.orig ]; then sudo mv profile.data.orig profile.data; fi" >> $1
        echo "sudo cp subscriber.data subscriber.data.orig" >> $LOC_EPC_VM_CMDS
        echo "sudo cp profile.data profile.data.orig" >> $LOC_EPC_VM_CMDS
        echo "sudo awk -v num_ues=$LOC_NB_USERS -f /home/ubuntu/add_user_to_subscriber_list.awk subscriber.data.orig > /tmp/subscriber.data" >> $LOC_EPC_VM_CMDS
        echo "sudo awk -v num_ues=$LOC_NB_USERS -f /home/ubuntu/add_user_to_subscriber_list.awk profile.data.orig > /tmp/profile.data" >> $LOC_EPC_VM_CMDS
        echo "sudo cp /tmp/subscriber.data subscriber.data" >> $LOC_EPC_VM_CMDS
        echo "sudo cp /tmp/profile.data profile.data" >> $LOC_EPC_VM_CMDS

        ssh -T -o StrictHostKeyChecking=no ubuntu@$LOC_EPC_VM_IP_ADDR < $LOC_EPC_VM_CMDS
        rm -f $LOC_EPC_VM_CMDS
    fi
}

function start_epc {
    local LOC_EPC_VM_NAME=$1
    local LOC_EPC_VM_CMDS=$2
    local LOC_EPC_VM_IP_ADDR=$3

    if [ $LTEBOX -eq 1 ]
    then
        echo "############################################################"
        echo "Start EPC on EPC VM ($LOC_EPC_VM_NAME)"
        echo "############################################################"
        echo "echo \"cd /opt/hss_sim0609\"" > $LOC_EPC_VM_CMDS
        echo "cd /opt/hss_sim0609" >> $LOC_EPC_VM_CMDS
        echo "sudo rm -f hss.log" >> $LOC_EPC_VM_CMDS
        echo "echo \"screen -dm -S simulated_hss ./starthss_real\"" >> $LOC_EPC_VM_CMDS
        echo "sudo su -c \"screen -dm -S simulated_hss ./starthss_real\"" >> $LOC_EPC_VM_CMDS

        echo "echo \"cd /opt/ltebox/tools/\"" >> $LOC_EPC_VM_CMDS
        echo "cd /opt/ltebox/tools/" >> $LOC_EPC_VM_CMDS
        echo "echo \"sudo ./start_ltebox\"" >> $LOC_EPC_VM_CMDS
        echo "nohup sudo ./start_ltebox > /home/ubuntu/ltebox.txt" >> $LOC_EPC_VM_CMDS
        echo "touch /home/ubuntu/try.txt" >> $LOC_EPC_VM_CMDS
        echo "sudo rm -f /home/ubuntu/*.txt" >> $LOC_EPC_VM_CMDS

        ssh -T -o StrictHostKeyChecking=no ubuntu@$LOC_EPC_VM_IP_ADDR < $LOC_EPC_VM_CMDS
        rm -f $LOC_EPC_VM_CMDS

        i="0"
        echo "ifconfig tun5 | egrep -c \"inet addr\"" > $LOC_EPC_VM_CMDS
        while [ $i -lt 10 ]
        do
            sleep 2
            CONNECTED=`ssh -T -o StrictHostKeyChecking=no ubuntu@$LOC_EPC_VM_IP_ADDR < $LOC_EPC_VM_CMDS`
            if [ $CONNECTED -eq 1 ]
            then
                i="100"
            else
                i=$[$i+1]
            fi
        done
        rm $LOC_EPC_VM_CMDS
        if [ $i -lt 50 ]
        then
            echo "Problem w/ starting ltebox EPC"
            echo "TEST_KO" > $ARCHIVES_LOC/test_final_status.log
            exit -1
        fi
    fi

    # HERE ADD ANY INSTALL ACTIONS FOR ANOTHER EPC

}

function retrieve_real_epc_ip_addr {
    local LOC_EPC_VM_NAME=$1
    local LOC_EPC_VM_CMDS=$2
    local LOC_EPC_VM_IP_ADDR=$3

    if [[ "$EPC_IPADDR" == "" ]]
    then
        if [ $LTEBOX -eq 1 ]
        then
            # in our configuration file, we are using pool 5
            echo "ifconfig tun5 | egrep \"inet addr\" | sed -e 's#^.*inet addr:##' -e 's#  P-t-P:.*\$##'" > $LOC_EPC_VM_CMDS
            REAL_EPC_IP_ADDR=`ssh -T -o StrictHostKeyChecking=no ubuntu@$LOC_EPC_VM_IP_ADDR < $LOC_EPC_VM_CMDS`
            rm $LOC_EPC_VM_CMDS
        fi
    else
        REAL_EPC_IP_ADDR=$EPC_TUN_IPADDR
    fi
    echo "EPC IP Address     is : $REAL_EPC_IP_ADDR"
}

function terminate_epc {
    if [ $LTEBOX -eq 1 ]
    then
        echo "echo \"cd /opt/ltebox/tools\"" > $1
        echo "cd /opt/ltebox/tools" >> $1
        echo "echo \"sudo ./stop_ltebox\"" >> $1
        echo "sudo ./stop_ltebox" >> $1
        echo "echo \"sudo killall --signal SIGKILL hss_sim\"" >> $1
        echo "sudo killall --signal SIGKILL hss_sim" >> $1
        ssh -T -o StrictHostKeyChecking=no ubuntu@$2 < $1
        rm $1
    fi
}

function start_flexran_ctrl {
    echo "cd /home/ubuntu/tmp" > $1
    echo "if [ -f cmake_targets/log/flexran_ctl_run.log ]; then rm -f cmake_targets/log/flexran_ctl_run.log cmake_targets/log/flexran_ctl_query*.log; fi" >> $1
    echo "echo \" sudo build/rt_controller -c log_config/basic_log\"" >> $1
    echo "nohup sudo build/rt_controller -c log_config/basic_log > cmake_targets/log/flexran_ctl_run.log 2>&1 &" >> $1
    ssh -T -o StrictHostKeyChecking=no ubuntu@$2 < $1
    rm $1
    sleep 10
}

function stop_flexran_ctrl {
    echo "echo \"sudo killall --signal SIGKILL rt_controller\"" > $1
    echo "sudo killall --signal SIGKILL rt_controller" > $1
    ssh -T -o StrictHostKeyChecking=no ubuntu@$2 < $1
    rm $1
    sleep 2
}

function query_flexran_ctrl_status {
    local LOC_MESSAGE=$3
    echo "cd /home/ubuntu/tmp" > $1
    echo "echo \"------------------------------------------------------------\" > cmake_targets/log/flexran_ctl_query_${LOC_MESSAGE}.log" >> $1
    echo "echo \"LOG_NAME: $LOC_MESSAGE\" >> cmake_targets/log/flexran_ctl_query_${LOC_MESSAGE}.log" >> $1
    echo "echo \"------------------------------------------------------------\" >> cmake_targets/log/flexran_ctl_query_${LOC_MESSAGE}.log" >> $1
    echo "curl http://localhost:9999/stats | jq '.' | tee -a cmake_targets/log/flexran_ctl_query_${LOC_MESSAGE}.log" >> $1
    ssh -T -o StrictHostKeyChecking=no ubuntu@$2 < $1
    rm $1
}

function build_ue_on_separate_folder {
    echo "mkdir tmp-ue" > $1
    echo "cd tmp-ue" >> $1
    echo "echo \"unzip -qq -DD ../localZip.zip\"" >> $1
    echo "unzip -qq -DD ../localZip.zip" >> $1
    echo "echo \"source oaienv\"" >> $1
    echo "source oaienv" >> $1
    echo "cd cmake_targets/" >> $1
    echo "mkdir log" >> $1
    echo "chmod 777 log" >> $1
    echo "echo \"./build_oai --UE \"" >> $1
    echo "./build_oai --UE > log/ue-build.txt 2>&1" >> $1
    echo "cd tools" >> $1
    echo "sudo ifconfig lo: 127.0.0.2 netmask 255.0.0.0 up" >> $1
    echo "sudo chmod 666 /etc/iproute2/rt_tables" >> $1
    echo "source init_nas_s1 UE" >> $1
    echo "ifconfig" >> $1
    ssh -T -o StrictHostKeyChecking=no ubuntu@$2 < $1
    rm -f $1
}

function start_l2_sim_enb {
    local LOC_ENB_VM_IP_ADDR=$2
    local LOC_EPC_IP_ADDR=$3
    local LOC_UE_VM_IP_ADDR=$4
    local LOC_LOG_FILE=$5
    local LOC_NB_RBS=$6
    local LOC_CONF_FILE=$7
    # 1 is with S1 and 0 without S1 aka noS1
    local LOC_S1_CONFIGURATION=$8
    echo "cd /home/ubuntu/tmp" > $1
    echo "echo \"sudo apt-get --yes --quiet install daemon \"" >> $1
    echo "sudo apt-get --yes install daemon >> /home/ubuntu/tmp/cmake_targets/log/daemon-install.txt 2>&1" >> $1
    echo "echo \"source oaienv\"" >> $1
    echo "source oaienv" >> $1
    echo "cd ci-scripts/conf_files/" >> $1
    echo "cp $LOC_CONF_FILE ci-$LOC_CONF_FILE" >> $1
    echo "sed -i -e 's#N_RB_DL.*=.*;#N_RB_DL                                         = $LOC_NB_RBS;#' -e 's#CI_MME_IP_ADDR#$LOC_EPC_IP_ADDR#' -e 's#CI_ENB_IP_ADDR#$LOC_ENB_VM_IP_ADDR#' -e 's#CI_UE_IP_ADDR#$LOC_UE_VM_IP_ADDR#' ci-$LOC_CONF_FILE" >> $1
    echo "echo \"grep N_RB_DL ci-$LOC_CONF_FILE\"" >> $1
    echo "grep N_RB_DL ci-$LOC_CONF_FILE | sed -e 's#N_RB_DL.*=#N_RB_DL =#'" >> $1
    echo "echo \"cd /home/ubuntu/tmp/cmake_targets/ran_build/build/\"" >> $1
    echo "sudo chmod 777 /home/ubuntu/tmp/cmake_targets/ran_build/build/" >> $1
    echo "cd /home/ubuntu/tmp/cmake_targets/ran_build/build/" >> $1
    if [ $LOC_S1_CONFIGURATION -eq 0 ]
    then
        echo "echo \"ulimit -c unlimited && ./lte-softmodem -O /home/ubuntu/tmp/ci-scripts/conf_files/ci-$LOC_CONF_FILE --log_config.global_log_options level,nocolor --noS1\" > ./my-lte-softmodem-run.sh " >> $1
    else
        echo "echo \"ulimit -c unlimited && ./lte-softmodem -O /home/ubuntu/tmp/ci-scripts/conf_files/ci-$LOC_CONF_FILE --log_config.global_log_options level,nocolor \" > ./my-lte-softmodem-run.sh " >> $1
    fi
    echo "chmod 775 ./my-lte-softmodem-run.sh" >> $1
    echo "cat ./my-lte-softmodem-run.sh" >> $1
    echo "if [ -e /home/ubuntu/tmp/cmake_targets/log/$LOC_LOG_FILE ]; then sudo sudo rm -f /home/ubuntu/tmp/cmake_targets/log/$LOC_LOG_FILE; fi" >> $1
    echo "sudo -E daemon --inherit --unsafe --name=enb_daemon --chdir=/home/ubuntu/tmp/cmake_targets/ran_build/build/ -o /home/ubuntu/tmp/cmake_targets/log/$LOC_LOG_FILE ./my-lte-softmodem-run.sh" >> $1

    ssh -T -o StrictHostKeyChecking=no ubuntu@$LOC_ENB_VM_IP_ADDR < $1
    rm $1

    local i="0"
    echo "egrep -c \"Waiting for PHY_config_req\" /home/ubuntu/tmp/cmake_targets/log/$LOC_LOG_FILE" > $1
    while [ $i -lt 10 ]
    do
        sleep 5
        CONNECTED=`ssh -T -o StrictHostKeyChecking=no ubuntu@$LOC_ENB_VM_IP_ADDR < $1`
        if [ $CONNECTED -ne 0 ]
        then
            i="100"
        else
            i=$[$i+1]
        fi
    done
    echo "echo \"free -m\"" > $1
    echo "free -m" >> $1
    ssh -T -o StrictHostKeyChecking=no ubuntu@$LOC_ENB_VM_IP_ADDR < $1
    rm $1
    ENB_SYNC=1
    if [ $i -lt 50 ]
    then
        ENB_SYNC=0
        echo "L2-SIM eNB is NOT sync'ed: process still alive?"
    else
        echo "L2-SIM eNB is sync'ed: waiting for UE(s) to connect"
    fi
    if [ $LOC_S1_CONFIGURATION -eq 0 ]
    then
        echo "ifconfig oaitun_enb1 | egrep -c \"inet addr\"" > $1
        # Checking oaitun_enb1 interface has now an IP address
        i="0"
        while [ $i -lt 10 ]
        do
            CONNECTED=`ssh -T -o StrictHostKeyChecking=no ubuntu@$LOC_ENB_VM_IP_ADDR < $1`
            if [ $CONNECTED -eq 1 ]
            then
                i="100"
            else
                i=$[$i+1]
                sleep 5
            fi
        done
        rm $1
        if [ $i -lt 50 ]
        then
            ENB_SYNC=0
            echo "L2-SIM eNB oaitun_enb1 is DOWN or NOT CONFIGURED"
        else
            echo "L2-SIM eNB oaitun_enb1 is UP and CONFIGURED"
        fi
    fi
    sleep 10
}

function add_ue_l2_sim_ue {
    local LOC_UE_VM_IP_ADDR=$2
    local LOC_NB_UES=$3
    echo "cd /home/ubuntu/tmp/" > $1
    echo "source oaienv" >> $1
    if [ $LOC_NB_UES -gt 1 ]
    then
        echo "echo \"cd openair3/NAS/TOOLS/\"" >> $1
        echo "cd openair3/NAS/TOOLS/" >> $1
        echo "echo \"awk -v num_ues=$LOC_NB_UES -f /home/ubuntu/tmp/ci-scripts/add_user_to_conf_file.awk ue_eurecom_test_sfr.conf > ue_eurecom_test_sfr_multi_ues.conf\"" >> $1
        echo "awk -v num_ues=$LOC_NB_UES -f /home/ubuntu/tmp/ci-scripts/add_user_to_conf_file.awk ue_eurecom_test_sfr.conf > ue_eurecom_test_sfr_multi_ues.conf" >> $1
    fi
    echo "echo \"cd /home/ubuntu/tmp/cmake_targets/ran_build/build/\"" >> $1
    echo "cd /home/ubuntu/tmp/cmake_targets/ran_build/build/" >> $1
    echo "sudo rm -f *.u*" >> $1
    if [ $LOC_NB_UES -eq 1 ]
    then
        echo "echo \"sudo ../../../targets/bin/conf2uedata -c ../../../openair3/NAS/TOOLS/ue_eurecom_test_sfr.conf -o .\"" >> $1
        echo "sudo ../../../targets/bin/conf2uedata -c ../../../openair3/NAS/TOOLS/ue_eurecom_test_sfr.conf -o . > /home/ubuntu/tmp/cmake_targets/log/ue_adapt.txt 2>&1" >> $1
    else
        echo "echo \"sudo ../../../targets/bin/conf2uedata -c ../../../openair3/NAS/TOOLS/ue_eurecom_test_sfr_multi_ues.conf -o .\"" >> $1
        echo "sudo ../../../targets/bin/conf2uedata -c ../../../openair3/NAS/TOOLS/ue_eurecom_test_sfr_multi_ues.conf -o . > /home/ubuntu/tmp/cmake_targets/log/ue_adapt.txt 2>&1" >> $1
    fi

    ssh -T -o StrictHostKeyChecking=no ubuntu@$LOC_UE_VM_IP_ADDR < $1
    rm $1
}

function start_l2_sim_ue {
    local LOC_UE_VM_IP_ADDR=$2
    local LOC_ENB_VM_IP_ADDR=$3
    local LOC_LOG_FILE=$4
    local LOC_CONF_FILE=$5
    local LOC_NB_UES=$6
    # 1 is with S1 and 0 without S1 aka noS1
    local LOC_S1_CONFIGURATION=$7
    echo "echo \"sudo apt-get --yes --quiet install daemon \"" > $1
    echo "sudo apt-get --yes install daemon >> /home/ubuntu/tmp/cmake_targets/log/daemon-install.txt 2>&1" >> $1
    echo "cd /home/ubuntu/tmp/ci-scripts/conf_files/" >> $1
    echo "cp $LOC_CONF_FILE ci-$LOC_CONF_FILE" >> $1
    echo "sed -i -e 's#CI_ENB_IP_ADDR#$LOC_ENB_VM_IP_ADDR#' -e 's#CI_UE_IP_ADDR#$LOC_UE_VM_IP_ADDR#' ci-$LOC_CONF_FILE" >> $1
    echo "echo \"cd /home/ubuntu/tmp/cmake_targets/ran_build/build/\"" >> $1
    echo "sudo chmod 777 /home/ubuntu/tmp/cmake_targets/ran_build/build/" >> $1
    echo "cd /home/ubuntu/tmp/cmake_targets/ran_build/build/" >> $1
    if [ $LOC_S1_CONFIGURATION -eq 0 ]
    then
        echo "echo \"ulimit -c unlimited && ./lte-uesoftmodem -O /home/ubuntu/tmp/ci-scripts/conf_files/ci-$LOC_CONF_FILE --L2-emul 3 --num-ues $LOC_NB_UES --nums_ue_thread 1 --nokrnmod 1 --log_config.global_log_options level,nocolor --noS1\" > ./my-lte-softmodem-run.sh " >> $1
    else
        echo "echo \"ulimit -c unlimited && ./lte-uesoftmodem -O /home/ubuntu/tmp/ci-scripts/conf_files/ci-$LOC_CONF_FILE --L2-emul 3 --num-ues $LOC_NB_UES --nums_ue_thread 1 --nokrnmod 1 --log_config.global_log_options level,nocolor\" > ./my-lte-softmodem-run.sh " >> $1
    fi
    echo "chmod 775 ./my-lte-softmodem-run.sh" >> $1
    echo "cat ./my-lte-softmodem-run.sh" >> $1
    echo "if [ -e /home/ubuntu/tmp/cmake_targets/log/$LOC_LOG_FILE ]; then sudo sudo rm -f /home/ubuntu/tmp/cmake_targets/log/$LOC_LOG_FILE; fi" >> $1
    echo "sudo -E daemon --inherit --unsafe --name=ue_daemon --chdir=/home/ubuntu/tmp/cmake_targets/ran_build/build/ -o /home/ubuntu/tmp/cmake_targets/log/$LOC_LOG_FILE ./my-lte-softmodem-run.sh" >> $1

    ssh -T -o StrictHostKeyChecking=no ubuntu@$LOC_UE_VM_IP_ADDR < $1
    rm $1

    local i="0"
    echo "egrep -c \"Received NFAPI_START_REQ\" /home/ubuntu/tmp/cmake_targets/log/$LOC_LOG_FILE" > $1
    while [ $i -lt 10 ]
    do
        sleep 5
        CONNECTED=`ssh -T -o StrictHostKeyChecking=no ubuntu@$LOC_UE_VM_IP_ADDR < $1`
        if [ $CONNECTED -eq 1 ]
        then
            i="100"
        else
            i=$[$i+1]
        fi
    done
    echo "echo \"free -m\"" > $1
    echo "free -m" >> $1
    ssh -T -o StrictHostKeyChecking=no ubuntu@$LOC_UE_VM_IP_ADDR < $1
    rm $1
    UE_SYNC=1
    if [ $i -lt 50 ]
    then
        UE_SYNC=0
        echo "L2-SIM UE is NOT sync'ed w/ eNB"
        return
    else
        echo "L2-SIM UE is sync'ed w/ eNB"
    fi
    local max_interfaces_to_check=$LOC_NB_UES
    #local max_interfaces_to_check=1
    #if [ $LOC_S1_CONFIGURATION -eq 0 ]; then max_interfaces_to_check=$LOC_NB_UES; fi
    local j="1"
    while [ $j -le $max_interfaces_to_check ]
    do
        echo "ifconfig oaitun_ue${j} | egrep -c \"inet addr\"" > $1
        # Checking oaitun_ue1 interface has now an IP address
        i="0"
        while [ $i -lt 10 ]
        do
            CONNECTED=`ssh -T -o StrictHostKeyChecking=no ubuntu@$LOC_UE_VM_IP_ADDR < $1`
            if [ $CONNECTED -eq 1 ]
            then
                i="100"
            else
                i=$[$i+1]
                sleep 5
            fi
        done
        rm $1
        if [ $i -lt 50 ]
        then
            UE_SYNC=0
            echo "L2-SIM UE oaitun_ue${j} is DOWN or NOT CONFIGURED"
        else
            echo "L2-SIM UE oaitun_ue${j} is UP and CONFIGURED"
        fi
        j=$[$j+1]
    done
    sleep 10
    # for debug
    if [ $LOC_S1_CONFIGURATION -eq 1 ]
    then
        echo "ifconfig" > $1
        ssh -T -o StrictHostKeyChecking=no ubuntu@$LOC_UE_VM_IP_ADDR < $1
        rm $1
    else
        echo "Setting Routes for all UEs"
        echo "cd /home/ubuntu/tmp/cmake_targets/tools" > $1
        echo "./setup_routes.sh $LOC_NB_UES" >> $1
        ssh -T -o StrictHostKeyChecking=no ubuntu@$LOC_UE_VM_IP_ADDR < $1
        rm $1
    fi
}

function full_l2_sim_destroy {
    if [ $KEEP_VM_ALIVE -eq 0 ]
    then
        echo "############################################################"
        echo "Destroying VMs"
        echo "############################################################"
        uvt-kvm destroy $ENB_VM_NAME
        ssh-keygen -R $ENB_VM_IP_ADDR
        uvt-kvm destroy $UE_VM_NAME
        ssh-keygen -R $UE_VM_IP_ADDR
    fi
}

function start_rf_sim_enb {
    local LOC_ENB_VM_IP_ADDR=$2
    local LOC_EPC_IP_ADDR=$3
    local LOC_LOG_FILE=$4
    local LOC_NB_RBS=$5
    local LOC_CONF_FILE=$6
    # 1 is with S1 and 0 without S1 aka noS1
    local LOC_S1_CONFIGURATION=$7
    echo "cd /home/ubuntu/tmp" > $1
    echo "echo \"sudo apt-get --yes --quiet install daemon \"" >> $1
    echo "sudo apt-get --yes install daemon >> /home/ubuntu/tmp/cmake_targets/log/daemon-install.txt 2>&1" >> $1
    echo "echo \"export RFSIMULATOR=enb\"" >> $1
    echo "export RFSIMULATOR=enb" >> $1
    echo "echo \"source oaienv\"" >> $1
    echo "source oaienv" >> $1
    echo "cd ci-scripts/conf_files/" >> $1
    echo "cp $LOC_CONF_FILE ci-$LOC_CONF_FILE" >> $1
    echo "sed -i -e 's#N_RB_DL.*=.*;#N_RB_DL                                         = $LOC_NB_RBS;#' -e 's#CI_MME_IP_ADDR#$LOC_EPC_IP_ADDR#' -e 's#CI_ENB_IP_ADDR#$LOC_ENB_VM_IP_ADDR#' -e 's#CI_UE_IP_ADDR#$LOC_UE_VM_IP_ADDR#' ci-$LOC_CONF_FILE" >> $1
    echo "echo \"grep N_RB_DL ci-$LOC_CONF_FILE\"" >> $1
    echo "grep N_RB_DL ci-$LOC_CONF_FILE | sed -e 's#N_RB_DL.*=#N_RB_DL =#'" >> $1
    echo "echo \"cd /home/ubuntu/tmp/cmake_targets/ran_build/build/\"" >> $1
    echo "sudo chmod 777 /home/ubuntu/tmp/cmake_targets/ran_build/build/" >> $1
    echo "cd /home/ubuntu/tmp/cmake_targets/ran_build/build/" >> $1
    if [ $LOC_S1_CONFIGURATION -eq 0 ]
    then
        echo "echo \"ulimit -c unlimited && ./lte-softmodem -O /home/ubuntu/tmp/ci-scripts/conf_files/ci-$LOC_CONF_FILE --rfsim --log_config.global_log_options level,nocolor --noS1 --eNBs.[0].rrc_inactivity_threshold 0\" > ./my-lte-softmodem-run.sh " >> $1
    else
        echo "echo \"ulimit -c unlimited && ./lte-softmodem -O /home/ubuntu/tmp/ci-scripts/conf_files/ci-$LOC_CONF_FILE --rfsim --log_config.global_log_options level,nocolor --eNBs.[0].rrc_inactivity_threshold 0 --eNBs.[0].plmn_list.[0].mnc 93\" > ./my-lte-softmodem-run.sh " >> $1
    fi
    echo "chmod 775 ./my-lte-softmodem-run.sh" >> $1
    echo "cat ./my-lte-softmodem-run.sh" >> $1
    echo "if [ -e /home/ubuntu/tmp/cmake_targets/log/$LOC_LOG_FILE ]; then sudo sudo rm -f /home/ubuntu/tmp/cmake_targets/log/$LOC_LOG_FILE; fi" >> $1
    echo "sudo -E daemon --inherit --unsafe --name=enb_daemon --chdir=/home/ubuntu/tmp/cmake_targets/ran_build/build/ -o /home/ubuntu/tmp/cmake_targets/log/$LOC_LOG_FILE ./my-lte-softmodem-run.sh" >> $1

    ssh -T -o StrictHostKeyChecking=no ubuntu@$LOC_ENB_VM_IP_ADDR < $1
    rm $1

    local i="0"
    echo "egrep -c \"got sync\" /home/ubuntu/tmp/cmake_targets/log/$LOC_LOG_FILE" > $1
    while [ $i -lt 10 ]
    do
        sleep 5
        CONNECTED=`ssh -T -o StrictHostKeyChecking=no ubuntu@$LOC_ENB_VM_IP_ADDR < $1`
        if [ $CONNECTED -ne 0 ]
        then
            i="100"
        else
            i=$[$i+1]
        fi
    done
    echo "echo \"free -m\"" > $1
    echo "free -m" >> $1
    ssh -T -o StrictHostKeyChecking=no ubuntu@$LOC_ENB_VM_IP_ADDR < $1
    rm $1
    if [ $i -lt 50 ]
    then
        ENB_SYNC=0
        echo "RF-SIM eNB is NOT sync'ed: process still alive?"
    else
        ENB_SYNC=1
        echo "RF-SIM eNB is sync'ed: waiting for UE(s) to connect"
    fi
    if [ $LOC_S1_CONFIGURATION -eq 0 ]
    then
        echo "ifconfig oaitun_enb1 | egrep -c \"inet addr\"" > $1
        # Checking oaitun_enb1 interface has now an IP address
        i="0"
        while [ $i -lt 10 ]
        do
            CONNECTED=`ssh -T -o StrictHostKeyChecking=no ubuntu@$LOC_ENB_VM_IP_ADDR < $1`
            if [ $CONNECTED -eq 1 ]
            then
                i="100"
            else
                i=$[$i+1]
                sleep 5
            fi
        done
        rm $1
        if [ $i -lt 50 ]
        then
            ENB_SYNC=0
            echo "RF-SIM eNB oaitun_enb1 is DOWN or NOT CONFIGURED"
        else
            echo "RF-SIM eNB oaitun_enb1 is UP and CONFIGURED"
        fi
        if [[ $LOC_CONF_FILE =~ .*mbms.* ]]
        then
            echo "ifconfig oaitun_enm1 | egrep -c \"inet addr\"" > $1
            # Checking oaitun_enm1 interface has now an IP address
            i="0"
            while [ $i -lt 10 ]
            do
                CONNECTED=`ssh -T -o StrictHostKeyChecking=no ubuntu@$LOC_ENB_VM_IP_ADDR < $1`
                if [ $CONNECTED -eq 1 ]
                then
                    i="100"
                else
                    i=$[$i+1]
                    sleep 5
                fi
            done
            rm $1
            if [ $i -lt 50 ]
            then
                ENB_SYNC=0
                echo "RF-SIM eNB oaitun_enm1 is DOWN or NOT CONFIGURED"
            else
                echo "RF-SIM eNB oaitun_enm1 is UP and CONFIGURED"
            fi
        fi
    fi
    sleep 10
}

function start_rf_sim_ue {
    local LOC_UE_VM_IP_ADDR=$2
    local LOC_ENB_VM_IP_ADDR=$3
    local LOC_LOG_FILE=$4
    local LOC_PRB=$5
    local LOC_FREQUENCY=$6
    # 1 is with S1 and 0 without S1 aka noS1
    local LOC_S1_CONFIGURATION=$7
    local LOC_MBMS_CONFIGURATION=$8
    echo "echo \"sudo apt-get --yes --quiet install daemon \"" > $1
    echo "sudo apt-get --yes install daemon >> /home/ubuntu/tmp/cmake_targets/log/daemon-install.txt 2>&1" >> $1
    echo "echo \"export RFSIMULATOR=${LOC_ENB_VM_IP_ADDR}\"" >> $1
    echo "export RFSIMULATOR=${LOC_ENB_VM_IP_ADDR}" >> $1
    echo "echo \"cd /home/ubuntu/tmp/cmake_targets/ran_build/build/\"" >> $1
    echo "sudo chmod 777 /home/ubuntu/tmp/cmake_targets/ran_build/build/" >> $1
    echo "cd /home/ubuntu/tmp/cmake_targets/ran_build/build/" >> $1
    if [ $LOC_S1_CONFIGURATION -eq 0 ]
    then
        echo "echo \"ulimit -c unlimited && ./lte-uesoftmodem -C ${LOC_FREQUENCY}000000 -r $LOC_PRB --ue-rxgain 140 --ue-txgain 120 --nokrnmod 1 --rfsim --log_config.global_log_options level,nocolor --noS1\" > ./my-lte-softmodem-run.sh " >> $1
    else
        echo "echo \"ulimit -c unlimited && ./lte-uesoftmodem -C ${LOC_FREQUENCY}000000 -r $LOC_PRB --ue-rxgain 140 --ue-txgain 120 --nokrnmod 1 --rfsim --log_config.global_log_options level,nocolor\" > ./my-lte-softmodem-run.sh " >> $1
    fi
    echo "chmod 775 ./my-lte-softmodem-run.sh" >> $1
    echo "cat ./my-lte-softmodem-run.sh" >> $1
    echo "if [ -e /home/ubuntu/tmp/cmake_targets/log/$LOC_LOG_FILE ]; then sudo sudo rm -f /home/ubuntu/tmp/cmake_targets/log/$LOC_LOG_FILE; fi" >> $1
    echo "sudo -E daemon --inherit --unsafe --name=ue_daemon --chdir=/home/ubuntu/tmp/cmake_targets/ran_build/build/ -o /home/ubuntu/tmp/cmake_targets/log/$LOC_LOG_FILE ./my-lte-softmodem-run.sh" >> $1

    ssh -T -o StrictHostKeyChecking=no ubuntu@$LOC_UE_VM_IP_ADDR < $1
    rm $1

    local i="0"
    echo "egrep -c \"got sync\" /home/ubuntu/tmp/cmake_targets/log/$LOC_LOG_FILE" > $1
    while [ $i -lt 10 ]
    do
        sleep 5
        CONNECTED=`ssh -T -o StrictHostKeyChecking=no ubuntu@$LOC_UE_VM_IP_ADDR < $1`
        if [ $CONNECTED -ne 0 ]
        then
            i="100"
        else
            i=$[$i+1]
        fi
    done
    UE_SYNC=1
    rm $1
    if [ $i -lt 50 ]
    then
        UE_SYNC=0
        echo "RF-SIM UE is NOT sync'ed w/ eNB"
        return
    else
        echo "RF-SIM UE is sync'ed w/ eNB"
    fi
    # Checking oaitun_ue1 interface has now an IP address
    i="0"
    echo "ifconfig oaitun_ue1 | egrep -c \"inet addr\"" > $1
    while [ $i -lt 10 ]
    do
        sleep 5
        CONNECTED=`ssh -T -o StrictHostKeyChecking=no ubuntu@$LOC_UE_VM_IP_ADDR < $1`
        if [ $CONNECTED -eq 1 ]
        then
            i="100"
        else
            i=$[$i+1]
        fi
    done
    echo "echo \"free -m\"" > $1
    echo "free -m" >> $1
    ssh -T -o StrictHostKeyChecking=no ubuntu@$LOC_UE_VM_IP_ADDR < $1
    rm $1
    if [ $i -lt 50 ]
    then
        UE_SYNC=0
        echo "RF-SIM UE oaitun_ue1 is DOWN or NOT CONFIGURED"
    else
        echo "RF-SIM UE oaitun_ue1 is UP and CONFIGURED"
    fi
    if [ $LOC_MBMS_CONFIGURATION -eq 1 ]
    then
        # Checking oaitun_uem1 interface has now an IP address
        i="0"
        echo "ifconfig oaitun_uem1 | egrep -c \"inet addr\"" > $1
        while [ $i -lt 10 ]
        do
            sleep 5
            CONNECTED=`ssh -T -o StrictHostKeyChecking=no ubuntu@$LOC_UE_VM_IP_ADDR < $1`
            if [ $CONNECTED -eq 1 ]
            then
                i="100"
            else
                i=$[$i+1]
            fi
        done
        rm $1
        if [ $i -lt 50 ]
        then
            UE_SYNC=0
            echo "RF-SIM UE oaitun_uem1 is DOWN or NOT CONFIGURED"
        else
            echo "RF-SIM UE oaitun_uem1 is UP and CONFIGURED"
        fi
    fi
    sleep 10
}


function start_rf_sim_gnb {
    local LOC_GNB_VM_IP_ADDR=$2
    local LOC_LOG_FILE=$3
    local LOC_NB_RBS=$4
    local LOC_CONF_FILE=$5
    # 1 is with S1 and 0 without S1 aka noS1
    local LOC_S1_CONFIGURATION=$6
    #LOC_RA_SA_TEST=1 will run the RA test check ; =2 will run the SA test
    local LOC_RA_SA_TEST=$7

    if [ -e rbconfig.raw ]; then rm -f rbconfig.raw; fi
    if [ -e reconfig.raw ]; then rm -f reconfig.raw; fi

    echo "cd /home/ubuntu/tmp" > $1
    echo "echo \"sudo apt-get --yes --quiet install daemon \"" >> $1
    echo "sudo apt-get --yes install daemon >> /home/ubuntu/tmp/cmake_targets/log/daemon-install.txt 2>&1" >> $1
    echo "echo \"source oaienv\"" >> $1
    echo "source oaienv" >> $1
    echo "echo \"export RFSIMULATOR=server\"" >> $1
    echo "export RFSIMULATOR=server" >> $1
    echo "cd ci-scripts/conf_files/" >> $1
    echo "cp $LOC_CONF_FILE ci-$LOC_CONF_FILE" >> $1
    #echo "sed -i -e 's#N_RB_DL.*=.*;#N_RB_DL                                         = $LOC_NB_RBS;#' -e 's#CI_MME_IP_ADDR#$LOC_EPC_IP_ADDR#' -e 's#CI_ENB_IP_ADDR#$LOC_ENB_VM_IP_ADDR#' -e 's#CI_UE_IP_ADDR#$LOC_UE_VM_IP_ADDR#' ci-$LOC_CONF_FILE" >> $1
    #echo "echo \"grep N_RB_DL ci-$LOC_CONF_FILE\"" >> $1
    #echo "grep N_RB_DL ci-$LOC_CONF_FILE | sed -e 's#N_RB_DL.*=#N_RB_DL =#'" >> $1
    echo "echo \"cd /home/ubuntu/tmp/cmake_targets/ran_build/build/\"" >> $1
    echo "sudo chmod 777 /home/ubuntu/tmp/cmake_targets/ran_build/build/" >> $1
    echo "cd /home/ubuntu/tmp/cmake_targets/ran_build/build/" >> $1
    echo "sudo rm -f r*config.raw" >> $1
    if [ $LOC_S1_CONFIGURATION -eq 0 ]
    then
        if [ $LOC_RA_SA_TEST -eq 0 ] #no RA test => use --phy-test option
        then
            echo "echo \"./nr-softmodem -O /home/ubuntu/tmp/ci-scripts/conf_files/ci-$LOC_CONF_FILE --log_config.global_log_options level,nocolor --parallel-config PARALLEL_SINGLE_THREAD --noS1 --nokrnmod 1 --rfsim --phy-test --lowmem --noS1\" > ./my-nr-softmodem-run.sh " >> $1
        elif [ $LOC_RA_SA_TEST -eq 1 ] #RA test => use --do-ra option
        then
            echo "echo \"./nr-softmodem -O /home/ubuntu/tmp/ci-scripts/conf_files/ci-$LOC_CONF_FILE --log_config.global_log_options level,nocolor --parallel-config PARALLEL_SINGLE_THREAD --rfsim --do-ra --lowmem --noS1\" > ./my-nr-softmodem-run.sh " >> $1
        else #SA test => use --sa option
            echo "echo \"./nr-softmodem -O /home/ubuntu/tmp/ci-scripts/conf_files/ci-$LOC_CONF_FILE --log_config.global_log_options level,nocolor --parallel-config PARALLEL_SINGLE_THREAD --rfsim --sa --lowmem \" > ./my-nr-softmodem-run.sh " >> $1
        fi
    fi
    echo "chmod 775 ./my-nr-softmodem-run.sh" >> $1
    echo "cat ./my-nr-softmodem-run.sh" >> $1
    echo "if [ -e /home/ubuntu/tmp/cmake_targets/log/$LOC_LOG_FILE ]; then sudo sudo rm -f /home/ubuntu/tmp/cmake_targets/log/$LOC_LOG_FILE; fi" >> $1
    echo "sudo -E daemon --inherit --unsafe --name=gnb_daemon --chdir=/home/ubuntu/tmp/cmake_targets/ran_build/build/ -o /home/ubuntu/tmp/cmake_targets/log/$LOC_LOG_FILE ./my-nr-softmodem-run.sh" >> $1

    ssh -T -o StrictHostKeyChecking=no ubuntu@$LOC_GNB_VM_IP_ADDR < $1
    rm $1


    local i="0"
    echo "egrep -c \"got sync\" /home/ubuntu/tmp/cmake_targets/log/$LOC_LOG_FILE" > $1
    while [ $i -lt 10 ]
    do
        sleep 5
        CONNECTED=`ssh -T -o StrictHostKeyChecking=no ubuntu@$LOC_GNB_VM_IP_ADDR < $1`
        if [ $CONNECTED -ne 0 ]
        then
            i="100"
        else
            i=$[$i+1]
        fi
    done
    rm $1
    if [ $i -lt 50 ]
    then
        GNB_SYNC=0
        echo "RF-SIM gNB is NOT sync'ed: process still alive?"
    else
        GNB_SYNC=1
        echo "RF-SIM gNB is sync'ed: waiting for UE(s) to connect"
    fi

    # check noS1 config only outside RA test (as it does not support noS1)
    if [ $LOC_S1_CONFIGURATION -eq 0 ] && [ $LOC_RA_SA_TEST -eq 0 ]
    then
        echo "ifconfig oaitun_enb1 | egrep -c \"inet addr\"" > $1
        # Checking oaitun_enb1 interface has now an IP address
        i="0"
        while [ $i -lt 10 ]
        do
            CONNECTED=`ssh -T -o StrictHostKeyChecking=no ubuntu@$LOC_GNB_VM_IP_ADDR < $1`
            if [ $CONNECTED -eq 1 ]
            then
                i="100"
            else
                i=$[$i+1]
                sleep 5
            fi
        done
        rm $1
        if [ $i -lt 50 ]
        then
            GNB_SYNC=0
            echo "RF-SIM gNB oaitun_enb1 is DOWN or NOT CONFIGURED"
        else
            echo "RF-SIM gNB oaitun_enb1 is UP and CONFIGURED"
        fi
    fi


    sleep 10
    echo "echo \"free -m\"" > $1
    echo "free -m" >> $1
    ssh -T -o StrictHostKeyChecking=no ubuntu@$LOC_GNB_VM_IP_ADDR < $1
    rm $1
    # Copy the RAW files from the gNB run for the NR-UE
    scp -o StrictHostKeyChecking=no ubuntu@$LOC_GNB_VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/ran_build/build/rbconfig.raw .
    scp -o StrictHostKeyChecking=no ubuntu@$LOC_GNB_VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/ran_build/build/reconfig.raw .
}

function start_rf_sim_nr_ue {
    local LOC_NR_UE_VM_IP_ADDR=$2
    local LOC_GNB_VM_IP_ADDR=$3
    local LOC_LOG_FILE=$4
    local LOC_PRB=$5
    local LOC_FREQUENCY=$6
    # 1 is with S1 and 0 without S1 aka noS1
    local LOC_S1_CONFIGURATION=$7
    #LOC_RA_SA_TEST=1 will run the RA test check ; =2 will run the SA test
    local LOC_RA_SA_TEST=$8

    # Copy the RAW files from the gNB run
    scp -o StrictHostKeyChecking=no rbconfig.raw ubuntu@$LOC_NR_UE_VM_IP_ADDR:/home/ubuntu/tmp
    scp -o StrictHostKeyChecking=no reconfig.raw ubuntu@$LOC_NR_UE_VM_IP_ADDR:/home/ubuntu/tmp

    echo "echo \"sudo apt-get --yes --quiet install daemon \"" > $1
    echo "sudo apt-get --yes install daemon >> /home/ubuntu/tmp/cmake_targets/log/daemon-install.txt 2>&1" >> $1
    echo "echo \"export RFSIMULATOR=${LOC_GNB_VM_IP_ADDR}\"" >> $1
    echo "export RFSIMULATOR=${LOC_GNB_VM_IP_ADDR}" >> $1
    echo "echo \"cd /home/ubuntu/tmp/cmake_targets/ran_build/build/\"" >> $1
    echo "sudo chmod 777 /home/ubuntu/tmp/cmake_targets/ran_build/build/" >> $1
    echo "sudo cp /home/ubuntu/tmp/r*config.raw /home/ubuntu/tmp/cmake_targets/ran_build/build/" >> $1
    echo "sudo chmod 666 /home/ubuntu/tmp/cmake_targets/ran_build/build/r*config.raw" >> $1
    echo "cd /home/ubuntu/tmp/cmake_targets/ran_build/build/" >> $1
    if [ $LOC_S1_CONFIGURATION -eq 0 ]
    then
        if [ $LOC_RA_SA_TEST -eq 0 ] #no RA test => use --phy-test option
        then
            echo "echo \"./nr-uesoftmodem --nokrnmod 1 --rfsim --phy-test --rrc_config_path /home/ubuntu/tmp/cmake_targets/ran_build/build/ --log_config.global_log_options level,nocolor --noS1\" > ./my-nr-softmodem-run.sh " >> $1
        elif [ $LOC_RA_SA_TEST -eq 1 ] #RA test => use --do-ra option
        then
            echo "echo \"./nr-uesoftmodem --rfsim --do-ra --log_config.global_log_options level,nocolor --rrc_config_path /home/ubuntu/tmp/cmake_targets/ran_build/build/\" > ./my-nr-softmodem-run.sh " >> $1
        else #SA test => use --sa option
            echo "echo \"./nr-uesoftmodem -r 106 --numerology 1 --band 78 -C 3619200000 --rfsim --sa --log_config.global_log_options level,nocolor\" > ./my-nr-softmodem-run.sh " >> $1
        fi
    fi
    echo "chmod 775 ./my-nr-softmodem-run.sh" >> $1
    echo "cat ./my-nr-softmodem-run.sh" >> $1
    echo "if [ -e /home/ubuntu/tmp/cmake_targets/log/$LOC_LOG_FILE ]; then sudo sudo rm -f /home/ubuntu/tmp/cmake_targets/log/$LOC_LOG_FILE; fi" >> $1
    echo "sudo -E daemon --inherit --unsafe --name=nr_ue_daemon --chdir=/home/ubuntu/tmp/cmake_targets/ran_build/build/ -o /home/ubuntu/tmp/cmake_targets/log/$LOC_LOG_FILE ./my-nr-softmodem-run.sh" >> $1

    ssh -T -o StrictHostKeyChecking=no ubuntu@$LOC_NR_UE_VM_IP_ADDR < $1
    rm $1

    local i="0"
    echo "egrep -c \"Initial sync: pbch decoded sucessfully\" /home/ubuntu/tmp/cmake_targets/log/$LOC_LOG_FILE" > $1
    while [ $i -lt 10 ]
    do
        sleep 5
        CONNECTED=`ssh -T -o StrictHostKeyChecking=no ubuntu@$LOC_NR_UE_VM_IP_ADDR < $1`
        if [ $CONNECTED -ne 0 ]
        then
            i="100"
        else
            i=$[$i+1]
        fi
    done
    NR_UE_SYNC=1
    rm $1
    if [ $i -lt 50 ]
    then
        NR_UE_SYNC=0
        echo "RF-SIM NR-UE is NOT sync'ed w/ gNB"
        return
    else
        echo "RF-SIM NR-UE is sync'ed w/ gNB"
    fi
    # Checking oaitun_ue1 interface has now an IP address (only outside RA test)
    if [ $LOC_RA_SA_TEST -eq  0 ]
    then
      i="0"
      echo "ifconfig oaitun_ue1 | egrep -c \"inet addr\"" > $1
      while [ $i -lt 10 ]
      do
        sleep 5
        CONNECTED=`ssh -T -o StrictHostKeyChecking=no ubuntu@$LOC_NR_UE_VM_IP_ADDR < $1`
        if [ $CONNECTED -eq 1 ]
        then
            i="100"
        else
            i=$[$i+1]
        fi
      done
      echo "echo \"free -m\"" > $1
      echo "free -m" >> $1
      ssh -T -o StrictHostKeyChecking=no ubuntu@$LOC_NR_UE_VM_IP_ADDR < $1
      rm $1
      if [ $i -lt 50 ]
      then
        NR_UE_SYNC=0
        echo "RF-SIM NR-UE oaitun_ue1 is DOWN or NOT CONFIGURED"
      else
        echo "RF-SIM NR-UE oaitun_ue1 is UP and CONFIGURED"
      fi
      sleep 10
   fi
}


function run_test_on_vm {
    echo "############################################################"
    echo "OAI CI VM script"
    echo "############################################################"
    if [[ (( "$RUN_OPTIONS" == "complex" ) && ( $VM_NAME =~ .*-l2-sim.* ))  ]] ||  [[ (( "$RUN_OPTIONS" == "complex" ) && ( $VM_NAME =~ .*-rf-sim.* ))  ]]
    then
        ENB_VM_NAME=`echo $VM_NAME | sed -e "s#l2-sim#enb-ethernet#" -e "s#rf-sim#enb-ethernet#"`
        ENB_VM_CMDS=${ENB_VM_NAME}_cmds.txt
        echo "ENB_VM_NAME         = $ENB_VM_NAME"
        echo "ENB_VM_CMD_FILE     = $ENB_VM_CMDS"
        UE_VM_NAME=`echo $VM_NAME | sed -e "s#l2-sim#ue-ethernet#" -e "s#rf-sim#ue-ethernet#"`
        UE_VM_CMDS=${UE_VM_NAME}_cmds.txt
        echo "UE_VM_NAME          = $UE_VM_NAME"
        echo "UE_VM_CMD_FILE      = $UE_VM_CMDS"
        GNB_VM_NAME=`echo $VM_NAME | sed -e "s#l2-sim#gnb-usrp#" -e "s#rf-sim#gnb-usrp#"`
        GNB_VM_CMDS=${GNB_VM_NAME}_cmds.txt
        echo "GNB_VM_NAME         = $GNB_VM_NAME"
        echo "GNB_VM_CMD_FILE     = $GNB_VM_CMDS"
        NR_UE_VM_NAME=`echo $VM_NAME | sed -e "s#l2-sim#nr-ue-usrp#" -e "s#rf-sim#nr-ue-usrp#"`
        NR_UE_VM_CMDS=${UE_VM_NAME}_cmds.txt
        echo "NR_UE_VM_NAME       = $NR_UE_VM_NAME"
        echo "NR_UE_VM_CMD_FILE   = $NR_UE_VM_CMDS"
    else
        echo "VM_NAME             = $VM_NAME"
        echo "VM_CMD_FILE         = $VM_CMDS"
    fi
    echo "JENKINS_WKSP        = $JENKINS_WKSP"
    echo "ARCHIVES_LOC        = $ARCHIVES_LOC"

    if [[ (( "$RUN_OPTIONS" == "complex" ) && ( $VM_NAME =~ .*-l2-sim.* ))  ]] ||  [[ (( "$RUN_OPTIONS" == "complex" ) && ( $VM_NAME =~ .*-rf-sim.* ))  ]]
    then
        echo "############################################################"
        echo "Waiting for ENB VM to be started"
        echo "############################################################"
        uvt-kvm wait $ENB_VM_NAME --insecure

        ENB_VM_IP_ADDR=`uvt-kvm ip $ENB_VM_NAME`
        echo "$ENB_VM_NAME has for IP addr = $ENB_VM_IP_ADDR"

        echo "############################################################"
        echo "Waiting for UE VM to be started"
        echo "############################################################"
        uvt-kvm wait $UE_VM_NAME --insecure

        UE_VM_IP_ADDR=`uvt-kvm ip $UE_VM_NAME`
        echo "$UE_VM_NAME has for IP addr = $UE_VM_IP_ADDR"

        echo "############################################################"
        echo "Waiting for GNB VM to be started"
        echo "############################################################"
        uvt-kvm wait $GNB_VM_NAME --insecure

        GNB_VM_IP_ADDR=`uvt-kvm ip $GNB_VM_NAME`
        echo "$GNB_VM_NAME has for IP addr = $GNB_VM_IP_ADDR"

        echo "############################################################"
        echo "Waiting for NR-UE VM to be started"
        echo "############################################################"
        uvt-kvm wait $NR_UE_VM_NAME --insecure

        NR_UE_VM_IP_ADDR=`uvt-kvm ip $NR_UE_VM_NAME`
        echo "$NR_UE_VM_NAME has for IP addr = $NR_UE_VM_IP_ADDR"
    else
        echo "############################################################"
        echo "Waiting for VM to be started"
        echo "############################################################"
        uvt-kvm wait $VM_NAME --insecure

        VM_IP_ADDR=`uvt-kvm ip $VM_NAME`
        echo "$VM_NAME has for IP addr = $VM_IP_ADDR"
    fi

    if [ "$RUN_OPTIONS" == "none" ] || [[ $RUN_OPTIONS =~ .*run_exec_autotests.* ]]
    then
        echo "No run on VM testing for this variant currently"
        return
    fi

    if [[ $RUN_OPTIONS =~ .*run_XXXX_autotests.* ]]
    then
        echo "############################################################"
        echo "Running test script on VM ($VM_NAME)"
        echo "############################################################"
        echo "echo \"sudo apt-get --yes --quiet install bc \"" > $VM_CMDS
        echo "sudo apt-get update > bc-install.txt 2>&1" >> $VM_CMDS
        echo "sudo apt-get --yes install bc >> bc-install.txt 2>&1" >> $VM_CMDS
        echo "cd tmp" >> $VM_CMDS
        echo "echo \"source oaienv\"" >> $VM_CMDS
        echo "source oaienv" >> $VM_CMDS
        echo "echo \"cd cmake_targets/autotests\"" >> $VM_CMDS
        echo "cd cmake_targets/autotests" >> $VM_CMDS
        echo "echo \"rm -Rf log\"" >> $VM_CMDS
        echo "rm -Rf log" >> $VM_CMDS
        echo "$RUN_OPTIONS" | sed -e 's@"@\\"@g' -e 's@^@echo "@' -e 's@$@"@' >> $VM_CMDS
        echo "$RUN_OPTIONS" >> $VM_CMDS
        echo "cp /home/ubuntu/bc-install.txt log" >> $VM_CMDS
        echo "cd log" >> $VM_CMDS
        echo "zip -r -qq tmp.zip *.* 0*" >> $VM_CMDS
        echo "echo \"############################################################\"" >> $VM_CMDS
        echo "echo \"Evaluating remaining memory on disk!\"" >> $VM_CMDS
        echo "echo \"############################################################\"" >> $VM_CMDS
        echo "echo \"df -h\"" >> $VM_CMDS
        echo "df -h" >> $VM_CMDS

        ssh -T -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR < $VM_CMDS

        echo "############################################################"
        echo "Creating a tmp folder to store results and artifacts"
        echo "############################################################"

        if [ -d $ARCHIVES_LOC ]
        then
            rm -Rf $ARCHIVES_LOC
        fi
        mkdir --parents $ARCHIVES_LOC

        scp -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/autotests/log/tmp.zip $ARCHIVES_LOC
        pushd $ARCHIVES_LOC
        unzip -qq -DD tmp.zip
        rm tmp.zip
        if [ -f results_autotests.xml ]
        then
            FUNCTION=`echo $VM_NAME | sed -e "s@$VM_TEMPLATE@@"`
            NEW_NAME=`echo "results_autotests.xml" | sed -e "s@results_autotests@results_autotests-$FUNCTION@"`
            echo "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" > $NEW_NAME
            echo "<?xml-stylesheet type=\"text/xsl\" href=\"$FUNCTION.xsl\" ?>" >> $NEW_NAME
            cat results_autotests.xml >> $NEW_NAME
            sed -e "s@TEMPLATE@$FUNCTION@" $JENKINS_WKSP/ci-scripts/template.xsl > $FUNCTION.xsl
            #mv results_autotests.xml $NEW_NAME
            rm results_autotests.xml
        fi
        popd

        if [ $KEEP_VM_ALIVE -eq 0 ]
        then
            echo "############################################################"
            echo "Destroying VM"
            echo "############################################################"
            uvt-kvm destroy $VM_NAME
            ssh-keygen -R $VM_IP_ADDR
        fi
        rm -f $VM_CMDS

        echo "############################################################"
        echo "Checking run status"
        echo "############################################################"

        LOG_FILES=`ls $ARCHIVES_LOC/results_autotests*.xml`
        NB_FOUND_FILES=0
        NB_RUNS=0
        NB_FAILURES=0

        for FULLFILE in $LOG_FILES
        do
            TESTSUITES=`egrep "testsuite errors" $FULLFILE`
            for TESTSUITE in $TESTSUITES
            do
                if [[ "$TESTSUITE" == *"tests="* ]]
                then
                    RUNS=`echo $TESTSUITE | awk 'BEGIN{FS="="}{print $2}END{}' | sed -e "s@'@@g" `
                    NB_RUNS=$((NB_RUNS + RUNS))
                fi
                if [[ "$TESTSUITE" == *"failures="* ]]
                then
                    FAILS=`echo $TESTSUITE | awk 'BEGIN{FS="="}{print $2}END{}' | sed -e "s@'@@g" `
                    NB_FAILURES=$((NB_FAILURES + FAILS))
                fi
            done
            NB_FOUND_FILES=$((NB_FOUND_FILES + 1))
        done

        echo "NB_FOUND_FILES = $NB_FOUND_FILES"
        echo "NB_RUNS        = $NB_RUNS"
        echo "NB_FAILURES    = $NB_FAILURES"

        if [ $NB_FOUND_FILES -eq 0 ]; then STATUS=-1; fi
        if [ $NB_RUNS -eq 0 ]; then STATUS=-1; fi
        if [ $NB_FAILURES -ne 0 ]; then STATUS=-1; fi

        if [ $STATUS -eq 0 ]
        then
            echo "TEST_OK" > $ARCHIVES_LOC/test_final_status.log
        else
            echo "TEST_KO" > $ARCHIVES_LOC/test_final_status.log
        fi
    fi

    if [[ "$RUN_OPTIONS" == "complex" ]] && [[ $VM_NAME =~ .*-basic-sim.* ]]
    then
        PING_STATUS=0
        IPERF_STATUS=0
        if [ -d $ARCHIVES_LOC ]
        then
            rm -Rf $ARCHIVES_LOC
        fi
        mkdir --parents $ARCHIVES_LOC

        # Creating a VM for EPC and installing SW
        EPC_VM_NAME=`echo $VM_NAME | sed -e "s#basic-sim#epc#"`
        EPC_VM_CMDS=${EPC_VM_NAME}_cmds.txt
        LTEBOX=0
        if [[ "$EPC_IPADDR" == "" ]]
        then
            # Creating a VM for EPC and installing SW
            install_epc_on_vm $EPC_VM_NAME $EPC_VM_CMDS
            EPC_VM_IP_ADDR=`uvt-kvm ip $EPC_VM_NAME`

            # Starting EPC
            start_epc $EPC_VM_NAME $EPC_VM_CMDS $EPC_VM_IP_ADDR
        else
            echo "We will use EPC on $EPC_IPADDR"
            EPC_VM_IP_ADDR=$EPC_IPADDR
        fi

        # Retrieve EPC real IP address
        retrieve_real_epc_ip_addr $EPC_VM_NAME $EPC_VM_CMDS $EPC_VM_IP_ADDR

        TRANS_MODES=("fdd" "tdd")
        BW_CASES=(05 10 20)

        for TMODE in ${TRANS_MODES[@]}
        do
          for BW in ${BW_CASES[@]}
          do
              # Not Running in TDD-20MHz : too unstable
              if [[ $TMODE =~ .*tdd.* ]] && [[ $BW =~ .*20.* ]]; then continue; fi

              if [[ $BW =~ .*05.* ]]; then PRB=25; fi
              if [[ $BW =~ .*10.* ]]; then PRB=50; fi
              if [[ $BW =~ .*20.* ]]; then PRB=100; fi
              if [[ $TMODE =~ .*fdd.* ]]; then FREQUENCY=2680; else FREQUENCY=2350; fi

              echo "############################################################"
              echo "Starting the eNB in ${TMODE}-${BW}MHz mode"
              echo "############################################################"
              CURRENT_ENB_LOG_FILE=${TMODE}_${BW}MHz_enb.log
              start_basic_sim_enb $VM_CMDS $VM_IP_ADDR $EPC_VM_IP_ADDR $CURRENT_ENB_LOG_FILE $PRB lte-${TMODE}-basic-sim.conf none

              echo "############################################################"
              echo "Starting the UE in ${TMODE}-${BW}MHz mode"
              echo "############################################################"
              CURRENT_UE_LOG_FILE=${TMODE}_${BW}MHz_ue.log
              start_basic_sim_ue $VM_CMDS $VM_IP_ADDR $CURRENT_UE_LOG_FILE $PRB $FREQUENCY
              if [ $UE_SYNC -eq 0 ]
              then
                  echo "Problem w/ eNB and UE not syncing"
                  full_terminate
                  STATUS=-1
                  continue
              fi
              get_ue_ip_addr $VM_CMDS $VM_IP_ADDR 1

              echo "############################################################"
              echo "Pinging the UE"
              echo "############################################################"
              PING_LOG_FILE=${TMODE}_${BW}MHz_ping_ue.txt
              ping_ue_ip_addr $EPC_VM_CMDS $EPC_VM_IP_ADDR $UE_IP_ADDR $PING_LOG_FILE 0
              scp -o StrictHostKeyChecking=no ubuntu@$EPC_VM_IP_ADDR:/home/ubuntu/$PING_LOG_FILE $ARCHIVES_LOC
              check_ping_result $ARCHIVES_LOC/$PING_LOG_FILE 20

              # Not more testing in any TDD : too unstable
              if [[ $TMODE =~ .*tdd.* ]] && [[ $BW =~ .*05.* ]]; then full_terminate; continue; fi
              if [[ $TMODE =~ .*tdd.* ]] && [[ $BW =~ .*10.* ]]; then full_terminate; continue; fi
              if [[ $TMODE =~ .*tdd.* ]] && [[ $BW =~ .*20.* ]]; then full_terminate; continue; fi
              echo "############################################################"
              echo "Iperf DL"
              echo "############################################################"
              if [[ $TMODE =~ .*fdd.* ]]
              then
                  if [[ $BW =~ .*20.* ]]; then THROUGHPUT=12; else THROUGHPUT=10; fi
              else
                  THROUGHPUT=6
              fi
              CURR_IPERF_LOG_BASE=${TMODE}_${BW}MHz_iperf_dl
              generic_iperf $VM_CMDS $VM_IP_ADDR $UE_IP_ADDR $EPC_VM_CMDS $EPC_VM_IP_ADDR $REAL_EPC_IP_ADDR $THROUGHPUT $CURR_IPERF_LOG_BASE 0 0
              scp -o StrictHostKeyChecking=no ubuntu@$EPC_VM_IP_ADDR:/home/ubuntu/${CURR_IPERF_LOG_BASE}_client.txt $ARCHIVES_LOC
              scp -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR:/home/ubuntu/${CURR_IPERF_LOG_BASE}_server.txt $ARCHIVES_LOC
              check_iperf $ARCHIVES_LOC/$CURR_IPERF_LOG_BASE $THROUGHPUT

              # Not more testing in FDD-20MHz : too unstable
              if [[ $TMODE =~ .*fdd.* ]] && [[ $BW =~ .*20.* ]]; then full_terminate; continue; fi
              echo "############################################################"
              echo "Iperf UL"
              echo "############################################################"
              THROUGHPUT=2
              CURR_IPERF_LOG_BASE=${TMODE}_${BW}MHz_iperf_ul
              generic_iperf $EPC_VM_CMDS $EPC_VM_IP_ADDR $REAL_EPC_IP_ADDR $VM_CMDS $VM_IP_ADDR $UE_IP_ADDR $THROUGHPUT $CURR_IPERF_LOG_BASE 0 0
              scp -o StrictHostKeyChecking=no ubuntu@$EPC_VM_IP_ADDR:/home/ubuntu/${CURR_IPERF_LOG_BASE}_server.txt $ARCHIVES_LOC
              scp -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR:/home/ubuntu/${CURR_IPERF_LOG_BASE}_client.txt $ARCHIVES_LOC
              check_iperf $ARCHIVES_LOC/$CURR_IPERF_LOG_BASE $THROUGHPUT

              full_terminate

          done
        done

        if [ -d $JENKINS_WKSP/flexran ]
        then
            echo "############################################################"
            echo "Flexran testing is possible"
            echo "############################################################"
            local j="0"
            while [ $j -lt 20 ]
            do
                if [ -e $JENKINS_WKSP/flexran/flexran_build_complete.txt ]
                then
                    echo "Found an proper flexran controller vm build"
                    j="100"
                else
                    j=$[$j+1]
                    sleep 30
                fi
            done
            if [ $j -lt 50 ]
            then
                echo "ERROR: compiling flexran controller on vm went wrong"
                terminate_epc $EPC_VM_CMDS $EPC_VM_IP_ADDR
                echo "TEST_KO" > $ARCHIVES_LOC/test_final_status.log
                STATUS=-1
                return
            fi
            FLEXRAN_CTL_VM_NAME=`echo $VM_NAME | sed -e "s#basic-sim#flexran-rtc#"`
            FLEXRAN_CTL_VM_CMDS=`echo $VM_CMDS | sed -e "s#cmds#flexran-rtc-cmds#"`
            IS_FLEXRAN_VM_ALIVE=`uvt-kvm list | grep -c $FLEXRAN_CTL_VM_NAME`
            if [ $IS_FLEXRAN_VM_ALIVE -eq 0 ]
            then
                echo "ERROR: Flexran Ctl VM is not alive"
                terminate_epc $EPC_VM_CMDS $EPC_VM_IP_ADDR
                full_basic_sim_destroy
                echo "TEST_KO" > $ARCHIVES_LOC/test_final_status.log
                STATUS=-1
                return
            fi
            uvt-kvm wait $FLEXRAN_CTL_VM_NAME --insecure
            FLEXRAN_CTL_VM_IP_ADDR=`uvt-kvm ip $FLEXRAN_CTL_VM_NAME`

            echo "$FLEXRAN_CTL_VM_NAME has for IP addr = $FLEXRAN_CTL_VM_IP_ADDR"
            echo "FLEXRAN_CTL_VM_CMDS     = $FLEXRAN_CTL_VM_CMDS"
            echo "############################################################"
            echo "Starting the FLEXRAN CONTROLLER"
            echo "############################################################"
            start_flexran_ctrl $FLEXRAN_CTL_VM_CMDS $FLEXRAN_CTL_VM_IP_ADDR
            query_flexran_ctrl_status $FLEXRAN_CTL_VM_CMDS $FLEXRAN_CTL_VM_IP_ADDR 01_no_enb_connected

            echo "############################################################"
            echo "Starting the eNB in FDD-5MHz mode with Flexran ON"
            echo "############################################################"
            CURRENT_ENB_LOG_FILE=fdd_05fMHz_enb.log
            start_basic_sim_enb $VM_CMDS $VM_IP_ADDR $EPC_VM_IP_ADDR $CURRENT_ENB_LOG_FILE 25 lte-fdd-basic-sim.conf $FLEXRAN_CTL_VM_IP_ADDR
            query_flexran_ctrl_status $FLEXRAN_CTL_VM_CMDS $FLEXRAN_CTL_VM_IP_ADDR 02_enb_connected

            echo "############################################################"
            echo "Starting the UE in FDD-5MHz mode"
            echo "############################################################"
            CURRENT_UE_LOG_FILE=fdd_05fMHz_ue.log
            start_basic_sim_ue $VM_CMDS $VM_IP_ADDR $CURRENT_UE_LOG_FILE 25 2680
            if [ $UE_SYNC -eq 0 ]
            then
                echo "Problem w/ eNB and UE not syncing"
                terminate_enb_ue_basic_sim $VM_CMDS $VM_IP_ADDR 0
                scp -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/$CURRENT_ENB_LOG_FILE $ARCHIVES_LOC
                scp -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/$CURRENT_UE_LOG_FILE $ARCHIVES_LOC
                recover_core_dump $VM_CMDS $VM_IP_ADDR $ARCHIVES_LOC/$CURRENT_ENB_LOG_FILE $ARCHIVES_LOC
                terminate_epc $EPC_VM_CMDS $EPC_VM_IP_ADDR
                stop_flexran_ctrl $FLEXRAN_CTL_VM_CMDS $FLEXRAN_CTL_VM_IP_ADDR
                echo "TEST_KO" > $ARCHIVES_LOC/test_final_status.log
                STATUS=-1
                return
            fi
            query_flexran_ctrl_status $FLEXRAN_CTL_VM_CMDS $FLEXRAN_CTL_VM_IP_ADDR 03_enb_ue_connected
            get_ue_ip_addr $VM_CMDS $VM_IP_ADDR 1

            sleep 30
            echo "############################################################"
            echo "Terminate enb/ue simulators"
            echo "############################################################"
            terminate_enb_ue_basic_sim $VM_CMDS $VM_IP_ADDR 0
            scp -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/$CURRENT_ENB_LOG_FILE $ARCHIVES_LOC
            scp -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/$CURRENT_UE_LOG_FILE $ARCHIVES_LOC
            recover_core_dump $VM_CMDS $VM_IP_ADDR $ARCHIVES_LOC/$CURRENT_ENB_LOG_FILE $ARCHIVES_LOC
            sleep 10

            echo "############################################################"
            echo "Stopping the FLEXRAN CONTROLLER"
            echo "############################################################"
            stop_flexran_ctrl $FLEXRAN_CTL_VM_CMDS $FLEXRAN_CTL_VM_IP_ADDR
            scp -o StrictHostKeyChecking=no ubuntu@$FLEXRAN_CTL_VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/flexran_ctl_*.log $ARCHIVES_LOC
        fi

        echo "############################################################"
        echo "Terminate EPC"
        echo "############################################################"

        if [[ "$EPC_IPADDR" == "" ]]
        then
            terminate_epc $EPC_VM_CMDS $EPC_VM_IP_ADDR
        fi

        full_basic_sim_destroy

        echo "############################################################"
        echo "Checking run status"
        echo "############################################################"

        if [ $PING_STATUS -ne 0 ]; then STATUS=-1; fi
        if [ $IPERF_STATUS -ne 0 ]; then STATUS=-1; fi

        if [ $STATUS -eq 0 ]
        then
            echo "TEST_OK" > $ARCHIVES_LOC/test_final_status.log
        else
            echo "TEST_KO" > $ARCHIVES_LOC/test_final_status.log
        fi
    fi

    if [[ "$RUN_OPTIONS" == "complex" ]] && [[ $VM_NAME =~ .*-rf-sim.* ]]
    then
        PING_STATUS=0
        IPERF_STATUS=0
        if [ -d $ARCHIVES_LOC ]
        then
            rm -Rf $ARCHIVES_LOC
        fi
        mkdir --parents $ARCHIVES_LOC
        if [[ "$EPC_IPADDR" == "" ]]
        then
        # Creating a VM for EPC and installing SW
            EPC_VM_NAME=`echo $VM_NAME | sed -e "s#rf-sim#epc#"`
            EPC_VM_CMDS=${EPC_VM_NAME}_cmds.txt
            LTEBOX=0
            install_epc_on_vm $EPC_VM_NAME $EPC_VM_CMDS
            EPC_VM_IP_ADDR=`uvt-kvm ip $EPC_VM_NAME`
        fi
        #EPC_CONFIGS=("wS1" "noS1")
        #TRANS_MODES=("fdd" "tdd")
        #BW_CASES=(05 10 20)
        EPC_CONFIGS=("wS1" "noS1")
        TRANS_MODES=("fdd")
        BW_CASES=(05 10)
        for CN_CONFIG in ${EPC_CONFIGS[@]}
        do
          if [[ $CN_CONFIG =~ .*wS1.* ]]
          then
              if [[ "$EPC_IPADDR" ==  "" ]]
              then
                  echo "############################################################"
                  echo "Start EPC for the wS1 configuration"
                  echo "############################################################"
                  start_epc $EPC_VM_NAME $EPC_VM_CMDS $EPC_VM_IP_ADDR
                  # Retrieve EPC real IP address
                  retrieve_real_epc_ip_addr $EPC_VM_NAME $EPC_VM_CMDS $EPC_VM_IP_ADDR
                  S1_NOS1_CFG=1
              else
                  echo "############################################################"
                  echo "Using external EPC " $EPC_IPADDR
                  echo "############################################################"
                  $EPC_VM_IP_ADDR=$EPC_IPADDR
                  S1_NOS1_CFG=1
                  LTEBOX=0
              fi
          else
              if [[ "$EPC_IPADDR" ==  "" ]]
              then
                  echo "############################################################"
                  echo "Terminate EPC"
                  echo "############################################################"
                  terminate_epc $EPC_VM_CMDS $EPC_VM_IP_ADDR

                  echo "############################################################"
                  echo "Running now in a no-S1 "
                  echo "############################################################"
                  S1_NOS1_CFG=0
              fi
          fi
          for TMODE in ${TRANS_MODES[@]}
          do
              if [[ $TMODE =~ .*fdd.* ]]
              then
                  CONF_FILE=enb.band7.tm1.50PRB.usrpb210.conf
                  FREQUENCY=2680
              else
                  CONF_FILE=enb.band40.tm1.50PRB.FairScheduler.usrpb210.conf
                  FREQUENCY=2350
              fi
              for BW in ${BW_CASES[@]}
              do
                  if [[ $BW =~ .*05.* ]]; then PRB=25; fi
                  if [[ $BW =~ .*10.* ]]; then PRB=50; fi
                  if [[ $BW =~ .*20.* ]]; then PRB=100; fi

                  echo "############################################################"
                  echo "${CN_CONFIG} : Starting the eNB in ${TMODE}-${BW}MHz mode"
                  echo "############################################################"
                  CURRENT_ENB_LOG_FILE=${TMODE}_${BW}MHz_${CN_CONFIG}_enb.log
                  start_rf_sim_enb $ENB_VM_CMDS "$ENB_VM_IP_ADDR" "$EPC_VM_IP_ADDR" $CURRENT_ENB_LOG_FILE $PRB $CONF_FILE $S1_NOS1_CFG

                  echo "############################################################"
                  echo "${CN_CONFIG} : Starting the UE"
                  echo "############################################################"
                  CURRENT_UE_LOG_FILE=${TMODE}_${BW}MHz_${CN_CONFIG}_ue.log
                  start_rf_sim_ue $UE_VM_CMDS $UE_VM_IP_ADDR $ENB_VM_IP_ADDR $CURRENT_UE_LOG_FILE $PRB $FREQUENCY $S1_NOS1_CFG 0
                  if [ $UE_SYNC -eq 0 ]
                  then
                      echo "Problem w/ eNB and UE not syncing"
                      terminate_enb_ue_basic_sim $ENB_VM_CMDS $ENB_VM_IP_ADDR 1
                      terminate_enb_ue_basic_sim $UE_VM_CMDS $UE_VM_IP_ADDR 2
                      scp -o StrictHostKeyChecking=no ubuntu@$ENB_VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/$CURRENT_ENB_LOG_FILE $ARCHIVES_LOC
                      scp -o StrictHostKeyChecking=no ubuntu@$UE_VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/$CURRENT_UE_LOG_FILE $ARCHIVES_LOC
                      #if [ $S1_NOS1_CFG -eq 1 ]
                      #then
                      #    terminate_epc $EPC_VM_CMDS $EPC_VM_IP_ADDR
                      #fi
                      # Now we keep running
                      STATUS=-1
                      break
                  fi

                  if [ $S1_NOS1_CFG -eq 1 ]
                  then
                      get_ue_ip_addr $UE_VM_CMDS $UE_VM_IP_ADDR 1

                      echo "############################################################"
                      echo "${CN_CONFIG} : Pinging the EPC from UE"
                      echo "############################################################"
                      PING_LOG_FILE=${TMODE}_${BW}MHz_${CN_CONFIG}_ping_epc.log
                      ping_epc_ip_addr $UE_VM_CMDS $UE_VM_IP_ADDR $REAL_EPC_IP_ADDR $PING_LOG_FILE 1 0
                      scp -o StrictHostKeyChecking=no ubuntu@$UE_VM_IP_ADDR:/home/ubuntu/$PING_LOG_FILE $ARCHIVES_LOC
                      check_ping_result $ARCHIVES_LOC/$PING_LOG_FILE 20
                  else
                      get_enb_noS1_ip_addr $ENB_VM_CMDS $ENB_VM_IP_ADDR

                      echo "############################################################"
                      echo "${CN_CONFIG} : Pinging the eNB from UE"
                      echo "############################################################"
                      PING_LOG_FILE=${TMODE}_${BW}MHz_${CN_CONFIG}_ping_enb_from_ue.log
                      ping_epc_ip_addr $UE_VM_CMDS $UE_VM_IP_ADDR $ENB_IP_ADDR $PING_LOG_FILE 1 0
                      scp -o StrictHostKeyChecking=no ubuntu@$UE_VM_IP_ADDR:/home/ubuntu/$PING_LOG_FILE $ARCHIVES_LOC
                      check_ping_result $ARCHIVES_LOC/$PING_LOG_FILE 20
                  fi

                  if [ $S1_NOS1_CFG -eq 1 ]
                  then
                      echo "############################################################"
                      echo "${CN_CONFIG} : Pinging the UE from EPC"
                      echo "############################################################"
                      PING_LOG_FILE=${TMODE}_${BW}MHz_${CN_CONFIG}_ping_ue.log
                      ping_ue_ip_addr $EPC_VM_CMDS $EPC_VM_IP_ADDR $UE_IP_ADDR $PING_LOG_FILE 0
                      scp -o StrictHostKeyChecking=no ubuntu@$EPC_VM_IP_ADDR:/home/ubuntu/$PING_LOG_FILE $ARCHIVES_LOC
                      check_ping_result $ARCHIVES_LOC/$PING_LOG_FILE 20
                  else
                      echo "############################################################"
                      echo "${CN_CONFIG} : Pinging the UE from eNB"
                      echo "############################################################"
                      get_ue_ip_addr $UE_VM_CMDS $UE_VM_IP_ADDR 1
                      PING_LOG_FILE=${TMODE}_${BW}MHz_${CN_CONFIG}_ping_from_enb_ue.log
                      ping_enb_ip_addr $ENB_VM_CMDS $ENB_VM_IP_ADDR $UE_IP_ADDR $PING_LOG_FILE 0
                      scp -o StrictHostKeyChecking=no ubuntu@$ENB_VM_IP_ADDR:/home/ubuntu/$PING_LOG_FILE $ARCHIVES_LOC
                      check_ping_result $ARCHIVES_LOC/$PING_LOG_FILE 20
                  fi

                  if [ $S1_NOS1_CFG -eq 1 ]
                  then
                      echo "############################################################"
                      echo "${CN_CONFIG} : iperf DL -- UE is server and EPC is client"
                      echo "############################################################"
                      IPERF_LOG_FILE=${TMODE}_${BW}MHz_${CN_CONFIG}_iperf_dl
                      get_ue_ip_addr $UE_VM_CMDS $UE_VM_IP_ADDR 1
                      THROUGHPUT=10
                      generic_iperf $UE_VM_CMDS $UE_VM_IP_ADDR $UE_IP_ADDR $EPC_VM_CMDS $EPC_VM_IP_ADDR $REAL_EPC_IP_ADDR $THROUGHPUT $IPERF_LOG_FILE 1 0
                      scp -o StrictHostKeyChecking=no ubuntu@$UE_VM_IP_ADDR:/home/ubuntu/${IPERF_LOG_FILE}_server.txt $ARCHIVES_LOC
                      scp -o StrictHostKeyChecking=no ubuntu@$EPC_VM_IP_ADDR:/home/ubuntu/${IPERF_LOG_FILE}_client.txt $ARCHIVES_LOC
                      check_iperf $ARCHIVES_LOC/$IPERF_LOG_FILE $THROUGHPUT

                      echo "############################################################"
                      echo "${CN_CONFIG} : iperf UL -- EPC is server and UE is client"
                      echo "############################################################"
                      IPERF_LOG_FILE=${TMODE}_${BW}MHz_${CN_CONFIG}_iperf_ul
                      THROUGHPUT=2
                      get_ue_ip_addr $UE_VM_CMDS $UE_VM_IP_ADDR 1
                      generic_iperf $EPC_VM_CMDS $EPC_VM_IP_ADDR $REAL_EPC_IP_ADDR $UE_VM_CMDS $UE_VM_IP_ADDR $UE_IP_ADDR $THROUGHPUT $IPERF_LOG_FILE 1 0
                      scp -o StrictHostKeyChecking=no ubuntu@$EPC_VM_IP_ADDR:/home/ubuntu/${IPERF_LOG_FILE}_server.txt $ARCHIVES_LOC
                      scp -o StrictHostKeyChecking=no ubuntu@$UE_VM_IP_ADDR:/home/ubuntu/${IPERF_LOG_FILE}_client.txt $ARCHIVES_LOC
                      check_iperf $ARCHIVES_LOC/$IPERF_LOG_FILE $THROUGHPUT
                  else
                      echo "############################################################"
                      echo "${CN_CONFIG} : iperf DL -- UE is server and eNB is client"
                      echo "############################################################"
                      get_enb_noS1_ip_addr $ENB_VM_CMDS $ENB_VM_IP_ADDR
                      IPERF_LOG_FILE=${TMODE}_${BW}MHz_${CN_CONFIG}_iperf_dl
                      get_ue_ip_addr $UE_VM_CMDS $UE_VM_IP_ADDR 1
                      THROUGHPUT=10
                      generic_iperf $UE_VM_CMDS $UE_VM_IP_ADDR $UE_IP_ADDR $ENB_VM_CMDS $ENB_VM_IP_ADDR $ENB_IP_ADDR $THROUGHPUT $IPERF_LOG_FILE 1 0
                      scp -o StrictHostKeyChecking=no ubuntu@$UE_VM_IP_ADDR:/home/ubuntu/${IPERF_LOG_FILE}_server.txt $ARCHIVES_LOC
                      scp -o StrictHostKeyChecking=no ubuntu@$ENB_VM_IP_ADDR:/home/ubuntu/${IPERF_LOG_FILE}_client.txt $ARCHIVES_LOC
                      check_iperf $ARCHIVES_LOC/$IPERF_LOG_FILE $THROUGHPUT

                      echo "############################################################"
                      echo "${CN_CONFIG} : iperf UL -- eNB is server and UE is client"
                      echo "############################################################"
                      IPERF_LOG_FILE=${TMODE}_${BW}MHz_${CN_CONFIG}_iperf_ul
                      THROUGHPUT=2
                      get_ue_ip_addr $UE_VM_CMDS $UE_VM_IP_ADDR 1
                      generic_iperf $ENB_VM_CMDS $ENB_VM_IP_ADDR $ENB_IP_ADDR $UE_VM_CMDS $UE_VM_IP_ADDR $UE_IP_ADDR $THROUGHPUT $IPERF_LOG_FILE 1 0
                      scp -o StrictHostKeyChecking=no ubuntu@$ENB_VM_IP_ADDR:/home/ubuntu/${IPERF_LOG_FILE}_server.txt $ARCHIVES_LOC
                      scp -o StrictHostKeyChecking=no ubuntu@$UE_VM_IP_ADDR:/home/ubuntu/${IPERF_LOG_FILE}_client.txt $ARCHIVES_LOC
                      check_iperf $ARCHIVES_LOC/$IPERF_LOG_FILE $THROUGHPUT
                  fi

                  echo "############################################################"
                  echo "${CN_CONFIG} : Terminate enb/ue simulators"
                  echo "############################################################"
                  terminate_enb_ue_basic_sim $ENB_VM_CMDS $ENB_VM_IP_ADDR 1
                  terminate_enb_ue_basic_sim $UE_VM_CMDS $UE_VM_IP_ADDR 2
                  scp -o StrictHostKeyChecking=no ubuntu@$ENB_VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/$CURRENT_ENB_LOG_FILE $ARCHIVES_LOC
                  scp -o StrictHostKeyChecking=no ubuntu@$UE_VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/$CURRENT_UE_LOG_FILE $ARCHIVES_LOC

              done
          done
        done

        ####################
        ## MSMS CASE noS1 ##
        ####################
        CONF_FILE=lte-fdd-mbms-basic-sim.conf
        CN_CONFIG="noS1"
        S1_NOS1_CFG=0
        LTEBOX=0
        TMODE="fdd"
        FREQUENCY=2680
        BW_CASES=(05)
        MBMS_STATUS=0

        for BW in ${BW_CASES[@]}
        do
            if [[ $BW =~ .*05.* ]]; then PRB=25; fi
            if [[ $BW =~ .*10.* ]]; then PRB=50; fi
            if [[ $BW =~ .*20.* ]]; then PRB=100; fi

            echo "############################################################"
            echo "${CN_CONFIG} : Starting the eNB with MSMS in ${TMODE}-${BW}MHz mode"
            echo "############################################################"
            CURRENT_ENB_LOG_FILE=${TMODE}_${BW}MHz_${CN_CONFIG}_enb_mbms.log
            start_rf_sim_enb $ENB_VM_CMDS "$ENB_VM_IP_ADDR" "$EPC_VM_IP_ADDR" $CURRENT_ENB_LOG_FILE $PRB $CONF_FILE $S1_NOS1_CFG

            echo "############################################################"
            echo "${CN_CONFIG} : Starting the UE"
            echo "############################################################"
            CURRENT_UE_LOG_FILE=${TMODE}_${BW}MHz_${CN_CONFIG}_ue_mbms.log
            start_rf_sim_ue $UE_VM_CMDS $UE_VM_IP_ADDR $ENB_VM_IP_ADDR $CURRENT_UE_LOG_FILE $PRB $FREQUENCY $S1_NOS1_CFG 1
            if [ $UE_SYNC -eq 0 ]
            then
                echo "Problem w/ eNB and UE not syncing"
                terminate_enb_ue_basic_sim $ENB_VM_CMDS $ENB_VM_IP_ADDR 1
                terminate_enb_ue_basic_sim $UE_VM_CMDS $UE_VM_IP_ADDR 2
                scp -o StrictHostKeyChecking=no ubuntu@$ENB_VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/$CURRENT_ENB_LOG_FILE $ARCHIVES_LOC
                scp -o StrictHostKeyChecking=no ubuntu@$UE_VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/$CURRENT_UE_LOG_FILE $ARCHIVES_LOC
                STATUS=-1
                break
            fi

            echo "############################################################"
            echo "${CN_CONFIG} : iperf DL -- UE is server and eNB is client"
            echo "############################################################"
            get_enb_mbms_noS1_ip_addr $ENB_VM_CMDS $ENB_VM_IP_ADDR
            IPERF_LOG_FILE=${TMODE}_${BW}MHz_${CN_CONFIG}_iperf_dl_mbms
            get_ue_mbms_ip_addr $UE_VM_CMDS $UE_VM_IP_ADDR 1
            THROUGHPUT=2
            generic_iperf $UE_VM_CMDS $UE_VM_IP_ADDR $UE_IP_ADDR $ENB_VM_CMDS $ENB_VM_IP_ADDR $ENB_IP_ADDR $THROUGHPUT $IPERF_LOG_FILE 1 0
            scp -o StrictHostKeyChecking=no ubuntu@$UE_VM_IP_ADDR:/home/ubuntu/${IPERF_LOG_FILE}_server.txt $ARCHIVES_LOC
            scp -o StrictHostKeyChecking=no ubuntu@$ENB_VM_IP_ADDR:/home/ubuntu/${IPERF_LOG_FILE}_client.txt $ARCHIVES_LOC
            #check_iperf $ARCHIVES_LOC/$IPERF_LOG_FILE $THROUGHPUT

            echo "############################################################"
            echo "${CN_CONFIG} : Terminate enb/ue simulators"
            echo "############################################################"
            terminate_enb_ue_basic_sim $ENB_VM_CMDS $ENB_VM_IP_ADDR 1
            terminate_enb_ue_basic_sim $UE_VM_CMDS $UE_VM_IP_ADDR 2
            scp -o StrictHostKeyChecking=no ubuntu@$ENB_VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/$CURRENT_ENB_LOG_FILE $ARCHIVES_LOC
            scp -o StrictHostKeyChecking=no ubuntu@$UE_VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/$CURRENT_UE_LOG_FILE $ARCHIVES_LOC
            NB_UE_MBMS_MESSAGES=`egrep -c "TRIED TO PUSH MBMS DATA TO" $ARCHIVES_LOC/$CURRENT_UE_LOG_FILE`
            if [ $NB_UE_MBMS_MESSAGES -eq 0 ]; then MBMS_STATUS=-1; fi

        done

        ####################
        ## FeMBMS CASE noS1 ##
        ####################
        CONF_FILE=lte-fdd-fembms-basic-sim.conf
        CN_CONFIG="noS1"
        S1_NOS1_CFG=0
        LTEBOX=0
        TMODE="fdd"
        FREQUENCY=2680
        BW_CASES=(05)
        FeMBMS_STATUS=0

        for BW in ${BW_CASES[@]}
        do
            if [[ $BW =~ .*05.* ]]; then PRB=25; fi
            if [[ $BW =~ .*10.* ]]; then PRB=50; fi
            if [[ $BW =~ .*20.* ]]; then PRB=100; fi

            echo "############################################################"
            echo "${CN_CONFIG} : Starting the eNB with MSMS in ${TMODE}-${BW}MHz mode"
            echo "############################################################"
            CURRENT_ENB_LOG_FILE=${TMODE}_${BW}MHz_${CN_CONFIG}_enb_fembms.log
            start_rf_sim_enb $ENB_VM_CMDS "$ENB_VM_IP_ADDR" "$EPC_VM_IP_ADDR" $CURRENT_ENB_LOG_FILE $PRB $CONF_FILE $S1_NOS1_CFG

            echo "############################################################"
            echo "${CN_CONFIG} : Starting the UE"
            echo "############################################################"
            CURRENT_UE_LOG_FILE=${TMODE}_${BW}MHz_${CN_CONFIG}_ue_fembms.log
            start_rf_sim_ue $UE_VM_CMDS $UE_VM_IP_ADDR $ENB_VM_IP_ADDR $CURRENT_UE_LOG_FILE $PRB $FREQUENCY $S1_NOS1_CFG 1
            if [ $UE_SYNC -eq 0 ]
            then
                echo "Problem w/ eNB and UE not syncing"
                terminate_enb_ue_basic_sim $ENB_VM_CMDS $ENB_VM_IP_ADDR 1
                terminate_enb_ue_basic_sim $UE_VM_CMDS $UE_VM_IP_ADDR 2
                scp -o StrictHostKeyChecking=no ubuntu@$ENB_VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/$CURRENT_ENB_LOG_FILE $ARCHIVES_LOC
                scp -o StrictHostKeyChecking=no ubuntu@$UE_VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/$CURRENT_UE_LOG_FILE $ARCHIVES_LOC
                STATUS=-1
                break
            fi

            echo "############################################################"
            echo "${CN_CONFIG} : iperf DL -- UE is server and eNB is client"
            echo "############################################################"
            get_enb_mbms_noS1_ip_addr $ENB_VM_CMDS $ENB_VM_IP_ADDR
            IPERF_LOG_FILE=${TMODE}_${BW}MHz_${CN_CONFIG}_iperf_dl_fembms
            get_ue_mbms_ip_addr $UE_VM_CMDS $UE_VM_IP_ADDR 1
            THROUGHPUT=2
            generic_iperf $UE_VM_CMDS $UE_VM_IP_ADDR $UE_IP_ADDR $ENB_VM_CMDS $ENB_VM_IP_ADDR $ENB_IP_ADDR $THROUGHPUT $IPERF_LOG_FILE 1 0
            scp -o StrictHostKeyChecking=no ubuntu@$UE_VM_IP_ADDR:/home/ubuntu/${IPERF_LOG_FILE}_server.txt $ARCHIVES_LOC
            scp -o StrictHostKeyChecking=no ubuntu@$ENB_VM_IP_ADDR:/home/ubuntu/${IPERF_LOG_FILE}_client.txt $ARCHIVES_LOC
            #check_iperf $ARCHIVES_LOC/$IPERF_LOG_FILE $THROUGHPUT

            echo "############################################################"
            echo "${CN_CONFIG} : Terminate enb/ue simulators"
            echo "############################################################"
            terminate_enb_ue_basic_sim $ENB_VM_CMDS $ENB_VM_IP_ADDR 1
            terminate_enb_ue_basic_sim $UE_VM_CMDS $UE_VM_IP_ADDR 2
            scp -o StrictHostKeyChecking=no ubuntu@$ENB_VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/$CURRENT_ENB_LOG_FILE $ARCHIVES_LOC
            scp -o StrictHostKeyChecking=no ubuntu@$UE_VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/$CURRENT_UE_LOG_FILE $ARCHIVES_LOC
            NB_UE_FeMBMS_MESSAGES=`egrep -c "TRIED TO PUSH MBMS DATA TO" $ARCHIVES_LOC/$CURRENT_UE_LOG_FILE`
            if [ $NB_UE_FeMBMS_MESSAGES -eq 0 ]; then FeMBMS_STATUS=-1; fi

        done

        full_l2_sim_destroy

        echo "############################################################"
        echo "Checking run status"
        echo "############################################################"

        if [ $PING_STATUS -ne 0 ]; then STATUS=-1; fi
        if [ $IPERF_STATUS -ne 0 ]; then STATUS=-1; fi
        if [ $MBMS_STATUS -eq 0 ]
        then
            echo "LTE MBMS RFSIM seems OK"
        else
            echo "LTE MBMS RFSIM seems to FAIL"
            STATUS=-1
        fi
        if [ $FeMBMS_STATUS -eq 0 ]
        then
            echo "LTE FeMBMS RFSIM seems OK"
        else
            echo "LTE FeMBMS RFSIM seems to FAIL"
            STATUS=-1
        fi
        if [ $STATUS -eq 0 ]
        then
            echo "LTE RFSIM seems OK"
            echo "LTE: TEST_OK" > $ARCHIVES_LOC/test_final_status.log
        else
            echo "LTE RFSIM seems to FAIL"
            echo "LTE: TEST_KO" > $ARCHIVES_LOC/test_final_status.log
        fi

    fi

    if [[ "$RUN_OPTIONS" == "complex" ]] && [[ $VM_NAME =~ .*-rf-sim.* ]]
    then
        echo "############################################################"
        echo "SA TEST"
        echo "############################################################"
        #SA test, attention : has a different config file from the rest of the test
        CN_CONFIG="noS1"
        CONF_FILE=gnb.band78.sa.fr1.106PRB.usrpb210.conf 
        S1_NOS1_CFG=0
        PRB=106
        FREQUENCY=3510

        if [ ! -d $ARCHIVES_LOC ]
        then
            mkdir --parents $ARCHIVES_LOC
        fi

        local try_cnt=0
        NR_STATUS=0

        ######### start of SA TEST loop
        while [ $try_cnt -lt 5 ] #5 because it hardly succeed within CI
        do

            SYNC_STATUS=0
            SA_STATUS=0
            rm -f $ARCHIVES_LOC/tdd_${PRB}prb_${CN_CONFIG}*sa_test.log

            echo "############################################################"
            echo "${CN_CONFIG} : Starting the gNB"
            echo "############################################################"
            CURRENT_GNB_LOG_FILE=tdd_${PRB}prb_${CN_CONFIG}_gnb_sa_test.log
            #last argument = 2 is to enable --sa for SA test
            start_rf_sim_gnb $GNB_VM_CMDS "$GNB_VM_IP_ADDR" $CURRENT_GNB_LOG_FILE $PRB $CONF_FILE $S1_NOS1_CFG 2

            echo "############################################################"
            echo "${CN_CONFIG} : Starting the NR-UE"
            echo "############################################################"
            CURRENT_NR_UE_LOG_FILE=tdd_${PRB}prb_${CN_CONFIG}_ue_sa_test.log
            #last argument = 2 is to enable --sa for SA test
            start_rf_sim_nr_ue $NR_UE_VM_CMDS $NR_UE_VM_IP_ADDR $GNB_VM_IP_ADDR $CURRENT_NR_UE_LOG_FILE $PRB $FREQUENCY $S1_NOS1_CFG 2
            if [ $NR_UE_SYNC -eq 0 ]
            then
                echo "Problem w/ gNB and NR-UE not syncing"
                terminate_enb_ue_basic_sim $NR_UE_VM_CMDS $NR_UE_VM_IP_ADDR 2
                terminate_enb_ue_basic_sim $GNB_VM_CMDS $GNB_VM_IP_ADDR 1
                scp -o StrictHostKeyChecking=no ubuntu@$GNB_VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/$CURRENT_GNB_LOG_FILE $ARCHIVES_LOC
                scp -o StrictHostKeyChecking=no ubuntu@$NR_UE_VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/$CURRENT_NR_UE_LOG_FILE $ARCHIVES_LOC
                SYNC_STATUS=-1
                try_cnt=$((try_cnt+1))
                continue
            fi

            echo "############################################################"
            echo "${CN_CONFIG} : Terminate gNB/NR-UE simulators"
            echo "############################################################"
            sleep 20
            terminate_enb_ue_basic_sim $NR_UE_VM_CMDS $NR_UE_VM_IP_ADDR 2
            terminate_enb_ue_basic_sim $GNB_VM_CMDS $GNB_VM_IP_ADDR 1
            scp -o StrictHostKeyChecking=no ubuntu@$GNB_VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/$CURRENT_GNB_LOG_FILE $ARCHIVES_LOC
            scp -o StrictHostKeyChecking=no ubuntu@$NR_UE_VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/$CURRENT_NR_UE_LOG_FILE $ARCHIVES_LOC

            #check SA markers in gNB and NR UE log files
            echo "############################################################"
            echo "${CN_CONFIG} : Checking SA on gNB / NR-UE"
            echo "############################################################"

            # Proper check to be done when SA test is working!
            check_sa_result $ARCHIVES_LOC/$CURRENT_GNB_LOG_FILE $ARCHIVES_LOC/$CURRENT_NR_UE_LOG_FILE
            if [ $SA_STATUS -ne 0 ]
            then
                echo "SA test NOT OK"
                echo "try_cnt = " $try_cnt
                try_cnt=$((try_cnt+1))
            else
                try_cnt=$((try_cnt+10))
            fi
        done
        ########### end SA test

        sleep 30



        echo "############################################################"
        echo "RA TEST FR2"
        echo "############################################################"
        #RA FR2 test, attention : has a different config file from the rest of the test
        CN_CONFIG="noS1"
        CONF_FILE=gnb.band261.tm1.32PRB.usrpn300.conf
        S1_NOS1_CFG=0
        PRB=32
        FREQUENCY=28000 #28GHz

        if [ ! -d $ARCHIVES_LOC ]
        then
            mkdir --parents $ARCHIVES_LOC
        fi

        local try_cnt=0
        NR_STATUS=0

        ######### start of RA TEST loop
        while [ $try_cnt -lt 5 ] #5 because it hardly succeed within CI
        do

            SYNC_STATUS=0
            RA_FR2_STATUS=0
            rm -f $ARCHIVES_LOC/tdd_${PRB}prb_${CN_CONFIG}*ra_fr2_test.log

            echo "############################################################"
            echo "${CN_CONFIG} : Starting the gNB"
            echo "############################################################"
            CURRENT_GNB_LOG_FILE=tdd_${PRB}prb_${CN_CONFIG}_gnb_ra_fr2_test.log
            #last argument = 1 is to enable --do-ra for RA test
            start_rf_sim_gnb $GNB_VM_CMDS "$GNB_VM_IP_ADDR" $CURRENT_GNB_LOG_FILE $PRB $CONF_FILE $S1_NOS1_CFG 1

            echo "############################################################"
            echo "${CN_CONFIG} : Starting the NR-UE"
            echo "############################################################"
            CURRENT_NR_UE_LOG_FILE=tdd_${PRB}prb_${CN_CONFIG}_ue_ra_fr2_test.log
            #last argument = 1 is to enable --do-ra for RA test
            start_rf_sim_nr_ue $NR_UE_VM_CMDS $NR_UE_VM_IP_ADDR $GNB_VM_IP_ADDR $CURRENT_NR_UE_LOG_FILE $PRB $FREQUENCY $S1_NOS1_CFG 1
            if [ $NR_UE_SYNC -eq 0 ]
            then
                echo "Problem w/ gNB and NR-UE not syncing"
                terminate_enb_ue_basic_sim $NR_UE_VM_CMDS $NR_UE_VM_IP_ADDR 2
                terminate_enb_ue_basic_sim $GNB_VM_CMDS $GNB_VM_IP_ADDR 1
                scp -o StrictHostKeyChecking=no ubuntu@$GNB_VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/$CURRENT_GNB_LOG_FILE $ARCHIVES_LOC
                scp -o StrictHostKeyChecking=no ubuntu@$NR_UE_VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/$CURRENT_NR_UE_LOG_FILE $ARCHIVES_LOC
                SYNC_STATUS=-1
                try_cnt=$((try_cnt+1))
                continue
            fi

            echo "############################################################"
            echo "${CN_CONFIG} : Terminate gNB/NR-UE simulators"
            echo "############################################################"
            sleep 20
            terminate_enb_ue_basic_sim $NR_UE_VM_CMDS $NR_UE_VM_IP_ADDR 2
            terminate_enb_ue_basic_sim $GNB_VM_CMDS $GNB_VM_IP_ADDR 1
            scp -o StrictHostKeyChecking=no ubuntu@$GNB_VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/$CURRENT_GNB_LOG_FILE $ARCHIVES_LOC
            scp -o StrictHostKeyChecking=no ubuntu@$NR_UE_VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/$CURRENT_NR_UE_LOG_FILE $ARCHIVES_LOC

            #check RA markers in gNB and NR UE log files
            echo "############################################################"
            echo "${CN_CONFIG} : Checking FR2 RA on gNB / NR-UE"
            echo "############################################################"

            # Proper check to be done when RA test is working!
            check_ra_result $ARCHIVES_LOC/$CURRENT_GNB_LOG_FILE $ARCHIVES_LOC/$CURRENT_NR_UE_LOG_FILE
            if [ $RA_FR2_STATUS -ne 0 ]
            then
                echo "RA FR2 test NOT OK"
                echo "try_cnt = " $try_cnt
                try_cnt=$((try_cnt+1))
            else
                try_cnt=$((try_cnt+10))
            fi
        done
        ########### end RA FR2 test

        sleep 30


        echo "############################################################"
        echo "RA TEST FR1"
        echo "############################################################"
        ######### redefine config for the rest of the test

        CN_CONFIG="noS1"
        CONF_FILE=gnb.band78.tm1.106PRB.usrpn300.conf
        S1_NOS1_CFG=0
        PRB=106
        FREQUENCY=3510

        if [ ! -d $ARCHIVES_LOC ]
        then
            mkdir --parents $ARCHIVES_LOC
        fi

        CN_CONFIG="noS1"
        S1_NOS1_CFG=0

        ######### start of RA TEST loop
        # for the moment only TDD
        TRANS_MODES=("tdd")
        for TMODE in ${TRANS_MODES[@]}
        do
          if [[ $TMODE =~ .*fdd.* ]]
          then
            CONF_FILE=gnb.band66.tm1.106PRB.usrpn300.conf
            PRB=106
            FREQUENCY=37000
          else
            CONF_FILE=gnb.band78.tm1.106PRB.usrpn300.conf
            PRB=106
            FREQUENCY=3510
          fi

          local try_cnt=0
          NR_STATUS=0
          while [ $try_cnt -lt 5 ] #5 because it hardly succeed within CI

          do
            SYNC_STATUS=0
            RA_STATUS=0
            rm -f $ARCHIVES_LOC/${TMODE}_${PRB}prb_${CN_CONFIG}*ra_test.log

            echo "############################################################"
            echo "${CN_CONFIG} : Starting the gNB in ${TMODE} mode (RA TEST)"
            echo "############################################################"
            CURRENT_GNB_LOG_FILE=${TMODE}_${PRB}prb_${CN_CONFIG}_gnb_ra_test.log
            #last argument = 1 is to enable --do-ra for RA test
            start_rf_sim_gnb $GNB_VM_CMDS "$GNB_VM_IP_ADDR" $CURRENT_GNB_LOG_FILE $PRB $CONF_FILE $S1_NOS1_CFG 1

            echo "############################################################"
            echo "${CN_CONFIG} : Starting the NR-UE in ${TMODE} mode (RA TEST)"
            echo "############################################################"
            CURRENT_NR_UE_LOG_FILE=${TMODE}_${PRB}prb_${CN_CONFIG}_ue_ra_test.log
            #last argument = 1 is to enable --do-ra for RA test
            start_rf_sim_nr_ue $NR_UE_VM_CMDS $NR_UE_VM_IP_ADDR $GNB_VM_IP_ADDR $CURRENT_NR_UE_LOG_FILE $PRB $FREQUENCY $S1_NOS1_CFG 1
            if [ $NR_UE_SYNC -eq 0 ]
            then
                echo "Problem w/ gNB and NR-UE not syncing"
                terminate_enb_ue_basic_sim $NR_UE_VM_CMDS $NR_UE_VM_IP_ADDR 2
                terminate_enb_ue_basic_sim $GNB_VM_CMDS $GNB_VM_IP_ADDR 1
                scp -o StrictHostKeyChecking=no ubuntu@$GNB_VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/$CURRENT_GNB_LOG_FILE $ARCHIVES_LOC
                scp -o StrictHostKeyChecking=no ubuntu@$NR_UE_VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/$CURRENT_NR_UE_LOG_FILE $ARCHIVES_LOC
                SYNC_STATUS=-1
                try_cnt=$((try_cnt+1))
                continue
            fi

            echo "############################################################"
            echo "${CN_CONFIG} : Terminate gNB/NR-UE simulators"
            echo "############################################################"
            sleep 20
            terminate_enb_ue_basic_sim $NR_UE_VM_CMDS $NR_UE_VM_IP_ADDR 2
            terminate_enb_ue_basic_sim $GNB_VM_CMDS $GNB_VM_IP_ADDR 1
            scp -o StrictHostKeyChecking=no ubuntu@$GNB_VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/$CURRENT_GNB_LOG_FILE $ARCHIVES_LOC
            scp -o StrictHostKeyChecking=no ubuntu@$NR_UE_VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/$CURRENT_NR_UE_LOG_FILE $ARCHIVES_LOC

            #check RA markers in gNB and NR UE log files
            echo "############################################################"
            echo "${CN_CONFIG} : Checking FR1 RA on gNB / NR-UE"
            echo "############################################################"

            # Proper check to be done when RA test is working!
            check_ra_result $ARCHIVES_LOC/$CURRENT_GNB_LOG_FILE $ARCHIVES_LOC/$CURRENT_NR_UE_LOG_FILE
            if [ $RA_STATUS -ne 0 ]
            then
                echo "RA FR1 test NOT OK"
                echo "try_cnt = " $try_cnt
                try_cnt=$((try_cnt+1))
            else
                try_cnt=$((try_cnt+10))
            fi
          done
        done
        ########### end RA test
        
        sleep 30

        ######### start of PHY TEST loop
        SYNC_STATUS=0
        PING_STATUS=0
        IPERF_STATUS=0
        TRANS_MODES=("fdd tdd")
        for TMODE in ${TRANS_MODES[@]}
        do
          if [[ $TMODE =~ .*fdd.* ]]
          then
            CONF_FILE=gnb.band66.tm1.106PRB.usrpn300.conf
            PRB=106
            FREQUENCY=37000
          else
            CONF_FILE=gnb.band78.tm1.106PRB.usrpn300.conf
            PRB=106
            FREQUENCY=3510
          fi

          try_cnt=0
          while [ $try_cnt -lt 4 ]
          do
            rm -f $ARCHIVES_LOC/${TMODE}_${PRB}prb_${CN_CONFIG}_gnb.log $ARCHIVES_LOC/${TMODE}_${PRB}prb_${CN_CONFIG}_ue.log
            rm -f $ARCHIVES_LOC/${TMODE}_${PRB}prb_${CN_CONFIG}_ping_gnb_from_nrue.log $ARCHIVES_LOC/${TMODE}_${PRB}prb_${CN_CONFIG}_ping_from_gnb_nrue.log
            rm -f $ARCHIVES_LOC/${TMODE}_${PRB}prb_${CN_CONFIG}_iperf_dl*txt $ARCHIVES_LOC/${TMODE}_${PRB}prb_${CN_CONFIG}_iperf_ul*txt

            echo "############################################################"
            echo "${CN_CONFIG} : Starting the gNB in ${TMODE} mode (PHY TEST)"
            echo "############################################################"
            CURRENT_GNB_LOG_FILE=${TMODE}_${PRB}prb_${CN_CONFIG}_gnb.log
            start_rf_sim_gnb $GNB_VM_CMDS "$GNB_VM_IP_ADDR" $CURRENT_GNB_LOG_FILE $PRB $CONF_FILE $S1_NOS1_CFG 0

            echo "############################################################"
            echo "${CN_CONFIG} : Starting the NR-UE in ${TMODE} mode (PHY TEST)"
            echo "############################################################"
            CURRENT_NR_UE_LOG_FILE=${TMODE}_${PRB}prb_${CN_CONFIG}_ue.log
            start_rf_sim_nr_ue $NR_UE_VM_CMDS $NR_UE_VM_IP_ADDR $GNB_VM_IP_ADDR $CURRENT_NR_UE_LOG_FILE $PRB $FREQUENCY $S1_NOS1_CFG 0
            if [ $NR_UE_SYNC -eq 0 ]
            then
                echo "Problem w/ gNB and NR-UE not syncing"
                terminate_enb_ue_basic_sim $NR_UE_VM_CMDS $NR_UE_VM_IP_ADDR 2
                terminate_enb_ue_basic_sim $GNB_VM_CMDS $GNB_VM_IP_ADDR 1
                scp -o StrictHostKeyChecking=no ubuntu@$GNB_VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/$CURRENT_GNB_LOG_FILE $ARCHIVES_LOC
                scp -o StrictHostKeyChecking=no ubuntu@$NR_UE_VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/$CURRENT_NR_UE_LOG_FILE $ARCHIVES_LOC
                SYNC_STATUS=-1
                try_cnt=$((try_cnt+1))
                continue
            fi

            echo "############################################################"
            echo "${CN_CONFIG} : Pinging the gNB from NR-UE"
            echo "############################################################"
            get_enb_noS1_ip_addr $GNB_VM_CMDS $GNB_VM_IP_ADDR
            PING_LOG_FILE=${TMODE}_${PRB}prb_${CN_CONFIG}_ping_gnb_from_nrue.log
            ping_epc_ip_addr $NR_UE_VM_CMDS $NR_UE_VM_IP_ADDR $ENB_IP_ADDR $PING_LOG_FILE 1 0
            scp -o StrictHostKeyChecking=no ubuntu@$NR_UE_VM_IP_ADDR:/home/ubuntu/$PING_LOG_FILE $ARCHIVES_LOC
            check_ping_result $ARCHIVES_LOC/$PING_LOG_FILE 20

            echo "############################################################"
            echo "${CN_CONFIG} : Pinging the NR-UE from gNB"
            echo "############################################################"
            get_ue_ip_addr $NR_UE_VM_CMDS $NR_UE_VM_IP_ADDR 1
            PING_LOG_FILE=${TMODE}_${PRB}prb_${CN_CONFIG}_ping_from_gnb_nrue.log
            ping_enb_ip_addr $GNB_VM_CMDS $GNB_VM_IP_ADDR $UE_IP_ADDR $PING_LOG_FILE 0
            scp -o StrictHostKeyChecking=no ubuntu@$GNB_VM_IP_ADDR:/home/ubuntu/$PING_LOG_FILE $ARCHIVES_LOC
            check_ping_result $ARCHIVES_LOC/$PING_LOG_FILE 20

            echo "############################################################"
            echo "${CN_CONFIG} : iperf DL -- NR-UE is server and gNB is client"
            echo "############################################################"
            THROUGHPUT="30K"
            CURR_IPERF_LOG_BASE=${TMODE}_${PRB}prb_${CN_CONFIG}_iperf_dl
            get_enb_noS1_ip_addr $GNB_VM_CMDS $GNB_VM_IP_ADDR
            get_ue_ip_addr $NR_UE_VM_CMDS $NR_UE_VM_IP_ADDR 1
            generic_iperf $NR_UE_VM_CMDS $NR_UE_VM_IP_ADDR $UE_IP_ADDR $GNB_VM_CMDS $GNB_VM_IP_ADDR $ENB_IP_ADDR $THROUGHPUT $CURR_IPERF_LOG_BASE 1 0 
            scp -o StrictHostKeyChecking=no ubuntu@$GNB_VM_IP_ADDR:/home/ubuntu/${CURR_IPERF_LOG_BASE}_client.txt $ARCHIVES_LOC
            scp -o StrictHostKeyChecking=no ubuntu@$NR_UE_VM_IP_ADDR:/home/ubuntu/${CURR_IPERF_LOG_BASE}_server.txt $ARCHIVES_LOC
            check_iperf $ARCHIVES_LOC/$CURR_IPERF_LOG_BASE $THROUGHPUT
            if [ $IPERF_STATUS -ne 0 ]
            then
                echo "DL test not OK"
                terminate_enb_ue_basic_sim $NR_UE_VM_CMDS $NR_UE_VM_IP_ADDR 2
                terminate_enb_ue_basic_sim $GNB_VM_CMDS $GNB_VM_IP_ADDR 1
                scp -o StrictHostKeyChecking=no ubuntu@$GNB_VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/$CURRENT_GNB_LOG_FILE $ARCHIVES_LOC
                scp -o StrictHostKeyChecking=no ubuntu@$NR_UE_VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/$CURRENT_NR_UE_LOG_FILE $ARCHIVES_LOC
                try_cnt=$((try_cnt+1))
                continue
            fi

            echo "############################################################"
            echo "${CN_CONFIG} : iperf UL -- gNB is server and NR-UE is client"
            echo "############################################################"
            THROUGHPUT="30K"
            CURR_IPERF_LOG_BASE=${TMODE}_${PRB}prb_${CN_CONFIG}_iperf_ul
            get_enb_noS1_ip_addr $GNB_VM_CMDS $GNB_VM_IP_ADDR
            get_ue_ip_addr $NR_UE_VM_CMDS $NR_UE_VM_IP_ADDR 1
            generic_iperf $GNB_VM_CMDS $GNB_VM_IP_ADDR $ENB_IP_ADDR $NR_UE_VM_CMDS $NR_UE_VM_IP_ADDR $UE_IP_ADDR $THROUGHPUT $CURR_IPERF_LOG_BASE 1 0
            scp -o StrictHostKeyChecking=no ubuntu@$GNB_VM_IP_ADDR:/home/ubuntu/${CURR_IPERF_LOG_BASE}_server.txt $ARCHIVES_LOC
            scp -o StrictHostKeyChecking=no ubuntu@$NR_UE_VM_IP_ADDR:/home/ubuntu/${CURR_IPERF_LOG_BASE}_client.txt $ARCHIVES_LOC
            check_iperf $ARCHIVES_LOC/$CURR_IPERF_LOG_BASE $THROUGHPUT

            echo "############################################################"
            echo "${CN_CONFIG} : Terminate gNB/NR-UE simulators"
            echo "############################################################"
            terminate_enb_ue_basic_sim $NR_UE_VM_CMDS $NR_UE_VM_IP_ADDR 2
            terminate_enb_ue_basic_sim $GNB_VM_CMDS $GNB_VM_IP_ADDR 1
            scp -o StrictHostKeyChecking=no ubuntu@$GNB_VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/$CURRENT_GNB_LOG_FILE $ARCHIVES_LOC
            scp -o StrictHostKeyChecking=no ubuntu@$NR_UE_VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/$CURRENT_NR_UE_LOG_FILE $ARCHIVES_LOC
            if [ $IPERF_STATUS -ne 0 ]
            then
                echo "UL test not OK"
                try_cnt=$((try_cnt+1))
            else
                try_cnt=$((try_cnt+10))
            fi
          done
        done
        ######### end of loop
        full_l2_sim_destroy


        echo "############################################################"
        echo "Checking run status"
        echo "############################################################"

        if [ $SA_STATUS -ne 0 ]; then NR_STATUS=-1; fi     
        if [ $RA_FR2_STATUS -ne 0 ]; then NR_STATUS=-1; fi        
        if [ $RA_STATUS -ne 0 ]; then NR_STATUS=-1; fi
        if [ $SYNC_STATUS -ne 0 ]; then NR_STATUS=-1; fi
        if [ $PING_STATUS -ne 0 ]; then NR_STATUS=-1; fi
        if [ $IPERF_STATUS -ne 0 ]; then NR_STATUS=-1; fi
        if [ $NR_STATUS -eq 0 ]
        then
            echo "5G-NR RFSIM seems OK"
            echo "5G-NR: TEST_OK" >> $ARCHIVES_LOC/test_final_status.log
        else
            echo "5G-NR RFSIM seems to FAIL"
            echo "5G-NR: TEST_KO" >> $ARCHIVES_LOC/test_final_status.log
            STATUS=-1
        fi
    fi

    if [[ "$RUN_OPTIONS" == "complex" ]] && [[ $VM_NAME =~ .*-l2-sim.* ]]
    then
        PING_STATUS=0
        IPERF_STATUS=0
        if [ -d $ARCHIVES_LOC ]
        then
            rm -Rf $ARCHIVES_LOC
        fi
        mkdir --parents $ARCHIVES_LOC

        # Building UE elsewhere in VM
        #build_ue_on_separate_folder $VM_CMDS $VM_IP_ADDR

        # Creating a VM for EPC and installing SW
        EPC_VM_NAME=`echo $VM_NAME | sed -e "s#l2-sim#epc#"`
        EPC_VM_CMDS=${EPC_VM_NAME}_cmds.txt
        LTEBOX=0
        install_epc_on_vm $EPC_VM_NAME $EPC_VM_CMDS
        EPC_VM_IP_ADDR=`uvt-kvm ip $EPC_VM_NAME`

        # adding 16 users to EPC subscriber lists
        add_user_to_epc_lists $EPC_VM_CMDS $EPC_VM_IP_ADDR 16

        EPC_CONFIGS=("wS1" "noS1")
        TRANS_MODES=("fdd")
        BW_CASES=(05)
        NB_USERS=(01 04)
        for CN_CONFIG in ${EPC_CONFIGS[@]}
        do
          if [[ $CN_CONFIG =~ .*wS1.* ]]
          then
              echo "############################################################"
              echo "Start EPC for the wS1 configuration"
              echo "############################################################"
              start_epc $EPC_VM_NAME $EPC_VM_CMDS $EPC_VM_IP_ADDR

              # Retrieve EPC real IP address
              retrieve_real_epc_ip_addr $EPC_VM_NAME $EPC_VM_CMDS $EPC_VM_IP_ADDR
              S1_NOS1_CFG=1
          else
              echo "############################################################"
              echo "Terminate EPC"
              echo "############################################################"
              terminate_epc $EPC_VM_CMDS $EPC_VM_IP_ADDR

              echo "############################################################"
              echo "Running now in a no-S1 configuration"
              echo "############################################################"
              S1_NOS1_CFG=0
          fi
          for TMODE in ${TRANS_MODES[@]}
          do
            for BW in ${BW_CASES[@]}
            do
              for UES in ${NB_USERS[@]}
              do
                INT_NB_UES=`echo $UES | sed -e "s#^0*##"`
                echo "############################################################"
                echo "${CN_CONFIG} : Adding ${INT_NB_UES} UE(s) to conf file and recompile"
                echo "############################################################"
                add_ue_l2_sim_ue $UE_VM_CMDS $UE_VM_IP_ADDR $INT_NB_UES

                echo "############################################################"
                echo "${CN_CONFIG} : Starting the eNB in ${TMODE}-${BW}MHz mode"
                echo "############################################################"
                CURRENT_ENB_LOG_FILE=${TMODE}_${BW}MHz_${UES}users_${CN_CONFIG}_enb.log
                start_l2_sim_enb $ENB_VM_CMDS $ENB_VM_IP_ADDR $EPC_VM_IP_ADDR $UE_VM_IP_ADDR $CURRENT_ENB_LOG_FILE 25 rcc.band7.tm1.nfapi.conf $S1_NOS1_CFG

                echo "############################################################"
                echo "${CN_CONFIG} : Starting the UE for ${INT_NB_UES} user(s)"
                echo "############################################################"
                CURRENT_UE_LOG_FILE=${TMODE}_${BW}MHz_${UES}users_${CN_CONFIG}_ue.log
                start_l2_sim_ue $UE_VM_CMDS $UE_VM_IP_ADDR $ENB_VM_IP_ADDR $CURRENT_UE_LOG_FILE ue.nfapi.conf $INT_NB_UES $S1_NOS1_CFG
                if [ $UE_SYNC -eq 0 ]
                then
                    echo "Problem w/ eNB and UE not syncing"
                    terminate_enb_ue_basic_sim $ENB_VM_CMDS $ENB_VM_IP_ADDR 1
                    terminate_enb_ue_basic_sim $UE_VM_CMDS $UE_VM_IP_ADDR 2
                    scp -o StrictHostKeyChecking=no ubuntu@$ENB_VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/$CURRENT_ENB_LOG_FILE $ARCHIVES_LOC
                    scp -o StrictHostKeyChecking=no ubuntu@$UE_VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/$CURRENT_UE_LOG_FILE $ARCHIVES_LOC
                    STATUS=-1
                    continue
                fi

                if [ $S1_NOS1_CFG -eq 1 ]
                then

                    echo "############################################################"
                    echo "${CN_CONFIG} : Pinging the EPC from UE(s)"
                    echo "############################################################"
                    echo " --- Sequentially ---"
                    local j="1"
                    while [ $j -le $INT_NB_UES ]
                    do
                        PING_LOG_FILE=${TMODE}_${BW}MHz_${UES}users_${CN_CONFIG}_ping_epc_seq_from_ue${j}.log
                        ping_epc_ip_addr $UE_VM_CMDS $UE_VM_IP_ADDR $REAL_EPC_IP_ADDR $PING_LOG_FILE ${j} 0
                        scp -o StrictHostKeyChecking=no ubuntu@$UE_VM_IP_ADDR:/home/ubuntu/$PING_LOG_FILE $ARCHIVES_LOC
                        check_ping_result $ARCHIVES_LOC/$PING_LOG_FILE 20
                        j=$[$j+1]
                    done
                    if [ $INT_NB_UES -gt 1 ]
                    then
                        echo " --- In parallel ---"
                        j="1"
                        while [ $j -le $INT_NB_UES ]
                        do
                            PING_LOG_FILE=${TMODE}_${BW}MHz_${UES}users_${CN_CONFIG}_ping_epc_para_from_ue${j}.log
                            ping_epc_ip_addr $UE_VM_CMDS $UE_VM_IP_ADDR $REAL_EPC_IP_ADDR $PING_LOG_FILE ${j} 1
                            j=$[$j+1]
                        done
                        sleep 25
                        j="1"
                        while [ $j -le $INT_NB_UES ]
                        do
                            PING_LOG_FILE=${TMODE}_${BW}MHz_${UES}users_${CN_CONFIG}_ping_epc_para_from_ue${j}.log
                            scp -o StrictHostKeyChecking=no ubuntu@$UE_VM_IP_ADDR:/home/ubuntu/$PING_LOG_FILE $ARCHIVES_LOC
                            tail -3 $ARCHIVES_LOC/$PING_LOG_FILE
                            check_ping_result $ARCHIVES_LOC/$PING_LOG_FILE 20
                            j=$[$j+1]
                        done
                    fi
                else
                    get_enb_noS1_ip_addr $ENB_VM_CMDS $ENB_VM_IP_ADDR

                    echo "############################################################"
                    echo "${CN_CONFIG} : Pinging the eNB from UE(s)"
                    echo "############################################################"
                    echo " --- Sequentially ---"
                    local j="1"
                    while [ $j -le $INT_NB_UES ]
                    do
                        PING_LOG_FILE=${TMODE}_${BW}MHz_${UES}users_${CN_CONFIG}_ping_enb_seq_from_ue${j}.log
                        ping_epc_ip_addr $UE_VM_CMDS $UE_VM_IP_ADDR $ENB_IP_ADDR $PING_LOG_FILE ${j} 0
                        scp -o StrictHostKeyChecking=no ubuntu@$UE_VM_IP_ADDR:/home/ubuntu/$PING_LOG_FILE $ARCHIVES_LOC
                        check_ping_result $ARCHIVES_LOC/$PING_LOG_FILE 20
                        j=$[$j+1]
                    done
                    if [ $INT_NB_UES -gt 1 ]
                    then
                        echo " --- In parallel ---"
                        j="1"
                        while [ $j -le $INT_NB_UES ]
                        do
                            PING_LOG_FILE=${TMODE}_${BW}MHz_${UES}users_${CN_CONFIG}_ping_enb_para_from_ue${j}.log
                            ping_epc_ip_addr $UE_VM_CMDS $UE_VM_IP_ADDR $ENB_IP_ADDR $PING_LOG_FILE ${j} 1
                            j=$[$j+1]
                        done
                        sleep 25
                        j="1"
                        while [ $j -le $INT_NB_UES ]
                        do
                            PING_LOG_FILE=${TMODE}_${BW}MHz_${UES}users_${CN_CONFIG}_ping_enb_para_from_ue${j}.log
                            scp -o StrictHostKeyChecking=no ubuntu@$UE_VM_IP_ADDR:/home/ubuntu/$PING_LOG_FILE $ARCHIVES_LOC
                            tail -3 $ARCHIVES_LOC/$PING_LOG_FILE
                            check_ping_result $ARCHIVES_LOC/$PING_LOG_FILE 20
                            j=$[$j+1]
                        done
                    fi
                fi

                if [ $S1_NOS1_CFG -eq 1 ]
                then
                    echo "############################################################"
                    echo "${CN_CONFIG} : Pinging the UE(s) from EPC"
                    echo "############################################################"
                    echo " --- Sequentially ---"
                    local j="1"
                    while [ $j -le $INT_NB_UES ]
                    do
                        get_ue_ip_addr $UE_VM_CMDS $UE_VM_IP_ADDR $j
                        PING_LOG_FILE=${TMODE}_${BW}MHz_${UES}users_${CN_CONFIG}_ping_from_epc_seq_ue${j}.log
                        ping_ue_ip_addr $EPC_VM_CMDS $EPC_VM_IP_ADDR $UE_IP_ADDR $PING_LOG_FILE 0
                        scp -o StrictHostKeyChecking=no ubuntu@$EPC_VM_IP_ADDR:/home/ubuntu/$PING_LOG_FILE $ARCHIVES_LOC
                        check_ping_result $ARCHIVES_LOC/$PING_LOG_FILE 20
                        j=$[$j+1]
                    done
                    if [ $INT_NB_UES -gt 1 ]
                    then
                        echo " --- In parallel ---"
                        j="1"
                        while [ $j -le $INT_NB_UES ]
                        do
                            get_ue_ip_addr $UE_VM_CMDS $UE_VM_IP_ADDR $j
                            PING_LOG_FILE=${TMODE}_${BW}MHz_${UES}users_${CN_CONFIG}_ping_from_epc_para_ue${j}.log
                            ping_ue_ip_addr $EPC_VM_CMDS $EPC_VM_IP_ADDR $UE_IP_ADDR $PING_LOG_FILE 1
                            j=$[$j+1]
                        done
                        sleep 25
                        j="1"
                        while [ $j -le $INT_NB_UES ]
                        do
                            PING_LOG_FILE=${TMODE}_${BW}MHz_${UES}users_${CN_CONFIG}_ping_from_epc_para_ue${j}.log
                            scp -o StrictHostKeyChecking=no ubuntu@$EPC_VM_IP_ADDR:/home/ubuntu/$PING_LOG_FILE $ARCHIVES_LOC
                            check_ping_result $ARCHIVES_LOC/$PING_LOG_FILE 20
                            j=$[$j+1]
                        done
                    fi
                else
                    echo "############################################################"
                    echo "${CN_CONFIG} : Pinging the UE(s) from eNB"
                    echo "############################################################"
                    echo " --- Sequentially ---"
                    local j="1"
                    while [ $j -le $INT_NB_UES ]
                    do
                        get_ue_ip_addr $UE_VM_CMDS $UE_VM_IP_ADDR $j
                        PING_LOG_FILE=${TMODE}_${BW}MHz_${UES}users_${CN_CONFIG}_ping_from_enb_seq_ue${j}.log
                        ping_enb_ip_addr $ENB_VM_CMDS $ENB_VM_IP_ADDR $UE_IP_ADDR $PING_LOG_FILE 0
                        scp -o StrictHostKeyChecking=no ubuntu@$ENB_VM_IP_ADDR:/home/ubuntu/$PING_LOG_FILE $ARCHIVES_LOC
                        check_ping_result $ARCHIVES_LOC/$PING_LOG_FILE 20
                        j=$[$j+1]
                    done
                    if [ $INT_NB_UES -gt 1 ]
                    then
                        echo " --- In parallel ---"
                        j="1"
                        while [ $j -le $INT_NB_UES ]
                        do
                            get_ue_ip_addr $UE_VM_CMDS $UE_VM_IP_ADDR $j
                            PING_LOG_FILE=${TMODE}_${BW}MHz_${UES}users_${CN_CONFIG}_ping_from_enb_para_ue${j}.log
                            ping_enb_ip_addr $ENB_VM_CMDS $ENB_VM_IP_ADDR $UE_IP_ADDR $PING_LOG_FILE 1
                            j=$[$j+1]
                        done
                        sleep 25
                        j="1"
                        while [ $j -le $INT_NB_UES ]
                        do
                            PING_LOG_FILE=${TMODE}_${BW}MHz_${UES}users_${CN_CONFIG}_ping_from_enb_para_ue${j}.log
                            scp -o StrictHostKeyChecking=no ubuntu@$ENB_VM_IP_ADDR:/home/ubuntu/$PING_LOG_FILE $ARCHIVES_LOC
                            tail -3 $ARCHIVES_LOC/$PING_LOG_FILE
                            check_ping_result $ARCHIVES_LOC/$PING_LOG_FILE 20
                            j=$[$j+1]
                        done
                    fi
                fi

                if [ $S1_NOS1_CFG -eq 0 ]
                then
                    get_enb_noS1_ip_addr $ENB_VM_CMDS $ENB_VM_IP_ADDR
                    echo "############################################################"
                    echo "${CN_CONFIG} : iperf DL -- UE is server and eNB is client"
                    echo "############################################################"
                    echo " --- Sequentially ---"
                    local j="1"
                    while [ $j -le $INT_NB_UES ]
                    do
                        IPERF_LOG_FILE=${TMODE}_${BW}MHz_${UES}users_${CN_CONFIG}_iperf_dl_seq_ue${j}
                        get_ue_ip_addr $UE_VM_CMDS $UE_VM_IP_ADDR $j
                        THROUGHPUT=3
                        generic_iperf $UE_VM_CMDS $UE_VM_IP_ADDR $UE_IP_ADDR $ENB_VM_CMDS $ENB_VM_IP_ADDR $ENB_IP_ADDR $THROUGHPUT $IPERF_LOG_FILE $j 0
                        scp -o StrictHostKeyChecking=no ubuntu@$UE_VM_IP_ADDR:/home/ubuntu/${IPERF_LOG_FILE}_server.txt $ARCHIVES_LOC
                        scp -o StrictHostKeyChecking=no ubuntu@$ENB_VM_IP_ADDR:/home/ubuntu/${IPERF_LOG_FILE}_client.txt $ARCHIVES_LOC
                        check_iperf $ARCHIVES_LOC/$IPERF_LOG_FILE $THROUGHPUT
                        j=$[$j+1]
                    done
                    if [ $INT_NB_UES -gt 1 ]
                    then
                        echo " --- In parallel ---"
                        j="1"
                        while [ $j -le $INT_NB_UES ]
                        do
                            IPERF_LOG_FILE=${TMODE}_${BW}MHz_${UES}users_${CN_CONFIG}_iperf_dl_para_ue${j}
                            THROUGHPUT=1
                            get_ue_ip_addr $UE_VM_CMDS $UE_VM_IP_ADDR $j
                            generic_iperf $UE_VM_CMDS $UE_VM_IP_ADDR $UE_IP_ADDR $ENB_VM_CMDS $ENB_VM_IP_ADDR $ENB_IP_ADDR $THROUGHPUT $IPERF_LOG_FILE $j 1
                            j=$[$j+1]
                        done
                        sleep 35
                        echo "killall --signal SIGKILL iperf"
                        echo "killall --signal SIGKILL iperf" > $UE_VM_CMDS
                        ssh -T -o StrictHostKeyChecking=no ubuntu@$UE_VM_IP_ADDR < $UE_VM_CMDS
                        rm $UE_VM_CMDS
                        j="1"
                        while [ $j -le $INT_NB_UES ]
                        do
                            IPERF_LOG_FILE=${TMODE}_${BW}MHz_${UES}users_${CN_CONFIG}_iperf_dl_para_ue${j}
                            scp -o StrictHostKeyChecking=no ubuntu@$UE_VM_IP_ADDR:/home/ubuntu/${IPERF_LOG_FILE}_server.txt $ARCHIVES_LOC
                            scp -o StrictHostKeyChecking=no ubuntu@$ENB_VM_IP_ADDR:/home/ubuntu/${IPERF_LOG_FILE}_client.txt $ARCHIVES_LOC
                            tail -3 $ARCHIVES_LOC/${IPERF_LOG_FILE}_client.txt | grep -v datagram
                            #check_iperf $ARCHIVES_LOC/$IPERF_LOG_FILE $THROUGHPUT
                            j=$[$j+1]
                        done
                    fi

                    echo "############################################################"
                    echo "${CN_CONFIG} : iperf UL -- eNB is server and UE is client"
                    echo "############################################################"
                    echo " --- Sequentially ---"
                    local j="1"
                    while [ $j -le $INT_NB_UES ]
                    do
                        IPERF_LOG_FILE=${TMODE}_${BW}MHz_${UES}users_${CN_CONFIG}_iperf_ul_seq_ue${j}
                        THROUGHPUT=2
                        get_ue_ip_addr $UE_VM_CMDS $UE_VM_IP_ADDR $j
                        generic_iperf $ENB_VM_CMDS $ENB_VM_IP_ADDR $ENB_IP_ADDR $UE_VM_CMDS $UE_VM_IP_ADDR $UE_IP_ADDR $THROUGHPUT $IPERF_LOG_FILE $j 0
                        scp -o StrictHostKeyChecking=no ubuntu@$ENB_VM_IP_ADDR:/home/ubuntu/${IPERF_LOG_FILE}_server.txt $ARCHIVES_LOC
                        scp -o StrictHostKeyChecking=no ubuntu@$UE_VM_IP_ADDR:/home/ubuntu/${IPERF_LOG_FILE}_client.txt $ARCHIVES_LOC
                        check_iperf $ARCHIVES_LOC/$IPERF_LOG_FILE $THROUGHPUT
                        j=$[$j+1]
                    done
                    if [ $INT_NB_UES -gt 1 ]
                    then
                        echo " --- In parallel ---"
                        j="1"
                        while [ $j -le $INT_NB_UES ]
                        do
                            IPERF_LOG_FILE=${TMODE}_${BW}MHz_${UES}users_${CN_CONFIG}_iperf_ul_para_ue${j}
                            THROUGHPUT=1
                            get_ue_ip_addr $UE_VM_CMDS $UE_VM_IP_ADDR $j
                            generic_iperf $ENB_VM_CMDS $ENB_VM_IP_ADDR $ENB_IP_ADDR $UE_VM_CMDS $UE_VM_IP_ADDR $UE_IP_ADDR $THROUGHPUT $IPERF_LOG_FILE $j 1
                            j=$[$j+1]
                        done
                        sleep 35
                        echo "killall --signal SIGKILL iperf"
                        echo "killall --signal SIGKILL iperf" > $UE_VM_CMDS
                        ssh -T -o StrictHostKeyChecking=no ubuntu@$UE_VM_IP_ADDR < $UE_VM_CMDS
                        rm $UE_VM_CMDS
                        j="1"
                        while [ $j -le $INT_NB_UES ]
                        do
                            IPERF_LOG_FILE=${TMODE}_${BW}MHz_${UES}users_${CN_CONFIG}_iperf_ul_para_ue${j}
                            scp -o StrictHostKeyChecking=no ubuntu@$ENB_VM_IP_ADDR:/home/ubuntu/${IPERF_LOG_FILE}_server.txt $ARCHIVES_LOC
                            scp -o StrictHostKeyChecking=no ubuntu@$UE_VM_IP_ADDR:/home/ubuntu/${IPERF_LOG_FILE}_client.txt $ARCHIVES_LOC
                            tail -3 $ARCHIVES_LOC/${IPERF_LOG_FILE}_client.txt | grep -v datagram
                            #check_iperf $ARCHIVES_LOC/$IPERF_LOG_FILE $THROUGHPUT
                            j=$[$j+1]
                        done
                    fi
                fi

                echo "############################################################"
                echo "${CN_CONFIG} : Terminate enb/ue simulators"
                echo "############################################################"
                terminate_enb_ue_basic_sim $ENB_VM_CMDS $ENB_VM_IP_ADDR 1
                terminate_enb_ue_basic_sim $UE_VM_CMDS $UE_VM_IP_ADDR 2
                scp -o StrictHostKeyChecking=no ubuntu@$ENB_VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/$CURRENT_ENB_LOG_FILE $ARCHIVES_LOC
                scp -o StrictHostKeyChecking=no ubuntu@$UE_VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/$CURRENT_UE_LOG_FILE $ARCHIVES_LOC

              done
            done
          done
        done

        full_l2_sim_destroy

        echo "############################################################"
        echo "Checking run status"
        echo "############################################################"

        if [ $PING_STATUS -ne 0 ]; then STATUS=-1; fi
        if [ $IPERF_STATUS -ne 0 ]; then STATUS=-1; fi
        if [ $STATUS -eq 0 ]
        then
            echo "TEST_OK" > $ARCHIVES_LOC/test_final_status.log
        else
            echo "TEST_KO" > $ARCHIVES_LOC/test_final_status.log
        fi
    fi
}
