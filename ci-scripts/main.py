# /*
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
# Version
#-----------------------------------------------------------
Version = '0.1'

#-----------------------------------------------------------
# Constants
#-----------------------------------------------------------
ALL_PROCESSES_OK = 0
ENB_PROCESS_FAILED = -1
ENB_PROCESS_OK = +1
ENB_PROCESS_SEG_FAULT = -11
ENB_PROCESS_ASSERTION = -12
ENB_PROCESS_REALTIME_ISSUE = -13
ENB_PROCESS_NOLOGFILE_TO_ANALYZE = -14
ENB_PROCESS_SLAVE_RRU_NOT_SYNCED = -15
HSS_PROCESS_FAILED = -2
HSS_PROCESS_OK = +2
MME_PROCESS_FAILED = -3
MME_PROCESS_OK = +3
SPGW_PROCESS_FAILED = -4
SPGW_PROCESS_OK = +4
UE_IP_ADDRESS_ISSUE = -5
OAI_UE_PROCESS_NOLOGFILE_TO_ANALYZE = -20
OAI_UE_PROCESS_COULD_NOT_SYNC = -21
OAI_UE_PROCESS_ASSERTION = -22
OAI_UE_PROCESS_FAILED = -23
OAI_UE_PROCESS_NO_TUNNEL_INTERFACE = -24
OAI_UE_PROCESS_OK = +6

UE_STATUS_DETACHED = 0
UE_STATUS_DETACHING = 1
UE_STATUS_ATTACHING = 2
UE_STATUS_ATTACHED = 3

X2_HO_REQ_STATE__IDLE = 0
X2_HO_REQ_STATE__TARGET_RECEIVES_REQ = 1
X2_HO_REQ_STATE__TARGET_RRC_RECFG_COMPLETE = 2
X2_HO_REQ_STATE__TARGET_SENDS_SWITCH_REQ = 3
X2_HO_REQ_STATE__SOURCE_RECEIVES_REQ_ACK = 10

#-----------------------------------------------------------
# Import
#-----------------------------------------------------------
import sys		# arg
import re		# reg
import pexpect		# pexpect
import time		# sleep
import os
import subprocess
import xml.etree.ElementTree as ET
import logging
import datetime
import signal
from multiprocessing import Process, Lock, SimpleQueue
logging.basicConfig(
	level=logging.DEBUG,
	format="[%(asctime)s] %(name)s:%(levelname)s: %(message)s"
)

