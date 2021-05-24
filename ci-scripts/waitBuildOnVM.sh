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

function wait_usage {
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
    echo "    oai-ci-vm-tool wait [OPTIONS]"
    echo ""
    command_options_usage
}

function wait_on_vm_build {
    echo "############################################################"
    echo "OAI CI VM script"
    echo "############################################################"

    echo "VM_NAME             = $VM_NAME"
    echo "VM_CMD_FILE         = $VM_CMDS"
    echo "JENKINS_WKSP        = $JENKINS_WKSP"
    echo "ARCHIVES_LOC        = $ARCHIVES_LOC"
    echo "BUILD_OPTIONS       = $BUILD_OPTIONS"

    if [[ "$VM_NAME" == *"-enb-usrp"* ]] || [[ "$VM_NAME" == *"-cppcheck"* ]] || [[ "$VM_NAME" == *"-phy-sim"* ]]
    then
        echo "This VM type is no longer supported in the pipeline framework"
        return
    fi

    IS_VM_ALIVE=`uvt-kvm list | grep -c $VM_NAME`

    if [ $IS_VM_ALIVE -eq 0 ]
    then
        echo "############################################################"
        echo "You should have created the VM before doing anything"
        echo "############################################################"
        STATUS=1
        return
    fi

    echo "Waiting for VM to be started"
    uvt-kvm wait $VM_NAME --insecure

    VM_IP_ADDR=`uvt-kvm ip $VM_NAME`
    echo "$VM_NAME has for IP addr = $VM_IP_ADDR"

    echo "############################################################"
    echo "Waiting build process to end on VM ($VM_NAME)"
    echo "############################################################"
    # Since the last VM was cppcheck and is removed
    # we are going too fast in wait and the build_oai is not yet started
    sleep 120

    if [[ "$VM_NAME" == *"-cppcheck"* ]]
    then
        echo "echo \"ps -aux | grep cppcheck \"" >> $VM_CMDS
        echo "while [ \$(ps -aux | grep --color=never cppcheck | grep -v grep | wc -l) -gt 0 ]; do sleep 3; done" >> $VM_CMDS
    else
        echo "echo \"ps -aux | grep build \"" >> $VM_CMDS
        echo "while [ \$(ps -aux | grep --color=never build_oai | grep -v grep | wc -l) -gt 0 ]; do sleep 3; done" >> $VM_CMDS
    fi
    echo "echo \"df -h\"" >> $VM_CMDS
    echo "df -h" >> $VM_CMDS

    ssh -T -o StrictHostKeyChecking=no ubuntu@$VM_IP_ADDR < $VM_CMDS
    rm -f $VM_CMDS
}

function check_on_vm_build {
    if [[ "$VM_NAME" == *"-enb-usrp"* ]] || [[ "$VM_NAME" == *"-cppcheck"* ]] || [[ "$VM_NAME" == *"-phy-sim"* ]]
    then
        echo "This VM type is no longer supported in the pipeline framework"
        return
    fi

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
      if [[ "$VM_NAME" == *"-enb-ethernet"* ]] || [[ "$VM_NAME" == *"-ue-ethernet"* ]]
      then
        echo "Hack to not destroy in current pipeline"
      else
        echo "############################################################"
        echo "Destroying VM"
        echo "############################################################"
        uvt-kvm destroy $VM_NAME
        ssh-keygen -R $VM_IP_ADDR
      fi
    fi
    rm -f $VM_CMDS

    echo "############################################################"
    echo "Checking build status" 
    echo "############################################################"

    if [[ "$VM_NAME" == *"-cppcheck"* ]]
    then
        LOG_FILES=`ls $ARCHIVES_LOC/*.txt $ARCHIVES_LOC/*.xml`
    else
        LOG_FILES=`ls $ARCHIVES_LOC/*.txt`
    fi
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

    if [ $NB_PATTERN_FILES -ne $NB_FOUND_FILES ]
    then
        echo "Expecting $NB_PATTERN_FILES log files and found $NB_FOUND_FILES"
        STATUS=-1
    fi

    # If we were building the FlexRan Controller, flag-touch for basic-simulator to continue
    if [[ "$VM_NAME" == *"-flexran-rtc"* ]]
    then
        if [[ $STATUS -eq 0 ]]
        then
            touch $JENKINS_WKSP/flexran/flexran_build_complete.txt
        fi
    fi

    if [[ "$VM_NAME" == *"-cppcheck"* ]]
    then
        echo "COMMAND: cppcheck $BUILD_OPTIONS . 2> cppcheck.xml" > $ARCHIVES_LOC/build_final_status.log
    elif [[ "$VM_NAME" == *"-flexran-rtc"* ]]
    then
        echo "COMMAND: $BUILD_OPTIONS" > $ARCHIVES_LOC/build_final_status.log
    else
        echo "COMMAND: build_oai -I $BUILD_OPTIONS" > $ARCHIVES_LOC/build_final_status.log
    fi
    if [[ $STATUS -eq 0 ]]
    then
        echo "BUILD_OK" >> $ARCHIVES_LOC/build_final_status.log
    else
        echo "BUILD_KO" >> $ARCHIVES_LOC/build_final_status.log
    fi
}
