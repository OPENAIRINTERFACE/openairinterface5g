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

# \author Navid Nikaein, Rohit Gupta

if [ -s $OPENAIR_DIR/cmake_targets/tools/build_helper ] ; then
   source $OPENAIR_DIR/cmake_targets/tools/build_helper
else
   echo "Error: no file in the file tree: is OPENAIR_DIR variable set?"
   exit 1
fi

trap handle_ctrl_c INT

source $OPENAIR_DIR/cmake_targets/tools/test_helper


SUDO="sudo -E -S"
tdir=$OPENAIR_DIR/cmake_targets/autotests
rm -fr $tdir/bin 
mkdir -p $tdir/bin
results_file="$tdir/log/results_autotests.xml"

updated=$(svn st -q $OPENAIR_DIR)
if [ "$updated" != "" ] ; then
	echo "some files are not in svn:\n $updated"
fi

cd $tdir 

#\param $1 -> name of test case
#\param $2 -> name of compilation program
#\param $3 -> arguments for compilation program
#\param $4 -> name of pre-executable to install kernel modules, etc
#\param $5 -> arguments of pre-executable
#\param $6 -> name of executable
#\param $7 -> arguments for running the program
#\param $8 -> search expression
#\param $9 -> search expression which should NOT be found (for ex. segmentation fault) 
#\param $10 -> number of runs
#\param $11 -> pre_compile program execution
#\param $12 -> class of the test case (compilation, execution)
#\param $13 -> output of compilation program that needs to be found for test case to pass
#\param $14 -> tags to help identify the test case for readability in output xml file
function test_compile() {

    xUnit_start
    test_case_name=$1
    log_dir=$tdir/log/$test_case_name
    log_file=$log_dir/test.$1.log.txt
    compile_prog=$2
    compile_args=$3
    pre_exec_file=$4
    pre_exec_args=$5
    exec_args=$7
    search_expr_array=("${!8}")
    search_expr_negative=$9
    nruns=${10}
    pre_compile_prog=${11}
    class=${12}
    compile_prog_out=${13}
    tags=${14}
    xmlfile_testcase=$log_dir/test.$1.xml
    #build_dir=$tdir/$1/build
    #exec_file=$build_dir/$6
    
    #compile_prog_out=`eval "echo $compile_prog_out"`
    #echo "compile_prog_out = $compile_prog_out"
    read -a compile_prog_out_array <<< "$compile_prog_out"

    #Temporary log file where execution log is stored.
    temp_exec_log=$log_dir/temp_log.txt
    rm -fr $log_dir
    mkdir -p $log_dir



    #echo "log_dir = $log_dir"
    #echo "log_file = $log_file"
    #echo "exec_file = $exec_file"
    #echo "exec_args = $exec_args"
    #echo "search_expr = $search_expr"
    #echo "pre_exec_file = $pre_exec_file"
    #echo "nruns = $nruns"
    echo "class = $class"
    
    compile_prog_array=()
    read -a compile_prog_array <<<"$compile_prog"
    
    #tags_array=()
    #read -a tags_array <<<"$tags"
    
    #pre_compile_prog_array=()
    #readarray -t pre_compile_prog_array <<< "$pre_compile_prog"
    result=1
    result_string=""
    for (( run_index=1; run_index <= $nruns; run_index++ ))
    do
       
    #tags_array_index=0
    #for pre_compile_prog_array_index in "${pre_compile_prog_array[@]}"  
    #do
    
    #for compile_prog_array_index in "${compile_prog_array[@]}"  
    #do
       echo "Compiling test case $test_case_name.${tags} Log file = $log_file"  
       date=`date`
       echo "<COMPILATION LOG file=$test_case_name.${tags} , Run = $run_index>, Date = $date " >> $log_file
       #rm -fr $build_dir
       #mkdir -p $build_dir
       cd $log_dir
       {   
          uname -a
          compile_log_dir=`eval echo \"$OPENAIR_DIR/cmake_targets/log/\"`
          echo "Removing compilation log files in $compile_log_dir"
          rm -frv $compile_log_dir
          echo "Executing $pre_exec_file $pre_exe_args ...."
          eval $pre_exec_file  $pre_exec_args
          echo "Executing $compile_prog $compile_prog_args ...."
          eval $compile_prog  $compile_prog_args
          echo "Copying compilation log files to test case log directory: $log_dir"
          cp -fvr $OPENAIR_DIR/cmake_targets/log/ $log_dir/compile_log
       }>> $log_file 2>&1
       echo "</COMPILATION LOG>" >> $log_file 2>&1
       if [ "$class" == "compilation" ]; then
         for compile_prog_out_index in ${compile_prog_out_array[@]}
            do 
                if  [ -s "$compile_prog_out_index" ]; then
                    let "result = result&1"
                    
                    echo_success "$test_case_name.${tags} RUN = $run_index $compile_prog_out_index = compiled"
                    
                else
                    let "result = result&0"
                            
                    echo_error "$test_case_name.${tags} RUN = $run_index $compile_prog_out_index failed"
                    
                 fi
            done #end of for loop compile_prog_out_index
         if [ "$result" == "1" ]; then
            result_string=$result_string" Run_$run_index = PASS"
         else   
            result_string=$result_string" Run_$run_index = FAIL"           
         fi

       fi
       #let "tags_array_index++"
    #done # End of for loop compile_prog_array
    #done # End of for loop (pre_compile_prog_array_index)
    done #End of for loop (run_index)
    
   
    #If for for some reason upper for loop does not execute, we fail the test case completely
    if [ "$result_string" == "" ]; then
      result=0
    fi
    if [ "$result" == "1" ]; then
      echo_success "$test_case_name.${tags} PASSED"
      xUnit_success "compilation" "$test_case_name.$tags" "PASS" "$result_string" "$xmlfile_testcase" ""
    else             
      echo_error "$test_case_name.${tags} FAILED"
      xUnit_fail "compilation" "$test_case_name.$tags" "FAIL" "$result_string" "$xmlfile_testcase" ""
    fi
}



