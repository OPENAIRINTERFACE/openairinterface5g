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

import os
from pyroute2 import IPRoute
import sys
import re
import threading
import signal
import traceback
import commands

def read_file(filename):
  try:
    file = open(filename, 'r')
    return file.read()
  except Exception, e:
    # WE just ignore the exception as some files are probably not present
    #error = ' Filename ' + filename 
    #error = error + ' In function: ' + sys._getframe().f_code.co_name + ': *** Caught exception: '  + str(e.__class__) + " : " + str( e)
    #error = error + traceback.format_exc()
    #print error
    return ''

def find_usb_path(idVendor, idProduct):
  for root, dirs, files in os.walk("/sys/bus/usb/devices", topdown=False):
    for name in dirs:
        tmpdir= os.path.join(root, name)
        tmpidVendor = read_file(tmpdir+'/idVendor').replace("\n","")
        tmpidProduct = read_file(tmpdir+'/idProduct').replace("\n","")
        if tmpidVendor == idVendor and tmpidProduct == idProduct:
            return tmpdir
  return ''

