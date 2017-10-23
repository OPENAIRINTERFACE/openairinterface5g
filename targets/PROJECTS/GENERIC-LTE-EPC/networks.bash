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
# file networks.bash
# brief
# author Lionel Gauthier
# company Eurecom
# email: lionel.gauthier@eurecom.fr
#


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



build_enb_vlan_network() {
    # create vlan interface
    is_vlan_interface $ENB_INTERFACE_NAME_FOR_S1_MME
    if [ $? -eq 1 ]; then
        delete_vlan_interface $ENB_INTERFACE_NAME_FOR_S1_MME 
        sync
        create_vlan_interface $ENB_INTERFACE_NAME_FOR_S1_MME 
        sync
        set_interface_up $ENB_INTERFACE_NAME_FOR_S1_MME $ENB_IPV4_ADDRESS_FOR_S1_MME $ENB_IPV4_NETMASK_FOR_S1_MME
    else
        echo_fatal "BAD INTERFACE NAME FOR ENB S1-MME $ENB_INTERFACE_NAME_FOR_S1_MME"' (waiting for ethx.y, wlanx.y or wifix.y)'
    fi;
    
    is_vlan_interface $ENB_INTERFACE_NAME_FOR_S1U
    if [ $? -eq 1 ]; then
        delete_vlan_interface $ENB_INTERFACE_NAME_FOR_S1U 
        sync
        create_vlan_interface $ENB_INTERFACE_NAME_FOR_S1U 
        sync
        set_interface_up $ENB_INTERFACE_NAME_FOR_S1U $ENB_IPV4_ADDRESS_FOR_S1U $ENB_IPV4_NETMASK_FOR_S1U
        sync
    else
        echo_fatal "BAD INTERFACE NAME FOR ENB S1U $ENB_INTERFACE_NAME_FOR_S1U"' (waiting for ethx.y, wlanx.y or wifix.y)'
    fi;
}

clean_enb_vlan_network() {
    is_vlan_interface $ENB_INTERFACE_NAME_FOR_S1_MME
    if [ $? -eq 1 ]; then
        echo_success "Found VLAN interface $ENB_INTERFACE_NAME_FOR_S1_MME ... deleting"
        delete_vlan_interface $ENB_INTERFACE_NAME_FOR_S1_MME
    fi;
    
    is_vlan_interface $ENB_INTERFACE_NAME_FOR_S1U
    if [ $? -eq 1 ]; then
        echo_success "Found VLAN interface $ENB_INTERFACE_NAME_FOR_S1U ... deleting"
        delete_vlan_interface $ENB_INTERFACE_NAME_FOR_S1U 
    fi;
    sync;
    clean_network
}



build_mme_spgw_vlan_network() {
    #                                                                           hss.eur
    #                                                                             |
    #        +-----------+                                +-----------+           v   +----------+
    #        |  eNB      +------+            VLAN 1+------+    MME    +----+      +---+   HSS    |
    #        |           |ethx.1+------------------+ethy.1|           |    +------+   |          |
    #        |           +------+                  +------+           +----+      +---+          |
    #        |           |ethx.2+-------+                 |           |               +----------+
    #        |           +------+       |                 +-+-------+-+
    #        |           |              |                   | s11mme|    
    #        |           |              |                   +---+---+    
    #        |           |              |             (optional)|   VLAN 3
    #        +-----------+              |                   +---+---+    
    #                                   |                   | s11sgw|            router.eur
    #                                   |                 +-+-------+-+              |   +--------------+
    #                                   |                 |  S+P-GW   |              v   |   ROUTER     |
    #                                   |  VLAN2   +------+           +-------+     +----+              +----+
    #                                   +----------+ethy.2|           |sgi    +-...-+    |              |    +---...Internet
    #                                              +------+           +-------+     +----+              +----+
    #                                                     |           |      11 VLANS    |              |
    #                                                     +-----------+   ids=[5..15]    +--------------+
    
    bash_exec "modprobe 8021q"
    
    # create vlan interface
    is_vlan_interface $MME_INTERFACE_NAME_FOR_S1_MME
    if [ $? -eq 1 ]; then
        delete_vlan_interface $MME_INTERFACE_NAME_FOR_S1_MME 
        sync
        create_vlan_interface $MME_INTERFACE_NAME_FOR_S1_MME
        sync
        set_interface_up $MME_INTERFACE_NAME_FOR_S1_MME $MME_IPV4_ADDRESS_FOR_S1_MME $MME_IPV4_NETMASK_FOR_S1_MME
    else
        echo_fatal "BAD INTERFACE NAME FOR SGW S1-MME $MME_INTERFACE_NAME_FOR_S1_MME"' (waiting for ethx.y, wlanx.y or wifix.y)'
    fi;
    
    is_vlan_interface $SGW_INTERFACE_NAME_FOR_S1U_S12_S4_UP
    if [ $? -eq 1 ]; then
        delete_vlan_interface $SGW_INTERFACE_NAME_FOR_S1U_S12_S4_UP 
        sync
        create_vlan_interface $SGW_INTERFACE_NAME_FOR_S1U_S12_S4_UP 
        sync
        set_interface_up $SGW_INTERFACE_NAME_FOR_S1U_S12_S4_UP $SGW_IPV4_ADDRESS_FOR_S1U_S12_S4_UP $SGW_IPV4_NETMASK_FOR_S1U_S12_S4_UP
    else
        echo_fatal "BAD INTERFACE NAME FOR SGW S1U $SGW_INTERFACE_NAME_FOR_S1U_S12_S4_UP"' (waiting for ethx.y, wlanx.y or wifix.y)'
    fi;
    
    #
    is_real_interface $PGW_INTERFACE_NAME_FOR_SGI
    if [ $? -eq 1 ]; then
        create_sgi_vlans
    else
        echo_warning "SGI interface disabled by config file"
    fi
}

