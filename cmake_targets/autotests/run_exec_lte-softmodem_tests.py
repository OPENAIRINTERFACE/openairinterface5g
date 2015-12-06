#! /usr/bin/python
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

# \file test01.py
# \brief test 01 for OAI
# \author Navid Nikaein
# \date 2013 - 2015
# \version 0.1
# @ingroup _test

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

import numpy as np

import log
import case01
import case02
import case03
import case04
import case05

from  openair import *

import paramiko

import subprocess
import commands
sys.path.append('/opt/ssh')

import ssh
from ssh import SSHSession

def write_file(filename, string, mode="w"):
   text_file = open(filename, mode)
   text_file.write(string)
   text_file.close()
   
#$1 name of file (assuming created with iperf -s -u ....
#$2 minimum throughput
#$3 maximum throughput
#$4 average throughput
#$5 minimum duration of throughput
#The throughput values found in file must be higher than values from from 2,3,4,5
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
      
      if (min_list >= min_tput and  max_list >= max_tput and average_list >= average and duration >= min_duration):
        return True
      else:
        return False
   else: 
      return False

    
def try_convert_to_float(string, fail=None):
    try:
        return float(string)
    except Exception:
        return fail;

def tput_test_search_expr (search_expr, logfile_traffic):
   result=0
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
             result = tput_test(logfile_traffic, min_tput, max_tput, avg_tput, duration)
   return result

      

def sftp_module (username, password, hostname, ports, paramList,logfile): 
   #localD = localfile
   #remoteD = remotefile
   #fd, paramiko_logfile  = tempfile.mkstemp()
   #res = os.close(fd )
   #paramiko logfile path should not be changed with multiple calls. The logs seem to in first file regardless
   error = ""
   #The lines below are outside exception loop to be sure to terminate the test case if the network connectivity goes down or there is authentication failure
 


   transport = paramiko.Transport(hostname, ports)
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

def finalize_deploy_script (timeout_cmd, terminate_missing_procs='True'):
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

def update_config_file(oai, config_string, logdirRepo, python_script):
  if config_string :
    stringArray = config_string.splitlines()
    cmd=""
    #python_script = '$OPENAIR_DIR/targets/autotests/tools/search_repl.py'
    for string in stringArray:
       #split the string based on space now
       string1=string.split()
       cmd = cmd + 'python ' + python_script + ' ' + logdirRepo+'/'+string1[0] + '  ' + string1[1] +  ' '+ string1[2] + '\n'
       #cmd = cmd + 'perl -p -i  -e \'s/'+ string1[1] + '\\s*=\\s*"\\S*"\\s*/' + string1[1] + ' = "' + string1[2] +'"' + '/g\'   ' + logdirRepo + '/' +string1[0] + '\n'
    return cmd
    #result = oai.send_recv(cmd)


 
#Function to clean old programs that might be running from earlier execution
#oai - parameter for making connection to machine
#programList - list of programs that must be terminated before execution of any test case 
def cleanOldPrograms(oai, programList, CleanUpAluLteBox):
  cmd = 'killall -q -r ' + programList
  result = oai.send(cmd, True)
  print "Killing old programs..." + result
  programArray = programList.split()
  programListJoin = '|'.join(programArray)
  cmd = cleanupOldProgramsScript + ' ' + '\''+programListJoin+'\''
  #result = oai.send_recv(cmd)
  #print result
  result = oai.send_expect_false(cmd, 'Match found', False)
  print result
  res=oai.send_recv(CleanUpAluLteBox, True)


class myThread (threading.Thread):
    def __init__(self, threadID, name, counter):
        threading.Thread.__init__(self)
        self.threadID = threadID
        self.name = name
        self.counter = counter
    def run(self):
        print "Starting " + self.name


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
          oai.connect(user, self.password)
          print "Starting " + self.threadname + " on machine " + self.machine
          result = oai.send_recv(self.cmd, self.sudo, self.timeout)
          print "result = " + result
          print "Exiting " + self.threadname
          oai.disconnect()
        except Exception, e:
           error=''
           error = error + ' In class oaiThread, function: ' + sys._getframe().f_code.co_name + ': *** Caught exception: '  + str(e.__class__) + " : " + str( e)
           error = error + '\n threadID = ' + str(self.threadID) + '\n threadname = ' + self.threadname + '\n timeout = ' + self.timeout + '\n machine = ' + self.machine + '\n cmd = ' + self.cmd + '\n timeout = ' + str(self.timeout) +  '\n'  
           error = error + traceback.format_exc()
           print error


