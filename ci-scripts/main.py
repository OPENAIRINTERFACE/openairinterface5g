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
# Version
#-----------------------------------------------------------
Version = '0.1'

#-----------------------------------------------------------
# Import
#-----------------------------------------------------------
import sys		# arg
import re		# reg
import pexpect		# pexpect
import time		# sleep
import os
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
		self.eNBIPAddress = ''
		self.eNBRepository = ''
		self.eNBBranch = ''
		self.eNB_AllowMerge = False
		self.eNBCommitID = ''
		self.eNBUserName = ''
		self.eNBPassword = ''
		self.eNBSourceCodePath = ''
		self.EPCIPAddress = ''
		self.EPCUserName = ''
		self.EPCPassword = ''
		self.EPCSourceCodePath = ''
		self.EPCType = ''
		self.ADBIPAddress = ''
		self.ADBUserName = ''
		self.ADBPassword = ''
		self.testCase_id = ''
		self.testXMLfile = ''
		self.desc = ''
		self.Build_eNB_args = ''
		self.Initialize_eNB_args = ''
		self.ping_args = ''
		self.ping_packetloss_threshold = ''
		self.iperf_args = ''
		self.iperf_packetloss_threshold = ''
		self.iperf_profile = ''
		self.UEDevices = []
		self.UEIPAddresses = []
		self.htmlFile = ''
		self.htmlHeaderCreated = False
		self.htmlFooterCreated = False
		self.htmlUEConnected = 0

	def open(self, ipaddress, username, password):
		self.ssh = pexpect.spawn('ssh', [username + '@' + ipaddress], timeout = 5)
		self.sshresponse = self.ssh.expect(['Are you sure you want to continue connecting (yes/no)?', 'password:', 'Last login', pexpect.EOF, pexpect.TIMEOUT])
		if self.sshresponse == 0:
			self.ssh.sendline('yes')
			self.ssh.expect('password:')
			self.ssh.sendline(password)
			self.sshresponse = self.ssh.expect(['\$', 'Permission denied', 'password:', pexpect.EOF, pexpect.TIMEOUT])
			if self.sshresponse == 0:
				pass
			else:
				logging.debug('self.sshresponse = ' + str(self.sshresponse))
				sys.exit('SSH Connection Failed')
		elif self.sshresponse == 1:
			self.ssh.sendline(password)
			self.sshresponse = self.ssh.expect(['\$', 'Permission denied', 'password:', pexpect.EOF, pexpect.TIMEOUT])
			if self.sshresponse == 0:
				pass
			else:
				logging.debug('self.sshresponse = ' + str(self.sshresponse))
				sys.exit('SSH Connection Failed')
		elif self.sshresponse == 2:
			# Checking if we are really on the remote client defined by its IP address
			self.command('stdbuf -o0 ifconfig | egrep --color=never "inet addr:"', '\$', 5)
			result = re.search(str(ipaddress), str(self.ssh.before))
			if result is None:
				sys.exit('SSH Connection Failed: TIMEOUT !!!')
			pass
		else:
			# debug output
			logging.debug(str(self.ssh.before))
			logging.debug('self.sshresponse = ' + str(self.sshresponse))
			sys.exit('SSH Connection Failed!!!')

	def command(self, commandline, expectedline, timeout):
		logging.debug(commandline)
		self.ssh.timeout = timeout
		self.ssh.sendline(commandline)
		self.sshresponse = self.ssh.expect([expectedline, pexpect.EOF, pexpect.TIMEOUT])
		if self.sshresponse == 0:
			pass
		elif self.sshresponse == 1:
			logging.debug('\u001B[1;37;41m Unexpected EOF \u001B[0m')
			logging.debug('Expected Line : ' + expectedline)
			sys.exit(self.sshresponse)
		elif self.sshresponse == 2:
			logging.debug('\u001B[1;37;41m Unexpected TIMEOUT \u001B[0m')
			logging.debug('Expected Line : ' + expectedline)
			sys.exit(self.sshresponse)
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
			logging.debug('\u001B[1;37;41m Unexpected TIMEOUT \u001B[0m')
		else:
			logging.debug('\u001B[1;37;41m Unexpected Others \u001B[0m')

	def copy(self, ipaddress, username, password, source, destination):
		logging.debug('scp '+ username + '@' + ipaddress + ':' + source + ' ' + destination)
		scp_spawn = pexpect.spawn('scp '+ username + '@' + ipaddress + ':' + source + ' ' + destination, timeout = 5)
		scp_response = scp_spawn.expect(['Are you sure you want to continue connecting (yes/no)?', 'password:', pexpect.EOF, pexpect.TIMEOUT])
		if scp_response == 0:
			scp_spawn.sendline('yes')
			scp_spawn.expect('password:')
			scp_spawn.sendline(password)
			scp_response = scp_spawn.expect(['\$', 'Permission denied', 'password:', pexpect.EOF, pexpect.TIMEOUT])
			if scp_response == 0:
				pass
			else:
				logging.debug('1 - scp_response = ' + str(scp_response))
				sys.exit('SCP failed')
		elif scp_response == 1:
			scp_spawn.sendline(password)
			scp_response = scp_spawn.expect(['\$', 'Permission denied', 'password:', pexpect.EOF, pexpect.TIMEOUT])
			if scp_response == 0 or scp_response == 3:
				pass
			else:
				logging.debug('2 - scp_response = ' + str(scp_response))
				sys.exit('SCP failed')
		elif scp_response == 2:
			pass
		else:
			logging.debug('3 - scp_response = ' + str(scp_response))
			sys.exit('SCP failed')

	def BuildeNB(self):
		if self.eNBIPAddress == '' or self.eNBRepository == '' or self.eNBBranch == '' or self.eNBUserName == '' or self.eNBPassword == '' or self.eNBSourceCodePath == '':
			Usage()
			sys.exit('Insufficient Parameter')
		self.open(self.eNBIPAddress, self.eNBUserName, self.eNBPassword)
		self.command('mkdir -p ' + self.eNBSourceCodePath, '\$', 5)
		self.command('cd ' + self.eNBSourceCodePath, '\$', 5)
		self.command('if [ ! -e .git ]; then stdbuf -o0 git clone ' + self.eNBRepository + ' .; else stdbuf -o0 git fetch; fi', '\$', 600)
		# Raphael: here add a check if git clone or git fetch went smoothly
		self.command('git config user.email "jenkins@openairinterface.org"', '\$', 5)
		self.command('git config user.name "OAI Jenkins"', '\$', 5)
		self.command('echo ' + self.eNBPassword + ' | sudo -S git clean -x -d -ff', '\$', 30)
		# if the commit ID is provided use it to point to it
		if self.eNBCommitID != '':
			self.command('git checkout -f ' + self.eNBCommitID, '\$', 5)
		# if the branch is not develop, then it is a merge request and we need to do 
		# the potential merge. Note that merge conflicts should already been checked earlier
		if (self.eNB_AllowMerge):
			if (self.eNBBranch != 'develop') and (self.eNBBranch != 'origin/develop'):
				self.command('git merge --ff origin/develop -m "Temporary merge for CI"', '\$', 5)
		self.command('source oaienv', '\$', 5)
		self.command('cd cmake_targets', '\$', 5)
		self.command('mkdir -p  log', '\$', 5)
		# no need to remove in log (git clean did the trick)
		self.command('echo ' + self.eNBPassword + ' | sudo -S stdbuf -o0 ./build_oai ' + self.Build_eNB_args + ' 2>&1 | stdbuf -o0 tee -a compile_oai_enb.log', 'Bypassing the Tests', 600)
		self.command('mkdir -p build_log_' + SSH.testCase_id, '\$', 5)
		self.command('echo ' + self.eNBPassword + ' | sudo -S mv log/* ' + 'build_log_' + SSH.testCase_id, '\$', 5)
		self.command('echo ' + self.eNBPassword + ' | sudo -S mv compile_oai_enb.log ' + 'build_log_' + SSH.testCase_id, '\$', 5)
		self.close()
		self.CreateHtmlTestRow(self.Build_eNB_args, 'OK', 0)

	def InitializeHSS(self):
		if self.EPCIPAddress == '' or self.EPCUserName == '' or self.EPCPassword == '' or self.EPCSourceCodePath == '' or self.EPCType == '':
			Usage()
			sys.exit('Insufficient Parameter')
		self.open(self.EPCIPAddress, self.EPCUserName, self.EPCPassword)
		if re.match('OAI', self.EPCType, re.IGNORECASE):
			logging.debug('Using the OAI EPC HSS')
			self.command('cd ' + self.EPCSourceCodePath, '\$', 5)
			self.command('source oaienv', '\$', 5)
			self.command('cd scripts', '\$', 5)
			self.command('echo ' + self.EPCPassword + ' | sudo -S ./run_hss 2>&1 | stdbuf -o0 awk \'{ print strftime("[%Y/%m/%d %H:%M:%S] ",systime()) $0 }\' | stdbuf -o0 tee -a hss_' + SSH.testCase_id + '.log &', 'Core state: 2 -> 3', 35)
		else:
			logging.debug('Using the ltebox simulated HSS')
			self.command('if [ -d ' + self.EPCSourceCodePath + '/scripts ]; then echo ' + self.eNBPassword + ' | sudo -S rm -Rf ' + self.EPCSourceCodePath + '/scripts ; fi', '\$', 5)
			self.command('mkdir -p ' + self.EPCSourceCodePath + '/scripts', '\$', 5)
			self.command('cd /opt/hss_sim0609', '\$', 5)
			self.command('echo ' + self.EPCPassword + ' | sudo -S rm -f hss.log daemon.log', '\$', 5)
			self.command('echo ' + self.EPCPassword + ' | sudo -S echo "Starting sudo session" && sudo daemon --unsafe --name=simulated_hss --chdir=/opt/hss_sim0609 ./starthss_real  ', '\$', 5)
		self.close()
		self.CreateHtmlTestRow(self.EPCType, 'OK', 0)

	def InitializeMME(self):
		if self.EPCIPAddress == '' or self.EPCUserName == '' or self.EPCPassword == '' or self.EPCSourceCodePath == '' or self.EPCType == '':
			Usage()
			sys.exit('Insufficient Parameter')
		self.open(self.EPCIPAddress, self.EPCUserName, self.EPCPassword)
		if re.match('OAI', self.EPCType, re.IGNORECASE):
			self.command('cd ' + self.EPCSourceCodePath, '\$', 5)
			self.command('source oaienv', '\$', 5)
			self.command('cd scripts', '\$', 5)
			self.command('stdbuf -o0 hostname', '\$', 5)
			result = re.search('hostname\\\\r\\\\n(?P<host_name>[a-zA-Z0-9\-\_]+)\\\\r\\\\n', str(self.ssh.before))
			if result is None:
				logging.debug('\u001B[1;37;41m Hostname Not Found! \u001B[0m')
				sys.exit(1)
			host_name = result.group('host_name')
			self.command('echo ' + self.EPCPassword + ' | sudo -S ./run_mme 2>&1 | stdbuf -o0 tee -a mme_' + SSH.testCase_id + '.log &', 'MME app initialization complete', 100)
		else:
			self.command('cd /opt/ltebox/tools', '\$', 5)
			self.command('echo ' + self.EPCPassword + ' | sudo -S ./start_mme', '\$', 5)
		self.close()
		self.CreateHtmlTestRow(self.EPCType, 'OK', 0)

	def InitializeSPGW(self):
		if self.EPCIPAddress == '' or self.EPCUserName == '' or self.EPCPassword == '' or self.EPCSourceCodePath == '' or self.EPCType == '':
			Usage()
			sys.exit('Insufficient Parameter')
		self.open(self.EPCIPAddress, self.EPCUserName, self.EPCPassword)
		if re.match('OAI', self.EPCType, re.IGNORECASE):
			self.command('cd ' + self.EPCSourceCodePath, '\$', 5)
			self.command('source oaienv', '\$', 5)
			self.command('cd scripts', '\$', 5)
			self.command('echo ' + self.EPCPassword + ' | sudo -S ./run_spgw 2>&1 | stdbuf -o0 tee -a spgw_' + SSH.testCase_id + '.log &', 'Initializing SPGW-APP task interface: DONE', 30)
		else:
			self.command('cd /opt/ltebox/tools', '\$', 5)
			self.command('echo ' + self.EPCPassword + ' | sudo -S ./start_xGw', '\$', 5)
		self.close()
		self.CreateHtmlTestRow(self.EPCType, 'OK', 0)

	def InitializeeNB(self):
		if self.eNBIPAddress == '' or self.eNBUserName == '' or self.eNBPassword == '' or self.eNBSourceCodePath == '':
			Usage()
			sys.exit('Insufficient Parameter')
		initialize_eNB_flag = True
		self.CheckProcessExist(initialize_eNB_flag)
		self.open(self.eNBIPAddress, self.eNBUserName, self.eNBPassword)
		self.command('cd ' + self.eNBSourceCodePath, '\$', 5)
		# Initialize_eNB_args usually start with -O and followed by the location in repository
		full_config_file = self.Initialize_eNB_args.replace('-O ','')
		extIdx = full_config_file.find('.conf')
		if (extIdx > 0):
			extra_options = full_config_file[extIdx + 5:]
			full_config_file = full_config_file[:extIdx + 5]
			config_path, config_file = os.path.split(full_config_file)
		else:
			sys.exit('Insufficient Parameter')
		ci_full_config_file = config_path + '/ci-' + config_file
		# Make a copy and adapt to EPC / eNB IP addresses
		self.command('cp ' + full_config_file + ' ' + ci_full_config_file, '\$', 5)
		self.command('sed -i -e \'s/mme_ip_address.*$/mme_ip_address      = ( { ipv4       = "' + self.EPCIPAddress + '";/\' ' + ci_full_config_file, '\$', 2);
		self.command('sed -i -e \'s/ENB_IPV4_ADDRESS_FOR_S1_MME.*$/ENB_IPV4_ADDRESS_FOR_S1_MME              = "' + self.eNBIPAddress + '";/\' ' + ci_full_config_file, '\$', 2);
		self.command('sed -i -e \'s/ENB_IPV4_ADDRESS_FOR_S1U.*$/ENB_IPV4_ADDRESS_FOR_S1U                 = "' + self.eNBIPAddress + '";/\' ' + ci_full_config_file, '\$', 2);
		# Launch eNB with the modified config file
		self.command('source oaienv', '\$', 5)
		self.command('cd cmake_targets', '\$', 5)
		self.command('echo "./lte_build_oai/build/lte-softmodem -O ' + self.eNBSourceCodePath + '/' + ci_full_config_file + extra_options + '" > ./my-lte-softmodem-run.sh ', '\$', 5)
		self.command('chmod 775 ./my-lte-softmodem-run.sh ', '\$', 5)
		self.command('echo ' + self.eNBPassword + ' | sudo -S -E daemon --inherit --unsafe --name=enb_daemon --chdir=' + self.eNBSourceCodePath + '/cmake_targets -o ' + self.eNBSourceCodePath + '/cmake_targets/enb_' + SSH.testCase_id + '.log ./my-lte-softmodem-run.sh', '\$', 5)
		time.sleep(6)
		doLoop = True
		loopCounter = 10
		while (doLoop):
			loopCounter = loopCounter - 1
			if (loopCounter == 0):
				doLoop = False
				logging.debug('\u001B[1;30;43m eNB logging system did not show got sync! See with attach later \u001B[0m')
				self.CreateHtmlTestRow(config_file, 'eNB not showing got sync!', 0)
				# Not getting got sync is bypassed for the moment
				#sys.exit(1)
			self.command('stdbuf -o0 cat enb_' + SSH.testCase_id + '.log | grep -i sync', '\$', 10)
			result = re.search('got sync', str(self.ssh.before))
			if result is None:
				time.sleep(6)
			else:
				doLoop = False
				self.CreateHtmlTestRow(config_file, 'OK', 0)
				logging.debug('\u001B[1m Initialize eNB Completed\u001B[0m')

		self.close()

	def InitializeUE_common(self, device_id):
		logging.debug('send adb commands')
		try:
			self.open(self.ADBIPAddress, self.ADBUserName, self.ADBPassword)
			# The following commands are deprecated since we no longer work on Android 7+
			# self.command('stdbuf -o0 adb -s ' + device_id + ' shell settings put global airplane_mode_on 1', '\$', 10)
			# self.command('stdbuf -o0 adb -s ' + device_id + ' shell am broadcast -a android.intent.action.AIRPLANE_MODE --ez state true', '\$', 60)
			# a dedicated script has to be installed inside the UE
			# airplane mode on means call /data/local/tmp/off
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
		for device_id in self.UEDevices:
			p = Process(target = SSH.InitializeUE_common, args = (device_id,))
			p.daemon = True
			p.start()
			multi_jobs.append(p)
		for job in multi_jobs:
			job.join()
		self.CreateHtmlTestRow('N/A', 'OK', 0)

	def AttachUE_common(self, device_id, statusQueue, lock):
		try:
			self.open(self.ADBIPAddress, self.ADBUserName, self.ADBPassword)
			self.command('stdbuf -o0 adb -s ' + device_id + ' shell /data/local/tmp/on', '\$', 60)
			time.sleep(2)
			max_count = 45
			count = max_count
			while count > 0:
				self.command('stdbuf -o0 adb -s ' + device_id + ' shell dumpsys telephony.registry | grep mDataConnectionState', '\$', 15)
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
					logging.debug('\u001B[1;37;43m Retry UE (' + device_id + ') Flight Mode Off \u001B[0m')
					self.command('stdbuf -o0 adb -s ' + device_id + ' shell /data/local/tmp/off', '\$', 60)
					time.sleep(0.5)
					self.command('stdbuf -o0 adb -s ' + device_id + ' shell /data/local/tmp/on', '\$', 60)
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
		initialize_eNB_flag = False
		self.CheckProcessExist(initialize_eNB_flag)
		multi_jobs = []
		status_queue = SimpleQueue()
		lock = Lock()
		for device_id in self.UEDevices:
			p = Process(target = SSH.AttachUE_common, args = (device_id, status_queue, lock,))
			p.daemon = True
			p.start()
			multi_jobs.append(p)
		for job in multi_jobs:
			job.join()

		if (status_queue.empty()):
			self.CreateHtmlTestRow('N/A', 'KO', len(self.UEDevices))
			sys.exit(1)
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
					html_cell = "<pre>UE (" + device_id + ")\n" + message + "</pre>"
				else:
					html_cell = "<pre>UE (" + device_id + ")\n" + message + ' in ' + str(count + 2) + ' seconds</pre>'
				html_queue.put(html_cell)
			if (attach_status):
				self.CreateHtmlTestRowQueue('N/A', 'OK', len(self.UEDevices), html_queue)
			else:
				self.CreateHtmlTestRowQueue('N/A', 'KO', len(self.UEDevices), html_queue)
				self.CreateHtmlFooter()
				sys.exit(1)

	def DetachUE_common(self, device_id):
		try:
			self.open(self.ADBIPAddress, self.ADBUserName, self.ADBPassword)
			self.command('stdbuf -o0 adb -s ' + device_id + ' shell /data/local/tmp/off', '\$', 60)
			logging.debug('\u001B[1mUE (' + device_id + ') Detach Completed\u001B[0m')
			self.close()
		except:
			os.kill(os.getppid(),signal.SIGUSR1)

	def DetachUE(self):
		if self.ADBIPAddress == '' or self.ADBUserName == '' or self.ADBPassword == '':
			Usage()
			sys.exit('Insufficient Parameter')
		initialize_eNB_flag = False
		self.CheckProcessExist(initialize_eNB_flag)
		multi_jobs = []
		for device_id in self.UEDevices:
			p = Process(target = SSH.DetachUE_common, args = (device_id,))
			p.daemon = True
			p.start()
			multi_jobs.append(p)
		for job in multi_jobs:
			job.join()
		self.CreateHtmlTestRow('N/A', 'OK', 0)

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
		initialize_eNB_flag = False
		self.CheckProcessExist(initialize_eNB_flag)
		multi_jobs = []
		for device_id in self.UEDevices:
			p = Process(target = SSH.RebootUE_common, args = (device_id,))
			p.daemon = True
			p.start()
			multi_jobs.append(p)
		for job in multi_jobs:
			job.join()
		self.CreateHtmlTestRow('N/A', 'OK', 0)

	def GetAllUEDevices(self, terminate_ue_flag):
		if self.ADBIPAddress == '' or self.ADBUserName == '' or self.ADBPassword == '':
			Usage()
			sys.exit('Insufficient Parameter')
		self.open(self.ADBIPAddress, self.ADBUserName, self.ADBPassword)
		self.command('adb devices', '\$', 15)
		self.UEDevices = re.findall("\\\\r\\\\n([A-Za-z0-9]+)\\\\tdevice",str(self.ssh.before))
		if terminate_ue_flag == False:
			if len(self.UEDevices) == 0:
				logging.debug('\u001B[1;37;41m UE Not Found! \u001B[0m')
				sys.exit(1)
		self.close()

	def GetAllUEIPAddresses(self):
		if self.ADBIPAddress == '' or self.ADBUserName == '' or self.ADBPassword == '':
			Usage()
			sys.exit('Insufficient Parameter')
		self.UEIPAddresses = []
		self.open(self.ADBIPAddress, self.ADBUserName, self.ADBPassword)
		for device_id in self.UEDevices:
			self.command('stdbuf -o0 adb -s ' + device_id + ' shell ip addr show | grep rmnet', '\$', 15)
			result = re.search('inet (?P<ueipaddress>[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+)\/[0-9]+[0-9a-zA-Z\.\s]+', str(self.ssh.before))
			if result is None:
				logging.debug('\u001B[1;37;41m UE IP Address Not Found! \u001B[0m')
				sys.exit(1)
			UE_IPAddress = result.group('ueipaddress')
			logging.debug('\u001B[1mUE (' + device_id + ') IP Address is ' + UE_IPAddress + '\u001B[0m')
			for ueipaddress in self.UEIPAddresses:
				if ueipaddress == UE_IPAddress:
					logging.debug('\u001B[1mUE (' + device_id + ') IP Address ' + UE_IPAddress + 'has been existed!' + '\u001B[0m')
					sys.exit(1)
			self.UEIPAddresses.append(UE_IPAddress)
		self.close()

	def Ping_common(self, lock, UE_IPAddress, device_id,statusQueue):
		try:
			self.open(self.EPCIPAddress, self.EPCUserName, self.EPCPassword)
			self.command('cd ' + self.EPCSourceCodePath, '\$', 5)
			self.command('cd scripts', '\$', 5)
			ping_time = re.findall("-c (\d+)",str(self.ping_args))
			self.command('stdbuf -o0 ping ' + self.ping_args + ' ' + UE_IPAddress + ' 2>&1 | stdbuf -o0 tee -a ping_' + SSH.testCase_id + '_' + device_id + '.log', '\$', int(ping_time[0])*1.5)
			result = re.search(', (?P<packetloss>[0-9\.]+)% packet loss, time [0-9\.]+ms', str(self.ssh.before))
			if result is None:
				message = 'Packet Loss Not Found!'
				logging.debug('\u001B[1;37;41m ' + message + ' \u001B[0m')
				lock.acquire()
				statusQueue.put(-1)
				statusQueue.put(device_id)
				statusQueue.put(message)
				lock.release()
				return
			packetloss = result.group('packetloss')
			if float(packetloss) == 100:
				message = 'Packet Loss is 100%'
				logging.debug('\u001B[1;37;41m ' + message + ' \u001B[0m')
				lock.acquire()
				statusQueue.put(-1)
				statusQueue.put(device_id)
				statusQueue.put(message)
				lock.release()
				return
			result = re.search('rtt min\/avg\/max\/mdev = (?P<rtt_min>[0-9\.]+)\/(?P<rtt_avg>[0-9\.]+)\/(?P<rtt_max>[0-9\.]+)\/[0-9\.]+ ms', str(self.ssh.before))
			if result is None:
				message = 'Ping RTT_Min RTT_Avg RTT_Max Not Found!'
				logging.debug('\u001B[1;37;41m ' + message + ' \u001B[0m')
				lock.acquire()
				statusQueue.put(-1)
				statusQueue.put(device_id)
				statusQueue.put(message)
				lock.release()
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
					logging.debug('\u001B[1;37;43m Packet Loss is not 0% \u001B[0m')
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

	def Ping(self):
		if self.EPCIPAddress == '' or self.EPCUserName == '' or self.EPCPassword == '' or self.EPCSourceCodePath == '':
			Usage()
			sys.exit('Insufficient Parameter')
		initialize_eNB_flag = False
		self.CheckProcessExist(initialize_eNB_flag)
		self.GetAllUEIPAddresses()
		multi_jobs = []
		i = 0
		lock = Lock()
		status_queue = SimpleQueue()
		for UE_IPAddress in self.UEIPAddresses:
			device_id = self.UEDevices[i]
			p = Process(target = SSH.Ping_common, args = (lock,UE_IPAddress,device_id,status_queue,))
			p.daemon = True
			p.start()
			multi_jobs.append(p)
			i = i + 1
		for job in multi_jobs:
			job.join()

		if (status_queue.empty()):
			self.CreateHtmlTestRow(self.ping_args, 'KO', len(self.UEDevices))
			sys.exit(1)
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
				html_cell = "<pre>UE (" + device_id + ")\nIP Address  : " + ip_addr + "\n" + message + "</pre>"
				html_queue.put(html_cell)
			if (ping_status):
				self.CreateHtmlTestRowQueue(self.ping_args, 'OK', len(self.UEDevices), html_queue)
			else:
				self.CreateHtmlTestRowQueue(self.ping_args, 'KO', len(self.UEDevices), html_queue)
				self.CreateHtmlFooter()
				sys.exit(1)

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
		if SSH.iperf_profile == 'balanced':
			iperf_bandwidth_new = float(iperf_bandwidth)/ue_num
		if SSH.iperf_profile == 'single-ue':
			iperf_bandwidth_new = float(iperf_bandwidth)
		if SSH.iperf_profile == 'unbalanced':
			# residual is 2% of max bw
			residualBW = float(iperf_bandwidth) / 50
			if idx == 0:
				iperf_bandwidth_new = float(iperf_bandwidth) - ((ue_num - 1) * residualBW)
			else:
				iperf_bandwidth_new = residualBW
		iperf_bandwidth_str = '-b ' + iperf_bandwidth
		iperf_bandwidth_str_new = '-b ' + str(iperf_bandwidth_new)
		result = re.sub(iperf_bandwidth_str, iperf_bandwidth_str_new, str(self.iperf_args))
		if result is None:
			logging.debug('\u001B[1;37;41m Calculate Iperf bandwidth Failed! \u001B[0m')
			sys.exit(1)
		return result

	def Iperf_analyzeV2Output(self, lock, UE_IPAddress, device_id, statusQueue, iperf_real_options):
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
		if (not os.path.isfile('iperf_server_' + SSH.testCase_id + '_' + device_id + '.log')):
			lock.acquire()
			statusQueue.put(-1)
			statusQueue.put(device_id)
			statusQueue.put(UE_IPAddress)
			statusQueue.put('Could not analyze from server log')
			lock.release()
			return
		# Computing the requested bandwidth in float
		result = re.search('-b (?P<iperf_bandwidth>[0-9\.]+)[KMG]', str(iperf_real_options))
		if result is None:
			logging.debug('Iperf bandwidth Not Found!')
			lock.acquire()
			statusQueue.put(-1)
			statusQueue.put(device_id)
			statusQueue.put(UE_IPAddress)
			statusQueue.put('Could not compute Iperf bandwidth!')
			lock.release()
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

		server_file = open('iperf_server_' + SSH.testCase_id + '_' + device_id + '.log', 'r')
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
			lock.acquire()
			statusQueue.put(-1)
			statusQueue.put(device_id)
			statusQueue.put(UE_IPAddress)
			statusQueue.put('Could not analyze from server log')
			lock.release()

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
		lock.release()

	def Iperf_UL_common(self, lock, UE_IPAddress, device_id, idx, ue_num, statusQueue):
		ipnumbers = UE_IPAddress.split('.')
		if (len(ipnumbers) == 4):
			ipnumbers[3] = '1'
		EPC_Iperf_UE_IPAddress = ipnumbers[0] + '.' + ipnumbers[1] + '.' + ipnumbers[2] + '.' + ipnumbers[3]

		# Launch iperf server on EPC side
		self.open(self.EPCIPAddress, self.EPCUserName, self.EPCPassword)
		self.command('cd ' + self.EPCSourceCodePath + '/scripts', '\$', 5)
		self.command('rm -f iperf_server_' + SSH.testCase_id + '_' + device_id + '.log', '\$', 5)
		port = 5001 + idx
		self.command('echo $USER; nohup iperf -u -s -i 1 -p ' + str(port) + ' > iperf_server_' + SSH.testCase_id + '_' + device_id + '.log &', self.EPCUserName, 5)
		time.sleep(0.5)
		self.close()

		# Launch iperf client on UE
		self.open(self.ADBIPAddress, self.ADBUserName, self.ADBPassword)
		self.command('cd ' + self.EPCSourceCodePath + '/scripts', '\$', 5)
		iperf_time = self.Iperf_ComputeTime()
		time.sleep(0.5)

		modified_options = self.Iperf_ComputeModifiedBW(idx, ue_num)
		modified_options = modified_options.replace('-R','')
		time.sleep(0.5)

		self.command('rm -f iperf_' + SSH.testCase_id + '_' + device_id + '.log', '\$', 5)
		self.command('stdbuf -o0 adb -s ' + device_id + ' shell "/data/local/tmp/iperf -c ' + EPC_Iperf_UE_IPAddress + ' ' + modified_options + ' -p ' + str(port) + '" 2>&1 | stdbuf -o0 tee -a iperf_' + SSH.testCase_id + '_' + device_id + '.log', '\$', int(iperf_time)*5.0)
		clientStatus = self.Iperf_analyzeV2Output(lock, UE_IPAddress, device_id, statusQueue, modified_options)

		# Launch iperf server on EPC side
		self.open(self.EPCIPAddress, self.EPCUserName, self.EPCPassword)
		self.command('killall --signal SIGKILL iperf', self.EPCUserName, 5)
		self.close()
		if (clientStatus == -1):
			time.sleep(1)
			if (os.path.isfile('iperf_server_' + SSH.testCase_id + '_' + device_id + '.log')):
				os.remove('iperf_server_' + SSH.testCase_id + '_' + device_id + '.log')
			self.copy(self.EPCIPAddress, self.EPCUserName, self.EPCPassword, self.EPCSourceCodePath + '/scripts/iperf_server_' + SSH.testCase_id + '_' + device_id + '.log', '.')
			self.Iperf_analyzeV2Server(lock, UE_IPAddress, device_id, statusQueue, modified_options)

	def Iperf_common(self, lock, UE_IPAddress, device_id, idx, ue_num, statusQueue):
		try:
			# Single-UE profile -- iperf only on one UE
			if SSH.iperf_profile == 'single-ue' and idx != 0:
				return
			useIperf3 = False
			self.open(self.ADBIPAddress, self.ADBUserName, self.ADBPassword)
			# if by chance ADB server and EPC are on the same remote host, at least log collection will take care of it
			self.command('if [ ! -d ' + self.EPCSourceCodePath + '/scripts ]; then mkdir -p ' + self.EPCSourceCodePath + '/scripts ; fi', '\$', 5)
			self.command('cd ' + self.EPCSourceCodePath + '/scripts', '\$', 5)
			# Checking if iperf / iperf3 are installed
			self.command('adb -s ' + device_id + ' shell "ls /data/local/tmp"', '\$', 5)
			result = re.search('iperf3', str(self.ssh.before))
			if result is None:
				result = re.search('iperf', str(self.ssh.before))
				if result is None:
					message = 'Neither iperf nor iperf3 installed on UE!'
					lock.acquire()
					logging.debug('\u001B[1;37;41m ' + message + ' \u001B[0m')
					statusQueue.put(-1)
					statusQueue.put(device_id)
					statusQueue.put(UE_IPAddress)
					statusQueue.put(message)
					lock.release()
					return
					#sys.exit(1)
			else:
				useIperf3 = True
			# in case of iperf, UL has its own function
			if (not useIperf3):
				result = re.search('-R', str(self.iperf_args))
				if result is not None:
					self.close()
					self.Iperf_UL_common(lock, UE_IPAddress, device_id, idx, ue_num, statusQueue)
					return

			if (useIperf3):
				self.command('stdbuf -o0 adb -s ' + device_id + ' shell /data/local/tmp/iperf3 -s &', '\$', 5)
			else:
				self.command('rm -f iperf_server_' + SSH.testCase_id + '_' + device_id + '.log', '\$', 5)
				self.command('echo $USER; nohup adb -s ' + device_id + ' shell "/data/local/tmp/iperf -u -s -i 1" > iperf_server_' + SSH.testCase_id + '_' + device_id + '.log &', self.ADBUserName, 5)
			time.sleep(0.5)
			self.close()

			self.open(self.EPCIPAddress, self.EPCUserName, self.EPCPassword)
			self.command('cd ' + self.EPCSourceCodePath + '/scripts', '\$', 5)
			iperf_time = self.Iperf_ComputeTime()
			time.sleep(0.5)

			modified_options = self.Iperf_ComputeModifiedBW(idx, ue_num)
			time.sleep(0.5)

			self.command('rm -f iperf_' + SSH.testCase_id + '_' + device_id + '.log', '\$', 5)
			if (useIperf3):
				self.command('stdbuf -o0 iperf3 -c ' + UE_IPAddress + ' ' + modified_options + ' 2>&1 | stdbuf -o0 tee -a iperf_' + SSH.testCase_id + '_' + device_id + '.log', '\$', int(iperf_time)*5.0)

				clientStatus = 0
				self.Iperf_analyzeV3Output(lock, UE_IPAddress, device_id, statusQueue)
			else:
				self.command('stdbuf -o0 iperf -c ' + UE_IPAddress + ' ' + modified_options + ' 2>&1 | stdbuf -o0 tee -a iperf_' + SSH.testCase_id + '_' + device_id + '.log', '\$', int(iperf_time)*5.0)

				clientStatus = self.Iperf_analyzeV2Output(lock, UE_IPAddress, device_id, statusQueue, modified_options)
			self.close()

			self.open(self.ADBIPAddress, self.ADBUserName, self.ADBPassword)
			self.command('stdbuf -o0 adb -s ' + device_id + ' shell ps | grep --color=never iperf | grep -v grep', '\$', 5)
			result = re.search('shell +(?P<pid>\d+)', str(self.ssh.before))
			if result is not None:
				pid_iperf = result.group('pid')
				self.command('stdbuf -o0 adb -s ' + device_id + ' shell kill -KILL ' + pid_iperf, '\$', 5)
			self.close()
			if (clientStatus == -1):
				time.sleep(1)
				if (os.path.isfile('iperf_server_' + SSH.testCase_id + '_' + device_id + '.log')):
					os.remove('iperf_server_' + SSH.testCase_id + '_' + device_id + '.log')
				self.copy(self.ADBIPAddress, self.ADBUserName, self.ADBPassword, self.EPCSourceCodePath + '/scripts/iperf_server_' + SSH.testCase_id + '_' + device_id + '.log', '.')
				self.Iperf_analyzeV2Server(lock, UE_IPAddress, device_id, statusQueue, modified_options)
		except:
			os.kill(os.getppid(),signal.SIGUSR1)

	def Iperf(self):
		if self.EPCIPAddress == '' or self.EPCUserName == '' or self.EPCPassword == '' or self.EPCSourceCodePath == '' or self.ADBIPAddress == '' or self.ADBUserName == '' or self.ADBPassword == '':
			Usage()
			sys.exit('Insufficient Parameter')
		initialize_eNB_flag = False
		self.CheckProcessExist(initialize_eNB_flag)
		self.GetAllUEIPAddresses()
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
			self.CreateHtmlTestRow(self.iperf_args, 'KO', len(self.UEDevices))
			sys.exit(1)
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
				html_cell = "<pre>UE (" + device_id + ")\nIP Address  : " + ip_addr + "\n" + message + "</pre>"
				html_queue.put(html_cell)
			if (iperf_noperf and iperf_status):
				self.CreateHtmlTestRowQueue(self.iperf_args, 'PERF NOT MET', len(self.UEDevices), html_queue)
			elif (iperf_status):
				self.CreateHtmlTestRowQueue(self.iperf_args, 'OK', len(self.UEDevices), html_queue)
			else:
				self.CreateHtmlTestRowQueue(self.iperf_args, 'KO', len(self.UEDevices), html_queue)
				self.CreateHtmlFooter()
				sys.exit(1)

	def CheckProcessExist(self, initialize_eNB_flag):
		multi_jobs = []
		p = Process(target = SSH.CheckHSSProcess, args = ())
		p.daemon = True
		p.start()
		multi_jobs.append(p)
		p = Process(target = SSH.CheckMMEProcess, args = ())
		p.daemon = True
		p.start()
		multi_jobs.append(p)
		p = Process(target = SSH.CheckSPGWProcess, args = ())
		p.daemon = True
		p.start()
		multi_jobs.append(p)
		if initialize_eNB_flag == False:
			p = Process(target = SSH.CheckeNBProcess, args = ())
			p.daemon = True
			p.start()
			multi_jobs.append(p)
		for job in multi_jobs:
			job.join()

	def CheckeNBProcess(self):
		try:
			self.open(self.eNBIPAddress, self.eNBUserName, self.eNBPassword)
			self.command('stdbuf -o0 ps -aux | grep -v grep | grep --color=never lte-softmodem', '\$', 5)
			result = re.search('lte-softmodem', str(self.ssh.before))
			if result is None:
				logging.debug('\u001B[1;37;41m eNB Process Not Found! \u001B[0m')
				sys.exit(1)
			self.close()
		except:
			os.kill(os.getppid(),signal.SIGUSR1)

	def CheckHSSProcess(self):
		try:
			self.open(self.EPCIPAddress, self.EPCUserName, self.EPCPassword)
			self.command('stdbuf -o0 ps -aux | grep -v grep | grep --color=never hss', '\$', 5)
			if re.match('OAI', self.EPCType, re.IGNORECASE):
				result = re.search('\/bin\/bash .\/run_', str(self.ssh.before))
			else:
				result = re.search('hss_sim s6as diam_hss', str(self.ssh.before))
			if result is None:
				logging.debug('\u001B[1;37;41m HSS Process Not Found! \u001B[0m')
				sys.exit(1)
			self.close()
		except:
			os.kill(os.getppid(),signal.SIGUSR1)

	def CheckMMEProcess(self):
		try:
			self.open(self.EPCIPAddress, self.EPCUserName, self.EPCPassword)
			self.command('stdbuf -o0 ps -aux | grep -v grep | grep --color=never mme', '\$', 5)
			if re.match('OAI', self.EPCType, re.IGNORECASE):
				result = re.search('\/bin\/bash .\/run_', str(self.ssh.before))
			else:
				result = re.search('mme', str(self.ssh.before))
			if result is None:
				logging.debug('\u001B[1;37;41m MME Process Not Found! \u001B[0m')
				sys.exit(1)
			self.close()
		except:
			os.kill(os.getppid(),signal.SIGUSR1)

	def CheckSPGWProcess(self):
		try:
			self.open(self.EPCIPAddress, self.EPCUserName, self.EPCPassword)
			if re.match('OAI', self.EPCType, re.IGNORECASE):
				self.command('stdbuf -o0 ps -aux | grep -v grep | grep --color=never spgw', '\$', 5)
				result = re.search('\/bin\/bash .\/run_', str(self.ssh.before))
			else:
				self.command('stdbuf -o0 ps -aux | grep -v grep | grep --color=never xGw', '\$', 5)
				result = re.search('xGw', str(self.ssh.before))
			if result is None:
				logging.debug('\u001B[1;37;41m SPGW Process Not Found! \u001B[0m')
				sys.exit(1)
			self.close()
		except:
			os.kill(os.getppid(),signal.SIGUSR1)

	def TerminateeNB(self):
		self.open(self.eNBIPAddress, self.eNBUserName, self.eNBPassword)
		self.command('cd ' + self.eNBSourceCodePath + '/cmake_targets', '\$', 5)
		self.command('echo ' + self.eNBPassword + ' | sudo -S daemon --name=enb_daemon --stop', '\$', 5)
		self.command('rm -f my-lte-softmodem-run.sh', '\$', 5)
		self.command('echo ' + self.eNBPassword + ' | sudo -S killall --signal SIGINT lte-softmodem || true', '\$', 5)
		time.sleep(5)
		self.command('stdbuf -o0  ps -aux | grep -v grep | grep lte-softmodem', '\$', 5)
		result = re.search('lte-softmodem', str(self.ssh.before))
		if result is not None:
			self.command('echo ' + self.eNBPassword + ' | sudo -S killall --signal SIGKILL lte-softmodem || true', '\$', 5)
		self.close()
		self.CreateHtmlTestRow('N/A', 'OK', 0)

	def TerminateHSS(self):
		self.open(self.EPCIPAddress, self.EPCUserName, self.EPCPassword)
		if re.match('OAI', self.EPCType, re.IGNORECASE):
			self.command('echo ' + self.EPCPassword + ' | sudo -S killall --signal SIGINT run_hss oai_hss || true', '\$', 5)
			time.sleep(2)
			self.command('stdbuf -o0  ps -aux | grep -v grep | grep hss', '\$', 5)
			result = re.search('\/bin\/bash .\/run_', str(self.ssh.before))
			if result is not None:
				self.command('echo ' + self.EPCPassword + ' | sudo -S killall --signal SIGKILL run_hss oai_hss || true', '\$', 5)
		else:
			self.command('cd ' + self.EPCSourceCodePath, '\$', 5)
			self.command('cd scripts', '\$', 5)
			self.command('rm -f ./kill_hss.sh', '\$', 5)
			self.command('echo ' + self.EPCPassword + ' | sudo -S daemon --name=simulated_hss --stop', '\$', 5)
			time.sleep(2)
			self.command('ps -aux | egrep --color=never "hss_sim|simulated_hss" | grep -v grep | awk \'BEGIN{n=0}{pidId[n]=$2;n=n+1}END{print "kill -9 " pidId[0] " " pidId[1]}\' > ./kill_hss.sh', '\$', 5)
			self.command('chmod 755 ./kill_hss.sh', '\$', 5)
			self.command('echo ' + self.EPCPassword + ' | sudo -S ./kill_hss.sh', '\$', 5)
			self.command('rm ./kill_hss.sh', '\$', 5)
		self.close()
		self.CreateHtmlTestRow('N/A', 'OK', 0)

	def TerminateMME(self):
		self.open(self.EPCIPAddress, self.EPCUserName, self.EPCPassword)
		if re.match('OAI', self.EPCType, re.IGNORECASE):
			self.command('echo ' + self.EPCPassword + ' | sudo -S killall --signal SIGINT run_mme mme || true', '\$', 5)
			time.sleep(2)
			self.command('stdbuf -o0 ps -aux | grep -v grep | grep mme', '\$', 5)
			result = re.search('\/bin\/bash .\/run_', str(self.ssh.before))
			if result is not None:
				self.command('echo ' + self.EPCPassword + ' | sudo -S killall --signal SIGKILL run_mme mme || true', '\$', 5)
		else:
			self.command('cd /opt/ltebox/tools', '\$', 5)
			self.command('echo ' + self.EPCPassword + ' | sudo -S ./stop_mme', '\$', 5)
		self.close()
		self.CreateHtmlTestRow('N/A', 'OK', 0)

	def TerminateSPGW(self):
		self.open(self.EPCIPAddress, self.EPCUserName, self.EPCPassword)
		if re.match('OAI', self.EPCType, re.IGNORECASE):
			self.command('echo ' + self.EPCPassword + ' | sudo -S killall --signal SIGINT run_spgw spgw || true', '\$', 5)
			time.sleep(2)
			self.command('stdbuf -o0 ps -aux | grep -v grep | grep spgw', '\$', 5)
			result = re.search('\/bin\/bash .\/run_', str(self.ssh.before))
			if result is not None:
				self.command('echo ' + self.EPCPassword + ' | sudo -S killall --signal SIGKILL run_spgw spgw || true', '\$', 5)
		else:
			self.command('cd /opt/ltebox/tools', '\$', 5)
			self.command('echo ' + self.EPCPassword + ' | sudo -S ./stop_xGw', '\$', 5)
		self.close()
		self.CreateHtmlTestRow('N/A', 'OK', 0)

	def TerminateUE_common(self, device_id):
		try:
			self.open(self.ADBIPAddress, self.ADBUserName, self.ADBPassword)
			self.command('stdbuf -o0 adb -s ' + device_id + ' shell /data/local/tmp/off', '\$', 60)
			logging.debug('\u001B[1mUE (' + device_id + ') Detach Completed\u001B[0m')

			self.command('stdbuf -o0 adb -s ' + device_id + ' shell ps | grep --color=never iperf | grep -v grep', '\$', 5)
			result = re.search('shell +(?P<pid>\d+)', str(self.ssh.before))
			if result is not None:
				pid_iperf = result.group('pid')
				self.command('stdbuf -o0 adb -s ' + device_id + ' shell kill -KILL ' + pid_iperf, '\$', 5)
			self.close()
		except:
			os.kill(os.getppid(),signal.SIGUSR1)

	def TerminateUE(self):
		terminate_ue_flag = True
		SSH.GetAllUEDevices(terminate_ue_flag)
		multi_jobs = []
		for device_id in self.UEDevices:
			p = Process(target= SSH.TerminateUE_common, args = (device_id,))
			p.daemon = True
			p.start()
			multi_jobs.append(p)
		for job in multi_jobs:
			job.join()
		self.CreateHtmlTestRow('N/A', 'OK', 0)

	def LogCollectBuild(self):
		self.open(self.eNBIPAddress, self.eNBUserName, self.eNBPassword)
		self.command('cd ' + self.eNBSourceCodePath, '\$', 5)
		self.command('cd cmake_targets', '\$', 5)
		self.command('rm -f build.log.zip', '\$', 5)
		self.command('zip build.log.zip build_log_*/*', '\$', 60)
		self.command('echo ' + self.eNBPassword + ' | sudo -S rm -rf build_log_*', '\$', 5)
		self.close()

	def LogCollecteNB(self):
		self.open(self.eNBIPAddress, self.eNBUserName, self.eNBPassword)
		self.command('cd ' + self.eNBSourceCodePath, '\$', 5)
		self.command('cd cmake_targets', '\$', 5)
		self.command('rm -f enb.log.zip', '\$', 5)
		self.command('zip enb.log.zip enb*.log', '\$', 60)
		self.command('echo ' + self.eNBPassword + ' | sudo -S rm enb*.log', '\$', 5)
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
		self.command('cd ' + self.EPCSourceCodePath, '\$', 5)
		self.command('cd scripts', '\$', 5)
		self.command('rm -f hss.log.zip', '\$', 5)
		if re.match('OAI', self.EPCType, re.IGNORECASE):
			self.command('zip hss.log.zip hss*.log', '\$', 60)
			self.command('rm hss*.log', '\$', 5)
		else:
			self.command('cp /opt/hss_sim0609/hss.log .', '\$', 60)
			self.command('zip hss.log.zip hss.log', '\$', 60)
		self.close()

	def LogCollectMME(self):
		self.open(self.EPCIPAddress, self.EPCUserName, self.EPCPassword)
		self.command('cd ' + self.EPCSourceCodePath, '\$', 5)
		self.command('cd scripts', '\$', 5)
		self.command('rm -f mme.log.zip', '\$', 5)
		if re.match('OAI', self.EPCType, re.IGNORECASE):
			self.command('zip mme.log.zip mme*.log', '\$', 60)
			self.command('rm mme*.log', '\$', 5)
		else:
			self.command('cp /opt/ltebox/var/log/*Log.0 .', '\$', 5)
			self.command('zip mme.log.zip mmeLog.0 s1apcLog.0 s1apsLog.0 s11cLog.0 libLog.0 s1apCodecLog.0', '\$', 60)
		self.close()

	def LogCollectSPGW(self):
		self.open(self.EPCIPAddress, self.EPCUserName, self.EPCPassword)
		self.command('cd ' + self.EPCSourceCodePath, '\$', 5)
		self.command('cd scripts', '\$', 5)
		self.command('rm -f spgw.log.zip', '\$', 5)
		if re.match('OAI', self.EPCType, re.IGNORECASE):
			self.command('zip spgw.log.zip spgw*.log', '\$', 60)
			self.command('rm spgw*.log', '\$', 5)
		else:
			self.command('cp /opt/ltebox/var/log/xGwLog.0 .', '\$', 5)
			self.command('zip spgw.log.zip xGwLog.0', '\$', 60)
		self.close()
