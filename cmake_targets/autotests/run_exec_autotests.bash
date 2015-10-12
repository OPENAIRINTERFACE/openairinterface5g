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
rm -fr $tdir/bin $tdir/log
mkdir -p $tdir/bin $tdir/log
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

test_compile() {

    xUnit_start
    test_case_name=$1
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
    pre_compile_prog=${11}
    class=${12}
    build_dir=$tdir/$1/build
    exec_file=$build_dir/$6
    
    #Temporary log file where execution log is stored.
    temp_exec_log=$log_dir/temp_log.txt



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
     
    pre_compile_prog_array=()
    readarray -t pre_compile_prog_array <<< "$pre_compile_prog"
    for (( run_index=1; run_index <= $nruns; run_index++ ))
    do

    for pre_compile_prog_array_index in "${pre_compile_prog_array[@]}"  
    do

    for compile_prog_array_index in "${compile_prog_array[@]}"  
    do
       echo "Compiling test case $test_case_name.$compile_prog_array_index Log file = $log_file"  
       echo "<COMPILATION LOG file=$compile_prog_array_index , Run = $run_index>" >> $log_file
       rm -fr $build_dir
       mkdir -p $build_dir
       cd $build_dir
       {   
          eval $pre_compile_prog_array_index
          cmake ..
          #rm -fv $exec_file
          make -j`nproc` $compile_prog_array_index $compile_args
       }>> $log_file 2>&1
       echo "</COMPILATION LOG>" >> $log_file 2>&1
       if [ "$class" == "compilation" ]; then
         if [ -s "$compile_prog_array_index" ] ; then
           echo_success "$test_case_name $compile_prog_array_index compiled"
           xUnit_success "compilation" "$test_case_name.$compile_prog_array_index" "PASS" "$run_index"
         else
           echo_error "$test_case_name $exec_prog compilation failed"
           xUnit_fail "compilation" "$test_case_name.$compile_prog_array_index" "FAIL" "$run_index"
         fi
       fi
    done # End of for loop compile_prog_array
    done # End of for loop (pre_compile_prog_array_index)
    done #End of for loop (run_index)
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

test_compile_and_run() {
    xUnit_start
    test_case_name=$1
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
    pre_compile_prog=${11}
    class=${12}
    build_dir=$tdir/$1/build
    exec_file=$build_dir/$6
    
    #Temporary log file where execution log is stored.
    temp_exec_log=$log_dir/temp_log.txt
    
    



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
  

    for search_expr in "${compile_prog_array[@]}"  
    do
       echo "Compiling test case $test_case_name.$search_expr Log file = $log_file"  
       echo "<COMPILATION LOG file=$search_expr>" >> $log_file

       rm -fr $build_dir
       mkdir -p $build_dir

       cd $build_dir
       {   
          eval $pre_compile_prog
          cmake ..
          #rm -fv $exec_file
          make -j`nproc` $search_expr $compile_args
       }>> $log_file 2>&1
       echo "</COMPILATION LOG>" >> $log_file 2>&1
       if [ "$class" == "compilation" ]; then
         echo "compilation 1"
         #TODO: The number of runs for compilation test cases is always 1
         if [ -s "$search_expr" ] ; then
           echo "compilation 1 pass"
           echo_success "$test_case_name $search_expr compiled"
           xUnit_success "compilation" "$test_name.$search_expr" "PASS" 1
         else
           echo "compilation 1 fail"
           echo_error "$test_case_name $exec_prog compilation failed"
           xUnit_fail "compilation" "$test_name.$search_expr" "FAIL" 1
         fi
       fi
    done

    #process the test case if it is that of execution
    if [ "$class" == "execution" ]; then
      for (( run_index=1; run_index <= $nruns; run_index++ ))
        do

        echo "Executing test case $test_case_name, Run Index = $run_index, Log file = $log_file"

        echo "-----------------------------------------------------------------------------" >> $log_file  2>&1
        echo "<EXECUTION LOG Run = $run_index >" >> $log_file  2>&1
 
        if [ -n "$pre_exec_file" ]; then
          { eval "source $pre_exec_file $pre_exec_args"; } >> $log_file  2>&1
        fi

        { eval "$exec_file $exec_args" ;} > $temp_exec_log  2>&1

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
         echo_error "execution $test_case_name.$compile_prog  FAIL $run_index"
         xUnit_fail "execution" "$test_case_name.$compile_prog" "FAIL" "$run_index"
       fi

       if [ "$test_case_result" == "PASS" ]; then
         echo_success "execution $test_case_name.$compile_prog  PASS $run_index"
	 xUnit_success "execution" "$test_case_name.$compile_prog" "PASS" "$run_index"        
       fi

      # End of for loop
     done
   fi
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

main () {

xml_conf="$OPENAIR_DIR/cmake_targets/autotests/test_case_list.xml"

test_case_list=`xmlstarlet sel -T -t -m /xml/testCaseList/testCase -s D:N:- "@id" -v "@id" -n $xml_conf`

echo "test_case_list = $test_case_list"

readarray -t test_case_array <<<"$test_case_list"

for search_expr in "${test_case_array[@]}"
  do
    name=$search_expr
    class=`xmlstarlet sel -t -v "/xml/testCaseList/testCase[@id='$search_expr']/class" $xml_conf`
    desc=`xmlstarlet sel -t -v "/xml/testCaseList/testCase[@id='$search_expr']/desc" $xml_conf`
    pre_compile_prog=`xmlstarlet sel -t -v "/xml/testCaseList/testCase[@id='$search_expr']/pre_compile_prog" $xml_conf`
    compile_prog=`xmlstarlet sel -t -v "/xml/testCaseList/testCase[@id='$search_expr']/compile_prog" $xml_conf`
    compile_prog_args=`xmlstarlet sel -t -v "/xml/testCaseList/testCase[@id='$search_expr']/compile_prog_args" $xml_conf`
    pre_exec=`xmlstarlet sel -t -v "/xml/testCaseList/testCase[@id='$search_expr']/pre_exec" $xml_conf`
    pre_exec_args=`xmlstarlet sel -t -v "/xml/testCaseList/testCase[@id='$search_expr']/pre_exec_args" $xml_conf`
    main_exec=`xmlstarlet sel -t -v "/xml/testCaseList/testCase[@id='$search_expr']/main_exec" $xml_conf`
    main_exec_args=`xmlstarlet sel -t -v "/xml/testCaseList/testCase[@id='$search_expr']/main_exec_args" $xml_conf`
    search_expr_true=`xmlstarlet sel -t -v "/xml/testCaseList/testCase[@id='$search_expr']/search_expr_true" $xml_conf`
    search_expr_false=`xmlstarlet sel -t -v "/xml/testCaseList/testCase[@id='$search_expr']/search_expr_false" $xml_conf`
    nruns=`xmlstarlet sel -t -v "/xml/testCaseList/testCase[@id='$search_expr']/nruns" $xml_conf`
    
    echo "class = $class"
    echo "name = $name"
    echo "Description = $desc"
    echo "pre_compile_prog = $pre_compile_prog"
    echo "compile_prog = $compile_prog"
    echo "pre_exec = $pre_exec"
    echo "pre_exec_args = $pre_exec_args"
    echo "main_exec = $main_exec"
    echo "main_exec_args = $main_exec_args"
    echo "search_expr_true = $search_expr_true"
    echo "search_expr_false = $search_expr_false"
    echo "nruns = $nruns"

    #eval $pre_exec

    search_array_true=()

    IFS=\"                  #set the shell's field separator
    set -f                  #don't try to glob 
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
        test_compile "$name" "$compile_prog" "$compile_prog_args" "$pre_exec" "$pre_exec_args" "$main_exec" "$main_exec_args" "search_array_true[@]" "$search_expr_false" "$nruns" "$pre_compile_prog" "$class"
    elif  "$class" == "execution" ]; then
        test_compile_and_run "$name" "$compile_prog" "$compile_prog_args" "$pre_exec" "$pre_exec_args" "$main_exec" "$main_exec_args" "search_array_true[@]" "$search_expr_false" "$nruns" "$pre_compile_prog" "$class"
    else
        echo "Unexpected class of test case...Exiting...."
    fi

    done
    
    

}

main

xUnit_write "$results_file"

echo "Test Results are written to $results_file"

exit




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
#$11 -> pre compile program



#oaisim tests
search_array=("Received RRCConnectionReconfigurationComplete from UE 0")
test_compile_and_run 010200 "oaisim_nos1" "" "$OPENAIR_DIR/cmake_targets/tools/init_nas_nos1" "" "oaisim_nos1" " -O $OPENAIR_TARGETS/PROJECTS/GENERIC-LTE-EPC/CONF/enb.band7.generic.oaisim.local_no_mme.conf -A AWGN -b1 -u1 -n100" "search_array[@]" "segmentation fault|assertion|exiting|fatal" 3

search_array=("Received RRCConnectionReconfigurationComplete from UE 0")
test_compile_and_run 010201 "oaisim_nos1" "" "$OPENAIR_DIR/cmake_targets/tools/init_nas_nos1" "" "oaisim_nos1" " -O $OPENAIR_TARGETS/PROJECTS/GENERIC-LTE-EPC/CONF/enb.band7.generic.oaisim.local_no_mme.conf -A AWGN -b1 -u1 -a -n100" "search_array[@]" "segmentation fault|assertion|exiting|fatal" 3

search_array=("Received RRCConnectionReconfigurationComplete from UE 0" "Received RRCConnectionReconfigurationComplete from UE 1" "Received RRCConnectionReconfigurationComplete from UE 2")
test_compile_and_run 010202 "oaisim_nos1" "" "$OPENAIR_DIR/cmake_targets/tools/init_nas_nos1" "" "oaisim_nos1" " -O $OPENAIR_TARGETS/PROJECTS/GENERIC-LTE-EPC/CONF/enb.band7.generic.oaisim.local_no_mme.conf -A AWGN -b1 -u3 -n100" "search_array[@]" "segmentation fault|assertion|exiting|fatal" 3

search_array=("Received RRCConnectionReconfigurationComplete from UE 0" "Received RRCConnectionReconfigurationComplete from UE 1" "Received RRCConnectionReconfigurationComplete from UE 2")
test_compile_and_run 010203 "oaisim_nos1" "" "$OPENAIR_DIR/cmake_targets/tools/init_nas_nos1" "" "oaisim_nos1" " -O $OPENAIR_TARGETS/PROJECTS/GENERIC-LTE-EPC/CONF/enb.band7.generic.oaisim.local_no_mme.conf -A AWGN -b1 -u3 -a -n100" "search_array[@]" "segmentation fault|assertion|exiting|fatal" 3

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




#test_compile_and_run 0200 "oaisim_nos1" "" "$OPENAIR_DIR/cmake_targets/tools/init_nas_nos1" "" "oaisim_nos1" " -O /home/calisson/rohit/oai_snav/taets/PROJECTS/GENERIC-LTE-EPC/CONF/enb.band7.generic.oaisim.local_no_mme.conf -b1 -u1 -a " "RRC_CONN" 3

#run_test 0200 "$dbin/oaisim.r8 -a -A AWGN -n 100" false grep -q '(Segmentation.fault)|(Exiting)|(FATAL)'

#run_test 0201 "$dbin/oaisim.r8 -a -A AWGN -n 100" false fgrep -q '[E]'

# write the test results into a file

xUnit_write "$results_file"

echo "Test Results are written to $results_file"



