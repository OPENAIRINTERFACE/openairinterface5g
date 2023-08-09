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
    echo "OAI GitLab merge request applying script"
    echo "   Original Author: Raphael Defosseux"
    echo ""
    echo "Usage:"
    echo "------"
    echo ""
    echo "    checkGitLabMergeRequestLabels.sh [OPTIONS]"
    echo ""
    echo "Options:"
    echo "------------------"
    echo ""
    echo "    --mr-id ####"
    echo "    Specify the ID of the merge request."
    echo ""
    echo "    --help OR -h"
    echo "    Print this help message."
    echo ""
}

if [ $# -ne 2 ] && [ $# -ne 1 ]
then
    echo "Syntax Error: not the correct number of arguments"
    echo ""
    usage
    exit 1
fi

while [[ $# -gt 0 ]]
do
key="$1"

case $key in
    -h|--help)
    shift
    usage
    exit 0
    ;;
    --mr-id)
    MERGE_REQUEST_ID="$2"
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

LABELS=`curl --silent "https://gitlab.eurecom.fr/api/v4/projects/oai%2Fopenairinterface5g/merge_requests/$MERGE_REQUEST_ID" | jq '.labels' || true`

IS_MR_DOCUMENTATION=`echo $LABELS | grep -ic documentation`
IS_MR_BUILD_ONLY=`echo $LABELS | grep -c BUILD-ONLY`
IS_MR_CI=`echo $LABELS | grep -c CI`
IS_MR_4G=`echo $LABELS | grep -c 4G-LTE`
IS_MR_5G=`echo $LABELS | grep -c 5G-NR`

# none is present! No CI
if [ $IS_MR_BUILD_ONLY -eq 0 ] && [ $IS_MR_CI -eq 0 ] && [ $IS_MR_4G -eq 0 ] && [ $IS_MR_5G -eq 0 ] && [ $IS_MR_DOCUMENTATION -eq 0 ]
then
    echo "NONE"
    exit 0
fi

# 4G and 5G or CI labels: run everything (4G, 5G)
if [ $IS_MR_4G -eq 1 ] && [ $IS_MR_5G -eq 1 ] || [ $IS_MR_CI -eq 1 ]
then
    echo "FULL"
    exit 0
fi

# 4G is present: run only 4G
if [ $IS_MR_4G -eq 1 ]
then
    echo "SHORTEN-4G"
    exit 0
fi

# 5G is present: run only 5G
if [ $IS_MR_5G -eq 1 ]
then
    echo "SHORTEN-5G"
    exit 0
fi

# BUILD-ONLY is present: only build stages
if [ $IS_MR_BUILD_ONLY -eq 1 ]
then
    echo "BUILD-ONLY"
    exit 0
fi

# Documentation is present: don't do anything
if [ $IS_MR_DOCUMENTATION -eq 1 ]
then
    echo "documentation"
    exit 0
fi