#-----------------------------------------------------------
# HTML Reporting....
#-----------------------------------------------------------
	def CreateHtmlHeader(self):
		if (not self.htmlHeaderCreated):
			self.htmlFile = open('test_results.html', 'w')
			self.htmlFile.write('<!DOCTYPE html>\n')
			self.htmlFile.write('<html class="no-js" lang="en-US">\n')
			self.htmlFile.write('<head>\n')
			self.htmlFile.write('  <title>Test Results for TEMPLATE_JOB_NAME job build #TEMPLATE_BUILD_ID</title>\n')
			self.htmlFile.write('  <base href = "http://www.openairinterface.org/" />\n')
			self.htmlFile.write('</head>\n')
			self.htmlFile.write('<body>\n')
			self.htmlFile.write('  <table style="border-collapse: collapse; border: none;">\n')
			self.htmlFile.write('    <tr style="border-collapse: collapse; border: none;">\n')
			self.htmlFile.write('      <td style="border-collapse: collapse; border: none;">\n')
			self.htmlFile.write('        <a href="http://www.openairinterface.org/">\n')
			self.htmlFile.write('           <img src="/wp-content/uploads/2016/03/cropped-oai_final_logo2.png" alt="" border="none" height=50 width=150>\n')
			self.htmlFile.write('           </img>\n')
			self.htmlFile.write('        </a>\n')
			self.htmlFile.write('      </td>\n')
			self.htmlFile.write('      <td style="border-collapse: collapse; border: none; vertical-align: center;">\n')
			self.htmlFile.write('        <b><font size = "6">Job Summary -- Job: TEMPLATE_JOB_NAME -- Build-ID: TEMPLATE_BUILD_ID</font></b>\n')
			self.htmlFile.write('      </td>\n')
			self.htmlFile.write('    </tr>\n')
			self.htmlFile.write('  </table>\n')
			self.htmlFile.write('  <br>\n')
			self.htmlFile.write('  <table border = "1">\n')
			self.htmlFile.write('     <tr>\n')
			self.htmlFile.write('       <td bgcolor = "lightcyan" >GIT Repository</td>\n')
			self.htmlFile.write('       <td>' + SSH.eNBRepository + '</td>\n')
			self.htmlFile.write('     </tr>\n')
			self.htmlFile.write('     <tr>\n')
			self.htmlFile.write('       <td bgcolor = "lightcyan" >Job Trigger</td>\n')
			if (SSH.eNB_AllowMerge):
				self.htmlFile.write('       <td>Merge-Request</td>\n')
			else:
				self.htmlFile.write('       <td>Push to Branch</td>\n')
			self.htmlFile.write('     </tr>\n')
			self.htmlFile.write('     <tr>\n')
			if (SSH.eNB_AllowMerge):
				self.htmlFile.write('       <td bgcolor = "lightcyan" >Source Branch</td>\n')
			else:
				self.htmlFile.write('       <td bgcolor = "lightcyan" >Branch</td>\n')
			self.htmlFile.write('       <td>' + SSH.eNBBranch + '</td>\n')
			self.htmlFile.write('     </tr>\n')
			self.htmlFile.write('     <tr>\n')
			if (SSH.eNB_AllowMerge):
				self.htmlFile.write('       <td bgcolor = "lightcyan" >Source Commit ID</td>\n')
			else:
				self.htmlFile.write('       <td bgcolor = "lightcyan" >Commit ID</td>\n')
			self.htmlFile.write('       <td>' + SSH.eNBCommitID + '</td>\n')
			self.htmlFile.write('     </tr>\n')
			if (SSH.eNB_AllowMerge):
				self.htmlFile.write('     <tr>\n')
				self.htmlFile.write('       <td bgcolor = "lightcyan" >Target Branch</td>\n')
				self.htmlFile.write('       <td>develop</td>\n')
				self.htmlFile.write('     </tr>\n')
			self.htmlFile.write('  </table>\n')

			terminate_ue_flag = True
			SSH.GetAllUEDevices(terminate_ue_flag)
			self.htmlUEConnected = len(self.UEDevices)
			self.htmlFile.write('<h2>' + str(self.htmlUEConnected) + ' UE(s) is(are) connected to ADB bench server</h2>\n')

			self.htmlFile.write('  <br>\n')
			self.htmlFile.write('  <h2>Test Summary for ' + SSH.testXMLfile + '</h2>\n')
			self.htmlFile.write('  <table border = "1">\n')
			self.htmlFile.write('      <tr bgcolor = "#33CCFF" >\n')
			self.htmlFile.write('        <th>Test Id</th>\n')
			self.htmlFile.write('        <th>Test Desc</th>\n')
			self.htmlFile.write('        <th>Test Options</th>\n')
			self.htmlFile.write('        <th>Test Status</th>\n')
			i = 0
			while (i < self.htmlUEConnected):
				self.htmlFile.write('        <th>UE' + str(i) + ' Status</th>\n')
				i += 1
			self.htmlFile.write('      </tr>\n')
		self.htmlHeaderCreated = True

	def CreateHtmlFooter(self):
		if ((not self.htmlFooterCreated) and (self.htmlHeaderCreated)):
			self.htmlFile.write('  </table>\n')
			self.htmlFile.write('</body>\n')
			self.htmlFile.write('</html>\n')
			self.htmlFile.close()
		self.htmlFooterCreated = False

	def CreateHtmlTestRow(self, options, status, ue_status):
		if ((not self.htmlFooterCreated) and (self.htmlHeaderCreated)):
			self.htmlFile.write('      <tr>\n')
			self.htmlFile.write('        <td bgcolor = "lightcyan" >' + SSH.testCase_id  + '</td>\n')
			self.htmlFile.write('        <td>' + SSH.desc  + '</td>\n')
			self.htmlFile.write('        <td>' + str(options)  + '</td>\n')
			if (str(status) == 'OK'):
				self.htmlFile.write('        <td bgcolor = "lightgreen" >' + str(status)  + '</td>\n')
			elif (str(status) == 'KO'):
				self.htmlFile.write('        <td bgcolor = "lightcoral" >' + str(status)  + '</td>\n')
			else:
				self.htmlFile.write('        <td bgcolor = "orange" >' + str(status)  + '</td>\n')
			i = 0
			while (i < self.htmlUEConnected):
				if (i < ue_status):
					self.htmlFile.write('        <td>-</td>\n')
				else:
					self.htmlFile.write('        <td>-</td>\n')
				i += 1
			self.htmlFile.write('      </tr>\n')

	def CreateHtmlTestRowQueue(self, options, status, ue_status, ue_queue):
		if ((not self.htmlFooterCreated) and (self.htmlHeaderCreated)):
			addOrangeBK = False
			self.htmlFile.write('      <tr>\n')
			self.htmlFile.write('        <td bgcolor = "lightcyan" >' + SSH.testCase_id  + '</td>\n')
			self.htmlFile.write('        <td>' + SSH.desc  + '</td>\n')
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
							self.htmlFile.write('        <td bgcolor = "orange" >' + str(ue_queue.get()) + '</td>\n')
						else:
							self.htmlFile.write('        <td>' + str(ue_queue.get()) + '</td>\n')
					else:
						self.htmlFile.write('        <td>-</td>\n')
				else:
					self.htmlFile.write('        <td>-</td>\n')
				i += 1
			self.htmlFile.write('      </tr>\n')

