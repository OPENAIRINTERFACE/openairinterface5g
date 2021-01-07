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
import sys              # arg
import re               # reg
import logging
import os
import time
from multiprocessing import Process, Lock, SimpleQueue

#-----------------------------------------------------------
# OAI Testing modules
#-----------------------------------------------------------
import sshconnection as SSH
import helpreadme as HELP
import constants as CONST

#-----------------------------------------------------------
# Class Declaration
#-----------------------------------------------------------
class Containerize():

	def __init__(self):
		
		self.ranRepository = ''
		self.ranBranch = ''
		self.ranAllowMerge = False
		self.ranCommitID = ''
		self.ranTargetBranch = ''
		self.eNBIPAddress = ''
		self.eNBUserName = ''
		self.eNBPassword = ''
		self.eNBSourceCodePath = ''
		self.eNB1IPAddress = ''
		self.eNB1UserName = ''
		self.eNB1Password = ''
		self.eNB1SourceCodePath = ''
		self.eNB2IPAddress = ''
		self.eNB2UserName = ''
		self.eNB2Password = ''
		self.eNB2SourceCodePath = ''
		self.forcedWorkspaceCleanup = False
		self.imageKind = ''
		self.eNB_instance = 0
		self.eNB_serverId = ['', '', '']
		self.yamlPath = ['', '', '']
		self.eNB_logFile = ['', '', '']

		self.testCase_id = ''

		self.flexranCtrlDeployed = False
		self.flexranCtrlIpAddress = ''

