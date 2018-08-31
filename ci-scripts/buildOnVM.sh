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
    echo "    --variant gnb-usrp     OR -v5"
    echo "    --variant nu-ue-usrp   OR -v6"
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
    echo "OAI VM Build Check script"
    echo "   Original Author: Raphael Defosseux"
    echo ""
    echo "    --variant enb-usrp     OR -v1"
    echo "    --variant basic-sim    OR -v2"
    echo "    --variant phy-sim      OR -v3"
    echo "    --variant cppcheck     OR -v4"
    echo "    --variant gnb-usrp     OR -v5"
    echo "    --variant nu-ue-usrp   OR -v6"
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
VM_MEMORY=2048
ARCHIVES_LOC=enb_usrp
LOG_PATTERN=.Rel15.txt
NB_PATTERN_FILES=4
BUILD_OPTIONS="--eNB -w USRP"
KEEP_VM_ALIVE=0

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
    ARCHIVES_LOC=enb_usrp
    LOG_PATTERN=.Rel15.txt
    NB_PATTERN_FILES=4
    BUILD_OPTIONS="--eNB -w USRP"
    shift
    ;;
    -v2)
    VM_NAME=ci-basic-sim
    ARCHIVES_LOC=basic_sim
    LOG_PATTERN=basic_simulator
    NB_PATTERN_FILES=2
    BUILD_OPTIONS="--basic-simulator"
    shift
    ;;
    -v3)
    VM_NAME=ci-phy-sim
    ARCHIVES_LOC=phy_sim
    LOG_PATTERN=.Rel15.txt
    NB_PATTERN_FILES=5
    BUILD_OPTIONS="--phy_simulators"
    shift
    ;;
    -v4)
    VM_NAME=ci-cppcheck
    VM_MEMORY=8192
    ARCHIVES_LOC=cppcheck
    LOG_PATTERN=cppcheck.xml
    NB_PATTERN_FILES=1
    BUILD_OPTIONS="--enable=warning --force --xml --xml-version=2 -i openair1/PHY/CODING/nrLDPC_decoder/nrLDPC_decoder.c"
    shift
    ;;
    -v5)
    VM_NAME=ci-gnb-usrp
    ARCHIVES_LOC=gnb_usrp
    LOG_PATTERN=.Rel15.txt
    NB_PATTERN_FILES=4
    BUILD_OPTIONS="--gNB -w USRP"
    shift
    ;;
    -v6)
    VM_NAME=ci-ue-nr-usrp
    ARCHIVES_LOC=nrue_usrp
    LOG_PATTERN=.Rel15.txt
    NB_PATTERN_FILES=4
    BUILD_OPTIONS="--nrUE -w USRP"
    shift
    ;;
    -v7)
    VM_NAME=ci-enb-ethernet
    ARCHIVES_LOC=enb_eth
    LOG_PATTERN=.Rel15.txt
    NB_PATTERN_FILES=6
    BUILD_OPTIONS="--eNB -t ETHERNET --noS1"
    shift
    ;;
    -v8)
    VM_NAME=ci-ue-ethernet
    ARCHIVES_LOC=ue_eth
    LOG_PATTERN=.Rel15.txt
    NB_PATTERN_FILES=6
    BUILD_OPTIONS="--UE -t ETHERNET --noS1"
    shift
    ;;
    --variant)
    variant="$2"
    case $variant in
        enb-usrp)
        VM_NAME=ci-enb-usrp
        ARCHIVES_LOC=enb_usrp
        LOG_PATTERN=.Rel15.txt
        NB_PATTERN_FILES=4
        BUILD_OPTIONS="--eNB -w USRP"
        ;;
        basic-sim)
        VM_NAME=ci-basic-sim
        ARCHIVES_LOC=basic_sim
        LOG_PATTERN=basic_simulator
        NB_PATTERN_FILES=2
        BUILD_OPTIONS="--basic-simulator"
        ;;
        phy-sim)
        VM_NAME=ci-phy-sim
        ARCHIVES_LOC=phy_sim
        LOG_PATTERN=.Rel15.txt
        NB_PATTERN_FILES=5
        BUILD_OPTIONS="--phy_simulators"
        ;;
        cppcheck)
        VM_NAME=ci-cppcheck
        VM_MEMORY=8192
        ARCHIVES_LOC=cppcheck
        LOG_PATTERN=cppcheck.xml
        NB_PATTERN_FILES=1
        BUILD_OPTIONS="--enable=warning --force --xml --xml-version=2 -i openair1/PHY/CODING/nrLDPC_decoder/nrLDPC_decoder.c"
        ;;
        gnb-usrp)
        VM_NAME=ci-gnb-usrp
        ARCHIVES_LOC=gnb_usrp
        LOG_PATTERN=.Rel15.txt
        NB_PATTERN_FILES=4
        BUILD_OPTIONS="--gNB -w USRP"
        ;;
        nu-ue-usrp)
        VM_NAME=ci-ue-nr-usrp
        ARCHIVES_LOC=nrue_usrp
        LOG_PATTERN=.Rel15.txt
        NB_PATTERN_FILES=4
        BUILD_OPTIONS="--nrUE -w USRP"
        ;;
        enb-ethernet)
        VM_NAME=ci-enb-ethernet
        ARCHIVES_LOC=enb_eth
        LOG_PATTERN=.Rel15.txt
        NB_PATTERN_FILES=6
        BUILD_OPTIONS="--eNB -t ETHERNET --noS1"
        ;;
        ue-ethernet)
        VM_NAME=ci-ue-ethernet
        ARCHIVES_LOC=ue_eth
        LOG_PATTERN=.Rel15.txt
        NB_PATTERN_FILES=6
        BUILD_OPTIONS="--UE -t ETHERNET --noS1"
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
echo "BUILD_OPTIONS       = $BUILD_OPTIONS"

