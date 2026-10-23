# SPDX-License-Identifier: LicenseRef-CSSL-1.0

import re
import os
import logging
import yaml
import signal

import xml.etree.ElementTree as ET
import json

import importlib, inspect

class Analysis():

	def _get_test_description(properties):
		env_vars = None
		for p in properties:
			if p["name"] == "ENVIRONMENT":
				env_vars = p["value"] # save for later if no custom property
			if p["name"] == "TEST_DESCRIPTION":
				return p["value"]
		# if we came till here, it means there is no custom test property
		# saved in JSON.  See if we have a description in environment variables
		if not env_vars:
			return "<none>"
		for ev in env_vars:
			name, value = ev.split("=", 1)
			if name == "TEST_DESCRIPTION":
				return value
		return "<none>"

	def analyze_physim(result_junit, details_json, logPath):
		try:
			tree = ET.parse(result_junit)
			root = tree.getroot()
			nb_tests = int(root.attrib["tests"])
			nb_failed = int(root.attrib["failures"])
		except ET.ParseError as e:
			return False, False, f'Could not parse XML log file {result_junit}: {e}'
		except FileNotFoundError as e:
			return False, False, f'JUnit XML log file {result_junit} not found: {e}'
		except Exception as e:
			return False, False, f'While parsing JUnit XML log file: exception: {e}'

		try:
			with open(details_json) as f:
				j = json.load(f)
			# prepare JSON for easier access of strings
			json_test_desc = {}
			for e in j["tests"]:
				json_test_desc[e["name"]] = e

		except json.JSONDecodeError as e:
			return False, False, f'Could not decode JSON log file {details_json}: {e}'
		except FileNotFoundError as e:
			return False, False, f'Physim JSON log file {details_json} not found: {e}'
		except Exception as e:
			return False, False, f'While parsing physim JSON log file: exception: {e}'

		test_result = {}
		for test in root: # for each test
			test_name = test.attrib["name"]
			test_exec = json_test_desc[test_name]["properties"][1]["value"][0]
			desc = Analysis._get_test_description(json_test_desc[test_name]["properties"])
			# get runtime and checks
			test_check = test.attrib["status"] == "run"
			time = round(float(test.attrib["time"]), 1)
			time_check = time < 150
			output = test.findtext("system-out")
			output_check = "exceeds the threshold" not in output
			# collect logs
			log_dir = f'{logPath}/{test_exec}'
			os.makedirs(log_dir, exist_ok=True)
			with open(f'{log_dir}/{test_name}.log', 'w') as f:
				f.write(output)
			# prepare result and info
			resultstr = 'PASS' if (test_check and time_check and output_check) else 'FAIL'
			info = f"{test_name}.log: test {resultstr} (in {time}s)"
			for l in output.splitlines():
				if l.startswith("CHECK "):
					info += f"\n{l}"
			if test_check:
				if not output_check:
					info += "\nTest log exceeds maximal allowed length 100 kB"
				if not time_check:
					info += "\nTest exceeds 150s"
				if not (time_check and output_check):
					nb_failed += 1 # time threshold/output length error, not counted for by ctest as of now
			test_result[test_name] = [desc, info, resultstr]

		test_summary = {}
		test_summary['Nbtests'] = nb_tests
		test_summary['Nbpass'] =  nb_tests - nb_failed
		test_summary['Nbfail'] =  nb_failed
		return nb_failed == 0, test_summary, test_result

	def analyze_rt_stats(thresholds, stat_files):
		with open(thresholds, 'r') as f:
			datalog_rt_stats = yaml.load(f, Loader=yaml.FullLoader)

		rt_keys = datalog_rt_stats['Ref']
		real_time_stats = {}
		for sf in stat_files:
			with open(sf, 'r') as f:
				for line in f.readlines():
					for k in rt_keys:
						result = re.search(k, line)
						if result is not None:
							tmp = re.match(rf'^.*?(\b{k}\b.*)', line.rstrip())
							if tmp is not None:
								real_time_stats[k] = tmp.group(1)

		# datalog_rt_stats format must align with HTML.CreateHtmlDataLogTable()
		datalog_rt_stats['Data']={}
		for k in real_time_stats:
			tmp = re.match(r'^(?P<metric>.*):\s+(?P<avg>\d+\.\d+) us;\s+(?P<max>\d+\.\d+) us;\s+(?P<std_dev>\d+\.\d+) us;\s+(?P<count>\d+);', real_time_stats[k])
			if tmp is not None:
				metric = tmp.group('metric')
				avg = float(tmp.group('avg'))
				max = float(tmp.group('max'))
				count = int(tmp.group('count'))
				datalog_rt_stats['Data'][metric] = ["{:.0f}".format(avg),"{:.0f}".format(max),"{:d}".format(count),"{:.2f}".format(avg / datalog_rt_stats['Ref'][metric])]

		success = True
		for k in datalog_rt_stats['Data']:
			valnorm = float(datalog_rt_stats['Data'][k][3])
			dev = datalog_rt_stats['DeviationThreshold'][k]
			if valnorm > 1.0 + dev or valnorm < 1.0 - dev: # condition for fail: avg/ref deviates by more than "deviation threshold"
				logging.debug(f'\u001B[1;30;43m normalized metric {k}={valnorm} deviates by more than {dev}\u001B[0m')
				success = False
		return success, datalog_rt_stats