#-----------------------------------------------------------
# Class Declaration
#-----------------------------------------------------------
class SSHConnection():
	def __init__(self):
		self.prematureExit = False
		self.ranRepository = ''
		self.ranBranch = ''
		self.ranAllowMerge = False
		self.ranCommitID = ''
		self.ranTargetBranch = ''
		self.eNBIPAddress = ''
		self.eNBUserName = ''
		self.eNBPassword = ''
		self.eNBSourceCodePath = ''
		self.EPCIPAddress = ''
		self.EPCUserName = ''
		self.EPCPassword = ''
		self.eNB1IPAddress = ''
		self.eNB1UserName = ''
		self.eNB1Password = ''
		self.eNB1SourceCodePath = ''
		self.eNB2IPAddress = ''
		self.eNB2UserName = ''
		self.eNB2Password = ''
		self.eNB2SourceCodePath = ''
		self.EPCSourceCodePath = ''
		self.EPCType = ''
		self.EPC_PcapFileName = ''
		self.ADBIPAddress = ''
		self.ADBUserName = ''
		self.ADBPassword = ''
		self.ADBCentralized = True
		self.testCase_id = ''
		self.testXMLfiles = []
		self.nbTestXMLfiles = 0
		self.desc = ''
		self.Build_eNB_args = ''
		self.backgroundBuild = False
		self.backgroundBuildTestId = ['', '', '']
		self.Initialize_eNB_args = ''
		self.eNB_instance = ''
		self.eNB_serverId = ''
		self.eNBLogFiles = ['', '', '']
		self.eNBOptions = ['', '', '']
		self.eNBmbmsEnables = [False, False, False]
		self.ping_args = ''
		self.ping_packetloss_threshold = ''
		self.iperf_args = ''
		self.iperf_packetloss_threshold = ''
		self.iperf_profile = ''
		self.iperf_options = ''
		self.nbMaxUEtoAttach = -1
		self.UEDevices = []
		self.UEDevicesStatus = []
		self.UEDevicesRemoteServer = []
		self.UEDevicesRemoteUser = []
		self.UEDevicesOffCmd = []
		self.UEDevicesOnCmd = []
		self.UEDevicesRebootCmd = []
		self.CatMDevices = []
		self.UEIPAddresses = []
		self.htmlFile = ''
		self.htmlHeaderCreated = False
		self.htmlFooterCreated = False
		self.htmlUEConnected = -1
		self.htmleNBFailureMsg = ''
		self.htmlUEFailureMsg = ''
		self.picocom_closure = False
		self.idle_sleep_time = 0
		self.x2_ho_options = 'network'
		self.x2NbENBs = 0
		self.x2ENBBsIds = []
		self.x2ENBConnectedUEs = []
		self.htmlTabRefs = []
		self.htmlTabNames = []
		self.htmlTabIcons = []
		self.repeatCounts = []
		self.finalStatus = False
		self.OsVersion = ''
		self.KernelVersion = ''
		self.UhdVersion = ''
		self.UsrpBoard = ''
		self.CpuNb = ''
		self.CpuModel = ''
		self.CpuMHz = ''
		self.UEIPAddress = ''
		self.UEUserName = ''
		self.UEPassword = ''
		self.UE_instance = ''
		self.UESourceCodePath = ''
		self.UELogFile = ''
		self.Build_OAI_UE_args = ''
		self.Initialize_OAI_UE_args = ''
		self.flexranCtrlInstalled = False
		self.flexranCtrlStarted = False
		self.expectedNbOfConnectedUEs = 0
		self.startTime = 0

	def open(self, ipaddress, username, password):
		count = 0
		connect_status = False
		while count < 4:
			self.ssh = pexpect.spawn('ssh', [username + '@' + ipaddress], timeout = 5)
			self.sshresponse = self.ssh.expect(['Are you sure you want to continue connecting (yes/no)?', 'password:', 'Last login', pexpect.EOF, pexpect.TIMEOUT])
			if self.sshresponse == 0:
				self.ssh.sendline('yes')
				self.ssh.expect('password:')
				self.ssh.sendline(password)
				self.sshresponse = self.ssh.expect(['\$', 'Permission denied', 'password:', pexpect.EOF, pexpect.TIMEOUT])
				if self.sshresponse == 0:
					count = 10
					connect_status = True
				else:
					logging.debug('self.sshresponse = ' + str(self.sshresponse))
			elif self.sshresponse == 1:
				self.ssh.sendline(password)
				self.sshresponse = self.ssh.expect(['\$', 'Permission denied', 'password:', pexpect.EOF, pexpect.TIMEOUT])
				if self.sshresponse == 0:
					count = 10
					connect_status = True
				else:
					logging.debug('self.sshresponse = ' + str(self.sshresponse))
			elif self.sshresponse == 2:
				# Checking if we are really on the remote client defined by its IP address
				self.command('stdbuf -o0 ifconfig | egrep --color=never "inet addr:|inet "', '\$', 5)
				result = re.search(str(ipaddress), str(self.ssh.before))
				if result is None:
					self.close()
				else:
					count = 10
					connect_status = True
			else:
				# debug output
				logging.debug(str(self.ssh.before))
				logging.debug('self.sshresponse = ' + str(self.sshresponse))
			# adding a tempo when failure
			if not connect_status:
				time.sleep(1)
			count += 1
		if connect_status:
			pass
		else:
			sys.exit('SSH Connection Failed')

	def command(self, commandline, expectedline, timeout):
		logging.debug(commandline)
		self.ssh.timeout = timeout
		self.ssh.sendline(commandline)
		self.sshresponse = self.ssh.expect([expectedline, pexpect.EOF, pexpect.TIMEOUT])
		if self.sshresponse == 0:
			return 0
		elif self.sshresponse == 1:
			logging.debug('\u001B[1;37;41m Unexpected EOF \u001B[0m')
			logging.debug('Expected Line : ' + expectedline)
			logging.debug(str(self.ssh.before))
			sys.exit(self.sshresponse)
		elif self.sshresponse == 2:
			logging.debug('\u001B[1;37;41m Unexpected TIMEOUT \u001B[0m')
			logging.debug('Expected Line : ' + expectedline)
			result = re.search('ping |iperf |picocom', str(commandline))
			if result is None:
				logging.debug(str(self.ssh.before))
				sys.exit(self.sshresponse)
			else:
				return -1
		else:
			logging.debug('\u001B[1;37;41m Unexpected Others \u001B[0m')
			logging.debug('Expected Line : ' + expectedline)
			sys.exit(self.sshresponse)

	def close(self):
		self.ssh.timeout = 5
		self.ssh.sendline('exit')
		self.sshresponse = self.ssh.expect([pexpect.EOF, pexpect.TIMEOUT])
		if self.sshresponse == 0:
			pass
		elif self.sshresponse == 1:
			if not self.picocom_closure:
				logging.debug('\u001B[1;37;41m Unexpected TIMEOUT during closing\u001B[0m')
		else:
			logging.debug('\u001B[1;37;41m Unexpected Others during closing\u001B[0m')

	def copyin(self, ipaddress, username, password, source, destination):
		count = 0
		copy_status = False
		logging.debug('scp '+ username + '@' + ipaddress + ':' + source + ' ' + destination)
		while count < 10:
			scp_spawn = pexpect.spawn('scp '+ username + '@' + ipaddress + ':' + source + ' ' + destination, timeout = 100)
			scp_response = scp_spawn.expect(['Are you sure you want to continue connecting (yes/no)?', 'password:', pexpect.EOF, pexpect.TIMEOUT])
			if scp_response == 0:
				scp_spawn.sendline('yes')
				scp_spawn.expect('password:')
				scp_spawn.sendline(password)
				scp_response = scp_spawn.expect(['\$', 'Permission denied', 'password:', pexpect.EOF, pexpect.TIMEOUT])
				if scp_response == 0:
					count = 10
					copy_status = True
				else:
					logging.debug('1 - scp_response = ' + str(scp_response))
			elif scp_response == 1:
				scp_spawn.sendline(password)
				scp_response = scp_spawn.expect(['\$', 'Permission denied', 'password:', pexpect.EOF, pexpect.TIMEOUT])
				if scp_response == 0 or scp_response == 3:
					count = 10
					copy_status = True
				else:
					logging.debug('2 - scp_response = ' + str(scp_response))
			elif scp_response == 2:
				count = 10
				copy_status = True
			else:
				logging.debug('3 - scp_response = ' + str(scp_response))
			# adding a tempo when failure
			if not copy_status:
				time.sleep(1)
			count += 1
		if copy_status:
			return 0
		else:
			return -1

	def copyout(self, ipaddress, username, password, source, destination):
		count = 0
		copy_status = False
		logging.debug('scp ' + source + ' ' + username + '@' + ipaddress + ':' + destination)
		while count < 4:
			scp_spawn = pexpect.spawn('scp ' + source + ' ' + username + '@' + ipaddress + ':' + destination, timeout = 100)
			scp_response = scp_spawn.expect(['Are you sure you want to continue connecting (yes/no)?', 'password:', pexpect.EOF, pexpect.TIMEOUT])
			if scp_response == 0:
				scp_spawn.sendline('yes')
				scp_spawn.expect('password:')
				scp_spawn.sendline(password)
				scp_response = scp_spawn.expect(['\$', 'Permission denied', 'password:', pexpect.EOF, pexpect.TIMEOUT])
				if scp_response == 0:
					count = 10
					copy_status = True
				else:
					logging.debug('1 - scp_response = ' + str(scp_response))
			elif scp_response == 1:
				scp_spawn.sendline(password)
				scp_response = scp_spawn.expect(['\$', 'Permission denied', 'password:', pexpect.EOF, pexpect.TIMEOUT])
				if scp_response == 0 or scp_response == 3:
					count = 10
					copy_status = True
				else:
					logging.debug('2 - scp_response = ' + str(scp_response))
			elif scp_response == 2:
				count = 10
				copy_status = True
			else:
				logging.debug('3 - scp_response = ' + str(scp_response))
			# adding a tempo when failure
			if not copy_status:
				time.sleep(1)
			count += 1
		if copy_status:
			pass
		else:
			sys.exit('SCP failed')

	def BuildeNB(self):
		if self.ranRepository == '' or self.ranBranch == '' or self.ranCommitID == '':
			Usage()
			sys.exit('Insufficient Parameter')
		if self.eNB_serverId == '0':
			lIpAddr = self.eNBIPAddress
			lUserName = self.eNBUserName
			lPassWord = self.eNBPassword
			lSourcePath = self.eNBSourceCodePath
		elif self.eNB_serverId == '1':
			lIpAddr = self.eNB1IPAddress
			lUserName = self.eNB1UserName
			lPassWord = self.eNB1Password
			lSourcePath = self.eNB1SourceCodePath
		elif self.eNB_serverId == '2':
			lIpAddr = self.eNB2IPAddress
			lUserName = self.eNB2UserName
			lPassWord = self.eNB2Password
			lSourcePath = self.eNB2SourceCodePath
		if lIpAddr == '' or lUserName == '' or lPassWord == '' or lSourcePath == '':
			Usage()
			sys.exit('Insufficient Parameter')
		self.open(lIpAddr, lUserName, lPassWord)
		self.command('mkdir -p ' + lSourcePath, '\$', 5)
		self.command('cd ' + lSourcePath, '\$', 5)
		self.command('if [ ! -e .git ]; then stdbuf -o0 git clone ' + self.ranRepository + ' .; else stdbuf -o0 git fetch --prune; fi', '\$', 600)
		# Raphael: here add a check if git clone or git fetch went smoothly
		self.command('git config user.email "jenkins@openairinterface.org"', '\$', 5)
		self.command('git config user.name "OAI Jenkins"', '\$', 5)
		# Checking the BUILD INFO file
		if not self.backgroundBuild:
			self.command('ls *.txt', '\$', 5)
			result = re.search('LAST_BUILD_INFO', str(self.ssh.before))
			if result is not None:
				mismatch = False
				self.command('grep SRC_COMMIT LAST_BUILD_INFO.txt', '\$', 2)
				result = re.search(self.ranCommitID, str(self.ssh.before))
				if result is None:
					mismatch = True
				self.command('grep MERGED_W_TGT_BRANCH LAST_BUILD_INFO.txt', '\$', 2)
				if (self.ranAllowMerge):
					result = re.search('YES', str(self.ssh.before))
					if result is None:
						mismatch = True
					self.command('grep TGT_BRANCH LAST_BUILD_INFO.txt', '\$', 2)
					if self.ranTargetBranch == '':
						result = re.search('develop', str(self.ssh.before))
					else:
						result = re.search(self.ranTargetBranch, str(self.ssh.before))
					if result is None:
						mismatch = True
				else:
					result = re.search('NO', str(self.ssh.before))
					if result is None:
						mismatch = True
				if not mismatch:
					self.close()
					self.CreateHtmlTestRow(self.Build_eNB_args, 'OK', ALL_PROCESSES_OK)
					return
				
		self.command('echo ' + lPassWord + ' | sudo -S git clean -x -d -ff', '\$', 30)
		# if the commit ID is provided use it to point to it
		if self.ranCommitID != '':
			self.command('git checkout -f ' + self.ranCommitID, '\$', 5)
		# if the branch is not develop, then it is a merge request and we need to do 
		# the potential merge. Note that merge conflicts should already been checked earlier
		if (self.ranAllowMerge):
			if self.ranTargetBranch == '':
				if (self.ranBranch != 'develop') and (self.ranBranch != 'origin/develop'):
					self.command('git merge --ff origin/develop -m "Temporary merge for CI"', '\$', 5)
			else:
				logging.debug('Merging with the target branch: ' + self.ranTargetBranch)
				self.command('git merge --ff origin/' + self.ranTargetBranch + ' -m "Temporary merge for CI"', '\$', 5)
		self.command('source oaienv', '\$', 5)
		self.command('cd cmake_targets', '\$', 5)
		self.command('mkdir -p log', '\$', 5)
		self.command('chmod 777 log', '\$', 5)
		# no need to remove in log (git clean did the trick)
		if self.backgroundBuild:
			self.command('echo "./build_oai ' + self.Build_eNB_args + '" > ./my-lte-softmodem-build.sh', '\$', 5)
			self.command('chmod 775 ./my-lte-softmodem-build.sh', '\$', 5)
			self.command('echo ' + lPassWord + ' | sudo -S -E daemon --inherit --unsafe --name=build_enb_daemon --chdir=' + lSourcePath + '/cmake_targets -o ' + lSourcePath + '/cmake_targets/compile_oai_enb.log ./my-lte-softmodem-build.sh', '\$', 5)
			self.close()
			self.CreateHtmlTestRow(self.Build_eNB_args, 'OK', ALL_PROCESSES_OK)
			self.backgroundBuildTestId[int(self.eNB_instance)] = self.testCase_id
			return
		self.command('stdbuf -o0 ./build_oai ' + self.Build_eNB_args + ' 2>&1 | stdbuf -o0 tee compile_oai_enb.log', 'Bypassing the Tests|build have failed', 1500)
		self.checkBuildeNB(lIpAddr, lUserName, lPassWord, lSourcePath, self.testCase_id)

	def WaitBuildeNBisFinished(self):
		if self.eNB_serverId == '0':
			lIpAddr = self.eNBIPAddress
			lUserName = self.eNBUserName
			lPassWord = self.eNBPassword
			lSourcePath = self.eNBSourceCodePath
		elif self.eNB_serverId == '1':
			lIpAddr = self.eNB1IPAddress
			lUserName = self.eNB1UserName
			lPassWord = self.eNB1Password
			lSourcePath = self.eNB1SourceCodePath
		elif self.eNB_serverId == '2':
			lIpAddr = self.eNB2IPAddress
			lUserName = self.eNB2UserName
			lPassWord = self.eNB2Password
			lSourcePath = self.eNB2SourceCodePath
		if lIpAddr == '' or lUserName == '' or lPassWord == '' or lSourcePath == '':
			Usage()
			sys.exit('Insufficient Parameter')
		self.open(lIpAddr, lUserName, lPassWord)
		count = 40
		buildOAIprocess = True
		while (count > 0) and buildOAIprocess:
			self.command('ps aux | grep --color=never build_ | grep -v grep', '\$', 3)
			result = re.search('build_oai', str(self.ssh.before))
			if result is None:
				buildOAIprocess = False
			else:
				count -= 1
				time.sleep(30)
		self.checkBuildeNB(lIpAddr, lUserName, lPassWord, lSourcePath, self.backgroundBuildTestId[int(self.eNB_instance)])

	def checkBuildeNB(self, lIpAddr, lUserName, lPassWord, lSourcePath, testcaseId):
		self.command('cd ' + lSourcePath + '/cmake_targets', '\$', 3)
		self.command('ls lte_build_oai/build', '\$', 3)
		self.command('ls lte_build_oai/build', '\$', 3)
		buildStatus = True
		result = re.search('lte-softmodem', str(self.ssh.before))
		if result is None:
			buildStatus = False
		else:
			# Generating a BUILD INFO file
			self.command('echo "SRC_BRANCH: ' + self.ranBranch + '" > ../LAST_BUILD_INFO.txt', '\$', 2)
			self.command('echo "SRC_COMMIT: ' + self.ranCommitID + '" >> ../LAST_BUILD_INFO.txt', '\$', 2)
			if (self.ranAllowMerge):
				self.command('echo "MERGED_W_TGT_BRANCH: YES" >> ../LAST_BUILD_INFO.txt', '\$', 2)
				if self.ranTargetBranch == '':
					self.command('echo "TGT_BRANCH: develop" >> ../LAST_BUILD_INFO.txt', '\$', 2)
				else:
					self.command('echo "TGT_BRANCH: ' + self.ranTargetBranch + '" >> ../LAST_BUILD_INFO.txt', '\$', 2)
			else:
				self.command('echo "MERGED_W_TGT_BRANCH: NO" >> ../LAST_BUILD_INFO.txt', '\$', 2)
		self.command('mkdir -p build_log_' + testcaseId, '\$', 5)
		self.command('mv log/* ' + 'build_log_' + testcaseId, '\$', 5)
		self.command('mv compile_oai_enb.log ' + 'build_log_' + testcaseId, '\$', 5)
		if self.eNB_serverId != '0':
			self.command('cd cmake_targets', '\$', 5)
			self.command('if [ -e tmp_build' + testcaseId + '.zip ]; then rm -f tmp_build' + testcaseId + '.zip; fi', '\$', 5)
			self.command('zip -r -qq tmp_build' + testcaseId + '.zip build_log_' + testcaseId, '\$', 5)
			self.close()
			if (os.path.isfile('./tmp_build' + testcaseId + '.zip')):
				os.remove('./tmp_build' + testcaseId + '.zip')
			self.copyin(lIpAddr, lUserName, lPassWord, lSourcePath + '/cmake_targets/tmp_build' + testcaseId + '.zip', '.')
			if (os.path.isfile('./tmp_build' + testcaseId + '.zip')):
				self.copyout(self.eNBIPAddress, self.eNBUserName, self.eNBPassword, './tmp_build' + testcaseId + '.zip', self.eNBSourceCodePath + '/cmake_targets/.')
				os.remove('./tmp_build' + testcaseId + '.zip')
				self.open(self.eNBIPAddress, self.eNBUserName, self.eNBPassword)
				self.command('cd ' + self.eNBSourceCodePath + '/cmake_targets', '\$', 5)
				self.command('unzip -qq -DD tmp_build' + testcaseId + '.zip', '\$', 5)
				self.command('rm -f tmp_build' + testcaseId + '.zip', '\$', 5)
				self.close()
		else:
			self.close()

		if buildStatus:
			self.CreateHtmlTestRow(self.Build_eNB_args, 'OK', ALL_PROCESSES_OK)
		else:
			logging.error('\u001B[1m Building OAI eNB Failed\u001B[0m')
			self.CreateHtmlTestRow(self.Build_eNB_args, 'KO', ALL_PROCESSES_OK)
			self.CreateHtmlTabFooter(False)
			sys.exit(1)

	def BuildOAIUE(self):
		if self.UEIPAddress == '' or self.ranRepository == '' or self.ranBranch == '' or self.UEUserName == '' or self.UEPassword == '' or self.UESourceCodePath == '':
			Usage()
			sys.exit('Insufficient Parameter')
		self.open(self.UEIPAddress, self.UEUserName, self.UEPassword)
		self.command('mkdir -p ' + self.UESourceCodePath, '\$', 5)
		self.command('cd ' + self.UESourceCodePath, '\$', 5)
		self.command('if [ ! -e .git ]; then stdbuf -o0 git clone ' + self.ranRepository + ' .; else stdbuf -o0 git fetch --prune; fi', '\$', 600)
		# here add a check if git clone or git fetch went smoothly
		self.command('git config user.email "jenkins@openairinterface.org"', '\$', 5)
		self.command('git config user.name "OAI Jenkins"', '\$', 5)
		self.command('ls *.txt', '\$', 5)
		result = re.search('LAST_BUILD_INFO', str(self.ssh.before))
		if result is not None:
			mismatch = False
			self.command('grep SRC_COMMIT LAST_BUILD_INFO.txt', '\$', 2)
			result = re.search(self.ranCommitID, str(self.ssh.before))
			if result is None:
				mismatch = True
			self.command('grep MERGED_W_TGT_BRANCH LAST_BUILD_INFO.txt', '\$', 2)
			if (self.ranAllowMerge):
				result = re.search('YES', str(self.ssh.before))
				if result is None:
					mismatch = True
				self.command('grep TGT_BRANCH LAST_BUILD_INFO.txt', '\$', 2)
				if self.ranTargetBranch == '':
					result = re.search('develop', str(self.ssh.before))
				else:
					result = re.search(self.ranTargetBranch, str(self.ssh.before))
				if result is None:
					mismatch = True
			else:
				result = re.search('NO', str(self.ssh.before))
				if result is None:
					mismatch = True
			if not mismatch:
				self.close()
				self.CreateHtmlTestRow(self.Build_eNB_args, 'OK', ALL_PROCESSES_OK)
				return

		self.command('echo ' + self.UEPassword + ' | sudo -S git clean -x -d -ff', '\$', 30)
		# if the commit ID is provided use it to point to it
		if self.ranCommitID != '':
			self.command('git checkout -f ' + self.ranCommitID, '\$', 5)
		# if the branch is not develop, then it is a merge request and we need to do 
		# the potential merge. Note that merge conflicts should already been checked earlier
		if (self.ranAllowMerge):
			if self.ranTargetBranch == '':
				if (self.ranBranch != 'develop') and (self.ranBranch != 'origin/develop'):
					self.command('git merge --ff origin/develop -m "Temporary merge for CI"', '\$', 5)
			else:
				logging.debug('Merging with the target branch: ' + self.ranTargetBranch)
				self.command('git merge --ff origin/' + self.ranTargetBranch + ' -m "Temporary merge for CI"', '\$', 5)
		self.command('source oaienv', '\$', 5)
		self.command('cd cmake_targets', '\$', 5)
		self.command('mkdir -p log', '\$', 5)
		self.command('chmod 777 log', '\$', 5)
		# no need to remove in log (git clean did the trick)
		self.command('stdbuf -o0 ./build_oai ' + self.Build_OAI_UE_args + ' 2>&1 | stdbuf -o0 tee compile_oai_ue.log', 'Bypassing the Tests|build have failed', 600)
		self.command('ls lte_build_oai/build', '\$', 3)
		self.command('ls lte_build_oai/build', '\$', 3)
		buildStatus = True
		result = re.search('lte-uesoftmodem', str(self.ssh.before))
		if result is None:
			buildStatus = False
		self.command('mkdir -p build_log_' + self.testCase_id, '\$', 5)
		self.command('mv log/* ' + 'build_log_' + self.testCase_id, '\$', 5)
		self.command('mv compile_oai_ue.log ' + 'build_log_' + self.testCase_id, '\$', 5)
		if buildStatus:
			# Generating a BUILD INFO file
			self.command('echo "SRC_BRANCH: ' + self.ranBranch + '" > ../LAST_BUILD_INFO.txt', '\$', 2)
			self.command('echo "SRC_COMMIT: ' + self.ranCommitID + '" >> ../LAST_BUILD_INFO.txt', '\$', 2)
			if (self.ranAllowMerge):
				self.command('echo "MERGED_W_TGT_BRANCH: YES" >> ../LAST_BUILD_INFO.txt', '\$', 2)
				if self.ranTargetBranch == '':
					self.command('echo "TGT_BRANCH: develop" >> ../LAST_BUILD_INFO.txt', '\$', 2)
				else:
					self.command('echo "TGT_BRANCH: ' + self.ranTargetBranch + '" >> ../LAST_BUILD_INFO.txt', '\$', 2)
			else:
				self.command('echo "MERGED_W_TGT_BRANCH: NO" >> ../LAST_BUILD_INFO.txt', '\$', 2)
			self.close()
			self.CreateHtmlTestRow(self.Build_OAI_UE_args, 'OK', ALL_PROCESSES_OK, 'OAI UE')
		else:
			self.close()
			logging.error('\u001B[1m Building OAI UE Failed\u001B[0m')
			self.CreateHtmlTestRow(self.Build_OAI_UE_args, 'KO', ALL_PROCESSES_OK, 'OAI UE')
			self.CreateHtmlTabFooter(False)
			sys.exit(1)

	def InitializeHSS(self):
		if self.EPCIPAddress == '' or self.EPCUserName == '' or self.EPCPassword == '' or self.EPCSourceCodePath == '' or self.EPCType == '':
			Usage()
			sys.exit('Insufficient Parameter')
		self.open(self.EPCIPAddress, self.EPCUserName, self.EPCPassword)
		if re.match('OAI-Rel14-CUPS', self.EPCType, re.IGNORECASE):
			logging.debug('Using the OAI EPC Release 14 Cassandra-based HSS')
			self.command('cd ' + self.EPCSourceCodePath + '/scripts', '\$', 5)
			logging.debug('\u001B[1m Launching tshark on all interfaces \u001B[0m')
			EPC_PcapFileName = 'epc_' + self.testCase_id + '.pcap'
			self.command('echo ' + self.EPCPassword + ' | sudo -S rm -f ' + EPC_PcapFileName, '\$', 5)
			self.command('echo $USER; nohup sudo tshark -f "tcp port not 22 and port not 53" -i any -w ' + self.EPCSourceCodePath + '/scripts/' + EPC_PcapFileName + ' > /tmp/tshark.log 2>&1 &', self.EPCUserName, 5)
			self.command('echo ' + self.EPCPassword + ' | sudo -S mkdir -p logs', '\$', 5)
			self.command('echo ' + self.EPCPassword + ' | sudo -S rm -f hss_' + self.testCase_id + '.log logs/hss*.*', '\$', 5)
			self.command('echo "oai_hss -j /usr/local/etc/oai/hss_rel14.json" > ./my-hss.sh', '\$', 5)
			self.command('chmod 755 ./my-hss.sh', '\$', 5)
			self.command('sudo daemon --unsafe --name=hss_daemon --chdir=' + self.EPCSourceCodePath + '/scripts -o ' + self.EPCSourceCodePath + '/scripts/hss_' + self.testCase_id + '.log ./my-hss.sh', '\$', 5)
		elif re.match('OAI', self.EPCType, re.IGNORECASE):
			logging.debug('Using the OAI EPC HSS')
			self.command('cd ' + self.EPCSourceCodePath, '\$', 5)
			self.command('source oaienv', '\$', 5)
			self.command('cd scripts', '\$', 5)
			self.command('echo ' + self.EPCPassword + ' | sudo -S ./run_hss 2>&1 | stdbuf -o0 awk \'{ print strftime("[%Y/%m/%d %H:%M:%S] ",systime()) $0 }\' | stdbuf -o0 tee -a hss_' + self.testCase_id + '.log &', 'Core state: 2 -> 3', 35)
		elif re.match('ltebox', self.EPCType, re.IGNORECASE):
			logging.debug('Using the ltebox simulated HSS')
			self.command('if [ -d ' + self.EPCSourceCodePath + '/scripts ]; then echo ' + self.eNBPassword + ' | sudo -S rm -Rf ' + self.EPCSourceCodePath + '/scripts ; fi', '\$', 5)
			self.command('mkdir -p ' + self.EPCSourceCodePath + '/scripts', '\$', 5)
			self.command('cd /opt/hss_sim0609', '\$', 5)
			self.command('echo ' + self.EPCPassword + ' | sudo -S rm -f hss.log daemon.log', '\$', 5)
			self.command('echo ' + self.EPCPassword + ' | sudo -S echo "Starting sudo session" && sudo daemon --unsafe --name=simulated_hss --chdir=/opt/hss_sim0609 ./starthss_real  ', '\$', 5)
		else:
			logging.error('This option should not occur!')
		self.close()
		self.CreateHtmlTestRow(self.EPCType, 'OK', ALL_PROCESSES_OK)

	def InitializeMME(self):
		if self.EPCIPAddress == '' or self.EPCUserName == '' or self.EPCPassword == '' or self.EPCSourceCodePath == '' or self.EPCType == '':
			Usage()
			sys.exit('Insufficient Parameter')
		self.open(self.EPCIPAddress, self.EPCUserName, self.EPCPassword)
		if re.match('OAI-Rel14-CUPS', self.EPCType, re.IGNORECASE):
			logging.debug('Using the OAI EPC Release 14 MME')
			self.command('cd ' + self.EPCSourceCodePath + '/scripts', '\$', 5)
			self.command('echo ' + self.EPCPassword + ' | sudo -S rm -f mme_' + self.testCase_id + '.log', '\$', 5)
			self.command('echo "./run_mme --config-file /usr/local/etc/oai/mme.conf --set-virt-if" > ./my-mme.sh', '\$', 5)
			self.command('chmod 755 ./my-mme.sh', '\$', 5)
			self.command('sudo daemon --unsafe --name=mme_daemon --chdir=' + self.EPCSourceCodePath + '/scripts -o ' + self.EPCSourceCodePath + '/scripts/mme_' + self.testCase_id + '.log ./my-mme.sh', '\$', 5)
		elif re.match('OAI', self.EPCType, re.IGNORECASE):
			self.command('cd ' + self.EPCSourceCodePath, '\$', 5)
			self.command('source oaienv', '\$', 5)
			self.command('cd scripts', '\$', 5)
			self.command('stdbuf -o0 hostname', '\$', 5)
			result = re.search('hostname\\\\r\\\\n(?P<host_name>[a-zA-Z0-9\-\_]+)\\\\r\\\\n', str(self.ssh.before))
			if result is None:
				logging.debug('\u001B[1;37;41m Hostname Not Found! \u001B[0m')
				sys.exit(1)
			host_name = result.group('host_name')
			self.command('echo ' + self.EPCPassword + ' | sudo -S ./run_mme 2>&1 | stdbuf -o0 tee -a mme_' + self.testCase_id + '.log &', 'MME app initialization complete', 100)
		elif re.match('ltebox', self.EPCType, re.IGNORECASE):
			self.command('cd /opt/ltebox/tools', '\$', 5)
			self.command('echo ' + self.EPCPassword + ' | sudo -S ./start_mme', '\$', 5)
		else:
			logging.error('This option should not occur!')
		self.close()
		self.CreateHtmlTestRow(self.EPCType, 'OK', ALL_PROCESSES_OK)

	def InitializeSPGW(self):
		if self.EPCIPAddress == '' or self.EPCUserName == '' or self.EPCPassword == '' or self.EPCSourceCodePath == '' or self.EPCType == '':
			Usage()
			sys.exit('Insufficient Parameter')
		self.open(self.EPCIPAddress, self.EPCUserName, self.EPCPassword)
		if re.match('OAI-Rel14-CUPS', self.EPCType, re.IGNORECASE):
			logging.debug('Using the OAI EPC Release 14 SPGW-CUPS')
			self.command('cd ' + self.EPCSourceCodePath + '/scripts', '\$', 5)
			self.command('echo ' + self.EPCPassword + ' | sudo -S rm -f spgwc_' + self.testCase_id + '.log spgwu_' + self.testCase_id + '.log', '\$', 5)
			self.command('echo "spgwc -c /usr/local/etc/oai/spgw_c.conf" > ./my-spgwc.sh', '\$', 5)
			self.command('chmod 755 ./my-spgwc.sh', '\$', 5)
			self.command('sudo daemon --unsafe --name=spgwc_daemon --chdir=' + self.EPCSourceCodePath + '/scripts -o ' + self.EPCSourceCodePath + '/scripts/spgwc_' + self.testCase_id + '.log ./my-spgwc.sh', '\$', 5)
			time.sleep(5)
			self.command('echo "spgwu -c /usr/local/etc/oai/spgw_u.conf" > ./my-spgwu.sh', '\$', 5)
			self.command('chmod 755 ./my-spgwu.sh', '\$', 5)
			self.command('sudo daemon --unsafe --name=spgwu_daemon --chdir=' + self.EPCSourceCodePath + '/scripts -o ' + self.EPCSourceCodePath + '/scripts/spgwu_' + self.testCase_id + '.log ./my-spgwu.sh', '\$', 5)
		elif re.match('OAI', self.EPCType, re.IGNORECASE):
			self.command('cd ' + self.EPCSourceCodePath, '\$', 5)
			self.command('source oaienv', '\$', 5)
			self.command('cd scripts', '\$', 5)
			self.command('echo ' + self.EPCPassword + ' | sudo -S ./run_spgw 2>&1 | stdbuf -o0 tee -a spgw_' + self.testCase_id + '.log &', 'Initializing SPGW-APP task interface: DONE', 30)
		elif re.match('ltebox', self.EPCType, re.IGNORECASE):
			self.command('cd /opt/ltebox/tools', '\$', 5)
			self.command('echo ' + self.EPCPassword + ' | sudo -S ./start_xGw', '\$', 5)
		else:
			logging.error('This option should not occur!')
		self.close()
		self.CreateHtmlTestRow(self.EPCType, 'OK', ALL_PROCESSES_OK)

	def CheckFlexranCtrlInstallation(self):
		if self.EPCIPAddress == '' or self.EPCUserName == '' or self.EPCPassword == '':
			return
		self.open(self.EPCIPAddress, self.EPCUserName, self.EPCPassword)
		self.command('ls -ls /opt/flexran_rtc/*/rt_controller', '\$', 5)
		result = re.search('/opt/flexran_rtc/build/rt_controller', str(self.ssh.before))
		if result is not None:
			self.flexranCtrlInstalled = True
			logging.debug('Flexran Controller is installed')
		self.close()

	def InitializeFlexranCtrl(self):
		if self.flexranCtrlInstalled == False:
			return
		if self.EPCIPAddress == '' or self.EPCUserName == '' or self.EPCPassword == '':
			Usage()
			sys.exit('Insufficient Parameter')
		self.open(self.EPCIPAddress, self.EPCUserName, self.EPCPassword)
		self.command('cd /opt/flexran_rtc', '\$', 5)
		self.command('echo ' + self.EPCPassword + ' | sudo -S rm -f log/*.log', '\$', 5)
		self.command('echo ' + self.EPCPassword + ' | sudo -S echo "build/rt_controller -c log_config/basic_log" > ./my-flexran-ctl.sh', '\$', 5)
		self.command('echo ' + self.EPCPassword + ' | sudo -S chmod 755 ./my-flexran-ctl.sh', '\$', 5)
		self.command('echo ' + self.EPCPassword + ' | sudo -S daemon --unsafe --name=flexran_rtc_daemon --chdir=/opt/flexran_rtc -o /opt/flexran_rtc/log/flexranctl_' + self.testCase_id + '.log ././my-flexran-ctl.sh', '\$', 5)
		self.command('ps -aux | grep --color=never rt_controller', '\$', 5)
		result = re.search('rt_controller -c ', str(self.ssh.before))
		if result is not None:
			logging.debug('\u001B[1m Initialize FlexRan Controller Completed\u001B[0m')
			self.flexranCtrlStarted = True
		self.close()
		self.CreateHtmlTestRow('N/A', 'OK', ALL_PROCESSES_OK)

	def InitializeeNB(self):
		if self.eNB_serverId == '0':
			lIpAddr = self.eNBIPAddress
			lUserName = self.eNBUserName
			lPassWord = self.eNBPassword
			lSourcePath = self.eNBSourceCodePath
		elif self.eNB_serverId == '1':
			lIpAddr = self.eNB1IPAddress
			lUserName = self.eNB1UserName
			lPassWord = self.eNB1Password
			lSourcePath = self.eNB1SourceCodePath
		elif self.eNB_serverId == '2':
			lIpAddr = self.eNB2IPAddress
			lUserName = self.eNB2UserName
			lPassWord = self.eNB2Password
			lSourcePath = self.eNB2SourceCodePath
		if lIpAddr == '' or lUserName == '' or lPassWord == '' or lSourcePath == '':
			Usage()
			sys.exit('Insufficient Parameter')
		check_eNB = False
		check_OAI_UE = False
		pStatus = self.CheckProcessExist(check_eNB, check_OAI_UE)
		if (pStatus < 0):
			self.CreateHtmlTestRow(self.Initialize_eNB_args, 'KO', pStatus)
			self.CreateHtmlTabFooter(False)
			sys.exit(1)
		# If tracer options is on, running tshark on EPC side and capture traffic b/ EPC and eNB
		result = re.search('T_stdout', str(self.Initialize_eNB_args))
		if result is not None:
			self.open(self.EPCIPAddress, self.EPCUserName, self.EPCPassword)
			self.command('ip addr show | awk -f /tmp/active_net_interfaces.awk | egrep -v "lo|tun"', '\$', 5)
			result = re.search('interfaceToUse=(?P<eth_interface>[a-zA-Z0-9\-\_]+)done', str(self.ssh.before))
			if result is not None:
				eth_interface = result.group('eth_interface')
				logging.debug('\u001B[1m Launching tshark on interface ' + eth_interface + '\u001B[0m')
				self.EPC_PcapFileName = 'enb_' + self.testCase_id + '_s1log.pcap'
				self.command('echo ' + self.EPCPassword + ' | sudo -S rm -f /tmp/' + self.EPC_PcapFileName, '\$', 5)
				self.command('echo $USER; nohup sudo tshark -f "host ' + lIpAddr +'" -i ' + eth_interface + ' -w /tmp/' + self.EPC_PcapFileName + ' > /tmp/tshark.log 2>&1 &', self.EPCUserName, 5)
			self.close()
		self.open(lIpAddr, lUserName, lPassWord)
		self.command('cd ' + lSourcePath, '\$', 5)
		# Initialize_eNB_args usually start with -O and followed by the location in repository
		full_config_file = self.Initialize_eNB_args.replace('-O ','')
		extra_options = ''
		extIdx = full_config_file.find('.conf')
		if (extIdx > 0):
			extra_options = full_config_file[extIdx + 5:]
			# if tracer options is on, compiling and running T Tracer
			result = re.search('T_stdout', str(extra_options))
			if result is not None:
				logging.debug('\u001B[1m Compiling and launching T Tracer\u001B[0m')
				self.command('cd common/utils/T/tracer', '\$', 5)
				self.command('make', '\$', 10)
				self.command('echo $USER; nohup ./record -d ../T_messages.txt -o ' + lSourcePath + '/cmake_targets/enb_' + self.testCase_id + '_record.raw -ON -off VCD -off HEAVY -off LEGACY_GROUP_TRACE -off LEGACY_GROUP_DEBUG > ' + lSourcePath + '/cmake_targets/enb_' + self.testCase_id + '_record.log 2>&1 &', lUserName, 5)
				self.command('cd ' + lSourcePath, '\$', 5)
			full_config_file = full_config_file[:extIdx + 5]
			config_path, config_file = os.path.split(full_config_file)
		else:
			sys.exit('Insufficient Parameter')
		ci_full_config_file = config_path + '/ci-' + config_file
		rruCheck = False
		result = re.search('^rru|^rcc|^du.band', str(config_file))
		if result is not None:
			rruCheck = True
		# do not reset board twice in IF4.5 case
		result = re.search('^rru|^enb|^du.band', str(config_file))
		if result is not None:
			self.command('echo ' + lPassWord + ' | sudo -S uhd_find_devices', '\$', 10)
			result = re.search('type: b200', str(self.ssh.before))
			if result is not None:
				logging.debug('Found a B2xx device --> resetting it')
				self.command('echo ' + lPassWord + ' | sudo -S b2xx_fx3_utils --reset-device', '\$', 10)
				# Reloading FGPA bin firmware
				self.command('echo ' + lPassWord + ' | sudo -S uhd_find_devices', '\$', 15)
		# Make a copy and adapt to EPC / eNB IP addresses
		self.command('cp ' + full_config_file + ' ' + ci_full_config_file, '\$', 5)
		self.command('sed -i -e \'s/CI_MME_IP_ADDR/' + self.EPCIPAddress + '/\' ' + ci_full_config_file, '\$', 2);
		self.command('sed -i -e \'s/CI_ENB_IP_ADDR/' + lIpAddr + '/\' ' + ci_full_config_file, '\$', 2);
		self.command('sed -i -e \'s/CI_RCC_IP_ADDR/' + self.eNBIPAddress + '/\' ' + ci_full_config_file, '\$', 2);
		self.command('sed -i -e \'s/CI_RRU1_IP_ADDR/' + self.eNB1IPAddress + '/\' ' + ci_full_config_file, '\$', 2);
		self.command('sed -i -e \'s/CI_RRU2_IP_ADDR/' + self.eNB2IPAddress + '/\' ' + ci_full_config_file, '\$', 2);
		if self.flexranCtrlInstalled and self.flexranCtrlStarted:
			self.command('sed -i -e \'s/FLEXRAN_ENABLED.*;/FLEXRAN_ENABLED        = "yes";/\' ' + ci_full_config_file, '\$', 2);
		else:
			self.command('sed -i -e \'s/FLEXRAN_ENABLED.*;/FLEXRAN_ENABLED        = "no";/\' ' + ci_full_config_file, '\$', 2);
		self.eNBmbmsEnables[int(self.eNB_instance)] = False
		self.command('grep enable_enb_m2 ' + ci_full_config_file, '\$', 2);
		result = re.search('yes', str(self.ssh.before))
		if result is not None:
			self.eNBmbmsEnables[int(self.eNB_instance)] = True
			logging.debug('\u001B[1m MBMS is enabled on this eNB\u001B[0m')
		result = re.search('noS1', str(self.Initialize_eNB_args))
		eNBinNoS1 = False
		if result is not None:
			eNBinNoS1 = True
			logging.debug('\u001B[1m eNB is in noS1 configuration \u001B[0m')
		# Launch eNB with the modified config file
		self.command('source oaienv', '\$', 5)
		self.command('cd cmake_targets', '\$', 5)
		self.command('echo "ulimit -c unlimited && ./lte_build_oai/build/lte-softmodem -O ' + lSourcePath + '/' + ci_full_config_file + extra_options + '" > ./my-lte-softmodem-run' + str(self.eNB_instance) + '.sh', '\$', 5)
		self.command('chmod 775 ./my-lte-softmodem-run' + str(self.eNB_instance) + '.sh', '\$', 5)
		self.command('echo ' + lPassWord + ' | sudo -S rm -Rf enb_' + self.testCase_id + '.log', '\$', 5)
		self.command('echo ' + lPassWord + ' | sudo -S -E daemon --inherit --unsafe --name=enb' + str(self.eNB_instance) + '_daemon --chdir=' + lSourcePath + '/cmake_targets -o ' + lSourcePath + '/cmake_targets/enb_' + self.testCase_id + '.log ./my-lte-softmodem-run' + str(self.eNB_instance) + '.sh', '\$', 5)
		self.eNBLogFiles[int(self.eNB_instance)] = 'enb_' + self.testCase_id + '.log'
		if extra_options != '':
			self.eNBOptions[int(self.eNB_instance)] = extra_options
		time.sleep(6)
		doLoop = True
		loopCounter = 10
		enbDidSync = False
		while (doLoop):
			loopCounter = loopCounter - 1
			if (loopCounter == 0):
				# In case of T tracer recording, we may need to kill it
				result = re.search('T_stdout', str(self.Initialize_eNB_args))
				if result is not None:
					self.command('killall --signal SIGKILL record', '\$', 5)
				self.close()
				doLoop = False
				logging.error('\u001B[1;37;41m eNB logging system did not show got sync! \u001B[0m')
				self.CreateHtmlTestRow('-O ' + config_file + extra_options, 'KO', ALL_PROCESSES_OK)
				# In case of T tracer recording, we need to kill tshark on EPC side
				result = re.search('T_stdout', str(self.Initialize_eNB_args))
				if result is not None:
					self.open(self.EPCIPAddress, self.EPCUserName, self.EPCPassword)
					logging.debug('\u001B[1m Stopping tshark \u001B[0m')
					self.command('echo ' + self.EPCPassword + ' | sudo -S killall --signal SIGKILL tshark', '\$', 5)
					if self.EPC_PcapFileName != '':
						time.sleep(0.5)
						self.command('echo ' + self.EPCPassword + ' | sudo -S chmod 666 /tmp/' + self.EPC_PcapFileName, '\$', 5)
					self.close()
					time.sleep(1)
					if self.EPC_PcapFileName != '':
						copyin_res = self.copyin(self.EPCIPAddress, self.EPCUserName, self.EPCPassword, '/tmp/' + self.EPC_PcapFileName, '.')
						if (copyin_res == 0):
							self.copyout(lIpAddr, lUserName, lPassWord, self.EPC_PcapFileName, lSourcePath + '/cmake_targets/.')
				self.prematureExit = True
				return
			else:
				self.command('stdbuf -o0 cat enb_' + self.testCase_id + '.log | egrep --text --color=never -i "wait|sync|Starting"', '\$', 4)
				if rruCheck:
					result = re.search('wait RUs', str(self.ssh.before))
				else:
					result = re.search('got sync|Starting F1AP at CU', str(self.ssh.before))
				if result is None:
					time.sleep(6)
				else:
					doLoop = False
					enbDidSync = True
					time.sleep(10)

		if enbDidSync and eNBinNoS1:
			self.command('ifconfig oaitun_enb1', '\$', 4)
			result = re.search('inet addr', str(self.ssh.before))
			if result is not None:
				logging.debug('\u001B[1m oaitun_enb1 interface is mounted and configured\u001B[0m')
			else:
				logging.error('\u001B[1m oaitun_enb1 interface is either NOT mounted or NOT configured\u001B[0m')
			if self.eNBmbmsEnables[int(self.eNB_instance)]:
				self.command('ifconfig oaitun_enm1', '\$', 4)
				result = re.search('inet addr', str(self.ssh.before))
				if result is not None:
					logging.debug('\u001B[1m oaitun_enm1 interface is mounted and configured\u001B[0m')
				else:
					logging.error('\u001B[1m oaitun_enm1 interface is either NOT mounted or NOT configured\u001B[0m')

		self.close()
		self.CreateHtmlTestRow('-O ' + config_file + extra_options, 'OK', ALL_PROCESSES_OK)
		logging.debug('\u001B[1m Initialize eNB Completed\u001B[0m')

	def InitializeUE_common(self, device_id, idx):
		try:
			self.open(self.ADBIPAddress, self.ADBUserName, self.ADBPassword)
			if not self.ADBCentralized:
				# Reboot UE
				self.command('ssh ' + self.UEDevicesRemoteUser[idx] + '@' + self.UEDevicesRemoteServer[idx] + ' ' + self.UEDevicesRebootCmd[idx], '\$', 60)
				# Wait
				time.sleep(60)
				# Put in LTE-Mode only
				self.command('ssh ' + self.UEDevicesRemoteUser[idx] + '@' + self.UEDevicesRemoteServer[idx] + ' \'adb -s ' + device_id + ' shell "settings put global preferred_network_mode 11"\'', '\$', 60)
				self.command('ssh ' + self.UEDevicesRemoteUser[idx] + '@' + self.UEDevicesRemoteServer[idx] + ' \'adb -s ' + device_id + ' shell "settings put global preferred_network_mode1 11"\'', '\$', 60)
				self.command('ssh ' + self.UEDevicesRemoteUser[idx] + '@' + self.UEDevicesRemoteServer[idx] + ' \'adb -s ' + device_id + ' shell "settings put global preferred_network_mode2 11"\'', '\$', 60)
				self.command('ssh ' + self.UEDevicesRemoteUser[idx] + '@' + self.UEDevicesRemoteServer[idx] + ' \'adb -s ' + device_id + ' shell "settings put global preferred_network_mode3 11"\'', '\$', 60)
				# enable data service
				self.command('ssh ' + self.UEDevicesRemoteUser[idx] + '@' + self.UEDevicesRemoteServer[idx] + ' \'adb -s ' + device_id + ' shell "svc data enable"\'', '\$', 60)
				# we need to do radio on/off cycle to make sure of above changes
				# airplane mode off // radio on
				self.command('ssh ' + self.UEDevicesRemoteUser[idx] + '@' + self.UEDevicesRemoteServer[idx] + ' ' + self.UEDevicesOnCmd[idx], '\$', 60)
				time.sleep(10)
				# airplane mode on // radio off
				self.command('ssh ' + self.UEDevicesRemoteUser[idx] + '@' + self.UEDevicesRemoteServer[idx] + ' ' + self.UEDevicesOffCmd[idx], '\$', 60)
				self.close()
				return
			# enable data service
			self.command('stdbuf -o0 adb -s ' + device_id + ' shell "svc data enable"', '\$', 60)

			# The following commands are deprecated since we no longer work on Android 7+
			# self.command('stdbuf -o0 adb -s ' + device_id + ' shell settings put global airplane_mode_on 1', '\$', 10)
			# self.command('stdbuf -o0 adb -s ' + device_id + ' shell am broadcast -a android.intent.action.AIRPLANE_MODE --ez state true', '\$', 60)
			# a dedicated script has to be installed inside the UE
			# airplane mode on means call /data/local/tmp/off
			if device_id == '84B7N16418004022':
				self.command('stdbuf -o0 adb -s ' + device_id + ' shell "su - root -c /data/local/tmp/off"', '\$', 60)
			else:
				self.command('stdbuf -o0 adb -s ' + device_id + ' shell /data/local/tmp/off', '\$', 60)
			#airplane mode off means call /data/local/tmp/on
			logging.debug('\u001B[1mUE (' + device_id + ') Initialize Completed\u001B[0m')
			self.close()
		except:
			os.kill(os.getppid(),signal.SIGUSR1)

	def InitializeUE(self):
		if self.ADBIPAddress == '' or self.ADBUserName == '' or self.ADBPassword == '':
			Usage()
			sys.exit('Insufficient Parameter')
		multi_jobs = []
		i = 0
		for device_id in self.UEDevices:
			p = Process(target = self.InitializeUE_common, args = (device_id,i,))
			p.daemon = True
			p.start()
			multi_jobs.append(p)
			i += 1
		for job in multi_jobs:
			job.join()
		self.CreateHtmlTestRow('N/A', 'OK', ALL_PROCESSES_OK)

	def InitializeOAIUE(self):
		if self.UEIPAddress == '' or self.UEUserName == '' or self.UEPassword == '' or self.UESourceCodePath == '':
			Usage()
			sys.exit('Insufficient Parameter')
		result = re.search('--no-L2-connect', str(self.Initialize_OAI_UE_args))
		if result is None:
			check_eNB = True
			check_OAI_UE = False
			pStatus = self.CheckProcessExist(check_eNB, check_OAI_UE)
			if (pStatus < 0):
				self.CreateHtmlTestRow(self.Initialize_OAI_UE_args, 'KO', pStatus)
				self.CreateHtmlTabFooter(False)
				sys.exit(1)
		self.open(self.UEIPAddress, self.UEUserName, self.UEPassword)
		# b2xx_fx3_utils reset procedure
		self.command('echo ' + self.UEPassword + ' | sudo -S uhd_find_devices', '\$', 10)
		result = re.search('type: b200', str(self.ssh.before))
		if result is not None:
			logging.debug('Found a B2xx device --> resetting it')
			self.command('echo ' + self.UEPassword + ' | sudo -S b2xx_fx3_utils --reset-device', '\$', 10)
			# Reloading FGPA bin firmware
			self.command('echo ' + self.UEPassword + ' | sudo -S uhd_find_devices', '\$', 15)
		else:
			logging.debug('Did not find any B2xx device')
		self.command('cd ' + self.UESourceCodePath, '\$', 5)
		self.command('source oaienv', '\$', 5)
		self.command('cd cmake_targets/lte_build_oai/build', '\$', 5)
		result = re.search('--no-L2-connect', str(self.Initialize_OAI_UE_args))
		# We may have to regenerate the .u* files
		if result is None:
			self.command('ls /tmp/*.sed', '\$', 5)
			result = re.search('adapt_usim_parameters', str(self.ssh.before))
			if result is not None:
				self.command('sed -f /tmp/adapt_usim_parameters.sed ../../../openair3/NAS/TOOLS/ue_eurecom_test_sfr.conf > ../../../openair3/NAS/TOOLS/ci-ue_eurecom_test_sfr.conf', '\$', 5)
			else:
				self.command('sed -e "s#93#92#" -e "s#8baf473f2f8fd09487cccbd7097c6862#fec86ba6eb707ed08905757b1bb44b8f#" -e "s#e734f8734007d6c5ce7a0508809e7e9c#C42449363BBAD02B66D16BC975D77CC1#" ../../../openair3/NAS/TOOLS/ue_eurecom_test_sfr.conf > ../../../openair3/NAS/TOOLS/ci-ue_eurecom_test_sfr.conf', '\$', 5)
			self.command('echo ' + self.UEPassword + ' | sudo -S rm -Rf .u*', '\$', 5)
			self.command('echo ' + self.UEPassword + ' | sudo -S ../../../targets/bin/conf2uedata -c ../../../openair3/NAS/TOOLS/ci-ue_eurecom_test_sfr.conf -o .', '\$', 5)
		# Launch UE with the modified config file
		self.command('echo "ulimit -c unlimited && ./lte-uesoftmodem ' + self.Initialize_OAI_UE_args + '" > ./my-lte-uesoftmodem-run' + str(self.UE_instance) + '.sh', '\$', 5)
		self.command('chmod 775 ./my-lte-uesoftmodem-run' + str(self.UE_instance) + '.sh', '\$', 5)
		self.UELogFile = 'ue_' + self.testCase_id + '.log'

		# We are now looping several times to hope we really sync w/ an eNB
		doOutterLoop = True
		outterLoopCounter = 5
		gotSyncStatus = True
		fullSyncStatus = True
		while (doOutterLoop):
			self.command('cd ' + self.UESourceCodePath + '/cmake_targets/lte_build_oai/build', '\$', 5)
			self.command('echo ' + self.UEPassword + ' | sudo -S rm -Rf ' + self.UESourceCodePath + '/cmake_targets/ue_' + self.testCase_id + '.log', '\$', 5)
			self.command('echo ' + self.UEPassword + ' | sudo -S -E daemon --inherit --unsafe --name=ue' + str(self.UE_instance) + '_daemon --chdir=' + self.UESourceCodePath + '/cmake_targets/lte_build_oai/build -o ' + self.UESourceCodePath + '/cmake_targets/ue_' + self.testCase_id + '.log ./my-lte-uesoftmodem-run' + str(self.UE_instance) + '.sh', '\$', 5)
			time.sleep(6)
			self.command('cd ../..', '\$', 5)
			doLoop = True
			loopCounter = 10
			gotSyncStatus = True
			# the 'got sync' message is for the UE threads synchronization
			while (doLoop):
				loopCounter = loopCounter - 1
				if (loopCounter == 0):
					# Here should never occur
					logging.error('"got sync" message never showed!')
					gotSyncStatus = False
					doLoop = False
					continue
				self.command('stdbuf -o0 cat ue_' + self.testCase_id + '.log | egrep --text --color=never -i "wait|sync"', '\$', 4)
				result = re.search('got sync', str(self.ssh.before))
				if result is None:
					time.sleep(6)
				else:
					doLoop = False
					logging.debug('Found "got sync" message!')
			if gotSyncStatus == False:
				# we certainly need to stop the lte-uesoftmodem process if it is still running!
				self.command('ps -aux | grep --text --color=never softmodem | grep -v grep', '\$', 4)
				result = re.search('lte-uesoftmodem', str(self.ssh.before))
				if result is not None:
					self.command('echo ' + self.UEPassword + ' | sudo -S killall --signal=SIGINT lte-uesoftmodem', '\$', 4)
					time.sleep(3)
			# We are now checking if sync w/ eNB DOES NOT OCCUR
			# Usually during the cell synchronization stage, the UE returns with No cell synchronization message
			doLoop = True
			loopCounter = 10
			while (doLoop):
				loopCounter = loopCounter - 1
				if (loopCounter == 0):
					# Here we do have a great chance that the UE did cell-sync w/ eNB
					doLoop = False
					doOutterLoop = False
					fullSyncStatus = True
					continue
				self.command('stdbuf -o0 cat ue_' + self.testCase_id + '.log | egrep --text --color=never -i "wait|sync"', '\$', 4)
				result = re.search('No cell synchronization found', str(self.ssh.before))
				if result is None:
					time.sleep(6)
				else:
					doLoop = False
					fullSyncStatus = False
					logging.debug('Found: "No cell synchronization" message! --> try again')
					time.sleep(6)
					self.command('ps -aux | grep --text --color=never softmodem | grep -v grep', '\$', 4)
					result = re.search('lte-uesoftmodem', str(self.ssh.before))
					if result is not None:
						self.command('echo ' + self.UEPassword + ' | sudo -S killall --signal=SIGINT lte-uesoftmodem', '\$', 4)
			outterLoopCounter = outterLoopCounter - 1
			if (outterLoopCounter == 0):
				doOutterLoop = False

		if fullSyncStatus and gotSyncStatus:
			result = re.search('--no-L2-connect', str(self.Initialize_OAI_UE_args))
			if result is None:
				self.command('ifconfig oaitun_ue1', '\$', 4)
				# ifconfig output is different between ubuntu 16 and ubuntu 18
				result = re.search('inet addr:1|inet 1', str(self.ssh.before))
				if result is not None:
					logging.debug('\u001B[1m oaitun_ue1 interface is mounted and configured\u001B[0m')
					tunnelInterfaceStatus = True
				else:
					logging.debug(str(self.ssh.before))
					logging.error('\u001B[1m oaitun_ue1 interface is either NOT mounted or NOT configured\u001B[0m')
					tunnelInterfaceStatus = False
				if self.eNBmbmsEnables[0]:
					self.command('ifconfig oaitun_uem1', '\$', 4)
					result = re.search('inet addr', str(self.ssh.before))
					if result is not None:
						logging.debug('\u001B[1m oaitun_uem1 interface is mounted and configured\u001B[0m')
						tunnelInterfaceStatus = tunnelInterfaceStatus and True
					else:
						logging.error('\u001B[1m oaitun_uem1 interface is either NOT mounted or NOT configured\u001B[0m')
						tunnelInterfaceStatus = False
			else:
				tunnelInterfaceStatus = True

		self.close()
		if fullSyncStatus and gotSyncStatus and tunnelInterfaceStatus:
			self.CreateHtmlTestRow(self.Initialize_OAI_UE_args, 'OK', ALL_PROCESSES_OK, 'OAI UE')
			logging.debug('\u001B[1m Initialize OAI UE Completed\u001B[0m')
			if (self.ADBIPAddress != 'none'):
				self.UEDevices = []
				self.UEDevices.append('OAI-UE')
				self.UEDevicesStatus = []
				self.UEDevicesStatus.append(UE_STATUS_DETACHED)
		else:
			if self.eNBmbmsEnables[0]:
				self.htmlUEFailureMsg = 'oaitun_ue1/oaitun_uem1 interfaces are either NOT mounted or NOT configured'
			else:
				self.htmlUEFailureMsg = 'oaitun_ue1 interface is either NOT mounted or NOT configured'
			self.CreateHtmlTestRow(self.Initialize_OAI_UE_args, 'KO', OAI_UE_PROCESS_NO_TUNNEL_INTERFACE, 'OAI UE')
			logging.error('\033[91mInitialize OAI UE Failed! \033[0m')
			self.AutoTerminateUEandeNB()

	def checkDevTTYisUnlocked(self):
		self.open(self.ADBIPAddress, self.ADBUserName, self.ADBPassword)
		count = 0
		while count < 5:
			self.command('echo ' + self.ADBPassword + ' | sudo -S lsof | grep ttyUSB0', '\$', 10)
			result = re.search('picocom', str(self.ssh.before))
			if result is None:
				count = 10
			else:
				time.sleep(5)
				count = count + 1
		self.close()

	def InitializeCatM(self):
		if self.ADBIPAddress == '' or self.ADBUserName == '' or self.ADBPassword == '':
			Usage()
			sys.exit('Insufficient Parameter')
		self.picocom_closure = True
		self.open(self.ADBIPAddress, self.ADBUserName, self.ADBPassword)
		# dummy call to start a sudo session. The picocom command does NOT handle well the `sudo -S`
		self.command('echo ' + self.ADBPassword + ' | sudo -S ls', '\$', 10)
		self.command('sudo picocom --baud 921600 --flow n --databits 8 /dev/ttyUSB0', 'Terminal ready', 10)
		time.sleep(1)
		# Calling twice AT to clear all buffers
		self.command('AT', 'OK|ERROR', 5)
		self.command('AT', 'OK', 5)
		# Doing a power cycle
		self.command('AT^RESET', 'SIMSTORE,READY', 15)
		self.command('AT', 'OK|ERROR', 5)
		self.command('AT', 'OK', 5)
		self.command('ATE1', 'OK', 5)
		# Disabling the Radio
		self.command('AT+CFUN=0', 'OK', 5)
		logging.debug('\u001B[1m Cellular Functionality disabled\u001B[0m')
		# Checking if auto-attach is enabled
		self.command('AT^AUTOATT?', 'OK', 5)
		result = re.search('AUTOATT: (?P<state>[0-9\-]+)', str(self.ssh.before))
		if result is not None:
			if result.group('state') is not None:
				autoAttachState = int(result.group('state'))
				if autoAttachState is not None:
					if autoAttachState == 0:
						self.command('AT^AUTOATT=1', 'OK', 5)
					logging.debug('\u001B[1m Auto-Attach enabled\u001B[0m')
		else:
			logging.debug('\u001B[1;37;41m Could not check Auto-Attach! \u001B[0m')
		# Force closure of picocom but device might still be locked
		self.close()
		self.picocom_closure = False
		self.CreateHtmlTestRow('N/A', 'OK', ALL_PROCESSES_OK)
		self.checkDevTTYisUnlocked()

	def TerminateCatM(self):
		if self.ADBIPAddress == '' or self.ADBUserName == '' or self.ADBPassword == '':
			Usage()
			sys.exit('Insufficient Parameter')
		self.picocom_closure = True
		self.open(self.ADBIPAddress, self.ADBUserName, self.ADBPassword)
		# dummy call to start a sudo session. The picocom command does NOT handle well the `sudo -S`
		self.command('echo ' + self.ADBPassword + ' | sudo -S ls', '\$', 10)
		self.command('sudo picocom --baud 921600 --flow n --databits 8 /dev/ttyUSB0', 'Terminal ready', 10)
		time.sleep(1)
		# Calling twice AT to clear all buffers
		self.command('AT', 'OK|ERROR', 5)
		self.command('AT', 'OK', 5)
		# Disabling the Radio
		self.command('AT+CFUN=0', 'OK', 5)
		logging.debug('\u001B[1m Cellular Functionality disabled\u001B[0m')
		self.close()
		self.picocom_closure = False
		self.CreateHtmlTestRow('N/A', 'OK', ALL_PROCESSES_OK)
		self.checkDevTTYisUnlocked()

	def AttachCatM(self):
		if self.ADBIPAddress == '' or self.ADBUserName == '' or self.ADBPassword == '':
			Usage()
			sys.exit('Insufficient Parameter')
		self.picocom_closure = True
		self.open(self.ADBIPAddress, self.ADBUserName, self.ADBPassword)
		# dummy call to start a sudo session. The picocom command does NOT handle well the `sudo -S`
		self.command('echo ' + self.ADBPassword + ' | sudo -S ls', '\$', 10)
		self.command('sudo picocom --baud 921600 --flow n --databits 8 /dev/ttyUSB0', 'Terminal ready', 10)
		time.sleep(1)
		# Calling twice AT to clear all buffers
		self.command('AT', 'OK|ERROR', 5)
		self.command('AT', 'OK', 5)
		# Enabling the Radio
		self.command('AT+CFUN=1', 'SIMSTORE,READY', 5)
		logging.debug('\u001B[1m Cellular Functionality enabled\u001B[0m')
		time.sleep(4)
		# We should check if we register
		count = 0
		attach_cnt = 0
		attach_status = False
		while count < 5:
			self.command('AT+CEREG?', 'OK', 5)
			result = re.search('CEREG: 2,(?P<state>[0-9\-]+),', str(self.ssh.before))
			if result is not None:
				mDataConnectionState = int(result.group('state'))
				if mDataConnectionState is not None:
					if mDataConnectionState == 1:
						count = 10
						attach_status = True
						result = re.search('CEREG: 2,1,"(?P<networky>[0-9A-Z]+)","(?P<networkz>[0-9A-Z]+)"', str(self.ssh.before))
						if result is not None:
							networky = result.group('networky')
							networkz = result.group('networkz')
							logging.debug('\u001B[1m CAT-M module attached to eNB (' + str(networky) + '/' + str(networkz) + ')\u001B[0m')
						else:
							logging.debug('\u001B[1m CAT-M module attached to eNB\u001B[0m')
					else:
						logging.debug('+CEREG: 2,' + str(mDataConnectionState))
						attach_cnt = attach_cnt + 1
			else:
				logging.debug(str(self.ssh.before))
				attach_cnt = attach_cnt + 1
			count = count + 1
			time.sleep(1)
		if attach_status:
			self.command('AT+CESQ', 'OK', 5)
			result = re.search('CESQ: 99,99,255,255,(?P<rsrq>[0-9]+),(?P<rsrp>[0-9]+)', str(self.ssh.before))
			if result is not None:
				nRSRQ = int(result.group('rsrq'))
				nRSRP = int(result.group('rsrp'))
				if (nRSRQ is not None) and (nRSRP is not None):
					logging.debug('    RSRQ = ' + str(-20+(nRSRQ/2)) + ' dB')
					logging.debug('    RSRP = ' + str(-140+nRSRP) + ' dBm')
		self.close()
		self.picocom_closure = False
		html_queue = SimpleQueue()
		self.checkDevTTYisUnlocked()
		if attach_status:
			html_cell = '<pre style="background-color:white">CAT-M module Attachment Completed in ' + str(attach_cnt+4) + ' seconds'
			if (nRSRQ is not None) and (nRSRP is not None):
				html_cell += '\n   RSRQ = ' + str(-20+(nRSRQ/2)) + ' dB'
				html_cell += '\n   RSRP = ' + str(-140+nRSRP) + ' dBm</pre>'
			else:
				html_cell += '</pre>'
			html_queue.put(html_cell)
			self.CreateHtmlTestRowQueue('N/A', 'OK', 1, html_queue)
		else:
			logging.error('\u001B[1m CAT-M module Attachment Failed\u001B[0m')
			html_cell = '<pre style="background-color:white">CAT-M module Attachment Failed</pre>'
			html_queue.put(html_cell)
			self.CreateHtmlTestRowQueue('N/A', 'KO', 1, html_queue)
			self.AutoTerminateUEandeNB()

	def PingCatM(self):
		if self.EPCIPAddress == '' or self.EPCUserName == '' or self.EPCPassword == '' or self.EPCSourceCodePath == '':
			Usage()
			sys.exit('Insufficient Parameter')
		check_eNB = True
		check_OAI_UE = False
		pStatus = self.CheckProcessExist(check_eNB, check_OAI_UE)
		if (pStatus < 0):
			self.CreateHtmlTestRow(self.ping_args, 'KO', pStatus)
			self.AutoTerminateUEandeNB()
			return
		try:
			statusQueue = SimpleQueue()
			lock = Lock()
			self.open(self.EPCIPAddress, self.EPCUserName, self.EPCPassword)
			self.command('cd ' + self.EPCSourceCodePath, '\$', 5)
			self.command('cd scripts', '\$', 5)
			if re.match('OAI', self.EPCType, re.IGNORECASE):
				logging.debug('Using the OAI EPC HSS: not implemented yet')
				self.CreateHtmlTestRow(self.ping_args, 'KO', pStatus)
				self.CreateHtmlTabFooter(False)
				sys.exit(1)
			else:
				self.command('egrep --color=never "Allocated ipv4 addr" /opt/ltebox/var/log/xGwLog.0', '\$', 5)
				result = re.search('Allocated ipv4 addr: (?P<ipaddr>[0-9\.]+) from Pool', str(self.ssh.before))
				if result is not None:
					moduleIPAddr = result.group('ipaddr')
				else:
					self.CreateHtmlTestRow(self.ping_args, 'KO', pStatus)
					self.AutoTerminateUEandeNB()
					return
			ping_time = re.findall("-c (\d+)",str(self.ping_args))
			device_id = 'catm'
			ping_status = self.command('stdbuf -o0 ping ' + self.ping_args + ' ' + str(moduleIPAddr) + ' 2>&1 | stdbuf -o0 tee ping_' + self.testCase_id + '_' + device_id + '.log', '\$', int(ping_time[0])*1.5)
			# TIMEOUT CASE
			if ping_status < 0:
				message = 'Ping with UE (' + str(moduleIPAddr) + ') crashed due to TIMEOUT!'
				logging.debug('\u001B[1;37;41m ' + message + ' \u001B[0m')
				self.close()
				self.ping_iperf_wrong_exit(lock, moduleIPAddr, device_id, statusQueue, message)
				return
			result = re.search(', (?P<packetloss>[0-9\.]+)% packet loss, time [0-9\.]+ms', str(self.ssh.before))
			if result is None:
				message = 'Packet Loss Not Found!'
				logging.debug('\u001B[1;37;41m ' + message + ' \u001B[0m')
				self.close()
				self.ping_iperf_wrong_exit(lock, moduleIPAddr, device_id, statusQueue, message)
				return
			packetloss = result.group('packetloss')
			if float(packetloss) == 100:
				message = 'Packet Loss is 100%'
				logging.debug('\u001B[1;37;41m ' + message + ' \u001B[0m')
				self.close()
				self.ping_iperf_wrong_exit(lock, moduleIPAddr, device_id, statusQueue, message)
				return
			result = re.search('rtt min\/avg\/max\/mdev = (?P<rtt_min>[0-9\.]+)\/(?P<rtt_avg>[0-9\.]+)\/(?P<rtt_max>[0-9\.]+)\/[0-9\.]+ ms', str(self.ssh.before))
			if result is None:
				message = 'Ping RTT_Min RTT_Avg RTT_Max Not Found!'
				logging.debug('\u001B[1;37;41m ' + message + ' \u001B[0m')
				self.close()
				self.ping_iperf_wrong_exit(lock, moduleIPAddr, device_id, statusQueue, message)
				return
			rtt_min = result.group('rtt_min')
			rtt_avg = result.group('rtt_avg')
			rtt_max = result.group('rtt_max')
			pal_msg = 'Packet Loss : ' + packetloss + '%'
			min_msg = 'RTT(Min)    : ' + rtt_min + ' ms'
			avg_msg = 'RTT(Avg)    : ' + rtt_avg + ' ms'
			max_msg = 'RTT(Max)    : ' + rtt_max + ' ms'
			lock.acquire()
			logging.debug('\u001B[1;37;44m ping result (' + moduleIPAddr + ') \u001B[0m')
			logging.debug('\u001B[1;34m    ' + pal_msg + '\u001B[0m')
			logging.debug('\u001B[1;34m    ' + min_msg + '\u001B[0m')
			logging.debug('\u001B[1;34m    ' + avg_msg + '\u001B[0m')
			logging.debug('\u001B[1;34m    ' + max_msg + '\u001B[0m')
			qMsg = pal_msg + '\n' + min_msg + '\n' + avg_msg + '\n' + max_msg
			packetLossOK = True
			if packetloss is not None:
				if float(packetloss) > float(self.ping_packetloss_threshold):
					qMsg += '\nPacket Loss too high'
					logging.debug('\u001B[1;37;41m Packet Loss too high \u001B[0m')
					packetLossOK = False
				elif float(packetloss) > 0:
					qMsg += '\nPacket Loss is not 0%'
					logging.debug('\u001B[1;30;43m Packet Loss is not 0% \u001B[0m')
			lock.release()
			self.close()
			html_cell = '<pre style="background-color:white">CAT-M module\nIP Address  : ' + moduleIPAddr + '\n' + qMsg + '</pre>'
			statusQueue.put(html_cell)
			if (packetLossOK):
				self.CreateHtmlTestRowQueue(self.ping_args, 'OK', 1, statusQueue)
			else:
				self.CreateHtmlTestRowQueue(self.ping_args, 'KO', 1, statusQueue)
				self.AutoTerminateUEandeNB()
		except:
			os.kill(os.getppid(),signal.SIGUSR1)

	def AttachUE_common(self, device_id, statusQueue, lock, idx):
		try:
			self.open(self.ADBIPAddress, self.ADBUserName, self.ADBPassword)
			if self.ADBCentralized:
				if device_id == '84B7N16418004022':
					self.command('stdbuf -o0 adb -s ' + device_id + ' shell "su - root -c /data/local/tmp/on"', '\$', 60)
				else:
					self.command('stdbuf -o0 adb -s ' + device_id + ' shell /data/local/tmp/on', '\$', 60)
			else:
				# airplane mode off // radio on
				self.command('ssh ' + self.UEDevicesRemoteUser[idx] + '@' + self.UEDevicesRemoteServer[idx] + ' ' + self.UEDevicesOnCmd[idx], '\$', 60)
			time.sleep(2)
			max_count = 45
			count = max_count
			while count > 0:
				if self.ADBCentralized:
					self.command('stdbuf -o0 adb -s ' + device_id + ' shell "dumpsys telephony.registry" | grep -m 1 mDataConnectionState', '\$', 15)
				else:
					self.command('ssh ' + self.UEDevicesRemoteUser[idx] + '@' + self.UEDevicesRemoteServer[idx] + ' \'adb -s ' + device_id + ' shell "dumpsys telephony.registry"\' | grep -m 1 mDataConnectionState', '\$', 60)
				result = re.search('mDataConnectionState.*=(?P<state>[0-9\-]+)', str(self.ssh.before))
				if result is None:
					logging.debug('\u001B[1;37;41m mDataConnectionState Not Found! \u001B[0m')
					lock.acquire()
					statusQueue.put(-1)
					statusQueue.put(device_id)
					statusQueue.put('mDataConnectionState Not Found!')
					lock.release()
					break
				mDataConnectionState = int(result.group('state'))
				if mDataConnectionState == 2:
					logging.debug('\u001B[1mUE (' + device_id + ') Attach Completed\u001B[0m')
					lock.acquire()
					statusQueue.put(max_count - count)
					statusQueue.put(device_id)
					statusQueue.put('Attach Completed')
					lock.release()
					break
				count = count - 1
				if count == 15 or count == 30:
					logging.debug('\u001B[1;30;43m Retry UE (' + device_id + ') Flight Mode Off \u001B[0m')
					if self.ADBCentralized:
						if device_id == '84B7N16418004022':
							self.command('stdbuf -o0 adb -s ' + device_id + ' shell "su - root -c /data/local/tmp/off"', '\$', 60)
						else:
							self.command('stdbuf -o0 adb -s ' + device_id + ' shell /data/local/tmp/off', '\$', 60)
					else:
						self.command('ssh ' + self.UEDevicesRemoteUser[idx] + '@' + self.UEDevicesRemoteServer[idx] + ' ' + self.UEDevicesOffCmd[idx], '\$', 60)
					time.sleep(0.5)
					if self.ADBCentralized:
						if device_id == '84B7N16418004022':
							self.command('stdbuf -o0 adb -s ' + device_id + ' shell "su - root -c /data/local/tmp/on"', '\$', 60)
						else:
							self.command('stdbuf -o0 adb -s ' + device_id + ' shell /data/local/tmp/on', '\$', 60)
					else:
						self.command('ssh ' + self.UEDevicesRemoteUser[idx] + '@' + self.UEDevicesRemoteServer[idx] + ' ' + self.UEDevicesOnCmd[idx], '\$', 60)
					time.sleep(0.5)
				logging.debug('\u001B[1mWait UE (' + device_id + ') a second until mDataConnectionState=2 (' + str(max_count-count) + ' times)\u001B[0m')
				time.sleep(1)
			if count == 0:
				logging.debug('\u001B[1;37;41m UE (' + device_id + ') Attach Failed \u001B[0m')
				lock.acquire()
				statusQueue.put(-1)
				statusQueue.put(device_id)
				statusQueue.put('Attach Failed')
				lock.release()
			self.close()
		except:
			os.kill(os.getppid(),signal.SIGUSR1)

	def AttachUE(self):
		if self.ADBIPAddress == '' or self.ADBUserName == '' or self.ADBPassword == '':
			Usage()
			sys.exit('Insufficient Parameter')
		check_eNB = True
		check_OAI_UE = False
		pStatus = self.CheckProcessExist(check_eNB, check_OAI_UE)
		if (pStatus < 0):
			self.CreateHtmlTestRow('N/A', 'KO', pStatus)
			self.AutoTerminateUEandeNB()
			return
		multi_jobs = []
		status_queue = SimpleQueue()
		lock = Lock()
		nb_ue_to_connect = 0
		for device_id in self.UEDevices:
			if (self.nbMaxUEtoAttach == -1) or (nb_ue_to_connect < self.nbMaxUEtoAttach):
				self.UEDevicesStatus[nb_ue_to_connect] = UE_STATUS_ATTACHING
				p = Process(target = self.AttachUE_common, args = (device_id, status_queue, lock,nb_ue_to_connect,))
				p.daemon = True
				p.start()
				multi_jobs.append(p)
			nb_ue_to_connect = nb_ue_to_connect + 1
		for job in multi_jobs:
			job.join()

		if (status_queue.empty()):
			self.CreateHtmlTestRow('N/A', 'KO', ALL_PROCESSES_OK)
			self.AutoTerminateUEandeNB()
			return
		else:
			attach_status = True
			html_queue = SimpleQueue()
			while (not status_queue.empty()):
				count = status_queue.get()
				if (count < 0):
					attach_status = False
				device_id = status_queue.get()
				message = status_queue.get()
				if (count < 0):
					html_cell = '<pre style="background-color:white">UE (' + device_id + ')\n' + message + '</pre>'
				else:
					html_cell = '<pre style="background-color:white">UE (' + device_id + ')\n' + message + ' in ' + str(count + 2) + ' seconds</pre>'
				html_queue.put(html_cell)
			if (attach_status):
				cnt = 0
				while cnt < len(self.UEDevices):
					if self.UEDevicesStatus[cnt] == UE_STATUS_ATTACHING:
						self.UEDevicesStatus[cnt] = UE_STATUS_ATTACHED
					cnt += 1
				self.CreateHtmlTestRowQueue('N/A', 'OK', len(self.UEDevices), html_queue)
				result = re.search('T_stdout', str(self.Initialize_eNB_args))
				if result is not None:
					logging.debug('Waiting 5 seconds to fill up record file')
					time.sleep(5)
			else:
				self.CreateHtmlTestRowQueue('N/A', 'KO', len(self.UEDevices), html_queue)
				self.AutoTerminateUEandeNB()

	def DetachUE_common(self, device_id, idx):
		try:
			self.open(self.ADBIPAddress, self.ADBUserName, self.ADBPassword)
			if self.ADBCentralized:
				if device_id == '84B7N16418004022':
					self.command('stdbuf -o0 adb -s ' + device_id + ' shell "su - root -c /data/local/tmp/off"', '\$', 60)
				else:
					self.command('stdbuf -o0 adb -s ' + device_id + ' shell /data/local/tmp/off', '\$', 60)
			else:
				self.command('ssh ' + self.UEDevicesRemoteUser[idx] + '@' + self.UEDevicesRemoteServer[idx] + ' ' + self.UEDevicesOffCmd[idx], '\$', 60)
			logging.debug('\u001B[1mUE (' + device_id + ') Detach Completed\u001B[0m')
			self.close()
		except:
			os.kill(os.getppid(),signal.SIGUSR1)

	def DetachUE(self):
		if self.ADBIPAddress == '' or self.ADBUserName == '' or self.ADBPassword == '':
			Usage()
			sys.exit('Insufficient Parameter')
		check_eNB = True
		check_OAI_UE = False
		pStatus = self.CheckProcessExist(check_eNB, check_OAI_UE)
		if (pStatus < 0):
			self.CreateHtmlTestRow('N/A', 'KO', pStatus)
			self.AutoTerminateUEandeNB()
			return
		multi_jobs = []
		cnt = 0
		for device_id in self.UEDevices:
			self.UEDevicesStatus[cnt] = UE_STATUS_DETACHING
			p = Process(target = self.DetachUE_common, args = (device_id,cnt,))
			p.daemon = True
			p.start()
			multi_jobs.append(p)
			cnt += 1
		for job in multi_jobs:
			job.join()
		self.CreateHtmlTestRow('N/A', 'OK', ALL_PROCESSES_OK)
		result = re.search('T_stdout', str(self.Initialize_eNB_args))
		if result is not None:
			logging.debug('Waiting 5 seconds to fill up record file')
			time.sleep(5)
		cnt = 0
		while cnt < len(self.UEDevices):
			self.UEDevicesStatus[cnt] = UE_STATUS_DETACHED
			cnt += 1

	def RebootUE_common(self, device_id):
		try:
			self.open(self.ADBIPAddress, self.ADBUserName, self.ADBPassword)
			previousmDataConnectionStates = []
			# Save mDataConnectionState
			self.command('stdbuf -o0 adb -s ' + device_id + ' shell dumpsys telephony.registry | grep mDataConnectionState', '\$', 15)
			self.command('stdbuf -o0 adb -s ' + device_id + ' shell dumpsys telephony.registry | grep mDataConnectionState', '\$', 15)
			result = re.search('mDataConnectionState.*=(?P<state>[0-9\-]+)', str(self.ssh.before))
			if result is None:
				logging.debug('\u001B[1;37;41m mDataConnectionState Not Found! \u001B[0m')
				sys.exit(1)
			previousmDataConnectionStates.append(int(result.group('state')))
			# Reboot UE
			self.command('stdbuf -o0 adb -s ' + device_id + ' shell reboot', '\$', 10)
			time.sleep(60)
			previousmDataConnectionState = previousmDataConnectionStates.pop(0)
			count = 180
			while count > 0:
				count = count - 1
				self.command('stdbuf -o0 adb -s ' + device_id + ' shell dumpsys telephony.registry | grep mDataConnectionState', '\$', 15)
				result = re.search('mDataConnectionState.*=(?P<state>[0-9\-]+)', str(self.ssh.before))
				if result is None:
					mDataConnectionState = None
				else:
					mDataConnectionState = int(result.group('state'))
					logging.debug('mDataConnectionState = ' + result.group('state'))
				if mDataConnectionState is None or (previousmDataConnectionState == 2 and mDataConnectionState != 2):
					logging.debug('\u001B[1mWait UE (' + device_id + ') a second until reboot completion (' + str(180-count) + ' times)\u001B[0m')
					time.sleep(1)
				else:
					logging.debug('\u001B[1mUE (' + device_id + ') Reboot Completed\u001B[0m')
					break
			if count == 0:
				logging.debug('\u001B[1;37;41m UE (' + device_id + ') Reboot Failed \u001B[0m')
				sys.exit(1)
			self.close()
		except:
			os.kill(os.getppid(),signal.SIGUSR1)

	def RebootUE(self):
		if self.ADBIPAddress == '' or self.ADBUserName == '' or self.ADBPassword == '':
			Usage()
			sys.exit('Insufficient Parameter')
		check_eNB = True
		check_OAI_UE = False
		pStatus = self.CheckProcessExist(check_eNB, check_OAI_UE)
		if (pStatus < 0):
			self.CreateHtmlTestRow('N/A', 'KO', pStatus)
			self.CreateHtmlTabFooter(False)
			sys.exit(1)
		multi_jobs = []
		for device_id in self.UEDevices:
			p = Process(target = self.RebootUE_common, args = (device_id,))
			p.daemon = True
			p.start()
			multi_jobs.append(p)
		for job in multi_jobs:
			job.join()
		self.CreateHtmlTestRow('N/A', 'OK', ALL_PROCESSES_OK)

	def DataDisableUE_common(self, device_id, idx):
		try:
			self.open(self.ADBIPAddress, self.ADBUserName, self.ADBPassword)
			# disable data service
			if self.ADBCentralized:
				self.command('stdbuf -o0 adb -s ' + device_id + ' shell "svc data disable"', '\$', 60)
			else:
				self.command('ssh ' + self.UEDevicesRemoteUser[idx] + '@' + self.UEDevicesRemoteServer[idx] + ' \'adb -s ' + device_id + ' shell "svc data disable"\'', '\$', 60)
			logging.debug('\u001B[1mUE (' + device_id + ') Disabled Data Service\u001B[0m')
			self.close()
		except:
			os.kill(os.getppid(),signal.SIGUSR1)

	def DataDisableUE(self):
		if self.ADBIPAddress == '' or self.ADBUserName == '' or self.ADBPassword == '':
			Usage()
			sys.exit('Insufficient Parameter')
		multi_jobs = []
		i = 0
		for device_id in self.UEDevices:
			p = Process(target = self.DataDisableUE_common, args = (device_id,i,))
			p.daemon = True
			p.start()
			multi_jobs.append(p)
			i += 1
		for job in multi_jobs:
			job.join()
		self.CreateHtmlTestRow('N/A', 'OK', ALL_PROCESSES_OK)

	def DataEnableUE_common(self, device_id, idx):
		try:
			self.open(self.ADBIPAddress, self.ADBUserName, self.ADBPassword)
			# enable data service
			if self.ADBCentralized:
				self.command('stdbuf -o0 adb -s ' + device_id + ' shell "svc data enable"', '\$', 60)
			else:
				self.command('ssh ' + self.UEDevicesRemoteUser[idx] + '@' + self.UEDevicesRemoteServer[idx] + ' \'adb -s ' + device_id + ' shell "svc data enable"\'', '\$', 60)
			logging.debug('\u001B[1mUE (' + device_id + ') Enabled Data Service\u001B[0m')
			self.close()
		except:
			os.kill(os.getppid(),signal.SIGUSR1)

	def DataEnableUE(self):
		if self.ADBIPAddress == '' or self.ADBUserName == '' or self.ADBPassword == '':
			Usage()
			sys.exit('Insufficient Parameter')
		multi_jobs = []
		i = 0
		for device_id in self.UEDevices:
			p = Process(target = self.DataEnableUE_common, args = (device_id,i,))
			p.daemon = True
			p.start()
			multi_jobs.append(p)
			i += 1
		for job in multi_jobs:
			job.join()
		self.CreateHtmlTestRow('N/A', 'OK', ALL_PROCESSES_OK)

	def GetAllUEDevices(self, terminate_ue_flag):
		if self.ADBIPAddress == '' or self.ADBUserName == '' or self.ADBPassword == '':
			Usage()
			sys.exit('Insufficient Parameter')
		self.open(self.ADBIPAddress, self.ADBUserName, self.ADBPassword)
		if self.ADBCentralized:
			self.command('adb devices', '\$', 15)
			self.UEDevices = re.findall("\\\\r\\\\n([A-Za-z0-9]+)\\\\tdevice",str(self.ssh.before))
			self.close()
		else:
			if (os.path.isfile('./phones_list.txt')):
				os.remove('./phones_list.txt')
			self.command('ls /etc/*/phones*.txt', '\$', 5)
			result = re.search('/etc/ci/phones_list.txt', str(self.ssh.before))
			self.close()
			if (result is not None) and (len(self.UEDevices) == 0):
				self.copyin(self.ADBIPAddress, self.ADBUserName, self.ADBPassword, '/etc/ci/phones_list.txt', '.')
				if (os.path.isfile('./phones_list.txt')):
					phone_list_file = open('./phones_list.txt', 'r')
					for line in phone_list_file.readlines():
						line = line.strip()
						result = re.search('^#', line)
						if result is not None:
							continue
						comma_split = line.split(",")
						self.UEDevices.append(comma_split[0])
						self.UEDevicesRemoteServer.append(comma_split[1])
						self.UEDevicesRemoteUser.append(comma_split[2])
						self.UEDevicesOffCmd.append(comma_split[3])
						self.UEDevicesOnCmd.append(comma_split[4])
						self.UEDevicesRebootCmd.append(comma_split[5])
					phone_list_file.close()

		if terminate_ue_flag == False:
			if len(self.UEDevices) == 0:
				logging.debug('\u001B[1;37;41m UE Not Found! \u001B[0m')
				sys.exit(1)
		if len(self.UEDevicesStatus) == 0:
			cnt = 0
			while cnt < len(self.UEDevices):
				self.UEDevicesStatus.append(UE_STATUS_DETACHED)
				cnt += 1

	def GetAllCatMDevices(self, terminate_ue_flag):
		if self.ADBIPAddress == '' or self.ADBUserName == '' or self.ADBPassword == '':
			Usage()
			sys.exit('Insufficient Parameter')
		self.open(self.ADBIPAddress, self.ADBUserName, self.ADBPassword)
		if self.ADBCentralized:
			self.command('lsusb | egrep "Future Technology Devices International, Ltd FT2232C" | sed -e "s#:.*##" -e "s# #_#g"', '\$', 15)
			self.CatMDevices = re.findall("\\\\r\\\\n([A-Za-z0-9_]+)",str(self.ssh.before))
		else:
			if (os.path.isfile('./modules_list.txt')):
				os.remove('./modules_list.txt')
			self.command('ls /etc/*/modules*.txt', '\$', 5)
			result = re.search('/etc/ci/modules_list.txt', str(self.ssh.before))
			self.close()
			if result is not None:
				logging.debug('Found a module list file on ADB server')
		if terminate_ue_flag == False:
			if len(self.CatMDevices) == 0:
				logging.debug('\u001B[1;37;41m CAT-M UE Not Found! \u001B[0m')
				sys.exit(1)
		self.close()

	def CheckUEStatus_common(self, lock, device_id, statusQueue, idx):
		try:
			self.open(self.ADBIPAddress, self.ADBUserName, self.ADBPassword)
			if self.ADBCentralized:
				self.command('stdbuf -o0 adb -s ' + device_id + ' shell "dumpsys telephony.registry"', '\$', 15)
			else:
				self.command('ssh ' + self.UEDevicesRemoteUser[idx] + '@' + self.UEDevicesRemoteServer[idx] + ' \'adb -s ' + device_id + ' shell "dumpsys telephony.registry"\'', '\$', 60)
			result = re.search('mServiceState=(?P<serviceState>[0-9]+)', str(self.ssh.before))
			serviceState = 'Service State: UNKNOWN'
			if result is not None:
				lServiceState = int(result.group('serviceState'))
				if lServiceState == 3:
					serviceState = 'Service State: RADIO_POWERED_OFF'
				if lServiceState == 1:
					serviceState = 'Service State: OUT_OF_SERVICE'
				if lServiceState == 0:
					serviceState = 'Service State: IN_SERVICE'
				if lServiceState == 2:
					serviceState = 'Service State: EMERGENCY_ONLY'
			result = re.search('mDataConnectionState=(?P<dataConnectionState>[0-9]+)', str(self.ssh.before))
			dataConnectionState = 'Data State:    UNKNOWN'
			if result is not None:
				lDataConnectionState = int(result.group('dataConnectionState'))
				if lDataConnectionState == 0:
					dataConnectionState = 'Data State:    DISCONNECTED'
				if lDataConnectionState == 1:
					dataConnectionState = 'Data State:    CONNECTING'
				if lDataConnectionState == 2:
					dataConnectionState = 'Data State:    CONNECTED'
				if lDataConnectionState == 3:
					dataConnectionState = 'Data State:    SUSPENDED'
			result = re.search('mDataConnectionReason=(?P<dataConnectionReason>[0-9a-zA-Z_]+)', str(self.ssh.before))
			dataConnectionReason = 'Data Reason:   UNKNOWN'
			if result is not None:
				dataConnectionReason = 'Data Reason:   ' + result.group('dataConnectionReason')
			lock.acquire()
			logging.debug('\u001B[1;37;44m Status Check (' + str(device_id) + ') \u001B[0m')
			logging.debug('\u001B[1;34m    ' + serviceState + '\u001B[0m')
			logging.debug('\u001B[1;34m    ' + dataConnectionState + '\u001B[0m')
			logging.debug('\u001B[1;34m    ' + dataConnectionReason + '\u001B[0m')
			statusQueue.put(0)
			statusQueue.put(device_id)
			qMsg = serviceState + '\n' + dataConnectionState + '\n' + dataConnectionReason
			statusQueue.put(qMsg)
			lock.release()
			self.close()
		except:
			os.kill(os.getppid(),signal.SIGUSR1)

	def CheckStatusUE(self):
		if self.ADBIPAddress == '' or self.ADBUserName == '' or self.ADBPassword == '':
			Usage()
			sys.exit('Insufficient Parameter')
		check_eNB = True
		check_OAI_UE = False
		pStatus = self.CheckProcessExist(check_eNB, check_OAI_UE)
		if (pStatus < 0):
			self.CreateHtmlTestRow('N/A', 'KO', pStatus)
			self.CreateHtmlTabFooter(False)
			sys.exit(1)
		multi_jobs = []
		lock = Lock()
		status_queue = SimpleQueue()
		i = 0
		for device_id in self.UEDevices:
			p = Process(target = self.CheckUEStatus_common, args = (lock,device_id,status_queue,i,))
			p.daemon = True
			p.start()
			multi_jobs.append(p)
			i += 1
		for job in multi_jobs:
			job.join()
		if self.flexranCtrlInstalled and self.flexranCtrlStarted:
			self.open(self.EPCIPAddress, self.EPCUserName, self.EPCPassword)
			self.command('cd /opt/flexran_rtc', '\$', 5)
			self.command('curl http://localhost:9999/stats | jq \'.\' > log/check_status_' + self.testCase_id + '.log 2>&1', '\$', 5)
			self.command('cat log/check_status_' + self.testCase_id + '.log | jq \'.eNB_config[0].UE\' | grep -c rnti | sed -e "s#^#Nb Connected UE = #"', '\$', 5)
			result = re.search('Nb Connected UE = (?P<nb_ues>[0-9]+)', str(self.ssh.before))
			passStatus = True
			if result is not None:
				nb_ues = int(result.group('nb_ues'))
				htmlOptions = 'Nb Connected UE(s) to eNB = ' + str(nb_ues)
				logging.debug('\u001B[1;37;44m ' + htmlOptions + ' \u001B[0m')
				if self.expectedNbOfConnectedUEs > -1:
					if nb_ues != self.expectedNbOfConnectedUEs:
						passStatus = False
			else:
				htmlOptions = 'N/A'
			self.close()
		else:
			passStatus = True
			htmlOptions = 'N/A'

		if (status_queue.empty()):
			self.CreateHtmlTestRow(htmlOptions, 'KO', ALL_PROCESSES_OK)
			self.AutoTerminateUEandeNB()
		else:
			check_status = True
			html_queue = SimpleQueue()
			while (not status_queue.empty()):
				count = status_queue.get()
				if (count < 0):
					check_status = False
				device_id = status_queue.get()
				message = status_queue.get()
				html_cell = '<pre style="background-color:white">UE (' + device_id + ')\n' + message + '</pre>'
				html_queue.put(html_cell)
			if check_status and passStatus:
				self.CreateHtmlTestRowQueue(htmlOptions, 'OK', len(self.UEDevices), html_queue)
			else:
				self.CreateHtmlTestRowQueue(htmlOptions, 'KO', len(self.UEDevices), html_queue)
				self.AutoTerminateUEandeNB()

	def GetAllUEIPAddresses(self):
		if self.ADBIPAddress == '' or self.ADBUserName == '' or self.ADBPassword == '':
			Usage()
			sys.exit('Insufficient Parameter')
		ue_ip_status = 0
		self.UEIPAddresses = []
		if (len(self.UEDevices) == 1) and (self.UEDevices[0] == 'OAI-UE'):
			if self.UEIPAddress == '' or self.UEUserName == '' or self.UEPassword == '' or self.UESourceCodePath == '':
				Usage()
				sys.exit('Insufficient Parameter')
			self.open(self.UEIPAddress, self.UEUserName, self.UEPassword)
			self.command('ifconfig oaitun_ue1', '\$', 4)
			result = re.search('inet addr:(?P<ueipaddress>[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+)|inet (?P<ueipaddress2>[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+)', str(self.ssh.before))
			if result is not None:
				if result.group('ueipaddress') is not None:
					UE_IPAddress = result.group('ueipaddress')
				else:
					UE_IPAddress = result.group('ueipaddress2')
				logging.debug('\u001B[1mUE (' + self.UEDevices[0] + ') IP Address is ' + UE_IPAddress + '\u001B[0m')
				self.UEIPAddresses.append(UE_IPAddress)
			else:
				logging.debug('\u001B[1;37;41m UE IP Address Not Found! \u001B[0m')
				ue_ip_status -= 1
			self.close()
			return ue_ip_status
		self.open(self.ADBIPAddress, self.ADBUserName, self.ADBPassword)
		idx = 0
		for device_id in self.UEDevices:
			if self.UEDevicesStatus[idx] != UE_STATUS_ATTACHED:
				idx += 1
				continue
			count = 0
			while count < 4:
				if self.ADBCentralized:
					self.command('stdbuf -o0 adb -s ' + device_id + ' shell "ip addr show | grep rmnet"', '\$', 15)
				else:
					self.command('ssh ' + self.UEDevicesRemoteUser[idx] + '@' + self.UEDevicesRemoteServer[idx] + ' \'adb -s ' + device_id + ' shell "ip addr show | grep rmnet"\'', '\$', 60)
				result = re.search('inet (?P<ueipaddress>[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+)\/[0-9]+[0-9a-zA-Z\.\s]+', str(self.ssh.before))
				if result is None:
					logging.debug('\u001B[1;37;41m UE IP Address Not Found! \u001B[0m')
					time.sleep(1)
					count += 1
				else:
					count = 10
			if count < 9:
				ue_ip_status -= 1
				continue
			UE_IPAddress = result.group('ueipaddress')
			logging.debug('\u001B[1mUE (' + device_id + ') IP Address is ' + UE_IPAddress + '\u001B[0m')
			for ueipaddress in self.UEIPAddresses:
				if ueipaddress == UE_IPAddress:
					logging.debug('\u001B[1mUE (' + device_id + ') IP Address ' + UE_IPAddress + ': has already been allocated to another device !' + '\u001B[0m')
					ue_ip_status -= 1
					continue
			self.UEIPAddresses.append(UE_IPAddress)
			idx += 1
		self.close()
		return ue_ip_status

	def ping_iperf_wrong_exit(self, lock, UE_IPAddress, device_id, statusQueue, message):
		lock.acquire()
		statusQueue.put(-1)
		statusQueue.put(device_id)
		statusQueue.put(UE_IPAddress)
		statusQueue.put(message)
		lock.release()

	def Ping_common(self, lock, UE_IPAddress, device_id, statusQueue):
		try:
			self.open(self.EPCIPAddress, self.EPCUserName, self.EPCPassword)
			self.command('cd ' + self.EPCSourceCodePath, '\$', 5)
			self.command('cd scripts', '\$', 5)
			ping_time = re.findall("-c (\d+)",str(self.ping_args))
			ping_status = self.command('stdbuf -o0 ping ' + self.ping_args + ' ' + UE_IPAddress + ' 2>&1 | stdbuf -o0 tee ping_' + self.testCase_id + '_' + device_id + '.log', '\$', int(ping_time[0])*1.5)
			# TIMEOUT CASE
			if ping_status < 0:
				message = 'Ping with UE (' + str(UE_IPAddress) + ') crashed due to TIMEOUT!'
				logging.debug('\u001B[1;37;41m ' + message + ' \u001B[0m')
				self.close()
				self.ping_iperf_wrong_exit(lock, UE_IPAddress, device_id, statusQueue, message)
				return
			result = re.search(', (?P<packetloss>[0-9\.]+)% packet loss, time [0-9\.]+ms', str(self.ssh.before))
			if result is None:
				message = 'Packet Loss Not Found!'
				logging.debug('\u001B[1;37;41m ' + message + ' \u001B[0m')
				self.close()
				self.ping_iperf_wrong_exit(lock, UE_IPAddress, device_id, statusQueue, message)
				return
			packetloss = result.group('packetloss')
			if float(packetloss) == 100:
				message = 'Packet Loss is 100%'
				logging.debug('\u001B[1;37;41m ' + message + ' \u001B[0m')
				self.close()
				self.ping_iperf_wrong_exit(lock, UE_IPAddress, device_id, statusQueue, message)
				return
			result = re.search('rtt min\/avg\/max\/mdev = (?P<rtt_min>[0-9\.]+)\/(?P<rtt_avg>[0-9\.]+)\/(?P<rtt_max>[0-9\.]+)\/[0-9\.]+ ms', str(self.ssh.before))
			if result is None:
				message = 'Ping RTT_Min RTT_Avg RTT_Max Not Found!'
				logging.debug('\u001B[1;37;41m ' + message + ' \u001B[0m')
				self.close()
				self.ping_iperf_wrong_exit(lock, UE_IPAddress, device_id, statusQueue, message)
				return
			rtt_min = result.group('rtt_min')
			rtt_avg = result.group('rtt_avg')
			rtt_max = result.group('rtt_max')
			pal_msg = 'Packet Loss : ' + packetloss + '%'
			min_msg = 'RTT(Min)    : ' + rtt_min + ' ms'
			avg_msg = 'RTT(Avg)    : ' + rtt_avg + ' ms'
			max_msg = 'RTT(Max)    : ' + rtt_max + ' ms'
			lock.acquire()
			logging.debug('\u001B[1;37;44m ping result (' + UE_IPAddress + ') \u001B[0m')
			logging.debug('\u001B[1;34m    ' + pal_msg + '\u001B[0m')
			logging.debug('\u001B[1;34m    ' + min_msg + '\u001B[0m')
			logging.debug('\u001B[1;34m    ' + avg_msg + '\u001B[0m')
			logging.debug('\u001B[1;34m    ' + max_msg + '\u001B[0m')
			qMsg = pal_msg + '\n' + min_msg + '\n' + avg_msg + '\n' + max_msg
			packetLossOK = True
			if packetloss is not None:
				if float(packetloss) > float(self.ping_packetloss_threshold):
					qMsg += '\nPacket Loss too high'
					logging.debug('\u001B[1;37;41m Packet Loss too high \u001B[0m')
					packetLossOK = False
				elif float(packetloss) > 0:
					qMsg += '\nPacket Loss is not 0%'
					logging.debug('\u001B[1;30;43m Packet Loss is not 0% \u001B[0m')
			if (packetLossOK):
				statusQueue.put(0)
			else:
				statusQueue.put(-1)
			statusQueue.put(device_id)
			statusQueue.put(UE_IPAddress)
			statusQueue.put(qMsg)
			lock.release()
			self.close()
		except:
			os.kill(os.getppid(),signal.SIGUSR1)

	def PingNoS1_wrong_exit(self, qMsg):
		html_queue = SimpleQueue()
		html_cell = '<pre style="background-color:white">OAI UE ping result\n' + qMsg + '</pre>'
		html_queue.put(html_cell)
		self.CreateHtmlTestRowQueue(self.ping_args, 'KO', len(self.UEDevices), html_queue)

	def PingNoS1(self):
		check_eNB = True
		check_OAI_UE = True
		pStatus = self.CheckProcessExist(check_eNB, check_OAI_UE)
		if (pStatus < 0):
			self.CreateHtmlTestRow(self.ping_args, 'KO', pStatus)
			self.AutoTerminateUEandeNB()
			return
		ping_from_eNB = re.search('oaitun_enb1', str(self.ping_args))
		if ping_from_eNB is not None:
			if self.eNBIPAddress == '' or self.eNBUserName == '' or self.eNBPassword == '':
				Usage()
				sys.exit('Insufficient Parameter')
		else:
			if self.UEIPAddress == '' or self.UEUserName == '' or self.UEPassword == '':
				Usage()
				sys.exit('Insufficient Parameter')
		try:
			if ping_from_eNB is not None:
				self.open(self.eNBIPAddress, self.eNBUserName, self.eNBPassword)
				self.command('cd ' + self.eNBSourceCodePath + '/cmake_targets/', '\$', 5)
			else:
				self.open(self.UEIPAddress, self.UEUserName, self.UEPassword)
				self.command('cd ' + self.UESourceCodePath + '/cmake_targets/', '\$', 5)
			self.command('cd cmake_targets', '\$', 5)
			ping_time = re.findall("-c (\d+)",str(self.ping_args))
			ping_status = self.command('stdbuf -o0 ping ' + self.ping_args + ' 2>&1 | stdbuf -o0 tee ping_' + self.testCase_id + '.log', '\$', int(ping_time[0])*1.5)
			# TIMEOUT CASE
			if ping_status < 0:
				message = 'Ping with OAI UE crashed due to TIMEOUT!'
				logging.debug('\u001B[1;37;41m ' + message + ' \u001B[0m')
				self.PingNoS1_wrong_exit(message)
				return
			result = re.search(', (?P<packetloss>[0-9\.]+)% packet loss, time [0-9\.]+ms', str(self.ssh.before))
			if result is None:
				message = 'Packet Loss Not Found!'
				logging.debug('\u001B[1;37;41m ' + message + ' \u001B[0m')
				self.PingNoS1_wrong_exit(message)
				return
			packetloss = result.group('packetloss')
			if float(packetloss) == 100:
				message = 'Packet Loss is 100%'
				logging.debug('\u001B[1;37;41m ' + message + ' \u001B[0m')
				self.PingNoS1_wrong_exit(message)
				return
			result = re.search('rtt min\/avg\/max\/mdev = (?P<rtt_min>[0-9\.]+)\/(?P<rtt_avg>[0-9\.]+)\/(?P<rtt_max>[0-9\.]+)\/[0-9\.]+ ms', str(self.ssh.before))
			if result is None:
				message = 'Ping RTT_Min RTT_Avg RTT_Max Not Found!'
				logging.debug('\u001B[1;37;41m ' + message + ' \u001B[0m')
				self.PingNoS1_wrong_exit(message)
				return
			rtt_min = result.group('rtt_min')
			rtt_avg = result.group('rtt_avg')
			rtt_max = result.group('rtt_max')
			pal_msg = 'Packet Loss : ' + packetloss + '%'
			min_msg = 'RTT(Min)    : ' + rtt_min + ' ms'
			avg_msg = 'RTT(Avg)    : ' + rtt_avg + ' ms'
			max_msg = 'RTT(Max)    : ' + rtt_max + ' ms'
			logging.debug('\u001B[1;37;44m OAI UE ping result \u001B[0m')
			logging.debug('\u001B[1;34m    ' + pal_msg + '\u001B[0m')
			logging.debug('\u001B[1;34m    ' + min_msg + '\u001B[0m')
			logging.debug('\u001B[1;34m    ' + avg_msg + '\u001B[0m')
			logging.debug('\u001B[1;34m    ' + max_msg + '\u001B[0m')
			qMsg = pal_msg + '\n' + min_msg + '\n' + avg_msg + '\n' + max_msg
			packetLossOK = True
			if packetloss is not None:
				if float(packetloss) > float(self.ping_packetloss_threshold):
					qMsg += '\nPacket Loss too high'
					logging.debug('\u001B[1;37;41m Packet Loss too high \u001B[0m')
					packetLossOK = False
				elif float(packetloss) > 0:
					qMsg += '\nPacket Loss is not 0%'
					logging.debug('\u001B[1;30;43m Packet Loss is not 0% \u001B[0m')
			self.close()
			html_queue = SimpleQueue()
			ip_addr = 'TBD'
			html_cell = '<pre style="background-color:white">OAI UE ping result\n' + qMsg + '</pre>'
			html_queue.put(html_cell)
			if packetLossOK:
				self.CreateHtmlTestRowQueue(self.ping_args, 'OK', len(self.UEDevices), html_queue)
			else:
				self.CreateHtmlTestRowQueue(self.ping_args, 'KO', len(self.UEDevices), html_queue)

			# copying on the EPC server for logCollection
			if ping_from_eNB is not None:
				copyin_res = self.copyin(self.eNBIPAddress, self.eNBUserName, self.eNBPassword, self.eNBSourceCodePath + '/cmake_targets/ping_' + self.testCase_id + '.log', '.')
			else:
				copyin_res = self.copyin(self.UEIPAddress, self.UEUserName, self.UEPassword, self.UESourceCodePath + '/cmake_targets/ping_' + self.testCase_id + '.log', '.')
			if (copyin_res == 0):
				self.copyout(self.EPCIPAddress, self.EPCUserName, self.EPCPassword, 'ping_' + self.testCase_id + '.log', self.EPCSourceCodePath + '/scripts')
		except:
			os.kill(os.getppid(),signal.SIGUSR1)

	def Ping(self):
		result = re.search('noS1', str(self.Initialize_eNB_args))
		if result is not None:
			self.PingNoS1()
			return
		if self.EPCIPAddress == '' or self.EPCUserName == '' or self.EPCPassword == '' or self.EPCSourceCodePath == '':
			Usage()
			sys.exit('Insufficient Parameter')
		check_eNB = True
		if (len(self.UEDevices) == 1) and (self.UEDevices[0] == 'OAI-UE'):
			check_OAI_UE = True
		else:
			check_OAI_UE = False
		pStatus = self.CheckProcessExist(check_eNB, check_OAI_UE)
		if (pStatus < 0):
			self.CreateHtmlTestRow(self.ping_args, 'KO', pStatus)
			self.AutoTerminateUEandeNB()
			return
		ueIpStatus = self.GetAllUEIPAddresses()
		if (ueIpStatus < 0):
			self.CreateHtmlTestRow(self.ping_args, 'KO', UE_IP_ADDRESS_ISSUE)
			self.AutoTerminateUEandeNB()
			return
		multi_jobs = []
		i = 0
		lock = Lock()
		status_queue = SimpleQueue()
		for UE_IPAddress in self.UEIPAddresses:
			device_id = self.UEDevices[i]
			p = Process(target = self.Ping_common, args = (lock,UE_IPAddress,device_id,status_queue,))
			p.daemon = True
			p.start()
			multi_jobs.append(p)
			i = i + 1
		for job in multi_jobs:
			job.join()

		if (status_queue.empty()):
			self.CreateHtmlTestRow(self.ping_args, 'KO', ALL_PROCESSES_OK)
			self.AutoTerminateUEandeNB()
		else:
			ping_status = True
			html_queue = SimpleQueue()
			while (not status_queue.empty()):
				count = status_queue.get()
				if (count < 0):
					ping_status = False
				device_id = status_queue.get()
				ip_addr = status_queue.get()
				message = status_queue.get()
				html_cell = '<pre style="background-color:white">UE (' + device_id + ')\nIP Address  : ' + ip_addr + '\n' + message + '</pre>'
				html_queue.put(html_cell)
			if (ping_status):
				self.CreateHtmlTestRowQueue(self.ping_args, 'OK', len(self.UEDevices), html_queue)
			else:
				self.CreateHtmlTestRowQueue(self.ping_args, 'KO', len(self.UEDevices), html_queue)
				self.AutoTerminateUEandeNB()

	def Iperf_ComputeTime(self):
		result = re.search('-t (?P<iperf_time>\d+)', str(self.iperf_args))
		if result is None:
			logging.debug('\u001B[1;37;41m Iperf time Not Found! \u001B[0m')
			sys.exit(1)
		return result.group('iperf_time')

	def Iperf_ComputeModifiedBW(self, idx, ue_num):
		result = re.search('-b (?P<iperf_bandwidth>[0-9\.]+)[KMG]', str(self.iperf_args))
		if result is None:
			logging.debug('\u001B[1;37;41m Iperf bandwidth Not Found! \u001B[0m')
			sys.exit(1)
		iperf_bandwidth = result.group('iperf_bandwidth')
		if self.iperf_profile == 'balanced':
			iperf_bandwidth_new = float(iperf_bandwidth)/ue_num
		if self.iperf_profile == 'single-ue':
			iperf_bandwidth_new = float(iperf_bandwidth)
		if self.iperf_profile == 'unbalanced':
			# residual is 2% of max bw
			residualBW = float(iperf_bandwidth) / 50
			if idx == 0:
				iperf_bandwidth_new = float(iperf_bandwidth) - ((ue_num - 1) * residualBW)
			else:
				iperf_bandwidth_new = residualBW
		iperf_bandwidth_str = '-b ' + iperf_bandwidth
		iperf_bandwidth_str_new = '-b ' + ('%.2f' % iperf_bandwidth_new)
		result = re.sub(iperf_bandwidth_str, iperf_bandwidth_str_new, str(self.iperf_args))
		if result is None:
			logging.debug('\u001B[1;37;41m Calculate Iperf bandwidth Failed! \u001B[0m')
			sys.exit(1)
		return result

	def Iperf_analyzeV2TCPOutput(self, lock, UE_IPAddress, device_id, statusQueue, iperf_real_options):
		self.command('awk -f /tmp/tcp_iperf_stats.awk /tmp/CI-eNB/scripts/iperf_' + self.testCase_id + '_' + device_id + '.log', '\$', 5)
		result = re.search('Avg Bitrate : (?P<average>[0-9\.]+ Mbits\/sec) Max Bitrate : (?P<maximum>[0-9\.]+ Mbits\/sec) Min Bitrate : (?P<minimum>[0-9\.]+ Mbits\/sec)', str(self.ssh.before))
		if result is not None:
			avgbitrate = result.group('average')
			maxbitrate = result.group('maximum')
			minbitrate = result.group('minimum')
			lock.acquire()
			logging.debug('\u001B[1;37;44m TCP iperf result (' + UE_IPAddress + ') \u001B[0m')
			msg = 'TCP Stats   :\n'
			if avgbitrate is not None:
				logging.debug('\u001B[1;34m    Avg Bitrate : ' + avgbitrate + '\u001B[0m')
				msg += 'Avg Bitrate : ' + avgbitrate + '\n'
			if maxbitrate is not None:
				logging.debug('\u001B[1;34m    Max Bitrate : ' + maxbitrate + '\u001B[0m')
				msg += 'Max Bitrate : ' + maxbitrate + '\n'
			if minbitrate is not None:
				logging.debug('\u001B[1;34m    Min Bitrate : ' + minbitrate + '\u001B[0m')
				msg += 'Min Bitrate : ' + minbitrate + '\n'
			statusQueue.put(0)
			statusQueue.put(device_id)
			statusQueue.put(UE_IPAddress)
			statusQueue.put(msg)
			lock.release()
		return 0

	def Iperf_analyzeV2Output(self, lock, UE_IPAddress, device_id, statusQueue, iperf_real_options):
		result = re.search('-u', str(iperf_real_options))
		if result is None:
			return self.Iperf_analyzeV2TCPOutput(lock, UE_IPAddress, device_id, statusQueue, iperf_real_options)

		result = re.search('Server Report:', str(self.ssh.before))
		if result is None:
			result = re.search('read failed: Connection refused', str(self.ssh.before))
			if result is not None:
				logging.debug('\u001B[1;37;41m Could not connect to iperf server! \u001B[0m')
			else:
				logging.debug('\u001B[1;37;41m Server Report and Connection refused Not Found! \u001B[0m')
			return -1
		# Computing the requested bandwidth in float
		result = re.search('-b (?P<iperf_bandwidth>[0-9\.]+)[KMG]', str(iperf_real_options))
		if result is not None:
			req_bandwidth = result.group('iperf_bandwidth')
			req_bw = float(req_bandwidth)
			result = re.search('-b [0-9\.]+K', str(iperf_real_options))
			if result is not None:
				req_bandwidth = '%.1f Kbits/sec' % req_bw
				req_bw = req_bw * 1000
			result = re.search('-b [0-9\.]+M', str(iperf_real_options))
			if result is not None:
				req_bandwidth = '%.1f Mbits/sec' % req_bw
				req_bw = req_bw * 1000000
			result = re.search('-b [0-9\.]+G', str(iperf_real_options))
			if result is not None:
				req_bandwidth = '%.1f Gbits/sec' % req_bw
				req_bw = req_bw * 1000000000

		result = re.search('Server Report:\\\\r\\\\n(?:|\[ *\d+\].*) (?P<bitrate>[0-9\.]+ [KMG]bits\/sec) +(?P<jitter>[0-9\.]+ ms) +(\d+\/..\d+) (\((?P<packetloss>[0-9\.]+)%\))', str(self.ssh.before))
		if result is not None:
			bitrate = result.group('bitrate')
			packetloss = result.group('packetloss')
			jitter = result.group('jitter')
			lock.acquire()
			logging.debug('\u001B[1;37;44m iperf result (' + UE_IPAddress + ') \u001B[0m')
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
					result = re.search('[0-9\.]+ G', bitrate)
					if result is not None:
						actual_bw = actual_bw * 1000000000
					br_loss = 100 * actual_bw / req_bw
					bitperf = '%.2f ' % br_loss
					msg += 'Bitrate Perf: ' + bitperf + '%\n'
					logging.debug('\u001B[1;34m    Bitrate Perf: ' + bitperf + '%\u001B[0m')
			if packetloss is not None:
				msg += 'Packet Loss : ' + packetloss + '%\n'
				logging.debug('\u001B[1;34m    Packet Loss : ' + packetloss + '%\u001B[0m')
				if float(packetloss) > float(self.iperf_packetloss_threshold):
					msg += 'Packet Loss too high!\n'
					logging.debug('\u001B[1;37;41m Packet Loss too high \u001B[0m')
					iperfStatus = False
			if jitter is not None:
				msg += 'Jitter      : ' + jitter + '\n'
				logging.debug('\u001B[1;34m    Jitter      : ' + jitter + '\u001B[0m')
			if (iperfStatus):
				statusQueue.put(0)
			else:
				statusQueue.put(-1)
			statusQueue.put(device_id)
			statusQueue.put(UE_IPAddress)
			statusQueue.put(msg)
			lock.release()
			return 0

	def Iperf_analyzeV2Server(self, lock, UE_IPAddress, device_id, statusQueue, iperf_real_options):
		if (not os.path.isfile('iperf_server_' + self.testCase_id + '_' + device_id + '.log')):
			self.ping_iperf_wrong_exit(lock, UE_IPAddress, device_id, statusQueue, 'Could not analyze from server log')
			return
		# Computing the requested bandwidth in float
		result = re.search('-b (?P<iperf_bandwidth>[0-9\.]+)[KMG]', str(iperf_real_options))
		if result is None:
			logging.debug('Iperf bandwidth Not Found!')
			self.ping_iperf_wrong_exit(lock, UE_IPAddress, device_id, statusQueue, 'Could not compute Iperf bandwidth!')
			return
		else:
			req_bandwidth = result.group('iperf_bandwidth')
			req_bw = float(req_bandwidth)
			result = re.search('-b [0-9\.]+K', str(iperf_real_options))
			if result is not None:
				req_bandwidth = '%.1f Kbits/sec' % req_bw
				req_bw = req_bw * 1000
			result = re.search('-b [0-9\.]+M', str(iperf_real_options))
			if result is not None:
				req_bandwidth = '%.1f Mbits/sec' % req_bw
				req_bw = req_bw * 1000000
			result = re.search('-b [0-9\.]+G', str(iperf_real_options))
			if result is not None:
				req_bandwidth = '%.1f Gbits/sec' % req_bw
				req_bw = req_bw * 1000000000

		server_file = open('iperf_server_' + self.testCase_id + '_' + device_id + '.log', 'r')
		br_sum = 0.0
		ji_sum = 0.0
		pl_sum = 0
		ps_sum = 0
		row_idx = 0
		for line in server_file.readlines():
			result = re.search('(?P<bitrate>[0-9\.]+ [KMG]bits\/sec) +(?P<jitter>[0-9\.]+ ms) +(?P<lostPack>[0-9]+)/ +(?P<sentPack>[0-9]+)', str(line))
			if result is not None:
				bitrate = result.group('bitrate')
				jitter = result.group('jitter')
				packetlost = result.group('lostPack')
				packetsent = result.group('sentPack')
				br = bitrate.split(' ')
				ji = jitter.split(' ')
				row_idx = row_idx + 1
				curr_br = float(br[0])
				pl_sum = pl_sum + int(packetlost)
				ps_sum = ps_sum + int(packetsent)
				if (br[1] == 'Kbits/sec'):
					curr_br = curr_br * 1000
				if (br[1] == 'Mbits/sec'):
					curr_br = curr_br * 1000 * 1000
				br_sum = curr_br + br_sum
				ji_sum = float(ji[0]) + ji_sum
		if (row_idx > 0):
			br_sum = br_sum / row_idx
			ji_sum = ji_sum / row_idx
			br_loss = 100 * br_sum / req_bw
			if (br_sum > 1000):
				br_sum = br_sum / 1000
				if (br_sum > 1000):
					br_sum = br_sum / 1000
					bitrate = '%.2f Mbits/sec' % br_sum
				else:
					bitrate = '%.2f Kbits/sec' % br_sum
			else:
				bitrate = '%.2f bits/sec' % br_sum
			bitperf = '%.2f ' % br_loss
			bitperf += '%'
			jitter = '%.2f ms' % (ji_sum)
			if (ps_sum > 0):
				pl = float(100 * pl_sum / ps_sum)
				packetloss = '%2.1f ' % (pl)
				packetloss += '%'
			else:
				packetloss = 'unknown'
			lock.acquire()
			if (br_loss < 90):
				statusQueue.put(1)
			else:
				statusQueue.put(0)
			statusQueue.put(device_id)
			statusQueue.put(UE_IPAddress)
			req_msg = 'Req Bitrate : ' + req_bandwidth
			bir_msg = 'Bitrate     : ' + bitrate
			brl_msg = 'Bitrate Perf: ' + bitperf
			jit_msg = 'Jitter      : ' + jitter
			pal_msg = 'Packet Loss : ' + packetloss
			statusQueue.put(req_msg + '\n' + bir_msg + '\n' + brl_msg + '\n' + jit_msg + '\n' + pal_msg + '\n')
			logging.debug('\u001B[1;37;45m iperf result (' + UE_IPAddress + ') \u001B[0m')
			logging.debug('\u001B[1;35m    ' + req_msg + '\u001B[0m')
			logging.debug('\u001B[1;35m    ' + bir_msg + '\u001B[0m')
			logging.debug('\u001B[1;35m    ' + brl_msg + '\u001B[0m')
			logging.debug('\u001B[1;35m    ' + jit_msg + '\u001B[0m')
			logging.debug('\u001B[1;35m    ' + pal_msg + '\u001B[0m')
			lock.release()
		else:
			self.ping_iperf_wrong_exit(lock, UE_IPAddress, device_id, statusQueue, 'Could not analyze from server log')

		server_file.close()


	def Iperf_analyzeV3Output(self, lock, UE_IPAddress, device_id, statusQueue):
		result = re.search('(?P<bitrate>[0-9\.]+ [KMG]bits\/sec) +(?:|[0-9\.]+ ms +\d+\/\d+ \((?P<packetloss>[0-9\.]+)%\)) +(?:|receiver)\\\\r\\\\n(?:|\[ *\d+\] Sent \d+ datagrams)\\\\r\\\\niperf Done\.', str(self.ssh.before))
		if result is None:
			result = re.search('(?P<error>iperf: error - [a-zA-Z0-9 :]+)', str(self.ssh.before))
			lock.acquire()
			statusQueue.put(-1)
			statusQueue.put(device_id)
			statusQueue.put(UE_IPAddress)
			if result is not None:
				logging.debug('\u001B[1;37;41m ' + result.group('error') + ' \u001B[0m')
				statusQueue.put(result.group('error'))
			else:
				logging.debug('\u001B[1;37;41m Bitrate and/or Packet Loss Not Found! \u001B[0m')
				statusQueue.put('Bitrate and/or Packet Loss Not Found!')
			lock.release()

		bitrate = result.group('bitrate')
		packetloss = result.group('packetloss')
		lock.acquire()
		logging.debug('\u001B[1;37;44m iperf result (' + UE_IPAddress + ') \u001B[0m')
		logging.debug('\u001B[1;34m    Bitrate     : ' + bitrate + '\u001B[0m')
		msg = 'Bitrate     : ' + bitrate + '\n'
		iperfStatus = True
		if packetloss is not None:
			logging.debug('\u001B[1;34m    Packet Loss : ' + packetloss + '%\u001B[0m')
			msg += 'Packet Loss : ' + packetloss + '%\n'
			if float(packetloss) > float(self.iperf_packetloss_threshold):
				logging.debug('\u001B[1;37;41m Packet Loss too high \u001B[0m')
				msg += 'Packet Loss too high!\n'
				iperfStatus = False
		if (iperfStatus):
			statusQueue.put(0)
		else:
			statusQueue.put(-1)
		statusQueue.put(device_id)
		statusQueue.put(UE_IPAddress)
		statusQueue.put(msg)
		lock.release()

	def Iperf_UL_common(self, lock, UE_IPAddress, device_id, idx, ue_num, statusQueue):
		udpIperf = True
		result = re.search('-u', str(self.iperf_args))
		if result is None:
			udpIperf = False
		ipnumbers = UE_IPAddress.split('.')
		if (len(ipnumbers) == 4):
			ipnumbers[3] = '1'
		EPC_Iperf_UE_IPAddress = ipnumbers[0] + '.' + ipnumbers[1] + '.' + ipnumbers[2] + '.' + ipnumbers[3]

		# Launch iperf server on EPC side (true for ltebox and old open-air-cn0
		# But for OAI-Rel14-CUPS, we launch from python executor and we are using its IP address as iperf client address
		launchFromEpc = True
		if re.match('OAI-Rel14-CUPS', self.EPCType, re.IGNORECASE):
			launchFromEpc = False
			cmd = 'hostname -I'
			ret = subprocess.run(cmd, shell=True, stdout=subprocess.PIPE, encoding='utf-8')
			if ret.stdout is not None:
				EPC_Iperf_UE_IPAddress = ret.stdout.strip()
		port = 5001 + idx
		if launchFromEpc:
			self.open(self.EPCIPAddress, self.EPCUserName, self.EPCPassword)
			self.command('cd ' + self.EPCSourceCodePath + '/scripts', '\$', 5)
			self.command('rm -f iperf_server_' + self.testCase_id + '_' + device_id + '.log', '\$', 5)
			if udpIperf:
				self.command('echo $USER; nohup iperf -u -s -i 1 -p ' + str(port) + ' > iperf_server_' + self.testCase_id + '_' + device_id + '.log &', self.EPCUserName, 5)
			else:
				self.command('echo $USER; nohup iperf -s -i 1 -p ' + str(port) + ' > iperf_server_' + self.testCase_id + '_' + device_id + '.log &', self.EPCUserName, 5)
			self.close()
		else:
			if udpIperf:
				cmd = 'nohup iperf -u -s -i 1 -p ' + str(port) + ' > iperf_server_' + self.testCase_id + '_' + device_id + '.log 2>&1 &'
			else:
				cmd = 'nohup iperf -s -i 1 -p ' + str(port) + ' > iperf_server_' + self.testCase_id + '_' + device_id + '.log 2>&1 &'
			logging.debug(cmd + '\n')
			subprocess.run(cmd, shell=True, stdout=subprocess.PIPE, encoding='utf-8')
		time.sleep(0.5)

		# Launch iperf client on UE
		if (device_id == 'OAI-UE'):
			self.open(self.UEIPAddress, self.UEUserName, self.UEPassword)
			self.command('cd ' + self.UESourceCodePath + '/cmake_targets', '\$', 5)
		else:
			self.open(self.ADBIPAddress, self.ADBUserName, self.ADBPassword)
			self.command('cd ' + self.EPCSourceCodePath + '/scripts', '\$', 5)
		iperf_time = self.Iperf_ComputeTime()
		time.sleep(0.5)

		if udpIperf:
			modified_options = self.Iperf_ComputeModifiedBW(idx, ue_num)
		else:
			modified_options = str(self.iperf_args)
		modified_options = modified_options.replace('-R','')
		time.sleep(0.5)

		self.command('rm -f iperf_' + self.testCase_id + '_' + device_id + '.log', '\$', 5)
		if (device_id == 'OAI-UE'):
			iperf_status = self.command('iperf -c ' + EPC_Iperf_UE_IPAddress + ' ' + modified_options + ' -p ' + str(port) + ' -B ' + UE_IPAddress + ' 2>&1 | stdbuf -o0 tee iperf_' + self.testCase_id + '_' + device_id + '.log', '\$', int(iperf_time)*5.0)
		else:
			iperf_status = self.command('stdbuf -o0 adb -s ' + device_id + ' shell "/data/local/tmp/iperf -c ' + EPC_Iperf_UE_IPAddress + ' ' + modified_options + ' -p ' + str(port) + '" 2>&1 | stdbuf -o0 tee iperf_' + self.testCase_id + '_' + device_id + '.log', '\$', int(iperf_time)*5.0)
		# TIMEOUT Case
		if iperf_status < 0:
			self.close()
			message = 'iperf on UE (' + str(UE_IPAddress) + ') crashed due to TIMEOUT !'
			logging.debug('\u001B[1;37;41m ' + message + ' \u001B[0m')
			self.close()
			self.ping_iperf_wrong_exit(lock, UE_IPAddress, device_id, statusQueue, message)
			return
		clientStatus = self.Iperf_analyzeV2Output(lock, UE_IPAddress, device_id, statusQueue, modified_options)
		self.close()

		# Kill iperf server on EPC side
		if launchFromEpc:
			self.open(self.EPCIPAddress, self.EPCUserName, self.EPCPassword)
			self.command('killall --signal SIGKILL iperf', self.EPCUserName, 5)
			self.close()
		else:
			cmd = 'killall --signal SIGKILL iperf'
			logging.debug(cmd + '\n')
			subprocess.run(cmd, shell=True)
			time.sleep(1)
			self.copyout(self.EPCIPAddress, self.EPCUserName, self.EPCPassword, 'iperf_server_' + self.testCase_id + '_' + device_id + '.log', self.EPCSourceCodePath + '/scripts')
		# in case of failure, retrieve server log
		if (clientStatus == -1):
			if launchFromEpc:
				time.sleep(1)
				if (os.path.isfile('iperf_server_' + self.testCase_id + '_' + device_id + '.log')):
					os.remove('iperf_server_' + self.testCase_id + '_' + device_id + '.log')
				self.copyin(self.EPCIPAddress, self.EPCUserName, self.EPCPassword, self.EPCSourceCodePath + '/scripts/iperf_server_' + self.testCase_id + '_' + device_id + '.log', '.')
			self.Iperf_analyzeV2Server(lock, UE_IPAddress, device_id, statusQueue, modified_options)
		# in case of OAI-UE 
		if (device_id == 'OAI-UE'):
			self.copyin(self.UEIPAddress, self.UEUserName, self.UEPassword, self.UESourceCodePath + '/cmake_targets/iperf_' + self.testCase_id + '_' + device_id + '.log', '.')
			self.copyout(self.EPCIPAddress, self.EPCUserName, self.EPCPassword, 'iperf_' + self.testCase_id + '_' + device_id + '.log', self.EPCSourceCodePath + '/scripts')

	def Iperf_common(self, lock, UE_IPAddress, device_id, idx, ue_num, statusQueue):
		try:
			# Single-UE profile -- iperf only on one UE
			if self.iperf_profile == 'single-ue' and idx != 0:
				return
			useIperf3 = False
			udpIperf = True
			if (device_id != 'OAI-UE'):
				self.open(self.ADBIPAddress, self.ADBUserName, self.ADBPassword)
				# if by chance ADB server and EPC are on the same remote host, at least log collection will take care of it
				self.command('if [ ! -d ' + self.EPCSourceCodePath + '/scripts ]; then mkdir -p ' + self.EPCSourceCodePath + '/scripts ; fi', '\$', 5)
				self.command('cd ' + self.EPCSourceCodePath + '/scripts', '\$', 5)
				# Checking if iperf / iperf3 are installed
				if self.ADBCentralized:
					self.command('adb -s ' + device_id + ' shell "ls /data/local/tmp"', '\$', 5)
				else:
					self.command('ssh ' + self.UEDevicesRemoteUser[idx] + '@' + self.UEDevicesRemoteServer[idx] + ' \'adb -s ' + device_id + ' shell "ls /data/local/tmp"\'', '\$', 60)
				result = re.search('iperf3', str(self.ssh.before))
				if result is None:
					result = re.search('iperf', str(self.ssh.before))
					if result is None:
						message = 'Neither iperf nor iperf3 installed on UE!'
						logging.debug('\u001B[1;37;41m ' + message + ' \u001B[0m')
						self.close()
						self.ping_iperf_wrong_exit(lock, UE_IPAddress, device_id, statusQueue, message)
						return
				else:
					useIperf3 = True
				self.close()
			# in case of iperf, UL has its own function
			if (not useIperf3):
				result = re.search('-R', str(self.iperf_args))
				if result is not None:
					self.Iperf_UL_common(lock, UE_IPAddress, device_id, idx, ue_num, statusQueue)
					return

			# Launch the IPERF server on the UE side for DL
			if (device_id == 'OAI-UE'):
				self.open(self.UEIPAddress, self.UEUserName, self.UEPassword)
				self.command('cd ' + self.UESourceCodePath + '/cmake_targets', '\$', 5)
				self.command('rm -f iperf_server_' + self.testCase_id + '_' + device_id + '.log', '\$', 5)
				result = re.search('-u', str(self.iperf_args))
				if result is None:
					self.command('echo $USER; nohup iperf -B ' + UE_IPAddress + ' -s -i 1 > iperf_server_' + self.testCase_id + '_' + device_id + '.log &', self.UEUserName, 5)
					udpIperf = False
				else:
					self.command('echo $USER; nohup iperf -B ' + UE_IPAddress + ' -u -s -i 1 > iperf_server_' + self.testCase_id + '_' + device_id + '.log &', self.UEUserName, 5)
			else:
				self.open(self.ADBIPAddress, self.ADBUserName, self.ADBPassword)
				self.command('cd ' + self.EPCSourceCodePath + '/scripts', '\$', 5)
				if self.ADBCentralized:
					if (useIperf3):
						self.command('stdbuf -o0 adb -s ' + device_id + ' shell /data/local/tmp/iperf3 -s &', '\$', 5)
					else:
						self.command('rm -f iperf_server_' + self.testCase_id + '_' + device_id + '.log', '\$', 5)
						result = re.search('-u', str(self.iperf_args))
						if result is None:
							self.command('echo $USER; nohup adb -s ' + device_id + ' shell "/data/local/tmp/iperf -s -i 1" > iperf_server_' + self.testCase_id + '_' + device_id + '.log &', self.ADBUserName, 5)
							udpIperf = False
						else:
							self.command('echo $USER; nohup adb -s ' + device_id + ' shell "/data/local/tmp/iperf -u -s -i 1" > iperf_server_' + self.testCase_id + '_' + device_id + '.log &', self.ADBUserName, 5)
				else:
					self.command('rm -f iperf_server_' + self.testCase_id + '_' + device_id + '.log', '\$', 5)
					self.command('echo $USER; nohup ssh ' + self.UEDevicesRemoteUser[idx] + '@' + self.UEDevicesRemoteServer[idx] + ' \'adb -s ' + device_id + ' shell "/data/local/tmp/iperf -u -s -i 1" > iperf_server_' + self.testCase_id + '_' + device_id + '.log &\' > /dev/null 2>&1', self.ADBUserName, 60)

			time.sleep(0.5)
			self.close()

			# Launch the IPERF client on the EPC side for DL (true for ltebox and old open-air-cn
			# But for OAI-Rel14-CUPS, we launch from python executor
			launchFromEpc = True
			if re.match('OAI-Rel14-CUPS', self.EPCType, re.IGNORECASE):
				launchFromEpc = False
			if launchFromEpc:
				self.open(self.EPCIPAddress, self.EPCUserName, self.EPCPassword)
				self.command('cd ' + self.EPCSourceCodePath + '/scripts', '\$', 5)
			iperf_time = self.Iperf_ComputeTime()
			time.sleep(0.5)

			if udpIperf:
				modified_options = self.Iperf_ComputeModifiedBW(idx, ue_num)
			else:
				modified_options = str(self.iperf_args)
			time.sleep(0.5)

			if launchFromEpc:
				self.command('rm -f iperf_' + self.testCase_id + '_' + device_id + '.log', '\$', 5)
			else:
				if (os.path.isfile('iperf_' + self.testCase_id + '_' + device_id + '.log')):
					os.remove('iperf_' + self.testCase_id + '_' + device_id + '.log')
			if (useIperf3):
				self.command('stdbuf -o0 iperf3 -c ' + UE_IPAddress + ' ' + modified_options + ' 2>&1 | stdbuf -o0 tee iperf_' + self.testCase_id + '_' + device_id + '.log', '\$', int(iperf_time)*5.0)

				clientStatus = 0
				self.Iperf_analyzeV3Output(lock, UE_IPAddress, device_id, statusQueue)
			else:
				if launchFromEpc:
					iperf_status = self.command('stdbuf -o0 iperf -c ' + UE_IPAddress + ' ' + modified_options + ' 2>&1 | stdbuf -o0 tee iperf_' + self.testCase_id + '_' + device_id + '.log', '\$', int(iperf_time)*5.0)
				else:
					cmd = 'iperf -c ' + UE_IPAddress + ' ' + modified_options + ' 2>&1 > iperf_' + self.testCase_id + '_' + device_id + '.log'
					message = cmd + '\n'
					logging.debug(cmd)
					ret = subprocess.run(cmd, shell=True)
					iperf_status = ret.returncode
					self.copyout(self.EPCIPAddress, self.EPCUserName, self.EPCPassword, 'iperf_' + self.testCase_id + '_' + device_id + '.log', self.EPCSourceCodePath + '/scripts')
					self.open(self.EPCIPAddress, self.EPCUserName, self.EPCPassword)
					self.command('cat ' + self.EPCSourceCodePath + '/scripts/iperf_' + self.testCase_id + '_' + device_id + '.log', '\$', 5)
				if iperf_status < 0:
					if launchFromEpc:
						self.close()
					message = 'iperf on UE (' + str(UE_IPAddress) + ') crashed due to TIMEOUT !'
					logging.debug('\u001B[1;37;41m ' + message + ' \u001B[0m')
					self.ping_iperf_wrong_exit(lock, UE_IPAddress, device_id, statusQueue, message)
					return
				clientStatus = self.Iperf_analyzeV2Output(lock, UE_IPAddress, device_id, statusQueue, modified_options)
			self.close()

			# Kill the IPERF server that runs in background
			if (device_id == 'OAI-UE'):
				self.open(self.UEIPAddress, self.UEUserName, self.UEPassword)
				self.command('killall iperf', '\$', 5)
			else:
				self.open(self.ADBIPAddress, self.ADBUserName, self.ADBPassword)
				if self.ADBCentralized:
					self.command('stdbuf -o0 adb -s ' + device_id + ' shell ps | grep --color=never iperf | grep -v grep', '\$', 5)
				else:
					self.command('ssh ' + self.UEDevicesRemoteUser[idx] + '@' + self.UEDevicesRemoteServer[idx] + ' \'adb -s ' + device_id + ' shell "ps" | grep --color=never iperf | grep -v grep\'', '\$', 60)
				result = re.search('shell +(?P<pid>\d+)', str(self.ssh.before))
				if result is not None:
					pid_iperf = result.group('pid')
					if self.ADBCentralized:
						self.command('stdbuf -o0 adb -s ' + device_id + ' shell kill -KILL ' + pid_iperf, '\$', 5)
					else:
						self.command('ssh ' + self.UEDevicesRemoteUser[idx] + '@' + self.UEDevicesRemoteServer[idx] + ' \'adb -s ' + device_id + ' shell "kill -KILL ' + pid_iperf + '"\'', '\$', 60)
			self.close()
			# if the client report is absent, try to analyze the server log file
			if (clientStatus == -1):
				time.sleep(1)
				if (os.path.isfile('iperf_server_' + self.testCase_id + '_' + device_id + '.log')):
					os.remove('iperf_server_' + self.testCase_id + '_' + device_id + '.log')
				if (device_id == 'OAI-UE'):
					self.copyin(self.UEIPAddress, self.UEUserName, self.UEPassword, self.UESourceCodePath + '/cmake_targets/iperf_server_' + self.testCase_id + '_' + device_id + '.log', '.')
				else:
					self.copyin(self.ADBIPAddress, self.ADBUserName, self.ADBPassword, self.EPCSourceCodePath + '/scripts/iperf_server_' + self.testCase_id + '_' + device_id + '.log', '.')
				self.Iperf_analyzeV2Server(lock, UE_IPAddress, device_id, statusQueue, modified_options)

			# in case of OAI UE: 
			if (device_id == 'OAI-UE'):
				if (os.path.isfile('iperf_server_' + self.testCase_id + '_' + device_id + '.log')):
					if not launchFromEpc:
						self.copyout(self.EPCIPAddress, self.EPCUserName, self.EPCPassword, 'iperf_server_' + self.testCase_id + '_' + device_id + '.log', self.EPCSourceCodePath + '/scripts')
				else:
					self.copyin(self.UEIPAddress, self.UEUserName, self.UEPassword, self.UESourceCodePath + '/cmake_targets/iperf_server_' + self.testCase_id + '_' + device_id + '.log', '.')
					self.copyout(self.EPCIPAddress, self.EPCUserName, self.EPCPassword, 'iperf_server_' + self.testCase_id + '_' + device_id + '.log', self.EPCSourceCodePath + '/scripts')
		except:
			os.kill(os.getppid(),signal.SIGUSR1)

	def IperfNoS1(self):
		if self.eNBIPAddress == '' or self.eNBUserName == '' or self.eNBPassword == '' or self.UEIPAddress == '' or self.UEUserName == '' or self.UEPassword == '':
			Usage()
			sys.exit('Insufficient Parameter')
		check_eNB = True
		check_OAI_UE = True
		pStatus = self.CheckProcessExist(check_eNB, check_OAI_UE)
		if (pStatus < 0):
			self.CreateHtmlTestRow(self.iperf_args, 'KO', pStatus)
			self.AutoTerminateUEandeNB()
			return
		server_on_enb = re.search('-R', str(self.iperf_args))
		if server_on_enb is not None:
			iServerIPAddr = self.eNBIPAddress
			iServerUser = self.eNBUserName
			iServerPasswd = self.eNBPassword
			iClientIPAddr = self.UEIPAddress
			iClientUser = self.UEUserName
			iClientPasswd = self.UEPassword
		else:
			iServerIPAddr = self.UEIPAddress
			iServerUser = self.UEUserName
			iServerPasswd = self.UEPassword
			iClientIPAddr = self.eNBIPAddress
			iClientUser = self.eNBUserName
			iClientPasswd = self.eNBPassword
		if self.iperf_options != 'sink':
			# Starting the iperf server
			self.open(iServerIPAddr, iServerUser, iServerPasswd)
			# args SHALL be "-c client -u any"
			# -c 10.0.1.2 -u -b 1M -t 30 -i 1 -fm -B 10.0.1.1
			# -B 10.0.1.1 -u -s -i 1 -fm
			server_options = re.sub('-u.*$', '-u -s -i 1 -fm', str(self.iperf_args))
			server_options = server_options.replace('-c','-B')
			self.command('rm -f /tmp/tmp_iperf_server_' + self.testCase_id + '.log', '\$', 5)
			self.command('echo $USER; nohup iperf ' + server_options + ' > /tmp/tmp_iperf_server_' + self.testCase_id + '.log 2>&1 &', iServerUser, 5)
			time.sleep(0.5)
			self.close()

		# Starting the iperf client
		modified_options = self.Iperf_ComputeModifiedBW(0, 1)
		modified_options = modified_options.replace('-R','')
		iperf_time = self.Iperf_ComputeTime()
		self.open(iClientIPAddr, iClientUser, iClientPasswd)
		self.command('rm -f /tmp/tmp_iperf_' + self.testCase_id + '.log', '\$', 5)
		iperf_status = self.command('stdbuf -o0 iperf ' + modified_options + ' 2>&1 | stdbuf -o0 tee /tmp/tmp_iperf_' + self.testCase_id + '.log', '\$', int(iperf_time)*5.0)
		status_queue = SimpleQueue()
		lock = Lock()
		if iperf_status < 0:
			message = 'iperf on OAI UE crashed due to TIMEOUT !'
			logging.debug('\u001B[1;37;41m ' + message + ' \u001B[0m')
			clientStatus = -2
		else:
			if self.iperf_options == 'sink':
				clientStatus = 0
				status_queue.put(0)
				status_queue.put('OAI-UE')
				status_queue.put('10.0.1.2')
				status_queue.put('Sink Test : no check')
			else:
				clientStatus = self.Iperf_analyzeV2Output(lock, '10.0.1.2', 'OAI-UE', status_queue, modified_options)
		self.close()

		# Stopping the iperf server
		if self.iperf_options != 'sink':
			self.open(iServerIPAddr, iServerUser, iServerPasswd)
			self.command('killall --signal SIGKILL iperf', '\$', 5)
			time.sleep(0.5)
			self.close()
		if (clientStatus == -1):
			if (os.path.isfile('iperf_server_' + self.testCase_id + '.log')):
				os.remove('iperf_server_' + self.testCase_id + '.log')
			self.copyin(iServerIPAddr, iServerUser, iServerPasswd, '/tmp/tmp_iperf_server_' + self.testCase_id + '.log', 'iperf_server_' + self.testCase_id + '_OAI-UE.log')
			self.Iperf_analyzeV2Server(lock, '10.0.1.2', 'OAI-UE', status_queue, modified_options)

		# copying on the EPC server for logCollection
		if (clientStatus == -1):
			copyin_res = self.copyin(iServerIPAddr, iServerUser, iServerPasswd, '/tmp/tmp_iperf_server_' + self.testCase_id + '.log', 'iperf_server_' + self.testCase_id + '_OAI-UE.log')
			if (copyin_res == 0):
				self.copyout(self.EPCIPAddress, self.EPCUserName, self.EPCPassword, 'iperf_server_' + self.testCase_id + '_OAI-UE.log', self.EPCSourceCodePath + '/scripts')
		copyin_res = self.copyin(iClientIPAddr, iClientUser, iClientPasswd, '/tmp/tmp_iperf_' + self.testCase_id + '.log', 'iperf_' + self.testCase_id + '_OAI-UE.log')
		if (copyin_res == 0):
			self.copyout(self.EPCIPAddress, self.EPCUserName, self.EPCPassword, 'iperf_' + self.testCase_id + '_OAI-UE.log', self.EPCSourceCodePath + '/scripts')
		iperf_noperf = False
		if status_queue.empty():
			iperf_status = False
		else:
			iperf_status = True
		html_queue = SimpleQueue()
		while (not status_queue.empty()):
			count = status_queue.get()
			if (count < 0):
				iperf_status = False
			if (count > 0):
				iperf_noperf = True
			device_id = status_queue.get()
			ip_addr = status_queue.get()
			message = status_queue.get()
			html_cell = '<pre style="background-color:white">UE (' + device_id + ')\nIP Address  : ' + ip_addr + '\n' + message + '</pre>'
			html_queue.put(html_cell)
		if (iperf_noperf and iperf_status):
			self.CreateHtmlTestRowQueue(self.iperf_args, 'PERF NOT MET', len(self.UEDevices), html_queue)
		elif (iperf_status):
			self.CreateHtmlTestRowQueue(self.iperf_args, 'OK', len(self.UEDevices), html_queue)
		else:
			self.CreateHtmlTestRowQueue(self.iperf_args, 'KO', len(self.UEDevices), html_queue)
			self.AutoTerminateUEandeNB()

	def Iperf(self):
		result = re.search('noS1', str(self.Initialize_eNB_args))
		if result is not None:
			self.IperfNoS1()
			return
		if self.EPCIPAddress == '' or self.EPCUserName == '' or self.EPCPassword == '' or self.EPCSourceCodePath == '' or self.ADBIPAddress == '' or self.ADBUserName == '' or self.ADBPassword == '':
			Usage()
			sys.exit('Insufficient Parameter')
		check_eNB = True
		if (len(self.UEDevices) == 1) and (self.UEDevices[0] == 'OAI-UE'):
			check_OAI_UE = True
		else:
			check_OAI_UE = False
		pStatus = self.CheckProcessExist(check_eNB, check_OAI_UE)
		if (pStatus < 0):
			self.CreateHtmlTestRow(self.iperf_args, 'KO', pStatus)
			self.AutoTerminateUEandeNB()
			return
		ueIpStatus = self.GetAllUEIPAddresses()
		if (ueIpStatus < 0):
			self.CreateHtmlTestRow(self.iperf_args, 'KO', UE_IP_ADDRESS_ISSUE)
			self.AutoTerminateUEandeNB()
			return
		multi_jobs = []
		i = 0
		ue_num = len(self.UEIPAddresses)
		lock = Lock()
		status_queue = SimpleQueue()
		for UE_IPAddress in self.UEIPAddresses:
			device_id = self.UEDevices[i]
			p = Process(target = SSH.Iperf_common, args = (lock,UE_IPAddress,device_id,i,ue_num,status_queue,))
			p.daemon = True
			p.start()
			multi_jobs.append(p)
			i = i + 1
		for job in multi_jobs:
			job.join()

		if (status_queue.empty()):
			self.CreateHtmlTestRow(self.iperf_args, 'KO', ALL_PROCESSES_OK)
			self.AutoTerminateUEandeNB()
		else:
			iperf_status = True
			iperf_noperf = False
			html_queue = SimpleQueue()
			while (not status_queue.empty()):
				count = status_queue.get()
				if (count < 0):
					iperf_status = False
				if (count > 0):
					iperf_noperf = True
				device_id = status_queue.get()
				ip_addr = status_queue.get()
				message = status_queue.get()
				html_cell = '<pre style="background-color:white">UE (' + device_id + ')\nIP Address  : ' + ip_addr + '\n' + message + '</pre>'
				html_queue.put(html_cell)
			if (iperf_noperf and iperf_status):
				self.CreateHtmlTestRowQueue(self.iperf_args, 'PERF NOT MET', len(self.UEDevices), html_queue)
			elif (iperf_status):
				self.CreateHtmlTestRowQueue(self.iperf_args, 'OK', len(self.UEDevices), html_queue)
			else:
				self.CreateHtmlTestRowQueue(self.iperf_args, 'KO', len(self.UEDevices), html_queue)
				self.AutoTerminateUEandeNB()

	def CheckProcessExist(self, check_eNB, check_OAI_UE):
		multi_jobs = []
		status_queue = SimpleQueue()
		# in noS1 config, no need to check status from EPC
		result = re.search('noS1', str(self.Initialize_eNB_args))
		if result is None:
			p = Process(target = SSH.CheckHSSProcess, args = (status_queue,))
			p.daemon = True
			p.start()
			multi_jobs.append(p)
			p = Process(target = SSH.CheckMMEProcess, args = (status_queue,))
			p.daemon = True
			p.start()
			multi_jobs.append(p)
			p = Process(target = SSH.CheckSPGWProcess, args = (status_queue,))
			p.daemon = True
			p.start()
			multi_jobs.append(p)
		else:
			if (check_eNB == False) and (check_OAI_UE == False):
				return 0
		if check_eNB:
			p = Process(target = SSH.CheckeNBProcess, args = (status_queue,))
			p.daemon = True
			p.start()
			multi_jobs.append(p)
		if check_OAI_UE:
			p = Process(target = SSH.CheckOAIUEProcess, args = (status_queue,))
			p.daemon = True
			p.start()
			multi_jobs.append(p)
		for job in multi_jobs:
			job.join()

		if (status_queue.empty()):
			return -15
		else:
			result = 0
			while (not status_queue.empty()):
				status = status_queue.get()
				if (status < 0):
					result = status
			if result == ENB_PROCESS_FAILED:
				fileCheck = re.search('enb_', str(self.eNBLogFiles[0]))
				if fileCheck is not None:
					self.copyin(self.eNBIPAddress, self.eNBUserName, self.eNBPassword, self.eNBSourceCodePath + '/cmake_targets/' + self.eNBLogFiles[0], '.')
					logStatus = self.AnalyzeLogFile_eNB(self.eNBLogFiles[0])
					if logStatus < 0:
						result = logStatus
					self.eNBLogFiles[0] = ''
				if self.flexranCtrlInstalled and self.flexranCtrlStarted:
					self.TerminateFlexranCtrl()
			return result

	def CheckOAIUEProcessExist(self, initialize_OAI_UE_flag):
		multi_jobs = []
		status_queue = SimpleQueue()
		if initialize_OAI_UE_flag == False:
			p = Process(target = SSH.CheckOAIUEProcess, args = (status_queue,))
			p.daemon = True
			p.start()
			multi_jobs.append(p)
		for job in multi_jobs:
			job.join()

		if (status_queue.empty()):
			return -15
		else:
			result = 0
			while (not status_queue.empty()):
				status = status_queue.get()
				if (status < 0):
					result = status
			if result == OAI_UE_PROCESS_FAILED:
				fileCheck = re.search('ue_', str(self.UELogFile))
				if fileCheck is not None:
					self.copyin(self.UEIPAddress, self.UEUserName, self.UEPassword, self.UESourceCodePath + '/cmake_targets/' + self.UELogFile, '.')
					logStatus = self.AnalyzeLogFile_UE(self.UELogFile)
					if logStatus < 0:
						result = logStatus
			return result

	def CheckOAIUEProcess(self, status_queue):
		try:
			self.open(self.UEIPAddress, self.UEUserName, self.UEPassword)
			self.command('stdbuf -o0 ps -aux | grep --color=never softmodem | grep -v grep', '\$', 5)
			result = re.search('lte-uesoftmodem', str(self.ssh.before))
			if result is None:
				logging.debug('\u001B[1;37;41m OAI UE Process Not Found! \u001B[0m')
				status_queue.put(OAI_UE_PROCESS_FAILED)
			else:
				status_queue.put(OAI_UE_PROCESS_OK)
			self.close()
		except:
			os.kill(os.getppid(),signal.SIGUSR1)

	def CheckeNBProcess(self, status_queue):
		try:
			self.open(self.eNBIPAddress, self.eNBUserName, self.eNBPassword)
			self.command('stdbuf -o0 ps -aux | grep --color=never softmodem | grep -v grep', '\$', 5)
			result = re.search('lte-softmodem', str(self.ssh.before))
			if result is None:
				logging.debug('\u001B[1;37;41m eNB Process Not Found! \u001B[0m')
				status_queue.put(ENB_PROCESS_FAILED)
			else:
				status_queue.put(ENB_PROCESS_OK)
			self.close()
		except:
			os.kill(os.getppid(),signal.SIGUSR1)

	def CheckHSSProcess(self, status_queue):
		try:
			self.open(self.EPCIPAddress, self.EPCUserName, self.EPCPassword)
			self.command('stdbuf -o0 ps -aux | grep --color=never hss | grep -v grep', '\$', 5)
			if re.match('OAI-Rel14-CUPS', self.EPCType, re.IGNORECASE):
				result = re.search('oai_hss -j', str(self.ssh.before))
			elif re.match('OAI', self.EPCType, re.IGNORECASE):
				result = re.search('\/bin\/bash .\/run_', str(self.ssh.before))
			elif re.match('ltebox', self.EPCType, re.IGNORECASE):
				result = re.search('hss_sim s6as diam_hss', str(self.ssh.before))
			else:
				logging.error('This should not happen!')
			if result is None:
				logging.debug('\u001B[1;37;41m HSS Process Not Found! \u001B[0m')
				status_queue.put(HSS_PROCESS_FAILED)
			else:
				status_queue.put(HSS_PROCESS_OK)
			self.close()
		except:
			os.kill(os.getppid(),signal.SIGUSR1)

	def CheckMMEProcess(self, status_queue):
		try:
			self.open(self.EPCIPAddress, self.EPCUserName, self.EPCPassword)
			self.command('stdbuf -o0 ps -aux | grep --color=never mme | grep -v grep', '\$', 5)
			if re.match('OAI-Rel14-CUPS', self.EPCType, re.IGNORECASE):
				result = re.search('mme -c', str(self.ssh.before))
			elif re.match('OAI', self.EPCType, re.IGNORECASE):
				result = re.search('\/bin\/bash .\/run_', str(self.ssh.before))
			elif re.match('ltebox', self.EPCType, re.IGNORECASE):
				result = re.search('mme', str(self.ssh.before))
			else:
				logging.error('This should not happen!')
			if result is None:
				logging.debug('\u001B[1;37;41m MME Process Not Found! \u001B[0m')
				status_queue.put(MME_PROCESS_FAILED)
			else:
				status_queue.put(MME_PROCESS_OK)
			self.close()
		except:
			os.kill(os.getppid(),signal.SIGUSR1)

	def CheckSPGWProcess(self, status_queue):
		try:
			self.open(self.EPCIPAddress, self.EPCUserName, self.EPCPassword)
			if re.match('OAI-Rel14-CUPS', self.EPCType, re.IGNORECASE):
				self.command('stdbuf -o0 ps -aux | grep --color=never spgw | grep -v grep', '\$', 5)
				result = re.search('spgwu -c ', str(self.ssh.before))
			elif re.match('OAI', self.EPCType, re.IGNORECASE):
				self.command('stdbuf -o0 ps -aux | grep --color=never spgw | grep -v grep', '\$', 5)
				result = re.search('\/bin\/bash .\/run_', str(self.ssh.before))
			elif re.match('ltebox', self.EPCType, re.IGNORECASE):
				self.command('stdbuf -o0 ps -aux | grep --color=never xGw | grep -v grep', '\$', 5)
				result = re.search('xGw', str(self.ssh.before))
			else:
				logging.error('This should not happen!')
			if result is None:
				logging.debug('\u001B[1;37;41m SPGW Process Not Found! \u001B[0m')
				status_queue.put(SPGW_PROCESS_FAILED)
			else:
				status_queue.put(SPGW_PROCESS_OK)
			self.close()
		except:
			os.kill(os.getppid(),signal.SIGUSR1)

	def AnalyzeLogFile_eNB(self, eNBlogFile):
		if (not os.path.isfile('./' + eNBlogFile)):
			return -1
		enb_log_file = open('./' + eNBlogFile, 'r')
		exitSignalReceived = False
		foundAssertion = False
		msgAssertion = ''
		msgLine = 0
		foundSegFault = False
		foundRealTimeIssue = False
		rrcSetupComplete = 0
		rrcReleaseRequest = 0
		rrcReconfigRequest = 0
		rrcReconfigComplete = 0
		rrcReestablishRequest = 0
		rrcReestablishComplete = 0
		rrcReestablishReject = 0
		rlcDiscardBuffer = 0
		rachCanceledProcedure = 0
		uciStatMsgCount = 0
		pdcpFailure = 0
		ulschFailure = 0
		cdrxActivationMessageCount = 0
		dropNotEnoughRBs = 0
		mbmsRequestMsg = 0
		self.htmleNBFailureMsg = ''
		isRRU = False
		isSlave = False
		slaveReceivesFrameResyncCmd = False
		X2HO_state = X2_HO_REQ_STATE__IDLE
		X2HO_inNbProcedures = 0
		X2HO_outNbProcedures = 0
		for line in enb_log_file.readlines():
			if X2HO_state == X2_HO_REQ_STATE__IDLE:
				result = re.search('target eNB Receives X2 HO Req X2AP_HANDOVER_REQ', str(line))
				if result is not None:
					X2HO_state = X2_HO_REQ_STATE__TARGET_RECEIVES_REQ
				result = re.search('source eNB receives the X2 HO ACK X2AP_HANDOVER_REQ_ACK', str(line))
				if result is not None:
					X2HO_state = X2_HO_REQ_STATE__SOURCE_RECEIVES_REQ_ACK
			if X2HO_state == X2_HO_REQ_STATE__TARGET_RECEIVES_REQ:
				result = re.search('Received LTE_RRCConnectionReconfigurationComplete from UE', str(line))
				if result is not None:
					X2HO_state = X2_HO_REQ_STATE__TARGET_RRC_RECFG_COMPLETE
			if X2HO_state == X2_HO_REQ_STATE__TARGET_RRC_RECFG_COMPLETE:
				result = re.search('issue rrc_eNB_send_PATH_SWITCH_REQ', str(line))
				if result is not None:
					X2HO_state = X2_HO_REQ_STATE__TARGET_SENDS_SWITCH_REQ
			if X2HO_state == X2_HO_REQ_STATE__TARGET_SENDS_SWITCH_REQ:
				result = re.search('received path switch ack S1AP_PATH_SWITCH_REQ_ACK', str(line))
				if result is not None:
					X2HO_state = X2_HO_REQ_STATE__IDLE
					X2HO_inNbProcedures += 1
			if X2HO_state == X2_HO_REQ_STATE__SOURCE_RECEIVES_REQ_ACK:
				result = re.search('source eNB receives the X2 UE CONTEXT RELEASE X2AP_UE_CONTEXT_RELEASE', str(line))
				if result is not None:
					X2HO_state = X2_HO_REQ_STATE__IDLE
					X2HO_outNbProcedures += 1

			if self.eNBOptions[int(self.eNB_instance)] != '':
				res1 = re.search('max_rxgain (?P<requested_option>[0-9]+)', self.eNBOptions[int(self.eNB_instance)])
				res2 = re.search('max_rxgain (?P<applied_option>[0-9]+)',  str(line))
				if res1 is not None and res2 is not None:
					requested_option = int(res1.group('requested_option'))
					applied_option = int(res2.group('applied_option'))
					if requested_option == applied_option:
						self.htmleNBFailureMsg += '<span class="glyphicon glyphicon-ok-circle"></span> Command line option(s) correctly applied <span class="glyphicon glyphicon-arrow-right"></span> ' + self.eNBOptions[int(self.eNB_instance)] + '\n\n'
					else:
						self.htmleNBFailureMsg += '<span class="glyphicon glyphicon-ban-circle"></span> Command line option(s) NOT applied <span class="glyphicon glyphicon-arrow-right"></span> ' + self.eNBOptions[int(self.eNB_instance)] + '\n\n'
			result = re.search('Exiting OAI softmodem', str(line))
			if result is not None:
				exitSignalReceived = True
			result = re.search('[Ss]egmentation [Ff]ault', str(line))
			if result is not None and not exitSignalReceived:
				foundSegFault = True
			result = re.search('[Cc]ore [dD]ump', str(line))
			if result is not None and not exitSignalReceived:
				foundSegFault = True
			result = re.search('./lte_build_oai/build/lte-softmodem', str(line))
			if result is not None and not exitSignalReceived:
				foundSegFault = True
			result = re.search('[Aa]ssertion', str(line))
			if result is not None and not exitSignalReceived:
				foundAssertion = True
			result = re.search('LLL', str(line))
			if result is not None and not exitSignalReceived:
				foundRealTimeIssue = True
			if foundAssertion and (msgLine < 3):
				msgLine += 1
				msgAssertion += str(line)
			result = re.search('Setting function for RU', str(line))
			if result is not None:
				isRRU = True
			if isRRU:
				result = re.search('RU 0 is_slave=yes', str(line))
				if result is not None:
					isSlave = True
				if isSlave:
					result = re.search('Received RRU_frame_resynch command', str(line))
					if result is not None:
						slaveReceivesFrameResyncCmd = True
			result = re.search('LTE_RRCConnectionSetupComplete from UE', str(line))
			if result is not None:
				rrcSetupComplete += 1
			result = re.search('Generate LTE_RRCConnectionRelease|Generate RRCConnectionRelease', str(line))
			if result is not None:
				rrcReleaseRequest += 1
			result = re.search('Generate LTE_RRCConnectionReconfiguration', str(line))
			if result is not None:
				rrcReconfigRequest += 1
			result = re.search('LTE_RRCConnectionReconfigurationComplete from UE rnti', str(line))
			if result is not None:
				rrcReconfigComplete += 1
			result = re.search('LTE_RRCConnectionReestablishmentRequest', str(line))
			if result is not None:
				rrcReestablishRequest += 1
			result = re.search('LTE_RRCConnectionReestablishmentComplete', str(line))
			if result is not None:
				rrcReestablishComplete += 1
			result = re.search('LTE_RRCConnectionReestablishmentReject', str(line))
			if result is not None:
				rrcReestablishReject += 1
			result = re.search('CDRX configuration activated after RRC Connection', str(line))
			if result is not None:
				cdrxActivationMessageCount += 1
			result = re.search('uci->stat', str(line))
			if result is not None:
				uciStatMsgCount += 1
			result = re.search('PDCP.*Out of Resources.*reason', str(line))
			if result is not None:
				pdcpFailure += 1
			result = re.search('ULSCH in error in round', str(line))
			if result is not None:
				ulschFailure += 1
			result = re.search('BAD all_segments_received', str(line))
			if result is not None:
				rlcDiscardBuffer += 1
			result = re.search('Canceled RA procedure for UE rnti', str(line))
			if result is not None:
				rachCanceledProcedure += 1
			result = re.search('dropping, not enough RBs', str(line))
			if result is not None:
				dropNotEnoughRBs += 1
			if self.eNBmbmsEnables[int(self.eNB_instance)]:
				result = re.search('MBMS USER-PLANE.*Requesting.*bytes from RLC', str(line))
				if result is not None:
					mbmsRequestMsg += 1
		enb_log_file.close()
		logging.debug('   File analysis completed')
		if uciStatMsgCount > 0:
			statMsg = 'eNB showed ' + str(uciStatMsgCount) + ' "uci->stat" message(s)'
			logging.debug('\u001B[1;30;43m ' + statMsg + ' \u001B[0m')
			self.htmleNBFailureMsg += statMsg + '\n'
		if pdcpFailure > 0:
			statMsg = 'eNB showed ' + str(pdcpFailure) + ' "PDCP Out of Resources" message(s)'
			logging.debug('\u001B[1;30;43m ' + statMsg + ' \u001B[0m')
			self.htmleNBFailureMsg += statMsg + '\n'
		if ulschFailure > 0:
			statMsg = 'eNB showed ' + str(ulschFailure) + ' "ULSCH in error in round" message(s)'
			logging.debug('\u001B[1;30;43m ' + statMsg + ' \u001B[0m')
			self.htmleNBFailureMsg += statMsg + '\n'
		if dropNotEnoughRBs > 0:
			statMsg = 'eNB showed ' + str(dropNotEnoughRBs) + ' "dropping, not enough RBs" message(s)'
			logging.debug('\u001B[1;30;43m ' + statMsg + ' \u001B[0m')
			self.htmleNBFailureMsg += statMsg + '\n'
		if rrcSetupComplete > 0:
			rrcMsg = 'eNB completed ' + str(rrcSetupComplete) + ' RRC Connection Setup(s)'
			logging.debug('\u001B[1;30;43m ' + rrcMsg + ' \u001B[0m')
			self.htmleNBFailureMsg += rrcMsg + '\n'
		if rrcReleaseRequest > 0:
			rrcMsg = 'eNB requested ' + str(rrcReleaseRequest) + ' RRC Connection Release(s)'
			logging.debug('\u001B[1;30;43m ' + rrcMsg + ' \u001B[0m')
			self.htmleNBFailureMsg += rrcMsg + '\n'
		if rrcReconfigRequest > 0 or rrcReconfigComplete > 0:
			rrcMsg = 'eNB requested ' + str(rrcReconfigRequest) + ' RRC Connection Reconfiguration(s)'
			logging.debug('\u001B[1;30;43m ' + rrcMsg + ' \u001B[0m')
			self.htmleNBFailureMsg += rrcMsg + '\n'
			rrcMsg = ' -- ' + str(rrcReconfigComplete) + ' were completed'
			logging.debug('\u001B[1;30;43m ' + rrcMsg + ' \u001B[0m')
			self.htmleNBFailureMsg += rrcMsg + '\n'
		if rrcReestablishRequest > 0 or rrcReestablishComplete > 0 or rrcReestablishReject > 0:
			rrcMsg = 'eNB requested ' + str(rrcReestablishRequest) + ' RRC Connection Reestablishment(s)'
			logging.debug('\u001B[1;30;43m ' + rrcMsg + ' \u001B[0m')
			self.htmleNBFailureMsg += rrcMsg + '\n'
			rrcMsg = ' -- ' + str(rrcReestablishComplete) + ' were completed'
			logging.debug('\u001B[1;30;43m ' + rrcMsg + ' \u001B[0m')
			self.htmleNBFailureMsg += rrcMsg + '\n'
			rrcMsg = ' -- ' + str(rrcReestablishReject) + ' were rejected'
			logging.debug('\u001B[1;30;43m ' + rrcMsg + ' \u001B[0m')
			self.htmleNBFailureMsg += rrcMsg + '\n'
		if self.eNBmbmsEnables[int(self.eNB_instance)]:
			if mbmsRequestMsg > 0:
				rrcMsg = 'eNB requested ' + str(mbmsRequestMsg) + ' times the RLC for MBMS USER-PLANE'
				logging.debug('\u001B[1;30;43m ' + rrcMsg + ' \u001B[0m')
				self.htmleNBFailureMsg += rrcMsg + '\n'
		if X2HO_inNbProcedures > 0:
			rrcMsg = 'eNB completed ' + str(X2HO_inNbProcedures) + ' X2 Handover Connection procedure(s)'
			logging.debug('\u001B[1;30;43m ' + rrcMsg + ' \u001B[0m')
			self.htmleNBFailureMsg += rrcMsg + '\n'
		if X2HO_outNbProcedures > 0:
			rrcMsg = 'eNB completed ' + str(X2HO_outNbProcedures) + ' X2 Handover Release procedure(s)'
			logging.debug('\u001B[1;30;43m ' + rrcMsg + ' \u001B[0m')
			self.htmleNBFailureMsg += rrcMsg + '\n'
		if self.eNBOptions[int(self.eNB_instance)] != '':
			res1 = re.search('drx_Config_present prSetup', self.eNBOptions[int(self.eNB_instance)])
			if res1 is not None:
				if cdrxActivationMessageCount > 0:
					rrcMsg = 'eNB activated the CDRX Configuration for ' + str(cdrxActivationMessageCount) + ' time(s)'
					logging.debug('\u001B[1;30;43m ' + rrcMsg + ' \u001B[0m')
					self.htmleNBFailureMsg += rrcMsg + '\n'
				else:
					rrcMsg = 'eNB did NOT ACTIVATE the CDRX Configuration'
					logging.debug('\u001B[1;37;43m ' + rrcMsg + ' \u001B[0m')
					self.htmleNBFailureMsg += rrcMsg + '\n'
		if rachCanceledProcedure > 0:
			rachMsg = 'eNB cancelled ' + str(rachCanceledProcedure) + ' RA procedure(s)'
			logging.debug('\u001B[1;30;43m ' + rachMsg + ' \u001B[0m')
			self.htmleNBFailureMsg += rachMsg + '\n'
		if isRRU:
			if isSlave:
				if slaveReceivesFrameResyncCmd:
					rruMsg = 'Slave RRU received the RRU_frame_resynch command from RAU'
					logging.debug('\u001B[1;30;43m ' + rruMsg + ' \u001B[0m')
					self.htmleNBFailureMsg += rruMsg + '\n'
				else:
					rruMsg = 'Slave RRU DID NOT receive the RRU_frame_resynch command from RAU'
					logging.debug('\u001B[1;37;41m ' + rruMsg + ' \u001B[0m')
					self.htmleNBFailureMsg += rruMsg + '\n'
					self.prematureExit = True
					return ENB_PROCESS_SLAVE_RRU_NOT_SYNCED
		if foundSegFault:
			logging.debug('\u001B[1;37;41m eNB ended with a Segmentation Fault! \u001B[0m')
			return ENB_PROCESS_SEG_FAULT
		if foundAssertion:
			logging.debug('\u001B[1;37;41m eNB ended with an assertion! \u001B[0m')
			self.htmleNBFailureMsg += msgAssertion
			return ENB_PROCESS_ASSERTION
		if foundRealTimeIssue:
			logging.debug('\u001B[1;37;41m eNB faced real time issues! \u001B[0m')
			self.htmleNBFailureMsg += 'eNB faced real time issues!\n'
			#return ENB_PROCESS_REALTIME_ISSUE
		if rlcDiscardBuffer > 0:
			rlcMsg = 'eNB RLC discarded ' + str(rlcDiscardBuffer) + ' buffer(s)'
			logging.debug('\u001B[1;37;41m ' + rlcMsg + ' \u001B[0m')
			self.htmleNBFailureMsg += rlcMsg + '\n'
			return ENB_PROCESS_REALTIME_ISSUE
		return 0

	def AnalyzeLogFile_UE(self, UElogFile):
		if (not os.path.isfile('./' + UElogFile)):
			return -1
		ue_log_file = open('./' + UElogFile, 'r')
		exitSignalReceived = False
		foundAssertion = False
		msgAssertion = ''
		msgLine = 0
		foundSegFault = False
		foundRealTimeIssue = False
		uciStatMsgCount = 0
		pdcpDataReqFailedCount = 0
		badDciCount = 0
		rrcConnectionRecfgComplete = 0
		no_cell_sync_found = False
		mib_found = False
		frequency_found = False
		plmn_found = False
		mbms_messages = 0
		self.htmlUEFailureMsg = ''
		for line in ue_log_file.readlines():
			result = re.search('Exiting OAI softmodem', str(line))
			if result is not None:
				exitSignalReceived = True
			result = re.search('System error|[Ss]egmentation [Ff]ault|======= Backtrace: =========|======= Memory map: ========', str(line))
			if result is not None and not exitSignalReceived:
				foundSegFault = True
			result = re.search('[Cc]ore [dD]ump', str(line))
			if result is not None and not exitSignalReceived:
				foundSegFault = True
			result = re.search('./lte-uesoftmodem', str(line))
			if result is not None and not exitSignalReceived:
				foundSegFault = True
			result = re.search('[Aa]ssertion', str(line))
			if result is not None and not exitSignalReceived:
				foundAssertion = True
			result = re.search('LLL', str(line))
			if result is not None and not exitSignalReceived:
				foundRealTimeIssue = True
			if foundAssertion and (msgLine < 3):
				msgLine += 1
				msgAssertion += str(line)
			result = re.search('uci->stat', str(line))
			if result is not None and not exitSignalReceived:
				uciStatMsgCount += 1
			result = re.search('PDCP data request failed', str(line))
			if result is not None and not exitSignalReceived:
				pdcpDataReqFailedCount += 1
			result = re.search('bad DCI 1A', str(line))
			if result is not None and not exitSignalReceived:
				badDciCount += 1
			result = re.search('Generating RRCConnectionReconfigurationComplete', str(line))
			if result is not None:
				rrcConnectionRecfgComplete += 1
			# No cell synchronization found, abandoning
			result = re.search('No cell synchronization found, abandoning', str(line))
			if result is not None:
				no_cell_sync_found = True
			if self.eNBmbmsEnables[0]:
				result = re.search('TRIED TO PUSH MBMS DATA', str(line))
				if result is not None:
					mbms_messages += 1
			result = re.search("MIB Information => ([a-zA-Z]{1,10}), ([a-zA-Z]{1,10}), NidCell (?P<nidcell>\d{1,3}), N_RB_DL (?P<n_rb_dl>\d{1,3}), PHICH DURATION (?P<phich_duration>\d), PHICH RESOURCE (?P<phich_resource>.{1,4}), TX_ANT (?P<tx_ant>\d)", str(line))
			if result is not None and (not mib_found):
				try:
					mibMsg = "MIB Information: " + result.group(1) + ', ' + result.group(2)
					self.htmlUEFailureMsg += mibMsg + '\n'
					logging.debug('\033[94m' + mibMsg + '\033[0m')
					mibMsg = "    nidcell = " + result.group('nidcell')
					self.htmlUEFailureMsg += mibMsg
					logging.debug('\033[94m' + mibMsg + '\033[0m')
					mibMsg = "    n_rb_dl = " + result.group('n_rb_dl')
					self.htmlUEFailureMsg += mibMsg + '\n'
					logging.debug('\033[94m' + mibMsg + '\033[0m')
					mibMsg = "    phich_duration = " + result.group('phich_duration')
					self.htmlUEFailureMsg += mibMsg
					logging.debug('\033[94m' + mibMsg + '\033[0m')
					mibMsg = "    phich_resource = " + result.group('phich_resource')
					self.htmlUEFailureMsg += mibMsg + '\n'
					logging.debug('\033[94m' + mibMsg + '\033[0m')
					mibMsg = "    tx_ant = " + result.group('tx_ant')
					self.htmlUEFailureMsg += mibMsg + '\n'
					logging.debug('\033[94m' + mibMsg + '\033[0m')
					mib_found = True
				except Exception as e:
					logging.error('\033[91m' + "MIB marker was not found" + '\033[0m')
			result = re.search("Measured Carrier Frequency (?P<measured_carrier_frequency>\d{1,15}) Hz", str(line))
			if result is not None and (not frequency_found):
				try:
					mibMsg = "Measured Carrier Frequency = " + result.group('measured_carrier_frequency') + ' Hz'
					self.htmlUEFailureMsg += mibMsg + '\n'
					logging.debug('\033[94m' + mibMsg + '\033[0m')
					frequency_found = True
				except Exception as e:
					logging.error('\033[91m' + "Measured Carrier Frequency not found" + '\033[0m')
			result = re.search("PLMN MCC (?P<mcc>\d{1,3}), MNC (?P<mnc>\d{1,3}), TAC", str(line))
			if result is not None and (not plmn_found):
				try:
					mibMsg = 'PLMN MCC = ' + result.group('mcc') + ' MNC = ' + result.group('mnc')
					self.htmlUEFailureMsg += mibMsg + '\n'
					logging.debug('\033[94m' + mibMsg + '\033[0m')
					plmn_found = True
				except Exception as e:
					logging.error('\033[91m' + "PLMN not found" + '\033[0m')
			result = re.search("Found (?P<operator>[\w,\s]{1,15}) \(name from internal table\)", str(line))
			if result is not None:
				try:
					mibMsg = "The operator is: " + result.group('operator')
					self.htmlUEFailureMsg += mibMsg + '\n'
					logging.debug('\033[94m' + mibMsg + '\033[0m')
				except Exception as e:
					logging.error('\033[91m' + "Operator name not found" + '\033[0m')
			result = re.search("SIB5 InterFreqCarrierFreq element (.{1,4})/(.{1,4})", str(line))
			if result is not None:
				try:
					mibMsg = "SIB5 InterFreqCarrierFreq element " + result.group(1) + '/' + result.group(2)
					self.htmlUEFailureMsg += mibMsg + ' -> '
					logging.debug('\033[94m' + mibMsg + '\033[0m')
				except Exception as e:
					logging.error('\033[91m' + "SIB5 InterFreqCarrierFreq element not found" + '\033[0m')
			result = re.search("DL Carrier Frequency/ARFCN : (?P<carrier_frequency>\d{1,15}/\d{1,4})", str(line))
			if result is not None:
				try:
					freq = result.group('carrier_frequency')
					new_freq = re.sub('/[0-9]+','',freq)
					float_freq = float(new_freq) / 1000000
					self.htmlUEFailureMsg += 'DL Freq: ' + ('%.1f' % float_freq) + ' MHz'
					logging.debug('\033[94m' + "    DL Carrier Frequency is: " + freq + '\033[0m')
				except Exception as e:
					logging.error('\033[91m' + "    DL Carrier Frequency not found" + '\033[0m')
			result = re.search("AllowedMeasBandwidth : (?P<allowed_bandwidth>\d{1,7})", str(line))
			if result is not None:
				try:
					prb = result.group('allowed_bandwidth')
					self.htmlUEFailureMsg += ' -- PRB: ' + prb + '\n'
					logging.debug('\033[94m' + "    AllowedMeasBandwidth: " + prb + '\033[0m')
				except Exception as e:
					logging.error('\033[91m' + "    AllowedMeasBandwidth not found" + '\033[0m')
		ue_log_file.close()
		if rrcConnectionRecfgComplete > 0:
			statMsg = 'UE connected to eNB (' + str(rrcConnectionRecfgComplete) + ' RRCConnectionReconfigurationComplete message(s) generated)'
			logging.debug('\033[94m' + statMsg + '\033[0m')
			self.htmlUEFailureMsg += statMsg + '\n'
		if uciStatMsgCount > 0:
			statMsg = 'UE showed ' + str(uciStatMsgCount) + ' "uci->stat" message(s)'
			logging.debug('\u001B[1;30;43m ' + statMsg + ' \u001B[0m')
			self.htmlUEFailureMsg += statMsg + '\n'
		if pdcpDataReqFailedCount > 0:
			statMsg = 'UE showed ' + str(pdcpDataReqFailedCount) + ' "PDCP data request failed" message(s)'
			logging.debug('\u001B[1;30;43m ' + statMsg + ' \u001B[0m')
			self.htmlUEFailureMsg += statMsg + '\n'
		if badDciCount > 0:
			statMsg = 'UE showed ' + str(badDciCount) + ' "bad DCI 1A" message(s)'
			logging.debug('\u001B[1;30;43m ' + statMsg + ' \u001B[0m')
			self.htmlUEFailureMsg += statMsg + '\n'
		if self.eNBmbmsEnables[0]:
			if mbms_messages > 0:
				statMsg = 'UE showed ' + str(mbms_messages) + ' "TRIED TO PUSH MBMS DATA" message(s)'
				logging.debug('\u001B[1;30;43m ' + statMsg + ' \u001B[0m')
			else:
				statMsg = 'UE did NOT SHOW "TRIED TO PUSH MBMS DATA" message(s)'
				logging.debug('\u001B[1;30;41m ' + statMsg + ' \u001B[0m')
			self.htmlUEFailureMsg += statMsg + '\n'
		if foundSegFault:
			logging.debug('\u001B[1;37;41m UE ended with a Segmentation Fault! \u001B[0m')
			return ENB_PROCESS_SEG_FAULT
		if foundAssertion:
			logging.debug('\u001B[1;30;43m UE showed an assertion! \u001B[0m')
			self.htmlUEFailureMsg += 'UE showed an assertion!\n'
			if not mib_found or not frequency_found:
				return OAI_UE_PROCESS_ASSERTION
		if foundRealTimeIssue:
			logging.debug('\u001B[1;37;41m UE faced real time issues! \u001B[0m')
			self.htmlUEFailureMsg += 'UE faced real time issues!\n'
			#return ENB_PROCESS_REALTIME_ISSUE
		if no_cell_sync_found and not mib_found:
			logging.debug('\u001B[1;37;41m UE could not synchronize ! \u001B[0m')
			self.htmlUEFailureMsg += 'UE could not synchronize!\n'
			return OAI_UE_PROCESS_COULD_NOT_SYNC
		return 0

	def TerminateeNB(self):
		if self.eNB_serverId == '0':
			lIpAddr = self.eNBIPAddress
			lUserName = self.eNBUserName
			lPassWord = self.eNBPassword
			lSourcePath = self.eNBSourceCodePath
		elif self.eNB_serverId == '1':
			lIpAddr = self.eNB1IPAddress
			lUserName = self.eNB1UserName
			lPassWord = self.eNB1Password
			lSourcePath = self.eNB1SourceCodePath
		elif self.eNB_serverId == '2':
			lIpAddr = self.eNB2IPAddress
			lUserName = self.eNB2UserName
			lPassWord = self.eNB2Password
			lSourcePath = self.eNB2SourceCodePath
		if lIpAddr == '' or lUserName == '' or lPassWord == '' or lSourcePath == '':
			Usage()
			sys.exit('Insufficient Parameter')
		self.open(lIpAddr, lUserName, lPassWord)
		self.command('cd ' + lSourcePath + '/cmake_targets', '\$', 5)
		self.command('stdbuf -o0  ps -aux | grep --color=never softmodem | grep -v grep', '\$', 5)
		result = re.search('lte-softmodem', str(self.ssh.before))
		if result is not None:
			self.command('echo ' + lPassWord + ' | sudo -S daemon --name=enb' + str(self.eNB_instance) + '_daemon --stop', '\$', 5)
			self.command('echo ' + lPassWord + ' | sudo -S killall --signal SIGINT lte-softmodem || true', '\$', 5)
			time.sleep(10)
			self.command('stdbuf -o0  ps -aux | grep --color=never softmodem | grep -v grep', '\$', 5)
			result = re.search('lte-softmodem', str(self.ssh.before))
			if result is not None:
				self.command('echo ' + lPassWord + ' | sudo -S killall --signal SIGKILL lte-softmodem || true', '\$', 5)
				time.sleep(5)
		self.command('rm -f my-lte-softmodem-run' + str(self.eNB_instance) + '.sh', '\$', 5)
		self.close()
		# If tracer options is on, stopping tshark on EPC side
		result = re.search('T_stdout', str(self.Initialize_eNB_args))
		if result is not None:
			self.open(self.EPCIPAddress, self.EPCUserName, self.EPCPassword)
			logging.debug('\u001B[1m Stopping tshark \u001B[0m')
			self.command('echo ' + self.EPCPassword + ' | sudo -S killall --signal SIGKILL tshark', '\$', 5)
			time.sleep(1)
			if self.EPC_PcapFileName != '':
				self.command('echo ' + self.EPCPassword + ' | sudo -S chmod 666 /tmp/' + self.EPC_PcapFileName, '\$', 5)
				self.copyin(self.EPCIPAddress, self.EPCUserName, self.EPCPassword, '/tmp/' + self.EPC_PcapFileName, '.')
				self.copyout(lIpAddr, lUserName, lPassWord, self.EPC_PcapFileName, lSourcePath + '/cmake_targets/.')
			self.close()
			logging.debug('\u001B[1m Replaying RAW record file\u001B[0m')
			self.open(lIpAddr, lUserName, lPassWord)
			self.command('cd ' + lSourcePath + '/common/utils/T/tracer/', '\$', 5)
			enbLogFile = self.eNBLogFiles[int(self.eNB_instance)]
			raw_record_file = enbLogFile.replace('.log', '_record.raw')
			replay_log_file = enbLogFile.replace('.log', '_replay.log')
			extracted_txt_file = enbLogFile.replace('.log', '_extracted_messages.txt')
			extracted_log_file = enbLogFile.replace('.log', '_extracted_messages.log')
			self.command('./extract_config -i ' + lSourcePath + '/cmake_targets/' + raw_record_file + ' > ' + lSourcePath + '/cmake_targets/' + extracted_txt_file, '\$', 5)
			self.command('echo $USER; nohup ./replay -i ' + lSourcePath + '/cmake_targets/' + raw_record_file + ' > ' + lSourcePath + '/cmake_targets/' + replay_log_file + ' 2>&1 &', lUserName, 5)
			self.command('./textlog -d ' +  lSourcePath + '/cmake_targets/' + extracted_txt_file + ' -no-gui -ON -full > ' + lSourcePath + '/cmake_targets/' + extracted_log_file, '\$', 5)
			self.close()
			self.copyin(lIpAddr, lUserName, lPassWord, lSourcePath + '/cmake_targets/' + extracted_log_file, '.')
			logging.debug('\u001B[1m Analyzing eNB replay logfile \u001B[0m')
			logStatus = self.AnalyzeLogFile_eNB(extracted_log_file)
			self.CreateHtmlTestRow('N/A', 'OK', ALL_PROCESSES_OK)
			self.eNBLogFiles[int(self.eNB_instance)] = ''
		else:
			analyzeFile = False
			if self.eNBLogFiles[int(self.eNB_instance)] != '':
				analyzeFile = True
				fileToAnalyze = self.eNBLogFiles[int(self.eNB_instance)]
				self.eNBLogFiles[int(self.eNB_instance)] = ''
			if analyzeFile:
				copyin_res = self.copyin(lIpAddr, lUserName, lPassWord, lSourcePath + '/cmake_targets/' + fileToAnalyze, '.')
				if (copyin_res == -1):
					logging.debug('\u001B[1;37;41m Could not copy eNB logfile to analyze it! \u001B[0m')
					self.htmleNBFailureMsg = 'Could not copy eNB logfile to analyze it!'
					self.CreateHtmlTestRow('N/A', 'KO', ENB_PROCESS_NOLOGFILE_TO_ANALYZE)
					self.eNBmbmsEnables[int(self.eNB_instance)] = False
					return
				if self.eNB_serverId != '0':
					self.copyout(self.eNBIPAddress, self.eNBUserName, self.eNBPassword, './' + fileToAnalyze, self.eNBSourceCodePath + '/cmake_targets/')
				logging.debug('\u001B[1m Analyzing eNB logfile \u001B[0m ' + fileToAnalyze)
				logStatus = self.AnalyzeLogFile_eNB(fileToAnalyze)
				if (logStatus < 0):
					self.CreateHtmlTestRow('N/A', 'KO', logStatus)
					self.preamtureExit = True
					self.eNBmbmsEnables[int(self.eNB_instance)] = False
					return
				else:
					self.CreateHtmlTestRow('N/A', 'OK', ALL_PROCESSES_OK)
			else:
				self.CreateHtmlTestRow('N/A', 'OK', ALL_PROCESSES_OK)
		self.eNBmbmsEnables[int(self.eNB_instance)] = False

	def TerminateHSS(self):
		self.open(self.EPCIPAddress, self.EPCUserName, self.EPCPassword)
		if re.match('OAI-Rel14-CUPS', self.EPCType, re.IGNORECASE):
			self.command('echo ' + self.EPCPassword + ' | sudo -S killall --signal SIGINT oai_hss || true', '\$', 5)
			time.sleep(2)
			self.command('stdbuf -o0  ps -aux | grep hss | grep -v grep', '\$', 5)
			result = re.search('oai_hss -j', str(self.ssh.before))
			if result is not None:
				self.command('echo ' + self.EPCPassword + ' | sudo -S killall --signal SIGKILL oai_hss || true', '\$', 5)
			self.command('rm -f ' + self.EPCSourceCodePath + '/scripts/my-hss.sh', '\$', 5)
		elif re.match('OAI', self.EPCType, re.IGNORECASE):
			self.command('echo ' + self.EPCPassword + ' | sudo -S killall --signal SIGINT run_hss oai_hss || true', '\$', 5)
			time.sleep(2)
			self.command('stdbuf -o0  ps -aux | grep hss | grep -v grep', '\$', 5)
			result = re.search('\/bin\/bash .\/run_', str(self.ssh.before))
			if result is not None:
				self.command('echo ' + self.EPCPassword + ' | sudo -S killall --signal SIGKILL run_hss oai_hss || true', '\$', 5)
		elif re.match('ltebox', self.EPCType, re.IGNORECASE):
			self.command('cd ' + self.EPCSourceCodePath, '\$', 5)
			self.command('cd scripts', '\$', 5)
			self.command('echo ' + self.EPCPassword + ' | sudo -S daemon --name=simulated_hss --stop', '\$', 5)
			time.sleep(1)
			self.command('echo ' + self.EPCPassword + ' | sudo -S killall --signal SIGKILL hss_sim', '\$', 5)
		else:
			logging.error('This should not happen!')
		self.close()
		self.CreateHtmlTestRow('N/A', 'OK', ALL_PROCESSES_OK)

	def TerminateMME(self):
		self.open(self.EPCIPAddress, self.EPCUserName, self.EPCPassword)
		if re.match('OAI', self.EPCType, re.IGNORECASE) or re.match('OAI-Rel14-CUPS', self.EPCType, re.IGNORECASE):
			self.command('echo ' + self.EPCPassword + ' | sudo -S killall --signal SIGINT run_mme mme || true', '\$', 5)
			time.sleep(2)
			self.command('stdbuf -o0 ps -aux | grep mme | grep -v grep', '\$', 5)
			result = re.search('mme -c', str(self.ssh.before))
			if result is not None:
				self.command('echo ' + self.EPCPassword + ' | sudo -S killall --signal SIGKILL run_mme mme || true', '\$', 5)
			self.command('rm -f ' + self.EPCSourceCodePath + '/scripts/my-mme.sh', '\$', 5)
		elif re.match('ltebox', self.EPCType, re.IGNORECASE):
			self.command('cd /opt/ltebox/tools', '\$', 5)
			self.command('echo ' + self.EPCPassword + ' | sudo -S ./stop_mme', '\$', 5)
		else:
			logging.error('This should not happen!')
		self.close()
		self.CreateHtmlTestRow('N/A', 'OK', ALL_PROCESSES_OK)

	def TerminateSPGW(self):
		self.open(self.EPCIPAddress, self.EPCUserName, self.EPCPassword)
		if re.match('OAI-Rel14-CUPS', self.EPCType, re.IGNORECASE):
			self.command('echo ' + self.EPCPassword + ' | sudo -S killall --signal SIGINT spgwc spgwu || true', '\$', 5)
			time.sleep(2)
			self.command('stdbuf -o0 ps -aux | grep spgw | grep -v grep', '\$', 5)
			result = re.search('spgwc -c |spgwu -c ', str(self.ssh.before))
			if result is not None:
				self.command('echo ' + self.EPCPassword + ' | sudo -S killall --signal SIGKILL spgwc spgwu || true', '\$', 5)
			self.command('rm -f ' + self.EPCSourceCodePath + '/scripts/my-spgw*.sh', '\$', 5)
			self.command('stdbuf -o0 ps -aux | grep tshark | grep -v grep', '\$', 5)
			result = re.search('-w ', str(self.ssh.before))
			if result is not None:
				self.command('echo ' + self.EPCPassword + ' | sudo -S killall --signal SIGINT tshark || true', '\$', 5)
				self.command('echo ' + self.EPCPassword + ' | sudo -S chmod 666 ' + self.EPCSourceCodePath + '/scripts/*.pcap', '\$', 5)
		elif re.match('OAI', self.EPCType, re.IGNORECASE):
			self.command('echo ' + self.EPCPassword + ' | sudo -S killall --signal SIGINT run_spgw spgw || true', '\$', 5)
			time.sleep(2)
			self.command('stdbuf -o0 ps -aux | grep spgw | grep -v grep', '\$', 5)
			result = re.search('\/bin\/bash .\/run_', str(self.ssh.before))
			if result is not None:
				self.command('echo ' + self.EPCPassword + ' | sudo -S killall --signal SIGKILL run_spgw spgw || true', '\$', 5)
		elif re.match('ltebox', self.EPCType, re.IGNORECASE):
			self.command('cd /opt/ltebox/tools', '\$', 5)
			self.command('echo ' + self.EPCPassword + ' | sudo -S ./stop_xGw', '\$', 5)
		else:
			logging.error('This should not happen!')
		self.close()
		self.CreateHtmlTestRow('N/A', 'OK', ALL_PROCESSES_OK)

	def TerminateFlexranCtrl(self):
		if self.flexranCtrlInstalled == False or self.flexranCtrlStarted == False:
			return
		if self.EPCIPAddress == '' or self.EPCUserName == '' or self.EPCPassword == '':
			Usage()
			sys.exit('Insufficient Parameter')
		self.open(self.EPCIPAddress, self.EPCUserName, self.EPCPassword)
		self.command('echo ' + self.EPCPassword + ' | sudo -S daemon --name=flexran_rtc_daemon --stop', '\$', 5)
		time.sleep(1)
		self.command('echo ' + self.EPCPassword + ' | sudo -S killall --signal SIGKILL rt_controller', '\$', 5)
		time.sleep(1)
		self.close()
		self.flexranCtrlStarted = False
		self.CreateHtmlTestRow('N/A', 'OK', ALL_PROCESSES_OK)

	def TerminateUE_common(self, device_id, idx):
		try:
			self.open(self.ADBIPAddress, self.ADBUserName, self.ADBPassword)
			# back in airplane mode on (ie radio off)
			if self.ADBCentralized:
				if device_id == '84B7N16418004022':
					self.command('stdbuf -o0 adb -s ' + device_id + ' shell "su - root -c /data/local/tmp/off"', '\$', 60)
				else:
					self.command('stdbuf -o0 adb -s ' + device_id + ' shell /data/local/tmp/off', '\$', 60)
			else:
				self.command('ssh ' + self.UEDevicesRemoteUser[idx] + '@' + self.UEDevicesRemoteServer[idx] + ' ' + self.UEDevicesOffCmd[idx], '\$', 60)
			logging.debug('\u001B[1mUE (' + device_id + ') Detach Completed\u001B[0m')

			if self.ADBCentralized:
				self.command('stdbuf -o0 adb -s ' + device_id + ' shell "ps | grep --color=never iperf | grep -v grep"', '\$', 5)
			else:
				self.command('ssh ' + self.UEDevicesRemoteUser[idx] + '@' + self.UEDevicesRemoteServer[idx] + ' \'adb -s ' + device_id + ' shell "ps | grep --color=never iperf | grep -v grep"\'', '\$', 60)
			result = re.search('shell +(?P<pid>\d+)', str(self.ssh.before))
			if result is not None:
				pid_iperf = result.group('pid')
				if self.ADBCentralized:
					self.command('stdbuf -o0 adb -s ' + device_id + ' shell "kill -KILL ' + pid_iperf + '"', '\$', 5)
				else:
					self.command('ssh ' + self.UEDevicesRemoteUser[idx] + '@' + self.UEDevicesRemoteServer[idx] + ' \'adb -s ' + device_id + ' shell "kill -KILL ' + pid_iperf + '"\'', '\$', 60)
			self.close()
		except:
			os.kill(os.getppid(),signal.SIGUSR1)

	def TerminateUE(self):
		terminate_ue_flag = True
		self.GetAllUEDevices(terminate_ue_flag)
		multi_jobs = []
		i = 0
		for device_id in self.UEDevices:
			p = Process(target= SSH.TerminateUE_common, args = (device_id,i,))
			p.daemon = True
			p.start()
			multi_jobs.append(p)
			i += 1
		for job in multi_jobs:
			job.join()
		self.CreateHtmlTestRow('N/A', 'OK', ALL_PROCESSES_OK)

	def TerminateOAIUE(self):
		self.open(self.UEIPAddress, self.UEUserName, self.UEPassword)
		self.command('cd ' + self.UESourceCodePath + '/cmake_targets', '\$', 5)
		self.command('ps -aux | grep --color=never softmodem | grep -v grep', '\$', 5)
		result = re.search('lte-uesoftmodem', str(self.ssh.before))
		if result is not None:
			self.command('echo ' + self.UEPassword + ' | sudo -S daemon --name=ue' + str(self.UE_instance) + '_daemon --stop', '\$', 5)
			self.command('echo ' + self.UEPassword + ' | sudo -S killall --signal SIGINT lte-uesoftmodem || true', '\$', 5)
			time.sleep(10)
			self.command('ps -aux | grep --color=never softmodem | grep -v grep', '\$', 5)
			result = re.search('lte-uesoftmodem', str(self.ssh.before))
			if result is not None:
				self.command('echo ' + self.UEPassword + ' | sudo -S killall --signal SIGKILL lte-uesoftmodem || true', '\$', 5)
				time.sleep(5)
		self.command('rm -f my-lte-uesoftmodem-run' + str(self.UE_instance) + '.sh', '\$', 5)
		self.close()
		result = re.search('ue_', str(self.UELogFile))
		if result is not None:
			copyin_res = self.copyin(self.UEIPAddress, self.UEUserName, self.UEPassword, self.UESourceCodePath + '/cmake_targets/' + self.UELogFile, '.')
			if (copyin_res == -1):
				logging.debug('\u001B[1;37;41m Could not copy UE logfile to analyze it! \u001B[0m')
				self.htmlUEFailureMsg = 'Could not copy UE logfile to analyze it!'
				self.CreateHtmlTestRow('N/A', 'KO', OAI_UE_PROCESS_NOLOGFILE_TO_ANALYZE, 'UE')
				self.UELogFile = ''
				return
			logging.debug('\u001B[1m Analyzing UE logfile \u001B[0m')
			logStatus = self.AnalyzeLogFile_UE(self.UELogFile)
			result = re.search('--no-L2-connect', str(self.Initialize_OAI_UE_args))
			if result is not None:
				ueAction = 'Sniffing'
			else:
				ueAction = 'Connection'
			if (logStatus < 0):
				logging.debug('\u001B[1m' + ueAction + ' Failed \u001B[0m')
				self.htmlUEFailureMsg = '<b>' + ueAction + ' Failed</b>\n' + self.htmlUEFailureMsg
				self.CreateHtmlTestRow('N/A', 'KO', logStatus, 'UE')
				# In case of sniffing on commercial eNBs we have random results
				# Not an error then
				if (logStatus != OAI_UE_PROCESS_COULD_NOT_SYNC) or (ueAction != 'Sniffing'):
					self.Initialize_OAI_UE_args = ''
					self.AutoTerminateUEandeNB()
			else:
				logging.debug('\u001B[1m' + ueAction + ' Completed \u001B[0m')
				self.htmlUEFailureMsg = '<b>' + ueAction + ' Completed</b>\n' + self.htmlUEFailureMsg
				self.CreateHtmlTestRow('N/A', 'OK', ALL_PROCESSES_OK)
			self.UELogFile = ''
		else:
			self.CreateHtmlTestRow('N/A', 'OK', ALL_PROCESSES_OK)

	def AutoTerminateUEandeNB(self):
		if (self.ADBIPAddress != 'none'):
			self.testCase_id = 'AUTO-KILL-UE'
			self.desc = 'Automatic Termination of UE'
			self.ShowTestID()
			self.TerminateUE()
		if (self.Initialize_OAI_UE_args != ''):
			self.testCase_id = 'AUTO-KILL-UE'
			self.desc = 'Automatic Termination of UE'
			self.ShowTestID()
			self.TerminateOAIUE()
		if (self.Initialize_eNB_args != ''):
			self.testCase_id = 'AUTO-KILL-eNB'
			self.desc = 'Automatic Termination of eNB'
			self.ShowTestID()
			self.eNB_instance = '0'
			self.TerminateeNB()
		if self.flexranCtrlInstalled and self.flexranCtrlStarted:
			self.testCase_id = 'AUTO-KILL-flexran-ctl'
			self.desc = 'Automatic Termination of FlexRan CTL'
			self.ShowTestID()
			self.TerminateFlexranCtrl()
		self.prematureExit = True

	def IdleSleep(self):
		time.sleep(self.idle_sleep_time)
		self.CreateHtmlTestRow(str(self.idle_sleep_time) + ' sec', 'OK', ALL_PROCESSES_OK)

	def X2_Status(self, idx, fileName):
		cmd = "curl --silent http://" + self.EPCIPAddress + ":9999/stats | jq '.' > " + fileName
		message = cmd + '\n'
		logging.debug(cmd)
		subprocess.run(cmd, shell=True)
		if idx == 0:
			cmd = "jq '.mac_stats | length' " + fileName
			strNbEnbs = subprocess.check_output(cmd, shell=True, universal_newlines=True)
			self.x2NbENBs = int(strNbEnbs.strip())
		cnt = 0
		while cnt < self.x2NbENBs:
			cmd = "jq '.mac_stats[" + str(cnt) + "].bs_id' " + fileName
			bs_id = subprocess.check_output(cmd, shell=True, universal_newlines=True)
			self.x2ENBBsIds[idx].append(bs_id.strip())
			cmd = "jq '.mac_stats[" + str(cnt) + "].ue_mac_stats | length' " + fileName
			stNbUEs = subprocess.check_output(cmd, shell=True, universal_newlines=True)
			nbUEs = int(stNbUEs.strip())
			ueIdx = 0
			self.x2ENBConnectedUEs[idx].append([])
			while ueIdx < nbUEs:
				cmd = "jq '.mac_stats[" + str(cnt) + "].ue_mac_stats[" + str(ueIdx) + "].rnti' " + fileName
				rnti = subprocess.check_output(cmd, shell=True, universal_newlines=True)
				self.x2ENBConnectedUEs[idx][cnt].append(rnti.strip())
				ueIdx += 1
			cnt += 1

		msg = "FlexRan Controller is connected to " + str(self.x2NbENBs) + " eNB(s)"
		logging.debug(msg)
		message += msg + '\n'
		cnt = 0
		while cnt < self.x2NbENBs:
			msg = "   -- eNB: " + str(self.x2ENBBsIds[idx][cnt]) + " is connected to " + str(len(self.x2ENBConnectedUEs[idx][cnt])) + " UE(s)"
			logging.debug(msg)
			message += msg + '\n'
			ueIdx = 0
			while ueIdx < len(self.x2ENBConnectedUEs[idx][cnt]):
				msg = "      -- UE rnti: " + str(self.x2ENBConnectedUEs[idx][cnt][ueIdx])
				logging.debug(msg)
				message += msg + '\n'
				ueIdx += 1
			cnt += 1
		return message

	def Perform_X2_Handover(self):
		html_queue = SimpleQueue()
		fullMessage = '<pre style="background-color:white">'
		msg = 'Doing X2 Handover w/ option ' + self.x2_ho_options
		logging.debug(msg)
		fullMessage += msg + '\n'
		if self.x2_ho_options == 'network':
			if self.flexranCtrlInstalled and self.flexranCtrlStarted:
				self.x2ENBBsIds = []
				self.x2ENBConnectedUEs = []
				self.x2ENBBsIds.append([])
				self.x2ENBBsIds.append([])
				self.x2ENBConnectedUEs.append([])
				self.x2ENBConnectedUEs.append([])
				fullMessage += self.X2_Status(0, self.testCase_id + '_pre_ho.json') 

				msg = "Activating the X2 Net control on each eNB"
				logging.debug(msg)
				fullMessage += msg + '\n'
				eNB_cnt = self.x2NbENBs
				cnt = 0
				while cnt < eNB_cnt:
					cmd = "curl --silent -XPOST http://" + self.EPCIPAddress + ":9999/rrc/x2_ho_net_control/enb/" + str(self.x2ENBBsIds[0][cnt]) + "/1"
					logging.debug(cmd)
					fullMessage += cmd + '\n'
					subprocess.run(cmd, shell=True)
					cnt += 1
				# Waiting for the activation to be active
				time.sleep(20)
				msg = "Switching UE(s) from eNB to eNB"
				logging.debug(msg)
				fullMessage += msg + '\n'
				cnt = 0
				while cnt < eNB_cnt:
					ueIdx = 0
					while ueIdx < len(self.x2ENBConnectedUEs[0][cnt]):
						cmd = "curl --silent -XPOST http://" + self.EPCIPAddress + ":9999/rrc/ho/senb/" + str(self.x2ENBBsIds[0][cnt]) + "/ue/" + str(self.x2ENBConnectedUEs[0][cnt][ueIdx]) + "/tenb/" + str(self.x2ENBBsIds[0][eNB_cnt - cnt - 1])
						logging.debug(cmd)
						fullMessage += cmd + '\n'
						subprocess.run(cmd, shell=True)
						ueIdx += 1
					cnt += 1
				time.sleep(20)
				# check
				logging.debug("Checking the Status after X2 Handover")
				fullMessage += self.X2_Status(1, self.testCase_id + '_post_ho.json') 
				cnt = 0
				x2Status = True
				while cnt < eNB_cnt:
					if len(self.x2ENBConnectedUEs[0][cnt]) == len(self.x2ENBConnectedUEs[1][cnt]):
						x2Status = False
					cnt += 1
				if x2Status:
					msg = "X2 Handover was successful"
					logging.debug(msg)
					fullMessage += msg + '</pre>'
					html_queue.put(fullMessage)
					self.CreateHtmlTestRowQueue('N/A', 'OK', len(self.UEDevices), html_queue)
				else:
					msg = "X2 Handover FAILED"
					logging.error(msg)
					fullMessage += msg + '</pre>'
					html_queue.put(fullMessage)
					self.CreateHtmlTestRowQueue('N/A', 'OK', len(self.UEDevices), html_queue)
			else:
				self.CreateHtmlTestRow('Cannot perform requested X2 Handover', 'KO', ALL_PROCESSES_OK)

	def LogCollectBuild(self):
		if (self.eNBIPAddress != '' and self.eNBUserName != '' and self.eNBPassword != ''):
			IPAddress = self.eNBIPAddress
			UserName = self.eNBUserName
			Password = self.eNBPassword
			SourceCodePath = self.eNBSourceCodePath
		elif (self.UEIPAddress != '' and self.UEUserName != '' and self.UEPassword != ''):
			IPAddress = self.UEIPAddress
			UserName = self.UEUserName
			Password = self.UEPassword
			SourceCodePath = self.UESourceCodePath
		else:
			sys.exit('Insufficient Parameter')
		self.open(IPAddress, UserName, Password)
		self.command('cd ' + SourceCodePath, '\$', 5)
		self.command('cd cmake_targets', '\$', 5)
		self.command('rm -f build.log.zip', '\$', 5)
		self.command('zip build.log.zip build_log_*/*', '\$', 60)
		self.close()

	def LogCollecteNB(self):
		self.open(self.eNBIPAddress, self.eNBUserName, self.eNBPassword)
		self.command('cd ' + self.eNBSourceCodePath, '\$', 5)
		self.command('cd cmake_targets', '\$', 5)
		self.command('echo ' + self.eNBPassword + ' | sudo -S rm -f enb.log.zip', '\$', 5)
		self.command('echo ' + self.eNBPassword + ' | sudo -S zip enb.log.zip enb*.log core* enb_*record.raw enb_*.pcap enb_*txt', '\$', 60)
		self.command('echo ' + self.eNBPassword + ' | sudo -S rm enb*.log core* enb_*record.raw enb_*.pcap enb_*txt', '\$', 5)
		self.close()

	def LogCollectPing(self):
		self.open(self.EPCIPAddress, self.EPCUserName, self.EPCPassword)
		self.command('cd ' + self.EPCSourceCodePath, '\$', 5)
		self.command('cd scripts', '\$', 5)
		self.command('rm -f ping.log.zip', '\$', 5)
		self.command('zip ping.log.zip ping*.log', '\$', 60)
		self.command('rm ping*.log', '\$', 5)
		self.close()

	def LogCollectIperf(self):
		self.open(self.EPCIPAddress, self.EPCUserName, self.EPCPassword)
		self.command('cd ' + self.EPCSourceCodePath, '\$', 5)
		self.command('cd scripts', '\$', 5)
		self.command('rm -f iperf.log.zip', '\$', 5)
		self.command('zip iperf.log.zip iperf*.log', '\$', 60)
		self.command('rm iperf*.log', '\$', 5)
		self.close()

	def LogCollectHSS(self):
		self.open(self.EPCIPAddress, self.EPCUserName, self.EPCPassword)
		self.command('cd ' + self.EPCSourceCodePath + '/scripts', '\$', 5)
		self.command('rm -f hss.log.zip', '\$', 5)
		if re.match('OAI', self.EPCType, re.IGNORECASE) or re.match('OAI-Rel14-CUPS', self.EPCType, re.IGNORECASE):
			self.command('zip hss.log.zip hss*.log', '\$', 60)
			self.command('echo ' + self.EPCPassword + ' | sudo -S rm hss*.log', '\$', 5)
			if re.match('OAI-Rel14-CUPS', self.EPCType, re.IGNORECASE):
				self.command('zip hss.log.zip logs/hss*.* *.pcap', '\$', 60)
				self.command('echo ' + self.EPCPassword + ' | sudo -S rm -f logs/hss*.* *.pcap', '\$', 5)
		elif re.match('ltebox', self.EPCType, re.IGNORECASE):
			self.command('cp /opt/hss_sim0609/hss.log .', '\$', 60)
			self.command('zip hss.log.zip hss.log', '\$', 60)
		else:
			logging.error('This option should not occur!')
		self.close()

	def LogCollectMME(self):
		self.open(self.EPCIPAddress, self.EPCUserName, self.EPCPassword)
		self.command('cd ' + self.EPCSourceCodePath + '/scripts', '\$', 5)
		self.command('rm -f mme.log.zip', '\$', 5)
		if re.match('OAI', self.EPCType, re.IGNORECASE) or re.match('OAI-Rel14-CUPS', self.EPCType, re.IGNORECASE):
			self.command('zip mme.log.zip mme*.log', '\$', 60)
			self.command('echo ' + self.EPCPassword + ' | sudo -S rm mme*.log', '\$', 5)
		elif re.match('ltebox', self.EPCType, re.IGNORECASE):
			self.command('cp /opt/ltebox/var/log/*Log.0 .', '\$', 5)
			self.command('zip mme.log.zip mmeLog.0 s1apcLog.0 s1apsLog.0 s11cLog.0 libLog.0 s1apCodecLog.0', '\$', 60)
		else:
			logging.error('This option should not occur!')
		self.close()

	def LogCollectSPGW(self):
		self.open(self.EPCIPAddress, self.EPCUserName, self.EPCPassword)
		self.command('cd ' + self.EPCSourceCodePath + '/scripts', '\$', 5)
		self.command('rm -f spgw.log.zip', '\$', 5)
		if re.match('OAI', self.EPCType, re.IGNORECASE) or re.match('OAI-Rel14-CUPS', self.EPCType, re.IGNORECASE):
			self.command('zip spgw.log.zip spgw*.log', '\$', 60)
			self.command('echo ' + self.EPCPassword + ' | sudo -S rm spgw*.log', '\$', 5)
		elif re.match('ltebox', self.EPCType, re.IGNORECASE):
			self.command('cp /opt/ltebox/var/log/xGwLog.0 .', '\$', 5)
			self.command('zip spgw.log.zip xGwLog.0', '\$', 60)
		else:
			logging.error('This option should not occur!')
		self.close()

	def LogCollectOAIUE(self):
		self.open(self.UEIPAddress, self.UEUserName, self.UEPassword)
		self.command('cd ' + self.UESourceCodePath, '\$', 5)
		self.command('cd cmake_targets', '\$', 5)
		self.command('echo ' + self.UEPassword + ' | sudo -S rm -f ue.log.zip', '\$', 5)
		self.command('echo ' + self.UEPassword + ' | sudo -S zip ue.log.zip ue*.log core* ue_*record.raw ue_*.pcap ue_*txt', '\$', 60)
		self.command('echo ' + self.UEPassword + ' | sudo -S rm ue*.log core* ue_*record.raw ue_*.pcap ue_*txt', '\$', 5)
		self.close()

	def RetrieveSystemVersion(self, machine):
		if self.eNBIPAddress == 'none' or self.UEIPAddress == 'none':
			self.OsVersion = 'Ubuntu 16.04.5 LTS'
			self.KernelVersion = '4.15.0-45-generic'
			self.UhdVersion = '3.13.0.1-0'
			self.UsrpBoard = 'B210'
			self.CpuNb = '4'
			self.CpuModel = 'Intel(R) Core(TM) i5-6200U'
			self.CpuMHz = '2399.996 MHz'
			return 0
		if machine == 'eNB':
			if self.eNBIPAddress != '' and self.eNBUserName != '' and self.eNBPassword != '':
				IPAddress = self.eNBIPAddress
				UserName = self.eNBUserName
				Password = self.eNBPassword
			else:
				return -1
		if machine == 'UE':
			if self.UEIPAddress != '' and self.UEUserName != '' and self.UEPassword != '':
				IPAddress = self.UEIPAddress
				UserName = self.UEUserName
				Password = self.UEPassword
			else:
				return -1

		self.open(IPAddress, UserName, Password)
		self.command('lsb_release -a', '\$', 5)
		result = re.search('Description:\\\\t(?P<os_type>[a-zA-Z0-9\-\_\.\ ]+)', str(self.ssh.before))
		if result is not None:
			self.OsVersion = result.group('os_type')
			logging.debug('OS is: ' + self.OsVersion)
		self.command('uname -r', '\$', 5)
		result = re.search('uname -r\\\\r\\\\n(?P<kernel_version>[a-zA-Z0-9\-\_\.]+)', str(self.ssh.before))
		if result is not None:
			self.KernelVersion = result.group('kernel_version')
			logging.debug('Kernel Version is: ' + self.KernelVersion)
		self.command('dpkg --list | egrep --color=never libuhd003', '\$', 5)
		result = re.search('libuhd003:amd64 *(?P<uhd_version>[0-9\.]+)', str(self.ssh.before))
		if result is not None:
			self.UhdVersion = result.group('uhd_version')
			logging.debug('UHD Version is: ' + self.UhdVersion)
		self.command('echo ' + Password + ' | sudo -S uhd_find_devices', '\$', 15)
		result = re.search('product: (?P<usrp_board>[0-9A-Za-z]+)\\\\r\\\\n', str(self.ssh.before))
		if result is not None:
			self.UsrpBoard = result.group('usrp_board')
			logging.debug('USRP Board  is: ' + self.UsrpBoard)
		self.command('lscpu', '\$', 5)
		result = re.search('CPU\(s\): *(?P<nb_cpus>[0-9]+).*Model name: *(?P<model>[a-zA-Z0-9\-\_\.\ \(\)]+).*CPU MHz: *(?P<cpu_mhz>[0-9\.]+)', str(self.ssh.before))
		if result is not None:
			self.CpuNb = result.group('nb_cpus')
			logging.debug('nb_cpus: ' + self.CpuNb)
			self.CpuModel = result.group('model')
			logging.debug('model: ' + self.CpuModel)
			self.CpuMHz = result.group('cpu_mhz') + ' MHz'
			logging.debug('cpu_mhz: ' + self.CpuMHz)
		self.close()

