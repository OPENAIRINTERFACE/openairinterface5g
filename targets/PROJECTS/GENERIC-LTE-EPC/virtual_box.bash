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

export HSS_VM_NAME='hss-vm-ubuntu-12.04.4-server-amd64'
export MME_VM_NAME='mme-vm-ubuntu-12.04.4-server-amd64'
export OS_INSTALL_IMAGE="/root/ubuntu-12.04.4-server-amd64.iso"
export OS_TYPE="Ubuntu"

export HOST_BRIDGED_IF_NAME="eth1"
export DEFAULT_VIRTUAL_BOX_VM_PATH='/root/VirtualBox VMs'
###########################################################
THIS_SCRIPT_PATH=$(dirname $(readlink -f $0))
. ./utils.bash
###########################################################

#   NETWORK SETTING AT EURECOM IN EXPERIMENTAL NETWORK (192.168.12.X)
#
#
#             INTERNET GW 192.168.12.100      
#                                  |
#                                  |
#                 192.168.12.17/24 | 
# +-----------+----------------+---+--+---------------------------------------+
# | COMPUTER 1|                | eth0 |                                       |
# +-----------+                +---+--+                                       |
# |                                |                                          |
# |                                |                                          |
# |                          +-----+------+                                   |
# |                          |MASQUERADING|                                   |
# |                          +-----+------+                                   |
# |                                |                                          |
# |                                |                                          |
# |                                |                                          |
# |                                |                                          |
# |                                |                                          |
# |                            +---+--+                                       |
# |                            | eth1 |                                       |
# +----------------------------+---+--+---------------------------------------+
#                192.168.13.17/24  |
#                                  |                 INTERNET GW 192.168.12.100
#                                  |                            |
#                192.168.13.175/24 |          192.168.12.175/24 |
# +-----------+----------------+---+--+---------------------+---+--+----------+
# | COMPUTER 2|                | eth1 | Physical            | eth0 |          |
# +-----------+                +-+--+-+ Interface           +------+          |
# |                              |  |  'HOST_BRIDGED_IF_NAME'                 |
# |                              |  |                                         |
# |                              |  |                 +-----------+           |   
# |                              |  |          +------+    HSS    |           |   
# |                              |  +----------+ eth0 |   (VM)    |           |   
# |                              |             +------+           |           |
# |              bridged network |   192.168.13.177   |           |           |
# |                              |                    +-+------+--+           |
# |                              |                      |eth1  |192.168.57.101|
# |                              |                      +--+---+              |
# |                              |                         |                  |
# |                              |   192.168.13.178        | host             |
# |                              +------------------+      | network vboxnet1 |
# |                                                 |      | 192.168.57.0/24  |
# |                                                 |      | 192.168.57.100   |
# |                                               +-+--++--+--+               |
# |                                               |eth0||eth1 |               |
# |+-----------+192.168.56.1      192.168.56.100+-+----+------+---+           |
# || LTE eNB 1 +--------+              +--------+     MME         |           |
# ||  (HOST)   |vboxnet0+--------------+ eth2   |     S+P/GW      |           |
# ||           +--------+ host         +--------+     (VM)        |           |
# ||           |          network vboxnet0      |                 |           |
# ||           |       192.168.56.0/24          |                 |           |
# ||           +--------+              +--------+                 |           |
# || LTE n UEs |vboxnet2+--------------+ eth3   |                 |           |
# |+-----------+--------+ host         +--------+-----------------+           |
# |       192.168.58.1    network vboxnet2 192.168.58.100                     |
# |                       192.168.58.0/24                                     |
# +---------------------------------------------------------------------------+
#


check_install_vbox_software

