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
# file utils.bash
# brief
# author Lionel Gauthier
# company Eurecom
# email: lionel.gauthier@eurecom.fr
#

black='\E[30m'
red='\E[31m'
green='\E[32m'
yellow='\E[33m'
blue='\E[34m'
magenta='\E[35m'
cyan='\E[36m'
white='\E[37m'
reset_color='\E[00m'

ROOT_UID=0
E_NOTROOT=67

HOSTNAME=$(hostname -f)
if [ $? -eq 1 ]
then
    HOSTNAME=$(hostname)
fi

IPTABLES=`which iptables`

trim ()
{
    echo "$1" | sed -n '1h;1!H;${;g;s/^[ \t]*//g;s/[ \t]*$//g;p;}'
}

trim2()
{
    local var=$@
    var="${var#"${var%%[![:space:]]*}"}"   # remove leading whitespace characters
    var="${var%"${var##*[![:space:]]}"}"   # remove trailing whitespace characters
    echo -n "$var"
}

cecho()   # Color-echo
# arg1 = message
# arg2 = color
{
    local default_msg="No Message."
    message=${1:-$default_msg}
    color=${2:-$green}
    echo -e -n "$color$message$reset_color"
    echo
    return
}

echo_error() {
    local my_string=""
    until [ -z "$1" ]
    do
        my_string="$my_string$1"
        shift
        done
        cecho "$my_string" $red
}

echo_fatal() {
    local my_string=""
    until [ -z "$1" ]
    do
        my_string="$my_string$1"
        shift
        done
        echo_error "$my_string"
    exit -1
}

echo_warning() {
    local my_string=""
    until [ -z "$1" ]
    do
        my_string="$my_string$1"
        shift
    done
    cecho "$my_string" $yellow
}

echo_success() {
    local my_string=""
    until [ -z "$1" ]
    do
        my_string="$my_string$1"
        shift
    done
    cecho "$my_string" $green
}

bash_exec() {
    output=$($1 2>&1)
    result=$?
    if [ $result -eq 0 ]
    then
        echo_success "$1"
    else
        echo_error "$1: $output"
    fi
}

extract() {
    if [ -f $1 ] ; then
        case $1 in
            *.tar.bz2)    tar xvjf $1        ;;
            *.tar.gz)     tar xvzf $1        ;;
            *.bz2)        bunzip2  $1        ;;
            *.rar)        unrar    $1        ;;
            *.gz)         gunzip   $1        ;;
            *.tar)        tar xvf  $1        ;;
            *.tbz2)       tar xvjf $1        ;;
            *.tgz)        tar xvzf $1        ;;
            *.zip)        unzip    $1        ;;
            *.Z)          uncompress $1      ;;
            *.7z)         7z x     $1        ;;
            *)            echo_error "'$1' cannot be extracted via >extract<" ; return 1;;
        esac
    else
        echo_error "'$1' is not a valid file"
        return 1
    fi
    return 0
}

