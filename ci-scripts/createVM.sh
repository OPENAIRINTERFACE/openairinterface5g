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

function create_usage {
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
    echo "    oai-ci-vm-tool create [OPTIONS]"
    echo ""
    echo "Mandatory Options:"
    echo "--------"
    echo "    --job-name #### OR -jn ####"
    echo "    Specify the name of the Jenkins job."
    echo ""
    echo "    --build-id #### OR -id ####"
    echo "    Specify the build ID of the Jenkins job."
    echo ""
    variant_usage
    echo "    Specify the variant to build."
    echo ""
    echo "    --help OR -h"
    echo "    Print this help message."
    echo ""
}

function create_vm {
    echo "############################################################"
    echo "OAI CI VM script"
    echo "############################################################"
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
}
