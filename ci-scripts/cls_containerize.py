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
import shutil
import subprocess
import time
from multiprocessing import Process, Lock, SimpleQueue
from zipfile import ZipFile

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
		self.services = ['', '', '']
		self.nb_healthy = [0, 0, 0]
		self.exitStatus = 0
		self.eNB_logFile = ['', '', '']

		self.testCase_id = ''

		self.flexranCtrlDeployed = False
		self.flexranCtrlIpAddress = ''
		self.cli = ''
		self.cliBuildOptions = ''
		self.dockerfileprefix = ''
		self.host = ''
		self.allImagesSize = {}
		self.collectInfo = {}

		self.tsharkStarted = False
		self.pingContName = ''
		self.pingOptions = ''
		self.pingLossThreshold = ''
		self.svrContName = ''
		self.svrOptions = ''
		self.cliContName = ''
		self.cliOptions = ''

		self.imageToCopy = ''
		self.registrySvrId = ''
		self.testSvrId = ''

#-----------------------------------------------------------
# Container management functions
#-----------------------------------------------------------

	def _createWorkspace(self, sshSession, password, sourcePath):
		# on RedHat/CentOS .git extension is mandatory
		result = re.search('([a-zA-Z0-9\:\-\.\/])+\.git', self.ranRepository)
		if result is not None:
			full_ran_repo_name = self.ranRepository.replace('git/', 'git')
		else:
			full_ran_repo_name = self.ranRepository + '.git'
		sshSession.command('mkdir -p ' + sourcePath, '\$', 5)
		sshSession.command('cd ' + sourcePath, '\$', 5)
		sshSession.command('if [ ! -e .git ]; then stdbuf -o0 git clone ' + full_ran_repo_name + ' .; else stdbuf -o0 git fetch --prune; fi', '\$', 600)
		# Raphael: here add a check if git clone or git fetch went smoothly
		sshSession.command('git config user.email "jenkins@openairinterface.org"', '\$', 5)
		sshSession.command('git config user.name "OAI Jenkins"', '\$', 5)

		sshSession.command('echo ' + password + ' | sudo -S git clean -x -d -ff', '\$', 30)
		sshSession.command('mkdir -p cmake_targets/log', '\$', 5)
		# if the commit ID is provided use it to point to it
		if self.ranCommitID != '':
			sshSession.command('git checkout -f ' + self.ranCommitID, '\$', 30)
		# if the branch is not develop, then it is a merge request and we need to do
		# the potential merge. Note that merge conflicts should already been checked earlier
		if (self.ranAllowMerge):
			if self.ranTargetBranch == '':
				if (self.ranBranch != 'develop') and (self.ranBranch != 'origin/develop'):
					sshSession.command('git merge --ff origin/develop -m "Temporary merge for CI"', '\$', 5)
			else:
				logging.debug('Merging with the target branch: ' + self.ranTargetBranch)
				sshSession.command('git merge --ff origin/' + self.ranTargetBranch + ' -m "Temporary merge for CI"', '\$', 5)

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
	
		# Checking the hostname to get adapted on cli and dockerfileprefixes
		mySSH.command('hostnamectl', '\$', 5)
		result = re.search('Ubuntu|Red Hat',  mySSH.getBefore())
		self.host = result.group(0)
		if self.host == 'Ubuntu':
			self.cli = 'docker'
			self.dockerfileprefix = '.ubuntu18'
			self.cliBuildOptions = '--no-cache'
		elif self.host == 'Red Hat':
			self.cli = 'sudo podman'
			self.dockerfileprefix = '.rhel8.2'
			self.cliBuildOptions = '--no-cache --disable-compression'

		imageNames = []
		result = re.search('eNB', self.imageKind)
		# Creating a tupple with the imageName and the DockerFile prefix pattern on obelix
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
					if self.host == 'Red Hat':
						imageNames.append(('oai-physim', 'phySim'))
					if self.host == 'Ubuntu':
						imageNames.append(('oai-lte-ru', 'lteRU'))
		if len(imageNames) == 0:
			imageNames.append(('oai-enb', 'eNB'))
		
		# Workaround for some servers, we need to erase completely the workspace
		if self.forcedWorkspaceCleanup:
			mySSH.command('echo ' + lPassWord + ' | sudo -S rm -Rf ' + lSourcePath, '\$', 15)
	
		self.testCase_id = HTML.testCase_id
	
		self._createWorkspace(mySSH, lPassWord, lSourcePath)

 		# if asterix, copy the entitlement and subscription manager configurations
		if self.host == 'Red Hat':
			mySSH.command('mkdir -p  tmp/ca/', '\$', 5)
			mySSH.command('mkdir -p tmp/entitlement/', '\$', 5) 
			mySSH.command('sudo cp /etc/rhsm/ca/redhat-uep.pem tmp/ca/', '\$', 5)
			mySSH.command('sudo cp /etc/pki/entitlement/*.pem tmp/entitlement/', '\$', 5)

		sharedimage = 'ran-build'
		sharedTag = 'develop'
		forceSharedImageBuild = False
		imageTag = 'develop'
		if (self.ranAllowMerge):
			imageTag = 'ci-temp'
			if self.ranTargetBranch == 'develop':
				mySSH.command('git diff HEAD..origin/develop -- docker/Dockerfile.ran' + self.dockerfileprefix + ' | grep --colour=never -i INDEX', '\$', 5)
				result = re.search('index', mySSH.getBefore())
				if result is not None:
					forceSharedImageBuild = True
					sharedTag = 'ci-temp'

		# Let's remove any previous run artifacts if still there
		mySSH.command(self.cli + ' image prune --force', '\$', 30)
		if forceSharedImageBuild:
			mySSH.command(self.cli + ' image rm ' + sharedimage + ':' + sharedTag + ' || true', '\$', 30)
		for image,pattern in imageNames:
			mySSH.command(self.cli + ' image rm ' + image + ':' + imageTag + ' || true', '\$', 30)

		# Build the shared image only on Push Events (not on Merge Requests)
		# On when the shared image docker file is being modified.
		if forceSharedImageBuild:
			mySSH.command(self.cli + ' build ' + self.cliBuildOptions + ' --target ' + sharedimage + ' --tag ' + sharedimage + ':' + sharedTag + ' --file docker/Dockerfile.ran' + self.dockerfileprefix + ' --build-arg NEEDED_GIT_PROXY="http://proxy.eurecom.fr:8080" . > cmake_targets/log/ran-build.log 2>&1', '\$', 1600)
		# First verify if the shared image was properly created.
		status = True
		mySSH.command(self.cli + ' image inspect --format=\'Size = {{.Size}} bytes\' ' + sharedimage + ':' + sharedTag, '\$', 5)
		if mySSH.getBefore().count('o such image') != 0:
			logging.error('\u001B[1m Could not build properly ran-build\u001B[0m')
			status = False
		else:
			result = re.search('Size *= *(?P<size>[0-9\-]+) *bytes', mySSH.getBefore())
			if result is not None:
				imageSize = float(result.group('size'))
				imageSize = imageSize / 1000
				if imageSize < 1000:
					logging.debug('\u001B[1m   ran-build size is ' + ('%.0f' % imageSize) + ' kbytes\u001B[0m')
					self.allImagesSize['ran-build'] = str(round(imageSize,1)) + ' kbytes'
				else:
					imageSize = imageSize / 1000
					if imageSize < 1000:
						logging.debug('\u001B[1m   ran-build size is ' + ('%.0f' % imageSize) + ' Mbytes\u001B[0m')
						self.allImagesSize['ran-build'] = str(round(imageSize,1)) + ' Mbytes'
					else:
						imageSize = imageSize / 1000
						logging.debug('\u001B[1m   ran-build size is ' + ('%.3f' % imageSize) + ' Gbytes\u001B[0m')
						self.allImagesSize['ran-build'] = str(round(imageSize,1)) + ' Gbytes'
			else:
				logging.debug('ran-build size is unknown')
		# If the shared image failed, no need to continue
		if not status:
			# Recover the name of the failed container?
			mySSH.command(self.cli + ' ps --quiet --filter "status=exited" -n1 | xargs ' + self.cli + ' rm -f', '\$', 5)
			mySSH.command(self.cli + ' image prune --force', '\$', 30)
			mySSH.close()
			logging.error('\u001B[1m Building OAI Images Failed\u001B[0m')
			HTML.CreateHtmlTestRow(self.imageKind, 'KO', CONST.ALL_PROCESSES_OK)
			HTML.CreateHtmlTabFooter(False)
			sys.exit(1)
		else:
			# Recover build logs, for the moment only possible when build is successful
			mySSH.command(self.cli + ' create --name test ' + sharedimage + ':' + sharedTag, '\$', 5)
			mySSH.command('mkdir -p cmake_targets/log/ran-build', '\$', 5)
			mySSH.command(self.cli + ' cp test:/oai-ran/cmake_targets/log/. cmake_targets/log/ran-build', '\$', 5)
			mySSH.command(self.cli + ' rm -f test', '\$', 5)

		# Build the target image(s)
		for image,pattern in imageNames:
			# the archived Dockerfiles have "ran-build:latest" as base image
			# we need to update them with proper tag
			mySSH.command('sed -i -e "s#' + sharedimage + ':latest#' + sharedimage + ':' + sharedTag + '#" docker/Dockerfile.' + pattern + self.dockerfileprefix, '\$', 5)
			mySSH.command(self.cli + ' build ' + self.cliBuildOptions + ' --target ' + image + ' --tag ' + image + ':' + imageTag + ' --file docker/Dockerfile.' + pattern + self.dockerfileprefix + ' . > cmake_targets/log/' + image + '.log 2>&1', '\$', 1200)
			# split the log
			mySSH.command('mkdir -p cmake_targets/log/' + image, '\$', 5)
			mySSH.command('python3 ci-scripts/docker_log_split.py --logfilename=cmake_targets/log/' + image + '.log', '\$', 5)
			# checking the status of the build
			mySSH.command(self.cli + ' image inspect --format=\'Size = {{.Size}} bytes\' ' + image + ':' + imageTag, '\$', 5)
			if mySSH.getBefore().count('o such image') != 0:
				logging.error('\u001B[1m Could not build properly ' + image + '\u001B[0m')
				status = False
				# Here we should check if the last container corresponds to a failed command and destroy it
				mySSH.command(self.cli + ' ps --quiet --filter "status=exited" -n1 | xargs ' + self.cli + ' rm -f', '\$', 5)
				self.allImagesSize[image] = 'N/A -- Build Failed'
			else:
				result = re.search('Size *= *(?P<size>[0-9\-]+) *bytes', mySSH.getBefore())
				if result is not None:
					imageSize = float(result.group('size'))
					imageSize = imageSize / 1000
					if imageSize < 1000:
						logging.debug('\u001B[1m   ' + image + ' size is ' + ('%.0f' % imageSize) + ' kbytes\u001B[0m')
						self.allImagesSize[image] = str(round(imageSize,1)) + ' kbytes'
					else:
						imageSize = imageSize / 1000
						if imageSize < 1000:
							logging.debug('\u001B[1m   ' + image + ' size is ' + ('%.0f' % imageSize) + ' Mbytes\u001B[0m')
							self.allImagesSize[image] = str(round(imageSize,1)) + ' Mbytes'
						else:
							imageSize = imageSize / 1000
							logging.debug('\u001B[1m   ' + image + ' size is ' + ('%.3f' % imageSize) + ' Gbytes\u001B[0m')
							self.allImagesSize[image] = str(round(imageSize,1)) + ' Gbytes'
				else:
					logging.debug('ran-build size is unknown')
					self.allImagesSize[image] = 'unknown'
			# Now pruning dangling images in between target builds
			mySSH.command(self.cli + ' image prune --force', '\$', 30)

		# Analyzing the logs
		mySSH.command('cd ' + lSourcePath + '/cmake_targets', '\$', 5)
		mySSH.command('mkdir -p build_log_' + self.testCase_id, '\$', 5)
		mySSH.command('mv log/* ' + 'build_log_' + self.testCase_id, '\$', 5)

		mySSH.command('cd ' + lSourcePath + '/cmake_targets', '\$', 5)
		mySSH.command('rm -f build_log_' + self.testCase_id + '.zip || true', '\$', 5)
		if (os.path.isfile('./build_log_' + self.testCase_id + '.zip')):
			os.remove('./build_log_' + self.testCase_id + '.zip')
		if (os.path.isdir('./build_log_' + self.testCase_id)):
			shutil.rmtree('./build_log_' + self.testCase_id)
		mySSH.command('zip -r -qq build_log_' + self.testCase_id + '.zip build_log_' + self.testCase_id, '\$', 5)
		mySSH.copyin(lIpAddr, lUserName, lPassWord, lSourcePath + '/cmake_targets/build_log_' + self.testCase_id + '.zip', '.')
		mySSH.command('rm -f build_log_' + self.testCase_id + '.zip','\$', 5)
		mySSH.close()
		ZipFile('build_log_' + self.testCase_id + '.zip').extractall('.')

		#Trying to identify the errors and warnings for each built images
		imageNames1 = imageNames
		shared = ('ran-build','ran')
		imageNames1.insert(0, shared) 
		for image,pattern in imageNames1:
			files = {}
			file_list = [f for f in os.listdir('build_log_' + self.testCase_id + '/' + image) if os.path.isfile(os.path.join('build_log_' + self.testCase_id + '/' + image, f)) and f.endswith('.txt')]
			for fil in file_list:
				errorandwarnings = {}
				warningsNo = 0
				errorsNo = 0
				with open('build_log_{}/{}/{}'.format(self.testCase_id,image,fil), mode='r') as inputfile:
					for line in inputfile:
						result = re.search(' ERROR ', str(line))
						if result is not None:
							errorsNo += 1
						result = re.search(' error:', str(line))
						if result is not None:
							errorsNo += 1
						result = re.search(' WARNING ', str(line))
						if result is not None:
							warningsNo += 1
						result = re.search(' warning:', str(line))
						if result is not None:
							warningsNo += 1
					errorandwarnings['errors'] = errorsNo
					errorandwarnings['warnings'] = warningsNo
					errorandwarnings['status'] = status
				files[fil] = errorandwarnings
			# Let analyze the target image creation part
			if os.path.isfile('build_log_{}/{}.log'.format(self.testCase_id,image)):
				errorandwarnings = {}
				with open('build_log_{}/{}.log'.format(self.testCase_id,image), mode='r') as inputfile:
					startOfTargetImageCreation = False
					buildStatus = False
					for line in inputfile:
						result = re.search('FROM .* [aA][sS] ' + image + '$', str(line))
						if result is not None:
							startOfTargetImageCreation = True
						if startOfTargetImageCreation:
							result = re.search('Successfully tagged ' + image + ':', str(line))
							if result is not None:
								buildStatus = True
							result = re.search('COMMIT ' + image + ':', str(line))
							if result is not None:
								buildStatus = True
					inputfile.close()
					if buildStatus:
						errorandwarnings['errors'] = 0
					else:
						errorandwarnings['errors'] = 1
					errorandwarnings['warnings'] = 0
					errorandwarnings['status'] = buildStatus
					files['Target Image Creation'] = errorandwarnings
			self.collectInfo[image] = files
		
		if status:
			logging.info('\u001B[1m Building OAI Image(s) Pass\u001B[0m')
			HTML.CreateHtmlTestRow(self.imageKind, 'OK', CONST.ALL_PROCESSES_OK)
			HTML.CreateHtmlNextTabHeaderTestRow(self.collectInfo, self.allImagesSize)
		else:
			logging.error('\u001B[1m Building OAI Images Failed\u001B[0m')
			HTML.CreateHtmlTestRow(self.imageKind, 'KO', CONST.ALL_PROCESSES_OK)
			HTML.CreateHtmlNextTabHeaderTestRow(self.collectInfo, self.allImagesSize)
			HTML.CreateHtmlTabFooter(False)
			sys.exit(1)

	def Copy_Image_to_Test_Server(self, HTML):
		imageTag = 'develop'
		if (self.ranAllowMerge):
			imageTag = 'ci-temp'

		lSsh = SSH.SSHConnection()
		# Going to the Docker Registry server
		if self.registrySvrId == '0':
			lIpAddr = self.eNBIPAddress
			lUserName = self.eNBUserName
			lPassWord = self.eNBPassword
		elif self.registrySvrId == '1':
			lIpAddr = self.eNB1IPAddress
			lUserName = self.eNB1UserName
			lPassWord = self.eNB1Password
		elif self.registrySvrId == '2':
			lIpAddr = self.eNB2IPAddress
			lUserName = self.eNB2UserName
			lPassWord = self.eNB2Password
		lSsh.open(lIpAddr, lUserName, lPassWord)
		lSsh.command('docker save ' + self.imageToCopy + ':' + imageTag + ' | gzip > ' + self.imageToCopy + '-' + imageTag + '.tar.gz', '\$', 60)
		lSsh.copyin(lIpAddr, lUserName, lPassWord, '~/' + self.imageToCopy + '-' + imageTag + '.tar.gz', '.')
		lSsh.command('rm ' + self.imageToCopy + '-' + imageTag + '.tar.gz', '\$', 60)
		lSsh.close()

		# Going to the Test Server
		if self.testSvrId == '0':
			lIpAddr = self.eNBIPAddress
			lUserName = self.eNBUserName
			lPassWord = self.eNBPassword
		elif self.testSvrId == '1':
			lIpAddr = self.eNB1IPAddress
			lUserName = self.eNB1UserName
			lPassWord = self.eNB1Password
		elif self.testSvrId == '2':
			lIpAddr = self.eNB2IPAddress
			lUserName = self.eNB2UserName
			lPassWord = self.eNB2Password
		lSsh.open(lIpAddr, lUserName, lPassWord)
		lSsh.copyout(lIpAddr, lUserName, lPassWord, './' + self.imageToCopy + '-' + imageTag + '.tar.gz', '~')
		lSsh.command('docker rmi ' + self.imageToCopy + ':' + imageTag, '\$', 10)
		lSsh.command('docker load < ' + self.imageToCopy + '-' + imageTag + '.tar.gz', '\$', 60)
		lSsh.command('rm ' + self.imageToCopy + '-' + imageTag + '.tar.gz', '\$', 60)
		lSsh.close()

		if os.path.isfile('./' + self.imageToCopy + '-' + imageTag + '.tar.gz'):
			os.remove('./' + self.imageToCopy + '-' + imageTag + '.tar.gz')

		HTML.CreateHtmlTestRow('N/A', 'OK', CONST.ALL_PROCESSES_OK)

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
		
		self._createWorkspace(mySSH, lPassWord, lSourcePath)

		mySSH.command('cd ' + lSourcePath + '/' + self.yamlPath[self.eNB_instance], '\$', 5)
		mySSH.command('cp docker-compose.yml ci-docker-compose.yml', '\$', 5)
		imageTag = 'develop'
		if (self.ranAllowMerge):
			imageTag = 'ci-temp'
		mySSH.command('sed -i -e "s/image: oai-enb:latest/image: oai-enb:' + imageTag + '/" ci-docker-compose.yml', '\$', 2)
		mySSH.command('sed -i -e "s/image: oai-gnb:latest/image: oai-gnb:' + imageTag + '/" ci-docker-compose.yml', '\$', 2)
		localMmeIpAddr = EPC.MmeIPAddress
		mySSH.command('sed -i -e "s/CI_MME_IP_ADDR/' + localMmeIpAddr + '/" ci-docker-compose.yml', '\$', 2)
