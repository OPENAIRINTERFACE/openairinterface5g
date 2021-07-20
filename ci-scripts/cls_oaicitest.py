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
logging.basicConfig(
	level=logging.DEBUG,
	format="[%(asctime)s] %(name)s:%(levelname)s: %(message)s"
)

#import our libs
import helpreadme as HELP
import constants as CONST
import sshconnection

import cls_module_ue
import cls_ci_ueinfra		#class defining the multi Ue infrastrucure


#-----------------------------------------------------------
# Utility functions
#-----------------------------------------------------------

def GetPingTimeAnalysis(ping_log_file):
	#ping time values read from file
	t_ping=[]
	#ping stats (dictionary) to be returned by the function
	ping_stat={}
	if (os.path.isfile(ping_log_file)):
		with open(ping_log_file,"r") as f:
			for line in f:
				#looking for time=xxx ms field
				result=re.match('^.+time=(?P<ping_time>[0-9\.]+)',line)
				if result != None:
					t_ping.append(float(result.group('ping_time')))

		#initial stats
		ping_stat['min_0']=min(t_ping)
		ping_stat['mean_0']=stat.mean(t_ping)
		ping_stat['median_0']=stat.median(t_ping)
		ping_stat['max_0']=max(t_ping)

		#get index of max value
		
		max_loc=t_ping.index(max(t_ping))
		ping_stat['max_loc']=max_loc
		#remove it
		t_ping.pop(max_loc)
		#new stats after removing max value
		ping_stat['min_1']=min(t_ping)
		ping_stat['mean_1']=stat.mean(t_ping)
		ping_stat['median_1']=stat.median(t_ping)
		ping_stat['max_1']=max(t_ping)

		return ping_stat

	else:
		logging.error("Ping log file does not exist")
		return -1

	

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
		self.ADBIPAddress = ''
		self.ADBUserName = ''
		self.ADBPassword = ''
		self.ADBCentralized = True
		self.testCase_id = ''
		self.testXMLfiles = []
		self.testUnstable = False
		self.testMinStableId = '999999'
		self.testStabilityPointReached = False
		self.desc = ''
		self.ping_args = ''
		self.ping_packetloss_threshold = ''
		self.iperf_args = ''
		self.iperf_packetloss_threshold = ''
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
		self.CatMDevices = []
		self.UEIPAddresses = []
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
		self.expectedNbOfConnectedUEs = 0
		self.ue_id = '' #used for module identification
		self.ue_trace ='' #used to enable QLog trace for Module UE, passed to Module UE object at InitializeUE()


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
				SSH.command('grep SRC_COMMIT LAST_BUILD_INFO.txt', '\$', 2)
				result = re.search(self.ranCommitID, SSH.getBefore())
				if result is None:
					mismatch = True
				SSH.command('grep MERGED_W_TGT_BRANCH LAST_BUILD_INFO.txt', '\$', 2)
				if self.ranAllowMerge:
					result = re.search('YES', SSH.getBefore())
					if result is None:
						mismatch = True
					SSH.command('grep TGT_BRANCH LAST_BUILD_INFO.txt', '\$', 2)
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
					SSH.command('git merge --ff origin/develop -m "Temporary merge for CI"', '\$', 5)
			else:
				logging.debug('Merging with the target branch: ' + self.ranTargetBranch)
				SSH.command('git merge --ff origin/' + self.ranTargetBranch + ' -m "Temporary merge for CI"', '\$', 5)
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

	def CheckFlexranCtrlInstallation(self,RAN,EPC,CONTAINERS):
		if EPC.IPAddress == '' or EPC.UserName == '' or EPC.Password == '':
			return
		SSH = sshconnection.SSHConnection()
		SSH.open(EPC.IPAddress, EPC.UserName, EPC.Password)
		SSH.command('ls -ls /opt/flexran_rtc/*/rt_controller', '\$', 5)
		result = re.search('/opt/flexran_rtc/build/rt_controller', SSH.getBefore())
		if result is not None:
			RAN.flexranCtrlInstalled=True
			RAN.flexranCtrlIpAddress=EPC.IPAddress
			logging.debug('Flexran Controller is installed')
		else:
			# Maybe flexran-rtc is deployed into a container
			SSH.command('docker inspect --format="FLEX_RTC_IP_ADDR = {{range .NetworkSettings.Networks}}{{.IPAddress}}{{end}}" prod-flexran-rtc', '\$', 5)
			result = re.search('FLEX_RTC_IP_ADDR = (?P<flex_ip_addr>[0-9\.]+)', SSH.getBefore())
			if result is not None:
				RAN.flexranCtrlDeployed=True
				RAN.flexranCtrlIpAddress=result.group('flex_ip_addr')
				CONTAINERS.flexranCtrlDeployed=True
				CONTAINERS.flexranCtrlIpAddress=result.group('flex_ip_addr')
				logging.debug('Flexran Controller is deployed: ' + RAN.flexranCtrlIpAddress)
		SSH.close()

	def InitializeFlexranCtrl(self, HTML,RAN,EPC):
		if RAN.flexranCtrlInstalled == False:
			return
		if EPC.IPAddress == '' or EPC.UserName == '' or EPC.Password == '':
			HELP.GenericHelp(CONST.Version)
			sys.exit('Insufficient Parameter')
		SSH = sshconnection.SSHConnection()
		SSH.open(EPC.IPAddress, EPC.UserName, EPC.Password)
		SSH.command('cd /opt/flexran_rtc', '\$', 5)
		SSH.command('echo ' + EPC.Password + ' | sudo -S rm -f log/*.log', '\$', 5)
		SSH.command('echo ' + EPC.Password + ' | sudo -S echo "build/rt_controller -c log_config/basic_log" > ./my-flexran-ctl.sh', '\$', 5)
		SSH.command('echo ' + EPC.Password + ' | sudo -S chmod 755 ./my-flexran-ctl.sh', '\$', 5)
		SSH.command('echo ' + EPC.Password + ' | sudo -S daemon --unsafe --name=flexran_rtc_daemon --chdir=/opt/flexran_rtc -o /opt/flexran_rtc/log/flexranctl_' + self.testCase_id + '.log ././my-flexran-ctl.sh', '\$', 5)
		SSH.command('ps -aux | grep --color=never rt_controller', '\$', 5)
		result = re.search('rt_controller -c ', SSH.getBefore())
		if result is not None:
			logging.debug('\u001B[1m Initialize FlexRan Controller Completed\u001B[0m')
			RAN.flexranCtrlStarted=True
		SSH.close()
		HTML.CreateHtmlTestRow('N/A', 'OK', CONST.ALL_PROCESSES_OK)
	
	def InitializeUE_common(self, device_id, idx,COTS_UE):
		try:
			SSH = sshconnection.SSHConnection()
			SSH.open(self.ADBIPAddress, self.ADBUserName, self.ADBPassword)
			if not self.ADBCentralized:
				# Reboot UE
				#SSH.command('ssh ' + self.UEDevicesRemoteUser[idx] + '@' + self.UEDevicesRemoteServer[idx] + ' ' + self.UEDevicesRebootCmd[idx], '\$', 60)
				# Wait
				#time.sleep(60)
				# Put in LTE-Mode only
				#SSH.command('ssh ' + self.UEDevicesRemoteUser[idx] + '@' + self.UEDevicesRemoteServer[idx] + ' \'adb -s ' + device_id + ' shell "settings put global preferred_network_mode 11"\'', '\$', 60)
				#SSH.command('ssh ' + self.UEDevicesRemoteUser[idx] + '@' + self.UEDevicesRemoteServer[idx] + ' \'adb -s ' + device_id + ' shell "settings put global preferred_network_mode1 11"\'', '\$', 60)
				#SSH.command('ssh ' + self.UEDevicesRemoteUser[idx] + '@' + self.UEDevicesRemoteServer[idx] + ' \'adb -s ' + device_id + ' shell "settings put global preferred_network_mode2 11"\'', '\$', 60)
				#SSH.command('ssh ' + self.UEDevicesRemoteUser[idx] + '@' + self.UEDevicesRemoteServer[idx] + ' \'adb -s ' + device_id + ' shell "settings put global preferred_network_mode3 11"\'', '\$', 60)
				# enable data service
				#SSH.command('ssh ' + self.UEDevicesRemoteUser[idx] + '@' + self.UEDevicesRemoteServer[idx] + ' \'adb -s ' + device_id + ' shell "svc data enable"\'', '\$', 60)
				# we need to do radio on/off cycle to make sure of above changes
				# airplane mode off // radio on
				#SSH.command('ssh ' + self.UEDevicesRemoteUser[idx] + '@' + self.UEDevicesRemoteServer[idx] + ' ' + self.UEDevicesOnCmd[idx], '\$', 60)
				#time.sleep(10)
				# airplane mode on // radio off
				#SSH.command('ssh ' + self.UEDevicesRemoteUser[idx] + '@' + self.UEDevicesRemoteServer[idx] + ' ' + self.UEDevicesOffCmd[idx], '\$', 60)

				# normal procedure without reboot
				# enable data service
				SSH.command('ssh ' + self.UEDevicesRemoteUser[idx] + '@' + self.UEDevicesRemoteServer[idx] + ' \'adb -s ' + device_id + ' shell "svc data enable"\'', '\$', 60)
				# airplane mode on // radio off
				SSH.command('ssh ' + self.UEDevicesRemoteUser[idx] + '@' + self.UEDevicesRemoteServer[idx] + ' ' + self.UEDevicesOffCmd[idx], '\$', 60)
				SSH.close()
				return

			#RH quick add-on to integrate cots control defined by yaml
			#if device_id exists in yaml dictionary, we execute the new procedure defined in cots_ue class
			#otherwise we use the legacy procedure
			logging.debug('Device id ' + str(device_id) + ', in COTS UE dict : ' + str(COTS_UE.Check_Exists(device_id)))
			if COTS_UE.Check_Exists(device_id):
				#switch device to Airplane mode ON (ie Radio OFF) 
				COTS_UE.Set_Airplane(device_id, 'ON')
			else:
				# enable data service
				SSH.command('stdbuf -o0 adb -s ' + device_id + ' shell "svc data enable"', '\$', 60)

				# The following commands are deprecated since we no longer work on Android 7+
				# SSH.command('stdbuf -o0 adb -s ' + device_id + ' shell settings put global airplane_mode_on 1', '\$', 10)
				# SSH.command('stdbuf -o0 adb -s ' + device_id + ' shell am broadcast -a android.intent.action.AIRPLANE_MODE --ez state true', '\$', 60)
				# a dedicated script has to be installed inside the UE
				# airplane mode on means call /data/local/tmp/off
				if device_id == '84B7N16418004022':
					SSH.command('stdbuf -o0 adb -s ' + device_id + ' shell "su - root -c /data/local/tmp/off"', '\$', 60)
				else:
					SSH.command('stdbuf -o0 adb -s ' + device_id + ' shell /data/local/tmp/off', '\$', 60)
				#airplane mode off means call /data/local/tmp/on
				logging.debug('\u001B[1mUE (' + device_id + ') Initialize Completed\u001B[0m')
				SSH.close()
		except:
			os.kill(os.getppid(),signal.SIGUSR1)

	def InitializeUE(self,HTML,RAN,EPC, COTS_UE, InfraUE,ue_trace):
		if self.ue_id=='':#no ID specified, then it is a COTS controlled by ADB
			if self.ADBIPAddress == '' or self.ADBUserName == '' or self.ADBPassword == '':
				HELP.GenericHelp(CONST.Version)
				sys.exit('Insufficient Parameter')
			multi_jobs = []
			i = 0
			for device_id in self.UEDevices:
				p = Process(target = self.InitializeUE_common, args = (device_id,i,COTS_UE,))
				p.daemon = True
				p.start()
				multi_jobs.append(p)
				i += 1
			for job in multi_jobs:
				job.join()
			HTML.CreateHtmlTestRow('N/A', 'OK', CONST.ALL_PROCESSES_OK)
		else: #if an ID is specified, it is a module from the yaml infrastructure file
			#RH
			Module_UE = cls_module_ue.Module_UE(InfraUE.ci_ue_infra[self.ue_id])
			Module_UE.ue_trace=ue_trace
			is_module=Module_UE.CheckCMProcess()
			if is_module:
				Module_UE.EnableTrace()
				time.sleep(5)
				Module_UE.Command("wup")
				logging.debug("Waiting for IP address to be assigned")
				time.sleep(20)
				logging.debug("Retrieve IP address")
				status=Module_UE.GetModuleIPAddress()
				if status==0:
					HTML.CreateHtmlTestRow(Module_UE.UEIPAddress, 'OK', CONST.ALL_PROCESSES_OK)	
					logging.debug('UE IP addresss : '+ Module_UE.UEIPAddress)
				else: #status==-1 failed to retrieve IP address
					HTML.CreateHtmlTestRow('N/A', 'KO', CONST.UE_IP_ADDRESS_ISSUE)
					self.AutoTerminateUEandeNB(HTML,RAN,COTS_UE,EPC,InfraUE)
					return


	def InitializeOAIUE(self,HTML,RAN,EPC,COTS_UE,InfraUE):
		if self.UEIPAddress == '' or self.UEUserName == '' or self.UEPassword == '' or self.UESourceCodePath == '':
			HELP.GenericHelp(CONST.Version)
			sys.exit('Insufficient Parameter')

			
		if self.air_interface == 'lte-uesoftmodem':
			result = re.search('--no-L2-connect', str(self.Initialize_OAI_UE_args))
			if result is None:
				check_eNB = True
				check_OAI_UE = False
				pStatus = self.CheckProcessExist(check_eNB, check_OAI_UE,RAN,EPC)
				if (pStatus < 0):
					HTML.CreateHtmlTestRow(self.air_interface + ' ' + self.Initialize_OAI_UE_args, 'KO', pStatus)
					HTML.CreateHtmlTabFooter(False)
					self.ConditionalExit()
			UE_prefix = ''
		else:
			UE_prefix = 'NR '
		SSH = sshconnection.SSHConnection()
		SSH.open(self.UEIPAddress, self.UEUserName, self.UEPassword)
		# b2xx_fx3_utils reset procedure
		SSH.command('echo ' + self.UEPassword + ' | sudo -S uhd_find_devices', '\$', 90)
		result = re.search('type: b200', SSH.getBefore())
		if result is not None:
			logging.debug('Found a B2xx device --> resetting it')
			SSH.command('echo ' + self.UEPassword + ' | sudo -S b2xx_fx3_utils --reset-device', '\$', 10)
			# Reloading FGPA bin firmware
			SSH.command('echo ' + self.UEPassword + ' | sudo -S uhd_find_devices', '\$', 90)
		result = re.search('type: n3xx', str(SSH.getBefore()))
		if result is not None:
			logging.debug('Found a N3xx device --> resetting it')
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
				SSH.command('echo ' + self.UEPassword + ' | sudo -S ../../../targets/bin/conf2uedata -c ../../../openair3/NAS/TOOLS/ci-ue_eurecom_test_sfr.conf -o .', '\$', 5)
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
		SSH.command('echo "ulimit -c unlimited && ./'+ self.air_interface +' ' + self.Initialize_OAI_UE_args + '" > ./my-lte-uesoftmodem-run' + str(self.UE_instance) + '.sh', '\$', 5)
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
			SSH.command('echo $USER; nohup sudo -E ./my-lte-uesoftmodem-run' + str(self.UE_instance) + '.sh' + ' > ' + self.UESourceCodePath + '/cmake_targets/ue_' + self.testCase_id + '.log ' + ' 2>&1 &', self.UEUserName, 5)
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
				result = re.search('--noS1 --nokrnmod 1', str(self.Initialize_OAI_UE_args))
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
			if (self.ADBIPAddress != 'none'):
				self.UEDevices = []
				self.UEDevices.append('OAI-UE')
				self.UEDevicesStatus = []
				self.UEDevicesStatus.append(CONST.UE_STATUS_DETACHED)
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
			self.AutoTerminateUEandeNB(HTML,RAN,COTS_UE,EPC,InfraUE)

	def checkDevTTYisUnlocked(self):
		SSH = sshconnection.SSHConnection()
		SSH.open(self.ADBIPAddress, self.ADBUserName, self.ADBPassword)
		count = 0
		while count < 5:
			SSH.command('echo ' + self.ADBPassword + ' | sudo -S lsof | grep ttyUSB0', '\$', 10)
			result = re.search('picocom', SSH.getBefore())
			if result is None:
				count = 10
			else:
				time.sleep(5)
				count = count + 1
		SSH.close()

	def InitializeCatM(self,HTML):
		if self.ADBIPAddress == '' or self.ADBUserName == '' or self.ADBPassword == '':
			HELP.GenericHelp(CONST.Version)
			sys.exit('Insufficient Parameter')
		SSH = sshconnection.SSHConnection()
		SSH.enablePicocomClosure()
		SSH.open(self.ADBIPAddress, self.ADBUserName, self.ADBPassword)
		# dummy call to start a sudo session. The picocom command does NOT handle well the `sudo -S`
		SSH.command('echo ' + self.ADBPassword + ' | sudo -S ls', '\$', 10)
		SSH.command('sudo picocom --baud 921600 --flow n --databits 8 /dev/ttyUSB0', 'Terminal ready', 10)
		time.sleep(1)
		# Calling twice AT to clear all buffers
		SSH.command('AT', 'OK|ERROR', 5)
		SSH.command('AT', 'OK', 5)
		# Doing a power cycle
		SSH.command('AT^RESET', 'SIMSTORE,READY', 15)
		SSH.command('AT', 'OK|ERROR', 5)
		SSH.command('AT', 'OK', 5)
		SSH.command('ATE1', 'OK', 5)
		# Disabling the Radio
		SSH.command('AT+CFUN=0', 'OK', 5)
		logging.debug('\u001B[1m Cellular Functionality disabled\u001B[0m')
		# Checking if auto-attach is enabled
		SSH.command('AT^AUTOATT?', 'OK', 5)
		result = re.search('AUTOATT: (?P<state>[0-9\-]+)', SSH.getBefore())
		if result is not None:
			if result.group('state') is not None:
				autoAttachState = int(result.group('state'))
				if autoAttachState is not None:
					if autoAttachState == 0:
						SSH.command('AT^AUTOATT=1', 'OK', 5)
					logging.debug('\u001B[1m Auto-Attach enabled\u001B[0m')
		else:
			logging.debug('\u001B[1;37;41m Could not check Auto-Attach! \u001B[0m')
		# Force closure of picocom but device might still be locked
		SSH.close()
		SSH.disablePicocomClosure()
		HTML.CreateHtmlTestRow('N/A', 'OK', CONST.ALL_PROCESSES_OK)
		self.checkDevTTYisUnlocked()

	def TerminateCatM(self,HTML):
		if self.ADBIPAddress == '' or self.ADBUserName == '' or self.ADBPassword == '':
			HELP.GenericHelp(CONST.Version)
			sys.exit('Insufficient Parameter')
		SSH = sshconnection.SSHConnection()
		SSH.enablePicocomClosure()
		SSH.open(self.ADBIPAddress, self.ADBUserName, self.ADBPassword)
		# dummy call to start a sudo session. The picocom command does NOT handle well the `sudo -S`
		SSH.command('echo ' + self.ADBPassword + ' | sudo -S ls', '\$', 10)
		SSH.command('sudo picocom --baud 921600 --flow n --databits 8 /dev/ttyUSB0', 'Terminal ready', 10)
		time.sleep(1)
		# Calling twice AT to clear all buffers
		SSH.command('AT', 'OK|ERROR', 5)
		SSH.command('AT', 'OK', 5)
		# Disabling the Radio
		SSH.command('AT+CFUN=0', 'OK', 5)
		logging.debug('\u001B[1m Cellular Functionality disabled\u001B[0m')
		SSH.close()
		SSH.disablePicocomClosure()
		HTML.CreateHtmlTestRow('N/A', 'OK', CONST.ALL_PROCESSES_OK)
		self.checkDevTTYisUnlocked()

	def AttachCatM(self,HTML,RAN,COTS_UE,EPC,InfraUE):
		if self.ADBIPAddress == '' or self.ADBUserName == '' or self.ADBPassword == '':
			HELP.GenericHelp(CONST.Version)
			sys.exit('Insufficient Parameter')
		SSH = sshconnection.SSHConnection()
		SSH.enablePicocomClosure()
		SSH.open(self.ADBIPAddress, self.ADBUserName, self.ADBPassword)
		# dummy call to start a sudo session. The picocom command does NOT handle well the `sudo -S`
		SSH.command('echo ' + self.ADBPassword + ' | sudo -S ls', '\$', 10)
		SSH.command('sudo picocom --baud 921600 --flow n --databits 8 /dev/ttyUSB0', 'Terminal ready', 10)
		time.sleep(1)
		# Calling twice AT to clear all buffers
		SSH.command('AT', 'OK|ERROR', 5)
		SSH.command('AT', 'OK', 5)
		# Enabling the Radio
		SSH.command('AT+CFUN=1', 'SIMSTORE,READY', 5)
		logging.debug('\u001B[1m Cellular Functionality enabled\u001B[0m')
		time.sleep(4)
		# We should check if we register
		count = 0
		attach_cnt = 0
		attach_status = False
		while count < 5:
			SSH.command('AT+CEREG?', 'OK', 5)
			result = re.search('CEREG: 2,(?P<state>[0-9\-]+),', SSH.getBefore())
			if result is not None:
				mDataConnectionState = int(result.group('state'))
				if mDataConnectionState is not None:
					if mDataConnectionState == 1:
						count = 10
						attach_status = True
						result = re.search('CEREG: 2,1,"(?P<networky>[0-9A-Z]+)","(?P<networkz>[0-9A-Z]+)"', SSH.getBefore())
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
				logging.debug(SSH.getBefore())
				attach_cnt = attach_cnt + 1
			count = count + 1
			time.sleep(1)
		if attach_status:
			SSH.command('AT+CESQ', 'OK', 5)
			result = re.search('CESQ: 99,99,255,255,(?P<rsrq>[0-9]+),(?P<rsrp>[0-9]+)', SSH.getBefore())
			if result is not None:
				nRSRQ = int(result.group('rsrq'))
				nRSRP = int(result.group('rsrp'))
				if (nRSRQ is not None) and (nRSRP is not None):
					logging.debug('    RSRQ = ' + str(-20+(nRSRQ/2)) + ' dB')
					logging.debug('    RSRP = ' + str(-140+nRSRP) + ' dBm')
		SSH.close()
		SSH.disablePicocomClosure()
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
			HTML.CreateHtmlTestRowQueue('N/A', 'OK', 1, html_queue)
		else:
			logging.error('\u001B[1m CAT-M module Attachment Failed\u001B[0m')
			html_cell = '<pre style="background-color:white">CAT-M module Attachment Failed</pre>'
			html_queue.put(html_cell)
			HTML.CreateHtmlTestRowQueue('N/A', 'KO', 1, html_queue)
			self.AutoTerminateUEandeNB(HTML,RAN,COTS_UE,EPC,InfraUE)

	def PingCatM(self,HTML,RAN,EPC,COTS_UE,InfraUE):
		if EPC.IPAddress == '' or EPC.UserName == '' or EPC.Password == '' or EPC.SourceCodePath == '':
			HELP.GenericHelp(CONST.Version)
			sys.exit('Insufficient Parameter')
		check_eNB = True
		check_OAI_UE = False
		pStatus = self.CheckProcessExist(check_eNB, check_OAI_UE,RAN,EPC)
		if (pStatus < 0):
			HTML.CreateHtmlTestRow(self.ping_args, 'KO', pStatus)
			self.AutoTerminateUEandeNB(HTML,RAN,COTS_UE,EPC,InfraUE)
			return
		try:
			statusQueue = SimpleQueue()
			lock = Lock()
			SSH = sshconnection.SSHConnection()
			SSH.open(EPC.IPAddress, EPC.UserName, EPC.Password)
			SSH.command('cd ' + EPC.SourceCodePath, '\$', 5)
			SSH.command('cd scripts', '\$', 5)
			if re.match('OAI', EPC.Type, re.IGNORECASE):
				logging.debug('Using the OAI EPC HSS: not implemented yet')
				HTML.CreateHtmlTestRow(self.ping_args, 'KO', pStatus)
				HTML.CreateHtmlTabFooter(False)
				self.ConditionalExit()
			else:
				SSH.command('egrep --color=never "Allocated ipv4 addr" /opt/ltebox/var/log/xGwLog.0', '\$', 5)
				result = re.search('Allocated ipv4 addr: (?P<ipaddr>[0-9\.]+) from Pool', SSH.getBefore())
				if result is not None:
					moduleIPAddr = result.group('ipaddr')
				else:
					HTML.CreateHtmlTestRow(self.ping_args, 'KO', pStatus)
					self.AutoTerminateUEandeNB(HTML,RAN,COTS_UE,EPC,InfraUE)
					return
			ping_time = re.findall("-c (\d+)",str(self.ping_args))
			device_id = 'catm'
			ping_status = SSH.command('stdbuf -o0 ping ' + self.ping_args + ' ' + str(moduleIPAddr) + ' 2>&1 | stdbuf -o0 tee ping_' + self.testCase_id + '_' + device_id + '.log', '\$', int(ping_time[0])*1.5)
			# TIMEOUT CASE
			if ping_status < 0:
				message = 'Ping with UE (' + str(moduleIPAddr) + ') crashed due to TIMEOUT!'
				logging.debug('\u001B[1;37;41m ' + message + ' \u001B[0m')
				SSH.close()
				self.ping_iperf_wrong_exit(lock, moduleIPAddr, device_id, statusQueue, message)
				return
			result = re.search(', (?P<packetloss>[0-9\.]+)% packet loss, time [0-9\.]+ms', SSH.getBefore())
			if result is None:
				message = 'Packet Loss Not Found!'
				logging.debug('\u001B[1;37;41m ' + message + ' \u001B[0m')
				SSH.close()
				self.ping_iperf_wrong_exit(lock, moduleIPAddr, device_id, statusQueue, message)
				return
			packetloss = result.group('packetloss')
			if float(packetloss) == 100:
				message = 'Packet Loss is 100%'
				logging.debug('\u001B[1;37;41m ' + message + ' \u001B[0m')
				SSH.close()
				self.ping_iperf_wrong_exit(lock, moduleIPAddr, device_id, statusQueue, message)
				return
			result = re.search('rtt min\/avg\/max\/mdev = (?P<rtt_min>[0-9\.]+)\/(?P<rtt_avg>[0-9\.]+)\/(?P<rtt_max>[0-9\.]+)\/[0-9\.]+ ms', SSH.getBefore())
			if result is None:
				message = 'Ping RTT_Min RTT_Avg RTT_Max Not Found!'
				logging.debug('\u001B[1;37;41m ' + message + ' \u001B[0m')
				SSH.close()
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
			SSH.close()
			html_cell = '<pre style="background-color:white">CAT-M module\nIP Address  : ' + moduleIPAddr + '\n' + qMsg + '</pre>'
			statusQueue.put(html_cell)
			if (packetLossOK):
				HTML.CreateHtmlTestRowQueue(self.ping_args, 'OK', 1, statusQueue)
			else:
				HTML.CreateHtmlTestRowQueue(self.ping_args, 'KO', 1, statusQueue)
				self.AutoTerminateUEandeNB(HTML,RAN,COTS_UE,EPC,InfraUE)
		except:
			os.kill(os.getppid(),signal.SIGUSR1)

	def AttachUE_common(self, device_id, statusQueue, lock, idx,COTS_UE):
		try:
			SSH = sshconnection.SSHConnection()
			SSH.open(self.ADBIPAddress, self.ADBUserName, self.ADBPassword)
			if self.ADBCentralized:
				#RH quick add on to integrate cots control defined by yaml
				#if device Id exists in yaml dictionary, we execute the new procedure defined in cots_ue class
				#otherwise we use the legacy procedure 
				if COTS_UE.Check_Exists(device_id):
					#switch device to Airplane mode OFF (ie Radio ON)
					COTS_UE.Set_Airplane(device_id, 'OFF')
				elif device_id == '84B7N16418004022':
					SSH.command('stdbuf -o0 adb -s ' + device_id + ' shell "su - root -c /data/local/tmp/on"', '\$', 60)
				else:
					SSH.command('stdbuf -o0 adb -s ' + device_id + ' shell /data/local/tmp/on', '\$', 60)
			else:
				# airplane mode off // radio on
				SSH.command('ssh ' + self.UEDevicesRemoteUser[idx] + '@' + self.UEDevicesRemoteServer[idx] + ' ' + self.UEDevicesOnCmd[idx], '\$', 60)
			time.sleep(2)
			max_count = 45
			count = max_count
			while count > 0:
				if self.ADBCentralized:
					SSH.command('stdbuf -o0 adb -s ' + device_id + ' shell "dumpsys telephony.registry" | grep -m 1 mDataConnectionState', '\$', 15)
				else:
					SSH.command('ssh ' + self.UEDevicesRemoteUser[idx] + '@' + self.UEDevicesRemoteServer[idx] + ' \'adb -s ' + device_id + ' shell "dumpsys telephony.registry"\' | grep -m 1 mDataConnectionState', '\$', 60)
				result = re.search('mDataConnectionState.*=(?P<state>[0-9\-]+)', SSH.getBefore())
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
					#RH quick add on to intgrate cots control defined by yaml
					#if device id exists in yaml dictionary, we execute the new procedure defined in cots_ue class
					#otherwise we use the legacy procedure
						if COTS_UE.Check_Exists(device_id):
							#switch device to Airplane mode ON  (ie Radio OFF)
							COTS_UE.Set_Airplane(device_id, 'ON')
						elif device_id == '84B7N16418004022':
							SSH.command('stdbuf -o0 adb -s ' + device_id + ' shell "su - root -c /data/local/tmp/off"', '\$', 60)
						else:
							SSH.command('stdbuf -o0 adb -s ' + device_id + ' shell /data/local/tmp/off', '\$', 60)
					else:
						SSH.command('ssh ' + self.UEDevicesRemoteUser[idx] + '@' + self.UEDevicesRemoteServer[idx] + ' ' + self.UEDevicesOffCmd[idx], '\$', 60)
					time.sleep(0.5)
					if self.ADBCentralized:
					#RH quick add on to integrate cots control defined by yaml
					#if device id exists in yaml dictionary, we execute the new procedre defined incots_ue class
					#otherwise we use the legacy procedure
						if COTS_UE.Check_Exists(device_id):
							#switch device to Airplane mode OFF (ie Radio ON)
							COTS_UE.Set_Airplane(device_id, 'OFF')
						elif device_id == '84B7N16418004022':
							SSH.command('stdbuf -o0 adb -s ' + device_id + ' shell "su - root -c /data/local/tmp/on"', '\$', 60)
						else:
							SSH.command('stdbuf -o0 adb -s ' + device_id + ' shell /data/local/tmp/on', '\$', 60)
					else:
						SSH.command('ssh ' + self.UEDevicesRemoteUser[idx] + '@' + self.UEDevicesRemoteServer[idx] + ' ' + self.UEDevicesOnCmd[idx], '\$', 60)
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
			SSH.close()
		except:
			os.kill(os.getppid(),signal.SIGUSR1)

	def AttachUE(self,HTML,RAN,EPC,COTS_UE,InfraUE):
		if self.ue_id=='':#no ID specified, then it is a COTS controlled by ADB
			if self.ADBIPAddress == '' or self.ADBUserName == '' or self.ADBPassword == '':
				HELP.GenericHelp(CONST.Version)
				sys.exit('Insufficient Parameter')
			check_eNB = True
			check_OAI_UE = False
			pStatus = self.CheckProcessExist(check_eNB, check_OAI_UE,RAN,EPC)
			if (pStatus < 0):
				HTML.CreateHtmlTestRow('N/A', 'KO', pStatus)
				self.AutoTerminateUEandeNB(HTML,RAN,COTS_UE,EPC,InfraUE)
				return
			multi_jobs = []
			status_queue = SimpleQueue()
			lock = Lock()
			nb_ue_to_connect = 0
			for device_id in self.UEDevices:
				if (self.nbMaxUEtoAttach == -1) or (nb_ue_to_connect < self.nbMaxUEtoAttach):
					self.UEDevicesStatus[nb_ue_to_connect] = CONST.UE_STATUS_ATTACHING
					p = Process(target = self.AttachUE_common, args = (device_id, status_queue, lock,nb_ue_to_connect,COTS_UE,))
					p.daemon = True
					p.start()
					multi_jobs.append(p)
				nb_ue_to_connect = nb_ue_to_connect + 1
			for job in multi_jobs:
				job.join()

			if (status_queue.empty()):
				HTML.CreateHtmlTestRow('N/A', 'KO', CONST.ALL_PROCESSES_OK)
				self.AutoTerminateUEandeNB(HTML,RAN,COTS_UE,EPC,InfraUE)
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
						if self.UEDevicesStatus[cnt] == CONST.UE_STATUS_ATTACHING:
							self.UEDevicesStatus[cnt] = CONST.UE_STATUS_ATTACHED
						cnt += 1
					HTML.CreateHtmlTestRowQueue('N/A', 'OK', len(self.UEDevices), html_queue)
					result = re.search('T_stdout', str(RAN.Initialize_eNB_args))
					if result is not None:
						logging.debug('Waiting 5 seconds to fill up record file')
						time.sleep(5)
				else:
					HTML.CreateHtmlTestRowQueue('N/A', 'KO', len(self.UEDevices), html_queue)
					self.AutoTerminateUEandeNB(HTML,RAN,COTS_UE,EPC,InfraUE)

		else: #if an ID is specified, it is a module from the yaml infrastructure file
			#Attention, as opposed to InitializeUE, the connect manager process is not checked as it is supposed to be active already
			#only 1- module wakeup, 2- check IP address
			Module_UE = cls_module_ue.Module_UE(InfraUE.ci_ue_infra[self.ue_id])
			Module_UE.Command("wup")
			logging.debug("Waiting for IP address to be assigned")
			time.sleep(20)
			logging.debug("Retrieve IP address")
			status=Module_UE.GetModuleIPAddress()
			if status==0:
				HTML.CreateHtmlTestRow(Module_UE.UEIPAddress, 'OK', CONST.ALL_PROCESSES_OK)	
				logging.debug('UE IP addresss : '+ Module_UE.UEIPAddress)
			else: #status==-1 failed to retrieve IP address
				HTML.CreateHtmlTestRow('N/A', 'KO', CONST.UE_IP_ADDRESS_ISSUE)
				self.AutoTerminateUEandeNB(HTML,RAN,COTS_UE,EPC,InfraUE)
				return					

	def DetachUE_common(self, device_id, idx,COTS_UE):
		try:
			SSH = sshconnection.SSHConnection()
			SSH.open(self.ADBIPAddress, self.ADBUserName, self.ADBPassword)
			if self.ADBCentralized:
				#RH quick add on to  integrate cots control defined by yaml
				#if device id exists in yaml dictionary, we execute the new procedure defined in cots_ue class
				#otherwise we use the legacy procedure
				if COTS_UE.Check_Exists(device_id):
					#switch device to Airplane mode ON (ie Radio OFF)
					COTS_UE.Set_Airplane(device_id,'ON')
				elif device_id == '84B7N16418004022':
					SSH.command('stdbuf -o0 adb -s ' + device_id + ' shell "su - root -c /data/local/tmp/off"', '\$', 60)
				else:
					SSH.command('stdbuf -o0 adb -s ' + device_id + ' shell /data/local/tmp/off', '\$', 60)
			else:
				SSH.command('ssh ' + self.UEDevicesRemoteUser[idx] + '@' + self.UEDevicesRemoteServer[idx] + ' ' + self.UEDevicesOffCmd[idx], '\$', 60)
			logging.debug('\u001B[1mUE (' + device_id + ') Detach Completed\u001B[0m')
			SSH.close()
		except:
			os.kill(os.getppid(),signal.SIGUSR1)

	def DetachUE(self,HTML,RAN,EPC,COTS_UE,InfraUE):
		if self.ue_id=='':#no ID specified, then it is a COTS controlled by ADB
			if self.ADBIPAddress == '' or self.ADBUserName == '' or self.ADBPassword == '':
				HELP.GenericHelp(CONST.Version)
				sys.exit('Insufficient Parameter')
			check_eNB = True
			check_OAI_UE = False
			pStatus = self.CheckProcessExist(check_eNB, check_OAI_UE,RAN,EPC)
			if (pStatus < 0):
				HTML.CreateHtmlTestRow('N/A', 'KO', pStatus)
				self.AutoTerminateUEandeNB(HTML,RAN,COTS_UE,EPC,InfraUE)
				return
			multi_jobs = []
			cnt = 0
			for device_id in self.UEDevices:
				self.UEDevicesStatus[cnt] = CONST.UE_STATUS_DETACHING
				p = Process(target = self.DetachUE_common, args = (device_id,cnt,COTS_UE,))
				p.daemon = True
				p.start()
				multi_jobs.append(p)
				cnt += 1
			for job in multi_jobs:
				job.join()
			HTML.CreateHtmlTestRow('N/A', 'OK', CONST.ALL_PROCESSES_OK)
			result = re.search('T_stdout', str(RAN.Initialize_eNB_args))
			if result is not None:
				logging.debug('Waiting 5 seconds to fill up record file')
				time.sleep(5)
			cnt = 0
			while cnt < len(self.UEDevices):
				self.UEDevicesStatus[cnt] = CONST.UE_STATUS_DETACHED
				cnt += 1
		else:#if an ID is specified, it is a module from the yaml infrastructure file
			Module_UE = cls_module_ue.Module_UE(InfraUE.ci_ue_infra[self.ue_id])
			Module_UE.Command("detach")
			HTML.CreateHtmlTestRow('NA', 'OK', CONST.ALL_PROCESSES_OK)	
				
							

	def RebootUE_common(self, device_id):
		try:
			SSH = sshconnection.SSHConnection()
			SSH.open(self.ADBIPAddress, self.ADBUserName, self.ADBPassword)
			previousmDataConnectionStates = []
			# Save mDataConnectionState
			SSH.command('stdbuf -o0 adb -s ' + device_id + ' shell dumpsys telephony.registry | grep mDataConnectionState', '\$', 15)
			SSH.command('stdbuf -o0 adb -s ' + device_id + ' shell dumpsys telephony.registry | grep mDataConnectionState', '\$', 15)
			result = re.search('mDataConnectionState.*=(?P<state>[0-9\-]+)', SSH.getBefore())
			if result is None:
				logging.debug('\u001B[1;37;41m mDataConnectionState Not Found! \u001B[0m')
				sys.exit(1)
			previousmDataConnectionStates.append(int(result.group('state')))
			# Reboot UE
			SSH.command('stdbuf -o0 adb -s ' + device_id + ' shell reboot', '\$', 10)
			time.sleep(60)
			previousmDataConnectionState = previousmDataConnectionStates.pop(0)
			count = 180
			while count > 0:
				count = count - 1
				SSH.command('stdbuf -o0 adb -s ' + device_id + ' shell dumpsys telephony.registry | grep mDataConnectionState', '\$', 15)
				result = re.search('mDataConnectionState.*=(?P<state>[0-9\-]+)', SSH.getBefore())
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
			SSH.close()
		except:
			os.kill(os.getppid(),signal.SIGUSR1)

	def RebootUE(self,HTML,RAN,EPC):
		if self.ADBIPAddress == '' or self.ADBUserName == '' or self.ADBPassword == '':
			HELP.GenericHelp(CONST.Version)
			sys.exit('Insufficient Parameter')
		check_eNB = True
		check_OAI_UE = False
		pStatus = self.CheckProcessExist(check_eNB, check_OAI_UE,RAN,EPC)
		if (pStatus < 0):
			HTML.CreateHtmlTestRow('N/A', 'KO', pStatus)
			HTML.CreateHtmlTabFooter(False)
			self.ConditionalExit()
		multi_jobs = []
		for device_id in self.UEDevices:
			p = Process(target = self.RebootUE_common, args = (device_id,))
			p.daemon = True
			p.start()
			multi_jobs.append(p)
		for job in multi_jobs:
			job.join()
		HTML.CreateHtmlTestRow('N/A', 'OK', CONST.ALL_PROCESSES_OK)

	def DataDisableUE_common(self, device_id, idx):
		try:
			SSH = sshconnection.SSHConnection()
			SSH.open(self.ADBIPAddress, self.ADBUserName, self.ADBPassword)
			# disable data service
			if self.ADBCentralized:
				SSH.command('stdbuf -o0 adb -s ' + device_id + ' shell "svc data disable"', '\$', 60)
			else:
				SSH.command('ssh ' + self.UEDevicesRemoteUser[idx] + '@' + self.UEDevicesRemoteServer[idx] + ' \'adb -s ' + device_id + ' shell "svc data disable"\'', '\$', 60)
			logging.debug('\u001B[1mUE (' + device_id + ') Disabled Data Service\u001B[0m')
			SSH.close()
		except:
			os.kill(os.getppid(),signal.SIGUSR1)

	def DataDisableUE(self,HTML):
		if self.ADBIPAddress == '' or self.ADBUserName == '' or self.ADBPassword == '':
			HELP.GenericHelp(CONST.Version)
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
		HTML.CreateHtmlTestRow('N/A', 'OK', CONST.ALL_PROCESSES_OK)

	def DataEnableUE_common(self, device_id, idx):
		try:
			SSH = sshconnection.SSHConnection()
			SSH.open(self.ADBIPAddress, self.ADBUserName, self.ADBPassword)
			# enable data service
			if self.ADBCentralized:
				SSH.command('stdbuf -o0 adb -s ' + device_id + ' shell "svc data enable"', '\$', 60)
			else:
				SSH.command('ssh ' + self.UEDevicesRemoteUser[idx] + '@' + self.UEDevicesRemoteServer[idx] + ' \'adb -s ' + device_id + ' shell "svc data enable"\'', '\$', 60)
			logging.debug('\u001B[1mUE (' + device_id + ') Enabled Data Service\u001B[0m')
			SSH.close()
		except:
			os.kill(os.getppid(),signal.SIGUSR1)

	def DataEnableUE(self,HTML):
		if self.ADBIPAddress == '' or self.ADBUserName == '' or self.ADBPassword == '':
			HELP.GenericHelp(CONST.Version)
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
		HTML.CreateHtmlTestRow('N/A', 'OK', CONST.ALL_PROCESSES_OK)

	def GetAllUEDevices(self, terminate_ue_flag):
		if self.ADBIPAddress == '' or self.ADBUserName == '' or self.ADBPassword == '':
			HELP.GenericHelp(CONST.Version)
			sys.exit('Insufficient Parameter')
		SSH = sshconnection.SSHConnection()
		SSH.open(self.ADBIPAddress, self.ADBUserName, self.ADBPassword)
		if self.ADBCentralized:
			SSH.command('adb devices', '\$', 15)
			self.UEDevices = re.findall("\\\\r\\\\n([A-Za-z0-9]+)\\\\tdevice",SSH.getBefore())
			#report number and id of devices found
			msg = "UEDevices found by GetAllUEDevices : " + " ".join(self.UEDevices)
			logging.debug(msg)
			SSH.close()
		else:
			if (os.path.isfile('./phones_list.txt')):
				os.remove('./phones_list.txt')
			SSH.command('ls /etc/*/phones*.txt', '\$', 5)
			result = re.search('/etc/ci/phones_list.txt', SSH.getBefore())
			SSH.close()
			if (result is not None) and (len(self.UEDevices) == 0):
				SSH.copyin(self.ADBIPAddress, self.ADBUserName, self.ADBPassword, '/etc/ci/phones_list.txt', '.')
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

		# better handling of the case when no UE detected
		# Sys-exit is now dealt by the calling function
		if terminate_ue_flag == True:
			if len(self.UEDevices) == 0:
				logging.debug('\u001B[1;37;41m UE Not Found! \u001B[0m')
				return False
		if len(self.UEDevicesStatus) == 0:
			cnt = 0
			while cnt < len(self.UEDevices):
				self.UEDevicesStatus.append(CONST.UE_STATUS_DETACHED)
				cnt += 1
		return True

	def GetAllCatMDevices(self, terminate_ue_flag):
		if self.ADBIPAddress == '' or self.ADBUserName == '' or self.ADBPassword == '':
			HELP.GenericHelp(CONST.Version)
			sys.exit('Insufficient Parameter')
		SSH = sshconnection.SSHConnection()
		SSH.open(self.ADBIPAddress, self.ADBUserName, self.ADBPassword)
		if self.ADBCentralized:
			SSH.command('lsusb | egrep "Future Technology Devices International, Ltd FT2232C" | sed -e "s#:.*##" -e "s# #_#g"', '\$', 15)
			#self.CatMDevices = re.findall("\\\\r\\\\n([A-Za-z0-9_]+)",SSH.getBefore())
			self.CatMDevices = re.findall("\\\\r\\\\n([A-Za-z0-9_]+)",SSH.getBefore())
		else:
			if (os.path.isfile('./modules_list.txt')):
				os.remove('./modules_list.txt')
			SSH.command('ls /etc/*/modules*.txt', '\$', 5)
			result = re.search('/etc/ci/modules_list.txt', SSH.getBefore())
			SSH.close()
			if result is not None:
				logging.debug('Found a module list file on ADB server')
		if terminate_ue_flag == True:
			if len(self.CatMDevices) == 0:
				logging.debug('\u001B[1;37;41m CAT-M UE Not Found! \u001B[0m')
				sys.exit(1)
		SSH.close()

	def CheckUEStatus_common(self, lock, device_id, statusQueue, idx):
		try:
			SSH = sshconnection.SSHConnection()
			SSH.open(self.ADBIPAddress, self.ADBUserName, self.ADBPassword)
			if self.ADBCentralized:
				SSH.command('stdbuf -o0 adb -s ' + device_id + ' shell "dumpsys telephony.registry"', '\$', 15)
			else:
				SSH.command('ssh ' + self.UEDevicesRemoteUser[idx] + '@' + self.UEDevicesRemoteServer[idx] + ' \'adb -s ' + device_id + ' shell "dumpsys telephony.registry"\'', '\$', 60)
			result = re.search('mServiceState=(?P<serviceState>[0-9]+)', SSH.getBefore())
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
			result = re.search('mDataConnectionState=(?P<dataConnectionState>[0-9]+)', SSH.getBefore())
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
			result = re.search('mDataConnectionReason=(?P<dataConnectionReason>[0-9a-zA-Z_]+)', SSH.getBefore())
			time.sleep(1)
			SSH.close()
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
		except:
			os.kill(os.getppid(),signal.SIGUSR1)

	def CheckStatusUE(self,HTML,RAN,EPC,COTS_UE,InfraUE):
		if self.ADBIPAddress == '' or self.ADBUserName == '' or self.ADBPassword == '':
			HELP.GenericHelp(CONST.Version)
			sys.exit('Insufficient Parameter')
		check_eNB = True
		check_OAI_UE = False
		pStatus = self.CheckProcessExist(check_eNB, check_OAI_UE,RAN,EPC)
		if (pStatus < 0):
			HTML.CreateHtmlTestRow('N/A', 'KO', pStatus)
			HTML.CreateHtmlTabFooter(False)
			self.ConditionalExit()
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
		if (RAN.flexranCtrlInstalled and RAN.flexranCtrlStarted) or RAN.flexranCtrlDeployed:
			SSH = sshconnection.SSHConnection()
			SSH.open(EPC.IPAddress, EPC.UserName, EPC.Password)
			SSH.command('cd ' + EPC.SourceCodePath + '/scripts', '\$', 5)
			SSH.command('curl http://' + RAN.flexranCtrlIpAddress + ':9999/stats | jq \'.\' > check_status_' + self.testCase_id + '.log 2>&1', '\$', 5)
			SSH.command('cat check_status_' + self.testCase_id + '.log | jq \'.eNB_config[0].UE\' | grep -c rnti | sed -e "s#^#Nb Connected UE = #"', '\$', 5)
			result = re.search('Nb Connected UE = (?P<nb_ues>[0-9]+)', SSH.getBefore())
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
			SSH.close()
		else:
			passStatus = True
			htmlOptions = 'N/A'

		if (status_queue.empty()):
			HTML.CreateHtmlTestRow(htmlOptions, 'KO', CONST.ALL_PROCESSES_OK)
			self.AutoTerminateUEandeNB(HTML,RAN,COTS_UE,EPC,InfraUE)
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
				HTML.CreateHtmlTestRowQueue(htmlOptions, 'OK', len(self.UEDevices), html_queue)
			else:
				HTML.CreateHtmlTestRowQueue(htmlOptions, 'KO', len(self.UEDevices), html_queue)
				self.AutoTerminateUEandeNB(HTML,RAN,COTS_UE,EPC,InfraUE)

	def GetAllUEIPAddresses(self):
		SSH = sshconnection.SSHConnection()
		if self.ADBIPAddress == '' or self.ADBUserName == '' or self.ADBPassword == '':
			HELP.GenericHelp(CONST.Version)
			sys.exit('Insufficient Parameter')
		ue_ip_status = 0
		self.UEIPAddresses = []
		if (len(self.UEDevices) == 1) and (self.UEDevices[0] == 'OAI-UE'):
			if self.UEIPAddress == '' or self.UEUserName == '' or self.UEPassword == '' or self.UESourceCodePath == '':
				HELP.GenericHelp(CONST.Version)
				sys.exit('Insufficient Parameter')

			SSH.open(self.UEIPAddress, self.UEUserName, self.UEPassword)
			SSH.command('ifconfig oaitun_ue1', '\$', 4)
			result = re.search('inet addr:(?P<ueipaddress>[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+)|inet (?P<ueipaddress2>[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+)', SSH.getBefore())
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
			SSH.close()
			return ue_ip_status
		SSH.open(self.ADBIPAddress, self.ADBUserName, self.ADBPassword)
		idx = 0
		for device_id in self.UEDevices:
			if self.UEDevicesStatus[idx] != CONST.UE_STATUS_ATTACHED:
				idx += 1
				continue
			count = 0
			while count < 4:
				if self.ADBCentralized:
					SSH.command('stdbuf -o0 adb -s ' + device_id + ' shell "ip addr show | grep rmnet"', '\$', 15)
				else:
					SSH.command('ssh ' + self.UEDevicesRemoteUser[idx] + '@' + self.UEDevicesRemoteServer[idx] + ' \'adb -s ' + device_id + ' shell "ip addr show | grep rmnet"\'', '\$', 60)
				result = re.search('inet (?P<ueipaddress>[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+)\/[0-9]+[0-9a-zA-Z\.\s]+', SSH.getBefore())
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
		SSH.close()
		return ue_ip_status

	def ping_iperf_wrong_exit(self, lock, UE_IPAddress, device_id, statusQueue, message):
		lock.acquire()
		statusQueue.put(-1)
		statusQueue.put(device_id)
		statusQueue.put(UE_IPAddress)
		statusQueue.put(message)
		lock.release()

	def Ping_common(self, lock, UE_IPAddress, device_id, statusQueue,EPC, Module_UE):
		try:
			SSH = sshconnection.SSHConnection()
			# Launch ping on the EPC side (true for ltebox and old open-air-cn)
			# But for OAI-Rel14-CUPS, we launch from python executor
			launchFromEpc = True
			launchFromModule = False
			if re.match('OAI-Rel14-CUPS', EPC.Type, re.IGNORECASE):
				launchFromEpc = False
			#if module, ping from module to EPC
			if self.ue_id!='':
				launchFromEpc = False
				launchfromModule = True

			ping_time = re.findall("-c (\d+)",str(self.ping_args))

			if launchFromEpc:
				SSH.open(EPC.IPAddress, EPC.UserName, EPC.Password)
				SSH.command('cd ' + EPC.SourceCodePath, '\$', 5)
				SSH.command('cd scripts', '\$', 5)
				# In case of a docker-based deployment, we need to ping from the trf-gen container
				launchFromTrfContainer = False
				if re.match('OAI-Rel14-Docker', EPC.Type, re.IGNORECASE):
					launchFromTrfContainer = True
				if launchFromTrfContainer:
					ping_status = SSH.command('docker exec -it prod-trf-gen /bin/bash -c "ping ' + self.ping_args + ' ' + UE_IPAddress + '" 2>&1 | tee ping_' + self.testCase_id + '_' + device_id + '.log', '\$', int(ping_time[0])*1.5)				
				else:
					ping_status = SSH.command('stdbuf -o0 ping ' + self.ping_args + ' ' + UE_IPAddress + ' 2>&1 | stdbuf -o0 tee ping_' + self.testCase_id + '_' + device_id + '.log', '\$', int(ping_time[0])*1.5)
				#copy the ping log file to have it locally for analysis (ping stats)
				SSH.copyin(EPC.IPAddress, EPC.UserName, EPC.Password, EPC.SourceCodePath + '/scripts/ping_' + self.testCase_id + '_' + device_id + '.log', '.')				
			else:
				if launchfromModule == False:
					#ping log file is on the python executor
					cmd = 'ping ' + self.ping_args + ' ' + UE_IPAddress + ' 2>&1 > ping_' + self.testCase_id + '_' + device_id + '.log' 
					message = cmd + '\n'
					logging.debug(cmd)
					ret = subprocess.run(cmd, shell=True)
					ping_status = ret.returncode
					#copy the ping log file to an other folder for log collection (source and destination are EPC)
					SSH.copyout(EPC.IPAddress, EPC.UserName, EPC.Password, 'ping_' + self.testCase_id + '_' + device_id + '.log', EPC.SourceCodePath + '/scripts')
                	#copy the ping log file to have it locally for analysis (ping stats)
					logging.debug(EPC.SourceCodePath + 'ping_' + self.testCase_id + '_' + device_id + '.log')
					SSH.copyin(EPC.IPAddress, EPC.UserName, EPC.Password, EPC.SourceCodePath  +'/scripts/ping_' + self.testCase_id + '_' + device_id + '.log', '.')

					SSH.open(EPC.IPAddress, EPC.UserName, EPC.Password)
					#cat is executed on EPC
					SSH.command('cat ' + EPC.SourceCodePath + '/scripts/ping_' + self.testCase_id + '_' + device_id + '.log', '\$', 5)
				else: #launch from Module
					SSH.open(Module_UE.HostIPAddress, Module_UE.HostUsername, Module_UE.HostPassword)
					#ping from module NIC rather than IP address to make sure round trip is over the air	
					cmd = 'ping -I ' + Module_UE.UENetwork  + ' ' + self.ping_args + ' ' +  EPC.IPAddress  + ' 2>&1 > ping_' + self.testCase_id + '_' + self.ue_id + '.log' 
					SSH.command(cmd,'\$',int(ping_time[0])*1.5)
					#copy the ping log file to have it locally for analysis (ping stats)
					SSH.copyin(Module_UE.HostIPAddress, Module_UE.HostUsername, Module_UE.HostPassword, 'ping_' + self.testCase_id + '_' + self.ue_id + '.log', '.')

					#cat is executed locally 
					SSH.command('cat ping_' + self.testCase_id + '_' + self.ue_id + '.log', '\$', 5)
					ping_status=0

			# TIMEOUT CASE
			if ping_status < 0:
				message = 'Ping with UE (' + str(UE_IPAddress) + ') crashed due to TIMEOUT!'
				logging.debug('\u001B[1;37;41m ' + message + ' \u001B[0m')
				SSH.close()
				self.ping_iperf_wrong_exit(lock, UE_IPAddress, device_id, statusQueue, message)
				return
			#search is done on cat result
			result = re.search(', (?P<packetloss>[0-9\.]+)% packet loss, time [0-9\.]+ms', SSH.getBefore())
			if result is None:
				message = 'Packet Loss Not Found!'
				logging.debug('\u001B[1;37;41m ' + message + ' \u001B[0m')
				SSH.close()
				self.ping_iperf_wrong_exit(lock, UE_IPAddress, device_id, statusQueue, message)
				return
			packetloss = result.group('packetloss')
			if float(packetloss) == 100:
				message = 'Packet Loss is 100%'
				logging.debug('\u001B[1;37;41m ' + message + ' \u001B[0m')
				SSH.close()
				self.ping_iperf_wrong_exit(lock, UE_IPAddress, device_id, statusQueue, message)
				return
			result = re.search('rtt min\/avg\/max\/mdev = (?P<rtt_min>[0-9\.]+)\/(?P<rtt_avg>[0-9\.]+)\/(?P<rtt_max>[0-9\.]+)\/[0-9\.]+ ms', SSH.getBefore())
			if result is None:
				message = 'Ping RTT_Min RTT_Avg RTT_Max Not Found!'
				logging.debug('\u001B[1;37;41m ' + message + ' \u001B[0m')
				SSH.close()
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

			#adding extra ping stats from local file
			ping_log_file='ping_' + self.testCase_id + '_' + device_id + '.log'
			logging.debug('Analyzing Ping log file : ' + os.getcwd() + '/' + ping_log_file)
			ping_stat=GetPingTimeAnalysis(ping_log_file)
			ping_stat_msg=''
			if (ping_stat!=-1) and (len(ping_stat)!=0):
				ping_stat_msg+='Ping stats before removing largest value : \n'
				ping_stat_msg+='RTT(Min)    : ' + str("{:.2f}".format(ping_stat['min_0'])) + 'ms \n'
				ping_stat_msg+='RTT(Mean)   : ' + str("{:.2f}".format(ping_stat['mean_0'])) + 'ms \n'
				ping_stat_msg+='RTT(Median) : ' + str("{:.2f}".format(ping_stat['median_0'])) + 'ms \n'
				ping_stat_msg+='RTT(Max)    : ' + str("{:.2f}".format(ping_stat['max_0'])) + 'ms \n'
				ping_stat_msg+='Max Index   : ' + str(ping_stat['max_loc']) + '\n'
				ping_stat_msg+='Ping stats after removing largest value : \n'
				ping_stat_msg+='RTT(Min)    : ' + str("{:.2f}".format(ping_stat['min_1'])) + 'ms \n'
				ping_stat_msg+='RTT(Mean)   : ' + str("{:.2f}".format(ping_stat['mean_1'])) + 'ms \n'
				ping_stat_msg+='RTT(Median) : ' + str("{:.2f}".format(ping_stat['median_1'])) + 'ms \n'
				ping_stat_msg+='RTT(Max)    : ' + str("{:.2f}".format(ping_stat['max_1'])) + 'ms \n'

			#building html message
			qMsg = pal_msg + '\n' + min_msg + '\n' + avg_msg + '\n' + max_msg + '\n' + ping_stat_msg
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
			SSH.close()
		except:
			os.kill(os.getppid(),signal.SIGUSR1)

	def PingNoS1_wrong_exit(self, qMsg,HTML):
		html_queue = SimpleQueue()
		html_cell = '<pre style="background-color:white">OAI UE ping result\n' + qMsg + '</pre>'
		html_queue.put(html_cell)
		HTML.CreateHtmlTestRowQueue(self.ping_args, 'KO', len(self.UEDevices), html_queue)

	def PingNoS1(self,HTML,RAN,EPC,COTS_UE,InfraUE):
		SSH=sshconnection.SSHConnection()
		check_eNB = True
		check_OAI_UE = True
		pStatus = self.CheckProcessExist(check_eNB, check_OAI_UE,RAN,EPC)
		if (pStatus < 0):
			HTML.CreateHtmlTestRow(self.ping_args, 'KO', pStatus)
			self.AutoTerminateUEandeNB(HTML,RAN,COTS_UE,EPC,InfraUE)
			return
		ping_from_eNB = re.search('oaitun_enb1', str(self.ping_args))
		if ping_from_eNB is not None:
			if RAN.eNBIPAddress == '' or RAN.eNBUserName == '' or RAN.eNBPassword == '':
				HELP.GenericHelp(CONST.Version)
				sys.exit('Insufficient Parameter')
		else:
			if self.UEIPAddress == '' or self.UEUserName == '' or self.UEPassword == '':
				HELP.GenericHelp(CONST.Version)
				sys.exit('Insufficient Parameter')
		try:
			if ping_from_eNB is not None:
				SSH.open(RAN.eNBIPAddress, RAN.eNBUserName, RAN.eNBPassword)
				SSH.command('cd ' + RAN.eNBSourceCodePath + '/cmake_targets/', '\$', 5)
			else:
				SSH.open(self.UEIPAddress, self.UEUserName, self.UEPassword)
				SSH.command('cd ' + self.UESourceCodePath + '/cmake_targets/', '\$', 5)
			ping_time = re.findall("-c (\d+)",str(self.ping_args))
			ping_status = SSH.command('stdbuf -o0 ping ' + self.ping_args + ' 2>&1 | stdbuf -o0 tee ping_' + self.testCase_id + '.log', '\$', int(ping_time[0])*1.5)
			# TIMEOUT CASE
			if ping_status < 0:
				message = 'Ping with OAI UE crashed due to TIMEOUT!'
				logging.debug('\u001B[1;37;41m ' + message + ' \u001B[0m')
				self.PingNoS1_wrong_exit(message,HTML)
				return
			result = re.search(', (?P<packetloss>[0-9\.]+)% packet loss, time [0-9\.]+ms', SSH.getBefore())
			if result is None:
				message = 'Packet Loss Not Found!'
				logging.debug('\u001B[1;37;41m ' + message + ' \u001B[0m')
				self.PingNoS1_wrong_exit(message,HTML)
				return
			packetloss = result.group('packetloss')
			if float(packetloss) == 100:
				message = 'Packet Loss is 100%'
				logging.debug('\u001B[1;37;41m ' + message + ' \u001B[0m')
				self.PingNoS1_wrong_exit(message,HTML)
				return
			result = re.search('rtt min\/avg\/max\/mdev = (?P<rtt_min>[0-9\.]+)\/(?P<rtt_avg>[0-9\.]+)\/(?P<rtt_max>[0-9\.]+)\/[0-9\.]+ ms', SSH.getBefore())
			if result is None:
				message = 'Ping RTT_Min RTT_Avg RTT_Max Not Found!'
				logging.debug('\u001B[1;37;41m ' + message + ' \u001B[0m')
				self.PingNoS1_wrong_exit(message,HTML)
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
			SSH.close()
			html_queue = SimpleQueue()
			ip_addr = 'TBD'
			html_cell = '<pre style="background-color:white">OAI UE ping result\n' + qMsg + '</pre>'
			html_queue.put(html_cell)
			if packetLossOK:
				HTML.CreateHtmlTestRowQueue(self.ping_args, 'OK', len(self.UEDevices), html_queue)
			else:
				HTML.CreateHtmlTestRowQueue(self.ping_args, 'KO', len(self.UEDevices), html_queue)

			# copying on the EPC server for logCollection
			if ping_from_eNB is not None:
				copyin_res = SSH.copyin(RAN.eNBIPAddress, RAN.eNBUserName, RAN.eNBPassword, RAN.eNBSourceCodePath + '/cmake_targets/ping_' + self.testCase_id + '.log', '.')
			else:
				copyin_res = SSH.copyin(self.UEIPAddress, self.UEUserName, self.UEPassword, self.UESourceCodePath + '/cmake_targets/ping_' + self.testCase_id + '.log', '.')
			if (copyin_res == 0):
				SSH.copyout(EPC.IPAddress, EPC.UserName, EPC.Password, 'ping_' + self.testCase_id + '.log', EPC.SourceCodePath + '/scripts')
		except:
			os.kill(os.getppid(),signal.SIGUSR1)

	def Ping(self,HTML,RAN,EPC,COTS_UE, InfraUE):
		result = re.search('noS1', str(RAN.Initialize_eNB_args))
		if result is not None:
			self.PingNoS1(HTML,RAN,EPC,COTS_UE,InfraUE)
			return
		if EPC.IPAddress == '' or EPC.UserName == '' or EPC.Password == '' or EPC.SourceCodePath == '':
			HELP.GenericHelp(CONST.Version)
			sys.exit('Insufficient Parameter')
		check_eNB = True
		if (len(self.UEDevices) == 1) and (self.UEDevices[0] == 'OAI-UE'):
			check_OAI_UE = True
		else:
			check_OAI_UE = False
		pStatus = self.CheckProcessExist(check_eNB, check_OAI_UE,RAN,EPC)
		if (pStatus < 0):
			HTML.CreateHtmlTestRow(self.ping_args, 'KO', pStatus)
			self.AutoTerminateUEandeNB(HTML,RAN,COTS_UE,EPC,InfraUE)
			return

		if self.ue_id=="":
			Module_UE = cls_module_ue.Module_UE(InfraUE.ci_ue_infra['dummy']) #RH, temporary, we need a dummy Module_UE object to pass to Ping_common
			ueIpStatus = self.GetAllUEIPAddresses()
			if (ueIpStatus < 0):
				HTML.CreateHtmlTestRow(self.ping_args, 'KO', CONST.UE_IP_ADDRESS_ISSUE)
				self.AutoTerminateUEandeNB(HTML,RAN,COTS_UE,EPC,InfraUE)
				return
		else:
			self.UEIPAddresses=[]
			Module_UE = cls_module_ue.Module_UE(InfraUE.ci_ue_infra[self.ue_id])
			Module_UE.GetModuleIPAddress()
			self.UEIPAddresses.append(Module_UE.UEIPAddress)
		logging.debug(self.UEIPAddresses)
		multi_jobs = []
		i = 0
		lock = Lock()
		status_queue = SimpleQueue()
		for UE_IPAddress in self.UEIPAddresses:
			if self.ue_id=="":
				device_id = self.UEDevices[i]
			else:
				device_id = Module_UE.ID + "-" + Module_UE.Kind 
			p = Process(target = self.Ping_common, args = (lock,UE_IPAddress,device_id,status_queue,EPC,Module_UE,))
			p.daemon = True
			p.start()
			multi_jobs.append(p)
			i = i + 1
		for job in multi_jobs:
			job.join()

		if (status_queue.empty()):
			HTML.CreateHtmlTestRow(self.ping_args, 'KO', CONST.ALL_PROCESSES_OK)
			self.AutoTerminateUEandeNB(HTML,RAN,COTS_UE,EPC,InfraUE)
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
				HTML.CreateHtmlTestRowQueue(self.ping_args, 'OK', len(self.UEDevices), html_queue)
			else:
				HTML.CreateHtmlTestRowQueue(self.ping_args, 'KO', len(self.UEDevices), html_queue)
				self.AutoTerminateUEandeNB(HTML,RAN,COTS_UE,EPC,InfraUE)

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

	def Iperf_analyzeV2TCPOutput(self, lock, UE_IPAddress, device_id, statusQueue, iperf_real_options,EPC,SSH):

		SSH.command('awk -f /tmp/tcp_iperf_stats.awk ' + EPC.SourceCodePath + '/scripts/iperf_' + self.testCase_id + '_' + device_id + '.log', '\$', 5)
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
			response = self.Iperf_analyzeV2TCPOutput(lock, UE_IPAddress, device_id, statusQueue, iperf_real_options,EPC,SSH)
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

		result = re.search('Server Report:\\\\r\\\\n(?:|\[ *\d+\].*) (?P<bitrate>[0-9\.]+ [KMG]bits\/sec) +(?P<jitter>[0-9\.]+ ms) +(\d+\/..\d+) +(\((?P<packetloss>[0-9\.]+)%\))', SSH.getBefore())
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
				result = re.search('^\[  \d\].+ +(?P<bitrate>[0-9\.]+ [KMG]bits\/sec) +(?P<jitter>[0-9\.]+ ms) +(?P<lostPack>[0-9]+)\/(?P<sentPack>[0-9]+)', str(line))

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


	def Iperf_analyzeV3Output(self, lock, UE_IPAddress, device_id, statusQueue,SSH):

		result = re.search('(?P<bitrate>[0-9\.]+ [KMG]bits\/sec) +(?:|[0-9\.]+ ms +\d+\/\d+ \((?P<packetloss>[0-9\.]+)%\)) +(?:|receiver)\\\\r\\\\n(?:|\[ *\d+\] Sent \d+ datagrams)\\\\r\\\\niperf Done\.', SSH.getBefore())
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

	def Iperf_UL_common(self, lock, UE_IPAddress, device_id, idx, ue_num, statusQueue,EPC):
		SSH = sshconnection.SSHConnection()
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
		if re.match('OAI-Rel14-CUPS', EPC.Type, re.IGNORECASE):
			launchFromEpc = False
			cmd = 'hostname -I'
			ret = subprocess.run(cmd, shell=True, stdout=subprocess.PIPE, encoding='utf-8')
			if ret.stdout is not None:
				EPC_Iperf_UE_IPAddress = ret.stdout.strip()
		# When using a docker-based deployment, IPERF client shall be launched from trf container
		launchFromTrfContainer = False
		if re.match('OAI-Rel14-Docker', EPC.Type, re.IGNORECASE):
			launchFromTrfContainer = True
			SSH.open(EPC.IPAddress, EPC.UserName, EPC.Password)
			SSH.command('docker inspect --format="TRF_IP_ADDR = {{range .NetworkSettings.Networks}}{{.IPAddress}}{{end}}" prod-trf-gen', '\$', 5)
			result = re.search('TRF_IP_ADDR = (?P<trf_ip_addr>[0-9\.]+)', SSH.getBefore())
			if result is not None:
				EPC_Iperf_UE_IPAddress = result.group('trf_ip_addr')
			SSH.close()
		port = 5001 + idx
		udpOptions = ''
		if udpIperf:
			udpOptions = '-u '
		if launchFromEpc:
			SSH.open(EPC.IPAddress, EPC.UserName, EPC.Password)
			SSH.command('cd ' + EPC.SourceCodePath + '/scripts', '\$', 5)
			SSH.command('rm -f iperf_server_' + self.testCase_id + '_' + device_id + '.log', '\$', 5)
			if launchFromTrfContainer:
				if self.ueIperfVersion == self.dummyIperfVersion:
					prefix = ''
				else:
					prefix = ''
					if self.ueIperfVersion == '2.0.5':
						prefix = '/iperf-2.0.5/bin/'
				SSH.command('docker exec -d prod-trf-gen /bin/bash -c "nohup ' + prefix + 'iperf ' + udpOptions + '-s -i 1 -p ' + str(port) + ' > iperf_server_' + self.testCase_id + '_' + device_id + '.log &"', '\$', 5)
			else:
				SSH.command('echo $USER; nohup iperf ' + udpOptions + '-s -i 1 -p ' + str(port) + ' > iperf_server_' + self.testCase_id + '_' + device_id + '.log &', EPC.UserName, 5)
			SSH.close()
		else:
			if self.ueIperfVersion == self.dummyIperfVersion:
				prefix = ''
			else:
				prefix = ''
				if self.ueIperfVersion == '2.0.5':
					prefix = '/opt/iperf-2.0.5/bin/'
			cmd = 'nohup ' + prefix + 'iperf ' + udpOptions + '-s -i 1 -p ' + str(port) + ' > iperf_server_' + self.testCase_id + '_' + device_id + '.log 2>&1 &'
			logging.debug(cmd)
			subprocess.run(cmd, shell=True, stdout=subprocess.PIPE, encoding='utf-8')
		time.sleep(0.5)

		# Launch iperf client on UE
		if (device_id == 'OAI-UE'):
			SSH.open(self.UEIPAddress, self.UEUserName, self.UEPassword)
			SSH.command('cd ' + self.UESourceCodePath + '/cmake_targets', '\$', 5)
		else:
			SSH.open(self.ADBIPAddress, self.ADBUserName, self.ADBPassword)
			SSH.command('cd ' + EPC.SourceCodePath+ '/scripts', '\$', 5)
		iperf_time = self.Iperf_ComputeTime()
		time.sleep(0.5)

		if udpIperf:
			modified_options = self.Iperf_ComputeModifiedBW(idx, ue_num)
		else:
			modified_options = str(self.iperf_args)
		modified_options = modified_options.replace('-R','')
		time.sleep(0.5)

		SSH.command('rm -f iperf_' + self.testCase_id + '_' + device_id + '.log', '\$', 5)
		if (device_id == 'OAI-UE'):
			iperf_status = SSH.command('iperf -c ' + EPC_Iperf_UE_IPAddress + ' ' + modified_options + ' -p ' + str(port) + ' -B ' + UE_IPAddress + ' 2>&1 | stdbuf -o0 tee iperf_' + self.testCase_id + '_' + device_id + '.log', '\$', int(iperf_time)*5.0)
		else:
			if self.ADBCentralized:
				iperf_status = SSH.command('stdbuf -o0 adb -s ' + device_id + ' shell "/data/local/tmp/iperf -c ' + EPC_Iperf_UE_IPAddress + ' ' + modified_options + ' -p ' + str(port) + '" 2>&1 | stdbuf -o0 tee iperf_' + self.testCase_id + '_' + device_id + '.log', '\$', int(iperf_time)*5.0)
			else:
				iperf_status = SSH.command('ssh ' + self.UEDevicesRemoteUser[idx] + '@' + self.UEDevicesRemoteServer[idx] + ' \'adb -s ' + device_id + ' shell "/data/local/tmp/iperf -c ' + EPC_Iperf_UE_IPAddress + ' ' + modified_options + ' -p ' + str(port) + '"\' 2>&1 > iperf_' + self.testCase_id + '_' + device_id + '.log', '\$', int(iperf_time)*5.0)
				SSH.command('fromdos -o iperf_' + self.testCase_id + '_' + device_id + '.log', '\$', 5)
				SSH.command('cat iperf_' + self.testCase_id + '_' + device_id + '.log', '\$', 5)
		# TIMEOUT Case
		if iperf_status < 0:
			SSH.close()
			message = 'iperf on UE (' + str(UE_IPAddress) + ') crashed due to TIMEOUT !'
			logging.debug('\u001B[1;37;41m ' + message + ' \u001B[0m')
			SSH.close()
			self.ping_iperf_wrong_exit(lock, UE_IPAddress, device_id, statusQueue, message)
			return
		clientStatus = self.Iperf_analyzeV2Output(lock, UE_IPAddress, device_id, statusQueue, modified_options,EPC,SSH)
		SSH.close()

		# Kill iperf server on EPC side
		if launchFromEpc:
			SSH.open(EPC.IPAddress, EPC.UserName, EPC.Password)
			if launchFromTrfContainer:
				SSH.command('docker exec -it prod-trf-gen /bin/bash -c "killall --signal SIGKILL iperf"', '\$', 5)
			else:
				SSH.command('killall --signal SIGKILL iperf', EPC.UserName, 5)
			SSH.close()
		else:
			cmd = 'killall --signal SIGKILL iperf'
			logging.debug(cmd)
			subprocess.run(cmd, shell=True)
			time.sleep(1)
			SSH.copyout(EPC.IPAddress, EPC.UserName, EPC.Password, 'iperf_server_' + self.testCase_id + '_' + device_id + '.log', EPC.SourceCodePath + '/scripts')
		# in case of failure, retrieve server log
		if (clientStatus == -1) or (clientStatus == -2):
			if launchFromEpc:
				time.sleep(1)
				if (os.path.isfile('iperf_server_' + self.testCase_id + '_' + device_id + '.log')):
					os.remove('iperf_server_' + self.testCase_id + '_' + device_id + '.log')
				if launchFromTrfContainer:
					SSH.open(EPC.IPAddress, EPC.UserName, EPC.Password)
					SSH.command('docker cp prod-trf-gen:/iperf-2.0.5/iperf_server_' + self.testCase_id + '_' + device_id + '.log ' + EPC.SourceCodePath + '/scripts', '\$', 5)
					SSH.close()
				SSH.copyin(EPC.IPAddress, EPC.UserName, EPC.Password, EPC.SourceCodePath+ '/scripts/iperf_server_' + self.testCase_id + '_' + device_id + '.log', '.')
			filename='iperf_server_' + self.testCase_id + '_' + device_id + '.log'
			self.Iperf_analyzeV2Server(lock, UE_IPAddress, device_id, statusQueue, modified_options,filename,0)
		# in case of OAI-UE 
		if (device_id == 'OAI-UE'):
			SSH.copyin(self.UEIPAddress, self.UEUserName, self.UEPassword, self.UESourceCodePath + '/cmake_targets/iperf_' + self.testCase_id + '_' + device_id + '.log', '.')
			SSH.copyout(EPC.IPAddress, EPC.UserName, EPC.Password, 'iperf_' + self.testCase_id + '_' + device_id + '.log', EPC.SourceCodePath + '/scripts')


	def Iperf_Module(self, lock, UE_IPAddress, device_id, idx, ue_num, statusQueue,EPC, Module_UE):
		SSH = sshconnection.SSHConnection()
		#RH temporary quick n dirty for test
		SSH.open(EPC.IPAddress, EPC.UserName, EPC.Password)
		cmd = 'echo ' + EPC.Password + ' | sudo -S ip link set dev tun5 mtu 1358'
		SSH.command(cmd,'\$',5)	
		SSH.close()
			

		#kill iperf processes before (in case there are still some remaining)
		SSH.open(Module_UE.HostIPAddress, Module_UE.HostUsername, Module_UE.HostPassword)
		cmd = 'killall --signal=SIGKILL iperf'
		SSH.command(cmd,'\$',5)
		SSH.close()
		SSH.open(EPC.IPAddress, EPC.UserName, EPC.Password)
		cmd = 'killall --signal=SIGKILL iperf'
		SSH.command(cmd,'\$',5)
		SSH.close()


		iperf_time = self.Iperf_ComputeTime()	
		if self.iperf_direction=="DL":
			logging.debug("Iperf for Module in DL mode detected")
			#server side UE
			SSH.open(Module_UE.HostIPAddress, Module_UE.HostUsername, Module_UE.HostPassword)
			cmd = 'rm iperf_server_' +  self.testCase_id + '_' + self.ue_id + '.log'
			SSH.command(cmd,'\$',5)
			cmd = 'echo $USER; nohup /opt/iperf-2.0.10/iperf -s -B ' + UE_IPAddress + ' -u  2>&1 > iperf_server_' + self.testCase_id + '_' + self.ue_id + '.log' 
			SSH.command(cmd,'\$',5)
			SSH.close()
			#client side EPC
			SSH.open(EPC.IPAddress, EPC.UserName, EPC.Password)
			cmd = 'rm iperf_client_' + self.testCase_id + '_' + self.ue_id + '.log'
			SSH.command(cmd,'\$',5)
			cmd = 'iperf -c ' + UE_IPAddress + ' ' + self.iperf_args + ' 2>&1 > iperf_client_' + self.testCase_id + '_' + self.ue_id + '.log' 
			SSH.command(cmd,'\$',int(iperf_time)*5.0)
			SSH.close()
			#copy the 2 resulting files locally
			SSH.copyin(Module_UE.HostIPAddress, Module_UE.HostUsername, Module_UE.HostPassword, 'iperf_server_' + self.testCase_id + '_' + self.ue_id + '.log', '.')
			SSH.copyin(EPC.IPAddress, EPC.UserName, EPC.Password, 'iperf_client_' + self.testCase_id + '_' + self.ue_id + '.log', '.')
			#send for analysis
			filename='iperf_server_' + self.testCase_id + '_' + self.ue_id + '.log'
			self.Iperf_analyzeV2Server(lock, UE_IPAddress, device_id, statusQueue, self.iperf_args,filename,1)	

		elif self.iperf_direction=="UL":#does not work at the moment
			logging.debug("Iperf for Module in UL mode detected")
			#server side EPC
			SSH.open(EPC.IPAddress, EPC.UserName, EPC.Password)
			cmd = 'rm iperf_server_' + self.testCase_id + '_' + self.ue_id + '.log'
			SSH.command(cmd,'\$',5)
			cmd = 'echo $USER; nohup iperf -s -u 2>&1 > iperf_server_' + self.testCase_id + '_' + self.ue_id + '.log'
			SSH.command(cmd,'\$',5)
			SSH.close()

			#client side UE
			SSH.open(Module_UE.HostIPAddress, Module_UE.HostUsername, Module_UE.HostPassword)
			cmd = 'rm iperf_client_' + self.testCase_id + '_' + self.ue_id + '.log'
			SSH.command(cmd,'\$',5)
			SSH.command('/opt/iperf-2.0.10/iperf -c 192.172.0.1 ' + self.iperf_args + ' 2>&1 > iperf_client_' + self.testCase_id + '_' + self.ue_id + '.log', '\$', int(iperf_time)*5.0)
			SSH.close()

			#copy the 2 resulting files locally
			SSH.copyin(Module_UE.HostIPAddress, Module_UE.HostUsername, Module_UE.HostPassword, 'iperf_client_' + self.testCase_id + '_' + self.ue_id + '.log', '.')
			SSH.copyin(EPC.IPAddress, EPC.UserName, EPC.Password, 'iperf_server_' + self.testCase_id + '_' + self.ue_id + '.log', '.')
			#send for analysis
			filename='iperf_server_' + self.testCase_id + '_' + self.ue_id + '.log'
			self.Iperf_analyzeV2Server(lock, UE_IPAddress, device_id, statusQueue, self.iperf_args,filename,1)
		else :
			logging.debug("Incorrect or missing IPERF direction in XML")

		#kill iperf processes after to be clean
		SSH.open(Module_UE.HostIPAddress, Module_UE.HostUsername, Module_UE.HostPassword)
		cmd = 'killall --signal=SIGKILL iperf'
		SSH.command(cmd,'\$',5)
		SSH.close()
		SSH.open(EPC.IPAddress, EPC.UserName, EPC.Password)
		cmd = 'killall --signal=SIGKILL iperf'
		SSH.command(cmd,'\$',5)
		SSH.close()
		return

	def Iperf_common(self, lock, UE_IPAddress, device_id, idx, ue_num, statusQueue,EPC):
		try:
			SSH = sshconnection.SSHConnection()
			# Single-UE profile -- iperf only on one UE
			if self.iperf_profile == 'single-ue' and idx != 0:
				return
			useIperf3 = False
			udpIperf = True

			self.ueIperfVersion = '2.0.5'
			if (device_id != 'OAI-UE'):
				SSH.open(self.ADBIPAddress, self.ADBUserName, self.ADBPassword)
				# if by chance ADB server and EPC are on the same remote host, at least log collection will take care of it
				SSH.command('if [ ! -d ' + EPC.SourceCodePath + '/scripts ]; then mkdir -p ' + EPC.SourceCodePath + '/scripts ; fi', '\$', 5)
				SSH.command('cd ' + EPC.SourceCodePath + '/scripts', '\$', 5)
				# Checking if iperf / iperf3 are installed
				if self.ADBCentralized:
					SSH.command('adb -s ' + device_id + ' shell "ls /data/local/tmp"', '\$', 5)
				else:
					SSH.command('ssh ' + self.UEDevicesRemoteUser[idx] + '@' + self.UEDevicesRemoteServer[idx] + ' \'adb -s ' + device_id + ' shell "ls /data/local/tmp"\'', '\$', 60)
				# DEBUG: disabling iperf3 usage for the moment
				result = re.search('iperf4', SSH.getBefore())
				if result is None:
					result = re.search('iperf', SSH.getBefore())
					if result is None:
						message = 'Neither iperf nor iperf3 installed on UE!'
						logging.debug('\u001B[1;37;41m ' + message + ' \u001B[0m')
						SSH.close()
						self.ping_iperf_wrong_exit(lock, UE_IPAddress, device_id, statusQueue, message)
						return
					else:
						if self.ADBCentralized:
							SSH.command('adb -s ' + device_id + ' shell "/data/local/tmp/iperf --version"', '\$', 5)
						else:
							SSH.command('ssh ' + self.UEDevicesRemoteUser[idx] + '@' + self.UEDevicesRemoteServer[idx] + ' \'adb -s ' + device_id + ' shell "/data/local/tmp/iperf --version"\'', '\$', 60)
						result = re.search('iperf version 2.0.5', SSH.getBefore())
						if result is not None:
							self.ueIperfVersion = '2.0.5'
						result = re.search('iperf version 2.0.10', SSH.getBefore())
						if result is not None:
							self.ueIperfVersion = '2.0.10'
				else:
					useIperf3 = True
				SSH.close()
			else:
				SSH.open(self.UEIPAddress, self.UEUserName, self.UEPassword)
				SSH.command('iperf --version', '\$', 5)
				result = re.search('iperf version 2.0.5', SSH.getBefore())
				if result is not None:
					self.ueIperfVersion = '2.0.5'
				result = re.search('iperf version 2.0.10', SSH.getBefore())
				if result is not None:
					self.ueIperfVersion = '2.0.10'
				SSH.close()
			# in case of iperf, UL has its own function
			if (not useIperf3):
				result = re.search('-R', str(self.iperf_args))
				if result is not None:
					self.Iperf_UL_common(lock, UE_IPAddress, device_id, idx, ue_num, statusQueue,EPC)
					return

			# Launch the IPERF server on the UE side for DL
			if (device_id == 'OAI-UE'):
				SSH.open(self.UEIPAddress, self.UEUserName, self.UEPassword)
				SSH.command('cd ' + self.UESourceCodePath + '/cmake_targets', '\$', 5)
				SSH.command('rm -f iperf_server_' + self.testCase_id + '_' + device_id + '.log', '\$', 5)
				result = re.search('-u', str(self.iperf_args))
				if result is None:
					SSH.command('echo $USER; nohup iperf -B ' + UE_IPAddress + ' -s -i 1 > iperf_server_' + self.testCase_id + '_' + device_id + '.log &', self.UEUserName, 5)
					udpIperf = False
				else:
					SSH.command('echo $USER; nohup iperf -B ' + UE_IPAddress + ' -u -s -i 1 > iperf_server_' + self.testCase_id + '_' + device_id + '.log &', self.UEUserName, 5)
			else:
				SSH.open(self.ADBIPAddress, self.ADBUserName, self.ADBPassword)
				SSH.command('cd ' + EPC.SourceCodePath + '/scripts', '\$', 5)
				if self.ADBCentralized:
					if (useIperf3):
						SSH.command('stdbuf -o0 adb -s ' + device_id + ' shell /data/local/tmp/iperf3 -s &', '\$', 5)
					else:
						SSH.command('rm -f iperf_server_' + self.testCase_id + '_' + device_id + '.log', '\$', 5)
						result = re.search('-u', str(self.iperf_args))
						if result is None:
							SSH.command('echo $USER; nohup adb -s ' + device_id + ' shell "/data/local/tmp/iperf -s -i 1" > iperf_server_' + self.testCase_id + '_' + device_id + '.log &', self.ADBUserName, 5)
							udpIperf = False
						else:
							SSH.command('echo $USER; nohup adb -s ' + device_id + ' shell "/data/local/tmp/iperf -u -s -i 1" > iperf_server_' + self.testCase_id + '_' + device_id + '.log &', self.ADBUserName, 5)
				else:
					SSH.command('rm -f iperf_server_' + self.testCase_id + '_' + device_id + '.log', '\$', 5)
					SSH.command('echo $USER; nohup ssh ' + self.UEDevicesRemoteUser[idx] + '@' + self.UEDevicesRemoteServer[idx] + ' \'adb -s ' + device_id + ' shell "/data/local/tmp/iperf -u -s -i 1" \' 2>&1 > iperf_server_' + self.testCase_id + '_' + device_id + '.log &', self.ADBUserName, 60)

			time.sleep(0.5)
			SSH.close()

			# Launch the IPERF client on the EPC side for DL (true for ltebox and old open-air-cn
			# But for OAI-Rel14-CUPS, we launch from python executor
			launchFromEpc = True
			launchFromModule = False
			if re.match('OAI-Rel14-CUPS', EPC.Type, re.IGNORECASE):
				launchFromEpc = False
			#if module
			if self.ue_id!='' and self.iperf :
				launchFromEpc = False
				launchfromModule = True
			# When using a docker-based deployment, IPERF client shall be launched from trf container
			launchFromTrfContainer = False
			if re.match('OAI-Rel14-Docker', EPC.Type, re.IGNORECASE):
				launchFromTrfContainer = True
			if launchFromEpc:
				SSH.open(EPC.IPAddress, EPC.UserName, EPC.Password)
				SSH.command('cd ' + EPC.SourceCodePath + '/scripts', '\$', 5)
			iperf_time = self.Iperf_ComputeTime()
			time.sleep(0.5)

			if udpIperf:
				modified_options = self.Iperf_ComputeModifiedBW(idx, ue_num)
			else:
				modified_options = str(self.iperf_args)
			time.sleep(0.5)

			if launchFromEpc:
				SSH.command('rm -f iperf_' + self.testCase_id + '_' + device_id + '.log', '\$', 5)
			else:
				if (os.path.isfile('iperf_' + self.testCase_id + '_' + device_id + '.log')):
					os.remove('iperf_' + self.testCase_id + '_' + device_id + '.log')
			if (useIperf3):
				SSH.command('stdbuf -o0 iperf3 -c ' + UE_IPAddress + ' ' + modified_options + ' 2>&1 | stdbuf -o0 tee iperf_' + self.testCase_id + '_' + device_id + '.log', '\$', int(iperf_time)*5.0)

				clientStatus = 0
				self.Iperf_analyzeV3Output(lock, UE_IPAddress, device_id, statusQueue,SSH)
			else:
				if launchFromEpc:
					if launchFromTrfContainer:
						if self.ueIperfVersion == self.dummyIperfVersion:
							prefix = ''
						else:
							prefix = ''
							if self.ueIperfVersion == '2.0.5':
								prefix = '/iperf-2.0.5/bin/'
						iperf_status = SSH.command('docker exec -it prod-trf-gen /bin/bash -c "' + prefix + 'iperf -c ' + UE_IPAddress + ' ' + modified_options + '" 2>&1 | tee iperf_' + self.testCase_id + '_' + device_id + '.log', '\$', int(iperf_time)*5.0)
					else:
						iperf_status = SSH.command('stdbuf -o0 iperf -c ' + UE_IPAddress + ' ' + modified_options + ' 2>&1 | stdbuf -o0 tee iperf_' + self.testCase_id + '_' + device_id + '.log', '\$', int(iperf_time)*5.0)
				else:
					if self.ueIperfVersion == self.dummyIperfVersion:
						prefix = ''
					else:
						prefix = ''
						if self.ueIperfVersion == '2.0.5':
							prefix = '/opt/iperf-2.0.5/bin/'
					cmd = prefix + 'iperf -c ' + UE_IPAddress + ' ' + modified_options + ' 2>&1 > iperf_' + self.testCase_id + '_' + device_id + '.log'
					message = cmd + '\n'
					logging.debug(cmd)
					ret = subprocess.run(cmd, shell=True)
					iperf_status = ret.returncode
					SSH.copyout(EPC.IPAddress, EPC.UserName, EPC.Password, 'iperf_' + self.testCase_id + '_' + device_id + '.log', EPC.SourceCodePath + '/scripts')					
					SSH.open(EPC.IPAddress, EPC.UserName, EPC.Password)
					SSH.command('cat ' + EPC.SourceCodePath + '/scripts/iperf_' + self.testCase_id + '_' + device_id + '.log', '\$', 5)
				if iperf_status < 0:
					if launchFromEpc:
						SSH.close()
					message = 'iperf on UE (' + str(UE_IPAddress) + ') crashed due to TIMEOUT !'
					logging.debug('\u001B[1;37;41m ' + message + ' \u001B[0m')
					self.ping_iperf_wrong_exit(lock, UE_IPAddress, device_id, statusQueue, message)
					return
				logging.debug('Into Iperf_analyzeV2Output client')
				clientStatus = self.Iperf_analyzeV2Output(lock, UE_IPAddress, device_id, statusQueue, modified_options, EPC,SSH)
				logging.debug('Iperf_analyzeV2Output clientStatus returned value = ' + str(clientStatus))
			SSH.close()

			# Kill the IPERF server that runs in background
			if (device_id == 'OAI-UE'):
				SSH.open(self.UEIPAddress, self.UEUserName, self.UEPassword)
				SSH.command('killall iperf', '\$', 5)
			else:
				SSH.open(self.ADBIPAddress, self.ADBUserName, self.ADBPassword)
				if self.ADBCentralized:
					SSH.command('stdbuf -o0 adb -s ' + device_id + ' shell ps | grep --color=never iperf | grep -v grep', '\$', 5)
				else:
					SSH.command('ssh ' + self.UEDevicesRemoteUser[idx] + '@' + self.UEDevicesRemoteServer[idx] + ' \'adb -s ' + device_id + ' shell "ps" | grep --color=never iperf | grep -v grep\'', '\$', 60)
				result = re.search('shell +(?P<pid>\d+)', SSH.getBefore())
				if result is not None:
					pid_iperf = result.group('pid')
					if self.ADBCentralized:
						SSH.command('stdbuf -o0 adb -s ' + device_id + ' shell kill -KILL ' + pid_iperf, '\$', 5)
					else:
						SSH.command('ssh ' + self.UEDevicesRemoteUser[idx] + '@' + self.UEDevicesRemoteServer[idx] + ' \'adb -s ' + device_id + ' shell "kill -KILL ' + pid_iperf + '"\'', '\$', 60)
			SSH.close()
			# if the client report is absent, try to analyze the server log file
			if (clientStatus == -1):
				time.sleep(1)
				if (os.path.isfile('iperf_server_' + self.testCase_id + '_' + device_id + '.log')):
					os.remove('iperf_server_' + self.testCase_id + '_' + device_id + '.log')
				if (device_id == 'OAI-UE'):
					SSH.copyin(self.UEIPAddress, self.UEUserName, self.UEPassword, self.UESourceCodePath + '/cmake_targets/iperf_server_' + self.testCase_id + '_' + device_id + '.log', '.')
				else:
					SSH.copyin(self.ADBIPAddress, self.ADBUserName, self.ADBPassword, EPC.SourceCodePath + '/scripts/iperf_server_' + self.testCase_id + '_' + device_id + '.log', '.')
				# fromdos has to be called on the python executor not on ADB server
				cmd = 'fromdos -o iperf_server_' + self.testCase_id + '_' + device_id + '.log 2>&1 > /dev/null'
				try:
					subprocess.run(cmd, shell=True)
				except:
					pass
				cmd = 'dos2unix -o iperf_server_' + self.testCase_id + '_' + device_id + '.log 2>&1 > /dev/null'
				try:
					subprocess.run(cmd, shell=True)
				except:
					pass
				filename='iperf_server_' + self.testCase_id + '_' + device_id + '.log'
				self.Iperf_analyzeV2Server(lock, UE_IPAddress, device_id, statusQueue, modified_options,filename,0)

			# in case of OAI UE: 
			if (device_id == 'OAI-UE'):
				if (os.path.isfile('iperf_server_' + self.testCase_id + '_' + device_id + '.log')):
					if not launchFromEpc:
						SSH.copyout(EPC.IPAddress, EPC.UserName, EPC.Password, 'iperf_server_' + self.testCase_id + '_' + device_id + '.log', EPC.SourceCodePath + '/scripts')
				else:
					SSH.copyin(self.UEIPAddress, self.UEUserName, self.UEPassword, self.UESourceCodePath + '/cmake_targets/iperf_server_' + self.testCase_id + '_' + device_id + '.log', '.')
					SSH.copyout(EPC.IPAddress, EPC.UserName, EPC.Password, 'iperf_server_' + self.testCase_id + '_' + device_id + '.log', EPC.SourceCodePath + '/scripts')
		except:
			os.kill(os.getppid(),signal.SIGUSR1)

	def IperfNoS1(self,HTML,RAN,EPC,COTS_UE,InfraUE):
		SSH = sshconnection.SSHConnection()
		if RAN.eNBIPAddress == '' or RAN.eNBUserName == '' or RAN.eNBPassword == '' or self.UEIPAddress == '' or self.UEUserName == '' or self.UEPassword == '':
			HELP.GenericHelp(CONST.Version)
			sys.exit('Insufficient Parameter')
		check_eNB = True
		check_OAI_UE = True
		pStatus = self.CheckProcessExist(check_eNB, check_OAI_UE,RAN,EPC)
		if (pStatus < 0):
			HTML.CreateHtmlTestRow(self.iperf_args, 'KO', pStatus)
			self.AutoTerminateUEandeNB(HTML,RAN,COTS_UE,EPC,InfraUE)
			return
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
			HTML.CreateHtmlTestRowQueue(self.iperf_args, 'PERF NOT MET', len(self.UEDevices), html_queue)
		elif (iperf_status):
			HTML.CreateHtmlTestRowQueue(self.iperf_args, 'OK', len(self.UEDevices), html_queue)
		else:
			HTML.CreateHtmlTestRowQueue(self.iperf_args, 'KO', len(self.UEDevices), html_queue)
			self.AutoTerminateUEandeNB(HTML,RAN,COTS_UE,EPC,InfraUE)

	def Iperf(self,HTML,RAN,EPC,COTS_UE, InfraUE):
		result = re.search('noS1', str(RAN.Initialize_eNB_args))
		if result is not None:
			self.IperfNoS1(HTML,RAN,EPC,COTS_UE,InfraUE)
			return
		if EPC.IPAddress == '' or EPC.UserName == '' or EPC.Password == '' or EPC.SourceCodePath == '' or self.ADBIPAddress == '' or self.ADBUserName == '' or self.ADBPassword == '':
			HELP.GenericHelp(CONST.Version)
			sys.exit('Insufficient Parameter')
		check_eNB = True
		if (len(self.UEDevices) == 1) and (self.UEDevices[0] == 'OAI-UE'):
			check_OAI_UE = True
		else:
			check_OAI_UE = False
		pStatus = self.CheckProcessExist(check_eNB, check_OAI_UE,RAN,EPC)
		if (pStatus < 0):
			HTML.CreateHtmlTestRow(self.iperf_args, 'KO', pStatus)
			self.AutoTerminateUEandeNB(HTML,RAN,COTS_UE,EPC,InfraUE)
			return

		if self.ue_id=="":#is not a module, follow legacy code
			ueIpStatus = self.GetAllUEIPAddresses()
			if (ueIpStatus < 0):
				HTML.CreateHtmlTestRow(self.iperf_args, 'KO', CONST.UE_IP_ADDRESS_ISSUE)
				self.AutoTerminateUEandeNB(HTML,RAN,COTS_UE,EPC,InfraUE)
				return
		else: #is a module
			self.UEIPAddresses=[]
			Module_UE = cls_module_ue.Module_UE(InfraUE.ci_ue_infra[self.ue_id])
			Module_UE.GetModuleIPAddress()
			self.UEIPAddresses.append(Module_UE.UEIPAddress)




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
		i = 0
		ue_num = len(self.UEIPAddresses)
		lock = Lock()
		status_queue = SimpleQueue()
		logging.debug(self.UEIPAddresses)
		for UE_IPAddress in self.UEIPAddresses:
			device_id = self.UEDevices[i]
        	#special quick and dirty treatment for modules, iperf to be restructured
			if self.ue_id!="": #is module
				device_id = Module_UE.ID + "-" + Module_UE.Kind
				p = Process(target = self.Iperf_Module ,args = (lock, UE_IPAddress, device_id, i, ue_num, status_queue, EPC, Module_UE,))
			else: #legacy code
				p = Process(target = self.Iperf_common, args = (lock, UE_IPAddress, device_id, i, ue_num, status_queue, EPC, ))
			p.daemon = True
			p.start()
			multi_jobs.append(p)
			i = i + 1
		for job in multi_jobs:
			job.join()

		if (status_queue.empty()):
			HTML.CreateHtmlTestRow(self.iperf_args, 'KO', CONST.ALL_PROCESSES_OK)
			self.AutoTerminateUEandeNB(HTML,RAN,COTS_UE,EPC,InfaUE)
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
				HTML.CreateHtmlTestRowQueue(self.iperf_args, 'PERF NOT MET', len(self.UEDevices), html_queue)
			elif (iperf_status):
				HTML.CreateHtmlTestRowQueue(self.iperf_args, 'OK', len(self.UEDevices), html_queue)
			else:
				HTML.CreateHtmlTestRowQueue(self.iperf_args, 'KO', len(self.UEDevices), html_queue)
				self.AutoTerminateUEandeNB(HTML,RAN,COTS_UE,EPC,InfraUE)

	def CheckProcessExist(self, check_eNB, check_OAI_UE,RAN,EPC):
		multi_jobs = []
		status_queue = SimpleQueue()
		# in noS1 config, no need to check status from EPC
		# in gNB also currently no need to check
		result = re.search('noS1|band78', str(RAN.Initialize_eNB_args))
		if result is None:
			p = Process(target = EPC.CheckHSSProcess, args = (status_queue,))
			p.daemon = True
			p.start()
			multi_jobs.append(p)
			p = Process(target = EPC.CheckMMEProcess, args = (status_queue,))
			p.daemon = True
			p.start()
			multi_jobs.append(p)
			p = Process(target = EPC.CheckSPGWProcess, args = (status_queue,))
			p.daemon = True
			p.start()
			multi_jobs.append(p)
		else:
			if (check_eNB == False) and (check_OAI_UE == False):
				return 0
		if check_eNB:
			p = Process(target = RAN.CheckeNBProcess, args = (status_queue,))
			p.daemon = True
			p.start()
			multi_jobs.append(p)
		if check_OAI_UE:
			p = Process(target = self.CheckOAIUEProcess, args = (status_queue,))
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
			if result == CONST.ENB_PROCESS_FAILED:
				fileCheck = re.search('enb_', str(RAN.eNBLogFiles[0]))
				if fileCheck is not None:
					SSH.copyin(RAN.eNBIPAddress, RAN.eNBUserName, RAN.eNBPassword, RAN.eNBSourceCodePath + '/cmake_targets/' + RAN.eNBLogFiles[0], '.')
					logStatus = RAN.AnalyzeLogFile_eNB(RAN.eNBLogFiles[0])
					if logStatus < 0:
						result = logStatus
					RAN.eNBLogFiles[0]=''
				if RAN.flexranCtrlInstalled and RAN.flexranCtrlStarted:
					self.TerminateFlexranCtrl()
			return result

	def CheckOAIUEProcessExist(self, initialize_OAI_UE_flag,HTML,RAN):
		multi_jobs = []
		status_queue = SimpleQueue()
		if initialize_OAI_UE_flag == False:
			p = Process(target = self.CheckOAIUEProcess, args = (status_queue,))
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
			if result == CONST.OAI_UE_PROCESS_FAILED:
				fileCheck = re.search('ue_', str(self.UELogFile))
				if fileCheck is not None:
					SSH.copyin(self.UEIPAddress, self.UEUserName, self.UEPassword, self.UESourceCodePath + '/cmake_targets/' + self.UELogFile, '.')
					logStatus = self.AnalyzeLogFile_UE(self.UELogFile,HTML,RAN)
					if logStatus < 0:
						result = logStatus
			return result

	def CheckOAIUEProcess(self, status_queue):
		try:
			SSH = sshconnection.SSHConnection()
			SSH.open(self.UEIPAddress, self.UEUserName, self.UEPassword)
			SSH.command('stdbuf -o0 ps -aux | grep --color=never ' + self.air_interface + ' | grep -v grep', '\$', 5)
			result = re.search(self.air_interface, SSH.getBefore())
			if result is None:
				logging.debug('\u001B[1;37;41m OAI UE Process Not Found! \u001B[0m')
				status_queue.put(CONST.OAI_UE_PROCESS_FAILED)
			else:
				status_queue.put(CONST.OAI_UE_PROCESS_OK)
			SSH.close()
		except:
			os.kill(os.getppid(),signal.SIGUSR1)


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
		HTML.htmlUEFailureMsg=''
		global_status = CONST.ALL_PROCESSES_OK
		for line in ue_log_file.readlines():
			result = re.search('nr_synchro_time', str(line))
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
				statMsg = 'UE showed ' + str(nrDecodeMib) + ' MIB decode message(s)'
				logging.debug('\u001B[1;30;43m ' + statMsg + ' \u001B[0m')
				HTML.htmlUEFailureMsg=HTML.htmlUEFailureMsg + statMsg + '\n'
			if nrFoundDCI > 0:
				statMsg = 'UE showed ' + str(nrFoundDCI) + ' DCI found message(s)'
				logging.debug('\u001B[1;30;43m ' + statMsg + ' \u001B[0m')
				HTML.htmlUEFailureMsg=HTML.htmlUEFailureMsg + statMsg + '\n'
			if nrCRCOK > 0:
				statMsg = 'UE showed ' + str(nrCRCOK) + ' PDSCH decoding message(s)'
				logging.debug('\u001B[1;30;43m ' + statMsg + ' \u001B[0m')
				HTML.htmlUEFailureMsg=HTML.htmlUEFailureMsg + statMsg + '\n'
			if not frequency_found:
				statMsg = 'NR-UE could NOT synch!'
				logging.error('\u001B[1;30;43m ' + statMsg + ' \u001B[0m')
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


	def TerminateFlexranCtrl(self,HTML,RAN,EPC):
		if RAN.flexranCtrlInstalled == False or RAN.flexranCtrlStarted == False:
			return
		if EPC.IPAddress == '' or EPC.UserName == '' or EPC.Password == '':
			HELP.GenericHelp(CONST.Version)
			sys.exit('Insufficient Parameter')
		SSH = sshconnection.SSHConnection()
		SSH.open(EPC.IPAddress, EPC.UserName, EPC.Password)
		SSH.command('echo ' + EPC.Password + ' | sudo -S daemon --name=flexran_rtc_daemon --stop', '\$', 5)
		time.sleep(1)
		SSH.command('echo ' + EPC.Password + ' | sudo -S killall --signal SIGKILL rt_controller', '\$', 5)
		time.sleep(1)
		SSH.close()
		RAN.flexranCtrlStarted=False
		HTML.CreateHtmlTestRow('N/A', 'OK', CONST.ALL_PROCESSES_OK)

	def TerminateUE_common(self, device_id, idx,COTS_UE):
		try:
			SSH = sshconnection.SSHConnection()
			SSH.open(self.ADBIPAddress, self.ADBUserName, self.ADBPassword)
			# back in airplane mode on (ie radio off)
			if self.ADBCentralized:
				#RH quick add on to intgrate cots control defined by yaml
				#if device Id exists in yaml dictionary, we execute the new procedure defined in cots_ue class
				#otherwise we use the legacy procedure 
				if COTS_UE.Check_Exists(device_id):
					#switch device to Airplane mode ON (ie Radio OFF)
					COTS_UE.Set_Airplane(device_id, 'ON')
				elif device_id == '84B7N16418004022':
					SSH.command('stdbuf -o0 adb -s ' + device_id + ' shell "su - root -c /data/local/tmp/off"', '\$', 60)
				else:
					SSH.command('stdbuf -o0 adb -s ' + device_id + ' shell /data/local/tmp/off', '\$', 60)
			else:
				SSH.command('ssh ' + self.UEDevicesRemoteUser[idx] + '@' + self.UEDevicesRemoteServer[idx] + ' ' + self.UEDevicesOffCmd[idx], '\$', 60)
			logging.debug('\u001B[1mUE (' + device_id + ') Detach Completed\u001B[0m')

			if self.ADBCentralized:
				SSH.command('stdbuf -o0 adb -s ' + device_id + ' shell "ps | grep --color=never iperf | grep -v grep"', '\$', 5)
			else:
				SSH.command('ssh ' + self.UEDevicesRemoteUser[idx] + '@' + self.UEDevicesRemoteServer[idx] + ' \'adb -s ' + device_id + ' shell "ps | grep --color=never iperf | grep -v grep"\'', '\$', 60)
			result = re.search('shell +(?P<pid>\d+)', SSH.getBefore())
			if result is not None:
				pid_iperf = result.group('pid')
				if self.ADBCentralized:
					SSH.command('stdbuf -o0 adb -s ' + device_id + ' shell "kill -KILL ' + pid_iperf + '"', '\$', 5)
				else:
					SSH.command('ssh ' + self.UEDevicesRemoteUser[idx] + '@' + self.UEDevicesRemoteServer[idx] + ' \'adb -s ' + device_id + ' shell "kill -KILL ' + pid_iperf + '"\'', '\$', 60)
			SSH.close()
		except:
			os.kill(os.getppid(),signal.SIGUSR1)

	def TerminateUE(self,HTML,COTS_UE,InfraUE,ue_trace):
		if self.ue_id=='':#no ID specified, then it is a COTS controlled by ADB
			terminate_ue_flag = False
			self.GetAllUEDevices(terminate_ue_flag)
			multi_jobs = []
			i = 0
			for device_id in self.UEDevices:
				p = Process(target= self.TerminateUE_common, args = (device_id,i,COTS_UE,))
				p.daemon = True
				p.start()
				multi_jobs.append(p)
				i += 1
			for job in multi_jobs:
				job.join()
			HTML.CreateHtmlTestRow('N/A', 'OK', CONST.ALL_PROCESSES_OK)
		else: #if an ID is specified, it is a module from the yaml infrastructure file
			Module_UE = cls_module_ue.Module_UE(InfraUE.ci_ue_infra[self.ue_id])
			Module_UE.ue_trace=ue_trace
			Module_UE.Command("detach")	
			Module_UE.DisableTrace()
			Module_UE.DisableCM()
			archive_destination=Module_UE.LogCollect()
			if Module_UE.ue_trace=='yes':
				HTML.CreateHtmlTestRow('QLog at : '+archive_destination, 'OK', CONST.ALL_PROCESSES_OK)
			else:
				HTML.CreateHtmlTestRow('QLog trace is disabled', 'OK', CONST.ALL_PROCESSES_OK)			

	def TerminateOAIUE(self,HTML,RAN,COTS_UE,EPC, InfraUE):
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
						self.AutoTerminateUEandeNB(HTML,RAN,COTS_UE,EPC,InfraUE)
				else:
					if (logStatus == CONST.OAI_UE_PROCESS_COULD_NOT_SYNC):
						self.Initialize_OAI_UE_args = ''
						self.AutoTerminateUEandeNB(HTML,RAN,COTS_UE,EPC,InfraUE)
			else:
				logging.debug('\u001B[1m' + ueAction + ' Completed \u001B[0m')
				HTML.htmlUEFailureMsg='<b>' + ueAction + ' Completed</b>\n' + HTML.htmlUEFailureMsg
				HTML.CreateHtmlTestRow('N/A', 'OK', CONST.ALL_PROCESSES_OK)
			self.UELogFile = ''
		else:
			HTML.CreateHtmlTestRow('N/A', 'OK', CONST.ALL_PROCESSES_OK)

	def AutoTerminateUEandeNB(self,HTML,RAN,COTS_UE,EPC,InfraUE):
		if (self.ADBIPAddress != 'none'):
			self.testCase_id = 'AUTO-KILL-UE'
			HTML.testCase_id=self.testCase_id
			self.desc = 'Automatic Termination of UE'
			HTML.desc='Automatic Termination of UE'
			self.ShowTestID()
			self.TerminateUE(HTML,COTS_UE,InfraUE,self.ue_trace)
		if (self.Initialize_OAI_UE_args != ''):
			self.testCase_id = 'AUTO-KILL-OAI-UE'
			HTML.testCase_id=self.testCase_id
			self.desc = 'Automatic Termination of OAI-UE'
			HTML.desc='Automatic Termination of OAI-UE'
			self.ShowTestID()
			self.TerminateOAIUE(HTML,RAN,COTS_UE,EPC,InfraUE)
		if (RAN.Initialize_eNB_args != ''):
			self.testCase_id = 'AUTO-KILL-RAN'
			HTML.testCase_id=self.testCase_id
			self.desc = 'Automatic Termination of all RAN nodes'
			HTML.desc='Automatic Termination of RAN nodes'
			self.ShowTestID()
			#terminate all RAN nodes eNB/gNB/OCP
			for instance in range(0, len(RAN.air_interface)):
				if RAN.air_interface[instance]!='':
					logging.debug('Auto Termination of Instance ' + str(instance) + ' : ' + RAN.air_interface[instance])
					RAN.eNB_instance=instance
					RAN.TerminateeNB(HTML,EPC)
		if RAN.flexranCtrlInstalled and RAN.flexranCtrlStarted:
			self.testCase_id = 'AUTO-KILL-flexran-ctl'
			HTML.testCase_id=self.testCase_id
			self.desc = 'Automatic Termination of FlexRan CTL'
			HTML.desc='Automatic Termination of FlexRan CTL'
			self.ShowTestID()
			self.TerminateFlexranCtrl(HTML,RAN,EPC)
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

	def Perform_X2_Handover(self,HTML,RAN,EPC):
		html_queue = SimpleQueue()
		fullMessage = '<pre style="background-color:white">'
		msg = 'Doing X2 Handover w/ option ' + self.x2_ho_options
		logging.debug(msg)
		fullMessage += msg + '\n'
		if self.x2_ho_options == 'network':
			if RAN.flexranCtrlInstalled and RAN.flexranCtrlStarted:
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
					cmd = "curl -XPOST http://" + EPC.IPAddress + ":9999/rrc/x2_ho_net_control/enb/" + str(self.x2ENBBsIds[0][cnt]) + "/1"
					logging.debug(cmd)
					fullMessage += cmd + '\n'
					subprocess.run(cmd, shell=True)
					cnt += 1
				# Waiting for the activation to be active
				time.sleep(10)
				msg = "Switching UE(s) from eNB to eNB"
				logging.debug(msg)
				fullMessage += msg + '\n'
				cnt = 0
				while cnt < eNB_cnt:
					ueIdx = 0
					while ueIdx < len(self.x2ENBConnectedUEs[0][cnt]):
						cmd = "curl -XPOST http://" + EPC.IPAddress() + ":9999/rrc/ho/senb/" + str(self.x2ENBBsIds[0][cnt]) + "/ue/" + str(self.x2ENBConnectedUEs[0][cnt][ueIdx]) + "/tenb/" + str(self.x2ENBBsIds[0][eNB_cnt - cnt - 1])
						logging.debug(cmd)
						fullMessage += cmd + '\n'
						subprocess.run(cmd, shell=True)
						ueIdx += 1
					cnt += 1
				time.sleep(10)
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
					HTML.CreateHtmlTestRowQueue('N/A', 'OK', len(self.UEDevices), html_queue)
				else:
					msg = "X2 Handover FAILED"
					logging.error(msg)
					fullMessage += msg + '</pre>'
					html_queue.put(fullMessage)
					HTML.CreateHtmlTestRowQueue('N/A', 'OK', len(self.UEDevices), html_queue)
			else:
				HTML.CreateHtmlTestRow('Cannot perform requested X2 Handover', 'KO', CONST.ALL_PROCESSES_OK)

	def LogCollectBuild(self,RAN):
		SSH = sshconnection.SSHConnection()
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
		SSH.open(IPAddress, UserName, Password)
		SSH.command('cd ' + SourceCodePath, '\$', 5)
		SSH.command('cd cmake_targets', '\$', 5)
		SSH.command('rm -f build.log.zip', '\$', 5)
		SSH.command('zip -r build.log.zip build_log_*/*', '\$', 60)
		SSH.close()

	def LogCollectPing(self,EPC):
		SSH = sshconnection.SSHConnection()
		SSH.open(EPC.IPAddress, EPC.UserName, EPC.Password)
		SSH.command('cd ' + EPC.SourceCodePath, '\$', 5)
		SSH.command('cd scripts', '\$', 5)
		SSH.command('rm -f ping.log.zip', '\$', 5)
		SSH.command('zip ping.log.zip ping*.log', '\$', 60)
		SSH.command('rm ping*.log', '\$', 5)
		SSH.close()

	def LogCollectIperf(self,EPC):
		SSH = sshconnection.SSHConnection()
		SSH.open(EPC.IPAddress, EPC.UserName, EPC.Password)
		SSH.command('cd ' + EPC.SourceCodePath, '\$', 5)
		SSH.command('cd scripts', '\$', 5)
		SSH.command('rm -f iperf.log.zip', '\$', 5)
		SSH.command('zip iperf.log.zip iperf*.log', '\$', 60)
		SSH.command('rm iperf*.log', '\$', 5)
		SSH.close()
	
	def LogCollectOAIUE(self):
		SSH = sshconnection.SSHConnection()
		SSH.open(self.UEIPAddress, self.UEUserName, self.UEPassword)
		SSH.command('cd ' + self.UESourceCodePath, '\$', 5)
		SSH.command('cd cmake_targets', '\$', 5)
		SSH.command('echo ' + self.UEPassword + ' | sudo -S rm -f ue.log.zip', '\$', 5)
		SSH.command('echo ' + self.UEPassword + ' | sudo -S zip ue.log.zip ue*.log core* ue_*record.raw ue_*.pcap ue_*txt', '\$', 60)
		SSH.command('echo ' + self.UEPassword + ' | sudo -S rm ue*.log core* ue_*record.raw ue_*.pcap ue_*txt', '\$', 5)
		SSH.close()

	def RetrieveSystemVersion(self, machine,HTML,RAN):
		if RAN.eNBIPAddress == 'none' or self.UEIPAddress == 'none':
			HTML.OsVersion[0]='Ubuntu 16.04.5 LTS'
			HTML.KernelVersion[0]='4.15.0-45-generic'
			HTML.UhdVersion[0]='3.13.0.1-0'
			HTML.UsrpBoard[0]='B210'
			HTML.CpuNb[0]='4'
			HTML.CpuModel[0]='Intel(R) Core(TM) i5-6200U'
			HTML.CpuMHz[0]='2399.996 MHz'
			return 0
		if machine == 'eNB':
			if RAN.eNBIPAddress != '' and RAN.eNBUserName != '' and RAN.eNBPassword != '':
				IPAddress = RAN.eNBIPAddress
				UserName = RAN.eNBUserName
				Password = RAN.eNBPassword
				idx = 0
			else:
				return -1
		if machine == 'UE':
			if self.UEIPAddress != '' and self.UEUserName != '' and self.UEPassword != '':
				IPAddress = self.UEIPAddress
				UserName = self.UEUserName
				Password = self.UEPassword
				idx = 1
			else:
				return -1

		SSH = sshconnection.SSHConnection()
		SSH.open(IPAddress, UserName, Password)
		SSH.command('lsb_release -a', '\$', 5)
		result = re.search('Description:\\\\t(?P<os_type>[a-zA-Z0-9\-\_\.\ ]+)', SSH.getBefore())
		if result is not None:
			OsVersion = result.group('os_type')
			logging.debug('OS is: ' + OsVersion)
			HTML.OsVersion[idx]=OsVersion
		else:
			SSH.command('hostnamectl', '\$', 5)
			result = re.search('Operating System: (?P<os_type>[a-zA-Z0-9\-\_\.\ ]+)', SSH.getBefore())
			if result is not None:
				OsVersion = result.group('os_type')
				if OsVersion == 'CentOS Linux 7 ':
					SSH.command('cat /etc/redhat-release', '\$', 5)
					result = re.search('CentOS Linux release (?P<os_version>[0-9\.]+)', SSH.getBefore())
					if result is not None:
						OsVersion = OsVersion.replace('7 ', result.group('os_version'))
				logging.debug('OS is: ' + OsVersion)
				HTML.OsVersion[idx]=OsVersion
		SSH.command('uname -r', '\$', 5)
		result = re.search('uname -r\\\\r\\\\n(?P<kernel_version>[a-zA-Z0-9\-\_\.]+)', SSH.getBefore())
		if result is not None:
			KernelVersion = result.group('kernel_version')
			logging.debug('Kernel Version is: ' + KernelVersion)
			HTML.KernelVersion[idx]=KernelVersion
		SSH.command('dpkg --list | egrep --color=never libuhd003', '\$', 5)
		result = re.search('libuhd003:amd64 *(?P<uhd_version>[0-9\.]+)', SSH.getBefore())
		if result is not None:
			UhdVersion = result.group('uhd_version')
			logging.debug('UHD Version is: ' + UhdVersion)
			HTML.UhdVersion[idx]=UhdVersion
		else:
			SSH.command('uhd_config_info --version', '\$', 5)
			result = re.search('UHD (?P<uhd_version>[a-zA-Z0-9\.\-]+)', SSH.getBefore())
			if result is not None:
				UhdVersion = result.group('uhd_version')
				logging.debug('UHD Version is: ' + UhdVersion)
				HTML.UhdVersion[idx]=UhdVersion
		SSH.command('echo ' + Password + ' | sudo -S uhd_find_devices', '\$', 90)
		usrp_boards = re.findall('product: ([0-9A-Za-z]+)\\\\r\\\\n', SSH.getBefore())
		count = 0
		for board in usrp_boards:
			if count == 0:
				UsrpBoard = board
			else:
				UsrpBoard += ',' + board
			count += 1
		if count > 0:
			logging.debug('USRP Board(s) : ' + UsrpBoard)
			HTML.UsrpBoard[idx]=UsrpBoard
		SSH.command('lscpu', '\$', 5)
		result = re.search('CPU\(s\): *(?P<nb_cpus>[0-9]+).*Model name: *(?P<model>[a-zA-Z0-9\-\_\.\ \(\)]+).*CPU MHz: *(?P<cpu_mhz>[0-9\.]+)', SSH.getBefore())
		if result is not None:
			CpuNb = result.group('nb_cpus')
			logging.debug('nb_cpus: ' + CpuNb)
			HTML.CpuNb[idx]=CpuNb
			CpuModel = result.group('model')
			logging.debug('model: ' + CpuModel)
			HTML.CpuModel[idx]=CpuModel
			CpuMHz = result.group('cpu_mhz') + ' MHz'
			logging.debug('cpu_mhz: ' + CpuMHz)
			HTML.CpuMHz[idx]=CpuMHz
		SSH.close()

	def ConditionalExit(self):
		if self.testUnstable:
			if self.testStabilityPointReached or self.testMinStableId == '999999':
				sys.exit(0)
		sys.exit(1)

	def ShowTestID(self):
		logging.debug('\u001B[1m----------------------------------------\u001B[0m')
		logging.debug('\u001B[1mTest ID:' + self.testCase_id + '\u001B[0m')
		logging.debug('\u001B[1m' + self.desc + '\u001B[0m')
		logging.debug('\u001B[1m----------------------------------------\u001B[0m')