#This class runs test cases with class execution, compilatation
class testCaseThread_generic (threading.Thread):
   def __init__(self, threadID, name, machine, logdirOAI5GRepo, testcasename,oldprogramList, CleanupAluLteBox, password, timeout):
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
   def run(self):
     try:
       mypassword=''
       #addsudo = 'echo \'' + mypassword + '\' | sudo -S -E '
       addpass = 'echo \'' + mypassword + '\' | '
       user = getpass.getuser()
       print "Starting test case : " + self.testcasename + " On machine " + self.machine + " timeout = " + str(self.timeout) 
       oai = openair('localdomain',self.machine)
       oai.connect(user, self.password)
       cleanOldPrograms(oai, self.oldprogramList, self.CleanupAluLteBox)
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
       ssh = SSHSession(self.machine , username=user, key_file=None, password=self.password)
       ssh.get_all(logdir_remote_testcase , logdir_local_base)
       print "Finishing test case : " + self.testcasename + " On machine " + self.machine
       cleanOldPrograms(oai, self.oldprogramList, self.CleanupAluLteBox)
       #oai.kill(user,mypassword)
       oai.disconnect()
     except Exception, e:
         error=''
         error = error + ' In Class = testCaseThread_generic,  function: ' + sys._getframe().f_code.co_name + ': *** Caught exception: '  + str(e.__class__) + " : " + str( e)
         error = error + '\n threadID = ' + str(self.threadID) + '\n threadName = ' + self.name + '\n testcasename = ' + self.testcasename + '\n machine = ' + self.machine + '\n logdirOAI5GRepo = ' + self.logdirOAI5GRepo +  '\n' + '\n timeout = ' + str(timeout)  
         error = error + traceback.format_exc()
         print error
         sys.exit()


def addsudo (cmd, password=""):
  cmd = 'echo \'' + password + '\' | sudo -S -E bash -c \' ' + cmd + '\' '
  return cmd

def handle_testcaseclass_generic (testcasename, threadListGeneric, oldprogramList, logdirOAI5GRepo, MachineList, password, CleanupAluLteBox,timeout):
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
    thread = testCaseThread_generic(1,"Generic Thread_"+testcasename+"_"+ "machine_", machine, logdirOAI5GRepo, testcasename, oldprogramList, CleanupAluLteBox, password, timeout)
    param={"thread_id":thread, "Machine":machine, "testcasename":testcasename}
    thread.start()
    threadListNew.append(param)
    return threadListNew
  except Exception, e:
     error=''
     error = error + ' In function: ' + sys._getframe().f_code.co_name + ': *** Caught exception: '  + str(e.__class__) + " : " + str( e)
     error = error + '\n testcasename = ' + testcasename + '\n logdirOAI5GRepo = ' + logdirOAI5GRepo + '\n MachineList = ' + ','.join(MachineList) + '\n timeout = ' + str(timeout) +  '\n'  
     error = error + traceback.format_exc()
     print error
     sys.exit(1)

#Blocking wait for all threads related to generic testcase execution, class (compilation and execution)
def wait_testcaseclass_generic_threads(threadListGeneric, timeout = 1):
   for param in threadListGeneric:
      thread_id = param["thread_id"]
      machine = param["Machine"]
      testcasenameold = param["testcasename"]
      thread_id.join(timeout)
      if thread_id.isAlive() == True:
         print "thread_id on machine: " + machine + "  is still alive: testcasename: " + testcasenameold
         print " Exiting now..."
         sys.exit(1)
      else:
         print "thread_id on machine: " + machine + "  is stopped: testcasename: " + testcasenameold
         threadListGeneric.remove(param)
   return threadListGeneric