clean_epc_vlan_network() {

    bash_exec "modprobe 8021q"

    delete_vlan_interface  $MME_INTERFACE_NAME_FOR_S1_MME  
    delete_vlan_interface  $SGW_INTERFACE_NAME_FOR_S1U_S12_S4_UP  

    for i in 5 6 7 8 9 10 11 12 13 14 15
    do
        # delete vlan interface
        delete_vlan_interface   $PGW_INTERFACE_NAME_FOR_SGI.$i  
    done
    clean_network
}


# arg1 is IF1 IP addr
# arg2 is IF2 IP addr
test_local_iperf() {
    
    TRY_NO=3

    until [  $TRY_NO -lt 1 ]; do
        iperf  --bind $1 -u -s 2>&1  > /dev/null &
        iperf  --bind $2 -u --num 1K -c $1 2>&1 | grep -i WARNING > /dev/null
        if [ $? -eq 0 ]; then
            pkill iperf 2>&1 > /dev/null
            echo_warning "NETWORK TEST FAILED between $1 and $2"
        else
            echo_success "NETWORK TEST SUCCESS between $1 and $2"
            pkill iperf 2>&1 > /dev/null
            return
        fi 
        let TRY_NO-=1
        pkill iperf 2>&1 > /dev/null
    done
    echo_fatal "FATAL: NETWORK TEST FAILED between $1 and $2"
}

