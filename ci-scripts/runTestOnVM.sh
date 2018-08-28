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

function usage {
    echo "OAI VM Test Run script"
    echo "   Original Author: Raphael Defosseux"
    echo "   Default:"
    echo "     -- eNB with USRP"
    echo ""
    echo "Usage:"
    echo "------"
    echo "    runTestOnVM.sh [OPTIONS]"
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
    echo "    --variant enb-usrp     OR -v1"
    echo "    --variant basic-sim    OR -v2"
    echo "    --variant phy-sim      OR -v3"
    echo "    --variant cppcheck     OR -v4"
    echo "    --variant enb-ethernet OR -v7"
    echo "    --variant ue-ethernet  OR -v8"
    echo "    Specify the variant to build."
    echo ""
    echo "    --keep-vm-alive OR -k"
    echo "    Keep the VM alive after the build."
    echo ""
    echo "    --help OR -h"
    echo "    Print this help message."
    echo ""
}

function variant_usage {
    echo "OAI VM Test Run script"
    echo "   Original Author: Raphael Defosseux"
    echo ""
    echo "    --variant enb-usrp     OR -v1"
    echo "    --variant basic-sim    OR -v2"
    echo "    --variant phy-sim      OR -v3"
    echo "    --variant cppcheck     OR -v4"
    echo "    --variant enb-ethernet OR -v7"
    echo "    --variant ue-ethernet  OR -v8"
    echo ""
}

if [ $# -lt 1 ] || [ $# -gt 9 ]
then
    echo "Syntax Error: not the correct number of arguments"
    echo ""
    usage
    exit 1
fi

VM_TEMPLATE=ci-
JOB_NAME=XX
BUILD_ID=XX
VM_NAME=ci-enb-usrp
ARCHIVES_LOC=enb_usrp/test
KEEP_VM_ALIVE=0
RUN_OPTIONS="none"
STATUS=0

while [[ $# -gt 0 ]]
do
key="$1"

case $key in
    -h|--help)
    shift
    usage
    exit 0
    ;;
    -jn|--job-name)
    JOB_NAME="$2"
    shift
    shift
    ;;
    -id|--build-id)
    BUILD_ID="$2"
    shift
    shift
    ;;
    -ws|--workspace)
    JENKINS_WKSP="$2"
    shift
    shift
    ;;
    -k|--keep-vm-alive)
    KEEP_VM_ALIVE=1
    shift
    ;;
    -v1)
    VM_NAME=ci-enb-usrp
    shift
    ;;
    -v2)
    VM_NAME=ci-basic-sim
    RUN_OPTIONS="complex"
    ARCHIVES_LOC=basic_sim/test
    shift
    ;;
    -v3)
    VM_NAME=ci-phy-sim
    RUN_OPTIONS="./run_exec_autotests.bash -g \"01510*\" -q -np -b"
    ARCHIVES_LOC=phy_sim/test
    shift
    ;;
    -v4)
    VM_NAME=ci-cppcheck
    shift
    ;;
    -v7)
    VM_NAME=ci-enb-ethernet
    shift
    ;;
    -v8)
    VM_NAME=ci-ue-ethernet
    shift
    ;;
    --variant)
    variant="$2"
    case $variant in
        enb-usrp)
        VM_NAME=ci-enb-usrp
        ;;
        basic-sim)
        VM_NAME=ci-basic-sim
        RUN_OPTIONS="complex"
        ARCHIVES_LOC=basic_sim/test
        ;;
        phy-sim)
        VM_NAME=ci-phy-sim
        RUN_OPTIONS="./run_exec_autotests.bash -g \"01510*\" -q -np -b"
        ARCHIVES_LOC=phy_sim/test
        ;;
        cppcheck)
        VM_NAME=ci-cppcheck
        ;;
        enb-ethernet)
        VM_NAME=ci-enb-ethernet
        ;;
        ue-ethernet)
        VM_NAME=ci-ue-ethernet
        ;;
        *)
        echo ""
        echo "Syntax Error: Invalid Variant option -> $variant"
        echo ""
        variant_usage
        exit 1
    esac
    shift
    shift
    ;;
    *)
    echo "Syntax Error: unknown option: $key"
    echo ""
    usage
    exit 1