#-----------------------------------------------------------
# HTML Reporting....
#-----------------------------------------------------------
	def CreateHtmlHeader(self):
		if (not self.htmlHeaderCreated):
			logging.debug('\u001B[1m----------------------------------------\u001B[0m')
			logging.debug('\u001B[1m  Creating HTML header \u001B[0m')
			logging.debug('\u001B[1m----------------------------------------\u001B[0m')
			self.htmlFile = open('test_results.html', 'w')
			self.htmlFile.write('<!DOCTYPE html>\n')
			self.htmlFile.write('<html class="no-js" lang="en-US">\n')
			self.htmlFile.write('<head>\n')
			self.htmlFile.write('  <meta name="viewport" content="width=device-width, initial-scale=1">\n')
			self.htmlFile.write('  <link rel="stylesheet" href="https://maxcdn.bootstrapcdn.com/bootstrap/3.3.7/css/bootstrap.min.css">\n')
			self.htmlFile.write('  <script src="https://ajax.googleapis.com/ajax/libs/jquery/3.3.1/jquery.min.js"></script>\n')
			self.htmlFile.write('  <script src="https://maxcdn.bootstrapcdn.com/bootstrap/3.3.7/js/bootstrap.min.js"></script>\n')
			self.htmlFile.write('  <title>Test Results for TEMPLATE_JOB_NAME job build #TEMPLATE_BUILD_ID</title>\n')
			self.htmlFile.write('</head>\n')
			self.htmlFile.write('<body><div class="container">\n')
			self.htmlFile.write('  <br>\n')
			self.htmlFile.write('  <table style="border-collapse: collapse; border: none;">\n')
			self.htmlFile.write('    <tr style="border-collapse: collapse; border: none;">\n')
			self.htmlFile.write('      <td style="border-collapse: collapse; border: none;">\n')
			self.htmlFile.write('        <a href="http://www.openairinterface.org/">\n')
			self.htmlFile.write('           <img src="http://www.openairinterface.org/wp-content/uploads/2016/03/cropped-oai_final_logo2.png" alt="" border="none" height=50 width=150>\n')
			self.htmlFile.write('           </img>\n')
			self.htmlFile.write('        </a>\n')
			self.htmlFile.write('      </td>\n')
			self.htmlFile.write('      <td style="border-collapse: collapse; border: none; vertical-align: center;">\n')
			self.htmlFile.write('        <b><font size = "6">Job Summary -- Job: TEMPLATE_JOB_NAME -- Build-ID: TEMPLATE_BUILD_ID</font></b>\n')
			self.htmlFile.write('      </td>\n')
			self.htmlFile.write('    </tr>\n')
			self.htmlFile.write('  </table>\n')
			self.htmlFile.write('  <br>\n')
			self.htmlFile.write('  <div class="alert alert-info"><strong> <span class="glyphicon glyphicon-dashboard"></span> TEMPLATE_STAGE_NAME</strong></div>\n')
			self.htmlFile.write('  <table border = "1">\n')
			self.htmlFile.write('     <tr>\n')
			self.htmlFile.write('       <td bgcolor = "lightcyan" > <span class="glyphicon glyphicon-time"></span> Build Start Time (UTC) </td>\n')
			self.htmlFile.write('       <td>TEMPLATE_BUILD_TIME</td>\n')
			self.htmlFile.write('     </tr>\n')
			self.htmlFile.write('     <tr>\n')
			self.htmlFile.write('       <td bgcolor = "lightcyan" > <span class="glyphicon glyphicon-cloud-upload"></span> GIT Repository </td>\n')
			self.htmlFile.write('       <td><a href="' + self.ranRepository + '">' + self.ranRepository + '</a></td>\n')
			self.htmlFile.write('     </tr>\n')
			self.htmlFile.write('     <tr>\n')
			self.htmlFile.write('       <td bgcolor = "lightcyan" > <span class="glyphicon glyphicon-wrench"></span> Job Trigger </td>\n')
			if (self.ranAllowMerge):
				self.htmlFile.write('       <td>Merge-Request</td>\n')
			else:
				self.htmlFile.write('       <td>Push to Branch</td>\n')
			self.htmlFile.write('     </tr>\n')
			self.htmlFile.write('     <tr>\n')
			if (self.ranAllowMerge):
				self.htmlFile.write('       <td bgcolor = "lightcyan" > <span class="glyphicon glyphicon-log-out"></span> Source Branch </td>\n')
			else:
				self.htmlFile.write('       <td bgcolor = "lightcyan" > <span class="glyphicon glyphicon-tree-deciduous"></span> Branch</td>\n')
			self.htmlFile.write('       <td>' + self.ranBranch + '</td>\n')
			self.htmlFile.write('     </tr>\n')
			self.htmlFile.write('     <tr>\n')
			if (self.ranAllowMerge):
				self.htmlFile.write('       <td bgcolor = "lightcyan" > <span class="glyphicon glyphicon-tag"></span> Source Commit ID </td>\n')
			else:
				self.htmlFile.write('       <td bgcolor = "lightcyan" > <span class="glyphicon glyphicon-tag"></span> Commit ID </td>\n')
			self.htmlFile.write('       <td>' + self.ranCommitID + '</td>\n')
			self.htmlFile.write('     </tr>\n')
			if self.ranAllowMerge != '':
				commit_message = subprocess.check_output("git log -n1 --pretty=format:\"%s\" " + self.ranCommitID, shell=True, universal_newlines=True)
				commit_message = commit_message.strip()
				self.htmlFile.write('     <tr>\n')
				if (self.ranAllowMerge):
					self.htmlFile.write('       <td bgcolor = "lightcyan" > <span class="glyphicon glyphicon-comment"></span> Source Commit Message </td>\n')
				else:
					self.htmlFile.write('       <td bgcolor = "lightcyan" > <span class="glyphicon glyphicon-comment"></span> Commit Message </td>\n')
				self.htmlFile.write('       <td>' + commit_message + '</td>\n')
				self.htmlFile.write('     </tr>\n')
			if (self.ranAllowMerge):
				self.htmlFile.write('     <tr>\n')
				self.htmlFile.write('       <td bgcolor = "lightcyan" > <span class="glyphicon glyphicon-log-in"></span> Target Branch </td>\n')
				if (self.ranTargetBranch == ''):
					self.htmlFile.write('       <td>develop</td>\n')
				else:
					self.htmlFile.write('       <td>' + self.ranTargetBranch + '</td>\n')
				self.htmlFile.write('     </tr>\n')
			self.htmlFile.write('  </table>\n')

			if (self.ADBIPAddress != 'none'):
				terminate_ue_flag = True
				self.GetAllUEDevices(terminate_ue_flag)
				self.GetAllCatMDevices(terminate_ue_flag)
				self.htmlUEConnected = len(self.UEDevices)
				self.htmlFile.write('  <h2><span class="glyphicon glyphicon-phone"></span> <span class="glyphicon glyphicon-menu-right"></span> ' + str(len(self.UEDevices)) + ' UE(s) is(are) connected to ADB bench server</h2>\n')
				self.htmlFile.write('  <h2><span class="glyphicon glyphicon-phone"></span> <span class="glyphicon glyphicon-menu-right"></span> ' + str(len(self.CatMDevices)) + ' CAT-M UE(s) is(are) connected to bench server</h2>\n')
			else:
				self.UEDevices.append('OAI-UE')
				self.htmlUEConnected = len(self.UEDevices)
				self.htmlFile.write('  <h2><span class="glyphicon glyphicon-phone"></span> <span class="glyphicon glyphicon-menu-right"></span> ' + str(len(self.UEDevices)) + ' OAI UE(s) is(are) connected to CI bench</h2>\n')
			self.htmlFile.write('  <br>\n')
			self.htmlFile.write('  <ul class="nav nav-pills">\n')
			count = 0
			while (count < self.nbTestXMLfiles):
				pillMsg = '    <li><a data-toggle="pill" href="#'
				pillMsg += self.htmlTabRefs[count]
				pillMsg += '">'
				pillMsg += '__STATE_' + self.htmlTabNames[count] + '__'
				pillMsg += self.htmlTabNames[count]
				pillMsg += ' <span class="glyphicon glyphicon-'
				pillMsg += self.htmlTabIcons[count]
				pillMsg += '"></span></a></li>\n'
				self.htmlFile.write(pillMsg)
				count += 1
			self.htmlFile.write('  </ul>\n')
			self.htmlFile.write('  <div class="tab-content">\n')
			self.htmlFile.close()

	def CreateHtmlTabHeader(self):
		if (not self.htmlHeaderCreated):
			if (not os.path.isfile('test_results.html')):
				self.CreateHtmlHeader()
			self.htmlFile = open('test_results.html', 'a')
			if (self.nbTestXMLfiles == 1):
				self.htmlFile.write('  <div id="' + self.htmlTabRefs[0] + '" class="tab-pane fade">\n')
				self.htmlFile.write('  <h3>Test Summary for <span class="glyphicon glyphicon-file"></span> ' + self.testXMLfiles[0] + '</h3>\n')
			else:
				self.htmlFile.write('  <div id="build-tab" class="tab-pane fade">\n')
			self.htmlFile.write('  <table class="table" border = "1">\n')
			self.htmlFile.write('      <tr bgcolor = "#33CCFF" >\n')
			self.htmlFile.write('        <th>Relative Time (ms)</th>\n')
			self.htmlFile.write('        <th>Test Id</th>\n')
			self.htmlFile.write('        <th>Test Desc</th>\n')
			self.htmlFile.write('        <th>Test Options</th>\n')
			self.htmlFile.write('        <th>Test Status</th>\n')
			if (self.htmlUEConnected == -1):
				terminate_ue_flag = True
				if (self.ADBIPAddress != 'none'):
					self.GetAllUEDevices(terminate_ue_flag)
					self.GetAllCatMDevices(terminate_ue_flag)
				else:
					self.UEDevices.append('OAI-UE')
				self.htmlUEConnected = len(self.UEDevices)

			i = 0
			while (i < self.htmlUEConnected):
				self.htmlFile.write('        <th>UE' + str(i) + ' Status</th>\n')
				i += 1
			self.htmlFile.write('      </tr>\n')
		self.htmlHeaderCreated = True

	def CreateHtmlTabFooter(self, passStatus):
		if ((not self.htmlFooterCreated) and (self.htmlHeaderCreated)):
			self.htmlFile.write('      <tr>\n')
			self.htmlFile.write('        <th bgcolor = "#33CCFF" colspan=3>Final Tab Status</th>\n')
			if passStatus:
				self.htmlFile.write('        <th bgcolor = "green" colspan=' + str(2 + self.htmlUEConnected) + '><font color="white">PASS <span class="glyphicon glyphicon-ok"></span> </font></th>\n')
			else:
				self.htmlFile.write('        <th bgcolor = "red" colspan=' + str(2 + self.htmlUEConnected) + '><font color="white">FAIL <span class="glyphicon glyphicon-remove"></span> </font></th>\n')
			self.htmlFile.write('      </tr>\n')
			self.htmlFile.write('  </table>\n')
			self.htmlFile.write('  </div>\n')
			self.htmlFile.close()
			time.sleep(1)
			if passStatus:
				cmd = "sed -i -e 's/__STATE_" + self.htmlTabNames[0] + "__//' test_results.html"
				subprocess.run(cmd, shell=True)
			else:
				cmd = "sed -i -e 's/__STATE_" + self.htmlTabNames[0] + "__/<span class=\"glyphicon glyphicon-remove\"><\/span>/' test_results.html"
				subprocess.run(cmd, shell=True)
		self.htmlFooterCreated = False

	def CreateHtmlFooter(self, passStatus):
		if (os.path.isfile('test_results.html')):
			logging.debug('\u001B[1m----------------------------------------\u001B[0m')
			logging.debug('\u001B[1m  Creating HTML footer \u001B[0m')
			logging.debug('\u001B[1m----------------------------------------\u001B[0m')

			self.htmlFile = open('test_results.html', 'a')
			self.htmlFile.write('</div>\n')
			self.htmlFile.write('  <p></p>\n')
			self.htmlFile.write('  <table class="table table-condensed">\n')

			machines = [ 'eNB', 'UE' ]
			for machine in machines:
				res = self.RetrieveSystemVersion(machine)
				if res == -1:
					continue
				self.htmlFile.write('      <tr>\n')
				self.htmlFile.write('        <th colspan=8>' + str(machine) + ' Server Characteristics</th>\n')
				self.htmlFile.write('      </tr>\n')
				self.htmlFile.write('      <tr>\n')
				self.htmlFile.write('        <td>OS Version</td>\n')
				self.htmlFile.write('        <td><span class="label label-default">' + self.OsVersion + '</span></td>\n')
				self.htmlFile.write('        <td>Kernel Version</td>\n')
				self.htmlFile.write('        <td><span class="label label-default">' + self.KernelVersion + '</span></td>\n')
				self.htmlFile.write('        <td>UHD Version</td>\n')
				self.htmlFile.write('        <td><span class="label label-default">' + self.UhdVersion + '</span></td>\n')
				self.htmlFile.write('        <td>USRP Board</td>\n')
				self.htmlFile.write('        <td><span class="label label-default">' + self.UsrpBoard + '</span></td>\n')
				self.htmlFile.write('      </tr>\n')
				self.htmlFile.write('      <tr>\n')
				self.htmlFile.write('        <td>Nb CPUs</td>\n')
				self.htmlFile.write('        <td><span class="label label-default">' + self.CpuNb + '</span></td>\n')
				self.htmlFile.write('        <td>CPU Model Name</td>\n')
				self.htmlFile.write('        <td><span class="label label-default">' + self.CpuModel + '</span></td>\n')
				self.htmlFile.write('        <td>CPU Frequency</td>\n')
				self.htmlFile.write('        <td><span class="label label-default">' + self.CpuMHz + '</span></td>\n')
				self.htmlFile.write('        <td></td>\n')
				self.htmlFile.write('        <td></td>\n')
				self.htmlFile.write('      </tr>\n')
			self.htmlFile.write('      <tr>\n')
			self.htmlFile.write('        <th colspan=5 bgcolor = "#33CCFF">Final Status</th>\n')
			if passStatus:
				self.htmlFile.write('        <th colspan=3 bgcolor="green"><font color="white">PASS <span class="glyphicon glyphicon-ok"></span></font></th>\n')
			else:
				self.htmlFile.write('        <th colspan=3 bgcolor="red"><font color="white">FAIL <span class="glyphicon glyphicon-remove"></span> </font></th>\n')
			self.htmlFile.write('      </tr>\n')
			self.htmlFile.write('  </table>\n')
			self.htmlFile.write('  <p></p>\n')
			self.htmlFile.write('  <div class="well well-lg">End of Test Report -- Copyright <span class="glyphicon glyphicon-copyright-mark"></span> 2018 <a href="http://www.openairinterface.org/">OpenAirInterface</a>. All Rights Reserved.</div>\n')
			self.htmlFile.write('</div></body>\n')
			self.htmlFile.write('</html>\n')
			self.htmlFile.close()

	def CreateHtmlTestRow(self, options, status, processesStatus, machine='eNB'):
		if ((not self.htmlFooterCreated) and (self.htmlHeaderCreated)):
			currentTime = int(round(time.time() * 1000)) - self.startTime
			self.htmlFile.write('      <tr>\n')
			self.htmlFile.write('        <td bgcolor = "lightcyan" >' + format(currentTime / 1000, '.1f') + '</td>\n')
			self.htmlFile.write('        <td bgcolor = "lightcyan" >' + self.testCase_id  + '</td>\n')
			self.htmlFile.write('        <td>' + self.desc  + '</td>\n')
			self.htmlFile.write('        <td>' + str(options)  + '</td>\n')
			if (str(status) == 'OK'):
				self.htmlFile.write('        <td bgcolor = "lightgreen" >' + str(status)  + '</td>\n')
			elif (str(status) == 'KO'):
				if (processesStatus == 0):
					self.htmlFile.write('        <td bgcolor = "lightcoral" >' + str(status)  + '</td>\n')
				elif (processesStatus == ENB_PROCESS_FAILED):
					self.htmlFile.write('        <td bgcolor = "lightcoral" >KO - eNB process not found</td>\n')
				elif (processesStatus == OAI_UE_PROCESS_FAILED):
					self.htmlFile.write('        <td bgcolor = "lightcoral" >KO - OAI UE process not found</td>\n')
				elif (processesStatus == ENB_PROCESS_SEG_FAULT):
					self.htmlFile.write('        <td bgcolor = "lightcoral" >KO - ' + machine + ' process ended in Segmentation Fault</td>\n')
				elif (processesStatus == ENB_PROCESS_ASSERTION) or (processesStatus == OAI_UE_PROCESS_ASSERTION):
					self.htmlFile.write('        <td bgcolor = "lightcoral" >KO - ' + machine + ' process ended in Assertion</td>\n')
				elif (processesStatus == ENB_PROCESS_REALTIME_ISSUE):
					self.htmlFile.write('        <td bgcolor = "lightcoral" >KO - ' + machine + ' process faced Real Time issue(s)</td>\n')
				elif (processesStatus == ENB_PROCESS_NOLOGFILE_TO_ANALYZE) or (processesStatus == OAI_UE_PROCESS_NOLOGFILE_TO_ANALYZE):
					self.htmlFile.write('        <td bgcolor = "orange" >OK?</td>\n')
				elif (processesStatus == ENB_PROCESS_SLAVE_RRU_NOT_SYNCED):
					self.htmlFile.write('        <td bgcolor = "lightcoral" >KO - ' + machine + ' Slave RRU could not synch</td>\n')
				elif (processesStatus == OAI_UE_PROCESS_COULD_NOT_SYNC):
					self.htmlFile.write('        <td bgcolor = "lightcoral" >KO - UE could not sync</td>\n')
				elif (processesStatus == HSS_PROCESS_FAILED):
					self.htmlFile.write('        <td bgcolor = "lightcoral" >KO - HSS process not found</td>\n')
				elif (processesStatus == MME_PROCESS_FAILED):
					self.htmlFile.write('        <td bgcolor = "lightcoral" >KO - MME process not found</td>\n')
				elif (processesStatus == SPGW_PROCESS_FAILED):
					self.htmlFile.write('        <td bgcolor = "lightcoral" >KO - SPGW process not found</td>\n')
				elif (processesStatus == UE_IP_ADDRESS_ISSUE):
					self.htmlFile.write('        <td bgcolor = "lightcoral" >KO - Could not retrieve UE IP address</td>\n')
				else:
					self.htmlFile.write('        <td bgcolor = "lightcoral" >' + str(status)  + '</td>\n')
			else:
				self.htmlFile.write('        <td bgcolor = "orange" >' + str(status)  + '</td>\n')
			if (len(str(self.htmleNBFailureMsg)) > 2):
				cellBgColor = 'white'
				result = re.search('ended with|faced real time issues', self.htmleNBFailureMsg)
				if result is not None:
					cellBgColor = 'red'
				else:
					result = re.search('showed|Reestablishment|Could not copy eNB logfile', self.htmleNBFailureMsg)
					if result is not None:
						cellBgColor = 'orange'
				self.htmlFile.write('        <td bgcolor = "' + cellBgColor + '" colspan=' + str(self.htmlUEConnected) + '><pre style="background-color:' + cellBgColor + '">' + self.htmleNBFailureMsg + '</pre></td>\n')
				self.htmleNBFailureMsg = ''
			elif (len(str(self.htmlUEFailureMsg)) > 2):
				cellBgColor = 'white'
				result = re.search('ended with|faced real time issues', self.htmlUEFailureMsg)
				if result is not None:
					cellBgColor = 'red'
				else:
					result = re.search('showed|Could not copy UE logfile|oaitun_ue1 interface is either NOT mounted or NOT configured', self.htmlUEFailureMsg)
					if result is not None:
						cellBgColor = 'orange'
				self.htmlFile.write('        <td bgcolor = "' + cellBgColor + '" colspan=' + str(self.htmlUEConnected) + '><pre style="background-color:' + cellBgColor + '">' + self.htmlUEFailureMsg + '</pre></td>\n')
				self.htmlUEFailureMsg = ''
			else:
				i = 0
				while (i < self.htmlUEConnected):
					self.htmlFile.write('        <td>-</td>\n')
					i += 1
			self.htmlFile.write('      </tr>\n')

	def CreateHtmlTestRowQueue(self, options, status, ue_status, ue_queue):
		if ((not self.htmlFooterCreated) and (self.htmlHeaderCreated)):
			currentTime = int(round(time.time() * 1000)) - self.startTime
			addOrangeBK = False
			self.htmlFile.write('      <tr>\n')
			self.htmlFile.write('        <td bgcolor = "lightcyan" >' + format(currentTime / 1000, '.1f') + '</td>\n')
			self.htmlFile.write('        <td bgcolor = "lightcyan" >' + self.testCase_id  + '</td>\n')
			self.htmlFile.write('        <td>' + self.desc  + '</td>\n')
			self.htmlFile.write('        <td>' + str(options)  + '</td>\n')
			if (str(status) == 'OK'):
				self.htmlFile.write('        <td bgcolor = "lightgreen" >' + str(status)  + '</td>\n')
			elif (str(status) == 'KO'):
				self.htmlFile.write('        <td bgcolor = "lightcoral" >' + str(status)  + '</td>\n')
			else:
				addOrangeBK = True
				self.htmlFile.write('        <td bgcolor = "orange" >' + str(status)  + '</td>\n')
			i = 0
			while (i < self.htmlUEConnected):
				if (i < ue_status):
					if (not ue_queue.empty()):
						if (addOrangeBK):
							self.htmlFile.write('        <td bgcolor = "orange" >' + str(ue_queue.get()).replace('white', 'orange') + '</td>\n')
						else:
							self.htmlFile.write('        <td>' + str(ue_queue.get()) + '</td>\n')
					else:
						self.htmlFile.write('        <td>-</td>\n')
				else:
					self.htmlFile.write('        <td>-</td>\n')
				i += 1
			self.htmlFile.write('      </tr>\n')