#Function to handle test case class : lte-softmodem
def handle_testcaseclass_softmodem (testcase, oldprogramList, logdirOAI5GRepo , logdirOpenaircnRepo, MachineList, password, CleanUpAluLteBox):
  #We ignore the password sent to this function for secuirity reasons for password present in log files
  #It is recommended to add a line in /etc/sudoers that looks something like below. The line below will run sudo without password prompt
  # your_user_name ALL=(ALL:ALL) NOPASSWD: ALL
  mypassword=''
  #addsudo = 'echo \'' + mypassword + '\' | sudo -S -E '
  addpass = 'echo \'' + mypassword + '\' | '
  user = getpass.getuser()
  testcasename = testcase.get('id')
  testcaseclass = testcase.findtext('class',default='')
  timeout_cmd = testcase.findtext('TimeOut_cmd',default='')
  timeout_cmd = int(float(timeout_cmd))
  #Timeout_thread is more than that of cmd to have room for compilation time, etc
  timeout_thread = timeout_cmd + 300 
  nruns = testcase.findtext('nruns',default='')
  nruns = int(float(nruns))
  tags = testcase.findtext('tags',default='')
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
  eNB_terminate_missing_procs = testcase.findtext('eNB_terminate_missing_procs',default='True')
  eNB_search_expr_true = testcase.findtext('eNB_search_expr_true','')

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
  UE_terminate_missing_procs = testcase.findtext('UE_terminate_missing_procs',default='True')
  UE_search_expr_true = testcase.findtext('UE_search_expr_true','')

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
  EPC_terminate_missing_procs = testcase.findtext('EPC_terminate_missing_procs',default='True')
  EPC_search_expr_true = testcase.findtext('EPC_search_expr_true','')

  index_eNBMachine = MachineList.index(eNBMachine)
  index_UEMachine = MachineList.index(UEMachine)
  index_EPCMachine = MachineList.index(EPCMachine)
  oai_eNB = openair('localdomain', eNBMachine)
  oai_eNB.connect(user, password)
  oai_UE = openair('localdomain', UEMachine)
  oai_UE.connect(user, password)
  oai_EPC = openair('localdomain', EPCMachine)
  oai_EPC.connect(user, password)

  cleanOldPrograms(oai_eNB, oldprogramList, CleanUpAluLteBox)
  cleanOldPrograms(oai_UE, oldprogramList, CleanUpAluLteBox)
  cleanOldPrograms(oai_EPC, oldprogramList, CleanUpAluLteBox)
  logdir_eNB = logdirOAI5GRepo+'/cmake_targets/autotests/log/'+ testcasename
  logdir_UE =  logdirOAI5GRepo+'/cmake_targets/autotests/log/'+ testcasename
  logdir_EPC = logdirOpenaircnRepo+'/TEST/autotests/log/'+ testcasename
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
    logdir_UE =  logdirOAI5GRepo+'/cmake_targets/autotests/log/'+ testcasename + '/run_' + str(run)
    logdir_EPC = logdirOpenaircnRepo+'/TEST/autotests/log/'+ testcasename + '/run_' + str(run)
    logdir_local_testcase = logdir_local + '/cmake_targets/autotests/log/'+ testcasename + '/run_' + str(run)
    #Make the log directory of test case
    cmd = 'rm -fr ' + logdir_eNB + ' ; mkdir -p ' + logdir_eNB
    result = oai_eNB.send_recv(cmd)
    cmd = 'rm -fr ' + logdir_UE + ' ; mkdir -p ' +  logdir_UE
    result = oai_UE.send_recv(cmd)
    cmd = 'rm -fr ' + logdir_EPC + '; mkdir -p ' + logdir_EPC
    result = oai_EPC.send_recv(cmd)
    cmd = ' rm -fr ' + logdir_local_testcase + ' ; mkdir -p ' + logdir_local_testcase
    result = os.system(cmd)
    
    logfile_compile_eNB = logdir_eNB + '/eNB_compile' + '_' + str(run) + '_.log'
    logfile_exec_eNB = logdir_eNB + '/eNB_exec' + '_' + str(run) + '_.log'
    logfile_pre_exec_eNB = logdir_eNB + '/eNB_pre_exec' + '_' + str(run) + '_.log'
    logfile_traffic_eNB = logdir_eNB + '/eNB_traffic' + '_' + str(run) + '_.log'
    logfile_task_eNB_compile_out = logdir_eNB + '/eNB_task_compile_out' + '_' + str(run) + '_.log'
    logfile_task_eNB_compile = logdir_local_testcase + '/eNB_task_compile' + '_' + str(run) + '_.log'
    logfile_task_eNB_out = logdir_eNB + '/eNB_task_out' + '_' + str(run) + '_.log'
    logfile_task_eNB = logdir_local_testcase + '/eNB_task' + '_' + str(run) + '_.log'

    task_eNB_compile = ' ( uname -a ; date \n'
    task_eNB_compile = task_eNB_compile + 'cd ' + logdirOAI5GRepo + ' ; source oaienv ; source cmake_targets/tools/build_helper \n'
    task_eNB_compile = task_eNB_compile + 'env |grep OPENAIR  \n'
    task_eNB_compile = task_eNB_compile + update_config_file(oai_eNB, eNB_config_file, logdirOAI5GRepo, '$OPENAIR_DIR/cmake_targets/autotests/tools/search_repl.py') + '\n'
    if eNB_compile_prog != "":
       task_eNB_compile  = task_eNB_compile +  ' ( ' + eNB_compile_prog + ' '+ eNB_compile_prog_args + ' ) > ' + logfile_compile_eNB + ' 2>&1 \n'
    task_eNB_compile =  task_eNB_compile + ' date ) > ' + logfile_task_eNB_compile_out + ' 2>&1  '
    write_file(logfile_task_eNB_compile, task_eNB_compile, mode="w")

    task_eNB = ' ( uname -a ; date \n'
    task_eNB = task_eNB + 'cd ' + logdirOAI5GRepo + ' ; source oaienv ; source cmake_targets/tools/build_helper \n'
    task_eNB = task_eNB + 'env |grep OPENAIR  \n' + 'array_exec_pid=() \n'

    if eNB_pre_exec != "":
       task_eNB  = task_eNB +  ' ( ' + eNB_pre_exec + ' '+ eNB_pre_exec_args + ' ) > ' + logfile_pre_exec_eNB + ' 2>&1 \n'
    if eNB_main_exec != "":
       task_eNB = task_eNB + ' ( ' + addsudo(eNB_main_exec + ' ' + eNB_main_exec_args, mypassword) + ' ) > ' + logfile_exec_eNB + ' 2>&1 & \n'
       task_eNB = task_eNB + 'array_exec_pid+=($!) \n'
       task_eNB = task_eNB + 'echo eNB_main_exec PID = $! \n'
    if eNB_traffic_exec != "":
       task_eNB = task_eNB + ' ( ' + eNB_traffic_exec + ' ' + eNB_traffic_exec_args + ' ) > ' + logfile_traffic_eNB + ' 2>&1 & \n '
       task_eNB = task_eNB + 'array_exec_pid+=($!) \n'
       task_eNB = task_eNB + 'echo eNB_traffic_exec PID = $! \n'
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

    task_UE_compile = ' ( uname -a ; date \n'
    task_UE_compile = task_UE_compile + 'array_exec_pid=()' + '\n'
    task_UE_compile = task_UE_compile + 'cd ' + logdirOAI5GRepo + '\n'  
    task_UE_compile = task_UE_compile + 'source oaienv \n'
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
       task_UE  = task_UE +  ' ( ' + UE_pre_exec + ' '+ UE_pre_exec_args + ' ) > ' + logfile_pre_exec_UE + ' 2>&1 \n'
    if UE_main_exec != "":
       task_UE = task_UE + ' ( ' + addsudo(UE_main_exec + ' ' + UE_main_exec_args, mypassword)  + ' ) > ' + logfile_exec_UE + ' 2>&1 & \n'
       task_UE = task_UE + 'array_exec_pid+=($!) \n'
       task_UE = task_UE + 'echo UE_main_exec PID = $! \n'
    if UE_traffic_exec != "":
       task_UE = task_UE + ' ( ' + UE_traffic_exec + ' ' + UE_traffic_exec_args + ' ) >' + logfile_traffic_UE + ' 2>&1 & \n'
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

    task_EPC_compile = ' ( uname -a ; date \n'
    task_EPC_compile = task_EPC_compile + 'array_exec_pid=()' + '\n'
    task_EPC_compile = task_EPC_compile + 'cd ' + logdirOpenaircnRepo + '\n'
    task_EPC_compile = task_EPC_compile + update_config_file(oai_EPC, EPC_config_file, logdirOpenaircnRepo, logdirOpenaircnRepo+'/TEST/autotests/tools/search_repl.py') + '\n'
    task_EPC_compile = task_EPC_compile +  'source BUILD/TOOLS/build_helper \n'
    if EPC_compile_prog != "":
       task_EPC_compile = task_EPC_compile + '(' + EPC_compile_prog + ' ' + EPC_compile_prog_args +  ' ) > ' + logfile_compile_EPC + ' 2>&1 \n'
    if HSS_compile_prog != "":
       task_EPC_compile = task_EPC_compile + '(' + HSS_compile_prog + ' ' + HSS_compile_prog_args + ' ) > ' + logfile_compile_HSS + ' 2>&1 \n'
    task_EPC_compile  = task_EPC_compile + ' ) > ' + logfile_task_EPC_compile_out + ' 2>&1 ' 
    write_file(logfile_task_EPC_compile, task_EPC_compile, mode="w")
    
    task_EPC = ' ( uname -a ; date \n'
    task_EPC = task_EPC + 'array_exec_pid=()' + '\n'
    task_EPC = task_EPC + 'cd ' + logdirOpenaircnRepo + '\n'
    task_EPC = task_EPC +  'source BUILD/TOOLS/build_helper \n'
    if EPC_pre_exec != "":
       task_EPC  = task_EPC +  ' ( ' + EPC_pre_exec + ' '+ EPC_pre_exec_args + ' ) > ' + logfile_pre_exec_EPC + ' 2>&1 \n'
    if HSS_main_exec !=  "":
       task_EPC  = task_EPC + '(' + addsudo (HSS_main_exec + ' ' + HSS_main_exec_args, mypassword) + ' ) > ' + logfile_exec_HSS  +  ' 2>&1   & \n'
       task_EPC = task_EPC + 'array_exec_pid+=($!) \n'
       task_EPC = task_EPC + 'echo HSS_main_exec PID = $! \n'
    if EPC_main_exec !=  "":
       task_EPC  = task_EPC + '(' + addsudo (EPC_main_exec + ' ' + EPC_main_exec_args, mypassword) + ' ) > ' + logfile_exec_EPC  +  ' 2>&1   & \n'
       task_EPC = task_EPC + 'array_exec_pid+=($!) \n'
       task_EPC = task_EPC + 'echo EPC_main_exec PID = $! \n'
    if EPC_traffic_exec !=  "":
       task_EPC  = task_EPC + '(' + EPC_traffic_exec + ' ' + EPC_traffic_exec_args + ' ) > ' + logfile_traffic_EPC  +  ' 2>&1   & \n' 
       task_EPC = task_EPC + 'array_exec_pid+=($!) \n'  
       task_EPC = task_EPC + 'echo EPC_traffic_exec PID = $! \n'
    #terminate the EPC test case after timeout_cmd seconds   
    task_EPC = task_EPC + finalize_deploy_script (timeout_cmd, EPC_terminate_missing_procs) + '\n'
    #task_EPC  = task_EPC + 'sleep ' +  str(timeout_cmd) + '\n'
    task_EPC  = task_EPC + 'handle_ctrl_c' '\n' 
    task_EPC  = task_EPC + ' ) > ' + logfile_task_EPC_out + ' 2>&1 ' 
    write_file(logfile_task_EPC, task_EPC, mode="w")
    
    #first we compile all the programs
    thread_EPC = oaiThread(1, "EPC_thread", EPCMachine, user, password , task_EPC_compile, False, timeout_thread)
    thread_eNB = oaiThread(2, "eNB_thread", eNBMachine, user, password , task_eNB_compile, False, timeout_thread)
    thread_UE = oaiThread(3, "UE_thread", UEMachine, user, password  , task_UE_compile, False, timeout_thread) 
    threads=[]
    threads.append(thread_eNB)
    threads.append(thread_UE)
    threads.append(thread_EPC)
    # Start new Threads
    thread_eNB.start()
    thread_UE.start()
    thread_EPC.start()
    #Wait for all the compile threads to complete
    for t in threads:
       t.join()

    #Now we execute all the threads
    thread_EPC = oaiThread(1, "EPC_thread", EPCMachine, user, password , task_EPC, False, timeout_thread)
    thread_eNB = oaiThread(2, "eNB_thread", eNBMachine, user, password , task_eNB, False, timeout_thread)
    thread_UE = oaiThread(3, "UE_thread", UEMachine, user, password  , task_UE, False, timeout_thread) 

    threads=[]
    threads.append(thread_eNB)
    threads.append(thread_UE)
    threads.append(thread_EPC)
    # Start new Threads

    thread_eNB.start()
    thread_UE.start()
    thread_EPC.start()

    #Wait for all the compile threads to complete
    for t in threads:
       t.join()
    #Now we get the log files from remote machines on the local machine

    cleanOldPrograms(oai_eNB, oldprogramList, CleanUpAluLteBox)
    cleanOldPrograms(oai_UE, oldprogramList, CleanUpAluLteBox)
    cleanOldPrograms(oai_EPC, oldprogramList, CleanUpAluLteBox)

    print "Copying files from EPCMachine : " + EPCMachine + "logdir_EPC = " + logdir_EPC
    ssh = SSHSession(EPCMachine , username=user, key_file=None, password=password)
    ssh.get_all(logdir_EPC , logdir_local + '/cmake_targets/autotests/log/'+ testcasename)

    print "Copying files from eNBMachine " + eNBMachine + "logdir_eNB = " + logdir_eNB
    ssh = SSHSession(eNBMachine , username=user, key_file=None, password=password)
    ssh.get_all(logdir_eNB, logdir_local + '/cmake_targets/autotests/log/'+ testcasename)

    print "Copying files from UEMachine : " + UEMachine + "logdir_UE = " + logdir_UE
    ssh = SSHSession(UEMachine , username=user, key_file=None, password=password)
    ssh.get_all(logdir_UE , logdir_local + '/cmake_targets/autotests/log/'+ testcasename)
    
    #Currently we only perform throughput tests
    result = tput_test_search_expr(eNB_search_expr_true, logfile_traffic_eNB)
    run_result=run_result&result
    result = tput_test_search_expr(EPC_search_expr_true, logfile_traffic_EPC)
    run_result=run_result&result
    result = tput_test_search_expr(UE_search_expr_true, logfile_traffic_UE)
    run_result=run_result&result
    
    if run_result == 1:  
      run_result_string = 'RUN_'+str(run) + ' = PASS'
    else:
      run_result_string = 'RUN_'+str(run) + ' = FAIL'

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
  xmlFile = logdir_local + '/cmake_targets/autotests/log/'+ testcasename + 'test.' + testcasename + '.xml'
  if test_result ==0: 
    result='FAIL'
  else:
    result = 'PASS'
  xml="<testcase classname=\'"+ testcaseclass +  "\' name=\'" + testcasename + "."+tags +  "\' Run_result=\'" + test_result_string + "\' time=\'" + duration + "\'s RESULT=\'" +result + "\'></testcase>"
  write_file(xmlFile, xml, mode="w")


