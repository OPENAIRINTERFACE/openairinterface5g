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

# \file openair.py
# \brief class that define the oaisim class and its attributes
# \author Navid Nikaein
# \date 2013
# \version 0.1
# @ingroup _test

import pexpect
import pxssh
import time
import os
import array
import shutil
import subprocess 
import sys
import traceback
import time
# import call

from core import *

SHELL = '/bin/bash'

class openair(core):
    def __init__(self, hostname, address):
        self.error = '% '
        self.hostname = hostname
        self.address = address
        self.localhost = None
        core.__init__(self)
              
    @property        
    def localhost(self):
        if self.localhost :
            return self.localhost 
        elif self.hostname in ['localhost', '127.0.0.7', '::1'] :
            self.localhost = self.hostname
        return self.localhost

    @localhost.setter
    def localhost(self,localhost):
        self.localhost = localhost

    def shcmd(self,cmd,sudo=False):
        
        if sudo:
            cmd = "sudo %s" % command
        
        proc = subprocess.Popen(command, shell=True, 
                             stdout = subprocess.PIPE, 
                             stderr = subprocess.PIPE)
                       
        stdout, stderr = proc.communicate()
        return (stdout, stderr)

    def connect(self, username, password, prompt='PEXPECT_OAI'):
     max_retries=10
     i=0
     while i <= max_retries:  
        self.prompt1 = prompt
        self.prompt2 = prompt
        self.password = '' 
        i=i+1
        # WE do not store the password when sending commands for secuirity reasons. The password might be accidentally logged in such cases.
        #The password is used only to make ssh connections. In case user wants to run programs with sudo, then he/she needs to add following line in /etc/sudoers
        # your_user_name  ALL=(ALL:ALL) NOPASSWD: ALL
        try:
            if  not username:
                username = root 
            if  not password:
                password = username 
            self.oai = pxssh.pxssh()
            self.oai.login(self.address,username,password)
            self.oai.sendline('PS1='+self.prompt1)
            self.oai.PROMPT='PEXPECT_OAI'
            # need to look for twice the string of the prompt
            self.oai.prompt()
            self.oai.prompt()
#            self.oai.sendline('uptime')
#            self.oai.prompt()
#           print self.oai.before
            break
        except Exception, e:
            error=''
            error = error + ' In function: ' + sys._getframe().f_code.co_name + ': *** Caught exception: '  + str(e.__class__) + " : " + str( e)
            error = error + 'address = "'+ self.address +' username = ' + username
            error = error + traceback.format_exc()
            print error
            print "Retrying again in 1  seconds"
            time.sleep(1)
            if i==max_retries:
              print "Fatal Error: Terminating the program now..."
              sys.exit(1)
                
    def connect2(self, username, password, prompt='$'):
        self.prompt1 = prompt
        self.prompt2 = prompt
        self.password = password     
        while 1:
            try:
                if  not username:
                    username = root 
                if  not password:
                    password = username 
                    
                self.oai = pexpect.spawn('ssh -o "UserKnownHostsFile=/dev/null" -o "StrictHostKeyChecking=no" -o "ConnectionAttempts=1" ' \
                                             + username + '@' + self.address)
                
                index = self.oai.expect([re.escape(self.prompt1), re.escape(self.prompt2), pexpect.TIMEOUT], timeout=40)
                if index == 0 :
                    return 'Ok'
                else :
                    index = self.oai.expect(['password:', pexpect.TIMEOUT], timeout=40)
                    if index == 0 : 
                        self.oai.sendline(password)
                        index = self.oai.expect([re.escape(self.prompt1), re.escape(self.prompt2), pexpect.TIMEOUT], timeout=10)
                        if index != 0:
                            print 'ERROR! could not login with SSH.'
                            print 'Expected ' + self.prompt1 + ', received >>>>' + self.oai.before + '<<<<'
                            sys.exit(1) 
                    return 'Ok'
                        
            except Exception, e:
               error=''
               error = error + ' In function: ' + sys._getframe().f_code.co_name + ': *** Caught exception: '  + str(e.__class__) + " : " + str( e)
               error = error + traceback.format_exc()
               print error
               sys.exit(1)

    def connect_localshell(self, prompt='$'):
        self.prompt1 = prompt
        self.prompt2 = prompt

        while 1:
            try:
                # start a shell and use the current environment
                self.oai = pexpect.spawn('bash --norc --noprofile')
                
                index = self.oai.expect([re.escape(self.prompt1), re.escape(self.prompt2), pexpect.TIMEOUT], timeout=40)
                if index == 0 :
                    return 'Ok'
                else :
                    sys.exit(1)

            except Exception, e:
               error=''
               error = error + ' In function: ' + sys._getframe().f_code.co_name + ': *** Caught exception: '  + str(e.__class__) + " : " + str( e)
               error = error + traceback.format_exc()
               print error
               sys.exit(1)

    def disconnect(self):
