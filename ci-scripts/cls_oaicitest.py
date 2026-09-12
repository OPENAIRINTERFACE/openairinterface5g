# SPDX-License-Identifier: LicenseRef-CSSL-1.0

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
import re		# reg
import time		# sleep
import os
import logging
import concurrent.futures
import json

#import our libs
import constants as CONST

import cls_module
import cls_corenetwork
import cls_analysis
import cls_cmd
from cls_ci_helper import archiveArtifact

#-----------------------------------------------------------
# Helper functions used here and in other classes
#-----------------------------------------------------------
def Iperf_ComputeModifiedBW(idx, ue_num, profile, args):
	result = re.search(r'-b\s*(?P<iperf_bandwidth>[0-9\.]+)(?P<unit>[KMG])', str(args))
	if result is None:
		raise ValueError(f'requested iperf bandwidth not found in iperf options "{args}"')
	iperf_bandwidth = float(result.group('iperf_bandwidth'))
	if iperf_bandwidth == 0:
		raise ValueError('iperf bandwidth set to 0 - invalid value')
	if profile == 'balanced':
		iperf_bandwidth_new = iperf_bandwidth/ue_num
	if profile =='single-ue':
		iperf_bandwidth_new = iperf_bandwidth
	if profile == 'unbalanced':
		# residual is 2% of max bw
		residualBW = iperf_bandwidth / 50
		if idx == 0:
			iperf_bandwidth_new = iperf_bandwidth - ((ue_num - 1) * residualBW)
		else:
			iperf_bandwidth_new = residualBW
	iperf_bandwidth_str = result.group(0)
	iperf_bandwidth_unit = result.group(2)
	iperf_bandwidth_str_new = f"-b {'%.2f' % iperf_bandwidth_new}{iperf_bandwidth_unit}"
	args_new = re.sub(iperf_bandwidth_str, iperf_bandwidth_str_new, str(args))
	if iperf_bandwidth_unit == 'K':
		iperf_bandwidth_new = iperf_bandwidth_new / 1000
	return iperf_bandwidth_new, args_new

def Iperf_ComputeTime(args):
	result = re.search(r'-t\s*(?P<iperf_time>\d+)', str(args))
	if result is None:
		raise Exception('Iperf time not found!')
	return int(result.group('iperf_time'))

def Iperf_UpdateBindPort(opts, ueIP, idx):
	# search if bind address present. Extract if yes, add if no.
	bind_m = re.search(r'(-B|--bind)\s+(?P<ip>\d+\.\d+\.\d+\.\d+)', opts)
	if bind_m:
		bindIP = bind_m.group('ip')
	else:
		bindIP = ueIP
		opts += f" -B {ueIP}"
	# search if port present. Extract if yes, add if no.
	port_m = re.search(r'(-p|--port)\s+(?P<port>\d+)', opts)
	if port_m:
		port = port_m.group('port')
	else:
		port = 5002 + idx
		opts += f" -p {port}"
	return bindIP, port, opts

def convert_to_mbps(value, magnitude):
	value = float(value)
	if magnitude == 'K' or magnitude == 'k':
		return value / 1000
	elif magnitude == 'M':
		return value
	elif magnitude == 'G':
		return value * 1000
	else:
		return value

def extract_iperf_data(res):
	if not res:
		return None
	bitrate_val = res.group('bitrate')
	magnitude = res.group('magnitude')
	return {
		'bitrate_mbps': convert_to_mbps(bitrate_val, magnitude),
		'jitter': res.group('jitter'),
		'packetloss': res.group('packetloss'),
		'bitrate_disp': f'{float(bitrate_val):.2f} {magnitude}bps'
	}

def Iperf_analyzeV3TCPJson(filename, iperf_tcp_rate_target):
	try:
		with open(filename) as f:
			 results = json.load(f)
		sender_bitrate   = round(results['end']['streams'][0]['sender']['bits_per_second'] / 1000000, 2)
		receiver_bitrate = round(results['end']['streams'][0]['receiver']['bits_per_second'] / 1000000, 2)
	except json.JSONDecodeError as e:
		return (False, f'Could not decode JSON log file {filename}: {e}')
	except KeyError as e:
		e_msg = results.get('error', f'error report not found in {filename}')
		return (False, f'While parsing Iperf3 results: missing key {e}, {e_msg}')
	except Exception as e:
		return (False, f'While parsing Iperf3 results: exception: {e}')
	snd_msg = f'Sender Bitrate   : {sender_bitrate} Mbps'
	rcv_msg = f'Receiver Bitrate : {receiver_bitrate} Mbps'
	success = True
	if iperf_tcp_rate_target is not None:
		success = float(receiver_bitrate) >= float(iperf_tcp_rate_target)
		if success:
			rcv_msg += f" (target: {iperf_tcp_rate_target})"
		else:
			rcv_msg += f" (too low! < {iperf_tcp_rate_target})"
	return(success, f'{snd_msg}\n{rcv_msg}')

