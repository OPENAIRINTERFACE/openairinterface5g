# SPDX-License-Identifier: LicenseRef-CSSL-1.0

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
import time

SSHTIMEOUT=7

def is_local(host):
	return host is None or host.lower() in ["", "none", "localhost"]

# helper that returns either LocalCmd or RemoteCmd based on passed host name
def getConnection(host, d=None):
	if is_local(host):
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
	def exec_script(path, timeout, parameters=None, redirect=None, silent=False):
		return

	@abc.abstractmethod
	def run(self, line, timeout=300, silent=False):
		return

	@abc.abstractmethod
	def close(self):
		return

	@abc.abstractmethod
	def getBefore(self):
		return

	@abc.abstractmethod
	def copyin(self, src, tgt):
		return

class LocalCmd(Cmd):
	def __enter__(self):
		return self

	def __exit__(self, exc_type, exc_value, exc_traceback):
		self.close()

	def __init__(self, d = None):
		self.cwd = d
		if self.cwd is not None:
			logging.debug(f'Working dir is {self.cwd}')
		self.cp = sp.CompletedProcess(args='', returncode=0, stdout='')

	def exec_script(self, path, timeout, parameters=None, redirect=None, silent=False):
		if redirect and not redirect.startswith("/"):
			raise ValueError(f"redirect must be absolute, but is {redirect}")
		c = f"{path} {parameters}" if parameters else path
		if not redirect:
			ret = sp.run(c, shell=True, cwd=self.cwd, timeout=timeout, stdout=sp.PIPE, stderr=sp.STDOUT)
			ret.stdout = ret.stdout.decode('utf-8').strip()
		else:
			with open(redirect, "w") as f:
				ret = sp.run(c, shell=True, timeout=timeout, stdout=f, stderr=f)
			ret.args += f" &> {redirect}"
			ret.stdout = ""
		if not silent:
			logging.debug(f"local> {ret.args}")
		return ret

	def run(self, line, timeout=300, silent=False, reportNonZero=True):
		if not silent:
			logging.debug(f"local> {line}")
		try:
			if line.strip().endswith('&'):
				# if we wait for stdout, subprocess does not return before the end of the command
				# however, we don't want to wait for commands with &, so just return fake command
				ret = sp.run(line, shell=True, cwd=self.cwd, timeout=5, executable='/bin/bash')
				time.sleep(0.1)
			else:
				ret = sp.run(line, shell=True, cwd=self.cwd, stdout=sp.PIPE, stderr=sp.STDOUT, timeout=timeout, executable='/bin/bash')
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

	def copyin(self, src, tgt):
		if src[0] != '/' or tgt[0] != '/':
			raise Exception(f'support only absolute file paths (src {src} tgt {tgt})!')
		if src == tgt:
			return True # nothing to copy, file is already where it should go
		return self.run(f'cp {src} {tgt}').returncode == 0

def GetFile(client, src, tgt):
	success = True
	sftp = client.open_sftp()
	try:
		sftp.get(src, tgt)
	except FileNotFoundError as error:
		logging.error(f"error while getting {src}: {error}")
		success = False
	sftp.close()
	return success

class RemoteCmd(Cmd):
	def __enter__(self):
		return self

	def __exit__(self, exc_type, exc_value, exc_traceback):
		self.close()

	def __init__(self, hostname, d=None):
		cIdx = 0
		self.hostname = hostname
		self.client = RemoteCmd._ssh_init()
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

	def _ssh_init():
		logging.getLogger('paramiko').setLevel(logging.ERROR) # prevent spamming through Paramiko
		client = paramiko.SSHClient()
		client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
		return client

	def _lookup_ssh_config(hostname):
		if is_local(hostname):
			raise ValueError("Using localhost as SSH target is not allowed: use LocalCmd instead.")
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

	def exec_script(self, path, timeout, parameters=None, redirect=None, silent=False):
		if redirect and not redirect.startswith("/"):
			raise ValueError(f"redirect must be absolute, but is {redirect}")
		p = parameters if parameters else ""
		r = f"> {redirect}" if redirect else ""
		if not silent:
			logging.debug(f"local> ssh {self.hostname} bash -s {p} < {path} {r}")
		bash_opt = 'BASH_XTRACEFD=1' # write bash set -x output to stdout, see bash(1)
		stdin, stdout, stderr = self.client.exec_command(f"{bash_opt} bash -s {p} {r}", timeout=timeout)
		# open() the file f at path, read() it and write() it into the stdin of the bash -s cmd
		with open(path) as f:
			stdin.write(f.read())
			stdin.close()
		cmd = path
		if parameters: cmd += f" {parameters}"
		if redirect: cmd += f" &> {redirect}"
		ret = sp.CompletedProcess(args=cmd, returncode=stdout.channel.recv_exit_status(), stdout=stdout.read(size=None) + stderr.read(size=None))
		ret.stdout = ret.stdout.decode('utf-8').strip()
		return ret

	def run(self, line, timeout=300, silent=False, reportNonZero=True):
		if not silent:
			logging.debug(f"ssh[{self.hostname}]> {line}")
		if self.cwd:
			line = f"cd {self.cwd} && {line}"
		try:
			if line.strip().endswith('&'):
				# if we wait for stdout, Paramiko does not return before the end of the command
				# however, we don't want to wait for commands with &, so just return fake command
				self.client.exec_command(line, timeout = 5)
				ret = sp.CompletedProcess(args=line, returncode=0, stdout=b'')
				time.sleep(0.1)
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

	# tgt and src must be a file name
	def copyin(self, src, tgt):
		logging.debug(f"copyin: {self.hostname}:{src} -> local:{tgt}")
		return GetFile(self.client, src, tgt)
