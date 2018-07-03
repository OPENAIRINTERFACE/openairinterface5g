#!/bin/bash

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
    shift
    ;;
    -v3)
    VM_NAME=ci-phy-sim
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

echo "VM_NAME             = $VM_NAME"

echo "############################################################"
echo "Creating VM ($VM_NAME) on Ubuntu Cloud Image base"
echo "############################################################"
uvt-kvm create $VM_NAME release=xenial --memory 2048 --cpu 4 --unsafe-caching --template ci-scripts/template-host.xml
echo "Waiting for VM to be started"
uvt-kvm wait $VM_NAME --insecure

VM_IP_ADDR=`uvt-kvm ip $VM_NAME`
echo "$VM_NAME has for IP addr = $VM_IP_ADDR"
exit 0
