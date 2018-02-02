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

# \author Rohit Gupta - Benoit ROBERT (benoit.robert@syrtem.com)
# \version 0.1
# @ingroup _test

# \Changelog
# 2016-11-18 : 
#   - Add progess bar during test execution update_progress()

import tempfile
import threading
import sys
import traceback
import wave
import os
import time
import datetime
import getpass
import math #from time import clock 
import xml.etree.ElementTree as ET
import re

#from dict2xml import dict2xml as xmlify

from colorama import Fore, Back, Style

import numpy as np

import log

from  openair import *

import paramiko

import subprocess
import commands
sys.path.append('/opt/ssh')
sys.path.append(os.path.expandvars('$OPENAIR_DIR/cmake_targets/autotests/tools/'))

from lib_autotest import *
import ssh
from ssh import SSHSession
import argparse


# update_progress() : Displays or updates a console progress bar
## Accepts a float between 0 and 1. Any int will be converted to a float.
## A value under 0 represents a 'halt'.
## A value at 1 or bigger represents 100%
def update_progress(progress, prefix_string):
    barLength = 20 # Modify this to change the length of the progress bar
    status = ""
    #print "progress = "+ str(progress)
    if isinstance(progress, int):
        progress = float(progress)
    if not isinstance(progress, float):
        progress = 0
        status = "error: progress var must be float\r\n"
    if progress < 0:
        progress = 0
        status = "Halt...\r\n"
    if progress >= 1:
        progress = 1
        status = "Done...\r\n"
    block = int(round(barLength*progress))
    text = "\r"+prefix_string+" [{0}] {1}% {2}".format( "="*block + " "*(barLength-block), progress*100, status)
    sys.stdout.write(text)
    sys.stdout.flush()


def exit_prog(exit_val):
  print(Style.RESET_ALL),
  sys.exit(exit_val)

# \brief write a string to a file
# \param filename name of file
# \param string string to write
# \mode file opening mode (default=write)
def write_file(filename, string, mode="w"):
   text_file = open(filename, mode)
   text_file.write(string)
   text_file.close()
 
# \brief function to check if test case passed throughput test
# \param filename name of file which has throughput results (usually from iperf -s ...
# \param min_tput minimum throughput
# \param max_tuput maximum throughput
# \param average average throughput
# \param min_duration minimum duration of throughput
#The throughput values found in file must be higher than values from from arguments 2,3,4,5
#The function returns True if throughput conditions are saisfied else it returns fails
def tput_test(filename, min_tput, max_tput, average, min_duration):
   
   if os.path.exists(filename):
      with open (filename, "r") as myfile:
         data=myfile.read()
      p=re.compile('(\d*.\d*) Mbits/sec')
      array=p.findall(data)
      array = [ float(x) for x in array ]
      duration = array.__len__()
      if duration !=0:
        min_list = min(array) 
        max_list = max(array)
        average_list = np.mean(array)
      else:
        min_list = 0
        max_list = 0
        average_list=0
      tput_string=' ( '+ "min=%0.2f"  % min_list + ' Mbps / ' + "max=%0.2f" %  max_list + ' Mbps / ' + "avg=%0.2f" % average_list + ' Mbps / ' + "dur=%0.2f" % duration + ' s) ' 
      if (min_list >= min_tput and  max_list >= max_tput and average_list >= average and duration >= min_duration):
        return True , tput_string
      else:
        return False , tput_string
   else: 
      return False , tput_string

# \brief Convert string to float or return None if there is exception    
def try_convert_to_float(string, fail=None):
    try:
        return float(string)
    except Exception:
        return fail;

# \brief get throughput statistics from log file
# \param search_expr search expression found in test_case_list.xml file
# \param logfile_traffic logfile which has traffic statistics
def tput_test_search_expr (search_expr, logfile_traffic):
   result=0
   tput_string=''
   if search_expr !='':
       if search_expr.find('throughput_test')!= -1 :
          p= re.compile('min\s*=\s*(\d*.\d*)\s*Mbits/sec')
          min_tput=p.findall(search_expr)
          if min_tput.__len__()==1:
             min_tput = min_tput[0]
          else:
             min_tput = None

          p= re.compile('max\s*=\s*(\d*.\d*)\s*Mbits/sec')
          max_tput=p.findall(search_expr)
          if max_tput.__len__()==1:
             max_tput = max_tput[0]
          else:
             max_tput = None

          p= re.compile('average\s*=\s*(\d*.\d*)\s*Mbits/sec')
          avg_tput=p.findall(search_expr)
          if avg_tput.__len__()==1:
             avg_tput=avg_tput[0]
          else:
             avg_tput = None

          p= re.compile('duration\s*=\s*(\d*.\d*)\s*s')
          duration=p.findall(search_expr)
          if duration.__len__()==1:
             duration = duration[0]
          else:
             duration = None
          
          min_tput = try_convert_to_float(min_tput)
          max_tput = try_convert_to_float(max_tput)
          avg_tput = try_convert_to_float(avg_tput)
          duration = try_convert_to_float(duration)
          
          if (min_tput != None and max_tput != None  and avg_tput != None  and duration != None ):
             result, tput_string = tput_test(logfile_traffic, min_tput, max_tput, avg_tput, duration)
   else: 
      result=1

   return result, tput_string
      
# \brief function to copy files to/from remote machine
# \param username user with which to make sftp connection
# \param password password of user
# \param hostname host to connect
# \ports port of remote machine on which server is listening
# \paramList This is list of operations as a set {operation: "get/put", localfile: "filename", remotefile: "filename"
# \param logfile Ignored currently and set once at the beginning of program
def sftp_module (username, password, hostname, ports, paramList,logfile): 
   #localD = localfile
   #remoteD = remotefile
   #fd, paramiko_logfile  = tempfile.mkstemp()
   #res = os.close(fd )
   #paramiko logfile path should not be changed with multiple calls. The logs seem to in first file regardless
   error = ""
   #The lines below are outside exception loop to be sure to terminate the test case if the network connectivity goes down or there is authentication failure
   transport = paramiko.Transport((hostname, ports))
   transport.connect(username = username, password = password)
   sftp = paramiko.SFTPClient.from_transport(transport)
   #  index =0 
   for param in paramList:
      try:
        operation = param["operation"] 
        localD = param["localfile"]
        remoteD = param["remotefile"]
        if operation == "put":
          sftp.put(remotepath=remoteD, localpath=localD)
        elif operation == "get":
          sftp.get(remotepath=remoteD, localpath=localD)
        else :
          print "sftp_module: unidentified operation:<" + operation + "> Exiting now"
          print "hostname = " + hostname
          print "ports = " + ports
          print "localfile = " + localD
          print "remotefile = " + remoteD
          print "operation = " + operation
          sys.exit()
      except Exception, e:
         error = error + ' In function: ' + sys._getframe().f_code.co_name + ': *** Caught exception: '  + str(e.__class__) + " : " + str( e)
         error = error + '\n username = ' + username + '\n hostname = ' + hostname + '\n localfile = ' + localD + '\n remotefile = ' + remoteD + '\n operation = ' + operation + '\nlogfile = ' + logfile + '\n ports = ' + str(ports) + '\n'  
         error = error + traceback.format_exc()
         print error

   sftp.close()
   transport.close() 
   res = os.system('\n echo \'SFTP Module Log for Machine: <' + hostname + '> starts...\' >> ' + logfile + ' 2>&1 ')
   res = os.system('cat ' + paramiko_logfile + ' >> ' + logfile + ' 2>&1 \n')
   write_file(logfile, error, "a")
   res = os.system('\n echo \'SFTP Module Log for Machine: <' + hostname + '> ends...\' >> ' + logfile + ' 2>&1 \n')

# \brief bash script stub put at the end of scripts to terminate it 
# \param timeout_cmd terminate script after timeout_cmd seconds
# \param terminate_missing_procs if True terminate all the processes launched by script if one of them terminates prematurely (due to error)
def finalize_deploy_script (timeout_cmd, terminate_missing_procs='False'):
  cmd = 'declare -i timeout_cmd='+str(timeout_cmd) + '\n'
  if terminate_missing_procs == 'True':
    cmd = cmd +  """
    #The code below checks if one the processes launched in background has crashed.
    #If it does, then the code below terminates all the child processes created by this script
    declare -i wakeup_interval=1
    declare -i step=0
    echo \"Array pid =  ${array_exec_pid[@]}\"
    while [ "$step" -lt "$timeout_cmd" ]
      do
       declare -i break_while_loop=0
       #Iterate over each process ID in array_exec_pid
       for i in "${array_exec_pid[@]}"
       do
        numchild=`pstree -p $i | perl -ne 's/\((\d+)\)/print " $1"/ge' |wc -w`
        echo "PID = $i, numchild = $numchild"
        if  [ "$numchild" -eq "0" ] ; then
            echo "Process ID $i has finished unexpectedly. Now preparing to kill all the processes "
            break_while_loop=1
            break
        fi
     done
    if  [ "$break_while_loop" -eq "1" ] ; then
             break
    fi
    step=$(( step + wakeup_interval ))
    sleep $wakeup_interval
    done
    echo "Final time step (Duration of test case) = $step "
    date
    """
  else:
    #We do not terminate the script if one of the processes has existed prematurely
    cmd = cmd + 'sleep ' + str(timeout_cmd) + ' ; date  \n'
  
  return cmd

# \brief run python script and update config file params of test case
# \param oai module with already open connection
# \param config_string config string taken from xml file
# \param logdirRepo directory of remote repository
# \param python_script python script location
def update_config_file(oai, config_string, logdirRepo, python_script):
  cmd=""
  if config_string :
    stringArray = config_string.splitlines()
    #python_script = '$OPENAIR_DIR/targets/autotests/tools/search_repl.py'
    for string in stringArray:
       #split the string based on space now
       string1=string.split()
       cmd = cmd + 'python ' + python_script + ' ' + logdirRepo+'/'+string1[0] + '  ' + string1[1] +  ' '+ string1[2] + '\n'
       #cmd = cmd + 'perl -p -i  -e \'s/'+ string1[1] + '\\s*=\\s*"\\S*"\\s*/' + string1[1] + ' = "' + string1[2] +'"' + '/g\'   ' + logdirRepo + '/' +string1[0] + '\n'
  return cmd
  #result = oai.send_recv(cmd)

# \brief thread safe sshsession wrapper due to occasional connection issues with ssh
# \param machine name of machine
# \param username user login for remote machine
# \param key_file file name which has keys to enable passwordless login
# \param password password for remote machine
# \param logdir_remote remote directory
# \param logdir_local_base local directory
# \param operation operation to perform (get_all, put_all) transfers recursively for directories
def SSHSessionWrapper(machine, username, key_file, password, logdir_remote, logdir_local_base, operation):
  max_tries = 10
  i=0
  while i <= max_tries:
    i = i +1
    try:
       ssh = SSHSession(machine , username, key_file, password)
       if operation == "get_all":
          ssh.get_all(logdir_remote , logdir_local_base)
       elif operation == "put_all":
          ssh.put_all(logdir_local_base, logdir_remote )
       else:
          print "Error: Uknown operation in SSHSessionWrapper. Exiting now..."
          sys.exit(1)
       break 
    except Exception, e:
       error=''
       error = error + ' In Class = function: ' + sys._getframe().f_code.co_name + ': *** Caught exception: '  + str(e.__class__) + " : " + str( e)
       error = error + '\n username = ' + username + '\n machine = ' + machine + '\n logdir_remote = ' + logdir_remote + '\n logdir_local_base = ' + logdir_local_base 
       error = error + traceback.format_exc()
       print error
       print " Retrying again in 1 seconds"
       time.sleep(1)
       print "Continuing ..."
       if i ==max_tries:
          print "Fatal Error: Max no of retries reached. Exiting now..."
          sys.exit(1)


 
# \briefFunction to clean old programs that might be running from earlier execution
# \param oai - parameter for making connection to machine
# \parm programList list of programs that must be terminated before execution of any test case 
# \param CleanUpAluLteBox program to terminate AlU Bell Labs LTE Box
# \param ExmimoRfStop String to stop EXMIMO card (specified in test_case_list.xml)
def cleanOldPrograms(oai, programList, CleanUpAluLteBox, ExmimoRfStop, logdir, logdirOAI5GRepo):
  cmd = 'killall -9 ' + programList
  result = oai.send(cmd, True)
#  print "\t\t > "+cmd
#  print "\t\t < "+result

#  print "Killing old programs on ..." + result
  programArray = programList.split()
  programListJoin = '|'.join(programArray)
  cmd = " ( date ;echo \"Starting cleaning old programs.. \" ; dmesg|tail ; echo \"Current disk space.. \" ; df -h )>& " + logdir + "/oai_test_setup_cleanup.log.`hostname` 2>&1 ; sync"
  result=oai.send_recv(cmd)
#  print "\t\t > "+cmd
#  print "\t\t < "+result

  cmd = cleanupOldProgramsScript + ' ' + '\''+programListJoin+'\''
  #result = oai.send_recv(cmd)
  #print result
  result = oai.send_expect_false(cmd, 'Match found', False)
#  print "\t\t > "+cmd
#  print "\t\t < "+result

#  print "Looking for old programs..." + result
  res=oai.send_recv(CleanUpAluLteBox, True)
#  print "\t\t > "+CleanUpAluLteBox
#  print "\t\t < "+res
  cmd= " echo \"Starting EXmimoRF Stop... \"  >> " + logdir + "/oai_test_setup_cleanup.log.`hostname` 2>&1  ; sync ";
  result = oai.send_recv(cmd)
#  print "\t\t > "+cmd
#  print "\t\t < "+result
  cmd  = "( " + "cd " + logdirOAI5GRepo + " ; source oaienv ;  "  +  ExmimoRfStop + " ) >> " + logdir + "/oai_test_setup_cleanup.log.`hostname` 2>&1  ; sync "
#  print "cleanoldprograms cmd = " + cmd
  res=oai.send_recv(cmd, False, timeout=600)
#  print "\t\t > "+cmd
#  print "\t\t < "+res

  cmd= " echo \"Stopping EXmimoRF Stop... \" >> " + logdir + "/oai_test_setup_cleanup.log.`hostname` 2>&1  ; sync ";
  result = oai.send_recv(cmd)
#  print "\t\t > "+cmd
#  print "\t\t < "+result

  #res = oai.send_recv(ExmimoRfStop, False)
  cmd = " ( date ;echo \"Finished cleaning old programs.. \" ; dmesg | tail)>> $HOME/.oai_test_setup_cleanup.log.`hostname` 2>&1 ; sync"
  res=oai.send_recv(cmd)
#  print "\t\t > "+cmd
#  print "\t\t < "+res

# \brief Class thread to launch a generic command on remote machine
# \param threadID number of thread (for book keeping)
# \param threadname string of threadname (for book keeping)
# \param machine machine name on which to run the command
# \param username username with which to login
# \param password password with which to login
# \param cmd command as a string to run on remote machine
# \parma sudo if True sudo is set
# \param timeout timeout of command in seconds 
class oaiThread (threading.Thread):
    def __init__(self, threadID, threadname, machine, username, password, cmd, sudo, timeout):
        threading.Thread.__init__(self)
        self.threadID = threadID
        self.threadname = threadname
        self.machine = machine
        self.username = username
        self.password = password
        self.cmd = cmd
        self.sudo = sudo
        self.timeout = timeout
    def run(self):
        try:
          oai = openair('localdomain',self.machine)
          oai.connect(self.username, self.password)
          # print "Starting " + self.threadname + " on machine " + self.machine
          result = oai.send_recv(self.cmd, self.sudo, self.timeout)
          #print "result = " + result
          #print "Exiting " + self.threadname
          oai.disconnect()
        except Exception, e:
           error=''
           error = error + ' In class oaiThread, function: ' + sys._getframe().f_code.co_name + ': *** Caught exception: '  + str(e.__class__) + " : " + str( e)
           error = error + '\n threadID = ' + str(self.threadID) + '\n threadname = ' + self.threadname + '\n timeout = ' + str(self.timeout) + '\n machine = ' + self.machine + '\n cmd = ' + self.cmd + '\n timeout = ' + str(self.timeout) +  '\n username = ' + self.username + '\n'  
           error = error + traceback.format_exc()
           print error


