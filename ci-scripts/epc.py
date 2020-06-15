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
import signal

from multiprocessing import Process, Lock, SimpleQueue

#-----------------------------------------------------------
# OAI Testing modules
#-----------------------------------------------------------
import sshconnection as SSH 
import helpreadme as HELP
import constants as CONST
import html

#-----------------------------------------------------------
# Class Declaration
#-----------------------------------------------------------
class EPCManagement():

	def __init__(self):
		
		self.IPAddress = ''
		self.UserName = ''
		self.Password = ''
		self.SourceCodePath = ''
		self.Type = ''
		self.PcapFileName = ''
		self.htmlObj = None
		self.testCase_id = ''
		self.MmeIPAddress = ''
		self.containerPrefix = 'prod'

#-----------------------------------------------------------
# Setter and Getters on Public Members
#-----------------------------------------------------------

	def SetIPAddress(self, ipaddress):
		self.IPAddress = ipaddress
	def GetIPAddress(self):
		return self.IPAddress
	def SetUserName(self, username):
		self.UserName = username
	def GetUserName(self):
		return self.UserName
	def SetPassword(self, password):
		self.Password = password
	def GetPassword(self):
		return self.Password
	def SetSourceCodePath(self, sourcecodepath):
		self.SourceCodePath = sourcecodepath
	def GetSourceCodePath(self):
		return self.SourceCodePath
	def SetType(self, kind):
		self.Type = kind
	def GetType(self):
		return self.Type
	def SetHtmlObj(self, obj):
		self.htmlObj = obj
	def SetTestCase_id(self, idx):
		self.testCase_id = idx
	def GetMmeIPAddress(self):
		return self.MmeIPAddress
	def SetContainerPrefix(self, prefix):
		self.containerPrefix = prefix