#-----------------------------------------------------------
# Container management functions
#-----------------------------------------------------------

	def BuildImage(self, HTML):
		if self.ranRepository == '' or self.ranBranch == '' or self.ranCommitID == '':
			HELP.GenericHelp(CONST.Version)
			sys.exit('Insufficient Parameter')
		if self.eNB_serverId[self.eNB_instance] == '0':
			lIpAddr = self.eNBIPAddress
			lUserName = self.eNBUserName
			lPassWord = self.eNBPassword
			lSourcePath = self.eNBSourceCodePath
		elif self.eNB_serverId[self.eNB_instance] == '1':
			lIpAddr = self.eNB1IPAddress
			lUserName = self.eNB1UserName
			lPassWord = self.eNB1Password
			lSourcePath = self.eNB1SourceCodePath
		elif self.eNB_serverId[self.eNB_instance] == '2':
			lIpAddr = self.eNB2IPAddress
			lUserName = self.eNB2UserName
			lPassWord = self.eNB2Password
			lSourcePath = self.eNB2SourceCodePath
		if lIpAddr == '' or lUserName == '' or lPassWord == '' or lSourcePath == '':
			HELP.GenericHelp(CONST.Version)
			sys.exit('Insufficient Parameter')
		logging.debug('Building on server: ' + lIpAddr)
		mySSH = SSH.SSHConnection()
		mySSH.open(lIpAddr, lUserName, lPassWord)

		imageNames = []
		result = re.search('eNB', self.imageKind)
		# Creating a tupple with the imageName and the DockerFile prefix pattern
		if result is not None:
			imageNames.append(('oai-enb', 'eNB'))
		else:
			result = re.search('gNB', self.imageKind)
			if result is not None:
				imageNames.append(('oai-gnb', 'gNB'))
			else:
				result = re.search('all', self.imageKind)
				if result is not None:
					imageNames.append(('oai-enb', 'eNB'))
					imageNames.append(('oai-gnb', 'gNB'))
					imageNames.append(('oai-lte-ue', 'lteUE'))
					imageNames.append(('oai-nr-ue', 'nrUE'))
		if len(imageNames) == 0:
			imageNames.append(('oai-enb', 'eNB'))
		# Workaround for some servers, we need to erase completely the workspace
		if self.forcedWorkspaceCleanup:
			mySSH.command('echo ' + lPassWord + ' | sudo -S rm -Rf ' + lSourcePath, '\$', 15)
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
		# if the branch is not develop, then it is a merge request and we need to do 
		# the potential merge. Note that merge conflicts should already been checked earlier
		imageTag = 'develop'
		if (self.ranAllowMerge):
			imageTag = 'ci-temp'
			if self.ranTargetBranch == '':
				if (self.ranBranch != 'develop') and (self.ranBranch != 'origin/develop'):
					mySSH.command('git merge --ff origin/develop -m "Temporary merge for CI"', '\$', 5)
			else:
				logging.debug('Merging with the target branch: ' + self.ranTargetBranch)
				mySSH.command('git merge --ff origin/' + self.ranTargetBranch + ' -m "Temporary merge for CI"', '\$', 5)
		# Let's remove any previous run artifacts if still there
		mySSH.command('docker image prune --force', '\$', 5)
		mySSH.command('docker image rm ran-build:' + imageTag, '\$', 5)
		for image,pattern in imageNames:
			mySSH.command('docker image rm ' + image + ':' + imageTag, '\$', 5)
		# Build the shared image
		mySSH.command('docker build --target ran-build --tag ran-build:' + imageTag + ' --file docker/Dockerfile.ran.ubuntu18 --build-arg NEEDED_GIT_PROXY="http://proxy.eurecom.fr:8080" . > cmake_targets/log/ran-build.log 2>&1', '\$', 800)
		# Build the target image(s)
		previousImage='ran-build:' + imageTag
		danglingShaOnes=[]
		for image,pattern in imageNames:
			# the archived Dockerfiles have "ran-build:latest" as base image
			# we need to update them with proper tag
			mySSH.command('sed -i -e "s#ran-build:latest#ran-build:' + imageTag + '#" docker/Dockerfile.' + pattern + '.ubuntu18', '\$', 5)
			mySSH.command('docker build --target ' + image + ' --tag ' + image + ':' + imageTag + ' --file docker/Dockerfile.' + pattern + '.ubuntu18 . > cmake_targets/log/' + image + '.log 2>&1', '\$', 600)
			# Retrieving the dangling image(s) for the log collection
			mySSH.command('docker images --filter "dangling=true" --filter "since=' + previousImage + '" -q | sed -e "s#^#sha=#"', '\$', 5)
			result = re.search('sha=(?P<imageShaOne>[a-zA-Z0-9\-\_]+)', mySSH.getBefore())
			if result is not None:
				danglingShaOnes.append((image, result.group('imageShaOne')))
			previousImage=image + ':' + imageTag

		imageTag = 'ci-temp'
		# First verify if images were properly created.
		status = True
		mySSH.command('docker image inspect --format=\'Size = {{.Size}} bytes\' ran-build:' + imageTag, '\$', 5)
		if mySSH.getBefore().count('No such object') != 0:
			logging.error('Could not build properly ran-build')
			status = False
		else:
			result = re.search('Size = (?P<size>[0-9\-]+) bytes', mySSH.getBefore())
			if result is not None:
				imageSize = float(result.group('size'))
				imageSize = imageSize / 1000
				if imageSize < 1000:
					logging.debug('\u001B[1m   ran-build size is ' + ('%.0f' % imageSize) + ' kbytes\u001B[0m')
				else:
					imageSize = imageSize / 1000
					if imageSize < 1000:
						logging.debug('\u001B[1m   ran-build size is ' + ('%.0f' % imageSize) + ' Mbytes\u001B[0m')
					else:
						imageSize = imageSize / 1000
						logging.debug('\u001B[1m   ran-build size is ' + ('%.3f' % imageSize) + ' Gbytes\u001B[0m')
			else:
				logging.debug('ran-build size is unknown')
		for image,pattern in imageNames:
			mySSH.command('docker image inspect --format=\'Size = {{.Size}} bytes\' ' + image + ':' + imageTag, '\$', 5)
			if mySSH.getBefore().count('No such object') != 0:
				logging.error('Could not build properly ' + image)
				status = False
			else:
				result = re.search('Size = (?P<size>[0-9\-]+) bytes', mySSH.getBefore())
				if result is not None:
					imageSize = float(result.group('size'))
					imageSize = imageSize / 1000
					if imageSize < 1000:
						logging.debug('\u001B[1m   ' + image + ' size is ' + ('%.0f' % imageSize) + ' kbytes\u001B[0m')
					else:
						imageSize = imageSize / 1000
						if imageSize < 1000:
							logging.debug('\u001B[1m   ' + image + ' size is ' + ('%.0f' % imageSize) + ' Mbytes\u001B[0m')
						else:
							imageSize = imageSize / 1000
							logging.debug('\u001B[1m   ' + image + ' size is ' + ('%.3f' % imageSize) + ' Gbytes\u001B[0m')
				else:
					logging.debug('ran-build size is unknown')
		if not status:
			mySSH.close()
			logging.error('\u001B[1m Building OAI Images Failed\u001B[0m')
			HTML.CreateHtmlTestRow(self.imageKind, 'KO', CONST.ALL_PROCESSES_OK)
			HTML.CreateHtmlTabFooter(False)
			sys.exit(1)

		# Recover build logs, for the moment only possible when build is successful
		mySSH.command('docker create --name test ran-build:' + imageTag, '\$', 5)
		mySSH.command('mkdir -p cmake_targets/log/ran-build', '\$', 5)
		mySSH.command('docker cp test:/oai-ran/cmake_targets/log/. cmake_targets/log/ran-build', '\$', 5)
		mySSH.command('docker rm -f test', '\$', 5)
		for image,shaone in danglingShaOnes:
			mySSH.command('mkdir -p cmake_targets/log/' + image, '\$', 5)
			mySSH.command('docker create --name test ' + shaone, '\$', 5)
			mySSH.command('docker cp test:/oai-ran/cmake_targets/log/. cmake_targets/log/' + image, '\$', 5)
			mySSH.command('docker rm -f test', '\$', 5)
		mySSH.command('docker image prune --force', '\$', 5)
		mySSH.command('cd cmake_targets', '\$', 5)
		mySSH.command('mkdir -p build_log_' + self.testCase_id, '\$', 5)
		mySSH.command('mv log/* ' + 'build_log_' + self.testCase_id, '\$', 5)
		mySSH.close()

		logging.info('\u001B[1m Building OAI Image(s) Pass\u001B[0m')
		HTML.CreateHtmlTestRow(self.imageKind, 'OK', CONST.ALL_PROCESSES_OK)

	def DeployObject(self, HTML, EPC):
		if self.eNB_serverId[self.eNB_instance] == '0':
			lIpAddr = self.eNBIPAddress
			lUserName = self.eNBUserName
			lPassWord = self.eNBPassword
			lSourcePath = self.eNBSourceCodePath
		elif self.eNB_serverId[self.eNB_instance] == '1':
			lIpAddr = self.eNB1IPAddress
			lUserName = self.eNB1UserName
			lPassWord = self.eNB1Password
			lSourcePath = self.eNB1SourceCodePath
		elif self.eNB_serverId[self.eNB_instance] == '2':
			lIpAddr = self.eNB2IPAddress
			lUserName = self.eNB2UserName
			lPassWord = self.eNB2Password
			lSourcePath = self.eNB2SourceCodePath
		if lIpAddr == '' or lUserName == '' or lPassWord == '' or lSourcePath == '':
			HELP.GenericHelp(CONST.Version)
			sys.exit('Insufficient Parameter')
		logging.debug('\u001B[1m Deploying OAI Object on server: ' + lIpAddr + '\u001B[0m')
		mySSH = SSH.SSHConnection()
		mySSH.open(lIpAddr, lUserName, lPassWord)
		# Putting the CPUs in a good state, we do that only on a few servers
		mySSH.command('hostname', '\$', 5)
		result = re.search('obelix|asterix',  mySSH.getBefore())
		if result is not None:
			mySSH.command('if command -v cpupower &> /dev/null; then echo ' + lPassWord + ' | sudo -S cpupower idle-set -D 0; fi', '\$', 5)
			time.sleep(5)
		
		mySSH.command('cd ' + lSourcePath + '/' + self.yamlPath[self.eNB_instance], '\$', 5)
		mySSH.command('cp docker-compose.yml ci-docker-compose.yml', '\$', 5)
		imageTag = 'develop'
		if (self.ranAllowMerge):
			imageTag = 'ci-temp'
		mySSH.command('sed -i -e "s/image: oai-enb:latest/image: oai-enb:' + imageTag + '/" ci-docker-compose.yml', '\$', 2)
		localMmeIpAddr = EPC.MmeIPAddress
		mySSH.command('sed -i -e "s/CI_MME_IP_ADDR/' + localMmeIpAddr + '/" ci-docker-compose.yml', '\$', 2)
		if self.flexranCtrlDeployed:
			mySSH.command('sed -i -e \'s/FLEXRAN_ENABLED:.*/FLEXRAN_ENABLED: "yes"/\' ci-docker-compose.yml', '\$', 2)
			mySSH.command('sed -i -e "s/CI_FLEXRAN_CTL_IP_ADDR/' + self.flexranCtrlIpAddress + '/" ci-docker-compose.yml', '\$', 2)
		else:
			mySSH.command('sed -i -e "s/FLEXRAN_ENABLED:.*$/FLEXRAN_ENABLED: \"no\"/" ci-docker-compose.yml', '\$', 2)
			mySSH.command('sed -i -e "s/CI_FLEXRAN_CTL_IP_ADDR/127.0.0.1/" ci-docker-compose.yml', '\$', 2)
		# Currently support only one
		mySSH.command('docker-compose --file ci-docker-compose.yml config --services | sed -e "s@^@service=@"', '\$', 2)
		result = re.search('service=(?P<svc_name>[a-zA-Z0-9\_]+)', mySSH.getBefore())
		if result is not None:
			svcName = result.group('svc_name')
			mySSH.command('docker-compose --file ci-docker-compose.yml up -d ' + svcName, '\$', 2)

		# Checking Status
		mySSH.command('docker-compose --file ci-docker-compose.yml config', '\$', 5)
		result = re.search('container_name: (?P<container_name>[a-zA-Z0-9\-\_]+)', mySSH.getBefore())
		unhealthyNb = 0
		healthyNb = 0
		startingNb = 0
		containerName = ''
		if result is not None:
			containerName = result.group('container_name')
			time.sleep(5)
			cnt = 0
			while (cnt < 3):
				mySSH.command('docker inspect --format=\'{{.State.Health.Status}}\' ' + containerName, '\$', 5)
				unhealthyNb = mySSH.getBefore().count('unhealthy')
				healthyNb = mySSH.getBefore().count('healthy') - unhealthyNb
				startingNb = mySSH.getBefore().count('starting')
				if healthyNb == 1:
					cnt = 10
				else:
					time.sleep(10)
					cnt += 1
		logging.debug(' -- ' + str(healthyNb) + ' healthy container(s)')
		logging.debug(' -- ' + str(unhealthyNb) + ' unhealthy container(s)')
		logging.debug(' -- ' + str(startingNb) + ' still starting container(s)')

		status = False
		if healthyNb == 1:
			cnt = 0
			while (cnt < 20):
				mySSH.command('docker logs ' + containerName + ' | egrep --text --color=never -i "wait|sync|Starting"', '\$', 30) 
				result = re.search('got sync|Starting F1AP at CU', mySSH.getBefore())
				if result is None:
					time.sleep(6)
					cnt += 1
				else:
					cnt = 100
					status = True
					logging.info('\u001B[1m Deploying OAI object Pass\u001B[0m')
					time.sleep(10)
		mySSH.close()

		self.testCase_id = HTML.testCase_id
		self.eNB_logFile[self.eNB_instance] = 'enb_' + self.testCase_id + '.log'

		if status:
			HTML.CreateHtmlTestRow('N/A', 'OK', CONST.ALL_PROCESSES_OK)
		else:
			HTML.CreateHtmlTestRow('N/A', 'KO', CONST.ALL_PROCESSES_OK)

	def UndeployObject(self, HTML, RAN):
		logging.info('\u001B[1m Undeploying OAI Object Pass\u001B[0m')
		if self.eNB_serverId[self.eNB_instance] == '0':
			lIpAddr = self.eNBIPAddress
			lUserName = self.eNBUserName
			lPassWord = self.eNBPassword
			lSourcePath = self.eNBSourceCodePath
		elif self.eNB_serverId[self.eNB_instance] == '1':
			lIpAddr = self.eNB1IPAddress
			lUserName = self.eNB1UserName
			lPassWord = self.eNB1Password
			lSourcePath = self.eNB1SourceCodePath
		elif self.eNB_serverId[self.eNB_instance] == '2':
			lIpAddr = self.eNB2IPAddress
			lUserName = self.eNB2UserName
			lPassWord = self.eNB2Password
			lSourcePath = self.eNB2SourceCodePath
		if lIpAddr == '' or lUserName == '' or lPassWord == '' or lSourcePath == '':
			HELP.GenericHelp(CONST.Version)
			sys.exit('Insufficient Parameter')
		logging.debug('\u001B[1m Deploying OAI Object on server: ' + lIpAddr + '\u001B[0m')
		mySSH = SSH.SSHConnection()
		mySSH.open(lIpAddr, lUserName, lPassWord)
		mySSH.command('cd ' + lSourcePath + '/' + self.yamlPath[self.eNB_instance], '\$', 5)
		# Currently support only one
		mySSH.command('docker-compose --file ci-docker-compose.yml config', '\$', 5)
		result = re.search('container_name: (?P<container_name>[a-zA-Z0-9\-\_]+)', mySSH.getBefore())
		if result is not None:
			containerName = result.group('container_name')
			mySSH.command('docker kill --signal INT ' + containerName, '\$', 30)
			time.sleep(5)
			mySSH.command('docker logs ' + containerName + ' > ' + lSourcePath + '/cmake_targets/' + self.eNB_logFile[self.eNB_instance], '\$', 30)
			mySSH.command('docker rm -f ' + containerName, '\$', 30)

		# Putting the CPUs back in a idle state, we do that only on a few servers
		mySSH.command('hostname', '\$', 5)
		result = re.search('obelix|asterix',  mySSH.getBefore())
		if result is not None:
			mySSH.command('if command -v cpupower &> /dev/null; then echo ' + lPassWord + ' | sudo -S cpupower idle-set -E; fi', '\$', 5)
		mySSH.close()

		# Analyzing log file!
		copyin_res = mySSH.copyin(lIpAddr, lUserName, lPassWord, lSourcePath + '/cmake_targets/' + self.eNB_logFile[self.eNB_instance], '.')
		nodeB_prefix = 'e'
		if (copyin_res == -1):
			HTML.htmleNBFailureMsg='Could not copy ' + nodeB_prefix + 'NB logfile to analyze it!'
			HTML.CreateHtmlTestRow('N/A', 'KO', CONST.ENB_PROCESS_NOLOGFILE_TO_ANALYZE)
		else:
			logging.debug('\u001B[1m Analyzing ' + nodeB_prefix + 'NB logfile \u001B[0m ' + self.eNB_logFile[self.eNB_instance])
			logStatus = RAN.AnalyzeLogFile_eNB(self.eNB_logFile[self.eNB_instance], HTML)
			if (logStatus < 0):
				HTML.CreateHtmlTestRow(RAN.runtime_stats, 'KO', logStatus)
			else:
				HTML.CreateHtmlTestRow(RAN.runtime_stats, 'OK', CONST.ALL_PROCESSES_OK)