#-----------------------------------------------------------
# Usage()
#-----------------------------------------------------------
def Usage():
	print('------------------------------------------------------------')
	print('main.py Ver:' + Version)
	print('------------------------------------------------------------')
	print('Usage: python main.py [options]')
	print('  --help  Show this help.')
	print('  --mode=[Mode]')
	print('      TesteNB')
	print('      TerminateeNB, TerminateUE, TerminateHSS, TerminateMME, TerminateSPGW')
	print('      LogCollectBuild, LogCollecteNB, LogCollectHSS, LogCollectMME, LogCollectSPGW, LogCollectPing, LogCollectIperf')
	print('  --eNBIPAddress=[eNB\'s IP Address]')
	print('  --eNBRepository=[eNB\'s Repository URL]')
	print('  --eNBBranch=[eNB\'s Branch Name]')
	print('  --eNBCommitID=[eNB\'s Commit Number]')
	print('  --eNBUserName=[eNB\'s Login User Name]')
	print('  --eNBPassword=[eNB\'s Login Password]')
	print('  --eNBSourceCodePath=[eNB\'s Source Code Path]')
	print('  --EPCIPAddress=[EPC\'s IP Address]')
	print('  --EPCUserName=[EPC\'s Login User Name]')
	print('  --EPCPassword=[EPC\'s Login Password]')
	print('  --EPCSourceCodePath=[EPC\'s Source Code Path]')
	print('  --EPCType=[EPC\'s Type: OAI or ltebox]')
	print('  --ADBIPAddress=[ADB\'s IP Address]')
	print('  --ADBUserName=[ADB\'s Login User Name]')
	print('  --ADBPassword=[ADB\'s Login Password]')
	print('  --XMLTestFile=[XML Test File to be run]')
	print('------------------------------------------------------------')