#		if self.flexranCtrlDeployed:
#			mySSH.command('sed -i -e "s/FLEXRAN_ENABLED:.*/FLEXRAN_ENABLED: \'yes\'/" ci-docker-compose.yml', '\$', 2)
#			mySSH.command('sed -i -e "s/CI_FLEXRAN_CTL_IP_ADDR/' + self.flexranCtrlIpAddress + '/" ci-docker-compose.yml', '\$', 2)
#		else:
#			mySSH.command('sed -i -e "s/FLEXRAN_ENABLED:.*$/FLEXRAN_ENABLED: \'no\'/" ci-docker-compose.yml', '\$', 2)
#			mySSH.command('sed -i -e "s/CI_FLEXRAN_CTL_IP_ADDR/127.0.0.1/" ci-docker-compose.yml', '\$', 2)
		# Currently support only one
		mySSH.command('docker-compose --file ci-docker-compose.yml config --services | sed -e "s@^@service=@" 2>&1', '\$', 10)
		result = re.search('service=(?P<svc_name>[a-zA-Z0-9\_]+)', mySSH.getBefore())
		if result is not None:
			svcName = result.group('svc_name')
			mySSH.command('docker-compose --file ci-docker-compose.yml up -d ' + svcName, '\$', 10)

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
				mySSH.command('docker inspect --format="{{.State.Health.Status}}" ' + containerName, '\$', 5)
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
		# Forcing the down now to remove the networks and any artifacts
		mySSH.command('docker-compose --file ci-docker-compose.yml down', '\$', 5)

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
		logging.info('\u001B[1m Undeploying OAI Object Pass\u001B[0m')

	def DeployGenObject(self, HTML):
		self.exitStatus = 0
		logging.info('\u001B[1m Checking Services to deploy\u001B[0m')
		cmd = 'cd ' + self.yamlPath[0] + ' && docker-compose config --services'
		logging.debug(cmd)
		try:
			listServices = subprocess.check_output(cmd, shell=True, universal_newlines=True)
		except Exception as e:
			self.exitStatus = 1
			HTML.CreateHtmlTestRow('SVC not Found', 'KO', CONST.ALL_PROCESSES_OK)
			return
		for reqSvc in self.services[0].split(' '):
			res = re.search(reqSvc, listServices)
			if res is None:
				logging.error(reqSvc + ' not found in specified docker-compose')
				self.exitStatus = 1
		if (self.exitStatus == 1):
			HTML.CreateHtmlTestRow('SVC not Found', 'KO', CONST.ALL_PROCESSES_OK)
			return

		if (self.ranAllowMerge):
			cmd = 'cd ' + self.yamlPath[0] + ' && sed -e "s@develop@ci-temp@" docker-compose.y*ml > docker-compose-ci.yml'
		else:
			cmd = 'cd ' + self.yamlPath[0] + ' && sed -e "s@develop@develop@" docker-compose.y*ml > docker-compose-ci.yml'
		logging.debug(cmd)
		subprocess.run(cmd, shell=True)

		cmd = 'cd ' + self.yamlPath[0] + ' && docker-compose -f docker-compose-ci.yml up -d ' + self.services[0]
		logging.debug(cmd)
		try:
			deployStatus = subprocess.check_output(cmd, shell=True, stderr=subprocess.STDOUT, universal_newlines=True, timeout=30)
		except Exception as e:
			self.exitStatus = 1
			logging.error('Could not deploy')
			HTML.CreateHtmlTestRow('Could not deploy', 'KO', CONST.ALL_PROCESSES_OK)
			return

		logging.info('\u001B[1m Checking if all deployed healthy\u001B[0m')
		cmd = 'cd ' + self.yamlPath[0] + ' && docker-compose -f docker-compose-ci.yml ps -a'
		count = 0
		healthy = 0
		while (count < 10):
			count += 1
			deployStatus = subprocess.check_output(cmd, shell=True, stderr=subprocess.STDOUT, universal_newlines=True, timeout=10)
			healthy = 0
			for state in deployStatus.split('\n'):
				res = re.search('Up \(healthy\)', state)
				if res is not None:
					healthy += 1
			if healthy == self.nb_healthy[0]:
				count = 100
			else:
				time.sleep(10)

		if count == 100 and healthy == self.nb_healthy[0]:
			if self.tsharkStarted == False:
				logging.debug('Starting tshark on public network')
				self.CaptureOnDockerNetworks()
				self.tsharkStarted = True
			HTML.CreateHtmlTestRow('n/a', 'OK', CONST.ALL_PROCESSES_OK)
			logging.info('\u001B[1m Deploying OAI Object(s) PASS\u001B[0m')
		else:
			self.exitStatus = 1
			HTML.CreateHtmlTestRow('Could not deploy in time', 'KO', CONST.ALL_PROCESSES_OK)
			logging.error('\u001B[1m Deploying OAI Object(s) FAILED\u001B[0m')

	def CaptureOnDockerNetworks(self):
		cmd = 'cd ' + self.yamlPath[0] + ' && docker-compose -f docker-compose-ci.yml config | grep com.docker.network.bridge.name | sed -e "s@^.*name: @@"'
		networkNames = subprocess.check_output(cmd, shell=True, stderr=subprocess.STDOUT, universal_newlines=True, timeout=10)
		cmd = 'sudo nohup tshark -f "not tcp and not arp and not port 53 and not host archive.ubuntu.com and not host security.ubuntu.com"'
		for name in networkNames.split('\n'):
			res = re.search('rfsim', name)
			if res is not None:
				cmd += ' -i ' + name
		cmd += ' -w /tmp/capture_'
		ymlPath = self.yamlPath[0].split('/')
		cmd += ymlPath[1] + '.pcap > /tmp/tshark.log 2>&1 &'
		logging.debug(cmd)
		networkNames = subprocess.check_output(cmd, shell=True, stderr=subprocess.STDOUT, universal_newlines=True, timeout=10)

	def UndeployGenObject(self, HTML):
		self.exitStatus = 0
		ymlPath = self.yamlPath[0].split('/')
		logPath = '../cmake_targets/log/' + ymlPath[1]

		if (self.ranAllowMerge):
			cmd = 'cd ' + self.yamlPath[0] + ' && sed -e "s@develop@ci-temp@" docker-compose.y*ml > docker-compose-ci.yml'
		else:
			cmd = 'cd ' + self.yamlPath[0] + ' && sed -e "s@develop@develop@" docker-compose.y*ml > docker-compose-ci.yml'
		logging.debug(cmd)
		subprocess.run(cmd, shell=True)

		# if the containers are running, recover the logs!
		cmd = 'cd ' + self.yamlPath[0] + ' && docker-compose -f docker-compose-ci.yml ps --all'
		logging.debug(cmd)
		deployStatus = subprocess.check_output(cmd, shell=True, stderr=subprocess.STDOUT, universal_newlines=True, timeout=10)
		anyLogs = False
		for state in deployStatus.split('\n'):
			res = re.search('Name|----------', state)
			if res is not None:
				continue
			if len(state) == 0:
				continue
			res = re.search('^(?P<container_name>[a-zA-Z0-9\-\_]+) ', state)
			if res is not None:
				anyLogs = True
				cName = res.group('container_name')
				cmd = 'cd ' + self.yamlPath[0] + ' && docker logs ' + cName + ' > ' + cName + '.log 2>&1'
				logging.debug(cmd)
				deployStatus = subprocess.check_output(cmd, shell=True, stderr=subprocess.STDOUT, universal_newlines=True, timeout=10)
		if anyLogs:
			cmd = 'mkdir -p '+ logPath + ' && mv ' + self.yamlPath[0] + '/*.log ' + logPath
			logging.debug(cmd)
			deployStatus = subprocess.check_output(cmd, shell=True, stderr=subprocess.STDOUT, universal_newlines=True, timeout=10)
			if self.tsharkStarted:
				self.tsharkStarted = True
				ymlPath = self.yamlPath[0].split('/')
				cmd = 'sudo chmod 666 /tmp/capture_' + ymlPath[1] + '.pcap'
				logging.debug(cmd)
				copyStatus = subprocess.check_output(cmd, shell=True, stderr=subprocess.STDOUT, universal_newlines=True, timeout=10)
				cmd = 'cp /tmp/capture_' + ymlPath[1] + '.pcap ' + logPath
				logging.debug(cmd)
				copyStatus = subprocess.check_output(cmd, shell=True, stderr=subprocess.STDOUT, universal_newlines=True, timeout=10)
				cmd = 'sudo rm /tmp/capture_' + ymlPath[1] + '.pcap'
				logging.debug(cmd)
				copyStatus = subprocess.check_output(cmd, shell=True, stderr=subprocess.STDOUT, universal_newlines=True, timeout=10)

		cmd = 'cd ' + self.yamlPath[0] + ' && docker-compose -f docker-compose-ci.yml down'
		logging.debug(cmd)
		try:
			deployStatus = subprocess.check_output(cmd, shell=True, stderr=subprocess.STDOUT, universal_newlines=True, timeout=100)
		except Exception as e:
			self.exitStatus = 1
			logging.error('Could not undeploy')
			HTML.CreateHtmlTestRow('Could not undeploy', 'KO', CONST.ALL_PROCESSES_OK)
			logging.error('\u001B[1m Undeploying OAI Object(s) FAILED\u001B[0m')
			return

		HTML.CreateHtmlTestRow('n/a', 'OK', CONST.ALL_PROCESSES_OK)
		logging.info('\u001B[1m Undeploying OAI Object(s) PASS\u001B[0m')

	def PingFromContainer(self, HTML):
		self.exitStatus = 0
		ymlPath = self.yamlPath[0].split('/')
		logPath = '../cmake_targets/log/' + ymlPath[1]
		cmd = 'mkdir -p ' + logPath
		deployStatus = subprocess.check_output(cmd, shell=True, stderr=subprocess.STDOUT, universal_newlines=True, timeout=10)

		cmd = 'docker exec ' + self.pingContName + ' /bin/bash -c "ping ' + self.pingOptions + '" 2>&1 | tee ' + logPath + '/ping_' + HTML.testCase_id + '.log'
		logging.debug(cmd)
		deployStatus = subprocess.check_output(cmd, shell=True, stderr=subprocess.STDOUT, universal_newlines=True, timeout=100)

		result = re.search(', (?P<packetloss>[0-9\.]+)% packet loss, time [0-9\.]+ms', deployStatus)
		if result is None:
			self.PingExit(HTML, False, 'Packet Loss Not Found')
			return

		packetloss = result.group('packetloss')
		if float(packetloss) == 100:
			self.PingExit(HTML, False, 'Packet Loss is 100%')
			return

		result = re.search('rtt min\/avg\/max\/mdev = (?P<rtt_min>[0-9\.]+)\/(?P<rtt_avg>[0-9\.]+)\/(?P<rtt_max>[0-9\.]+)\/[0-9\.]+ ms', deployStatus)
		if result is None:
			self.PingExit(HTML, False, 'Ping RTT_Min RTT_Avg RTT_Max Not Found!')
			return

		rtt_min = result.group('rtt_min')
		rtt_avg = result.group('rtt_avg')
		rtt_max = result.group('rtt_max')
		pal_msg = 'Packet Loss : ' + packetloss + '%'
		min_msg = 'RTT(Min)    : ' + rtt_min + ' ms'
		avg_msg = 'RTT(Avg)    : ' + rtt_avg + ' ms'
		max_msg = 'RTT(Max)    : ' + rtt_max + ' ms'

		message = 'ping result\n'
		message += '    ' + pal_msg + '\n'
		message += '    ' + min_msg + '\n'
		message += '    ' + avg_msg + '\n'
		message += '    ' + max_msg + '\n'
		packetLossOK = True
		if float(packetloss) > float(self.pingLossThreshold):
			message += '\nPacket Loss too high'
			packetLossOK = False
		elif float(packetloss) > 0:
			message += '\nPacket Loss is not 0%'
		self.PingExit(HTML, packetLossOK, message)

		if packetLossOK:
			logging.debug('\u001B[1;37;44m ping result \u001B[0m')
			logging.debug('\u001B[1;34m    ' + pal_msg + '\u001B[0m')
			logging.debug('\u001B[1;34m    ' + min_msg + '\u001B[0m')
			logging.debug('\u001B[1;34m    ' + avg_msg + '\u001B[0m')
			logging.debug('\u001B[1;34m    ' + max_msg + '\u001B[0m')
			logging.info('\u001B[1m Ping Test PASS\u001B[0m')

	def PingExit(self, HTML, status, message):
		html_queue = SimpleQueue()
		html_cell = '<pre style="background-color:white">UE\n' + message + '</pre>'
		html_queue.put(html_cell)
		if status:
			HTML.CreateHtmlTestRowQueue(self.pingOptions, 'OK', 1, html_queue)
		else:
			logging.error('\u001B[1;37;41m ping test FAIL -- ' + message + ' \u001B[0m')
			HTML.CreateHtmlTestRowQueue(self.pingOptions, 'KO', 1, html_queue)
			# Automatic undeployment
			logging.debug('----------------------------------------')
			logging.debug('\u001B[1m Starting Automatic undeployment \u001B[0m')
			logging.debug('----------------------------------------')
			HTML.testCase_id = 'AUTO-UNDEPLOY'
			HTML.desc = 'Automatic Un-Deployment'
			self.UndeployGenObject(HTML)
			self.exitStatus = 1

	def IperfFromContainer(self, HTML):
		self.exitStatus = 0

		ymlPath = self.yamlPath[0].split('/')
		logPath = '../cmake_targets/log/' + ymlPath[1]
		cmd = 'mkdir -p ' + logPath
		logStatus = subprocess.check_output(cmd, shell=True, stderr=subprocess.STDOUT, universal_newlines=True, timeout=10)

		# Start the server process
		cmd = 'docker exec -d ' + self.svrContName + ' /bin/bash -c "nohup iperf ' + self.svrOptions + ' > /tmp/iperf_server.log 2>&1"'
		logging.debug(cmd)
		serverStatus = subprocess.check_output(cmd, shell=True, stderr=subprocess.STDOUT, universal_newlines=True, timeout=10)
		time.sleep(5)

		# Start the client process
		cmd = 'docker exec ' + self.cliContName + ' /bin/bash -c "iperf ' + self.cliOptions + '" 2>&1 | tee ' + logPath + '/iperf_client_' + HTML.testCase_id + '.log'
		logging.debug(cmd)
		clientStatus = subprocess.check_output(cmd, shell=True, stderr=subprocess.STDOUT, universal_newlines=True, timeout=100)

		# Stop the server process
		cmd = 'docker exec ' + self.svrContName + ' /bin/bash -c "pkill iperf"'
		logging.debug(cmd)
		serverStatus = subprocess.check_output(cmd, shell=True, stderr=subprocess.STDOUT, universal_newlines=True, timeout=10)
		time.sleep(5)
		cmd = 'docker cp ' + self.svrContName + ':/tmp/iperf_server.log '+ logPath + '/iperf_server_' + HTML.testCase_id + '.log'
		logging.debug(cmd)
		serverStatus = subprocess.check_output(cmd, shell=True, stderr=subprocess.STDOUT, universal_newlines=True, timeout=10)

		# Analyze client output
		result = re.search('Server Report:', clientStatus)
		if result is None:
			result = re.search('read failed: Connection refused', clientStatus)
			if result is not None:
				message = 'Could not connect to iperf server!'
			else:
				message = 'Server Report and Connection refused Not Found!'
			self.IperfExit(HTML, False, message)
			return

		# Computing the requested bandwidth in float
		result = re.search('-b (?P<iperf_bandwidth>[0-9\.]+)[KMG]', self.cliOptions)
		if result is not None:
			req_bandwidth = result.group('iperf_bandwidth')
			req_bw = float(req_bandwidth)
			result = re.search('-b [0-9\.]+K', self.cliOptions)
			if result is not None:
				req_bandwidth = '%.1f Kbits/sec' % req_bw
				req_bw = req_bw * 1000
			result = re.search('-b [0-9\.]+M', self.cliOptions)
			if result is not None:
				req_bandwidth = '%.1f Mbits/sec' % req_bw
				req_bw = req_bw * 1000000

		reportLine = None
		reportLineFound = False
		for iLine in clientStatus.split('\n'):
			if reportLineFound:
				reportLine = iLine
				reportLineFound = False
			res = re.search('Server Report:', iLine)
			if res is not None:
				reportLineFound = True
		result = None
		if reportLine is not None:
			result = re.search('(?:|\[ *\d+\].*) (?P<bitrate>[0-9\.]+ [KMG]bits\/sec) +(?P<jitter>[0-9\.]+ ms) +(\d+\/ ..\d+) +(\((?P<packetloss>[0-9\.]+)%\))', reportLine)
		iperfStatus = True
		if result is not None:
			bitrate = result.group('bitrate')
			packetloss = result.group('packetloss')
			jitter = result.group('jitter')
			logging.debug('\u001B[1;37;44m iperf result \u001B[0m')
			iperfStatus = True
			msg = 'Req Bitrate : ' + req_bandwidth + '\n'
			logging.debug('\u001B[1;34m    Req Bitrate : ' + req_bandwidth + '\u001B[0m')
			if bitrate is not None:
				msg += 'Bitrate     : ' + bitrate + '\n'
				logging.debug('\u001B[1;34m    Bitrate     : ' + bitrate + '\u001B[0m')
				result = re.search('(?P<real_bw>[0-9\.]+) [KMG]bits/sec', str(bitrate))
				if result is not None:
					actual_bw = float(str(result.group('real_bw')))
					result = re.search('[0-9\.]+ K', bitrate)
					if result is not None:
						actual_bw = actual_bw * 1000
					result = re.search('[0-9\.]+ M', bitrate)
					if result is not None:
						actual_bw = actual_bw * 1000000
					br_loss = 100 * actual_bw / req_bw
					if br_loss < 90:
						iperfStatus = False
					bitperf = '%.2f ' % br_loss
					msg += 'Bitrate Perf: ' + bitperf + '%\n'
					logging.debug('\u001B[1;34m    Bitrate Perf: ' + bitperf + '%\u001B[0m')
			if packetloss is not None:
				msg += 'Packet Loss : ' + packetloss + '%\n'
				logging.debug('\u001B[1;34m    Packet Loss : ' + packetloss + '%\u001B[0m')
				if float(packetloss) > float(5):
					msg += 'Packet Loss too high!\n'
					logging.debug('\u001B[1;37;41m Packet Loss too high \u001B[0m')
					iperfStatus = False
			if jitter is not None:
				msg += 'Jitter      : ' + jitter + '\n'
				logging.debug('\u001B[1;34m    Jitter      : ' + jitter + '\u001B[0m')
			self.IperfExit(HTML, iperfStatus, msg)
		else:
			iperfStatus = False
			logging.error('problem?')
			self.IperfExit(HTML, iperfStatus, 'problem?')
		if iperfStatus:
			logging.info('\u001B[1m Iperf Test PASS\u001B[0m')

	def IperfExit(self, HTML, status, message):
		html_queue = SimpleQueue()
		html_cell = '<pre style="background-color:white">UE\n' + message + '</pre>'
		html_queue.put(html_cell)
		if status:
			HTML.CreateHtmlTestRowQueue(self.cliOptions, 'OK', 1, html_queue)
		else:
			logging.error('\u001B[1m Iperf Test FAIL -- ' + message + ' \u001B[0m')
			HTML.CreateHtmlTestRowQueue(self.cliOptions, 'KO', 1, html_queue)


	def CheckAndAddRoute(self, svrName, ipAddr, userName, password):
		logging.debug('Checking IP routing on ' + svrName)
		mySSH = SSH.SSHConnection()
		if svrName == 'porcepix':
			mySSH.open(ipAddr, userName, password)
			# Check if route to asterix gnb exists
			mySSH.command('ip route | grep --colour=never "192.168.68.64/26"', '\$', 10)
			result = re.search('192.168.18.194', mySSH.getBefore())
			if result is None:
				mySSH.command('echo ' + password + ' | sudo -S ip route add 192.168.68.64/26 via 192.168.18.194 dev eno1', '\$', 10)
			# Check if route to obelix enb exists
			mySSH.command('ip route | grep --colour=never "192.168.68.128/26"', '\$', 10)
			result = re.search('192.168.18.193', mySSH.getBefore())
			if result is None:
				mySSH.command('echo ' + password + ' | sudo -S ip route add 192.168.68.128/26 via 192.168.18.193 dev eno1', '\$', 10)
			# Check if route to nepes gnb exists
			mySSH.command('ip route | grep --colour=never "192.168.68.192/26"', '\$', 10)
			result = re.search('192.168.18.209', mySSH.getBefore())
			if result is None:
				mySSH.command('echo ' + password + ' | sudo -S ip route add 192.168.68.192/26 via 192.168.18.209 dev eno1', '\$', 10)
			# Check if forwarding is enabled
			mySSH.command('sysctl net.ipv4.conf.all.forwarding', '\$', 10)
			result = re.search('net.ipv4.conf.all.forwarding = 1', mySSH.getBefore())
			if result is None:
				mySSH.command('echo ' + password + ' | sudo -S sysctl net.ipv4.conf.all.forwarding=1', '\$', 10)
			# Check if iptables forwarding is accepted
			mySSH.command('echo ' + password + ' | sudo -S iptables -L', '\$', 10)
			result = re.search('Chain FORWARD .*policy ACCEPT', mySSH.getBefore())
			if result is None:
				mySSH.command('echo ' + password + ' | sudo -S iptables -P FORWARD ACCEPT', '\$', 10)
			mySSH.close()
		if svrName == 'asterix':
			mySSH.open(ipAddr, userName, password)
			# Check if route to porcepix epc exists
			mySSH.command('ip route | grep --colour=never "192.168.61.192/26"', '\$', 10)
			result = re.search('192.168.18.210', mySSH.getBefore())
			if result is None:
				mySSH.command('echo ' + password + ' | sudo -S ip route add 192.168.61.192/26 via 192.168.18.210 dev em1', '\$', 10)
			# Check if route to porcepix cn5g exists
			mySSH.command('ip route | grep --colour=never "192.168.70.128/26"', '\$', 10)
			result = re.search('192.168.18.210', mySSH.getBefore())
			if result is None:
				mySSH.command('echo ' + password + ' | sudo -S ip route add 192.168.70.128/26 via 192.168.18.210 dev em1', '\$', 10)
			# Check if X2 route to obelix enb exists
			mySSH.command('ip route | grep --colour=never "192.168.68.128/26"', '\$', 10)
			result = re.search('192.168.18.193', mySSH.getBefore())
			if result is None:
				mySSH.command('echo ' + password + ' | sudo -S ip route add 192.168.68.128/26 via 192.168.18.193 dev em1', '\$', 10)
			# Check if forwarding is enabled
			mySSH.command('sysctl net.ipv4.conf.all.forwarding', '\$', 10)
			result = re.search('net.ipv4.conf.all.forwarding = 1', mySSH.getBefore())
			if result is None:
				mySSH.command('echo ' + password + ' | sudo -S sysctl net.ipv4.conf.all.forwarding=1', '\$', 10)
			# Check if iptables forwarding is accepted
			mySSH.command('echo ' + password + ' | sudo -S iptables -L', '\$', 10)
			result = re.search('Chain FORWARD .*policy ACCEPT', mySSH.getBefore())
			if result is None:
				mySSH.command('echo ' + password + ' | sudo -S iptables -P FORWARD ACCEPT', '\$', 10)
			mySSH.close()
		if svrName == 'obelix':
			mySSH.open(ipAddr, userName, password)
			# Check if route to porcepix epc exists
			mySSH.command('ip route | grep --colour=never "192.168.61.192/26"', '\$', 10)
			result = re.search('192.168.18.210', mySSH.getBefore())
			if result is None:
				mySSH.command('echo ' + password + ' | sudo -S ip route add 192.168.61.192/26 via 192.168.18.210 dev eno1', '\$', 10)
			# Check if X2 route to asterix gnb exists
			mySSH.command('ip route | grep --colour=never "192.168.68.64/26"', '\$', 10)
			result = re.search('192.168.18.194', mySSH.getBefore())
			if result is None:
				mySSH.command('echo ' + password + ' | sudo -S ip route add 192.168.68.64/26 via 192.168.18.194 dev eno1', '\$', 10)
			# Check if X2 route to nepes gnb exists
			mySSH.command('ip route | grep --colour=never "192.168.68.192/26"', '\$', 10)
			result = re.search('192.168.18.209', mySSH.getBefore())
			if result is None:
				mySSH.command('echo ' + password + ' | sudo -S ip route add 192.168.68.192/26 via 192.168.18.209 dev eno1', '\$', 10)
			# Check if forwarding is enabled
			mySSH.command('sysctl net.ipv4.conf.all.forwarding', '\$', 10)
			result = re.search('net.ipv4.conf.all.forwarding = 1', mySSH.getBefore())
			if result is None:
				mySSH.command('echo ' + password + ' | sudo -S sysctl net.ipv4.conf.all.forwarding=1', '\$', 10)
			# Check if iptables forwarding is accepted
			mySSH.command('echo ' + password + ' | sudo -S iptables -L', '\$', 10)
			result = re.search('Chain FORWARD .*policy ACCEPT', mySSH.getBefore())
			if result is None:
				mySSH.command('echo ' + password + ' | sudo -S iptables -P FORWARD ACCEPT', '\$', 10)
			mySSH.close()
		if svrName == 'nepes':
			mySSH.open(ipAddr, userName, password)
			# Check if route to porcepix epc exists
			mySSH.command('ip route | grep --colour=never "192.168.61.192/26"', '\$', 10)
			result = re.search('192.168.18.210', mySSH.getBefore())
			if result is None:
				mySSH.command('echo ' + password + ' | sudo -S ip route add 192.168.61.192/26 via 192.168.18.210 dev enp0s31f6', '\$', 10)
			# Check if X2 route to obelix enb exists
			mySSH.command('ip route | grep --colour=never "192.168.68.128/26"', '\$', 10)
			result = re.search('192.168.18.193', mySSH.getBefore())
			if result is None:
				mySSH.command('echo ' + password + ' | sudo -S ip route add 192.168.68.128/26 via 192.168.18.193 dev enp0s31f6', '\$', 10)
			# Check if forwarding is enabled
			mySSH.command('sysctl net.ipv4.conf.all.forwarding', '\$', 10)
			result = re.search('net.ipv4.conf.all.forwarding = 1', mySSH.getBefore())
			if result is None:
				mySSH.command('echo ' + password + ' | sudo -S sysctl net.ipv4.conf.all.forwarding=1', '\$', 10)
			# Check if iptables forwarding is accepted
			mySSH.command('echo ' + password + ' | sudo -S iptables -L', '\$', 10)
			result = re.search('Chain FORWARD .*policy ACCEPT', mySSH.getBefore())
			if result is None:
				mySSH.command('echo ' + password + ' | sudo -S iptables -P FORWARD ACCEPT', '\$', 10)
			mySSH.close()