#This function searches if test case is present in list of test cases that need to be executed by user
def search_test_case_group(testcasename, testcasegroup):
    if testcasegroup == '':
       return True
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
   
       

#thread1 = myThread(1, "Thread-1", 1)
debug = 0
pw =''
i = 0
dlsim=0
localshell=0
is_compiled = 0
timeout=2000
xmlInputFile="./test_case_list.xml"
NFSResultsDir = '/mnt/sradio'
cleanupOldProgramsScript = '$OPENAIR_DIR/cmake_targets/autotests/tools/remove_old_programs.bash'
testcasegroup=''

logdir = '/tmp/' + 'OAITestFrameWork-' + getpass.getuser() + '/'
logdirOAI5GRepo = logdir + 'openairinterface5g/'
logdirOpenaircnRepo = logdir + 'openair-cn/'

openairdir_local = os.environ.get('OPENAIR_DIR')
if openairdir_local is None:
   print "Environment variable OPENAIR_DIR not set correctly"
   sys.exit()
locallogdir = openairdir_local + '/cmake_targets/autotests/log/'
#Remove  the contents of local log directory
#os.system(' rm -fr ' + locallogdir + '; mkdir -p ' +  locallogdir  )
flag_remove_logdir=False
i=1
while i < len (sys.argv):
    arg=sys.argv[i]
    if arg == '-d':
        debug = 1
    elif arg == '-dd':
        debug = 2
    elif arg == '-p' :
        prompt2 = sys.argv[i+1]
        i = i +1
    elif arg == '-r':
        flag_remove_logdir=True 
    elif arg == '-w' :
        pw = sys.argv[i+1]
        i = i +1  
    elif arg == '-P' :
        dlsim = 1
    elif arg == '-l' :
        localshell = 1
    elif arg == '-c' :
        is_compiled = 1
    elif arg == '-t' :
        timeout = sys.argv[i+1]
        i = i +1  
    elif arg == '-g' :
        testcasegroup = sys.argv[i+1].replace("\"","")
        i = i +1   
    elif arg == '-h' :
        print "-d:  low debug level"
        print "-dd: high debug level"
        print "-p:  set the prompt"
        print "-r:  Remove the log directory in autotests/"
        print "-w:  set the password for ssh to localhost"
        print "-l:  use local shell instead of ssh connection"
        print "-t:  set the time out in second for commands"
        sys.exit()
    else :
        print "Unrecongnized Option: <" + arg + ">. Use -h to see valid options"
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

