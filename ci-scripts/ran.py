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
class RANManagement():

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
		self.eNB1IPAddress = ''
		self.eNB1UserName = ''
		self.eNB1Password = ''
		self.eNB1SourceCodePath = ''
		self.eNB2IPAddress = ''
		self.eNB2UserName = ''
		self.eNB2Password = ''
		self.eNB2SourceCodePath = ''
		self.Build_eNB_args = ''
		self.backgroundBuild = False
		self.backgroundBuildTestId = ['', '', '']
		self.Build_eNB_forced_workspace_cleanup = False
		self.Initialize_eNB_args = ''
		self.air_interface = 'lte'
		self.eNB_instance = ''
		self.eNB_serverId = ''
		self.eNBLogFiles = ['', '', '']
		self.eNBOptions = ['', '', '']
		self.eNBmbmsEnables = [False, False, False]
		self.eNBstatuses = [-1, -1, -1]
		self.flexranCtrlInstalled = False
		self.flexranCtrlStarted = False
		self.testCase_id = ''
		self.epcPcapFile = ''
		self.htmlObj = None
		self.epcObj = None

#-----------------------------------------------------------
# Setters and Getters on Public members
#-----------------------------------------------------------

	def SetHtmlObj(self, obj):
		self.htmlObj = obj
	def SetEpcObj(self, obj):
		self.epcObj = obj

	def SetflexranCtrlInstalled(self,fxrctin):
		self.flexranCtrlInstalled = fxrctin
	def GetflexranCtrlInstalled(self):
		return self.flexranCtrlInstalled
	def SetflexranCtrlStarted(self,fxrctst):
		self.flexranCtrlStarted = fxrctst
	def GetflexranCtrlStarted(self):
		return self.flexranCtrlStarted
	def SetpStatus(self, pSt):
		self.pStatus = pSt
	def SetranRepository(self, repository):
		self.ranRepository = repository
	def GetranRepository(self):
		return self.ranRepository
	def SetranBranch(self, branch):
		self.ranBranch = branch
	def GetranBranch(self):
		return self.ranBranch
	def SetranCommitID(self, commitid):
		self.ranCommitID = commitid
	def GetranCommitID(self):
		return self.ranCommitID
	def SeteNB_serverId(self, enbsrvid):
		self.eNB_serverId = enbsrvid
	def GeteNB_serverId(self):
		return self.eNB_serverId
	def SeteNBIPAddress(self, enbip):
		self.eNBIPAddress = enbip
	def GeteNBIPAddress(self):
		return self.eNBIPAddress
	def SeteNBUserName(self, enbusr):
		self.eNBUserName = enbusr
	def GeteNBUserName(self):
		return self.eNBUserName
	def SeteNBPassword(self, enbpw):
		self.eNBPassword = enbpw
	def GeteNBPassword(self):
		return self.eNBPassword
	def SeteNBSourceCodePath(self, enbcodepath):
		self.eNBSourceCodePath = enbcodepath
	def GeteNBSourceCodePath(self):
		return self.eNBSourceCodePath
	def SetranAllowMerge(self, merge):
		self.ranAllowMerge = merge
	def GetranAllowMerge(self):
		return self.ranAllowMerge
	def SetranTargetBranch(self, tbranch):
		self.ranTargetBranch = tbranch
	def GetranTargetBranch(self):
		return self.ranTargetBranch
	def SetBuild_eNB_args(self, enbbuildarg):
		self.Build_eNB_args = enbbuildarg
	def GetBuild_eNB_args(self):
		return self.Build_eNB_args
	def SetInitialize_eNB_args(self, initenbarg):
		self.Initialize_eNB_args = initenbarg
	def GetInitialize_eNB_args(self):
		return self.Initialize_eNB_args
	def SetbackgroundBuild(self, bkbuild):
		self.backgroundBuild = bkbuild
	def GetbackgroundBuild(self):
		return self.backgroundBuild
	def SetbackgroundBuildTestId(self, bkbuildid):
		self.backgroundBuildTestId = bkbuildid
	def GetbackgroundBuildTestId(self):
		return self.backgroundBuildTestId
	def SetBuild_eNB_forced_workspace_cleanup(self, fcdwspclean):
		self.Build_eNB_forced_workspace_cleanup = fcdwspclean
	def GetBuild_eNB_forced_workspace_cleanup(self):
		return self.Build_eNB_forced_workspace_cleanup
	def Setair_interface(self, airif):
		self.air_interface = airif
	def Getair_interface(self):
		return self.air_interface
	def SeteNB_instance(self, enbinst):
		self.eNB_instance = enbinst
	def GeteNB_instance(self):
		return self.eNB_instance

	def SeteNBLogFile(self, enblog, idx):
		self.eNBLogFiles[idx] = enblog
	def GeteNBLogFile(self, idx):
		return self.eNBLogFiles[idx]

	def GeteNBmbmsEnable(self, idx):
		return self.eNBmbmsEnables[idx]

	def SeteNB1IPAddress(self,enb1ip):
		self.eNB1IPAddress = enb1ip
	def GeteNB1IPAddress(self):
		return self.eNB1IPAddress
	def SeteNB1UserName(self, enb1usr):
		self.eNB1UserName = enb1usr
	def GeteNB1UserName(self):
		return self.eNB1UserName
	def SeteNB1Password(self, enb1pw):
		self.eNB1Password = enb1pw
	def GeteNB1Password(self):
		return self.eNB1Password
	def SeteNB1SourceCodePath(self, enb1codepath):
		self.eNB1SourceCodePath = enb1codepath
	def GeteNB1SourceCodePath(self):
		return self.eNB1SourceCodePath

	def SeteNB2IPAddress(self, enb2ip):
		self.eNB2IPAddress = enb2ip
	def GeteNB2IPAddress(self):
		return self.eNB2IPAddress
	def SeteNB2UserName(self, enb2usr):
		self.eNB2UserName = enb2usr
	def GeteNB2UserName(self):
		return self.eNB2UserName
	def SeteNB2Password(self, enb2pw):
		self.eNB2Password = enb2pw
	def GeteNB2Password(self):
		return self.eNB2Password
	def SeteNB2SourceCodePath(self, enb2codepath):
		self.eNB2SourceCodePath = enb2codepath
	def GeteNB2SourceCodePath(self):
		return self.eNB2SourceCodePath

	def SetprematureExit(self, premex):
		self.prematureExit = premex
	def GetprematureExit(self):
		return self.prematureExit

