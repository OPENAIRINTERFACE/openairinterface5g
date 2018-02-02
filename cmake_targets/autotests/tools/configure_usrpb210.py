#!/usr/bin/python 
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

import time
import serial
import os
from pyroute2 import IPRoute
import sys
import re
import threading
import signal
import traceback
import os
import commands

# configure the serial connections (the parameters differs on the device you are connecting to)
#First we find an open port to work with
serial_port=''
openair_dir = os.environ.get('OPENAIR_DIR')
if openair_dir == None:
  print "Error getting OPENAIR_DIR environment variable"
  sys.exit(1)

sys.path.append(os.path.expandvars('$OPENAIR_DIR/cmake_targets/autotests/tools/'))

from lib_autotest import *

#Stop the USB BUS of USRPB210
def stop_usrpb210():
  stringIdBandrich='National Instruments Corp.'
  status, out = commands.getstatusoutput('lsusb | grep -i \'' + stringIdBandrich + '\'')
  if (out == '') :
     print "USRP B210 not found. Exiting now..."
     sys.exit()
  p=re.compile('Bus\s*(\w+)\s*Device\s*(\w+):\s*ID\s*(\w+):(\w+)')
  res=p.findall(out)
  BusId=res[0][0]
  DeviceId=res[0][1]
  VendorId=res[0][2]
  ProductId=res[0][3]
  usb_dir= find_usb_path(VendorId, ProductId)
  print "USRP B210 found in..." + usb_dir
  cmd = "sudo sh -c \"echo 0 > " + usb_dir + "/authorized\""
  os.system(cmd)

#Start the USB bus of USRP B210
def start_usrpb210():
  stringIdBandrich='National Instruments Corp.'  
  status, out = commands.getstatusoutput('lsusb | grep -i \'' + stringIdBandrich + '\'')
  if (out == '') :
     print "USRP B210 not found. Exiting now..."
     sys.exit()
  p=re.compile('Bus\s*(\w+)\s*Device\s*(\w+):\s*ID\s*(\w+):(\w+)')
  res=p.findall(out)
  BusId=res[0][0]
  DeviceId=res[0][1]
  VendorId=res[0][2]
  ProductId=res[0][3]
  usb_dir= find_usb_path(VendorId, ProductId)
  print "USRP B210 found in..." + usb_dir
  cmd = "sudo sh -c \"echo 1 > " + usb_dir + "/authorized\""
  os.system(cmd)

i=1
while i <  len(sys.argv):
    arg=sys.argv[i]
    if arg == '--start-usrpb210' :
        start_usrpb210()
    elif arg == '--stop-usrpb210' :
        stop_usrpb210()
    elif arg == '-h' :
        print "--stop-usrpb210:  Stop the USRP B210. It cannot be found in uhd_find_devices" 
        print "--start-usrpb210:  Start the USRP B210. It can now be found in uhd_find_devices"
    else :
        print " Script called with wrong arguments, arg = " + arg
        sys.exit()
    i = i +1


