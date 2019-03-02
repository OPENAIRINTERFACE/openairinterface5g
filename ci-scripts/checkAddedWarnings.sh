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
    echo "OAI Warning Check script"
    echo "   Original Author: Raphael Defosseux"
    echo ""
    echo "Usage:"
    echo "------"
    echo "    checkAddedWarnings.sh [OPTIONS]"
    echo ""
    echo "Options:"
    echo "--------"
    echo "    --src-branch #### OR -sb ####"
    echo "    Specify the source branch of the merge request."
    echo ""
    echo "    --target-branch #### OR -tb ####"
    echo "    Specify the target branch of the merge request (usually develop)."
    echo ""
    echo "    --help OR -h"
    echo "    Print this help message."
    echo ""
}

if [ $# -ne 4 ] && [ $# -ne 1 ]
then
    echo "Syntax Error: not the correct number of arguments"
    echo ""
    usage
    exit 1
fi

checker=0
while [[ $# -gt 0 ]]
do
key="$1"

case $key in
    -h|--help)
    shift
    usage
    exit 0
    ;;
    -sb|--src-branch)
    SOURCE_BRANCH="$2"
    let "checker|=0x1"
    shift
    shift
    ;;
    -tb|--target-branch)
    TARGET_BRANCH="$2"
    let "checker|=0x2"
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


if [ $checker -ne 3 ]
then
    echo "Source Branch is    : $SOURCE_BRANCH"
    echo "Target Branch is    : $TARGET_BRANCH"
    echo ""
    echo "Syntax Error: missing option"
    echo ""
    usage
    exit 1
fi

# Merge request scenario

MERGE_COMMMIT=`git log -n1 --pretty=format:%H`
TARGET_INIT_COMMIT=`cat .git/refs/remotes/origin/$TARGET_BRANCH`

echo " ---- Checking the modified files by the merge request ----"
echo ""
echo "Source Branch is    : $SOURCE_BRANCH"
echo "Target Branch is    : $TARGET_BRANCH"
echo "Merged Commit is    : $MERGE_COMMMIT"
echo "Target Init   is    : $TARGET_INIT_COMMIT"

# Retrieve the list of modified files since the latest develop commit
MODIFIED_FILES=`git log $TARGET_INIT_COMMIT..$MERGE_COMMMIT --oneline --name-status | egrep "^M|^A" | sed -e "s@^M\t*@@" -e "s@^A\t*@@" | sort | uniq`
NB_WARNINGS_FILES=0

# Retrieve list of warnings 
LIST_WARNING_FILES=`egrep "error:|warning:" archives/*/*.Rel14.txt archives/*/basic_simulator_*txt | egrep -v "jobserver unavailable|Clock skew detected.|flexran.proto" | sed -e "s#^.*/home/ubuntu/tmp/##" -e "s#^.*/tmp/CI-eNB/##" -e "s#common/utils/.*/itti#common/utils/itti#" | awk -F ":" '{print $1}' | sort | uniq`

echo ""
echo "List of files that have been modified by the Merge Request AND"
echo "  that have compilation warnings/errors"
echo "--------------------------------------------------------------------"
declare -a ARRAYNAME

for FULLFILE in $MODIFIED_FILES
do
    filename=$(basename -- "$FULLFILE")
    EXT="${filename##*.}"
    if [ $EXT = "c" ] || [ $EXT = "h" ] || [ $EXT = "cpp" ] || [ $EXT = "hpp" ]
    then
        for WARNING_FILE in $LIST_WARNING_FILES
        do
            if [ $FULLFILE = $WARNING_FILE ]
            then
                echo $WARNING_FILE
                ARRAYNAME[$NB_WARNINGS_FILES]=$WARNING_FILE
                NB_WARNINGS_FILES=$((NB_WARNINGS_FILES + 1))
            fi
        done
    fi
done

echo ""
echo "NB Files impacted by warnings/errors in Merge Request: $NB_WARNINGS_FILES"
echo $NB_WARNINGS_FILES > oai_warning_files.txt
echo ${ARRAYNAME[*]} > oai_warning_files_list.txt

exit 0
