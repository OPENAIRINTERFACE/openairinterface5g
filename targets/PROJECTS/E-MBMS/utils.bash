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
################################################################################
# file utils.bash
# brief
# author Lionel Gauthier
# company Eurecom
# email: lionel.gauthier@eurecom.fr
#

cidr2mask() {
  local i mask=""
  local full_octets=$(($1/8))
  local partial_octet=$(($1%8))

  for ((i=0;i<4;i+=1)); do
    if [ $i -lt $full_octets ]; then
      mask+=255
    elif [ $i -eq $full_octets ]; then
      mask+=$((256 - 2**(8-$partial_octet)))
    else
      mask+=0
    fi
    test $i -lt 3 && mask+=.
  done

  echo $mask
}


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

rotate_log_file () {
    if [ -f $1 ]; then
        TIMESTAMP=`date +%Y-%m-%d.%Hh_%Mm_%Ss`
        NEWLOGFILE=$1.$TIMESTAMP
        mv $1 $NEWLOGFILE
        cat /dev/null > $1
        nohup gzip -f -9 $NEWLOGFILE &
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


set_openair() {
    path=`pwd`
    declare -i length_path
    declare -i index
    length_path=${#path}

    for i in 'openair1' 'openair2' 'openair3' 'openair3' 'targets'
    do
        index=`echo $path | grep -b -o $i | cut -d: -f1`
        #echo ${path%$token*}
        if [[ $index -lt $length_path  && index -gt 0 ]]
           then
               declare -x OPENAIR_DIR
               index=`expr $index - 1`
               openair_path=`echo $path | cut -c1-$index`
               #openair_path=`echo ${path:0:$index}`
               export OPENAIR_DIR=$openair_path
               export OPENAIR_DIR=$openair_path
               export OPENAIR1_DIR=$openair_path/openair1
               export OPENAIR2_DIR=$openair_path/openair2
               export OPENAIR3_DIR=$openair_path/openair3
               export OPENAIR3_DIR=$openair_path/openair3
               export OPENAIR_TARGETS=$openair_path/targets
               return 0
           fi
    done
    return -1
}

test_install_asn1c_4_rrc_cellular() {
    if [ -d $OPENAIR2_DIR/RRC/LITE/MESSAGES/asn1c/asn1c ]; then
        if [ -x $OPENAIR2_DIR/RRC/LITE/MESSAGES/asn1c/asn1c/asn1c/asn1c ]; then
            if [ -x /usr/local/bin/asn1c ]; then
                diff /usr/local/bin/asn1c $OPENAIR2_DIR/RRC/LITE/MESSAGES/asn1c/asn1c/asn1c/asn1c >/dev/null 2>&1;
                if [ $? -eq 0 ]; then
                    echo_success "asn1c for RRC cellular installed"
                    return 0
                fi
            fi
            echo_warning "Installing asn1c for RRC cellular..."
            cd $OPENAIR2_DIR/RRC/LITE/MESSAGES/asn1c/asn1c
            make install
            return 0
        fi
    else
        echo_warning "asn1c for RRC cellular is not installed in $OPENAIR2_DIR/RRC/LITE/MESSAGES/asn1c/. Installing it"
        cd $OPENAIR2_DIR/RRC/LITE/MESSAGES/asn1c
        svn co https://asn1c.svn.sourceforge.net/svnroot/asn1c/trunk asn1c
    fi
    echo_warning "Configuring and building and installing asn1c for RRC cellular..."
    cd $OPENAIR2_DIR/RRC/LITE/MESSAGES/asn1c/asn1c
    ./configure
    make
    make install
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
        echo_error "WAITING FOR PROCESS START: ERROR NO PROCESS NAME IN ARGUMENT"
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
        echo "Assertion failed:  \"$1\""
        echo "File \"$0\", line $lineno"
        exit $E_ASSERT_FAILED
    fi
}


test_install_package() {
  # usage: test_install_package package_name_to_be_installed optional_option_to_apt_get_install
  dpkg --get-selections $1  | grep -i install > /dev/null 2>&1
  if [ $? -ne 0 ]; then
      echo_warning "Package $1 is not installed. Installing it." >&2
      apt-get install $2 $1 -y
      dpkg --get-selections $1  | grep -i install > /dev/null 2>&1
      if [ $? -ne 0 ]; then
          exit 1
      fi
  else
      echo_success "$1 is installed"
  fi
  return 0
}



test_command_install_package() {
  # usage: test_command_install_package searched_binary package_to_be_installed_if_binary_not_found optional_option_to_apt_get_install
  if [ $# -eq 2 ]; then
      command -v $1 >/dev/null 2>&1 || { echo_warning "Program $1 is not installed. Trying installing it." >&2; apt-get install $2 -y; command -v $1 >/dev/null 2>&1 || { echo_error "Program $1 is not installed. Aborting." >&2; exit 1; };}
  else
      if [ $# -eq 3 ]; then
          command -v $1 >/dev/null 2>&1 || { echo_warning "Program $1 is not installed. Trying installing it (apt-get install $3 $2)." >&2; apt-get install $3 $2 -y; command -v $1 >/dev/null 2>&1 || { echo_error "Program $1 is not installed. Aborting." >&2; exit 1; };}
      else
          echo_success "test_command_install_package: BAD PARAMETER"
          exit 1
      fi
  fi
  echo_success "$1 available"
}

test_command_install_script() {
  # usage: test_command_install_script searched_binary script_to_be_invoked_if_binary_not_found
  command -v $1 >/dev/null 2>&1 || { echo_warning "Program $1 is not installed. Trying installing it." >&2; bash $2; command -v $1 >/dev/null 2>&1 || { echo_error "Program $1 is not installed. Aborting." >&2; exit 1; };}
  echo_success "$1 available"
}

start_openswitch_daemon() {
  rmmod -s bridge
  if [[ -e "/lib/modules/`uname -r`/extra/openvswitch.ko" ]] ; then
      bash_exec "insmod /lib/modules/`uname -r`/extra/openvswitch.ko"
  else
      echo_error "/lib/modules/`uname -r`/extra/openvswitch.ko not found, exiting"
      exit -1
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

check_enb_config() {
    if [ ! -f $OPENAIR3_DIR/OPENAIRMME/UTILS/CONF/enb_$HOSTNAME.conf ]
        then
        echo "Cannot find file $OPENAIR3_DIR/OPENAIRMME/UTILS/CONF/enb_$HOSTNAME.conf"
        echo "Please make sure to create one that fits your use (you can use mme_default.conf file as template)"
        exit -1
        fi
}


check_for_root_rights() {
    if [[ $EUID -ne 0 ]]; then
        echo "This script must be run as root" 1>&2
        exit -1
    fi
}

###########################################################
declare -x OPENAIR_DIR=""
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
