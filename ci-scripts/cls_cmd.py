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
#   Required Python Version
#     Python 3.x
#
#---------------------------------------------------------------------

import abc
import logging
import subprocess as sp
import os
import paramiko
import uuid
import sys 

SSHTIMEOUT=7

# helper that returns either LocalCmd or RemoteCmd based on passed host name
def getConnection(host, d=None):
	if host is None or host.lower() in ["", "none", "localhost"]:
		return LocalCmd(d=d)
	else:
		return RemoteCmd(host, d=d)

# provides a partial interface for the legacy SSHconnection class (getBefore(), command())
class Cmd(metaclass=abc.ABCMeta):
	def cd(self, d, silent=False):
		if d == None or d == '':
			self.cwd = None
		elif d[0] == '/':
			self.cwd = d
		else:
			if not self.cwd:
				# no cwd set: get current working directory
				self.cwd = self.run('pwd').stdout.strip()
			self.cwd += f"/{d}"
		if not silent:
			logging.debug(f'cd {self.cwd}')

	@abc.abstractmethod
	def run(self, line, timeout=300, silent=False):
		return

	def command(self, commandline, expectedline=None, timeout=300, silent=False, resync=False):
		splitted = commandline.split(' ')
		if splitted[0] == 'cd':
			self.cd(' '.join(splitted[1:]), silent)
		else:
			self.run(commandline, timeout, silent)
		return 0

	@abc.abstractmethod
	def close(self):
		return

	@abc.abstractmethod
	def getBefore(self):
		return

	@abc.abstractmethod
	def copyin(self, scpIp, scpUser, scpPw, src, tgt):
		return

	@abc.abstractmethod
	def copyout(self, scpIp, scpUser, scpPw, src, tgt):
		return

class LocalCmd(Cmd):
	def __init__(self, d = None):
		self.cwd = d
		if self.cwd is not None:
			logging.debug(f'Working dir is {self.cwd}')
		self.cp = sp.CompletedProcess(args='', returncode=0, stdout='')

	def run(self, line, timeout=300, silent=False, reportNonZero=True):
		if not silent:
			logging.info(line)
		try:
			if line.strip().endswith('&'):
				# if we wait for stdout, subprocess does not return before the end of the command
				# however, we don't want to wait for commands with &, so just return fake command
				ret = sp.run(line, shell=True, cwd=self.cwd, timeout=5)
			else:
				ret = sp.run(line, shell=True, cwd=self.cwd, stdout=sp.PIPE, stderr=sp.STDOUT, timeout=timeout)
		except Exception as e:
			ret = sp.CompletedProcess(args=line, returncode=255, stdout=f'Exception: {str(e)}'.encode('utf-8'))
		if ret.stdout is None:
			ret.stdout = b''
		ret.stdout = ret.stdout.decode('utf-8').strip()
		if reportNonZero and ret.returncode != 0:
			logging.warning(f'command "{ret.args}" returned non-zero returncode {ret.returncode}: output:\n{ret.stdout}')
		self.cp = ret
		return ret

	def close(self):
		pass

	def getBefore(self):
		return self.cp.stdout

	def copyin(self, src, tgt, recursive=False):
		if src[0] != '/' or tgt[0] != '/':
			raise Exception('support only absolute file paths!')
		opt = '-r' if recursive else ''
		self.run(f'cp {opt} {src} {tgt}')

	def copyout(self, src, tgt, recursive=False):
		self.copyin(src, tgt, recursive)