#-----------------------------------------------------------
# ShowTestID()
#-----------------------------------------------------------
	def ShowTestID(self):
		logging.debug('\u001B[1m----------------------------------------\u001B[0m')
		logging.debug('\u001B[1mTest ID:' + self.testCase_id + '\u001B[0m')
		logging.debug('\u001B[1m' + self.desc + '\u001B[0m')
		logging.debug('\u001B[1m----------------------------------------\u001B[0m')

#-----------------------------------------------------------
# Usage()
#-----------------------------------------------------------
def Usage():
	print('----------------------------------------------------------------------------------------------------------------------')
	print('main.py Ver:' + Version)
	print('----------------------------------------------------------------------------------------------------------------------')
	print('Usage: python main.py [options]')
	print('  --help  Show this help.')
	print('  --mode=[Mode]')
	print('      TesteNB')
	print('      InitiateHtml, FinalizeHtml')
	print('      TerminateeNB, TerminateUE, TerminateHSS, TerminateMME, TerminateSPGW')
	print('      LogCollectBuild, LogCollecteNB, LogCollectHSS, LogCollectMME, LogCollectSPGW, LogCollectPing, LogCollectIperf')
	print('---------------------------------------------------------------------------------------------------- Git Options --')
	print('  --ranRepository=[OAI RAN Repository URL]')
	print('  --ranBranch=[OAI RAN Repository Branch]')
	print('  --ranCommitID=[OAI RAN Repository Commit SHA-1]')
	print('  --ranAllowMerge=[Allow Merge Request (with target branch) (true or false)]')
	print('  --ranTargetBranch=[Target Branch in case of a Merge Request]')
	print('--------------------------------------------------------------------------------------------- eNB Server Options --')
	print('  --eNBIPAddress=[eNB\'s IP Address]')
	print('  --eNBUserName=[eNB\'s Login User Name]')
	print('  --eNBPassword=[eNB\'s Login Password]')
	print('  --eNBSourceCodePath=[eNB\'s Source Code Path]')
	print('------------------------------------------------------------------------------------------ OAI UE Server Options --')
	print('  --UEIPAddress=[UE\'s IP Address]')
	print('  --UEUserName=[UE\'s Login User Name]')
	print('  --UEPassword=[UE\'s Login Password]')
	print('  --UESourceCodePath=[UE\'s Source Code Path]')
	print('--------------------------------------------------------------------------------------------- EPC Server Options --')
	print('  --EPCIPAddress=[EPC\'s IP Address]')
	print('  --EPCUserName=[EPC\'s Login User Name]')
	print('  --EPCPassword=[EPC\'s Login Password]')
	print('  --EPCSourceCodePath=[EPC\'s Source Code Path]')
	print('  --EPCType=[EPC\'s Type: OAI or ltebox or OAI-Rel14-CUPS]')
	print('--------------------------------------------------------------------------------------------- ABD Server Options --')
	print('  --ADBIPAddress=[ADB\'s IP Address]')
	print('  --ADBUserName=[ADB\'s Login User Name]')
	print('  --ADBPassword=[ADB\'s Login Password]')
	print('----------------------------------------------------------------------------------------------------------------------')
	print('  --XMLTestFile=[XML Test File to be run]')
	print('----------------------------------------------------------------------------------------------------------------------')