#-----------------------------------------------------------
# ShowTestID()
#-----------------------------------------------------------
def ShowTestID():
	logging.debug('\u001B[1m----------------------------------------\u001B[0m')
	logging.debug('\u001B[1mTest ID:' + SSH.testCase_id + '\u001B[0m')
	logging.debug('\u001B[1m' + SSH.desc + '\u001B[0m')
	logging.debug('\u001B[1m----------------------------------------\u001B[0m')

def CheckClassValidity(action,id):
	if action != 'Build_eNB' and action != 'Initialize_eNB' and action != 'Terminate_eNB' and action != 'Initialize_UE' and action != 'Terminate_UE' and action != 'Attach_UE' and action != 'Detach_UE' and action != 'Ping' and action != 'Iperf' and action != 'Reboot_UE' and action != 'Initialize_HSS' and action != 'Terminate_HSS' and action != 'Initialize_MME' and action != 'Terminate_MME' and action != 'Initialize_SPGW' and action != 'Terminate_SPGW':
		logging.debug('ERROR: test-case ' + id + ' has wrong class ' + action)
		return False
	return True

def GetParametersFromXML(action):
	if action == 'Build_eNB':
		SSH.Build_eNB_args = test.findtext('Build_eNB_args')

	if action == 'Initialize_eNB':
		SSH.Initialize_eNB_args = test.findtext('Initialize_eNB_args')

	if action == 'Ping':
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

