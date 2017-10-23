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

