#!/bin/bash
#set -xv
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

if [ -s $OPENAIR_DIR/cmake_targets/tools/build_helper ] ; then
   source $OPENAIR_DIR/cmake_targets/tools/build_helper
else
   echo "Error: no file in the file tree: is OPENAIR_DIR variable set?"
   exit 1
fi

source $OPENAIR_DIR/cmake_targets/tools/test_helper

SUDO="sudo -E"
tdir=$OPENAIR_DIR/cmake_targets/autotests
rm -fr $tdir/bin
mkdir -p $tdir/bin
results_file="$tdir/log/results_autotests.xml"


#\param $1 -> name of test case
#\param $2 -> name of executable
#\param $3 -> arguments for running the program
#\param $4 -> search expression
#\param $5 -> search expression which should NOT be found (for ex. segmentation fault)
#\param $6 -> number of runs
#\param $7 -> tags to help identify the test case for readability in output xml file
#\param $8 => test config file params to be modified
#\param $9 -> desc to help identify the test case for readability in output xml file

function test_run() {
    xUnit_start
    test_case_name=$1
    log_dir=$tdir/log/$test_case_name
    log_file=$log_dir/test.$1.log.txt
    main_exec=$2
    exec_args=$3
    search_expr_array=("${!4}")
    search_expr_negative=$5
    nruns=${6}
    tags=${7}
    test_config_file=${8}
    desc=${9}

    xmlfile_testcase=$log_dir/test.$1.xml
    #Temporary log file where execution log is stored.
    temp_exec_log=$log_dir/temp_log.txt
    export OPENAIR_LOGDIR=$log_dir
    rm -fr $log_dir
    mkdir -p $log_dir

    echo "" > $temp_exec_log
    echo "start at $(date)" > "$log_file"
    #echo "log_dir = $log_dir"
    #echo "log_file = $log_file"
    #echo "exec_file = $exec_file"
    #echo "exec_args = $exec_args"
    #echo "search_expr = $search_expr"
    #echo "nruns = $nruns"
    #echo "desc = $desc"

    tags_array=()
    read -a tags_array <<<"$tags"
    desc_array=()
    readarray -t desc_array <<<"$desc"

    main_exec_args_array=()
    readarray -t main_exec_args_array <<< "$exec_args"

    if [ ! -f $main_exec ]; then
      echo_fatal "no executable $main_exec found"
    fi

    tags_array_index=0
    for main_exec_args_array_index in "${main_exec_args_array[@]}"
    do
      global_result=PASS
      result_string=""
      PROPER_DESC=`echo ${desc_array[$tags_array_index]} | sed -e "s@^.*Test cases.*(Test@Test@" -e "s@^ *(@@" -e"s/)$//" -e "s/),$//"`
      echo_info "$PROPER_DESC"

     for (( run_index=1; run_index <= $nruns; run_index++ ))
      do
        temp_exec_log="$log_dir/test.$test_case_name.${tags_array[$tags_array_index]}.run_$run_index"
        echo "Executing test case $test_case_name.${tags_array[$tags_array_index]}, Run Index = $run_index, Execution Log file = $temp_exec_log"

        echo "-----------------------------------------------------------------------------" > "$temp_exec_log"
        echo "<EXECUTION LOG Test Case = $test_case_name.${tags_array[$tags_array_index]}, Run = $run_index >" >> "$temp_exec_log"
        echo "Executing $main_exec $main_exec_args_array_index at $(date)" >> "$temp_exec_log"
        uname -a >> "$temp_exec_log"
        time "$main_exec" $main_exec_args_array_index >> "$temp_exec_log"  2>&1 &
     done

     wait

     for (( run_index=1; run_index <= $nruns; run_index++ ))
      do
        temp_exec_log="$log_dir/test.$test_case_name.${tags_array[$tags_array_index]}.run_$run_index"
        echo "</EXECUTION LOG Test Case = $test_case_name.${tags_array[$tags_array_index]},  Run = $run_index >" >> $temp_exec_log  2>&1
        cat "$temp_exec_log" >> "$log_file"  2>&1

        result=PASS
        for search_expr in "${search_expr_array[@]}"
        do
          grep -Eq "$search_expr" $temp_exec_log || result=FAIL
        done

        #If we find a negative search result then there is crash of program and test case is failed even if above condition is true
        grep -q -iE "$search_expr_negative" $temp_exec_log && result=FAIL
        
        if [ "$result" != PASS ]; then
          echo_error "$test_case_name.${tags_array[$tags_array_index]} RUN = $run_index Result = FAIL"
        else
          echo_success "$test_case_name.${tags_array[$tags_array_index]} RUN = $run_index Result = PASS"
        fi

        result_string=$result_string" Run_$run_index =$result"
        if [ $result != PASS ] ; then 
          global_result=FAIL
        fi

     done #End of for loop (nindex)

     echo "END at $(date)" >> "$log_file"

      if [ "$global_result" != PASS ]; then
         echo_error "execution $test_case_name.${tags_array[$tags_array_index]} {$PROPER_DESC} Run_Result = \"$result_string\" Result =  FAIL"
         xUnit_fail "execution" "$test_case_name.${tags_array[$tags_array_index]}" "FAIL" "$result_string" "$xmlfile_testcase" "$PROPER_DESC"
       else
         echo_success "execution $test_case_name.${tags_array[$tags_array_index]} {$PROPER_DESC} Run_Result = \"$result_string\"  Result = PASS "
         xUnit_success "execution" "$test_case_name.${tags_array[$tags_array_index]}" "PASS" "$result_string"   "$xmlfile_testcase"  "$PROPER_DESC"
      fi

     let "tags_array_index++"
   done
}

