#!/usr/bin/python
import time
import serial
import os
from pyroute2 import IPRoute
import sys
import re
import threading
import signal
import traceback
import commands

if os.environ.get('OPENAIR_DIR') == None:
  print "Error getting OPENAIR_DIR environment variable"
  sys.exit(1)

sys.path.append(os.path.expandvars('$OPENAIR_DIR/cmake_targets/autotests/tools/'))

from lib_autotest import *

def reset_bladerf():
  stringIdBladeRF='OpenMoko, Inc'
  status, out = commands.getstatusoutput('lsusb | grep -i \'' + stringIdBladeRF + '\'')
  if (out == '') :
     print "BladeRF not found. Exiting now..."
     sys.exit()
  p=re.compile('Bus\s*(\w+)\s*Device\s*(\w+):\s*ID\s*(\w+):(\w+)')
  res=p.findall(out)
  BusId=res[0][0]
  DeviceId=res[0][1]
  VendorId=res[0][2]
  ProductId=res[0][3]
  usb_dir= find_usb_path(VendorId, ProductId)
  print "BladeRF Found in directory..." + usb_dir
  cmd = "sudo sh -c \"echo 0 > " + usb_dir + "/authorized\""
  os.system(cmd + " ; sleep 5" )
  cmd = "sudo sh -c \"echo 1 > " + usb_dir + "/authorized\""
  os.system(cmd + " ; sleep 5" )

os.system ('sudo -E bladeRF-cli --flash-firmware /usr/share/Nuand/bladeRF/bladeRF_fw.img')

print "Resettting BladeRF..."

reset_bladerf()

os.system ("dmesg|tail")
