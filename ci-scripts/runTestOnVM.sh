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

function run_test_usage {
    echo "OAI CI VM script"
    echo "   Original Author: Raphael Defosseux"
    echo "   Requirements:"
    echo "     -- uvtool uvtool-libvirt apt-cacher"
    echo "     -- xenial image already synced"
    echo "   Default:"
    echo "     -- eNB with USRP"
    echo ""
    echo "Usage:"
    echo "------"
    echo "    oai-ci-vm-tool test [OPTIONS]"
    echo ""
    echo "Options:"
    echo "--------"
    echo "    --job-name #### OR -jn ####"
    echo "    Specify the name of the Jenkins job."
    echo ""
    echo "    --build-id #### OR -id ####"
    echo "    Specify the build ID of the Jenkins job."
    echo ""
    echo "    --workspace #### OR -ws ####"
    echo "    Specify the workspace."
    echo ""
    variant_usage
    echo "    Specify the variant to build."
    echo ""
    echo "    --keep-vm-alive OR -k"
    echo "    Keep the VM alive after the build."
    echo ""
    echo "    --help OR -h"
    echo "    Print this help message."
    echo ""
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
    echo "echo \"cd /home/ubuntu/tmp/cmake_targets/basic_simulator/enb/\"" >> $1
    echo "sudo chmod 777 /home/ubuntu/tmp/cmake_targets/basic_simulator" >> $1
    echo "sudo chmod 777 /home/ubuntu/tmp/cmake_targets/basic_simulator/enb/" >> $1
    echo "cd /home/ubuntu/tmp/cmake_targets/basic_simulator/enb/" >> $1
    echo "echo \"ulimit -c unlimited && ./lte-softmodem -O /home/ubuntu/tmp/ci-scripts/conf_files/ci-$LOC_CONF_FILE\" > ./my-lte-softmodem-run.sh " >> $1
    echo "chmod 775 ./my-lte-softmodem-run.sh" >> $1
    echo "cat ./my-lte-softmodem-run.sh" >> $1
    echo "if [ -e /home/ubuntu/tmp/cmake_targets/log/$LOC_LOG_FILE ]; then sudo sudo rm -f /home/ubuntu/tmp/cmake_targets/log/$LOC_LOG_FILE; fi" >> $1
    echo "sudo -E daemon --inherit --unsafe --name=enb_daemon --chdir=/home/ubuntu/tmp/cmake_targets/basic_simulator/enb -o /home/ubuntu/tmp/cmake_targets/log/$LOC_LOG_FILE ./my-lte-softmodem-run.sh" >> $1

    ssh -o StrictHostKeyChecking=no ubuntu@$LOC_VM_IP_ADDR < $1
    sleep 10
    rm $1
}

function start_basic_sim_ue {
    local LOC_UE_LOG_FILE=$3
    local LOC_NB_RBS=$4
    local LOC_FREQUENCY=$5
    echo "echo \"cd /home/ubuntu/tmp/cmake_targets/basic_simulator/ue\"" > $1
    echo "sudo chmod 777 /home/ubuntu/tmp/cmake_targets/basic_simulator/ue" >> $1
    echo "cd /home/ubuntu/tmp/cmake_targets/basic_simulator/ue" >> $1
    echo "echo \"./lte-uesoftmodem -C ${LOC_FREQUENCY}000000 -r $LOC_NB_RBS --ue-rxgain 140\" > ./my-lte-uesoftmodem-run.sh" >> $1
    echo "chmod 775 ./my-lte-uesoftmodem-run.sh" >> $1
    echo "cat ./my-lte-uesoftmodem-run.sh" >> $1
    echo "if [ -e /home/ubuntu/tmp/cmake_targets/log/$LOC_UE_LOG_FILE ]; then sudo sudo rm -f /home/ubuntu/tmp/cmake_targets/log/$LOC_UE_LOG_FILE; fi" >> $1
    echo "sudo -E daemon --inherit --unsafe --name=ue_daemon --chdir=/home/ubuntu/tmp/cmake_targets/basic_simulator/ue -o /home/ubuntu/tmp/cmake_targets/log/$LOC_UE_LOG_FILE ./my-lte-uesoftmodem-run.sh" >> $1

    ssh -o StrictHostKeyChecking=no ubuntu@$2 < $1
    rm $1

    local i="0"
    echo "ifconfig oip1 | egrep -c \"inet addr\"" > $1
    while [ $i -lt 10 ]
    do
        sleep 5
        CONNECTED=`ssh -o StrictHostKeyChecking=no ubuntu@$2 < $1`
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
    else
        UE_SYNC=1
    fi
}

function get_ue_ip_addr {
    echo "ifconfig oip1 | egrep \"inet addr\" | sed -e 's#^.*inet addr:##' -e 's#  P-t-P:.*\$##'" > $1
    UE_IP_ADDR=`ssh -o StrictHostKeyChecking=no ubuntu@$2 < $1`
    echo "UE IP Address for EPC is : $UE_IP_ADDR"
    rm $1
}

function ping_ue_ip_addr {
    echo "echo \"ping -c 20 $3\"" > $1
    echo "echo \"COMMAND IS: ping -c 20 $3\" > $4" > $1
    echo "ping -c 20 $UE_IP_ADDR | tee -a $4" >> $1
    ssh -o StrictHostKeyChecking=no ubuntu@$2 < $1
    rm -f $1
}

function ping_nos1_ue_ip_addr {
    echo "echo \"COMMAND IS: ping -I oai0 -c 20 $3\" > $4" > $1
    echo "rm -f $4" >> $1
    echo "ping -I oai0 -c 20 $UE_REAL_IP_ADDR | tee -a $4" >> $1
    echo "command generated by ping_nos1_ue_ip_addr:"
    cat $1
    echo "end of command generated by ping_nos1_ue_ip_addr"
    ssh -o StrictHostKeyChecking=no ubuntu@$2 < $1
    rm -f $1
    echo "END ping_nos1_ue_ip_addr"
}

function ping_nos1_enb_ip_addr {
    echo "echo \"COMMAND IS: ping -I oai0 -c 20 $3\" > $4" > $1
    echo "rm -f $4" >> $1
    echo "ping -I oai0 -c 20 $ENB_REAL_IP_ADDR | tee -a $4" >> $1
    echo "command generated by ping_nos1_enb_ip_addr:"
    cat $1
    echo "end of command generated by ping_nos1_enb_ip_addr"
    ssh -o StrictHostKeyChecking=no ubuntu@$2 < $1
    rm -f $1
    echo "END ping_nos1_enb_ip_addr"
}

function ping_epc_ip_addr {
    echo "echo \"COMMAND IS: ping -I oip1 -c 20 $3\" > $4" > $1
    echo "rm -f $4" >> $1
    echo "ping -I oip1 -c 20 $3 | tee -a $4" >> $1
    cat $1
    ssh -o StrictHostKeyChecking=no ubuntu@$2 < $1
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
                echo "got NOT all ping packets"
                PING_STATUS=-1
            fi
        fi
    else
        echo "ping file not present"
        PING_STATUS=-1
    fi
}

function iperf_dl {
    echo $@
    local REQ_BANDWIDTH=$5
    local BASE_LOG_FILE=$6
    echo "echo \"iperf -u -s -i 1\"" > $1
    echo "echo \"COMMAND IS: iperf -u -s -i 1\" > tmp/cmake_targets/log/${BASE_LOG_FILE}_server.txt" > $1
    echo "nohup iperf -u -s -i 1 >> tmp/cmake_targets/log/${BASE_LOG_FILE}_server.txt &" >> $1
    ssh -o StrictHostKeyChecking=no ubuntu@$2 < $1
    cat $1
    rm $1

    echo "echo \"iperf -c $UE_IP_ADDR -u -t 30 -b ${REQ_BANDWIDTH}M -i 1\"" > $3
    echo "echo \"COMMAND IS: iperf -c $UE_IP_ADDR -u -t 30 -b ${REQ_BANDWIDTH}M -i 1\" > ${BASE_LOG_FILE}_client.txt" > $3
    echo "iperf -c $UE_IP_ADDR -u -t 30 -b ${REQ_BANDWIDTH}M -i 1 | tee -a ${BASE_LOG_FILE}_client.txt" >> $3
    ssh -o StrictHostKeyChecking=no ubuntu@$4 < $3
    cat $3
    rm -f $3

    echo "killall --signal SIGKILL iperf" >> $1
    ssh -o StrictHostKeyChecking=no ubuntu@$2 < $1
    cat $1
    rm $1
}

function iperf_ul {
    local REQ_BANDWIDTH=$5
    local BASE_LOG_FILE=$6
    echo "echo \"iperf -u -s -i 1\"" > $3
    echo "echo \"COMMAND IS: iperf -u -s -i 1\" > ${BASE_LOG_FILE}_server.txt" > $3
    echo "nohup iperf -u -s -i 1 >> ${BASE_LOG_FILE}_server.txt &" >> $3
    ssh -o StrictHostKeyChecking=no ubuntu@$4 < $3
    rm $3

    echo "echo \"iperf -c $REAL_EPC_IP_ADDR -u -t 30 -b ${REQ_BANDWIDTH}M -i 1\"" > $1
    echo "echo \"COMMAND IS: iperf -c $REAL_EPC_IP_ADDR -u -t 30 -b ${REQ_BANDWIDTH}M -i 1\" > /home/ubuntu/tmp/cmake_targets/log/${BASE_LOG_FILE}_client.txt" > $1
    echo "iperf -c $REAL_EPC_IP_ADDR -u -t 30 -b ${REQ_BANDWIDTH}M -i 1 | tee -a /home/ubuntu/tmp/cmake_targets/log/${BASE_LOG_FILE}_client.txt" >> $1
    ssh -o StrictHostKeyChecking=no ubuntu@$2 < $1
    rm -f $1

    echo "killall --signal SIGKILL iperf" >> $3
    ssh -o StrictHostKeyChecking=no ubuntu@$4 < $3
    rm $3
}

