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
from socket import AF_INET
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
ser=serial.Serial()
openair_dir = os.environ.get('OPENAIR_DIR')
if openair_dir == None:
  print "Error getting OPENAIR_DIR environment variable"
  sys.exit(1)

sys.path.append(os.path.expandvars('$OPENAIR_DIR/cmake_targets/autotests/tools/'))

from lib_autotest import *

def find_open_port():
   global serial_port, ser
   max_ports=100
   serial_port=''
   while True:
     if os.path.exists(serial_port) == True:
       return serial_port
     for port in range(0,100):
        serial_port_tmp = '/dev/ttyUSB'+str(port)
        if os.path.exists(serial_port_tmp) == True:
           print 'New Serial Port : ' + serial_port_tmp
           serial_port = serial_port_tmp
           break
     if serial_port == '':
        print" Not able to detect valid serial ports. Resetting the modem now..."
        reset_ue()
     else :
        ser = serial.Serial(port=serial_port)
        return

    
#serial_port = '/dev/ttyUSB2'
bandrich_ppd_config = os.environ.get('OPENAIR_DIR') + '/cmake_targets/autotests/tools/wdial.bandrich.conf'

exit_flag=0

def signal_handler(signal, frame):
        print('You pressed Ctrl+C!')
        print('Resetting the UE to detached state')
        timeout=10
        exit_flag=1
        send_command('AT+CGATT=0' , 'OK' , timeout)
        sys.exit(0)

signal.signal(signal.SIGINT, signal_handler)



#ser.open()
#ser.isOpen()

class pppThread (threading.Thread):
    def __init__(self, threadID, name, counter,port):
        threading.Thread.__init__(self)
        self.threadID = threadID
        self.name = name
        self.counter = counter
        self.port=port
    def run(self):
        print "Starting " + self.name
        #Here we keep running pppd thread in indefinite loop as this script terminates sometimes
        #while 1:
        while 1:
           time.sleep(5) #Hard coded, do not reduce this number! 
           print "Starting wvdial now..."
           print 'exit_flag = ' + str(exit_flag)
           send_command('AT+CGATT=1','OK', 300)
           
           #Now we do search and replace on wvdial config file
           cmd="sed -i \"s%Modem = .*%Modem = " + self.port + "%g\" " +  bandrich_ppd_config
           os.system(cmd)
           os.system('wvdial -C ' + bandrich_ppd_config + '' )
           if exit_flag == 1:
              print "Exit flag set to true. Exiting pppThread now"
           print "Terminating wvdial now..."

def send_command (cmd, response, timeout):
   count=0
   sleep_duration = 1
   print 'In function: send_command: cmd = <' + cmd + '> response: <' + response + '> \n'
   global serial_port, ser
   while count <= timeout:
      try:
        #Sometimes the port does not exist coz of reset in modem.
        #In that case, we need to search for this port again
        if os.path.exists(serial_port) == False:
            find_open_port()
        ser.write (cmd + '\r\n')
        out = ''
        time.sleep(sleep_duration)
        count = count + sleep_duration
        while ser.inWaiting() > 0:
            out += ser.read(1)
        print 'out = <' + out + '> response = <' + response + '> \n'
        if re.search(response, out):
          break
      except Exception, e:
        error = ' cmd : ' + cmd + ' response : ' + response
        error = error + ' In function: ' + sys._getframe().f_code.co_name + ': *** Caught exception: '  + str(e.__class__) + " : " + str( e)
        error = error + traceback.format_exc()
        print error
        time.sleep(1)
        

