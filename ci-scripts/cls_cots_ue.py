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

#to use isfile
import os
import sys
#to use logging.info()
import logging
#to create a SSH object locally in the methods
import sshconnection
#time.sleep
import time
#to load cots_ue dictionary
import yaml

class CotsUe:
	def __init__(self,ADBIPAddr,ADBUserName,ADBPassWord):
		self.cots_id = '' #cots id from yaml oppo, s10 etc...
		self.ADBIPAddr = ADBIPAddr 
		self.ADBUserName = ADBUserName
		self.ADBPassWord = ADBPassWord
		self.cots_run_mode = '' #on of off to toggle airplane mode on/off
		self.__cots_cde_dict_file = 'cots_ue_ctl.yaml'
		self.__SetAirplaneRetry = 3


#-----------------$
#PUBLIC Methods$
#-----------------$

	def Check_Airplane(self):
		mySSH = sshconnection.SSHConnection()
		mySSH.open(self.ADBIPAddr, self.ADBUserName, self.ADBPassWord)
		status=mySSH.cde_check_value('adb shell settings get global airplane_mode_on ', ['0','1'],5)
		mySSH.close()
		return status

	#simply check if the device id exists in the dictionary
	#returns true if it exists, false otherwise
	def Check_Exists(self,target_id):
		#load cots commands dictionary
		if (os.path.isfile(self.__cots_cde_dict_file)):
			yaml_file=self.__cots_cde_dict_file
		elif (os.path.isfile('ci-scripts/'+self.__cots_cde_dict_file)):
			yaml_file='ci-scripts/'+self.__cots_cde_dict_file
		else:
			logging.error("COTS UE dictionary yaml file cannot be found")
			sys.exit("COTS UE dictionary yaml file cannot be found")
		
		#load cots commands dictionary
		with open(yaml_file,'r') as file:
			cots_ue_ctl = yaml.load(file,Loader=yaml.FullLoader)
		#check if ue id is in the dictionary
		if target_id in cots_ue_ctl:
			return True
		else:
			return False

	def Set_Airplane(self, target_id, target_state_str):
		#loading cots commands dictionary

		if (os.path.isfile(self.__cots_cde_dict_file)):
			yaml_file=self.__cots_cde_dict_file
		elif (os.path.isfile('ci-scripts/'+self.__cots_cde_dict_file)):
			yaml_file='ci-scripts/'+self.__cots_cde_dict_file
		else:
			logging.error("COTS UE dictionary yaml file cannot be found")
			sys.exit("COTS UE dictionary yaml file cannot be found")
		
		#load cots commands dictionary
		with open(yaml_file,'r') as file:
			cots_ue_ctl = yaml.load(file,Loader=yaml.FullLoader)
		#check if ue id is in the dictionary
		if target_id in cots_ue_ctl:
			mySSH = sshconnection.SSHConnection()
			mySSH.open(self.ADBIPAddr, self.ADBUserName, self.ADBPassWord)
			logging.info(str(self.ADBIPAddr)+' '+str(self.ADBUserName)+' '+str(self.ADBPassWord))
			mySSH.command('adb start-server','\$',5)
			mySSH.command('adb devices','\$',5)
			logging.info("Toggling COTS UE Airplane mode to : "+target_state_str)
			#get current state
			current_state = self.Check_Airplane()
			if target_state_str.lower()=="on": 
				target_state=1
			else:
				target_state=0
			if current_state != target_state:
				#toggle state
				retry = 0 
				while (current_state!=target_state) and (retry < self.__SetAirplaneRetry):
					#loop over the command list from dictionary for the selected ue, to switch to required state
					for i in range (0,len(cots_ue_ctl[target_id])):
						mySSH.command(cots_ue_ctl[target_id][i], '\$', 5)
					time.sleep(1)
					current_state = self.Check_Airplane()
					retry+=1
				#could not toggle despite the retry
				if current_state != target_state:
					logging.error("ATTENTION : Could not toggle to : "+target_state_str)
					logging.error("Current state is : "+ str(current_state))
			else:
				logging.info("Airplane mode is already "+ target_state_str)
			mySSH.command('adb kill-server','\$',5)
			mySSH.close()
		#ue id is NOT in the dictionary
		else:
			logging.error("COTS UE Id from XML could not be found in UE YAML dictionary " + self.__cots_cde_dict_file)
			sys.exit("COTS UE Id from XML could not be found in UE YAML dictionary " + self.__cots_cde_dict_file)