if flag_remove_logdir == True:
   print "Removing directory: " + locallogdir
   os.system(' rm -fr ' + locallogdir + '; mkdir -p ' +  locallogdir  )



paramiko_logfile = os.path.expandvars('$OPENAIR_DIR/cmake_targets/autotests/log/paramiko.log')
res=os.system(' echo > ' + paramiko_logfile)
paramiko.util.log_to_file(paramiko_logfile)

# get the oai object
host = os.uname()[1]
#oai = openair('localdomain','calisson')
oai_list = {}


#start_time = time.time()  # datetime.datetime.now()
user = getpass.getuser()
print "host = " + host 
print "user = " + user
pw=getpass.getpass()

#Now we parse the xml file for basic configuration
xmlTree = ET.parse(xmlInputFile)
xmlRoot = xmlTree.getroot()




MachineList = xmlRoot.findtext('MachineList',default='')
NFSResultsShare = xmlRoot.findtext('NFSResultsShare',default='')
GitOpenaircnRepo = xmlRoot.findtext('GitOpenair-cnRepo',default='')
GitOAI5GRepo = xmlRoot.findtext('GitOAI5GRepo',default='')
GitOAI5GRepoBranch = xmlRoot.findtext('GitOAI5GRepoBranch',default='')
GitOpenaircnRepoBranch = xmlRoot.findtext('GitOpenair-cnRepoBranch',default='')
CleanUpOldProgs = xmlRoot.findtext('CleanUpOldProgs',default='')
CleanUpAluLteBox = xmlRoot.findtext('CleanUpAluLteBox',default='')
Timeout_execution = int (xmlRoot.findtext('Timeout_execution'))
MachineListGeneric = xmlRoot.findtext('MachineListGeneric',default='')
print "MachineList = " + MachineList
print "GitOpenair-cnRepo = " + GitOpenaircnRepo
print "GitOAI5GRepo = " + GitOAI5GRepo
print "GitOAI5GBranch = " + GitOAI5GRepoBranch
print "GitOpenaircnRepoBranch = " + GitOpenaircnRepoBranch
print "NFSResultsShare = " + NFSResultsShare
cmd = "git show-ref --heads -s "+ GitOAI5GRepoBranch
GitOAI5GHeadVersion = subprocess.check_output ([cmd], shell=True)
print "GitOAI5GHeadVersion = " + GitOAI5GHeadVersion
print "CleanUpOldProgs = " + CleanUpOldProgs
print "Timeout_execution = " + str(Timeout_execution)