def CheckClassValidity(action,id):
	if action != 'Build_eNB' and action != 'WaitEndBuild_eNB' and action != 'Initialize_eNB' and action != 'Terminate_eNB' and action != 'Initialize_UE' and action != 'Terminate_UE' and action != 'Attach_UE' and action != 'Detach_UE' and action != 'Build_OAI_UE' and action != 'Initialize_OAI_UE' and action != 'Terminate_OAI_UE' and action != 'DataDisable_UE' and action != 'DataEnable_UE' and action != 'CheckStatusUE' and action != 'Ping' and action != 'Iperf' and action != 'Reboot_UE' and action != 'Initialize_FlexranCtrl' and action != 'Terminate_FlexranCtrl' and action != 'Initialize_HSS' and action != 'Terminate_HSS' and action != 'Initialize_MME' and action != 'Terminate_MME' and action != 'Initialize_SPGW' and action != 'Terminate_SPGW' and action != 'Initialize_CatM_module' and action != 'Terminate_CatM_module' and action != 'Attach_CatM_module' and action != 'Detach_CatM_module' and action != 'Ping_CatM_module' and action != 'IdleSleep' and action != 'Perform_X2_Handover':
		logging.debug('ERROR: test-case ' + id + ' has wrong class ' + action)
		return False
	return True