# \brief This class runs test cases with class {execution, compilatation}
# \param threadID number of thread (for book keeping)
# \param name string of threadname (for book keeping)
# \param machine machine name on which to run the command
# \param logdirOAI5GRepo directory on remote machine which as openairinterface5g repo installed
# \param testcasename name of test case to run on remote machine
# \param CleanupAluLteBox string that contains commands to stop ALU Bell Labs LTEBox (specified in test_case_list.xml)
# \param user username with which to login
# \param password password with which to login
# \param timeout timeout of command in seconds
# \param ExmimoRfStop command to stop EXMIMO Card
class testCaseThread_generic (threading.Thread):
   def __init__(self, threadID, name, machine, logdirOAI5GRepo, testcasename,oldprogramList, CleanupAluLteBox, user, password, timeout, ExmimoRfStop):
       threading.Thread.__init__(self)
       self.threadID = threadID
       self.name = name
       self.testcasename = testcasename
       self.timeout = timeout
       self.machine = machine
       self.logdirOAI5GRepo = logdirOAI5GRepo
       self.oldprogramList = oldprogramList
       self.CleanupAluLteBox = CleanupAluLteBox
       self.password=password
       self.ExmimoRfStop = ExmimoRfStop
       self.user = user
   def run(self):
     try:
       mypassword=''
       #addsudo = 'echo \'' + mypassword + '\' | sudo -S -E '
       addpass = 'echo \'' + mypassword + '\' | '
       #user = getpass.getuser()
       print "Starting test case : " + self.testcasename + " On machine " + self.machine + " timeout = " + str(self.timeout) 
       oai = openair('localdomain',self.machine)
       oai.connect(self.user, self.password)
       #cleanOldPrograms(oai, self.oldprogramList, self.CleanupAluLteBox, self.ExmimoRfStop)
       logdir_local = os.environ.get('OPENAIR_DIR')
       logdir_local_testcase = logdir_local +'/cmake_targets/autotests/log/'+ self.testcasename
       logdir_local_base = logdir_local +'/cmake_targets/autotests/log/'
       logdir_remote_testcase = self.logdirOAI5GRepo + '/cmake_targets/autotests/log/' + self.testcasename
       logdir_remote = self.logdirOAI5GRepo + '/cmake_targets/autotests/log/'
       logfile_task_testcasename = logdir_local_testcase + '/test_task' + '_' + self.testcasename + '_.log'
       logfile_task_testcasename_out = logdir_remote + '/test_task_out' + '_' + self.testcasename + '_.log'
       #print "logdir_local_testcase = " + logdir_local_testcase
       #print "logdir_remote_testcase = " + logdir_remote_testcase
       #if os.path.exists(logdir_local_testcase) == True :
       #    os.removedirs(logdir_local_testcase)
       #os.mkdir(logdir_local_testcase)
       os.system("rm -fr " + logdir_local_testcase )
       os.system("mkdir -p " +  logdir_local_testcase)
       cmd = "mkdir -p " + logdir_remote_testcase
       res = oai.send_recv(cmd, False, self.timeout) 
       #print "res = " + res

       cmd =  "( cd " +  self.logdirOAI5GRepo + " \n "
       cmd = cmd + "source oaienv \n"
       cmd = cmd + "$OPENAIR_DIR/cmake_targets/autotests/run_exec_autotests.bash --run-group \"" + self.testcasename + "\" -p \'\'"
       cmd = cmd + " ) >& "   + logfile_task_testcasename_out + " ; " + "mkdir -p " + logdir_remote_testcase +  "; mv " + logfile_task_testcasename_out + " " +logdir_remote_testcase 
      
       #print "cmd = " + cmd
       res = oai.send_recv(cmd, False, self.timeout) 
       #print "res = " + res
       #print "ThreadID = " + str(self.threadID) + "ThreadName: " + self.name + " testcasename: " + self.testcasename + "Execution Result = " + res
       write_file(logfile_task_testcasename, cmd, mode="w")
       #Now we copy all the remote files
       #ssh = SSHSession(self.machine , username=user, key_file=None, password=self.password)
       #ssh.get_all(logdir_remote_testcase , logdir_local_base)
       SSHSessionWrapper(self.machine, self.user, None, self.password, logdir_remote_testcase, logdir_local_base, "get_all")
       print "Finishing test case : " + self.testcasename + " On machine " + self.machine
       #cleanOldPrograms(oai, self.oldprogramList, self.CleanupAluLteBox, self.ExmimoRfStop)
       #oai.kill(user,mypassword)
       oai.disconnect()
     except Exception, e:
         error=''
         error = error + ' In Class = testCaseThread_generic,  function: ' + sys._getframe().f_code.co_name + ': *** Caught exception: '  + str(e.__class__) + " : " + str( e)
         error = error + '\n threadID = ' + str(self.threadID) + '\n threadName = ' + self.name + '\n testcasename = ' + self.testcasename + '\n machine = ' + self.machine + '\n logdirOAI5GRepo = ' + self.logdirOAI5GRepo +  '\n' + '\n timeout = ' + str(self.timeout)  + '\n user = ' + self.user
         error = error + traceback.format_exc()
         print error
         print "Continuing with next test case..."
         #sys.exit()


       
# \bried function to run a command as a sudo
# \param cmd command as a string
# \param password password to be supplied   
def addsudo (cmd, password=""):
  cmd = 'echo \'' + password + '\' | sudo -S -E bash -c \' ' + cmd + '\' '
  return cmd

# \brief handler for executing test cases (compilation, execution)
# \param name of testcase
# \param threadListGeneric list of threads which are already running on remote machines
# \param oldprogramList list of programs which must be terminated before running a test case
# \param logdirOAI5GRepo directory on remote machine which as openairinterface5g repo installed
# \param MachineList list of all machines on which generic test cases can be run
# \param user username with which to login
# \param password password with which to login
# \param CleanupAluLteBox string that contains commands to stop ALU Bell Labs LTEBox (specified in test_case_list.xml)
# \param timeout timeout of command in seconds
# \param ExmimoRfStop command to stop EXMIMO Card
def handle_testcaseclass_generic (testcasename, threadListGeneric, oldprogramList, logdirOAI5GRepo, MachineList, user, password, CleanupAluLteBox,timeout, ExmimoRfStop):
  try:
    mypassword=password
    MachineListFree=[]
    threadListNew=[]
    while MachineListFree.__len__() == 0 :
       MachineListBusy=[]
       MachineListFree=[]
       threadListNew=[]
       #first we need to find the list of free machines that we could run our test case
       if threadListGeneric.__len__() ==0 :
       #This means no thread is started yet
          MachineListFree = MachineList[:]
       else :
          for param in threadListGeneric :
             thread_id = param["thread_id"]
             machine = param["Machine"]
             testcasenameold = param["testcasename"]
             thread_id.join(1)
             if thread_id.isAlive() == True:
                threadListNew.append(param)
                print "thread_id is alive: testcasename: " + testcasenameold +  " on machine "+ machine
                if machine not in MachineListBusy:
                   MachineListBusy.append(machine)
             else :
                print "thread_id is finished: testcasename: " + testcasenameold + " on machine " + machine
                #threadListGeneric.remove(param)
                #if machine not in MachineListFree:
                #   MachineListFree.append(machine)
       #Now we check if there is at least one free machine
       MachineListFree = MachineList[:]
       for machine in MachineListBusy:
          if machine in MachineListFree:
            MachineListFree.remove(machine)
       print "MachineListFree = " + ','.join(MachineListFree)
       print "MachineListBusy = " + ','.join(MachineListBusy)
       print "MachineList = " + ','.join(MachineList)
    machine = MachineListFree[0]
    thread = testCaseThread_generic(1,"Generic Thread_"+testcasename+"_"+ "machine_", machine, logdirOAI5GRepo, testcasename, oldprogramList, CleanupAluLteBox, user, password, timeout, ExmimoRfStop)
    param={"thread_id":thread, "Machine":machine, "testcasename":testcasename}
    thread.start()
    threadListNew.append(param)
    return threadListNew
  except Exception, e:
     error=''
     error = error + ' In function: ' + sys._getframe().f_code.co_name + ': *** Caught exception: '  + str(e.__class__) + " : " + str( e)
     error = error + '\n testcasename = ' + testcasename + '\n logdirOAI5GRepo = ' + logdirOAI5GRepo + '\n MachineList = ' + ','.join(MachineList) + '\n timeout = ' + str(timeout) +  '\n' + 'user = ' + user
     error = error + traceback.format_exc()
     print error
     print "Continuing..."
     #sys.exit(1)

# \brief Blocking wait for all threads related to generic testcase execution, class (compilation and execution)
# \param threadListGeneric list of threads which are running on remote machines
# \param timeout time to wait on threads in seconds
def wait_testcaseclass_generic_threads(threadListGeneric, timeout = 1):
   threadListGenericNew=[]
   for param in threadListGeneric:
      thread_id = param["thread_id"]
      machine = param["Machine"]
      testcasenameold = param["testcasename"]
      thread_id.join(timeout)
      if thread_id.isAlive() == True:
         threadListGenericNew.append(param)
         print "thread_id on machine: " + machine + "  is still alive: testcasename: " + testcasenameold
         print " Exiting now..."
         sys.exit(1)
      else:
         print "thread_id on machine: " + machine + "  is stopped: testcasename: " + testcasenameold
         #threadListGeneric.remove(param)
   return threadListGenericNew