MachineList = MachineList.split()
MachineListGeneric = MachineListGeneric.split()

index=0
for machine in MachineList: 
  oai_list[index] = openair('localdomain',machine)
  index = index + 1


#myThread (1,"sddsf", 1)


#thread1 = oaiThread1(1, "Thread-1", 1)
#def __init__(self, threadID, name, counter, oai, cmd, sudo, timeout):

#sys.exit()







print "\nTesting the sanity of machines used for testing..."
if localshell == 0:
    try:
        index=0
        for machine in MachineList:
           print '\n******* Note that the user <'+user+'> should be a sudoer *******\n'
           print '******* Connecting to the machine <'+machine+'> to perform the test *******\n'
           if not pw :
              print "username: " + user 
              #pw = getpass.getpass() 
              #print "password: " + pw            
           else :
              print "username: " + user 
              #print "password: " + pw 
           # issues in ubuntu 12.04
           oai_list[index].connect(user,pw)
           #print "result = " + result
           

           #print '\nCleaning Older running programs : ' + CleanUpOldProgs
           #cleanOldPrograms(oai_list[index], CleanUpOldProgs)



           print '\nChecking for sudo permissions on machine <'+machine+'>...'
           result = oai_list[index].send_expect_false('sudo -S -v','may not run sudo',True)
           print "Sudo permissions..." + result
           
           print '\nCleaning Older running programs : ' + CleanUpOldProgs
           cleanOldPrograms(oai_list[index], CleanUpOldProgs, CleanUpAluLteBox)

           result = oai_list[index].send('mount ' + NFSResultsDir, True)
           print "Mounting NFS Share " + NFSResultsDir + "..." + result

           # Check if NFS share is mounted correctly.
           print 'Checking if NFS Share<' + NFSResultsDir + '> is mounted correctly...'
           #result = oai_list[index].send_expect('mount | grep ' + NFSResultsDir,  NFSResultsDir )
           cmd = 'if grep -qs '+NFSResultsDir+ ' /proc/mounts; then  echo \'' + NFSResultsDir  + ' is mounted\' ; fi'
           search_expr = NFSResultsDir + ' is mounted'
           print "cmd = " + cmd
           print "search_expr = " + search_expr
           result = oai_list[index].send_expect(cmd, search_expr)
           print "Mount NFS_Results_Dir..." + result
           index = index + 1
           
           #oai.connect2(user,pw) 
           #oai.get_shell()
    except :
        print 'Fail to connect to the machine: '+ machine 
        sys.exit(1)