while len(argvs) > 1:
	myArgv = argvs.pop(1)	# 0th is this file's name
	if re.match('^\-\-help$', myArgv, re.IGNORECASE):
		Usage()
		sys.exit(0)
	elif re.match('^\-\-mode=(.+)$', myArgv, re.IGNORECASE):
		matchReg = re.match('^\-\-mode=(.+)$', myArgv, re.IGNORECASE)
		mode = matchReg.group(1)
	elif re.match('^\-\-eNBIPAddress=(.+)$', myArgv, re.IGNORECASE):
		matchReg = re.match('^\-\-eNBIPAddress=(.+)$', myArgv, re.IGNORECASE)
		SSH.eNBIPAddress = matchReg.group(1)
	elif re.match('^\-\-eNBRepository=(.+)$', myArgv, re.IGNORECASE):
		matchReg = re.match('^\-\-eNBRepository=(.+)$', myArgv, re.IGNORECASE)
		SSH.eNBRepository = matchReg.group(1)
	elif re.match('^\-\-eNB_AllowMerge=(.+)$', myArgv, re.IGNORECASE):
		matchReg = re.match('^\-\-eNB_AllowMerge=(.+)$', myArgv, re.IGNORECASE)
		doMerge = matchReg.group(1)
		if ((doMerge == 'true') or (doMerge == 'True')):
			SSH.eNB_AllowMerge = True
	elif re.match('^\-\-eNBBranch=(.+)$', myArgv, re.IGNORECASE):
		matchReg = re.match('^\-\-eNBBranch=(.+)$', myArgv, re.IGNORECASE)
		SSH.eNBBranch = matchReg.group(1)
	elif re.match('^\-\-eNBCommitID=(.*)$', myArgv, re.IGNORECASE):
		matchReg = re.match('^\-\-eNBCommitID=(.*)$', myArgv, re.IGNORECASE)
		SSH.eNBCommitID = matchReg.group(1)
	elif re.match('^\-\-eNBUserName=(.+)$', myArgv, re.IGNORECASE):
		matchReg = re.match('^\-\-eNBUserName=(.+)$', myArgv, re.IGNORECASE)
		SSH.eNBUserName = matchReg.group(1)
	elif re.match('^\-\-eNBPassword=(.+)$', myArgv, re.IGNORECASE):
		matchReg = re.match('^\-\-eNBPassword=(.+)$', myArgv, re.IGNORECASE)
		SSH.eNBPassword = matchReg.group(1)
	elif re.match('^\-\-eNBSourceCodePath=(.+)$', myArgv, re.IGNORECASE):
		matchReg = re.match('^\-\-eNBSourceCodePath=(.+)$', myArgv, re.IGNORECASE)
		SSH.eNBSourceCodePath = matchReg.group(1)
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
		if re.match('OAI', matchReg.group(1), re.IGNORECASE) or re.match('ltebox', matchReg.group(1), re.IGNORECASE):
			SSH.EPCType = matchReg.group(1)
		else:
			sys.exit('Invalid EPC Type: ' + matchReg.group(1) + ' -- (should be OAI or ltebox)')
	elif re.match('^\-\-ADBIPAddress=(.+)$', myArgv, re.IGNORECASE):
		matchReg = re.match('^\-\-ADBIPAddress=(.+)$', myArgv, re.IGNORECASE)
		SSH.ADBIPAddress = matchReg.group(1)
	elif re.match('^\-\-ADBUserName=(.+)$', myArgv, re.IGNORECASE):
		matchReg = re.match('^\-\-ADBUserName=(.+)$', myArgv, re.IGNORECASE)
		SSH.ADBUserName = matchReg.group(1)
	elif re.match('^\-\-ADBPassword=(.+)$', myArgv, re.IGNORECASE):
		matchReg = re.match('^\-\-ADBPassword=(.+)$', myArgv, re.IGNORECASE)
		SSH.ADBPassword = matchReg.group(1)
	elif re.match('^\-\-XMLTestFile=(.+)$', myArgv, re.IGNORECASE):
		matchReg = re.match('^\-\-XMLTestFile=(.+)$', myArgv, re.IGNORECASE)
		SSH.testXMLfile = matchReg.group(1)
	else:
		Usage()
		sys.exit('Invalid Parameter: ' + myArgv)