# \brief handler for executing test cases (lte-softmodem)
# \param testcase name of testcase
# \param oldprogramList list of programs which must be terminated before running a test case
# \param logdirOAI5GRepo directory on remote machine which has openairinterface5g repo installed
# \param logdirOpenaircnRepo directory on remote machine which has openair-cn repo installed
# \param MachineList list of all machines on which test cases can be run
# \param user username with which to login
# \param password password with which to login
# \param CleanupAluLteBox string that contains commands to stop ALU Bell Labs LTEBox (specified in test_case_list.xml)
# \param ExmimoRfStop command to stop EXMIMO Card
# \param nruns_lte-softmodem global parameter to override number of runs (nruns) within the test case
def handle_testcaseclass_softmodem (testcase, oldprogramList, logdirOAI5GRepo , logdirOpenaircnRepo, MachineList, user, password, CleanUpAluLteBox, ExmimoRfStop, nruns_lte_softmodem,  GitOAI5GRepoBranch,  GitOpenaircnRepoBranch,timeout_cmd):
  #We ignore the password sent to this function for secuirity reasons for password present in log files
  #It is recommended to add a line in /etc/sudoers that looks something like below. The line below will run sudo without password prompt
  # your_user_name ALL=(ALL:ALL) NOPASSWD: ALL
  mypassword=''
  #addsudo = 'echo \'' + mypassword + '\' | sudo -S -E '
  addpass = 'echo \'' + mypassword + '\' | '
  #user = getpass.getuser()
  testcasename = testcase.get('id')
  testcaseclass = testcase.findtext('class',default='')
  if timeout_cmd == '':
     timeout_cmd = testcase.findtext('TimeOut_cmd',default='')
  timeout_cmd = int(float(timeout_cmd))
  #Timeout_thread is more than that of cmd to have room for compilation time, etc
  timeout_thread = timeout_cmd + 300 
  if nruns_lte_softmodem == '':
    nruns = testcase.findtext('nruns',default='')
  else:
    nruns = nruns_lte_softmodem
  nruns = int(float(nruns))
  tags = testcase.findtext('tags',default='')

  RRHMachine = testcase.findtext('RRH',default='')
  RRH_config_file = testcase.findtext('RRH_config_file',default='')
  RRH_compile_prog = testcase.findtext('RRH_compile_prog',default='')
  RRH_compile_prog_args = testcase.findtext('RRH_compile_prog_args',default='')
  RRH_pre_exec = testcase.findtext('RRH_pre_exec',default='')
  RRH_pre_exec_args = testcase.findtext('RRH_pre_exec_args',default='')
  RRH_main_exec = testcase.findtext('RRH_main_exec',default='')
  RRH_main_exec_args = testcase.findtext('RRH_main_exec_args',default='')
  RRH_terminate_missing_procs = testcase.findtext('RRH_terminate_missing_procs',default='False')
  RRH_branch = testcase.findtext('RRH_branch',default=GitOAI5GRepoBranch)

  eNBMachine = testcase.findtext('eNB',default='')
  eNB_config_file = testcase.findtext('eNB_config_file',default='')
  eNB_compile_prog = testcase.findtext('eNB_compile_prog',default='')
  eNB_compile_prog_args = testcase.findtext('eNB_compile_prog_args',default='')
  eNB_pre_exec = testcase.findtext('eNB_pre_exec',default='')
  eNB_pre_exec_args = testcase.findtext('eNB_pre_exec_args',default='')
  eNB_main_exec = testcase.findtext('eNB_main_exec',default='')
  eNB_main_exec_args = testcase.findtext('eNB_main_exec_args',default='')
  eNB_traffic_exec = testcase.findtext('eNB_traffic_exec',default='')
  eNB_traffic_exec_args = testcase.findtext('eNB_traffic_exec_args',default='')
  eNB_terminate_missing_procs = testcase.findtext('eNB_terminate_missing_procs',default='False')
  eNB_search_expr_true = testcase.findtext('eNB_search_expr_true','')
  if re.compile('\w+').match(eNB_search_expr_true) != None:
      eNB_search_expr_true = eNB_search_expr_true + '  duration=' + str(timeout_cmd-90) + 's' 
  eNB_branch = testcase.findtext('eNB_branch',default=GitOAI5GRepoBranch)

  UEMachine = testcase.findtext('UE',default='')
  UE_config_file = testcase.findtext('UE_config_file',default='')
  UE_compile_prog = testcase.findtext('UE_compile_prog',default='')
  UE_compile_prog_args = testcase.findtext('UE_compile_prog_args',default='')
  UE_pre_exec = testcase.findtext('UE_pre_exec',default='')
  UE_pre_exec_args = testcase.findtext('UE_pre_exec_args',default='')
  UE_main_exec = testcase.findtext('UE_main_exec',default='')
  UE_main_exec_args = testcase.findtext('UE_main_exec_args',default='')
  UE_traffic_exec = testcase.findtext('UE_traffic_exec',default='')
  UE_traffic_exec_args = testcase.findtext('UE_traffic_exec_args',default='')
  UE_terminate_missing_procs = testcase.findtext('UE_terminate_missing_procs',default='False')
  UE_search_expr_true = testcase.findtext('UE_search_expr_true','')
  UE_stop_script =  testcase.findtext('UE_stop_script','')
  if re.compile('\w+').match(UE_search_expr_true) != None:
      UE_search_expr_true = UE_search_expr_true + '  duration=' + str(timeout_cmd-90) + 's'
  UE_branch = testcase.findtext('UE_branch',default=GitOAI5GRepoBranch)

  EPCMachine = testcase.findtext('EPC',default='')
  EPC_config_file = testcase.findtext('EPC_config_file',default='')
  EPC_compile_prog = testcase.findtext('EPC_compile_prog',default='')
  EPC_compile_prog_args = testcase.findtext('EPC_compile_prog_args',default='')
  HSS_compile_prog = testcase.findtext('HSS_compile_prog',default='')
  HSS_compile_prog_args = testcase.findtext('HSS_compile_prog_args',default='')
  
  EPC_pre_exec= testcase.findtext('EPC_pre_exec',default='')
  EPC_pre_exec_args = testcase.findtext('EPC_pre_exec_args',default='')  
  EPC_main_exec= testcase.findtext('EPC_main_exec',default='')
  EPC_main_exec_args = testcase.findtext('EPC_main_exec_args',default='')  
  HSS_main_exec= testcase.findtext('HSS_main_exec',default='')
  HSS_main_exec_args = testcase.findtext('HSS_main_exec_args',default='')  
  EPC_traffic_exec = testcase.findtext('EPC_traffic_exec',default='')
  EPC_traffic_exec_args = testcase.findtext('EPC_traffic_exec_args',default='')
  EPC_terminate_missing_procs = testcase.findtext('EPC_terminate_missing_procs',default='False')
  EPC_search_expr_true = testcase.findtext('EPC_search_expr_true','')
  EPC_branch = testcase.findtext('EPC_branch',default=GitOpenaircnRepoBranch)

  if re.compile('\w+').match(EPC_search_expr_true) != None:
     EPC_search_expr_true = EPC_search_expr_true + '  duration=' + str(timeout_cmd-90) + 's'

  index_eNBMachine = MachineList.index(eNBMachine)
  index_UEMachine = MachineList.index(UEMachine)
  index_EPCMachine = MachineList.index(EPCMachine)
  cmd = 'cd ' + logdirOAI5GRepo + '; source oaienv ; env|grep OPENAIR'
  oai_eNB = openair('localdomain', eNBMachine)
  oai_eNB.connect(user, password)
  res= oai_eNB.send_recv(cmd)
  oai_UE = openair('localdomain', UEMachine)
  oai_UE.connect(user, password)
  res = oai_eNB.send_recv(cmd)
  oai_EPC = openair('localdomain', EPCMachine)
  oai_EPC.connect(user, password)
  res = oai_eNB.send_recv(cmd)
  if RRHMachine != '':
    cmd = 'cd ' + logdirOAI5GRepo + '; source oaienv ; env|grep OPENAIR'
    index_RRHMachine = MachineList.index(RRHMachine)
    oai_RRH = openair('localdomain', RRHMachine)
    oai_RRH.connect(user, password)
    res= oai_RRH.send_recv(cmd)
  #cleanOldPrograms(oai_eNB, oldprogramList, CleanUpAluLteBox, ExmimoRfStop)
  #cleanOldPrograms(oai_UE, oldprogramList, CleanUpAluLteBox, ExmimoRfStop)
  #cleanOldPrograms(oai_EPC, oldprogramList, CleanUpAluLteBox, ExmimoRfStop)
  logdir_local = os.environ.get('OPENAIR_DIR')
  if logdir_local is None:
     print "Environment variable OPENAIR_DIR not set correctly"
     sys.exit()
   
  #Make the log directory of test case
  #cmd = 'mkdir -p ' + logdir_eNB
  #result = oai_eNB.send_recv(cmd)
  #cmd = 'mkdir -p ' +  logdir_UE
  #result = oai_UE.send_recv(cmd)
  #cmd = 'mkdir -p ' + logdir_EPC
  #result = oai_EPC.send_recv(cmd)
  
  print "Updating the config files for ENB/UE/EPC..."
  #updating the eNB/UE/EPC configuration file from the test case 
  #update_config_file(oai_eNB, eNB_config_file, logdirOAI5GRepo)
  #update_config_file(oai_UE, UE_config_file, logdirOAI5GRepo)
  #update_config_file(oai_EPC, EPC_config_file, logdirOpenaircnRepo)
  test_result=1
  test_result_string=''
  start_time=time.time()
  for run in range(0,nruns):
    run_result=1
    run_result_string=''
    logdir_eNB = logdirOAI5GRepo+'/cmake_targets/autotests/log/'+ testcasename + '/run_' + str(run)
    logdir_RRH = logdirOAI5GRepo+'/cmake_targets/autotests/log/'+ testcasename + '/run_' + str(run)
    logdir_UE =  logdirOAI5GRepo+'/cmake_targets/autotests/log/'+ testcasename + '/run_' + str(run)
    logdir_EPC = logdirOpenaircnRepo+'/TEST/autotests/log/'+ testcasename + '/run_' + str(run)
    logdir_local_testcase = logdir_local + '/cmake_targets/autotests/log/'+ testcasename + '/run_' + str(run)
    #Make the log directory of test case
    if RRHMachine != '':
      cmd = 'rm -fr ' + logdir_RRH + ' ; mkdir -p ' + logdir_RRH 
      result = oai_RRH.send_recv(cmd)
    cmd = 'rm -fr ' + logdir_eNB + ' ; mkdir -p ' + logdir_eNB
    result = oai_eNB.send_recv(cmd)
    cmd = 'rm -fr ' + logdir_UE + ' ; mkdir -p ' +  logdir_UE
    result = oai_UE.send_recv(cmd)
    cmd = 'rm -fr ' + logdir_EPC + '; mkdir -p ' + logdir_EPC
    result = oai_EPC.send_recv(cmd)
    cmd = ' rm -fr ' + logdir_local_testcase + ' ; mkdir -p ' + logdir_local_testcase
    result = os.system(cmd)

    if RRHMachine != '':
       logfile_compile_RRH = logdir_RRH + '/RRH_compile' + '_' + str(run) + '_.log'
       logfile_exec_RRH = logdir_RRH + '/RRH_exec' + '_' + str(run) + '_.log'
       logfile_pre_exec_RRH = logdir_RRH + '/RRH_pre_exec' + '_' + str(run) + '_.log'
       logfile_task_RRH_compile_out = logdir_RRH + '/RRH_task_compile_out' + '_' + str(run) + '_.log'
       logfile_task_RRH_compile = logdir_local_testcase + '/RRH_task_compile' + '_' + str(run) + '_.log'
       logfile_task_RRH_out = logdir_RRH + '/RRH_task_out' + '_' + str(run) + '_.log'
       logfile_task_RRH = logdir_local_testcase + '/RRH_task' + '_' + str(run) + '_.log'
       task_RRH_compile = ' ( uname -a ; date \n'
       task_RRH_compile = task_RRH_compile + 'cd ' + logdirOAI5GRepo + '; git reset --hard HEAD ; git checkout ' + RRH_branch + ' ; source oaienv   \n' 
       task_RRH_compile = task_RRH_compile + ' source cmake_targets/tools/build_helper \n'
       task_RRH_compile = task_RRH_compile + 'env |grep OPENAIR  \n'
       task_RRH_compile = task_RRH_compile + update_config_file(oai_RRH, RRH_config_file, logdirOAI5GRepo, '$OPENAIR_DIR/cmake_targets/autotests/tools/search_repl.py') + '\n'
       if RRH_compile_prog != "":
         task_RRH_compile  = task_RRH_compile +  ' ( ' + RRH_compile_prog + ' '+ RRH_compile_prog_args + ' ) > ' + logfile_compile_RRH + ' 2>&1 \n'
       task_RRH_compile =  task_RRH_compile + ' date ) > ' + logfile_task_RRH_compile_out + ' 2>&1  '
       write_file(logfile_task_RRH_compile, task_RRH_compile, mode="w")

       task_RRH = ' ( uname -a ; date \n'
       task_RRH = task_RRH + ' export OPENAIR_TESTDIR=' + logdir_RRH + '\n'
       task_RRH = task_RRH + 'cd ' + logdirOAI5GRepo + ' ; source oaienv ; source cmake_targets/tools/build_helper \n'
       task_RRH = task_RRH + 'env |grep OPENAIR  \n' + 'array_exec_pid=() \n'

       if RRH_pre_exec != "":
          task_RRH  = task_RRH +  ' ( date; ' + RRH_pre_exec + ' '+ RRH_pre_exec_args + ' ) > ' + logfile_pre_exec_RRH + ' 2>&1 \n'
       if RRH_main_exec != "":
          task_RRH = task_RRH + ' ( date; ' + addsudo(RRH_main_exec + ' ' + RRH_main_exec_args, mypassword) + ' ) > ' + logfile_exec_RRH + ' 2>&1 & \n'
          task_RRH = task_RRH + 'array_exec_pid+=($!) \n'
          task_RRH = task_RRH + 'echo eNB_main_exec PID = $! \n'
       #terminate the eNB test case after timeout_cmd seconds
       task_RRH  = task_RRH + finalize_deploy_script (timeout_cmd, RRH_terminate_missing_procs) + ' \n'
       task_RRH  = task_RRH + 'handle_ctrl_c' + '\n' 
       task_RRH  = task_RRH + ' ) > ' + logfile_task_RRH_out + ' 2>&1  '
       write_file(logfile_task_RRH, task_RRH, mode="w")
    
    logfile_compile_eNB = logdir_eNB + '/eNB_compile' + '_' + str(run) + '_.log'
    logfile_exec_eNB = logdir_eNB + '/eNB_exec' + '_' + str(run) + '_.log'
    logfile_pre_exec_eNB = logdir_eNB + '/eNB_pre_exec' + '_' + str(run) + '_.log'
    logfile_traffic_eNB = logdir_eNB + '/eNB_traffic' + '_' + str(run) + '_.log'
    logfile_task_eNB_compile_out = logdir_eNB + '/eNB_task_compile_out' + '_' + str(run) + '_.log'
    logfile_task_eNB_compile = logdir_local_testcase + '/eNB_task_compile' + '_' + str(run) + '_.log'
    logfile_task_eNB_out = logdir_eNB + '/eNB_task_out' + '_' + str(run) + '_.log'
    logfile_task_eNB = logdir_local_testcase + '/eNB_task' + '_' + str(run) + '_.log'
    logfile_local_traffic_eNB_out = logdir_local_testcase + '/eNB_traffic' + '_' + str(run) + '_.log' 
    logfile_tshark_eNB = logdir_eNB + '/eNB_tshark' + '_' + str(run) + '_.log'
    logfile_pcap_eNB = logdir_eNB + '/eNB_tshark' + '_' + str(run) + '_.pcap'
    logfile_pcap_zip_eNB = logdir_eNB + '/eNB_tshark' + '_' + str(run) + '_.pcap.zip'
    logfile_pcap_tmp_eNB = '/tmp/' + '/eNB_tshark' + '_' + str(run) + '_.pcap'

    task_eNB_compile = ' ( uname -a ; date \n'
    task_eNB_compile = task_eNB_compile + 'cd ' + logdirOAI5GRepo + '; git reset --hard HEAD ; git checkout ' + eNB_branch + ' ; source oaienv   \n' 
    task_eNB_compile = task_eNB_compile + ' source cmake_targets/tools/build_helper \n'
    task_eNB_compile = task_eNB_compile + 'env |grep OPENAIR  \n'
    task_eNB_compile = task_eNB_compile + update_config_file(oai_eNB, eNB_config_file, logdirOAI5GRepo, '$OPENAIR_DIR/cmake_targets/autotests/tools/search_repl.py') + '\n'
    if eNB_compile_prog != "":
       task_eNB_compile  = task_eNB_compile +  ' ( ' + eNB_compile_prog + ' '+ eNB_compile_prog_args + ' ) > ' + logfile_compile_eNB + ' 2>&1 \n'
    task_eNB_compile =  task_eNB_compile + ' date ) > ' + logfile_task_eNB_compile_out + ' 2>&1  '
    write_file(logfile_task_eNB_compile, task_eNB_compile, mode="w")

    task_eNB = ' ( uname -a ; date \n'
    task_eNB = task_eNB + ' export OPENAIR_TESTDIR=' + logdir_eNB + '\n'
    task_eNB = task_eNB + 'cd ' + logdirOAI5GRepo + ' ; source oaienv ; source cmake_targets/tools/build_helper \n'
    task_eNB = task_eNB + 'env |grep OPENAIR  \n' + 'array_exec_pid=() \n'

    if eNB_pre_exec != "":
       task_eNB  = task_eNB +  ' ( date; ' + eNB_pre_exec + ' '+ eNB_pre_exec_args + ' ) > ' + logfile_pre_exec_eNB + ' 2>&1 \n'
    if eNB_main_exec != "":
       task_eNB = task_eNB + ' ( date; ' + addsudo(eNB_main_exec + ' ' + eNB_main_exec_args, mypassword) + ' ) > ' + logfile_exec_eNB + ' 2>&1 & \n'
       task_eNB = task_eNB + 'array_exec_pid+=($!) \n'
       task_eNB = task_eNB + 'echo eNB_main_exec PID = $! \n'
    if eNB_traffic_exec != "":
       cmd_traffic = eNB_traffic_exec + ' ' + eNB_traffic_exec_args
       if cmd_traffic.find('-c') >= 0:
          cmd_traffic = cmd_traffic + ' -t ' + str(timeout_cmd - 60)
       task_eNB = task_eNB + ' (date;  ' + cmd_traffic + ' ) > ' + logfile_traffic_eNB + ' 2>&1 & \n'
       task_eNB = task_eNB + 'array_exec_pid+=($!) \n'
       task_eNB = task_eNB + 'echo eNB_traffic_exec PID = $! \n'

    task_eNB = task_eNB + ' (date; sudo rm -f ' + logfile_pcap_tmp_eNB + ' ; sudo -E tshark -i lo -s 65535 -a duration:' + str(timeout_cmd-10)+ ' -w ' + logfile_pcap_tmp_eNB+ ' ; sudo -E chown ' + user + ' ' + logfile_pcap_tmp_eNB + ' ; zip -j -9  ' + logfile_pcap_zip_eNB + ' ' + logfile_pcap_tmp_eNB + '   ) > ' + logfile_tshark_eNB + ' 2>&1 & \n '
    task_eNB = task_eNB + 'array_exec_pid+=($!) \n'
    task_eNB = task_eNB + 'echo eNB_tshark_exec PID = $! \n'
    #terminate the eNB test case after timeout_cmd seconds
    task_eNB  = task_eNB + finalize_deploy_script (timeout_cmd, eNB_terminate_missing_procs) + ' \n'
    #task_eNB  = task_eNB + 'sleep ' +  str(timeout_cmd) + ' \n'
    task_eNB  = task_eNB + 'handle_ctrl_c' + '\n' 
    task_eNB  = task_eNB + ' ) > ' + logfile_task_eNB_out + ' 2>&1  '
    write_file(logfile_task_eNB, task_eNB, mode="w")

    #task_eNB =  'echo \" ' + task_eNB + '\" > ' + logfile_script_eNB + ' 2>&1 ; ' + task_eNB 
    logfile_compile_UE = logdir_UE + '/UE_compile' + '_' + str(run) + '_.log'
    logfile_exec_UE = logdir_UE + '/UE_exec' + '_' + str(run) + '_.log'
    logfile_pre_exec_UE = logdir_UE + '/UE_pre_exec' + '_' + str(run) + '_.log'
    logfile_traffic_UE = logdir_UE + '/UE_traffic' + '_' + str(run) + '_.log'    
    logfile_task_UE_out = logdir_UE + '/UE_task_out' + '_' + str(run) + '_.log'
    logfile_task_UE = logdir_local_testcase + '/UE_task' + '_' + str(run) + '_.log'
    logfile_task_UE_compile_out = logdir_UE + '/UE_task_compile_out' + '_' + str(run) + '_.log'
    logfile_task_UE_compile = logdir_local_testcase + '/UE_task_compile' + '_' + str(run) + '_.log'
    logfile_local_traffic_UE_out = logdir_local_testcase + '/UE_traffic' + '_' + str(run) + '_.log' 

    task_UE_compile = ' ( uname -a ; date \n'
    task_UE_compile = task_UE_compile + 'array_exec_pid=()' + '\n'
    task_UE_compile = task_UE_compile + 'cd ' + logdirOAI5GRepo + '; git reset --hard HEAD ; git checkout ' + UE_branch + ' ; source oaienv   \n' 
    task_UE_compile = task_UE_compile + 'source cmake_targets/tools/build_helper \n'
    task_UE_compile = task_UE_compile + 'env |grep OPENAIR  \n'
    task_UE_compile = task_UE_compile + update_config_file(oai_UE, UE_config_file, logdirOAI5GRepo, '$OPENAIR_DIR/cmake_targets/autotests/tools/search_repl.py') + '\n'
    if UE_compile_prog != "":
       task_UE_compile = task_UE_compile + ' ( ' + UE_compile_prog + ' '+ UE_compile_prog_args + ' ) > ' + logfile_compile_UE + ' 2>&1 \n'
    task_UE_compile  = task_UE_compile + ' ) > ' + logfile_task_UE_compile_out + ' 2>&1 '
    write_file(logfile_task_UE_compile, task_UE_compile, mode="w")

    task_UE = ' ( uname -a ; date \n'
    task_UE = task_UE + 'array_exec_pid=()' + '\n'
    task_UE = task_UE + 'cd ' + logdirOAI5GRepo + '\n'  
    task_UE = task_UE + 'source oaienv \n'
    task_UE = task_UE + 'source cmake_targets/tools/build_helper \n'
    task_UE = task_UE + 'env |grep OPENAIR  \n'
    if UE_pre_exec != "":
       task_UE  = task_UE +  ' ( date; ' + UE_pre_exec + ' '+ UE_pre_exec_args + ' ) > ' + logfile_pre_exec_UE + ' 2>&1 \n'
    if UE_main_exec != "":
       task_UE = task_UE + ' ( date;  ' + addsudo(UE_main_exec + ' ' + UE_main_exec_args, mypassword)  + ' ) > ' + logfile_exec_UE + ' 2>&1 & \n'
       task_UE = task_UE + 'array_exec_pid+=($!) \n'
       task_UE = task_UE + 'echo UE_main_exec PID = $! \n'
    if UE_traffic_exec != "":
       cmd_traffic = UE_traffic_exec + ' ' + UE_traffic_exec_args
       if cmd_traffic.find('-c') >= 0:
          cmd_traffic = cmd_traffic + ' -t ' + str(timeout_cmd - 60)
       task_UE = task_UE + ' ( date;  ' + cmd_traffic + ' ) >' + logfile_traffic_UE + ' 2>&1 & \n'
       task_UE = task_UE + 'array_exec_pid+=($!) \n'
       task_UE = task_UE + 'echo UE_traffic_exec PID = $! \n'
    #terminate the UE test case after timeout_cmd seconds
    task_UE  = task_UE + finalize_deploy_script (timeout_cmd, UE_terminate_missing_procs) + ' \n'
    #task_UE  = task_UE + 'sleep ' +  str(timeout_cmd) + ' \n'
    task_UE  = task_UE + 'handle_ctrl_c' + '\n' 
    task_UE  = task_UE + ' ) > ' + logfile_task_UE_out + ' 2>&1 '
    write_file(logfile_task_UE, task_UE, mode="w")
    #task_UE = 'echo \" ' + task_UE + '\" > ' + logfile_script_UE + ' 2>&1 ; ' + task_UE

    logfile_compile_EPC = logdir_EPC + '/EPC_compile' + '_' + str(run) + '_.log'
    logfile_compile_HSS = logdir_EPC + '/HSS_compile' + '_' + str(run) + '_.log'
    logfile_exec_EPC = logdir_EPC + '/EPC_exec' + '_' + str(run) + '_.log'
    logfile_pre_exec_EPC = logdir_EPC + '/EPC_pre_exec' + '_' + str(run) + '_.log'
    logfile_exec_HSS = logdir_EPC + '/HSS_exec' + '_' + str(run) + '_.log'
    logfile_traffic_EPC = logdir_EPC + '/EPC_traffic' + '_' + str(run) + '_.log'
    logfile_task_EPC_out = logdir_EPC + '/EPC_task_out' + '_' + str(run) + '_.log'
    logfile_task_EPC = logdir_local_testcase + '/EPC_task' + '_' + str(run) + '_.log'
    logfile_task_EPC_compile_out = logdir_EPC + '/EPC_task_compile_out' + '_' + str(run) + '_.log'
    logfile_task_EPC_compile = logdir_local_testcase + '/EPC_task_compile' + '_' + str(run) + '_.log'
    logfile_local_traffic_EPC_out = logdir_local_testcase + '/EPC_traffic' + '_' + str(run) + '_.log' 

    task_EPC_compile = ' ( uname -a ; date \n'
    task_EPC_compile = task_EPC_compile + 'array_exec_pid=()' + '\n'
    task_EPC_compile = task_EPC_compile + 'cd ' + logdirOpenaircnRepo + '; git reset --hard HEAD ; git checkout ' + EPC_branch + ' ; source oaienv   \n' 
    task_EPC_compile = task_EPC_compile + update_config_file(oai_EPC, EPC_config_file, logdirOpenaircnRepo, logdirOpenaircnRepo+'/TEST/autotests/tools/search_repl.py') + '\n'
    task_EPC_compile = task_EPC_compile +  'source BUILD/TOOLS/build_helper \n'
    if EPC_compile_prog != "":
       task_EPC_compile = task_EPC_compile + '(' + EPC_compile_prog + ' ' + EPC_compile_prog_args +  ' ) > ' + logfile_compile_EPC + ' 2>&1 \n'
    if HSS_compile_prog != "":
       task_EPC_compile = task_EPC_compile + '(' + HSS_compile_prog + ' ' + HSS_compile_prog_args + ' ) > ' + logfile_compile_HSS + ' 2>&1 \n'
    task_EPC_compile  = task_EPC_compile + ' ) > ' + logfile_task_EPC_compile_out + ' 2>&1 ' 
    write_file(logfile_task_EPC_compile, task_EPC_compile, mode="w")
    
    task_EPC = ' ( uname -a ; date \n'
    task_EPC = task_EPC + ' export OPENAIRCN_TESTDIR=' + logdir_EPC + '\n'
    task_EPC = task_EPC + 'array_exec_pid=()' + '\n'
    task_EPC = task_EPC + 'cd ' + logdirOpenaircnRepo + '; source oaienv\n'
    task_EPC = task_EPC +  'source BUILD/TOOLS/build_helper \n'
    if EPC_pre_exec != "":
       task_EPC  = task_EPC +  ' ( date; ' + EPC_pre_exec + ' '+ EPC_pre_exec_args + ' ) > ' + logfile_pre_exec_EPC + ' 2>&1 \n'
    if HSS_main_exec !=  "":
       task_EPC  = task_EPC + '( date; ' + addsudo (HSS_main_exec + ' ' + HSS_main_exec_args, mypassword) + ' ) > ' + logfile_exec_HSS  +  ' 2>&1   & \n'
       task_EPC = task_EPC + 'array_exec_pid+=($!) \n'
       task_EPC = task_EPC + 'echo HSS_main_exec PID = $! \n'
    if EPC_main_exec !=  "":
       task_EPC  = task_EPC + '( date; ' + addsudo (EPC_main_exec + ' ' + EPC_main_exec_args, mypassword) + ' ) > ' + logfile_exec_EPC  +  ' 2>&1   & \n'
       task_EPC = task_EPC + 'array_exec_pid+=($!) \n'
       task_EPC = task_EPC + 'echo EPC_main_exec PID = $! \n'
    if EPC_traffic_exec !=  "":
       cmd_traffic = EPC_traffic_exec + ' ' + EPC_traffic_exec_args
       if cmd_traffic.find('-c') >= 0:
          cmd_traffic = cmd_traffic + ' -t ' + str(timeout_cmd - 60)
       task_EPC  = task_EPC + '( date; ' + cmd_traffic + ' ) > ' + logfile_traffic_EPC  +  ' 2>&1   & \n' 
       task_EPC = task_EPC + 'array_exec_pid+=($!) \n'  
       task_EPC = task_EPC + 'echo EPC_traffic_exec PID = $! \n'
    #terminate the EPC test case after timeout_cmd seconds   
    task_EPC = task_EPC + finalize_deploy_script (timeout_cmd, EPC_terminate_missing_procs) + '\n'
    #task_EPC  = task_EPC + 'sleep ' +  str(timeout_cmd) + '\n'
    task_EPC  = task_EPC + 'handle_ctrl_c' '\n' 
    task_EPC  = task_EPC + ' ) > ' + logfile_task_EPC_out + ' 2>&1 ' 
    write_file(logfile_task_EPC, task_EPC, mode="w")
    
    #first we compile all the programs but only for run_0
    if run == 0:
       thread_EPC = oaiThread(1, "EPC_thread", EPCMachine, user, password , task_EPC_compile, False, timeout_thread)
       thread_eNB = oaiThread(2, "eNB_thread", eNBMachine, user, password , task_eNB_compile, False, timeout_thread)
       thread_UE = oaiThread(3, "UE_thread", UEMachine, user, password  , task_UE_compile, False, timeout_thread) 
       if RRHMachine != '':
          thread_RRH = oaiThread(4, "RRH_thread", RRHMachine, user, password  , task_RRH_compile, False, timeout_thread) 
       threads=[]
       threads.append(thread_eNB)
       threads.append(thread_UE)
       threads.append(thread_EPC)
       if RRHMachine != '':
         threads.append(thread_RRH)
       # Start new Threads
       thread_eNB.start()
       thread_UE.start()
       thread_EPC.start()
       if RRHMachine != '':
         thread_RRH.start()
       #Wait for all the compile threads to complete
       for t in threads:
         t.join()

    #Now we execute all the threads
    thread_EPC = oaiThread(1, "EPC_thread", EPCMachine, user, password , task_EPC, False, timeout_thread)
    thread_eNB = oaiThread(2, "eNB_thread", eNBMachine, user, password , task_eNB, False, timeout_thread)
    thread_UE = oaiThread(3, "UE_thread", UEMachine, user, password  , task_UE, False, timeout_thread) 
    if RRHMachine != '':
        thread_RRH = oaiThread(4, "RRH_thread", RRHMachine, user, password  , task_RRH, False, timeout_thread) 
    threads=[]
    threads.append(thread_eNB)
    threads.append(thread_UE)
    threads.append(thread_EPC)
    if RRHMachine != '':
        threads.append(thread_RRH)
    # Start new Threads

    thread_eNB.start()
    thread_UE.start()
    thread_EPC.start()
    if RRHMachine != '':
        thread_RRH.start()
    #Wait for all the compile threads to complete
    for t in threads:
       t.join()
    #Now we get the log files from remote machines on the local machine
    if RRHMachine != '':
       cleanOldProgramsAllMachines([oai_eNB, oai_UE, oai_EPC, oai_RRH] , oldprogramList, CleanUpAluLteBox, ExmimoRfStop, [logdir_eNB, logdir_UE, logdir_EPC, logdir_RRH], logdirOAI5GRepo)
    else:
       cleanOldProgramsAllMachines([oai_eNB, oai_UE, oai_EPC] , oldprogramList, CleanUpAluLteBox, ExmimoRfStop, [logdir_eNB, logdir_UE, logdir_EPC], logdirOAI5GRepo)       
    logfile_UE_stop_script_out = logdir_UE + '/UE_stop_script_out' + '_' + str(run) + '_.log'
    logfile_UE_stop_script = logdir_local_testcase + '/UE_stop_script' + '_' + str(run) + '_.log'

    if UE_stop_script != "":
      cmd = ' ( uname -a ; date \n'
      cmd = cmd + 'cd ' + logdirOAI5GRepo + ' ; source oaienv ; source cmake_targets/tools/build_helper \n'
      cmd = cmd + 'env |grep OPENAIR  \n' + 'array_exec_pid=() \n'
      cmd = cmd + UE_stop_script + '\n'
      cmd = cmd + ') > ' + logfile_UE_stop_script_out + ' 2>&1 ' 
      write_file(logfile_UE_stop_script , cmd, mode="w")
      thread_UE = oaiThread(4, "UE_thread", UEMachine, user, password  , cmd, False, timeout_thread)
      thread_UE.start()
      thread_UE.join()
   
    #Now we change the permissions of the logfiles to avoid some of them being with root permissions
    cmd = 'sudo -E chown -R ' + user + ' ' + logdir_eNB
    res= oai_eNB.send_recv(cmd)
    print "Changing permissions of logdir <" + logdir_eNB + "> in eNB machine..." + res

    cmd = 'sudo -E chown -R ' + user + ' ' +  logdir_UE
    res= oai_UE.send_recv(cmd)
    print "Changing permissions of logdir <" + logdir_UE + "> in UE machine..." + res

    cmd = 'sudo -E chown -R ' + user + ' ' +  logdir_EPC
    res= oai_EPC.send_recv(cmd)
    print "Changing permissions of logdir <" + logdir_EPC + "> in EPC machine..." + res

    if RRHMachine != '':
       cmd = 'sudo -E chown -R ' + user + ' ' +  logdir_RRH
       res= oai_RRH.send_recv(cmd)
       print "Changing permissions of logdir <" + logdir_RRH + "> in RRH machine..." + res

    print "Copying files from EPCMachine : " + EPCMachine + "logdir_EPC = " + logdir_EPC
    SSHSessionWrapper(EPCMachine, user, None, password, logdir_EPC, logdir_local + '/cmake_targets/autotests/log/'+ testcasename, "get_all")

    print "Copying files from eNBMachine " + eNBMachine + "logdir_eNB = " + logdir_eNB
    SSHSessionWrapper(eNBMachine, user, None, password, logdir_eNB, logdir_local + '/cmake_targets/autotests/log/'+ testcasename, "get_all")

    print "Copying files from UEMachine : " + UEMachine + "logdir_UE = " + logdir_UE
    SSHSessionWrapper(UEMachine, user, None, password, logdir_UE, logdir_local + '/cmake_targets/autotests/log/'+ testcasename, "get_all")

    if RRHMachine != '':
       print "Copying files from RRHMachine : " + RRHMachine + "logdir_RRH = " + logdir_RRH
       SSHSessionWrapper(RRHMachine, user, None, password, logdir_RRH, logdir_local + '/cmake_targets/autotests/log/'+ testcasename, "get_all")

    
    #Currently we only perform throughput tests
    tput_run_string=''
    result, tput_string = tput_test_search_expr(eNB_search_expr_true, logfile_local_traffic_eNB_out)
    tput_run_string = tput_run_string + tput_string
    run_result=run_result&result
    result, tput_string = tput_test_search_expr(EPC_search_expr_true, logfile_local_traffic_EPC_out)
    run_result=run_result&result
    tput_run_string = tput_run_string + tput_string
    result, tput_string = tput_test_search_expr(UE_search_expr_true, logfile_local_traffic_UE_out)
    run_result=run_result&result
    tput_run_string = tput_run_string + tput_string
    
    if run_result == 1:  
      run_result_string = ' RUN_'+str(run) + ' = PASS'
    else:
      run_result_string = ' RUN_'+str(run) + ' = FAIL'

    #If there is assertion, we mark the test case as failure as most likely eNB crashed
    cmd = "grep -ilr \"assertion\" " + logdir_local_testcase + " | cat " 
    cmd_out = subprocess.check_output ([cmd], shell=True)
    if len(cmd_out) !=0 :
      run_result=0
      run_result_string = ' RUN_'+str(run) + ' = FAIL(Assert)'

    #If there is thread busy error, we mark the test case as failure as most likely eNB crashed
    cmd = "grep -ilr \"thread busy\" " + logdir_local_testcase + " | cat "
    cmd_out = subprocess.check_output ([cmd], shell=True)
    if len(cmd_out) !=0:
      run_result=0
      run_result_string = ' RUN_'+str(run) + ' = FAIL(Thread_Busy)'

    #If there is Segmentation fault, we mark the test case as failure as most likely eNB crashed
    cmd = "grep -ilr \"segmentation fault\" " + logdir_local_testcase + " | cat "
    cmd_out = subprocess.check_output ([cmd], shell=True)
    if len(cmd_out) !=0:
      run_result=0
      run_result_string = ' RUN_'+str(run) + ' = FAIL(SEGFAULT)'

    run_result_string = run_result_string + tput_run_string

    test_result=test_result & run_result
    test_result_string=test_result_string + run_result_string

    oai_eNB.disconnect()
    oai_UE.disconnect()
    oai_EPC.disconnect()
    #We need to close the new ssh session that was created  
    #if index_eNBMachine == index_EPCMachine:
    #    oai_EPC.disconnect()
  #Now we finalize the xml file of the test case
  end_time=time.time()
  duration= end_time - start_time
  xmlFile = logdir_local + '/cmake_targets/autotests/log/'+ testcasename + '/test.' + testcasename + '.xml'
  if test_result ==0: 
    result='FAIL'
  else:
    result = 'PASS'
  xml="\n<testcase classname=\'"+ testcaseclass +  "\' name=\'" + testcasename + "."+tags +  "\' Run_result=\'" + test_result_string + "\' time=\'" + str(duration) + " s \' RESULT=\'" + result + "\'></testcase> \n"
  write_file(xmlFile, xml, mode="w")