def start_ue () :
   #print 'Enter your commands below.\r\nInsert "exit" to leave the application.'
   global serial_port
   timeout=60 #timeout in seconds
   send_command('AT', 'OK' , timeout)
   send_command('AT+CFUN=1' , 'OK' , timeout)
   #send_command('AT+CGATT=0' , 'OK' , timeout)
   send_command('AT+CGATT=1','OK', 300)
   #os.system('wvdial -C ' + bandrich_ppd_config + ' &' )
   
   thread_ppp = pppThread(1, "ppp_thread", 1,port=serial_port)
   thread_ppp.start()

   #iface='ppp0'
   
   while 1:
     time.sleep ( 2)
     iface=''
     #Now we check if ppp0 interface is up and running
     try:
        if exit_flag == 1:
          break
        cmd="ifconfig -a | sed 's/[ \t].*//;/^$/d' | grep ppp"
        status, out = commands.getstatusoutput(cmd)
        iface=out
        ip = IPRoute()
        idx = ip.link_lookup(ifname=iface)[0]
        print "iface = " + iface
        print " Setting route now..."
        #os.system("status=1; while [ \"$status\" -ne \"0\" ]; do route add -host " + gw + ' ' + iface + " ; status=$? ;sleep 1; echo \"status = $status\"  ; sleep 2; done ")
        os.system ('route add -host ' + gw + ' ' + iface + ' 2> /dev/null')
        #ip.route('add', dst=gw, oif=iface)
        
        os.system('sleep 5')
        #print "Starting ping now..."
        os.system ('ping -c 1 ' + gw)
        #break
     except Exception, e:
        error = ' Interface ' + iface + 'does not exist...'
        error = error + ' In function: ' + sys._getframe().f_code.co_name + ': *** Caught exception: '  + str(e.__class__) + " : " + str( e)
        error = error + traceback.format_exc()
        print error
    
   thread_ppp.join()

def stop_ue():
   timeout=60
   os.system('killall wvdial')
   send_command('AT', 'OK' , timeout)
   send_command('AT+CGATT=0' , 'OK|ERROR' , timeout)
   send_command('AT+CFUN=4' , 'OK' , timeout)


#reset the USB BUS of Bandrich UE
def reset_ue():
  stringIdBandrich='BandRich, Inc. 4G LTE adapter'
  status, out = commands.getstatusoutput('lsusb | grep -i \'' + stringIdBandrich + '\'')
  if (out == '') :
     print "Bandrich 4G LTE Adapter not found. Exiting now..."
     sys.exit()
  p=re.compile('Bus\s*(\w+)\s*Device\s*(\w+):\s*ID\s*(\w+):(\w+)')
  res=p.findall(out)
  BusId=res[0][0]
  DeviceId=res[0][1]
  VendorId=res[0][2]
  ProductId=res[0][3]
  usb_dir= find_usb_path(VendorId, ProductId)
  print "Bandrich 4G LTE Adapter found in..." + usb_dir
  print "Sleeping now for 45 seconds...please wait..."
  cmd = "sudo sh -c \"echo 0 > " + usb_dir + "/authorized\""
  os.system(cmd + " ; sleep 15" )
  cmd = "sudo sh -c \"echo 1 > " + usb_dir + "/authorized\""
  os.system(cmd + " ; sleep 30" )
  find_open_port()
  stop_ue()

i=1
gw='192.172.0.1'
while i <  len(sys.argv):
    arg=sys.argv[i]
    if arg == '--start-ue' :
        print "Turning on UE..."
        find_open_port()
        print 'Using Serial port : ' + serial_port  
        start_ue()
    elif arg == '--stop-ue' :
        print "Turning off UE..."
        find_open_port()
        print 'Using Serial port : ' + serial_port  
        stop_ue()
    elif arg == '--reset-ue' :
        print "Resetting UE..."
        find_open_port()
        reset_ue()
    elif arg == '-gw' :
        gw = sys.argv[i+1]
        i=i+1
    elif arg == '-h' :
        print "--reset-ue:  Reset the UE on USB Bus. Similar to unplugging and plugging the UE"
        print "--stop-ue:  Stop the UE. Send DETACH command" 
        print "--start-ue:  Start the UE. Send ATTACH command"
        print "-gw:  Specify the default gw as sometimes the gateway/route arguments are not set properly via wvdial"
    else :
        print " Script called with wrong arguments, arg = " + arg
        sys.exit()
    i = i +1


