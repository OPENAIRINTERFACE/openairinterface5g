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
    echo "OAI RedHat Build Check script"
    echo "   Original Author: Raphael Defosseux"

    echo ""
    echo "Usage:"
    echo "------"
    echo "    buildOnRH.sh [OPTIONS]"
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
    echo "    --remote-host #### OR -rh ####"
    echo "    Specify the RedHat remote server."
    echo ""
    echo "    --remote-user-name #### OR -ru ####"
    echo "    Specify the RedHat remote server username."
    echo ""
    echo "    --remote-password #### OR -rp ####"
    echo "    Specify the RedHat remote server password."
    echo ""
    echo "    --remote-path #### OR -ra ####"
    echo "    Specify the RedHat remote server path to work on."
    echo ""
}

if [ $# -lt 1 ] || [ $# -gt 14 ]
then
    echo "Syntax Error: not the correct number of arguments"
    echo ""
    usage
    exit 1
fi

RH_HOST=XX
RH_USER=XX
RH_PASSWD=XX
RH_PATH=XX
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
    -ws|--workspace)
    JENKINS_WKSP="$2"
    shift
    shift
    ;;
    -rh|--remote-host)
    RH_HOST="$2"
    shift
    shift
    ;;
    -ru|--remote-user-name)
    RH_USER="$2"
    shift
    shift
    ;;
    -rp|--remote-password)
    RH_PASSWD="$2"
    shift
    shift
    ;;
    -ra|--remote-path)
    RH_PATH="$2"
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

if [ "$JOB_NAME" == "XX" ] || [ "$BUILD_ID" == "XX" ] || [ "$RH_HOST" == "XX" ] || [ "$RH_USER" == "XX" ] || [ "$RH_PASSWD" == "XX" ] || [ "$RH_PATH" == "XX" ]
then
    echo "Missing options"
    usage
    exit 1
fi

echo "############################################################"
echo "Copying GIT repo into RedHat Server" 
echo "############################################################"
echo "rm -Rf ${RH_PATH}" >> rh-cmd.txt
echo "mkdir -p ${RH_PATH}" >> rh-cmd.txt

sshpass -p ${RH_PASSWD} ssh -o 'StrictHostKeyChecking no' ${RH_USER}@${RH_HOST} < rh-cmd.txt
rm -f rh-cmd.txt

echo "############################################################"
echo "Running install and build script on RedHat Server"
echo "############################################################"
sshpass -p ${RH_PASSWD} scp -o 'StrictHostKeyChecking no' $JENKINS_WKSP/localZip.zip ${RH_USER}@${RH_HOST}:${RH_PATH}

echo "cd ${RH_PATH}" > rh-cmd.txt
echo "unzip -qq localZip.zip" >> rh-cmd.txt
echo "source oaienv" >> rh-cmd.txt
echo "cd cmake_targets" >> rh-cmd.txt
echo "mkdir -p log" >> rh-cmd.txt
echo "./build_oai -I -w USRP --eNB > log/install-build.txt 2>&1" >> rh-cmd.txt
sshpass -p ${RH_PASSWD} ssh -o 'StrictHostKeyChecking no' ${RH_USER}@${RH_HOST} < rh-cmd.txt

rm -f rh-cmd.txt

echo "############################################################"
echo "Creating a tmp folder to store results and artifacts"
echo "############################################################"
if [ ! -d $JENKINS_WKSP/archives ]
then
    mkdir -p $JENKINS_WKSP/archives
fi

ARCHIVES_LOC=$JENKINS_WKSP/archives/red_hat
if [ ! -d $ARCHIVES_LOC ]
then
    mkdir -p $ARCHIVES_LOC
fi

sshpass -p ${RH_PASSWD} scp -o 'StrictHostKeyChecking no' ${RH_USER}@${RH_HOST}:${RH_PATH}/cmake_targets/log/*.txt $ARCHIVES_LOC

echo "############################################################"
echo "Checking build status" 
echo "############################################################"

LOG_PATTERN=.Rel14.txt
NB_PATTERN_FILES=7

LOG_FILES=`ls $ARCHIVES_LOC/*.txt`
STATUS=0
NB_FOUND_FILES=0

for FULLFILE in $LOG_FILES
do
    if [[ $FULLFILE == *"$LOG_PATTERN"* ]]
    then
        filename=$(basename -- "$FULLFILE")
        PASS_PATTERN=`echo $filename | sed -e "s#$LOG_PATTERN##"`
        LOCAL_STAT=`egrep -c "Built target $PASS_PATTERN" $FULLFILE`
        if [ $LOCAL_STAT -eq 0 ]; then STATUS=-1; fi
        NB_FOUND_FILES=$((NB_FOUND_FILES + 1))
    fi
done

if [ $NB_PATTERN_FILES -ne $NB_FOUND_FILES ]
then
    echo "Expecting $NB_PATTERN_FILES log files and found $NB_FOUND_FILES"
    STATUS=-1
fi

echo "COMMAND: build_oai -I -w USRP --eNB" > $ARCHIVES_LOC/build_final_status.log
if [ $STATUS -eq 0 ]
then
    echo "BUILD_OK" >> $ARCHIVES_LOC/build_final_status.log
    echo "STATUS seems OK"
else
    echo "BUILD_KO" >> $ARCHIVES_LOC/build_final_status.log
    echo "STATUS failed?"
fi
exit $STATUS