#\param $1 -> name of test case
#\param $2 -> name of compilation program
#\param $3 -> arguments for compilation program
#\param $4 -> name of pre-executable to install kernel modules, etc
#\param $5 -> arguments of pre-executable
#\param $6 -> name of executable
#\param $7 -> arguments for running the program
#\param $8 -> search expression
#\param $9 -> search expression which should NOT be found (for ex. segmentation fault) 
#\param $10 -> number of runs
#\param $11 -> pre_compile program execution
#\param $12 -> class of the test case (compilation, execution)
#\param $13 -> output of compilation program that needs to be found for test case to pass
#\param $14 -> tags to help identify the test case for readability in output xml file
#\param $15 => password for the user to run certain commands as sudo
#\param $16 => test config file params to be modified
#\param $17 => bypass flag if main_exec if available
#\param $18 -> desc to help identify the test case for readability in output xml file

function test_compile_and_run() {
    xUnit_start
    test_case_name=$1
    log_dir=$tdir/log/$test_case_name
    log_file=$log_dir/test.$1.log.txt
    compile_prog=$2
    compile_args=$3
    pre_exec_file=$4
    pre_exec_args=$5
    main_exec=$6
    exec_args=$7
    search_expr_array=("${!8}")
    search_expr_negative=$9
    nruns=${10}
    pre_compile_prog=${11}
    class=${12}
    compile_prog_out=${13}
    tags=${14}
    mypassword=${15}
    test_config_file=${16}
    bypass_compile=${17}
    desc=${18}

    build_dir=$tdir/$1/build
    #exec_file=$build_dir/$6
    xmlfile_testcase=$log_dir/test.$1.xml
    #Temporary log file where execution log is stored.
    temp_exec_log=$log_dir/temp_log.txt
    export OPENAIR_LOGDIR=$log_dir
    rm -fr $log_dir
    mkdir -p $log_dir
    
    echo "" > $temp_exec_log
    echo "" > $log_file
    #echo "log_dir = $log_dir"
    #echo "log_file = $log_file"
    #echo "exec_file = $exec_file"
    #echo "exec_args = $exec_args"
    #echo "search_expr = $search_expr"
    #echo "pre_exec_file = $pre_exec_file"
    #echo "nruns = $nruns"
    echo "class = $class"
    #echo "desc = $desc"
    
    #compile_prog_array=()
    #read -a compile_prog_array <<<"$compile_prog"

    #test_config_file=`eval "echo \"$test_config_file\" "`

    #echo "test_config_file = $test_config_file"
  
    tags_array=()
    read -a tags_array <<<"$tags"
    desc_array=()
    readarray -t desc_array <<<"$desc"
    
    main_exec_args_array=()
    readarray -t main_exec_args_array <<< "$exec_args"
    
    REAL_MAIN_EXEC=`eval "echo $main_exec"`
    if [ "$bypass_compile" == "1" ] && [ -f $REAL_MAIN_EXEC ]
    then
        echo "Bypassing compilation for $main_exec"
    else
        rm -fr $OPENAIR_DIR/cmake_targets/log

    #for search_expr in "${compile_prog_array[@]}"  
    #do
       echo "Compiling test case $test_case_name Log file = $log_file"  
       echo "<COMPILATION LOG file=$log_file>" >> $log_file

       #rm -fr $build_dir
       #mkdir -p $build_dir

       cd $log_dir
       {   
          uname -a
          echo "Executing $pre_compile_prog"
          eval $pre_compile_prog
 
          if [ "$test_config_file" != "" ]; then
            echo "Modifying test_config_file parameters..."
            echo "$test_config_file" |xargs -L 1 $OPENAIR_DIR/cmake_targets/autotests/tools/search_repl.py 
          fi
          echo "Executing $compile_prog $compile_args"
          eval "$compile_prog $compile_args"
          echo "Copying compilation log files to test case log directory: $log_dir"
          cp -fvr $OPENAIR_DIR/cmake_targets/log/ $log_dir/compile_log
       }>> $log_file 2>&1
       echo "</COMPILATION LOG>" >> $log_file 2>&1
    #done
    fi
    
    #process the test case if it is that of execution
    if [ "$class" == "execution" ]; then
      tags_array_index=0
      for main_exec_args_array_index in "${main_exec_args_array[@]}"  
      do
        global_result=1
        result_string=""
        PROPER_DESC=`echo ${desc_array[$tags_array_index]} | sed -e "s@^.*lsim.*est case.*(Test@Test@" -e "s@^ *(@@" -e "s/),$//"`
        
       for (( run_index=1; run_index <= $nruns; run_index++ ))
        do
          temp_exec_log=$log_dir/test.$test_case_name.${tags_array[$tags_array_index]}.run_$run_index
          echo "" > $temp_exec_log

          echo "Executing test case $test_case_name.${tags_array[$tags_array_index]}, Run Index = $run_index, Execution Log file = $temp_exec_log"

          echo "-----------------------------------------------------------------------------" >> $temp_exec_log  2>&1
          echo "<EXECUTION LOG Test Case = $test_case_name.${tags_array[$tags_array_index]}, Run = $run_index >" >> $temp_exec_log  2>&1
           
          if [ -n "$pre_exec_file" ]; then
            {  echo " Executing $pre_exec_file $pre_exec_args " 
               eval " $pre_exec_file $pre_exec_args " ; }>> $temp_exec_log  2>&1

          fi
          echo "Executing $main_exec $main_exec_args_array_index "
          echo "Executing $main_exec $main_exec_args_array_index " >> $temp_exec_log
          { uname -a ; eval "$main_exec $main_exec_args_array_index" ;} >> $temp_exec_log  2>&1

          echo "</EXECUTION LOG Test Case = $test_case_name.${tags_array[$tags_array_index]},  Run = $run_index >" >> $temp_exec_log  2>&1
          cat $temp_exec_log >> $log_file  2>&1
          
    
          result=1
          for search_expr in "${search_expr_array[@]}"
          do
     
            search_result=`grep -E "$search_expr" $temp_exec_log`

            #echo "search_expr  =   $search_expr"
            #echo "search_result = $search_result"

            if [ -z "$search_result" ]; then
              let "result = result & 0"
            else
              let "result = result & 1"
            fi
          done
          
          #If we find a negative search result then there is crash of program and test case is failed even if above condition is true
          search_result=`grep -iE "$search_expr_negative" $temp_exec_log`
          if [ -n "$search_result" ]; then
            result=0
          fi
          let "global_result = global_result & result"

          #echo "result = $result"
          
          #this is a result of this run
          #test_case_result=""
          if [ "$result" -eq "0" ]; then
            result_string=$result_string" Run_$run_index =FAIL"
            echo_error "$test_case_name.${tags_array[$tags_array_index]} RUN = $run_index Result = FAIL"
          fi

          if [ "$result" -eq "1" ]; then
            result_string=$result_string" Run_$run_index =PASS"
            echo_success "$test_case_name.${tags_array[$tags_array_index]} RUN = $run_index Result = PASS"
          fi

          
        done #End of for loop (nindex)

       echo " Result String = $result_string" 

       if [ "$result_string" == "" ]; then
           echo_error "execution $test_case_name.$compile_prog.${tags_array[$tags_array_index]} Run_Result = \"$result_string\"  Result = FAIL"
	   xUnit_fail "execution" "$test_case_name.${tags_array[$tags_array_index]}" "FAIL" "$result_string" "$xmlfile_testcase" "$PROPER_DESC"
       else
        if [ "$global_result" == "0" ]; then
           echo_error "execution $test_case_name.${tags_array[$tags_array_index]} Run_Result = \"$result_string\" Result =  FAIL"
           xUnit_fail "execution" "$test_case_name.${tags_array[$tags_array_index]}" "FAIL" "$result_string" "$xmlfile_testcase" "$PROPER_DESC"
        fi

        if [ "$global_result" == "1" ]; then
            echo_success "execution $test_case_name.${tags_array[$tags_array_index]} Run_Result = \"$result_string\"  Result = PASS "
	    xUnit_success "execution" "$test_case_name.${tags_array[$tags_array_index]}" "PASS" "$result_string"   "$xmlfile_testcase"  "$PROPER_DESC"
        fi  
       fi

       let "tags_array_index++"
     done 
   fi
   rm -fr $build_dir
}