def GetParametersFromXML(action):
	if action == 'Build_eNB':
		SSH.Build_eNB_args = test.findtext('Build_eNB_args')
		SSH.eNB_instance = test.findtext('eNB_instance')
		if (SSH.eNB_instance is None):
			SSH.eNB_instance = '0'
		SSH.eNB_serverId = test.findtext('eNB_serverId')
		if (SSH.eNB_serverId is None):
			SSH.eNB_serverId = '0'
		xmlBgBuildField = test.findtext('backgroundBuild')
		if (xmlBgBuildField is None):
			SSH.backgroundBuild = False
		else:
			if re.match('true', xmlBgBuildField, re.IGNORECASE):
				SSH.backgroundBuild = True
			else:
				SSH.backgroundBuild = False

	if action == 'WaitEndBuild_eNB':
		SSH.Build_eNB_args = test.findtext('Build_eNB_args')
		SSH.eNB_instance = test.findtext('eNB_instance')
		if (SSH.eNB_instance is None):
			SSH.eNB_instance = '0'
		SSH.eNB_serverId = test.findtext('eNB_serverId')
		if (SSH.eNB_serverId is None):
			SSH.eNB_serverId = '0'

	if action == 'Initialize_eNB':
		SSH.Initialize_eNB_args = test.findtext('Initialize_eNB_args')
		SSH.eNB_instance = test.findtext('eNB_instance')
		if (SSH.eNB_instance is None):
			SSH.eNB_instance = '0'
		SSH.eNB_serverId = test.findtext('eNB_serverId')
		if (SSH.eNB_serverId is None):
			SSH.eNB_serverId = '0'

	if action == 'Terminate_eNB':
		SSH.eNB_instance = test.findtext('eNB_instance')
		if (SSH.eNB_instance is None):
			SSH.eNB_instance = '0'
		SSH.eNB_serverId = test.findtext('eNB_serverId')
		if (SSH.eNB_serverId is None):
			SSH.eNB_serverId = '0'

	if action == 'Attach_UE':
		nbMaxUEtoAttach = test.findtext('nbMaxUEtoAttach')
		if (nbMaxUEtoAttach is None):
			SSH.nbMaxUEtoAttach = -1
		else:
			SSH.nbMaxUEtoAttach = int(nbMaxUEtoAttach)

	if action == 'CheckStatusUE':
		expectedNBUE = test.findtext('expectedNbOfConnectedUEs')
		if (expectedNBUE is None):
			SSH.expectedNbOfConnectedUEs = -1
		else:
			SSH.expectedNbOfConnectedUEs = int(expectedNBUE)

	if action == 'Build_OAI_UE':
		SSH.Build_OAI_UE_args = test.findtext('Build_OAI_UE_args')

	if action == 'Initialize_OAI_UE':
		SSH.Initialize_OAI_UE_args = test.findtext('Initialize_OAI_UE_args')
		SSH.UE_instance = test.findtext('UE_instance')
		if (SSH.UE_instance is None):
			SSH.UE_instance = '0'

	if action == 'Terminate_OAI_UE':
		SSH.eNB_instance = test.findtext('UE_instance')
		if (SSH.UE_instance is None):
			SSH.UE_instance = '0'

	if action == 'Ping' or action == 'Ping_CatM_module':
		SSH.ping_args = test.findtext('ping_args')
		SSH.ping_packetloss_threshold = test.findtext('ping_packetloss_threshold')

	if action == 'Iperf':
		SSH.iperf_args = test.findtext('iperf_args')
		SSH.iperf_packetloss_threshold = test.findtext('iperf_packetloss_threshold')
		SSH.iperf_profile = test.findtext('iperf_profile')
		if (SSH.iperf_profile is None):
			SSH.iperf_profile = 'balanced'
		else:
			if SSH.iperf_profile != 'balanced' and SSH.iperf_profile != 'unbalanced' and SSH.iperf_profile != 'single-ue':
				logging.debug('ERROR: test-case has wrong profile ' + SSH.iperf_profile)
				SSH.iperf_profile = 'balanced'
		SSH.iperf_options = test.findtext('iperf_options')
		if (SSH.iperf_options is None):
			SSH.iperf_options = 'check'
		else:
			if SSH.iperf_options != 'check' and SSH.iperf_options != 'sink':
				logging.debug('ERROR: test-case has wrong option ' + SSH.iperf_options)
				SSH.iperf_options = 'check'

	if action == 'IdleSleep':
		string_field = test.findtext('idle_sleep_time_in_sec')
		if (string_field is None):
			SSH.idle_sleep_time = 5
		else:
			SSH.idle_sleep_time = int(string_field)

	if action == 'Perform_X2_Handover':
		string_field = test.findtext('x2_ho_options')
		if (string_field is None):
			SSH.x2_ho_options = 'network'
		else:
			if string_field != 'network':
				logging.error('ERROR: test-case has wrong option ' + string_field)
				SSH.x2_ho_options = 'network'
			else:
				SSH.x2_ho_options = string_field


