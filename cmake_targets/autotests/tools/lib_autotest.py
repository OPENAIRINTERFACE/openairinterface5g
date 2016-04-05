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