dbin=$OPENAIR_DIR/cmake_targets/autotests/bin
dlog=$OPENAIR_DIR/cmake_targets/autotests/log


function print_help() {
 echo_info '
This program runs automated test case system for OpenAirInterface
You should have ubuntu 14.xx, updated, and the Linux kernel >= 3.14
Options
-h | --help
   This help
-g | --run-group
   Run test cases in a group. For example, ./run_exec_autotests "0101* 010102"
-p
   Use password for logging
-np | --no-password
   No need for a password
-q | --quiet
   Quiet  mode;  eliminate  informational  messages and comment prompts.
-b | --bypass-compile
   Bypass compilation of main-exec if already present
'
}

function main () {
QUIET=0
BYPASS_COMPILE=0
RUN_GROUP=0
SET_PASSWORD=0
passwd=""
test_case_group=""
test_case_group_array=()
test_case_array=()
echo_info "Note that the user should be sudoer for executing certain commands, for example loading kernel modules"


until [ -z "$1" ]
  do
    case "$1" in
       -g | --run-group)
            RUN_GROUP=1
            test_case_group=$2
            test_case_group=`sed "s/\+/\*/g" <<<  "${test_case_group}"` # Replace + with * for bash string substituion
            echo_info "Will execute test cases only in group $test_case_group"
            shift 2;;
        -p)
            SET_PASSWORD=1
            passwd=$2
            shift 2;;
        -np|--no-password)
            SET_PASSWORD=1
            shift ;;
        -q|--quiet)
            QUIET=1
            shift ;;
        -b|--bypass-compile)
            BYPASS_COMPILE=1
            echo "bypass option ON"
            shift ;;
        -h | --help)
            print_help
            exit 1;;
	*)
	    print_help
            echo_fatal "Unknown option $1"
            break;;
   esac
  done