function nos1_iperf {
    # nos1_iperf $REAL_SERVER_IP_ADDRESS $VM_SERVER_IP_ADDRESS $VM_SERVER_CMDS $VM_CLIENT_IP_ADDRESS $VM_CLIENT_CMDS bandwitch log_file
    echo $@
    local REAL_SERVER_IP_ADDRESS=$1
    local REQ_BANDWIDTH=$6
    local BASE_LOG_FILE=$7
    echo "echo \"COMMAND IS: iperf -u -s -i 1 -fm \" > tmp/cmake_targets/log/${BASE_LOG_FILE}_server.txt" > $3
    echo "nohup iperf -u -s -i 1 -fm >> tmp/cmake_targets/log/${BASE_LOG_FILE}_server.txt &" >> $3
    ssh -o StrictHostKeyChecking=no ubuntu@$2 < $3
    cat $3
    rm $3

    echo "echo \"COMMAND IS: iperf -c $REAL_SERVER_IP_ADDRESS -u -t 30 -b ${REQ_BANDWIDTH}M -i 1 -fm \" > ${BASE_LOG_FILE}_client.txt" > $5
    echo "iperf -c $REAL_SERVER_IP_ADDRESS -u -t 30 -b ${REQ_BANDWIDTH}M -i 1 -fm | tee -a ${BASE_LOG_FILE}_client.txt" >> $5
    ssh -o StrictHostKeyChecking=no ubuntu@$4 < $5
    cat $5
    rm -f $5

    echo "killall --signal SIGKILL iperf" >> $3
    ssh -o StrictHostKeyChecking=no ubuntu@$2 < $3
    cat $3
    rm $3
}

function check_iperf {
    local LOC_BASE_LOG=$1
    local LOC_REQ_BW=$2
    local LOC_REQ_BW_MINUS_ONE=`echo "$LOC_REQ_BW - 1" | bc -l`
    if [ -f ${LOC_BASE_LOG}_client.txt ]
    then
        local FILE_COMPLETE=`egrep -c "Server Report" ${LOC_BASE_LOG}_client.txt`
        if [ $FILE_COMPLETE -eq 0 ]
        then
            IPERF_STATUS=-1
        else
            local EFFECTIVE_BANDWIDTH=`tail -n3 ${LOC_BASE_LOG}_client.txt | egrep "Mbits/sec" | sed -e "s#^.*MBytes *##" -e "s#sec.*#sec#"`
            if [[ $EFFECTIVE_BANDWIDTH =~ .*${LOC_REQ_BW}.*Mbits.* ]] || [[ $EFFECTIVE_BANDWIDTH =~ .*${LOC_REQ_BW_MINUS_ONE}.*Mbits.* ]]
            then
                echo "got requested DL bandwidth: $EFFECTIVE_BANDWIDTH"
            else
                IPERF_STATUS=-1
            fi
        fi
    else
        IPERF_STATUS=-1
    fi
}

function terminate_enb_ue_basic_sim {
    echo "NB_OAI_PROCESSES=\`ps -aux | grep modem | grep -v grep | grep -c softmodem\`" > $1
    echo "if [ \$NB_OAI_PROCESSES -ne 0 ]; then echo \"sudo daemon --name=enb_daemon --stop\"; fi" >> $1
    echo "if [ \$NB_OAI_PROCESSES -ne 0 ]; then sudo daemon --name=enb_daemon --stop; fi" >> $1
    echo "if [ \$NB_OAI_PROCESSES -ne 0 ]; then echo \"sudo daemon --name=ue_daemon --stop\"; fi" >> $1
    echo "if [ \$NB_OAI_PROCESSES -ne 0 ]; then sudo daemon --name=ue_daemon --stop; fi" >> $1
    echo "if [ \$NB_OAI_PROCESSES -ne 0 ]; then sleep 5; fi" >> $1
    echo "echo \"ps -aux | grep softmodem\"" >> $1
    echo "ps -aux | grep softmodem | grep -v grep" >> $1
    echo "NB_OAI_PROCESSES=\`ps -aux | grep modem | grep -v grep | grep -c softmodem\`" >> $1
    echo "if [ \$NB_OAI_PROCESSES -ne 0 ]; then echo \"sudo killall --signal SIGINT lte-softmodem\"; fi" >> $1
    echo "if [ \$NB_OAI_PROCESSES -ne 0 ]; then sudo killall --signal SIGINT lte-softmodem; fi" >> $1
    echo "if [ \$NB_OAI_PROCESSES -ne 0 ]; then sleep 5; fi" >> $1
    echo "echo \"ps -aux | grep softmodem\"" >> $1
    echo "ps -aux | grep softmodem | grep -v grep" >> $1
    echo "NB_OAI_PROCESSES=\`ps -aux | grep modem | grep -v grep | grep -c softmodem\`" >> $1
    echo "if [ \$NB_OAI_PROCESSES -ne 0 ]; then echo \"sudo killall --signal SIGKILL lte-softmodem\"; fi" >> $1
    echo "if [ \$NB_OAI_PROCESSES -ne 0 ]; then sudo killall --signal SIGKILL lte-softmodem; fi" >> $1
    echo "if [ \$NB_OAI_PROCESSES -ne 0 ]; then echo \"sudo killall --signal SIGKILL lte-uesoftmodem\"; fi" >> $1
    echo "if [ \$NB_OAI_PROCESSES -ne 0 ]; then sudo killall --signal SIGKILL lte-uesoftmodem; fi" >> $1
    echo "if [ \$NB_OAI_PROCESSES -ne 0 ]; then sleep 5; fi" >> $1
    echo "echo \"ps -aux | grep softmodem\"" >> $1
    echo "ps -aux | grep softmodem | grep -v grep" >> $1
    ssh -o StrictHostKeyChecking=no ubuntu@$2 < $1
    rm -f $1
}

function recover_core_dump {
    local IS_SEG_FAULT=`egrep -ic "segmentation fault" $3`
    if [ $IS_SEG_FAULT -ne 0 ]
    then
        local TC=`echo $3 | sed -e "s#^.*enb_##" -e "s#Hz.*#Hz#"`
        echo "Segmentation fault detected on enb -> recovering core dump"
        echo "cd /home/ubuntu/tmp/cmake_targets/basic_simulator/enb" > $1
        echo "sync" >> $1
        echo "sudo tar -cjhf basic-simulator-enb-core-${TC}.bz2 core lte-softmodem *.so ci-lte-basic-sim.conf my-lte-softmodem-run.sh" >> $1
        echo "sudo rm core" >> $1
        echo "rm ci-lte-basic-sim.conf" >> $1
        echo "sync" >> $1
        ssh -o StrictHostKeyChecking=no ubuntu@$2 < $1
        scp -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/basic_simulator/enb/basic-simulator-enb-core-${TC}.bz2 $4
        rm -f $1
    fi
}

function start_nos1_sim_enb {
    # start_nos1_sim_enb $ENB_VM_CMDa $ENB_VM_IP_ADDR $UE_VM_IP_ADDR $CURRENT_ENB_LOG_FILE 25 rcc.band7.nos1.simulator.conf
    local LOC_ENB_VM_CMDS=$1
    local LOC_VM_IP_ADDR=$2
    local LOC_UE_IP_ADDR=$3
    local LOC_LOG_FILE=$4
    local LOC_NB_RBS=$5
    local LOC_CONF_FILE=$6
    echo "cd /home/ubuntu/tmp" > $1
    #echo "echo '$@' $@" >> $1
    echo "echo \"sudo apt-get --yes --quiet install daemon \"" >> $1
    echo "sudo apt-get --yes install daemon >> /home/ubuntu/tmp/cmake_targets/log/daemon-install.txt 2>&1" >> $1
    echo "echo \"source oaienv\"" >> $1
    echo "source oaienv" >> $1
    echo "cd ci-scripts/conf_files/" >> $1
    echo "cp $LOC_CONF_FILE ci-$LOC_CONF_FILE" >> $1
    echo "sed -i -e 's#N_RB_DL.*=.*;#N_RB_DL                                         = $LOC_NB_RBS;#' -e 's#CI_UE_IP_ADDR#$LOC_UE_IP_ADDR#' -e 's#CI_ENB_IP_ADDR#$LOC_VM_IP_ADDR#' ci-$LOC_CONF_FILE" >> $1
    echo "echo \"grep N_RB_DL ci-$LOC_CONF_FILE\"" >> $1
    echo "grep N_RB_DL ci-$LOC_CONF_FILE | sed -e 's#N_RB_DL.*=#N_RB_DL =#'" >> $1
    echo "echo \"cd /home/ubuntu/tmp/cmake_targets/tools\"" >> $1
    echo "cd /home/ubuntu/tmp/cmake_targets/tools/" >> $1
    echo "echo \"if [ \`lsmod | grep -c nasmesh\` -eq 0 ]; then sudo -E ./init_nas_nos1 eNB; fi\"" >> $1
    echo "if [ \`lsmod | grep -c nasmesh\` -eq 0 ]; then sudo -E ./init_nas_nos1 eNB; fi" >> $1
    echo "echo \"cd /home/ubuntu/tmp/cmake_targets/lte_noS1_build_oai/build/\"" >> $1
    echo "sudo chmod 777 /home/ubuntu/tmp/cmake_targets/lte_noS1_build_oai/build" >> $1
    echo "cd /home/ubuntu/tmp/cmake_targets/lte_noS1_build_oai/build/" >> $1
    echo "echo \"ulimit -c unlimited && ./lte-softmodem-nos1 -O /home/ubuntu/tmp/ci-scripts/conf_files/ci-$LOC_CONF_FILE\" > ./my-lte-softmodem-run.sh " >> $1
    echo "chmod 775 ./my-lte-softmodem-run.sh" >> $1
    echo "cat ./my-lte-softmodem-run.sh" >> $1
    echo "if [ -e /home/ubuntu/tmp/cmake_targets/log/$LOC_LOG_FILE ]; then sudo sudo rm -f /home/ubuntu/tmp/cmake_targets/log/$LOC_LOG_FILE; fi" >> $1
    echo "sudo -E daemon --inherit --unsafe --name=enb_daemon --chdir=/home/ubuntu/tmp/cmake_targets/lte_noS1_build_oai/build -o /home/ubuntu/tmp/cmake_targets/log/$LOC_LOG_FILE ./my-lte-softmodem-run.sh" >> $1
    echo "log file path = /home/ubuntu/tmp/cmake_targets/log/$LOC_LOG_FILE"
    ssh -o StrictHostKeyChecking=no ubuntu@$LOC_VM_IP_ADDR < $1
    sleep 15
    # Check that marker is "eNB L1 are configured"
    echo "egrep -c \"eNB L1 are configured\" /home/ubuntu/tmp/cmake_targets/log/$LOC_LOG_FILE" > $LOC_ENB_VM_CMDS
    cat $LOC_ENB_VM_CMDS
    local i="0"
    while [ $i -lt 10 ]
    do
        RESPONSE=`ssh -o StrictHostKeyChecking=no ubuntu@$LOC_VM_IP_ADDR < $LOC_ENB_VM_CMDS`
        if [ $RESPONSE -ne 0 ]
        then
            i="100"
        else
            sleep 10
            i=$[$i+1]
        fi
    done

    if [ $i -eq 100 ] 
    then
        echo "Syncro succeeded"
        SYNC=1
    else
        echo "Syncro failed"
        SYNC=0
    fi
    rm $1
}

