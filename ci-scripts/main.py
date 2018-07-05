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
from multiprocessing import Process,Lock
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
		self.desc = ''
		self.Build_eNB_args = ''
		self.Initialize_eNB_args = ''
		self.ping_args = ''
		self.ping_packetloss_threshold = ''
		self.iperf_args = ''
		self.iperf_packetloss_threshold = ''
		self.UEDevices = []
		self.UEIPAddresses = []
	def open(self, ipaddress, username, password):
		self.ssh = pexpect.spawn('ssh', [username + '@' + ipaddress], timeout = 10)
		self.ssh.delaybeforesend = None
		self.sshresponse = self.ssh.expect(['Are you sure you want to continue connecting (yes/no)?', 'password:', pexpect.TIMEOUT, pexpect.EOF])
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
			pass
			logging.debug('Connected to server')
		else:
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
		
	def BuildeNB(self):
		if self.eNBIPAddress == '' or self.eNBRepository == '' or self.eNBBranch == '' or self.eNBUserName == '' or self.eNBPassword == '' or self.eNBSourceCodePath == '':
			Usage()
			sys.exit('Insufficient Parameter')
		self.open(self.eNBIPAddress, self.eNBUserName, self.eNBPassword)
		# We are in the context of CI. We are starting from a clean slate
		# So we are removing the whole CI folder and starting from scratch
		# Also we are not necessary always on the same branch. The path might differ
		self.command('if [ -d ' + self.eNBSourceCodePath +' ]; rm -Rf ' + self.eNBSourceCodePath + '; fi', '\$', 5)
		self.command('mkdir -p ' + self.eNBSourceCodePath, '\$', 5)
		self.command('cd ' + self.eNBSourceCodePath, '\$', 5)
		self.command('git clone -b ' + self.eNBBranch + ' ' + self.eNBRepository + ' .', '\$', 600)
		self.command('git config user.email "jenkins@openairinterface.org"', '\$', 5)
		self.command('git config user.name "OAI Jenkins"', '\$', 5)
		# if the commit ID is provided use it to point to it
		if self.eNBCommitID != '':
			self.command('git checkout -f ' + self.eNBCommitID, '\$', 5)
		# if the branch is not develop, then it is a merge request and we need to do 
		# the potential merge. Note that merge conflicts should already been checked earlier
		if self.eNBBranch != 'develop':
			self.command('git merge --ff origin/develop', '\$', 5)
		self.command('source oaienv', '\$', 5)
		self.command('cd cmake_targets', '\$', 5)
		self.command('mkdir -p  log', '\$', 5)
		self.command('cd log', '\$', 5)
		self.command('echo ' + self.eNBPassword + ' | sudo -S rm -f *', '\$', 5)
		self.command('cd ..', '\$', 5)
		self.command('echo ' + self.eNBPassword + ' | sudo -S stdbuf -o0 ./build_oai ' + self.Build_eNB_args + ' 2>&1 | stdbuf -o0 tee -a compile_oai_enb.log', 'Bypassing the Tests', 600)
		self.command('mkdir -p build_log_' + SSH.testCase_id, '\$', 5)
		self.command('echo ' + self.eNBPassword + ' | sudo -S mv log/* ' + 'build_log_' + SSH.testCase_id, '\$', 5)
		self.command('echo ' + self.eNBPassword + ' | sudo -S mv compile_oai_enb.log ' + 'build_log_' + SSH.testCase_id, '\$', 5)
		self.close()
		
	def InitializeHSS(self):
		if self.EPCIPAddress == '' or self.EPCUserName == '' or self.EPCPassword == '' or self.EPCSourceCodePath == '' or self.EPCType == '':
			Usage()
			sys.exit('Insufficient Parameter')
		self.open(self.EPCIPAddress, self.EPCUserName, self.EPCPassword)
		self.command('cd ' + self.EPCSourceCodePath, '\$', 5)
		if re.match('OAI', self.EPCType, re.IGNORECASE):
			logging.debug('Using the OAI EPC HSS')
			self.command('source oaienv', '\$', 5)
			self.command('cd scripts', '\$', 5)
			self.command('echo ' + self.EPCPassword + ' | sudo -S ./run_hss 2>&1 | stdbuf -o0 tee -a hss_' + SSH.testCase_id + '.log &', 'Core state: 2 -> 3', 35)
