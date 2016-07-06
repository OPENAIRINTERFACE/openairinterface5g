#!/usr/bin/python 
#******************************************************************************

#    OpenAirInterface 
#    Copyright(c) 1999 - 2014 Eurecom

#    OpenAirInterface is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.


#    OpenAirInterface is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.

#   You should have received a copy of the GNU General Public License
#   along with OpenAirInterface.The full GNU General Public License is 
#   included in this distribution in the file called "COPYING". If not, 
#   see <http://www.gnu.org/licenses/>.

#  Contact Information
#  OpenAirInterface Admin: openair_admin@eurecom.fr
#  OpenAirInterface Tech : openair_tech@eurecom.fr
#  OpenAirInterface Dev  : openair4g-devel@lists.eurecom.fr
  
#  Address      : Eurecom, Campus SophiaTech, 450 Route des Chappes, CS 50193 - 06904 Biot Sophia Antipolis cedex, FRANCE

# *******************************************************************************/
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


