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
# 
#
#   Required Python Version
#     Python 3.x
#
#   Required Python Package
#     pexpect
#---------------------------------------------------------------------


#-----------------------------------------------------------
# Import Libs
#-----------------------------------------------------------
import sys		# arg
import re		# reg
import pexpect	# pexpect
import time		# sleep
import os
import subprocess
import xml.etree.ElementTree as ET
import logging
import datetime
import signal
import statistics as stat
from multiprocessing import Process, Lock, SimpleQueue
import concurrent.futures

#import our libs
import helpreadme as HELP
import constants as CONST
import cls_cluster as OC
import sshconnection

import cls_module_ue
import cls_cmd

logging.getLogger("matplotlib").setLevel(logging.WARNING)
import matplotlib.pyplot as plt
import numpy as np

#-----------------------------------------------------------
# OaiCiTest Class Definition
#-----------------------------------------------------------
class OaiCiTest():
	
	def __init__(self):
		self.ranRepository = ''
		self.ranBranch = ''
		self.ranCommitID = ''
		self.ranAllowMerge = False
		self.ranTargetBranch = ''

		self.FailReportCnt = 0
		self.testCase_id = ''
		self.testXMLfiles = []
		self.testUnstable = False
		self.testMinStableId = '999999'
		self.testStabilityPointReached = False
		self.desc = ''
		self.ping_args = ''
		self.ping_packetloss_threshold = ''
		self.ping_rttavg_threshold =''
		self.iperf_args = ''
		self.iperf_packetloss_threshold = ''
		self.iperf_bitrate_threshold = ''
		self.iperf_profile = ''
		self.iperf_options = ''
		self.iperf_direction = ''
		self.nbMaxUEtoAttach = -1
		self.UEDevices = []
		self.UEDevicesStatus = []
		self.UEDevicesRemoteServer = []
		self.UEDevicesRemoteUser = []
		self.UEDevicesOffCmd = []
		self.UEDevicesOnCmd = []
		self.UEDevicesRebootCmd = []
		self.idle_sleep_time = 0
		self.x2_ho_options = 'network'
		self.x2NbENBs = 0
		self.x2ENBBsIds = []
		self.x2ENBConnectedUEs = []
		self.repeatCounts = []
		self.finalStatus = False
		self.UEIPAddress = ''
		self.UEUserName = ''
		self.UEPassword = ''
		self.UE_instance = 0
		self.UESourceCodePath = ''
		self.UELogFile = ''
		self.Build_OAI_UE_args = ''
		self.Initialize_OAI_UE_args = ''
		self.clean_repository = True
		self.air_interface=''
		self.ue_ids = []
		self.cmd_prefix = '' # prefix before {lte,nr}-uesoftmodem


	def BuildOAIUE(self,HTML):
		if self.UEIPAddress == '' or self.ranRepository == '' or self.ranBranch == '' or self.UEUserName == '' or self.UEPassword == '' or self.UESourceCodePath == '':
			HELP.GenericHelp(CONST.Version)
			sys.exit('Insufficient Parameter')
		SSH = sshconnection.SSHConnection()
		SSH.open(self.UEIPAddress, self.UEUserName, self.UEPassword)
		result = re.search('--nrUE', self.Build_OAI_UE_args)
		if result is not None:
			self.air_interface='nr-uesoftmodem'
			ue_prefix = 'NR '
		else:
			self.air_interface='lte-uesoftmodem'
			ue_prefix = ''
		result = re.search('([a-zA-Z0-9\:\-\.\/])+\.git', self.ranRepository)
		if result is not None:
			full_ran_repo_name = self.ranRepository.replace('git/', 'git')
		else:
			full_ran_repo_name = self.ranRepository + '.git'
		SSH.command('mkdir -p ' + self.UESourceCodePath, '\$', 5)
		SSH.command('cd ' + self.UESourceCodePath, '\$', 5)
		SSH.command('if [ ! -e .git ]; then stdbuf -o0 git clone ' + full_ran_repo_name + ' .; else stdbuf -o0 git fetch --prune; fi', '\$', 600)
		# here add a check if git clone or git fetch went smoothly
		SSH.command('git config user.email "jenkins@openairinterface.org"', '\$', 5)
		SSH.command('git config user.name "OAI Jenkins"', '\$', 5)
		if self.clean_repository:
			SSH.command('ls *.txt', '\$', 5)
			result = re.search('LAST_BUILD_INFO', SSH.getBefore())
			if result is not None:
				mismatch = False
				SSH.command('grep --colour=never SRC_COMMIT LAST_BUILD_INFO.txt', '\$', 2)
				result = re.search(self.ranCommitID, SSH.getBefore())
				if result is None:
					mismatch = True
				SSH.command('grep --colour=never MERGED_W_TGT_BRANCH LAST_BUILD_INFO.txt', '\$', 2)
				if self.ranAllowMerge:
					result = re.search('YES', SSH.getBefore())
					if result is None:
						mismatch = True
					SSH.command('grep --colour=never TGT_BRANCH LAST_BUILD_INFO.txt', '\$', 2)
					if self.ranTargetBranch == '':
						result = re.search('develop', SSH.getBefore())
					else:
						result = re.search(self.ranTargetBranch, SSH.getBefore())
					if result is None:
						mismatch = True
				else:
					result = re.search('NO', SSH.getBefore())
					if result is None:
						mismatch = True
				if not mismatch:
					SSH.close()
					HTML.CreateHtmlTestRow(self.Build_OAI_UE_args, 'OK', CONST.ALL_PROCESSES_OK)
					return

			SSH.command('echo ' + self.UEPassword + ' | sudo -S git clean -x -d -ff', '\$', 30)

		# if the commit ID is provided use it to point to it
		if self.ranCommitID != '':
			SSH.command('git checkout -f ' + self.ranCommitID, '\$', 30)
		# if the branch is not develop, then it is a merge request and we need to do 
		# the potential merge. Note that merge conflicts should already been checked earlier
		if self.ranAllowMerge:
			if self.ranTargetBranch == '':
				if (self.ranBranch != 'develop') and (self.ranBranch != 'origin/develop'):
					SSH.command('git merge --ff origin/develop -m "Temporary merge for CI"', '\$', 30)
			else:
				logging.debug('Merging with the target branch: ' + self.ranTargetBranch)
				SSH.command('git merge --ff origin/' + self.ranTargetBranch + ' -m "Temporary merge for CI"', '\$', 30)
		SSH.command('source oaienv', '\$', 5)
		SSH.command('cd cmake_targets', '\$', 5)
		SSH.command('mkdir -p log', '\$', 5)
		SSH.command('chmod 777 log', '\$', 5)
		# no need to remove in log (git clean did the trick)
		SSH.command('stdbuf -o0 ./build_oai ' + self.Build_OAI_UE_args + ' 2>&1 | stdbuf -o0 tee compile_oai_ue.log', 'Bypassing the Tests|build have failed', 1200)
		SSH.command('ls ran_build/build', '\$', 3)
		SSH.command('ls ran_build/build', '\$', 3)
		buildStatus = True
		result = re.search(self.air_interface, SSH.getBefore())
		if result is None:
			buildStatus = False
		SSH.command('mkdir -p build_log_' + self.testCase_id, '\$', 5)
		SSH.command('mv log/* ' + 'build_log_' + self.testCase_id, '\$', 5)
		SSH.command('mv compile_oai_ue.log ' + 'build_log_' + self.testCase_id, '\$', 5)
		if buildStatus:
			# Generating a BUILD INFO file
			SSH.command('echo "SRC_BRANCH: ' + self.ranBranch + '" > ../LAST_BUILD_INFO.txt', '\$', 2)
			SSH.command('echo "SRC_COMMIT: ' + self.ranCommitID + '" >> ../LAST_BUILD_INFO.txt', '\$', 2)
			if self.ranAllowMerge:
				SSH.command('echo "MERGED_W_TGT_BRANCH: YES" >> ../LAST_BUILD_INFO.txt', '\$', 2)
				if self.ranTargetBranch == '':
					SSH.command('echo "TGT_BRANCH: develop" >> ../LAST_BUILD_INFO.txt', '\$', 2)
				else:
					SSH.command('echo "TGT_BRANCH: ' + self.ranTargetBranch + '" >> ../LAST_BUILD_INFO.txt', '\$', 2)
			else:
				SSH.command('echo "MERGED_W_TGT_BRANCH: NO" >> ../LAST_BUILD_INFO.txt', '\$', 2)
			SSH.close()
			HTML.CreateHtmlTestRow(self.Build_OAI_UE_args, 'OK', CONST.ALL_PROCESSES_OK, 'OAI UE')
		else:
			SSH.close()
			logging.error('\u001B[1m Building OAI UE Failed\u001B[0m')
			HTML.CreateHtmlTestRow(self.Build_OAI_UE_args, 'KO', CONST.ALL_PROCESSES_OK, 'OAI UE')
			HTML.CreateHtmlTabFooter(False)
			self.ConditionalExit()


	def InitializeUE(self, HTML):
		ues = [cls_module_ue.Module_UE(n.strip()) for n in self.ue_ids]
		messages = []
		with concurrent.futures.ThreadPoolExecutor() as executor:
			futures = [executor.submit(ue.initialize) for ue in ues]
			for f, ue in zip(futures, ues):
				uename = f'UE {ue.getName()}'
				messages.append(f'{uename}: initialized' if f.result() else f'{uename}: ERROR during Initialization')
			[f.result() for f in futures]
		HTML.CreateHtmlTestRowQueue('N/A', 'OK', messages)

	def InitializeOAIUE(self,HTML,RAN,EPC,CONTAINERS):
		if self.UEIPAddress == '' or self.UEUserName == '' or self.UEPassword == '' or self.UESourceCodePath == '':
			HELP.GenericHelp(CONST.Version)
			sys.exit('Insufficient Parameter')

			
		if self.air_interface == 'lte-uesoftmodem':
			result = re.search('--no-L2-connect', str(self.Initialize_OAI_UE_args))
			if result is None:
				check_eNB = True
				check_OAI_UE = False
			UE_prefix = ''
		else:
			UE_prefix = 'NR '
		SSH = sshconnection.SSHConnection()
		SSH.open(self.UEIPAddress, self.UEUserName, self.UEPassword)
		SSH.command('cd ' + self.UESourceCodePath, '\$', 5)
		# Initialize_OAI_UE_args usually start with -C and followed by the location in repository
		SSH.command('source oaienv', '\$', 5)
		SSH.command('cd cmake_targets/ran_build/build', '\$', 5)
		if self.air_interface == 'lte-uesoftmodem':
			result = re.search('--no-L2-connect', str(self.Initialize_OAI_UE_args))
			# We may have to regenerate the .u* files
			if result is None:
				SSH.command('ls /tmp/*.sed', '\$', 5)
				result = re.search('adapt_usim_parameters', SSH.getBefore())
				if result is not None:
					SSH.command('sed -f /tmp/adapt_usim_parameters.sed ../../../openair3/NAS/TOOLS/ue_eurecom_test_sfr.conf > ../../../openair3/NAS/TOOLS/ci-ue_eurecom_test_sfr.conf', '\$', 5)
				else:
					SSH.command('sed -e "s#93#92#" -e "s#8baf473f2f8fd09487cccbd7097c6862#fec86ba6eb707ed08905757b1bb44b8f#" -e "s#e734f8734007d6c5ce7a0508809e7e9c#C42449363BBAD02B66D16BC975D77CC1#" ../../../openair3/NAS/TOOLS/ue_eurecom_test_sfr.conf > ../../../openair3/NAS/TOOLS/ci-ue_eurecom_test_sfr.conf', '\$', 5)
				SSH.command('echo ' + self.UEPassword + ' | sudo -S rm -Rf .u*', '\$', 5)
				SSH.command('echo ' + self.UEPassword + ' | sudo -S ../../nas_sim_tools/build/conf2uedata -c ../../../openair3/NAS/TOOLS/ci-ue_eurecom_test_sfr.conf -o .', '\$', 5)
		else:
			SSH.command('if [ -e rbconfig.raw ]; then echo ' + self.UEPassword + ' | sudo -S rm rbconfig.raw; fi', '\$', 5)
			SSH.command('if [ -e reconfig.raw ]; then echo ' + self.UEPassword + ' | sudo -S rm reconfig.raw; fi', '\$', 5)
			# Copy the RAW files from gNB running directory (maybe on another machine)
			copyin_res = SSH.copyin(RAN.eNBIPAddress, RAN.eNBUserName, RAN.eNBPassword, RAN.eNBSourceCodePath + '/cmake_targets/rbconfig.raw', '.')
			if (copyin_res == 0):
				SSH.copyout(self.UEIPAddress, self.UEUserName, self.UEPassword, './rbconfig.raw', self.UESourceCodePath + '/cmake_targets/ran_build/build')
			copyin_res = SSH.copyin(RAN.eNBIPAddress, RAN.eNBUserName, RAN.eNBPassword, RAN.eNBSourceCodePath + '/cmake_targets/reconfig.raw', '.')
			if (copyin_res == 0):
				SSH.copyout(self.UEIPAddress, self.UEUserName, self.UEPassword, './reconfig.raw', self.UESourceCodePath + '/cmake_targets/ran_build/build')
		SSH.command(f'echo "ulimit -c unlimited && {self.cmd_prefix} ./{self.air_interface} {self.Initialize_OAI_UE_args}" > ./my-lte-uesoftmodem-run{self.UE_instance}.sh', '\$', 5)
		SSH.command('chmod 775 ./my-lte-uesoftmodem-run' + str(self.UE_instance) + '.sh', '\$', 5)
		SSH.command('echo ' + self.UEPassword + ' | sudo -S rm -Rf ' + self.UESourceCodePath + '/cmake_targets/ue_' + self.testCase_id + '.log', '\$', 5)
		self.UELogFile = 'ue_' + self.testCase_id + '.log'

		# We are now looping several times to hope we really sync w/ an eNB
		doOutterLoop = True
		outterLoopCounter = 5
		gotSyncStatus = True
		fullSyncStatus = True
		while (doOutterLoop):
			SSH.command('cd ' + self.UESourceCodePath + '/cmake_targets/ran_build/build', '\$', 5)
			SSH.command('echo ' + self.UEPassword + ' | sudo -S rm -Rf ' + self.UESourceCodePath + '/cmake_targets/ue_' + self.testCase_id + '.log', '\$', 5)
			SSH.command('echo $USER; nohup sudo -E stdbuf -o0 ./my-lte-uesoftmodem-run' + str(self.UE_instance) + '.sh' + ' > ' + self.UESourceCodePath + '/cmake_targets/ue_' + self.testCase_id + '.log ' + ' 2>&1 &', self.UEUserName, 5)
			time.sleep(6)
			SSH.command('cd ../..', '\$', 5)
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
				SSH.command('stdbuf -o0 cat ue_' + self.testCase_id + '.log | egrep --text --color=never -i "wait|sync"', '\$', 4)
				if self.air_interface == 'nr-uesoftmodem':
					result = re.search('Starting sync detection', SSH.getBefore())
				else:
					result = re.search('got sync', SSH.getBefore())
				if result is None:
					time.sleep(10)
				else:
					doLoop = False
					logging.debug('Found "got sync" message!')
			if gotSyncStatus == False:
				# we certainly need to stop the lte-uesoftmodem process if it is still running!
				SSH.command('ps -aux | grep --text --color=never softmodem | grep -v grep', '\$', 4)
				result = re.search('-uesoftmodem', SSH.getBefore())
				if result is not None:
					SSH.command('echo ' + self.UEPassword + ' | sudo -S killall --signal=SIGINT -r *-uesoftmodem', '\$', 4)
					time.sleep(3)
				outterLoopCounter = outterLoopCounter - 1
				if (outterLoopCounter == 0):
					doOutterLoop = False
				continue
			# We are now checking if sync w/ eNB DOES NOT OCCUR
			# Usually during the cell synchronization stage, the UE returns with No cell synchronization message
			# That is the case for LTE
			# In NR case, it's a positive message that will show if synchronization occurs
			doLoop = True
			if self.air_interface == 'nr-uesoftmodem':
				loopCounter = 10
			else:
				# We are now checking if sync w/ eNB DOES NOT OCCUR
				# Usually during the cell synchronization stage, the UE returns with No cell synchronization message
				loopCounter = 10
			while (doLoop):
				loopCounter = loopCounter - 1
				if (loopCounter == 0):
					if self.air_interface == 'nr-uesoftmodem':
						# Here we do have great chances that UE did NOT cell-sync w/ gNB
						doLoop = False
						fullSyncStatus = False
						logging.debug('Never seen the NR-Sync message (Measured Carrier Frequency) --> try again')
						time.sleep(6)
						# Stopping the NR-UE  
						SSH.command('ps -aux | grep --text --color=never softmodem | grep -v grep', '\$', 4)
						result = re.search('nr-uesoftmodem', SSH.getBefore())
						if result is not None:
							SSH.command('echo ' + self.UEPassword + ' | sudo -S killall --signal=SIGINT nr-uesoftmodem', '\$', 4)
						time.sleep(6)
					else:
						# Here we do have a great chance that the UE did cell-sync w/ eNB
						doLoop = False
						doOutterLoop = False
						fullSyncStatus = True
						continue
				SSH.command('stdbuf -o0 cat ue_' + self.testCase_id + '.log | egrep --text --color=never -i "wait|sync|Frequency"', '\$', 4)
				if self.air_interface == 'nr-uesoftmodem':
					# Positive messaging -->
					result = re.search('Measured Carrier Frequency', SSH.getBefore())
					if result is not None:
						doLoop = False
						doOutterLoop = False
						fullSyncStatus = True
					else:
						time.sleep(6)
				else:
					# Negative messaging -->
					result = re.search('No cell synchronization found', SSH.getBefore())
					if result is None:
						time.sleep(6)
					else:
						doLoop = False
						fullSyncStatus = False
						logging.debug('Found: "No cell synchronization" message! --> try again')
						time.sleep(6)
						SSH.command('ps -aux | grep --text --color=never softmodem | grep -v grep', '\$', 4)
						result = re.search('lte-uesoftmodem', SSH.getBefore())
						if result is not None:
							SSH.command('echo ' + self.UEPassword + ' | sudo -S killall --signal=SIGINT lte-uesoftmodem', '\$', 4)
			outterLoopCounter = outterLoopCounter - 1
			if (outterLoopCounter == 0):
				doOutterLoop = False

		if fullSyncStatus and gotSyncStatus:
			doInterfaceCheck = False
			if self.air_interface == 'lte-uesoftmodem':
				result = re.search('--no-L2-connect', str(self.Initialize_OAI_UE_args))
				if result is None:
					doInterfaceCheck = True
			# For the moment, only in explicit noS1 without kernel module (ie w/ tunnel interface)
			if self.air_interface == 'nr-uesoftmodem':
				result = re.search('--noS1', str(self.Initialize_OAI_UE_args))
				if result is not None:
					doInterfaceCheck = True
			if doInterfaceCheck:
				SSH.command('ifconfig oaitun_ue1', '\$', 4)
				SSH.command('ifconfig oaitun_ue1', '\$', 4)
				# ifconfig output is different between ubuntu 16 and ubuntu 18
				result = re.search('inet addr:[0-9]|inet [0-9]', SSH.getBefore())
				if result is not None:
					logging.debug('\u001B[1m oaitun_ue1 interface is mounted and configured\u001B[0m')
					tunnelInterfaceStatus = True
				else:
					logging.debug(SSH.getBefore())
					logging.error('\u001B[1m oaitun_ue1 interface is either NOT mounted or NOT configured\u001B[0m')
					tunnelInterfaceStatus = False
				if RAN.eNBmbmsEnables[0]:
					SSH.command('ifconfig oaitun_uem1', '\$', 4)
					result = re.search('inet addr', SSH.getBefore())
					if result is not None:
						logging.debug('\u001B[1m oaitun_uem1 interface is mounted and configured\u001B[0m')
						tunnelInterfaceStatus = tunnelInterfaceStatus and True
					else:
						logging.error('\u001B[1m oaitun_uem1 interface is either NOT mounted or NOT configured\u001B[0m')
						tunnelInterfaceStatus = False
			else:
				tunnelInterfaceStatus = True
		else:
			tunnelInterfaceStatus = True

		SSH.close()
		if fullSyncStatus and gotSyncStatus and tunnelInterfaceStatus:
			HTML.CreateHtmlTestRow(self.air_interface + ' ' + self.Initialize_OAI_UE_args, 'OK', CONST.ALL_PROCESSES_OK, 'OAI UE')
			logging.debug('\u001B[1m Initialize OAI UE Completed\u001B[0m')
		else:
			if self.air_interface == 'lte-uesoftmodem':
				if RAN.eNBmbmsEnables[0]:
					HTML.htmlUEFailureMsg='oaitun_ue1/oaitun_uem1 interfaces are either NOT mounted or NOT configured'
				else:
					HTML.htmlUEFailureMsg='oaitun_ue1 interface is either NOT mounted or NOT configured'
				HTML.CreateHtmlTestRow(self.air_interface + ' ' + self.Initialize_OAI_UE_args, 'KO', CONST.OAI_UE_PROCESS_NO_TUNNEL_INTERFACE, 'OAI UE')
			else:
				HTML.htmlUEFailureMsg='nr-uesoftmodem did NOT synced'
				HTML.CreateHtmlTestRow(self.air_interface + ' ' +  self.Initialize_OAI_UE_args, 'KO', CONST.OAI_UE_PROCESS_COULD_NOT_SYNC, 'OAI UE')
			logging.error('\033[91mInitialize OAI UE Failed! \033[0m')
			self.AutoTerminateUEandeNB(HTML,RAN,EPC,CONTAINERS)

	def AttachUE(self, HTML, RAN, EPC, CONTAINERS):
		ues = [cls_module_ue.Module_UE(n.strip()) for n in self.ue_ids]
		with concurrent.futures.ThreadPoolExecutor() as executor:
			futures = [executor.submit(ue.attach) for ue in ues]
			attached = [f.result() for f in futures]
			futures = [executor.submit(ue.checkMTU) for ue in ues]
			mtus = [f.result() for f in futures]
			messages = [f"UE {ue.getName()}: {ue.getIP()}" for ue in ues]
		if all(attached) and all(mtus):
			HTML.CreateHtmlTestRowQueue('N/A', 'OK', messages)
		else:
			logging.error(f'error attaching or wrong MTU: attached {attached}, mtus {mtus}')
			HTML.CreateHtmlTestRowQueue('N/A', 'KO', ["Could not retrieve UE IP address(es) or MTU(s) wrong!"])
			self.AutoTerminateUEandeNB(HTML, RAN, EPC, CONTAINERS)

	def DetachUE(self, HTML):
		ues = [cls_module_ue.Module_UE(n.strip()) for n in self.ue_ids]
		with concurrent.futures.ThreadPoolExecutor() as executor:
			futures = [executor.submit(ue.detach) for ue in ues]
			[f.result() for f in futures]
			messages = [f"UE {ue.getName()}: detached" for ue in ues]
		HTML.CreateHtmlTestRowQueue('NA', 'OK', messages)

	def DataDisableUE(self, HTML):
		ues = [cls_module_ue.Module_UE(n.strip()) for n in self.ue_ids]
		with concurrent.futures.ThreadPoolExecutor() as executor:
			futures = [executor.submit(ue.dataDisable) for ue in ues]
			status = [f.result() for f in futures]
		if all(status):
			messages = [f"UE {ue.getName()}: data disabled" for ue in ues]
			HTML.CreateHtmlTestRowQueue('NA', 'OK', messages)
		else:
			logging.error(f'error enabling data: {status}')
			HTML.CreateHtmlTestRowQueue('N/A', 'KO', ["Could not disable UE data!"])

	def DataEnableUE(self, HTML):
		ues = [cls_module_ue.Module_UE(n.strip()) for n in self.ue_ids]
		logging.debug(f'disabling data for UEs {ues}')
		with concurrent.futures.ThreadPoolExecutor() as executor:
			futures = [executor.submit(ue.dataEnable) for ue in ues]
			status = [f.result() for f in futures]
		if all(status):
			messages = [f"UE {ue.getName()}: data enabled" for ue in ues]
			HTML.CreateHtmlTestRowQueue('NA', 'OK', messages)
		else:
			logging.error(f'error enabling data: {status}')
			HTML.CreateHtmlTestRowQueue('N/A', 'KO', ["Could not enable UE data!"])

	def CheckStatusUE(self,HTML):
		ues = [cls_module_ue.Module_UE(n.strip()) for n in self.ue_ids]
		logging.debug(f'checking status of UEs {ues}')
		messages = []
		with concurrent.futures.ThreadPoolExecutor() as executor:
			futures = [executor.submit(ue.check) for ue in ues]
			messages = [f.result() for f in futures]
		HTML.CreateHtmlTestRowQueue('NA', 'OK', messages)

	def ping_iperf_wrong_exit(self, lock, UE_IPAddress, device_id, statusQueue, message):
		logging.error(f"ue {device_id} {UE_IPAddress}: {message}")
		lock.acquire()
		statusQueue.put(-1)
		statusQueue.put(device_id)
		statusQueue.put(UE_IPAddress)
		statusQueue.put(message)
		lock.release()

	def Ping_common(self, EPC, ue, RAN):
		# Launch ping on the EPC side (true for ltebox and old open-air-cn)
		ping_status = 0
		ueIP = ue.getIP()
		if not ueIP:
			return (False, f"UE {ue.getName()} has no IP address")
		ping_log_file = f'ping_{self.testCase_id}_{ue.getName()}.log'
		ping_time = re.findall("-c *(\d+)",str(self.ping_args))
		#target address is different depending on EPC type
		if re.match('OAI-Rel14-Docker', EPC.Type, re.IGNORECASE):
			Target = EPC.MmeIPAddress
		elif re.match('OAICN5G', EPC.Type, re.IGNORECASE):
			Target = EPC.MmeIPAddress
		elif re.match('OC-OAI-CN5G', EPC.Type, re.IGNORECASE):
			Target = "172.21.6.100"
		else:
			Target = EPC.IPAddress
		#ping from module NIC rather than IP address to make sure round trip is over the air
		interface = f'-I {ue.getIFName()}' if ue.getIFName() else ''
		ping_cmd = f'{ue.getCmdPrefix()} ping {interface} {self.ping_args} {Target} &> /tmp/{ping_log_file}'
		cmd = cls_cmd.getConnection(ue.getHost())
		response = cmd.run(ping_cmd, timeout=int(ping_time[0])*1.5)
		if response.returncode != 0:
			ping_status = -1
		else:
			#copy the ping log file to have it locally for analysis (ping stats)
			cmd.copyin(src=f'/tmp/{ping_log_file}', tgt=ping_log_file)

		cmd.close()

		# TIMEOUT CASE
		ue_header = f'UE {ue.getName()} ({ueIP})'
		if ping_status < 0:
			message = ue_header + ': ping crashed: TIMEOUT?'
			logging.error('\u001B[1;37;41m ' + message + ' \u001B[0m')
			return (False, message)
		#search is done on cat result
		logging.debug('Analyzing Ping log file : ' + os.getcwd() + '/' + ping_log_file)
		with open(ping_log_file, 'r') as f:
			ping_output = "".join(f.readlines())
		result = re.search(', (?P<packetloss>[0-9\.]+)% packet loss, time [0-9\.]+ms', ping_output)
		if result is None:
			message = ue_header + ': Packet Loss Not Found!'
			logging.error('\u001B[1;37;41m ' + message + ' \u001B[0m')
			return (False, message)
		packetloss = result.group('packetloss')
		result = re.search('rtt min\/avg\/max\/mdev = (?P<rtt_min>[0-9\.]+)\/(?P<rtt_avg>[0-9\.]+)\/(?P<rtt_max>[0-9\.]+)\/[0-9\.]+ ms', ping_output)
		if result is None:
			message = ue_header + ': Ping RTT_Min RTT_Avg RTT_Max Not Found!'
			logging.error('\u001B[1;37;41m ' + message + ' \u001B[0m')
			return (False, message)
		rtt_min = result.group('rtt_min')
		rtt_avg = result.group('rtt_avg')
		rtt_max = result.group('rtt_max')

		pal_msg = f'Packet Loss: {packetloss}%'
		min_msg = f'RTT(Min)   : {rtt_min} ms'
		avg_msg = f'RTT(Avg)   : {rtt_avg} ms'
		max_msg = f'RTT(Max)   : {rtt_max} ms'

		logging.info(f'\u001B[1;37;44m ping result for {ue_header} \u001B[0m')
		logging.info(f'\u001B[1;34m    ' + pal_msg + '\u001B[0m')
		logging.info(f'\u001B[1;34m    ' + min_msg + '\u001B[0m')
		logging.info(f'\u001B[1;34m    ' + avg_msg + '\u001B[0m')
		logging.info(f'\u001B[1;34m    ' + max_msg + '\u001B[0m')

		message = f'{ue_header}\n{pal_msg}\n{min_msg}\n{avg_msg}\n{max_msg}'

		#checking packet loss compliance
		if float(packetloss) > float(self.ping_packetloss_threshold):
			message += '\nPacket Loss too high'
			logging.error(f'\u001B[1;37;41m Packet Loss too high; Target: {self.ping_packetloss_threshold}%\u001B[0m')
			return (False, message)
		elif float(packetloss) > 0:
			message += '\nPacket Loss is not 0%'
			logging.info('\u001B[1;30;43m Packet Loss is not 0% \u001B[0m')

		if self.ping_rttavg_threshold != '':
			if float(rtt_avg) > float(self.ping_rttavg_threshold):
				ping_rttavg_error_msg = f'RTT(Avg) too high: {rtt_avg} ms; Target: {self.ping_rttavg_threshold} ms'
				message += '\n' + ping_rttavg_error_msg
				logging.error('\u001B[1;37;41m'+ ping_rttavg_error_msg +' \u001B[0m')
				return (False, message)

		return (True, message)

	def Ping(self,HTML,RAN,EPC,CONTAINERS):
		if EPC.IPAddress == '' or EPC.UserName == '' or EPC.Password == '' or EPC.SourceCodePath == '':
			HELP.GenericHelp(CONST.Version)
			sys.exit('Insufficient Parameter')

		if self.ue_ids == []:
			raise Exception("no module names in self.ue_ids provided")

		ues = [cls_module_ue.Module_UE(n.strip()) for n in self.ue_ids]
		logging.debug(ues)
		with concurrent.futures.ThreadPoolExecutor() as executor:
			futures = [executor.submit(self.Ping_common, EPC, ue, RAN) for ue in ues]
			results = [f.result() for f in futures]
			# each result in results is a tuple, first member goes to successes, second to messages
			successes, messages = map(list, zip(*results))
		if len(successes) == len(ues) and all(successes):
			HTML.CreateHtmlTestRowQueue(self.ping_args, 'OK', messages)
		else:
			HTML.CreateHtmlTestRowQueue(self.ping_args, 'KO', messages)
			self.AutoTerminateUEandeNB(HTML,RAN,EPC,CONTAINERS)

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

	def Iperf_analyzeV2TCPOutput(self, lock, UE_IPAddress, device_id, statusQueue, iperf_real_options,EPC,SSH, filename):

		SSH.command(f'awk -f /tmp/tcp_iperf_stats.awk {filename}', '\$', 5)
		result = re.search('Avg Bitrate : (?P<average>[0-9\.]+ Mbits\/sec) Max Bitrate : (?P<maximum>[0-9\.]+ Mbits\/sec) Min Bitrate : (?P<minimum>[0-9\.]+ Mbits\/sec)', SSH.getBefore())
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

	def Iperf_analyzeV2Output(self, lock, UE_IPAddress, device_id, statusQueue, iperf_real_options,EPC,SSH):

		result = re.search('-u', str(iperf_real_options))
		if result is None:
			logging.debug('Into Iperf_analyzeV2TCPOutput client')
			filename = EPC.SourceCodePath + '/scripts/iperf_' + self.testCase_id + '_' + device_id + '.log'
			response = self.Iperf_analyzeV2TCPOutput(lock, UE_IPAddress, device_id, statusQueue, iperf_real_options, EPC, SSH, filename)
			logging.debug('Iperf_analyzeV2TCPOutput response returned value = ' + str(response))
			return response

		result = re.search('Server Report:', SSH.getBefore())
		if result is None:
			result = re.search('read failed: Connection refused', SSH.getBefore())
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

		result = re.search('Server Report:\r\n(?:|\[ *\d+\].*) (?P<bitrate>[0-9\.]+ [KMG]bits\/sec) +(?P<jitter>[0-9\.]+ ms) +(\d+\/..\d+) +(\((?P<packetloss>[0-9\.]+)%\))', SSH.getBefore())
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
		else:
			return -2


	def Iperf_analyzeV2BIDIR(self, lock, UE_IPAddress, device_id, statusQueue,server_filename,client_filename):

		#check the 2 files are here 
		if (not os.path.isfile(client_filename)) or (not os.path.isfile(server_filename)):
			self.ping_iperf_wrong_exit(lock, UE_IPAddress, device_id, statusQueue, 'Bidir TCP : Client or Server Log File not present')
			return
		#check the 2 files size
		if (os.path.getsize(client_filename)==0) and (os.path.getsize(server_filename)==0):
			self.ping_iperf_wrong_exit(lock, UE_IPAddress, device_id, statusQueue, 'Bidir TCP : Client and Server Log File are empty')
			return

		report_msg='TCP BIDIR Report:\n'
		#if client is not empty, all the info is in, otherwise we ll use the server file to get some partial info
		client_filesize = os.path.getsize(client_filename)
		if client_filesize == 0:
			report_msg+="Client file (UE) present but !!! EMPTY !!!\n"
			report_msg+="Partial report from server file\n"
			filename = server_filename
		else :		
			report_msg+="Report from client file (UE)\n"
			filename = client_filename

		report=[] #used to check if relevant lines were found

		with open(filename, 'r') as f_client:
			for line in f_client.readlines():
				result = re.search(rf'^\[\s+\d+\](?P<direction>\[.+\]).*\s+(?P<bitrate>[0-9\.]+ [KMG]bits\/sec).*\s+(?P<role>\bsender|receiver\b)', str(line))
				if result is not None:
					report.append(str(line))
					report_msg+=result.group('role') + ' ' + result.group('direction')+ '\t = ' +result.group('bitrate')+'\n'

		if len(report)>0:
			lock.acquire()
			statusQueue.put(0)
			statusQueue.put(device_id)
			statusQueue.put(UE_IPAddress)
			statusQueue.put(report_msg)
			logging.debug('\u001B[1;37;45m TCP Bidir Iperf Result (' + UE_IPAddress + ') \u001B[0m')
			for rLine in report_msg.split('\n'):
				logging.debug('\u001B[1;35m    ' + rLine + '\u001B[0m')
			lock.release()
		else:
			self.ping_iperf_wrong_exit(lock, UE_IPAddress, device_id, statusQueue, 'Bidir TCP : Could not analyze from Log file')



	def Iperf_analyzeV2Server(self, lock, UE_IPAddress, device_id, statusQueue, iperf_real_options, filename,type):
		if (not os.path.isfile(filename)):
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

		server_file = open(filename, 'r')
		br_sum = 0.0
		ji_sum = 0.0
		pl_sum = 0
		ps_sum = 0
		row_idx = 0
		for line in server_file.readlines():
			if type==0:
				result = re.search('(?P<bitrate>[0-9\.]+ [KMG]bits\/sec) +(?P<jitter>[0-9\.]+ ms) +(?P<lostPack>[0-9]+)/ +(?P<sentPack>[0-9]+)', str(line))
			else:
				result = re.search('^\[\s+\d\].+  (?P<bitrate>[0-9\.]+ [KMG]bits\/sec) +(?P<jitter>[0-9\.]+ ms) +(?P<lostPack>[0-9]+)\/\s*(?P<sentPack>[0-9]+)', str(line))

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
				#checking packet loss compliance
				if float(pl) > float(self.iperf_packetloss_threshold):
					pal_too_high_msg = 'Packet Loss too high :  tested = '+packetloss+', target = '+self.iperf_packetloss_threshold+'%'
				else:
					pal_too_high_msg='Packet Loss value is within acceptance range'
				#checking bitrate perf compliance
				if float(br_loss) < float(self.iperf_bitrate_threshold):
					bit_too_low_msg = 'Bitrate too low :  tested = '+bitperf+', target = '+self.iperf_bitrate_threshold+'%'
				else:
					bit_too_low_msg='Bitrate perf value is within acceptance range'
			lock.acquire()
			if (float(br_loss) < float(self.iperf_bitrate_threshold)) and (float(pl) > float(self.iperf_packetloss_threshold)):
				statusQueue.put(-1)
			elif (float(br_loss) < float(self.iperf_bitrate_threshold)) or (float(pl) > float(self.iperf_packetloss_threshold)): 
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
			statusQueue.put(req_msg + '\n' + bir_msg + '\n' + brl_msg + '\n' + jit_msg + '\n' + pal_msg + '\n' + pal_too_high_msg + '\n' + bit_too_low_msg + '\n')
			logging.debug('\u001B[1;37;45m iperf result (' + UE_IPAddress + ') \u001B[0m')
			logging.debug('\u001B[1;35m    ' + req_msg + '\u001B[0m')
			logging.debug('\u001B[1;35m    ' + bir_msg + '\u001B[0m')
			logging.debug('\u001B[1;35m    ' + brl_msg + '\u001B[0m')
			logging.debug('\u001B[1;35m    ' + jit_msg + '\u001B[0m')
			logging.debug('\u001B[1;35m    ' + pal_msg + '\u001B[0m')
			logging.debug('\u001B[1;35m    ' + pal_too_high_msg + '\u001B[0m')
			logging.debug('\u001B[1;35m    ' + bit_too_low_msg + '\u001B[0m')
			lock.release()
		else:
			self.ping_iperf_wrong_exit(lock, UE_IPAddress, device_id, statusQueue, 'Could not analyze from server log')

		server_file.close()


	def Iperf_analyzeV3Output(self, lock, UE_IPAddress, device_id, statusQueue,SSH):

		result = re.search('(?P<bitrate>[0-9\.]+ [KMG]bits\/sec) +(?:|[0-9\.]+ ms +\d+\/\d+ \((?P<packetloss>[0-9\.]+)%\)) +(?:|receiver)\r\n(?:|\[ *\d+\] Sent \d+ datagrams)\r\niperf Done\.', SSH.getBefore())
		if result is None:
			result = re.search('(?P<error>iperf: error - [a-zA-Z0-9 :]+)', SSH.getBefore())
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

	def Iperf_Module(self, lock, statusQueue, EPC, ue, RAN, idx, ue_num):
		SSH = sshconnection.SSHConnection()
		server_filename = f'iperf_server_{self.testCase_id}_{ue.getName()}.log'
		client_filename = f'iperf_client_{self.testCase_id}_{ue.getName()}.log'
		if (re.match('OAI-Rel14-Docker', EPC.Type, re.IGNORECASE)) or (re.match('OAICN5G', EPC.Type, re.IGNORECASE)):
			#retrieve trf-gen container IP address
			SSH.open(EPC.IPAddress, EPC.UserName, EPC.Password)
			SSH.command('docker inspect --format="TRF_IP_ADDR = {{range .NetworkSettings.Networks}}{{.IPAddress}}{{end}}" prod-trf-gen', '\$', 5)
			result = re.search('TRF_IP_ADDR = (?P<trf_ip_addr>[0-9\.]+)', SSH.getBefore())
			if result is None:
				raise Exception("could not corver prod-trf-gen IP address")
			cn_target_ip = result.group('trf_ip_addr')
			SSH.close()
			cn_iperf_prefix = "docker exec  prod-trf-gen" # -w /iperf-2.0.13  necessary?
		elif (re.match('OC-OAI-CN5G', EPC.Type, re.IGNORECASE)):
			cn_target_ip = "172.21.6.102"
		else: # lteboix, sabox
			cn_target_ip = "192.172.0.1"
			cn_iperf_prefix = ""

		iperf_opt = self.iperf_args
		udpIperf = re.search('-u', iperf_opt) is not None
		udpSwitch = '-u' if udpIperf else ''
		if udpIperf:
			iperf_opt = self.Iperf_ComputeModifiedBW(idx, ue_num)
			logging.info(f'iperf options modified from "{self.iperf_args}" to "{iperf_opt}" for {ue.getName()}')
		iperf_time = int(self.Iperf_ComputeTime())
		port = f'-p {5001+idx}'
		# hack: the ADB UEs don't have iperf in $PATH, so we need to hardcode for the moment
		iperf_ue = '/data/local/tmp/iperf' if re.search('adb', ue.getName()) else 'iperf'

		if self.iperf_direction == "DL":
			logging.debug("Iperf in DL requested")
			cmd = cls_cmd.getConnection(ue.getHost())
			cmd.run(f'rm {server_filename}')
			cmd.run(f'{ue.getCmdPrefix()} {iperf_ue} -s -B {ue.getIP()} {udpSwitch} -i 1 -t {iperf_time * 1.5} {port} &> /tmp/{server_filename} &')
			cmd.close()

			cmd = cls_cmd.getConnection(EPC.IPAddress)
			cmd.run(f'rm {EPC.SourceCodePath}/{client_filename}')
			cmd.run(f'{cn_iperf_prefix} iperf -c {ue.getIP()} {iperf_opt} {port} &> {EPC.SourceCodePath}/{client_filename}', timeout=iperf_time * 1.5)
			cmd.copyin(f'{EPC.SourceCodePath}/{client_filename}', client_filename)
			cmd.close()

			cmd = cls_cmd.getConnection(ue.getHost())
			cmd.copyin(f'/tmp/{server_filename}', server_filename)
			cmd.close()

			if udpIperf:
				self.Iperf_analyzeV2Server(lock, ue.getIP(), ue.getName(), statusQueue, iperf_opt, server_filename, 1)
			else:
				cmd = cls_cmd.getConnection(EPC.IPAddress)
				self.Iperf_analyzeV2TCPOutput(lock, ue.getIP(), ue.getName(), statusQueue, iperf_opt, EPC, cmd, f"{EPC.SourceCodePath}/{client_filename}")
				cmd.close()

		elif self.iperf_direction == "UL":
			logging.debug("Iperf in UL requested")
			cmd = cls_cmd.getConnection(EPC.IPAddress)
			cmd.run(f'rm {EPC.SourceCodePath}/{server_filename}')
			cmd.run(f'{cn_iperf_prefix} iperf -s {udpSwitch} -t {iperf_time * 1.5} {port} &> {EPC.SourceCodePath}/{server_filename} &')
			cmd.close()

			cmd = cls_cmd.getConnection(ue.getHost())
			cmd.run(f'rm /tmp/{client_filename}')
			cmd.run(f'{ue.getCmdPrefix()} {iperf_ue} -B {ue.getIP()} -c {cn_target_ip} {iperf_opt} {port} &> /tmp/{client_filename}', timeout=iperf_time*1.5)
			cmd.copyin(f'/tmp/{client_filename}', client_filename)
			cmd.close()

			cmd = cls_cmd.getConnection(EPC.IPAddress)
			cmd.copyin(f'{EPC.SourceCodePath}/{server_filename}', server_filename)
			cmd.close()

			if udpIperf:
				self.Iperf_analyzeV2Server(lock, ue.getIP(), ue.getName(), statusQueue, iperf_opt, server_filename, 1)
			else:
				cmd = cls_cmd.getConnection(ue.getHost())
				self.Iperf_analyzeV2TCPOutput(lock, ue.getIP(), ue.getName(), statusQueue, iperf_opt, EPC, cmd, f"/tmp/{client_filename}")
				cmd.close()

		elif self.iperf_direction=="BIDIR":
			logging.debug("Bi-directional iperf requested")
			cmd = cls_cmd.getConnection(EPC.IPAddress)
			cmd.run(f'rm {EPC.SourceCodePath}/{server_filename}')
			cmd.run(f'{cn_iperf_prefix} iperf3 -s -i 1 -1 {port} &> {EPC.SourceCodePath}/{server_filename} &')
			cmd.close()

			cmd = cls_cmd.getConnection(ue.getHost())
			cmd.run(f'rm /tmp/{client_filename}')
			cmd.run(f'iperf3 -B {ue.getIP()} -c {cn_target_ip} {iperf_opt} {port} &> /tmp/{client_filename}', timeout=iperf_time*1.5)
			cmd.copyin(f'/tmp/{client_filename}', client_filename)
			cmd.close()

			cmd = cls_cmd.getConnection(EPC.IPAddress)
			cmd.copyin(f'{EPC.SourceCodePath}/{server_filename}', server_filename)
			cmd.close()

			self.Iperf_analyzeV2BIDIR(lock, ue.getHost(), ue.getName(), statusQueue, server_filename, client_filename)

		elif self.iperf_direction == "IPERF3":
			cmd = cls_cmd.getConnection(ue.getHost())
			cmd.run(f'rm /tmp/{server_filename}', reportNonZero=False)
			port = f'{5002+idx}'
			cmd.run(f'{ue.getCmdPrefix()} iperf3 -c {cn_target_ip} -p {port} {iperf_opt} --get-server-output &> /tmp/{server_filename}', timeout=iperf_time*1.5)
			cmd.copyin(f'/tmp/{server_filename}', server_filename)
			cmd.close()
			if udpIperf:
				self.Iperf_analyzeV2Server(lock, ue.getIP(), ue.getName(), statusQueue, iperf_opt, server_filename, 1)
			else:
				cmd = cls_cmd.getConnection(EPC.IPAddress)
				self.Iperf_analyzeV2TCPOutput(lock, ue.getIP(), ue.getName(), statusQueue, iperf_opt, EPC, cmd, f'/tmp/{server_filename}')
				cmd.close()

		else :
			raise Exception("Incorrect or missing IPERF direction in XML")

	def IperfNoS1(self,HTML,RAN,EPC,CONTAINERS):
		SSH = sshconnection.SSHConnection()
		if RAN.eNBIPAddress == '' or RAN.eNBUserName == '' or RAN.eNBPassword == '' or self.UEIPAddress == '' or self.UEUserName == '' or self.UEPassword == '':
			HELP.GenericHelp(CONST.Version)
			sys.exit('Insufficient Parameter')
		check_eNB = True
		check_OAI_UE = True
		server_on_enb = re.search('-R', str(self.iperf_args))
		if server_on_enb is not None:
			iServerIPAddr = RAN.eNBIPAddress
			iServerUser = RAN.eNBUserName
			iServerPasswd = RAN.eNBPassword
			iClientIPAddr = self.UEIPAddress
			iClientUser = self.UEUserName
			iClientPasswd = self.UEPassword
		else:
			iServerIPAddr = self.UEIPAddress
			iServerUser = self.UEUserName
			iServerPasswd = self.UEPassword
			iClientIPAddr = RAN.eNBIPAddress
			iClientUser = RAN.eNBUserName
			iClientPasswd = RAN.eNBPassword
		if self.iperf_options != 'sink':
			# Starting the iperf server
			SSH.open(iServerIPAddr, iServerUser, iServerPasswd)
			# args SHALL be "-c client -u any"
			# -c 10.0.1.2 -u -b 1M -t 30 -i 1 -fm -B 10.0.1.1
			# -B 10.0.1.1 -u -s -i 1 -fm
			server_options = re.sub('-u.*$', '-u -s -i 1 -fm', str(self.iperf_args))
			server_options = server_options.replace('-c','-B')
			SSH.command('rm -f /tmp/tmp_iperf_server_' + self.testCase_id + '.log', '\$', 5)
			SSH.command('echo $USER; nohup iperf ' + server_options + ' > /tmp/tmp_iperf_server_' + self.testCase_id + '.log 2>&1 &', iServerUser, 5)
			time.sleep(0.5)
			SSH.close()

		# Starting the iperf client
		modified_options = self.Iperf_ComputeModifiedBW(0, 1)
		modified_options = modified_options.replace('-R','')
		iperf_time = self.Iperf_ComputeTime()
		SSH.open(iClientIPAddr, iClientUser, iClientPasswd)
		SSH.command('rm -f /tmp/tmp_iperf_' + self.testCase_id + '.log', '\$', 5)
		iperf_status = SSH.command('stdbuf -o0 iperf ' + modified_options + ' 2>&1 | stdbuf -o0 tee /tmp/tmp_iperf_' + self.testCase_id + '.log', '\$', int(iperf_time)*5.0)
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
				clientStatus = self.Iperf_analyzeV2Output(lock, '10.0.1.2', 'OAI-UE', status_queue, modified_options, EPC,SSH)
		SSH.close()

		# Stopping the iperf server
		if self.iperf_options != 'sink':
			SSH.open(iServerIPAddr, iServerUser, iServerPasswd)
			SSH.command('killall --signal SIGKILL iperf', '\$', 5)
			time.sleep(0.5)
			SSH.close()

		if (clientStatus == -1):
			if (os.path.isfile('iperf_server_' + self.testCase_id + '.log')):
				os.remove('iperf_server_' + self.testCase_id + '.log')
			SSH.copyin(iServerIPAddr, iServerUser, iServerPasswd, '/tmp/tmp_iperf_server_' + self.testCase_id + '.log', 'iperf_server_' + self.testCase_id + '_OAI-UE.log')
			filename='iperf_server_' + self.testCase_id + '_OAI-UE.log'
			self.Iperf_analyzeV2Server(lock, '10.0.1.2', 'OAI-UE', status_queue, modified_options,filename,0)

		# copying on the EPC server for logCollection
		if (clientStatus == -1):
			copyin_res = SSH.copyin(iServerIPAddr, iServerUser, iServerPasswd, '/tmp/tmp_iperf_server_' + self.testCase_id + '.log', 'iperf_server_' + self.testCase_id + '_OAI-UE.log')
			if (copyin_res == 0):
				SSH.copyout(EPC.IPAddress, EPC.UserName, EPC.Password, 'iperf_server_' + self.testCase_id + '_OAI-UE.log', EPC.SourceCodePath + '/scripts')
		copyin_res = SSH.copyin(iClientIPAddr, iClientUser, iClientPasswd, '/tmp/tmp_iperf_' + self.testCase_id + '.log', 'iperf_' + self.testCase_id + '_OAI-UE.log')
		if (copyin_res == 0):
			SSH.copyout(EPC.IPAddress, EPC.UserName, EPC.Password, 'iperf_' + self.testCase_id + '_OAI-UE.log', EPC.SourceCodePath + '/scripts')
		iperf_noperf = False
		if status_queue.empty():
			iperf_status = False
		else:
			iperf_status = True
		messages = []
		while (not status_queue.empty()):
			count = status_queue.get()
			if (count < 0):
				iperf_status = False
			if (count > 0):
				iperf_noperf = True
			device_id = status_queue.get()
			ip_addr = status_queue.get()
			message = status_queue.get()
			messages.append(f'UE ({device_id})\nIP Address  : {ip_addr}\n{message}')
		if (iperf_noperf and iperf_status):
			HTML.CreateHtmlTestRowQueue(self.iperf_args, 'PERF NOT MET', messages)
		elif (iperf_status):
			HTML.CreateHtmlTestRowQueue(self.iperf_args, 'OK', messages)
		else:
			HTML.CreateHtmlTestRowQueue(self.iperf_args, 'KO', messages)
			self.AutoTerminateUEandeNB(HTML,RAN,EPC,CONTAINERS)

	def Iperf(self,HTML,RAN,EPC,CONTAINERS):
		result = re.search('noS1', str(RAN.Initialize_eNB_args))
		if result is not None:
			self.IperfNoS1(HTML,RAN,EPC,CONTAINERS)
			return
		if EPC.IPAddress == '' or EPC.UserName == '' or EPC.Password == '' or EPC.SourceCodePath == '':
			HELP.GenericHelp(CONST.Version)
			sys.exit('Insufficient Parameter')

		logging.debug(f'Iperf: iperf_args "{self.iperf_args}" iperf_direction "{self.iperf_direction}" iperf_packetloss_threshold "{self.iperf_packetloss_threshold}" iperf_bitrate_threshold "{self.iperf_bitrate_threshold}" iperf_profile "{self.iperf_profile}" iperf_options "{self.iperf_options}"')

		ues = []
		for ue_name in self.ue_ids:
			ue = cls_module_ue.Module_UE(ue_name)
			if not ue.getIP():
				logging.error("no IP addresses returned")
				HTML.CreateHtmlTestRow(self.ping_args, 'KO', CONST.UE_IP_ADDRESS_ISSUE)
				self.AutoTerminateUEandeNB(HTML,RAN,EPC,CONTAINERS)
			ues.append(ue)
		logging.debug(ues)

		self.dummyIperfVersion = '2.0.10'
		#cmd = 'iperf --version'
		#logging.debug(cmd + '\n')
		#iperfStdout = subprocess.check_output(cmd, shell=True, universal_newlines=True)
		#result = re.search('iperf version 2.0.5', str(iperfStdout.strip()))
		#if result is not None:
		#	dummyIperfVersion = '2.0.5'
		#result = re.search('iperf version 2.0.10', str(iperfStdout.strip()))
		#if result is not None:
		#	dummyIperfVersion = '2.0.10'
		multi_jobs = []
		ue_num = len(ues)
		i = 0
		lock = Lock()
		status_queue = SimpleQueue()
		for ue in ues:
			p = Process(target = self.Iperf_Module ,args = (lock, status_queue, EPC, ue, RAN, i, ue_num))
			p.daemon = True
			p.start()
			multi_jobs.append(p)
			i = i + 1
		for job in multi_jobs:
			job.join()

		if (status_queue.empty()):
			HTML.CreateHtmlTestRow(self.iperf_args, 'KO', CONST.ALL_PROCESSES_OK)
			self.AutoTerminateUEandeNB(HTML,RAN,EPC,CONTAINERS)
		else:
			iperf_status = True
			iperf_noperf = False
			messages = []
			while (not status_queue.empty()):
				count = status_queue.get()
				if (count < 0):
					iperf_status = False
				if (count > 0):
					iperf_noperf = True
				device_id = status_queue.get()
				ip_addr = status_queue.get()
				msg = status_queue.get()
				messages.append(f'UE ({device_id})\nIP Address  : {ip_addr}\n{msg}')
			if (iperf_noperf and iperf_status):
				HTML.CreateHtmlTestRowQueue(self.iperf_args, 'PERF NOT MET', messages)
			elif (iperf_status):
				HTML.CreateHtmlTestRowQueue(self.iperf_args, 'OK', messages)
			else:
				HTML.CreateHtmlTestRowQueue(self.iperf_args, 'KO', messages)
				self.AutoTerminateUEandeNB(HTML,RAN,EPC,CONTAINERS)

	def AnalyzeLogFile_UE(self, UElogFile,HTML,RAN):
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
		f1aRetransmissionCount = 0
		fatalErrorCount = 0
		macBsrTimerExpiredCount = 0
		rrcConnectionRecfgComplete = 0
		no_cell_sync_found = False
		mib_found = False
		frequency_found = False
		plmn_found = False
		nrUEFlag = False
		nrDecodeMib = 0
		nrFoundDCI = 0
		nrCRCOK = 0
		mbms_messages = 0
		nbPduSessAccept = 0
		nbPduDiscard = 0
		HTML.htmlUEFailureMsg=''
		global_status = CONST.ALL_PROCESSES_OK
		for line in ue_log_file.readlines():
			result = re.search('nr_synchro_time|Starting NR UE soft modem', str(line))
			if result is not None:
				nrUEFlag = True
			if nrUEFlag:
				result = re.search('decode mib', str(line))
				if result is not None:
					nrDecodeMib += 1
				result = re.search('found 1 DCIs', str(line))
				if result is not None:
					nrFoundDCI += 1
				result = re.search('CRC OK', str(line))
				if result is not None:
					nrCRCOK += 1
				result = re.search('Received PDU Session Establishment Accept', str(line))
				if result is not None:
					nbPduSessAccept += 1
				result = re.search('warning: discard PDU, sn out of window', str(line))
				if result is not None:
					nbPduDiscard += 1
				result = re.search('--nfapi STANDALONE_PNF --node-number 2 --sa', str(line))
				if result is not None:
					frequency_found = True
			result = re.search('Exiting OAI softmodem', str(line))
			if result is not None:
				exitSignalReceived = True
			result = re.search('System error|[Ss]egmentation [Ff]ault|======= Backtrace: =========|======= Memory map: ========', str(line))
			if result is not None and not exitSignalReceived:
				foundSegFault = True
			result = re.search('[Cc]ore [dD]ump', str(line))
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
			result = re.search('bad DCI 1', str(line))
			if result is not None and not exitSignalReceived:
				badDciCount += 1
			result = re.search('Format1A Retransmission but TBS are different', str(line))
			if result is not None and not exitSignalReceived:
				f1aRetransmissionCount += 1
			result = re.search('FATAL ERROR', str(line))
			if result is not None and not exitSignalReceived:
				fatalErrorCount += 1
			result = re.search('MAC BSR Triggered ReTxBSR Timer expiry', str(line))
			if result is not None and not exitSignalReceived:
				macBsrTimerExpiredCount += 1
			result = re.search('Generating RRCConnectionReconfigurationComplete', str(line))
			if result is not None:
				rrcConnectionRecfgComplete += 1
			# No cell synchronization found, abandoning
			result = re.search('No cell synchronization found, abandoning', str(line))
			if result is not None:
				no_cell_sync_found = True
			if RAN.eNBmbmsEnables[0]:
				result = re.search('TRIED TO PUSH MBMS DATA', str(line))
				if result is not None:
					mbms_messages += 1
			result = re.search("MIB Information => ([a-zA-Z]{1,10}), ([a-zA-Z]{1,10}), NidCell (?P<nidcell>\d{1,3}), N_RB_DL (?P<n_rb_dl>\d{1,3}), PHICH DURATION (?P<phich_duration>\d), PHICH RESOURCE (?P<phich_resource>.{1,4}), TX_ANT (?P<tx_ant>\d)", str(line))
			if result is not None and (not mib_found):
				try:
					mibMsg = "MIB Information: " + result.group(1) + ', ' + result.group(2)
					HTML.htmlUEFailureMsg=HTML.htmlUEFailureMsg + mibMsg + '\n'
					logging.debug('\033[94m' + mibMsg + '\033[0m')
					mibMsg = "    nidcell = " + result.group('nidcell')
					HTML.htmlUEFailureMsg=HTML.htmlUEFailureMsg + mibMsg
					logging.debug('\033[94m' + mibMsg + '\033[0m')
					mibMsg = "    n_rb_dl = " + result.group('n_rb_dl')
					HTML.htmlUEFailureMsg=HTML.htmlUEFailureMsg + mibMsg + '\n'
					logging.debug('\033[94m' + mibMsg + '\033[0m')
					mibMsg = "    phich_duration = " + result.group('phich_duration')
					HTML.htmlUEFailureMsg=HTML.htmlUEFailureMsg + mibMsg
					logging.debug('\033[94m' + mibMsg + '\033[0m')
					mibMsg = "    phich_resource = " + result.group('phich_resource')
					HTML.htmlUEFailureMsg=HTML.htmlUEFailureMsg + mibMsg + '\n'
					logging.debug('\033[94m' + mibMsg + '\033[0m')
					mibMsg = "    tx_ant = " + result.group('tx_ant')
					HTML.htmlUEFailureMsg=HTML.htmlUEFailureMsg + mibMsg + '\n'
					logging.debug('\033[94m' + mibMsg + '\033[0m')
					mib_found = True
				except Exception as e:
					logging.error('\033[91m' + "MIB marker was not found" + '\033[0m')
			result = re.search("Measured Carrier Frequency (?P<measured_carrier_frequency>\d{1,15}) Hz", str(line))
			if result is not None and (not frequency_found):
				try:
					mibMsg = "Measured Carrier Frequency = " + result.group('measured_carrier_frequency') + ' Hz'
					HTML.htmlUEFailureMsg=HTML.htmlUEFailureMsg + mibMsg + '\n'
					logging.debug('\033[94m' + mibMsg + '\033[0m')
					frequency_found = True
				except Exception as e:
					logging.error('\033[91m' + "Measured Carrier Frequency not found" + '\033[0m')
			result = re.search("PLMN MCC (?P<mcc>\d{1,3}), MNC (?P<mnc>\d{1,3}), TAC", str(line))
			if result is not None and (not plmn_found):
				try:
					mibMsg = 'PLMN MCC = ' + result.group('mcc') + ' MNC = ' + result.group('mnc')
					HTML.htmlUEFailureMsg=HTML.htmlUEFailureMsg + mibMsg + '\n'
					logging.debug('\033[94m' + mibMsg + '\033[0m')
					plmn_found = True
				except Exception as e:
					logging.error('\033[91m' + "PLMN not found" + '\033[0m')
			result = re.search("Found (?P<operator>[\w,\s]{1,15}) \(name from internal table\)", str(line))
			if result is not None:
				try:
					mibMsg = "The operator is: " + result.group('operator')
					HTML.htmlUEFailureMsg=HTML.htmlUEFailureMsg + mibMsg + '\n'
					logging.debug('\033[94m' + mibMsg + '\033[0m')
				except Exception as e:
					logging.error('\033[91m' + "Operator name not found" + '\033[0m')
			result = re.search("SIB5 InterFreqCarrierFreq element (.{1,4})/(.{1,4})", str(line))
			if result is not None:
				try:
					mibMsg = "SIB5 InterFreqCarrierFreq element " + result.group(1) + '/' + result.group(2)
					HTML.htmlUEFailureMsg=HTML.htmlUEFailureMsg + mibMsg + ' -> '
					logging.debug('\033[94m' + mibMsg + '\033[0m')
				except Exception as e:
					logging.error('\033[91m' + "SIB5 InterFreqCarrierFreq element not found" + '\033[0m')
			result = re.search("DL Carrier Frequency/ARFCN : \-*(?P<carrier_frequency>\d{1,15}/\d{1,4})", str(line))
			if result is not None:
				try:
					freq = result.group('carrier_frequency')
					new_freq = re.sub('/[0-9]+','',freq)
					float_freq = float(new_freq) / 1000000
					HTML.htmlUEFailureMsg=HTML.htmlUEFailureMsg + 'DL Freq: ' + ('%.1f' % float_freq) + ' MHz'
					logging.debug('\033[94m' + "    DL Carrier Frequency is: " + str(freq) + '\033[0m')
				except Exception as e:
					logging.error('\033[91m' + "    DL Carrier Frequency not found" + '\033[0m')
			result = re.search("AllowedMeasBandwidth : (?P<allowed_bandwidth>\d{1,7})", str(line))
			if result is not None:
				try:
					prb = result.group('allowed_bandwidth')
					HTML.htmlUEFailureMsg=HTML.htmlUEFailureMsg + ' -- PRB: ' + prb + '\n'
					logging.debug('\033[94m' + "    AllowedMeasBandwidth: " + prb + '\033[0m')
				except Exception as e:
					logging.error('\033[91m' + "    AllowedMeasBandwidth not found" + '\033[0m')
		ue_log_file.close()
		if rrcConnectionRecfgComplete > 0:
			statMsg = 'UE connected to eNB (' + str(rrcConnectionRecfgComplete) + ' RRCConnectionReconfigurationComplete message(s) generated)'
			logging.debug('\033[94m' + statMsg + '\033[0m')
			HTML.htmlUEFailureMsg=HTML.htmlUEFailureMsg + statMsg + '\n'
		if nrUEFlag:
			if nrDecodeMib > 0:
				statMsg = 'UE showed ' + str(nrDecodeMib) + ' "MIB decode" message(s)'
				logging.debug('\u001B[1;30;43m ' + statMsg + ' \u001B[0m')
				HTML.htmlUEFailureMsg=HTML.htmlUEFailureMsg + statMsg + '\n'
			if nrFoundDCI > 0:
				statMsg = 'UE showed ' + str(nrFoundDCI) + ' "DCI found" message(s)'
				logging.debug('\u001B[1;30;43m ' + statMsg + ' \u001B[0m')
				HTML.htmlUEFailureMsg=HTML.htmlUEFailureMsg + statMsg + '\n'
			if nrCRCOK > 0:
				statMsg = 'UE showed ' + str(nrCRCOK) + ' "PDSCH decoding" message(s)'
				logging.debug('\u001B[1;30;43m ' + statMsg + ' \u001B[0m')
				HTML.htmlUEFailureMsg=HTML.htmlUEFailureMsg + statMsg + '\n'
			if not frequency_found:
				statMsg = 'NR-UE could NOT synch!'
				logging.error('\u001B[1;30;43m ' + statMsg + ' \u001B[0m')
				HTML.htmlUEFailureMsg=HTML.htmlUEFailureMsg + statMsg + '\n'
			if nbPduSessAccept > 0:
				statMsg = 'UE showed ' + str(nbPduSessAccept) + ' "Received PDU Session Establishment Accept" message(s)'
				logging.debug('\u001B[1;30;43m ' + statMsg + ' \u001B[0m')
				HTML.htmlUEFailureMsg=HTML.htmlUEFailureMsg + statMsg + '\n'
			if nbPduDiscard > 0:
				statMsg = 'UE showed ' + str(nbPduDiscard) + ' "warning: discard PDU, sn out of window" message(s)'
				logging.debug('\u001B[1;30;43m ' + statMsg + ' \u001B[0m')
				HTML.htmlUEFailureMsg=HTML.htmlUEFailureMsg + statMsg + '\n'
		if uciStatMsgCount > 0:
			statMsg = 'UE showed ' + str(uciStatMsgCount) + ' "uci->stat" message(s)'
			logging.debug('\u001B[1;30;43m ' + statMsg + ' \u001B[0m')
			HTML.htmlUEFailureMsg=HTML.htmlUEFailureMsg + statMsg + '\n'
		if pdcpDataReqFailedCount > 0:
			statMsg = 'UE showed ' + str(pdcpDataReqFailedCount) + ' "PDCP data request failed" message(s)'
			logging.debug('\u001B[1;30;43m ' + statMsg + ' \u001B[0m')
			HTML.htmlUEFailureMsg=HTML.htmlUEFailureMsg + statMsg + '\n'
		if badDciCount > 0:
			statMsg = 'UE showed ' + str(badDciCount) + ' "bad DCI 1(A)" message(s)'
			logging.debug('\u001B[1;30;43m ' + statMsg + ' \u001B[0m')
			HTML.htmlUEFailureMsg=HTML.htmlUEFailureMsg + statMsg + '\n'
		if f1aRetransmissionCount > 0:
			statMsg = 'UE showed ' + str(f1aRetransmissionCount) + ' "Format1A Retransmission but TBS are different" message(s)'
			logging.debug('\u001B[1;30;43m ' + statMsg + ' \u001B[0m')
			HTML.htmlUEFailureMsg=HTML.htmlUEFailureMsg + statMsg + '\n'
		if fatalErrorCount > 0:
			statMsg = 'UE showed ' + str(fatalErrorCount) + ' "FATAL ERROR:" message(s)'
			logging.debug('\u001B[1;30;43m ' + statMsg + ' \u001B[0m')
			HTML.htmlUEFailureMsg=HTML.htmlUEFailureMsg + statMsg + '\n'
		if macBsrTimerExpiredCount > 0:
			statMsg = 'UE showed ' + str(fatalErrorCount) + ' "MAC BSR Triggered ReTxBSR Timer expiry" message(s)'
			logging.debug('\u001B[1;30;43m ' + statMsg + ' \u001B[0m')
			HTML.htmlUEFailureMsg=HTML.htmlUEFailureMsg + statMsg + '\n'
		if RAN.eNBmbmsEnables[0]:
			if mbms_messages > 0:
				statMsg = 'UE showed ' + str(mbms_messages) + ' "TRIED TO PUSH MBMS DATA" message(s)'
				logging.debug('\u001B[1;30;43m ' + statMsg + ' \u001B[0m')
			else:
				statMsg = 'UE did NOT SHOW "TRIED TO PUSH MBMS DATA" message(s)'
				logging.debug('\u001B[1;30;41m ' + statMsg + ' \u001B[0m')
				global_status = CONST.OAI_UE_PROCESS_NO_MBMS_MSGS
			HTML.htmlUEFailureMsg=HTML.htmlUEFailureMsg + statMsg + '\n'
		if foundSegFault:
			logging.debug('\u001B[1;37;41m UE ended with a Segmentation Fault! \u001B[0m')
			if not nrUEFlag:
				global_status = CONST.OAI_UE_PROCESS_SEG_FAULT
			else:
				if not frequency_found:
					global_status = CONST.OAI_UE_PROCESS_SEG_FAULT
		if foundAssertion:
			logging.debug('\u001B[1;30;43m UE showed an assertion! \u001B[0m')
			HTML.htmlUEFailureMsg=HTML.htmlUEFailureMsg + 'UE showed an assertion!\n'
			if not nrUEFlag:
				if not mib_found or not frequency_found:
					global_status = CONST.OAI_UE_PROCESS_ASSERTION
			else:
				if not frequency_found:
					global_status = CONST.OAI_UE_PROCESS_ASSERTION
		if foundRealTimeIssue:
			logging.debug('\u001B[1;37;41m UE faced real time issues! \u001B[0m')
			HTML.htmlUEFailureMsg=HTML.htmlUEFailureMsg + 'UE faced real time issues!\n'
		if nrUEFlag:
			if not frequency_found:
				global_status = CONST.OAI_UE_PROCESS_COULD_NOT_SYNC
		else:
			if no_cell_sync_found and not mib_found:
				logging.debug('\u001B[1;37;41m UE could not synchronize ! \u001B[0m')
				HTML.htmlUEFailureMsg=HTML.htmlUEFailureMsg + 'UE could not synchronize!\n'
				global_status = CONST.OAI_UE_PROCESS_COULD_NOT_SYNC
		return global_status

	def TerminateUE(self, HTML):
		ues = [cls_module_ue.Module_UE(n.strip()) for n in self.ue_ids]
		with concurrent.futures.ThreadPoolExecutor() as executor:
			futures = [executor.submit(ue.terminate) for ue in ues]
			archives = [f.result() for f in futures]
		archive_info = [f'Log at: {a}' if a else 'No log available' for a in archives]
		messages = [f"UE {ue.getName()}: {log}" for (ue, log) in zip(ues, archive_info)]
		HTML.CreateHtmlTestRowQueue(f'N/A', 'OK', messages)

	def TerminateOAIUE(self,HTML,RAN,EPC,CONTAINERS):
		SSH = sshconnection.SSHConnection()
		SSH.open(self.UEIPAddress, self.UEUserName, self.UEPassword)
		SSH.command('cd ' + self.UESourceCodePath + '/cmake_targets', '\$', 5)
		SSH.command('ps -aux | grep --color=never softmodem | grep -v grep', '\$', 5)
		result = re.search('-uesoftmodem', SSH.getBefore())
		if result is not None:
			SSH.command('echo ' + self.UEPassword + ' | sudo -S killall --signal SIGINT -r .*-uesoftmodem || true', '\$', 5)
			time.sleep(10)
			SSH.command('ps -aux | grep --color=never softmodem | grep -v grep', '\$', 5)
			result = re.search('-uesoftmodem', SSH.getBefore())
			if result is not None:
				SSH.command('echo ' + self.UEPassword + ' | sudo -S killall --signal SIGKILL -r .*-uesoftmodem || true', '\$', 5)
				time.sleep(5)
		SSH.command('rm -f my-lte-uesoftmodem-run' + str(self.UE_instance) + '.sh', '\$', 5)
		SSH.close()
		result = re.search('ue_', str(self.UELogFile))
		if result is not None:
			copyin_res = SSH.copyin(self.UEIPAddress, self.UEUserName, self.UEPassword, self.UESourceCodePath + '/cmake_targets/' + self.UELogFile, '.')
			if (copyin_res == -1):
				logging.debug('\u001B[1;37;41m Could not copy UE logfile to analyze it! \u001B[0m')
				HTML.htmlUEFailureMsg='Could not copy UE logfile to analyze it!'
				HTML.CreateHtmlTestRow('N/A', 'KO', CONST.OAI_UE_PROCESS_NOLOGFILE_TO_ANALYZE, 'UE')
				self.UELogFile = ''
				return
			logging.debug('\u001B[1m Analyzing UE logfile \u001B[0m')
			logStatus = self.AnalyzeLogFile_UE(self.UELogFile,HTML,RAN)
			result = re.search('--no-L2-connect', str(self.Initialize_OAI_UE_args))
			if result is not None:
				ueAction = 'Sniffing'
			else:
				ueAction = 'Connection'
			if (logStatus < 0):
				logging.debug('\u001B[1m' + ueAction + ' Failed \u001B[0m')
				HTML.htmlUEFailureMsg='<b>' + ueAction + ' Failed</b>\n' + HTML.htmlUEFailureMsg
				HTML.CreateHtmlTestRow('N/A', 'KO', logStatus, 'UE')
				if self.air_interface == 'lte-uesoftmodem':
					# In case of sniffing on commercial eNBs we have random results
					# Not an error then
					if (logStatus != CONST.OAI_UE_PROCESS_COULD_NOT_SYNC) or (ueAction != 'Sniffing'):
						self.Initialize_OAI_UE_args = ''
						self.AutoTerminateUEandeNB(HTML,RAN,EPC,CONTAINERS)
				else:
					if (logStatus == CONST.OAI_UE_PROCESS_COULD_NOT_SYNC):
						self.Initialize_OAI_UE_args = ''
						self.AutoTerminateUEandeNB(HTML,RAN,EPC,CONTAINERS)
			else:
				logging.debug('\u001B[1m' + ueAction + ' Completed \u001B[0m')
				HTML.htmlUEFailureMsg='<b>' + ueAction + ' Completed</b>\n' + HTML.htmlUEFailureMsg
				HTML.CreateHtmlTestRow('N/A', 'OK', CONST.ALL_PROCESSES_OK)
			self.UELogFile = ''
		else:
			HTML.CreateHtmlTestRow('N/A', 'OK', CONST.ALL_PROCESSES_OK)

	def AutoTerminateUEandeNB(self,HTML,RAN,EPC,CONTAINERS):
		# TODO: terminate UE?
		if (self.Initialize_OAI_UE_args != ''):
			self.testCase_id = 'AUTO-KILL-OAI-UE'
			HTML.testCase_id = self.testCase_id
			self.desc = 'Automatic Termination of OAI-UE'
			HTML.desc = self.desc
			self.ShowTestID()
			self.TerminateOAIUE(HTML,RAN,EPC,CONTAINERS)
		if (RAN.Initialize_eNB_args != ''):
			self.testCase_id = 'AUTO-KILL-RAN'
			HTML.testCase_id = self.testCase_id
			self.desc = 'Automatic Termination of all RAN nodes'
			HTML.desc = self.desc
			self.ShowTestID()
			#terminate all RAN nodes eNB/gNB/OCP
			for instance in range(0, len(RAN.air_interface)):
				if RAN.air_interface[instance]!='':
					logging.debug('Auto Termination of Instance ' + str(instance) + ' : ' + RAN.air_interface[instance])
					RAN.eNB_instance=instance
					RAN.TerminateeNB(HTML,EPC)
		if CONTAINERS.yamlPath[0] != '':
			self.testCase_id = 'AUTO-KILL-CONTAINERS'
			HTML.testCase_id = self.testCase_id
			self.desc = 'Automatic Termination of all RAN containers'
			HTML.desc = self.desc
			self.ShowTestID()
			for instance in range(0, len(CONTAINERS.yamlPath)):
				if CONTAINERS.yamlPath[instance]!='':
					CONTAINERS.eNB_instance=instance
					CONTAINERS.UndeployObject(HTML,RAN)
		RAN.prematureExit=True

	#this function is called only if eNB/gNB fails to start
	#RH to be re-factored
	def AutoTerminateeNB(self,HTML,RAN,EPC,CONTAINERS):
		if (RAN.Initialize_eNB_args != ''):
			self.testCase_id = 'AUTO-KILL-RAN'
			HTML.testCase_id = self.testCase_id
			self.desc = 'Automatic Termination of all RAN nodes'
			HTML.desc = self.desc
			self.ShowTestID()
			#terminate all RAN nodes eNB/gNB/OCP
			for instance in range(0, len(RAN.air_interface)):
				if RAN.air_interface[instance]!='':
					logging.debug('Auto Termination of Instance ' + str(instance) + ' : ' + RAN.air_interface[instance])
					RAN.eNB_instance=instance
					RAN.TerminateeNB(HTML,EPC)
		if CONTAINERS.yamlPath[0] != '':
			self.testCase_id = 'AUTO-KILL-CONTAINERS'
			HTML.testCase_id = self.testCase_id
			self.desc = 'Automatic Termination of all RAN containers'
			HTML.desc = self.desc
			self.ShowTestID()
			for instance in range(0, len(CONTAINERS.yamlPath)):
				if CONTAINERS.yamlPath[instance]!='':
					CONTAINERS.eNB_instance=instance
					CONTAINERS.UndeployObject(HTML,RAN)
		RAN.prematureExit=True

	def IdleSleep(self,HTML):
		time.sleep(self.idle_sleep_time)
		HTML.CreateHtmlTestRow(str(self.idle_sleep_time) + ' sec', 'OK', CONST.ALL_PROCESSES_OK)

	def X2_Status(self, idx, fileName):
		cmd = "curl --silent http://" + EPC.IPAddress + ":9999/stats | jq '.' > " + fileName
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

	def Perform_X2_Handover(self,HTML,RAN,EPC):
		html_queue = SimpleQueue()
		fullMessage = '<pre style="background-color:white">'
		msg = 'Doing X2 Handover w/ option ' + self.x2_ho_options
		logging.debug(msg)
		fullMessage += msg + '\n'
		if self.x2_ho_options == 'network':
			HTML.CreateHtmlTestRow('Cannot perform requested X2 Handover', 'KO', CONST.ALL_PROCESSES_OK)

	def LogCollectBuild(self,RAN):
		# Some pipelines are using "none" IP / Credentials
		# In that case, just forget about it
		if RAN.eNBIPAddress == 'none' or self.UEIPAddress == 'none':
			sys.exit(0)

		if (RAN.eNBIPAddress != '' and RAN.eNBUserName != '' and RAN.eNBPassword != ''):
			IPAddress = RAN.eNBIPAddress
			UserName = RAN.eNBUserName
			Password = RAN.eNBPassword
			SourceCodePath = RAN.eNBSourceCodePath
		elif (self.UEIPAddress != '' and self.UEUserName != '' and self.UEPassword != ''):
			IPAddress = self.UEIPAddress
			UserName = self.UEUserName
			Password = self.UEPassword
			SourceCodePath = self.UESourceCodePath
		else:
			sys.exit('Insufficient Parameter')
		SSH = sshconnection.SSHConnection()
		SSH.open(IPAddress, UserName, Password)
		SSH.command('cd ' + SourceCodePath, '\$', 5)
		SSH.command('cd cmake_targets', '\$', 5)
		SSH.command('rm -f build.log.zip', '\$', 5)
		SSH.command('zip -r build.log.zip build_log_*/*', '\$', 60)
		SSH.close()

	def LogCollectPing(self,EPC):
		# Some pipelines are using "none" IP / Credentials
		# In that case, just forget about it
		if EPC.IPAddress == 'none':
			sys.exit(0)
		SSH = sshconnection.SSHConnection()
		SSH.open(EPC.IPAddress, EPC.UserName, EPC.Password)
		SSH.command('cd ' + EPC.SourceCodePath, '\$', 5)
		SSH.command('cd scripts', '\$', 5)
		SSH.command('rm -f ping.log.zip', '\$', 5)
		SSH.command('zip ping.log.zip ping*.log', '\$', 60)
		SSH.command('rm ping*.log', '\$', 5)
		SSH.close()

	def LogCollectIperf(self,EPC):
		# Some pipelines are using "none" IP / Credentials
		# In that case, just forget about it
		if EPC.IPAddress == 'none':
			sys.exit(0)
		SSH = sshconnection.SSHConnection()
		SSH.open(EPC.IPAddress, EPC.UserName, EPC.Password)
		SSH.command('cd ' + EPC.SourceCodePath, '\$', 5)
		SSH.command('cd scripts', '\$', 5)
		SSH.command('rm -f iperf.log.zip', '\$', 5)
		SSH.command('zip iperf.log.zip iperf*.log', '\$', 60)
		SSH.command('rm iperf*.log', '\$', 5)
		SSH.close()
	
	def LogCollectOAIUE(self):
		# Some pipelines are using "none" IP / Credentials
		# In that case, just forget about it
		if self.UEIPAddress == 'none':
			sys.exit(0)
		SSH = sshconnection.SSHConnection()
		SSH.open(self.UEIPAddress, self.UEUserName, self.UEPassword)
		SSH.command('cd ' + self.UESourceCodePath, '\$', 5)
		SSH.command('cd cmake_targets', '\$', 5)
		SSH.command('echo ' + self.UEPassword + ' | sudo -S rm -f ue.log.zip', '\$', 5)
		SSH.command('echo ' + self.UEPassword + ' | sudo -S zip ue.log.zip ue*.log core* ue_*record.raw ue_*.pcap ue_*txt', '\$', 60)
		SSH.command('echo ' + self.UEPassword + ' | sudo -S rm ue*.log core* ue_*record.raw ue_*.pcap ue_*txt', '\$', 5)
		SSH.close()

	def ConditionalExit(self):
		if self.testUnstable:
			if self.testStabilityPointReached or self.testMinStableId == '999999':
				sys.exit(0)
		sys.exit(1)

	def ShowTestID(self):
		logging.info('\u001B[1m----------------------------------------\u001B[0m')
		logging.info('\u001B[1mTest ID:' + self.testCase_id + '\u001B[0m')
		logging.info('\u001B[1m' + self.desc + '\u001B[0m')
		logging.info('\u001B[1m----------------------------------------\u001B[0m')
