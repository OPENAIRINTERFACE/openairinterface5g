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

function destroy_usage {
    echo "OAI CI VM script"
    echo "   Original Author: Raphael Defosseux"
    echo "   Requirements:"
    echo "     -- uvtool uvtool-libvirt apt-cacher"
    echo ""
    echo "Usage:"
    echo "------"
    echo "    oai-ci-vm-tool destroy [OPTIONS]"
    echo ""
    command_options_usage
}

function destroy_vm {
    echo "############################################################"
    echo "OAI CI VM script"
    echo "############################################################"
    echo "VM_TEMPLATE         = $VM_TEMPLATE"

    LIST_CI_VM=`uvt-kvm list | grep $VM_TEMPLATE`

    for CI_VM in $LIST_CI_VM
    do
        VM_IP_ADDR=`uvt-kvm ip $CI_VM`
        echo "VM to destroy: $CI_VM -- IP $VM_IP_ADDR"
        uvt-kvm destroy $CI_VM
        ssh-keygen -R $VM_IP_ADDR
    done
}
