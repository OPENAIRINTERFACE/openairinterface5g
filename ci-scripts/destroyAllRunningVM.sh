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
    echo "OAI VM Destroy script"
    echo "   Original Author: Raphael Defosseux"
    echo ""
    echo "Usage:"
    echo "------"
    echo "    destroyAllRunningVM.sh [OPTIONS]"
    echo ""
    echo "Options:"
    echo "--------"
    echo "    --job-name #### OR -jn ####"
    echo "    Specify the name of the Jenkins job."
    echo ""
    echo "    --build-id #### OR -id ####"
    echo "    Specify the build ID of the Jenkins job."
    echo ""
    echo "    --help OR -h"
    echo "    Print this help message."
    echo ""
}

if [ $# -gt 4 ]
then
    echo "Syntax Error: not the correct number of arguments"
    echo ""
    usage
    exit 1
fi

VM_TEMPLATE=ci-
JOB_NAME=XX
BUILD_ID=XX

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

LIST_CI_VM=`uvt-kvm list | grep $VM_TEMPLATE`

for CI_VM in $LIST_CI_VM
do
    VM_IP_ADDR=`uvt-kvm ip $CI_VM`
    echo "VM to destroy: $CI_VM -- IP $VM_IP_ADDR"
    uvt-kvm destroy $CI_VM
    ssh-keygen -R $VM_IP_ADDR
done

exit 0

