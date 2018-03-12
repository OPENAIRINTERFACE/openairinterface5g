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

# Find a device ID by running sudo adb devices
# The device ID below is for Sony Xperia M4

device_id='YT9115PX1E' 


openair_dir = os.environ.get('OPENAIR_DIR')
if openair_dir == None:
  print "Error getting OPENAIR_DIR environment variable"
  sys.exit(1)

sys.path.append(os.path.expandvars('$OPENAIR_DIR/cmake_targets/autotests/tools/'))

from lib_autotest import *

def signal_handler(signal, frame):
        print('You pressed Ctrl+C!')
        print('Exiting now...')
        timeout=10
        exit_flag=1
        sys.exit(0)

signal.signal(signal.SIGINT, signal_handler)

# Find all the process IDs in a phone given the name of process
def kill_processes(name):
   print " Killing all processes by name..." + name
   while 1:
     cmd = 'sudo adb -s ' + device_id +' shell "ps |grep ' + name + '"' 
     status, out = commands.getstatusoutput(cmd)
     if status != 0:
       print "Error executing command to kill process " + name
       print "Error =" + out
       sys.exit(1)
     print "Out = " + out
     if out=='':
       break;
     out_arr = out.split()
     pid_to_kill = out_arr[1]
     print "Now killing process ID " + pid_to_kill + " on Phone" 
     cmd = 'sudo adb -s ' + device_id +' shell "kill -9 ' + pid_to_kill + '"' 
     status, out = commands.getstatusoutput(cmd)
     if status != 0:
       print "Error execting command to kill process " + name
       sys.exit(1)
     print "Out = " + out

def start_ue () :
   #print 'Enter your commands below.\r\nInsert "exit" to leave the application.'
   print 'Killing old iperf/ping sessions'
   kill_processes('iperf')
   kill_processes('iperf3')
   kill_processes('ping')
   print "Turning off airplane mode"
   os.system('sudo -E adb devices')
   os.system('sudo -E adb -s ' + device_id + ' shell \"settings put global airplane_mode_on 0; am broadcast -a android.intent.action.AIRPLANE_MODE --ez state false\"')

   while 1:
     time.sleep ( 2)
     #Now we check if ppp0 interface is up and running
     try:
        cmd = 'sudo adb -s ' + device_id + ' shell netcfg |grep UP'
        status, out = commands.getstatusoutput(cmd)
        if (out == '') :
            print "Waiting for UE to connect and get IP Address..."
        else :
            print "UE is now connected. IP Address settings are..." + out
            os.system('sleep 5')
            os.system ('sudo adb -s ' + device_id  + ' shell ping ' + gw)
            break
     except Exception, e:
        error = error + ' In function: ' + sys._getframe().f_code.co_name + ': *** Caught exception: '  + str(e.__class__) + " : " + str( e)
        error = error + traceback.format_exc()
        print error

    

def stop_ue():
   print "Turning on airplane mode"
   os.system('sudo adb devices')
   os.system('sudo adb -s ' + device_id + ' shell \"settings put global airplane_mode_on 1; am broadcast -a android.intent.action.AIRPLANE_MODE --ez state true\" ')
   print "Killing iperf/ping sessions"
   kill_processes('iperf')
   kill_processes('iperf3')
   kill_processes('ping')
   
i=1
gw='192.172.0.1'
while i <  len(sys.argv):
    arg=sys.argv[i]
    if arg == '--start-ue' :
        start_ue()
    elif arg == '--stop-ue' :
        stop_ue()
    elif arg == '-gw' :
        gw = sys.argv[i+1]
        i=i+1
    elif arg == '-h' :
        print "--stop-ue:  Stop the UE. Turn on airplane mode" 
        print "--start-ue:  Start the UE. Turn off airplane mode"
        print "-gw:  Specify the default gw as sometimes the gateway/route arguments are not set properly via wvdial"
    else :
        print " Script called with wrong arguments, arg = " + arg
        sys.exit()
    i = i +1


