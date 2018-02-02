#! /bin/bash
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
################################################################################
# file build_all.bash
# brief
# author Lionel Gauthier
# company Eurecom
# email: lionel.gauthier@eurecom.fr
#
###########################################################
THIS_SCRIPT_PATH=$(dirname $(readlink -f $0))
###########################################################

echo_success "\n###############################"
echo_success "# Check installed utils and libs"
echo_success "###############################"
test_command_install_package "gccxml" "gccxml" "--force-yes"
test_command_install_package "iptables" "iptables"
#test_command_install_package "ebtables" "ebtables" "--force-yes"
test_command_install_package "ip" "iproute"
test_install_package "openssl"
test_install_package "libblas-dev"
# for itti analyser
test_install_package "libgtk-3-dev"
test_install_package "libxml2"
test_install_package "libxml2-dev"
test_install_package "libforms-bin" "--force-yes"
test_install_package "libforms-dev"
test_install_package "libatlas-dev"
test_install_package "libatlas-base-dev"
test_install_package "libpgm-5.1-0" "--force-yes"
test_install_package "libpgm-dev"   "--force-yes"
test_install_package linux-headers-`uname -r`
test_install_package "tshark"       "--force-yes"
# for ODTONE git clone
test_install_package "git"

test_install_asn1c_4_rrc_cellular


echo_success "\n###############################"
echo_success "# COMPILE oaisim"
echo_success "###############################"
cd $OPENAIR_TARGETS/SIMU/USER
echo_success "Executing: make oaisim NAS=1 OAI_NW_DRIVER_TYPE_ETHERNET=1 ENABLE_ITTI=1 OPENAIR2=1  Rel10=1 -j`grep -c ^processor /proc/cpuinfo `"
make oaisim NAS=1 OAI_NW_DRIVER_TYPE_ETHERNET=1 ENABLE_ITTI=1 OPENAIR2=1  Rel10=1 -j`grep -c ^processor /proc/cpuinfo `
if [[ $? -eq 2 ]] ; then
    exit 1
fi

echo_success "\n###############################"
echo_success "# COMPILE IP kernel drivers"
echo_success "###############################"
echo_success "Compiling IP Drivers"
cd $OPENAIR2_DIR
make naslite_netlink_ether.ko
cd $OPENAIR2_DIR/NAS/DRIVER/LITE/RB_TOOL/
make


echo_success "\n###############################"
echo_success "# COMPILE ITTI ANALYSER"
echo_success "###############################"
#cd  $OPENAIR_DIR/common/utils/itti_analyzer
#if [ ! -f $OPENAIR_DIR/common/utils/itti_analyzer/Makefile ]
#    then
#        ./autogen.sh
#        ./configure
#    fi
#make install



