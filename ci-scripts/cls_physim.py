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
#to update the HTML object
import cls_oai_html
from multiprocessing import SimpleQueue
#for log folder maintenance
import os

class PhySim:
	def __init__(self):
		self.buildargs = ""
		self.runargs = ""
		self.eNBIpAddr = ""
		self.eNBUserName = ""
		self.eNBPassWord = ""
		self.eNBSourceCodePath = ""
		self.ranRepository = ""
		self.ranBranch = ""
		self.ranCommitID= ""
		self.ranAllowMerge= ""
		self.ranTargetBranch= ""
		self.exitStatus=0
		self.forced_workspace_cleanup=False
		#private attributes
		self.__workSpacePath=''
		self.__buildLogFile='compile_phy_sim.log'
		self.__runLogFile=''
		self.__runLogPath='phy_sim_logs'


#-----------------
#PRIVATE Methods
#-----------------

	def __CheckResults_LDPCTest(self,HTML,CONST,testcase_id):
		mySSH = sshconnection.SSHConnection()
		mySSH.open(self.eNBIpAddr, self.eNBUserName, self.eNBPassWord)
		#retrieve run log file and store it locally$
		mySSH.copyin(self.eNBIpAddr, self.eNBUserName, self.eNBPassWord, self.__workSpacePath+self.__runLogFile, '.')
		mySSH.close()
		#parse results looking for Encoding and Decoding mean values
		runResults=[]
		with open(self.__runLogFile) as f:
			for line in f:
				if 'mean' in line:
					runResults.append(line)
		#the values are appended for each mean value (2), so we take these 2 values from the list
		info = runResults[0] + runResults[1]

		#once parsed move the local logfile to its folder for tidiness
		os.system('mv '+self.__runLogFile+' '+ self.__runLogPath+'/.')

		HTML.CreateHtmlTestRowQueue(self.runargs, 'OK', [info])
		return HTML

	def __CheckResults_LDPCt1Test(self,HTML,CONST,testcase_id):
		thrs_NOK = 500
		thrs_KO = 1000
		mySSH = sshconnection.SSHConnection()
		mySSH.open(self.eNBIpAddr, self.eNBUserName, self.eNBPassWord)
		#retrieve run log file and store it locally$
		mySSH.copyin(self.eNBIpAddr, self.eNBUserName, self.eNBPassWord, self.__workSpacePath+self.__runLogFile, '.')
		mySSH.close()
		#parse results looking for Decoding values
		runResultsT1=[]
		with open(self.__runLogFile) as g:
			for line in g:
				if 'decoding time' in line:
					runResultsT1.append(line)

		# In case the T1 board does work properly, there is no statistics
		if len(runResultsT1) == 0:
			logging.error('no statistics')
			HTML.CreateHtmlTestRowQueue(self.runargs, 'KO', ['no statistics'])
			self.exitStatus = 1
			os.system('mv '+self.__runLogFile+' '+ self.__runLogPath+'/.')
			return HTML

		info = runResultsT1[0][15:-13]
		result = int(''.join(filter(str.isdigit, info)))/100
		#once parsed move the local logfile to its folder for tidiness
		os.system('mv '+self.__runLogFile+' '+ self.__runLogPath+'/.')
		if result < thrs_NOK:
			HTML.CreateHtmlTestRowQueue(self.runargs, 'OK', [info])
		elif result > thrs_KO:
			error_msg = f'Decoding time exceeds a limit of {thrs_KO} us'
			logging.error(error_msg)
			HTML.CreateHtmlTestRowQueue(self.runargs, 'KO', [info + '\n' + error_msg])
			self.exitStatus = 1
		else:
			HTML.CreateHtmlTestRowQueue(self.runargs, 'NOK', [info])
		return HTML

	def __CheckResults_NRulsimTest(self, HTML, CONST, testcase_id):
		#retrieve run log file and store it locally
		mySSH = sshconnection.SSHConnection()
		filename = self.__workSpacePath + self.__runLogFile
		ret = mySSH.copyin(self.eNBIpAddr, self.eNBUserName, self.eNBPassWord, filename, '.')
		if ret != 0:
			error_msg = f'could not recover test result file {filename}'
			logging.error(error_msg)
			HTML.CreateHtmlTestRowQueue("could not recover results", 'KO', [error_msg])
			self.exitStatus = 1
			return HTML

		PUSCH_OK = False
		with open(self.__runLogFile) as f:
			PUSCH_OK = 'PUSCH test OK' in f.read()

		# once parsed move the local logfile to its folder for tidiness
		os.system(f'mv {self.__runLogFile} {self.__runLogPath}/.')

		#updating the HTML with results
		if PUSCH_OK:
			HTML.CreateHtmlTestRowQueue(self.runargs, 'OK', 1, ["succeeded"])
		else:
			error_msg = 'error: no "PUSCH test OK"'
			logging.error(error_msg)
			HTML.CreateHtmlTestRowQueue(self.runargs, 'KO', 1, [error_msg])
			self.exitStatus = 1
		return HTML

	def __CheckBuild_PhySim(self, HTML, CONST):
		self.__workSpacePath=self.eNBSourceCodePath+'/cmake_targets/'
		mySSH = sshconnection.SSHConnection()
		mySSH.open(self.eNBIpAddr, self.eNBUserName, self.eNBPassWord)
		#retrieve compile log file and store it locally
		mySSH.copyin(self.eNBIpAddr, self.eNBUserName, self.eNBPassWord, self.__workSpacePath+self.__buildLogFile, '.')
		#delete older run log file
		mySSH.command('rm ' + self.__workSpacePath+self.__runLogFile, '\$', 5)
		mySSH.close()
		#check build result from local compile log file
		with open(self.__buildLogFile) as f:
			if 'BUILD SHOULD BE SUCCESSFUL' in f.read():
				HTML.CreateHtmlTestRow(self.buildargs, 'OK', CONST.ALL_PROCESSES_OK, 'PhySim')
				self.exitStatus=0
				return HTML
		logging.error('\u001B[1m Building Physical Simulators Failed\u001B[0m')
		HTML.CreateHtmlTestRow(self.buildargs, 'KO', CONST.ALL_PROCESSES_OK, 'LDPC')
		HTML.CreateHtmlTabFooter(False)
		#exitStatus=1 will do a sys.exit in main
		self.exitStatus = 1
		return HTML


