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
# file interfaces.bash
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

# example: netcalc 192.168.12.100 255.255.255.0
netcalc(){
    local IFS='.' ip i
    local -a oct msk
    
    read -ra oct <<<"$1"
    read -ra msk <<<"$2"

    for i in ${!oct[@]}; do
        ip+=( "$(( oct[i] & msk[i] ))" )
    done
    
    echo "${ip[*]}"
}

# example: s
bcastcalc(){

    local IFS='.' ip i
    local -a oct msk
    
    read -ra oct <<<"$1"
    read -ra msk <<<"$2"

    for i in ${!oct[@]}; do
        ip+=( "$(( oct[i] + ( 255 - ( oct[i] | msk[i] ) ) ))" )
    done

    echo "${ip[*]}"
}


is_real_interface() {
    my_bool=1
    for var in "$@"
    do
        if [ "a$var" == "a" ]; then
           return 0
        fi
        if [ "a$var" == "anone" ]; then
           return 0
        fi
        IF=`cat /etc/udev/rules.d/70-persistent-net.rules | grep $var | sed 's/^.*NAME=//' | tr -d '"'`
        if [ "$IF" == "$var" ]; then
            if [ "a${var:0:3}" != "aeth" ]; then
                if [ "a${var:0:4}" != "awlan" ]; then
                    if [ "a${var:0:4}" != "awifi" ]; then
                        my_bool=0;
                    fi
                fi
            fi
        fi
    done
    return $my_bool
}

is_vlan_interface() {
    my_bool=1
    for var in "$@"
    do
        if [ "a$var" == "a" ]; then
            return 0
        fi
        if [[ $var == *.* ]]
        then
            interface_name=`echo $var | cut -f1 -d '.'`
            vlan=`echo $var | cut -f2 -d '.'`
            IF=`cat /etc/udev/rules.d/70-persistent-net.rules | grep $interface_name | sed 's/^.*NAME=//' | tr -d '"'`
            if [ "$IF" == "$interface_name" ]; then
                if [ "a${interface_name:0:3}" != "aeth" ]; then
                    if [ "a${interface_name:0:4}" != "awlan" ]; then
                        if [ "a${interface_name:0:4}" != "awifi" ]; then
                            return 0;
                        fi
                    fi
                fi
            fi
        else
            return 0;
        fi
    done
    return $my_bool
}

is_tun_interface() {
    my_bool=1
    for var in "$@"
    do
        if [ "a$var" == "a" ]; then
            return 0
        fi
        if [[ "$IF" != *tun* ]]; then
            return 0;
        fi
        bus_info=`ethtool -i $var | grep bus-info | cut -d: -f2`
        bus_info=trim2 $bus_info
        if [[ "$bus_info" != *tun* ]]; then
            return 0;
        fi
    done
    return $my_bool
}

is_openvswitch_interface() {
    for var in "$@"
    do
        if [ "a$var" == "a" ]; then
            return 0
        fi
        if [ "a${var:0:3}" != "aeth" ]; then
            if [ "a${var:0:4}" != "awlan" ]; then
                if [ "a${var:0:4}" != "awifi" ]; then
                    if [ "a${var:0:4}" != "anone" ]; then
                        if [ "a${var:0:3}" != "atun" ]; then
                            if [ "a${var:0:4}" != "avbox" ]; then
                                return 1;
                            fi
                        fi
                    fi
                fi
            fi
        fi
    done
    return 0;
}

delete_tun_interface() {
  is_tun_interface $1 
  if [ $? -eq 1 ]; then
      ip link set $1 down  > /dev/null 2>&1
      openvpn --rmtun --dev $1  > /dev/null 2>&1
  fi
}

delete_vlan_interface() {
  is_vlan_interface $1 
  if [ $? -eq 1 ]; then
    bash_exec "ifconfig    $1 down "
    bash_exec "vconfig rem $1      "
  fi
}

delete_openvswitch_interface() {
  is_openvswitch_interface $1 
  if [ $? -eq 1 ]; then
      bash_exec "ifconfig $1 down  "
      bash_exec "tunctl -d $1 "
  fi
}


create_tun_interface() {
  is_tun_interface $1 
  if [ $? -eq 1 ]; then
      bash_exec "openvpn --mktun --dev $1"
  fi
}


create_vlan_interface() {
  is_vlan_interface $1 
  if [ $? -eq 1 ]; then
        interface_name=`echo $1 | cut -f1 -d '.'`
        vlan=`echo $1 | cut -f2 -d '.'`
        bash_exec "vconfig add $interface_name $vlan"
  fi
}


create_openvswitch_interface() {
  is_openvswitch_interface $1
  if [ $? -eq 1 ]; then
      bash_exec "tunctl -t $1"
  fi
}

# arg1 = interface name
# arg2 = ipv4 addr cidr
# arg3 = netmask cidr
set_interface_up() {
    interface=$1
    address=$2
    cidr_netmask=$3
    if [ "a${interface:0:4}" == "anone" ]; then
        return;
    fi
    bash_exec "ifconfig  $interface up"
    sync
    netmask=`cidr2mask $cidr_netmask`
    broadcast=`bcastcalc $address $netmask`
    bash_exec "ip -4 addr add  $address/$cidr_netmask broadcast $broadcast dev $interface"
    sync
}

# input is hostname, you should have checked it is reachable by
# ping -c 1 $the_hostname > /dev/null || { echo_fatal "the_hostname does not respond to ping" >&2 ; }
get_ip() {
    IP=$(python -c 'import socket; print socket.gethostbyname("'$1'")')
    echo "$IP"
}

get_mac_router() {
    echo_success "Resolving router.eur"
    ping -c 1 router.eur > /dev/null || { echo_fatal "router.eur does not respond to ping" >&2 ; }
    IP_ROUTER=$(get_ip router.eur)
    #IP_ROUTER=`python -c 'import socket; print socket.gethostbyname("router.eur")'`
    export MAC_ROUTER=`ip neigh show | grep $IP_ROUTER' ' | cut -d ' '  -f5 | tr -d ':'`
    echo_success "ROUTER IP  ADDRESS= $IP_ROUTER"
    echo_success "ROUTER MAC ADDRESS= $MAC_ROUTER"
}