# \brief handler for executing test cases (lte-softmodem-noS1)
# \param testcase name of testcase
# \param oldprogramList list of programs which must be terminated before running a test case
# \param logdirOAI5GRepo directory on remote machine which has openairinterface5g repo installed
# \param logdirOpenaircnRepo directory on remote machine which has openair-cn repo installed
# \param MachineList list of all machines on which test cases can be run
# \param user username with which to login
# \param password password with which to login
# \param CleanupAluLteBox string that contains commands to stop ALU Bell Labs LTEBox (specified in test_case_list.xml)
# \param ExmimoRfStop command to stop EXMIMO Card
# \param nruns_lte-softmodem global parameter to override number of runs (nruns) within the test case
def handle_testcaseclass_softmodem_noS1 (testcase, oldprogramList, logdirOAI5GRepo , logdirOpenaircnRepo, MachineList, user, password, CleanUpAluLteBox, ExmimoRfStop, nruns_lte_softmodem, timeout_cmd):
  #We ignore the password sent to this function for secuirity reasons for password present in log files
  #It is recommended to add a line in /etc/sudoers that looks something like below. The line below will run sudo without password prompt
  # your_user_name ALL=(ALL:ALL) NOPASSWD: ALL

  indent="\t"
  threads=[]

  #
  # Test case parameters
  # -----------------------------------------------------------------------------
  testcase_verdict = 'PASS'
  testcase_time_start = datetime.datetime.now()
  testcase_name   = testcase.get('id')
  testcase_class  = testcase.findtext('class',default='')
  testcase_desc   = testcase.findtext('desc',default='')
  if timeout_cmd == '':
     timeout_cmd = testcase.findtext('TimeOut_cmd',default='')
  timeout_cmd = int(float(timeout_cmd))
  timeout_thread = timeout_cmd + 60    #Timeout_thread is more than that of cmd to have room for compilation time, etc
  if nruns_lte_softmodem == '':
    nruns = testcase.findtext('nruns',default='')
  else:
    nruns = nruns_lte_softmodem
  nruns = int(float(nruns))
  tags = testcase.findtext('tags',default='')

  max_ntries = testcase.findtext('max_ntries',default='')
  if max_ntries :
    max_ntries = int(float(max_ntries))
  else :
    max_ntries = nruns

  print( indent + "> testcase time start     : " + str(testcase_time_start) )
  print( indent + "> testcase class          : " + testcase_class )
  print( indent + "> testcase description    : " + testcase_desc )
  print( indent + "> testcase timeout        : " + str(timeout_cmd) )
  print( indent + "> testcase thread timeout : " + str(timeout_thread) )
  print( indent + "> number of runs          : " + str(nruns) )
  print( indent + "> number of max tries     : " + str(max_ntries) )  
  print( indent + "> testcase tags           : " + tags )

  logdir_local_testcase = openairdir_local + '/cmake_targets/autotests/log/'+ testcasename
  logdir_eNB_testcase   = logdirOAI5GRepo +'/cmake_targets/autotests/log/'+ testcasename 
  logdir_UE_testcase    = logdirOAI5GRepo +'/cmake_targets/autotests/log/'+ testcasename 

  #
  # Local checks for test cases 
  # -----------------------------------------------
  
  # Logging directory
  if (not os.path.exists(logdir_local_testcase)):
    os.system('mkdir -p ' + logdir_local_testcase)

  #
  # REMOTE MACHINE COMPILATION
  # ------------------------------------------------
  
  eNBMachine = testcase.findtext('eNB',default='')
  eNB_config_file = testcase.findtext('eNB_config_file',default='')
  eNB_compile_prog = testcase.findtext('eNB_compile_prog',default='')
  eNB_compile_prog_args = testcase.findtext('eNB_compile_prog_args',default='')

  logfile_compile_eNB           = logdir_eNB_testcase + '/eNB_compile.log'
  logfile_task_eNB_compile_out  = logdir_eNB_testcase + '/eNB_task_compile_out.log'
  logfile_task_eNB_compile      = logdir_local_testcase + '/eNB_task_compile.log'

  # Check that machine is in test setup machine list
  print( Fore.WHITE + indent + "> testcase eNB machine    :"),
  if (eNBMachine not in MachineList):
    print( Fore.RED + eNBMachine + " not in test setup machine list")
    testcase_verdict = 'INCON'
  else :
    print eNBMachine,
    # get machine description
    eNBMachineDesc = MachineDescDic[eNBMachine]                   
    index_eNBMachine = MachineList.index(eNBMachine)
    # check that openairinterface is installed on machine
    oai_eNB = openair('localdomain', eNBMachine)
    oai_eNB.connect(user, password)
    cmd = 'cd ' + logdirOAI5GRepo + '; source oaienv ; env|grep --color=never OPENAIR'
    res= oai_eNB.send_recv(cmd)
    m = re.search('OPENAIR_HOME', res, re.DOTALL)
    if  m:  
      print
      # Create testcase directory on remote eNB
      cmd = 'rm -fr ' + logdir_eNB_testcase + ' ; mkdir -p ' + logdir_eNB_testcase
      result = oai_eNB.send_recv(cmd)
    
      # Check if we need to compile lte-softmodem-noS1 on remote machine
      eNB_compile_cmd = eNB_compile_prog + ' '+ eNB_compile_prog_args
      if  ('last_compile_prog' in eNBMachineDesc) and eNBMachineDesc['last_compile_prog'] == eNB_compile_cmd:
        print( Fore.WHITE + indent + "> eNB machine compilation : skipped -> "+eNB_compile_cmd )
      else:
        print( Fore.WHITE + indent + "> eNB machine compilation : triggered -> "+eNB_compile_cmd )
        eNBMachineDesc['last_compile_prog'] = eNB_compile_prog + ' '+ eNB_compile_prog_args        # if last compilation is the same do not compile again
        task_eNB_compile = ' ( uname -a ; date \n'
        task_eNB_compile = task_eNB_compile + 'cd ' + logdirOAI5GRepo + ' ; source oaienv ; source cmake_targets/tools/build_helper \n'
        task_eNB_compile = task_eNB_compile + 'env |grep OPENAIR  \n'
        task_eNB_compile = task_eNB_compile + update_config_file(oai_eNB, eNB_config_file, logdirOAI5GRepo, '$OPENAIR_DIR/cmake_targets/autotests/tools/search_repl.py') + '\n'
        if eNB_compile_cmd != "":
          task_eNB_compile  = task_eNB_compile +  ' ( ' + eNB_compile_cmd + ' ) > ' + logfile_compile_eNB + ' 2>&1 \n'
        task_eNB_compile =  task_eNB_compile + ' date ) > ' + logfile_task_eNB_compile_out + ' 2>&1  '
        write_file(logfile_task_eNB_compile, task_eNB_compile, mode="w")


        thread_eNB = oaiThread(1, "eNB_thread_compile", eNBMachine, user, password , task_eNB_compile, False, timeout_thread)
        threads.append(thread_eNB)
        thread_eNB.start()

    else:
      print( Fore.RED + " -> OAI5G not properly setup ! please check REMOTE MACHINE PREPARATION step")
      testcase_verdict = 'INCON'


  UEMachine = testcase.findtext('UE',default='')
  UE_config_file = testcase.findtext('UE_config_file',default='')
  UE_compile_prog = testcase.findtext('UE_compile_prog',default='')
  UE_compile_prog_args = testcase.findtext('UE_compile_prog_args',default='')

  logfile_compile_UE           = logdir_UE_testcase + '/UE_compile.log'
  logfile_task_UE_compile_out  = logdir_UE_testcase + '/UE_task_compile_out.log'
  logfile_task_UE_compile      = logdir_local_testcase + '/UE_task_compile.log'

  # Check that machine is in test setup machine list
  print( Fore.WHITE + indent + "> testcase UE machine     :"),
  if (UEMachine not in MachineList):
    print( Fore.RED + UEMachine + " not in test setup machine list")
    testcase_verdict = 'INCON'
  else :
    print UEMachine,
    # get machine description
    UEMachineDesc = MachineDescDic[UEMachine]                   
    index_UEMachine = MachineList.index(UEMachine)
    # check that openairinterface is installed on machine
    oai_UE = openair('localdomain', UEMachine)
    oai_UE.connect(user, password)
    cmd = 'cd ' + logdirOAI5GRepo + '; source oaienv ; env|grep --color=never OPENAIR'
    res= oai_UE.send_recv(cmd)
    m = re.search('OPENAIR_HOME', res, re.DOTALL)
    if  m:  
      print
      # Create testcase directory on remote UE
      cmd = 'rm -fr ' + logdir_UE_testcase + ' ; mkdir -p ' + logdir_UE_testcase
      result = oai_UE.send_recv(cmd)
    
      # Check if we need to compile lte-softmodem-noS1 on remote machine
      UE_compile_cmd = UE_compile_prog + ' '+ UE_compile_prog_args
      if  ('last_compile_prog' in UEMachineDesc) and UEMachineDesc['last_compile_prog'] == UE_compile_cmd:
        print( Fore.WHITE + indent + "> UE machine compilation  : skipped -> "+UE_compile_cmd )
      else:
        print( Fore.WHITE + indent + "> UE machine compilation  : triggered -> "+UE_compile_cmd )
        UEMachineDesc['last_compile_prog'] = UE_compile_prog + ' '+ UE_compile_prog_args
        task_UE_compile = ' ( uname -a ; date \n'
        task_UE_compile = task_UE_compile + 'cd ' + logdirOAI5GRepo + ' ; source oaienv ; source cmake_targets/tools/build_helper \n'
        task_UE_compile = task_UE_compile + 'env |grep OPENAIR  \n'
        task_UE_compile = task_UE_compile + update_config_file(oai_UE, UE_config_file, logdirOAI5GRepo, '$OPENAIR_DIR/cmake_targets/autotests/tools/search_repl.py') + '\n'
        if UE_compile_cmd != "":
          task_UE_compile  = task_UE_compile +  ' ( ' + UE_compile_cmd + ' ) > ' + logfile_compile_UE + ' 2>&1 \n'
        task_UE_compile =  task_UE_compile + ' date ) > ' + logfile_task_UE_compile_out + ' 2>&1  '
        write_file(logfile_task_UE_compile, task_UE_compile, mode="w")

        thread_UE = oaiThread(2, "UE_thread_compile", UEMachine, user, password , task_UE_compile, False, timeout_thread)
        threads.append(thread_UE)
        thread_UE.start()

    else:
      print( Fore.RED + " -> OAI5G not properly setup ! please check REMOTE MACHINE PREPARATION step")
      testcase_verdict = 'INCON'






  # Wait for Compilation thread to terminate
  #-----------------------------------------
  for t in threads:
    t.join()

    # TODO check that compilation is succeed
    
    #first we compile all the programs
