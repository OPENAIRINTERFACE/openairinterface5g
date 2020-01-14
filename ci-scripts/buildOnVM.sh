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

function build_usage {
    echo "OAI CI VM script"
    echo "   Original Author: Raphael Defosseux"
    echo "   Requirements:"
    echo "     -- uvtool uvtool-libvirt apt-cacher"
    echo "     -- $VM_OSREL image already synced"
    echo "   Default:"
    echo "     -- eNB with USRP"
    echo ""
    echo "Usage:"
    echo "------"
    echo "    oai-ci-vm-tool build [OPTIONS]"
    echo ""
    command_options_usage

}

function build_on_vm {
    if [ ! -f $JENKINS_WKSP/localZip.zip ]
    then
        echo "Missing localZip.zip file!"
        STATUS=1
        return
    fi
    if [[ ! -f /etc/apt/apt.conf.d/01proxy ]] && [[ "$OPTIONAL_APTCACHER" != "true" ]]
    then
        echo "Missing /etc/apt/apt.conf.d/01proxy file!"
        echo "Is apt-cacher installed and configured?"
        STATUS=1
        return
    fi

    echo "############################################################"
    echo "OAI CI VM script"
    echo "############################################################"

    echo "VM_NAME             = $VM_NAME"
    echo "VM_CMD_FILE         = $VM_CMDS"
    echo "JENKINS_WKSP        = $JENKINS_WKSP"
    echo "ARCHIVES_LOC        = $ARCHIVES_LOC"
    echo "BUILD_OPTIONS       = $BUILD_OPTIONS"

    IS_VM_ALIVE=`uvt-kvm list | grep -c $VM_NAME`

    if [ $IS_VM_ALIVE -eq 0 ]
    then
        echo "VM_MEMORY           = $VM_MEMORY MBytes"
        echo "VM_CPU              = $VM_CPU"
        echo "############################################################"
        echo "Creating VM ($VM_NAME) on Ubuntu Cloud Image base"
        echo "############################################################"
        acquire_vm_create_lock
        uvt-kvm create $VM_NAME release=$VM_OSREL --memory $VM_MEMORY --cpu $VM_CPU --unsafe-caching --template ci-scripts/template-host.xml
        echo "Waiting for VM to be started"
        uvt-kvm wait $VM_NAME --insecure

        VM_IP_ADDR=`uvt-kvm ip $VM_NAME`
        echo "$VM_NAME has for IP addr = $VM_IP_ADDR"
        release_vm_create_lock
    else
        echo "Waiting for VM to be started"
        uvt-kvm wait $VM_NAME --insecure

        VM_IP_ADDR=`uvt-kvm ip $VM_NAME`
        echo "$VM_NAME has for IP addr = $VM_IP_ADDR"
    fi

    echo "############################################################"
    echo "Copying GIT repo into VM ($VM_NAME)"
    echo "############################################################"
    if [[ "$VM_NAME" == *"-flexran-rtc"* ]]
    then
        scp -o StrictHostKeyChecking=no $JENKINS_WKSP/flexran/flexran.zip ubuntu@$VM_IP_ADDR:/home/ubuntu/localZip.zip
    else
        scp -o StrictHostKeyChecking=no $JENKINS_WKSP/localZip.zip ubuntu@$VM_IP_ADDR:/home/ubuntu
    fi
    [ -f /etc/apt/apt.conf.d/01proxy ] && scp -o StrictHostKeyChecking=no /etc/apt/apt.conf.d/01proxy ubuntu@$VM_IP_ADDR:/home/ubuntu

    echo "############################################################"
    echo "Running install and build script on VM ($VM_NAME)"
    echo "############################################################"
    echo "[ -f 01proxy ] && sudo cp 01proxy /etc/apt/apt.conf.d/" > $VM_CMDS
    echo "touch /home/ubuntu/.hushlogin" >> $VM_CMDS
    if [[ "$VM_NAME" == *"-cppcheck"* ]]
    then
        if [ $DAEMON -eq 0 ]
        then
            echo "echo \"sudo apt-get --yes --quiet install zip cppcheck \"" >> $VM_CMDS
            echo "sudo apt-get update > zip-install.txt 2>&1" >> $VM_CMDS
            echo "sudo apt-get --yes install zip cppcheck >> zip-install.txt 2>&1" >> $VM_CMDS
        else
            echo "echo \"sudo apt-get --yes --quiet install zip daemon cppcheck \"" >> $VM_CMDS
            echo "sudo apt-get update > zip-install.txt 2>&1" >> $VM_CMDS
            echo "sudo apt-get --yes install zip daemon cppcheck >> zip-install.txt 2>&1" >> $VM_CMDS
        fi
    fi
    if [[ "$VM_NAME" == *"-flexran-rtc"* ]]
    then
        if [ $DAEMON -eq 0 ]
        then
            echo "echo \"sudo apt-get --yes --quiet install zip curl jq \"" >> $VM_CMDS
            echo "sudo apt-get update > zip-install.txt 2>&1" >> $VM_CMDS
            echo "sudo apt-get --yes install zip curl jq >> zip-install.txt 2>&1" >> $VM_CMDS
        else
            echo "echo \"sudo apt-get --yes --quiet install zip daemon curl jq \"" >> $VM_CMDS
            echo "sudo apt-get update > zip-install.txt 2>&1" >> $VM_CMDS
            echo "sudo apt-get --yes install zip daemon curl jq >> zip-install.txt 2>&1" >> $VM_CMDS
        fi
    fi
    if [[ "$VM_NAME" != *"-cppcheck"* ]] && [[ "$VM_NAME" != *"-flexran-rtc"* ]]
    then
        if [ $DAEMON -eq 0 ]
        then
            echo "echo \"sudo apt-get --yes --quiet install zip subversion libboost-dev \"" >> $VM_CMDS
            echo "sudo apt-get update > zip-install.txt 2>&1" >> $VM_CMDS
            echo "sudo apt-get --yes install zip subversion libboost-dev >> zip-install.txt 2>&1" >> $VM_CMDS
        else
            echo "echo \"sudo apt-get --yes --quiet install zip daemon subversion libboost-dev \"" >> $VM_CMDS
            echo "sudo apt-get update > zip-install.txt 2>&1" >> $VM_CMDS
            echo "sudo apt-get --yes install zip daemon subversion libboost-dev >> zip-install.txt 2>&1" >> $VM_CMDS
        fi
    fi
    echo "mkdir tmp" >> $VM_CMDS
    echo "cd tmp" >> $VM_CMDS
    echo "echo \"unzip -qq -DD ../localZip.zip\"" >> $VM_CMDS
    echo "unzip -qq -DD ../localZip.zip" >> $VM_CMDS
    if [[ "$VM_NAME" == *"-cppcheck"* ]]
    then
        echo "mkdir cmake_targets/log" >> $VM_CMDS
        echo "chmod 777 cmake_targets/log" >> $VM_CMDS
        echo "cp /home/ubuntu/zip-install.txt cmake_targets/log" >> $VM_CMDS
        echo "echo \"cppcheck $BUILD_OPTIONS . \"" >> $VM_CMDS
        if [ $DAEMON -eq 0 ]
        then
            echo "cppcheck $BUILD_OPTIONS . 2> cmake_targets/log/cppcheck.xml 1> cmake_targets/log/cppcheck_build.txt" >> $VM_CMDS
        else
            echo "echo \"cppcheck $BUILD_OPTIONS .\" > ./my-vm-build.sh" >> $VM_CMDS
            echo "chmod 775 ./my-vm-build.sh " >> $VM_CMDS
            echo "sudo -E daemon --inherit --unsafe --name=build_daemon --chdir=/home/ubuntu/tmp -O /home/ubuntu/tmp/cmake_targets/log/cppcheck_build.txt -E /home/ubuntu/tmp/cmake_targets/log/cppcheck.xml ./my-vm-build.sh" >> $VM_CMDS
        fi
    fi
    if [[ "$VM_NAME" == *"-flexran-rtc"* ]]
    then
        echo "mkdir -p cmake_targets/log" >> $VM_CMDS
        echo "chmod 777 cmake_targets/log" >> $VM_CMDS
        echo "cp /home/ubuntu/zip-install.txt cmake_targets/log" >> $VM_CMDS
        echo "echo \"./tools/install_dependencies \"" >> $VM_CMDS
        echo "./tools/install_dependencies > cmake_targets/log/install-build.txt 2>&1" >> $VM_CMDS
        echo "echo \"mkdir build\"" >> $VM_CMDS
        echo "mkdir build" >> $VM_CMDS
        echo "echo \"cd build\"" >> $VM_CMDS
        echo "cd build" >> $VM_CMDS
        echo "echo \"$BUILD_OPTIONS \"" >> $VM_CMDS
        echo "$BUILD_OPTIONS > ../cmake_targets/log/rt_controller.Rel14.txt 2>&1" >> $VM_CMDS
    fi
    if [[ "$VM_NAME" != *"-cppcheck"* ]] && [[ "$VM_NAME" != *"-flexran-rtc"* ]]
    then
        echo "echo \"source oaienv\"" >> $VM_CMDS
        echo "source oaienv" >> $VM_CMDS
        echo "cd cmake_targets/" >> $VM_CMDS
        echo "mkdir log" >> $VM_CMDS
        echo "chmod 777 log" >> $VM_CMDS
        echo "cp /home/ubuntu/zip-install.txt log" >> $VM_CMDS
        if [ $DAEMON -eq 0 ]
        then
            echo "echo \"./build_oai -I $BUILD_OPTIONS \"" >> $VM_CMDS
            echo "./build_oai -I $BUILD_OPTIONS > log/install-build.txt 2>&1" >> $VM_CMDS
        else
            echo "echo \"./build_oai -I $BUILD_OPTIONS\" > ./my-vm-build.sh" >> $VM_CMDS
            echo "chmod 775 ./my-vm-build.sh " >> $VM_CMDS
            echo "echo \"sudo -E daemon --inherit --unsafe --name=build_daemon --chdir=/home/ubuntu/tmp/cmake_targets -o /home/ubuntu/tmp/cmake_targets/log/install-build.txt ./my-vm-build.sh\"" >> $VM_CMDS
            echo "sudo -E daemon --inherit --unsafe --name=build_daemon --chdir=/home/ubuntu/tmp/cmake_targets -o /home/ubuntu/tmp/cmake_targets/log/install-build.txt ./my-vm-build.sh" >> $VM_CMDS
        fi
    fi
    ssh -T -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR < $VM_CMDS
    rm -f $VM_CMDS
}
