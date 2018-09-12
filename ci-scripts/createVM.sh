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
    echo "OAI VM Creation script"
    echo "   Original Author: Raphael Defosseux"
    echo "   Requirements:"
    echo "     -- uvtool uvtool-libvirt apt-cacher"
    echo "     -- xenial image already synced"
    echo "   Default:"
    echo "     -- eNB with USRP"
    echo ""
    echo "Usage:"
    echo "------"
    echo "    createVM.sh [OPTIONS]"
    echo ""
    echo "Options:"
    echo "--------"
    echo "    --job-name #### OR -jn ####"
    echo "    Specify the name of the Jenkins job."
    echo ""
    echo "    --build-id #### OR -id ####"
    echo "    Specify the build ID of the Jenkins job."
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

if [ $# -lt 1 ] || [ $# -gt 6 ]
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
VM_CPU=4

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
    -v1)
    VM_NAME=ci-enb-usrp
    shift
    ;;
    -v2)
    VM_NAME=ci-basic-sim
    VM_MEMORY=8192
    shift
    ;;
    -v3)
    VM_NAME=ci-phy-sim
    shift
    ;;
    -v4)
    VM_NAME=ci-cppcheck
    VM_MEMORY=8192
    shift
    ;;
    -v5)
    VM_NAME=ci-gnb-usrp
    shift
    ;;
    -v6)
    VM_NAME=ci-ue-nr-usrp
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
        VM_MEMORY=8192
        ;;
        phy-sim)
        VM_NAME=ci-phy-sim
        ;;
        cppcheck)
        VM_NAME=ci-cppcheck
        VM_MEMORY=8192
        ;;
        gnb-usrp)
        VM_NAME=ci-gnb-usrp
        ;;
        nu-ue-usrp)
        VM_NAME=ci-ue-nr-usrp
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

echo "VM_NAME             = $VM_NAME"
echo "VM_MEMORY           = $VM_MEMORY MBytes"
echo "VM_CPU              = $VM_CPU"

echo "############################################################"
echo "Creating VM ($VM_NAME) on Ubuntu Cloud Image base"
echo "############################################################"
uvt-kvm create $VM_NAME release=xenial --memory $VM_MEMORY --cpu $VM_CPU --unsafe-caching --template ci-scripts/template-host.xml
echo "Waiting for VM to be started"
uvt-kvm wait $VM_NAME --insecure

VM_IP_ADDR=`uvt-kvm ip $VM_NAME`
echo "$VM_NAME has for IP addr = $VM_IP_ADDR"
exit 0
