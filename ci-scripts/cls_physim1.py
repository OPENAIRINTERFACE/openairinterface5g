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
#---------------------------------------------------------------------
# Python for CI of OAI-eNB + COTS-UE
#
#   Required Python Version
#     Python 3.x
#
#   Required Python Package
#     pexpect
#---------------------------------------------------------------------

#-----------------------------------------------------------
# Import
#-----------------------------------------------------------
import logging
import sshconnection as SSH
import html
import os
import re
import time
import sys
import constants as CONST
import helpreadme as HELP

class PhySim:
	def __init__(self):
		self.eNBIpAddr = ""
		self.eNBUserName = ""
		self.eNBPassword = ""
		self.OCUserName = ""
		self.OCPassword = ""
		self.OCWorkspace = ""
		self.eNBSourceCodePath = ""
		self.ranRepository = ""
		self.ranBranch = ""
		self.ranCommitID= ""
		self.ranAllowMerge= ""
		self.ranTargetBranch= ""
		self.exitStatus=0

#-----------------$
#PUBLIC Methods$
#-----------------$

	def Deploy_PhySim(self, HTML):
		if self.ranRepository == '' or self.ranBranch == '' or self.ranCommitID == '':
			HELP.GenericHelp(CONST.Version)
			sys.exit('Insufficient Parameter')
		lIpAddr = self.eNBIPAddress
		lUserName = self.eNBUserName
		lPassWord = self.eNBPassword
		lSourcePath = self.eNBSourceCodePath

		if lIpAddr == '' or lUserName == '' or lPassWord == '' or lSourcePath == '':
			HELP.GenericHelp(CONST.Version)
			sys.exit('Insufficient Parameter')
		logging.debug('Building on server: ' + lIpAddr)
		mySSH = SSH.SSHConnection()
		mySSH.open(lIpAddr, lUserName, lPassWord)

		self.testCase_id = HTML.testCase_id

		# on RedHat/CentOS .git extension is mandatory
		result = re.search('([a-zA-Z0-9\:\-\.\/])+\.git', self.ranRepository)
		if result is not None:
			full_ran_repo_name = self.ranRepository
		else:
			full_ran_repo_name = self.ranRepository + '.git'
		mySSH.command('mkdir -p ' + lSourcePath, '\$', 5)
		mySSH.command('cd ' + lSourcePath, '\$', 5)
		mySSH.command('if [ ! -e .git ]; then stdbuf -o0 git clone ' + full_ran_repo_name + ' .; else stdbuf -o0 git fetch --prune; fi', '\$', 600)
		# Raphael: here add a check if git clone or git fetch went smoothly
		mySSH.command('git config user.email "jenkins@openairinterface.org"', '\$', 5)
		mySSH.command('git config user.name "OAI Jenkins"', '\$', 5)

		mySSH.command('echo ' + lPassWord + ' | sudo -S git clean -x -d -ff', '\$', 30)
		mySSH.command('mkdir -p cmake_targets/log', '\$', 5)
		# if the commit ID is provided use it to point to it
		if self.ranCommitID != '':
			mySSH.command('git checkout -f ' + self.ranCommitID, '\$', 5)

		mySSH.command("sudo podman image inspect --format='Size = {{.Size}} bytes' oai-physim:ci-temp", '\$', 60)
		if mySSH.getBefore().count('no such image') != 0:
			logging.error('\u001B[1m No such image oai-physim\u001B[0m')
			sys.exit(-1)
		else:
			result = re.search('Size *= *(?P<size>[0-9\-]+) *bytes', mySSH.getBefore())
			if result is not None:
				imageSize = float(result.group('size'))
				imageSize = imageSize / 1000
				if imageSize < 1000:
					logging.debug('\u001B[1m   oai-physim size is ' + ('%.0f' % imageSize) + ' kbytes\u001B[0m')
				else:
					imageSize = imageSize / 1000
					if imageSize < 1000:
						logging.debug('\u001B[1m   oai-physim size is ' + ('%.0f' % imageSize) + ' Mbytes\u001B[0m')
					else:
						imageSize = imageSize / 1000
						logging.debug('\u001B[1m   oai-physim is ' + ('%.3f' % imageSize) + ' Gbytes\u001B[0m')
			else:
				logging.debug('oai-physim size is unknown')

		# logging to OC cluster
		mySSH.command(f'oc login -u {self.OCUserName} -p {self.OCPassword}', '\$', 6)
		print(mySSH.getBefore())
		if mySSH.getBefore().count('Login successful.') == 0:
			logging.error('\u001B[1m OC Cluster Login Failed\u001B[0m')
			sys.exit(-1)
		else:
			logging.debug('\u001B[1m   Login to OC Cluster Successfully\u001B[0m')
		mySSH.command(f'oc project {self.OCWorkspace}', '\$', 6)
		if (mySSH.getBefore().count(f'Already on project "{self.OCWorkspace}"')) == 0 or (mySSH.getBefore().count(f'Now using project "{self.OCWorkspace}"')) == 0:
			logging.error(f'\u001B[1m Unable to access OC project {self.OCWorkspace}\u001B[0m')
			sys.exit(-1)
		else:
			logging.debug(f'\u001B[1m   Now using project {self.OCWorkspace}\u001B[0m')
        
		# Using helm charts deployment
		mySSH.command('helm install physim ./charts/physims/', '\$', 6)
		if mySSH.getBefore().count('STATUS: deployed') == 0:
			logging.error('\u001B[1m Deploying PhySim Failed using helm chart\u001B[0m')
			sys.exit(-1)
		else:
			logging.debug('\u001B[1m   Deployed PhySim Successfully using helm chart\u001B[0m')
		isRunning = False
		while(isRunning == False):
			mySSH.command('oc get pods -l app.kubernetes.io/instance=physim', '\$', 6)
			if mySSH.getBefore().count('Running') == 12:
				logging.debug('\u001B[1m Running the physim test Scenarios\u001B[0m')
				isRunning = True
				podNames = re.findall('oai[\S\d\w]+', mySSH.getBefore())
		# Waiting to complete the running test
		count = 0
		isFinished = False
		while(count < 25 or isFinished == False):
			time.sleep(60)
			mySSH.command('oc get pods -l app.kubernetes.io/instance=physim', '\$', 6)
			result = re.search('oai-nr-dlsim[\S\d\w]+', mySSH.getBefore())
			if result is not None:
				podName1 = result.group()
				mySSH.command(f'oc logs {podName1}', '\$', 6)
				if mySSH.getBefore().count('Finished') != 0:
					isFinished = True
			count += 1
		if isFinished:
			logging.debug('\u001B[1m PhySim test is Complete\u001B[0m')
        
		# Getting the logs of each executables running in individual pods
		for podName in podNames:
			mySSH.command(f'oc logs {podName} >> cmake_targets/log/physim_test.txt 2>&1', '\$', 6)
		mySSH.command('cd ' + lSourcePath + '/cmake_targets', '\$', 5)
		mySSH.command('mkdir -p physim_test_log_' + self.testCase_id, '\$', 5)
		mySSH.command('mv log/* ' + 'physim_test_log_' + self.testCase_id, '\$', 5)
		# UnDeploy the physical simulator pods
		mySSH.command('helm uninstall physim', '\$', 10)
		if mySSH.getBefore().count('release "physim" uninstalled') != 0:
			logging.debug('\u001B[1m UnDeployed PhySim Successfully on OC Cluster\u001B[0m')
		else:
			logging.debug('\u001B[1m Failed to UnDeploy PhySim on OC Cluster\u001B[0m')
		mySSH.close()
		mySSH.copyin(lIpAddr, lUserName, lPassWord, lSourcePath + '/cmake_targets/physim_test_log_' + self.testCase_id + '/*', '.')
		testResult = {}
		testCount = 0
		if (os.path.isfile('./physim_test.txt')):
			with open('./physim_test.txt', 'r') as logfile:
				for line in logfile:
					ret = re.search('execution', str(line))
					if ret is not None:
						testCount += 1
						testName = line.split()
						ret1 = re.search('Result = PASS', str(line))
						if ret1 is not None:
							testResult[testName[1]] = 'PASS'
						else:
							testResult[testName[1]] = 'FAIL'				
		
		HTML.CreateHtmlTestRow('N/A', 'OK', CONST.ALL_PROCESSES_OK)
		HTML.CreateHtmlTestRowPhySimTestResult(testResult)
		logging.info('\u001B[1m Physical Simulator Pass\u001B[0m')

		return 0


            