#    thread_UE = oaiThread(3, "UE_thread", UEMachine, user, password  , task_UE_compile, False, timeout_thread) 
#    threads.append(thread_UE)
#    thread_UE.start()
    




#  index_UEMachine = MachineList.index(UEMachine)
#  oai_UE = openair('localdomain', UEMachine)
#  oai_UE.connect(user, password)
#  res = oai_UE.send_recv(cmd)
#  res = oai_eNB.send_recv(cmd)



  #
  # RUN LOOP
  # ------------------------------------------------
  if testcase_verdict != 'PASS':      # if something went wrong to not run test cases
    max_ntries=0

  indent="\t\t"

  runs_results = []
  nb_runs       = 0
  nb_run_pass   = 0
  nb_run_failed = 0
  nb_run_inc    = 0
  nb_run_skip   = 0

  nb_seg_fault  = 0

  for run in range(0,max_ntries):

    if nruns == nb_run_pass + nb_run_failed:
      break

    nb_runs   += 1

    #
    # RUN initialization
    # ----------------------------------------------------
    
    prefix_string = Fore.WHITE + indent + "> RUN_"+str(run).zfill(2)+"              :"

#    print (Fore.WHITE + indent + "> RUN_"+str(run).zfill(2)+"                  : " ),
#    sys.stdout.flush()

    run_start_time=datetime.datetime.now()

    logdir_local_run = openairdir_local + '/cmake_targets/autotests/log/'+ testcasename + '/run_' + str(run)
    logdir_eNB_run   = logdirOAI5GRepo +'/cmake_targets/autotests/log/'  + testcasename + '/run_' + str(run)
    logdir_UE_run    = logdirOAI5GRepo +'/cmake_targets/autotests/log/'  + testcasename + '/run_' + str(run)

    cmd = 'rm -fr ' + logdir_eNB_run + ' ; mkdir -p ' + logdir_eNB_run
    result = oai_eNB.send_recv(cmd)
    cmd = 'rm -fr ' + logdir_UE_run + ' ; mkdir -p ' +  logdir_UE_run
    result = oai_UE.send_recv(cmd)
    cmd = ' rm -fr ' + logdir_local_run + ' ; mkdir -p ' + logdir_local_run
    result = os.system(cmd)


    #
    # RUN parametrization
    # ----------------------------------------------------
    eNB_pre_exec          = testcase.findtext('eNB_pre_exec',default='')
    eNB_pre_exec_args     = testcase.findtext('eNB_pre_exec_args',default='')
    eNB_main_exec         = testcase.findtext('eNB_main_exec',default='')
    eNB_main_exec_args    = testcase.findtext('eNB_main_exec_args',default='')
    eNB_traffic_exec      = testcase.findtext('eNB_traffic_exec',default='')
    eNB_traffic_exec_args = testcase.findtext('eNB_traffic_exec_args',default='')
    eNB_terminate_missing_procs = testcase.findtext('eNB_terminate_missing_procs',default='True')

    logfile_exec_eNB      = logdir_eNB_run   + '/eNB_exec'     + '_' + str(run) + '_.log'
    logfile_pre_exec_eNB  = logdir_eNB_run   + '/eNB_pre_exec' + '_' + str(run) + '_.log'
    logfile_task_eNB_out  = logdir_eNB_run   + '/eNB_task_out' + '_' + str(run) + '_.log'
    logfile_traffic_eNB   = logdir_eNB_run   + '/eNB_traffic'  + '_' + str(run) + '_.log'
    logfile_task_eNB      = logdir_local_run + '/eNB_task'     + '_' + str(run) + '_.log'

    task_eNB = ' ( uname -a ; date \n'
    task_eNB = task_eNB + 'cd ' + logdirOAI5GRepo + ' ; source oaienv ; source cmake_targets/tools/build_helper \n'
    task_eNB = task_eNB + 'env |grep OPENAIR  \n' + 'array_exec_pid=() \n'
    if eNB_pre_exec != "":
       task_eNB  = task_eNB +  ' ( date; ' + eNB_pre_exec + ' '+ eNB_pre_exec_args + ' ) > ' + logfile_pre_exec_eNB + ' 2>&1 \n'
    if eNB_main_exec != "":
       task_eNB = task_eNB + ' ( date; ' + addsudo(eNB_main_exec + ' ' + eNB_main_exec_args, '') + ' ) > ' + logfile_exec_eNB + ' 2>&1 & \n'
       task_eNB = task_eNB + 'array_exec_pid+=($!) \n'
       task_eNB = task_eNB + 'echo eNB_main_exec PID = $! \n'
    if eNB_traffic_exec != "":
       cmd_traffic = eNB_traffic_exec + ' ' + eNB_traffic_exec_args
       if cmd_traffic.find('-c') >= 0:
          cmd_traffic = cmd_traffic + ' -t ' + str(timeout_cmd - 80)
       task_eNB = task_eNB + ' (date;  ' + cmd_traffic + ' ) > ' + logfile_traffic_eNB + ' 2>&1 & \n'
       task_eNB = task_eNB + 'array_exec_pid+=($!) \n'
       task_eNB = task_eNB + 'echo eNB_traffic_exec PID = $! \n'
    #terminate the eNB test case after timeout_cmd seconds
    task_eNB  = task_eNB + finalize_deploy_script (timeout_cmd, eNB_terminate_missing_procs) + ' \n'
    #task_eNB  = task_eNB + 'sleep ' +  str(timeout_cmd) + ' \n'
    task_eNB  = task_eNB + 'handle_ctrl_c' + '\n' 
    task_eNB  = task_eNB + ' ) > ' + logfile_task_eNB_out + ' 2>&1  '
    write_file(logfile_task_eNB, task_eNB, mode="w")


    UE_pre_exec                 = testcase.findtext('UE_pre_exec',default='')
    UE_pre_exec_args            = testcase.findtext('UE_pre_exec_args',default='')
    UE_main_exec                = testcase.findtext('UE_main_exec',default='')
    UE_main_exec_args           = testcase.findtext('UE_main_exec_args',default='')
    UE_traffic_exec             = testcase.findtext('UE_traffic_exec',default='')
    UE_traffic_exec_args        = testcase.findtext('UE_traffic_exec_args',default='')
    UE_terminate_missing_procs  = testcase.findtext('UE_terminate_missing_procs',default='True')
    UE_search_expr_true         = testcase.findtext('UE_search_expr_true','')

    logfile_exec_UE     = logdir_UE_run + '/UE_exec'     + '_' + str(run) + '_.log'
    logfile_pre_exec_UE = logdir_UE_run + '/UE_pre_exec' + '_' + str(run) + '_.log'
    logfile_task_UE_out = logdir_UE_run + '/UE_task_out' + '_' + str(run) + '_.log'
    logfile_traffic_UE  = logdir_UE_run + '/UE_traffic'  + '_' + str(run) + '_.log'   
    logfile_task_UE     = logdir_local_run + '/UE_task'  + '_' + str(run) + '_.log'
  

    task_UE = ' ( uname -a ; date \n'
    task_UE = task_UE + 'array_exec_pid=()' + '\n'
    task_UE = task_UE + 'cd ' + logdirOAI5GRepo + '\n'  
    task_UE = task_UE + 'source oaienv \n'
    task_UE = task_UE + 'source cmake_targets/tools/build_helper \n'
    task_UE = task_UE + 'env |grep OPENAIR  \n'
    if UE_pre_exec != "":
      task_UE  = task_UE +  ' ( date; ' + UE_pre_exec + ' '+ UE_pre_exec_args + ' ) > ' + logfile_pre_exec_UE + ' 2>&1 \n'
    if UE_main_exec != "":
      task_UE = task_UE + ' ( date;  ' + addsudo(UE_main_exec + ' ' + UE_main_exec_args, '')  + ' ) > ' + logfile_exec_UE + ' 2>&1 & \n'
      task_UE = task_UE + 'array_exec_pid+=($!) \n'
      task_UE = task_UE + 'echo UE_main_exec PID = $! \n'
    if UE_traffic_exec != "":
       cmd_traffic = UE_traffic_exec + ' ' + UE_traffic_exec_args
       if cmd_traffic.find('-c') >= 0:
          cmd_traffic = cmd_traffic + ' -t ' + str(timeout_cmd - 60)
       task_UE = task_UE + ' ( date;  ' + cmd_traffic + ' ) >' + logfile_traffic_UE + ' 2>&1 & \n'
       task_UE = task_UE + 'array_exec_pid+=($!) \n'
       task_UE = task_UE + 'echo UE_traffic_exec PID = $! \n'
    #terminate the UE test case after timeout_cmd seconds
    task_UE  = task_UE + finalize_deploy_script (timeout_cmd, UE_terminate_missing_procs) + ' \n'
    #task_UE  = task_UE + 'sleep ' +  str(timeout_cmd) + ' \n'
    task_UE  = task_UE + 'handle_ctrl_c' + '\n' 
    task_UE  = task_UE + ' ) > ' + logfile_task_UE_out + ' 2>&1 '
    write_file(logfile_task_UE, task_UE, mode="w")
    #task_UE = 'echo \" ' + task_UE + '\" > ' + logfile_script_UE + ' 2>&1 ; ' + task_UE







    #
    # RUN work
    # ----------------------------------------------------
    thread_eNB = oaiThread(1, "eNB_thread", eNBMachine, user, password , task_eNB, False, timeout_thread)
    thread_UE = oaiThread(2, "UE_thread", UEMachine, user, password  , task_UE, False, timeout_thread) 
    threads=[]
    threads.append(thread_eNB)
    threads.append(thread_UE)
    thread_eNB.start()
    thread_UE.start()

    run_work_inprogress_flag = True
    progress_step = timeout_thread/20
    progress_count = 0

    while (run_work_inprogress_flag):


      update_progress(float(progress_count)/float(timeout_thread),prefix_string)

      time.sleep(progress_step)

      progress_count += progress_step

#      sys.stdout.write ('#')
#      sys.stdout.flush()

      run_work_inprogress_flag = False
      for t in threads:
          if t.isAlive():
            run_work_inprogress_flag = True

    
    update_progress(1,prefix_string)

    for t in threads:
      t.join()


    #
    # 
    #-----------------------------------------------------
    cleanOldProgramsAllMachines([oai_eNB, oai_UE] , oldprogramList, CleanUpAluLteBox, ExmimoRfStop, [logdir_eNB_run, logdir_UE_run], logdirOAI5GRepo)       

    SSHSessionWrapper(eNBMachine, user, None, password, logdir_eNB_run, logdir_local_testcase, "get_all")
   #print "Copying files from UEMachine : " + UEMachine + "logdir_UE = " + logdir_UE
    SSHSessionWrapper(UEMachine, user, None, password, logdir_UE_run, logdir_local_testcase, "get_all")

    metric_checks_flag = 1

    fname = logdir_local_run+ '/UE_exec'     + '_' + str(run) + '_.log'
    cell_synch_status = analyser.check_cell_synchro(fname)
    print (Fore.WHITE + indent + "> Check Cell synchro  :"),
      
    if cell_synch_status == 'CELL_SYNCH':
      print ( Fore.GREEN + cell_synch_status)
    else :
      print ( Fore.RED + cell_synch_status)
      metric_checks_flag = 0

    ue_seg_fault_status = analyser.check_exec_seg_fault(fname)
    if ue_seg_fault_status == 'SEG_FAULT' :
      nb_seg_fault += 1

    #
    # Check metrics
    #----------------------------------------------------
    runs_metrics = []
    run_traffic = []

    if metric_checks_flag :
      verdict = 'PASS'

      # UE side metrics
      metricList=testcase.findall('UE_metric')
      for metric in metricList:
        
        metric_verdict = 'PASS'

        metric_def = {}
        metric_def['id']            = metric.get('id') 
        metric_def['description']   = metric.get('description') 
        metric_def['regex']         = metric.get('regex') 
        metric_def['unit_of_meas']  = metric.get('unit_of_meas') 
        metric_def['pass_fail_stat']= metric.get('pass_fail_stat') 
        metric_min_lim = metric.get('min_limit')
        if metric_min_lim:
          metric_def['min_limit']     = float(metric_min_lim)
        metric_max_lim = metric.get('max_limit')
        if metric_max_lim:
          metric_def['max_limit']     = float(metric_max_lim)

        fname = logdir_local_run+ '/UE_exec'     + '_' + str(run) + '_.log'

        args = {'metric' : metric_def,
                'file' : fname }
    
        metric_extracted = analyser.do_extract_metrics(args)

        print (Fore.WHITE + indent + "> Metric              : "+metric_def['id']),
        print ("(min="+str( metric_extracted['metric_min'])+", max="+str( metric_extracted['metric_max'])+", mean="+str( metric_extracted['metric_mean'])+", median="+str( metric_extracted['metric_median'])+")")

