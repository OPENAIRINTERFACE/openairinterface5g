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
#---------------------------------------------------------------------
#
#   Required Python Version
#     Python 3.x
#
#---------------------------------------------------------------------

#to use isfile
import os
import sys
import logging
#to create a SSH object locally in the methods
import sshconnection
#time.sleep
import time


import re
import subprocess

from datetime import datetime

#for log rotation mgt
import cls_log_mgt

class Module_UE:

	def __init__(self,Module):
		#create attributes as in the Module dictionary
		for k, v in Module.items():
			setattr(self, k, v)
		self.UEIPAddress = ""
		#dictionary linking command names and related module scripts
		self.cmd_dict= {"wup": self.WakeupScript,"detach":self.DetachScript}#dictionary of function scripts		



#-----------------$
#PUBLIC Methods$
#-----------------$

	#this method checks if the specified Process is running on the server hosting the module
	#if not it will be started
	def CheckCMProcess(self):
		HOST=self.HostIPAddress
		COMMAND="ps aux | grep " + self.Process['Name'] + " | grep -v grep "
		logging.debug(COMMAND)
		ssh = subprocess.Popen(["ssh", "%s" % HOST, COMMAND],shell=False,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
		result = ssh.stdout.readlines()
		if len(result)!=0:
			logging.debug(self.Process['Name'] + " process found")
			return True 
		else:#start process and check again  
			logging.debug(self.Process['Name'] + " process NOT found")
			#starting the process
			logging.debug('Starting ' + self.Process['Name'])
			mySSH = sshconnection.SSHConnection()
			mySSH.open(self.HostIPAddress, self.HostUsername, self.HostPassword)
			mySSH.command('echo ' + self.HostPassword + ' | sudo -S ' + self.Process['Cmd'] + ' &','\$',5)
			mySSH.close()
			#checking the process
			HOST=self.HostIPAddress
			COMMAND="ps aux | grep " + self.Process['Name'] + " | grep -v grep "
			logging.debug(COMMAND)
			ssh = subprocess.Popen(["ssh", "%s" % HOST, COMMAND],shell=False,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
			result = ssh.stdout.readlines()
			if len(result)!=0:
				logging.debug(self.Process['Name'] + " process found")
				return True
			else:
				logging.debug(self.Process['Name'] + " process NOT found")
				return False 

	#Generic command function, using function pointers dictionary
	def Command(self,cmd):
		mySSH = sshconnection.SSHConnection()
		mySSH.open(self.HostIPAddress, self.HostUsername, self.HostPassword)
		mySSH.command('echo ' + self.HostPassword + ' | sudo -S python3 ' + self.cmd_dict[cmd],'\$',10)
		time.sleep(5)
		logging.debug("Module "+ cmd)
		mySSH.close()


	#this method retrieves the Module IP address (not the Host IP address) 
	def GetModuleIPAddress(self):
		HOST=self.HostIPAddress
		response= []
		tentative = 3 
		while (len(response)==0) and (tentative>0):
			COMMAND="ip a show dev " + self.UENetwork + " | grep inet | grep " + self.UENetwork
			logging.debug(COMMAND)
			ssh = subprocess.Popen(["ssh", "%s" % HOST, COMMAND],shell=False,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
			response = ssh.stdout.readlines()
			tentative-=1
			time.sleep(10)
		if (tentative==0) and (len(response)==0):
			logging.debug('\u001B[1;37;41m Module IP Address Not Found! Time expired \u001B[0m')
			return -1
		else: #check response
			result = re.search('inet (?P<moduleipaddress>[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+)', response[0].decode("utf-8") )
			if result is not None: 
				if result.group('moduleipaddress') is not None: 
					self.UEIPAddress = result.group('moduleipaddress')
					logging.debug('\u001B[1mUE Module IP Address is ' + self.UEIPAddress + '\u001B[0m')
					return 0
				else:
					logging.debug('\u001B[1;37;41m Module IP Address Not Found! \u001B[0m')
					return -1
			else:
				logging.debug('\u001B[1;37;41m Module IP Address Not Found! \u001B[0m')
				return -1

	def EnableTrace(self):
		mySSH = sshconnection.SSHConnection()
		mySSH.open(self.HostIPAddress, self.HostUsername, self.HostPassword)
		#delete old artifacts
		mySSH.command('echo ' + self.HostPassword + ' | sudo -S rm -rf ci_qlog','\$',5)
		#start Trace
		mySSH.command('echo $USER; nohup sudo -E QLog/QLog -s ci_qlog -f NR5G.cfg &','\$', 5)
		mySSH.close()

	def DisableTrace(self):
		mySSH = sshconnection.SSHConnection()
		mySSH.open(self.HostIPAddress, self.HostUsername, self.HostPassword)
		mySSH.command('echo ' + self.HostPassword + ' | sudo -S killall --signal=SIGINT *QLog*', '\$',5)
		mySSH.close()


	def DisableCM(self):
		mySSH = sshconnection.SSHConnection()
		mySSH.open(self.HostIPAddress, self.HostUsername, self.HostPassword)
		mySSH.command('echo ' + self.HostPassword + ' | sudo -S killall --signal SIGKILL *'+self.Process['Name']+'*', '\$', 5)
		mySSH.close()


	def LogCollect(self):
		mySSH = sshconnection.SSHConnection()
		mySSH.open(self.HostIPAddress, self.HostUsername, self.HostPassword)
		#archive qlog to /opt/ci_qlogs with datetime suffix
		now=datetime.now()
		now_string = now.strftime("%Y%m%d-%H%M")
		source='ci_qlog'
		destination='/opt/ci_qlogs/ci_qlog_'+now_string+'.zip'
		#qlog artifact is zipped into the target folder
		mySSH.command('echo $USER; echo ' + self.HostPassword + ' | nohup sudo -S zip -r '+destination+' '+source+' &','\$', 10)
		mySSH.close()
		#post action : log cleaning to make sure enough space is reserved for the next run
		Log_Mgt=cls_log_mgt.Log_Mgt(self.HostIPAddress, self.HostPassword, "/opt/ci_qlogs")
		Log_Mgt.LogRotation()

		return destination