dlog=$OPENAIR_DIR/cmake_targets/autotests/log
exec_dir=$OPENAIR_DIR/cmake_targets/ran_build/build
xml_conf="$OPENAIR_DIR/cmake_targets/autotests/test_case_list.xml"

function print_help() {
 echo_info '
This program runs automated test cases
Options
-c | --config
   Set the configuration file to execute (default: $xml_conf)
-d | --exec-dir
   Set directory of executables (default: $exec_dir)
-g | --run-group
   Run test cases in a group. For example, ./run_exec_autotests "0101* 010102"
-h | --help
   This help
'
}

function main () {
RUN_GROUP=0
test_case_group=""
test_case_group_array=()
test_case_array=()
echo_info "Note that the user should be sudoer for executing certain commands, for example loading kernel modules"


until [ -z "$1" ]; do
  case "$1" in
    -c|--config)
      xml_conf=$2
      echo "setting xml_conf to $xml_conf"
      shift 2;;
    -d|--exec-dir)
      exec_dir=$2
      echo "setting exec_dir to $exec_dir"
      shift 2;;
    -g | --run-group)
      RUN_GROUP=1
      test_case_group=$2
      test_case_group=`sed "s/\+/\*/g" <<<  "${test_case_group}"` # Replace + with * for bash string substituion
      echo_info "Will execute test cases only in group $test_case_group"
      shift 2;;
    -h | --help)
      print_help
      exit 1;;
    *)
      print_help
      echo_fatal "Unknown option $1"
      break;;
  esac
done

tmpfile=`mktemp`
$SUDO echo $HOME > $tmpfile
tstsudo=`cat $tmpfile`
if [ "$tstsudo" != "$HOME" ]; then
  echo_error "$USER does not have sudo privileges. Exiting"
  exit
fi
rm -fr $tmpfile

test_case_excl_list=`xmlstarlet sel -t -v "/testCaseList/TestCaseExclusionList" $xml_conf`
test_case_excl_list=`sed "s/\+/\*/g" <<< "$test_case_excl_list" ` # Replace + with * for bash string substituion
echo "Test Case Exclusion List = $test_case_excl_list "
read -a test_case_excl_array <<< "$test_case_excl_list"

test_case_list=`xmlstarlet sel -T -t -m /testCaseList/testCase -s A:N:- "@id" -v "@id" -n $xml_conf`
echo "test_case_list = $test_case_list"
readarray -t test_case_array <<<"$test_case_list"

read -a test_case_group_array <<< "$test_case_group"
for search_expr in "${test_case_array[@]}"
  do
    flag_run_test_case=0
    # search if this test case needs to be executed
    if [ "$RUN_GROUP" -eq "1" ]; then
       for search_group in "${test_case_group_array[@]}"
       do
          if [[ $search_expr == $search_group ]];then
             flag_run_test_case=1
             echo_info "Test case $search_expr match found in group"
             break
          fi
       done
    else
       flag_run_test_case=1
    fi

    for search_excl in "${test_case_excl_array[@]}"
       do
          if [[ $search_expr == $search_excl ]];then
             flag_run_test_case=0
             echo_info "Test case $search_expr match found in test case excl group. Will skip the test case for execution..."
             break
          fi
       done


    #We skip this test case if it is not in the group list
    if [ "$flag_run_test_case" -ne "1" ]; then
      continue
    fi

    name=$search_expr
    desc=`xmlstarlet sel -t -v "/testCaseList/testCase[@id='$search_expr']/desc" $xml_conf`
    main_exec=$exec_dir/`xmlstarlet sel -t -v "/testCaseList/testCase[@id='$search_expr']/main_exec" $xml_conf`
    main_exec_args=`xmlstarlet sel -t -v "/testCaseList/testCase[@id='$search_expr']/main_exec_args" $xml_conf`
    search_expr_true=`xmlstarlet sel -t -v "/testCaseList/testCase[@id='$search_expr']/search_expr_true" $xml_conf`
    search_expr_false=`xmlstarlet sel -t -v "/testCaseList/testCase[@id='$search_expr']/search_expr_false" $xml_conf`
    nruns=`xmlstarlet sel -t -v "/testCaseList/testCase[@id='$search_expr']/nruns" $xml_conf`
    tags=`xmlstarlet sel -t -v "/testCaseList/testCase[@id='$search_expr']/tags" $xml_conf`

    echo "name = $name"
    echo "Description = $desc"
    echo "main_exec = $main_exec"
    echo "main_exec_args = $main_exec_args"
    echo "search_expr_true = $search_expr_true"
    echo "search_expr_false = $search_expr_false"
    echo "tags = $tags"
    echo "nruns = $nruns"


    search_array_true=()

    IFS=\"                  #set the shell field separator
    set -f                  #dont try to glob
    #set -- $search_expr_true             #split on $IFS
    for i in $search_expr_true
      do echo "i = $i"
        if [ -n "$i" ] && [ "$i" != " " ]; then
          search_array_true+=("$i")
          #echo "inside i = \"$i\" "
        fi
      done
    unset IFS

    #echo "arg1 = ${search_array_true[0]}"
    #echo " arg2 = ${search_array_true[1]}"

    test_run \
      "$name" \
      "$main_exec" \
      "$main_exec_args" \
      "search_array_true[@]" \
      "$search_expr_false" \
      "$nruns" \
      "$tags" \
      "$test_config_file" \
      "$desc"

    done
}

uname -a

main "$@"

xUnit_write "$results_file"
echo "Test Results are written to $results_file"