#-----------------------------------------------------------
# RAN management functions
#-----------------------------------------------------------

	def BuildeNB(self):
		if self.ranRepository == '' or self.ranBranch == '' or self.ranCommitID == '':
			HELP.GenericHelp(CONST.Version)
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
			HELP.GenericHelp(CONST.Version)
			sys.exit('Insufficient Parameter')
		mySSH = SSH.SSHConnection()
		mySSH.open(lIpAddr, lUserName, lPassWord)
		# Check if we build an 5G-NR gNB or an LTE eNB
		result = re.search('--gNB', self.Build_eNB_args)
		if result is not None:
			self.air_interface = 'nr'
		else:
			self.air_interface = 'lte'
		# Worakround for some servers, we need to erase completely the workspace
		if self.Build_eNB_forced_workspace_cleanup:
			mySSH.command('echo ' + lPassWord + ' | sudo -S rm -Rf ' + lSourcePath, '\$', 15)
		if self.htmlObj is not None:
			self.testCase_id = self.htmlObj.GettestCase_id()
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
		# Checking the BUILD INFO file
		if not self.backgroundBuild:
			mySSH.command('ls *.txt', '\$', 5)
			result = re.search('LAST_BUILD_INFO', mySSH.getBefore())
			if result is not None:
				mismatch = False
				mySSH.command('grep SRC_COMMIT LAST_BUILD_INFO.txt', '\$', 2)
				result = re.search(self.ranCommitID, mySSH.getBefore())
				if result is None:
					mismatch = True
				mySSH.command('grep MERGED_W_TGT_BRANCH LAST_BUILD_INFO.txt', '\$', 2)
				if (self.ranAllowMerge):
					result = re.search('YES', mySSH.getBefore())
					if result is None:
						mismatch = True
					mySSH.command('grep TGT_BRANCH LAST_BUILD_INFO.txt', '\$', 2)
					if self.ranTargetBranch == '':
						result = re.search('develop', mySSH.getBefore())
					else:
						result = re.search(self.ranTargetBranch, mySSH.getBefore())
					if result is None:
						mismatch = True
				else:
					result = re.search('NO', mySSH.getBefore())
					if result is None:
						mismatch = True
				if not mismatch:
					mySSH.close()
					if self.htmlObj is not None:
						self.htmlObj.CreateHtmlTestRow(self.Build_eNB_args, 'OK', CONST.ALL_PROCESSES_OK)
					return

		mySSH.command('echo ' + lPassWord + ' | sudo -S git clean -x -d -ff', '\$', 30)
		# if the commit ID is provided use it to point to it
		if self.ranCommitID != '':
			mySSH.command('git checkout -f ' + self.ranCommitID, '\$', 5)
		# if the branch is not develop, then it is a merge request and we need to do 
		# the potential merge. Note that merge conflicts should already been checked earlier
		if (self.ranAllowMerge):
			if self.ranTargetBranch == '':
				if (self.ranBranch != 'develop') and (self.ranBranch != 'origin/develop'):
					mySSH.command('git merge --ff origin/develop -m "Temporary merge for CI"', '\$', 5)
			else:
				logging.debug('Merging with the target branch: ' + self.ranTargetBranch)
				mySSH.command('git merge --ff origin/' + self.ranTargetBranch + ' -m "Temporary merge for CI"', '\$', 5)
		mySSH.command('source oaienv', '\$', 5)
		mySSH.command('cd cmake_targets', '\$', 5)
		mySSH.command('mkdir -p log', '\$', 5)
		mySSH.command('chmod 777 log', '\$', 5)
		# no need to remove in log (git clean did the trick)
		if self.backgroundBuild:
			mySSH.command('echo "./build_oai ' + self.Build_eNB_args + '" > ./my-lte-softmodem-build.sh', '\$', 5)
			mySSH.command('chmod 775 ./my-lte-softmodem-build.sh', '\$', 5)
			mySSH.command('echo ' + lPassWord + ' | sudo -S -E daemon --inherit --unsafe --name=build_enb_daemon --chdir=' + lSourcePath + '/cmake_targets -o ' + lSourcePath + '/cmake_targets/compile_oai_enb.log ./my-lte-softmodem-build.sh', '\$', 5)
			mySSH.close()
			if self.htmlObj is not None:
				self.htmlObj.CreateHtmlTestRow(self.Build_eNB_args, 'OK', CONST.ALL_PROCESSES_OK)
			self.backgroundBuildTestId[int(self.eNB_instance)] = self.testCase_id
			return
		mySSH.command('stdbuf -o0 ./build_oai ' + self.Build_eNB_args + ' 2>&1 | stdbuf -o0 tee compile_oai_enb.log', 'Bypassing the Tests|build have failed', 1500)
		mySSH.close()
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
			HELP.GenericHelp(CONST.Version)
			sys.exit('Insufficient Parameter')
		mySSH = SSH.SSHConnection()
		mySSH.open(lIpAddr, lUserName, lPassWord)
		count = 40
		buildOAIprocess = True
		while (count > 0) and buildOAIprocess:
			mySSH.command('ps aux | grep --color=never build_ | grep -v grep', '\$', 3)
			result = re.search('build_oai', mySSH.getBefore())
			if result is None:
				buildOAIprocess = False
			else:
				count -= 1
				time.sleep(30)
		mySSH.close()
		self.checkBuildeNB(lIpAddr, lUserName, lPassWord, lSourcePath, self.backgroundBuildTestId[int(self.eNB_instance)])

	def checkBuildeNB(self, lIpAddr, lUserName, lPassWord, lSourcePath, testcaseId):
		if self.htmlObj is not None:
			self.htmlObj.SettestCase_id(testcaseId)
		mySSH = SSH.SSHConnection()
		mySSH.open(lIpAddr, lUserName, lPassWord)
		mySSH.command('cd ' + lSourcePath + '/cmake_targets', '\$', 3)
		mySSH.command('ls ran_build/build', '\$', 3)
		mySSH.command('ls ran_build/build', '\$', 3)
		if self.air_interface == 'nr':
			nodeB_prefix = 'g'
		else:
			nodeB_prefix = 'e'
		buildStatus = True
		result = re.search(self.air_interface + '-softmodem', mySSH.getBefore())
		if result is None:
			buildStatus = False
		else:
			# Generating a BUILD INFO file
			mySSH.command('echo "SRC_BRANCH: ' + self.ranBranch + '" > ../LAST_BUILD_INFO.txt', '\$', 2)
			mySSH.command('echo "SRC_COMMIT: ' + self.ranCommitID + '" >> ../LAST_BUILD_INFO.txt', '\$', 2)
			if (self.ranAllowMerge):
				mySSH.command('echo "MERGED_W_TGT_BRANCH: YES" >> ../LAST_BUILD_INFO.txt', '\$', 2)
				if self.ranTargetBranch == '':
					mySSH.command('echo "TGT_BRANCH: develop" >> ../LAST_BUILD_INFO.txt', '\$', 2)
				else:
					mySSH.command('echo "TGT_BRANCH: ' + self.ranTargetBranch + '" >> ../LAST_BUILD_INFO.txt', '\$', 2)
			else:
				mySSH.command('echo "MERGED_W_TGT_BRANCH: NO" >> ../LAST_BUILD_INFO.txt', '\$', 2)
		mySSH.command('mkdir -p build_log_' + testcaseId, '\$', 5)
		mySSH.command('mv log/* ' + 'build_log_' + testcaseId, '\$', 5)
		mySSH.command('mv compile_oai_enb.log ' + 'build_log_' + testcaseId, '\$', 5)
		if self.eNB_serverId != '0':
			mySSH.command('cd cmake_targets', '\$', 5)
			mySSH.command('if [ -e tmp_build' + testcaseId + '.zip ]; then rm -f tmp_build' + testcaseId + '.zip; fi', '\$', 5)
			mySSH.command('zip -r -qq tmp_build' + testcaseId + '.zip build_log_' + testcaseId, '\$', 5)
			mySSH.close()
			if (os.path.isfile('./tmp_build' + testcaseId + '.zip')):
				os.remove('./tmp_build' + testcaseId + '.zip')
			mySSH.copyin(lIpAddr, lUserName, lPassWord, lSourcePath + '/cmake_targets/tmp_build' + testcaseId + '.zip', '.')
			if (os.path.isfile('./tmp_build' + testcaseId + '.zip')):
				mySSH.copyout(self.eNBIPAddress, self.eNBUserName, self.eNBPassword, './tmp_build' + testcaseId + '.zip', self.eNBSourceCodePath + '/cmake_targets/.')
				os.remove('./tmp_build' + testcaseId + '.zip')
				mySSH.open(self.eNBIPAddress, self.eNBUserName, self.eNBPassword)
				mySSH.command('cd ' + self.eNBSourceCodePath + '/cmake_targets', '\$', 5)
				mySSH.command('unzip -qq -DD tmp_build' + testcaseId + '.zip', '\$', 5)
				mySSH.command('rm -f tmp_build' + testcaseId + '.zip', '\$', 5)
				mySSH.close()
		else:
			mySSH.close()

		if buildStatus:
			logging.info('\u001B[1m Building OAI ' + nodeB_prefix + 'NB Pass\u001B[0m')
			if self.htmlObj is not None:
				self.htmlObj.CreateHtmlTestRow(self.Build_eNB_args, 'OK', CONST.ALL_PROCESSES_OK)
		else:
			logging.error('\u001B[1m Building OAI ' + nodeB_prefix + 'NB Failed\u001B[0m')
			if self.htmlObj is not None:
				self.htmlObj.CreateHtmlTestRow(self.Build_eNB_args, 'KO', CONST.ALL_PROCESSES_OK)
				self.htmlObj.CreateHtmlTabFooter(False)
			sys.exit(1)

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
			HELP.GenericHelp(CONST.Version)
			sys.exit('Insufficient Parameter')

		if self.htmlObj is not None:
			self.testCase_id = self.htmlObj.GettestCase_id()
		else:
			self.testCase_id = '000000'
		mySSH = SSH.SSHConnection()
		
		if (self.pStatus < 0):
			if self.htmlObj is not None:
				self.htmlObj.CreateHtmlTestRow(self.Initialize_eNB_args, 'KO', self.pStatus)
				self.htmlObj.CreateHtmlTabFooter(False)
			sys.exit(1)
		# If tracer options is on, running tshark on EPC side and capture traffic b/ EPC and eNB
		result = re.search('T_stdout', str(self.Initialize_eNB_args))
		if (result is not None) and (self.epcObj is not None):
			localEpcIpAddr = self.epcObj.GetIPAddress()
			localEpcUserName = self.epcObj.GetUserName()
			localEpcPassword = self.epcObj.GetPassword()
			mySSH.open(localEpcIpAddr, localEpcUserName, localEpcPassword)
			mySSH.command('ip addr show | awk -f /tmp/active_net_interfaces.awk | egrep -v "lo|tun"', '\$', 5)
			result = re.search('interfaceToUse=(?P<eth_interface>[a-zA-Z0-9\-\_]+)done', mySSH.getBefore())
			if result is not None:
				eth_interface = result.group('eth_interface')
				logging.debug('\u001B[1m Launching tshark on interface ' + eth_interface + '\u001B[0m')
				self.epcPcapFile = 'enb_' + self.testCase_id + '_s1log.pcap'
				mySSH.command('echo ' + localEpcPassword + ' | sudo -S rm -f /tmp/' + self.epcPcapFile , '\$', 5)
				mySSH.command('echo $USER; nohup sudo tshark -f "host ' + lIpAddr +'" -i ' + eth_interface + ' -w /tmp/' + self.epcPcapFile + ' > /tmp/tshark.log 2>&1 &', localEpcUserName, 5)
			mySSH.close()
		mySSH.open(lIpAddr, lUserName, lPassWord)
		mySSH.command('cd ' + lSourcePath, '\$', 5)
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
				mySSH.command('cd common/utils/T/tracer', '\$', 5)
				mySSH.command('make', '\$', 10)
				mySSH.command('echo $USER; nohup ./record -d ../T_messages.txt -o ' + lSourcePath + '/cmake_targets/enb_' + self.testCase_id + '_record.raw -ON -off VCD -off HEAVY -off LEGACY_GROUP_TRACE -off LEGACY_GROUP_DEBUG > ' + lSourcePath + '/cmake_targets/enb_' + self.testCase_id + '_record.log 2>&1 &', lUserName, 5)
				mySSH.command('cd ' + lSourcePath, '\$', 5)
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
			mySSH.command('echo ' + lPassWord + ' | sudo -S uhd_find_devices', '\$', 60)
			result = re.search('type: b200', mySSH.getBefore())
			if result is not None:
				logging.debug('Found a B2xx device --> resetting it')
				mySSH.command('echo ' + lPassWord + ' | sudo -S b2xx_fx3_utils --reset-device', '\$', 10)
				# Reloading FGPA bin firmware
				mySSH.command('echo ' + lPassWord + ' | sudo -S uhd_find_devices', '\$', 60)
		# Make a copy and adapt to EPC / eNB IP addresses
		mySSH.command('cp ' + full_config_file + ' ' + ci_full_config_file, '\$', 5)
		if self.epcObj is not None:
			localMmeIpAddr = self.epcObj.GetMmeIPAddress()
			mySSH.command('sed -i -e \'s/CI_MME_IP_ADDR/' + localMmeIpAddr + '/\' ' + ci_full_config_file, '\$', 2);
		mySSH.command('sed -i -e \'s/CI_ENB_IP_ADDR/' + lIpAddr + '/\' ' + ci_full_config_file, '\$', 2);
		mySSH.command('sed -i -e \'s/CI_RCC_IP_ADDR/' + self.eNBIPAddress + '/\' ' + ci_full_config_file, '\$', 2);
		mySSH.command('sed -i -e \'s/CI_RRU1_IP_ADDR/' + self.eNB1IPAddress + '/\' ' + ci_full_config_file, '\$', 2);
		mySSH.command('sed -i -e \'s/CI_RRU2_IP_ADDR/' + self.eNB2IPAddress + '/\' ' + ci_full_config_file, '\$', 2);
		if self.flexranCtrlInstalled and self.flexranCtrlStarted:
			mySSH.command('sed -i -e \'s/FLEXRAN_ENABLED.*;/FLEXRAN_ENABLED        = "yes";/\' ' + ci_full_config_file, '\$', 2);
		else:
			mySSH.command('sed -i -e \'s/FLEXRAN_ENABLED.*;/FLEXRAN_ENABLED        = "no";/\' ' + ci_full_config_file, '\$', 2);
		self.eNBmbmsEnables[int(self.eNB_instance)] = False
		mySSH.command('grep enable_enb_m2 ' + ci_full_config_file, '\$', 2);
		result = re.search('yes', mySSH.getBefore())
		if result is not None:
			self.eNBmbmsEnables[int(self.eNB_instance)] = True
			logging.debug('\u001B[1m MBMS is enabled on this eNB\u001B[0m')
		result = re.search('noS1', str(self.Initialize_eNB_args))
		eNBinNoS1 = False
		if result is not None:
			eNBinNoS1 = True
			logging.debug('\u001B[1m eNB is in noS1 configuration \u001B[0m')
		# Launch eNB with the modified config file
		mySSH.command('source oaienv', '\$', 5)
		mySSH.command('cd cmake_targets', '\$', 5)
		mySSH.command('echo "ulimit -c unlimited && ./ran_build/build/' + self.air_interface + '-softmodem -O ' + lSourcePath + '/' + ci_full_config_file + extra_options + '" > ./my-lte-softmodem-run' + str(self.eNB_instance) + '.sh', '\$', 5)
		mySSH.command('chmod 775 ./my-lte-softmodem-run' + str(self.eNB_instance) + '.sh', '\$', 5)
		mySSH.command('echo ' + lPassWord + ' | sudo -S rm -Rf enb_' + self.testCase_id + '.log', '\$', 5)
		mySSH.command('hostnamectl','\$', 5)
		result = re.search('CentOS Linux 7', mySSH.getBefore())
		if result is not None:
			mySSH.command('echo $USER; nohup sudo ./my-lte-softmodem-run' + str(self.eNB_instance) + '.sh > ' + lSourcePath + '/cmake_targets/enb_' + self.testCase_id + '.log 2>&1 &', lUserName, 10)
		else:
			mySSH.command('echo ' + lPassWord + ' | sudo -S -E daemon --inherit --unsafe --name=enb' + str(self.eNB_instance) + '_daemon --chdir=' + lSourcePath + '/cmake_targets -o ' + lSourcePath + '/cmake_targets/enb_' + self.testCase_id + '.log ./my-lte-softmodem-run' + str(self.eNB_instance) + '.sh', '\$', 5)
		self.eNBLogFiles[int(self.eNB_instance)] = 'enb_' + self.testCase_id + '.log'
		if extra_options != '':
			self.eNBOptions[int(self.eNB_instance)] = extra_options
		time.sleep(6)
		doLoop = True
		loopCounter = 20
		enbDidSync = False
		while (doLoop):
			loopCounter = loopCounter - 1
			if (loopCounter == 0):
				# In case of T tracer recording, we may need to kill it
				result = re.search('T_stdout', str(self.Initialize_eNB_args))
				if result is not None:
					mySSH.command('killall --signal SIGKILL record', '\$', 5)
				mySSH.close()
				doLoop = False
				logging.error('\u001B[1;37;41m eNB logging system did not show got sync! \u001B[0m')
				if self.htmlObj is not None:
					self.htmlObj.CreateHtmlTestRow('-O ' + config_file + extra_options, 'KO', CONST.ALL_PROCESSES_OK)
				# In case of T tracer recording, we need to kill tshark on EPC side
				result = re.search('T_stdout', str(self.Initialize_eNB_args))
				if (result is not None) and (self.epcObj is not None):
					localEpcIpAddr = self.epcObj.GetIPAddress()
					localEpcUserName = self.epcObj.GetUserName()
					localEpcPassword = self.epcObj.GetPassword()
					mySSH.open(localEpcIpAddr, localEpcUserName, localEpcPassword)
					logging.debug('\u001B[1m Stopping tshark \u001B[0m')
					mySSH.command('echo ' + localEpcPassword + ' | sudo -S killall --signal SIGKILL tshark', '\$', 5)
					if self.epcPcapFile  != '':
						time.sleep(0.5)
						mySSH.command('echo ' + localEpcPassword + ' | sudo -S chmod 666 /tmp/' + self.epcPcapFile, '\$', 5)
					mySSH.close()
					time.sleep(1)
					if self.epcPcapFile != '':
						copyin_res = mySSH.copyin(localEpcIpAddr, localEpcUserName, localEpcPassword, '/tmp/' + self.epcPcapFile, '.')
						if (copyin_res == 0):
							mySSH.copyout(lIpAddr, lUserName, lPassWord, self.epcPcapFile, lSourcePath + '/cmake_targets/.')
				self.prematureExit = True
				return
			else:
				mySSH.command('stdbuf -o0 cat enb_' + self.testCase_id + '.log | egrep --text --color=never -i "wait|sync|Starting"', '\$', 4)
				if rruCheck:
					result = re.search('wait RUs', mySSH.getBefore())
				else:
					result = re.search('got sync|Starting F1AP at CU', mySSH.getBefore())
				if result is None:
					time.sleep(6)
				else:
					doLoop = False
					enbDidSync = True
					time.sleep(10)

		if enbDidSync and eNBinNoS1:
			mySSH.command('ifconfig oaitun_enb1', '\$', 4)
			mySSH.command('ifconfig oaitun_enb1', '\$', 4)
			result = re.search('inet addr:1|inet 1', mySSH.getBefore())
			if result is not None:
				logging.debug('\u001B[1m oaitun_enb1 interface is mounted and configured\u001B[0m')
			else:
				logging.error('\u001B[1m oaitun_enb1 interface is either NOT mounted or NOT configured\u001B[0m')
			if self.eNBmbmsEnables[int(self.eNB_instance)]:
				mySSH.command('ifconfig oaitun_enm1', '\$', 4)
				result = re.search('inet addr', mySSH.getBefore())
				if result is not None:
					logging.debug('\u001B[1m oaitun_enm1 interface is mounted and configured\u001B[0m')
				else:
					logging.error('\u001B[1m oaitun_enm1 interface is either NOT mounted or NOT configured\u001B[0m')
		if enbDidSync:
			self.eNBstatuses[int(self.eNB_instance)] = int(self.eNB_serverId)

		mySSH.close()
		if self.htmlObj is not None:
			self.htmlObj.CreateHtmlTestRow('-O ' + config_file + extra_options, 'OK', CONST.ALL_PROCESSES_OK)
		logging.debug('\u001B[1m Initialize eNB Completed\u001B[0m')

	def CheckeNBProcess(self, status_queue):
		try:
			# At least the instance 0 SHALL be on!
			if self.eNBstatuses[0] == 0:
				lIpAddr = self.eNBIPAddress
				lUserName = self.eNBUserName
				lPassWord = self.eNBPassword
			elif self.eNBstatuses[0] == 1:
				lIpAddr = self.eNB1IPAddress
				lUserName = self.eNB1UserName
				lPassWord = self.eNB1Password
			elif self.eNBstatuses[0] == 2:
				lIpAddr = self.eNB2IPAddress
				lUserName = self.eNB2UserName
				lPassWord = self.eNB2Password
			else:
				lIpAddr = self.eNBIPAddress
				lUserName = self.eNBUserName
				lPassWord = self.eNBPassword
			mySSH = SSH.SSHConnection()
			mySSH.open(lIpAddr, lUserName, lPassWord)
			mySSH.command('stdbuf -o0 ps -aux | grep --color=never ' + self.air_interface + '-softmodem | grep -v grep', '\$', 5)
			result = re.search(self.air_interface + '-softmodem', mySSH.getBefore())
			if result is None:
				logging.debug('\u001B[1;37;41m eNB Process Not Found! \u001B[0m')
				status_queue.put(CONST.ENB_PROCESS_FAILED)
			else:
				status_queue.put(CONST.ENB_PROCESS_OK)
			mySSH.close()
		except:
			os.kill(os.getppid(),signal.SIGUSR1)

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
			HELP.GenericHelp(CONST.Version)
			sys.exit('Insufficient Parameter')
		mySSH = SSH.SSHConnection()
		mySSH.open(lIpAddr, lUserName, lPassWord)
		mySSH.command('cd ' + lSourcePath + '/cmake_targets', '\$', 5)
		if self.air_interface == 'lte':
			nodeB_prefix = 'e'
		else:
			nodeB_prefix = 'g'
		mySSH.command('stdbuf -o0  ps -aux | grep --color=never softmodem | grep -v grep', '\$', 5)
		result = re.search('-softmodem', mySSH.getBefore())
		if result is not None:
			mySSH.command('echo ' + lPassWord + ' | sudo -S daemon --name=enb' + str(self.eNB_instance) + '_daemon --stop', '\$', 5)
			mySSH.command('echo ' + lPassWord + ' | sudo -S killall --signal SIGINT -r .*-softmodem || true', '\$', 5)
			time.sleep(10)
			mySSH.command('stdbuf -o0  ps -aux | grep --color=never softmodem | grep -v grep', '\$', 5)
			result = re.search('-softmodem', mySSH.getBefore())
			if result is not None:
				mySSH.command('echo ' + lPassWord + ' | sudo -S killall --signal SIGKILL -r .*-softmodem || true', '\$', 5)
				time.sleep(5)
		mySSH.command('rm -f my-lte-softmodem-run' + str(self.eNB_instance) + '.sh', '\$', 5)
		mySSH.close()
		# If tracer options is on, stopping tshark on EPC side
		result = re.search('T_stdout', str(self.Initialize_eNB_args))
		if (result is not None) and (self.epcObj is not None):
			localEpcIpAddr = self.epcObj.GetIPAddress()
			localEpcUserName = self.epcObj.GetUserName()
			localEpcPassword = self.epcObj.GetPassword()
			mySSH.open(localEpcIpAddr, localEpcUserName, localEpcPassword)
			logging.debug('\u001B[1m Stopping tshark \u001B[0m')
			mySSH.command('echo ' + localEpcPassword + ' | sudo -S killall --signal SIGKILL tshark', '\$', 5)
			time.sleep(1)
			if self.epcPcapFile != '':
				mySSH.command('echo ' + localEpcPassword + ' | sudo -S chmod 666 /tmp/' + self.epcPcapFile, '\$', 5)
				mySSH.copyin(localEpcIpAddr, localEpcUserName, localEpcPassword, '/tmp/' + self.epcPcapFile, '.')
				mySSH.copyout(lIpAddr, lUserName, lPassWord, self.epcPcapFile, lSourcePath + '/cmake_targets/.')
			mySSH.close()
			logging.debug('\u001B[1m Replaying RAW record file\u001B[0m')
			mySSH.open(lIpAddr, lUserName, lPassWord)
			mySSH.command('cd ' + lSourcePath + '/common/utils/T/tracer/', '\$', 5)
			enbLogFile = self.eNBLogFiles[int(self.eNB_instance)]
			raw_record_file = enbLogFile.replace('.log', '_record.raw')
			replay_log_file = enbLogFile.replace('.log', '_replay.log')
			extracted_txt_file = enbLogFile.replace('.log', '_extracted_messages.txt')
			extracted_log_file = enbLogFile.replace('.log', '_extracted_messages.log')
			mySSH.command('./extract_config -i ' + lSourcePath + '/cmake_targets/' + raw_record_file + ' > ' + lSourcePath + '/cmake_targets/' + extracted_txt_file, '\$', 5)
			mySSH.command('echo $USER; nohup ./replay -i ' + lSourcePath + '/cmake_targets/' + raw_record_file + ' > ' + lSourcePath + '/cmake_targets/' + replay_log_file + ' 2>&1 &', lUserName, 5)
			mySSH.command('./textlog -d ' +  lSourcePath + '/cmake_targets/' + extracted_txt_file + ' -no-gui -ON -full > ' + lSourcePath + '/cmake_targets/' + extracted_log_file, '\$', 5)
			mySSH.close()
			mySSH.copyin(lIpAddr, lUserName, lPassWord, lSourcePath + '/cmake_targets/' + extracted_log_file, '.')
			logging.debug('\u001B[1m Analyzing eNB replay logfile \u001B[0m')
			logStatus = self.AnalyzeLogFile_eNB(extracted_log_file)
			if self.htmlObj is not None:
				self.htmlObj.CreateHtmlTestRow('N/A', 'OK', CONST.ALL_PROCESSES_OK)
			self.eNBLogFiles[int(self.eNB_instance)] = ''
		else:
			analyzeFile = False
			if self.eNBLogFiles[int(self.eNB_instance)] != '':
				analyzeFile = True
				fileToAnalyze = self.eNBLogFiles[int(self.eNB_instance)]
				self.eNBLogFiles[int(self.eNB_instance)] = ''
			if analyzeFile:
				copyin_res = mySSH.copyin(lIpAddr, lUserName, lPassWord, lSourcePath + '/cmake_targets/' + fileToAnalyze, '.')
				if (copyin_res == -1):
					logging.debug('\u001B[1;37;41m Could not copy ' + nodeB_prefix + 'NB logfile to analyze it! \u001B[0m')
					if self.htmlObj is not None:
						self.htmlObj.SetHmleNBFailureMsg('Could not copy ' + nodeB_prefix + 'NB logfile to analyze it!')
						self.htmlObj.CreateHtmlTestRow('N/A', 'KO', CONST.ENB_PROCESS_NOLOGFILE_TO_ANALYZE)
					self.eNBmbmsEnables[int(self.eNB_instance)] = False
					return
				if self.eNB_serverId != '0':
					mySSH.copyout(self.eNBIPAddress, self.eNBUserName, self.eNBPassword, './' + fileToAnalyze, self.eNBSourceCodePath + '/cmake_targets/')
				logging.debug('\u001B[1m Analyzing ' + nodeB_prefix + 'NB logfile \u001B[0m ' + fileToAnalyze)
				logStatus = self.AnalyzeLogFile_eNB(fileToAnalyze)
				if (logStatus < 0):
					if self.htmlObj is not None:
						self.htmlObj.CreateHtmlTestRow('N/A', 'KO', logStatus)
					self.preamtureExit = True
					self.eNBmbmsEnables[int(self.eNB_instance)] = False
					return
				else:
					if self.htmlObj is not None:
						self.htmlObj.CreateHtmlTestRow('N/A', 'OK', CONST.ALL_PROCESSES_OK)
			else:
				if self.htmlObj is not None:
					self.htmlObj.CreateHtmlTestRow('N/A', 'OK', CONST.ALL_PROCESSES_OK)
		self.eNBmbmsEnables[int(self.eNB_instance)] = False
		self.eNBstatuses[int(self.eNB_instance)] = -1

	def LogCollecteNB(self):
		mySSH = SSH.SSHConnection()
		mySSH.open(self.eNBIPAddress, self.eNBUserName, self.eNBPassword)
		mySSH.command('cd ' + self.eNBSourceCodePath, '\$', 5)
		mySSH.command('cd cmake_targets', '\$', 5)
		mySSH.command('echo ' + self.eNBPassword + ' | sudo -S rm -f enb.log.zip', '\$', 5)
		mySSH.command('echo ' + self.eNBPassword + ' | sudo -S zip enb.log.zip enb*.log core* enb_*record.raw enb_*.pcap enb_*txt', '\$', 60)
		mySSH.command('echo ' + self.eNBPassword + ' | sudo -S rm enb*.log core* enb_*record.raw enb_*.pcap enb_*txt', '\$', 5)
		mySSH.close()

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
		ulschReceiveOK = 0
		gnbRxTxWakeUpFailure = 0
		cdrxActivationMessageCount = 0
		dropNotEnoughRBs = 0
		mbmsRequestMsg = 0
		htmleNBFailureMsg = ''
		isRRU = False
		isSlave = False
		slaveReceivesFrameResyncCmd = False
		X2HO_state = CONST.X2_HO_REQ_STATE__IDLE
		X2HO_inNbProcedures = 0
		X2HO_outNbProcedures = 0
		for line in enb_log_file.readlines():
			if X2HO_state == CONST.X2_HO_REQ_STATE__IDLE:
				result = re.search('target eNB Receives X2 HO Req X2AP_HANDOVER_REQ', str(line))
				if result is not None:
					X2HO_state = CONST.X2_HO_REQ_STATE__TARGET_RECEIVES_REQ
				result = re.search('source eNB receives the X2 HO ACK X2AP_HANDOVER_REQ_ACK', str(line))
				if result is not None:
					X2HO_state = CONST.X2_HO_REQ_STATE__SOURCE_RECEIVES_REQ_ACK
			if X2HO_state == CONST.X2_HO_REQ_STATE__TARGET_RECEIVES_REQ:
				result = re.search('Received LTE_RRCConnectionReconfigurationComplete from UE', str(line))
				if result is not None:
					X2HO_state = CONST.X2_HO_REQ_STATE__TARGET_RRC_RECFG_COMPLETE
			if X2HO_state == CONST.X2_HO_REQ_STATE__TARGET_RRC_RECFG_COMPLETE:
				result = re.search('issue rrc_eNB_send_PATH_SWITCH_REQ', str(line))
				if result is not None:
					X2HO_state = CONST.X2_HO_REQ_STATE__TARGET_SENDS_SWITCH_REQ
			if X2HO_state == CONST.X2_HO_REQ_STATE__TARGET_SENDS_SWITCH_REQ:
				result = re.search('received path switch ack S1AP_PATH_SWITCH_REQ_ACK', str(line))
				if result is not None:
					X2HO_state = CONST.X2_HO_REQ_STATE__IDLE
					X2HO_inNbProcedures += 1
			if X2HO_state == CONST.X2_HO_REQ_STATE__SOURCE_RECEIVES_REQ_ACK:
				result = re.search('source eNB receives the X2 UE CONTEXT RELEASE X2AP_UE_CONTEXT_RELEASE', str(line))
				if result is not None:
					X2HO_state = CONST.X2_HO_REQ_STATE__IDLE
					X2HO_outNbProcedures += 1

			if self.eNBOptions[int(self.eNB_instance)] != '':
				res1 = re.search('max_rxgain (?P<requested_option>[0-9]+)', self.eNBOptions[int(self.eNB_instance)])
				res2 = re.search('max_rxgain (?P<applied_option>[0-9]+)',  str(line))
				if res1 is not None and res2 is not None:
					requested_option = int(res1.group('requested_option'))
					applied_option = int(res2.group('applied_option'))
					if requested_option == applied_option:
						htmleNBFailureMsg += '<span class="glyphicon glyphicon-ok-circle"></span> Command line option(s) correctly applied <span class="glyphicon glyphicon-arrow-right"></span> ' + self.eNBOptions[int(self.eNB_instance)] + '\n\n'
					else:
						htmleNBFailureMsg += '<span class="glyphicon glyphicon-ban-circle"></span> Command line option(s) NOT applied <span class="glyphicon glyphicon-arrow-right"></span> ' + self.eNBOptions[int(self.eNB_instance)] + '\n\n'
			result = re.search('Exiting OAI softmodem', str(line))
			if result is not None:
				exitSignalReceived = True
			result = re.search('[Ss]egmentation [Ff]ault', str(line))
			if result is not None and not exitSignalReceived:
				foundSegFault = True
			result = re.search('[Cc]ore [dD]ump', str(line))
			if result is not None and not exitSignalReceived:
				foundSegFault = True
			result = re.search('./ran_build/build/lte-softmodem', str(line))
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
			result = re.search('could not wakeup gNB rxtx process', str(line))
			if result is not None:
				gnbRxTxWakeUpFailure += 1
			result = re.search('ULSCH in error in round|ULSCH 0 in error', str(line))
			if result is not None:
				ulschFailure += 1
			result = re.search('ULSCH received ok', str(line))
			if result is not None:
				ulschReceiveOK += 1
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
		if self.air_interface == 'lte':
			nodeB_prefix = 'e'
		else:
			nodeB_prefix = 'g'
		if self.air_interface == 'nr':
			if ulschReceiveOK > 0:
				statMsg = nodeB_prefix + 'NB showed ' + str(ulschReceiveOK) + ' "ULSCH received ok" message(s)'
				logging.debug('\u001B[1;30;43m ' + statMsg + ' \u001B[0m')
				htmleNBFailureMsg += statMsg + '\n'
			if gnbRxTxWakeUpFailure > 0:
				statMsg = nodeB_prefix + 'NB showed ' + str(gnbRxTxWakeUpFailure) + ' "could not wakeup gNB rxtx process" message(s)'
				logging.debug('\u001B[1;30;43m ' + statMsg + ' \u001B[0m')
				htmleNBFailureMsg += statMsg + '\n'
		if uciStatMsgCount > 0:
			statMsg = nodeB_prefix + 'NB showed ' + str(uciStatMsgCount) + ' "uci->stat" message(s)'
			logging.debug('\u001B[1;30;43m ' + statMsg + ' \u001B[0m')
			htmleNBFailureMsg += statMsg + '\n'
		if pdcpFailure > 0:
			statMsg = nodeB_prefix + 'NB showed ' + str(pdcpFailure) + ' "PDCP Out of Resources" message(s)'
			logging.debug('\u001B[1;30;43m ' + statMsg + ' \u001B[0m')
			htmleNBFailureMsg += statMsg + '\n'
		if ulschFailure > 0:
			statMsg = nodeB_prefix + 'NB showed ' + str(ulschFailure) + ' "ULSCH in error in round" message(s)'
			logging.debug('\u001B[1;30;43m ' + statMsg + ' \u001B[0m')
			htmleNBFailureMsg += statMsg + '\n'
		if dropNotEnoughRBs > 0:
			statMsg = 'eNB showed ' + str(dropNotEnoughRBs) + ' "dropping, not enough RBs" message(s)'
			logging.debug('\u001B[1;30;43m ' + statMsg + ' \u001B[0m')
			htmleNBFailureMsg += statMsg + '\n'
		if rrcSetupComplete > 0:
			rrcMsg = nodeB_prefix + 'NB completed ' + str(rrcSetupComplete) + ' RRC Connection Setup(s)'
			logging.debug('\u001B[1;30;43m ' + rrcMsg + ' \u001B[0m')
			htmleNBFailureMsg += rrcMsg + '\n'
			rrcMsg = ' -- ' + str(rrcSetupComplete) + ' were completed'
			logging.debug('\u001B[1;30;43m ' + rrcMsg + ' \u001B[0m')
			htmleNBFailureMsg += rrcMsg + '\n'
		if rrcReleaseRequest > 0:
			rrcMsg = nodeB_prefix + 'NB requested ' + str(rrcReleaseRequest) + ' RRC Connection Release(s)'
			logging.debug('\u001B[1;30;43m ' + rrcMsg + ' \u001B[0m')
			htmleNBFailureMsg += rrcMsg + '\n'
		if rrcReconfigRequest > 0 or rrcReconfigComplete > 0:
			rrcMsg = nodeB_prefix + 'NB requested ' + str(rrcReconfigRequest) + ' RRC Connection Reconfiguration(s)'
			logging.debug('\u001B[1;30;43m ' + rrcMsg + ' \u001B[0m')
			htmleNBFailureMsg += rrcMsg + '\n'
			rrcMsg = ' -- ' + str(rrcReconfigComplete) + ' were completed'
			logging.debug('\u001B[1;30;43m ' + rrcMsg + ' \u001B[0m')
			htmleNBFailureMsg += rrcMsg + '\n'
		if rrcReestablishRequest > 0 or rrcReestablishComplete > 0 or rrcReestablishReject > 0:
			rrcMsg = nodeB_prefix + 'NB requested ' + str(rrcReestablishRequest) + ' RRC Connection Reestablishment(s)'
			logging.debug('\u001B[1;30;43m ' + rrcMsg + ' \u001B[0m')
			htmleNBFailureMsg += rrcMsg + '\n'
			rrcMsg = ' -- ' + str(rrcReestablishComplete) + ' were completed'
			logging.debug('\u001B[1;30;43m ' + rrcMsg + ' \u001B[0m')
			htmleNBFailureMsg += rrcMsg + '\n'
			rrcMsg = ' -- ' + str(rrcReestablishReject) + ' were rejected'
			logging.debug('\u001B[1;30;43m ' + rrcMsg + ' \u001B[0m')
			htmleNBFailureMsg += rrcMsg + '\n'
		if self.eNBmbmsEnables[int(self.eNB_instance)]:
			if mbmsRequestMsg > 0:
				rrcMsg = 'eNB requested ' + str(mbmsRequestMsg) + ' times the RLC for MBMS USER-PLANE'
				logging.debug('\u001B[1;30;43m ' + rrcMsg + ' \u001B[0m')
				htmleNBFailureMsg += rrcMsg + '\n'
		if X2HO_inNbProcedures > 0:
			rrcMsg = 'eNB completed ' + str(X2HO_inNbProcedures) + ' X2 Handover Connection procedure(s)'
			logging.debug('\u001B[1;30;43m ' + rrcMsg + ' \u001B[0m')
			htmleNBFailureMsg += rrcMsg + '\n'
		if X2HO_outNbProcedures > 0:
			rrcMsg = 'eNB completed ' + str(X2HO_outNbProcedures) + ' X2 Handover Release procedure(s)'
			logging.debug('\u001B[1;30;43m ' + rrcMsg + ' \u001B[0m')
			htmleNBFailureMsg += rrcMsg + '\n'
		if self.eNBOptions[int(self.eNB_instance)] != '':
			res1 = re.search('drx_Config_present prSetup', self.eNBOptions[int(self.eNB_instance)])
			if res1 is not None:
				if cdrxActivationMessageCount > 0:
					rrcMsg = 'eNB activated the CDRX Configuration for ' + str(cdrxActivationMessageCount) + ' time(s)'
					logging.debug('\u001B[1;30;43m ' + rrcMsg + ' \u001B[0m')
					htmleNBFailureMsg += rrcMsg + '\n'
				else:
					rrcMsg = 'eNB did NOT ACTIVATE the CDRX Configuration'
					logging.debug('\u001B[1;37;43m ' + rrcMsg + ' \u001B[0m')
					htmleNBFailureMsg += rrcMsg + '\n'
		if rachCanceledProcedure > 0:
			rachMsg = nodeB_prefix + 'NB cancelled ' + str(rachCanceledProcedure) + ' RA procedure(s)'
			logging.debug('\u001B[1;30;43m ' + rachMsg + ' \u001B[0m')
			htmleNBFailureMsg += rachMsg + '\n'
		if isRRU:
			if isSlave:
				if slaveReceivesFrameResyncCmd:
					rruMsg = 'Slave RRU received the RRU_frame_resynch command from RAU'
					logging.debug('\u001B[1;30;43m ' + rruMsg + ' \u001B[0m')
					htmleNBFailureMsg += rruMsg + '\n'
				else:
					rruMsg = 'Slave RRU DID NOT receive the RRU_frame_resynch command from RAU'
					logging.debug('\u001B[1;37;41m ' + rruMsg + ' \u001B[0m')
					htmleNBFailureMsg += rruMsg + '\n'
					self.prematureExit(True)
					return CONST.ENB_PROCESS_SLAVE_RRU_NOT_SYNCED
		if foundSegFault:
			logging.debug('\u001B[1;37;41m ' + nodeB_prefix + 'NB ended with a Segmentation Fault! \u001B[0m')
			if self.htmlObj is not None:
				self.htmlObj.SetHmleNBFailureMsg(htmleNBFailureMsg)
			return CONST.ENB_PROCESS_SEG_FAULT
		if foundAssertion:
			logging.debug('\u001B[1;37;41m ' + nodeB_prefix + 'NB ended with an assertion! \u001B[0m')
			htmleNBFailureMsg += msgAssertion
			if self.htmlObj is not None:
				self.htmlObj.SetHmleNBFailureMsg(htmleNBFailureMsg)
			return CONST.ENB_PROCESS_ASSERTION
		if foundRealTimeIssue:
			logging.debug('\u001B[1;37;41m ' + nodeB_prefix + 'NB faced real time issues! \u001B[0m')
			htmleNBFailureMsg += nodeB_prefix + 'NB faced real time issues!\n'
			#return CONST.ENB_PROCESS_REALTIME_ISSUE
		if rlcDiscardBuffer > 0:
			rlcMsg = nodeB_prefix + 'NB RLC discarded ' + str(rlcDiscardBuffer) + ' buffer(s)'
			logging.debug('\u001B[1;37;41m ' + rlcMsg + ' \u001B[0m')
			htmleNBFailureMsg += rlcMsg + '\n'
			if self.htmlObj is not None:
				self.htmlObj.SetHmleNBFailureMsg(htmleNBFailureMsg)
			return CONST.ENB_PROCESS_REALTIME_ISSUE
		if self.htmlObj is not None:
			self.htmlObj.SetHmleNBFailureMsg(htmleNBFailureMsg)
		return 0
