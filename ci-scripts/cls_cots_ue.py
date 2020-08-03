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
# Python for CI of OAI-eNB + COTS-UE
#
#   Required Python Version
#     Python 3.x
#
#   Required Python Package
#     pexpect
#---------------------------------------------------------------------

#to use logging.info()
import logging
#to create a SSH object locally in the methods
import sshconnection
#time.sleep
import time

class CotsUe:
	def __init__(self,model,UEIPAddr,UEUserName,UEPassWord):
		self.model = model
		self.UEIPAddr = UEIPAddr 
		self.UEUserName = UEUserName
		self.UEPassWord = UEPassWord
		self.runargs = '' #on of off to toggle airplane mode on/off
		self.__SetAirplaneRetry = 3

#-----------------$
#PUBLIC Methods$
#-----------------$

	def Check_Airplane(self):
		mySSH = sshconnection.SSHConnection()
		mySSH.open(self.UEIPAddr, self.UEUserName, self.UEPassWord)
		status=mySSH.cde_check_value('sudo adb shell settings get global airplane_mode_on ', ['0','1'],5)
		mySSH.close()
		return status


	def Set_Airplane(self,target_state_str):
		mySSH = sshconnection.SSHConnection()
		mySSH.open(self.UEIPAddr, self.UEUserName, self.UEPassWord)
		mySSH.command('sudo adb start-server','$',5)
		logging.info("Toggling COTS UE Airplane mode to : "+target_state_str)
		current_state = self.Check_Airplane()
		if target_state_str.lower()=="on": 
			target_state=1
		else:
			target_state=0
		if current_state != target_state:
			#toggle state
			retry = 0 
			while (current_state!=target_state) and (retry < self.__SetAirplaneRetry):
				mySSH.command('sudo adb shell am start -a android.settings.AIRPLANE_MODE_SETTINGS', '\$', 5)
				mySSH.command('sudo adb shell input keyevent 20', '\$', 5)
				mySSH.command('sudo adb shell input tap 968 324', '\$', 5)
				time.sleep(1)
				current_state = self.Check_Airplane()
				retry+=1
			if current_state != target_state:
				logging.error("ATTENTION : Could not toggle to : "+target_state_str)
				logging.error("Current state is : "+ str(current_state))
		else:
			print("Airplane mode is already "+ target_state_str)
		mySSH.command('sudo adb kill-server','$',5)
		mySSH.close()