def Iperf_analyzeV3BIDIRJson(filename):
	try:
		with open(filename) as f:
			results = json.load(f)
		sender_bitrate_ul   = round(results['end']['streams'][0]['sender']['bits_per_second'] / 1000000, 2)
		receiver_bitrate_ul = round(results['end']['streams'][0]['receiver']['bits_per_second'] / 1000000, 2)
		sender_bitrate_dl   = round(results['end']['streams'][1]['sender']['bits_per_second'] / 1000000, 2)
		receiver_bitrate_dl = round(results['end']['streams'][1]['receiver']['bits_per_second'] / 1000000, 2)
	except json.JSONDecodeError as e:
		return (False, f'Could not decode JSON log file: {e}')
	except KeyError as e:
		e_msg = results.get('error', f'error report not found in {filename}')
		return (False, f'While parsing Iperf3 results: missing key {e}, {e_msg}')
	except Exception as e:
		return (False, f'While parsing Iperf3 results: exception: {e}')

	msg = f'Sender Bitrate DL   : {sender_bitrate_dl} Mbps\n'
	msg += f'Receiver Bitrate DL : {receiver_bitrate_dl} Mbps\n'
	msg += f'Sender Bitrate UL   : {sender_bitrate_ul} Mbps\n'
	msg += f'Receiver Bitrate UL : {receiver_bitrate_ul} Mbps\n'
	return (True, msg)

def Iperf_analyzeV3UDP(filename, iperf_bitrate_threshold, iperf_packetloss_threshold, target_bitrate):
	if not os.path.isfile(filename):
		return (False, 'Iperf3 UDP: Log file not present')
	if os.path.getsize(filename) == 0:
		return (False, 'Iperf3 UDP: Log file is empty')

	sender_data = None
	receiver_data = None
	with open(filename, 'r') as server_file:
		for line in server_file:
			res_sender = re.search(r'(?P<bitrate>[0-9\.]+)\s+(?P<magnitude>[kKMG]?)bits\/sec\s+(?P<jitter>[0-9\.]+\s+ms)\s+(?P<lostPack>-?\d+)/(?P<sentPack>-?\d+)\s+\((?P<packetloss>[0-9\.eE\-\+]+).*?\s+(sender)', line)
			res_receiver = re.search(r'(?P<bitrate>[0-9\.]+)\s+(?P<magnitude>[kKMG]?)bits\/sec\s+(?P<jitter>[0-9\.]+\s+ms)\s+(?P<lostPack>-?\d+)/(?P<receivedPack>-?\d+)\s+\((?P<packetloss>[0-9\.eE\-\+]+)%\).*?(receiver)', line)
			if res_sender:
				sender_data = extract_iperf_data(res_sender)
			if res_receiver:
				receiver_data = extract_iperf_data(res_receiver)
	if not sender_data or not receiver_data:
		return (False, 'Could not analyze iperf report')

	br_perf = 100 * receiver_data['bitrate_mbps'] / float(target_bitrate)
	br_perf_str = f'{br_perf:.2f}%'
	req_msg = f"Sender Bitrate  : {sender_data['bitrate_disp']}"
	bir_msg = f"Receiver Bitrate: {receiver_data['bitrate_disp']} ({br_perf_str})"
	jit_msg = f"Jitter          : {receiver_data['jitter']}"
	pal_msg = f"Packet Loss     : {receiver_data['packetloss']}%"
	if br_perf < float(iperf_bitrate_threshold):
		bir_msg += f' (too low! < {iperf_bitrate_threshold}%)'
	if float(receiver_data['packetloss']) > float(iperf_packetloss_threshold):
		pal_msg += f' (too high! > {iperf_packetloss_threshold}%)'
	result = br_perf >= float(iperf_bitrate_threshold) and float(receiver_data['packetloss']) <= float(iperf_packetloss_threshold)
	return (result, f'{req_msg}\n{bir_msg}\n{jit_msg}\n{pal_msg}')

