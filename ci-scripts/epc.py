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
		self.testCase_id = ''
		self.MmeIPAddress = ''
		self.AmfIPAddress = ''
		self.containerPrefix = 'prod'
		self.mmeConfFile = 'mme.conf'
		self.yamlPath = ''


#-----------------------------------------------------------
# EPC management functions
#-----------------------------------------------------------

	def InitializeHSS(self, HTML):
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
		HTML.CreateHtmlTestRow(self.Type, 'OK', CONST.ALL_PROCESSES_OK)

	def InitializeMME(self, HTML):
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
			mySSH.command('docker exec -d ' + self.containerPrefix + '-oai-mme /bin/bash -c "nohup ./bin/oai_mme -c ./etc/' + self.mmeConfFile + ' > mme_check_run.log 2>&1"', '\$', 5)
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
		HTML.CreateHtmlTestRow(self.Type, 'OK', CONST.ALL_PROCESSES_OK)

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

	def InitializeSPGW(self, HTML):
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
		HTML.CreateHtmlTestRow(self.Type, 'OK', CONST.ALL_PROCESSES_OK)

	def Initialize5GCN(self, HTML):
		if self.IPAddress == '' or self.UserName == '' or self.Password == '' or self.SourceCodePath == '' or self.Type == '':
			HELP.GenericHelp(CONST.Version)
			HELP.EPCSrvHelp(self.IPAddress, self.UserName, self.Password, self.SourceCodePath, self.Type)
			sys.exit('Insufficient EPC Parameters')
		mySSH = SSH.SSHConnection()
		mySSH.open(self.IPAddress, self.UserName, self.Password)
		if re.match('ltebox', self.Type, re.IGNORECASE):
			logging.debug('Using the SABOX simulated HSS')
			mySSH.command('if [ -d ' + self.SourceCodePath + '/scripts ]; then echo ' + self.Password + ' | sudo -S rm -Rf ' + self.SourceCodePath + '/scripts ; fi', '\$', 5)
			mySSH.command('mkdir -p ' + self.SourceCodePath + '/scripts', '\$', 5)
			mySSH.command('cd /opt/hss_sim0609', '\$', 5)
			mySSH.command('echo ' + self.Password + ' | sudo -S rm -f hss.log', '\$', 5)
			mySSH.command('echo ' + self.Password + ' | sudo -S echo "Starting sudo session" && sudo su -c "screen -dm -S simulated_5g_hss ./start_5g_hss"', '\$', 5)
			logging.debug('Using the sabox')
			mySSH.command('cd /opt/ltebox/tools', '\$', 5)
			mySSH.command('echo ' + self.Password + ' | sudo -S ./start_sabox', '\$', 5)
		elif re.match('OAICN5G', self.Type, re.IGNORECASE):
			logging.debug('Starting OAI CN5G')
			mySSH.command('if [ -d ' + self.SourceCodePath + '/scripts ]; then echo ' + self.Password + ' | sudo -S rm -Rf ' + self.SourceCodePath + '/scripts ; fi', '\$', 5)
			mySSH.command('mkdir -p ' + self.SourceCodePath + '/scripts', '\$', 5)
			mySSH.command('cd /opt/oai-cn5g-fed/docker-compose', '\$', 5)
			mySSH.command('./core-network.sh start nrf spgwu', '\$', 60)
		else:
			logging.error('This option should not occur!')
		mySSH.close()
		HTML.CreateHtmlTestRow(self.Type, 'OK', CONST.ALL_PROCESSES_OK)

	def SetAmfIPAddress(self):
		# Not an error if we don't need an 5GCN
		if self.IPAddress == '' or self.UserName == '' or self.Password == '' or self.SourceCodePath == '' or self.Type == '':
			return
		if self.IPAddress == 'none':
			return
		if re.match('ltebox', self.Type, re.IGNORECASE):
			self.MmeIPAddress = self.IPAddress
		elif re.match('OAICN5G', self.Type, re.IGNORECASE):
			mySSH = SSH.SSHConnection()
			mySSH.open(self.IPAddress, self.UserName, self.Password)
			response=mySSH.command3('docker container ls -f name=oai-amf', 10)
			if len(response)>1:
				response=mySSH.command3('docker inspect --format=\'{{range.NetworkSettings.Networks}}{{.IPAddress}}{{end}}\' oai-amf', 10)
				tmp = str(response[0],'utf-8')
				self.MmeIPAddress = tmp.rstrip()
				logging.debug('AMF IP Address ' + self.MmeIPAddress)
			mySSH.close()

	def CheckHSSProcess(self, status_queue):
		try:
			mySSH = SSH.SSHConnection()
			mySSH.open(self.IPAddress, self.UserName, self.Password)
			if re.match('OAI-Rel14-Docker', self.Type, re.IGNORECASE):
				mySSH.command('docker top ' + self.containerPrefix + '-oai-hss', '\$', 5)
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
				mySSH.command('docker top ' + self.containerPrefix + '-oai-mme', '\$', 5)
			else:
				mySSH.command('stdbuf -o0 ps -aux | grep --color=never mme | grep -v grep', '\$', 5)
			if re.match('OAI-Rel14-Docker', self.Type, re.IGNORECASE):
				result = re.search('oai_mme -c ', mySSH.getBefore())
			elif re.match('OAI-Rel14-CUPS', self.Type, re.IGNORECASE):
				result = re.search('mme -c', mySSH.getBefore())
			elif re.match('OAI', self.Type, re.IGNORECASE):
				result = re.search('\/bin\/bash .\/run_', mySSH.getBefore())
			elif re.match('ltebox', self.Type, re.IGNORECASE):
				result = re.search('mme|amf', mySSH.getBefore())
			else:
				logging.error('This should not happen!')
			if result is None:
				logging.debug('\u001B[1;37;41m MME|AMF Process Not Found! \u001B[0m')
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
				mySSH.command('docker top ' + self.containerPrefix + '-oai-spgwc', '\$', 5)
				result = re.search('oai_spgwc -', mySSH.getBefore())
				if result is not None:
					mySSH.command('docker top ' + self.containerPrefix + '-oai-spgwu-tiny', '\$', 5)
					result = re.search('oai_spgwu -', mySSH.getBefore())
			elif re.match('OAI-Rel14-CUPS', self.Type, re.IGNORECASE):
				mySSH.command('stdbuf -o0 ps -aux | grep --color=never spgw | grep -v grep', '\$', 5)
				result = re.search('spgwu -c ', mySSH.getBefore())
			elif re.match('OAI', self.Type, re.IGNORECASE):
				mySSH.command('stdbuf -o0 ps -aux | grep --color=never spgw | grep -v grep', '\$', 5)
				result = re.search('\/bin\/bash .\/run_', mySSH.getBefore())
			elif re.match('ltebox', self.Type, re.IGNORECASE):
				mySSH.command('stdbuf -o0 ps -aux | grep --color=never xGw | grep -v grep', '\$', 5)
				result = re.search('xGw|upf', mySSH.getBefore())
			else:
				logging.error('This should not happen!')
			if result is None:
				logging.debug('\u001B[1;37;41m SPGW|UPF Process Not Found! \u001B[0m')
				status_queue.put(CONST.SPGW_PROCESS_FAILED)
			else:
				status_queue.put(CONST.SPGW_PROCESS_OK)
			mySSH.close()
		except:
			os.kill(os.getppid(),signal.SIGUSR1)

	def TerminateHSS(self, HTML):
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
			mySSH.command('echo ' + self.Password + ' | sudo -S screen -S simulated_hss -X quit', '\$', 5)
		else:
			logging.error('This should not happen!')
		mySSH.close()
		HTML.CreateHtmlTestRow('N/A', 'OK', CONST.ALL_PROCESSES_OK)

	def TerminateMME(self, HTML):
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
		HTML.CreateHtmlTestRow('N/A', 'OK', CONST.ALL_PROCESSES_OK)

	def TerminateSPGW(self, HTML):
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
		HTML.CreateHtmlTestRow('N/A', 'OK', CONST.ALL_PROCESSES_OK)

	def Terminate5GCN(self, HTML):
		mySSH = SSH.SSHConnection()
		mySSH.open(self.IPAddress, self.UserName, self.Password)
		if re.match('ltebox', self.Type, re.IGNORECASE):
			logging.debug('Terminating SA BOX')
			mySSH.command('cd /opt/ltebox/tools', '\$', 5)
			mySSH.command('echo ' + self.Password + ' | sudo -S ./stop_sabox', '\$', 5)
			time.sleep(1)
			mySSH.command('cd ' + self.SourceCodePath, '\$', 5)
			mySSH.command('cd scripts', '\$', 5)
			time.sleep(1)
			mySSH.command('echo ' + self.Password + ' | sudo -S screen -S simulated_5g_hss -X quit', '\$', 5)
		elif re.match('OAICN5G', self.Type, re.IGNORECASE):
			self.LogCollectOAICN5G()
			logging.debug('Terminating OAI CN5G')
			mySSH.command('cd /opt/oai-cn5g-fed/docker-compose', '\$', 5)
			mySSH.command('./core-network.sh stop nrf spgwu', '\$', 60)
		else:
			logging.error('This should not happen!')
		mySSH.close()
		HTML.CreateHtmlTestRow('N/A', 'OK', CONST.ALL_PROCESSES_OK)

	def DeployEpc(self, HTML):
		logging.debug('Trying to deploy')
		if not re.match('OAI-Rel14-Docker', self.Type, re.IGNORECASE):
			HTML.CreateHtmlTestRow(self.Type, 'KO', CONST.INVALID_PARAMETER)
			HTML.CreateHtmlTabFooter(False)
			sys.exit('Deploy not possible with this EPC type: ' + self.Type)

		if self.IPAddress == '' or self.UserName == '' or self.Password == '' or self.SourceCodePath == '' or self.Type == '':
			HELP.GenericHelp(CONST.Version)
			HELP.EPCSrvHelp(self.IPAddress, self.UserName, self.Password, self.SourceCodePath, self.Type)
			sys.exit('Insufficient EPC Parameters')
		mySSH = SSH.SSHConnection()
		mySSH.open(self.IPAddress, self.UserName, self.Password)
		mySSH.command('docker-compose --version', '\$', 5)
		result = re.search('docker-compose version 1', mySSH.getBefore())
		if result is None:
			mySSH.close()
			HTML.CreateHtmlTestRow(self.Type, 'KO', CONST.INVALID_PARAMETER)
			HTML.CreateHtmlTabFooter(False)
			sys.exit('docker-compose not installed on ' + self.IPAddress)

		mySSH.command('if [ -d ' + self.SourceCodePath + '/scripts ]; then echo ' + self.Password + ' | sudo -S rm -Rf ' + self.SourceCodePath + '/scripts ; fi', '\$', 5)
		mySSH.command('if [ -d ' + self.SourceCodePath + '/logs ]; then echo ' + self.Password + ' | sudo -S rm -Rf ' + self.SourceCodePath + '/logs ; fi', '\$', 5)
		mySSH.command('mkdir -p ' + self.SourceCodePath + '/scripts ' + self.SourceCodePath + '/logs', '\$', 5)

		# deploying and configuring the cassandra database
		# container names and services are currently hard-coded.
		# they could be recovered by:
		# - docker-compose config --services
		# - docker-compose config | grep container_name
		mySSH.command('cd ' + self.SourceCodePath + '/scripts', '\$', 5)
		mySSH.copyout(self.IPAddress, self.UserName, self.Password, './' + self.yamlPath + '/docker-compose.yml', self.SourceCodePath + '/scripts')
		mySSH.command('wget --quiet --tries=3 --retry-connrefused https://raw.githubusercontent.com/OPENAIRINTERFACE/openair-hss/develop/src/hss_rel14/db/oai_db.cql', '\$', 30)
		mySSH.command('docker-compose down', '\$', 60)
		mySSH.command('docker-compose up -d db_init', '\$', 60)

		# databases take time...
		time.sleep(10)
		cnt = 0
		db_init_status = False
		while (cnt < 10):
			mySSH.command('docker logs prod-db-init', '\$', 5)
			result = re.search('OK', mySSH.getBefore())
			if result is not None:
				cnt = 10
				db_init_status = True
			else:
				time.sleep(5)
				cnt += 1
		mySSH.command('docker rm -f prod-db-init', '\$', 5)
		if not db_init_status:
			HTML.CreateHtmlTestRow(self.Type, 'KO', CONST.INVALID_PARAMETER)
			HTML.CreateHtmlTabFooter(False)
			sys.exit('Cassandra DB deployment/configuration went wrong!')

		# deploying EPC cNFs
		mySSH.command('docker-compose up -d oai_spgwu', '\$', 60)
		listOfContainers = 'prod-cassandra prod-oai-hss prod-oai-mme prod-oai-spgwc prod-oai-spgwu-tiny'
		expectedHealthyContainers = 5

		# Checking for additional services
		mySSH.command('docker-compose config', '\$', 5)
		configResponse = mySSH.getBefore()
		if configResponse.count('flexran_rtc') == 1:
			mySSH.command('docker-compose up -d flexran_rtc', '\$', 60)
			listOfContainers += ' prod-flexran-rtc'
			expectedHealthyContainers += 1
		if configResponse.count('trf_gen') == 1:
			mySSH.command('docker-compose up -d trf_gen', '\$', 60)
			listOfContainers += ' prod-trf-gen'
			expectedHealthyContainers += 1

		# Checking if all are healthy
		cnt = 0
		while (cnt < 3):
			mySSH.command('docker inspect --format=\'{{.State.Health.Status}}\' ' + listOfContainers, '\$', 10)
			unhealthyNb = mySSH.getBefore().count('unhealthy')
			healthyNb = mySSH.getBefore().count('healthy') - unhealthyNb
			startingNb = mySSH.getBefore().count('starting')
			if healthyNb == expectedHealthyContainers:
				cnt = 10
			else:
				time.sleep(10)
				cnt += 1
		logging.debug(' -- ' + str(healthyNb) + ' healthy container(s)')
		logging.debug(' -- ' + str(unhealthyNb) + ' unhealthy container(s)')
		logging.debug(' -- ' + str(startingNb) + ' still starting container(s)')
		if healthyNb == expectedHealthyContainers:
			mySSH.command('docker exec -d prod-oai-hss /bin/bash -c "nohup tshark -i any -f \'port 9042 or port 3868\' -w /tmp/hss_check_run.pcap 2>&1 > /dev/null"', '\$', 5)
			mySSH.command('docker exec -d prod-oai-mme /bin/bash -c "nohup tshark -i any -f \'port 3868 or port 2123 or port 36412\' -w /tmp/mme_check_run.pcap 2>&1 > /dev/null"', '\$', 10)
			mySSH.command('docker exec -d prod-oai-spgwc /bin/bash -c "nohup tshark -i any -f \'port 2123 or port 8805\' -w /tmp/spgwc_check_run.pcap 2>&1 > /dev/null"', '\$', 10)
			# on SPGW-U, not capturing on SGI to avoid huge file
			mySSH.command('docker exec -d prod-oai-spgwu-tiny /bin/bash -c "nohup tshark -i any -f \'port 8805\'  -w /tmp/spgwu_check_run.pcap 2>&1 > /dev/null"', '\$', 10)
			mySSH.close()
			logging.debug('Deployment OK')
			HTML.CreateHtmlTestRow(self.Type, 'OK', CONST.ALL_PROCESSES_OK)
		else:
			mySSH.close()
			logging.debug('Deployment went wrong')
			HTML.CreateHtmlTestRow(self.Type, 'KO', CONST.INVALID_PARAMETER)

	def UndeployEpc(self, HTML):
		logging.debug('Trying to undeploy')
		# No check down, we suppose everything done before.

		mySSH = SSH.SSHConnection()
		mySSH.open(self.IPAddress, self.UserName, self.Password)
		# Recovering logs and pcap files
		mySSH.command('cd ' + self.SourceCodePath + '/logs', '\$', 5)
		mySSH.command('docker exec -it prod-oai-hss /bin/bash -c "killall --signal SIGINT oai_hss tshark"', '\$', 5)
		mySSH.command('docker exec -it prod-oai-mme /bin/bash -c "killall --signal SIGINT tshark"', '\$', 5)
		mySSH.command('docker exec -it prod-oai-spgwc /bin/bash -c "killall --signal SIGINT oai_spgwc tshark"', '\$', 5)
		mySSH.command('docker exec -it prod-oai-spgwu-tiny /bin/bash -c "killall --signal SIGINT tshark"', '\$', 5)
		mySSH.command('docker logs prod-oai-hss > hss_' + self.testCase_id + '.log', '\$', 5)
		mySSH.command('docker logs prod-oai-mme > mme_' + self.testCase_id + '.log', '\$', 5)
		mySSH.command('docker logs prod-oai-spgwc > spgwc_' + self.testCase_id + '.log', '\$', 5)
		mySSH.command('docker logs prod-oai-spgwu-tiny > spgwu_' + self.testCase_id + '.log', '\$', 5)
		mySSH.command('docker cp prod-oai-hss:/tmp/hss_check_run.pcap hss_' + self.testCase_id + '.pcap', '\$', 60)
		mySSH.command('docker cp prod-oai-mme:/tmp/mme_check_run.pcap mme_' + self.testCase_id + '.pcap', '\$', 60)
		mySSH.command('docker cp prod-oai-spgwc:/tmp/spgwc_check_run.pcap spgwc_' + self.testCase_id + '.pcap', '\$', 60)
		mySSH.command('docker cp prod-oai-spgwu-tiny:/tmp/spgwu_check_run.pcap spgwu_' + self.testCase_id + '.pcap', '\$', 60)
		# Remove all
		mySSH.command('cd ' + self.SourceCodePath + '/scripts', '\$', 5)
		listOfContainers = 'prod-cassandra prod-oai-hss prod-oai-mme prod-oai-spgwc prod-oai-spgwu-tiny'
		nbContainers = 5
		# Checking for additional services
		mySSH.command('docker-compose config', '\$', 5)
		configResponse = mySSH.getBefore()
		if configResponse.count('flexran_rtc') == 1:
			listOfContainers += ' prod-flexran-rtc'
			nbContainers += 1
		if configResponse.count('trf_gen') == 1:
			listOfContainers += ' prod-trf-gen'
			nbContainers += 1

		mySSH.command('docker-compose down', '\$', 60)
		mySSH.command('docker inspect --format=\'{{.State.Health.Status}}\' ' + listOfContainers, '\$', 10)
		noMoreContainerNb = mySSH.getBefore().count('No such object')
		mySSH.command('docker inspect --format=\'{{.Name}}\' prod-oai-public-net prod-oai-private-net', '\$', 10)
		noMoreNetworkNb = mySSH.getBefore().count('No such object')
		mySSH.close()
		if noMoreContainerNb == nbContainers and noMoreNetworkNb == 2:
			logging.debug('Undeployment OK')
			HTML.CreateHtmlTestRow(self.Type, 'OK', CONST.ALL_PROCESSES_OK)
		else:
			logging.debug('Undeployment went wrong')
			HTML.CreateHtmlTestRow(self.Type, 'KO', CONST.INVALID_PARAMETER)

	def LogCollectHSS(self):
		mySSH = SSH.SSHConnection()
		mySSH.open(self.IPAddress, self.UserName, self.Password)
		mySSH.command('cd ' + self.SourceCodePath + '/scripts', '\$', 5)
		mySSH.command('rm -f hss.log.zip', '\$', 5)
		if re.match('OAI-Rel14-Docker', self.Type, re.IGNORECASE):
			mySSH.command('docker inspect prod-oai-hss', '\$', 10)
			result = re.search('No such object', mySSH.getBefore())
			if result is not None:
				mySSH.command('cd ../logs', '\$', 5)
				mySSH.command('rm -f hss.log.zip', '\$', 5)
				mySSH.command('zip hss.log.zip hss_*.*', '\$', 60)
				mySSH.command('mv hss.log.zip ../scripts', '\$', 60)
			else:
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
			mySSH.command('docker inspect prod-oai-mme', '\$', 10)
			result = re.search('No such object', mySSH.getBefore())
			if result is not None:
				mySSH.command('cd ../logs', '\$', 5)
				mySSH.command('rm -f mme.log.zip', '\$', 5)
				mySSH.command('zip mme.log.zip mme_*.*', '\$', 60)
				mySSH.command('mv mme.log.zip ../scripts', '\$', 60)
			else:
				mySSH.command('docker cp ' + self.containerPrefix + '-oai-mme:/openair-mme/mme_check_run.log .', '\$', 60)
				mySSH.command('docker cp ' + self.containerPrefix + '-oai-mme:/tmp/mme_check_run.pcap .', '\$', 60)
				mySSH.command('zip mme.log.zip mme_check_run.*', '\$', 60)
		elif re.match('OAI', self.Type, re.IGNORECASE) or re.match('OAI-Rel14-CUPS', self.Type, re.IGNORECASE):
			mySSH.command('zip mme.log.zip mme*.log', '\$', 60)
			mySSH.command('echo ' + self.Password + ' | sudo -S rm mme*.log', '\$', 5)
		elif re.match('ltebox', self.Type, re.IGNORECASE):
			mySSH.command('cp /opt/ltebox/var/log/*Log.0 .', '\$', 5)
			mySSH.command('zip mme.log.zip mmeLog.0 s1apcLog.0 s1apsLog.0 s11cLog.0 libLog.0 s1apCodecLog.0 amfLog.0 ngapcLog.0 ngapcommonLog.0 ngapsLog.0', '\$', 60)
		else:
			logging.error('This option should not occur!')
		mySSH.close()

	def LogCollectSPGW(self):
		mySSH = SSH.SSHConnection()
		mySSH.open(self.IPAddress, self.UserName, self.Password)
		mySSH.command('cd ' + self.SourceCodePath + '/scripts', '\$', 5)
		mySSH.command('rm -f spgw.log.zip', '\$', 5)
		if re.match('OAI-Rel14-Docker', self.Type, re.IGNORECASE):
			mySSH.command('docker inspect prod-oai-mme', '\$', 10)
			result = re.search('No such object', mySSH.getBefore())
			if result is not None:
				mySSH.command('cd ../logs', '\$', 5)
				mySSH.command('rm -f spgw.log.zip', '\$', 5)
				mySSH.command('zip spgw.log.zip spgw*.*', '\$', 60)
				mySSH.command('mv spgw.log.zip ../scripts', '\$', 60)
			else:
				mySSH.command('docker cp ' + self.containerPrefix + '-oai-spgwc:/openair-spgwc/spgwc_check_run.log .', '\$', 60)
				mySSH.command('docker cp ' + self.containerPrefix + '-oai-spgwu-tiny:/openair-spgwu-tiny/spgwu_check_run.log .', '\$', 60)
				mySSH.command('docker cp ' + self.containerPrefix + '-oai-spgwc:/tmp/spgwc_check_run.pcap .', '\$', 60)
				mySSH.command('docker cp ' + self.containerPrefix + '-oai-spgwu-tiny:/tmp/spgwu_check_run.pcap .', '\$', 60)
				mySSH.command('zip spgw.log.zip spgw*_check_run.*', '\$', 60)
		elif re.match('OAI', self.Type, re.IGNORECASE) or re.match('OAI-Rel14-CUPS', self.Type, re.IGNORECASE):
			mySSH.command('zip spgw.log.zip spgw*.log', '\$', 60)
			mySSH.command('echo ' + self.Password + ' | sudo -S rm spgw*.log', '\$', 5)
		elif re.match('ltebox', self.Type, re.IGNORECASE):
			mySSH.command('cp /opt/ltebox/var/log/*Log.0 .', '\$', 5)
			mySSH.command('zip spgw.log.zip xGwLog.0 upfLog.0', '\$', 60)
		else:
			logging.error('This option should not occur!')
		mySSH.close()

	def LogCollectOAICN5G(self):
		mySSH = SSH.SSHConnection()
		mySSH.open(self.IPAddress, self.UserName, self.Password)
		logging.debug('OAI CN5G Collecting Log files to workspace')
		mySSH.command('echo ' + self.Password + ' | sudo rm -rf ' + self.SourceCodePath + '/logs', '\$', 5)
		mySSH.command('mkdir ' + self.SourceCodePath + '/logs','\$', 5)	
		containers_list=['oai-smf','oai-spgwu','oai-amf','oai-nrf']
		for c in containers_list:
			mySSH.command('docker logs ' + c + ' > ' + self.SourceCodePath + '/logs/' + c + '.log', '\$', 5)
		mySSH.command('cd ' + self.SourceCodePath + '/logs', '\$', 5)		
		mySSH.command('zip oai-cn5g.log.zip *.log', '\$', 60)
		mySSH.close()