else:
    pw = ''
    oai_list[0].connect_localshell()





cpu_freq = int(oai_list[0].cpu_freq())
if timeout == 2000 : 
    if cpu_freq <= 2000 : 
        timeout = 3000
    elif cpu_freq < 2700 :
        timeout = 2000 
    elif cpu_freq < 3300 :
        timeout = 1500
print "cpu freq(MHz): " + str(cpu_freq) + "timeout(s): " + str(timeout)

# The log files are stored in branch/version/



#result = oai_list[0].send('uname -a ' )
#print result

#We now prepare the machines for testing
#index=0
threads_init_setup=[]
for index in oai_list:
  try:
      print "setting up machine: " + MachineList[index]
      #print oai_list[oai].send_recv('echo \''+pw+'\' |sudo -S -v')
      #print oai_list[oai].send_recv('sudo su')
      #print oai_list[oai].send_recv('who am i') 
      #cleanUpPrograms(oai_list[oai]
      cmd =  'mkdir -p ' + logdir + ' ; rm -fr ' + logdir + '/*'
      result = oai_list[index].send_recv(cmd)
     
      setuplogfile  = logdir  + '/setup_log_' + MachineList[index] + '_.txt'
      setup_script  = locallogdir  + '/setup_script_' + MachineList[index] +  '_.txt'
      cmd = ' ( \n'
      #cmd = cmd  + 'rm -fR ' +  logdir + '\n'
      #cmd = cmd + 'mkdir -p ' + logdir + '\n'
      cmd = cmd + 'cd '+ logdir   + '\n'
      cmd = cmd + 'git clone '+ GitOAI5GRepo  + '\n'
      cmd = cmd + 'git clone '+ GitOpenaircnRepo   + '\n'
      cmd = cmd +  'cd ' + logdirOAI5GRepo  + '\n'
      cmd = cmd + 'git checkout ' + GitOAI5GHeadVersion   + '\n'
      cmd = cmd + 'source oaienv'   + '\n'
      cmd = cmd +  'cd ' + logdirOpenaircnRepo  + '\n'
      cmd = cmd +  'git checkout ' + GitOpenaircnRepoBranch  + '\n'
      cmd = cmd +  'env |grep OPENAIR'  + '\n'
      cmd = cmd + ' cd ' + logdir   + '\n'
      cmd = cmd + ' ) > ' +  setuplogfile + ' 2>&1   '
      #cmd = cmd + 'echo \' ' + cmd  + '\' > ' + setup_script + ' 2>&1 \n '
      #result = oai_list[index].send_recv(cmd, False, 300 )
      write_file(setup_script, cmd, mode="w")
      tempThread = oaiThread(index, 'thread_setup_'+str(index)+'_' + MachineList[index] , MachineList[index] , user, pw, cmd, False, 300)
      threads_init_setup.append(tempThread )
      tempThread.start()

      #localfile = locallogdir + '/setup_log_' + MachineList[index] + '_.txt'
      #remotefile = logdir  + '/setup_log_' + MachineList[index] + '_.txt'

      #sftp_log = os.path.expandvars(locallogdir + '/sftp_module.log')
      #sftp_module (user, pw, MachineList[index], 22, localfile, remotefile, sftp_log, "get")


      #Now we copy test_case_list.xml on the remote machines
      #localfile = os.path.expandvars('$OPENAIR_DIR/cmake_targets/autotests/test_case_list.xml')
      #remotefile = logdirOAI5GRepo + '/cmake_targets/autotests/test_case_list.xml'

      #sftp_log = os.path.expandvars(locallogdir + '/sftp_module.log')
      #sftp_module (user, pw, MachineList[index], 22, localfile, remotefile, sftp_log, "put")


      #print oai_list[index].send('rm -fR ' +  logdir)
      #print oai_list[index].send('mkdir -p ' + logdir)
      #print oai_list[index].send('cd '+ logdir)
      #print oai_list[index].send('git clone '+ GitOAI5GRepo )
      #print oai_list[index].send('git clone '+ GitOpenaircnRepo)
      #print oai_list[index].send('cd ' + logdirOAI5GRepo)
      #print oai_list[index].send('git checkout ' + GitOAI5GHeadVersion)
      #print oai_list[index].send('source oaienv')
      #print oai_list[index].send('cd ' + logdirOpenaircnRepo)
      #print oai_list[index].send('git checkout ' + GitOpenaircnRepoBranch)
      #print oai_list[index].send_recv('cd ' + logdirOAI5GRepo)
      #print oai_list[index].send_recv('source oaienv')
      #print oai_list[index].send_recv('env |grep OPENAIR')

      #print '\nCleaning Older running programs : ' + CleanUpOldProgs
      #cleanOldPrograms(oai_list[index], CleanUpOldProgs)
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
   setuplogfile  = logdir  + '/setup_log_' + MachineList[index] + '_.txt'
   setup_script  = locallogdir  + '/setup_script_' + MachineList[index] +  '_.txt'
   localfile = locallogdir + '/setup_log_' + MachineList[index] + '_.txt'
   remotefile = logdir  + '/setup_log_' + MachineList[index] + '_.txt'
   port = 22
   
   paramList=[]
   sftp_log = os.path.expandvars(locallogdir + '/sftp_module.log')
   paramList.append ( {"operation":'get', "localfile":localfile, "remotefile":remotefile} )
   #sftp_module (user, pw, MachineList[index], port, localfile, remotefile, sftp_log, "get")

   #Now we copy test_case_list.xml on the remote machines
   localfile = os.path.expandvars('$OPENAIR_DIR/cmake_targets/autotests/test_case_list.xml')
   remotefile = logdirOAI5GRepo + '/cmake_targets/autotests/test_case_list.xml'
   paramList.append ( {"operation":'put', "localfile":localfile, "remotefile":remotefile} )
   sftp_log = os.path.expandvars(locallogdir + '/sftp_module.log')
   sftp_module (user, pw, MachineList[index], port, paramList, sftp_log)
   index = index+1

