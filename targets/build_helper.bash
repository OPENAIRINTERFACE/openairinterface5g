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
# file build_helper.bash
# brief
# author Lionel Gauthier and Navid Nikaein
# company Eurecom
# email: lionel.gauthier@eurecom.fr and navid.nikaein@eurecom.fr
#
#######################################
#            Helper Func
######################################

declare ROOT_UID=0
declare E_NOTROOT=67
declare NUM_CPU=`cat /proc/cpuinfo | grep processor | wc -l`
declare OAI_INSTALLED=1
declare PWD=`pwd`
declare USER=`whoami`
declare BUILD_FROM_MAKEFILE=0
declare SUDO=''
declare PW=''
declare UBUNTU_REL=`lsb_release -r | cut  -f2`
declare UBUNTU_REL_NAME=`lsb_release -cs`

set_build_from_makefile(){
    BUILD_FROM_MAKEFILE=$1
}

check_for_root_rights() {

  #  if [[ $EUID -ne $ROOT_EUID ]]; then
    if [ $USER != "root" ]; then
        SUDO="sudo -E "
        echo "Run as a sudoers"
        return 1
    else
        echo  "Run as a root"
        return 0
    fi
}

test_install_package() {
  # usage: test_install_package package_name

 if [ $# -eq 1 ]; then
      dpkg -s "$1" > /dev/null 2>&1 && {
          echo "$1 is installed."
      } || {
          echo "$1 is not installed."
          OAI_INSTALLED=0
          $SUDO apt-get install -y $@
      }
  fi
}

test_uninstall_package() {

 if [ $# -eq 1 ]; then
      dpkg -s "$1" > /dev/null 2>&1 && {
          $SUDO apt-get remove --assume-yes $1
          echo "$1 is uninstalled."
      } || {
          echo "$1 is not installed."
      }
  fi
}
test_command_install_script() {
  # usage: test_command_install_script searched_binary script_to_be_invoked_if_binary_not_found
  command -v $1 >/dev/null 2>&1 || { echo_warning "Program $1 is not installed. Trying installing it." >&2; bash $2; command -v $1 >/dev/null 2>&1 || { echo_fatal "Program $1 is not installed. Aborting." >&2; };}
  echo_success "$1 available"
}


check_for_machine_type(){
    MACHINE_TYPE=`uname -m`
    if [ ${MACHINE_TYPE} = "x86_64" ]; then
        return 64  # 64-bit stuff here
    else
        if [ ${MACHINE_TYPE} = "i686" ]; then
            return 32 # 32-bit stuff here
        else
            return -1
        fi
    fi
}

####################################################
## OAI related functions
#####################################################

####################################################
# 1. install the required packages
####################################################

make_certs(){

 # for certtificate generation
    rm -rf demoCA
    mkdir -m 777 -p demoCA
    echo 01 > demoCA/serial
    touch demoCA/index.txt

    echo "creating the certificate"

    user=$(whoami)
    HOSTNAME=$(hostname -f)

    echo "Creating certificate for user '$HOSTNAME'"

# CA self certificate
    openssl req  -new -batch -x509 -days 3650 -nodes -newkey rsa:1024 -out cacert.pem -keyout cakey.pem -subj /CN=eur/C=FR/ST=PACA/L=Aix/O=Eurecom/OU=CM

   # openssl genrsa -out user.key.pem 1024
    openssl genrsa -out hss.key.pem 1024
    #openssl req -new -batch -out user.csr.pem -key user.key.pem -subj /CN=$HOSTNAME.eur/C=FR/ST=PACA/L=Aix/O=Eurecom/OU=CM
    openssl req -new -batch -out hss.csr.pem -key hss.key.pem -subj /CN=hss.eur/C=FR/ST=PACA/L=Aix/O=Eurecom/OU=CM
    openssl ca -cert cacert.pem -keyfile cakey.pem -in hss.csr.pem -out hss.cert.pem -outdir . -batch

    if [ ! -d /usr/local/etc/freeDiameter ];  then
        echo "Creating non existing directory: /usr/local/etc/freeDiameter/"
        $SUDO mkdir /usr/local/etc/freeDiameter/
    fi

    echo "Copying *.pem to /usr/local/etc/freeDiameter/"
    $SUDO cp *.pem /usr/local/etc/freeDiameter/
    mv *.pem bin/

# openssl genrsa -out ubuntu.key.pem 1024
# openssl req -new -batch -x509 -out ubuntu.csr.pem -key ubuntu.key.pem -subj /CN=ubuntu.localdomain/C=FR/ST=BdR/L=Aix/O=fD/OU=Tests
# openssl ca -cert cacert.pem -keyfile cakey.pem -in ubuntu.csr.pem -out ubuntu.cert.pem -outdir . -batch

}

check_install_nettle(){
    if [ ! -f ./.lock_oaibuild ]; then
	if [ $UBUNTU_REL = "12.04" ]; then
	    test_uninstall_package nettle-dev
	    test_uninstall_package nettle-bin

	    if [ ! -d /usr/local/src/ ]; then
		echo "/usr/local/src/ doesn't exist please create one"
		exit -1
	    fi

	    if [ ! -w /usr/local/src/ ];  then
		echo "You don't have permissions to write to /usr/local/src/, installing as a sudoer"
		#	exit -1
	    fi

	    cd /usr/local/src/

	    echo "Downloading nettle archive"

	    if [ -f nettle-2.5.tar.gz ]; then
		$SUDO rm -f nettle-2.5.tar.gz
	    fi
	    if [ -f nettle-2.5.tar ]; then
		$SUDO rm -f nettle-2.5.tar
	    fi
	    if [ -d nettle-2.5 ];  then
		$SUDO rm -rf nettle-2.5/
	    fi


	    $SUDO wget ftp://ftp.lysator.liu.se/pub/security/lsh/nettle-2.5.tar.gz
	    $SUDO gunzip nettle-2.5.tar.gz
	    $SUDO echo "Uncompressing nettle archive"
	    $SUDO tar -xf nettle-2.5.tar
	    cd nettle-2.5/
	    $SUDO ./configure --disable-openssl --enable-shared --prefix=/usr
	    if [ $? -ne 0 ]; then
		exit -1
	    fi
	    echo "Compiling nettle"
	    $SUDO make -j $NUM_CPU
	    $SUDO make check
	    $SUDO make install
	    cd ../
	fi
    fi
}

check_install_freediamter(){

    if [ $UBUNTU_REL = "12.04" ]; then

	if [ ! -d /usr/local/src/ ]; then
	    echo "/usr/local/src/ doesn't exist please create one"
	    exit -1
	fi

	if [ ! -w /usr/local/src/ ];  then
	    echo "You don't have permissions to write to /usr/local/src/, installing as a sudoer"
#	exit -1
	fi

	cd /usr/local/src/

	echo "Downloading nettle archive"

	if [ -f nettle-2.5.tar.gz ]; then
	    $SUDO rm -f nettle-2.5.tar.gz
	fi
	if [ -f nettle-2.5.tar ]; then
	    $SUDO rm -f nettle-2.5.tar
	fi
	if [ -d nettle-2.5 ];  then
	    $SUDO rm -rf nettle-2.5/
	fi


	$SUDO wget ftp://ftp.lysator.liu.se/pub/security/lsh/nettle-2.5.tar.gz
	$SUDO gunzip nettle-2.5.tar.gz
	$SUDO echo "Uncompressing nettle archive"
	$SUDO tar -xf nettle-2.5.tar
	cd nettle-2.5/
	$SUDO ./configure --disable-openssl --enable-shared --prefix=/usr
	if [ $? -ne 0 ]; then
	    exit -1
	fi
	echo "Compiling nettle"
	$SUDO make -j $NUM_CPU
	$SUDO make check
	$SUDO make install
	cd ../
    fi

    echo "Downloading gnutls archive"

    if [ -f gnutls-3.1.23.tar.xz ];  then
	$SUDO rm -f gnutls-3.1.23.tar.xz
    fi
    if [ -d gnutls-3.1.23/ ];  then
	$SUDO rm -rf gnutls-3.1.23/
    fi

    test_uninstall_package libgnutls-dev

    $SUDO wget ftp://ftp.gnutls.org/gcrypt/gnutls/v3.1/gnutls-3.1.23.tar.xz
    $SUDO tar -xf gnutls-3.1.23.tar.xz
    echo "Uncompressing gnutls archive ($PWD)"
    cd gnutls-3.1.23/
    $SUDO ./configure --prefix=/usr
    if [ $? -ne 0 ];   then
	exit -1
    fi
    echo "Compiling gnutls"
    $SUDO make -j $NUM_CPU
    $SUDO make install
    cd ../

    echo "Downloading freeDiameter archive"

    if [ -f 1.1.5.tar.gz ];  then
	$SUDO rm -f 1.1.5.tar.gz
    fi
    if [ -d freeDiameter-1.1.5/ ];   then
	$SUDO rm -rf freeDiameter-1.1.5/
    fi

    $SUDO wget http://www.freediameter.net/hg/freeDiameter/archive/1.1.5.tar.gz
    $SUDO tar -xzf 1.1.5.tar.gz
    echo "Uncompressing freeDiameter archive"
    cd freeDiameter-1.1.5
    $SUDO patch -p1 < $OPENAIR3_DIR/S6A/freediameter/freediameter-1.1.5.patch
    $SUDO mkdir build
    cd build
    $SUDO cmake -DCMAKE_INSTALL_PREFIX:PATH=/usr ../
    if [ $? -ne 0 ];  then
	exit -1
    fi
    echo "Compiling freeDiameter"
    $SUDO make -j $NUM_CPU
#make help
    $SUDO make test
    $SUDO sudo make install

#    make_certs

}


test_is_host_reachable() {
    ping -c 1 $1 > /dev/null || { echo_fatal "$2 host $1 does not respond to ping" >&2 ; }
    echo_success "$2 host $1 is reachable"
}


check_epc_s6a_certificate() {
    if [ -d /usr/local/etc/freeDiameter ]
    then
        if [ -f /usr/local/etc/freeDiameter/user.cert.pem ]
        then
            full_hostname=`cat /usr/local/etc/freeDiameter/user.cert.pem | grep "Subject" | grep "CN" | cut -d '=' -f6`
            if [ a$full_hostname == a`hostname`.${1:-'eur'} ]
            then
                echo_success "EPC S6A: Found valid certificate in /usr/local/etc/freeDiameter"
                return 0
            fi
        fi
    fi
    echo_error "EPC S6A: Did not find valid certificate in /usr/local/etc/freeDiameter"
    echo_warning "EPC S6A: generatting new certificate in /usr/local/etc/freeDiameter..."
    cd $OPENAIR3_DIR/S6A/freediameter
    ./make_certs.sh ${1:-'eur'}
    if [ $# -lt 2 ] ; then
        check_epc_s6a_certificate ${1:-'eur'}  2
        return $?
    else
       exit 1
    fi
}


check_hss_s6a_certificate() {
    if [ -d /usr/local/etc/freeDiameter ]; then
        if [ -f /usr/local/etc/freeDiameter/hss.cert.pem ];  then
            full_hostname=`cat /usr/local/etc/freeDiameter/hss.cert.pem | grep "Subject" | grep "CN" | cut -d '=' -f6`
            if [ a$full_hostname == a`hostname`.${1:-'eur'} ]
            then
                echo_success "HSS S6A: Found valid certificate in /usr/local/etc/freeDiameter"
                return 0
            else
                echo_error "Bad hss hostname found in cert file: "$full_hostname " hostname is "`hostname`
            fi
        fi
    fi
    echo_error "S6A: Did not find valid certificate in /usr/local/etc/freeDiameter"
    echo_warning "S6A: generatting new certificate in /usr/local/etc/freeDiameter..."
    cd $OPENAIR3_DIR/OPENAIRHSS/conf
    ./make_certs.sh ${1:-'eur'}
    if [ $# -lt 2 ] ; then
        check_hss_s6a_certificate ${1:-'eur'} 2
        return $?
    else
       exit 1
    fi
}

check_install_usrp_uhd_driver(){
    if [ ! -f /etc/apt/sources.list.d/ettus.list ] ; then
        $SUDO bash -c 'echo "deb http://files.ettus.com/binaries/uhd/repo/uhd/ubuntu/`lsb_release -cs` `lsb_release -cs` main" >> /etc/apt/sources.list.d/ettus.list'
        $SUDO apt-get update
    fi
    $SUDO apt-get install -t $UBUNTU_REL_NAME uhd
    test_install_package python
    test_install_package libboost-all-dev
    test_install_package libusb-1.0-0-dev
    #test_install_package uhd
}

check_install_oai_software() {

    if [ ! -f ./.lock_oaibuild ]; then
        $SUDO apt-get update
        if [ $UBUNTU_REL = "12.04" ]; then
            test_uninstall_package nettle-dev
            test_uninstall_package nettle-bin
        else
            test_install_package nettle-dev
            test_install_package nettle-bin
        fi
        test_install_package autoconf
        test_install_package automake
        test_install_package bison
        test_install_package build-essential
        test_install_package dialog
        test_install_package flex
        test_install_package gawk
        test_install_package gcc
        test_install_package gdb
        test_install_package make
        test_install_package cmake
        test_install_package openssh-client
        test_install_package openssh-server
        sudo service ssh start
        test_install_package unzip
        test_install_package autoconf
        test_install_package automake
        test_install_package bison
        test_install_package build-essential
        test_install_package check
        test_install_package ethtool
        test_install_package flex
        test_install_package g++
        test_install_package gawk
        test_install_package gcc
        test_install_package gccxml
        test_install_package gdb
        test_install_package guile-2.0-dev
        test_install_package iperf
        test_install_package iproute
        test_install_package iptables
        test_install_package libatlas-base-dev
        test_install_package libatlas-dev
        test_install_package libblas3gf
        test_install_package libblas-dev
        test_install_package liblapack-dev
        test_install_package liblapack-dev
#	if [ $MACHINE_ARCH = 64 ]; then
            test_install_package libconfig8-dev
#	else
#            test_install_package libconfig-dev
#	fi
        test_install_package libforms-bin
        test_install_package libforms-dev
        test_install_package libgcrypt11-dev
        test_install_package libgmp-dev
        test_install_package libgtk-3-dev
        test_install_package libidn11-dev
        test_install_package libidn2-0-dev
        test_install_package libpgm-dev
        test_install_package libpgm-5.1-0
        test_install_package libpthread-stubs0-dev
        test_install_package libsctp1
        test_install_package libsctp-dev
        test_install_package libtasn1-3-dev
        test_install_package libxml2
        test_install_package libxml2-dev
#       test_install_package linux-headers-`uname -r`
        test_install_package openssl
        test_install_package libssl-dev
        test_install_package pkg-config
        test_install_package python-dev
        test_install_package python-pexpect
        test_install_package sshfs
        test_install_package subversion
        test_install_package valgrind
        test_install_package doxygen
        test_install_package graphviz

#        test_install_package libboost-all-dev

        if [ $OAI_INSTALLED = 1 ]; then
            touch ./.lock_oaibuild
        fi

     else
        echo_info "All the required packages installed: skip"
    fi

}

check_install_hss_software() {
    if [ ! -f ./.lock_oaibuild ]; then
        $SUDO apt-get update
        if [ $UBUNTU_REL = "12.04" ]; then
            test_uninstall_package nettle-dev
            test_uninstall_package nettle-bin
        else
            test_install_package nettle-dev
            test_install_package nettle-bin
        fi
	test_install_package autoconf
	test_install_package automake
	test_install_package bison
	test_install_package build-essential
	test_install_package cmake
	test_install_package cmake-curses-gui
    test_install_package dialog
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
#	test_install_package linux-headers-`uname -r`
	test_install_package make
	test_install_package mysql-client
	test_install_package mysql-server-core-5.5
	test_install_package mysql-server
	test_install_package openssh-client
	test_install_package openssh-server
        sudo service ssh start
	test_install_package phpmyadmin
	test_install_package python-dev
	test_install_package sshfs
	test_install_package swig
	test_install_package unzip
#	test_install_package nettle-bin
#	test_install_package nettle-dev
	test_install_package valgrind

	if [ $OAI_INSTALLED = 1 ]; then
	    touch ./.lock_oaibuild
	fi

    else
	echo_info "All the required packages installed: skip"
    fi

}

check_install_epc_software() {

    if [ ! -f ./.lock_oaibuild ]; then
        $SUDO apt-get update
        if [ $UBUNTU_REL = "12.04" ]; then
            test_uninstall_package nettle-dev
            test_uninstall_package nettle-bin
        else
            test_install_package nettle-dev
            test_install_package nettle-bin
        fi
        test_install_package autoconf
        test_install_package automake
        test_install_package bison
        test_install_package build-essential
        test_install_package check
        test_install_package cmake
        test_install_package cmake-curses-gui
        test_install_package dialog
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
#       if [ $MACHINE_ARCH = 64 ]; then
            test_install_package libconfig8-dev
#        else
#            test_install_package libconfig-dev
#       fi
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
        test_install_package libssl-dev
        test_install_package libtasn1-3-dev
        test_install_package libtool
        test_install_package libxml2
        test_install_package libxml2-dev
#        test_install_package linux-headers-`uname -r`
        test_install_package make
        test_install_package openssh-client
        test_install_package openssh-server
        $SUDO service ssh start
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

        if [ $OAI_INSTALLED = 1 ]; then
            touch ./.lock_oaibuild
        fi

    else
        echo_info "All the required packages installed: skip"
    fi

}

check_install_asn1c(){

    test_command_install_script   "asn1c" "$OPENAIR_TARGETS/SCRIPTS/install_asn1c_0.9.24.modified.bash $SUDO"

    # One mor check about version of asn1c
    ASN1C_COMPILER_REQUIRED_VERSION_MESSAGE="ASN.1 Compiler, v0.9.24"
    ASN1C_COMPILER_VERSION_MESSAGE=`asn1c -h 2>&1 | grep -i ASN\.1\ Compiler`
    ##ASN1C_COMPILER_VERSION_MESSAGE=`trim $ASN1C_COMPILER_VERSION_MESSAGE`
    if [ "$ASN1C_COMPILER_VERSION_MESSAGE" != "$ASN1C_COMPILER_REQUIRED_VERSION_MESSAGE" ]; then
      #  diff <(echo -n "$ASN1C_COMPILER_VERSION_MESSAGE") <(echo -n "$ASN1C_COMPILER_REQUIRED_VERSION_MESSAGE")
        echo_error "Version of asn1c is not the required one, do you want to install the required one (overwrite installation) ? (Y/n)"
        echo_error "$ASN1C_COMPILER_VERSION_MESSAGE"
        while read -r -n 1 -s answer; do
            if [[ $answer = [YyNn] ]]; then
                [[ $answer = [Yy] ]] && $OPENAIR_TARGETS/SCRIPTS/install_asn1c_0.9.24.modified.bash $SUDO
                [[ $answer = [Nn] ]] && echo_error "Version of asn1c is not the required one, exiting." && exit 1
                break
            fi
        done
    fi

}

#################################################
# 2. compile
################################################
compile_hss() {
    cd $OPENAIR3_DIR/OPENAIRHSS

    if [ "$1" -eq 1 ]; then
        echo_info "build a clean HSS"
        rm -rfv obj*
        rm -rfv m4
        rm -rfv .autom4*
        rm -fv configure
    fi

    OBJ_DIR=`find . -maxdepth 1 -type d -iname obj*`

    if [ ! -n "$OBJ_DIR" ]; then
        OBJ_DIR="objs"
        mkdir --verbose -m 777 ./$OBJ_DIR
    else
        OBJ_DIR=`basename $OBJ_DIR`
    fi

    if [ ! -f "$OBJ_DIR"/Makefile ]; then
        if [ ! -d m4 ]; then
            mkdir --verbose -m 777 m4
        fi
        echo_success "Invoking autogen"
        ./autogen.sh
        if [ $? -ne 0 ]; then
            return 1
        fi
        cd $OBJ_DIR
        echo_success "Invoking configure"
        ../configure
        if [ $? -ne 0 ]; then
            return 1
        fi
    else
        cd $OBJ_DIR
    fi

    if [ -f Makefile ];  then
        echo_success "Compiling..."
        make ; #-j $NUM_CPU
        if [ $? -ne 0 ]; then
            echo_error "Build failed, exiting"
            return 1
        else
            return 0
        fi
    else
        echo_error "Configure failed, aborting"
    fi
    return 1
}


compile_epc() {
    cd $OPENAIR3_DIR
    if [ $1 = 1 ]; then
        echo_info "build a clean EPC"
        bash_exec "rm -rf objs"
    fi
    OBJ_DIR=`find . -maxdepth 1 -type d -iname obj*`
    if [ ! -n "$OBJ_DIR" ]; then
        OBJ_DIR="objs"
        bash_exec "mkdir -m 777 ./$OBJ_DIR"
        echo_success "Created $OBJ_DIR directory"
    else
        OBJ_DIR=`basename $OBJ_DIR`
    fi
    if [ ! -f $OBJ_DIR/Makefile ]; then
        if [ ! -n "m4" ]; then
            mkdir -m 777 m4
        fi
        bash_exec "autoreconf -i -f"
        echo_success "Invoking autogen"
        bash_exec "libtoolize"
        bash_exec "./autogen.sh"
        cd ./$OBJ_DIR
        echo_success "Invoking configure"
        if [ $DEBUG -ne 0 ]; then
            ../configure --enable-debug --enable-standalone-epc --enable-gtp1u-in-kernel LDFLAGS=-L/usr/local/lib
        else
            ../configure                --enable-standalone-epc --enable-gtp1u-in-kernel LDFLAGS=-L/usr/local/lib
        fi
    else
        cd ./$OBJ_DIR
    fi

#    pkill oai_epc
#    pkill tshark

    if [ -f Makefile ]; then
        echo_success "Compiling..."
        make -j $NUM_CPU
        if [ $? -ne 0 ]; then
            echo_error "Build failed, exiting"
            return 1
        else
            cp -pf ./OAI_EPC/oai_epc  $OPENAIR_TARGETS/bin
        fi
    else
        echo_error "Configure failed, exiting"
        return 1
    fi

    cd $OPENAIR3_DIR/GTPV1-U/GTPUAH;
    make
    if [ $? -ne 0 ]; then
        echo_error "Build GTPUAH module failed, exiting"
        return 1
    else
       	$SUDO cp -pfv ./Bin/libxt_*.so /lib/xtables
        $SUDO cp -pfv ./Bin/*.ko $OPENAIR_TARGETS/bin
    fi

    cd $OPENAIR3_DIR/GTPV1-U/GTPURH;
    make
    if [ $? -ne 0 ]; then
        echo_error "Build GTPURH module failed, exiting"
        return 1
    else
       $SUDO cp -pfv ./Bin/libxt_*.so /lib/xtables
       $SUDO cp -pfv ./Bin/*.ko $OPENAIR_TARGETS/bin
    fi
    return 0
}

compile_exmimo2_driver() {
    cd $OPENAIR_TARGETS/ARCH/EXMIMO/DRIVER/eurecom && make clean && make   || exit 1
    cd $OPENAIR_TARGETS/ARCH/EXMIMO/USERSPACE/OAI_FW_INIT && make clean && make   || exit 1
    # TO DO CHECKS...
}


compile_ltesoftmodem() {
    cd $OPENAIR_TARGETS/RT/USER
    if [ -f Makefile ];  then
        rm -f ./lte-softmodem
        echo "LTE softmodem compiling directives: $SOFTMODEM_DIRECTIVES"
        if [ $OAI_CLEAN -ne 0 ]; then
            echo "Cleaning LTE softmodem"
            make cleanall > /dev/null 2>&1
        fi
        make -j $NUM_CPU $SOFTMODEM_DIRECTIVES
        if [ $? -ne 0 ]; then
            # to locate easily compilation error in log file
            make  $SOFTMODEM_DIRECTIVES
        fi
        if [ $? -ne 0 ]; then
            if [ ! -f ./lte-softmodem ]; then
                echo_error "Build lte-softmodem failed, returning"
                return 1
            else
                cp -pf ./lte-softmodem  $OPENAIR_TARGETS/bin
                return 0
            fi
        else
            cp -pf ./lte-softmodem  $OPENAIR_TARGETS/bin
            return 0
        fi
    else
        echo_error "make file for oai softmodem not found, returning"
        return 1
    fi
}

compile_oaisim() {
    cd $OPENAIR_TARGETS/SIMU/USER
    if [ -f Makefile ]; then
        echo "Compiling for oaisim  target ($OAISIM_DIRECTIVES)"
        make cleanall > /dev/null
        make -j $NUM_CPU $OAISIM_DIRECTIVES
        if [ $? -ne 0 ]; then
            echo_error "Build oaisim failed, returning"
            return 1
        else
            cp -pf ./oaisim $OPENAIR_TARGETS/bin
            return 0
        fi
    else
        echo_error "Makefile not found for oaisim target, returning"
        return 1
    fi
}

compile_unisim() {
    cd $OPENAIR1_DIR/SIMULATION/LTE_PHY
    if [ -f Makefile ]; then
        echo "Compiling for UNISIM target ..."
        make cleanall
        make  -j $NUM_CPU all
        if [ $? -ne 0 ]; then
            echo_error "Build unisim failed, returning"
            return 1
        else
            cp -pf ./dlsim     $OPENAIR_TARGETS/bin
            cp -pf ./ulsim     $OPENAIR_TARGETS/bin
            cp -pf ./pucchsim  $OPENAIR_TARGETS/bin
            cp -pf ./prachsim  $OPENAIR_TARGETS/bin
            cp -pf ./pdcchsim  $OPENAIR_TARGETS/bin
            cp -pf ./pbchsim   $OPENAIR_TARGETS/bin
            cp -pf ./mbmssim   $OPENAIR_TARGETS/bin
            return 0
        fi
    else
        echo_error "Configure failed, exiting"
        return 1
    fi
}

compile_nas_tools() {

    export NVRAM_DIR=$OPENAIR_TARGETS/bin

    cd $NVRAM_DIR

        if [ ! -f /tmp/nas_cleaned ]; then
            echo_success "make --directory=$OPENAIR3_DIR/NAS/EURECOM-NAS/tools veryveryclean"
            make --directory=$OPENAIR3_DIR/NAS/EURECOM-NAS/tools veryveryclean
        fi
        echo_success "make --directory=$OPENAIR3_DIR/NAS/EURECOM-NAS/tools all"
        make --directory=$OPENAIR3_DIR/NAS/EURECOM-NAS/tools all
        rm .ue.nvram0
        rm .usim.nvram0
        touch /tmp/nas_cleaned
}


compile_ue_ip_nw_driver() {
    cd $OPENAIR2_DIR && make ue_ip.ko
}

################################################
# 1. check if the executable functions exist
###############################################

check_for_ltesoftmodem_executable() {
    if [ ! -f $OPENAIR_TARGETS/RT/USER/lte-softmodem ];   then
        echo_error "Cannot find lte-softmodem executable object in directory $OPENAIR_TARGETS/RT/USER"
        echo_error "Check the compilation logs in bin/install_log.txt"
        exit 1
    fi
}

check_for_epc_executable() {
    if [ ! -f $OPENAIR3_DIR/objs/OAI_EPC/oai_epc ]; then
        echo_error "Cannot find oai_epc executable object in directory $OPENAIR3_DIR/objs/OAI_EPC/"
        echo_fatal "Please make sure you have compiled OAI EPC with --enable-standalone-epc option"
    fi
}

check_for_hss_executable() {
    if [ ! -f $OPENAIR3_DIR/OPENAIRHSS/objs/openair-hss ]; then
        echo_error "Cannot find openair-hss executable object in directory $OPENAIR3_DIR/OPENAIRHSS/objs/"
        echo_fatal "Please make sure you have compiled OAI HSS"
    fi
}

check_for_sgw_executable() {
    if [ ! -f $OPENAIR3_DIR/objs/OAI_SGW/oai_sgw ]; then
        echo_error "Cannot find oai_sgw executable object in directory $OPENAIR3_DIR/OPENAIRMME/objs/OAI_SGW/"
        echo_fatal "Please make sure you have compiled OAI EPC without --enable-standalone-epc option"
    fi
}

check_for_oaisim_executable() {
    if [ ! -f $OPENAIR_TARGETS/SIMU/USER/oaisim ]; then
        echo_error "Cannot find oaisim executable object in directory $OPENAIR_TARGETS/SIMU/USER"
        echo_error "Check the compilation logs in bin/install_log.txt"
    fi
}

check_for_dlsim_executable() {
    if [ ! -f $OPENAIR1_DIR/SIMULATION/LTE_PHY/dlsim ];  then
        echo_error "Cannot find dlsim executable object in directory $OPENAIR1_DIR/SIMULATION/LTE_PHY"
        echo_error "Check the compilation logs in bin/install_log.txt"
    fi
}

check_for_ulsim_executable() {
    if [ ! -f $OPENAIR1_DIR/SIMULATION/LTE_PHY/ulsim ]; then
        echo_error "Cannot find ulsim executable object in directory $OPENAIR1_DIR/SIMULATION/LTE_PHY"
        echo_error "Check the compilation logs in bin/install_log.txt"
    fi
}

check_for_pucchsim_executable() {
    if [ ! -f $OPENAIR1_DIR/SIMULATION/LTE_PHY/pucchsim ]; then
        echo_error "Cannot find pucchsim executable object in directory $OPENAIR1_DIR/SIMULATION/LTE_PHY"
        echo_error "Check the compilation logs in bin/install_log.txt"
    fi
}

check_for_prachsim_executable() {
    if [ ! -f $OPENAIR1_DIR/SIMULATION/LTE_PHY/prachsim ]; then
        echo_error "Cannot find prachsim executable object in directory $OPENAIR1_DIR/SIMULATION/LTE_PHY"
        echo_error "Check the compilation logs in bin/install_log.txt"
    fi
}

check_for_pdcchsim_executable() {
    if [ ! -f $OPENAIR1_DIR/SIMULATION/LTE_PHY/pdcchsim ]; then
        echo_error "Cannot find pdcchsim executable object in directory $OPENAIR1_DIR/SIMULATION/LTE_PHY"
        echo_error "Check the compilation logs in bin/install_log.txt"
    fi
}

check_for_pbchsim_executable() {
    if [ ! -f $OPENAIR1_DIR/SIMULATION/LTE_PHY/pbchsim ]; then
        echo_error "Cannot find pbchsim executable object in directory $OPENAIR1_DIR/SIMULATION/LTE_PHY"
        echo_error "Check the compilation logs in bin/install_log.txt"
    fi
}

check_for_mbmssim_executable() {
    if [ ! -f $OPENAIR1_DIR/SIMULATION/LTE_PHY/mbmssim ]; then
        echo_error "Cannot find mbmssim executable object in directory $OPENAIR1_DIR/SIMULATION/LTE_PHY"
        echo_error "Check the compilation logs in bin/install_log.txt"
    fi
}

check_for_nas_ue_executable() {
    if [ ! -f $OPENAIR3_DIR/NAS/EURECOM-NAS/bin/UserProcess ]; then
        echo_error "Cannot find UserProcess executable object in directory  $OPENAIR3_DIR/NAS/EURECOM-NAS/bin"
        echo_fatal "Check the compilation logs in bin/install_log.txt"
    fi
}

################################################
# 1. check if the executable functions exist
###############################################

# arg1 is RT
# arg2 is HW
# arg3 is ENB_S1
install_ltesoftmodem() {
    # RT
    if [ $1 = "RTAI" ]; then
        if [ ! -f /tmp/init_rt_done.tmp ]; then
            echo_info "  8.1 Insert RTAI modules"
            $SUDO insmod /usr/realtime/modules/rtai_hal.ko     > /dev/null 2>&1
            $SUDO insmod /usr/realtime/modules/rtai_sched.ko   > /dev/null 2>&1
            $SUDO insmod /usr/realtime/modules/rtai_sem.ko     > /dev/null 2>&1
            $SUDO insmod /usr/realtime/modules/rtai_fifos.ko   > /dev/null 2>&1
            $SUDO insmod /usr/realtime/modules/rtai_mbx.ko     > /dev/null 2>&1
            $SUDO touch /tmp/init_rt_done.tmp
            $SUDO chmod 666 /tmp/init_rt_done.tmp
        else
            echo_warning "  8.1 RTAI modules already inserted"
        fi
    fi
    #HW
    if [ $2 = "EXMIMO" ]; then
	echo_info "  8.2 [EXMIMO] creating RTAI fifos"
	for i in `seq 0 64`; do
	    have_rtfX=`ls /dev/ |grep -c rtf$i`;
	    if [ "$have_rtfX" -eq 0 ] ; then
		$SUDO mknod -m 666 /dev/rtf$i c 150 $i;
	    fi;
	done
	echo_info "  8.3 [EXMIMO] Build lte-softmodemdrivers"
	cd $OPENAIR_TARGETS/ARCH/EXMIMO/DRIVER/eurecom && make clean && make  # || exit 1
	cd $OPENAIR_TARGETS/ARCH/EXMIMO/USERSPACE/OAI_FW_INIT && make clean && make  # || exit 1

	echo_info "  8.4 [EXMIMO] Setup RF card"
	cd $OPENAIR_TARGETS/RT/USER
	. ./init_exmimo2.sh
    else
	if [ $2 = "USRP" ]; then
	    echo_info "  8.2 [USRP] "
	fi

    fi

    # ENB_S1
    if [ $3 = 0 ]; then
        cd $OPENAIR2_DIR && make clean && make nasmesh_netlink.ko  #|| exit 1
        cd $OPENAIR2_DIR/NAS/DRIVER/MESH/RB_TOOL && make clean && make  # || exit 1
    fi

}

# arg1 is ENB_S1 'boolean'
install_oaisim() {
   if [ $1 = 0 ]; then
       cd $OPENAIR2_DIR && make clean && make nasmesh_netlink.ko  #|| exit 1
       cd $OPENAIR2_DIR/NAS/DRIVER/MESH/RB_TOOL && make clean && make  # || exit 1
   else
       compile_ue_ip_nw_driver
       install_nas_tools
   fi

}


install_nas_tools() {
    cd $OPENAIR_TARGETS/bin
    if [ ! -f .ue.nvram0 ]; then
        echo_success "generate .ue_emm.nvram0 .ue.nvram0"
        $OPENAIR3_DIR/NAS/EURECOM-NAS/bin/ue_data --gen
    fi

    if [ ! -f .usim.nvram0 ]; then
        echo_success "generate .usim.nvram0"
        $OPENAIR3_DIR/NAS/EURECOM-NAS/bin/usim_data --gen
    fi
    $OPENAIR3_DIR/NAS/EURECOM-NAS/bin/ue_data --print
    $OPENAIR3_DIR/NAS/EURECOM-NAS/bin/usim_data --print
}

install_nasmesh(){
    echo_success "LOAD NASMESH IP DRIVER FOR UE AND eNB"
    (cd $OPENAIR2_DIR/NAS/DRIVER/MESH/RB_TOOL && make clean && make)
    (cd $OPENAIR2_DIR && make clean && make nasmesh_netlink_address_fix.ko)
    $SUDO rmmod nasmesh
    $SUDO insmod $OPENAIR2_DIR/NAS/DRIVER/MESH/nasmesh.ko
}

##################################
# create HSS DB
################################

# arg 1 is mysql user      (root)
# arg 2 is mysql password  (linux)
# arg 3 is hss username    (hssadmin)
# arg 4 is hss password    (admin)
# arg 5 is database name   (oai_db)
create_hss_database(){
    EXPECTED_ARGS=5
    E_BADARGS=65
    MYSQL=`which mysql`
    rv=0
    if [ $# -ne $EXPECTED_ARGS ]
    then
        echo_fatal "Usage: $0 dbuser dbpass hssuser hsspass databasename"
        rv=1
    fi

    set_openair_env

    # removed %
    #Q1="GRANT ALL PRIVILEGES ON *.* TO '$3'@'%' IDENTIFIED BY '$4' WITH GRANT OPTION;"
    Q1="GRANT ALL PRIVILEGES ON *.* TO '$3'@'localhost' IDENTIFIED BY '$4' WITH GRANT OPTION;"
    Q2="FLUSH PRIVILEGES;"
    SQL="${Q1}${Q2}"
    $MYSQL -u $1 --password=$2 -e "$SQL"
    if [ $? -ne 0 ]; then
       echo_error "$3 permissions failed"
       return 1
    else
       echo_success "$3 permissions succeeded"
    fi


    Q1="CREATE DATABASE IF NOT EXISTS ${BTICK}$5${BTICK};"
    SQL="${Q1}"
    $MYSQL -u $3 --password=$4 -e "$SQL"
    if [ $? -ne 0 ]; then
       echo_error "$5 creation failed"
       return 1
    else
       echo_success "$5 creation succeeded"
    fi


    # test if tables have been created
    mysql -u $3 --password=$4  -e "desc $5.users" > /dev/null 2>&1

    if [ $? -eq 1 ]; then
        $MYSQL -u $3 --password=$4 $5 < $OPENAIR3_DIR/OPENAIRHSS/db/oai_db.sql
        if [ $? -ne 0 ]; then
            echo_error "$5 tables creation failed"
            return 1
        else
            echo_success "$5 tables creation succeeded"
        fi
    fi

    return 0
}

################################
# set_openair_env
###############################
set_openair_env(){

    fullpath=`readlink -f $BASH_SOURCE`
    [ -f "/.$fullpath" ] || fullpath=`readlink -f $PWD/$fullpath`
    openair_path=${fullpath%/targets/*}
    openair_path=${openair_path%/openair3/*}
    openair_path=${openair_path%/openair[123]/*}

    export OPENAIR_DIR=$openair_path
    export OPENAIR_HOME=$openair_path
    export OPENAIR1_DIR=$openair_path/openair1
    export OPENAIR2_DIR=$openair_path/openair2
    export OPENAIR3_DIR=$openair_path/openair3
    export OPENAIR3_DIR=$openair_path/openair3
    export OPENAIR_TARGETS=$openair_path/targets

}

########################################
### print help
######################################

print_help(){
    echo_success "Name : build_oai  - install and build OAI"
    echo_success "Usage: build_oai.bash -a -b -c -d -e RTAI -m -r REL8 -s -t OAISIM -w EXMIMO -x"
    echo_success "-a | --doxygen                          : Enable doxygen for documentation (default disabled)"
    echo_success "-b | --disable-s1                       : Disable S1 interface for eNB (default enabled)"
    echo_success "-c | --clean                            : Enable clean OAI build (default disabled)"
    echo_success "-C | --config-file                      : Set the config file local path"
    echo_success "-d | --debug                            : Enable debug mode (default disabled)"
    echo_success "-D | --disable-check-installed-software : Disable the checking of installed software (default is check)"
    echo_success "-e | --realtime                         : Set realtime mode: RTAI, NONE (default NONE)"
    echo_success "-g | --run-with-gdb                     : Run the executable built by this script with gdb"
    echo_success "-K | --itti-dump-file                   : Set the execution events trace file"
    echo_success "-l | --build-target                     : Set the LTE build target: ENB,EPC,HSS,NONE (default ENB)"
    echo_success "-m | --build-from-makefile              : Enable build from the makefile (default disabled)"
    echo_success "-r | --3gpp-release                     : Set the release: REL8, REL10 (default REL8)"
    echo_success "-p | --wireshark                        : enable wireshark interface for L2 pdus"
    echo_success "-R | --run                              : Run the executable built by this script"
    echo_success "-s | --check                            : Enable OAI testing and sanity check (default disabled)"
    echo_success "-t | --enb-build-target                 : Set the eNB build target: ALL, SOFTMODEM,OAISIM,UNISIM (default ALL)"
    echo_success "-V | --vcd                              : Log vcd events"
    echo_success "-w | --hardware                         : Set the hardware platform: EXMIMO, USRP (also installs UHD driver), ETHERNET, NONE, (default EXMIMO)"
    echo_success "-x | --xforms                           : Enable xforms (default disabled)"
    echo_success "-z | --defaults                         : Set the default build options"
}

print_help_perf(){
    echo_success "Name : perf_oai  generate traffic and evaluate the performance "
    echo_success "Usage: perf_oai.bash -l ITG "
    echo_success "-l | --perf-app               : Set Performance evaluation app: ITGS, ITGD,PING, OTG-OAISIM, OTG-CBA, (default PING)"
    echo_success "-m | --owd                    : enable D-ITG one-way-delay meter (default disabled)"
    echo_success "-e | --duration               : set the duration of the experiment (default 60000ms)"
    echo_success "-i | --idt-dist               : set the distribution of the inter-departure time: CONSTANT, UNIFORM,EXPONENTIAL (default CONSTANT)"
    echo_success "-s | --ps-dist                : set the distribution of the inter-departure time (default CONSTANT, available options: UNIFORM,EXPONENTIAL)"
    echo_success "-d | --dst                    : set the destination address (default 127.0.0.1)"
    echo_success "-p | --dst-port               : set the destination address (default NONE)"
    echo_success "-t | --test                   : enable test mode to validate the functionality (default disabled)"
    echo_success "-k | --keep-log-file          : keep the log files (default disabled)"

}
###############################
## echo and  family
###############################
black='\E[30m'
red='\E[31m'
green='\E[32m'
yellow='\E[33m'
blue='\E[34m'
magenta='\E[35m'
cyan='\E[36m'
white='\E[37m'
reset_color='\E[00m'


cecho()   # Color-echo
# arg1 = message
# arg2 = color
{
    local default_msg="No Message."
    message=${1:-$default_msg}
    color=${2:-$green}
    if [ $BUILD_FROM_MAKEFILE = 0 ]; then
	echo -e -n "$color$message$reset_color"
	echo
    else
	echo "$message"
    fi
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
echo_info() {
    local my_string=""
    until [ -z "$1" ]
    do
        my_string="$my_string$1"
        shift
    done
    cecho "$my_string" $blue
}

bash_exec() {
    output=$($1 2>&1)
    result=$?
    if [ $result -eq 0 ]; then
        echo_success "$1"
    else
        echo_error "$1: $output"
    fi
}


wait_process_started () {
    if  [ -z "$1" ]; then
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
    if  [ -z "$1" ]; then
        echo_error "WAITING FOR PROCESS START: NO PROCESS"
        return 1
    fi
    ps -C $1 > /dev/null 2>&1
    if [ $? -ne 0 ]; then
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

    if [ -z "$2" ] ; then # Not enought parameters passed.
        return $E_PARAM_ERR
    fi

    lineno=$2
    if [ ! $1 ]; then
        echo_error "Assertion failed:  \"$1\""
        echo_fatal "File \"$0\", line $lineno"
    fi
}
