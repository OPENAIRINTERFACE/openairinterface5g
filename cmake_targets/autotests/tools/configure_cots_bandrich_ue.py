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
import os

# configure the serial connections (the parameters differs on the device you are connecting to)
#First we find an open port to work with
serial_port=''
ser=serial.Serial()
openair_dir = os.environ.get('OPENAIR_DIR')
if openair_dir == None:
  print "Error getting OPENAIR_DIR environment variable"
  sys.exit(1)

def find_open_port():
   global serial_port, ser
   max_ports=100
   if os.path.exists(serial_port) == True:
     return serial_port
   for port in range(2,100):
      serial_port = '/dev/ttyUSB'+str(port)
      if os.path.exists(serial_port) == True:
         print 'New Serial Port : ' + serial_port
         break

   ser = serial.Serial(port=serial_port)
   return

find_open_port()
print 'Using Serial port : ' + serial_port  
    
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
    def __init__(self, threadID, name, counter):
        threading.Thread.__init__(self)
        self.threadID = threadID
        self.name = name
        self.counter = counter
    def run(self):
        print "Starting " + self.name
        #Here we keep running pppd thread in indefinite loop as this script terminates sometimes
        #while 1:
        while 1:
           time.sleep(5) #Hard coded, do not reduce this number! 
           print "Starting wvdial now..."
           print 'exit_flag = ' + str(exit_flag)
           send_command('AT+CGATT=1','OK', 300)
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
        

def start_ue () :
   #print 'Enter your commands below.\r\nInsert "exit" to leave the application.'
   timeout=60 #timeout in seconds
   send_command('AT', 'OK' , timeout)
   send_command('AT+CFUN=1' , 'OK' , timeout)
   #send_command('AT+CGATT=0' , 'OK' , timeout)
   send_command('AT+CGATT=1','OK', 300)
   #os.system('wvdial -C ' + bandrich_ppd_config + ' &' )
   
   thread_ppp = pppThread(1, "ppp_thread", 1)
   thread_ppp.start()

   iface='ppp0'
   
   while 1:
     time.sleep ( 2)
     #Now we check if ppp0 interface is up and running
     try:
        if exit_flag == 1:
          break
        ip = IPRoute()
        idx = ip.link_lookup(ifname=iface)[0]
        os.system ('route add 192.172.0.1 ppp0')
        os.system ('ping -c 5 192.172.0.1')
        break
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

for arg in sys.argv[1:]:
    if arg == '--start-ue' :
        start_ue()
    elif arg == '--stop-ue' :
        stop_ue()
    else :
        print " Script called with wrong arguments, arg = " + arg
        sys.exit()