class RemoteCmd(Cmd):
	def __init__(self, hostname, d=None):
		cIdx = 0
		logging.getLogger('paramiko').setLevel(logging.ERROR) # prevent spamming through Paramiko
		self.client = paramiko.SSHClient()
		self.client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
		cfg = RemoteCmd._lookup_ssh_config(hostname)
		self.cwd = d
		self.cp = sp.CompletedProcess(args='', returncode=0, stdout='')
		while cIdx < 3:
			try:
				self.client.connect(**cfg)
				return
			except:
				logging.error(f'Could not connect to {hostname}, tried for {cIdx} time')
				cIdx +=1
		raise Exception ("Error: max retries, did not connect to host")

	def _lookup_ssh_config(hostname):
		ssh_config = paramiko.SSHConfig()
		user_config_file = os.path.expanduser("~/.ssh/config")
		if os.path.exists(user_config_file):
			with open(user_config_file) as f:
				ssh_config.parse(f)
		else:
			raise FileNotFoundError('class needs SSH config at ~/.ssh/config')
		ucfg = ssh_config.lookup(hostname)
		if 'identityfile' not in ucfg or 'user' not in ucfg:
			raise KeyError(f'no identityfile or user in SSH config for host {hostname}')
		cfg = {'hostname':hostname, 'username':ucfg['user'], 'key_filename':ucfg['identityfile'], 'timeout':SSHTIMEOUT}
		if 'hostname' in ucfg:
			cfg['hostname'] = ucfg['hostname'] # override user-given hostname with what is in config
		if 'port' in ucfg:
			cfg['port'] = int(ucfg['port'])
		if 'proxycommand' in ucfg:
			cfg['sock'] = paramiko.ProxyCommand(ucfg['proxycommand'])
		return cfg

	def run(self, line, timeout=300, silent=False, reportNonZero=True):
		if not silent:
			logging.info(line)
		if self.cwd:
			line = f"cd {self.cwd} && {line}"
		try:
			if line.strip().endswith('&'):
				# if we wait for stdout, Paramiko does not return before the end of the command
				# however, we don't want to wait for commands with &, so just return fake command
				self.client.exec_command(line, timeout = 5)
				ret = sp.CompletedProcess(args=line, returncode=0, stdout=b'')
			else:
				stdin, stdout, stderr = self.client.exec_command(line, timeout=timeout)
				ret = sp.CompletedProcess(args=line, returncode=stdout.channel.recv_exit_status(), stdout=stdout.read(size=None) + stderr.read(size=None))
		except Exception as e:
			ret = sp.CompletedProcess(args=line, returncode=255, stdout=f'Exception: {str(e)}'.encode('utf-8'))
		ret.stdout = ret.stdout.decode('utf-8').strip()
		if reportNonZero and ret.returncode != 0:
			logging.warning(f'command "{line}" returned non-zero returncode {ret.returncode}: output:\n{ret.stdout}')
		self.cp = ret
		return ret

	def close(self):
		self.client.close()

	def getBefore(self):
		return self.cp.stdout

	def copyout(self, src, tgt, recursive=False):
		logging.debug(f"copyout: local:{src} -> remote:{tgt}")
		if recursive:
			tmpfile = f"{uuid.uuid4()}.tar"
			abstmpfile = f"/tmp/{tmpfile}"
			cmd = LocalCmd()
			cmd.run(f"tar -cf {abstmpfile} {src}")
			sftp = self.client.open_sftp()
			sftp.put(abstmpfile, abstmpfile)
			sftp.close()
			cmd.run(f"rm {abstmpfile}")
			self.run(f"mv {abstmpfile} {tgt}; cd {tgt} && tar -xf {tmpfile} && rm {tmpfile}")
		else:
			sftp = self.client.open_sftp()
			sftp.put(src, tgt)
			sftp.close()

	def copyin(self, src, tgt, recursive=False):
		logging.debug(f"copyin: remote:{src} -> local:{tgt}")
		if recursive:
			tmpfile = f"{uuid.uuid4()}.tar"
			abstmpfile = f"/tmp/{tmpfile}"
			self.run(f"tar -cf {abstmpfile} {src}")
			sftp = self.client.open_sftp()
			sftp.get(abstmpfile, abstmpfile)
			sftp.close()
			self.run(f"rm {abstmpfile}")
			cmd = LocalCmd()
			cmd.run(f"mv {abstmpfile} {tgt}; cd {tgt} && tar -xf {tmpfile} && rm {tmpfile}")
		else:
			sftp = self.client.open_sftp()
			sftp.get(src, tgt)
			sftp.close()