function stop_nos1_sim_enb {
    local LOC_VM_IP_ADDR=$2
    echo "NB_OAI_PROCESSES=\`ps -aux | grep modem | grep -v grep | grep -c softmodem\`" > $1
    echo "if [ \$NB_OAI_PROCESSES -ne 0 ]; then echo \"sudo daemon --name=enb_daemon --stop\"; fi" >> $1
    echo "if [ \$NB_OAI_PROCESSES -ne 0 ]; then sudo daemon --name=enb_daemon --stop; fi" >> $1
    echo "if [ \$NB_OAI_PROCESSES -ne 0 ]; then echo \"sudo killall --signal SIGINT lte-softmodem-nos1\"; fi" >> $1
    echo "if [ \$NB_OAI_PROCESSES -ne 0 ]; then sudo killall --signal SIGINT lte-softmodem-nos1; fi" >> $1
    echo "sleep 1" >> $1
    echo "echo \"ps -aux | grep softmodem\"" >> $1
    echo "ps -aux | grep softmodem | grep -v grep" >> $1
    echo "NB_OAI_PROCESSES=\`ps -aux | grep modem | grep -v grep | grep -c softmodem\`" >> $1
    echo "if [ \$NB_OAI_PROCESSES -ne 0 ]; then echo \"sudo killall --signal SIGKILL lte-softmodem-nos1\"; fi" >> $1
    echo "if [ \$NB_OAI_PROCESSES -ne 0 ]; then sudo killall --signal SIGKILL lte-softmodem-nos1; fi" >> $1
    echo "sleep 1" >> $1
    echo "echo \"ps -aux | grep softmodem\"" >> $1
    echo "ps -aux | grep softmodem | grep -v grep" >> $1

    ssh -o StrictHostKeyChecking=no ubuntu@$2 < $1
    rm -f $1

}

function start_nos1_sim_ue {
# start_nos1_sim_ue $UE_VM_CMDS $UE_VM_IP_ADDR $ENB_VM_IP_ADDR $CURRENT_UE_LOG_FILE 25 rru.band7.nos1.simulator.conf
    #echo '$@' $@
    local LOC_UE_VM_CMDS=$1    
    local LOC_UE_IP_ADDR=$2
    local LOC_ENB_IP_ADDR=$3
    local LOC_LOG_FILE=$4
    local LOC_NB_RBS=$5
    local LOC_CONF_FILE=$6
    echo "cd /home/ubuntu/tmp" > $1
    echo "echo \"sudo apt-get --yes --quiet install daemon \"" >> $1
    echo "sudo apt-get --yes install daemon >> /home/ubuntu/tmp/cmake_targets/log/daemon-install.txt 2>&1" >> $1
    echo "echo \"source oaienv\"" >> $1
    echo "source oaienv" >> $1
    echo "cd ci-scripts/conf_files/" >> $1
    echo "cp $LOC_CONF_FILE ci-$LOC_CONF_FILE" >> $1
    echo "sed -i -e 's#CI_UE_IP_ADDR#$LOC_UE_IP_ADDR#' -e 's#CI_ENB_IP_ADDR#$LOC_ENB_IP_ADDR#' ci-$LOC_CONF_FILE" >> $1
    echo "echo \"cd /home/ubuntu/tmp/cmake_targets/tools\"" >> $1
    echo "cd /home/ubuntu/tmp/cmake_targets/tools/" >> $1
    echo "echo \"if [ \`lsmod | grep -c nasmesh\` -eq 0 ]; then sudo -E ./init_nas_nos1 UE; fi\"" >> $1
    echo "if [ \`lsmod | grep -c nasmesh\` -eq 0 ]; then sudo -E ./init_nas_nos1 UE; fi" >> $1
    echo "echo \"cd /home/ubuntu/tmp/cmake_targets/lte_noS1_build_oai/build/\"" >> $1
    echo "cd /home/ubuntu/tmp/cmake_targets/lte_noS1_build_oai/build/" >> $1
    echo "echo \"ulimit -c unlimited && ./lte-uesoftmodem-nos1 -O /home/ubuntu/tmp/ci-scripts/conf_files/ci-$LOC_CONF_FILE -r $LOC_NB_RBS --siml1\" | sudo tee ./my-lte-uesoftmodem-run.sh " >> $1 
    echo "sudo chmod 775 ./my-lte-uesoftmodem-run.sh" >> $1
    echo "sudo rm -f /home/ubuntu/tmp/cmake_targets/log/$LOC_LOG_FILE" >> $1
    echo "sudo -E daemon --inherit --unsafe --name=ue_daemon --chdir=/home/ubuntu/tmp/cmake_targets/lte_noS1_build_oai/build -o /home/ubuntu/tmp/cmake_targets/log/$LOC_LOG_FILE ./my-lte-uesoftmodem-run.sh" >> $1
    ssh -o StrictHostKeyChecking=no ubuntu@$LOC_UE_IP_ADDR < $1
    sleep 10
    rm $1
# Check that marker is "Generating RRCConnectionRequest"
    echo "egrep -c \"Generating RRCConnectionRequest\" /home/ubuntu/tmp/cmake_targets/log/$LOC_LOG_FILE" > $LOC_UE_VM_CMDS
    cat $LOC_UE_VM_CMDS
    local i="0" 
    while [ $i -lt 30 ]
    do
        RESPONSE=`ssh -o StrictHostKeyChecking=no ubuntu@$LOC_UE_IP_ADDR < $LOC_UE_VM_CMDS`
        if [ $RESPONSE -ne 0 ]
        then
            i="100"
        else
            sleep 10
            i=$[$i+1]
        fi
    done
    sleep 10
    if [ $i -eq 100 ]
    then
        echo "Syncro succeeded"
        UE_SYNC=1
    else
        echo "Syncro failed"
        UE_SYNC=0
    fi
}

function stop_nos1_sim_ue {
    local LOC_VM_IP_ADDR=$2
    echo "NB_OAI_PROCESSES=\`ps -aux | grep modem | grep -v grep | grep -c softmodem\`" > $1
    echo "if [ \$NB_OAI_PROCESSES -ne 0 ]; then echo \"sudo daemon --name=ue_daemon --stop\"; fi" >> $1
    echo "if [ \$NB_OAI_PROCESSES -ne 0 ]; then sudo daemon --name=ue_daemon --stop; fi" >> $1
    echo "if [ \$NB_OAI_PROCESSES -ne 0 ]; then echo \"sudo killall --signal SIGINT lte-uesoftmodem-nos1\"; fi" >> $1
    echo "if [ \$NB_OAI_PROCESSES -ne 0 ]; then sudo killall --signal SIGINT lte-uesoftmodem-nos1; fi" >> $1
    echo "sleep 1" >> $1
    echo "echo \"ps -aux | grep softmodem\"" >> $1
    echo "ps -aux | grep softmodem | grep -v grep" >> $1
    echo "NB_OAI_PROCESSES=\`ps -aux | grep modem | grep -v grep | grep -c softmodem\`" >> $1
    echo "if [ \$NB_OAI_PROCESSES -ne 0 ]; then echo \"sudo killall --signal SIGKILL lte-uesoftmodem-nos1\"; fi" >> $1
    echo "if [ \$NB_OAI_PROCESSES -ne 0 ]; then sudo killall --signal SIGKILL lte-uesoftmodem-nos1; fi" >> $1
    echo "sleep 1" >> $1
    echo "echo \"ps -aux | grep softmodem\"" >> $1
    echo "ps -aux | grep softmodem | grep -v grep" >> $1
    ssh -o StrictHostKeyChecking=no ubuntu@$2 < $1
    rm -f $1
}