#			self.command('echo ' + self.EPCPassword + ' | sudo -S ./run_hss 2>&1 | stdbuf -o0 awk \'{ print strftime("[%Y/%m/%d %H:%M:%S] ",systime()) $0 }\' | stdbuf -o0 tee -a hss_' + SSH.testCase_id + '.log &', 'Initializing s6a layer: DONE', 35)
		else:
			logging.debug('Using the ltebox simulated HSS')
			self.command('cd hss_sim0609', '\$', 5)
			self.command('echo ' + self.EPCPassword + ' | sudo -S echo "Starting sudo session" && sudo daemon --unsafe --name=simulated_hss --chdir=/opt/hss_sim0609 ./starthss_real  ', '\$', 5)
		self.close()
		
	def InitializeMME(self):
		if self.EPCIPAddress == '' or self.EPCUserName == '' or self.EPCPassword == '' or self.EPCSourceCodePath == '' or self.EPCType == '':
			Usage()
			sys.exit('Insufficient Parameter')
		self.open(self.EPCIPAddress, self.EPCUserName, self.EPCPassword)
		self.command('cd ' + self.EPCSourceCodePath, '\$', 5)
		if re.match('OAI', self.EPCType, re.IGNORECASE):
			self.command('source oaienv', '\$', 5)
			self.command('cd scripts', '\$', 5)
			self.command('hostname', '\$', 5)
			result = re.search('hostname\\\\r\\\\n(?P<host_name>[a-zA-Z0-9\-\_]+)\\\\r\\\\n', str(self.ssh.before))
			if result is None:
				logging.debug('\u001B[1;37;41m Hostname Not Found! \u001B[0m')
				sys.exit(1)
			host_name = result.group('host_name')
			self.command('echo ' + self.EPCPassword + ' | sudo -S ./run_mme 2>&1 | stdbuf -o0 tee -a mme_' + SSH.testCase_id + '.log &', 'MME app initialization complete', 100)
		else:
			self.command('cd ltebox/tools', '\$', 5)
			self.command('echo ' + self.EPCPassword + ' | sudo -S ./start_mme', '\$', 5)
		self.close()

	def InitializeSPGW(self):
		if self.EPCIPAddress == '' or self.EPCUserName == '' or self.EPCPassword == '' or self.EPCSourceCodePath == '' or self.EPCType == '':
                        Usage()
                        sys.exit('Insufficient Parameter')
		self.open(self.EPCIPAddress, self.EPCUserName, self.EPCPassword)
		self.command('cd ' + self.EPCSourceCodePath, '\$', 5)
		if re.match('OAI', self.EPCType, re.IGNORECASE):
			self.command('source oaienv', '\$', 5)
			self.command('cd scripts', '\$', 5)
			self.command('echo ' + self.EPCPassword + ' | sudo -S ./run_spgw 2>&1 | stdbuf -o0 tee -a spgw_' + SSH.testCase_id + '.log &', 'Initializing SPGW-APP task interface: DONE', 30)
		else:
			self.command('cd ltebox/tools', '\$', 5)
			self.command('echo ' + self.EPCPassword + ' | sudo -S ./start_xGw', '\$', 5)
		self.close()

	def InitializeeNB(self):
		if self.eNBIPAddress == '' or self.eNBUserName == '' or self.eNBPassword == '' or self.eNBSourceCodePath == '':
                        Usage()
                        sys.exit('Insufficient Parameter')
		self.CheckHSSProcess()
		self.CheckMMEProcess()
		self.CheckSPGWProcess()
		self.open(self.eNBIPAddress, self.eNBUserName, self.eNBPassword)
		self.command('cd ' + self.eNBSourceCodePath, '\$', 5)
		self.command('cd targets/PROJECTS/GENERIC-LTE-EPC/CONF/', '\$', 5)
		self.command('cp ' + self.Initialize_eNB_args + ' ci-' + self.Initialize_eNB_args, '\$', 5)
		self.command('sed -i -e \'s/mme_ip_address.*$/mme_ip_address      = ( { ipv4       = "' + self.EPCIPAddress + '";/\' ci-' + self.Initialize_eNB_args, '\$', 2);
		self.command('sed -i -e \'s/ENB_IPV4_ADDRESS_FOR_S1_MME.*$/ENB_IPV4_ADDRESS_FOR_S1_MME              = "' + self.eNBIPAddress + '";/\' ci-' + self.Initialize_eNB_args, '\$', 2);
		self.command('sed -i -e \'s/ENB_IPV4_ADDRESS_FOR_S1U.*$/ENB_IPV4_ADDRESS_FOR_S1U                 = "' + self.eNBIPAddress + '";/\' ci-' + self.Initialize_eNB_args, '\$', 2);
		self.command('cd ' + self.eNBSourceCodePath, '\$', 5)
		self.command('source oaienv', '\$', 5)
		self.command('cd cmake_targets', '\$', 5)
		self.command('echo ' + self.eNBPassword + ' | ' + 'sudo -S -E ./lte_build_oai/build/lte-softmodem -O ' + self.eNBSourceCodePath + '/targets/PROJECTS/GENERIC-LTE-EPC/CONF/ci-' + self.Initialize_eNB_args + ' 2>&1 | stdbuf -o0 tee -a enb_' + SSH.testCase_id + '.log &', 'got sync', 60)
		self.close()

	def CheckeNBProcess(self):
		self.open(self.eNBIPAddress, self.eNBUserName, self.eNBPassword)
		self.command('echo ' + self.eNBPassword + ' | ps -aux | grep -v grep | grep lte-softmodem', '\$', 5)
		result = re.search('lte-softmodem', str(self.ssh.before))
		if result is None:
			logging.debug('\u001B[1;37;41m eNB Process Not Found! \u001B[0m')
			sys.exit(1)
		self.close()

	def CheckHSSProcess(self):
		self.open(self.EPCIPAddress, self.EPCUserName, self.EPCPassword)
		self.command('ps -aux | grep -v grep | grep hss > hss_processes.txt', '\$', 5)
		# If we don't timeout we are OK
		if re.match('OAI', self.EPCType, re.IGNORECASE):
			self.command('cat hss_processes.txt', '/bin/bash ./run_', 5)
		else:
			self.command('cat hss_processes.txt', 'hss_sim s6as diam_hss', 5)
		self.command('rm hss_processes.txt', '\$', 5)
		self.close()

	def CheckMMEProcess(self):
		self.open(self.EPCIPAddress, self.EPCUserName, self.EPCPassword)
		self.command('ps -aux | grep -v grep | grep mme > mme_processes.txt', '\$', 5)
		# If we don't timeout we are OK
		if re.match('OAI', self.EPCType, re.IGNORECASE):
			self.command('cat mme_processes.txt', '/bin/bash ./run_', 5)
		else:
			self.command('cat mme_processes.txt', 'mme', 5)
		self.command('rm mme_processes.txt', '\$', 5)
		self.close()

	def CheckSPGWProcess(self):
		self.open(self.EPCIPAddress, self.EPCUserName, self.EPCPassword)
		# If we don't timeout we are OK
		if re.match('OAI', self.EPCType, re.IGNORECASE):
			self.command('ps -aux | grep -v grep | grep spgw > spgw_processes.txt', '\$', 5)
			self.command('cat spgw_processes.txt', '/bin/bash ./run_', 5)
		else:
			self.command('ps -aux | grep -v grep | grep xGw > spgw_processes.txt', '\$', 5)
			self.command('cat spgw_processes.txt', 'xGw', 5)
		self.command('rm spgw_processes.txt', '\$', 5)
		self.close()

	def TerminateeNB(self):
		self.open(self.eNBIPAddress, self.eNBUserName, self.eNBPassword)
		self.command('echo ' + self.eNBPassword + ' | sudo -S killall --signal SIGINT lte-softmodem || true', '\$', 5)
		time.sleep(5)
		self.command('echo ' + self.eNBPassword + ' | ps -aux | grep -v grep | grep lte-softmodem', '\$', 5)
		result = re.search('lte-softmodem', str(self.ssh.before))
		if result is not None:
			self.command('echo ' + self.eNBPassword + ' | sudo -S killall --signal SIGKILL lte-softmodem || true', '\$', 5)
		self.close()

	def TerminateHSS(self):
		self.open(self.EPCIPAddress, self.EPCUserName, self.EPCPassword)
		if re.match('OAI', self.EPCType, re.IGNORECASE):
			self.command('echo ' + self.EPCPassword + ' | sudo -S killall --signal SIGINT run_hss oai_hss || true', '\$', 5)
			time.sleep(2)
			self.command('ps -aux | grep -v grep | grep hss', '\$', 5)
			result = re.search('\/bin\/bash .\/run_', str(self.ssh.before))
			if result is not None:
				self.command('echo ' + self.EPCPassword + ' | sudo -S killall --signal SIGKILL run_hss oai_hss || true', '\$', 5)
		else:
			self.command('echo ' + self.EPCPassword + ' | sudo -S daemon --name=simulated_hss --stop', '\$', 5)
			time.sleep(2)
			self.command('ps -aux | egrep "hss_sim|simulated_hss" | grep -v grep | awk \'BEGIN{n=0}{pidId[n]=$2;n=n+1}END{print "kill -9 " pidId[0] " " pidId[1]}\' > /home/' + self.EPCUserName + '/kill_hss.sh', '\$', 5)
			self.command('chmod 755 /home/' + self.EPCUserName + '/kill_hss.sh', '\$', 5)
			self.command('echo ' + self.EPCPassword + ' | sudo -S /home/' + self.EPCUserName + '/kill_hss.sh', '\$', 5)
			self.command('rm /home/' + self.EPCUserName + '/kill_hss.sh', '\$', 5)
		self.close()

	def TerminateMME(self):
		self.open(self.EPCIPAddress, self.EPCUserName, self.EPCPassword)
		if re.match('OAI', self.EPCType, re.IGNORECASE):
			self.command('echo ' + self.EPCPassword + ' | sudo -S killall --signal SIGINT run_mme mme || true', '\$', 5)
			time.sleep(2)
			self.command('echo ' + self.EPCPassword + ' | ps -aux | grep -v grep | grep mme', '\$', 5)
			result = re.search('\/bin\/bash .\/run_', str(self.ssh.before))
			if result is not None:
				self.command('echo ' + self.EPCPassword + ' | sudo -S killall --signal SIGKILL run_mme mme || true', '\$', 5)
		else:
			self.command('cd ltebox/tools', '\$', 5)
			self.command('echo ' + self.EPCPassword + ' | sudo -S ./stop_mme', '\$', 5)
		self.close()

	def TerminateSPGW(self):
		self.open(self.EPCIPAddress, self.EPCUserName, self.EPCPassword)
		if re.match('OAI', self.EPCType, re.IGNORECASE):
			self.command('echo ' + self.EPCPassword + ' | sudo -S killall --signal SIGINT run_spgw spgw || true', '\$', 5)
			time.sleep(2)
			self.command('echo ' + self.EPCPassword + ' | ps -aux | grep -v grep | grep spgw', '\$', 5)
			result = re.search('\/bin\/bash .\/run_', str(self.ssh.before))
			if result is not None:
				self.command('echo ' + self.EPCPassword + ' | sudo -S killall --signal SIGKILL run_spgw spgw || true', '\$', 5)
		else:
			self.command('cd ltebox/tools', '\$', 5)
			self.command('echo ' + self.EPCPassword + ' | sudo -S ./stop_xGw', '\$', 5)
		self.close()

	def LogCollectBuild(self):
		self.open(self.eNBIPAddress, self.eNBUserName, self.eNBPassword)
		self.command('cd ' + self.eNBSourceCodePath, '\$', 5)
		self.command('cd cmake_targets', '\$', 5)
		self.command('zip build.log.zip build_log_*/*', '\$', 60)
		self.command('rm -rf build_log_*', '\$', 5)
		self.close()
		
	def LogCollecteNB(self):
		self.open(self.eNBIPAddress, self.eNBUserName, self.eNBPassword)
		self.command('cd ' + self.eNBSourceCodePath, '\$', 5)
		self.command('cd cmake_targets', '\$', 5)
		self.command('zip enb.log.zip enb*.log', '\$', 60)
		self.command('rm enb*.log', '\$', 5)
		self.close()
		
	def LogCollectPing(self):
		self.open(self.EPCIPAddress, self.EPCUserName, self.EPCPassword)
		self.command('cd ' + self.EPCSourceCodePath, '\$', 5)
		self.command('cd scripts', '\$', 5)
		self.command('zip ping.log.zip ping*.log', '\$', 60)
		self.command('rm ping*.log', '\$', 5)
		self.close()
		
	def LogCollectIperf(self):
		self.open(self.EPCIPAddress, self.EPCUserName, self.EPCPassword)
		self.command('cd ' + self.EPCSourceCodePath, '\$', 5)
		self.command('cd scripts', '\$', 5)
		self.command('zip iperf.log.zip iperf*.log', '\$', 60)
		self.command('rm iperf*.log', '\$', 5)
		self.close()
		
	def LogCollectHSS(self):
		self.open(self.EPCIPAddress, self.EPCUserName, self.EPCPassword)
		if re.match('OAI', self.EPCType, re.IGNORECASE):
			self.command('cd ' + self.EPCSourceCodePath, '\$', 5)
			self.command('cd scripts', '\$', 5)
			self.command('zip hss.log.zip hss*.log', '\$', 60)
			self.command('rm hss*.log', '\$', 5)
		self.close()
		
	def LogCollectMME(self):
		self.open(self.EPCIPAddress, self.EPCUserName, self.EPCPassword)
		self.command('cd ' + self.EPCSourceCodePath, '\$', 5)
		if re.match('OAI', self.EPCType, re.IGNORECASE):
			self.command('cd scripts', '\$', 5)
			self.command('zip mme.log.zip mme*.log', '\$', 60)
			self.command('rm mme*.log', '\$', 5)
		else:
			self.command('cd ltebox/var/log', '\$', 5)
			self.command('zip /home/' + self.EPCUserName + '/mme.log.zip mmeLog.0 s1apcLog.0 s1apsLog.0 s11cLog.0 libLog.0 s1apCodecLog.0', '\$', 60)
		self.close()
		
	def LogCollectSPGW(self):
		self.open(self.EPCIPAddress, self.EPCUserName, self.EPCPassword)
		self.command('cd ' + self.EPCSourceCodePath, '\$', 5)
		if re.match('OAI', self.EPCType, re.IGNORECASE):
			self.command('cd scripts', '\$', 5)
			self.command('zip spgw.log.zip spgw*.log', '\$', 60)
			self.command('rm spgw*.log', '\$', 5)
		else:
			self.command('cd ltebox/var/log', '\$', 5)
			self.command('zip /home/' + self.EPCUserName + '/spgw.log.zip xGwLog.0', '\$', 60)
		self.close()
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
	print('      TerminateeNB, TerminateEPC')
	print('      LogCollectBuild, LogCollecteNB, LogCollectEPC, LogCollectADB')
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
	if SSH.EPCIPAddress == '' or SSH.EPCUserName == '' or SSH.EPCPassword == '':
		Usage()
		sys.exit('Insufficient Parameter')
	SSH.TerminateHSS()