#        print "\t  > Metric "+metric_def['id']+" :"
#        print "\t\t> min       = "+ str( metric_extracted['metric_min'] )
#        print "\t\t> min_index = "+ str( metric_extracted['metric_min_index'] )
#        print "\t\t> max       = "+ str( metric_extracted['metric_max'] )
#        print "\t\t> max_index = "+ str( metric_extracted['metric_max_index'] )
#        print "\t\t> mean      = "+ str( metric_extracted['metric_mean'] )
#        print "\t\t> median    = "+ str( metric_extracted['metric_median'] )      

        if metric_def['pass_fail_stat'] :
          metric_verdict = analyser.do_check_verdict(metric_def, metric_extracted)
   
        metric_fig = logdir_local_run+ '/UE_metric_'+ metric_def['id']+'_' + str(run) + '_.png'
        analyser.do_img_metrics(metric_def, metric_extracted, metric_fig)


        metric_fig_report = '../log/'+ testcasename + '/run_' + str(run) + '/UE_metric_'+ metric_def['id']+'_' + str(run) + '_.png'

        run_metrics = dict( metric_id       = metric_def['id'],
                            metric_desc     = metric_def['description'],
                            metric_uom      = metric_def['unit_of_meas'],
                            metric_min      = metric_extracted['metric_min'],
                            metric_min_index= metric_extracted['metric_min_index'],
                            metric_max      = metric_extracted['metric_max'],
                            metric_max_index= metric_extracted['metric_max_index'],
                            metric_mean     = metric_extracted['metric_mean'],
                            metric_median   = metric_extracted['metric_median'],
                            metric_fig      = metric_fig_report)

        if metric_def['pass_fail_stat'] :
          run_metrics['pass_fail_stat'] = metric_def['pass_fail_stat']
        if metric_min_lim :
          run_metrics['pass_fail_min_limit'] = metric_def['min_limit']
        if metric_max_lim :
          run_metrics['pass_fail_max_limit'] = metric_def['max_limit']

        runs_metrics.append(run_metrics)

        if metric_verdict != 'PASS':
          verdict = metric_verdict

      # End Metrics LOOP
      # ---------------------


      # Traffic analysis
      if UE_traffic_exec != "":

        fname = logdir_local_run+ '/UE_traffic'     + '_' + str(run) + '_.log'
        args = {'file' : fname }
        traffic_metrics = analyser.do_extract_traffic_metrics(args)
        traffic_fig = logdir_local_run+ '/UE_traffic'+'_' + str(run) + '_.png'
        analyser.do_img_traffic(traffic_metrics, traffic_fig)

        traffic_fig_report= '../log/'+ testcasename + '/run_' + str(run) + '/UE_traffic'+'_' + str(run) + '_.png'

        dur_pass_fail_crit = 'none'
        if traffic_metrics['traffic_count'] == 0 :
          verdict = 'FAIL'
        elif UE_search_expr_true != "":
          traffic_duration_limit = float(UE_search_expr_true)
          dur_pass_fail_crit = str(traffic_duration_limit)
  #        print 'traffic_duration_limit = '+str(traffic_duration_limit)
  #        print 'traffic_metrics[interval_stop_max] = '+str(traffic_metrics['interval_stop_max'])
          if traffic_metrics['interval_stop_max'] < traffic_duration_limit :
            verdict = 'FAIL'


        run_traffic = dict( traffic_count   = traffic_metrics['traffic_count'],
                            bw_min          = traffic_metrics['bw_min'],
                            bw_max          = traffic_metrics['bw_max'],
                            bw_mean         = traffic_metrics['bw_mean'],
                            bw_median       = traffic_metrics['bw_median'],
                            jitter_min      = traffic_metrics['jitter_min'],
                            jitter_max      = traffic_metrics['jitter_max'],
                            jitter_mean     = traffic_metrics['jitter_mean'],
                            jitter_median   = traffic_metrics['jitter_median'],
                            rl_min          = traffic_metrics['rl_min'],
                            rl_max          = traffic_metrics['rl_max'],
                            rl_mean         = traffic_metrics['rl_mean'],
                            rl_median       = traffic_metrics['rl_median'],
                            iperf_duration  = traffic_metrics['interval_stop_max'],
                            dur_pass_fail_crit = dur_pass_fail_crit,
                            traffic_fig     = traffic_fig_report )
      else:
        run_traffic = dict( traffic_count   = 0)

    else :
      verdict = 'SKIP'



    #
    # RUN verdict and finalization
    # ----------------------------------------------------
    run_stop_time=datetime.datetime.now()
    run_duration = run_stop_time-run_start_time
#   print (Fore.WHITE + ("duration=" :"),
    print (Fore.WHITE + indent + "> RUN duration        : "+ str(run_duration) +"s" )

    print (Fore.WHITE + indent + "> RUN verdict         :"),

    if verdict == 'PASS':
      nb_run_pass   += 1
      print ( Fore.GREEN + verdict)
    elif verdict == 'FAIL':
      nb_run_failed += 1
      testcase_verdict = 'FAIL'
      print ( Fore.RED + verdict)
    elif verdict == 'SKIP':
      nb_run_skip += 1
      print ( Fore.YELLOW + verdict)
    else:
      nb_run_inc    += 1
      if testcase_verdict == 'PASS' : testcase_verdict = 'INCON'
      print (Fore.YELLOW+'INCONCLUSIVE')

    run_results = dict( run_id=str(run),
                        run_start_time = run_start_time,
                        run_stop_time  = run_stop_time ,
                        run_verdict    = verdict,
                        ue_seg_fault_status = ue_seg_fault_status,
                        run_duration   = run_duration,
                        runs_metrics   = runs_metrics,
                        run_traffic   = run_traffic)
    runs_results.append(run_results)

  # END RUN LOOP
  #----------------------------------------------------

  indent="\t"

  # Test case duration
  # ----------------------------------  
  testcase_time_stop = datetime.datetime.now()
  print(Fore.WHITE + indent + "> testcase time stop      : " + str(testcase_time_start) ),
  print " -> TestCase duration = "+str(testcase_time_stop - testcase_time_start)

  # Save remote log files
  # -----------------------------------  
  #Now we change the permissions of the logfiles to avoid some of them being with root permissions
  cmd = 'sudo -E chown -R ' + user + ' ' + logdir_eNB_testcase
  res= oai_eNB.send_recv(cmd)
  cmd = 'sudo -E chown -R ' + user + ' ' +  logdir_UE_testcase
  res= oai_UE.send_recv(cmd)

  # Save remote log files on MTC
  #print "Copying files from eNBMachine " + eNBMachine + "logdir_eNB = " + logdir_eNB
  SSHSessionWrapper(eNBMachine, user, None, password, logdir_eNB_testcase, openairdir_local + '/cmake_targets/autotests/log/', "get_all")
  #print "Copying files from UEMachine : " + UEMachine + "logdir_UE = " + logdir_UE
  SSHSessionWrapper(UEMachine, user, None, password, logdir_UE_testcase, openairdir_local + '/cmake_targets/autotests/log/', "get_all")

  oai_eNB.disconnect()
  oai_UE.disconnect()

  # Set test case final verdict
  # -----------------------------------
  test_result_string = 'do be completed'

  if testcase_verdict == 'PASS':
    if nb_run_pass == 0 : 
      testcase_verdict = 'INCON'

  print(Fore.WHITE + indent + "> testcase final verdict  :"),
  if testcase_verdict == 'PASS':
    print ( Fore.GREEN + testcase_verdict)
  elif testcase_verdict == 'FAIL':
    print ( Fore.RED + testcase_verdict)
  else:
    print (Fore.YELLOW+'INCONCLUSIVE')

  print(Style.RESET_ALL)

  duration= testcase_time_stop - testcase_time_start

  test_result = dict(testcase_name=testcasename,
                     testcaseclass=testcaseclass, 
                     testcase_verdict = testcase_verdict,
                     testcase_time_start=testcase_time_start,
                     testcase_time_stop=testcase_time_stop,
                     tags=tags,
                     nruns=nb_runs,
                     nb_run_pass = nb_run_pass,
                     nb_run_skip = nb_run_skip,
                     nb_run_failed = nb_run_failed,
                     nb_run_inc=nb_run_inc,
                     nb_seg_fault = nb_seg_fault,
                     testcase_timeout=timeout_cmd,
                     testcase_duration = duration,
                     testcase_eNBMachine =eNBMachine,
                     testcase_UEMachine =UEMachine,
                     runs_results = runs_results)
  test_results.append(test_result)

  xmlFile = logdir_local_testcase + '/test.' + testcasename + '.xml'
  xml="\n<testcase classname=\'"+ testcaseclass +  "\' name=\'" + testcasename + "."+tags +  "\' Run_result=\'" + test_result_string + "\' time=\'" + str(duration) + " s \' RESULT=\'" + testcase_verdict + "\'></testcase> \n"
  write_file(xmlFile, xml, mode="w")

#  xmlFile_ng = logdir_local_testcase + '/test.' + testcasename + '_ng.xml'
#  xml_ng = xmlify(test_result, wrap=testcasename, indent="  ")
#  write_file(xmlFile_ng, xml_ng, mode="w")



  return testcase_verdict




# \brief This function searches if test case is present in list of test cases that need to be executed by user
# \param testcasename the test case to search for
# \param testcasegroup list that is passed from the arguments
# \param test_case_exclude list of test cases excluded from execution (specified in test_case_list.xml)
def search_test_case_group(testcasename, testcasegroup, test_case_exclude):
    
    if test_case_exclude != "":
       testcase_exclusion_list=test_case_exclude.split()
       for entry in testcase_exclusion_list:
          if entry.find('+') >=0:
            match = re.search(entry, testcasename)
            if match:
               #print "\nSkipping test case as it is found in black list: " + testcasename
               return False
          else:
             match = entry.find(testcasename)
             if match >=0:
                #print "\nSkipping test case as it is found in black list: " + testcasename
                return False
    if testcasegroup == '':
         return True
    else:
      testcaselist = testcasegroup.split()
      for entry in testcaselist:
        if entry.find('+') >=0:
           match = re.search(entry, testcasename)
           if match:
              return True
        else:
           match = testcasename.find(entry)
           if match >=0:
             return True
    return False

# \brief thread that cleans up remote machines from pre-existing test case executions
# \param threadID number of thread (for book keeping)
# \param threadname name of thread (for book keeping)
# \param oai handler that can be used to execute programs on remote machines
# \param CleanUpOldProgs list of programs which must be terminated before running a test case (specified in test_case_list.xml)
# \param CleanupAluLteBox string that contains commands to stop ALU Bell Labs LTEBox (specified in test_case_list.xml)
# \param ExmimoRfStop command to stop EXMIMO Card
class oaiCleanOldProgramThread (threading.Thread):
    def __init__(self, threadID, threadname, oai, CleanUpOldProgs, CleanUpAluLteBox, ExmimoRfStop, logdir, logdirOAI5GRepo):
        threading.Thread.__init__(self)
        self.threadID = threadID
        self.threadname = threadname
        self.oai = oai
        self.CleanUpOldProgs = CleanUpOldProgs
        self.CleanUpAluLteBox = CleanUpAluLteBox
        self.ExmimoRfStop = ExmimoRfStop
        self.logdir = logdir
        self.logdirOAI5GRepo = logdirOAI5GRepo
    def run(self):
        try:
          cleanOldPrograms(self.oai, self.CleanUpOldProgs, self.CleanUpAluLteBox, self.ExmimoRfStop, self.logdir, self.logdirOAI5GRepo)
        except Exception, e:
           error=''
           error = error + ' In class oaiCleanOldProgramThread, function: ' + sys._getframe().f_code.co_name + ': *** Caught exception: '  + str(e.__class__) + " : " + str( e)
           error = error + '\n threadID = ' + str(self.threadID) + '\n threadname = ' + self.threadname + '\n CleanUpOldProgs = ' + self.CleanUpOldProgs + '\n CleanUpAluLteBox = ' + self.CleanUpAluLteBox + '\n ExmimoRfStop = ' + self.ExmimoRfStop + '\n'  
           error = error + traceback.format_exc()
           print error
           print "There is error in cleaning up old programs....."
           #sys.exit(1)

# \brief Run parallel threads in all machines for clean up old execution of test cases
# \param oai_list list of handlers that can be used to execute programs on remote machines
# \param CleanUpOldProgs list of programs which must be terminated before running a test case (specified in test_case_list.xml)
# \param CleanupAluLteBox string that contains commands to stop ALU Bell Labs LTEBox (specified in test_case_list.xml)
# \param ExmimoRfStop command to stop EXMIMO Card
def cleanOldProgramsAllMachines(oai_list, CleanOldProgs, CleanUpAluLteBox, ExmimoRfStop, logdir_list, logdirOAI5GRepo):
   threadId=0
   threadList=[]
   index=0
   if len(oai_list)!=len(logdir_list) :
       logdir_list=[logdir[0]]*len(oai_list)

   for oai in oai_list:
      threadName="cleanup_thread_"+str(threadId)
      thread=oaiCleanOldProgramThread(threadId, threadName, oai, CleanUpOldProgs, CleanUpAluLteBox, ExmimoRfStop, logdir_list[index],logdirOAI5GRepo)
      threadList.append(thread)
      thread.start()
      threadId = threadId + 1
      index = index+1
   for t in threadList:
      t.join()


debug = 0
pw =''
i = 0
dlsim=0
localshell=0
GitOAI5GRepo=''
GitOAI5GRepoBranch=''
GitOAI5GHeadVersion=''
user=''
pw=''
testcasegroup=''
NFSResultsShare=''
cleanUpRemoteMachines=False
openairdir_local = os.environ.get('OPENAIR_DIR')
if openairdir_local is None:
   print "Environment variable OPENAIR_DIR not set correctly"
   sys.exit()
locallogdir = openairdir_local + '/cmake_targets/autotests/log'
reportdir = openairdir_local + '/cmake_targets/autotests/report'
MachineList = ''
MachineListGeneric=''
MachineDescDic={}
flag_remove_logdir=False
flag_start_testcase=False
nruns_lte_softmodem=''
flag_skip_git_head_check=False
flag_skip_oai_install=False
flag_skip_machine_preparation=False
flag_skip_sanity_check=False
flag_generate_html_report = False
Timeout_cmd=''
xmlInputFile=''

#print "Number of arguments argc = " + str(len(sys.argv))
#for index in range(1,len(sys.argv) ):
#  print "argv_" + str(index) + " : " + sys.argv[index]

oai_list = []


