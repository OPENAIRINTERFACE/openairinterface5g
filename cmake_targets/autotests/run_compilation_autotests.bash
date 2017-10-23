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

tdir=$OPENAIR_DIR/cmake_targets/autotests

results_file=$tdir/log/compilation_autotests.xml

# include the jUnit-like logging functions
source $OPENAIR_DIR/cmake_targets/tools/test_helper

test_compile() {
    xUnit_start
    test_name=$1.$2
    compile_prog=$2
    exec_prog=$3
    build_dir=$tdir/$1/build
    log_file=$tdir/log/test.$1.$2.$5.txt
    target=$5
    echo "Compiling test case $test_name. Log file = $log_file"
    rm -fr $build_dir
    mkdir -p $tdir/$1/build
    cd $build_dir
    {
        cmake ..
        rm -f $exec_prog
        make -j`nproc` $compile_prog
    } >> $log_file 2>&1
    if [ -s $exec_prog ] ; then
        cp $exec_prog $tdir/bin/`basename $exec_prog`.$target.$test_name
        echo_success "$test_name $exec_prog $target compiled"
        xUnit_success "compilation" $test_name "PASS" 1 "$results_file"
    else
        echo_error "$test_name $exec_prog $target compilation failed"
        xUnit_fail "compilation" $test_name "FAIL" 1 "$results_file"
    fi
}

mkdir -p $tdir/bin $tdir/log

updated=$(svn st -q $OPENAIR_DIR)
if [ "$updated" != "" ] ; then
	echo_warning "some files are not in svn:\n $updated"
fi

cd $tdir 

test_compile \
    010101 oaisim_nos1 \
    oaisim_nos1  $tdir/bin/oaisim.r8 rel8.nos1

test_compile \
    010102 oaisim_nos1 \
    oaisim_nos1  $tdir/bin/oaisim.r8.nas rel8.nos1.nas

cp $tdir/010103/CMakeLists.txt.Rel8  $tdir/010103/CMakeLists.txt 
test_compile \
    010103 lte-softmodem \
    lte-softmodem  $tdir/bin/lte-softmodem.r8.rf Rel8.EXMIMO

cp $tdir/010103/CMakeLists.txt.Rel10  $tdir/010103/CMakeLists.txt   
test_compile \
    010103 lte-softmodem \
    lte-softmodem  $tdir/bin/lte-softmodem.r10.rf Rel10.EXMIMO

cp $tdir/010103/CMakeLists.txt.USRP  $tdir/010103/CMakeLists.txt   
test_compile \
    010103 lte-softmodem \
    lte-softmodem  $tdir/bin/lte-softmodem.r10.rf Rel10.USRP

test_compile \
    010104 dlsim \
    dlsim  $tdir/bin/dlsim dlsim.Rel8

test_compile \
    010104 ulsim \
    ulsim  $tdir/bin/ulsim ulsim.Rel8

test_compile \
    010104 pucchsim \
    pucchsim  $tdir/bin/pucchsim pucchsim.Rel8

test_compile \
    010104 prachsim \
    prachsim  $tdir/bin/prachsim prachsim.Rel8

test_compile \
    010104 pdcchsim \
    pdcchsim  $tdir/bin/pdcchsim pdcchsim.Rel8

test_compile \
    010104 pbchsim \
    pbchsim  $tdir/bin/pbchim pbchsim.Rel8

test_compile \
    010104 mbmssim \
    mbmssim  $tdir/bin/mbmssim mbmssim.Rel8

test_compile \
    010104 test_secu_knas_encrypt_eia1 \
    test_secu_knas_encrypt_eia1  $tdir/bin/test_secu_knas_encrypt_eia1 test_secu_knas_encrypt_eia1.Rel10

test_compile \
    010104 test_secu_kenb \
    test_secu_kenb  $tdir/bin/test_secu_kenb test_secu_kenb.Rel10

test_compile \
    010104 test_aes128_ctr_encrypt \
    test_aes128_ctr_encrypt  $tdir/bin/test_aes128_ctr_encrypt test_aes128_ctr_encrypt.Rel10

test_compile \
    010104 test_aes128_ctr_decrypt \
    test_aes128_ctr_decrypt  $tdir/bin/test_aes128_ctr_decrypt test_aes128_ctr_decrypt.Rel10

test_compile \
    010104 test_secu_knas_encrypt_eea2 \
    test_secu_knas_encrypt_eea2  $tdir/bin/test_secu_knas_encrypt_eea2 test_secu_knas_encrypt_eea2.Rel10

test_compile \
    010104 test_secu_knas \
    test_secu_knas  $tdir/bin/test_secu_knas test_secu_knas.Rel10

test_compile \
    010104 test_secu_knas_encrypt_eea1 \
    test_secu_knas_encrypt_eea1  $tdir/bin/test_secu_knas_encrypt_eea1 test_secu_knas_encrypt_eea1.Rel10

test_compile \
    010104 test_kdf \
    test_kdf  $tdir/bin/test_kdf test_kdf.Rel10

test_compile \
    010104 test_aes128_cmac_encrypt \
    test_aes128_cmac_encrypt  $tdir/bin/test_aes128_cmac_encrypt test_aes128_cmac_encrypt.Rel10

test_compile \
    010104 test_secu_knas_encrypt_eia2 \
    test_secu_knas_encrypt_eia2  $tdir/bin/test_secu_knas_encrypt_eia2 test_secu_knas_encrypt_eia2.Rel10

test_compile \
    010106 oaisim \
    oaisim  $tdir/bin/oaisim.r8.itti Rel8.itti

test_compile \
    010107 oaisim_nos1 \
    oaisim_nos1  $tdir/bin/oaisim.r10 Rel10.nos1

test_compile \
    010108 oaisim \
    oaisim  $tdir/bin/oaisim.r10.itti rel10.itti

#test_compile \  LG: RAL REMOVED
#    test.0114 oaisim \
#    oaisim  $tdir/bin/oaisim.r8.itti.ral rel8.itti.ral

#test_compile \  LG: RAL REMOVED
#    test.0115 oaisim \
#    oaisim  $tdir/bin/oaisim.r10.itti.ral rel10.itti.ral 

test_compile \
    010120 nasmesh \
    CMakeFiles/nasmesh/nasmesh.ko $tdir/bin/nasmesh.ko 

test_compile \
    010130 rrh_gw \
    rrh_gw $tdir/bin/rrh_gw

# write the test results into a file
xUnit_write "$results_file"

echo "Test Results are written to $results_file"
