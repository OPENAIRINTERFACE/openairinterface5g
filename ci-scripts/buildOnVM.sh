#!/bin/bash

function usage {
    echo "OAI VM Build Check script"
    echo "   Original Author: Raphael Defosseux"
    echo "   Requirements:"
    echo "     -- uvtool uvtool-libvirt apt-cacher"
    echo "     -- xenial image already synced"
    echo "   Default:"
    echo "     -- eNB with USRP"
    echo ""
    echo "Usage:"
    echo "------"
    echo "    buildOnVM.sh [OPTIONS]"
    echo ""
    echo "Options:"
    echo "--------"
    echo "    --workspace #### OR -ws ####"
    echo "    Specify the workspace"
    echo ""
    echo "    --variant enb-usrp   OR -v1"
    echo "    --variant basic-sim  OR -v2"
    echo "    Specify the variant to build"
    echo ""
    echo "    --help OR -h"
    echo "    Print this help message."
    echo ""
}

function variant_usage {
    echo "OAI VM Build Check script"
    echo "   Original Author: Raphael Defosseux"
    echo ""
    echo "    --variant enb-usrp   OR -v1"
    echo "    --variant basic-sim  OR -v2"
    echo ""
}

if [ $# -ne 2 ] && [ $# -ne 1 ] && [ $# -ne 4 ] && [ $# -ne 3 ]
then
    echo "Syntax Error: not the correct number of arguments"
    echo ""
    usage
    exit 1
fi

VM_NAME=ci-enb-usrp
ARCHIVES_LOC=enb_usrp
LOG_PATTERN=.Rel14.txt
NB_PATTERN_FILES=4
BUILD_OPTIONS="--eNB -w USRP"
BUILD_EXTRA_OPTIONS="--cflags_processor \"-mssse3 -msse4.1 -mavx2\""

while [[ $# -gt 0 ]]
do
key="$1"

case $key in
    -h|--help)
    shift
    usage
    exit 0
    ;;
    -ws|--workspace)
    JENKINS_WKSP="$2"
    shift
    shift
    ;;
    -v1)
    VM_NAME=ci-enb-usrp
    ARCHIVES_LOC=enb_usrp
    LOG_PATTERN=.Rel14.txt
    NB_PATTERN_FILES=4
    BUILD_OPTIONS="--eNB -w USRP"
    BUILD_EXTRA_OPTIONS="--cflags_processor \"-mssse3 -msse4.1 -mavx2\""
    shift
    ;;
    -v2)
    VM_NAME=ci-basic-sim
    ARCHIVES_LOC=basic_sim
    LOG_PATTERN=basic_simulator
    NB_PATTERN_FILES=2
    BUILD_OPTIONS="--basic-simulator"
    BUILD_EXTRA_OPTIONS="--cflags_processor \"-mssse3 -msse4.1 -mavx2\""
    shift
    ;;
    --variant)
    variant="$2"
    case $variant in
        enb-usrp)
        VM_NAME=ci-enb-usrp
        ARCHIVES_LOC=enb_usrp
        LOG_PATTERN=.Rel14.txt
        NB_PATTERN_FILES=4
        BUILD_OPTIONS="--eNB -w USRP"
        BUILD_EXTRA_OPTIONS="--cflags_processor \"-mssse3 -msse4.1 -mavx2\""
        ;;
        basic-sim)
        VM_NAME=ci-basic-sim
        ARCHIVES_LOC=basic_sim
        LOG_PATTERN=basic_simulator
        NB_PATTERN_FILES=2
        BUILD_OPTIONS="--basic-simulator"
        BUILD_EXTRA_OPTIONS="--cflags_processor \"-mssse3 -msse4.1 -mavx2\""
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

if [ ! -f $JENKINS_WKSP/localZip.zip ]
then
    echo "Missing localZip.zip file!"
    exit 1
fi
if [ ! -f /etc/apt/apt.conf.d/01proxy ]
then
    echo "Missing /etc/apt/apt.conf.d/01proxy file!"
    echo "Is apt-cacher installed and configured?"
    exit 1
fi

VM_CMDS=${VM_NAME}_cmds.txt
ARCHIVES_LOC=${JENKINS_WKSP}/archives/${ARCHIVES_LOC}