elif re.match('^TerminateMME$', mode, re.IGNORECASE):
	if SSH.EPCIPAddress == '' or SSH.EPCUserName == '' or SSH.EPCPassword == '':
		Usage()
		sys.exit('Insufficient Parameter')
	SSH.TerminateMME()
elif re.match('^TerminateSPGW$', mode, re.IGNORECASE):
	if SSH.EPCIPAddress == '' or SSH.EPCUserName == '' or SSH.EPCPassword == '':
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
	if SSH.EPCIPAddress == '' or SSH.EPCUserName == '' or SSH.EPCPassword == '' or SSH.EPCSourceCodePath == '':
		Usage()
		sys.exit('Insufficient Parameter')
	SSH.LogCollectHSS()
elif re.match('^LogCollectMME$', mode, re.IGNORECASE):
	if SSH.EPCIPAddress == '' or SSH.EPCUserName == '' or SSH.EPCPassword == '' or SSH.EPCSourceCodePath == '':
		Usage()
		sys.exit('Insufficient Parameter')
	SSH.LogCollectMME()
elif re.match('^LogCollectSPGW$', mode, re.IGNORECASE):
	if SSH.EPCIPAddress == '' or SSH.EPCUserName == '' or SSH.EPCPassword == '' or SSH.EPCSourceCodePath == '':
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
	#read test_case_list.xml file
	xml_test_file = sys.path[0] + "/test_case_list.xml"

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
			logging.debug('INFO: test will be run: ' + test)
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
				SSH.GetAllUEDevices()
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
else:
	Usage()
	sys.exit('Invalid mode')
sys.exit(0)