#check if given test is in list
#it is in list if one of the strings in 'list' is at the beginning of 'test'
def test_in_list(test, list):
	for check in list:
		check=check.replace('+','')
		if (test.startswith(check)):
			return True
	return False

def receive_signal(signum, frame):
	sys.exit(1)

#-----------------------------------------------------------
# Parameter Check
#-----------------------------------------------------------
mode = ''
SSH = SSHConnection()

argvs = sys.argv
argc = len(argvs)
cwd = os.getcwd()

while len(argvs) > 1:
	myArgv = argvs.pop(1)	# 0th is this file's name
	if re.match('^\-\-help$', myArgv, re.IGNORECASE):
		Usage()
		sys.exit(0)
	elif re.match('^\-\-mode=(.+)$', myArgv, re.IGNORECASE):
		matchReg = re.match('^\-\-mode=(.+)$', myArgv, re.IGNORECASE)
		mode = matchReg.group(1)
	elif re.match('^\-\-eNBRepository=(.+)$|^\-\-ranRepository(.+)$', myArgv, re.IGNORECASE):
		if re.match('^\-\-eNBRepository=(.+)$', myArgv, re.IGNORECASE):
			matchReg = re.match('^\-\-eNBRepository=(.+)$', myArgv, re.IGNORECASE)
		else:
			matchReg = re.match('^\-\-ranRepository=(.+)$', myArgv, re.IGNORECASE)
		SSH.ranRepository = matchReg.group(1)
	elif re.match('^\-\-eNB_AllowMerge=(.+)$|^\-\-ranAllowMerge=(.+)$', myArgv, re.IGNORECASE):
		if re.match('^\-\-eNB_AllowMerge=(.+)$', myArgv, re.IGNORECASE):
			matchReg = re.match('^\-\-eNB_AllowMerge=(.+)$', myArgv, re.IGNORECASE)
		else:
			matchReg = re.match('^\-\-ranAllowMerge=(.+)$', myArgv, re.IGNORECASE)
		doMerge = matchReg.group(1)
		if ((doMerge == 'true') or (doMerge == 'True')):
			SSH.ranAllowMerge = True
	elif re.match('^\-\-eNBBranch=(.+)$|^\-\-ranBranch=(.+)$', myArgv, re.IGNORECASE):
		if re.match('^\-\-eNBBranch=(.+)$', myArgv, re.IGNORECASE):
			matchReg = re.match('^\-\-eNBBranch=(.+)$', myArgv, re.IGNORECASE)
		else:
			matchReg = re.match('^\-\-ranBranch=(.+)$', myArgv, re.IGNORECASE)
		SSH.ranBranch = matchReg.group(1)
	elif re.match('^\-\-eNBCommitID=(.*)$|^\-\-ranCommitID=(.*)$', myArgv, re.IGNORECASE):
		if re.match('^\-\-eNBCommitID=(.*)$', myArgv, re.IGNORECASE):
			matchReg = re.match('^\-\-eNBCommitID=(.*)$', myArgv, re.IGNORECASE)
		else:
			matchReg = re.match('^\-\-ranCommitID=(.*)$', myArgv, re.IGNORECASE)
		SSH.ranCommitID = matchReg.group(1)
	elif re.match('^\-\-eNBTargetBranch=(.*)$|^\-\-ranTargetBranch=(.*)$', myArgv, re.IGNORECASE):
		if re.match('^\-\-eNBTargetBranch=(.*)$', myArgv, re.IGNORECASE):
			matchReg = re.match('^\-\-eNBTargetBranch=(.*)$', myArgv, re.IGNORECASE)
		else:
			matchReg = re.match('^\-\-ranTargetBranch=(.*)$', myArgv, re.IGNORECASE)
		SSH.ranTargetBranch = matchReg.group(1)
	elif re.match('^\-\-eNBIPAddress=(.+)$|^\-\-eNB[1-2]IPAddress=(.+)$', myArgv, re.IGNORECASE):
		if re.match('^\-\-eNBIPAddress=(.+)$', myArgv, re.IGNORECASE):
			matchReg = re.match('^\-\-eNBIPAddress=(.+)$', myArgv, re.IGNORECASE)
			SSH.eNBIPAddress = matchReg.group(1)
		elif re.match('^\-\-eNB1IPAddress=(.+)$', myArgv, re.IGNORECASE):
			matchReg = re.match('^\-\-eNB1IPAddress=(.+)$', myArgv, re.IGNORECASE)
			SSH.eNB1IPAddress = matchReg.group(1)
		elif re.match('^\-\-eNB2IPAddress=(.+)$', myArgv, re.IGNORECASE):
			matchReg = re.match('^\-\-eNB2IPAddress=(.+)$', myArgv, re.IGNORECASE)
			SSH.eNB2IPAddress = matchReg.group(1)
	elif re.match('^\-\-eNBUserName=(.+)$|^\-\-eNB[1-2]UserName=(.+)$', myArgv, re.IGNORECASE):
		if re.match('^\-\-eNBUserName=(.+)$', myArgv, re.IGNORECASE):
			matchReg = re.match('^\-\-eNBUserName=(.+)$', myArgv, re.IGNORECASE)
			SSH.eNBUserName = matchReg.group(1)
		elif re.match('^\-\-eNB1UserName=(.+)$', myArgv, re.IGNORECASE):
			matchReg = re.match('^\-\-eNB1UserName=(.+)$', myArgv, re.IGNORECASE)
			SSH.eNB1UserName = matchReg.group(1)
		elif re.match('^\-\-eNB2UserName=(.+)$', myArgv, re.IGNORECASE):
			matchReg = re.match('^\-\-eNB2UserName=(.+)$', myArgv, re.IGNORECASE)
			SSH.eNB2UserName = matchReg.group(1)
	elif re.match('^\-\-eNBPassword=(.+)$|^\-\-eNB[1-2]Password=(.+)$', myArgv, re.IGNORECASE):
		if re.match('^\-\-eNBPassword=(.+)$', myArgv, re.IGNORECASE):
			matchReg = re.match('^\-\-eNBPassword=(.+)$', myArgv, re.IGNORECASE)
			SSH.eNBPassword = matchReg.group(1)
		elif re.match('^\-\-eNB1Password=(.+)$', myArgv, re.IGNORECASE):
			matchReg = re.match('^\-\-eNB1Password=(.+)$', myArgv, re.IGNORECASE)
			SSH.eNB1Password = matchReg.group(1)
		elif re.match('^\-\-eNB2Password=(.+)$', myArgv, re.IGNORECASE):
			matchReg = re.match('^\-\-eNB2Password=(.+)$', myArgv, re.IGNORECASE)
			SSH.eNB2Password = matchReg.group(1)
	elif re.match('^\-\-eNBSourceCodePath=(.+)$|^\-\-eNB[1-2]SourceCodePath=(.+)$', myArgv, re.IGNORECASE):
		if re.match('^\-\-eNBSourceCodePath=(.+)$', myArgv, re.IGNORECASE):
			matchReg = re.match('^\-\-eNBSourceCodePath=(.+)$', myArgv, re.IGNORECASE)
			SSH.eNBSourceCodePath = matchReg.group(1)
		elif re.match('^\-\-eNB1SourceCodePath=(.+)$', myArgv, re.IGNORECASE):
			matchReg = re.match('^\-\-eNB1SourceCodePath=(.+)$', myArgv, re.IGNORECASE)
			SSH.eNB1SourceCodePath = matchReg.group(1)
		elif re.match('^\-\-eNB2SourceCodePath=(.+)$', myArgv, re.IGNORECASE):
			matchReg = re.match('^\-\-eNB2SourceCodePath=(.+)$', myArgv, re.IGNORECASE)
			SSH.eNB2SourceCodePath = matchReg.group(1)
	elif re.match('^\-\-EPCIPAddress=(.+)$', myArgv, re.IGNORECASE):
		matchReg = re.match('^\-\-EPCIPAddress=(.+)$', myArgv, re.IGNORECASE)
		SSH.EPCIPAddress = matchReg.group(1)
	elif re.match('^\-\-EPCBranch=(.+)$', myArgv, re.IGNORECASE):
		matchReg = re.match('^\-\-EPCBranch=(.+)$', myArgv, re.IGNORECASE)
		SSH.EPCBranch = matchReg.group(1)
	elif re.match('^\-\-EPCUserName=(.+)$', myArgv, re.IGNORECASE):
		matchReg = re.match('^\-\-EPCUserName=(.+)$', myArgv, re.IGNORECASE)
		SSH.EPCUserName = matchReg.group(1)
	elif re.match('^\-\-EPCPassword=(.+)$', myArgv, re.IGNORECASE):
		matchReg = re.match('^\-\-EPCPassword=(.+)$', myArgv, re.IGNORECASE)
		SSH.EPCPassword = matchReg.group(1)
	elif re.match('^\-\-EPCSourceCodePath=(.+)$', myArgv, re.IGNORECASE):
		matchReg = re.match('^\-\-EPCSourceCodePath=(.+)$', myArgv, re.IGNORECASE)
		SSH.EPCSourceCodePath = matchReg.group(1)
	elif re.match('^\-\-EPCType=(.+)$', myArgv, re.IGNORECASE):
		matchReg = re.match('^\-\-EPCType=(.+)$', myArgv, re.IGNORECASE)
		if re.match('OAI', matchReg.group(1), re.IGNORECASE) or re.match('ltebox', matchReg.group(1), re.IGNORECASE) or re.match('OAI-Rel14-CUPS', matchReg.group(1), re.IGNORECASE):
			SSH.EPCType = matchReg.group(1)
		else:
			sys.exit('Invalid EPC Type: ' + matchReg.group(1) + ' -- (should be OAI or ltebox or OAI-Rel14-CUPS)')
	elif re.match('^\-\-ADBIPAddress=(.+)$', myArgv, re.IGNORECASE):
		matchReg = re.match('^\-\-ADBIPAddress=(.+)$', myArgv, re.IGNORECASE)
		SSH.ADBIPAddress = matchReg.group(1)
	elif re.match('^\-\-ADBUserName=(.+)$', myArgv, re.IGNORECASE):
		matchReg = re.match('^\-\-ADBUserName=(.+)$', myArgv, re.IGNORECASE)
		SSH.ADBUserName = matchReg.group(1)
	elif re.match('^\-\-ADBType=(.+)$', myArgv, re.IGNORECASE):
		matchReg = re.match('^\-\-ADBType=(.+)$', myArgv, re.IGNORECASE)
		if re.match('centralized', matchReg.group(1), re.IGNORECASE) or re.match('distributed', matchReg.group(1), re.IGNORECASE):
			if re.match('distributed', matchReg.group(1), re.IGNORECASE):
				SSH.ADBCentralized = False
			else:
				SSH.ADBCentralized = True
		else:
			sys.exit('Invalid ADB Type: ' + matchReg.group(1) + ' -- (should be centralized or distributed)')
	elif re.match('^\-\-ADBPassword=(.+)$', myArgv, re.IGNORECASE):
		matchReg = re.match('^\-\-ADBPassword=(.+)$', myArgv, re.IGNORECASE)
		SSH.ADBPassword = matchReg.group(1)
	elif re.match('^\-\-XMLTestFile=(.+)$', myArgv, re.IGNORECASE):
		matchReg = re.match('^\-\-XMLTestFile=(.+)$', myArgv, re.IGNORECASE)
		SSH.testXMLfiles.append(matchReg.group(1))
		SSH.nbTestXMLfiles += 1
	elif re.match('^\-\-UEIPAddress=(.+)$', myArgv, re.IGNORECASE):
		matchReg = re.match('^\-\-UEIPAddress=(.+)$', myArgv, re.IGNORECASE)
		SSH.UEIPAddress = matchReg.group(1)
	elif re.match('^\-\-UEUserName=(.+)$', myArgv, re.IGNORECASE):
		matchReg = re.match('^\-\-UEUserName=(.+)$', myArgv, re.IGNORECASE)
		SSH.UEUserName = matchReg.group(1)
	elif re.match('^\-\-UEPassword=(.+)$', myArgv, re.IGNORECASE):
		matchReg = re.match('^\-\-UEPassword=(.+)$', myArgv, re.IGNORECASE)
		SSH.UEPassword = matchReg.group(1)
	elif re.match('^\-\-UESourceCodePath=(.+)$', myArgv, re.IGNORECASE):
		matchReg = re.match('^\-\-UESourceCodePath=(.+)$', myArgv, re.IGNORECASE)
		SSH.UESourceCodePath = matchReg.group(1)
	elif re.match('^\-\-finalStatus=(.+)$', myArgv, re.IGNORECASE):
		matchReg = re.match('^\-\-finalStatus=(.+)$', myArgv, re.IGNORECASE)
		finalStatus = matchReg.group(1)
		if ((finalStatus == 'true') or (finalStatus == 'True')):
			SSH.finalStatus = True
	else:
		Usage()
		sys.exit('Invalid Parameter: ' + myArgv)

