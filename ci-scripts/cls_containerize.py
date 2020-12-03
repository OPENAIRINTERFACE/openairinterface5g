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
import epc 
import helpreadme as HELP
import constants as CONST
import html

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
		self.testCase_id = ''
		self.htmlObj = None
		self.epcObj = None

#-----------------------------------------------------------
# Container management functions
#-----------------------------------------------------------

	def BuildImage(self):
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
		logging.debug('lIpAddr = ' + lIpAddr + ' lUserName = ' + lUserName + ' lPassWord = ' + lPassWord + ' lSourcePath = ' + lSourcePath)
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
		if self.htmlObj is not None:
			self.testCase_id = self.htmlObj.testCase_id
		else:
			self.testCase_id = '000000'
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
			if self.htmlObj is not None:
				self.htmlObj.CreateHtmlTestRow(self.imageKind, 'KO', CONST.ALL_PROCESSES_OK)
				self.htmlObj.CreateHtmlTabFooter(False)
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
		if self.htmlObj is not None:
			self.htmlObj.CreateHtmlTestRow(self.imageKind, 'OK', CONST.ALL_PROCESSES_OK)		