def Iperf_analyzeV2UDP(server_filename, iperf_bitrate_threshold, iperf_packetloss_threshold, target_bitrate):
		result = None
		if (not os.path.isfile(server_filename)):
			return (False, 'Iperf UDP: Server report not found!')
		if (os.path.getsize(server_filename)==0):
			return (False, 'Iperf UDP: Log file is empty')
		statusTemplate = r'(?:|\[ *\d+\].*) +0\.0+-\s*(?P<duration>[0-9\.]+) +sec +[0-9\.]+ [kKMG]Bytes +(?P<bitrate>[0-9\.]+) (?P<magnitude>[kKMG])bits\/sec +(?P<jitter>[0-9\.]+) ms +(\d+\/ *\d+) +(\((?P<packetloss>[0-9\.]+)%\))'
		with open(server_filename, 'r') as server_file:
			for line in server_file.readlines():
				result = re.search(statusTemplate, str(line)) or result
		if result is None:
			return (False, 'Could not parse server report!')
		bitrate_val = float(result.group('bitrate'))
		magnitude = result.group('magnitude')
		bitrate = convert_to_mbps(bitrate_val, magnitude)
		jitter = float(result.group('jitter'))
		packetloss = float(result.group('packetloss'))
		br_perf = float(bitrate)/float(target_bitrate) * 100
		br_perf = '%.2f ' % br_perf

		result = float(br_perf) >= float(iperf_bitrate_threshold) and float(packetloss) <= float(iperf_packetloss_threshold)
		req_msg = f'Req Bitrate : {target_bitrate}'
		bir_msg = f'Bitrate     : {bitrate}'
		brl_msg = f'Bitrate Perf: {br_perf} %'
		if float(br_perf) < float(iperf_bitrate_threshold):
			brl_msg += f' (too low! <{iperf_bitrate_threshold}%)'
		jit_msg = f'Jitter      : {jitter}'
		pal_msg = f'Packet Loss : {packetloss}'
		if float(packetloss) > float(iperf_packetloss_threshold):
			pal_msg += f' (too high! >{iperf_packetloss_threshold}%)'
		return (result, f'{req_msg}\n{bir_msg}\n{brl_msg}\n{jit_msg}\n{pal_msg}')

def Custom_Command(HTML, node, command):
    logging.info(f"Executing custom command on {node}")
    cmd = cls_cmd.getConnection(node)
    ret = cmd.run(command)
    cmd.close()
    logging.debug(f"Custom_Command: {command} on node: {node} - {'OK, command succeeded' if ret.returncode == 0 else f'Error, return code: {ret.returncode}'}")
    status = 'OK'
    message = []
    if ret.returncode != 0:
        message = [ret.stdout]
        logging.error(f'Custom_Command failed: output: {message}')
        status = 'KO'
    HTML.CreateHtmlTestRowQueue(command, status, message)
    return status == 'OK' or status == 'Warning'

def Custom_Script(HTML, node, script, args):
	logging.info(f"Executing custom script on {node}")
	with cls_cmd.getConnection(node) as c:
		ret = c.exec_script(script, 90, args)
	logging.debug(f"Custom_Script: {script} on node: {node} - return code {ret.returncode}, output:\n{ret.stdout}")
	status = 'OK'
	message = [ret.stdout]
	if ret.returncode != 0:
		status = 'KO'
	HTML.CreateHtmlTestRowQueue(script, status, message)
	return status == 'OK' or status == 'Warning'

def IdleSleep(HTML, idle_sleep_time):
	logging.debug(f"sleep for {idle_sleep_time} seconds")
	time.sleep(idle_sleep_time)
	HTML.CreateHtmlTestRow(f"{idle_sleep_time} sec", 'OK', CONST.ALL_PROCESSES_OK)
	return True