test_network() {

    # TEST INTERFACES
    if [ "a${MME_INTERFACE_NAME_FOR_S1_MME:0:4}" != "anone" ]; then
        ping -q -c 1 $MME_IPV4_ADDRESS_FOR_S1_MME > /dev/null 2>&1
        if [ $? -ne 0 ]; then echo_fatal "PING INTERFACE $MME_INTERFACE_NAME_FOR_S1_MME ERROR, ADDRESS IS $MME_IPV4_ADDRESS_FOR_S1_MME $LINENO"; fi;
    fi
    
    if [ "a${SGW_INTERFACE_NAME_FOR_S1U_S12_S4_UP:0:4}" != "anone" ]; then
        ping -q -c 1 $SGW_IPV4_ADDRESS_FOR_S1U_S12_S4_UP > /dev/null 2>&1
        if [ $? -ne 0 ]; then echo_fatal "PING INTERFACE $SGW_INTERFACE_NAME_FOR_S1U_S12_S4_UP ERROR, ADDRESS IS $SGW_IPV4_ADDRESS_FOR_S1U_S12_S4_UP $LINENO"; fi;
    fi
    
    if [ "a${ENB_INTERFACE_NAME_FOR_S1_MME:0:4}" != "anone" ]; then
        ping -q -c 1 $ENB_IPV4_ADDRESS_FOR_S1_MME > /dev/null 2>&1
        if [ $? -ne 0 ]; then echo_fatal "PING INTERFACE $ENB_INTERFACE_NAME_FOR_S1_MME ADDR ERROR, ADDRESS IS $ENB_IPV4_ADDRESS_FOR_S1_MME $LINENO"; fi;
    fi
    
    if [ "a${ENB_INTERFACE_NAME_FOR_S1U:0:4}" != "anone" ]; then
        ping -q -c 1 $ENB_IPV4_ADDRESS_FOR_S1U > /dev/null 2>&1
        if [ $? -ne 0 ]; then echo_fatal "PING INTERFACE $ENB_INTERFACE_NAME_FOR_S1U ERROR, ADDRESS IS $ENB_IPV4_ADDRESS_FOR_S1U $LINENO"; fi;
    fi
    
    if [ "a${MME_INTERFACE_NAME_FOR_S11_MME:0:4}" != "anone" ]; then
        ping -q -c 1 $MME_IPV4_ADDRESS_FOR_S11_MME > /dev/null 2>&1
        if [ $? -ne 0 ]; then echo_fatal "PING INTERFACE $MME_INTERFACE_NAME_FOR_S11_MME ERROR, ADDRESS IS $MME_IPV4_ADDRESS_FOR_S11_MME $LINENO"; fi;
    fi
    
    if [ "a${SGW_INTERFACE_NAME_FOR_S11:0:4}" != "anone" ]; then
        ping -q -c 1 $SGW_IPV4_ADDRESS_FOR_S11 > /dev/null 2>&1
        if [ $? -ne 0 ]; then echo_fatal "PING INTERFACE $SGW_INTERFACE_NAME_FOR_S11 ERROR, ADDRESS IS $SGW_IPV4_ADDRESS_FOR_S11 $LINENO"; fi;
    fi
    
    if [ "a${MME_INTERFACE_NAME_FOR_S6A:0:4}" != "anone" ]; then
        ping -q -c 1 $MME_IPV4_ADDRESS_FOR_S6A > /dev/null 2>&1
        if [ $? -ne 0 ]; then echo_fatal "PING INTERFACE $MME_INTERFACE_NAME_FOR_S6A ERROR, ADDRESS IS $MME_IPV4_ADDRESS_FOR_S6A $LINENO"; fi;
    fi
    
    if [ "a${HSS_INTERFACE_NAME_FOR_S6A:0:4}" != "anone" ]; then
        ping -q -c 1 $HSS_IPV4_ADDRESS_FOR_S6A > /dev/null 2>&1
        if [ $? -ne 0 ]; then echo_fatal "PING INTERFACE $HSS_INTERFACE_NAME_FOR_S6A ERROR, ADDRESS IS $HSS_IPV4_ADDRESS_FOR_S6A $LINENO"; fi;
    fi
    
    ## TEST NETWORK BETWEEN ENB-MME-SP-GW
    
    if [ "a${MME_INTERFACE_NAME_FOR_S1_MME:0:4}" != "anone" ] && [ "a${ENB_INTERFACE_NAME_FOR_S1_MME:0:4}" != "anone" ]; then
        test_local_iperf $ENB_IPV4_ADDRESS_FOR_S1_MME  $MME_IPV4_ADDRESS_FOR_S1_MME 
    fi
    
    if [ "a${ENB_INTERFACE_NAME_FOR_S1U:0:4}" != "anone" ] && [ "a${SGW_INTERFACE_NAME_FOR_S1U_S12_S4_UP:0:4}" != "anone" ]; then
        test_local_iperf $ENB_IPV4_ADDRESS_FOR_S1U     $SGW_IPV4_ADDRESS_FOR_S1U_S12_S4_UP
    fi
    
    if [ "a${MME_INTERFACE_NAME_FOR_S11_MME:0:4}" != "anone" ] && [ "a${SGW_INTERFACE_NAME_FOR_S11:0:4}" != "anone" ]; then
        test_local_iperf $MME_IPV4_ADDRESS_FOR_S11_MME $SGW_IPV4_ADDRESS_FOR_S11
    fi
    
    if [ "a${MME_INTERFACE_NAME_FOR_S6A:0:4}" != "anone" ]  &&  [ "a${HSS_INTERFACE_NAME_FOR_S6A:0:4}" != "anone" ]; then
        test_local_iperf $MME_IPV4_ADDRESS_FOR_S6A     $HSS_IPV4_ADDRESS_FOR_S6A
    fi

    # Get MAC address of router.eur
    ping -c 1 hss.eur > /dev/null || { echo_fatal "hss.eur does not respond to ping" >&2 ; }
    ping -c 1 router.eur > /dev/null || { echo_fatal "router.eur does not respond to ping" >&2 ; }
    return 0
}

clean_network() {
  interfaces=`ifconfig | grep HWaddr | cut -d " " -f1-2 | tr -d '\n'`
  for interface in $interfaces
  do
      
      is_vlan_interface $interface
      if [ $? -eq 1 ]; then
         echo_success "Found VLAN interface $interface ... deleting"
         delete_vlan_interface  $interface  
      fi
  done
}


delete_sgi_vlans() {
    
    for i in 5 6 7 8 9 10 11 12 13 14 15
    do
        delete_vlan_interface  $PGW_INTERFACE_NAME_FOR_SGI.$i 
    done
}



create_sgi_vlans() {

    get_mac_router

    bash_exec "modprobe 8021q"
    
    bash_exec "ip link set $PGW_INTERFACE_NAME_FOR_SGI promisc on"

    for i in 5 6 7 8 9 10 11 12 13 14 15
    do
        # create vlan interface
        delete_vlan_interface  $PGW_INTERFACE_NAME_FOR_SGI.$i 
        sync
        create_vlan_interface  $PGW_INTERFACE_NAME_FOR_SGI.$i
        sync
        # configure vlan interface
        base=200
        NET=$(( $i + $base ))
        set_interface_up $PGW_INTERFACE_NAME_FOR_SGI.$i "10.0."$NET".2" 24
        
    done
}



