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
import pexpect          # pexpect
import logging
import time             # sleep
import re
import subprocess
import sys

#-----------------------------------------------------------
# Class Declaration
#-----------------------------------------------------------
class SSHConnection():
	def __init__(self):
		self.ssh = ''
		self.picocom_closure = False
		self.ipaddress = ''
		self.username = ''
		self.cmd2Results = ''

	def disablePicocomClosure(self):
		self.picocom_closure = False

	def enablePicocomClosure(self):
		self.picocom_closure = True

	def open(self, ipaddress, username, password):
		count = 0
		connect_status = False
		while count < 4:
			self.ssh = pexpect.spawn('ssh -o PubkeyAuthentication=no {}@{}'.format(username,ipaddress))
			self.ssh.timeout = 5
			self.sshresponse = self.ssh.expect(['Are you sure you want to continue connecting (yes/no)?', 'password:', 'Last login', pexpect.EOF, pexpect.TIMEOUT])
			if self.sshresponse == 0:
				self.ssh.sendline('yes')
				self.sshresponse = self.ssh.expect(['password:', username + '@'])
				if self.sshresponse == 0:
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
		self.ipaddress = ipaddress
		self.username = username




	def cde_check_value(self, commandline, expected, timeout):
		logging.debug(commandline)
		self.ssh.timeout = timeout
		self.ssh.sendline(commandline)
		expected.append(pexpect.EOF)
		expected.append(pexpect.TIMEOUT)
		self.sshresponse = self.ssh.expect(expected)
		return self.sshresponse

	def command(self, commandline, expectedline, timeout, silent=False, resync=False):
		if not silent:
			logging.debug(commandline)
		self.ssh.timeout = timeout
		# Nasty patch when pexpect output is out of sync.
		# Much pronounced when running back-to-back-back oc commands
		if resync:
			self.ssh.send(commandline)
			self.ssh.expect([commandline, pexpect.TIMEOUT])
			self.ssh.send('\r\n')
			self.sshresponse = self.ssh.expect([expectedline, pexpect.EOF, pexpect.TIMEOUT])
		else:
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

	def command2(self, commandline, timeout, silent=False):
		if not silent:
			logging.debug(commandline)
		self.cmd2Results = ''
		myHost = self.username + '@' + self.ipaddress
		# CAUTION: THIS METHOD IMPLIES THAT THERE ARE VALID SSH KEYS
		# BETWEEN THE PYTHON EXECUTOR NODE AND THE REMOTE HOST
		# OTHERWISE IT WON'T WORK
		lSsh = subprocess.Popen(["ssh", "%s" % myHost, commandline],shell=False,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
		self.cmd2Results = str(lSsh.stdout.readlines())

	def command3(self, commandline, timeout, silent=False):
		if not silent:
			logging.debug(commandline)
		self.cmd2Results = ''
		myHost = self.username + '@' + self.ipaddress
		# CAUTION: THIS METHOD IMPLIES THAT THERE ARE VALID SSH KEYS
		# BETWEEN THE PYTHON EXECUTOR NODE AND THE REMOTE HOST
		# OTHERWISE IT WON'T WORK
		lSsh = subprocess.Popen(["ssh", "%s" % myHost, commandline],shell=False,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
		return lSsh.stdout.readlines()

		
	def close(self):
		self.ssh.timeout = 5
		self.ssh.sendline('exit')
		self.sshresponse = self.ssh.expect([pexpect.EOF, pexpect.TIMEOUT])
		self.ipaddress = ''
		self.username = ''
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

	def getBefore(self):
		return str(self.ssh.before)