build_vbox_vm_mme() {

    if [ $OS_TYPE ]; then
        echo_success "OS_TYPE: $OS_TYPE"
    else
        echo_fatal "OS_TYPE bash variable must be set to any of the following values (The type of OS you want to install on your VM): `VBoxManage list ostypes | grep ID | cut -d: -f2 | tr -d ' ' | sed -n -e 'H;${x;s/\n/,/g;s/^,//;p;}'`"
    fi

    if [ $MME_VM_NAME ]; then
        echo_success "MME_VM_NAME: $MME_VM_NAME"
    else
        echo_fatal "MME_VM_NAME bash variable must be set"
    fi

    if [ $OS_INSTALL_IMAGE ]; then
        if [[ ! -e "$OS_INSTALL_IMAGE" ]] ; then  
            echo_fatal "$OS_INSTALL_IMAGE: File not found"
        else
            echo_success "OS_INSTALL_IMAGE: $OS_INSTALL_IMAGE"
        fi
    else
        echo_fatal "OS_INSTALL_IMAGE bash variable must be set to a valid ISO OS installation file"
    fi

    bash_exec "VBoxManage unregistervm $MME_VM_NAME --delete"
    rm -Rf  "$DEFAULT_VIRTUAL_BOX_VM_PATH"/"$MME_VM_NAME"
 
    bash_exec "VBoxManage createvm --name $MME_VM_NAME --register"

    UUID=`VBoxManage showvminfo  $MME_VM_NAME | grep Hardware\ UUID | cut -d: -f2 | tr -d ' '` 

    echo ENB UUID=$UUID

    bash_exec "VBoxManage modifyvm  $UUID --ostype $OS_TYPE --memory 512 --vram 12 --cpus 1 \
                                          --rtcuseutc on --cpuhotplug off --cpuexecutioncap 100 --pae on --hpet on \
                                          --hwvirtex on --nestedpaging on \
                                          --firmware bios --biosbootmenu messageandmenu --boot1 dvd --boot2 disk \
                                          --nic1 bridged        --nic2 hostonly      --nic3 hostonly      --nic4 hostonly \
                                          --nictype1 82545EM    --nictype2 82545EM   --nictype3 82545EM   --nictype4 82545EM \
                                          --cableconnected1 on  --cableconnected2 on --cableconnected3 on --cableconnected4 on \
                                          --macaddress1 c8d3a3020201 --macaddress2 c8d3a3020202 \
                                          --macaddress3 c8d3a3020203 --macaddress4 c8d3a3020204 \
                                          --bridgeadapter1 $HOST_BRIDGED_IF_NAME \
                                          --hostonly2 vboxnet1    \
                                          --hostonly3 vboxnet0    \
                                          --hostonly4 vboxnet2    \
                                          --nicpromisc1 allow-all \
                                          --nicpromisc2 allow-all \
                                          --nicpromisc3 allow-all \
                                          --nicpromisc4 allow-all \                                   
                                          --audio none            \
                                          --usb off --usbehci off"
                                          

#    BRIDGED_MAC_ADDRESS=`vboxmanage showvminfo $UUID | grep Bridged\ Interface |  sed 's/^.*MAC:\ //'  |  cut --delimiter=',' -f 1`

    mkdir -p -m 777 "$DEFAULT_VIRTUAL_BOX_VM_PATH"/"$MME_VM_NAME"/MKISO
    rm -Rf "$DEFAULT_VIRTUAL_BOX_VM_PATH"/"$MME_VM_NAME"/MKISO/*

    chmod -Rf 777 "$DEFAULT_VIRTUAL_BOX_VM_PATH"/"$MME_VM_NAME"/MKISO
#    mkisofs -max-iso9660-filenames -untranslated-filenames -no-iso-translate -o "$DEFAULT_VIRTUAL_BOX_VM_PATH"/"$MME_VM_NAME"/cd_guest_setup.iso "$DEFAULT_VIRTUAL_BOX_VM_PATH"/"$MME_VM_NAME"/MKISO/                                         
                                          
    VBoxManage createhd --filename "$DEFAULT_VIRTUAL_BOX_VM_PATH"/"$MME_VM_NAME"/"$MME_VM_NAME".vdi --size 16384 --format VDI
                                          
    VBoxManage storagectl $UUID --name "IDE Controller" --add ide --controller PIIX4 --hostiocache on --bootable on
    VBoxManage storageattach $UUID --storagectl "IDE Controller" --type dvddrive --passthrough off --port 1 --device 0 --medium $OS_INSTALL_IMAGE

#    VBoxManage storageattach $UUID --storagectl "IDE Controller" --type dvddrive --passthrough off --port 1 --device 1 --medium "$DEFAULT_VIRTUAL_BOX_VM_PATH"/"$MME_VM_NAME"/cd_guest_setup.iso

    VBoxManage storagectl $UUID --name "SATA Controller" --add sata --controller IntelAhci --sataportcount 4 --hostiocache off --bootable on
    VBoxManage storageattach $UUID --storagectl "SATA Controller" --type hdd --port 0 --device 0 --medium "$DEFAULT_VIRTUAL_BOX_VM_PATH"/"$MME_VM_NAME"/"$MME_VM_NAME".vdi
  
    bash_exec "VBoxManage sharedfolder add $MME_VM_NAME --name $TRUNK_SHARED_FOLDER_NAME --hostpath $OPENAIR_DIR"                                          
}

build_vbox_vm_hss() {

    UUID=`VBoxManage clonevm $MME_VM_NAME --mode all --name $HSS_VM_NAME --register`
    HSS_UUID=`VBoxManage showvminfo  $HSS_VM_NAME | grep Hardware\ UUID | cut -d: -f2 | tr -d ' '` 
    echo HSS_UUID=$HSS_UUID

    bash_exec "VBoxManage modifyvm  $HSS_UUID --ostype $OS_TYPE --memory 512 --vram 12 --cpus 1 \
                                          --rtcuseutc on --cpuhotplug off --cpuexecutioncap 100 --pae on --hpet on \
                                          --hwvirtex on --nestedpaging on \
                                          --firmware bios --biosbootmenu messageandmenu --boot1 disk \
                                          --nic1 bridged        --nic2 hostonly      --nic3 none           --nic4 none \
                                          --nictype1 82545EM    --nictype2 82545EM   --nictype3 82545EM    --nictype4 82545EM \
                                          --cableconnected1 on  --cableconnected2 on --cableconnected3 off --cableconnected4 off \
                                          --macaddress1 c8d3a3020101 --macaddress2 c8d3a3020102 \
                                          --macaddress3 c8d3a3020103 --macaddress4 c8d3a3020104 \
                                          --bridgeadapter1 $HOST_BRIDGED_IF_NAME \
                                          --hostonly2 vboxnet1 \
                                          --nicpromisc1 allow-all \
                                          --nicpromisc2 allow-all \
                                          --audio none \
                                          --usb off --usbehci off"
}



build_vms() {
    build_vbox_vm_mme
    build_vbox_vm_hss
    echo_warning "!!!!!!! once VM are created, you have to harmonize IP addresses and MAC addresses !!!!!!!" 
    echo_warning "!!!!!!! /etc/network/interfaces and /etc/udev/rules.d/70-persistent-net.rules     !!!!!!!" 
    echo_warning "!!!!!!!                                                                           !!!!!!!" 
    echo_warning "!!!!!!! to share open air source code: use sshfs (you can use vbox shared folders,!!!!!!!"
    echo_warning "!!!!!!! in this case, help yourself...)                                           !!!!!!!"
    echo_warning "!!!!!!! exchange ssh keys between host and guests                                 !!!!!!!"
    echo_warning "!!!!!!! in /etc/fstab on guests: add following line:                              !!!!!!!"
    echo_warning "!!!!!!! sshfs#root@<IP@ of host>:/root/trunk  /mnt/sshfs/trunk fuse comment=sshfs !!!!!!!"
    echo_warning "!!!!!!! ,noauto,users,exec,uid=0,gid=0,allow_other,reconnect,transform_symlinks,  !!!!!!!"
    echo_warning "!!!!!!! BatchMode=yes 0 0                                                         !!!!!!!"
    echo_warning "!!!!!!! on guest: create a mount point: /mnt/sshfs/trunk for example, then mount: !!!!!!!"
    echo_warning "!!!!!!! mount /mnt/sshfs/trunk                                                    !!!!!!!"
    
}

##########################
#                        #
#  A N N E X             #
#                        #
##########################
################################################################################
## MME: FILE /etc/network/interfaces (Remove one '#' at each start of line)
################################################################################
## This file describes the network interfaces available on your system
## and how to activate them. For more information, see interfaces(5).

## The loopback network interface
#auto lo
#iface lo inet loopback

## The primary network interface
#auto eth0
#iface eth0 inet static
#        address 192.168.13.178
#        netmask 255.255.255.0
#        network 192.168.13.0
#        broadcast 192.168.13.255
#        gateway 192.168.13.17
#        # dns-* options are implemented by the resolvconf package, if installed
#        dns-nameservers 192.168.12.100
#
#auto eth2
#iface eth2 inet static
#        address 192.168.56.100
#        netmask 255.255.255.0
#        network 192.168.56.0
#        broadcast 192.168.56.255
#
#auto eth1
#iface eth1 inet static
#        address 192.168.57.100
#        netmask 255.255.255.0
#        network 192.168.57.0
#        broadcast 192.168.57.255
#
#auto eth3
#iface eth3 inet static
#        address 192.168.58.100
#        netmask 255.255.255.0
#        network 192.168.58.0
#        broadcast 192.168.58.255


################################################################################
## HSS: FILE /etc/network/interfaces (Remove one '#' at each start of line)
################################################################################
## This file describes the network interfaces available on your system
## and how to activate them. For more information, see interfaces(5).
#
## The loopback network interface
#auto lo
#iface lo inet loopback
#
## The primary network interface
#auto eth0
#iface eth0 inet static
#        address 192.168.13.177
#        netmask 255.255.255.0
#        network 192.168.13.0
#        broadcast 192.168.13.255
#        gateway 192.168.13.17
#        # dns-* options are implemented by the resolvconf package, if installed
#        dns-nameservers 192.168.12.100
#
#auto eth1 
#iface eth1 inet static
#        address 192.168.57.101
#        netmask 255.255.255.0
#        network 192.168.57.0
#        broadcast 192.168.57.255 