#Now we process all the test cases
#Now we check if there was error in setup files

status, out = commands.getstatusoutput('grep ' +  ' -il \'error\' ' + locallogdir + '/setup*')
if (out != '') :
  print "There is error in setup of machines"
  print "status  = " + str(status) + "\n out = " + out
  print sys.exit(1)


threadListGlobal=[]
testcaseList=xmlRoot.findall('testCase')
#print testcaseList
for testcase in testcaseList:
  try:
    testcasename = testcase.get('id')
    testcaseclass = testcase.findtext('class',default='')
    desc = testcase.findtext('desc',default='')
    #print "Machine list top level = " + ','.join(MachineList)
    if search_test_case_group(testcasename, testcasegroup) == True:
      if testcaseclass == 'lte-softmodem' :
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
        threadListGlobal = wait_testcaseclass_generic_threads(threadListGlobal, Timeout_execution)
        handle_testcaseclass_softmodem (testcase, CleanUpOldProgs, logdirOAI5GRepo, logdirOpenaircnRepo, MachineList, pw, CleanUpAluLteBox )
      elif (testcaseclass == 'compilation'): 
        threadListGlobal = handle_testcaseclass_generic (testcasename, threadListGlobal, CleanUpOldProgs, logdirOAI5GRepo, MachineListGeneric, pw, CleanUpAluLteBox,Timeout_execution)
      elif (testcaseclass == 'execution'): 
        threadListGlobal = handle_testcaseclass_generic (testcasename, threadListGlobal, CleanUpOldProgs, logdirOAI5GRepo, MachineListGeneric, pw, CleanUpAluLteBox,Timeout_execution)
      else :
        print "Unknown test case class: " + testcaseclass
        sys.exit()

  except Exception, e:
     error=''
     error = error + ' In function: ' + sys._getframe().f_code.co_name + ': *** Caught exception: '  + str(e.__class__) + " : " + str( e)
     error = error + '\n testcasename = ' + testcasename + '\n testcaseclass = ' + testcaseclass + '\n desc = ' + 'desc' + '\n'  
     error = error + traceback.format_exc()
     print error
     sys.exit(1)


print "Exiting the test cases execution now..."

for t in threadListGlobal:
   t.join

sys.exit()

   #+ "class = "+ classx



      #index = index +1

test = 'test01'
ctime=datetime.datetime.utcnow().strftime("%Y-%m-%d.%Hh%M")
logfile = user+'.'+test+'.'+ctime+'.txt'  
logdir = os.getcwd() + '/pre-ci-logs-'+host;
oai.create_dir(logdir,debug)    
print 'log dir: ' + logdir
print 'log file: ' + logfile
pwd = oai.send_recv('pwd') 
print "pwd = " + pwd
result = oai.send('echo linux | sudo -S ls -al;sleep 5')
print "result =" + result
sys.exit()

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