IS_VM_ALIVE=`uvt-kvm list | grep -c $VM_NAME`

if [ $IS_VM_ALIVE -eq 0 ]
then
    echo "############################################################"
    echo "Creating VM ($VM_NAME) on Ubuntu Cloud Image base"
    echo "############################################################"
    uvt-kvm create $VM_NAME release=xenial --memory $VM_MEMORY --cpu 4 --unsafe-caching --template ci-scripts/template-host.xml
fi

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
if [[ "$VM_NAME" == *"-cppcheck"* ]]
then
    echo "echo \"sudo apt-get --yes --quiet install zip cppcheck \"" >> $VM_CMDS
    echo "sudo apt-get update > zip-install.txt 2>&1" >> $VM_CMDS
    echo "sudo apt-get --yes install zip cppcheck >> zip-install.txt 2>&1" >> $VM_CMDS
else
    echo "echo \"sudo apt-get --yes --quiet install zip subversion libboost-dev \"" >> $VM_CMDS
    echo "sudo apt-get update > zip-install.txt 2>&1" >> $VM_CMDS
    echo "sudo apt-get --yes install zip subversion libboost-dev >> zip-install.txt 2>&1" >> $VM_CMDS
fi
echo "mkdir tmp" >> $VM_CMDS
echo "cd tmp" >> $VM_CMDS
echo "echo \"unzip -qq -DD ../localZip.zip\"" >> $VM_CMDS
echo "unzip -qq -DD ../localZip.zip" >> $VM_CMDS
if [[ "$VM_NAME" == *"-cppcheck"* ]]
then
    echo "mkdir cmake_targets/log" >> $VM_CMDS
    echo "cp /home/ubuntu/zip-install.txt cmake_targets/log" >> $VM_CMDS
    echo "echo \"cppcheck $BUILD_OPTIONS . \"" >> $VM_CMDS
    echo "cppcheck $BUILD_OPTIONS . 2> cmake_targets/log/cppcheck.xml 1> cmake_targets/log/cppcheck_build.txt" >> $VM_CMDS
else
    echo "echo \"source oaienv\"" >> $VM_CMDS
    echo "source oaienv" >> $VM_CMDS
    echo "cd cmake_targets/" >> $VM_CMDS
    echo "mkdir log" >> $VM_CMDS
    echo "cp /home/ubuntu/zip-install.txt log" >> $VM_CMDS
    echo "echo \"./build_oai -I $BUILD_OPTIONS \"" >> $VM_CMDS
    echo "./build_oai -I $BUILD_OPTIONS > log/install-build.txt 2>&1" >> $VM_CMDS
fi
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
if [[ "$VM_NAME" == *"-cppcheck"* ]]
then
    scp -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR:/home/ubuntu/tmp/cmake_targets/log/*.xml $ARCHIVES_LOC
fi

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

LOG_FILES=`ls $ARCHIVES_LOC/*.txt $ARCHIVES_LOC/*.xml`
STATUS=0
NB_FOUND_FILES=0

for FULLFILE in $LOG_FILES 
do
    if [[ $FULLFILE == *"$LOG_PATTERN"* ]]
    then
        filename=$(basename -- "$FULLFILE")
        if [ "$LOG_PATTERN" == ".Rel15.txt" ]
        then
            PASS_PATTERN=`echo $filename | sed -e "s#$LOG_PATTERN##"`
        fi
        if [ "$LOG_PATTERN" == "basic_simulator" ]
        then
            PASS_PATTERN="lte-"
        fi
        if [ "$LOG_PATTERN" == "cppcheck.xml" ]
        then
            PASS_PATTERN="results version"
            LOCAL_STAT=`egrep -c "$PASS_PATTERN" $FULLFILE`
        else
            LOCAL_STAT=`egrep -c "Built target $PASS_PATTERN" $FULLFILE`
        fi
        if [ $LOCAL_STAT -eq 0 ]; then STATUS=-1; fi
        NB_FOUND_FILES=$((NB_FOUND_FILES + 1))
    fi
done

if [ $NB_PATTERN_FILES -ne $NB_FOUND_FILES ]; then STATUS=-1; fi

if [ $STATUS -eq 0 ]
then
    echo "STATUS seems OK"
else
    echo "STATUS failed?"
fi
exit $STATUS