esac
done

if [ "$JOB_NAME" == "XX" ] || [ "$BUILD_ID" == "XX" ]
then
    VM_TEMPLATE=ci-
else
    VM_TEMPLATE=${JOB_NAME}-b${BUILD_ID}-
fi

VM_NAME=`echo $VM_NAME | sed -e "s#ci-#$VM_TEMPLATE#"`
VM_CMDS=${VM_NAME}_cmds.txt
ARCHIVES_LOC=${JENKINS_WKSP}/archives/${ARCHIVES_LOC}

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
        echo "STATUS seems OK"
    else
        echo "STATUS failed?"
    fi

fi

if [[ "$RUN_OPTIONS" == "complex" ]] && [[ $VM_NAME =~ .*-basic-sim.* ]]
then
    if [ -d $ARCHIVES_LOC ]
    then
        rm -Rf $ARCHIVES_LOC
    fi
    mkdir --parents $ARCHIVES_LOC

    EPC_VM_NAME=`echo $VM_NAME | sed -e "s#basic-sim#epc#"`
    LTEBOX=0
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
    EPC_VM_CMDS=`echo $VM_CMDS | sed -e "s#cmds#epc-cmds#"`
    echo "EPC_VM_CMD_FILE     = $EPC_VM_CMDS"
    IS_EPC_VM_ALIVE=`uvt-kvm list | grep -c $EPC_VM_NAME`
    if [ $IS_EPC_VM_ALIVE -eq 0 ]
    then
        echo "############################################################"
        echo "Creating test EPC VM ($EPC_VM_NAME) on Ubuntu Cloud Image base"
        echo "############################################################"
        uvt-kvm create $EPC_VM_NAME release=xenial --unsafe-caching
    fi

    uvt-kvm wait $EPC_VM_NAME --insecure
    EPC_VM_IP_ADDR=`uvt-kvm ip $EPC_VM_NAME`
    echo "$EPC_VM_NAME has for IP addr = $EPC_VM_IP_ADDR"
    scp -o StrictHostKeyChecking=no /etc/apt/apt.conf.d/01proxy ubuntu@$EPC_VM_IP_ADDR:/home/ubuntu

    # ltebox specific actions (install and start)
    if [ $LTEBOX -eq 1 ]
    then
        echo "############################################################"
        echo "Copying ltebox archives into EPC VM ($EPC_VM_NAME)" 
        echo "############################################################"
        scp -o StrictHostKeyChecking=no /opt/ltebox-archives/ltebox_2.2.70_16_04_amd64.deb ubuntu@$EPC_VM_IP_ADDR:/home/ubuntu
        scp -o StrictHostKeyChecking=no /opt/ltebox-archives/etc-conf.zip ubuntu@$EPC_VM_IP_ADDR:/home/ubuntu
        scp -o StrictHostKeyChecking=no /opt/ltebox-archives/hss-sim.zip ubuntu@$EPC_VM_IP_ADDR:/home/ubuntu

        echo "############################################################"
        echo "Running install and start EPC on EPC VM ($EPC_VM_NAME)"
        echo "############################################################"
        echo "sudo cp 01proxy /etc/apt/apt.conf.d/" > $EPC_VM_CMDS
        echo "touch /home/ubuntu/.hushlogin" >> $EPC_VM_CMDS
        echo "echo \"sudo apt-get --yes --quiet install zip openjdk-8-jre libconfuse-dev libreadline-dev liblog4c-dev libgcrypt-dev libsctp-dev python2.7 python2.7-dev daemon iperf\"" >> $EPC_VM_CMDS
        echo "sudo apt-get update > zip-install.txt 2>&1" >> $EPC_VM_CMDS
        echo "sudo apt-get --yes install zip openjdk-8-jre libconfuse-dev libreadline-dev liblog4c-dev libgcrypt-dev libsctp-dev python2.7 python2.7-dev daemon iperf >> zip-install.txt 2>&1" >> $EPC_VM_CMDS

        # Installing and Starting HSS
        echo "echo \"cd /opt\"" >> $EPC_VM_CMDS
        echo "cd /opt" >> $EPC_VM_CMDS
        echo "echo \"sudo unzip -qq /home/ubuntu/hss-sim.zip\"" >> $EPC_VM_CMDS
        echo "sudo unzip -qq /home/ubuntu/hss-sim.zip" >> $EPC_VM_CMDS
        echo "echo \"cd /opt/hss_sim0609\"" >> $EPC_VM_CMDS
        echo "cd /opt/hss_sim0609" >> $EPC_VM_CMDS

        echo "echo \"sudo daemon --unsafe --name=simulated_hss --chdir=/opt/hss_sim0609 ./starthss_real\"" >> $EPC_VM_CMDS
        echo "sudo daemon --unsafe --name=simulated_hss --chdir=/opt/hss_sim0609 ./starthss_real" >> $EPC_VM_CMDS

        # Installing and Starting ltebox
        echo "echo \"cd /home/ubuntu\"" >> $EPC_VM_CMDS
        echo "cd /home/ubuntu" >> $EPC_VM_CMDS
        echo "echo \"sudo dpkg -i ltebox_2.2.70_16_04_amd64.deb \"" >> $EPC_VM_CMDS
        echo "sudo dpkg -i ltebox_2.2.70_16_04_amd64.deb >> zip-install.txt 2>&1" >> $EPC_VM_CMDS

        echo "echo \"cd /opt/ltebox/etc/\"" >> $EPC_VM_CMDS
        echo "cd /opt/ltebox/etc/" >> $EPC_VM_CMDS
        echo "echo \"sudo unzip -qq -o /home/ubuntu/etc-conf.zip\"" >> $EPC_VM_CMDS
        echo "sudo unzip -qq -o /home/ubuntu/etc-conf.zip" >> $EPC_VM_CMDS
        echo "sudo sed -i  -e 's#EPC_VM_IP_ADDRESS#$EPC_VM_IP_ADDR#' gw.conf" >> $EPC_VM_CMDS
        echo "sudo sed -i  -e 's#EPC_VM_IP_ADDRESS#$EPC_VM_IP_ADDR#' mme.conf" >> $EPC_VM_CMDS

        echo "echo \"cd /opt/ltebox/tools/\"" >> $EPC_VM_CMDS
        echo "cd /opt/ltebox/tools/" >> $EPC_VM_CMDS
        echo "echo \"sudo ./start_ltebox\"" >> $EPC_VM_CMDS
        echo "nohup sudo ./start_ltebox > /home/ubuntu/ltebox.txt" >> $EPC_VM_CMDS

        ssh -o StrictHostKeyChecking=no ubuntu@$EPC_VM_IP_ADDR < $EPC_VM_CMDS
        rm -f $EPC_VM_CMDS

        # We may have some adaptation to do
        if [ -f /opt/ltebox-archives/adapt_ue_sim.txt ]
        then
            echo "############################################################"
            echo "Doing some adaptation on UE side"
            echo "############################################################"
            ssh -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR < /opt/ltebox-archives/adapt_ue_sim.txt
        fi
    fi

    # HERE ADD any install actions for another EPC

    # Retrieve EPC real IP address
    if [ $LTEBOX -eq 1 ]
    then
        # in our configuration file, we are using pool 5
        echo "ifconfig tun5 | egrep \"inet addr\" | sed -e 's#^.*inet addr:##' -e 's#  P-t-P:.*\$##'" > $EPC_VM_CMDS
        REAL_EPC_IP_ADDR=`ssh -o StrictHostKeyChecking=no ubuntu@$EPC_VM_IP_ADDR < $EPC_VM_CMDS`
        echo "EPC IP Address     is : $REAL_EPC_IP_ADDR"
        rm $EPC_VM_CMDS
    fi

    echo "############################################################"
    echo "Starting the eNB"
    echo "############################################################"
    echo "cd /home/ubuntu/tmp" > $VM_CMDS
    echo "echo \"sudo apt-get --yes --quiet install daemon \"" >> $VM_CMDS
    echo "sudo apt-get --yes install daemon >> /home/ubuntu/tmp/cmake_targets/log/daemon-install.txt 2>&1" >> $VM_CMDS
    echo "echo \"export ENODEB=1\"" >> $VM_CMDS
    echo "export ENODEB=1" >> $VM_CMDS
    echo "echo \"source oaienv\"" >> $VM_CMDS
    echo "source oaienv" >> $VM_CMDS
    echo "cd ci-scripts/conf_files/" >> $VM_CMDS
    echo "cp lte-basic-sim.conf ci-lte-basic-sim.conf" >> $VM_CMDS
    echo "sed -i -e 's#CI_MME_IP_ADDR#$EPC_VM_IP_ADDR#' -e 's#CI_ENB_IP_ADDR#$VM_IP_ADDR#' ci-lte-basic-sim.conf" >> $VM_CMDS
    echo "echo \"cd /home/ubuntu/tmp/cmake_targets/basic_simulator/enb/\"" >> $VM_CMDS
    echo "cd /home/ubuntu/tmp/cmake_targets/basic_simulator/enb/" >> $VM_CMDS
    echo "echo \"./lte-softmodem -O /home/ubuntu/tmp/ci-scripts/conf_files/ci-lte-basic-sim.conf\" > ./my-lte-softmodem-run.sh " >> $VM_CMDS
    echo "chmod 775 ./my-lte-softmodem-run.sh" >> $VM_CMDS
    echo "cat ./my-lte-softmodem-run.sh" >> $VM_CMDS
    echo "sudo -E daemon --inherit --unsafe --name=enb_daemon --chdir=/home/ubuntu/tmp/cmake_targets/basic_simulator/enb -o /home/ubuntu/tmp/cmake_targets/log/enb.log ./my-lte-softmodem-run.sh" >> $VM_CMDS

    ssh -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR < $VM_CMDS

    echo "############################################################"
    echo "Starting the UE"
    echo "############################################################"
    echo "echo \"cd /home/ubuntu/tmp/cmake_targets/basic_simulator/ue\"" > $VM_CMDS
    echo "cd /home/ubuntu/tmp/cmake_targets/basic_simulator/ue" > $VM_CMDS
    echo "echo \"./lte-uesoftmodem -C 2680000000 -r 25 --ue-rxgain 140\" > ./my-lte-uesoftmodem-run.sh" >> $VM_CMDS
    echo "chmod 775 ./my-lte-uesoftmodem-run.sh" >> $VM_CMDS
    echo "cat ./my-lte-uesoftmodem-run.sh" >> $VM_CMDS
    echo "sudo -E daemon --inherit --unsafe --name=ue_daemon --chdir=/home/ubuntu/tmp/cmake_targets/basic_simulator/ue -o /home/ubuntu/tmp/cmake_targets/log/ue.log ./my-lte-uesoftmodem-run.sh" >> $VM_CMDS

    ssh -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR < $VM_CMDS
    rm $VM_CMDS
    sleep 10
    echo "ifconfig oip1 | egrep \"inet addr\" | sed -e 's#^.*inet addr:##' -e 's#  P-t-P:.*\$##'" > $VM_CMDS
    UE_IP_ADDR=`ssh -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR < $VM_CMDS`
    echo "UE IP Address for EPC is : $UE_IP_ADDR"
    rm $VM_CMDS

    echo "############################################################"
    echo "Pinging the UE"
    echo "############################################################"
    echo "echo \"ping -c 20 $UE_IP_ADDR\"" > $EPC_VM_CMDS
    echo "ping -c 20 $UE_IP_ADDR | tee -a ping_ue.txt" >> $EPC_VM_CMDS
    ssh -o StrictHostKeyChecking=no ubuntu@$EPC_VM_IP_ADDR < $EPC_VM_CMDS
    rm -f $EPC_VM_CMDS
    scp -o StrictHostKeyChecking=no ubuntu@$EPC_VM_IP_ADDR:/home/ubuntu/ping_ue.txt $ARCHIVES_LOC

    echo "############################################################"
    echo "Iperf DL"
    echo "############################################################"
    echo "echo \"iperf -u -s -i 1\"" > $VM_CMDS
    echo "nohup iperf -u -s -i 1 > tmp/cmake_targets/log/iperf_dl_server.txt &" >> $VM_CMDS
    ssh -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR < $VM_CMDS
    rm $VM_CMDS

    echo "echo \"iperf -c $UE_IP_ADDR -u -t 30 -b 15M -i 1\"" > $EPC_VM_CMDS
    echo "iperf -c $UE_IP_ADDR -u -t 30 -b 15M -i 1 | tee -a iperf_dl_client.txt" >> $EPC_VM_CMDS
    ssh -o StrictHostKeyChecking=no ubuntu@$EPC_VM_IP_ADDR < $EPC_VM_CMDS
    rm -f $EPC_VM_CMDS

    echo "killall --signal SIGKILL iperf" >> $VM_CMDS
    ssh -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR < $VM_CMDS
    rm $VM_CMDS
    scp -o StrictHostKeyChecking=no ubuntu@$EPC_VM_IP_ADDR:/home/ubuntu/iperf_dl_client.txt $ARCHIVES_LOC
    scp -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/iperf_dl_server.txt $ARCHIVES_LOC

    echo "############################################################"
    echo "Iperf UL"
    echo "############################################################"
    echo "echo \"iperf -u -s -i 1\"" > $EPC_VM_CMDS
    echo "nohup iperf -u -s -i 1 > iperf_ul_server.txt &" >> $EPC_VM_CMDS
    ssh -o StrictHostKeyChecking=no ubuntu@$EPC_VM_IP_ADDR < $EPC_VM_CMDS
    rm $EPC_VM_CMDS

    echo "echo \"iperf -c $REAL_EPC_IP_ADDR -u -t 30 -b 4M -i 1\"" > $VM_CMDS
    echo "iperf -c $REAL_EPC_IP_ADDR -u -t 30 -b 4M -i 1 | tee -a /home/ubuntu/tmp/cmake_targets/log/iperf_ul_client.txt" >> $VM_CMDS
    ssh -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR < $VM_CMDS
    rm -f $VM_CMDS

    echo "killall --signal SIGKILL iperf" >> $EPC_VM_CMDS
    ssh -o StrictHostKeyChecking=no ubuntu@$EPC_VM_IP_ADDR < $EPC_VM_CMDS
    rm $EPC_VM_CMDS
    scp -o StrictHostKeyChecking=no ubuntu@$EPC_VM_IP_ADDR:/home/ubuntu/iperf_ul_server.txt $ARCHIVES_LOC
    scp -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/iperf_ul_client.txt $ARCHIVES_LOC

    echo "############################################################"
    echo "Terminate enb/ue simulators"
    echo "############################################################"
    echo "echo \"sudo daemon --name=enb_daemon --stop\"" > $VM_CMDS
    echo "sudo daemon --name=enb_daemon --stop" >> $VM_CMDS
    echo "echo \"sudo daemon --name=ue_daemon --stop\"" >> $VM_CMDS
    echo "sudo daemon --name=ue_daemon --stop" >> $VM_CMDS
    echo "echo \"sudo killall --signal SIGKILL lte-softmodem\"" >> $VM_CMDS
    echo "sudo killall --signal SIGKILL lte-softmodem" >> $VM_CMDS
    ssh -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR < $VM_CMDS
    rm -f $VM_CMDS
    scp -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/enb.log $ARCHIVES_LOC
    scp -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/ue.log $ARCHIVES_LOC
fi

exit $STATUS