# returns tuple of (service, analyzer, option string)
def _lookupServiceAnalyzerOpt(s, analyzers):
	res = s.split("=", 2)
	name = res[0]
	if len(res) == 1:
		return res[0], analyzers["Default"], None
	opt = res[2] if len(res) > 2 else None
	func = res[1]
	a = analyzers[func] if func in analyzers else None
	return name, a, opt

# groups requested service analysis (service=func[=option]) on per service
# basis and looks up analyzer func. If log analysis is requested, will always
# run Default log analysis
def _groupServices(to_analyze):
	req_analysis = {}
	# get content of cls_loganalysis module, then get all analyzers (classes) in this module
	mod = importlib.import_module("cls_loganalysis")
	analyzers = {name:cl for name, cl in inspect.getmembers(mod, inspect.isclass)}
	for req in to_analyze:
		s, func, opt = _lookupServiceAnalyzerOpt(req, analyzers)
		logging.debug(f"requested check '{req}' => service {s}, function {func.__name__}, options '{opt}'")
		# always put default analyzer first
		l = req_analysis[s] if s in req_analysis else [(analyzers["Default"], None, "default")]
		if func is not analyzers["Default"]:
			l.append((func, opt, req))
		req_analysis[s] = l
	return req_analysis

def _describe_exit_code(code):
	if code > 128:
		sig = code - 128
		try:
			return f"terminated by signal {sig} ({signal.Signals(sig).name})"
		except ValueError:
			return f"terminated by unknown signal {sig}"
	else:
		return f"custom exit code"

def AnalyzeServices(HTML, service_desc, to_analyze):
	success = True
	# hack: we want to give as a description "log analysis", but the description
	# is set outside, so retain what was set before
	orig_html_desc = HTML.desc
	# group analysis on a per-service basis, then iterate
	for serv, list_analysis in _groupServices(to_analyze).items():
		HTML.desc = f"Log analysis for service {serv}"
		if serv not in service_desc:
			success = False
			logging.error(f"requested service {serv} not in list of services")
			HTML.CreateHtmlTestRowQueue("N/A", 'KO', ["service not detected"])
			continue
		# pre-initialize with return code
		rc = service_desc[serv]["returncode"]
		logging.info(f"analyze service {serv}: return code {rc}")
		service_success = True # TODO rc == 0: too many functions (eNB, lteUE, RIC, nrUE) fail non-zero
		logs = []
		if rc != 0:
			logs.append(f"=> return code {rc}, likely " + _describe_exit_code(rc) + " [ignored by CI]")
		# skip if the file is too big
		logfile = service_desc[serv]["logfile"]
		filename = os.path.basename(logfile)
		b = os.path.getsize(logfile)
		logging.debug(f"using logfile {logfile} of size {b} bytes")
		if b > 10 * 1024 * 1024:
			success = False
			logs.append(f"logfile too big (>10MB)")
			logging.error(logs)
			HTML.CreateHtmlTestRowQueue(filename, 'KO', ["\n".join(logs)])
			continue
		# run each analyzer with its options on the logfile
		for (func, opt, desc) in list_analysis:
			if func is None:
				service_success = False
				logging.error(f"request analysis function for desc {desc} not found")
				HTML.CreateHtmlTestRowQueue(serv, 'KO', [f"no analysis function for {desc}"])
				continue
			result, l = func.run(logfile, opt)
			logging.info(f"service {serv}: analysis with func {func}, result {result}, logs '{l}'")
			service_success = service_success and result
			if not result:
				logs.append(f"=> {func.__name__} check (options '{opt}') FAILED")
				logs.append(l)
		logs = '\n'.join(logs)
		if not service_success:
			logging.error(l)
		else:
			logging.info(l)
		all_funcs = ", ".join([f.__name__ for (f, _, _) in list_analysis])
		HTML.CreateHtmlTestRowQueue(f"Check {all_funcs} on {filename}", 'OK' if service_success else 'KO', [logs])
		success = success and service_success
	HTML.desc = orig_html_desc
	return success