if [ "$SET_PASSWORD" != "1" ]; then
   read -s -p "Enter Password: " passwd
fi

tmpfile=`mktemp`
echo $passwd | $SUDO echo $HOME > $tmpfile
tstsudo=`cat $tmpfile`
if [ "$tstsudo" != "$HOME" ]; then
  echo "$USER might not have sudo privileges. Exiting" 
  exit
else
  echo "$USER has sudo privileges"
fi
echo "tstsudo = $tstsudo"
rm -fr $tmpfile

xml_conf="$OPENAIR_DIR/cmake_targets/autotests/test_case_list.xml"

test_case_list=`xmlstarlet sel -T -t -m /testCaseList/testCase -s A:N:- "@id" -v "@id" -n $xml_conf`
test_case_excl_list=`xmlstarlet sel -t -v "/testCaseList/TestCaseExclusionList" $xml_conf`
if [ $QUIET -eq 0 ]; then echo "Test Case Exclusion List = $test_case_excl_list "; fi

test_case_excl_list=`sed "s/\+/\*/g" <<< "$test_case_excl_list" ` # Replace + with * for bash string substituion

read -a test_case_excl_array <<< "$test_case_excl_list"

if [ $QUIET -eq 0 ]; then echo "test_case_list = $test_case_list"; fi