#-----------------$
#PUBLIC Methods$
#-----------------$

	def Build_PhySim(self,htmlObj,constObj):
		mySSH = sshconnection.SSHConnection()
		mySSH.open(self.eNBIpAddr, self.eNBUserName, self.eNBPassWord)

		#create working dir
		mySSH.command('mkdir -p ' + self.eNBSourceCodePath, '\$', 5)
		mySSH.command('cd ' + self.eNBSourceCodePath, '\$', 5)

		if not self.ranRepository.lower().endswith('.git'):
			self.ranRepository+='.git'

		#git clone
		mySSH.command('if [ ! -e .git ]; then stdbuf -o0 git clone '  + self.ranRepository + ' .; else stdbuf -o0 git fetch --prune; fi', '\$', 600)
		#git config
		mySSH.command('git config user.email "jenkins@openairinterface.org"', '\$', 5)
		mySSH.command('git config user.name "OAI Jenkins"', '\$', 5)

		#git clean depending on self.forced_workspace_cleanup captured in xml
		if self.forced_workspace_cleanup==True:
			logging.info('Cleaning workspace ...')
			mySSH.command('echo ' + self.eNBPassWord + ' | sudo -S git clean -x -d -ff', '\$', 30)
		else:
			logging.info('Workspace cleaning was disabled')

		# if the commit ID is provided, use it to point to it
		if self.ranCommitID != '':
			mySSH.command('git checkout -f ' + self.ranCommitID, '\$', 30)
		# if the branch is not develop, then it is a merge request and we need to do
		# the potential merge. Note that merge conflicts should have already been checked earlier
		if (self.ranAllowMerge):
			if self.ranTargetBranch == '':
				if (self.ranBranch != 'develop') and (self.ranBranch != 'origin/develop'):
					mySSH.command('git merge --ff origin/develop -m "Temporary merge for CI"', '\$', 30)
			else:
				logging.info('Merging with the target branch: ' + self.ranTargetBranch)
				mySSH.command('git merge --ff origin/' + self.ranTargetBranch + ' -m "Temporary merge for CI"', '\$', 30)

		#build
		mySSH.command('source oaienv', '\$', 5)
		mySSH.command('cd cmake_targets', '\$', 5)
		mySSH.command('mkdir -p log', '\$', 5)
		mySSH.command(f'./build_oai {self.buildargs} 2>&1 | tee {self.__buildLogFile}', '\$', 1500)

		mySSH.close()
		#check build status and update HTML object
		lHTML = cls_oai_html.HTMLManagement()
		lHTML=self.__CheckBuild_PhySim(htmlObj,constObj)
		return lHTML


	def Run_LDPCTest(self,htmlObj,constObj,testcase_id):
		self.__workSpacePath = self.eNBSourceCodePath+'/cmake_targets/'
		#create run logs folder locally
		os.system('mkdir -p ./'+self.__runLogPath)
		#log file is tc_<testcase_id>.log remotely
		self.__runLogFile='physim_'+str(testcase_id)+'.log'
		#open a session for test run
		mySSH = sshconnection.SSHConnection()
		mySSH.open(self.eNBIpAddr, self.eNBUserName, self.eNBPassWord)
		mySSH.command('cd '+self.__workSpacePath,'\$',5)
		#run and redirect the results to a log file
		mySSH.command(self.__workSpacePath+'ran_build/build/ldpctest ' + self.runargs + ' >> '+self.__runLogFile, '\$', 30)
		mySSH.close()
		#return updated HTML to main
		lHTML = cls_oai_html.HTMLManagement()
		lHTML=self.__CheckResults_LDPCTest(htmlObj,constObj,testcase_id)
		return lHTML

	def Run_LDPCt1Test(self,htmlObj,constObj,testcase_id):
		self.__workSpacePath = self.eNBSourceCodePath+'/cmake_targets/'
		#create run logs folder locally
		os.system('mkdir -p ./'+self.__runLogPath)
		#log file is tc_<testcase_id>.log remotely
		self.__runLogFile='physim_'+str(testcase_id)+'.log'
		#open a session for test run
		mySSH = sshconnection.SSHConnection()
		mySSH.open(self.eNBIpAddr, self.eNBUserName, self.eNBPassWord)
		mySSH.command('cd '+self.__workSpacePath,'\$',5)
		#run and redirect the results to a log file
		mySSH.command(f'sudo {self.__workSpacePath}ran_build/build/nr_ulsim {self.runargs} > {self.__runLogFile} 2>&1', '\$', 30)
		mySSH.close()
		#return updated HTML to main
		lHTML = cls_oai_html.HTMLManagement()
		lHTML=self.__CheckResults_LDPCt1Test(htmlObj,constObj,testcase_id)
		return lHTML

	def Run_NRulsimTest(self, htmlObj, constObj, testcase_id):
		self.__workSpacePath=self.eNBSourceCodePath+'/cmake_targets/'
		os.system(f'mkdir -p ./{self.__runLogPath}')
		self.__runLogFile = f'physim_{testcase_id}.log'
		mySSH = sshconnection.SSHConnection()
		mySSH.open(self.eNBIpAddr, self.eNBUserName, self.eNBPassWord)
		mySSH.command(f'cd {self.__workSpacePath}', '\$', 5)
		mySSH.command(f'sudo {self.__workSpacePath}ran_build/build/nr_ulsim {self.runargs} > {self.__runLogFile} 2>&1', '\$', 30)
		mySSH.close()
		#return updated HTML to main
		lHTML = self.__CheckResults_NRulsimTest(htmlObj, constObj, testcase_id)
		return lHTML
