#!/bin/bash
#
# Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
# contributor license agreements.  See the NOTICE file distributed with
# this work for additional information regarding copyright ownership.
# The OpenAirInterface Software Alliance licenses this file to You under
# the OAI Public License, Version 1.1  (the "License"); you may not use this file
# except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.openairinterface.org/?page_id=698
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#-------------------------------------------------------------------------------
# For more information about the OpenAirInterface (OAI) Software Alliance:
#      contact@openairinterface.org
#
#----------------------
# File: run_tests.sh
#----------------------
# Description: script batch for:
# - building tests
# - executing tests
# - checking output files with reference files.
# ./run_test to build/run and check all tests
# -b no build
# -e no execution
# -c no check
# -m no run of meld
# an alone specific test can be executed ./run_tests test_name
#----------------------------------------------------------------------------
# CONFIGURATION
# files and directories

WORK_DIR="openair1/SIMULATION/NR_UE_PHY/unit_tests/build"
OAIENV_DIR="../../../../.."
EXECUTABLE_DIR="."
REFERENCE_DIR="./reflogs"
RESULT_TEST_FILE=""
RESULT_DIR="./testlogs"
BROWSER="firefox"

#----------------------------------------------------------------------------
# For building and executing test application

BUILD_TEST="yes"
CHECK_TEST="yes"
EXECUTE_TEST="yes"
SINGLE_TEST="no"
RUN_MELD="yes"
# for removing files
REMOVE="rm -f"
COMPARE="cmp"
TESTS_RUN=0
TESTS_PASS=0
TESTS_FAIL=0

#----------------------------------------------------------------------------
# List of tests

tst_files="
   pss_test
   sss_test
   pbch_test
   srs_test
   frame_config_test
   harq_test
   pucch_uci_test"

#---------------------------------------------------------------------------
# manage input parameters

while [ $# != "0" ]
do  
  case $1 in
   -b) 
      echo "No build of unit test"
      BUILD_TEST="no"
      ;;
   -e) 
      echo "No execution of unit test"
      EXECUTE_TEST="no"
      ;;
   -c) 
      echo "No check of unit test"
      CHECK_TEST="no"
      ;;
   -m) 
      echo "No run of meld tool"
      RUN_MELD="no"
      ;;
   -h) 
      echo "Option of run_test script"
      echo "-b : No Build of unit tests"
      echo "-c : No check for unit test"
      echo "-e : No run of unit tests"
      echo "-m : No run of meld tool"
      exit
      BUILD_TEST="no"
      CHECK_TEST="no"
      EXECUTE_TEST="no"
      ;;
    *) 
      for file in $tst_files
      do
       if [ $file == $1 ]
       then        
         SINGLE_TEST="yes"
         TEST=$1
         echo "Single test $TEST"
       fi
      done
      if  [ $SINGLE_TEST == "no" ]
      then
       echo "Unknown parameter $1"
       BUILD_TEST="no"
       CHECK_TEST="no"
       EXECUTE_TEST="no"
       exit
      fi
      ;;
  esac
  # shift of input parameter (excluding $0 ) $1 <- $2 <- $3 ...
  # $#  is decrement by shift
  shift  
done

#--------------------------------------------------------------------------------------------
#  RUN AND CHECK RESULTS

numberDisplayFilesDifferencies=0
let MAX_NUMBER_DISPLAY_FILES_DIFFERENCES=8
num_diff=0

if [ $EXECUTE_TEST == "yes" ] 
then
  mkdir $RESULT_DIR
fi
  
if [ $BUILD_TEST == "yes" ] 
then
  if [ $SINGLE_TEST == "no" ]
  then
    echo "cd $OAIENV_DIR"
    cd $OAIENV_DIR
    echo "source oaienv"
    source oaienv
    echo "cd $WORK_DIR"
    cd $WORK_DIR
    echo "cmake CMakeLists.txt"
    cmake CMakeLists.txt
    echo -e "make clean\n"
    #make clean    
  fi
fi

if [ $CHECK_TEST == "yes" ]
then
  echo "Display differencies for a maximum of "$MAX_NUMBER_DISPLAY_FILES_DIFFERENCES" files"
fi 

for file in $tst_files
  do
  
    if [ $SINGLE_TEST == "yes" ] 
    then
      file=$TEST
    fi
   
    echo "Test : $file"
    RESULT_TEST_FILE="$file.txt"
        
    # build executable
    if [ $BUILD_TEST == "yes" ] 
    then
      echo -e "make $file\n"     
      make $file   
    fi

    if [ $EXECUTE_TEST == "yes" ] 
    then
      echo "rm $RESULT_DIR/$RESULT_TEST_FILE"
      rm $RESULT_DIR/$RESULT_TEST_FILE
      echo "$EXECUTABLE_DIR/$file > $RESULT_DIR/$RESULT_TEST_FILE"
      $EXECUTABLE_DIR/$file > $RESULT_DIR/$RESULT_TEST_FILE    
      TESTS_RUN=$((TESTS_RUN+1))
    fi
    
    if [ $CHECK_TEST == "yes" ] 
    then
      # check if there is already a reference file
      if [ -f "$REFERENCE_DIR/$RESULT_TEST_FILE" ]
      then

        echo File "$REFERENCE_DIR/$RESULT_TEST_FILE" exists
       
        $COMPARE $RESULT_DIR/$RESULT_TEST_FILE $REFERENCE_DIR/$RESULT_TEST_FILE > /dev/null 2>&1

        if [ $? == 0 ]
        then
        	echo "Test $file is PASS"
            echo "Same logging file for $file"
            TESTS_PASS=$((TESTS_PASS+1))
        else
          echo "Test $file is FAIL"
          TESTS_FAIL=$((TESTS_FAIL+1))
      echo "Difference of logging file for scenario $file"          
          let "num_diff=$num_diff + 1"
          if  [ $RUN_MELD == "yes" ]
          then
            meld $RESULT_DIR/$RESULT_TEST_FILE $REFERENCE_DIR/$RESULT_TEST_FILE &
            let "numberDisplayFilesDifferencies=$numberDisplayFilesDifferencies + 1"
            echo "Meld number "$numberDisplayFilesDifferencies
            if [ $numberDisplayFilesDifferencies = $MAX_NUMBER_DISPLAY_FILES_DIFFERENCES ] 
            then
              echo "######  ERROR  ######  ERROR  ######  ERROR  ######  ERROR  ######  ERROR  ######  ERROR  ######"
              echo "=> ERROR Test aborted because there are already $MAX_NUMBER_DISPLAY_FILES_DIFFERENCES detected logging files which are different"
              break
            fi
          fi
         fi
      else
        echo "No reference file for test $file"
      fi   
    fi
    
    if [ $SINGLE_TEST == "yes" ] 
    then
      echo "Test $file has been executed"
      break
    fi
  done

if [ $CHECK_TEST == "yes" ]
then
  echo "There are $num_diff result files which are different"
fi

  echo " tests run : $TESTS_RUN pass : $TESTS_PASS fail : $TESTS_FAIL" 

# end of script