def Deploy_Physim(ctx, HTML, node, workdir, script, options):
	logging.debug(f'Running physims on server {node} workdir {workdir}')
	with cls_cmd.getConnection(node) as c:
		sys_info = c.exec_script("scripts/sys-info.sh", 5)
		ret = c.exec_script(script, 1500, options)
	logging.debug(f'"{script}" finished with code {ret.returncode}, output:\n{ret.stdout}')
	HTML.CreateHtmlTestRowQueue('Query system info', 'OK', [sys_info.stdout])
	with cls_cmd.getConnection(node) as ssh:
		details_json = archiveArtifact(ssh, ctx, f'{workdir}/desc-tests.json')
		result_junit = archiveArtifact(ssh, ctx, f'{workdir}/results-run.xml')
		archiveArtifact(ssh, ctx, f'{workdir}/physim_log.txt')
		archiveArtifact(ssh, ctx, f'{workdir}/LastTestsFailed.log')
		archiveArtifact(ssh, ctx, f'{workdir}/LastTest.log')
	test_status, test_summary, test_result = cls_analysis.Analysis.analyze_physim(result_junit, details_json, ctx.logPath)
	if test_summary:
		if test_status:
			HTML.CreateHtmlTestRow('N/A', 'OK', CONST.ALL_PROCESSES_OK)
			HTML.CreateHtmlTestRowPhySimTestResult(test_summary, test_result)
			logging.info('\u001B[1m Physical Simulator Pass\u001B[0m')
		else:
			HTML.CreateHtmlTestRowQueue('At least one physical simulator test failed!', 'KO', ["See below for details"])
			HTML.CreateHtmlTestRowPhySimTestResult(test_summary, test_result)
			logging.error('\u001B[1m Physical Simulator Fail\u001B[0m')
	else:
		HTML.CreateHtmlTestRowQueue('Physical simulator failed', 'KO', [test_result])
		logging.error('\u001B[1m Physical Simulator Fail\u001B[0m')
	return test_status

def DeployWithScript(HTML, node, script, options, tag):
	logging.debug(f'Deploy with script {script} on node: {node}')
	opt = options.replace('%%image_tag%%', tag)
	with cls_cmd.getConnection(node) as c:
		ret = c.exec_script(script, 600, opt)
	logging.debug(f'"{script}" finished with code {ret.returncode}, output:\n{ret.stdout}')
	HTML.CreateHtmlTestRowQueue(f'on node {node}', 'OK' if ret.returncode == 0 else 'KO', [f'{ret.stdout}'])
	return ret.returncode == 0

def UndeployWithScript(HTML, ctx, node, script, options):
	logging.debug(f'Undeploy with script {script} on node: {node}')
	remote_dir = '/tmp/undeploy'
	opt = options.replace('%%log_dir%%', remote_dir)
	with cls_cmd.getConnection(node) as c:
		# create a directory for log collection
		c.run(f'rm -rf {remote_dir}')
		ret = c.run(f'mkdir {remote_dir}')
		if ret.returncode != 0:
			logging.error("cannot create directory for log collection")
			return False
		ret = c.exec_script(script, 600, opt)
		logging.debug(f'"{script}" finished with code {ret.returncode}, output:\n{ret.stdout}')
		ret_ls = c.run(f'ls -1 {remote_dir}')
		files = ret_ls.stdout.strip().splitlines()
		log_files = []
		for lf in files:
			name = archiveArtifact(c, ctx, f'{remote_dir}/{lf}')
			log_files.append(name)
	msg = "Log files:\n" + "\n".join([os.path.basename(lf) for lf in log_files])
	HTML.CreateHtmlTestRowQueue(f'on node {node}', 'OK' if ret.returncode == 0 else 'KO', [f'{ret.stdout}\n\n{msg}'])
	return ret.returncode == 0