echo "VM_NAME             = $VM_NAME"
echo "VM_CMD_FILE         = $VM_CMDS"
echo "JENKINS_WKSP        = $JENKINS_WKSP"
echo "ARCHIVES_LOC        = $ARCHIVES_LOC"
echo "BUILD_OPTIONS       = $BUILD_OPTIONS"
echo "BUILD_EXTRA_OPTIONS = $BUILD_EXTRA_OPTIONS"

echo "############################################################"
echo "Creating VM ($VM_NAME) on Ubuntu Cloud Image base"
echo "############################################################"
uvt-kvm create $VM_NAME release=xenial --memory 2048 --cpu 4 --unsafe-caching --template ci-scripts/template-host.xml
echo "Waiting for VM to be started"
uvt-kvm wait $VM_NAME --insecure

VM_IP_ADDR=`uvt-kvm ip $VM_NAME`
echo "$VM_NAME has for IP addr = $VM_IP_ADDR"

echo "############################################################"
echo "Copying GIT repo into VM ($VM_NAME)" 
echo "############################################################"
scp -o StrictHostKeyChecking=no localZip.zip ubuntu@$VM_IP_ADDR:/home/ubuntu
scp -o StrictHostKeyChecking=no /etc/apt/apt.conf.d/01proxy ubuntu@$VM_IP_ADDR:/home/ubuntu

echo "############################################################"
echo "Running install and build script on VM ($VM_NAME)"
echo "############################################################"
echo "sudo cp 01proxy /etc/apt/apt.conf.d/" > $VM_CMDS
echo "echo \"sudo apt-get --yes --quiet install zip subversion libboost-dev \"" >> $VM_CMDS
echo "sudo apt-get --yes install zip subversion libboost-dev > zip-install.txt 2>&1" >> $VM_CMDS
echo "mkdir tmp" >> $VM_CMDS
echo "cd tmp" >> $VM_CMDS
echo "echo \"unzip -qq ../localZip.zip\"" >> $VM_CMDS
echo "unzip -qq ../localZip.zip" >> $VM_CMDS
echo "echo \"source oaienv\"" >> $VM_CMDS
echo "source oaienv" >> $VM_CMDS
echo "cd cmake_targets/" >> $VM_CMDS
echo "mkdir log" >> $VM_CMDS
echo "cp /home/ubuntu/zip-install.txt log" >> $VM_CMDS
echo "echo \"./build_oai -I $BUILD_OPTIONS \"" >> $VM_CMDS
echo "./build_oai -I $BUILD_OPTIONS $BUILD_EXTRA_OPTIONS > log/install-build.txt 2>&1" >> $VM_CMDS

ssh -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR < $VM_CMDS

echo "############################################################"
echo "Creating a tmp folder to store results and artifacts"
echo "############################################################"
if [ ! -d $JENKINS_WKSP/archives ]
then
    mkdir $JENKINS_WKSP/archives
fi

if [ ! -d $ARCHIVES_LOC ]
then
    mkdir $ARCHIVES_LOC
fi

scp -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/*.txt $ARCHIVES_LOC

echo "############################################################"
echo "Destroying VM"
echo "############################################################"
uvt-kvm destroy $VM_NAME
ssh-keygen -R $VM_IP_ADDR
rm -f $VM_CMDS

echo "############################################################"
echo "Checking build status" 
echo "############################################################"

LOG_FILES=`ls $ARCHIVES_LOC/*.txt`
STATUS=0
NB_FOUND_FILES=0

for FULLFILE in $LOG_FILES 
do
    if [[ $FULLFILE == *"$LOG_PATTERN"* ]]
    then
        filename=$(basename -- "$FULLFILE")
        if [ "$LOG_PATTERN" == ".Rel14.txt" ]
        then
            PASS_PATTERN=`echo $filename | sed -e "s#$LOG_PATTERN##"`
        fi
        if [ "$LOG_PATTERN" == "basic_simulator" ]
        then
            PASS_PATTERN="Built target lte-"
        fi
        LOCAL_STAT=`egrep -c "Built target $PASS_PATTERN" $FULLFILE`
        if [ $LOCAL_STAT -eq 0 ]; then STATUS=-1; fi
        NB_FOUND_FILES=$((NB_FOUND_FILES + 1))
    fi
done

if [ $NB_PATTERN_FILES -ne $NB_FOUND_FILES ]; then STATUS=-1; fi

exit $STATUS