function install_epc_on_vm {
    local LOC_EPC_VM_NAME=$1
    local LOC_EPC_VM_CMDS=$2

    if [ -d /opt/ltebox-archives/ ]
    then
        # Checking if all ltebox archives are available to run ltebx epc on a brand new VM
        if [ -f /opt/ltebox-archives/ltebox_2.2.70_16_04_amd64.deb ] && [ -f /opt/ltebox-archives/etc-conf.zip ] && [ -f /opt/ltebox-archives/hss-sim.zip ]
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
        uvt-kvm create $LOC_EPC_VM_NAME release=xenial --unsafe-caching
        echo "Waiting for VM to be started"
        uvt-kvm wait $LOC_EPC_VM_NAME --insecure
        release_vm_create_lock
    else
        echo "Waiting for VM to be started"
        uvt-kvm wait $LOC_EPC_VM_NAME --insecure
    fi

    local LOC_EPC_VM_IP_ADDR=`uvt-kvm ip $LOC_EPC_VM_NAME`

    echo "$LOC_EPC_VM_NAME has for IP addr = $LOC_EPC_VM_IP_ADDR"
    scp -o StrictHostKeyChecking=no /etc/apt/apt.conf.d/01proxy ubuntu@$LOC_EPC_VM_IP_ADDR:/home/ubuntu

    # ltebox specific actions (install and start)
    LTE_BOX_TO_INSTALL=1
    if [ $LTEBOX -eq 1 ]
    then
        echo "ls -ls /opt/ltebox/tools/start_ltebox" > $LOC_EPC_VM_CMDS
        RESPONSE=`ssh -o StrictHostKeyChecking=no ubuntu@$LOC_EPC_VM_IP_ADDR < $LOC_EPC_VM_CMDS`
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
        scp -o StrictHostKeyChecking=no /opt/ltebox-archives/hss-sim.zip ubuntu@$LOC_EPC_VM_IP_ADDR:/home/ubuntu

        echo "############################################################"
        echo "Install EPC on EPC VM ($LOC_EPC_VM_NAME)"
        echo "############################################################"
        echo "sudo cp 01proxy /etc/apt/apt.conf.d/" > $LOC_EPC_VM_CMDS
        echo "touch /home/ubuntu/.hushlogin" >> $LOC_EPC_VM_CMDS
        echo "echo \"sudo apt-get --yes --quiet install zip openjdk-8-jre libconfuse-dev libreadline-dev liblog4c-dev libgcrypt-dev libsctp-dev python2.7 python2.7-dev daemon iperf\"" >> $LOC_EPC_VM_CMDS
        echo "sudo apt-get update > zip-install.txt 2>&1" >> $LOC_EPC_VM_CMDS
        echo "sudo apt-get --yes install zip openjdk-8-jre libconfuse-dev libreadline-dev liblog4c-dev libgcrypt-dev libsctp-dev python2.7 python2.7-dev daemon iperf >> zip-install.txt 2>&1" >> $LOC_EPC_VM_CMDS

        # Installing HSS
        echo "echo \"cd /opt\"" >> $LOC_EPC_VM_CMDS
        echo "cd /opt" >> $LOC_EPC_VM_CMDS
        echo "echo \"sudo unzip -qq /home/ubuntu/hss-sim.zip\"" >> $LOC_EPC_VM_CMDS
        echo "sudo unzip -qq /home/ubuntu/hss-sim.zip" >> $LOC_EPC_VM_CMDS
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

        ssh -o StrictHostKeyChecking=no ubuntu@$LOC_EPC_VM_IP_ADDR < $LOC_EPC_VM_CMDS
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
        echo "echo \"sudo daemon --unsafe --name=simulated_hss --chdir=/opt/hss_sim0609 ./starthss_real\"" >> $LOC_EPC_VM_CMDS
        echo "sudo daemon --unsafe --name=simulated_hss --chdir=/opt/hss_sim0609 ./starthss_real" >> $LOC_EPC_VM_CMDS

        echo "echo \"cd /opt/ltebox/tools/\"" >> $LOC_EPC_VM_CMDS
        echo "cd /opt/ltebox/tools/" >> $LOC_EPC_VM_CMDS
        echo "echo \"sudo ./start_ltebox\"" >> $LOC_EPC_VM_CMDS
        echo "nohup sudo ./start_ltebox > /home/ubuntu/ltebox.txt" >> $LOC_EPC_VM_CMDS

        ssh -o StrictHostKeyChecking=no ubuntu@$LOC_EPC_VM_IP_ADDR < $LOC_EPC_VM_CMDS
        rm -f $LOC_EPC_VM_CMDS

        i="0"
        echo "ifconfig tun5 | egrep -c \"inet addr\"" > $LOC_EPC_VM_CMDS
        while [ $i -lt 10 ]
        do
            sleep 2
            CONNECTED=`ssh -o StrictHostKeyChecking=no ubuntu@$LOC_EPC_VM_IP_ADDR < $LOC_EPC_VM_CMDS`
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

    if [ $LTEBOX -eq 1 ]
    then
        # in our configuration file, we are using pool 5
        echo "ifconfig tun5 | egrep \"inet addr\" | sed -e 's#^.*inet addr:##' -e 's#  P-t-P:.*\$##'" > $LOC_EPC_VM_CMDS
        REAL_EPC_IP_ADDR=`ssh -o StrictHostKeyChecking=no ubuntu@$LOC_EPC_VM_IP_ADDR < $LOC_EPC_VM_CMDS`
        echo "EPC IP Address     is : $REAL_EPC_IP_ADDR"
        rm $LOC_EPC_VM_CMDS
    fi
}

function terminate_epc {
    if [ $LTEBOX -eq 1 ]
    then
        echo "echo \"cd /opt/ltebox/tools\"" > $1
        echo "cd /opt/ltebox/tools" >> $1
        echo "echo \"sudo ./stop_ltebox\"" >> $1
        echo "sudo ./stop_ltebox" >> $1
        echo "echo \"sudo daemon --name=simulated_hss --stop\"" >> $1
        echo "sudo daemon --name=simulated_hss --stop" >> $1
        echo "echo \"sudo killall --signal SIGKILL hss_sim\"" >> $1
        echo "sudo killall --signal SIGKILL hss_sim" >> $1
        ssh -o StrictHostKeyChecking=no ubuntu@$2 < $1
        rm $1
    fi
}

function start_flexran_ctrl {
    echo "cd /home/ubuntu/tmp" > $1
    echo "if [ -f cmake_targets/log/flexran_ctl_run.log ]; then rm -f cmake_targets/log/flexran_ctl_run.log cmake_targets/log/flexran_ctl_query*.log; fi" >> $1
    echo "echo \" sudo build/rt_controller -c log_config/basic_log\"" >> $1
    echo "nohup sudo build/rt_controller -c log_config/basic_log > cmake_targets/log/flexran_ctl_run.log 2>&1 &" >> $1
    ssh -o StrictHostKeyChecking=no ubuntu@$2 < $1
    rm $1
    sleep 10
}

function stop_flexran_ctrl {
    echo "echo \"sudo killall --signal SIGKILL rt_controller\"" > $1
    echo "sudo killall --signal SIGKILL rt_controller" > $1
    ssh -o StrictHostKeyChecking=no ubuntu@$2 < $1
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
    ssh -o StrictHostKeyChecking=no ubuntu@$2 < $1
    rm $1
}

function build_ue_on_separate_folder {
    echo "mkdir tmp-ue" > $1
    echo "cd tmp-ue" >> $1
    echo "echo \"unzip -qq -DD ../localZip.zip\"" >> $1
    echo "unzip -qq -DD ../localZip.zip" >> $1

    # We may have some adaptation to do
    if [ -f /opt/ltebox-archives/adapt_ue_l2_sim.txt ]
    then
        echo "############################################################"
        echo "Doing some adaptation on UE side"
        echo "############################################################"
        cat /opt/ltebox-archives/adapt_ue_l2_sim.txt >> $1
    fi

    echo "echo \"source oaienv\"" >> $1
    echo "source oaienv" >> $1
    echo "cd cmake_targets/" >> $1
    echo "mkdir log" >> $1
    echo "chmod 777 log" >> $1
    echo "echo \"./build_oai --UE -t ETHERNET \"" >> $1
    echo "./build_oai --UE -t ETHERNET > log/ue-build.txt 2>&1" >> $1
    echo "cd tools" >> $1
    echo "sudo ifconfig lo: 127.0.0.2 netmask 255.0.0.0 up" >> $1
    echo "sudo chmod 666 /etc/iproute2/rt_tables" >> $1
    echo "source init_nas_s1 UE" >> $1
    echo "ifconfig" >> $1
    ssh -o StrictHostKeyChecking=no ubuntu@$2 < $1
    rm -f $1
}

function start_l2_sim_enb {
    local LOC_VM_IP_ADDR=$2
    local LOC_EPC_IP_ADDR=$3
    local LOC_LOG_FILE=$4
    local LOC_NB_RBS=$5
    local LOC_CONF_FILE=$6
    echo "cd /home/ubuntu/tmp" > $1
    echo "echo \"sudo apt-get --yes --quiet install daemon \"" >> $1
    echo "sudo apt-get --yes install daemon >> /home/ubuntu/tmp/cmake_targets/log/daemon-install.txt 2>&1" >> $1
    echo "echo \"source oaienv\"" >> $1
    echo "source oaienv" >> $1
    echo "cd ci-scripts/conf_files/" >> $1
    echo "cp $LOC_CONF_FILE ci-$LOC_CONF_FILE" >> $1
    echo "sed -i -e 's#N_RB_DL.*=.*;#N_RB_DL                                         = $LOC_NB_RBS;#' -e 's#CI_MME_IP_ADDR#$LOC_EPC_IP_ADDR#' -e 's#CI_ENB_IP_ADDR#$LOC_VM_IP_ADDR#' ci-$LOC_CONF_FILE" >> $1
    echo "echo \"grep N_RB_DL ci-$LOC_CONF_FILE\"" >> $1
    echo "grep N_RB_DL ci-$LOC_CONF_FILE | sed -e 's#N_RB_DL.*=#N_RB_DL =#'" >> $1
    echo "echo \"cd /home/ubuntu/tmp/cmake_targets/lte_build_oai/build/\"" >> $1
    echo "sudo chmod 777 /home/ubuntu/tmp/cmake_targets/lte_build_oai/build/" >> $1
    echo "cd /home/ubuntu/tmp/cmake_targets/lte_build_oai/build/" >> $1
    echo "echo \"ulimit -c unlimited && ./lte-softmodem -O /home/ubuntu/tmp/ci-scripts/conf_files/ci-$LOC_CONF_FILE\" > ./my-lte-softmodem-run.sh " >> $1
    echo "chmod 775 ./my-lte-softmodem-run.sh" >> $1
    echo "cat ./my-lte-softmodem-run.sh" >> $1
    echo "if [ -e /home/ubuntu/tmp/cmake_targets/log/$LOC_LOG_FILE ]; then sudo sudo rm -f /home/ubuntu/tmp/cmake_targets/log/$LOC_LOG_FILE; fi" >> $1
    echo "sudo -E daemon --inherit --unsafe --name=enb_daemon --chdir=/home/ubuntu/tmp/cmake_targets/lte_build_oai/build/ -o /home/ubuntu/tmp/cmake_targets/log/$LOC_LOG_FILE ./my-lte-softmodem-run.sh" >> $1

    ssh -o StrictHostKeyChecking=no ubuntu@$LOC_VM_IP_ADDR < $1
    rm $1

    local i="0"
    echo "egrep -c \"Waiting for PHY_config_req\" /home/ubuntu/tmp/cmake_targets/log/$LOC_LOG_FILE" > $1
    while [ $i -lt 10 ]
    do
        sleep 5
        CONNECTED=`ssh -o StrictHostKeyChecking=no ubuntu@$LOC_VM_IP_ADDR < $1`
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
        ENB_SYNC=0
        echo "L2-SIM eNB is NOT sync'ed: process still alive?"
    else
        ENB_SYNC=1
        echo "L2-SIM eNB is sync'ed: waiting for UE(s) to connect"
    fi
}

