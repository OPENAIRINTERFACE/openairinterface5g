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

#to create a SSH object locally in the methods
import sshconnection
#to update the HTML object
import html
from multiprocessing import SimpleQueue


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
		self.workSpacePath=''
		self.buildLogFile='compile_oai_enb.log'
		self.runLogFile='ldpctest_run_results.log'
		self.runResults=[]

	def __CheckResults_PhySim(self,HTML,CONST):
		mySSH = sshconnection.SSHConnection()
		mySSH.open(self.eNBIpAddr, self.eNBUserName, self.eNBPassWord)
		#retrieve run log file locally$
		mySSH.copyin(self.eNBIpAddr, self.eNBUserName, self.eNBPassWord, self.workSpacePath+self.runLogFile, '.')
		mySSH.close()
		with open(self.runLogFile) as f:
			for line in f:
				if 'mean' in line:
					self.runResults.append(line)
		info=self.runResults[-1]+self.runResults[-2]

		html_cell = '<pre style="background-color:white">' + info  + '</pre>'
		html_queue=SimpleQueue()
		html_queue.put(html_cell)
		HTML.CreateHtmlTestRowQueue(self.runargs, 'OK', 1, html_queue)
		return HTML


	def __CheckBuild_PhySim(self, HTML, CONST):
		self.workSpacePath=self.eNBSourceCodePath+'/cmake_targets/'
		mySSH = sshconnection.SSHConnection()
		mySSH.open(self.eNBIpAddr, self.eNBUserName, self.eNBPassWord)
		#retrieve compile log file locally
		mySSH.copyin(self.eNBIpAddr, self.eNBUserName, self.eNBPassWord, self.workSpacePath+self.buildLogFile, '.')
		#delete older run log file
		mySSH.command('rm ' + self.workSpacePath+self.runLogFile, '\$', 5)
		mySSH.close()
		#check build result from local compile log file
		buildStatus=False
		with open(self.buildLogFile) as f:
			if 'nr_prachsim compiled' in f.read():
				buildStatus=True
		#update HTML based on build status
		if buildStatus:
			HTML.CreateHtmlTestRow(self.buildargs, 'OK', CONST.ALL_PROCESSES_OK, 'LDPC')
			self.exitStatus=0
		else:
			logging.error('\u001B[1m Building OAI UE Failed\u001B[0m')
			HTML.CreateHtmlTestRow(self.buildargs, 'KO', CONST.ALL_PROCESSES_OK, 'LDPC')
			HTML.CreateHtmlTabFooter(False)
			self.exitStatus=1
		return HTML

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

		#git clean
		mySSH.command('echo ' + self.eNBPassWord + ' | sudo -S git clean -x -d -ff', '\$', 30)

		# if the commit ID is provided, use it to point to it
		if self.ranCommitID != '':
			mySSH.command('git checkout -f ' + self.ranCommitID, '\$', 5)
		# if the branch is not develop, then it is a merge request and we need to do 
		# the potential merge. Note that merge conflicts should have already been checked earlier
		if (self.ranAllowMerge):
			if self.ranTargetBranch == '':
				if (self.ranBranch != 'develop') and (self.ranBranch != 'origin/develop'):
					mySSH.command('git merge --ff origin/develop -m "Temporary merge for CI"', '\$', 5)
			else:
				print('Merging with the target branch: ' + self.ranTargetBranch)
				mySSH.command('git merge --ff origin/' + self.ranTargetBranch + ' -m "Temporary merge for CI"', '\$', 5)

		#build
		mySSH.command('source oaienv', '\$', 5)
		mySSH.command('cd cmake_targets', '\$', 5)
		mySSH.command('mkdir -p log', '\$', 5)
		mySSH.command('chmod 777 log', '\$', 5)
		mySSH.command('stdbuf -o0 ./build_oai ' + self.buildargs + ' 2>&1 | stdbuf -o0 tee compile_oai_enb.log', 'Bypassing the Tests|build have failed', 1500) 

		mySSH.close()
		#check build status and update HTML object
		lHTML = html.HTMLManagement()
		lHTML=self.__CheckBuild_PhySim(htmlObj,constObj)
		return lHTML


	def Run_PhySim(self,htmlObj,constObj):
		mySSH = sshconnection.SSHConnection()
		mySSH.open(self.eNBIpAddr, self.eNBUserName, self.eNBPassWord)
		mySSH.command('cd '+self.workSpacePath,'\$',5)
		mySSH.command(self.workSpacePath+'phy_simulators/build/ldpctest ' + self.runargs + ' >> '+self.runLogFile, '\$', 30)   
		mySSH.close()
		lHTML = html.HTMLManagement()
		lHTML=self.__CheckResults_PhySim(htmlObj,constObj)
		return lHTML