i=1
while i < len (sys.argv):
    arg=sys.argv[i]
    if arg == '-r':
        flag_remove_logdir=True
    elif arg == '-s' :
        flag_start_testcase=True
    elif arg == '-g' :
        testcasegroup = sys.argv[i+1].replace("\"","")
        i = i +1   
    elif arg == '-c':
        cleanUpRemoteMachines=True
    elif arg == '-5GRepo':
        GitOAI5GRepo = sys.argv[i+1]
        i = i +1
    elif arg == '-5GRepoBranch':
        GitOAI5GRepoBranch = sys.argv[i+1]
        i = i +1
    elif arg == '-5GRepoHeadVersion':
        GitOAI5GHeadVersion = sys.argv[i+1]
        #We now find the branch that corresponds to this Git Head Commit
        cmd = "git show-ref --head " + " | grep " + GitOAI5GHeadVersion
        cmd_out = subprocess.check_output ([cmd], shell=True)
        cmd_out=cmd_out.replace("\n","")
        cmd_out = cmd_out.split('/')
        GitOAI5GRepoBranch = cmd_out[-1]
        if GitOAI5GRepoBranch == '':
           print "Error extracting GitBranch from head commit. Exiting now..."
           sys.exit(1)
        i = i +1
    elif arg == '-u':
        user = sys.argv[i+1]
        i = i +1
    elif arg == '-p': 
        pw = sys.argv[i+1]
        i = i +1
    elif arg == '-n': 
        NFSResultsShare = sys.argv[i+1]
        i = i +1
    elif arg == '--nrun_lte_softmodem': 
        nruns_lte_softmodem = sys.argv[i+1]
        i = i +1
    elif arg == '-MachineList':
        MachineList =  sys.argv[i+1]
        MachineList = MachineList.replace("\"","")
        MachineList = MachineList.replace("\'","")
        i = i +1
    elif arg == '-MachineListGeneric':
        MachineListGeneric =  sys.argv[i+1]
        MachineListGeneric = MachineListGeneric.replace("\"","")
        MachineListGeneric = MachineListGeneric.replace("\'","")
        i = i +1
    elif arg == '--skip-git-head-check':
        flag_skip_git_head_check=True
    elif arg == '--timeout_cmd': 
        Timeout_cmd = sys.argv[i+1]
        i = i +1
    elif arg == '--skip-oai-install':
        flag_skip_oai_install=True
    elif arg == '--skip-machine-preparation':
        flag_skip_machine_preparation=True
    elif arg == '--skip-sanity-check':
        flag_skip_sanity_check=True
    elif arg == '--test-suite' :
        xmlInputFile = sys.argv[i+1]
        i = i +1
    elif arg == '--generate-html-report':
        flag_generate_html_report=True
    elif arg == '-h' :
        print "-s:  This flag *MUST* be set to start the test cases"
        print "-r:  Remove the log directory in autotests"
        print "-g:  Run test cases in a group"
        print "-c:  Run cleanup scripts on remote machines and exit"
        print "-5GRepo:  Repository for OAI 5G to use to run tests (overrides GitOAI5GRepo in test_case_list.xml)"
        print "-5GRepoBranch:  Branch for OAI 5G Repository to run tests (overrides the branch in test_case_list.xml)"
        print "-5GRepoHeadVersion:  Head commit on which to run tests (overrides the branch in test_case_list.xml)"
        print "-u:  use the user name passed as argument"
        print "-p:  use the password passed as an argument"
        print "-n:  Set the NFS share passed as an argument"
        print "--nrun_lte_softmodem:  Set the number of runs for lte-softmodem test case class"
        print "-MachineList : overrides the MachineList parameter in test_case_list.xml"
        print "-MachineListGeneric : overrides the MachineListGeneric  parameter in test_case_list.xml"
        print "--skip-git-head-check: skip checking of GitHead remote/local branch (only for debugging)"
        print "--timeout_cmd: Override the default parameter (timeout_cmd) in test_case_list.xml. This parameter is in seconds and should be > 120"
        print "--skip-oai-install: Skips the openairinterface5g installer"
        print "--skip-machine-preparation: skipp the whole system preparation -> direct to test cases"
        print "--skip-sanity-check: skipp the machine sanity checks"
        print "--test-suite: Select a XML test-suite file"
        print "--generate-html-report: Generate an HTML report of the test campaign (supported only for noS1 testsuite !!)"
        sys.exit()
    else :
        print "Unrecongnized Option: <" + arg + ">. Use -h to see valid options"
        sys.exit()
    i= i + 1     

logdir = '/tmp/' + 'OAITestFrameWork-' + user + '/'
logdirOAI5GRepo = logdir + 'openairinterface5g/'
logdirOpenaircnRepo = logdir + 'openair-cn/'
patchdir = logdirOAI5GRepo + 'cmake_targets/autotests/patches/'
NFSResultsDir = '/mnt/sradio'
cleanupOldProgramsScript = '$OPENAIR_DIR/cmake_targets/autotests/tools/remove_old_programs.bash'

#Now we parse the xml file for basic configuration
if xmlInputFile == '':
  xmlInputFile=os.environ.get('OPENAIR_DIR')+"/cmake_targets/autotests/test_case_list.xml"
xmlTree = ET.parse(xmlInputFile)
xmlRoot = xmlTree.getroot()

#
# Read test session configuration
# ---------------------------------------------------------------------------------
print (Fore.YELLOW + '\nStep 0.1 - Read test session configuration')
print (Fore.YELLOW + '-------------------------------------------------')

test_session_start_time = datetime.datetime.now()
print (Fore.WHITE + '  - start time                    : '+ str(test_session_start_time))

host = os.uname()[1]
print (Fore.WHITE + '  - MTC host                      : '+host)


if user=='':
  user = getpass.getuser()
print (Fore.WHITE + '  - user                          : '+user)

if pw=='':
  pw = getpass.getpass()
print (Fore.WHITE + '  - password                      : '+pw)

print (Fore.WHITE + '  - cmd line flags                : ')
print (Fore.WHITE + '    +  -s                         : '+str(flag_start_testcase))
print (Fore.WHITE + '    +  -r                         : '+str(flag_remove_logdir))
print (Fore.WHITE + '    +  -c                         : '+str(cleanUpRemoteMachines))
print (Fore.WHITE + '    +  --skip-git-head-check      : '+str(flag_skip_git_head_check))
print (Fore.WHITE + '    +  --skip-oai-install         : '+str(flag_skip_oai_install))
print (Fore.WHITE + '    +  --skip-machine-preparation : '+str(flag_skip_machine_preparation))
print (Fore.WHITE + '    +  --skip-sanity-check        : '+str(flag_skip_sanity_check))


print (Fore.WHITE + '  - Test setup basic config       : (* -> overwrite by cmd line parameter)')
print (Fore.WHITE + '    +  test setup config file     : '+str(xmlInputFile))
print (Fore.WHITE + '    +  remote setup dir           : '+str(logdir))
print (Fore.WHITE + '    +  remote OAI5G repo          : '+str(logdirOAI5GRepo))
print (Fore.WHITE + '    +  remote OpenAirCN repo      : '+str(logdirOpenaircnRepo))


if MachineList =='':
   MachineList = xmlRoot.findtext('MachineList',default='')
NFSResultsShare = xmlRoot.findtext('NFSResultsShare',default='')
GitOpenaircnRepo = xmlRoot.findtext('GitOpenair-cnRepo',default='')

if GitOAI5GRepo == '':
   GitOAI5GRepo = xmlRoot.findtext('GitOAI5GRepo',default='')

if GitOAI5GRepoBranch == '':
   GitOAI5GRepoBranch = xmlRoot.findtext('GitOAI5GRepoBranch',default='')
if GitOAI5GHeadVersion == '':
   GitOAI5GHeadVersion = xmlRoot.findtext('GitOAI5GHeadVersion',default='')

GitOpenaircnRepoBranch = xmlRoot.findtext('GitOpenair-cnRepoBranch',default='')
CleanUpOldProgs = xmlRoot.findtext('CleanUpOldProgs',default='')
CleanUpAluLteBox = xmlRoot.findtext('CleanUpAluLteBox',default='')
Timeout_execution = int (xmlRoot.findtext('Timeout_execution'))
if MachineListGeneric == '':
   MachineListGeneric = xmlRoot.findtext('MachineListGeneric',default='')
TestCaseExclusionList = xmlRoot.findtext('TestCaseExclusionList',default='')
ExmimoRfStop = xmlRoot.findtext('ExmimoRfStop',default='')
if nruns_lte_softmodem == '':
   nruns_lte_softmodem = xmlRoot.findtext('nruns_lte-softmodem',default='')

print (Fore.WHITE + '    +  MachineList                : '+ MachineList)
print (Fore.WHITE + '    +  GitOpenair-cnRepo          : '+  GitOpenaircnRepo)
print (Fore.WHITE + '    +  GitOAI5GRepo               : '+ GitOAI5GRepo)
print (Fore.WHITE + '    +  GitOAI5GBranch             : '+ GitOAI5GRepoBranch)
print (Fore.WHITE + '    +  GitOpenaircnRepoBranch     : '+ GitOpenaircnRepoBranch)
print (Fore.WHITE + '    +  NFSResultsShare            : '+ NFSResultsShare)
print (Fore.WHITE + '    +  nruns_lte_softmodem        : '+ nruns_lte_softmodem)
print (Fore.WHITE + '    +  Timeout_cmd                : '+ Timeout_cmd)


if GitOAI5GHeadVersion == '':
  cmd = "git log --pretty=format:\'%H\' -n 1" # origin/"+ GitOAI5GRepoBranch
#  cmd = "git show-ref --heads -s "+ GitOAI5GRepoBranch
  GitOAI5GHeadVersion = subprocess.check_output ([cmd], shell=True)
  GitOAI5GHeadVersion=GitOAI5GHeadVersion.replace("\n","")

 # Print commiter and date : git log -1 --pretty=format:"%an %ad" 

print (Fore.WHITE + '    +  GitOAI5GHeadVersion        : '+  GitOAI5GHeadVersion)
#print "CleanUpOldProgs = " + CleanUpOldProgs
print (Fore.WHITE + '    +  Timeout_execution (s)      : '+  str(Timeout_execution))

if GitOAI5GHeadVersion == '':
  print "Error getting the OAI5GBranch Head version...Exiting"
  sys.exit()

NFSTestsResultsDir = NFSResultsShare + '/'+ GitOAI5GRepoBranch + '/' + GitOAI5GHeadVersion

#print "NFSTestsResultsDir = " + NFSTestsResultsDir

OAI5GpatchFileList=xmlRoot.findall('OAI5GPatchFile')



MachineList = MachineList.split()
MachineListGeneric = MachineListGeneric.split()


MachineDescDic


#
# 'Check Mater Test Controller sanity (MTC)'
# ---------------------------------------------------------------------------------
print (Fore.YELLOW + '\nStep 0.2 - Check Mater Test Controller sanity (MTC)')
print (Fore.YELLOW + '-------------------------------------------------')

try:
  print (Fore.WHITE + '  - check OpenAirInterface environment variable OPENAIR1_DIR... '),
  os.environ["OPENAIR1_DIR"]
except KeyError:
    print (Fore.RED + 'FAILED')
    print "Please set the environment variable OPENAIR1_DIR in the .bashrc or run 'source oaienv' "
    exit_prog(1)
print (Fore.GREEN + 'OK')

try:
  print (Fore.WHITE + '  - check OpenAirInterface environment variable OPENAIR2_DIR... '),
  os.environ["OPENAIR2_DIR"]
except KeyError: 
  print (Fore.RED + 'FAILED')
  print "Please set the environment variable OPENAIR2_DIR in the .bashrc or run 'source oaienv' "
  exit_prog(1)
print (Fore.GREEN + 'OK')

try:  
   os.environ["OPENAIR_TARGETS"]
except KeyError: 
   print "Please set the environment variable OPENAIR_TARGETS in the .bashrc or run 'source oaienv' "
   sexit_prog(1)

print (Fore.WHITE + '  - kill zombie ssh sessions from earlier sessions...         '),
cmd='ps aux |grep \"/usr/bin/ssh -q -l ' + user + '\"| tr -s \" \" :| cut -f 2 -d :| xargs kill -9 '
os.system(cmd)
print (Fore.GREEN + 'DONE')

if flag_start_testcase == False:
  print "You need to start the testcase by passing option -s. Use -h to see all options. Aborting now..."
  exit_prog(1)

  
#print "Killing zombie ssh sessions from earlier sessions..."
#cmd='ps aux |grep \"/usr/bin/ssh -q -l guptar\"|tr -s \" \" :|cut -f 2 -d :|xargs kill -9 '
#cmd = cmd + '; ps aux |grep \"/usr/bin/ssh -q -l ' + user + '\"| tr -s \" \" :| cut -f 2 -d :| xargs kill -9 '
#os.system(cmd)
#print cmd

try:
  analyser = __import__("lib_autotest_analyser")
except ImportError as err:
  print('Import error: ' + str(err))
  exit(0)

if flag_remove_logdir == True:
   print "Removing directory: " + locallogdir
   os.system(' rm -fr ' + locallogdir + '; mkdir -p ' +  locallogdir  )
   
if not os.path.exists(locallogdir):
	os.system('mkdir -p ' +  locallogdir  )


paramiko_logfile = os.path.expandvars('$OPENAIR_DIR/cmake_targets/autotests/log/paramiko.log')
res=os.system(' echo > ' + paramiko_logfile)
paramiko.util.log_to_file(paramiko_logfile)


#
# 'REMOTE MACHINE SANITY CHEKS'
# ---------------------------------------------------------------------------------
print (Fore.YELLOW + '\nStep 0.3 - REMOTE MACHINE SANITY CHEKS')
print (Fore.YELLOW + '-------------------------------------------------')


for machine in MachineList: 
  oai_list.append( openair('localdomain',machine))
  MachineDescDic[machine]={}


if not flag_skip_sanity_check:

  if localshell == 0:
    try:
      index=0
      for machine in MachineList:
        print (Fore.WHITE + "  - Check sanity for remote machine "+machine)

        oai_list[index].connect(user,pw)
        print (Fore.WHITE + "    > Checking for sudo permissions for user "+user+ " : "),
        result = oai_list[index].send_expect_false('sudo -S -v','may not run sudo',True)
        if (result == 'OK'):
          print (Fore.GREEN + result )
        else:
          print (Fore.RED + result )
         
        print (Fore.WHITE + "    > Cleaning Older running programs : "),
        cleanOldPrograms(oai_list[index], CleanUpOldProgs, CleanUpAluLteBox, ExmimoRfStop, '$HOME', '/tmp')
        print (Fore.WHITE + " Done")

        #result = oai_list[index].send('mount ' + NFSResultsDir, True)
        #print "Mounting NFS Share " + NFSResultsDir + "..." + result

        # Check if NFS share is mounted correctly.
        #print 'Checking if NFS Share<' + NFSResultsDir + '> is mounted correctly...'
        #cmd = 'if grep -qs '+NFSResultsDir+ ' /proc/mounts; then  echo \'' + NFSResultsDir  + ' is mounted\' ; fi'
        #search_expr = NFSResultsDir + ' is mounted'
        #print "cmd = " + cmd
        #print "search_expr = " + search_expr
        #result = oai_list[index].send_expect(cmd, search_expr)
        #print "Mount NFS_Results_Dir..." + result
        index = index + 1

        #oai.connect2(user,pw) 
        #oai.get_shell()
    except :
        print 'Fail to connect to the machine: '+ machine 
        sys.exit(1)
  else:
      pw = ''
      oai_list[0].connect_localshell()
else:
  print (Fore.RED + "  Skipping Machine sanity check...")

#
# 'REMOTE MACHINE TEST SETUP CONFIGURATION'
# ---------------------------------------------------------------------------------
print (Fore.YELLOW + '\nStep 0.4 - REMOTE MACHINE PREPARATION')
print (Fore.YELLOW + '-------------------------------------------------')

