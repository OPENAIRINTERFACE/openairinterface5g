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



import logging
import re
import subprocess
import sshconnection

class Log_Mgt:

	def __init__(self,Username, IPAddress,Password,Path):
		self.Username=Username
		self.IPAddress=IPAddress
		self.Password=Password
		self.path=Path

#-----------------$
#PRIVATE# Methods$
#-----------------$


	def __CheckUsedSpace(self):
		HOST=self.Username+'@'+self.IPAddress
		COMMAND="df "+ self.path
		ssh = subprocess.Popen(["ssh", "%s" % HOST, COMMAND],shell=False,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
		result = ssh.stdout.readlines()
		s=result[1].decode('utf-8').rstrip()#result[1] is the second line with the results we are looking for
		used=s.split()[4] #get 4th field ex: 70%
		m = re.match('^(\d+)\%',used)
		if m is not None:
			return int(m.group(1))

	def __RemoveOldest(self, days):
		mySSH = sshconnection.SSHConnection()
		mySSH.open(self.IPAddress, self.Username, self.Password)
		COMMAND='echo ' + self.Password + ' | sudo -S find ' + self.path + ' -type f -mtime +' +  str(days) + ' -delete'
		mySSH.command(COMMAND,'\$',20)
		mySSH.close()



#-----------------$
#PUBLIC Methods$
#-----------------$


	def LogRotation(self):
		doLoop = True
		nbDays = 14
		while doLoop and nbDays > 1:
			used_space = self.__CheckUsedSpace() #avail space in target folder
			if used_space > 80 :
				logging.debug('\u001B[1;37;41m  Used Disk (' + str(used_space) + '%) > 80%, on '  + self.Username+'@'+self.IPAddress + '\u001B[0m')
				logging.debug('\u001B[1;37;41m  Removing Artifacts older than ' + str(nbDays) + ' days \u001B[0m')
				self.__RemoveOldest(nbDays)
				nbDays -= 1
			else:
				logging.debug('Used Disk (' + str(used_space) + '%) < 80%, on '  + self.Username+'@'+self.IPAddress +', no cleaning required')
				doLoop = False


			