rotate_log_file () {
    FULLPATH=$1
    if [ -f $FULLPATH ]; then
        FILENAME=${FULLPATH##*/}
        FILEEXTENSION=${FILENAME##*.}
        BASEDIRECTORY=${FULLPATH%$FILENAME}
        if [ "a$BASEDIRECTORY" == "a" ]; then
            BASEDIRECTORY='.'
        fi
        FILENAME_NO_EXT=$(echo "$FILENAME" | sed 's/\.[^\.]*$//')

        # use file last modification time
        TIMESTAMP=`date -r $FULLPATH +%Y-%m-%d.%Hh_%Mm_%Ss`
        
        NEWLOGFILE=$TIMESTAMP.$FILENAME_NO_EXT.$FILEEXTENSION
        mv $FULLPATH /tmp/$NEWLOGFILE
        cat /dev/null > $FULLPATH
        sync
        gzip -c --name -f -9 /tmp/$NEWLOGFILE > $BASEDIRECTORY/$NEWLOGFILE.gz &
    fi
}

set_openair() {
    fullpath=`readlink -f $BASH_SOURCE`
    [ -f "/.$fullpath" ] || fullpath=`readlink -f $PWD/$fullpath`
    openair_path=${fullpath%/targets/*}

    export OPENAIR_DIR=$openair_path
    export OPENAIR_DIR=$openair_path
    export OPENAIR1_DIR=$openair_path/openair1
    export OPENAIR2_DIR=$openair_path/openair2
    export OPENAIR3_DIR=$openair_path/openair3
    export OPENAIR3_DIR=$openair_path/openair3
    export OPENAIR_TARGETS=$openair_path/targets
    export CDPATH=$CDPATH:$openair_path
}

wait_process_started () {
    if  [ -z "$1" ]
    then
        echo_error "WAITING FOR PROCESS START: NO PROCESS"
        return 1
    fi
    ps -C $1 > /dev/null 2>&1
    while [ $? -ne 0 ]; do
        echo_warning "WAITING FOR $1 START"
        sleep 2
        ps -C $1 > /dev/null 2>&1
    done
    echo_success "PROCESS $1 STARTED"
    return 0
}

is_process_started () {
    if  [ -z "$1" ]
    then
        echo_error "WAITING FOR PROCESS START: NO PROCESS"
        return 1
    fi
    ps -C $1 > /dev/null 2>&1
    if [ $? -ne 0 ]
    then
        echo_success "PROCESS $1 NOT STARTED"
        return 1
    fi
    echo_success "PROCESS $1 STARTED"
    return 0
}

assert() {
    # If condition false
    # exit from script with error message
    E_PARAM_ERR=98
    E_PARAM_FAILED=99

    if [ -z "$2" ] # Not enought parameters passed.
    then
        return $E_PARAM_ERR
    fi

    lineno=$2
    if [ ! $1 ]
    then
        echo_error "Assertion failed:  \"$1\""
        echo_fatal "File \"$0\", line $lineno"
    fi
}


test_install_package() {
  # usage: test_install_package package_name
  if [ $# -eq 1 ]; then
      dpkg -s "$1" > /dev/null 2>&1 && {
          echo "$1 is installed."
      } || {
          echo "$1 is not installed."
          apt-get install --assume-yes $1 
      }
  fi
}

test_command_install_script() {
  # usage: test_command_install_script searched_binary script_to_be_invoked_if_binary_not_found
  command -v $1 >/dev/null 2>&1 || { echo_warning "Program $1 is not installed. Trying installing it." >&2; bash $2; command -v $1 >/dev/null 2>&1 || { echo_fatal "Program $1 is not installed. Aborting." >&2; };}
  echo_success "$1 available"
}




check_for_epc_executable() {
    if [ ! -f $OPENAIR3_DIR/OPENAIRMME/objs/OAI_EPC/oai_epc ]
        then
        echo_error "Cannot find oai_epc executable object in directory $OPENAIR3_DIR/OPENAIRMME/objs/OAI_EPC/"
        echo_fatal "Please make sure you have compiled OAI EPC with --enable-standalone-epc option"
        fi
}

check_for_sgw_executable() {
    if [ ! -f $OPENAIR3_DIR/OPENAIRMME/objs/OAI_SGW/oai_sgw ]
    then
        echo_error "Cannot find oai_sgw executable object in directory $OPENAIR3_DIR/OPENAIRMME/objs/OAI_SGW/"
        echo_fatal "Please make sure you have compiled OAI EPC without --enable-standalone-epc option"
    fi
}

check_for_mme_executable() {
    if [ ! -f $OPENAIR3_DIR/OPENAIRMME/objs/OAISIM_MME/oaisim_mme ]
    then
        echo_error "Cannot find oai_sgw executable object in directory $OPENAIR3_DIR/OPENAIRMME/objs/OAISIM_MME/"
        echo_fatal "Please make sure you have compiled OAI EPC without --enable-standalone-epc option"
    fi
}

check_for_root_rights() {
    if [[ $EUID -ne 0 ]]; then
        echo_fatal "This script must be run as root" 1>&2
    fi
}



start_openswitch_daemon() {
  rmmod -s bridge
  if [[ -e "/lib/modules/`uname -r`/extra/openvswitch.ko" ]] ; then
      bash_exec "insmod /lib/modules/`uname -r`/extra/openvswitch.ko" > /dev/null 2>&1
  else
      echo_error "/lib/modules/`uname -r`/extra/openvswitch.ko not found"
      bash $OPENAIR3_DIR/SCRIPTS/install_openvswitch1.9.0.bash
  fi
  is_process_started "ovsdb-server"
  if [ $? -ne 0 ]
  then
      ovsdb-server --remote=punix:/usr/local/var/run/openvswitch/db.sock --remote=db:Open_vSwitch,manager_options --pidfile --detach
      wait_process_started "ovsdb-server"
  fi
  # To be done after installation
  # ovs-vsctl    --no-wait init
  is_process_started "ovs-vswitchd"
  if [ $? -ne 0 ]
  then
      ovs-vswitchd --pidfile --detach
      wait_process_started "ovs-vswitchd"
  fi
}

stop_openswitch_daemon() {
    pkill ovs-vswitchd
    pkill ovsdb-server
    sync
    if  ! is_process_started ovs-vswitchd  ; then
        pkill -9 ovs-vswitchd
    fi
    if ! is_process_started ovsdb-server ; then
        pkill -9 ovsdb-server
    fi
    rmmod -f openvswitch
}


check_epc_s6a_certificate() {
    if [ -d /usr/local/etc/freeDiameter ]
    then
        if [ -f /usr/local/etc/freeDiameter/user.cert.pem ]
        then
            full_hostname=`cat /usr/local/etc/freeDiameter/user.cert.pem | grep "Subject" | grep "CN" | cut -d '=' -f6`
            if [ a$full_hostname == a`hostname`.${1:-'eur'} ]
            then
                echo_success "MME S6A: Found valid certificate in /usr/local/etc/freeDiameter"
                return 1
            fi
        fi
    fi
    echo_error "MME S6A: Did not find valid certificate in /usr/local/etc/freeDiameter"
    echo_warning "MME S6A: generatting new certificate in /usr/local/etc/freeDiameter..."
    cd $OPENAIR3_DIR/S6A/freediameter
    ./make_certs.sh ${1:-'eur'}
    check_epc_s6a_certificate ${1:-'eur'}
    return 1
}

check_hss_s6a_certificate() {
        if [ -f $OPENAIR3_DIR/OPENAIRHSS/conf/hss.cert.pem ]
        then
            full_hostname=`cat $OPENAIR3_DIR/OPENAIRHSS/conf/hss.cert.pem | grep "Subject" | grep "CN" | cut -d '=' -f6`
            # we should replace 'hss' with hostname 
            if [ a$full_hostname == ahss.${1:-'eur'} ]
            then
                echo_success "HSS S6A: Found valid certificate in $OPENAIR3_DIR/OPENAIRHSS/conf/"
                return 1
            fi
        fi
    echo_error "HSS S6A: Did not find valid certificate in $OPENAIR3_DIR/OPENAIRHSS/conf"
    echo_warning "HSS S6A: generatting new certificate in $OPENAIR3_DIR/OPENAIRHSS/conf..."
    cd $OPENAIR3_DIR/OPENAIRHSS/conf
    ./make_certs.sh ${1:-'eur'}
    check_hss_s6a_certificate ${1:-'eur'}
    return 1
}

generate_ssh_keys() {
    ssh-keygen -t dsa
}  
  
# arg 1 is dest machine
copy_ssh_keys() {
    ssh-copy-id -i ~/.ssh/id_dsa.pub $LOGNAME@$1
}

# arg 1 is mysql user      (root)
# arg 2 is mysql password  (linux)
create_hss_database(){
    EXPECTED_ARGS=2
    E_BADARGS=65
    MYSQL=`which mysql`
    
    if [ $# -ne $EXPECTED_ARGS ]
    then
        echo_fatal "Usage: $0 dbuser dbpass"
    fi

    set_openair
    
    Q1="CREATE DATABASE IF NOT EXISTS ${BTICK}oai_db${BTICK};"
    SQL="${Q1}"
    $MYSQL -u $1 --password=$2 -e "$SQL"
    if [ $? -ne 0 ]; then
       echo_error "oai_db creation failed"
    else
       echo_success "oai_db creation succeeded"
    fi
    
    $MYSQL -u $1 --password=$2 oai_db < $OPENAIR3_DIR/OPENAIRHSS/db/oai_db.sql
    if [ $? -ne 0 ]; then
       echo_error "oai_db tables creation failed"
    else
       echo_success "oai_db tables creation succeeded"
    fi
    
    Q1="GRANT ALL PRIVILEGES ON *.* TO 'hssadmin'@'%' IDENTIFIED BY 'admin' WITH GRANT OPTION;"
    Q2="FLUSH PRIVILEGES;"
    SQL="${Q1}${Q2}"
    $MYSQL -u $1 --password=$2 -e "$SQL"
    if [ $? -ne 0 ]; then
       echo_error "hssadmin permissions failed"
    else
       echo_success "hssadmin permissions succeeded"
    fi
}

check_install_hss_software() {
    test_install_package autoconf 
    test_install_package automake 
    test_install_package bison 
    test_install_package build-essential
    test_install_package cmake
    test_install_package cmake-curses-gui 
    test_install_package dkms
    test_install_package flex 
    test_install_package gawk
    test_install_package gcc
    test_install_package gdb 
    test_install_package guile-2.0-dev 
    test_install_package g++
    test_install_package libgmp-dev 
    test_install_package libgcrypt11-dev 
    test_install_package libidn11-dev 
    test_install_package libidn2-0-dev 
    test_install_package libmysqlclient-dev
    test_install_package libtasn1-3-dev 
    test_install_package libsctp1 
    test_install_package libsctp-dev 
    test_install_package libxml2-dev 
    test_install_package linux-headers-`uname -r` 
    test_install_package make
    test_install_package mysql-client
    test_install_package mysql-common 
    test_install_package mysql-server
    test_install_package openssh-client
    test_install_package openssh-server
    test_install_package phpmyadmin
    test_install_package python-dev 
    test_install_package sshfs
    test_install_package swig 
    test_install_package unzip 
    test_install_package valgrind 
}

check_install_vbox_software() {
    test_install_package dkms
    test_install_package openssh-client
    test_install_package openssh-server
    test_install_package sshfs
    test_install_package virtualbox
    test_install_package virtualbox-dkms
    test_install_package virtualbox-guest-additions-iso
    test_install_package virtualbox-qt
    test_install_package virtualbox-guest-utils
}

check_install_epc_software() {
    test_install_package autoconf
    test_install_package automake
    test_install_package bison
    test_install_package build-essential
    test_install_package check
    test_install_package cmake
    test_install_package cmake-curses-gui
    test_install_package ethtool
    test_install_package flex
    test_install_package g++
    test_install_package gawk
    test_install_package gcc
    test_install_package gccxml
    test_install_package gdb 
    test_install_package guile-2.0-dev
    test_install_package gtkwave
    test_install_package iperf
    test_install_package iproute
    test_install_package iptables
    test_install_package iptables-dev
    test_install_package libatlas-base-dev
    test_install_package libatlas-dev
    test_install_package libblas
    test_install_package libblas-dev
    if [ x$UTILS_ARCH == x64 ]; then
        test_install_package libconfig8-dev
    else
        test_install_package libconfig-dev
    fi
    test_install_package libforms-bin
    test_install_package libforms-dev
    test_install_package libgcrypt11-dev
    test_install_package libgmp-dev
    test_install_package libgtk-3-dev
    test_install_package libidn11-dev
    test_install_package libidn2-0-dev
    test_install_package libmysqlclient-dev
    test_install_package libpgm-dev
    test_install_package libpthread-stubs0-dev
    test_install_package libsctp1
    test_install_package libsctp-dev
    test_install_package libtasn1-3-dev
    test_install_package libxml2
    test_install_package libxml2-dev
    test_install_package linux-headers-`uname -r`
    test_install_package make
    test_install_package openssh-client
    test_install_package openssh-server
    test_install_package openssl
    test_install_package openvpn
    test_install_package pkg-config
    test_install_package python-dev
    test_install_package sshfs
    test_install_package subversion
    test_install_package swig
    test_install_package tshark
    test_install_package uml-utilities
    test_install_package unzip
    test_install_package valgrind
    test_install_package vlan

    if [ ! -d /usr/local/etc/freeDiameter ]
        then
           # This script make certificates also
            cd $OPENAIR3_DIR/S6A/freediameter && ./install_freediameter.sh
        else
            echo_success "freediameter is installed"
            check_epc_s6a_certificate
    fi

    test_command_install_script   "asn1c" "$OPENAIR3_DIR/SCRIPTS/install_asn1c_0.9.24.modified.bash"

    # One mor check about version of asn1c
    ASN1C_COMPILER_REQUIRED_VERSION_MESSAGE="ASN.1 Compiler, v0.9.24"
    ASN1C_COMPILER_VERSION_MESSAGE=`asn1c -h 2>&1 | grep -i ASN\.1\ Compiler`
    ##ASN1C_COMPILER_VERSION_MESSAGE=`trim $ASN1C_COMPILER_VERSION_MESSAGE`
    if [ "$ASN1C_COMPILER_VERSION_MESSAGE" != "$ASN1C_COMPILER_REQUIRED_VERSION_MESSAGE" ]
    then
        diff <(echo -n "$ASN1C_COMPILER_VERSION_MESSAGE") <(echo -n "$ASN1C_COMPILER_REQUIRED_VERSION_MESSAGE")
        echo_error "Version of asn1c is not the required one, do you want to install the required one (overwrite installation) ? (Y/n)"
        echo_error "$ASN1C_COMPILER_VERSION_MESSAGE"
        while read -r -n 1 -s answer; do
            if [[ $answer = [YyNn] ]]; then
                [[ $answer = [Yy] ]] && $OPENAIR3_DIR/SCRIPTS/install_asn1c_0.9.24.modified.bash
                [[ $answer = [Nn] ]] && echo_error "Version of asn1c is not the required one, exiting." && exit 1
                break
            fi
        done
    fi
}

function cgrp()
{
    OPTIND=1
    local case=""
    local usage="cgrp: find string in files.
    Usage: cgrp \"pattern\" "

    set_openair
    find $OPENAIR2_DIR    -name *.c -exec grep  --color=auto -Hni $1 {} \;
    find $OPENAIR1_DIR    -name *.c -exec grep  --color=auto -Hni $1 {} \;
    find $OPENAIR3_DIR    -name *.c -exec grep  --color=auto -Hni $1 {} \;
    find $OPENAIR3_DIR   -name *.c -exec grep  --color=auto -Hni $1 {} \;
    find $OPENAIR_TARGETS -name *.c -exec grep  --color=auto -Hni $1 {} \;
}

function hgrp()
{
    OPTIND=1
    local case=""
    local usage="hgrp: find string in files.
    Usage: hgrp \"pattern\" "
    set_openair
    find $OPENAIR2_DIR    -name *.h -exec grep  --color=auto -Hni $1 {} \;
    find $OPENAIR1_DIR    -name *.h -exec grep  --color=auto -Hni $1 {} \;
    find $OPENAIR3_DIR    -name *.h -exec grep  --color=auto -Hni $1 {} \;
    find $OPENAIR3_DIR   -name *.h -exec grep  --color=auto -Hni $1 {} \;
    find $OPENAIR_TARGETS -name *.h -exec grep  --color=auto -Hni $1 {} \;
}

function svn_find_str_in_file_history()
{
    for REV in `svn log $1 | grep ^r[0-9] | awk '{print $1}'`; do 
        svn cat $1 -r $REV | grep $2
        if [ $? -eq 0 ]; then 
            echo "$REV"
        fi 
    done
}

compile_hss() {
    cd $OPENAIR3_DIR/OPENAIRHSS
    OBJ_DIR=`find . -maxdepth 1 -type d -iname obj*`
    if [ ! -n "$OBJ_DIR" ]
    then
        OBJ_DIR="objs"
        bash_exec "mkdir -m 777 ./$OBJ_DIR"
        echo_success "Created $OBJ_DIR directory"
    else
        OBJ_DIR=`basename $OBJ_DIR`
    fi
    if [ ! -f $OBJ_DIR/Makefile ]
    then
        if [ ! -n "m4" ]
        then
            mkdir -m 777 m4
        fi
        echo_success "Invoking autogen"
        bash_exec "./autogen.sh"
        cd ./$OBJ_DIR
        echo_success "Invoking configure"
        ../configure 
    else
        cd ./$OBJ_DIR
    fi
    if [ -f Makefile ]
    then
        echo_success "Compiling..."
        make -j `cat /proc/cpuinfo | grep processor | wc -l`
        if [ $? -ne 0 ]; then
            echo_error "Build failed, exiting"
            exit 1
        fi
    else
        echo_error "Configure failed, exiting"
        exit 1
    fi
}


compile_epc() {
    cd $OPENAIR3_DIR
    OBJ_DIR=`find . -maxdepth 1 -type d -iname obj*`
    if [ ! -n "$OBJ_DIR" ]
    then
        OBJ_DIR="objs"
        bash_exec "mkdir -m 777 ./$OBJ_DIR"
        echo_success "Created $OBJ_DIR directory"
    else
        OBJ_DIR=`basename $OBJ_DIR`
    fi
    if [ ! -f $OBJ_DIR/Makefile ]
    then
        if [ ! -n "m4" ]
        then
            mkdir -m 777 m4
        fi
        echo_success "Invoking autogen"
        bash_exec "./autogen.sh"
        cd ./$OBJ_DIR
        echo_success "Invoking configure"
        ../configure --enable-standalone-epc --enable-raw-socket-for-sgi  LDFLAGS=-L/usr/local/lib
    else
        cd ./$OBJ_DIR
    fi

    pkill oai_epc
    pkill tshark

    if [ -f Makefile ]
    then
        echo_success "Compiling..."
        make -j `cat /proc/cpuinfo | grep processor | wc -l`
        if [ $? -ne 0 ]; then
            echo_error "Build failed, exiting"
            exit 1
        fi
    else
        echo_error "Configure failed, exiting"
        exit 1
    fi
}

###########################################################
IPTABLES=/sbin/iptables
THIS_SCRIPT_PATH=$(dirname $(readlink -f $0))
declare -x OPENAIR_DIR=""
declare -x OPENAIR1_DIR=""
declare -x OPENAIR2_DIR=""
declare -x OPENAIR3_DIR=""
declare -x OPENAIR3_DIR=""
declare -x OPENAIR_TARGETS=""
###########################################################

set_openair
cecho "OPENAIR_DIR     = $OPENAIR_DIR" $green
cecho "OPENAIR_DIR    = $OPENAIR_DIR" $green
cecho "OPENAIR1_DIR    = $OPENAIR1_DIR" $green
cecho "OPENAIR2_DIR    = $OPENAIR2_DIR" $green
cecho "OPENAIR3_DIR    = $OPENAIR3_DIR" $green
cecho "OPENAIR3_DIR   = $OPENAIR3_DIR" $green
cecho "OPENAIR_TARGETS = $OPENAIR_TARGETS" $green

export UTILS_ARCH=$(uname -m | sed 's/x86_//;s/i[3-6]86/32/')
if [ -f /etc/lsb-release ]; then
    . /etc/lsb-release
    export OS=$DISTRIB_ID
    export VER=$DISTRIB_RELEASE
elif [ -f /etc/debian_version ]; then
    export OS=Debian  # XXX or Ubuntu??
    export VER=$(cat /etc/debian_version)
elif [ -f /etc/redhat-release ]; then
    # TODO add code for Red Hat and CentOS here
    ...
else
    export OS=$(uname -s)
    export VER=$(uname -r)
fi