if re.match('^TerminateeNB$', mode, re.IGNORECASE):
	if SSH.eNBIPAddress == '' or SSH.eNBUserName == '' or SSH.eNBPassword == '':
		Usage()
		sys.exit('Insufficient Parameter')
	SSH.TerminateeNB()
elif re.match('^TerminateUE$', mode, re.IGNORECASE):
	if SSH.ADBIPAddress == '' or SSH.ADBUserName == '' or SSH.ADBPassword == '':
		Usage()
		sys.exit('Insufficient Parameter')
	signal.signal(signal.SIGUSR1, receive_signal)
	SSH.TerminateUE()
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
	if SSH.eNBIPAddress == '' or SSH.eNBUserName == '' or SSH.eNBPassword == '' or SSH.eNBSourceCodePath == '':
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
elif re.match('^TesteNB$', mode, re.IGNORECASE):
	if SSH.eNBIPAddress == '' or SSH.eNBRepository == '' or SSH.eNBBranch == '' or SSH.eNBUserName == '' or SSH.eNBPassword == '' or SSH.eNBSourceCodePath == '' or SSH.EPCIPAddress == '' or SSH.EPCUserName == '' or SSH.EPCPassword == '' or SSH.EPCType == '' or SSH.EPCSourceCodePath == '' or SSH.ADBIPAddress == '' or SSH.ADBUserName == '' or SSH.ADBPassword == '':
		Usage()
		sys.exit('Insufficient Parameter')

	SSH.CreateHtmlHeader()

	#read test_case_list.xml file
        # if no parameters for XML file, use default value
	if SSH.testXMLfile == '':
		xml_test_file = sys.path[0] + "/test_case_list.xml"
	else:
		xml_test_file = sys.path[0] + "/" + SSH.testXMLfile

	xmlTree = ET.parse(xml_test_file)
	xmlRoot = xmlTree.getroot()

	exclusion_tests=xmlRoot.findtext('TestCaseExclusionList',default='')
	requested_tests=xmlRoot.findtext('TestCaseRequestedList',default='')
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

	#get the list of tests to be done
	todo_tests=[]
	for test in requested_tests:
		if    (test_in_list(test, exclusion_tests)):
			logging.debug('INFO: test will be skipped: ' + test)
		else:
			#logging.debug('INFO: test will be run: ' + test)
			todo_tests.append(test)

	signal.signal(signal.SIGUSR1, receive_signal)

	for test_case_id in todo_tests:
		for test in all_tests:
			id = test.get('id')
			if test_case_id != id:
				continue
			SSH.testCase_id = id
			SSH.desc = test.findtext('desc')
			action = test.findtext('class')
			if (CheckClassValidity(action, id) == False):
				continue
			ShowTestID()
			GetParametersFromXML(action)
			if action == 'Initialize_UE' or action == 'Attach_UE' or action == 'Detach_UE' or action == 'Ping' or action == 'Iperf' or action == 'Reboot_UE':
				terminate_ue_flag = False
				SSH.GetAllUEDevices(terminate_ue_flag)
			if action == 'Build_eNB':
				SSH.BuildeNB()
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
			else:
				sys.exit('Invalid action')

	SSH.CreateHtmlFooter()
else:
	Usage()
	sys.exit('Invalid mode')
sys.exit(0)
