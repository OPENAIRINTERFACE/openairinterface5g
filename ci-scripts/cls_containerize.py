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
import sys	      # arg
import re	       # reg
import logging
import os
import shutil
import subprocess
import time
import pyshark
import threading
import cls_cmd
from multiprocessing import Process, Lock, SimpleQueue
from zipfile import ZipFile

#-----------------------------------------------------------
# OAI Testing modules
#-----------------------------------------------------------
import cls_cluster as OC
import cls_cmd
import sshconnection as SSH
import helpreadme as HELP
import constants as CONST
import cls_oaicitest

#-----------------------------------------------------------
# Helper functions used here and in other classes
# (e.g., cls_cluster.py)
#-----------------------------------------------------------
IMAGES = ['oai-enb', 'oai-lte-ru', 'oai-lte-ue', 'oai-gnb', 'oai-nr-cuup', 'oai-gnb-aw2s', 'oai-nr-ue', 'oai-gnb-asan', 'oai-nr-ue-asan', 'oai-nr-cuup-asan']

def CreateWorkspace(sshSession, sourcePath, ranRepository, ranCommitID, ranTargetBranch, ranAllowMerge):
	if ranCommitID == '':
		logging.error('need ranCommitID in CreateWorkspace()')
		sys.exit('Insufficient Parameter in CreateWorkspace()')

	sshSession.command(f'rm -rf {sourcePath}', '\$', 10)
	sshSession.command('mkdir -p ' + sourcePath, '\$', 5)
	sshSession.command('cd ' + sourcePath, '\$', 5)
	# Recent version of git (>2.20?) should handle missing .git extension # without problems
	if ranTargetBranch == 'null':
		ranTargetBranch = 'develop'
	baseBranch = re.sub('origin/', '', ranTargetBranch)
	sshSession.command(f'git clone --filter=blob:none -n -b {baseBranch} {ranRepository} .', '\$', 60)
	if sshSession.getBefore().count('error') > 0 or sshSession.getBefore().count('error') > 0:
		sys.exit('error during clone')
	sshSession.command('git config user.email "jenkins@openairinterface.org"', '\$', 5)
	sshSession.command('git config user.name "OAI Jenkins"', '\$', 5)

	sshSession.command('mkdir -p cmake_targets/log', '\$', 5)
	# if the commit ID is provided use it to point to it
	sshSession.command(f'git checkout -f {ranCommitID}', '\$', 30)
	if sshSession.getBefore().count(f'HEAD is now at {ranCommitID[:6]}') != 1:
		sshSession.command('git log --oneline | head -n5', '\$', 5)
		logging.warning(f'problems during checkout, is at: {sshSession.getBefore()}')
	else:
		logging.debug('successful checkout')
	# if the branch is not develop, then it is a merge request and we need to do
	# the potential merge. Note that merge conflicts should already been checked earlier
	if ranAllowMerge:
		if ranTargetBranch == '':
			ranTargetBranch = 'develop'
		logging.debug(f'Merging with the target branch: {ranTargetBranch}')
		sshSession.command(f'git merge --ff origin/{ranTargetBranch} -m "Temporary merge for CI"', '\$', 30)

def ImageTagToUse(imageName, ranCommitID, ranBranch, ranAllowMerge):
	shortCommit = ranCommitID[0:8]
	if ranAllowMerge:
		# Allowing contributor to have a name/branchName format
		branchName = ranBranch.replace('/','-')
		tagToUse = f'{branchName}-{shortCommit}'
	else:
		tagToUse = f'develop-{shortCommit}'
	fullTag = f'{imageName}:{tagToUse}'
	return fullTag

def CopyLogsToExecutor(cmd, sourcePath, log_name):
	cmd.cd(f'{sourcePath}/cmake_targets')
	cmd.run(f'rm -f {log_name}.zip')
	cmd.run(f'mkdir -p {log_name}')
	cmd.run(f'mv log/* {log_name}')
	cmd.run(f'zip -r -qq {log_name}.zip {log_name}')

	# copy zip to executor for analysis
	if (os.path.isfile(f'./{log_name}.zip')):
		os.remove(f'./{log_name}.zip')
	if (os.path.isdir(f'./{log_name}')):
		shutil.rmtree(f'./{log_name}')
	cmd.copyin(f'{sourcePath}/cmake_targets/{log_name}.zip', f'./{log_name}.zip')
	cmd.run(f'rm -f {log_name}.zip')
	ZipFile(f'{log_name}.zip').extractall('.')

def AnalyzeBuildLogs(buildRoot, images, globalStatus):
	collectInfo = {}
	for image in images:
		files = {}
		file_list = [f for f in os.listdir(f'{buildRoot}/{image}') if os.path.isfile(os.path.join(f'{buildRoot}/{image}', f)) and f.endswith('.txt')]
		# Analyze the "sub-logs" of every target image
		for fil in file_list:
			errorandwarnings = {}
			warningsNo = 0
			errorsNo = 0
			with open(f'{buildRoot}/{image}/{fil}', mode='r') as inputfile:
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
				errorandwarnings['status'] = globalStatus
			files[fil] = errorandwarnings
		# Analyze the target image
		if os.path.isfile(f'{buildRoot}/{image}.log'):
			errorandwarnings = {}
			committed = False
			tagged = False
			with open(f'{buildRoot}/{image}.log', mode='r') as inputfile:
				for line in inputfile:
					lineHasTag = re.search(f'Successfully tagged {image}:', str(line)) is not None
					lineHasTag2 = re.search(f'naming to docker.io/library/{image}:', str(line)) is not None
					tagged = tagged or lineHasTag or lineHasTag2
					# the OpenShift Cluster builder prepends image registry URL
					lineHasCommit = re.search(f'COMMIT [a-zA-Z0-9\.:/\-]*{image}', str(line)) is not None
					committed = committed or lineHasCommit
			errorandwarnings['errors'] = 0 if committed or tagged else 1
			errorandwarnings['warnings'] = 0
			errorandwarnings['status'] = committed or tagged
			files['Target Image Creation'] = errorandwarnings
		collectInfo[image] = files
	return collectInfo

