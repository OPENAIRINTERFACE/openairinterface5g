#!/bin/bash

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

    ssh -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR < $VM_CMDS

    echo "############################################################"
    echo "Creating a tmp folder to store results and artifacts"
    echo "############################################################"

    if [ -d $ARCHIVES_LOC ]
    then
        rm -Rf $ARCHIVES_LOC
    fi
    scp -rf -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/autotests/log $ARCHIVES_LOC

    if [ $KEEP_VM_ALIVE -eq 0 ]
    then
        echo "############################################################"
        echo "Destroying VM"
        echo "############################################################"
        uvt-kvm destroy $VM_NAME
        ssh-keygen -R $VM_IP_ADDR
    fi
    rm -f $VM_CMDS
fi

exit 0
