#!/bin/bash

if [ -s $OPENAIR_DIR/cmake_targets/tools/build_helper ] ; then
   source $OPENAIR_DIR/cmake_targets/tools/build_helper
else
   echo "Error: no file in the file tree: is OPENAIR_DIR variable set?"
   exit 1
fi

source $OPENAIR_DIR/cmake_targets/tools/test_helper

#SUDO="sudo -E "
tdir=$OPENAIR_DIR/cmake_targets/autotests
mkdir -p $tdir/bin $tdir/log
results_file="$tdir/log/execution_autotests.xml"

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

test_compile_and_run() {
    xUnit_start
    test_case_name=$1.$2
    log_dir=$tdir/log
    log_file=$tdir/log/test.$1.txt
    compile_prog=$2
    compile_args=$3
    pre_exec_file=$4
    pre_exec_args=$5
    exec_args=$7
    search_expr_array=("${!8}")
    search_expr_negative=$9
    nruns=${10}
    build_dir=$tdir/$1/build
    exec_file=$build_dir/$6
    
    #Temporary log file where execution log is stored.
    temp_exec_log=$log_dir/temp_log.txt
    
    echo "Compiling test case $test_case_name. Log file = $log_file"

    rm -fr $build_dir
    mkdir -p $build_dir

    #echo "log_dir = $log_dir"
    #echo "log_file = $log_file"
    #echo "exec_file = $exec_file"
    #echo "args = $args"
    #echo "search_expr = $search_expr"
    #echo "pre_exec_file = $pre_exec_file"
    #echo "nruns = $nruns"
    

 

    echo "<COMPILATION LOG>" > $log_file
    cd $build_dir
    {
        cmake ..
        #rm -fv $exec_file
        make -j`nproc` $compile_prog
    }>> $log_file 2>&1
    echo "</COMPILATION LOG>" >> $log_file 2>&1

    for (( run_index=1; run_index <= $nruns; run_index++ ))
     do

     echo "Executing test case $test_case_name, Run Index = $run_index, Log file = $log_file"

     echo "-----------------------------------------------------------------------------" >> $log_file  2>&1
     echo "<EXECUTION LOG Run = $run_index >" >> $log_file  2>&1
 
     if [ -n "$pre_exec_file" ]; then
       { source $pre_exec_file $pre_exec_args; } >> $log_file  2>&1
     fi

     { $exec_file $exec_args ;} > $temp_exec_log  2>&1

     cat $temp_exec_log >> $log_file  2>&1
     echo "</EXECUTION LOG Run = $run_index >" >> $log_file  2>&1
    
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

     #echo "result = $result"

     test_case_result=""
     if [ "$result" -eq "0" ]; then
        test_case_result="FAIL"
     fi

     if [ "$result" -eq "1" ]; then
        test_case_result="PASS"
     fi

     #If we find a negative search result then there is crash of program and test case is failed even if above condition is true

     search_result=`grep -iE "$search_expr_negative" $temp_exec_log`
     if [ -n "$search_result" ]; then
        test_case_result="FAIL"
     fi
     
     if [ "$test_case_result" == "FAIL" ]; then
        echo_error "execution $test_case_name  FAIL $run_index"
        xUnit_fail "execution" "$test_case_name" "FAIL" "$run_index"
     fi

     if [ "$test_case_result" == "PASS" ]; then
        echo_success "execution $test_case_name  PASS $run_index"
	xUnit_success "execution" "$test_case_name" "PASS" "$run_index"        
     fi

# End of for loop
    done

}

dbin=$OPENAIR_DIR/cmake_targets/autotests/bin
dlog=$OPENAIR_DIR/cmake_targets/autotests/log

run_test() {
case=case$1; shift
cmd=$1; shift
expected=$3; shift
echo "expected = $expected"
exit

$cmd > $dlog/$case.txt 2>&1
if [ $expected = "true" ] ; then	 
  if $* $dlog/$case.txt; then
    echo_success "test $case, command: $cmd ok"
  else
    echo_error "test $case, command: $cmd Failed"
  fi
else 
  if $* $dlog/$case.txt; then
    echo_error "test $case, command: $cmd Failed"
  else
    echo_success "test $case, command: $cmd ok"
  fi
fi
}

#$1 -> name of test case
#$2 -> name of compilation program
#$3 -> arguments for compilation program
#$4 -> name of pre-executable to install kernel modules, etc
#$5 -> arguments of pre-executable
#$6 -> name of executable
#$7 -> arguments for running the program
#$8 -> search expression ARRAY which needs to be found
#$9 -> search expression which should NOT be found (for ex. segmentation fault) 
#$10 -> number of runs

#oaisim tests
search_array=("Received RRCConnectionReconfigurationComplete from UE 0")
test_compile_and_run 010200 "oaisim_nos1" "" "$OPENAIR_DIR/cmake_targets/tools/init_nas_nos1" "" "oaisim_nos1" " -O $OPENAIR_TARGETS/PROJECTS/GENERIC-LTE-EPC/CONF/enb.band7.generic.oaisim.local_no_mme.conf -b1 -u1 -n100" "search_array[@]" "segmentation fault|assertion|exiting|fatal" 3