def AnalyzeIperf(cliOptions, clientReport, serverReport):
	req_bw = 1.0 # default iperf throughput, in Mbps
	result = re.search('-b *(?P<iperf_bandwidth>[0-9\.]+)(?P<magnitude>[kKMG])', cliOptions)
	if result is not None:
		req_bw = float(result.group('iperf_bandwidth'))
		magn = result.group('magnitude')
		if magn == "k" or magn == "K":
			req_bw /= 1000
		elif magn == "G":
			req_bw *= 1000
	req_dur = 10 # default iperf send duration
	result = re.search('-t *(?P<duration>[0-9]+)', cliOptions)
	if result is not None:
		req_dur = int(result.group('duration'))

	reportLine = None
	# find server report in client status
	clientReportLines = clientReport.split('\n')
	for l in range(len(clientReportLines)):
		res = re.search('read failed: Connection refused', clientReportLines[l])
		if res is not None:
			message = 'iperf connection refused by server!'
			logging.error(f'\u001B[1;37;41mIperf Test FAIL: {message}\u001B[0m')
			return (False, message)
		res = re.search('Server Report:', clientReportLines[l])
		if res is not None and l + 1 < len(clientReportLines):
			reportLine = clientReportLines[l+1]
			logging.debug(f'found server report: "{reportLine}"')

	statusTemplate = '(?:|\[ *\d+\].*) +0\.0-\s*(?P<duration>[0-9\.]+) +sec +[0-9\.]+ [kKMG]Bytes +(?P<bitrate>[0-9\.]+) (?P<magnitude>[kKMG])bits\/sec +(?P<jitter>[0-9\.]+) ms +(\d+\/ *\d+) +(\((?P<packetloss>[0-9\.]+)%\))'
	# if we do not find a server report in the client logs, check the server logs
	# and use the last line which is typically close/identical to server report
	if reportLine is None:
		for l in serverReport.split('\n'):
			res = re.search(statusTemplate, l)
			if res is not None:
				reportLine = l
		if reportLine is None:
			logging.warning('no report in server status found!')
			return (False, 'could not parse iperf logs')
		logging.debug(f'found client status: {reportLine}')

	result = re.search(statusTemplate, reportLine)
	if result is None:
		logging.error('could not analyze report from statusTemplate')
		return (False, 'could not parse iperf logs')

	duration = float(result.group('duration'))
	bitrate = float(result.group('bitrate'))
	magn = result.group('magnitude')
	if magn == "k" or magn == "K":
		bitrate /= 1000
	elif magn == "G": # we assume bitrate in Mbps, therefore it must be G now
		bitrate *= 1000
	jitter = float(result.group('jitter'))
	packetloss = float(result.group('packetloss'))

	logging.debug('\u001B[1;37;44m iperf result \u001B[0m')
	msg = f'Req Bitrate: {req_bw}'
	logging.debug(f'\u001B[1;34m{msg}\u001B[0m')

	br_loss = bitrate/req_bw
	bmsg = f'Bitrate    : {bitrate} (perf {br_loss})'
	logging.debug(f'\u001B[1;34m{bmsg}\u001B[0m')
	msg += '\n' + bmsg
	if br_loss < 0.9:
		msg += '\nBitrate performance too low (<90%)'
		logging.debug(f'\u001B[1;37;41mBitrate performance too low (<90%)\u001B[0m')
		return (False, msg)

	plmsg = f'Packet Loss: {packetloss}%'
	logging.debug(f'\u001B[1;34m{plmsg}\u001B[0m')
	msg += '\n' + plmsg
	if packetloss > 5.0:
		msg += '\nPacket Loss too high!'
		logging.debug(f'\u001B[1;37;41mPacket Loss too high \u001B[0m')
		return (False, msg)

	dmsg = f'Duration   : {duration} (req {req_dur})'
	logging.debug(f'\u001B[1;34m{dmsg}\u001B[0m')
	msg += '\n' + dmsg
	if duration < float(req_dur):
		msg += '\nDuration of iperf too short!'
		logging.debug(f'\u001B[1;37;41mDuration of iperf too short\u001B[0m')
		return (False, msg)

	jmsg = f'Jitter     : {jitter}'
	logging.debug(f'\u001B[1;34m{jmsg}\u001B[0m')
	msg += '\n' + jmsg
	return (True, msg)

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
		self.proxyCommit = None
		self.eNB_instance = 0
		self.eNB_serverId = ['', '', '']
		self.deployKind = [True, True, True]
		self.yamlPath = ['', '', '']
		self.services = ['', '', '']
		self.nb_healthy = [0, 0, 0]
		self.exitStatus = 0
		self.eNB_logFile = ['', '', '']

		self.testCase_id = ''

		self.cli = ''
		self.cliBuildOptions = ''
		self.dockerfileprefix = ''
		self.host = ''

		self.deployedContainers = []
		self.tsharkStarted = False
		self.displayedNewTags = False
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
		self.imageToPull = []
		#checkers from xml
		self.ran_checkers={}

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
		cmd = cls_cmd.RemoteCmd(lIpAddr)
	
		# Checking the hostname to get adapted on cli and dockerfileprefixes
		cmd.run('hostnamectl')
		result = re.search('Ubuntu|Red Hat', cmd.getBefore())
		self.host = result.group(0)
		if self.host == 'Ubuntu':
			self.cli = 'docker'
			self.dockerfileprefix = '.ubuntu20'
			self.cliBuildOptions = ''
		elif self.host == 'Red Hat':
			self.cli = 'sudo podman'
			self.dockerfileprefix = '.rhel9'
			self.cliBuildOptions = '--disable-compression'

		# we always build the ran-build image with all targets
		# Creating a tupple with the imageName, the DockerFile prefix pattern, targetName and sanitized option
		imageNames = [('ran-build', 'build', 'ran-build', '')]
		result = re.search('eNB', self.imageKind)
		if result is not None:
			imageNames.append(('oai-enb', 'eNB', 'oai-enb', ''))
		result = re.search('gNB', self.imageKind)
		if result is not None:
			imageNames.append(('oai-gnb', 'gNB', 'oai-gnb', ''))
		result = re.search('all', self.imageKind)
		if result is not None:
			imageNames.append(('oai-enb', 'eNB', 'oai-enb', ''))
			imageNames.append(('oai-gnb', 'gNB', 'oai-gnb', ''))
			imageNames.append(('oai-nr-cuup', 'nr-cuup', 'oai-nr-cuup', ''))
			imageNames.append(('oai-lte-ue', 'lteUE', 'oai-lte-ue', ''))
			imageNames.append(('oai-nr-ue', 'nrUE', 'oai-nr-ue', ''))
			if self.host == 'Red Hat':
				imageNames.append(('oai-physim', 'phySim', 'oai-physim', ''))
			if self.host == 'Ubuntu':
				imageNames.append(('oai-lte-ru', 'lteRU', 'oai-lte-ru', ''))
				# Building again the 5G images with Address Sanitizer
				imageNames.append(('ran-build', 'build', 'ran-build-asan', '--build-arg "BUILD_OPTION=--sanitize"'))
				imageNames.append(('oai-gnb', 'gNB', 'oai-gnb-asan', '--build-arg "BUILD_OPTION=--sanitize"'))
				imageNames.append(('oai-nr-ue', 'nrUE', 'oai-nr-ue-asan', '--build-arg "BUILD_OPTION=--sanitize"'))
				imageNames.append(('oai-nr-cuup', 'nr-cuup', 'oai-nr-cuup-asan', '--build-arg "BUILD_OPTION=--sanitize"'))
		result = re.search('build_cross_arm64', self.imageKind)
		if result is not None:
			self.dockerfileprefix = '.ubuntu20.cross-arm64'
		
		# Workaround for some servers, we need to erase completely the workspace
		if self.forcedWorkspaceCleanup:
			cmd.run(f'sudo -S rm -Rf {lSourcePath}')
	
		self.testCase_id = HTML.testCase_id
	
		CreateWorkspace(cmd, lSourcePath, self.ranRepository, self.ranCommitID, self.ranTargetBranch, self.ranAllowMerge)

		# if asterix, copy the entitlement and subscription manager configurations
		if self.host == 'Red Hat':
			cmd.run('mkdir -p ./etc-pki-entitlement ./rhsm-conf ./rhsm-ca')
			cmd.run('cp /etc/rhsm/rhsm.conf ./rhsm-conf/')
			cmd.run('cp /etc/rhsm/ca/redhat-uep.pem ./rhsm-ca/')
			cmd.run('cp /etc/pki/entitlement/*.pem ./etc-pki-entitlement/')

		baseImage = 'ran-base'
		baseTag = 'develop'
		forceBaseImageBuild = False
		imageTag = 'develop'
		if (self.ranAllowMerge):
			imageTag = 'ci-temp'
			if self.ranTargetBranch == 'develop':
				cmd.run(f'git diff HEAD..origin/develop -- cmake_targets/build_oai cmake_targets/tools/build_helper docker/Dockerfile.base{self.dockerfileprefix} | grep --colour=never -i INDEX')
				result = re.search('index', cmd.getBefore())
				if result is not None:
					forceBaseImageBuild = True
					baseTag = 'ci-temp'
			# if the branch name contains integration_20xx_wyy, let rebuild ran-base
			result = re.search('integration_20([0-9]{2})_w([0-9]{2})', self.ranBranch)
			if not forceBaseImageBuild and result is not None:
				forceBaseImageBuild = True
				baseTag = 'ci-temp'
		else:
			forceBaseImageBuild = True

		# Let's remove any previous run artifacts if still there
		cmd.run(f"{self.cli} image prune --force")
		for image,pattern,name,option in imageNames:
			cmd.run(f"{self.cli} image rm {name}:{imageTag}")

		# Build the base image only on Push Events (not on Merge Requests)
		# On when the base image docker file is being modified.
		if forceBaseImageBuild:
			cmd.run(f"{self.cli} image rm {baseImage}:{baseTag}")
			cmd.run(f"{self.cli} build {self.cliBuildOptions} --target {baseImage} --tag {baseImage}:{baseTag} --file docker/Dockerfile.base{self.dockerfileprefix} . &> cmake_targets/log/ran-base.log", timeout=1600)
		# First verify if the base image was properly created.
		ret = cmd.run(f"{self.cli} image inspect --format=\'Size = {{{{.Size}}}} bytes\' {baseImage}:{baseTag}")
		allImagesSize = {}
		if ret.returncode != 0:
			logging.error('\u001B[1m Could not build properly ran-base\u001B[0m')
			# Recover the name of the failed container?
			cmd.run(f"{self.cli} ps --quiet --filter \"status=exited\" -n1 | xargs --no-run-if-empty {self.cli} rm -f")
			cmd.run(f"{self.cli} image prune --force")
			cmd.close()
			logging.error('\u001B[1m Building OAI Images Failed\u001B[0m')
			HTML.CreateHtmlTestRow(self.imageKind, 'KO', CONST.ALL_PROCESSES_OK)
			HTML.CreateHtmlTabFooter(False)
			return False
		else:
			result = re.search('Size *= *(?P<size>[0-9\-]+) *bytes', cmd.getBefore())
			if result is not None:
				size = float(result.group("size")) / 1000000
				imageSizeStr = f'{size:.1f}'
				logging.debug(f'\u001B[1m   ran-base size is {imageSizeStr} Mbytes\u001B[0m')
				allImagesSize['ran-base'] = f'{imageSizeStr} Mbytes'
			else:
				logging.debug('ran-base size is unknown')

		# Recover build logs, for the moment only possible when build is successful
		cmd.run(f"{self.cli} create --name test {baseImage}:{baseTag}")
		cmd.run("mkdir -p cmake_targets/log/ran-base")
		cmd.run(f"{self.cli} cp test:/oai-ran/cmake_targets/log/. cmake_targets/log/ran-base")
		cmd.run(f"{self.cli} rm -f test")

		# Build the target image(s)
		status = True
		attemptedImages = ['ran-base']
		for image,pattern,name,option in imageNames:
			attemptedImages += [name]
			# the archived Dockerfiles have "ran-base:latest" as base image
			# we need to update them with proper tag
			cmd.run(f'git checkout -- docker/Dockerfile.{pattern}{self.dockerfileprefix}')
			cmd.run(f'sed -i -e "s#{baseImage}:latest#{baseImage}:{baseTag}#" docker/Dockerfile.{pattern}{self.dockerfileprefix}')
			# target images should use the proper ran-build image
			if image != 'ran-build' and "-asan" in name:
				cmd.run(f'sed -i -e "s#ran-build:latest#ran-build-asan:{imageTag}#" docker/Dockerfile.{pattern}{self.dockerfileprefix}')
			elif image != 'ran-build':
				cmd.run(f'sed -i -e "s#ran-build:latest#ran-build:{imageTag}#" docker/Dockerfile.{pattern}{self.dockerfileprefix}')
			ret = cmd.run(f'{self.cli} build {self.cliBuildOptions} --target {image} --tag {name}:{imageTag} --file docker/Dockerfile.{pattern}{self.dockerfileprefix} {option} . > cmake_targets/log/{name}.log 2>&1', timeout=1200)
			if image == 'ran-build' and ret.returncode == 0:
				cmd.run(f"docker run --name test-log -d {name}:{imageTag} /bin/true")
				cmd.run(f"docker cp test-log:/oai-ran/cmake_targets/log/ cmake_targets/log/{name}/")
				cmd.run(f"docker rm -f test-log")
			else:
				cmd.run(f"mkdir -p cmake_targets/log/{name}")
			# check the status of the build
			ret = cmd.run(f"{self.cli} image inspect --format=\'Size = {{{{.Size}}}} bytes\' {name}:{imageTag}")
			if ret.returncode != 0:
				logging.error('\u001B[1m Could not build properly ' + name + '\u001B[0m')
				status = False
				# Here we should check if the last container corresponds to a failed command and destroy it
				cmd.run(f"{self.cli} ps --quiet --filter \"status=exited\" -n1 | xargs --no-run-if-empty {self.cli} rm -f")
				allImagesSize[name] = 'N/A -- Build Failed'
				break
			else:
				result = re.search('Size *= *(?P<size>[0-9\-]+) *bytes', cmd.getBefore())
				if result is not None:
					size = float(result.group("size")) / 1000000 # convert to MB
					imageSizeStr = f'{size:.1f}'
					logging.debug(f'\u001B[1m   {name} size is {imageSizeStr} Mbytes\u001B[0m')
					allImagesSize[name] = f'{imageSizeStr} Mbytes'
				else:
					logging.debug(f'{name} size is unknown')
					allImagesSize[name] = 'unknown'
			# Now pruning dangling images in between target builds
			cmd.run(f"{self.cli} image prune --force")

		# Remove all intermediate build images and clean up
		cmd.run(f"{self.cli} image rm ran-build:{imageTag} ran-build-asan:{imageTag}")
		cmd.run(f"{self.cli} volume prune --force")
		# Remove any cached artifacts: we don't use the cache for now, prevent
		# out of diskspace problem
		cmd.run(f"{self.cli} buildx prune --filter=until=6h --force")

		# create a zip with all logs
		build_log_name = f'build_log_{self.testCase_id}'
		CopyLogsToExecutor(cmd, lSourcePath, build_log_name)
		cmd.close()

		# Analyze the logs
		collectInfo = AnalyzeBuildLogs(build_log_name, attemptedImages, status)
		
		if status:
			logging.info('\u001B[1m Building OAI Image(s) Pass\u001B[0m')
			HTML.CreateHtmlTestRow(self.imageKind, 'OK', CONST.ALL_PROCESSES_OK)
			HTML.CreateHtmlNextTabHeaderTestRow(collectInfo, allImagesSize)
			return True
		else:
			logging.error('\u001B[1m Building OAI Images Failed\u001B[0m')
			HTML.CreateHtmlTestRow(self.imageKind, 'KO', CONST.ALL_PROCESSES_OK)
			HTML.CreateHtmlNextTabHeaderTestRow(collectInfo, allImagesSize)
			HTML.CreateHtmlTabFooter(False)
			return False

	def BuildProxy(self, HTML):
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
		if self.proxyCommit is None:
			HELP.GenericHelp(CONST.Version)
			sys.exit('Insufficient Parameter (need proxyCommit for proxy build)')
		logging.debug('Building on server: ' + lIpAddr)
		mySSH = SSH.SSHConnection()
		mySSH.open(lIpAddr, lUserName, lPassWord)

		# Check that we are on Ubuntu
		mySSH.command('hostnamectl', '\$', 5)
		result = re.search('Ubuntu',  mySSH.getBefore())
		self.host = result.group(0)
		if self.host != 'Ubuntu':
			logging.error('\u001B[1m Can build proxy only on Ubuntu server\u001B[0m')
			mySSH.close()
			sys.exit(1)

		self.cli = 'docker'
		self.cliBuildOptions = ''

		# Workaround for some servers, we need to erase completely the workspace
		if self.forcedWorkspaceCleanup:
			mySSH.command('echo ' + lPassWord + ' | sudo -S rm -Rf ' + lSourcePath, '\$', 15)

		oldRanCommidID = self.ranCommitID
		oldRanRepository = self.ranRepository
		oldRanAllowMerge = self.ranAllowMerge
		oldRanTargetBranch = self.ranTargetBranch
		self.ranCommitID = self.proxyCommit
		self.ranRepository = 'https://github.com/EpiSci/oai-lte-5g-multi-ue-proxy.git'
		self.ranAllowMerge = False
		self.ranTargetBranch = 'master'
		CreateWorkspace(mySSH, lSourcePath, self.ranRepository, self.ranCommitID, self.ranTargetBranch, self.ranAllowMerge)
		# to prevent accidentally overwriting data that might be used later
		self.ranCommitID = oldRanCommidID
		self.ranRepository = oldRanRepository
		self.ranAllowMerge = oldRanAllowMerge
		self.ranTargetBranch = oldRanTargetBranch

		# Let's remove any previous run artifacts if still there
		mySSH.command(self.cli + ' image prune --force', '\$', 30)
		# Remove any previous proxy image
		mySSH.command(self.cli + ' image rm oai-lte-multi-ue-proxy:latest || true', '\$', 30)

		tag = self.proxyCommit
		logging.debug('building L2sim proxy image for tag ' + tag)
		# check if the corresponding proxy image with tag exists. If not, build it
		mySSH.command(self.cli + ' image inspect --format=\'Size = {{.Size}} bytes\' proxy:' + tag, '\$', 5)
		buildProxy = mySSH.getBefore().count('o such image') != 0
		if buildProxy:
			mySSH.command(self.cli + ' build ' + self.cliBuildOptions + ' --target oai-lte-multi-ue-proxy --tag proxy:' + tag + ' --file docker/Dockerfile.ubuntu18.04 . > cmake_targets/log/proxy-build.log 2>&1', '\$', 180)
			# Note: at this point, OAI images are flattened, but we cannot do this
			# here, as the flatten script is not in the proxy repo
			mySSH.command(self.cli + ' image inspect --format=\'Size = {{.Size}} bytes\' proxy:' + tag, '\$', 5)
			mySSH.command(self.cli + ' image prune --force || true','\$', 15)
			if mySSH.getBefore().count('o such image') != 0:
				logging.error('\u001B[1m Build of L2sim proxy failed\u001B[0m')
				mySSH.close()
				HTML.CreateHtmlTestRow('commit ' + tag, 'KO', CONST.ALL_PROCESSES_OK)
				HTML.CreateHtmlTabFooter(False)
				return False
		else:
			logging.debug('L2sim proxy image for tag ' + tag + ' already exists, skipping build')

		# retag the build images to that we pick it up later
		mySSH.command('docker image tag proxy:' + tag + ' oai-lte-multi-ue-proxy:latest', '\$', 5)

		# no merge: is a push to develop, tag the image so we can push it to the registry
		if not self.ranAllowMerge:
			mySSH.command('docker image tag proxy:' + tag + ' proxy:develop', '\$', 5)

		# we assume that the host on which this is built will also run the proxy. The proxy
		# currently requires the following command, and the docker-compose up mechanism of
		# the CI does not allow to run arbitrary commands. Note that the following actually
		# belongs to the deployment, not the build of the proxy...
		logging.warning('the following command belongs to deployment, but no mechanism exists to exec it there!')
		mySSH.command('sudo ifconfig lo: 127.0.0.2 netmask 255.0.0.0 up', '\$', 5)

		# Analyzing the logs
		if buildProxy:
			self.testCase_id = HTML.testCase_id
			mySSH.command('cd ' + lSourcePath + '/cmake_targets', '\$', 5)
			mySSH.command('mkdir -p proxy_build_log_' + self.testCase_id, '\$', 5)
			mySSH.command('mv log/* ' + 'proxy_build_log_' + self.testCase_id, '\$', 5)
			if (os.path.isfile('./proxy_build_log_' + self.testCase_id + '.zip')):
				os.remove('./proxy_build_log_' + self.testCase_id + '.zip')
			if (os.path.isdir('./proxy_build_log_' + self.testCase_id)):
				shutil.rmtree('./proxy_build_log_' + self.testCase_id)
			mySSH.command('zip -r -qq proxy_build_log_' + self.testCase_id + '.zip proxy_build_log_' + self.testCase_id, '\$', 5)
			mySSH.copyin(lIpAddr, lUserName, lPassWord, lSourcePath + '/cmake_targets/build_log_' + self.testCase_id + '.zip', '.')
			# don't delete such that we might recover the zips
			#mySSH.command('rm -f build_log_' + self.testCase_id + '.zip','\$', 5)

		# we do not analyze the logs (we assume the proxy builds fine at this stage),
		# but need to have the following information to correctly display the HTML
		files = {}
		errorandwarnings = {}
		errorandwarnings['errors'] = 0
		errorandwarnings['warnings'] = 0
		errorandwarnings['status'] = True
		files['Target Image Creation'] = errorandwarnings
		collectInfo = {}
		collectInfo['proxy'] = files
		mySSH.command('docker image inspect --format=\'Size = {{.Size}} bytes\' proxy:' + tag, '\$', 5)
		result = re.search('Size *= *(?P<size>[0-9\-]+) *bytes', mySSH.getBefore())
		allImagesSize = {}
		if result is not None:
			imageSize = float(result.group('size')) / 1000000
			logging.debug('\u001B[1m   proxy size is ' + ('%.0f' % imageSize) + ' Mbytes\u001B[0m')
			allImagesSize['proxy'] = str(round(imageSize,1)) + ' Mbytes'
		else:
			logging.debug('proxy size is unknown')
			allImagesSize['proxy'] = 'unknown'

		# Cleaning any created tmp volume
		mySSH.command(self.cli + ' volume prune --force || true','\$', 15)
		mySSH.close()

		logging.info('\u001B[1m Building L2sim Proxy Image Pass\u001B[0m')
		HTML.CreateHtmlTestRow('commit ' + tag, 'OK', CONST.ALL_PROCESSES_OK)
		HTML.CreateHtmlNextTabHeaderTestRow(collectInfo, allImagesSize)

	def BuildRunTests(self, HTML):
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
		cmd = cls_cmd.RemoteCmd(lIpAddr)
		cmd.cd(lSourcePath)

		ret = cmd.run('hostnamectl')
		result = re.search('Ubuntu', ret.stdout)
		host = result.group(0)
		if host != 'Ubuntu':
			cmd.close()
			raise Exception("Can build unit tests only on Ubuntu server")
		logging.debug('running on Ubuntu as expected')

		if self.forcedWorkspaceCleanup:
			cmd.run(f'sudo -S rm -Rf {lSourcePath}')
		self.testCase_id = HTML.testCase_id
	
		# check that ran-base image exists as we expect it
		baseImage = 'ran-base'
		baseTag = 'develop'
		if self.ranAllowMerge:
			if self.ranTargetBranch == 'develop':
				cmd.run(f'git diff HEAD..origin/develop -- cmake_targets/build_oai cmake_targets/tools/build_helper docker/Dockerfile.base{self.dockerfileprefix} | grep --colour=never -i INDEX')
				result = re.search('index', cmd.getBefore())
				if result is not None:
					baseTag = 'develop'
		ret = cmd.run(f"docker image inspect --format=\'Size = {{{{.Size}}}} bytes\' {baseImage}:{baseTag}")
		if ret.returncode != 0:
			logging.error(f'No {baseImage} image present, cannot build tests')
			HTML.CreateHtmlTestRow(self.imageKind, 'KO', CONST.ALL_PROCESSES_OK)
			HTML.CreateHtmlTabFooter(False)
			return False

		# build ran-unittests image
		dockerfile = "ci-scripts/docker/Dockerfile.unittest.ubuntu20"
		ret = cmd.run(f'docker build --progress=plain --tag ran-unittests:{baseTag} --file {dockerfile} . &> {lSourcePath}/cmake_targets/log/unittest-build.log')
		if ret.returncode != 0:
			logging.error(f'Cannot build unit tests')
			HTML.CreateHtmlTestRow("Unit test build failed", 'KO', [dockerfile])
			HTML.CreateHtmlTabFooter(False)
			return False

		HTML.CreateHtmlTestRowQueue("Build unit tests", 'OK', [dockerfile])

		# it worked, build and execute tests, and close connection
		ret = cmd.run(f'docker run -a STDOUT --rm ran-unittests:develop ctest --output-on-failure --no-label-summary -j$(nproc)')
		cmd.run(f'docker rmi ran-unittests:develop')
		build_log_name = f'build_log_{self.testCase_id}'
		CopyLogsToExecutor(cmd, lSourcePath, build_log_name)
		cmd.close()

		if ret.returncode == 0:
			HTML.CreateHtmlTestRowQueue('Unit tests succeeded', 'OK', [ret.stdout])
			HTML.CreateHtmlTabFooter(True)
			return True
		else:
			HTML.CreateHtmlTestRowQueue('Unit tests failed (see also doc/UnitTests.md)', 'KO', [ret.stdout])
			HTML.CreateHtmlTabFooter(False)
			return False

	def Push_Image_to_Local_Registry(self, HTML):
		if self.registrySvrId == '0':
			lIpAddr = self.eNBIPAddress
			lUserName = self.eNBUserName
			lPassWord = self.eNBPassword
			lSourcePath = self.eNBSourceCodePath
		elif self.registrySvrId == '1':
			lIpAddr = self.eNB1IPAddress
			lUserName = self.eNB1UserName
			lPassWord = self.eNB1Password
			lSourcePath = self.eNB1SourceCodePath
		elif self.registrySvrId == '2':
			lIpAddr = self.eNB2IPAddress
			lUserName = self.eNB2UserName
			lPassWord = self.eNB2Password
			lSourcePath = self.eNB2SourceCodePath
		if lIpAddr == '' or lUserName == '' or lPassWord == '' or lSourcePath == '':
			HELP.GenericHelp(CONST.Version)
			sys.exit('Insufficient Parameter')
		logging.debug('Pushing images from server: ' + lIpAddr)
		mySSH = SSH.SSHConnection()
		mySSH.open(lIpAddr, lUserName, lPassWord)
		imagePrefix = 'porcepix.sboai.cs.eurecom.fr'
		mySSH.command(f'docker login -u oaicicd -p oaicicd {imagePrefix}', '\$', 5)
		if re.search('Login Succeeded', mySSH.getBefore()) is None:
			msg = 'Could not log into local registry'
			logging.error(msg)
			mySSH.close()
			HTML.CreateHtmlTestRow(msg, 'KO', CONST.ALL_PROCESSES_OK)
			return False

		orgTag = 'develop'
		if self.ranAllowMerge:
			orgTag = 'ci-temp'
		for image in IMAGES:
			tagToUse = ImageTagToUse(image, self.ranCommitID, self.ranBranch, self.ranAllowMerge)
			mySSH.command(f'docker image tag {image}:{orgTag} {imagePrefix}/{tagToUse}', '\$', 5)
			if re.search('Error response from daemon: No such image:', mySSH.getBefore()) is not None:
				continue
			mySSH.command(f'docker push {imagePrefix}/{tagToUse}', '\$', 120)
			if re.search(': digest:', mySSH.getBefore()) is None:
				logging.debug(mySSH.getBefore())
				msg = f'Could not push {image} to local registry : {tagToUse}'
				logging.error(msg)
				mySSH.close()
				HTML.CreateHtmlTestRow(msg, 'KO', CONST.ALL_PROCESSES_OK)
				return False
			mySSH.command(f'docker rmi {imagePrefix}/{tagToUse} {image}:{orgTag}', '\$', 30)

		mySSH.command(f'docker logout {imagePrefix}', '\$', 5)
		if re.search('Removing login credentials', mySSH.getBefore()) is None:
			msg = 'Could not log off from local registry'
			logging.error(msg)
			mySSH.close()
			HTML.CreateHtmlTestRow(msg, 'KO', CONST.ALL_PROCESSES_OK)
			return False

		mySSH.close()
		HTML.CreateHtmlTestRow('N/A', 'OK', CONST.ALL_PROCESSES_OK)
		return True

	def Pull_Image_from_Local_Registry(self, HTML):
		# This method can be called either onto a remote server (different from python executor)
		# or directly on the python executor (ie lIpAddr == 'none')
		if self.testSvrId == '0':
			lIpAddr = self.eNBIPAddress
			lUserName = self.eNBUserName
			lPassWord = self.eNBPassword
			lSourcePath = self.eNBSourceCodePath
		elif self.testSvrId == '1':
			lIpAddr = self.eNB1IPAddress
			lUserName = self.eNB1UserName
			lPassWord = self.eNB1Password
			lSourcePath = self.eNB1SourceCodePath
		elif self.testSvrId == '2':
			lIpAddr = self.eNB2IPAddress
			lUserName = self.eNB2UserName
			lPassWord = self.eNB2Password
			lSourcePath = self.eNB2SourceCodePath
		if lIpAddr == '' or lUserName == '' or lPassWord == '' or lSourcePath == '':
			HELP.GenericHelp(CONST.Version)
			sys.exit('Insufficient Parameter')
		logging.debug('\u001B[1m Pulling image(s) on server: ' + lIpAddr + '\u001B[0m')
		myCmd = cls_cmd.getConnection(lIpAddr)
		imagePrefix = 'porcepix.sboai.cs.eurecom.fr'
		response = myCmd.run(f'docker login -u oaicicd -p oaicicd {imagePrefix}')
		if response.returncode != 0:
			msg = 'Could not log into local registry'
			logging.error(msg)
			myCmd.close()
			HTML.CreateHtmlTestRow(msg, 'KO', CONST.ALL_PROCESSES_OK)
			return False
		for image in self.imageToPull:
			tagToUse = ImageTagToUse(image, self.ranCommitID, self.ranBranch, self.ranAllowMerge)
			cmd = f'docker pull {imagePrefix}/{tagToUse}'
			response = myCmd.run(cmd, timeout=120)
			if response.returncode != 0:
				logging.debug(response)
				msg = f'Could not pull {image} from local registry : {tagToUse}'
				logging.error(msg)
				myCmd.close()
				HTML.CreateHtmlTestRow('msg', 'KO', CONST.ALL_PROCESSES_OK)
				return False
			myCmd.run(f'docker tag {imagePrefix}/{tagToUse} oai-ci/{tagToUse}')
			myCmd.run(f'docker rmi {imagePrefix}/{tagToUse}')
		response = myCmd.run(f'docker logout {imagePrefix}')
		if response.returncode != 0:
			msg = 'Could not log off from local registry'
			logging.error(msg)
			myCmd.close()
			HTML.CreateHtmlTestRow(msg, 'KO', CONST.ALL_PROCESSES_OK)
			return False
		myCmd.close()
		HTML.CreateHtmlTestRow('N/A', 'OK', CONST.ALL_PROCESSES_OK)
		return True

	def Clean_Test_Server_Images(self, HTML):
		# This method can be called either onto a remote server (different from python executor)
		# or directly on the python executor (ie lIpAddr == 'none')
		if self.testSvrId == '0':
			lIpAddr = self.eNBIPAddress
			lUserName = self.eNBUserName
			lPassWord = self.eNBPassword
			lSourcePath = self.eNBSourceCodePath
		elif self.testSvrId == '1':
			lIpAddr = self.eNB1IPAddress
			lUserName = self.eNB1UserName
			lPassWord = self.eNB1Password
			lSourcePath = self.eNB1SourceCodePath
		elif self.testSvrId == '2':
			lIpAddr = self.eNB2IPAddress
			lUserName = self.eNB2UserName
			lPassWord = self.eNB2Password
			lSourcePath = self.eNB2SourceCodePath
		if lIpAddr == '' or lUserName == '' or lPassWord == '' or lSourcePath == '':
			HELP.GenericHelp(CONST.Version)
			sys.exit('Insufficient Parameter')
		if lIpAddr != 'none':
			logging.debug('Removing test images from server: ' + lIpAddr)
			myCmd = cls_cmd.RemoteCmd(lIpAddr)
		else:
			logging.debug('Removing test images locally')
			myCmd = cls_cmd.LocalCmd()

		for image in IMAGES:
			imageTag = ImageTagToUse(image, self.ranCommitID, self.ranBranch, self.ranAllowMerge)
			cmd = f'docker rmi oai-ci/{imageTag}'
			myCmd.run(cmd, reportNonZero=False)

		myCmd.close()
		HTML.CreateHtmlTestRow('N/A', 'OK', CONST.ALL_PROCESSES_OK)
		return True

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
		self.deployKind[self.eNB_instance] = True

		mySSH = SSH.SSHConnection()
		mySSH.open(lIpAddr, lUserName, lPassWord)

		CreateWorkspace(mySSH, lSourcePath, self.ranRepository, self.ranCommitID, self.ranTargetBranch, self.ranAllowMerge)

		mySSH.command('cd ' + lSourcePath + '/' + self.yamlPath[self.eNB_instance], '\$', 5)
		mySSH.command('cp docker-compose.y*ml ci-docker-compose.yml', '\$', 5)
		for image in IMAGES:
			imageTag = ImageTagToUse(image, self.ranCommitID, self.ranBranch, self.ranAllowMerge)
			mySSH.command(f'sed -i -e "s#image: {image}:latest#image: oai-ci/{imageTag}#" ci-docker-compose.yml', '\$', 2)

		# Currently support only one
		svcName = self.services[self.eNB_instance]
		if svcName == '':
			logging.warning('no service name given: starting all services in ci-docker-compose.yml!')

		mySSH.command(f'docker-compose --file ci-docker-compose.yml up -d -- {svcName}', '\$', 30)

		# Checking Status
		grep = ''
		if svcName != '': grep = f' | grep -A3 {svcName}'
		mySSH.command(f'docker-compose --file ci-docker-compose.yml config {grep}', '\$', 5)
		result = re.search('container_name: (?P<container_name>[a-zA-Z0-9\-\_]+)', mySSH.getBefore())
		unhealthyNb = 0
		healthyNb = 0
		startingNb = 0
		containerName = ''
		usedImage = ''
		imageInfo = ''
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

			mySSH.command('docker inspect --format="ImageUsed: {{.Config.Image}}" ' + containerName, '\$', 5)
			for stdoutLine in mySSH.getBefore().split('\n'):
				if stdoutLine.count('ImageUsed: oai-ci'):
					usedImage = stdoutLine.replace('ImageUsed: oai-ci', 'oai-ci').strip()
					logging.debug('Used image is ' + usedImage)
			if usedImage != '':
				mySSH.command('docker image inspect --format "* Size     = {{.Size}} bytes\n* Creation = {{.Created}}\n* Id       = {{.Id}}" ' + usedImage, '\$', 5, silent=True)
				for stdoutLine in mySSH.getBefore().split('\n'):
					if re.search('Size     = [0-9]', stdoutLine) is not None:
						imageInfo += stdoutLine.strip() + '\n'
					if re.search('Creation = [0-9]', stdoutLine) is not None:
						imageInfo += stdoutLine.strip() + '\n'
					if re.search('Id       = sha256', stdoutLine) is not None:
						imageInfo += stdoutLine.strip() + '\n'
		logging.debug(' -- ' + str(healthyNb) + ' healthy container(s)')
		logging.debug(' -- ' + str(unhealthyNb) + ' unhealthy container(s)')
		logging.debug(' -- ' + str(startingNb) + ' still starting container(s)')

		self.testCase_id = HTML.testCase_id
		self.eNB_logFile[self.eNB_instance] = 'enb_' + self.testCase_id + '.log'

		status = False
		if healthyNb == 1:
			cnt = 0
			while (cnt < 20):
				mySSH.command('docker logs ' + containerName + ' | egrep --text --color=never -i "wait|sync|Starting"', '\$', 30)
				result = re.search('got sync|Starting E1AP at CU UP|Starting F1AP at CU|Got sync|Waiting for RUs to be configured', mySSH.getBefore())
				if result is None:
					time.sleep(6)
					cnt += 1
				else:
					cnt = 100
					status = True
					logging.info('\u001B[1m Deploying OAI object Pass\u001B[0m')
					time.sleep(10)
		else:
			# containers are unhealthy, so we won't start. However, logs are stored at the end
			# in UndeployObject so we here store the logs of the unhealthy container to report it
			logfilename = f'{lSourcePath}/cmake_targets/log/{self.eNB_logFile[self.eNB_instance]}'
			mySSH.command(f'docker logs {containerName} > {logfilename}', '\$', 30)
			mySSH.copyin(lIpAddr, lUserName, lPassWord, logfilename, '.')
		mySSH.close()

		message = ''
		if usedImage != '':
			message += f'Used Image = {usedImage} :\n'
			message += imageInfo
		else:
			message += 'Could not retrieve used image info!\n'
		if status:
			message += '\nHealthy deployment!\n'
		else:
			message += '\nUnhealthy deployment! -- Check logs for reason!\n'
		if status:
			HTML.CreateHtmlTestRowQueue('N/A', 'OK', [message])
		else:
			self.exitStatus = 1
			HTML.CreateHtmlTestRowQueue('N/A', 'KO', [message])


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
		logging.debug('\u001B[1m Undeploying OAI Object from server: ' + lIpAddr + '\u001B[0m')
		mySSH = SSH.SSHConnection()
		mySSH.open(lIpAddr, lUserName, lPassWord)

		mySSH.command('cd ' + lSourcePath + '/' + self.yamlPath[self.eNB_instance], '\$', 5)

		svcName = self.services[self.eNB_instance]
		forceDown = False
		if svcName != '':
			logging.warning(f'service name given, but will stop all services in ci-docker-compose.yml!')
			svcName = ''

		mySSH.command(f'docker-compose -f ci-docker-compose.yml config --services', '\$', 5)
		# first line has command, last line has next command prompt
		allServices = mySSH.getBefore().split('\r\n')[1:-1]
		services = []
		for s in allServices:
			mySSH.command(f'docker-compose -f ci-docker-compose.yml ps --all -- {s}', '\$', 5, silent=False)
			running = mySSH.getBefore().split('\r\n')[2:-1]
			logging.debug(f'running services: {running}')
			if len(running) > 0: # something is running for that service
				services.append(s)
		logging.info(f'stopping services {services}')

		mySSH.command(f'docker-compose -f ci-docker-compose.yml stop -t3', '\$', 30)
		time.sleep(5)  # give some time to running containers to stop
		for svcName in services:
			# head -n -1 suppresses the final "X exited with status code Y"
			filename = f'{svcName}-{HTML.testCase_id}.log'
			mySSH.command(f'docker-compose -f ci-docker-compose.yml logs --no-log-prefix -- {svcName} &> {lSourcePath}/cmake_targets/log/{filename}', '\$', 120)

		mySSH.command('docker-compose -f ci-docker-compose.yml down -v', '\$', 5)
		mySSH.close()

		# Analyzing log file!
		files = ','.join([f'{s}-{HTML.testCase_id}' for s in services])
		if len(services) > 1:
			files = '{' + files + '}'
		copyin_res = 0
		if len(services) > 0:
			copyin_res = mySSH.copyin(lIpAddr, lUserName, lPassWord, f'{lSourcePath}/cmake_targets/log/{files}.log', '.')
		if copyin_res == -1:
			HTML.htmleNBFailureMsg='Could not copy logfile to analyze it!'
			HTML.CreateHtmlTestRow('N/A', 'KO', CONST.ENB_PROCESS_NOLOGFILE_TO_ANALYZE)
			self.exitStatus = 1
		# use function for UE log analysis, when oai-nr-ue container is used
		elif 'oai-nr-ue' in services or 'lte_ue0' in services:
			self.exitStatus == 0
			logging.debug('\u001B[1m Analyzing UE logfile ' + filename + ' \u001B[0m')
			logStatus = cls_oaicitest.OaiCiTest().AnalyzeLogFile_UE(f'{filename}', HTML, RAN)
			if (logStatus < 0):
				fullStatus = False
				HTML.CreateHtmlTestRow('UE log Analysis', 'KO', logStatus)
			else:
				HTML.CreateHtmlTestRow('UE log Analysis', 'OK', CONST.ALL_PROCESSES_OK)
		else:
			for svcName in services:
				filename = f'{svcName}-{HTML.testCase_id}.log'
				logging.debug(f'\u001B[1m Analyzing logfile {filename}\u001B[0m')
				logStatus = RAN.AnalyzeLogFile_eNB(filename, HTML, self.ran_checkers)
				if (logStatus < 0):
					HTML.CreateHtmlTestRow(RAN.runtime_stats, 'KO', logStatus)
					self.exitStatus = 1
				else:
					HTML.CreateHtmlTestRow(RAN.runtime_stats, 'OK', CONST.ALL_PROCESSES_OK)
			# all the xNB run logs shall be on the server 0 for logCollecting
			if self.eNB_serverId[self.eNB_instance] != '0':
				mySSH.copyout(self.eNBIPAddress, self.eNBUserName, self.eNBPassword, f'./{files}.log', f'{self.eNBSourceCodePath}/cmake_targets/')
		if self.exitStatus == 0:
			logging.info('\u001B[1m Undeploying OAI Object Pass\u001B[0m')
		else:
			logging.error('\u001B[1m Undeploying OAI Object Failed\u001B[0m')

	def DeployGenObject(self, HTML, RAN, UE):
		self.exitStatus = 0
		logging.debug('\u001B[1m Checking Services to deploy\u001B[0m')
		# Implicitly we are running locally
		myCmd = cls_cmd.LocalCmd(d = self.yamlPath[0])
		self.deployKind[0] = False
		cmd = 'docker-compose config --services'
		listServices = myCmd.run(cmd)
		if listServices.returncode != 0:
			myCmd.close()
			self.exitStatus = 1
			HTML.CreateHtmlTestRow('SVC not Found', 'KO', CONST.ALL_PROCESSES_OK)
			return
		for reqSvc in self.services[0].split(' '):
			res = re.search(reqSvc, listServices.stdout)
			if res is None:
				logging.error(reqSvc + ' not found in specified docker-compose')
				self.exitStatus = 1
		if (self.exitStatus == 1):
			myCmd.close()
			HTML.CreateHtmlTestRow('SVC not Found', 'KO', CONST.ALL_PROCESSES_OK)
			return
		cmd = 'cp docker-compose.y*ml docker-compose-ci.yml'
		myCmd.run(cmd, silent=self.displayedNewTags)
		imageNames = ['oai-enb', 'oai-gnb', 'oai-lte-ue', 'oai-nr-ue', 'oai-lte-ru', 'oai-nr-cuup']
		for image in imageNames:
			tagToUse = ImageTagToUse(image, self.ranCommitID, self.ranBranch, self.ranAllowMerge)
			# In the scenario, for 5G images, we have the choice of either pulling normal images
			# or -asan images. We need to detect which kind we did pull.
			if image == 'oai-gnb' or image == 'oai-nr-ue' or image == 'oai-nr-cuup':
				ret = myCmd.run(f'docker image inspect oai-ci/{tagToUse}', reportNonZero=False, silent=self.displayedNewTags)
				if ret.returncode != 0:
					tagToUse = tagToUse.replace('oai-gnb', 'oai-gnb-asan')
					tagToUse = tagToUse.replace('oai-nr-ue', 'oai-nr-ue-asan')
					tagToUse = tagToUse.replace('oai-nr-cuup', 'oai-nr-cuup-asan')
					if not self.displayedNewTags:
						logging.debug(f'\u001B[1m Using sanitized version of {image} with {tagToUse}\u001B[0m')
			cmd = f'sed -i -e "s@oaisoftwarealliance/{image}:develop@oai-ci/{tagToUse}@" docker-compose-ci.yml'
			myCmd.run(cmd, silent=self.displayedNewTags)
		self.displayedNewTags = True

		cmd = f'docker-compose -f docker-compose-ci.yml up -d {self.services[0]}'
		deployStatus = myCmd.run(cmd, timeout=100)
		if deployStatus.returncode != 0:
			myCmd.close()
			self.exitStatus = 1
			logging.error('Could not deploy')
			HTML.CreateHtmlTestRow('Could not deploy', 'KO', CONST.ALL_PROCESSES_OK)
			return

		logging.debug('\u001B[1m Checking if all deployed healthy\u001B[0m')
		cmd = 'docker-compose -f docker-compose-ci.yml ps -a'
		count = 0
		healthy = 0
		restarting = 0
		newContainers = []
		while (count < 10):
			count += 1
			containerStatus = []
			deployStatus = myCmd.run(cmd, 30, silent=True)
			healthy = 0
			restarting = 0
			for state in deployStatus.stdout.split('\n'):
				state = state.strip()
				res = re.search('Name|NAME|----------', state)
				if res is not None:
					continue
				if len(state) == 0:
					continue
				res = re.search('^(?P<container_name>[a-zA-Z0-9\-\_]+) ', state)
				if res is not None:
					cName = res.group('container_name')
					found = False
					for alreadyDeployed in self.deployedContainers:
						if cName == alreadyDeployed:
							found = True
					if not found:
						newContainers.append(cName)
						self.deployedContainers.append(cName)
				if re.search('\(healthy\)', state) is not None:
					healthy += 1
				if re.search('rfsim4g-db-init.*Exit 0', state) is not None or re.search('rfsim4g-db-init.*Exited \(0\)', state) is not None:
					myCmd.run('docker rm -f rfsim4g-db-init', timeout=30, silent=True, reportNonZero=False)
				if re.search('l2sim4g-db-init.*Exit 0', state) is not None or re.search('l2sim4g-db-init.*Exited \(0\)', state) is not None:
					myCmd.run('docker rm -f l2sim4g-db-init', timeout=30, silent=True, reportNonZero=False)
				if re.search('Restarting', state) is None:
					containerStatus.append(state)
				else:
					restarting += 1
			if healthy == self.nb_healthy[0] and restarting == 0:
				count = 100
			else:
				time.sleep(10)

		html_cell = ''
		for newCont in newContainers:
			if newCont == 'rfsim4g-db-init':
				continue
			cmd = 'docker inspect -f "{{.Config.Image}}" ' + newCont
			imageInspect = myCmd.run(cmd, timeout=30, silent=True)
			imageName = str(imageInspect.stdout).strip()
			cmd = 'docker image inspect --format \'{{.RepoTags}}\t{{.Size}} bytes\t{{index (split .Created ".") 0}}\n{{.Id}}\' ' + imageName
			imageInspect = myCmd.run(cmd, 30, silent=True)
			html_cell += imageInspect.stdout + '\n'
		myCmd.close()

		for cState in containerStatus:
			html_cell += cState + '\n'
		if count == 100 and healthy == self.nb_healthy[0]:
			if self.tsharkStarted == False:
				logging.debug('Starting tshark on public network')
				self.CaptureOnDockerNetworks()
			HTML.CreateHtmlTestRowQueue('n/a', 'OK', [html_cell])
			for cState in containerStatus:
				logging.debug(cState)
			logging.info('\u001B[1m Deploying OAI Object(s) PASS\u001B[0m')
		else:
			HTML.CreateHtmlTestRowQueue('Could not deploy in time', 'KO', [html_cell])
			for cState in containerStatus:
				logging.debug(cState)
			logging.error('\u001B[1m Deploying OAI Object(s) FAILED\u001B[0m')
			HTML.testCase_id = 'AUTO-UNDEPLOY'
			UE.testCase_id = 'AUTO-UNDEPLOY'
			HTML.desc = 'Automatic Undeployment'
			UE.desc = 'Automatic Undeployment'
			UE.ShowTestID()
			self.UndeployGenObject(HTML, RAN, UE)
			self.exitStatus = 1

	# pyshark livecapture launches 2 processes:
	# * One using dumpcap -i lIfs -w - (ie redirecting the packets to STDOUT)
	# * One using tshark -i - -w loFile (ie capturing from STDIN from previous process)
	# but in fact the packets are read by the following loop before being in fact
	# really written to loFile.
	# So it is mandatory to keep the loop
	def LaunchPySharkCapture(self, lIfs, lFilter, loFile):
		capture = pyshark.LiveCapture(interface=lIfs, bpf_filter=lFilter, output_file=loFile, debug=False)
		for packet in capture.sniff_continuously():
			pass

	def CaptureOnDockerNetworks(self):
		myCmd = cls_cmd.LocalCmd(d = self.yamlPath[0])
		cmd = 'docker-compose -f docker-compose-ci.yml config | grep com.docker.network.bridge.name | sed -e "s@^.*name: @@"'
		networkNames = myCmd.run(cmd, silent=True)
		myCmd.close()
		# Allow only: control plane RAN (SCTP), HTTP of control in CN (port 80), PFCP traffic (port 8805), MySQL (port 3306)
		capture_filter = 'sctp or port 80 or port 8805 or icmp or port 3306'
		interfaces = []
		iInterfaces = ''
		for name in networkNames.stdout.split('\n'):
			if re.search('rfsim', name) is not None or re.search('l2sim', name) is not None:
				interfaces.append(name)
				iInterfaces += f'-i {name} '
		ymlPath = self.yamlPath[0].split('/')
		output_file = f'/tmp/capture_{ymlPath[1]}.pcap'
		self.tsharkStarted = True
		# On old systems (ubuntu 18), pyshark live-capture is buggy.
		# Going back to old method
		if sys.version_info < (3, 7):
			cmd = f'nohup tshark -f "{capture_filter}" {iInterfaces} -w {output_file} > /tmp/tshark.log 2>&1 &'
			myCmd = cls_cmd.LocalCmd()
			myCmd.run(cmd, timeout=5, reportNonZero=False)
			myCmd.close()
			return
		x = threading.Thread(target = self.LaunchPySharkCapture, args = (interfaces,capture_filter,output_file,))
		x.daemon = True
		x.start()

	def UndeployGenObject(self, HTML, RAN, UE):
		self.exitStatus = 0
		# Implicitly we are running locally
		ymlPath = self.yamlPath[0].split('/')
		logPath = '../cmake_targets/log/' + ymlPath[1]
		myCmd = cls_cmd.LocalCmd(d = self.yamlPath[0])
		cmd = 'cp docker-compose.y*ml docker-compose-ci.yml'
		myCmd.run(cmd)

		# check which containers are running for log recovery later
		cmd = 'docker-compose -f docker-compose-ci.yml ps --all'
		deployStatusLogs = myCmd.run(cmd, timeout=30)

		# Stop the containers to shut down objects
		logging.debug('\u001B[1m Stopping containers \u001B[0m')
		cmd = 'docker-compose -f docker-compose-ci.yml stop -t3'
		deployStatus = myCmd.run(cmd, timeout=100)
		if deployStatus.returncode != 0:
			myCmd.close()
			self.exitStatus = 1
			logging.error('Could not stop containers')
			HTML.CreateHtmlTestRow('Could not stop', 'KO', CONST.ALL_PROCESSES_OK)
			logging.error('\u001B[1m Undeploying OAI Object(s) FAILED\u001B[0m')
			return

		anyLogs = False
		logging.debug('Working dir is now . (ie ci-scripts)')
		myCmd2 = cls_cmd.LocalCmd()
		myCmd2.run(f'mkdir -p {logPath}')
		myCmd2.cd(logPath)
		for state in deployStatusLogs.stdout.split('\n'):
			res = re.search('Name|NAME|----------', state)
			if res is not None:
				continue
			if len(state) == 0:
				continue
			res = re.search('^(?P<container_name>[a-zA-Z0-9\-\_]+) ', state)
			if res is not None:
				anyLogs = True
				cName = res.group('container_name')
				cmd = f'docker logs {cName} > {cName}.log 2>&1'
				myCmd2.run(cmd, timeout=30, reportNonZero=False)
				if re.search('magma-mme', cName) is not None:
					cmd = f'docker cp -L {cName}:/var/log/mme.log {cName}-full.log'
					myCmd2.run(cmd, timeout=30, reportNonZero=False)
		fullStatus = True
		if anyLogs:
			# Analyzing log file(s)!
			listOfPossibleRanContainers = ['enb*', 'gnb*', 'cu*', 'du*']
			for container in listOfPossibleRanContainers:
				filenames = './*-oai-' + container + '.log'
				cmd = f'ls {filenames}'
				lsStatus = myCmd2.run(cmd, silent=True, reportNonZero=False)
				if lsStatus.returncode != 0:
					continue
				filenames = str(lsStatus.stdout).strip()

				for filename in filenames.split('\n'):
					logging.debug('\u001B[1m Analyzing xNB logfile ' + filename + ' \u001B[0m')
					logStatus = RAN.AnalyzeLogFile_eNB(f'{logPath}/{filename}', HTML, self.ran_checkers)
					if (logStatus < 0):
						fullStatus = False
						self.exitStatus = 1
						HTML.CreateHtmlTestRow(RAN.runtime_stats, 'KO', logStatus)
					else:
						HTML.CreateHtmlTestRow(RAN.runtime_stats, 'OK', CONST.ALL_PROCESSES_OK)

			listOfPossibleUeContainers = ['lte-ue*', 'nr-ue*']
			for container in listOfPossibleUeContainers:
				filenames = './*-oai-' + container + '.log'
				cmd = f'ls {filenames}'
				lsStatus = myCmd2.run(cmd, silent=True, reportNonZero=False)
				if lsStatus.returncode != 0:
					continue
				filenames = str(lsStatus.stdout).strip()

				for filename in filenames.split('\n'):
					logging.debug('\u001B[1m Analyzing UE logfile ' + filename + ' \u001B[0m')
					logStatus = UE.AnalyzeLogFile_UE(f'{logPath}/{filename}', HTML, RAN)
					if (logStatus < 0):
						fullStatus = False
						HTML.CreateHtmlTestRow('UE log Analysis', 'KO', logStatus)
					else:
						HTML.CreateHtmlTestRow('UE log Analysis', 'OK', CONST.ALL_PROCESSES_OK)

			if self.tsharkStarted:
				self.tsharkStarted = True
				cmd = 'killall tshark'
				myCmd2.run(cmd, reportNonZero=False)
				cmd = 'killall dumpcap'
				myCmd2.run(cmd, reportNonZero=False)
				time.sleep(5)
				ymlPath = self.yamlPath[0].split('/')
				# The working dir is still logPath
				cmd = f'mv /tmp/capture_{ymlPath[1]}.pcap .'
				myCmd2.run(cmd, timeout=100, reportNonZero=False)
				self.tsharkStarted = False
		myCmd2.close()

		logging.debug('\u001B[1m Undeploying \u001B[0m')
		logging.debug(f'Working dir is back {self.yamlPath[0]}')
		cmd = 'docker-compose -f docker-compose-ci.yml down -v'
		deployStatus = myCmd.run(cmd, timeout=100)
		if deployStatus.returncode != 0:
			myCmd.close()
			self.exitStatus = 1
			logging.error('Could not undeploy')
			HTML.CreateHtmlTestRow('Could not undeploy', 'KO', CONST.ALL_PROCESSES_OK)
			logging.error('\u001B[1m Undeploying OAI Object(s) FAILED\u001B[0m')
			return

		self.deployedContainers = []
		myCmd.close()

		if fullStatus:
			HTML.CreateHtmlTestRow('n/a', 'OK', CONST.ALL_PROCESSES_OK)
			logging.info('\u001B[1m Undeploying OAI Object(s) PASS\u001B[0m')
		else:
			HTML.CreateHtmlTestRow('n/a', 'KO', CONST.ALL_PROCESSES_OK)
			logging.error('\u001B[1m Undeploying OAI Object(s) FAIL\u001B[0m')

	def StatsFromGenObject(self, HTML):
		self.exitStatus = 0
		ymlPath = self.yamlPath[0].split('/')
		logPath = '../cmake_targets/log/' + ymlPath[1]

		# if the containers are running, recover the logs!
		myCmd = cls_cmd.LocalCmd(d = self.yamlPath[0])
		cmd = 'docker-compose -f docker-compose-ci.yml ps --all'
		deployStatus = myCmd.run(cmd, timeout=30)
		cmd = 'docker stats --no-stream --format "table {{.Container}}\t{{.CPUPerc}}\t{{.MemUsage}}\t{{.MemPerc}}" '
		anyLogs = False
		for state in deployStatus.stdout.split('\n'):
			res = re.search('Name|NAME|----------', state)
			if res is not None:
				continue
			if len(state) == 0:
				continue
			res = re.search('^(?P<container_name>[a-zA-Z0-9\-\_]+) ', state)
			if res is not None:
				anyLogs = True
				cmd += res.group('container_name') + ' '
		message = ''
		if anyLogs:
			stats = myCmd.run(cmd, timeout=30)
			for statLine in stats.stdout.split('\n'):
				logging.debug(statLine)
				message += statLine + '\n'
		myCmd.close()

		HTML.CreateHtmlTestRowQueue(self.pingOptions, 'OK', [message])

	def PingExit(self, HTML, RAN, UE, status, message):
		if status:
			HTML.CreateHtmlTestRowQueue(self.pingOptions, 'OK', [message])
		else:
			logging.error('\u001B[1;37;41m ping test FAIL -- ' + message + ' \u001B[0m')
			HTML.CreateHtmlTestRowQueue(self.pingOptions, 'KO', [message])
			# Automatic undeployment
			logging.warning('----------------------------------------')
			logging.warning('\u001B[1m Starting Automatic undeployment \u001B[0m')
			logging.warning('----------------------------------------')
			HTML.testCase_id = 'AUTO-UNDEPLOY'
			HTML.desc = 'Automatic Un-Deployment'
			self.UndeployGenObject(HTML, RAN, UE)
			self.exitStatus = 1

	def IperfFromContainer(self, HTML, RAN, UE):
		myCmd = cls_cmd.LocalCmd()
		self.exitStatus = 0

		ymlPath = self.yamlPath[0].split('/')
		logPath = '../cmake_targets/log/' + ymlPath[1]
		cmd = f'mkdir -p {logPath}'
		myCmd.run(cmd, silent=True)

		# Start the server process
		cmd = f'docker exec -d {self.svrContName} /bin/bash -c "nohup iperf {self.svrOptions} > /tmp/iperf_server.log 2>&1"'
		myCmd.run(cmd)
		time.sleep(3)

		# Start the client process
		cmd = f'docker exec {self.cliContName} /bin/bash -c "iperf {self.cliOptions}" 2>&1 | tee {logPath}/iperf_client_{HTML.testCase_id}.log'
		clientStatus = myCmd.run(cmd, timeout=100)

		# Stop the server process
		cmd = f'docker exec {self.svrContName} /bin/bash -c "pkill iperf"'
		myCmd.run(cmd)
		time.sleep(3)
		serverStatusFilename = f'{logPath}/iperf_server_{HTML.testCase_id}.log'
		cmd = f'docker cp {self.svrContName}:/tmp/iperf_server.log {serverStatusFilename}'
		myCmd.run(cmd, timeout=60)
		myCmd.close()

		# clientStatus was retrieved above. The serverStatus was
		# written in the background, then copied to the local machine
		with open(serverStatusFilename, 'r') as f:
			serverStatus = f.read()
		(iperfStatus, msg) = AnalyzeIperf(self.cliOptions, clientStatus.stdout, serverStatus)
		if iperfStatus:
			logging.info('\u001B[1m Iperf Test PASS\u001B[0m')
		else:
			logging.error('\u001B[1;37;41m Iperf Test FAIL\u001B[0m')
		self.IperfExit(HTML, RAN, UE, iperfStatus, msg)

	def IperfExit(self, HTML, RAN, UE, status, message):
		html_cell = f'UE\n{message}'
		if status:
			HTML.CreateHtmlTestRowQueue(self.cliOptions, 'OK', [html_cell])
		else:
			logging.error('\u001B[1m Iperf Test FAIL -- ' + message + ' \u001B[0m')
			HTML.CreateHtmlTestRowQueue(self.cliOptions, 'KO', [html_cell])
			# Automatic undeployment
			logging.warning('----------------------------------------')
			logging.warning('\u001B[1m Starting Automatic undeployment \u001B[0m')
			logging.warning('----------------------------------------')
			HTML.testCase_id = 'AUTO-UNDEPLOY'
			HTML.desc = 'Automatic Un-Deployment'
			self.UndeployGenObject(HTML, RAN, UE)
			self.exitStatus = 1


	def CheckAndAddRoute(self, svrName, ipAddr, userName, password):
		logging.debug('Checking IP routing on ' + svrName)
		mySSH = SSH.SSHConnection()
		if svrName == 'porcepix':
			mySSH.open(ipAddr, userName, password)
			# Check if route to asterix gnb exists
			mySSH.command('ip route | grep --colour=never "192.168.68.64/26"', '\$', 10)
			result = re.search('172.21.16.127', mySSH.getBefore())
			if result is None:
				mySSH.command('echo ' + password + ' | sudo -S ip route add 192.168.68.64/26 via 172.21.16.127 dev eno1', '\$', 10)
			# Check if route to obelix enb exists
			mySSH.command('ip route | grep --colour=never "192.168.68.128/26"', '\$', 10)
			result = re.search('172.21.16.128', mySSH.getBefore())
			if result is None:
				mySSH.command('echo ' + password + ' | sudo -S ip route add 192.168.68.128/26 via 172.21.16.128 dev eno1', '\$', 10)
			# Check if forwarding is enabled
			mySSH.command('sysctl net.ipv4.conf.all.forwarding', '\$', 10)
			result = re.search('net.ipv4.conf.all.forwarding = 1', mySSH.getBefore())
			if result is None:
				mySSH.command('echo ' + password + ' | sudo -S sysctl net.ipv4.conf.all.forwarding=1', '\$', 10)
			# Check if iptables forwarding is accepted
			mySSH.command('echo ' + password + ' | sudo -S iptables -L FORWARD', '\$', 10)
			result = re.search('Chain FORWARD .*policy ACCEPT', mySSH.getBefore())
			if result is None:
				mySSH.command('echo ' + password + ' | sudo -S iptables -P FORWARD ACCEPT', '\$', 10)
			mySSH.close()
		if svrName == 'asterix':
			mySSH.open(ipAddr, userName, password)
			# Check if route to porcepix epc exists
			mySSH.command('ip route | grep --colour=never "192.168.61.192/26"', '\$', 10)
			result = re.search('172.21.16.136', mySSH.getBefore())
			if result is None:
				mySSH.command('echo ' + password + ' | sudo -S ip route add 192.168.61.192/26 via 172.21.16.136 dev em1', '\$', 10)
			# Check if route to porcepix cn5g exists
			mySSH.command('ip route | grep --colour=never "192.168.70.128/26"', '\$', 10)
			result = re.search('172.21.16.136', mySSH.getBefore())
			if result is None:
				mySSH.command('echo ' + password + ' | sudo -S ip route add 192.168.70.128/26 via 172.21.16.136 dev em1', '\$', 10)
			# Check if X2 route to obelix enb exists
			mySSH.command('ip route | grep --colour=never "192.168.68.128/26"', '\$', 10)
			result = re.search('172.21.16.128', mySSH.getBefore())
			if result is None:
				mySSH.command('echo ' + password + ' | sudo -S ip route add 192.168.68.128/26 via 172.21.16.128 dev em1', '\$', 10)
			# Check if forwarding is enabled
			mySSH.command('sysctl net.ipv4.conf.all.forwarding', '\$', 10)
			result = re.search('net.ipv4.conf.all.forwarding = 1', mySSH.getBefore())
			if result is None:
				mySSH.command('echo ' + password + ' | sudo -S sysctl net.ipv4.conf.all.forwarding=1', '\$', 10)
			# Check if iptables forwarding is accepted
			mySSH.command('echo ' + password + ' | sudo -S iptables -L FORWARD', '\$', 10)
			result = re.search('Chain FORWARD .*policy ACCEPT', mySSH.getBefore())
			if result is None:
				mySSH.command('echo ' + password + ' | sudo -S iptables -P FORWARD ACCEPT', '\$', 10)
			mySSH.close()
		if svrName == 'obelix':
			mySSH.open(ipAddr, userName, password)
			# Check if route to porcepix epc exists
			mySSH.command('ip route | grep --colour=never "192.168.61.192/26"', '\$', 10)
			result = re.search('172.21.16.136', mySSH.getBefore())
			if result is None:
				mySSH.command('echo ' + password + ' | sudo -S ip route add 192.168.61.192/26 via 172.21.16.136 dev eno1', '\$', 10)
			# Check if X2 route to asterix gnb exists
			mySSH.command('ip route | grep --colour=never "192.168.68.64/26"', '\$', 10)
			result = re.search('172.21.16.127', mySSH.getBefore())
			if result is None:
				mySSH.command('echo ' + password + ' | sudo -S ip route add 192.168.68.64/26 via 172.21.16.127 dev eno1', '\$', 10)
			# Check if X2 route to nepes gnb exists
			mySSH.command('ip route | grep --colour=never "192.168.68.192/26"', '\$', 10)
			result = re.search('172.21.16.137', mySSH.getBefore())
			if result is None:
				mySSH.command('echo ' + password + ' | sudo -S ip route add 192.168.68.192/26 via 172.21.16.137 dev eno1', '\$', 10)
			# Check if forwarding is enabled
			mySSH.command('sysctl net.ipv4.conf.all.forwarding', '\$', 10)
			result = re.search('net.ipv4.conf.all.forwarding = 1', mySSH.getBefore())
			if result is None:
				mySSH.command('echo ' + password + ' | sudo -S sysctl net.ipv4.conf.all.forwarding=1', '\$', 10)
			# Check if iptables forwarding is accepted
			mySSH.command('echo ' + password + ' | sudo -S iptables -L FORWARD', '\$', 10)
			result = re.search('Chain FORWARD .*policy ACCEPT', mySSH.getBefore())
			if result is None:
				mySSH.command('echo ' + password + ' | sudo -S iptables -P FORWARD ACCEPT', '\$', 10)
			mySSH.close()
		if svrName == 'nepes':
			mySSH.open(ipAddr, userName, password)
			# Check if route to ofqot gnb exists
			mySSH.command('ip route | grep --colour=never "192.168.68.192/26"', '\$', 10)
			result = re.search('172.21.16.109', mySSH.getBefore())
			if result is None:
				mySSH.command('echo ' + password + ' | sudo -S ip route add 192.168.68.192/26 via 172.21.16.109 dev enp0s31f6', '\$', 10)
			mySSH.command('sysctl net.ipv4.conf.all.forwarding', '\$', 10)
			result = re.search('net.ipv4.conf.all.forwarding = 1', mySSH.getBefore())
			if result is None:
				mySSH.command('echo ' + password + ' | sudo -S sysctl net.ipv4.conf.all.forwarding=1', '\$', 10)
			# Check if iptables forwarding is accepted
			mySSH.command('echo ' + password + ' | sudo -S iptables -L FORWARD', '\$', 10)
			result = re.search('Chain FORWARD .*policy ACCEPT', mySSH.getBefore())
			if result is None:
				mySSH.command('echo ' + password + ' | sudo -S iptables -P FORWARD ACCEPT', '\$', 10)
			mySSH.close()
		if svrName == 'ofqot':
			mySSH.open(ipAddr, userName, password)
			# Check if X2 route to nepes enb/epc exists
			mySSH.command('ip route | grep --colour=never "192.168.68.128/26"', '\$', 10)
			result = re.search('172.21.16.137', mySSH.getBefore())
			if result is None:
				mySSH.command('echo ' + password + ' | sudo -S ip route add 192.168.68.128/26 via 172.21.16.137 dev enp2s0', '\$', 10)
			# Check if forwarding is enabled
			mySSH.command('sysctl net.ipv4.conf.all.forwarding', '\$', 10)
			result = re.search('net.ipv4.conf.all.forwarding = 1', mySSH.getBefore())
			if result is None:
				mySSH.command('echo ' + password + ' | sudo -S sysctl net.ipv4.conf.all.forwarding=1', '\$', 10)
			# Check if iptables forwarding is accepted
			mySSH.command('echo ' + password + ' | sudo -S iptables -L FORWARD', '\$', 10)
			result = re.search('Chain FORWARD .*policy ACCEPT', mySSH.getBefore())
			if result is None:
				mySSH.command('echo ' + password + ' | sudo -S iptables -P FORWARD ACCEPT', '\$', 10)
			mySSH.close()
