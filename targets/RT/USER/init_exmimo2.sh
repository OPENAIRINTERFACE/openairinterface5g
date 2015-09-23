#!/bin/bash

PCI=`lspci -m | grep Xilinx`
if [ -z "$PCI" ]; then
 echo "No card found. Stopping!"
 return
fi

## This part corrects the wrong configuration of the endpoint done by the bios in some machines
echo "$PCI" | while read config_reg; do
SLOT_NUMBER=`echo $config_reg | awk -F\" '{print $1}'`
sudo setpci -s $SLOT_NUMBER 60.b=10
done

load_module() {
  mod_name=${1##*/}
  mod_name=${mod_name%.*}
  if awk "/$mod_name/ {found=1 ;exit} END {if (found!=1) exit 1}" /proc/modules
    then
      echo "module $mod_name already loaded: I remove it first"
      sudo rmmod $mod_name
  fi
  echo loading $mod_name
  sudo insmod $1
}

load_module $OPENAIR_TARGETS/ARCH/EXMIMO/DRIVER/eurecom/openair_rf.ko
sleep 1

if [ ! -e /dev/openair0 ]; then 
 sudo mknod /dev/openair0 c 127 0
 sudo chmod a+rw /dev/openair0
fi

DEVICE=`echo $PCI | awk -F\" '{print $(NF-1)}' | awk '{print $2}'`
DEVICE_SWID=${DEVICE:2:2}
if [ $DEVICE_SWID == '08' ]; then
 echo "Using firmware version 8"
 $OPENAIR_TARGETS/ARCH/EXMIMO/USERSPACE/OAI_FW_INIT/updatefw -s 0x43fffff0 -b -f $OPENAIR_TARGETS/ARCH/EXMIMO/USERSPACE/OAI_FW_INIT/sdr_expressmimo2
else 
 if [ $DEVICE_SWID == '09' ]; then
  echo "Using firmware version 9"
  $OPENAIR_TARGETS/ARCH/EXMIMO/USERSPACE/OAI_FW_INIT/updatefw -s 0x43fffff0 -b -f $OPENAIR_TARGETS/ARCH/EXMIMO/USERSPACE/OAI_FW_INIT/sdr_expressmimo2_v9
 else
  if [ $DEVICE_SWID == '0a' ]; then
   echo "Using firware version 10"
   #$OPENAIR_TARGETS/ARCH/EXMIMO/USERSPACE/OAI_FW_INIT/updatefw -s 0x43fffff0 -b -f $OPENAIR0_DIR/express-mimo/software/sdr/exmimo2/sdr_expressmimo2
   #$OPENAIR_TARGETS/ARCH/EXMIMO/USERSPACE/OAI_FW_INIT/updatefw -s 0x43fffff0 -b -f $OPENAIR_TARGETS/ARCH/EXMIMO/USERSPACE/OAI_FW_INIT/sdr_expressmimo2_v10_spectra
   $OPENAIR_TARGETS/ARCH/EXMIMO/USERSPACE/OAI_FW_INIT/updatefw -s 0x43fffff0 -b -f $OPENAIR_TARGETS/ARCH/EXMIMO/USERSPACE/OAI_FW_INIT/sdr_expressmimo2_v10
  else
   echo 'No corresponding firmware found'
   return
  fi
 fi
fi

#load_module $OPENAIR2_DIR/NAS/DRIVER/MESH/nasmesh.ko

#if [ "$1" = "eNB" ]; then 
#     echo "bring up oai0 interface for enb"
#     sudo ifconfig oai0 10.0.1.1 netmask 255.255.255.0 broadcast 10.0.1.255
#    $OPENAIR2_DIR/NAS/DRIVER/MESH/RB_TOOL/rb_tool -a -c0 -i0 -z0 -s 10.0.1.1 -t 10.0.1.9 -r 1
#else
#    if [ "$1" = "UE" ]; then 
#	echo "bring up oai0 interface for UE"
#	sudo ifconfig oai0 10.0.1.9 netmask 255.255.255.0 broadcast 10.0.1.255
#	$OPENAIR2_DIR/NAS/DRIVER/MESH/RB_TOOL/rb_tool -a -c0 -i0 -z0 -s 10.0.1.9 -t 10.0.1.1 -r 1
#    fi
#fi  