search_array=("Received RRCConnectionReconfigurationComplete from UE 0")
test_compile_and_run 010201 "oaisim_nos1" "" "$OPENAIR_DIR/cmake_targets/tools/init_nas_nos1" "" "oaisim_nos1" " -O $OPENAIR_TARGETS/PROJECTS/GENERIC-LTE-EPC/CONF/enb.band7.generic.oaisim.local_no_mme.conf -b1 -u1 -a -n100" "search_array[@]" "segmentation fault|assertion|exiting|fatal" 3

search_array=("Received RRCConnectionReconfigurationComplete from UE 0" "Received RRCConnectionReconfigurationComplete from UE 1" "Received RRCConnectionReconfigurationComplete from UE 2")
test_compile_and_run 010202 "oaisim_nos1" "" "$OPENAIR_DIR/cmake_targets/tools/init_nas_nos1" "" "oaisim_nos1" " -O $OPENAIR_TARGETS/PROJECTS/GENERIC-LTE-EPC/CONF/enb.band7.generic.oaisim.local_no_mme.conf -b1 -u3 -n100" "search_array[@]" "segmentation fault|assertion|exiting|fatal" 3

search_array=("Received RRCConnectionReconfigurationComplete from UE 0" "Received RRCConnectionReconfigurationComplete from UE 1" "Received RRCConnectionReconfigurationComplete from UE 2")
test_compile_and_run 010203 "oaisim_nos1" "" "$OPENAIR_DIR/cmake_targets/tools/init_nas_nos1" "" "oaisim_nos1" " -O $OPENAIR_TARGETS/PROJECTS/GENERIC-LTE-EPC/CONF/enb.band7.generic.oaisim.local_no_mme.conf -b1 -u3 -a -n100" "search_array[@]" "segmentation fault|assertion|exiting|fatal" 3

#PHY unitary simulations for secuirity tests
search_array=("finished with 0 errors")
test_compile_and_run 010300 "test_aes128_cmac_encrypt" "" "" "" "test_aes128_cmac_encrypt" " --verbose" "search_array[@]" "segmentation fault|assertion|exiting|fatal" 3

search_array=("finished with 0 errors")
test_compile_and_run 010301 "test_aes128_ctr_decrypt" "" "" "" "test_aes128_ctr_decrypt" " --verbose" "search_array[@]" "segmentation fault|assertion|exiting|fatal" 3

search_array=("finished with 0 errors")
test_compile_and_run 010302 "test_aes128_ctr_encrypt" "" "" "" "test_aes128_ctr_encrypt" " --verbose" "search_array[@]" "segmentation fault|assertion|exiting|fatal" 3

search_array=("finished with 0 errors")
test_compile_and_run 010303 "test_secu_kenb" "" "" "" "test_secu_kenb" " --verbose" "search_array[@]" "segmentation fault|assertion|exiting|fatal" 3

search_array=("finished with 0 errors")
test_compile_and_run 010304 "test_secu_knas" "" "" "" "test_secu_knas" " --verbose" "search_array[@]" "segmentation fault|assertion|exiting|fatal" 3

search_array=("finished with 0 errors")
test_compile_and_run 010305 "test_secu_knas_encrypt_eea1" "" "" "" "test_secu_knas_encrypt_eea1" " --verbose" "search_array[@]" "segmentation fault|assertion|exiting|fatal"  3

search_array=("finished with 0 errors")
test_compile_and_run 010306 "test_secu_knas_encrypt_eea2" "" "" "" "test_secu_knas_encrypt_eea2" " --verbose" "search_array[@]" "segmentation fault|assertion|exiting|fatal"  3

search_array=("finished with 0 errors")
test_compile_and_run 010307 "test_secu_knas_encrypt_eia1" "" "" "" "test_secu_knas_encrypt_eia1" " --verbose" "search_array[@]" "segmentation fault|assertion|exiting|fatal"  3

search_array=("finished with 0 errors")
test_compile_and_run 010308 "test_secu_knas_encrypt_eia2" "" "" "" "test_secu_knas_encrypt_eia2" " --verbose" "search_array[@]" "segmentation fault|assertion|exiting|fataln"  3

search_array=("finished with 0 errors")
test_compile_and_run 010309 "test_kdf" "" "" "" "test_kdf" " --verbose" "search_array[@]" "segmentation fault|assertion|exiting|fatal" 3

#TODO: Add test cases for 10,20 MHz
#TODO: Test and compile seperately for Rel8/Rel10


#test_compile_and_run 0200 "oaisim_nos1" "" "$OPENAIR_DIR/cmake_targets/tools/init_nas_nos1" "" "oaisim_nos1" " -O /home/calisson/rohit/oai_snav/taets/PROJECTS/GENERIC-LTE-EPC/CONF/enb.band7.generic.oaisim.local_no_mme.conf -b1 -u1 -a " "RRC_CONN" 3

#run_test 0200 "$dbin/oaisim.r8 -a -A AWGN -n 100" false grep -q '(Segmentation.fault)|(Exiting)|(FATAL)'

#run_test 0201 "$dbin/oaisim.r8 -a -A AWGN -n 100" false fgrep -q '[E]'

# write the test results into a file

xUnit_write "$results_file"

echo "Test Results are written to $results_file"

