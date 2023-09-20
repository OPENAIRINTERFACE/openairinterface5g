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
import os
import re
import time
import subprocess
import sys

import sshconnection as SSH
import cls_oai_html
import constants as CONST
import helpreadme as HELP

class PhySim:
	def __init__(self):
		self.eNBIpAddr = ""
		self.eNBUserName = ""
		self.eNBPassword = ""
		self.OCUserName = ""
		self.OCPassword = ""
		self.OCProjectName = ""
		self.eNBSourceCodePath = ""
		self.ranRepository = ""
		self.ranBranch = ""
		self.ranCommitID= ""
		self.ranAllowMerge= False
		self.ranTargetBranch= ""
		self.testResult = {}
		self.testSummary = {}
		self.testStatus = False

#-----------------$
#PUBLIC Methods$
#-----------------$

	def Deploy_PhySim(self, HTML, RAN):
		if self.ranRepository == '' or self.ranBranch == '' or self.ranCommitID == '':
			HELP.GenericHelp(CONST.Version)
			sys.exit('Insufficient Parameter')
		lIpAddr = self.eNBIPAddress
		lUserName = self.eNBUserName
		lPassWord = self.eNBPassword
		lSourcePath = self.eNBSourceCodePath
		ocUserName = self.OCUserName
		ocPassword = self.OCPassword
		ocProjectName = self.OCProjectName

		if lIpAddr == '' or lUserName == '' or lPassWord == '' or lSourcePath == '' or ocUserName == '' or ocPassword == '' or ocProjectName == '':
			HELP.GenericHelp(CONST.Version)
			sys.exit('Insufficient Parameter')
		logging.debug('Building on server: ' + lIpAddr)
		mySSH = SSH.SSHConnection()
		mySSH.open(lIpAddr, lUserName, lPassWord)

		self.testCase_id = HTML.testCase_id

		# on RedHat/CentOS .git extension is mandatory
		result = re.search('([a-zA-Z0-9\:\-\.\/])+\.git', self.ranRepository)
		if result is not None:
			full_ran_repo_name = self.ranRepository.replace('git/', 'git')
		else:
			full_ran_repo_name = self.ranRepository + '.git'
		mySSH.command('rm -Rf ' + lSourcePath, '\$', 30)
		mySSH.command('mkdir -p ' + lSourcePath, '\$', 5)
		mySSH.command('cd ' + lSourcePath, '\$', 5)
		mySSH.command('if [ ! -e .git ]; then stdbuf -o0 git clone ' + full_ran_repo_name + ' .; else stdbuf -o0 git fetch --prune; fi', '\$', 600)
		# Raphael: here add a check if git clone or git fetch went smoothly
		mySSH.command('git config user.email "jenkins@openairinterface.org"', '\$', 5)
		mySSH.command('git config user.name "OAI Jenkins"', '\$', 5)

		mySSH.command('git clean -x -d -ff', '\$', 30)
		mySSH.command('mkdir -p cmake_targets/log', '\$', 5)
		# if the commit ID is provided use it to point to it
		if self.ranCommitID != '':
			mySSH.command('git checkout -f ' + self.ranCommitID, '\$', 30)
		if self.ranAllowMerge:
			branchName = self.ranBranch.replace('/','-')
			imageTag = f'{branchName}-{self.ranCommitID[0:8]}'
			if self.ranTargetBranch == '':
				if (self.ranBranch != 'develop') and (self.ranBranch != 'origin/develop'):
					mySSH.command('git merge --ff origin/develop -m "Temporary merge for CI"', '\$', 30)
			else:
				logging.debug('Merging with the target branch: ' + self.ranTargetBranch)
				mySSH.command('git merge --ff origin/' + self.ranTargetBranch + ' -m "Temporary merge for CI"', '\$', 30)
		else:
			imageTag = f'develop-{self.ranCommitID[0:8]}'

		# logging to OC Cluster and then switch to corresponding project
		mySSH.command(f'oc login -u {ocUserName} -p {ocPassword} --server https://api.oai.cs.eurecom.fr:6443', '\$', 30)
		if mySSH.getBefore().count('Login successful.') == 0:
			logging.error('\u001B[1m OC Cluster Login Failed\u001B[0m')
			mySSH.close()
			HTML.CreateHtmlTestRow('N/A', 'KO', CONST.OC_LOGIN_FAIL)
			RAN.prematureExit = True
			return
		else:
			logging.debug('\u001B[1m   Login to OC Cluster Successfully\u001B[0m')
		mySSH.command(f'oc project {ocProjectName}', '\$', 30)
		if mySSH.getBefore().count(f'Already on project "{ocProjectName}"') == 0 and mySSH.getBefore().count(f'Now using project "{self.OCProjectName}"') == 0:
			logging.error(f'\u001B[1m Unable to access OC project {ocProjectName}\u001B[0m')
			mySSH.command('oc logout', '\$', 30)
			mySSH.close()
			HTML.CreateHtmlTestRow('N/A', 'KO', CONST.OC_PROJECT_FAIL)
			RAN.prematureExit = True
			return
		else:
			logging.debug(f'\u001B[1m   Now using project {ocProjectName}\u001B[0m')

		# Using helm charts deployment
		mySSH.command(f'grep -rl OAICICD_PROJECT ./charts/ | xargs sed -i -e "s#OAICICD_PROJECT#{ocProjectName}#"', '\$', 30)
		mySSH.command(f'sed -i -e "s#TAG#{imageTag}#g" ./charts/physims/values.yaml', '\$', 6)
		mySSH.command('helm install physim ./charts/physims/ --wait 2>&1 | tee -a cmake_targets/log/physim_helm_summary.txt', '\$', 30)
		if mySSH.getBefore().count('STATUS: deployed') == 0:
			logging.error('\u001B[1m Deploying PhySim Failed using helm chart on OC Cluster\u001B[0m')
			mySSH.command('helm uninstall physim | tee -a cmake_targets/log/physim_helm_summary.txt 2>&1', '\$', 30)
			isFinished1 = False
			while(isFinished1 == False):
				time.sleep(20)
				mySSH.command('oc get pods -l app=physim', '\$', 6, resync=True)
				if re.search('No resources found', mySSH.getBefore()):
					isFinished1 = True
			mySSH.command('oc logout', '\$', 30)
			mySSH.close()
			self.AnalyzeLogFile_phySim()
			RAN.prematureExit = True
			return
		else:
			logging.debug('\u001B[1m   Deployed PhySim Successfully using helm chart\u001B[0m')
		isRunning = False
		count = 0
		# Check whether the containers are in Running state or not under 2 mins
		while(count < 5 and isRunning == False):
			time.sleep(60)
			mySSH.command('oc get pods -o wide -l app=physim | tee -a cmake_targets/log/physim_pods_summary.txt', '\$', 30, resync=True)
			running_count = mySSH.getBefore().count('Running')
			completed_count = mySSH.getBefore().count('Completed')
			if (running_count + completed_count) == 21:
				logging.debug('\u001B[1m Running the physim test Scenarios\u001B[0m')
				isRunning = True
				podNames = re.findall('oai-[\S\d\w]+', mySSH.getBefore())
			count +=1
		if isRunning == False:
			logging.error('\u001B[1m Some pods are in Error state \u001B[0m')
			mySSH.command('oc get pods -l app=physim 2>&1 | tee -a cmake_targets/log/physim_pods_summary.txt', '\$', 6)
			mySSH.command('for pod in $(oc get pods | tail -n +2 | awk \'{print $1}\'); do oc describe pod $pod >> cmake_targets/log/physim_pods_summary.txt; done', '\$', 10)
			mySSH.command('helm uninstall physim | tee -a cmake_targets/log/physim_helm_summary.txt 2>&1', '\$', 6)
			self.AnalyzeLogFile_phySim()
			isFinished1 = False
			while(isFinished1 == False):
				time.sleep(20)
				mySSH.command('oc get pods -l app=physim', '\$', 6, resync=True)
				if re.search('No resources found', mySSH.getBefore()):
					isFinished1 = True
			mySSH.command('oc logout', '\$', 30)
			HTML.CreateHtmlTestRow('N/A', 'KO', CONST.OC_PHYSIM_DEPLOY_FAIL)
			HTML.CreateHtmlTestRowPhySimTestResult(self.testSummary,self.testResult)
			RAN.prematureExit = True
			return
		# Waiting to complete the running test
		count = 0
		isFinished = False
		# doing a deep copy!
		tmpPodNames = podNames.copy()
		while(count < 9 and isFinished == False):
			time.sleep(60)
			for podName in tmpPodNames:
				mySSH.command2(f'oc logs --tail=1 {podName} 2>&1', 6, silent=True)
				if mySSH.cmd2Results.count('FINISHED') != 0:
					logging.debug(podName + ' is finished')
					tmpPodNames.remove(podName)
			if not tmpPodNames:
				isFinished = True
			count += 1
		if isFinished:
			logging.debug('\u001B[1m PhySim test is Complete\u001B[0m')
		else:
			logging.error('\u001B[1m PhySim test Timed-out!\u001B[0m')

		# Getting the logs of each executables running in individual pods
		for podName in podNames:
			mySSH.command(f'oc logs {podName} >> cmake_targets/log/physim_test.txt 2>&1', '\$', 15, resync=True)
		mySSH.command('for pod in $(oc get pods | tail -n +2 | awk \'{print $1}\'); do oc describe pod $pod >> cmake_targets/log/physim_pods_summary.txt; done', '\$', 10)
		mySSH.copyin(lIpAddr, lUserName, lPassWord, lSourcePath + '/cmake_targets/log/physim_test.txt', '.')
		try:
			listLogFiles =  subprocess.check_output('egrep --colour=never "Execution Log file|Linux oai-" physim_test.txt', shell=True, universal_newlines=True)
			for line in listLogFiles.split('\n'):
				res1 = re.search('Linux (?P<pod>oai-[a-zA-Z0-9\-]+) ', str(line))
				res2 = re.search('Execution Log file = (?P<name>[a-zA-Z0-9\-\/\.\_\+]+)', str(line))
				if res1 is not None:
					podName = res1.group('pod')
				if res2 is not None:
					logFileInPod = res2.group('name')
					folderName = logFileInPod.replace('/opt/oai-physim/cmake_targets/autotests/log/', '')
					folderName = re.sub('/test.*', '', folderName)
					fileName = logFileInPod.replace('/opt/oai-physim/cmake_targets/autotests/log/' + folderName + '/', '')
					mySSH.command('mkdir -p cmake_targets/log/' + folderName, '\$', 5, silent=True)
					mySSH.command('oc cp ' + podName + ':' + logFileInPod + ' cmake_targets/log/' + folderName + '/' + fileName, '\$', 20, silent=True)
		except Exception as e:
			pass

		# UnDeploy the physical simulator pods
		mySSH.command('helm uninstall physim | tee -a cmake_targets/log/physim_helm_summary.txt 2>&1', '\$', 6)
		isFinished1 = False
		while(isFinished1 == False):
			time.sleep(20)
			mySSH.command('oc get pods -l app=physim', '\$', 6, resync=True)
			if re.search('No resources found', mySSH.getBefore()):
				isFinished1 = True
		if isFinished1 == True:
			logging.debug('\u001B[1m UnDeployed PhySim Successfully on OC Cluster\u001B[0m')
		mySSH.command('oc logout', '\$', 6)
		mySSH.close()
		self.AnalyzeLogFile_phySim()
		if self.testStatus and isFinished:
			HTML.CreateHtmlTestRow('N/A', 'OK', CONST.ALL_PROCESSES_OK)
			HTML.CreateHtmlTestRowPhySimTestResult(self.testSummary,self.testResult)
			logging.info('\u001B[1m Physical Simulator Pass\u001B[0m')
		else:
			RAN.prematureExit = True
			if isFinished:
				HTML.CreateHtmlTestRow('N/A', 'KO', CONST.ALL_PROCESSES_OK)
			else:
				HTML.CreateHtmlTestRow('Some test(s) timed-out!', 'KO', CONST.ALL_PROCESSES_OK)
			HTML.CreateHtmlTestRowPhySimTestResult(self.testSummary,self.testResult)
			logging.error('\u001B[1m Physical Simulator Fail\u001B[0m')

	def AnalyzeLogFile_phySim(self):
		mySSH = SSH.SSHConnection()
		mySSH.open(self.eNBIPAddress, self.eNBUserName, self.eNBPassword)
		dirToCopy = f'{self.eNBSourceCodePath}/cmake_targets/log/'
		mySSH.copyin(self.eNBIPAddress, self.eNBUserName, self.eNBPassword, dirToCopy, f'./physim_test_logs_{self.testCase_id}/')
		mySSH.command(f'rm -rf {dirToCopy}', '\$', 5)
		mySSH.close()
		# physim test log analysis
		nextt = 0
		nbTests = 0
		nbFailed = 0
		if (os.path.isfile(f'./physim_test_logs_{self.testCase_id}/physim_test.txt')):
			with open(f'./physim_test_logs_{self.testCase_id}/physim_test.txt', 'r') as logfile:
				for line in logfile:
					# the following regex would match the following line:
					# execution nr_pbchsim.106rb.test1 {Test1: PBCH-only, 106 PRB} Run_Result = " Run_1 =PASS Run_2 =PASS Run_3 =PASS"  Result = PASS
					#           ^testname               ^testdescription                                 ^test1      ^test2      ^test3          ^status
					ret = re.search('execution\s+(?P<name>[a-zA-Z0-9\._\-\+]+)\s+{(?P<desc>[A-Za-z0-9\.\_\-\+:,;\/\%\=\(\)\s]+)}\s+Run_Result\s*=\s*\"\s*Run_1\s*=\s*(?P<run1>[A-Za-z]+)\s*Run_2\s*=\s*(?P<run2>[A-Za-z]+)\s*Run_3\s*=\s*(?P<run3>[A-Za-z]+)\s*\"\s+Result\s*=\s*(?P<status>[A-Za-z]+)', line)
					if ret is None:
						continue
					nbTests += 1
					r = [ret.group('run1'), ret.group('run2'), ret.group('run3')]
					nbPass = sum([x == 'PASS' for x in r])
					resultstr = 'PASS'
					if nbPass < 3 or ret.group('status') != 'PASS':
						resultstr = f'FAIL ({3-nbPass}/3)'
						nbFailed += 1
					self.testResult[ret.group('name')] = [ret.group('desc'), resultstr]
		self.testSummary['Nbtests'] = nbTests
		self.testSummary['Nbpass'] =  nbTests - nbFailed
		self.testSummary['Nbfail'] =  nbFailed
		if self.testSummary['Nbfail'] == 0:
			self.testStatus = True
		return 0
