# SPDX-License-Identifier: LicenseRef-CSSL-1.0

#---------------------------------------------------------------------
#
#   Required Python Version
#     Python 3.x
#
#---------------------------------------------------------------------

import logging
import time
import re
import yaml

import cls_cmd
from cls_ci_helper import archiveArtifact

class Module_UE:

	def __init__(self, module_name, node=None, filename="ci_infra.yaml"):
		with open(filename, 'r') as f:
			all_ues = yaml.load(f, Loader=yaml.FullLoader)
			m = all_ues.get(module_name)
			if m is None:
				raise Exception(f'no such module name "{module_name}" in "{filename}"')
			self.module_name = module_name
			self.host = m['Host'] if m['Host'] != "%%current_host%%" else None
			if node is None and self.host is None:
				raise Exception(f'node not provided when needed')
			elif node is not None and self.host is None:
				self.host = node
			self.cmd_dict = {
				"attach": m.get('AttachScript'),
				"detach": m.get('DetachScript'),
				"initialize": m.get('InitScript'),
				"terminate": m.get('TermScript'),
				"getNetwork": m.get('NetworkScript'),
				"check": m.get('CheckStatusScript'),
				"dataEnable": m.get('DataEnableScript'),
				"dataDisable": m.get('DataDisableScript'),
			}
			self.interface = m.get('IF')
			self.cmd_prefix = m.get('CmdPrefix')
			logging.info(f'initialized UE {self} from {filename}')

			t = m.get('Tracing')
			self.trace = t is not None
			if self.trace:
				if t.get('Start') is None or t.get('Stop') is None or t.get('Collect')is None :
					raise ValueError("need to have Start/Stop/Collect for tracing")
				self.cmd_dict["traceStart"] = t.get('Start')
				self.cmd_dict["traceStop"] = t.get('Stop')
				self._logCollect = t.get('Collect')
				if "%%log_dir%%" not in self._logCollect:
					raise ValueError(f"(At least one) LogCollect expression for {module_name} must contain \"%%log_dir%%\"")

	def __str__(self):
		return f"{self.module_name}@{self.host}"

	def __repr__(self):
		return self.__str__()

	def _command(self, cmd, silent=False, reportNonZero=True):
		if cmd is None:
			raise Exception("no command provided")
		with cls_cmd.getConnection(self.host) as c:
			response = c.run(cmd, silent=silent, reportNonZero=reportNonZero)
		return response

#-----------------$
#PUBLIC Methods$
#-----------------$

	def initialize(self):
		# we first terminate to make sure the UE has been stopped
		if self.cmd_dict["detach"]:
			self._command(self.cmd_dict["detach"], silent=True)
		self._command(self.cmd_dict["terminate"], silent=True)
		ret = self._command(self.cmd_dict["initialize"])
		logging.info(f'For command: {ret.args} | return output: {ret.stdout} | Code: {ret.returncode}')
		if self.trace:
			self._enableTrace()
		# Here each UE returns differently for the successful initialization, requires check based on UE
		return ret.returncode == 0


	def terminate(self, ctx=None):
		self._command(self.cmd_dict["terminate"])
		if self.trace and ctx is not None:
			self._disableTrace()
			return self._collectTrace(ctx)
		return None

	def attach(self, attach_tries = 3, attach_timeout = 40):
		ip = None
		while attach_tries > 0:
			self._command(self.cmd_dict["attach"])
			timeout = attach_timeout
			logging.debug("Waiting for IP address to be assigned")
			ip = self.getIP(silent=False, reportNonZero=True)
			while timeout > 0 and not ip:
				time.sleep(1)
				timeout -= 1
				ip = self.getIP(silent=True, reportNonZero=False)
			if ip:
				break
			logging.warning(f"UE did not receive IP address after {attach_timeout} s, detaching")
			attach_timeout += 20
			attach_tries -= 1
			self._command(self.cmd_dict["detach"])
			time.sleep(5)
		if ip:
			logging.debug(f'\u001B[1mUE IP Address for UE {self.module_name} is {ip}\u001B[0m')
		else:
			logging.debug(f'\u001B[1;37;41mUE IP Address for UE {self.module_name} Not Found!\u001B[0m')
		return ip

	def detach(self):
		self._command(self.cmd_dict["detach"])

	def check(self):
		cmd = self.cmd_dict["check"]
		if cmd:
			return self._command(cmd).stdout
		else:
			logging.warning(f"requested status check of UE {self.getName()}, but operation is not supported")
			return f"UE {self.getName()} does not support status checking"

	def dataEnable(self):
		cmd = self.cmd_dict["dataEnable"]
		if cmd:
			self._command(cmd)
			return True
		else:
			message = f"requested enabling data of UE {self.getName()}, but operation is not supported"
			logging.error(message)
			return False

	def dataDisable(self):
		cmd = self.cmd_dict["dataDisable"]
		if cmd:
			self._command(cmd)
			return True
		else:
			message = f"requested disabling data of UE {self.getName()}, but operation is not supported"
			logging.error(message)
			return False

	def getIP(self, silent=True, reportNonZero=True):
		output = self._command(self.cmd_dict["getNetwork"], silent=silent, reportNonZero=reportNonZero)
		result = re.search(r'inet (?P<ip>[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+)', output.stdout)
		if result and result.group('ip'):
			ip = result.group('ip')
			return ip
		return None

	def getName(self):
		return self.module_name

	def getIFName(self):
		return self.interface

	def getHost(self):
		return self.host

	def getCmdPrefix(self):
		return self.cmd_prefix if self.cmd_prefix else ""

	def _enableTrace(self):
		logging.info(f'UE {self}: start UE tracing')
		self._command(self.cmd_dict["traceStart"])

	def _disableTrace(self):
		logging.info(f'UE {self}: stop UE tracing')
		self._command(self.cmd_dict["traceStop"])

	def _collectTrace(self, ctx):
		remote_dir = "/tmp/ue-trace-logs"
		with cls_cmd.getConnection(self.host) as c:
			# create a directory for log collection
			c.run(f'rm -rf {remote_dir}')
			ret = c.run(f'mkdir {remote_dir}')
			if ret.returncode != 0:
				logging.error("cannot create directory for log collection")
				return []
			log_cmd = self._logCollect.replace('%%log_dir%%', remote_dir)
			self._command(log_cmd)
			# enumerate collected files
			ret = c.run(f'ls {remote_dir}/*')
			if ret.returncode != 0:
				logging.error("cannot enumerate log files")
				return []
			log_files = []
			# copy them to the executor one by one, and store in log_dir
			for f in ret.stdout.split("\n"):
				name = archiveArtifact(c, ctx, f)
				log_files.append(name)
			c.run(f'rm -rf {remote_dir}')
			return log_files