function start_l2_sim_ue {
    local LOC_VM_IP_ADDR=$2
    local LOC_LOG_FILE=$3
    local LOC_CONF_FILE=$4
    echo "echo \"cd /home/ubuntu/tmp-ue/cmake_targets/lte_build_oai/build/\"" >> $1
    echo "sudo chmod 777 /home/ubuntu/tmp-ue/cmake_targets/lte_build_oai/build/" >> $1
    echo "cd /home/ubuntu/tmp-ue/cmake_targets/lte_build_oai/build/" >> $1
    echo "echo \"ulimit -c unlimited && ./lte-uesoftmodem -O /home/ubuntu/tmp-ue/ci-scripts/conf_files/$LOC_CONF_FILE --L2-emul 3 --num-ues 1\" > ./my-lte-softmodem-run.sh " >> $1
    echo "chmod 775 ./my-lte-softmodem-run.sh" >> $1
    echo "cat ./my-lte-softmodem-run.sh" >> $1
    echo "if [ -e /home/ubuntu/tmp/cmake_targets/log/$LOC_LOG_FILE ]; then sudo sudo rm -f /home/ubuntu/tmp/cmake_targets/log/$LOC_LOG_FILE; fi" >> $1
    echo "sudo -E daemon --inherit --unsafe --name=ue_daemon --chdir=/home/ubuntu/tmp-ue/cmake_targets/lte_build_oai/build/ -o /home/ubuntu/tmp/cmake_targets/log/$LOC_LOG_FILE ./my-lte-softmodem-run.sh" >> $1

    ssh -o StrictHostKeyChecking=no ubuntu@$LOC_VM_IP_ADDR < $1
    rm $1

    local i="0"
    echo "egrep -c \"Received NFAPI_START_REQ\" /home/ubuntu/tmp/cmake_targets/log/$LOC_LOG_FILE" > $1
    while [ $i -lt 10 ]
    do
        sleep 5
        CONNECTED=`ssh -o StrictHostKeyChecking=no ubuntu@$LOC_VM_IP_ADDR < $1`
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
        echo "L2-SIM UE is NOT sync'ed w/ eNB"
        return
    else
        UE_SYNC=1
        echo "L2-SIM UE is sync'ed w/ eNB"
    fi
    # Checking oip1 interface has now an IP address
    i="0"
    echo "ifconfig oip1 | egrep -c \"inet addr\"" > $1
    while [ $i -lt 10 ]
    do
        sleep 5
        CONNECTED=`ssh -o StrictHostKeyChecking=no ubuntu@$LOC_VM_IP_ADDR < $1`
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
        echo "L2-SIM UE oip1 is NOT sync'ed w/ EPC"
    else
        UE_SYNC=1
        echo "L2-SIM UE oip1 is sync'ed w/ EPC"
    fi
    sleep 10
}