if re.match('^TerminateeNB$', mode, re.IGNORECASE):
	if SSH.eNBIPAddress == '' or SSH.eNBUserName == '' or SSH.eNBPassword == '':
		Usage()
		sys.exit('Insufficient Parameter')
	SSH.eNB_serverId = '0'
	SSH.eNB_instance = '0'
	SSH.eNBSourceCodePath = '/tmp/'
	SSH.TerminateeNB()
elif re.match('^TerminateUE$', mode, re.IGNORECASE):
	if (SSH.ADBIPAddress == '' or SSH.ADBUserName == '' or SSH.ADBPassword == ''):
		Usage()
		sys.exit('Insufficient Parameter')
	signal.signal(signal.SIGUSR1, receive_signal)
	SSH.TerminateUE()
elif re.match('^TerminateOAIUE$', mode, re.IGNORECASE):
	if SSH.UEIPAddress == '' or SSH.UEUserName == '' or SSH.UEPassword == '':
		Usage()
		sys.exit('Insufficient Parameter')
	signal.signal(signal.SIGUSR1, receive_signal)
	SSH.TerminateOAIUE()
elif re.match('^TerminateHSS$', mode, re.IGNORECASE):
	if SSH.EPCIPAddress == '' or SSH.EPCUserName == '' or SSH.EPCPassword == '' or SSH.EPCType == '' or SSH.EPCSourceCodePath == '':
		Usage()
		sys.exit('Insufficient Parameter')
	SSH.TerminateHSS()
elif re.match('^TerminateMME$', mode, re.IGNORECASE):
	if SSH.EPCIPAddress == '' or SSH.EPCUserName == '' or SSH.EPCPassword == '' or SSH.EPCType == '' or SSH.EPCSourceCodePath == '':
		Usage()
		sys.exit('Insufficient Parameter')
	SSH.TerminateMME()
elif re.match('^TerminateSPGW$', mode, re.IGNORECASE):
	if SSH.EPCIPAddress == '' or SSH.EPCUserName == '' or SSH.EPCPassword == '' or SSH.EPCType == '' or SSH.EPCSourceCodePath == '':
		Usage()
		sys.exit('Insufficient Parameter')
	SSH.TerminateSPGW()
elif re.match('^LogCollectBuild$', mode, re.IGNORECASE):
	if (SSH.eNBIPAddress == '' or SSH.eNBUserName == '' or SSH.eNBPassword == '' or SSH.eNBSourceCodePath == '') and (SSH.UEIPAddress == '' or SSH.UEUserName == '' or SSH.UEPassword == '' or SSH.UESourceCodePath == ''):
		Usage()
		sys.exit('Insufficient Parameter')
	SSH.LogCollectBuild()
elif re.match('^LogCollecteNB$', mode, re.IGNORECASE):
	if SSH.eNBIPAddress == '' or SSH.eNBUserName == '' or SSH.eNBPassword == '' or SSH.eNBSourceCodePath == '':
		Usage()
		sys.exit('Insufficient Parameter')
	SSH.LogCollecteNB()
elif re.match('^LogCollectHSS$', mode, re.IGNORECASE):
	if SSH.EPCIPAddress == '' or SSH.EPCUserName == '' or SSH.EPCPassword == '' or SSH.EPCType == '' or SSH.EPCSourceCodePath == '':
		Usage()
		sys.exit('Insufficient Parameter')
	SSH.LogCollectHSS()
elif re.match('^LogCollectMME$', mode, re.IGNORECASE):
	if SSH.EPCIPAddress == '' or SSH.EPCUserName == '' or SSH.EPCPassword == '' or SSH.EPCType == '' or SSH.EPCSourceCodePath == '':
		Usage()
		sys.exit('Insufficient Parameter')
	SSH.LogCollectMME()
elif re.match('^LogCollectSPGW$', mode, re.IGNORECASE):
	if SSH.EPCIPAddress == '' or SSH.EPCUserName == '' or SSH.EPCPassword == '' or SSH.EPCType == '' or SSH.EPCSourceCodePath == '':
		Usage()
		sys.exit('Insufficient Parameter')
	SSH.LogCollectSPGW()
elif re.match('^LogCollectPing$', mode, re.IGNORECASE):
	if SSH.EPCIPAddress == '' or SSH.EPCUserName == '' or SSH.EPCPassword == '' or SSH.EPCSourceCodePath == '':
		Usage()
		sys.exit('Insufficient Parameter')
	SSH.LogCollectPing()
elif re.match('^LogCollectIperf$', mode, re.IGNORECASE):
	if SSH.EPCIPAddress == '' or SSH.EPCUserName == '' or SSH.EPCPassword == '' or SSH.EPCSourceCodePath == '':
		Usage()
		sys.exit('Insufficient Parameter')
	SSH.LogCollectIperf()
elif re.match('^LogCollectOAIUE$', mode, re.IGNORECASE):
	if SSH.UEIPAddress == '' or SSH.UEUserName == '' or SSH.UEPassword == '' or SSH.UESourceCodePath == '':
		Usage()
		sys.exit('Insufficient Parameter')
	SSH.LogCollectOAIUE()
elif re.match('^InitiateHtml$', mode, re.IGNORECASE):
	if (SSH.ADBIPAddress == '' or SSH.ADBUserName == '' or SSH.ADBPassword == ''):
		Usage()
		sys.exit('Insufficient Parameter')
	count = 0
	foundCount = 0
	while (count < SSH.nbTestXMLfiles):
		xml_test_file = cwd + "/" + SSH.testXMLfiles[count]
		xml_test_file = sys.path[0] + "/" + SSH.testXMLfiles[count]
		if (os.path.isfile(xml_test_file)):
			xmlTree = ET.parse(xml_test_file)
			xmlRoot = xmlTree.getroot()
			SSH.htmlTabRefs.append(xmlRoot.findtext('htmlTabRef',default='test-tab-' + str(count)))
			SSH.htmlTabNames.append(xmlRoot.findtext('htmlTabName',default='Test-' + str(count)))
			SSH.htmlTabIcons.append(xmlRoot.findtext('htmlTabIcon',default='info-sign'))
			foundCount += 1
		count += 1
	if foundCount != SSH.nbTestXMLfiles:
		SSH.nbTestXMLfiles = foundCount
	SSH.CreateHtmlHeader()
elif re.match('^FinalizeHtml$', mode, re.IGNORECASE):
	SSH.CreateHtmlFooter(SSH.finalStatus)
elif re.match('^TesteNB$', mode, re.IGNORECASE) or re.match('^TestUE$', mode, re.IGNORECASE):
	if re.match('^TesteNB$', mode, re.IGNORECASE):
		if SSH.eNBIPAddress == '' or SSH.ranRepository == '' or SSH.ranBranch == '' or SSH.eNBUserName == '' or SSH.eNBPassword == '' or SSH.eNBSourceCodePath == '' or SSH.EPCIPAddress == '' or SSH.EPCUserName == '' or SSH.EPCPassword == '' or SSH.EPCType == '' or SSH.EPCSourceCodePath == '' or SSH.ADBIPAddress == '' or SSH.ADBUserName == '' or SSH.ADBPassword == '':
			Usage()
			sys.exit('Insufficient Parameter')

		if (SSH.EPCIPAddress != '') and (SSH.EPCIPAddress != 'none'):
			SSH.copyout(SSH.EPCIPAddress, SSH.EPCUserName, SSH.EPCPassword, cwd + "/tcp_iperf_stats.awk", "/tmp")
			SSH.copyout(SSH.EPCIPAddress, SSH.EPCUserName, SSH.EPCPassword, cwd + "/active_net_interfaces.awk", "/tmp")
	else:
		if SSH.UEIPAddress == '' or SSH.ranRepository == '' or SSH.ranBranch == '' or SSH.UEUserName == '' or SSH.UEPassword == '' or SSH.UESourceCodePath == '':
			Usage()
			sys.exit('UE: Insufficient Parameter')

	#read test_case_list.xml file
	# if no parameters for XML file, use default value
	if (SSH.nbTestXMLfiles != 1):
		xml_test_file = cwd + "/test_case_list.xml"
	else:
		xml_test_file = cwd + "/" + SSH.testXMLfiles[0]

	xmlTree = ET.parse(xml_test_file)
	xmlRoot = xmlTree.getroot()

	exclusion_tests=xmlRoot.findtext('TestCaseExclusionList',default='')
	requested_tests=xmlRoot.findtext('TestCaseRequestedList',default='')
	if (SSH.nbTestXMLfiles == 1):
		SSH.htmlTabRefs.append(xmlRoot.findtext('htmlTabRef',default='test-tab-0'))
		SSH.htmlTabNames.append(xmlRoot.findtext('htmlTabName',default='Test-0'))
		repeatCount = xmlRoot.findtext('repeatCount',default='1')
		SSH.repeatCounts.append(int(repeatCount))
	all_tests=xmlRoot.findall('testCase')

	exclusion_tests=exclusion_tests.split()
	requested_tests=requested_tests.split()

	#check that exclusion tests are well formatted
	#(6 digits or less than 6 digits followed by +)
	for test in exclusion_tests:
		if     (not re.match('^[0-9]{6}$', test) and
				not re.match('^[0-9]{1,5}\+$', test)):
			logging.debug('ERROR: exclusion test is invalidly formatted: ' + test)
			sys.exit(1)
		else:
			logging.debug(test)

	#check that requested tests are well formatted
	#(6 digits or less than 6 digits followed by +)
	#be verbose
	for test in requested_tests:
		if     (re.match('^[0-9]{6}$', test) or
				re.match('^[0-9]{1,5}\+$', test)):
			logging.debug('INFO: test group/case requested: ' + test)
		else:
			logging.debug('ERROR: requested test is invalidly formatted: ' + test)
			sys.exit(1)
	if (SSH.EPCIPAddress != '') and (SSH.EPCIPAddress != 'none'):
		SSH.CheckFlexranCtrlInstallation()

	#get the list of tests to be done
	todo_tests=[]
	for test in requested_tests:
		if    (test_in_list(test, exclusion_tests)):
			logging.debug('INFO: test will be skipped: ' + test)
		else:
			#logging.debug('INFO: test will be run: ' + test)
			todo_tests.append(test)

	signal.signal(signal.SIGUSR1, receive_signal)

	SSH.CreateHtmlTabHeader()

	cnt = 0
	SSH.prematureExit = True
	SSH.startTime = int(round(time.time() * 1000))
	while cnt < SSH.repeatCounts[0] and SSH.prematureExit:
		SSH.prematureExit = False
		for test_case_id in todo_tests:
			if SSH.prematureExit:
				break
			for test in all_tests:
				if SSH.prematureExit:
					break
				id = test.get('id')
				if test_case_id != id:
					continue
				SSH.testCase_id = id
				SSH.desc = test.findtext('desc')
				action = test.findtext('class')
				if (CheckClassValidity(action, id) == False):
					continue
				SSH.ShowTestID()
				GetParametersFromXML(action)
				if action == 'Initialize_UE' or action == 'Attach_UE' or action == 'Detach_UE' or action == 'Ping' or action == 'Iperf' or action == 'Reboot_UE' or action == 'DataDisable_UE' or action == 'DataEnable_UE' or action == 'CheckStatusUE':
					if (SSH.ADBIPAddress != 'none'):
						terminate_ue_flag = False
						SSH.GetAllUEDevices(terminate_ue_flag)
				if action == 'Build_eNB':
					SSH.BuildeNB()
				elif action == 'WaitEndBuild_eNB':
					SSH.WaitBuildeNBisFinished()
				elif action == 'Initialize_eNB':
					SSH.InitializeeNB()
				elif action == 'Terminate_eNB':
					SSH.TerminateeNB()
				elif action == 'Initialize_UE':
					SSH.InitializeUE()
				elif action == 'Terminate_UE':
					SSH.TerminateUE()
				elif action == 'Attach_UE':
					SSH.AttachUE()
				elif action == 'Detach_UE':
					SSH.DetachUE()
				elif action == 'DataDisable_UE':
					SSH.DataDisableUE()
				elif action == 'DataEnable_UE':
					SSH.DataEnableUE()
				elif action == 'CheckStatusUE':
					SSH.CheckStatusUE()
				elif action == 'Build_OAI_UE':
					SSH.BuildOAIUE()
				elif action == 'Initialize_OAI_UE':
					SSH.InitializeOAIUE()
				elif action == 'Terminate_OAI_UE':
					SSH.TerminateOAIUE()
				elif action == 'Initialize_CatM_module':
					SSH.InitializeCatM()
				elif action == 'Terminate_CatM_module':
					SSH.TerminateCatM()
				elif action == 'Attach_CatM_module':
					SSH.AttachCatM()
				elif action == 'Detach_CatM_module':
					SSH.TerminateCatM()
				elif action == 'Ping_CatM_module':
					SSH.PingCatM()
				elif action == 'Ping':
					SSH.Ping()
				elif action == 'Iperf':
					SSH.Iperf()
				elif action == 'Reboot_UE':
					SSH.RebootUE()
				elif action == 'Initialize_HSS':
					SSH.InitializeHSS()
				elif action == 'Terminate_HSS':
					SSH.TerminateHSS()
				elif action == 'Initialize_MME':
					SSH.InitializeMME()
				elif action == 'Terminate_MME':
					SSH.TerminateMME()
				elif action == 'Initialize_SPGW':
					SSH.InitializeSPGW()
				elif action == 'Terminate_SPGW':
					SSH.TerminateSPGW()
				elif action == 'Initialize_FlexranCtrl':
					SSH.InitializeFlexranCtrl()
				elif action == 'Terminate_FlexranCtrl':
					SSH.TerminateFlexranCtrl()
				elif action == 'IdleSleep':
					SSH.IdleSleep()
				elif action == 'Perform_X2_Handover':
					SSH.Perform_X2_Handover()
				else:
					sys.exit('Invalid action')
		cnt += 1
	if cnt == SSH.repeatCounts[0] and SSH.prematureExit:
		logging.debug('Testsuite failed ' + str(cnt) + ' time(s)')
		SSH.CreateHtmlTabFooter(False)
		sys.exit('Failed Scenario')
	else:
		logging.info('Testsuite passed after ' + str(cnt) + ' time(s)')
		SSH.CreateHtmlTabFooter(True)
else:
	Usage()
	sys.exit('Invalid mode')
sys.exit(0)
