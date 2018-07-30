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
else

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
    echo "Checking build status"
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

exit $STATUS