#-----------------------------------------------------------
# EPC management functions
#-----------------------------------------------------------

	def InitializeHSS(self):
		if self.IPAddress == '' or self.UserName == '' or self.Password == '' or self.SourceCodePath == '' or self.Type == '':
			HELP.GenericHelp(CONST.Version)
			HELP.EPCSrvHelp(self.IPAddress, self.UserName, self.Password, self.SourceCodePath, self.Type)
			sys.exit('Insufficient EPC Parameters')
		mySSH = SSH.SSHConnection() 
		mySSH.open(self.IPAddress, self.UserName, self.Password)
		if re.match('OAI-Rel14-Docker', self.Type, re.IGNORECASE):
			logging.debug('Using the OAI EPC Release 14 Cassandra-based HSS in Docker')
			mySSH.command('if [ -d ' + self.SourceCodePath + '/scripts ]; then echo ' + self.Password + ' | sudo -S rm -Rf ' + self.SourceCodePath + '/scripts ; fi', '\$', 5)
			mySSH.command('mkdir -p ' + self.SourceCodePath + '/scripts', '\$', 5)
			mySSH.command('docker exec -d ' + self.containerPrefix + '-oai-hss /bin/bash -c "nohup tshark -i eth0 -i eth1 -w /tmp/hss_check_run.pcap 2>&1 > /dev/null"', '\$', 5)
			time.sleep(5)
			mySSH.command('docker exec -d ' + self.containerPrefix + '-oai-hss /bin/bash -c "nohup ./bin/oai_hss -j ./etc/hss_rel14.json --reloadkey true > hss_check_run.log 2>&1"', '\$', 5)
		elif re.match('OAI-Rel14-CUPS', self.Type, re.IGNORECASE):
			logging.debug('Using the OAI EPC Release 14 Cassandra-based HSS')
			mySSH.command('cd ' + self.SourceCodePath + '/scripts', '\$', 5)
			logging.debug('\u001B[1m Launching tshark on all interfaces \u001B[0m')
			self.PcapFileName = 'epc_' + self.testCase_id + '.pcap'
			mySSH.command('echo ' + self.Password + ' | sudo -S rm -f ' + self.PcapFileName, '\$', 5)
			mySSH.command('echo $USER; nohup sudo tshark -f "tcp port not 22 and port not 53" -i any -w ' + self.SourceCodePath + '/scripts/' + self.PcapFileName + ' > /tmp/tshark.log 2>&1 &', self.UserName, 5)
			mySSH.command('echo ' + self.Password + ' | sudo -S mkdir -p logs', '\$', 5)
			mySSH.command('echo ' + self.Password + ' | sudo -S rm -f hss_' + self.testCase_id + '.log logs/hss*.*', '\$', 5)
			mySSH.command('echo "oai_hss -j /usr/local/etc/oai/hss_rel14.json" > ./my-hss.sh', '\$', 5)
			mySSH.command('chmod 755 ./my-hss.sh', '\$', 5)
			mySSH.command('sudo daemon --unsafe --name=hss_daemon --chdir=' + self.SourceCodePath + '/scripts -o ' + self.SourceCodePath + '/scripts/hss_' + self.testCase_id + '.log ./my-hss.sh', '\$', 5)
		elif re.match('OAI', self.Type, re.IGNORECASE):
			logging.debug('Using the OAI EPC HSS')
			mySSH.command('cd ' + self.SourceCodePath, '\$', 5)
			mySSH.command('source oaienv', '\$', 5)
			mySSH.command('cd scripts', '\$', 5)
			mySSH.command('echo ' + self.Password + ' | sudo -S ./run_hss 2>&1 | stdbuf -o0 awk \'{ print strftime("[%Y/%m/%d %H:%M:%S] ",systime()) $0 }\' | stdbuf -o0 tee -a hss_' + self.testCase_id + '.log &', 'Core state: 2 -> 3', 35)
		elif re.match('ltebox', self.Type, re.IGNORECASE):
			logging.debug('Using the ltebox simulated HSS')
			mySSH.command('if [ -d ' + self.SourceCodePath + '/scripts ]; then echo ' + self.Password + ' | sudo -S rm -Rf ' + self.SourceCodePath + '/scripts ; fi', '\$', 5)
			mySSH.command('mkdir -p ' + self.SourceCodePath + '/scripts', '\$', 5)
			mySSH.command('cd /opt/hss_sim0609', '\$', 5)
			mySSH.command('echo ' + self.Password + ' | sudo -S rm -f hss.log', '\$', 5)
			mySSH.command('echo ' + self.Password + ' | sudo -S echo "Starting sudo session" && sudo su -c "screen -dm -S simulated_hss ./starthss"', '\$', 5)
		else:
			logging.error('This option should not occur!')
		mySSH.close()
		if self.htmlObj is not None:
			self.htmlObj.CreateHtmlTestRow(self.Type, 'OK', CONST.ALL_PROCESSES_OK)

	def InitializeMME(self):
		if self.IPAddress == '' or self.UserName == '' or self.Password == '' or self.SourceCodePath == '' or self.Type == '':
			HELP.GenericHelp(CONST.Version)
			HELP.EPCSrvHelp(self.IPAddress, self.UserName, self.Password, self.SourceCodePath, self.Type)
			sys.exit('Insufficient EPC Parameters')
		mySSH = SSH.SSHConnection() 
		mySSH.open(self.IPAddress, self.UserName, self.Password)
		if re.match('OAI-Rel14-Docker', self.Type, re.IGNORECASE):
			logging.debug('Using the OAI EPC Release 14 MME in Docker')
			mySSH.command('docker exec -d ' + self.containerPrefix + '-oai-mme /bin/bash -c "nohup tshark -i eth0 -i lo:s10 -w /tmp/mme_check_run.pcap 2>&1 > /dev/null"', '\$', 5)
			time.sleep(5)
			mySSH.command('docker exec -d ' + self.containerPrefix + '-oai-mme /bin/bash -c "nohup ./bin/oai_mme -c ./etc/mme.conf > mme_check_run.log 2>&1"', '\$', 5)
		elif re.match('OAI-Rel14-CUPS', self.Type, re.IGNORECASE):
			logging.debug('Using the OAI EPC Release 14 MME')
			mySSH.command('cd ' + self.SourceCodePath + '/scripts', '\$', 5)
			mySSH.command('echo ' + self.Password + ' | sudo -S rm -f mme_' + self.testCase_id + '.log', '\$', 5)
			mySSH.command('echo "./run_mme --config-file /usr/local/etc/oai/mme.conf --set-virt-if" > ./my-mme.sh', '\$', 5)
			mySSH.command('chmod 755 ./my-mme.sh', '\$', 5)
			mySSH.command('sudo daemon --unsafe --name=mme_daemon --chdir=' + self.SourceCodePath + '/scripts -o ' + self.SourceCodePath + '/scripts/mme_' + self.testCase_id + '.log ./my-mme.sh', '\$', 5)
		elif re.match('OAI', self.Type, re.IGNORECASE):
			mySSH.command('cd ' + self.SourceCodePath, '\$', 5)
			mySSH.command('source oaienv', '\$', 5)
			mySSH.command('cd scripts', '\$', 5)
			mySSH.command('stdbuf -o0 hostname', '\$', 5)
			result = re.search('hostname\\\\r\\\\n(?P<host_name>[a-zA-Z0-9\-\_]+)\\\\r\\\\n', mySSH.getBefore())
			if result is None:
				logging.debug('\u001B[1;37;41m Hostname Not Found! \u001B[0m')
				sys.exit(1)
			host_name = result.group('host_name')
			mySSH.command('echo ' + self.Password + ' | sudo -S ./run_mme 2>&1 | stdbuf -o0 tee -a mme_' + self.testCase_id + '.log &', 'MME app initialization complete', 100)
		elif re.match('ltebox', self.Type, re.IGNORECASE):
			mySSH.command('cd /opt/ltebox/tools', '\$', 5)
			mySSH.command('echo ' + self.Password + ' | sudo -S ./start_mme', '\$', 5)
		else:
			logging.error('This option should not occur!')
		mySSH.close()
		if self.htmlObj is not None:
			self.htmlObj.CreateHtmlTestRow(self.Type, 'OK', CONST.ALL_PROCESSES_OK)

	def SetMmeIPAddress(self):
		# Not an error if we don't need an EPC
		if self.IPAddress == '' or self.UserName == '' or self.Password == '' or self.SourceCodePath == '' or self.Type == '':
			return
		if self.IPAddress == 'none':
			return
		# Only in case of Docker containers, MME IP address is not the EPC HOST IP address
		if re.match('OAI-Rel14-Docker', self.Type, re.IGNORECASE):
			mySSH = SSH.SSHConnection() 
			mySSH.open(self.IPAddress, self.UserName, self.Password)
			mySSH.command('docker inspect --format="MME_IP_ADDR = {{range .NetworkSettings.Networks}}{{.IPAddress}}{{end}}" ' + self.containerPrefix + '-oai-mme', '\$', 5)
			result = re.search('MME_IP_ADDR = (?P<mme_ip_addr>[0-9\.]+)', mySSH.getBefore())
			if result is not None:
				self.MmeIPAddress = result.group('mme_ip_addr')
				logging.debug('MME IP Address is ' + self.MmeIPAddress)
			mySSH.close()
		else:
			self.MmeIPAddress = self.IPAddress

	def InitializeSPGW(self):
		if self.IPAddress == '' or self.UserName == '' or self.Password == '' or self.SourceCodePath == '' or self.Type == '':
			HELP.GenericHelp(CONST.Version)
			HELP.EPCSrvHelp(self.IPAddress, self.UserName, self.Password, self.SourceCodePath, self.Type)
			sys.exit('Insufficient EPC Parameters')
		mySSH = SSH.SSHConnection() 
		mySSH.open(self.IPAddress, self.UserName, self.Password)
		if re.match('OAI-Rel14-Docker', self.Type, re.IGNORECASE):
			logging.debug('Using the OAI EPC Release 14 SPGW-CUPS in Docker')
			mySSH.command('docker exec -d ' + self.containerPrefix + '-oai-spgwc /bin/bash -c "nohup tshark -i eth0 -i lo:p5c -i lo:s5c -w /tmp/spgwc_check_run.pcap 2>&1 > /dev/null"', '\$', 5)
			mySSH.command('docker exec -d ' + self.containerPrefix + '-oai-spgwu-tiny /bin/bash -c "nohup tshark -i eth0 -w /tmp/spgwu_check_run.pcap 2>&1 > /dev/null"', '\$', 5)
			time.sleep(5)
			mySSH.command('docker exec -d ' + self.containerPrefix + '-oai-spgwc /bin/bash -c "nohup ./bin/oai_spgwc -o -c ./etc/spgw_c.conf > spgwc_check_run.log 2>&1"', '\$', 5)
			time.sleep(5)
			mySSH.command('docker exec -d ' + self.containerPrefix + '-oai-spgwu-tiny /bin/bash -c "nohup ./bin/oai_spgwu -o -c ./etc/spgw_u.conf > spgwu_check_run.log 2>&1"', '\$', 5)
		elif re.match('OAI-Rel14-CUPS', self.Type, re.IGNORECASE):
			logging.debug('Using the OAI EPC Release 14 SPGW-CUPS')
			mySSH.command('cd ' + self.SourceCodePath + '/scripts', '\$', 5)
			mySSH.command('echo ' + self.Password + ' | sudo -S rm -f spgwc_' + self.testCase_id + '.log spgwu_' + self.testCase_id + '.log', '\$', 5)
			mySSH.command('echo "spgwc -c /usr/local/etc/oai/spgw_c.conf" > ./my-spgwc.sh', '\$', 5)
			mySSH.command('chmod 755 ./my-spgwc.sh', '\$', 5)
			mySSH.command('sudo daemon --unsafe --name=spgwc_daemon --chdir=' + self.SourceCodePath + '/scripts -o ' + self.SourceCodePath + '/scripts/spgwc_' + self.testCase_id + '.log ./my-spgwc.sh', '\$', 5)
			time.sleep(5)
			mySSH.command('echo "spgwu -c /usr/local/etc/oai/spgw_u.conf" > ./my-spgwu.sh', '\$', 5)
			mySSH.command('chmod 755 ./my-spgwu.sh', '\$', 5)
			mySSH.command('sudo daemon --unsafe --name=spgwu_daemon --chdir=' + self.SourceCodePath + '/scripts -o ' + self.SourceCodePath + '/scripts/spgwu_' + self.testCase_id + '.log ./my-spgwu.sh', '\$', 5)
		elif re.match('OAI', self.Type, re.IGNORECASE):
			mySSH.command('cd ' + self.SourceCodePath, '\$', 5)
			mySSH.command('source oaienv', '\$', 5)
			mySSH.command('cd scripts', '\$', 5)
			mySSH.command('echo ' + self.Password + ' | sudo -S ./run_spgw 2>&1 | stdbuf -o0 tee -a spgw_' + self.testCase_id + '.log &', 'Initializing SPGW-APP task interface: DONE', 30)
		elif re.match('ltebox', self.Type, re.IGNORECASE):
			mySSH.command('cd /opt/ltebox/tools', '\$', 5)
			mySSH.command('echo ' + self.Password + ' | sudo -S ./start_xGw', '\$', 5)
		else:
			logging.error('This option should not occur!')
		mySSH.close()
		if self.htmlObj is not None:
			self.htmlObj.CreateHtmlTestRow(self.Type, 'OK', CONST.ALL_PROCESSES_OK)

	def CheckHSSProcess(self, status_queue):
		try:
			mySSH = SSH.SSHConnection() 
			mySSH.open(self.IPAddress, self.UserName, self.Password)
			if re.match('OAI-Rel14-Docker', self.Type, re.IGNORECASE):
				mySSH.command('docker exec -it ' + self.containerPrefix + '-oai-hss /bin/bash -c "ps aux | grep oai_hss"', '\$', 5)
			else:
				mySSH.command('stdbuf -o0 ps -aux | grep --color=never hss | grep -v grep', '\$', 5)
			if re.match('OAI-Rel14-CUPS', self.Type, re.IGNORECASE) or re.match('OAI-Rel14-Docker', self.Type, re.IGNORECASE):
				result = re.search('oai_hss -j', mySSH.getBefore())
			elif re.match('OAI', self.Type, re.IGNORECASE):
				result = re.search('\/bin\/bash .\/run_', mySSH.getBefore())
			elif re.match('ltebox', self.Type, re.IGNORECASE):
				result = re.search('hss_sim s6as diam_hss', mySSH.getBefore())
			else:
				logging.error('This should not happen!')
			if result is None:
				logging.debug('\u001B[1;37;41m HSS Process Not Found! \u001B[0m')
				status_queue.put(CONST.HSS_PROCESS_FAILED)
			else:
				status_queue.put(CONST.HSS_PROCESS_OK)
			mySSH.close()
		except:
			os.kill(os.getppid(),signal.SIGUSR1)

	def CheckMMEProcess(self, status_queue):
		try:
			mySSH = SSH.SSHConnection() 
			mySSH.open(self.IPAddress, self.UserName, self.Password)
			if re.match('OAI-Rel14-Docker', self.Type, re.IGNORECASE):
				mySSH.command('docker exec -it ' + self.containerPrefix + '-oai-mme /bin/bash -c "ps aux | grep oai_mme"', '\$', 5)
			else:
				mySSH.command('stdbuf -o0 ps -aux | grep --color=never mme | grep -v grep', '\$', 5)
			if re.match('OAI-Rel14-Docker', self.Type, re.IGNORECASE):
				result = re.search('oai_mme -c ', mySSH.getBefore())
			elif re.match('OAI-Rel14-CUPS', self.Type, re.IGNORECASE):
				result = re.search('mme -c', mySSH.getBefore())
			elif re.match('OAI', self.Type, re.IGNORECASE):
				result = re.search('\/bin\/bash .\/run_', mySSH.getBefore())
			elif re.match('ltebox', self.Type, re.IGNORECASE):
				result = re.search('mme', mySSH.getBefore())
			else:
				logging.error('This should not happen!')
			if result is None:
				logging.debug('\u001B[1;37;41m MME Process Not Found! \u001B[0m')
				status_queue.put(CONST.MME_PROCESS_FAILED)
			else:
				status_queue.put(CONST.MME_PROCESS_OK)
			mySSH.close()
		except:
			os.kill(os.getppid(),signal.SIGUSR1)

	def CheckSPGWProcess(self, status_queue):
		try:
			mySSH = SSH.SSHConnection() 
			mySSH.open(self.IPAddress, self.UserName, self.Password)
			if re.match('OAI-Rel14-Docker', self.Type, re.IGNORECASE):
				mySSH.command('docker exec -it ' + self.containerPrefix + '-oai-spgwc /bin/bash -c "ps aux | grep oai_spgwc"', '\$', 5)
				result = re.search('oai_spgwc -o -c ', mySSH.getBefore())
				if result is not None:
					mySSH.command('docker exec -it ' + self.containerPrefix + '-oai-spgwu-tiny /bin/bash -c "ps aux | grep oai_spgwu"', '\$', 5)
					result = re.search('oai_spgwu -o -c ', mySSH.getBefore())
			elif re.match('OAI-Rel14-CUPS', self.Type, re.IGNORECASE):
				mySSH.command('stdbuf -o0 ps -aux | grep --color=never spgw | grep -v grep', '\$', 5)
				result = re.search('spgwu -c ', mySSH.getBefore())
			elif re.match('OAI', self.Type, re.IGNORECASE):
				mySSH.command('stdbuf -o0 ps -aux | grep --color=never spgw | grep -v grep', '\$', 5)
				result = re.search('\/bin\/bash .\/run_', mySSH.getBefore())
			elif re.match('ltebox', self.Type, re.IGNORECASE):
				mySSH.command('stdbuf -o0 ps -aux | grep --color=never xGw | grep -v grep', '\$', 5)
				result = re.search('xGw', mySSH.getBefore())
			else:
				logging.error('This should not happen!')
			if result is None:
				logging.debug('\u001B[1;37;41m SPGW Process Not Found! \u001B[0m')
				status_queue.put(CONST.SPGW_PROCESS_FAILED)
			else:
				status_queue.put(CONST.SPGW_PROCESS_OK)
			mySSH.close()
		except:
			os.kill(os.getppid(),signal.SIGUSR1)

	def TerminateHSS(self):
		mySSH = SSH.SSHConnection() 
		mySSH.open(self.IPAddress, self.UserName, self.Password)
		if re.match('OAI-Rel14-Docker', self.Type, re.IGNORECASE):
			mySSH.command('docker exec -it ' + self.containerPrefix + '-oai-hss /bin/bash -c "killall --signal SIGINT oai_hss tshark"', '\$', 5)
			time.sleep(2)
			mySSH.command('docker exec -it ' + self.containerPrefix + '-oai-hss /bin/bash -c "ps aux | grep oai_hss"', '\$', 5)
			result = re.search('oai_hss -j ', mySSH.getBefore())
			if result is not None:
				mySSH.command('docker exec -it ' + self.containerPrefix + '-oai-hss /bin/bash -c "killall --signal SIGKILL oai_hss"', '\$', 5)
		elif re.match('OAI-Rel14-CUPS', self.Type, re.IGNORECASE):
			mySSH.command('echo ' + self.Password + ' | sudo -S killall --signal SIGINT oai_hss || true', '\$', 5)
			time.sleep(2)
			mySSH.command('stdbuf -o0  ps -aux | grep hss | grep -v grep', '\$', 5)
			result = re.search('oai_hss -j', mySSH.getBefore())
			if result is not None:
				mySSH.command('echo ' + self.Password + ' | sudo -S killall --signal SIGKILL oai_hss || true', '\$', 5)
			mySSH.command('rm -f ' + self.SourceCodePath + '/scripts/my-hss.sh', '\$', 5)
		elif re.match('OAI', self.Type, re.IGNORECASE):
			mySSH.command('echo ' + self.Password + ' | sudo -S killall --signal SIGINT run_hss oai_hss || true', '\$', 5)
			time.sleep(2)
			mySSH.command('stdbuf -o0  ps -aux | grep hss | grep -v grep', '\$', 5)
			result = re.search('\/bin\/bash .\/run_', mySSH.getBefore())
			if result is not None:
				mySSH.command('echo ' + self.Password + ' | sudo -S killall --signal SIGKILL run_hss oai_hss || true', '\$', 5)
		elif re.match('ltebox', self.Type, re.IGNORECASE):
			mySSH.command('cd ' + self.SourceCodePath, '\$', 5)
			mySSH.command('cd scripts', '\$', 5)
			time.sleep(1)
			mySSH.command('echo ' + self.Password + ' | sudo -S killall --signal SIGKILL hss_sim', '\$', 5)
		else:
			logging.error('This should not happen!')
		mySSH.close()
		if self.htmlObj is not None:
			self.htmlObj.CreateHtmlTestRow('N/A', 'OK', CONST.ALL_PROCESSES_OK)

	def TerminateMME(self):
		mySSH = SSH.SSHConnection() 
		mySSH.open(self.IPAddress, self.UserName, self.Password)
		if re.match('OAI-Rel14-Docker', self.Type, re.IGNORECASE):
			mySSH.command('docker exec -it ' + self.containerPrefix + '-oai-mme /bin/bash -c "killall --signal SIGINT oai_mme tshark"', '\$', 5)
			time.sleep(2)
			mySSH.command('docker exec -it ' + self.containerPrefix + '-oai-mme /bin/bash -c "ps aux | grep oai_mme"', '\$', 5)
			result = re.search('oai_mme -c ', mySSH.getBefore())
			if result is not None:
				mySSH.command('docker exec -it ' + self.containerPrefix + '-oai-mme /bin/bash -c "killall --signal SIGKILL oai_mme"', '\$', 5)
		elif re.match('OAI', self.Type, re.IGNORECASE) or re.match('OAI-Rel14-CUPS', self.Type, re.IGNORECASE):
			mySSH.command('echo ' + self.Password + ' | sudo -S killall --signal SIGINT run_mme mme || true', '\$', 5)
			time.sleep(2)
			mySSH.command('stdbuf -o0 ps -aux | grep mme | grep -v grep', '\$', 5)
			result = re.search('mme -c', mySSH.getBefore())
			if result is not None:
				mySSH.command('echo ' + self.Password + ' | sudo -S killall --signal SIGKILL run_mme mme || true', '\$', 5)
			mySSH.command('rm -f ' + self.SourceCodePath + '/scripts/my-mme.sh', '\$', 5)
		elif re.match('ltebox', self.Type, re.IGNORECASE):
			mySSH.command('cd /opt/ltebox/tools', '\$', 5)
			mySSH.command('echo ' + self.Password + ' | sudo -S ./stop_mme', '\$', 5)
		else:
			logging.error('This should not happen!')
		mySSH.close()
		if self.htmlObj is not None:
			self.htmlObj.CreateHtmlTestRow('N/A', 'OK', CONST.ALL_PROCESSES_OK)

	def TerminateSPGW(self):
		mySSH = SSH.SSHConnection() 
		mySSH.open(self.IPAddress, self.UserName, self.Password)
		if re.match('OAI-Rel14-Docker', self.Type, re.IGNORECASE):
			mySSH.command('docker exec -it ' + self.containerPrefix + '-oai-spgwc /bin/bash -c "killall --signal SIGINT oai_spgwc tshark"', '\$', 5)
			mySSH.command('docker exec -it ' + self.containerPrefix + '-oai-spgwu-tiny /bin/bash -c "killall --signal SIGINT oai_spgwu tshark"', '\$', 5)
			time.sleep(2)
			mySSH.command('docker exec -it ' + self.containerPrefix + '-oai-spgwc /bin/bash -c "ps aux | grep oai_spgwc"', '\$', 5)
			result = re.search('oai_spgwc -o -c ', mySSH.getBefore())
			if result is not None:
				mySSH.command('docker exec -it ' + self.containerPrefix + '-oai-spgwc /bin/bash -c "killall --signal SIGKILL oai_spgwc"', '\$', 5)
			mySSH.command('docker exec -it ' + self.containerPrefix + '-oai-spgwu-tiny /bin/bash -c "ps aux | grep oai_spgwu"', '\$', 5)
			result = re.search('oai_spgwu -o -c ', mySSH.getBefore())
			if result is not None:
				mySSH.command('docker exec -it ' + self.containerPrefix + '-oai-spgwu-tiny /bin/bash -c "killall --signal SIGKILL oai_spgwu"', '\$', 5)
		elif re.match('OAI-Rel14-CUPS', self.Type, re.IGNORECASE):
			mySSH.command('echo ' + self.Password + ' | sudo -S killall --signal SIGINT spgwc spgwu || true', '\$', 5)
			time.sleep(2)
			mySSH.command('stdbuf -o0 ps -aux | grep spgw | grep -v grep', '\$', 5)
			result = re.search('spgwc -c |spgwu -c ', mySSH.getBefore())
			if result is not None:
				mySSH.command('echo ' + self.Password + ' | sudo -S killall --signal SIGKILL spgwc spgwu || true', '\$', 5)
			mySSH.command('rm -f ' + self.SourceCodePath + '/scripts/my-spgw*.sh', '\$', 5)
			mySSH.command('stdbuf -o0 ps -aux | grep tshark | grep -v grep', '\$', 5)
			result = re.search('-w ', mySSH.getBefore())
			if result is not None:
				mySSH.command('echo ' + self.Password + ' | sudo -S killall --signal SIGINT tshark || true', '\$', 5)
				mySSH.command('echo ' + self.Password + ' | sudo -S chmod 666 ' + self.SourceCodePath + '/scripts/*.pcap', '\$', 5)
		elif re.match('OAI', self.Type, re.IGNORECASE):
			mySSH.command('echo ' + self.Password + ' | sudo -S killall --signal SIGINT run_spgw spgw || true', '\$', 5)
			time.sleep(2)
			mySSH.command('stdbuf -o0 ps -aux | grep spgw | grep -v grep', '\$', 5)
			result = re.search('\/bin\/bash .\/run_', mySSH.getBefore())
			if result is not None:
				mySSH.command('echo ' + self.Password + ' | sudo -S killall --signal SIGKILL run_spgw spgw || true', '\$', 5)
		elif re.match('ltebox', self.Type, re.IGNORECASE):
			mySSH.command('cd /opt/ltebox/tools', '\$', 5)
			mySSH.command('echo ' + self.Password + ' | sudo -S ./stop_xGw', '\$', 5)
		else:
			logging.error('This should not happen!')
		mySSH.close()
		if self.htmlObj is not None:
			self.htmlObj.CreateHtmlTestRow('N/A', 'OK', CONST.ALL_PROCESSES_OK)

	def LogCollectHSS(self):
		mySSH = SSH.SSHConnection() 
		mySSH.open(self.IPAddress, self.UserName, self.Password)
		mySSH.command('cd ' + self.SourceCodePath + '/scripts', '\$', 5)
		mySSH.command('rm -f hss.log.zip', '\$', 5)
		if re.match('OAI-Rel14-Docker', self.Type, re.IGNORECASE):
			mySSH.command('docker cp ' + self.containerPrefix + '-oai-hss:/openair-hss/hss_check_run.log .', '\$', 60)
			mySSH.command('docker cp ' + self.containerPrefix + '-oai-hss:/tmp/hss_check_run.pcap .', '\$', 60)
			mySSH.command('zip hss.log.zip hss_check_run.*', '\$', 60)
		elif re.match('OAI', self.Type, re.IGNORECASE) or re.match('OAI-Rel14-CUPS', self.Type, re.IGNORECASE):
			mySSH.command('zip hss.log.zip hss*.log', '\$', 60)
			mySSH.command('echo ' + self.Password + ' | sudo -S rm hss*.log', '\$', 5)
			if re.match('OAI-Rel14-CUPS', self.Type, re.IGNORECASE):
				mySSH.command('zip hss.log.zip logs/hss*.* *.pcap', '\$', 60)
				mySSH.command('echo ' + self.Password + ' | sudo -S rm -f logs/hss*.* *.pcap', '\$', 5)
		elif re.match('ltebox', self.Type, re.IGNORECASE):
			mySSH.command('cp /opt/hss_sim0609/hss.log .', '\$', 60)
			mySSH.command('zip hss.log.zip hss.log', '\$', 60)
		else:
			logging.error('This option should not occur!')
		mySSH.close()

	def LogCollectMME(self):
		mySSH = SSH.SSHConnection() 
		mySSH.open(self.IPAddress, self.UserName, self.Password)
		mySSH.command('cd ' + self.SourceCodePath + '/scripts', '\$', 5)
		mySSH.command('rm -f mme.log.zip', '\$', 5)
		if re.match('OAI-Rel14-Docker', self.Type, re.IGNORECASE):
			mySSH.command('docker cp ' + self.containerPrefix + '-oai-mme:/openair-mme/mme_check_run.log .', '\$', 60)
			mySSH.command('docker cp ' + self.containerPrefix + '-oai-mme:/tmp/mme_check_run.pcap .', '\$', 60)
			mySSH.command('zip mme.log.zip mme_check_run.*', '\$', 60)
		elif re.match('OAI', self.Type, re.IGNORECASE) or re.match('OAI-Rel14-CUPS', self.Type, re.IGNORECASE):
			mySSH.command('zip mme.log.zip mme*.log', '\$', 60)
			mySSH.command('echo ' + self.Password + ' | sudo -S rm mme*.log', '\$', 5)
		elif re.match('ltebox', self.Type, re.IGNORECASE):
			mySSH.command('cp /opt/ltebox/var/log/*Log.0 .', '\$', 5)
			mySSH.command('zip mme.log.zip mmeLog.0 s1apcLog.0 s1apsLog.0 s11cLog.0 libLog.0 s1apCodecLog.0', '\$', 60)
		else:
			logging.error('This option should not occur!')
		mySSH.close()

	def LogCollectSPGW(self):
		mySSH = SSH.SSHConnection() 
		mySSH.open(self.IPAddress, self.UserName, self.Password)
		mySSH.command('cd ' + self.SourceCodePath + '/scripts', '\$', 5)
		mySSH.command('rm -f spgw.log.zip', '\$', 5)
		if re.match('OAI-Rel14-Docker', self.Type, re.IGNORECASE):
			mySSH.command('docker cp ' + self.containerPrefix + '-oai-spgwc:/openair-spgwc/spgwc_check_run.log .', '\$', 60)
			mySSH.command('docker cp ' + self.containerPrefix + '-oai-spgwu-tiny:/openair-spgwu-tiny/spgwu_check_run.log .', '\$', 60)
			mySSH.command('docker cp ' + self.containerPrefix + '-oai-spgwc:/tmp/spgwc_check_run.pcap .', '\$', 60)
			mySSH.command('docker cp ' + self.containerPrefix + '-oai-spgwu-tiny:/tmp/spgwu_check_run.pcap .', '\$', 60)
			mySSH.command('zip spgw.log.zip spgw*_check_run.*', '\$', 60)
		elif re.match('OAI', self.Type, re.IGNORECASE) or re.match('OAI-Rel14-CUPS', self.Type, re.IGNORECASE):
			mySSH.command('zip spgw.log.zip spgw*.log', '\$', 60)
			mySSH.command('echo ' + self.Password + ' | sudo -S rm spgw*.log', '\$', 5)
		elif re.match('ltebox', self.Type, re.IGNORECASE):
			mySSH.command('cp /opt/ltebox/var/log/xGwLog.0 .', '\$', 5)
			mySSH.command('zip spgw.log.zip xGwLog.0', '\$', 60)
		else:
			logging.error('This option should not occur!')
		mySSH.close()

