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



class Module_UE:
#  ID: idefix
#  State : enabled
#  Kind : quectel
#  Process : quectel-cm
#  WakeupScript : ci_qtel.py
#  UENetwork : wwan0
#  HostIPAddress : 192.168.18.188
#  HostUsername : oaicicd
#  HostPassword : oaicicd
#  HostSourceCodePath : none

	def __init__(self,Module):
		#create attributes as in the Module dictionary
		for k, v in Module.items():
			setattr(self, k, v)
		self.UEIPAddress = ""
		



#-----------------$
#PUBLIC Methods$
#-----------------$

	#this method checks if the specified Process is running on the server hosting the module
	def CheckIsModule(self):
		HOST=self.HostIPAddress
		COMMAND="ps aux | grep " + self.Process + " | grep -v grep "
		logging.debug(COMMAND)
		ssh = subprocess.Popen(["ssh", "%s" % HOST, COMMAND],shell=False,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
		result = ssh.stdout.readlines()
		if len(result)!=0:
			logging.debug(self.Process + " process found")
			return True 
		else:
			logging.debug(self.Process + " process NOT found")
			return False 

	#Wakeup/Detach can probably be improved with encapsulation of the command such def Command(self, command)
	#this method wakes up the module by calling the specified python script 
	def WakeUp(self):
		mySSH = sshconnection.SSHConnection()
		mySSH.open(self.HostIPAddress, self.HostUsername, self.HostPassword)
		mySSH.command('echo ' + self.HostPassword + ' | sudo -S python3 ' + self.WakeupScript + ' ','\$',5)
		time.sleep(5)
		logging.debug("Module wake-up")
		mySSH.close()

	#this method detaches the module by calling the specified python script 
	def Detach(self):
		mySSH = sshconnection.SSHConnection()
		mySSH.open(self.HostIPAddress, self.HostUsername, self.HostPassword)
		mySSH.command('echo ' + self.HostPassword + ' | sudo -S python3 ' + self.DetachScript + ' ','\$',5)
		time.sleep(5)
		logging.debug("Module detach")
		mySSH.close()

	#this method retrieves the Module IP address (not the Host IP address) 
	def GetModuleIPAddress(self):
		HOST=self.HostIPAddress
		COMMAND="ip a show dev " + self.UENetwork + " | grep inet | grep " + self.UENetwork
		ssh = subprocess.Popen(["ssh", "%s" % HOST, COMMAND],shell=False,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
		response = ssh.stdout.readlines()
		if len(response)!=0:
			result = re.search('inet (?P<moduleipaddress>[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+)', response[0].decode("utf-8") )
			if result is not None: 
				if result.group('moduleipaddress') is not None: 
					self.UEIPAddress = result.group('moduleipaddress')
					logging.debug('\u001B[1mUE Module IP Address is ' + self.UEIPAddress + '\u001B[0m')
				else:
					logging.debug('\u001B[1;37;41m Module IP Address Not Found! \u001B[0m')
			else:
				logging.debug('\u001B[1;37;41m Module IP Address Not Found! \u001B[0m')
		else:
			logging.debug('\u001B[1;37;41m Module IP Address Not Found! \u001B[0m')