#        print 'disconnecting the ssh connection to ' + self.address + '\n'
        self.oai.send('exit')
#        self.cancel()

    def kill(self, user, pw):
        try:
            if user == 'root' :
                os.system('pkill oaisim oaisim_nos1')
                os.system('pkill cc1') 
                time.sleep(1)
                os.system('pkill oaisim oaisim_nos1')
            else :
                os.system('echo '+pw+' | sudo -S pkill oaisim oaisim_nos1')
                os.system('echo '+pw+' | sudo -S pkill cc1') 
                time.sleep(1)
                os.system('echo '+pw+' | sudo -S pkill oaisim oaisim_nos1')
        except Exception, e:
               error=''
               error = error + ' In function: ' + sys._getframe().f_code.co_name + ': *** Caught exception: '  + str(e.__class__) + " : " + str( e)
               error = error + traceback.format_exc()
               print error
               #sys.exit(1)
            
    def rm_driver(self,oai,user, pw):
        try:
            if user == 'root' : 
                #oai.send_nowait('rmmod nasmesh;')
                os.system('rmmod nasmesh;')
            else :
                oai.send_nowait('echo '+pw+ ' | sudo -S rmmod nasmesh;')
                #os.system('echo '+pw+ ' | sudo -S rmmod nasmesh;')
        except Exception, e:
               error=''
               error = error + ' In function: ' + sys._getframe().f_code.co_name + ': *** Caught exception: '  + str(e.__class__) + " : " + str( e)
               error = error + traceback.format_exc()
               print error
               #sys.exit(1)
   
    def driver(self,oai,user,pw):
        #pwd = oai.send_recv('pwd') 
        oai.send('cd $OPENAIR_TARGETS;')   
        oai.send('cd SIMU/USER;')   
        try:
            if user == 'root' : 
                oai.send_nowait('insmod ./nasmesh.ko;')
            else :
                oai.send('echo '+pw+ ' | sudo -S insmod ./nasmesh.ko;')
                
        except Exception, e:
               error=''
               error = error + ' In function: ' + sys._getframe().f_code.co_name + ': *** Caught exception: '  + str(e.__class__) + " : " + str( e)
               error = error + traceback.format_exc()
               print error
               #sys.exit(1)
    
    def cleandir (self, logdir,debug) :
        
        for filename in os.listdir(logdir):
            filepath = os.path.join(logdir, filename)
            if debug == 2 :
                print 'logdir is ' + logdir
                print 'filepath is ' + filepath 
            try:
                shutil.rmtree(filepath)
            except Exception, e:
               error=''
               error = error + ' In function: ' + sys._getframe().f_code.co_name + ': *** Caught exception: '  + str(e.__class__) + " : " + str( e)
               error = error + traceback.format_exc()
               print error
               #sys.exit(1)
               #print 'Could not remove the filepath'+ filepath + ' with error ' + OSError
    
    def create_dir(self,dirname,debug) :
        if not os.path.exists(dirname) :
            try:
                os.makedirs(dirname,0755)
            except OSError:
                # There was an error on creation, so make sure we know about it
                raise            
    def cpu_freq(self):
        freq=0
        proc = subprocess.Popen(["cat","/proc/cpuinfo"],
                                stdout=subprocess.PIPE)
        out, err = proc.communicate()
        
        for line in out.split("\n"):
            if "cpu MHz" in line:
                freq = float(line.split(":")[1])
                break 
            
        return freq 