if [ $QUIET -eq 0 ]; then echo "Test Case Exclusion List = $test_case_excl_list \n"; fi

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
             if [ $QUIET -eq 0 ]; then echo_info "Test case $search_expr match found in test case excl group. Will skip the test case for execution..."; fi
             break
          fi
       done


    #We skip this test case if it is not in the group list
    if [ "$flag_run_test_case" -ne "1" ]; then
      continue
    fi

    name=$search_expr
    class=`xmlstarlet sel -t -v "/testCaseList/testCase[@id='$search_expr']/class" $xml_conf`
    desc=`xmlstarlet sel -t -v "/testCaseList/testCase[@id='$search_expr']/desc" $xml_conf`
    pre_compile_prog=`xmlstarlet sel -t -v "/testCaseList/testCase[@id='$search_expr']/pre_compile_prog" $xml_conf`
    compile_prog=`xmlstarlet sel -t -v "/testCaseList/testCase[@id='$search_expr']/compile_prog" $xml_conf`
    compile_prog_args=`xmlstarlet sel -t -v "/testCaseList/testCase[@id='$search_expr']/compile_prog_args" $xml_conf`
    pre_exec=`xmlstarlet sel -t -v "/testCaseList/testCase[@id='$search_expr']/pre_exec" $xml_conf`
    pre_exec_args=`xmlstarlet sel -t -v "/testCaseList/testCase[@id='$search_expr']/pre_exec_args" $xml_conf`
    main_exec=`xmlstarlet sel -t -v "/testCaseList/testCase[@id='$search_expr']/main_exec" $xml_conf`
    main_exec_args=`xmlstarlet sel -t -v "/testCaseList/testCase[@id='$search_expr']/main_exec_args" $xml_conf`
    search_expr_true=`xmlstarlet sel -t -v "/testCaseList/testCase[@id='$search_expr']/search_expr_true" $xml_conf`
    search_expr_false=`xmlstarlet sel -t -v "/testCaseList/testCase[@id='$search_expr']/search_expr_false" $xml_conf`
    nruns=`xmlstarlet sel -t -v "/testCaseList/testCase[@id='$search_expr']/nruns" $xml_conf`
    compile_prog_out=`xmlstarlet sel -t -v "/testCaseList/testCase[@id='$search_expr']/compile_prog_out" $xml_conf`
    tags=`xmlstarlet sel -t -v "/testCaseList/testCase[@id='$search_expr']/tags" $xml_conf`
    test_config_file=`xmlstarlet sel -t -v "/testCaseList/testCase[@id='$search_expr']/test_config_file" $xml_conf`

    echo "class = $class"
    echo "name = $name"
    echo "Description = $desc"
    echo "pre_compile_prog = $pre_compile_prog"
    echo "compile_prog = $compile_prog"
    echo "compile_prog_args = $compile_prog_args"
    echo "compile_prog_out = $compile_prog_out"
    echo "pre_exec = $pre_exec"
    echo "pre_exec_args = $pre_exec_args"
    echo "main_exec = $main_exec"
    echo "main_exec_args = $main_exec_args"
    echo "search_expr_true = $search_expr_true"
    echo "search_expr_false = $search_expr_false"
    echo "nruns = $nruns"


    #eval $pre_exec
    compile_prog_out=`eval echo \"$compile_prog_out\"`

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
    if [ "$class" == "compilation" ]; then
        test_compile "$name" "$compile_prog" "$compile_prog_args" "$pre_exec" "$pre_exec_args" "$main_exec" "$main_exec_args" "search_array_true[@]" "$search_expr_false" "$nruns" "$pre_compile_prog" "$class" "$compile_prog_out" "$tags" "$desc"
    elif  [ "$class" == "execution" ]; then
        echo \'passwd\' | $SUDO killall -q oaisim_nos1
        test_compile_and_run "$name" "$compile_prog" "$compile_prog_args" "$pre_exec" "$pre_exec_args" "$main_exec" "$main_exec_args" "search_array_true[@]" "$search_expr_false" "$nruns" "$pre_compile_prog" "$class" "$compile_prog_out" "$tags" "$mypassword" "$test_config_file" "$BYPASS_COMPILE" "$desc"
    else
        echo "Unexpected class of test case...Skipping the test case $name ...."
    fi

    done
    
    

}

uname -a

main "$@"

xUnit_write "$results_file"

echo "Test Results are written to $results_file"

exit



