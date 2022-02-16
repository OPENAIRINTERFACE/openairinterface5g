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

#USAGE: 
#	log=Log_Mgt(Username,IPAddress,Password,Path)
#	log.LogRotation()




import re
import subprocess
import logging
import math

class Log_Mgt:

	def __init__(self,Username, IPAddress,Password,Path):
		self.Username=Username
		self.IPAddress=IPAddress
		self.Password=Password
		self.path=Path

#-----------------$
#PRIVATE# Methods$
#-----------------$


	def __CheckAvailSpace(self):
		HOST=self.Username+'@'+self.IPAddress
		COMMAND="df "+ self.path
		ssh = subprocess.Popen(["ssh", "%s" % HOST, COMMAND],shell=False,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
		result = ssh.stdout.readlines()
		s=result[1].decode('utf-8').rstrip()#result[1] is the second line with the results we are looking for
		tmp=s.split()
		return tmp[3] #return avail space from the line

	def __GetOldestFile(self):
		HOST=self.Username+'@'+self.IPAddress
		COMMAND="ls -rtl "+ self.path #-rtl will bring oldest file on top
		ssh = subprocess.Popen(["ssh", "%s" % HOST, COMMAND],shell=False,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
		result = ssh.stdout.readlines()
		s=result[1].decode('utf-8').rstrip()
		tmp=s.split()
		return tmp[8]#return filename from the line


	def __AvgSize(self):
		HOST=self.Username+'@'+self.IPAddress
		COMMAND="ls -rtl "+ self.path
		ssh = subprocess.Popen(["ssh", "%s" % HOST, COMMAND],shell=False,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
		result = ssh.stdout.readlines()
		if len(result)>1: #at least 1 file present
			total_size=0
			for i in range(1,len(result)):
				s=result[i].decode('utf-8').rstrip()
				tmp=s.split()
				total_size+=int(tmp[4]) #get filesize
			return math.floor(total_size/(len(result)-1)) #compute average file/artifact size
		else:#empty,no files
			return 0


#-----------------$
#PUBLIC Methods$
#-----------------$


	def LogRotation(self):
		avail_space =int(self.__CheckAvailSpace())*1000 #avail space in target folder, initially displayed in Gb
		avg_size=self.__AvgSize() #average size of artifacts in the target folder
		logging.debug("Avail Space : " + str(avail_space) + " / Artifact Avg Size : " + str(avg_size))
		if avail_space < 50*avg_size: #reserved space is 50x artifact file ; oldest file will be deleted
			oldestfile=self.__GetOldestFile()
			HOST=self.Username+'@'+self.IPAddress
			COMMAND="echo " + self.Password + " | sudo -S rm "+ self.path + "/" + oldestfile
			logging.debug(COMMAND)
			ssh = subprocess.Popen(["ssh", "%s" % HOST, COMMAND],shell=False,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
		else:
			logging.debug("Still some space left for artifacts storage")
			