#-----------------------------------------------------------
# OaiCiTest Class Definition
#-----------------------------------------------------------
class OaiCiTest():
	
	def __init__(self):
		self.ping_args = ''
		self.ping_packetloss_threshold = ''
		self.ping_rttavg_threshold =''
		self.iperf_args = ''
		self.iperf_packetloss_threshold = ''
		self.iperf_bitrate_threshold = ''
		self.iperf_profile = ''
		self.iperf_tcp_rate_target = ''
		self.ue_ids = []
		self.svr_node = None
		self.svr_id = None

	def InitializeUE(self, node, HTML):
		ues = [cls_module.Module_UE(n.strip(), node) for n in self.ue_ids]
		messages = []
		with concurrent.futures.ThreadPoolExecutor(max_workers=64) as executor:
			futures = [executor.submit(ue.initialize) for ue in ues]
			for f, ue in zip(futures, ues):
				uename = f'UE {ue.getName()}'
				messages.append(f'{uename}: initialized' if f.result() else f'{uename}: ERROR during Initialization')
			[f.result() for f in futures]
		HTML.CreateHtmlTestRowQueue('N/A', 'OK', messages)
		return True

	def AttachUE(self, node, HTML):
		ues = [cls_module.Module_UE(ue_id, node) for ue_id in self.ue_ids]
		with concurrent.futures.ThreadPoolExecutor(max_workers=64) as executor:
			futures = [executor.submit(ue.attach) for ue in ues]
			attached = [f.result() for f in futures]
			futures = [executor.submit(ue.checkMTU) for ue in ues]
			mtus = [f.result() for f in futures]
			messages = [f"UE {ue.getName()}: {ue.getIP()}" for ue in ues]
		success = all(attached) and all(mtus)
		if success:
			HTML.CreateHtmlTestRowQueue('N/A', 'OK', messages)
		else:
			logging.error(f'error attaching or wrong MTU: attached {attached}, mtus {mtus}')
			HTML.CreateHtmlTestRowQueue('N/A', 'KO', ["Could not retrieve UE IP address(es) or MTU(s) wrong!"])
		return success

	def DetachUE(self, node, HTML):
		ues = [cls_module.Module_UE(ue_id, node) for ue_id in self.ue_ids]
		with concurrent.futures.ThreadPoolExecutor(max_workers=64) as executor:
			futures = [executor.submit(ue.detach) for ue in ues]
			[f.result() for f in futures]
			messages = [f"UE {ue.getName()}: detached" for ue in ues]
		HTML.CreateHtmlTestRowQueue('NA', 'OK', messages)
		return True

	def DataDisableUE(self, node, HTML):
		ues = [cls_module.Module_UE(n.strip(), node) for n in self.ue_ids]
		with concurrent.futures.ThreadPoolExecutor(max_workers=64) as executor:
			futures = [executor.submit(ue.dataDisable) for ue in ues]
			status = [f.result() for f in futures]
		success = all(status)
		if success:
			messages = [f"UE {ue.getName()}: data disabled" for ue in ues]
			HTML.CreateHtmlTestRowQueue('NA', 'OK', messages)
		else:
			logging.error(f'error enabling data: {status}')
			HTML.CreateHtmlTestRowQueue('N/A', 'KO', ["Could not disable UE data!"])
		return success

	def DataEnableUE(self, node, HTML):
		ues = [cls_module.Module_UE(n.strip(), node) for n in self.ue_ids]
		logging.debug(f'disabling data for UEs {ues}')
		with concurrent.futures.ThreadPoolExecutor(max_workers=64) as executor:
			futures = [executor.submit(ue.dataEnable) for ue in ues]
			status = [f.result() for f in futures]
		success = all(status)
		if success:
			messages = [f"UE {ue.getName()}: data enabled" for ue in ues]
			HTML.CreateHtmlTestRowQueue('NA', 'OK', messages)
		else:
			logging.error(f'error enabling data: {status}')
			HTML.CreateHtmlTestRowQueue('N/A', 'KO', ["Could not enable UE data!"])
		return success

	def CheckStatusUE(self, node, HTML):
		ues = [cls_module.Module_UE(n.strip(), node) for n in self.ue_ids]
		logging.debug(f'checking status of UEs {ues}')
		messages = []
		with concurrent.futures.ThreadPoolExecutor(max_workers=64) as executor:
			futures = [executor.submit(ue.check) for ue in ues]
			messages = [f.result() for f in futures]
		HTML.CreateHtmlTestRowQueue('NA', 'OK', messages)
		return True

	def Ping_common(self, ctx, cn, ue):
		ping_status = 0
		ueIP = ue.getIP()
		if not ueIP:
			return (False, f"UE {ue.getName()} has no IP address")
		svrIP = cn.getIP()
		if not svrIP:
			return (False, f"CN {cn.getName()} has no IP address")
		ping_log_file = f'/tmp/ping_{ue.getName()}.log'
		ping_time = re.findall(r"-c *(\d+)",str(self.ping_args))
		if re.search('%cn_ip%', self.ping_args) or re.search(r'[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+', self.ping_args):
			raise Exception(f"ping_args should not have IP address: {self.ping_args}")
		interface = f'-I {ue.getIFName()}' if ue.getIFName() else ''
		ping_cmd = f'{ue.getCmdPrefix()} ping {interface} {self.ping_args} {svrIP} 2>&1 | tee {ping_log_file}'
		cmd = cls_cmd.getConnection(ue.getHost())
		response = cmd.run(ping_cmd, timeout=int(ping_time[0])*1.5)
		ue_header = f'UE {ue.getName()} ({ueIP})'
		if response.returncode != 0:
			message = ue_header + ': ping crashed: TIMEOUT?'
			return (False, message)

		local_ping_log_file = archiveArtifact(cmd, ctx, ping_log_file)
		cmd.close()

		with open(local_ping_log_file, 'r') as f:
			ping_output = "".join(f.readlines())
		result = re.search(r', (?P<packetloss>[0-9\.]+)% packet loss, time [0-9\.]+ms', ping_output)
		if result is None:
			message = ue_header + ': Packet Loss Not Found!'
			return (False, message)
		packetloss = result.group('packetloss')
		result = re.search(r'rtt min\/avg\/max\/mdev = (?P<rtt_min>[0-9\.]+)\/(?P<rtt_avg>[0-9\.]+)\/(?P<rtt_max>[0-9\.]+)\/[0-9\.]+ ms', ping_output)
		if result is None:
			message = ue_header + ': Ping RTT_Min RTT_Avg RTT_Max Not Found!'
			return (False, message)
		rtt_min = result.group('rtt_min')
		rtt_avg = result.group('rtt_avg')
		rtt_max = result.group('rtt_max')

		pal_msg = f'Packet Loss: {packetloss}%'
		min_msg = f'RTT(Min)   : {rtt_min} ms'
		avg_msg = f'RTT(Avg)   : {rtt_avg} ms'
		max_msg = f'RTT(Max)   : {rtt_max} ms'

		message = f'{ue_header}\n{pal_msg}\n{min_msg}\n{avg_msg}\n{max_msg}'

		#checking packet loss compliance
		if float(packetloss) > float(self.ping_packetloss_threshold):
			message += '\nPacket Loss too high'
			return (False, message)
		elif float(packetloss) > 0:
			message += '\nPacket Loss is not 0%'

		if self.ping_rttavg_threshold != '':
			if float(rtt_avg) > float(self.ping_rttavg_threshold):
				ping_rttavg_error_msg = f'RTT(Avg) too high: {rtt_avg} ms; Target: {self.ping_rttavg_threshold} ms'
				message += f'\n {ping_rttavg_error_msg}'
				return (False, message)

		return (True, message)

	def Ping(self, ctx, node, HTML, infra_file="ci_infra.yaml"):
		if self.ue_ids == [] or self.svr_id == None:
			raise Exception("no module names in self.ue_ids or/and self.svr_id provided")
		ues = [cls_module.Module_UE(ue_id, node, infra_file) for ue_id in self.ue_ids]
		cn = cls_corenetwork.CoreNetwork(self.svr_id, self.svr_node, filename=infra_file)
		with concurrent.futures.ThreadPoolExecutor(max_workers=64) as executor:
			futures = [executor.submit(self.Ping_common, ctx, cn, ue) for ue in ues]
			results = [f.result() for f in futures]
			# each result in results is a tuple, first member goes to successes, second to messages
			successes, messages = map(list, zip(*results))

		success = len(successes) == len(ues) and all(successes)
		logger = logging.info if success else logging.error
		hcolor = "\u001B[1;37;44m" if success else "\u001B[1;37;41m"
		lcolor = "\u001B[1;34m" if success else "\u001B[1;31m"
		for m in messages:
			lines = m.split('\n')
			logger(f'{hcolor} ping result for {lines[0]} \u001B[0m')
			for l in lines[1:]:
				logger(f'{lcolor}    {l}\u001B[0m')

		if success:
			HTML.CreateHtmlTestRowQueue(self.ping_args, 'OK', messages)
		else:
			HTML.CreateHtmlTestRowQueue(self.ping_args, 'KO', messages)
		return success

	def Iperf_Module(self, ctx, cn, ue, idx, ue_num):
		ueIP = ue.getIP()
		if not ueIP:
			return (False, f"UE {ue.getName()} has no IP address")
		svrIP = cn.getIP()
		if not svrIP:
			return (False, f"Iperf server {cn.getName()} has no IP address")
		iperf_opt = self.iperf_args
		jsonReport = "--json"
		serverReport = ""
		udpIperf = re.search('-u', iperf_opt) is not None
		bidirIperf = re.search('--bidir', iperf_opt) is not None
		client_filename = f'/tmp/iperf_client_{ue.getName()}.log'
		if udpIperf:
			target_bitrate, iperf_opt = Iperf_ComputeModifiedBW(idx, ue_num, self.iperf_profile, self.iperf_args)
			# note: for UDP testing we don't want to use json report - reports 0 Mbps received bitrate
			jsonReport = ""
			# note: enable server report collection on the UE side, no need to store and collect server report separately on the server side
			serverReport = "--get-server-output"
		iperf_time = Iperf_ComputeTime(self.iperf_args)
		bindIP, port, iperf_opt = Iperf_UpdateBindPort(iperf_opt, ueIP, idx)
		# hack: the ADB UEs don't have iperf in $PATH, so we need to hardcode for the moment
		iperf_ue = '/data/local/tmp/iperf3' if re.search('adb', ue.getName()) else 'iperf3'
		ue_header = f'UE {ue.getName()} ({bindIP})'
		with cls_cmd.getConnection(ue.getHost()) as cmd_ue, cls_cmd.getConnection(cn.getHost()) as cmd_svr:
			# note: some core setups start an iperf3 server automatically, indicated in ci_infra by runIperf3Server: False`
			t = iperf_time * 2.5
			cmd_ue.run(f'rm {client_filename}', reportNonZero=False, silent=True)
			if cn.runIperf3Server():
				# Clean up any existing iperf3 server processes on this port.
				ret = cmd_svr.run(f"{cn.getCmdPrefix()} pkill -f '.*iperf3.*{port}'", reportNonZero=False)
				# If pkill succeeds, it means there was a leftover iperf3 server.
				if ret.returncode == 0:
					logging.warning(f'Iperf3 server on port {port} detected and terminated')
				cmd_svr.run(f'{cn.getCmdPrefix()} timeout -vk3 {t} iperf3 -s -B {svrIP} -p {port} -1 {jsonReport} >> /dev/null &', timeout=t)
			client_ret = cmd_ue.run(f'{ue.getCmdPrefix()} timeout -vk3 {t} {iperf_ue} -c {svrIP} {iperf_opt} {jsonReport} {serverReport} -O 5 >> {client_filename}', timeout=t, reportNonZero=False)
			dest_filename = archiveArtifact(cmd_ue, ctx, client_filename)
		if udpIperf:
			status, msg = Iperf_analyzeV3UDP(dest_filename, self.iperf_bitrate_threshold, self.iperf_packetloss_threshold, target_bitrate)
		elif bidirIperf:
			status, msg = Iperf_analyzeV3BIDIRJson(dest_filename)
		else:
			status, msg = Iperf_analyzeV3TCPJson(dest_filename, self.iperf_tcp_rate_target)

		# add some diagnostic messages if the actual iperf command returned non-zero
		if client_ret.returncode != 0:
			msg = f'{msg}\nIperf client command failed on {bindIP} -> {svrIP}:{port} (return code: {client_ret.returncode})'

		return (status, f'{ue_header}\n{msg}')

	def Iperf(self, ctx, node, HTML, infra_file="ci_infra.yaml"):
		logging.debug(f'Iperf: iperf_args "{self.iperf_args}" iperf_packetloss_threshold "{self.iperf_packetloss_threshold}" iperf_bitrate_threshold "{self.iperf_bitrate_threshold}" iperf_profile "{self.iperf_profile}"')

		if self.ue_ids == [] or self.svr_id == None:
			raise Exception("no module names in self.ue_ids or/and self.svr_id provided")
		ues = [cls_module.Module_UE(ue_id, node, infra_file) for ue_id in self.ue_ids]
		cn = cls_corenetwork.CoreNetwork(self.svr_id, self.svr_node, filename=infra_file)
		with concurrent.futures.ThreadPoolExecutor(max_workers=64) as executor:
			futures = [executor.submit(self.Iperf_Module, ctx, cn, ue, i, len(ues)) for i, ue in enumerate(ues)]
			results = [f.result() for f in futures]
			# each result in results is a tuple, first member goes to successes, second to messages
			successes, messages = map(list, zip(*results))

		success = len(successes) == len(ues) and all(successes)
		logger = logging.info if success else logging.error
		hcolor = "\u001B[1;37;45m" if success else "\u001B[1;37;41m"
		lcolor = "\u001B[1;35m" if success else "\u001B[1;31m"
		for m in messages:
			lines = m.split('\n')
			logger(f'{hcolor} iperf result for {lines[0]} \u001B[0m')
			for l in lines[1:]:
				logger(f'{lcolor}    {l}\u001B[0m')

		if success:
			HTML.CreateHtmlTestRowQueue(self.iperf_args, 'OK', messages)
		else:
			HTML.CreateHtmlTestRowQueue(self.iperf_args, 'KO', messages)
		return success

	def Iperf2_Unidir(self, ctx, node, HTML, infra_file="ci_infra.yaml"):
		if self.ue_ids == [] or self.svr_id == None or len(self.ue_ids) != 1:
			raise Exception("no module names in self.ue_ids or/and self.svr_id provided, multi UE scenario not supported")
		ue = cls_module.Module_UE(self.ue_ids[0].strip(), node, infra_file)
		cn = cls_corenetwork.CoreNetwork(self.svr_id, self.svr_node, filename=infra_file)
		ueIP = ue.getIP()
		if not ueIP:
			return False
		svrIP = cn.getIP()
		if not svrIP:
			return False
		server_filename = f'/tmp/iperf_server_{ue.getName()}.log'
		iperf_time = Iperf_ComputeTime(self.iperf_args)
		target_bitrate, iperf_opt = Iperf_ComputeModifiedBW(0, 1, self.iperf_profile, self.iperf_args)
		t = iperf_time*2.5
		with cls_cmd.getConnection(ue.getHost()) as cmd_ue, cls_cmd.getConnection(cn.getHost()) as cmd_svr:
			cmd_ue.run(f'rm {server_filename}', reportNonZero=False)
			cmd_ue.run(f'{ue.getCmdPrefix()} timeout -vk3 {t} iperf -B {ueIP} -s -u -i1 >> {server_filename} &', timeout=t)
			cmd_svr.run(f'{cn.getCmdPrefix()} timeout -vk3 {t} iperf -c {ueIP} -B {svrIP} {iperf_opt} -i1 >> /dev/null', timeout=t)
			localPath = f'{os.getcwd()}'
			local = archiveArtifact(cmd_ue, ctx, server_filename)
		success, msg = Iperf_analyzeV2UDP(local, self.iperf_bitrate_threshold, self.iperf_packetloss_threshold, target_bitrate)
		ue_header = f'UE {ue.getName()} ({ueIP})'
		logging.info(f'\u001B[1;37;45m iperf result for {ue_header}\u001B[0m')
		for l in msg.split('\n'):
			logging.info(f'\u001B[1;35m	{l} \u001B[0m')
		if success:
			HTML.CreateHtmlTestRowQueue(self.iperf_args, 'OK', [f'{ue_header}\n{msg}'])
		else:
			HTML.CreateHtmlTestRowQueue(self.iperf_args, 'KO', [f'{ue_header}\n{msg}'])
		return success

	def TerminateUE(self, ctx, node, HTML):
		ues = [cls_module.Module_UE(n.strip(), node) for n in self.ue_ids]
		with concurrent.futures.ThreadPoolExecutor(max_workers=64) as executor:
			futures = [executor.submit(ue.terminate, ctx) for ue in ues]
			archives = [f.result() for f in futures]
		archive_info = [f'Log at: {a}' if a else 'No log available' for a in archives]
		messages = [f"UE {ue.getName()}: {log}" for (ue, log) in zip(ues, archive_info)]
		HTML.CreateHtmlTestRowQueue(f'N/A', 'OK', messages)
		return True

	def DeployCoreNetwork(cn_id, ctx, HTML):
		core_name = cn_id.strip()
		cn = cls_corenetwork.CoreNetwork(core_name)
		success, output = cn.deploy()
		logging.info(f"deployment core network {core_name} success {success}, output:\n{output}")
		if success:
			msg = f"Started {cn} [{cn.getIP()}]"
			HTML.CreateHtmlTestRowQueue(core_name, 'OK', [msg])
		else:
			msg = f"deployment of core network {core_name} FAILED"
			logging.error(msg)
			HTML.CreateHtmlTestRowQueue(core_name, 'KO', [msg])
		return success

	def UndeployCoreNetwork(cn_id, ctx, HTML):
		core_name = cn_id.strip()
		cn = cls_corenetwork.CoreNetwork(core_name)
		logs, output = cn.undeploy(ctx=ctx)
		logging.info(f"undeployed core network {core_name}, logs {logs}, output:\n{output}")
		message = "Log files:\n" + "\n".join([os.path.basename(l) for l in logs])
		HTML.CreateHtmlTestRowQueue(core_name, 'OK', [message])
		return True
