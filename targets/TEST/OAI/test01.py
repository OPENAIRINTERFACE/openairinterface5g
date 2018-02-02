#! /usr/bin/python
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

# \file test01.py
# \brief test 01 for OAI
# \author Navid Nikaein
# \date 2013 - 2015
# \version 0.1
# @ingroup _test


import sys
import wave
import os
import time
import datetime
import getpass
import math #from time import clock 

import log
import case01
import case02
import case03
import case04
import case05

from  openair import *

debug = 0
pw =''
i = 0
dlsim=0
localshell=0
is_compiled = 0
timeout=2000

for arg in sys.argv:
    if arg == '-d':
        debug = 1
    elif arg == '-dd':
        debug = 2
    elif arg == '-p' :
        prompt2 = sys.argv[i+1]
    elif arg == '-w' :
        pw = sys.argv[i+1]
    elif arg == '-P' :
        dlsim = 1
    elif arg == '-l' :
        localshell = 1
    elif arg == '-c' :
        is_compiled = 1
    elif arg == '-t' :
        timeout = sys.argv[i+1]
    elif arg == '-h' :
        print "-d:  low debug level"
        print "-dd: high debug level"
        print "-p:  set the prompt"
        print "-w:  set the password for ssh to localhost"
        print "-l:  use local shell instead of ssh connection"
        print "-t:  set the time out in second for commands"
        sys.exit()
    i= i + 1     

try:  
   os.environ["OPENAIR1_DIR"]
except KeyError: 
   print "Please set the environment variable OPENAIR1_DIR in the .bashrc"
   sys.exit(1)

try:  
   os.environ["OPENAIR2_DIR"]
except KeyError: 
   print "Please set the environment variable OPENAIR2_DIR in the .bashrc"
   sys.exit(1)

try:  
   os.environ["OPENAIR_TARGETS"]
except KeyError: 
   print "Please set the environment variable OPENAIR_TARGETS in the .bashrc"
   sys.exit(1)

# get the oai object
host = os.uname()[1]
oai = openair('localdomain','localhost')
#start_time = time.time()  # datetime.datetime.now()
user = getpass.getuser()
if localshell == 0:
    try: 
        print '\n******* Note that the user <'+user+'> should be a sudoer *******\n'
        print '******* Connecting to the localhost to perform the test *******\n'
    
        if not pw :
            print "username: " + user 
            pw = getpass.getpass() 
        else :
            print "username: " + user 
            #print "password: " + pw 

        # issues in ubuntu 12.04
        #  oai.connect(user,pw)
        oai.connect2(user,pw) 
        #oai.get_shell()
    except :
        print 'Fail to connect to the local host'
        sys.exit(1)
else:
    pw = ''
    oai.connect_localshell()

cpu_freq = int(oai.cpu_freq())
if timeout == 2000 : 
    if cpu_freq <= 2000 : 
        timeout = 3000
    elif cpu_freq < 2700 :
        timeout = 2000 
    elif cpu_freq < 3300 :
        timeout = 1500
#print "cpu freq(MHz): " + str(cpu_freq) + "timeout(s): " + str(timeout)


test = 'test01'
ctime=datetime.datetime.utcnow().strftime("%Y-%m-%d.%Hh%M")
logfile = user+'.'+test+'.'+ctime+'.txt'  
logdir = os.getcwd() + '/pre-ci-logs-'+host;
oai.create_dir(logdir,debug)    
print 'log dir: ' + logdir
#oai.send_nowait('mkdir -p -m 755' + logdir + ';')

#print '=================start the ' + test + ' at ' + ctime + '=================\n'
#print 'Results will be reported in log file : ' + logfile
log.writefile(logfile,'====================start'+test+' at ' + ctime + '=======================\n')
log.set_debug_level(debug)

oai.kill(user, pw)   
oai.rm_driver(oai,user,pw)

# start te test cases 
if is_compiled == 0 :
    is_compiled=case01.execute(oai, user, pw, host,logfile,logdir,debug,timeout)
    
if is_compiled != 0 :
    case02.execute(oai, user, pw, host, logfile,logdir,debug)
    case03.execute(oai, user, pw, host, logfile,logdir,debug)
    case04.execute(oai, user, pw, host, logfile,logdir,debug)
    case05.execute(oai, user, pw, host, logfile,logdir,debug)
else :
    print 'Compilation error: skip test case 02,03,04,05'

oai.kill(user, pw) 
oai.rm_driver(oai,user,pw)

# perform the stats
log.statistics(logfile)


oai.disconnect()

ctime=datetime.datetime.utcnow().strftime("%Y-%m-%d_%Hh%M")
log.writefile(logfile,'====================end the '+ test + ' at ' + ctime +'====================')
print 'Test results can be found in : ' + logfile 
#print '\nThis test took %f minutes\n' % math.ceil((time.time() - start_time)/60) 

#print '\n=====================end the '+ test + ' at ' + ctime + '====================='
