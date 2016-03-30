#!/bin/bash

#arg1 idVendor
#arg2 idProduct
argIdVendor=$1
argIdProduct=$2

echo $1
echo $2

for X in /sys/bus/usb/devices/*; do 
    #echo "$X"
    idVendor=`cat "$X/idVendor" 2>/dev/null` 
    idProduct=`cat "$X/idProduct" 2>/dev/null`
    if [ "$argIdVendor" == "$idVendor" ] && [ "$argIdProduct" == "$idProduct" ]
    then
      echo "$X"
    fi
done