if not flag_skip_machine_preparation:
  index=0
  threads_init_setup=[]
  for oai in oai_list:
    try:
        print "setting up machine: " + MachineList[index]
        #print oai_list[oai].send_recv('echo \''+pw+'\' |sudo -S -v')
        #print oai_list[oai].send_recv('sudo su')
        #print oai_list[oai].send_recv('who am i') 
        #cleanUpPrograms(oai_list[oai]
        cmd = 'sudo -S -E rm -fr ' + logdir + ' ; mkdir -p ' + logdir 
        result = oai.send_recv(cmd)
        print cmd

        setuplogfile  = logdir  + '/setup_log_' + MachineList[index] + '_.txt'
        setup_script  = locallogdir  + '/setup_script_' + MachineList[index] +  '_.txt'

        #Sometimes git fails so the script below retries in that case
        localfile = os.path.expandvars('$OPENAIR_DIR/cmake_targets/autotests/tools/git-retry.sh')
        remotefile = logdir + '/git-retry.sh'
        paramList=[]
        port=22
        paramList.append ( {"operation":'put', "localfile":localfile, "remotefile":remotefile} )
        sftp_log = os.path.expandvars(locallogdir + '/sftp_module.log')
        sftp_module (user, pw, MachineList[index], port, paramList, sftp_log)

        cmd = ' ( \n'
        #cmd = cmd  + 'rm -fR ' +  logdir + '\n'
        #cmd = cmd + 'mkdir -p ' + logdir + '\n'
        cmd = cmd + 'cd '+ logdir   + '\n'
        cmd = cmd + 'sudo apt-get install -y git \n'
        cmd = cmd + 'chmod 700 ' + logdir + '/git-retry.sh \n' 
        cmd = cmd + logdir + '/git-retry.sh clone  '+ GitOAI5GRepo  +' \n'
        cmd = cmd + logdir + '/git-retry.sh clone '+ GitOpenaircnRepo + ' \n'
        cmd = cmd +  'cd ' + logdirOAI5GRepo  + '\n'
        cmd = cmd + 'git checkout ' + GitOAI5GRepoBranch   + '\n'
        if GitOAI5GHeadVersion :
          cmd = cmd + 'git checkout ' + GitOAI5GHeadVersion   + '\n'
        cmd = cmd + 'git_head=`git ls-remote |grep \'' + GitOAI5GRepoBranch + '\'` \n'
        cmd = cmd + 'git_head=($git_head) \n'
        cmd = cmd + 'git_head=${git_head[0]} \n'
        cmd = cmd + 'echo \"GitOAI5GHeadVersion_remote = $git_head\" \n'
        cmd = cmd + 'echo \"GitOAI5GHeadVersion_local = ' + GitOAI5GHeadVersion + '\" \n'
        if flag_skip_git_head_check==True:
           cmd = cmd + 'echo \"skipping GitHead check...\" \n '
        else:
           cmd = cmd + 'if [ \"$git_head\" != \"'+ GitOAI5GHeadVersion + '\" ]; then echo \"error: Git openairinterface5g head version does not match\" ; fi \n'
        cmd = cmd + 'source oaienv'   + '\n'
        if flag_skip_oai_install == False:
           cmd = cmd + 'source $OPENAIR_DIR/cmake_targets/tools/build_helper \n'
           cmd = cmd + 'echo \"Installing core OAI dependencies...Start\" \n'
           cmd = cmd + '$OPENAIR_DIR/cmake_targets/build_oai -I --install-optional-packages \n'
           cmd = cmd + 'echo \"Installing core OAI dependencies...Finished\" \n'
           #cmd = cmd + 'echo \"Installing BLADERF OAI dependencies...Start\" \n'
           #cmd = cmd + 'check_install_bladerf_driver \n'
           #cmd = cmd + 'echo \"Installing BLADERF OAI dependencies...Finished\" \n'
           #cmd = cmd + 'echo \"Installing USRP OAI dependencies...Start\" \n'
           #cmd = cmd + 'check_install_usrp_uhd_driver \n'
           #cmd = cmd + 'echo \"Installing USRP OAI dependencies...Finished\" \n'
        cmd = cmd +  'cd ' + logdirOpenaircnRepo  + '\n'
        cmd = cmd +  'git checkout ' + GitOpenaircnRepoBranch  + '\n'
        cmd = cmd +  'env |grep OPENAIR'  + '\n'
        cmd = cmd + ' cd ' + logdir   + '\n'
        cmd = cmd + 'mkdir -p ' + patchdir + '\n'
        cmd = cmd + ' ) > ' +  setuplogfile + ' 2>&1 \n'
        #cmd = cmd + 'echo \' ' + cmd  + '\' > ' + setup_script + ' 2>&1 \n '
        #result = oai_list[index].send_recv(cmd, False, 300 )
        write_file(setup_script, cmd, mode="w")
        tempThread = oaiThread(index, 'thread_setup_'+str(index)+'_' + MachineList[index] , MachineList[index] , user, pw, cmd, False, 3000)
        threads_init_setup.append(tempThread )
        tempThread.start()
        index = index + 1
    except Exception, e:
           print 'There is error in one of the commands to setup the machine '+ MachineList[index] 
           error=''
           error = error + ' In function: ' + sys._getframe().f_code.co_name + ': *** Caught exception: '  + str(e.__class__) + " : " + str( e)
           error = error + traceback.format_exc()
           print error
           sys.exit(1)

  #Now we wait for all the threads to complete
  index = 0
  for t in threads_init_setup:
    t.join()

    port = 22
    paramList=[]
    sftp_log = os.path.expandvars(locallogdir + '/sftp_module.log')

    #Now we copy patch files and apply them
    print( Fore.WHITE + "  - Installating patch files on machine " + MachineList[index])
    for patchFile in OAI5GpatchFileList:
      localfile = os.path.expandvars('$OPENAIR_DIR/cmake_targets/autotests/patches/')+patchFile.get('name')
      remotefile = logdirOAI5GRepo + '/cmake_targets/autotests/patches/'+patchFile.get('name')
      if patchFile.get('machine') == MachineList[index] or patchFile.get('machine') == None:     
        if os.path.isfile(localfile):
          print( Fore.WHITE + "\t> PATCH FILE :"+localfile)
          paramList=[]
          paramList.append ( {"operation":'put', "localfile":localfile, "remotefile":remotefile} )
          sftp_module (user, pw, MachineList[index], port, paramList, sftp_log)
          cmd =  '  cd ' + logdirOAI5GRepo + ' ;git apply cmake_targets/autotests/patches/'+patchFile.get('name')
          res = oai_list[index].send_recv(cmd)
          #print res
     
    paramList=[]

    setuplogfile  = logdir  + '/setup_log_' + MachineList[index] + '_.txt'
    setup_script  = locallogdir  + '/setup_script_' + MachineList[index] +  '_.txt'
    localfile = locallogdir + '/setup_log_' + MachineList[index] + '_.txt'
    remotefile = logdir  + '/setup_log_' + MachineList[index] + '_.txt'
    sftp_log = os.path.expandvars(locallogdir + '/sftp_module.log')
    paramList.append ( {"operation":'get', "localfile":localfile, "remotefile":remotefile} )
    #sftp_module (user, pw, MachineList[index], port, localfile, remotefile, sftp_log, "get")

    #Now we copy test_case_list.xml on the remote machines
    localfile = os.path.expandvars('$OPENAIR_DIR/cmake_targets/autotests/test_case_list.xml')
    remotefile = logdirOAI5GRepo + '/cmake_targets/autotests/test_case_list.xml'
    # paramList.append ( {"operation":'put', "localfile":localfile, "remotefile":remotefile} )
    #     sftp_module (user, pw, MachineList[index], port, paramList, sftp_log)

    sftp_module (user, pw, MachineList[index], port, paramList, sftp_log)


    cmd =  '  cd ' + logdirOAI5GRepo + ' ; source oaienv ; env|grep OPENAIR \n'
    res = oai_list[index].send_recv(cmd)
    index  = index +1
    if os.path.exists(localfile) == 0:
      print "Setup log file <" + localfile + "> missing for machine <" + MachineList[index] + ">.  Please check the setup log files. Exiting now"
      sys.exit(1)

  #Now we process all the test cases
  #Now we check if there was error in setup files

  status, out = commands.getstatusoutput('grep ' +  ' -il \'error\' ' + locallogdir + '/setup_log*')
  if (out != '') :
    print "There is error in setup of machines"
    print "status  = " + str(status) + "\n Check files for error = " + out
    print "Exiting now..."
    sys.exit(1)

  cleanOldProgramsAllMachines(oai_list, CleanUpOldProgs, CleanUpAluLteBox, ExmimoRfStop, '$HOME' , logdirOAI5GRepo)
  if cleanUpRemoteMachines == True:
    print "Error while cleaning Remote machines"
    print "Exiting now..."
    sys.exit(0)


else:
  print (Fore.RED + "  Skipping Machine preparation...")

# **************************************************************************
# **************************************************************************
#           STEP 2 - TEST SELECTION AND PARAMETRIZATION
# **************************************************************************
# **************************************************************************

print (Fore.YELLOW + '\nStep 2 - TEST SELECTION')
print (Fore.YELLOW + '-------------------------------------------------')

print (Fore.WHITE + "  Parsing test case list for test selection...")
print (Fore.WHITE + "  - TestCaseExclusionList      : "+TestCaseExclusionList)
print (Fore.WHITE + "  - testcasegroup              : "+testcasegroup)


testcaseList=xmlRoot.findall('testCase')

nb_total_testcases              = 0
nb_total_testcases_softmodem    = 0
nb_total_testcases_softmodem_noS1 = 0
nb_total_testcases_compilation  = 0
nb_total_testcases_execution    = 0

nb_run_testcases                = 0
nb_run_testcases_softmodem      = 0
nb_run_testcases_softmodem_noS1 = 0
nb_run_testcases_compilation    = 0
nb_run_testcases_execution      = 0


for testcase in testcaseList:
  try:
    testcasename = testcase.get('id')
    testcaseclass = testcase.findtext('class',default='')
    desc = testcase.findtext('desc',default='')
    nb_total_testcases+=1

    run_flag = search_test_case_group(testcasename, testcasegroup, TestCaseExclusionList)
    if (run_flag):
      nb_run_testcases+=1

    if testcaseclass == 'lte-softmodem' :
      nb_total_testcases_softmodem+=1
      if (run_flag): nb_run_testcases_softmodem+=1
    elif testcaseclass == 'lte-softmodem-noS1':  
      nb_total_testcases_softmodem_noS1+=1
      if (run_flag): nb_run_testcases_softmodem_noS1+=1
    elif testcaseclass == 'compilation':
      nb_total_testcases_compilation+=1 
      if (run_flag): nb_run_testcases_compilation+=1
    elif testcaseclass == 'execution': 
      nb_total_testcases_execution+=1
      if (run_flag): nb_run_testcases_execution+=1
    else :
      print "Unknown test case class: " + testcaseclass
      sys.exit()

  except Exception, e:
     error=''
     error = error + ' In function: ' + sys._getframe().f_code.co_name + ': *** Caught exception: '  + str(e.__class__) + " : " + str( e)
     error = error + '\n testcasename = ' + testcasename + '\n testcaseclass = ' + testcaseclass + '\n desc = ' + 'desc' + '\n'  
     error = error + traceback.format_exc()
     print error
     print "Continuing to next test case..."

print (Fore.WHITE + "  Parsing done...")

print (Fore.WHITE + "  - Total number of test cases        : "+str(nb_total_testcases))
print (Fore.WHITE + "  - Total number class softmodem      : "+str(nb_total_testcases_softmodem))
print (Fore.WHITE + "  - Total number class softmodem-noS1 : "+str(nb_total_testcases_softmodem_noS1))
print (Fore.WHITE + "  - Total number class compilation    : "+str(nb_total_testcases_compilation))
print (Fore.WHITE + "  - Total number class execution      : "+str(nb_total_testcases_execution))
print
print (Fore.WHITE + "  - Selected number of test cases     : "+str(nb_run_testcases))
print (Fore.WHITE + "  - Selected class softmodem          : "+str(nb_run_testcases_softmodem))
print (Fore.WHITE + "  - Selected class softmodem_noS1     : "+str(nb_run_testcases_softmodem_noS1))
print (Fore.WHITE + "  - Selected class compilation        : "+str(nb_run_testcases_compilation))
print (Fore.WHITE + "  - Selected class execution          : "+str(nb_run_testcases_execution))


# **************************************************************************
# **************************************************************************
#           STEP 3 - TEST CAMPAIGNS
# **************************************************************************
# **************************************************************************

test_results = []


print (Fore.YELLOW + '\nStep 3 - TEST CAMPAIGNS')
print (Fore.YELLOW + '-------------------------------------------------')

print (Fore.WHITE + "  Start test campaign...")

threadListGlobal=[]
run_count = 0
for testcase in testcaseList:
  try:
    testcasename = testcase.get('id')
    testcaseclass = testcase.findtext('class',default='')
    desc = testcase.findtext('desc',default='')
    #print "Machine list top level = " + ','.join(MachineList)

    if search_test_case_group(testcasename, testcasegroup, TestCaseExclusionList) == True:
      run_count+=1
      print (Fore.WHITE + "  ("+str(run_count).zfill(3)+"/"+str(nb_run_testcases).zfill(3)+") - test case "+testcasename+" : "),
      if testcaseclass == 'lte-softmodem' :
        #First we wait for all the test cases in generic test case class to finish as they are running in parallel
        threadListGlobal = wait_testcaseclass_generic_threads(threadListGlobal, Timeout_execution)
        eNBMachine = testcase.findtext('eNB',default='')
        UEMachine = testcase.findtext('UE',default='')
        EPCMachine = testcase.findtext('EPC',default='')
        #index_eNBMachine = MachineList.index(eNBMachine)
        #index_UEMachine = MachineList.index(UEMachine)
        #index_EPCMachine = MachineList.index(EPCMachine)
        if (eNBMachine not in MachineList)|(UEMachine not in MachineList)|(UEMachine not in MachineList):
           print "One of the machines is not in the machine list"
           print "eNBMachine : " + eNBMachine + "UEMachine : " + UEMachine + "EPCMachine : " + EPCMachine + "MachineList : " + ','.join(MachineList)
        print "testcasename = " + testcasename + " class = " + testcaseclass
        #cleanOldProgramsAllMachines(oai_list, CleanUpOldProgs, CleanUpAluLteBox, ExmimoRfStop)
        handle_testcaseclass_softmodem (testcase, CleanUpOldProgs, logdirOAI5GRepo, logdirOpenaircnRepo, MachineList, user, pw, CleanUpAluLteBox, ExmimoRfStop, nruns_lte_softmodem, GitOAI5GRepoBranch,  GitOpenaircnRepoBranch, Timeout_cmd )
        
        #The lines below are copied from below to trace the failure of some of the machines in test setup. These lines below need to be removed in long term
        print "Creating xml file for overall results..."
        cmd = "cat $OPENAIR_DIR/cmake_targets/autotests/log/*/*.xml > $OPENAIR_DIR/cmake_targets/autotests/log/results_autotests.xml "
        res=os.system(cmd)
        os.system('sync')
        print "Now copying files to NFS Share"
        oai_localhost = openair('localdomain','localhost')
        oai_localhost.connect(user,pw)
        cmd = ' mkdir -p ' + NFSTestsResultsDir
        res = oai_localhost.send_recv(cmd)

        print "Copying files from GilabCI Runner Machine : " + host + " .locallogdir = " + locallogdir + ", NFSTestsResultsDir = " + NFSTestsResultsDir
        SSHSessionWrapper('localhost', user, None, pw , NFSTestsResultsDir , locallogdir, "put_all")
        oai_localhost.disconnect()

      elif testcaseclass == 'lte-softmodem-noS1' :
        eNBMachine = testcase.findtext('eNB',default='')
        UEMachine = testcase.findtext('UE',default='')
        if (eNBMachine not in MachineList)|(UEMachine not in MachineList):
           print "One of the machines is not in the machine list"
           print "eNBMachine : " + eNBMachine + "UEMachine : " + UEMachine + "MachineList : " + ','.join(MachineList)
        print "testcasename = " + testcasename + " class = " + testcaseclass
        threadListGlobal = wait_testcaseclass_generic_threads(threadListGlobal, Timeout_execution)
        
        handle_testcaseclass_softmodem_noS1 (testcase, CleanUpOldProgs, logdirOAI5GRepo, logdirOpenaircnRepo, MachineList, user, pw, CleanUpAluLteBox, ExmimoRfStop, nruns_lte_softmodem, Timeout_cmd ) 

        #The lines below are copied from below to trace the failure of some of the machines in test setup. These lines below need to be removed in long term
        print "Creating xml file for overall results..."
        cmd = "cat $OPENAIR_DIR/cmake_targets/autotests/log/*/*.xml > $OPENAIR_DIR/cmake_targets/autotests/log/results_autotests.xml "
        res=os.system(cmd)
        os.system('sync')

      elif (testcaseclass == 'compilation'): 
        threadListGlobal = handle_testcaseclass_generic (testcasename, threadListGlobal, CleanUpOldProgs, logdirOAI5GRepo, MachineListGeneric, user, pw, CleanUpAluLteBox,Timeout_execution, ExmimoRfStop)
      elif (testcaseclass == 'execution'): 
        threadListGlobal = handle_testcaseclass_generic (testcasename, threadListGlobal, CleanUpOldProgs, logdirOAI5GRepo, MachineListGeneric, user, pw, CleanUpAluLteBox, Timeout_execution, ExmimoRfStop)
      else :
        print "Unknown test case class: " + testcaseclass
        sys.exit()

  except Exception, e:
     error=''
     error = error + ' In function: ' + sys._getframe().f_code.co_name + ': *** Caught exception: '  + str(e.__class__) + " : " + str( e)
     error = error + '\n testcasename = ' + testcasename + '\n testcaseclass = ' + testcaseclass + '\n desc = ' + 'desc' + '\n'  
     error = error + traceback.format_exc()
     print error
     print "Continuing to next test case..."
     #sys.exit(1)


print "Exiting the test cases execution now. Waiting for existing threads to complete..."

for param in threadListGlobal:
   thread_id = param["thread_id"]
   thread_id.join()


# **************************************************************************
# **************************************************************************
#           STEP 4 - TEST REPORT PRODUCTION
# **************************************************************************
# **************************************************************************



test_session_stop_time = datetime.datetime.now()

print (Fore.YELLOW + '\nStep 4 - TEST REPORT PRODUCTION')
print (Fore.YELLOW + '-------------------------------------------------')

# XML test report
# --------------------------------------------------------------------
print "Creating xml file for overall results..."
cmd = "cat $OPENAIR_DIR/cmake_targets/autotests/log/*/*.xml > $OPENAIR_DIR/cmake_targets/autotests/log/results_autotests.xml "
res=os.system(cmd)

print "Now copying files to NFS Share"
oai_localhost = openair('localdomain','localhost')
oai_localhost.connect(user,pw)
cmd = 'mkdir -p ' + NFSTestsResultsDir
res = oai_localhost.send_recv(cmd)

print "Copying files from GilabCI Runner Machine : " + host + " .locallogdir = " + locallogdir + ", NFSTestsResultsDir = " + NFSTestsResultsDir
SSHSessionWrapper('localhost', user, None, pw , NFSTestsResultsDir , locallogdir, "put_all")

cmd = "cat " + NFSTestsResultsDir + "/log/*/*.xml > " + NFSTestsResultsDir + "/log/results_autotests.xml"
res = oai_localhost.send_recv(cmd)
 
oai_localhost.disconnect()


# HTML test report
# --------------------------------------------------------------------
if flag_generate_html_report:
  
  print "Creating html test report..."

  report_dir = reportdir + '/'+ test_session_start_time.strftime("%Y-%m-%d_%H-%M")+"_"+host

  cmd = 'mkdir -p ' + report_dir
  result = os.system(cmd)

  cmd = 'cp -r '+locallogdir+' '+report_dir
  result = os.system(cmd)

  context = {
      'report_path' : report_dir,
      'test_session_start_time' : test_session_start_time,
      'test_session_stop_time'  : test_session_stop_time,
      'test_session_duration'   : test_session_stop_time-test_session_start_time,    
      'mtc_host'                : host,
      'user'                    : user,
      'password'                : pw,
      'test_results'            : test_results,  
  }

  for test_result in test_results:
    cmd = 'mkdir -p ' + report_dir + '/'+ test_result['testcase_name']
    result = os.system(cmd)

    report_file = report_dir + '/'+ test_result['testcase_name'] + '/'+ test_result['testcase_name']+ '_report.html'

    analyser.create_test_report_detailed_html(test_result, report_file )

   # print test_result

  analyser.create_report_html(context)


sys.exit()