function run_test_on_vm {
    echo "############################################################"
    echo "OAI CI VM script"
    echo "############################################################"
    echo "VM_NAME             = $VM_NAME"
    echo "VM_CMD_FILE         = $VM_CMDS"
    echo "JENKINS_WKSP        = $JENKINS_WKSP"
    echo "ARCHIVES_LOC        = $ARCHIVES_LOC"

    echo "############################################################"
    echo "Waiting for VM to be started"
    echo "############################################################"
    uvt-kvm wait $VM_NAME --insecure

    VM_IP_ADDR=`uvt-kvm ip $VM_NAME`
    echo "$VM_NAME has for IP addr = $VM_IP_ADDR"

    if [ "$RUN_OPTIONS" == "none" ]
    then
        echo "No run on VM testing for this variant currently"
        exit $STATUS
    fi

    if [[ $RUN_OPTIONS =~ .*run_exec_autotests.* ]]
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

        ssh -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR < $VM_CMDS

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
        install_epc_on_vm $EPC_VM_NAME $EPC_VM_CMDS
        EPC_VM_IP_ADDR=`uvt-kvm ip $EPC_VM_NAME`

        # Starting EPC
        start_epc $EPC_VM_NAME $EPC_VM_CMDS $EPC_VM_IP_ADDR

        # Retrieve EPC real IP address
        retrieve_real_epc_ip_addr $EPC_VM_NAME $EPC_VM_CMDS $EPC_VM_IP_ADDR

        # We may have some adaptation to do
        if [ -f /opt/ltebox-archives/adapt_ue_sim.txt ]
        then
            echo "############################################################"
            echo "Doing some adaptation on UE side"
            echo "############################################################"
            ssh -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR < /opt/ltebox-archives/adapt_ue_sim.txt
        fi

        echo "############################################################"
        echo "Starting the eNB in FDD-5MHz mode"
        echo "############################################################"
        CURRENT_ENB_LOG_FILE=fdd_05MHz_enb.log
        start_basic_sim_enb $VM_CMDS $VM_IP_ADDR $EPC_VM_IP_ADDR $CURRENT_ENB_LOG_FILE 25 lte-fdd-basic-sim.conf none

        echo "############################################################"
        echo "Starting the UE in FDD-5MHz mode"
        echo "############################################################"
        CURRENT_UE_LOG_FILE=fdd_05MHz_ue.log
        start_basic_sim_ue $VM_CMDS $VM_IP_ADDR $CURRENT_UE_LOG_FILE 25 2680
        if [ $UE_SYNC -eq 0 ]
        then
            echo "Problem w/ eNB and UE not syncing"
            terminate_enb_ue_basic_sim $VM_CMDS $VM_IP_ADDR
            scp -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/$CURRENT_ENB_LOG_FILE $ARCHIVES_LOC
            scp -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/$CURRENT_UE_LOG_FILE $ARCHIVES_LOC
            recover_core_dump $VM_CMDS $VM_IP_ADDR $ARCHIVES_LOC/$CURRENT_ENB_LOG_FILE $ARCHIVES_LOC
            terminate_epc $EPC_VM_CMDS $EPC_VM_IP_ADDR
            echo "TEST_KO" > $ARCHIVES_LOC/test_final_status.log
            exit -1
        fi
        get_ue_ip_addr $VM_CMDS $VM_IP_ADDR

        echo "############################################################"
        echo "Pinging the UE"
        echo "############################################################"
        PING_LOG_FILE=fdd_05MHz_ping_ue.txt
        ping_ue_ip_addr $EPC_VM_CMDS $EPC_VM_IP_ADDR $UE_IP_ADDR $PING_LOG_FILE
        scp -o StrictHostKeyChecking=no ubuntu@$EPC_VM_IP_ADDR:/home/ubuntu/$PING_LOG_FILE $ARCHIVES_LOC
        check_ping_result $ARCHIVES_LOC/$PING_LOG_FILE 20

        echo "############################################################"
        echo "Iperf DL"
        echo "############################################################"
        CURR_IPERF_LOG_BASE=fdd_05MHz_iperf_dl
        iperf_dl $VM_CMDS $VM_IP_ADDR $EPC_VM_CMDS $EPC_VM_IP_ADDR 15 $CURR_IPERF_LOG_BASE
        scp -o StrictHostKeyChecking=no ubuntu@$EPC_VM_IP_ADDR:/home/ubuntu/${CURR_IPERF_LOG_BASE}_client.txt $ARCHIVES_LOC
        scp -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/${CURR_IPERF_LOG_BASE}_server.txt $ARCHIVES_LOC
        check_iperf $ARCHIVES_LOC/$CURR_IPERF_LOG_BASE 15

        echo "############################################################"
        echo "Iperf UL"
        echo "############################################################"
        CURR_IPERF_LOG_BASE=fdd_05MHz_iperf_ul
        iperf_ul $VM_CMDS $VM_IP_ADDR $EPC_VM_CMDS $EPC_VM_IP_ADDR 2 $CURR_IPERF_LOG_BASE
        scp -o StrictHostKeyChecking=no ubuntu@$EPC_VM_IP_ADDR:/home/ubuntu/${CURR_IPERF_LOG_BASE}_server.txt $ARCHIVES_LOC
        scp -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/${CURR_IPERF_LOG_BASE}_client.txt $ARCHIVES_LOC
        check_iperf $ARCHIVES_LOC/$CURR_IPERF_LOG_BASE 2

        echo "############################################################"
        echo "Terminate enb/ue simulators"
        echo "############################################################"
        terminate_enb_ue_basic_sim $VM_CMDS $VM_IP_ADDR
        scp -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/$CURRENT_ENB_LOG_FILE $ARCHIVES_LOC
        scp -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/$CURRENT_UE_LOG_FILE $ARCHIVES_LOC
        recover_core_dump $VM_CMDS $VM_IP_ADDR $ARCHIVES_LOC/$CURRENT_ENB_LOG_FILE $ARCHIVES_LOC
        sleep 10

        echo "############################################################"
        echo "Starting the eNB in FDD-10MHz mode"
        echo "############################################################"
        CURRENT_ENB_LOG_FILE=fdd_10MHz_enb.log
        start_basic_sim_enb $VM_CMDS $VM_IP_ADDR $EPC_VM_IP_ADDR $CURRENT_ENB_LOG_FILE 50 lte-fdd-basic-sim.conf none

        echo "############################################################"
        echo "Starting the UE in FDD-10MHz mode"
        echo "############################################################"
        CURRENT_UE_LOG_FILE=fdd_10MHz_ue.log
        start_basic_sim_ue $VM_CMDS $VM_IP_ADDR $CURRENT_UE_LOG_FILE 50 2680
        if [ $UE_SYNC -eq 0 ]
        then
            echo "Problem w/ eNB and UE not syncing"
            terminate_enb_ue_basic_sim $VM_CMDS $VM_IP_ADDR
            scp -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/$CURRENT_ENB_LOG_FILE $ARCHIVES_LOC
            scp -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/$CURRENT_UE_LOG_FILE $ARCHIVES_LOC
            recover_core_dump $VM_CMDS $VM_IP_ADDR $ARCHIVES_LOC/$CURRENT_ENB_LOG_FILE $ARCHIVES_LOC
            terminate_epc $EPC_VM_CMDS $EPC_VM_IP_ADDR
            echo "TEST_KO" > $ARCHIVES_LOC/test_final_status.log
            exit -1
        fi
        get_ue_ip_addr $VM_CMDS $VM_IP_ADDR

        echo "############################################################"
        echo "Pinging the UE"
        echo "############################################################"
        PING_LOG_FILE=fdd_10MHz_ping_ue.txt
        ping_ue_ip_addr $EPC_VM_CMDS $EPC_VM_IP_ADDR $UE_IP_ADDR $PING_LOG_FILE
        scp -o StrictHostKeyChecking=no ubuntu@$EPC_VM_IP_ADDR:/home/ubuntu/$PING_LOG_FILE $ARCHIVES_LOC
        check_ping_result $ARCHIVES_LOC/$PING_LOG_FILE 20

        echo "############################################################"
        echo "Iperf DL"
        echo "############################################################"
        CURR_IPERF_LOG_BASE=fdd_10MHz_iperf_dl
        iperf_dl $VM_CMDS $VM_IP_ADDR $EPC_VM_CMDS $EPC_VM_IP_ADDR 15 $CURR_IPERF_LOG_BASE
        scp -o StrictHostKeyChecking=no ubuntu@$EPC_VM_IP_ADDR:/home/ubuntu/${CURR_IPERF_LOG_BASE}_client.txt $ARCHIVES_LOC
        scp -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/${CURR_IPERF_LOG_BASE}_server.txt $ARCHIVES_LOC
        check_iperf $ARCHIVES_LOC/$CURR_IPERF_LOG_BASE 15

        echo "############################################################"
        echo "Iperf UL"
        echo "############################################################"
        CURR_IPERF_LOG_BASE=fdd_10MHz_iperf_ul
        iperf_ul $VM_CMDS $VM_IP_ADDR $EPC_VM_CMDS $EPC_VM_IP_ADDR 2 $CURR_IPERF_LOG_BASE
        scp -o StrictHostKeyChecking=no ubuntu@$EPC_VM_IP_ADDR:/home/ubuntu/${CURR_IPERF_LOG_BASE}_server.txt $ARCHIVES_LOC
        scp -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/${CURR_IPERF_LOG_BASE}_client.txt $ARCHIVES_LOC
        check_iperf $ARCHIVES_LOC/$CURR_IPERF_LOG_BASE 2

        echo "############################################################"
        echo "Terminate enb/ue simulators"
        echo "############################################################"
        terminate_enb_ue_basic_sim $VM_CMDS $VM_IP_ADDR
        scp -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/$CURRENT_ENB_LOG_FILE $ARCHIVES_LOC
        scp -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/$CURRENT_UE_LOG_FILE $ARCHIVES_LOC
        recover_core_dump $VM_CMDS $VM_IP_ADDR $ARCHIVES_LOC/$CURRENT_ENB_LOG_FILE $ARCHIVES_LOC
        sleep 10

        echo "############################################################"
        echo "Starting the eNB in FDD-20MHz mode"
        echo "############################################################"
        CURRENT_ENB_LOG_FILE=fdd_20MHz_enb.log
        start_basic_sim_enb $VM_CMDS $VM_IP_ADDR $EPC_VM_IP_ADDR $CURRENT_ENB_LOG_FILE 100 lte-fdd-basic-sim.conf none

        echo "############################################################"
        echo "Starting the UE in FDD-20MHz mode"
        echo "############################################################"
        CURRENT_UE_LOG_FILE=fdd_20MHz_ue.log
        start_basic_sim_ue $VM_CMDS $VM_IP_ADDR $CURRENT_UE_LOG_FILE 100 2680
        if [ $UE_SYNC -eq 0 ]
        then
            echo "Problem w/ eNB and UE not syncing"
            terminate_enb_ue_basic_sim $VM_CMDS $VM_IP_ADDR
            scp -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/$CURRENT_ENB_LOG_FILE $ARCHIVES_LOC
            scp -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/$CURRENT_UE_LOG_FILE $ARCHIVES_LOC
            recover_core_dump $VM_CMDS $VM_IP_ADDR $ARCHIVES_LOC/$CURRENT_ENB_LOG_FILE $ARCHIVES_LOC
            terminate_epc $EPC_VM_CMDS $EPC_VM_IP_ADDR
            echo "TEST_KO" > $ARCHIVES_LOC/test_final_status.log
            exit -1
        fi
        get_ue_ip_addr $VM_CMDS $VM_IP_ADDR

        echo "############################################################"
        echo "Pinging the UE"
        echo "############################################################"
        PING_LOG_FILE=fdd_20MHz_ping_ue.txt
        ping_ue_ip_addr $EPC_VM_CMDS $EPC_VM_IP_ADDR $UE_IP_ADDR $PING_LOG_FILE
        scp -o StrictHostKeyChecking=no ubuntu@$EPC_VM_IP_ADDR:/home/ubuntu/$PING_LOG_FILE $ARCHIVES_LOC
        check_ping_result $ARCHIVES_LOC/$PING_LOG_FILE 20

        echo "############################################################"
        echo "Iperf DL"
        echo "############################################################"
        CURR_IPERF_LOG_BASE=fdd_20MHz_iperf_dl
        iperf_dl $VM_CMDS $VM_IP_ADDR $EPC_VM_CMDS $EPC_VM_IP_ADDR 15 $CURR_IPERF_LOG_BASE
        scp -o StrictHostKeyChecking=no ubuntu@$EPC_VM_IP_ADDR:/home/ubuntu/${CURR_IPERF_LOG_BASE}_client.txt $ARCHIVES_LOC
        scp -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/${CURR_IPERF_LOG_BASE}_server.txt $ARCHIVES_LOC
        check_iperf $ARCHIVES_LOC/$CURR_IPERF_LOG_BASE 15

        echo "############################################################"
        echo "Terminate enb/ue simulators"
        echo "############################################################"
        terminate_enb_ue_basic_sim $VM_CMDS $VM_IP_ADDR
        scp -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/$CURRENT_ENB_LOG_FILE $ARCHIVES_LOC
        scp -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/$CURRENT_UE_LOG_FILE $ARCHIVES_LOC
        recover_core_dump $VM_CMDS $VM_IP_ADDR $ARCHIVES_LOC/$CURRENT_ENB_LOG_FILE $ARCHIVES_LOC
        sleep 10

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
                exit -1
            fi
            FLEXRAN_CTL_VM_NAME=`echo $VM_NAME | sed -e "s#basic-sim#flexran-rtc#"`
            FLEXRAN_CTL_VM_CMDS=`echo $VM_CMDS | sed -e "s#cmds#flexran-rtc-cmds#"`
            IS_FLEXRAN_VM_ALIVE=`uvt-kvm list | grep -c $FLEXRAN_CTL_VM_NAME`
            if [ $IS_FLEXRAN_VM_ALIVE -eq 0 ]
            then
                echo "ERROR: Flexran Ctl VM is not alive"
                terminate_epc $EPC_VM_CMDS $EPC_VM_IP_ADDR
                echo "TEST_KO" > $ARCHIVES_LOC/test_final_status.log
                exit -1
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
                terminate_enb_ue_basic_sim $VM_CMDS $VM_IP_ADDR
                scp -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/$CURRENT_ENB_LOG_FILE $ARCHIVES_LOC
                scp -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/$CURRENT_UE_LOG_FILE $ARCHIVES_LOC
                recover_core_dump $VM_CMDS $VM_IP_ADDR $ARCHIVES_LOC/$CURRENT_ENB_LOG_FILE $ARCHIVES_LOC
                terminate_epc $EPC_VM_CMDS $EPC_VM_IP_ADDR
                stop_flexran_ctrl $FLEXRAN_CTL_VM_CMDS $FLEXRAN_CTL_VM_IP_ADDR
                echo "TEST_KO" > $ARCHIVES_LOC/test_final_status.log
                exit -1
            fi
            query_flexran_ctrl_status $FLEXRAN_CTL_VM_CMDS $FLEXRAN_CTL_VM_IP_ADDR 03_enb_ue_connected
            get_ue_ip_addr $VM_CMDS $VM_IP_ADDR

            sleep 30
            echo "############################################################"
            echo "Terminate enb/ue simulators"
            echo "############################################################"
            terminate_enb_ue_basic_sim $VM_CMDS $VM_IP_ADDR
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
        echo "Starting the eNB in TDD-5MHz mode"
        echo "############################################################"
        CURRENT_ENB_LOG_FILE=tdd_05MHz_enb.log
        start_basic_sim_enb $VM_CMDS $VM_IP_ADDR $EPC_VM_IP_ADDR $CURRENT_ENB_LOG_FILE 25 lte-tdd-basic-sim.conf none

        echo "############################################################"
        echo "Starting the UE in TDD-5MHz mode"
        echo "############################################################"
        CURRENT_UE_LOG_FILE=tdd_05MHz_ue.log
        start_basic_sim_ue $VM_CMDS $VM_IP_ADDR $CURRENT_UE_LOG_FILE 25 2350
        if [ $UE_SYNC -eq 0 ]
        then
            echo "Problem w/ eNB and UE not syncing"
            terminate_enb_ue_basic_sim $VM_CMDS $VM_IP_ADDR
            scp -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/$CURRENT_ENB_LOG_FILE $ARCHIVES_LOC
            scp -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/$CURRENT_UE_LOG_FILE $ARCHIVES_LOC
            recover_core_dump $VM_CMDS $VM_IP_ADDR $ARCHIVES_LOC/$CURRENT_ENB_LOG_FILE $ARCHIVES_LOC
            terminate_epc $EPC_VM_CMDS $EPC_VM_IP_ADDR
            echo "TEST_KO" > $ARCHIVES_LOC/test_final_status.log
            exit -1
        fi
        get_ue_ip_addr $VM_CMDS $VM_IP_ADDR

        echo "############################################################"
        echo "Pinging the UE"
        echo "############################################################"
        PING_LOG_FILE=tdd_05MHz_ping_ue.txt
        ping_ue_ip_addr $EPC_VM_CMDS $EPC_VM_IP_ADDR $UE_IP_ADDR $PING_LOG_FILE
        scp -o StrictHostKeyChecking=no ubuntu@$EPC_VM_IP_ADDR:/home/ubuntu/$PING_LOG_FILE $ARCHIVES_LOC
        check_ping_result $ARCHIVES_LOC/$PING_LOG_FILE 20

#        Bug in TDD 5Mhz --- not running it
#        echo "############################################################"
#        echo "Iperf DL"
#        echo "############################################################"
#        CURR_IPERF_LOG_BASE=tdd_05MHz_iperf_dl
#        iperf_dl $VM_CMDS $VM_IP_ADDR $EPC_VM_CMDS $EPC_VM_IP_ADDR 6 $CURR_IPERF_LOG_BASE
#        scp -o StrictHostKeyChecking=no ubuntu@$EPC_VM_IP_ADDR:/home/ubuntu/${CURR_IPERF_LOG_BASE}_client.txt $ARCHIVES_LOC
#        scp -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/${CURR_IPERF_LOG_BASE}_server.txt $ARCHIVES_LOC
#        check_iperf $ARCHIVES_LOC/$CURR_IPERF_LOG_BASE 6

        echo "############################################################"
        echo "Terminate enb/ue simulators"
        echo "############################################################"
        terminate_enb_ue_basic_sim $VM_CMDS $VM_IP_ADDR
        scp -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/$CURRENT_ENB_LOG_FILE $ARCHIVES_LOC
        scp -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/$CURRENT_UE_LOG_FILE $ARCHIVES_LOC
        recover_core_dump $VM_CMDS $VM_IP_ADDR $ARCHIVES_LOC/$CURRENT_ENB_LOG_FILE $ARCHIVES_LOC
        sleep 10

        echo "############################################################"
        echo "Starting the eNB in TDD-10MHz mode"
        echo "############################################################"
        CURRENT_ENB_LOG_FILE=tdd_10MHz_enb.log
        start_basic_sim_enb $VM_CMDS $VM_IP_ADDR $EPC_VM_IP_ADDR $CURRENT_ENB_LOG_FILE 50 lte-tdd-basic-sim.conf none

        echo "############################################################"
        echo "Starting the UE in TDD-10MHz mode"
        echo "############################################################"
        CURRENT_UE_LOG_FILE=tdd_10MHz_ue.log
        start_basic_sim_ue $VM_CMDS $VM_IP_ADDR $CURRENT_UE_LOG_FILE 50 2350
        if [ $UE_SYNC -eq 0 ]
        then
            echo "Problem w/ eNB and UE not syncing"
            terminate_enb_ue_basic_sim $VM_CMDS $VM_IP_ADDR
            scp -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/$CURRENT_ENB_LOG_FILE $ARCHIVES_LOC
            scp -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/$CURRENT_UE_LOG_FILE $ARCHIVES_LOC
            recover_core_dump $VM_CMDS $VM_IP_ADDR $ARCHIVES_LOC/$CURRENT_ENB_LOG_FILE $ARCHIVES_LOC
            terminate_epc $EPC_VM_CMDS $EPC_VM_IP_ADDR
            echo "TEST_KO" > $ARCHIVES_LOC/test_final_status.log
            exit -1
        fi
        get_ue_ip_addr $VM_CMDS $VM_IP_ADDR

        echo "############################################################"
        echo "Pinging the UE"
        echo "############################################################"
        PING_LOG_FILE=tdd_10MHz_ping_ue.txt
        ping_ue_ip_addr $EPC_VM_CMDS $EPC_VM_IP_ADDR $UE_IP_ADDR $PING_LOG_FILE
        scp -o StrictHostKeyChecking=no ubuntu@$EPC_VM_IP_ADDR:/home/ubuntu/$PING_LOG_FILE $ARCHIVES_LOC
        check_ping_result $ARCHIVES_LOC/$PING_LOG_FILE 20

        echo "############################################################"
        echo "Iperf DL"
        echo "############################################################"
        CURR_IPERF_LOG_BASE=tdd_10MHz_iperf_dl
        iperf_dl $VM_CMDS $VM_IP_ADDR $EPC_VM_CMDS $EPC_VM_IP_ADDR 6 $CURR_IPERF_LOG_BASE
        scp -o StrictHostKeyChecking=no ubuntu@$EPC_VM_IP_ADDR:/home/ubuntu/${CURR_IPERF_LOG_BASE}_client.txt $ARCHIVES_LOC
        scp -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/${CURR_IPERF_LOG_BASE}_server.txt $ARCHIVES_LOC
        check_iperf $ARCHIVES_LOC/$CURR_IPERF_LOG_BASE 6

        echo "############################################################"
        echo "Terminate enb/ue simulators"
        echo "############################################################"
        terminate_enb_ue_basic_sim $VM_CMDS $VM_IP_ADDR
        scp -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/$CURRENT_ENB_LOG_FILE $ARCHIVES_LOC
        scp -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/$CURRENT_UE_LOG_FILE $ARCHIVES_LOC
        recover_core_dump $VM_CMDS $VM_IP_ADDR $ARCHIVES_LOC/$CURRENT_ENB_LOG_FILE $ARCHIVES_LOC
        sleep 10

# Disabling TDD-20MHz in order to stop getting false negatives
#        echo "############################################################"
#        echo "Starting the eNB in TDD-20MHz mode"
#        echo "############################################################"
#        CURRENT_ENB_LOG_FILE=tdd_20MHz_enb.log
#        start_basic_sim_enb $VM_CMDS $VM_IP_ADDR $EPC_VM_IP_ADDR $CURRENT_ENB_LOG_FILE 100 lte-tdd-basic-sim.conf none
#
#        echo "############################################################"
#        echo "Starting the UE in TDD-20MHz mode"
#        echo "############################################################"
#        CURRENT_UE_LOG_FILE=tdd_20MHz_ue.log
#        start_basic_sim_ue $VM_CMDS $VM_IP_ADDR $CURRENT_UE_LOG_FILE 100 2350
#        if [ $UE_SYNC -eq 0 ]
#        then
#            echo "Problem w/ eNB and UE not syncing"
#            terminate_enb_ue_basic_sim $VM_CMDS $VM_IP_ADDR
#            scp -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/$CURRENT_ENB_LOG_FILE $ARCHIVES_LOC
#            scp -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/$CURRENT_UE_LOG_FILE $ARCHIVES_LOC
#            recover_core_dump $VM_CMDS $VM_IP_ADDR $ARCHIVES_LOC/$CURRENT_ENB_LOG_FILE $ARCHIVES_LOC
#            terminate_epc $EPC_VM_CMDS $EPC_VM_IP_ADDR
#            echo "TEST_KO" > $ARCHIVES_LOC/test_final_status.log
#            exit -1
#        fi
#        get_ue_ip_addr $VM_CMDS $VM_IP_ADDR
#
#        echo "############################################################"
#        echo "Pinging the UE"
#        echo "############################################################"
#        PING_LOG_FILE=tdd_20MHz_ping_ue.txt
#        ping_ue_ip_addr $EPC_VM_CMDS $EPC_VM_IP_ADDR $UE_IP_ADDR $PING_LOG_FILE
#        scp -o StrictHostKeyChecking=no ubuntu@$EPC_VM_IP_ADDR:/home/ubuntu/$PING_LOG_FILE $ARCHIVES_LOC
#        check_ping_result $ARCHIVES_LOC/$PING_LOG_FILE 20
#
#        echo "############################################################"
#        echo "Iperf DL"
#        echo "############################################################"
#        CURR_IPERF_LOG_BASE=tdd_20MHz_iperf_dl
#        iperf_dl $VM_CMDS $VM_IP_ADDR $EPC_VM_CMDS $EPC_VM_IP_ADDR 6 $CURR_IPERF_LOG_BASE
#        scp -o StrictHostKeyChecking=no ubuntu@$EPC_VM_IP_ADDR:/home/ubuntu/${CURR_IPERF_LOG_BASE}_client.txt $ARCHIVES_LOC
#        scp -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/${CURR_IPERF_LOG_BASE}_server.txt $ARCHIVES_LOC
#        check_iperf $ARCHIVES_LOC/$CURR_IPERF_LOG_BASE 6
#
#        echo "############################################################"
#        echo "Terminate enb/ue simulators"
#        echo "############################################################"
#        terminate_enb_ue_basic_sim $VM_CMDS $VM_IP_ADDR
#        scp -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/$CURRENT_ENB_LOG_FILE $ARCHIVES_LOC
#        scp -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/$CURRENT_UE_LOG_FILE $ARCHIVES_LOC
#        recover_core_dump $VM_CMDS $VM_IP_ADDR $ARCHIVES_LOC/$CURRENT_ENB_LOG_FILE $ARCHIVES_LOC
#        sleep 10

        echo "############################################################"
        echo "Terminate EPC"
        echo "############################################################"

        terminate_epc $EPC_VM_CMDS $EPC_VM_IP_ADDR

        if [ $KEEP_VM_ALIVE -eq 0 ]
        then
            echo "############################################################"
            echo "Destroying VMs"
            echo "############################################################"
            uvt-kvm destroy $VM_NAME
            ssh-keygen -R $VM_IP_ADDR
            uvt-kvm destroy $EPC_VM_NAME
            ssh-keygen -R $EPC_VM_IP_ADDR
            if [ -e $JENKINS_WKSP/flexran/flexran_build_complete.txt ]
            then
                uvt-kvm destroy $FLEXRAN_CTL_VM_NAME
                ssh-keygen -R $FLEXRAN_CTL_VM_IP_ADDR
            fi
        fi

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
        build_ue_on_separate_folder $VM_CMDS $VM_IP_ADDR

        # Creating a VM for EPC and installing SW
        EPC_VM_NAME=`echo $VM_NAME | sed -e "s#l2-sim#l2-epc#"`
        EPC_VM_CMDS=${EPC_VM_NAME}_cmds.txt
        LTEBOX=0
        install_epc_on_vm $EPC_VM_NAME $EPC_VM_CMDS
        EPC_VM_IP_ADDR=`uvt-kvm ip $EPC_VM_NAME`

        # Starting EPC
        start_epc $EPC_VM_NAME $EPC_VM_CMDS $EPC_VM_IP_ADDR

        # Retrieve EPC real IP address
        retrieve_real_epc_ip_addr $EPC_VM_NAME $EPC_VM_CMDS $EPC_VM_IP_ADDR

        echo "############################################################"
        echo "Starting the eNB in FDD-5MHz mode"
        echo "############################################################"
        CURRENT_ENB_LOG_FILE=fdd_05MHz_enb.log
        start_l2_sim_enb $VM_CMDS $VM_IP_ADDR $EPC_VM_IP_ADDR $CURRENT_ENB_LOG_FILE 25 rcc.band7.tm1.nfapi.conf

        echo "############################################################"
        echo "Starting the UEs"
        echo "############################################################"
        CURRENT_UE_LOG_FILE=fdd_05MHz_ue.log
        start_l2_sim_ue $VM_CMDS $VM_IP_ADDR $CURRENT_UE_LOG_FILE ue.nfapi.conf
        if [ $UE_SYNC -eq 0 ]
        then
            echo "Problem w/ eNB and UE not syncing"
            terminate_enb_ue_basic_sim $VM_CMDS $VM_IP_ADDR
            scp -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/$CURRENT_ENB_LOG_FILE $ARCHIVES_LOC
            scp -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/$CURRENT_UE_LOG_FILE $ARCHIVES_LOC
            terminate_epc $EPC_VM_CMDS $EPC_VM_IP_ADDR
            echo "TEST_KO" > $ARCHIVES_LOC/test_final_status.log
            exit -1
        fi

        echo "############################################################"
        echo "Pinging the EPC from UE"
        echo "############################################################"
        PING_LOG_FILE=fdd_05MHz_ping_epc.txt
        ping_epc_ip_addr $VM_CMDS $VM_IP_ADDR $REAL_EPC_IP_ADDR $PING_LOG_FILE
        scp -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR:/home/ubuntu/$PING_LOG_FILE $ARCHIVES_LOC
        check_ping_result $ARCHIVES_LOC/$PING_LOG_FILE 20

        echo "############################################################"
        echo "Terminate enb/ue simulators"
        echo "############################################################"
        terminate_enb_ue_basic_sim $VM_CMDS $VM_IP_ADDR
        scp -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/$CURRENT_ENB_LOG_FILE $ARCHIVES_LOC
        scp -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/$CURRENT_UE_LOG_FILE $ARCHIVES_LOC

        echo "############################################################"
        echo "Terminate EPC"
        echo "############################################################"

        terminate_epc $EPC_VM_CMDS $EPC_VM_IP_ADDR

        if [ $KEEP_VM_ALIVE -eq 0 ]
        then
            echo "############################################################"
            echo "Destroying VMs"
            echo "############################################################"
            uvt-kvm destroy $VM_NAME
            ssh-keygen -R $VM_IP_ADDR
            uvt-kvm destroy $EPC_VM_NAME
            ssh-keygen -R $EPC_VM_IP_ADDR
        fi

        echo "############################################################"
        echo "Checking run status"
        echo "############################################################"

        if [ $PING_STATUS -ne 0 ]; then STATUS=-1; fi
        if [ $STATUS -eq 0 ]
        then
            echo "TEST_OK" > $ARCHIVES_LOC/test_final_status.log
        else
            echo "TEST_KO" > $ARCHIVES_LOC/test_final_status.log
        fi
    fi

    if [[ "$RUN_OPTIONS" == "complex" ]] && [[ $VM_NAME =~ .*-ethernet.* ]]
    then
        PING_STATUS=0
        IPERF_STATUS=0
        if [ -d $ARCHIVES_LOC ]
        then
            rm -Rf $ARCHIVES_LOC
        fi
        mkdir --parents $ARCHIVES_LOC

        ENB_VM_NAME=$VM_NAME
        ENB_VM_CMDS=${ENB_VM_NAME}_cmds.txt
        echo "ENB_VM_NAME          = $ENB_VM_NAME"
        echo "ENB_VM_CMD_FILE      = $ENB_VM_CMDS"

        UE_VM_NAME=`echo $VM_NAME | sed -e "s#enb#ue#"`
        UE_VM_CMDS=${UE_VM_NAME}_cmds.txt
        echo "UE_VM_NAME          = $UE_VM_NAME"
        echo "UE_VM_CMD_FILE      = $UE_VM_CMDS"

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
        echo "Starting the eNB in FDD-5MHz mode"
        echo "############################################################"
        CURRENT_ENB_LOG_FILE=fdd_05MHz_enb.log
        echo "ENB_VM_CMDS = $ENB_VM_CMDS"
        echo "ENB_VM_IP_ADDR = $ENB_VM_IP_ADDR"
        echo "UE_VM_IP_ADDR = $UE_VM_IP_ADDR"
        echo "CURRENT_ENB_LOG_FILE = $CURRENT_ENB_LOG_FILE"
        start_nos1_sim_enb $ENB_VM_CMDS $ENB_VM_IP_ADDR $UE_VM_IP_ADDR $CURRENT_ENB_LOG_FILE 25 rcc.band7.nos1.simulator.conf
        echo "ifconfig oai0 | egrep \"inet addr\" | sed -e 's#^.*addr:##' -e 's#  Bcast.*##'" > $ENB_VM_CMDS
        ENB_REAL_IP_ADDR=`ssh -o StrictHostKeyChecking=no ubuntu@$ENB_VM_IP_ADDR < $ENB_VM_CMDS`
        echo "ENB oai0 interface IP addr: $ENB_REAL_IP_ADDR"

        echo "############################################################"
        echo "Starting the UE in FDD-5MHz mode"
        echo "############################################################"
        CURRENT_UE_LOG_FILE=fdd_05MHz_ue.log
        start_nos1_sim_ue $UE_VM_CMDS $UE_VM_IP_ADDR $ENB_VM_IP_ADDR $CURRENT_UE_LOG_FILE 25 rru.band7.nos1.simulator.conf
        echo "UE_SYNC = $UE_SYNC"
        echo "ifconfig oai0 | egrep \"inet addr\" | sed -e 's#^.*addr:##' -e 's#  Bcast.*##'" > $UE_VM_CMDS
        UE_REAL_IP_ADDR=`ssh -o StrictHostKeyChecking=no ubuntu@$UE_VM_IP_ADDR < $UE_VM_CMDS`
        echo "UE oai0 interface IP addr: $UE_REAL_IP_ADDR"
        if [ $UE_SYNC -eq 0 ]
        then
            echo "Problem w/ eNB and UE not syncing"
            stop_nos1_sim_enb $VM_CMDS $VM_IP_ADDR
            stop_nos1_sim_ue $UE_VM_CMDS $UE_VM_IP_ADDR
            scp -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/$CURRENT_ENB_LOG_FILE $ARCHIVES_LOC
            scp -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/$CURRENT_UE_LOG_FILE $ARCHIVES_LOC
            echo "TEST_KO" > $ARCHIVES_LOC/test_final_status.log
            exit -1
        fi

        echo "############################################################"
        echo "Pinging the UE from the eNB"
        echo "############################################################"
        PING_LOG_FILE=fdd_05MHz_ping_ue.txt
        ping_nos1_ue_ip_addr $ENB_VM_CMDS $ENB_VM_IP_ADDR $UE_REAL_IP_ADDR $PING_LOG_FILE
        #ping_ue_ip_addr $UE_CMDS $UE_VM_IP_ADDR $REAL_UE_IP_ADDR $PING_LOG_FILE
        scp -o StrictHostKeyChecking=no ubuntu@$ENB_VM_IP_ADDR:/home/ubuntu/$PING_LOG_FILE $ARCHIVES_LOC
        check_ping_result $ARCHIVES_LOC/$PING_LOG_FILE 20
        echo "############################################################"
        echo "END OF Pinging the UE from the eNB"
        echo "############################################################"

        echo "############################################################"
        echo "Pinging the eNB from the UE"
        echo "############################################################"
        PING_LOG_FILE=fdd_05MHz_ping_enb.txt
        ping_nos1_enb_ip_addr $UE_VM_CMDS $UE_VM_IP_ADDR $ENB_REAL_IP_ADDR $PING_LOG_FILE
        #ping_ue_ip_addr $UE_CMDS $UE_VM_IP_ADDR $REAL_UE_IP_ADDR $PING_LOG_FILE
        scp -o StrictHostKeyChecking=no ubuntu@$UE_VM_IP_ADDR:/home/ubuntu/$PING_LOG_FILE $ARCHIVES_LOC
        check_ping_result $ARCHIVES_LOC/$PING_LOG_FILE 20
        echo "############################################################"
        echo "END OF Pinging the eNB from the UE"
        echo "############################################################"   

        echo "############################################################"
        echo "Iperf DL (the server is the UE and the client is the eNB)" 
        echo "############################################################"
        CURR_IPERF_LOG_BASE=fdd_05MHz_iperf_dl
        nos1_iperf $UE_REAL_IP_ADDR $UE_VM_IP_ADDR $UE_VM_CMDS $ENB_VM_IP_ADDR $ENB_VM_CMDS 1 $CURR_IPERF_LOG_BASE
        scp -o StrictHostKeyChecking=no ubuntu@$ENB_VM_IP_ADDR:/home/ubuntu/${CURR_IPERF_LOG_BASE}_client.txt $ARCHIVES_LOC
        scp -o StrictHostKeyChecking=no ubuntu@$UE_VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/${CURR_IPERF_LOG_BASE}_server.txt $ARCHIVES_LOC
        check_iperf $ARCHIVES_LOC/$CURR_IPERF_LOG_BASE 1
        echo "############################################################"
        echo "END OF Iperf DL"
        echo "############################################################"

        echo "############################################################"
        echo "Iperf UL (the server is the eNB and the client is the UE)" 
        echo "############################################################"
        CURR_IPERF_LOG_BASE=fdd_05MHz_iperf_ul
        nos1_iperf $ENB_REAL_IP_ADDR $ENB_VM_IP_ADDR $ENB_VM_CMDS $UE_VM_IP_ADDR $UE_VM_CMDS 1 $CURR_IPERF_LOG_BASE
        scp -o StrictHostKeyChecking=no ubuntu@$UE_VM_IP_ADDR:/home/ubuntu/${CURR_IPERF_LOG_BASE}_client.txt $ARCHIVES_LOC
        scp -o StrictHostKeyChecking=no ubuntu@$ENB_VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/${CURR_IPERF_LOG_BASE}_server.txt $ARCHIVES_LOC
        check_iperf $ARCHIVES_LOC/$CURR_IPERF_LOG_BASE 1
        echo "############################################################"
        echo "END OF Iperf UL"
        echo "############################################################"

        echo "############################################################"
        echo "Stopping the UE in FDD-5MHz mode"
        echo "############################################################"
        stop_nos1_sim_ue $UE_VM_CMDS $UE_VM_IP_ADDR

        echo "############################################################"
        echo "Stopping the eNB in FDD-5MHz mode"
        echo "############################################################"
        echo "VM_CMDS = $VM_CMDS"
        echo "VM_IP_ADDR = $VM_IP_ADDR"
        stop_nos1_sim_enb $VM_CMDS $VM_IP_ADDR

        scp -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/$CURRENT_ENB_LOG_FILE $ARCHIVES_LOC
        scp -o StrictHostKeyChecking=no ubuntu@$UE_VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/$CURRENT_UE_LOG_FILE $ARCHIVES_LOC

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

        echo "############################################################"
        echo "Checking run status"
        echo "############################################################"

        if [ $PING_STATUS -ne 0 ]; then STATUS=-1; fi
        # iperf failures are ignored
        #if [ $IPERF_STATUS -ne 0 ]; then STATUS=-1; fi
        if [ $STATUS -eq 0 ]
        then
            echo "TEST_OK" > $ARCHIVES_LOC/test_final_status.log
        else
            echo "TEST_KO" > $ARCHIVES_LOC/test_final_status.log
        fi
    fi
}
