#!/usr/bin/python

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

