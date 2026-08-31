# SPDX-License-Identifier: LicenseRef-CSSL-1.0

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
# Import Components
#-----------------------------------------------------------

import constants as CONST


import cls_oaicitest		 #main class for OAI CI test framework
import cls_containerize	 #class Containerize for all container-based operations on RAN/UE objects
import cls_static_code_analysis as SCA
import cls_cluster		 # class for building/deploying on cluster
import cls_native        # class for all native/source-based operations
from cls_ci_helper import TestCaseCtx

import ran
import cls_cmd
import cls_oai_html


#-----------------------------------------------------------
# Import Libs
#-----------------------------------------------------------
import sys		# arg
import re		# reg
import time		# sleep
import os
import subprocess
import lxml.etree as ET
import logging
import signal
import traceback


#-----------------------------------------------------------
# General Functions
#-----------------------------------------------------------



def CheckClassValidity(xml_class_list,action,id):
	if action not in xml_class_list:
		logging.error('test-case ' + id + ' has unlisted class ' + action + ' ##CHECK xml_class_list.yml')
		resp=False
	else:
		resp=True
	return resp

def ExecuteActionWithParam(action, ctx, node):
	global RAN
	global HTML
	global CONTAINERS
	global CLUSTER
	if action == 'Build_eNB' or action == 'Build_Image' or action == "Build_Cluster_Image" or action == "Build_Run_Tests":
		RAN.Build_eNB_args=test.findtext('Build_eNB_args')
		CONTAINERS.imageKind=test.findtext('kind')
		dockerfile = test.findtext('dockerfile') or ''
		runtime_opt = test.findtext('runtime-opt') or ''
		ctest_opt = test.findtext('ctest-opt') or ''
		if action == 'Build_eNB':
			success = cls_native.Native.Build(ctx, node, HTML, RAN.workspace, RAN.Build_eNB_args)
		elif action == 'Build_Image':
			success = CONTAINERS.BuildImage(ctx, node, HTML)
		elif action == 'Build_Cluster_Image':
			success = CLUSTER.BuildClusterImage(ctx, node, HTML)
		elif action == 'Build_Run_Tests':
			success = CONTAINERS.BuildRunTests(ctx, node, dockerfile, runtime_opt, ctest_opt, HTML)

	elif action == 'Initialize_eNB':
		RAN.Initialize_eNB_args=test.findtext('Initialize_eNB_args')
		cmd_prefix = test.findtext('cmd_prefix')
		if cmd_prefix is not None: RAN.cmd_prefix = cmd_prefix
		success = RAN.InitializeeNB(ctx, node, HTML)

	elif action == 'Terminate_eNB':
		services = []
		analysis = test.find("analysis")
		if analysis is not None:
			# services: multiple services to analyse, separated by whitespace
			services = analysis.findtext("services", default="").split()
			# service: individual services to analyze, in case they have whitespace
			services = services + [s.text for s in analysis.findall("service")]
		success = RAN.TerminateeNB(ctx, node, HTML, services)

	elif action == 'Initialize_UE' or action == 'Attach_UE' or action == 'Detach_UE' or action == 'Terminate_UE' or action == 'CheckStatusUE' or action == 'DataEnable_UE' or action == 'DataDisable_UE':
		CiTestObj.ue_ids = test.findtext('id').split(' ')
		if action == 'Initialize_UE':
			success = CiTestObj.InitializeUE(node, HTML)
		elif action == 'Attach_UE':
			success = CiTestObj.AttachUE(node, HTML)
		elif action == 'Detach_UE':
			success = CiTestObj.DetachUE(node, HTML)
		elif action == 'Terminate_UE':
			success = CiTestObj.TerminateUE(ctx, node, HTML)
		elif action == 'CheckStatusUE':
			success = CiTestObj.CheckStatusUE(node, HTML)
		elif action == 'DataEnable_UE':
			success = CiTestObj.DataEnableUE(node, HTML)
		elif action == 'DataDisable_UE':
			success = CiTestObj.DataDisableUE(node, HTML)

	elif action == 'Ping':
		CiTestObj.ping_args = test.findtext('ping_args')
		CiTestObj.ping_packetloss_threshold = test.findtext('ping_packetloss_threshold')
		CiTestObj.ue_ids = test.findtext('id').split(' ')
		CiTestObj.svr_id = test.findtext('svr_id')
		if test.findtext('svr_node'):
			CiTestObj.svr_node = test.findtext('svr_node') if not force_local else 'localhost'
		ping_rttavg_threshold = test.findtext('ping_rttavg_threshold') or ''
		success = CiTestObj.Ping(ctx, node, HTML)

	elif action == 'Iperf' or action == 'Iperf2_Unidir':
		CiTestObj.iperf_args = test.findtext('iperf_args')
		CiTestObj.ue_ids = test.findtext('id').split(' ')
		CiTestObj.svr_id = test.findtext('svr_id')
		if test.findtext('svr_node'):
			CiTestObj.svr_node = test.findtext('svr_node') if not force_local else 'localhost'
		CiTestObj.iperf_packetloss_threshold = test.findtext('iperf_packetloss_threshold')
		CiTestObj.iperf_bitrate_threshold = test.findtext('iperf_bitrate_threshold') or '90'
		CiTestObj.iperf_profile = test.findtext('iperf_profile') or 'balanced'
		CiTestObj.iperf_tcp_rate_target = test.findtext('iperf_tcp_rate_target') or None
		if CiTestObj.iperf_profile != 'balanced' and CiTestObj.iperf_profile != 'unbalanced' and CiTestObj.iperf_profile != 'single-ue':
			logging.error(f'test-case has wrong profile {CiTestObj.iperf_profile}, forcing balanced')
			CiTestObj.iperf_profile = 'balanced'
		if action == 'Iperf':
			success = CiTestObj.Iperf(ctx, node, HTML)
		elif action == 'Iperf2_Unidir':
			success = CiTestObj.Iperf2_Unidir(ctx, node, HTML)

	elif action == 'IdleSleep':
		st = test.findtext('idle_sleep_time_in_sec') or "5"
		success = cls_oaicitest.IdleSleep(HTML, int(st))

	elif action == 'Deploy_Run_OC_PhySim':
		oc_release = test.findtext('oc_release')
		script = "scripts/oc-deploy-physims.sh"
		image_tag = CLUSTER.branch
		options = f"oaicicd-core-for-ci-ran {oc_release} {image_tag} {CLUSTER.workspace}"
		workdir = CLUSTER.workspace
		success = cls_oaicitest.Deploy_Physim(ctx, HTML, node, workdir, script, options)

	elif action == 'Build_Deploy_PhySim':
		ctest_opt = test.findtext('ctest-opt') or ''
		script = test.findtext('script')
		options = f"{CONTAINERS.workspace} {ctest_opt}"
		workdir = CONTAINERS.workspace
		success = cls_oaicitest.Deploy_Physim(ctx, HTML, node, workdir, script, options)

	elif action == 'DeployCoreNetwork' or action == 'UndeployCoreNetwork':
		cn_id = test.findtext('cn_id')
		core_op = getattr(cls_oaicitest.OaiCiTest, action)
		success = core_op(cn_id, ctx, HTML)

	elif action == 'DeployWithScript' or action == 'UndeployWithScript':
		script = test.findtext('script')
		options = test.findtext('options')
		if action == 'DeployWithScript':
			deploymentTag = RAN.branch
			success = cls_oaicitest.DeployWithScript(HTML, node, script, options, deploymentTag)
		elif action == 'UndeployWithScript':
			success = cls_oaicitest.UndeployWithScript(HTML, ctx, node, script, options)

	elif action == 'Deploy_Object' or action == 'Undeploy_Object' or action == "Create_Workspace" or action == "Stop_Object":
		CONTAINERS.yamlPath = test.findtext('yaml_path')
		CONTAINERS.services = test.findtext('services')
		CONTAINERS.num_attempts = int(test.findtext('num_attempts') or 1)
		CONTAINERS.deploymentTag = CONTAINERS.branch
		if action == 'Deploy_Object':
			success = CONTAINERS.DeployObject(ctx, node, HTML)
		elif action == 'Stop_Object':
			success = CONTAINERS.StopObject(ctx, node, HTML)
		elif action == 'Undeploy_Object':
			analysis = test.find("analysis")
			services = []
			if analysis is not None:
				# services: multiple services to analyse, separated by whitespace
				services = analysis.findtext("services", default="").split()
				# service: individual services to analyze, in case they have whitespace
				services = services + [s.text for s in analysis.findall("service")]
			success = CONTAINERS.UndeployObject(ctx, node, HTML, services)
		elif action == 'Create_Workspace':
			if force_local:
				# Do not create a working directory when running locally. Current repo directory will be used
				return True
			success = CONTAINERS.Create_Workspace(node, HTML)

	elif action == 'LicenceAndFormattingCheck':
		success = SCA.StaticCodeAnalysis.LicenceAndFormattingCheck(ctx, node, HTML, RAN.workspace, RAN.branch, RAN.merge, RAN.targetBranch)

	elif action == 'Push_Local_Registry':
		tag_prefix = test.findtext('tag_prefix') or ""
		success = CONTAINERS.Push_Image_to_Local_Registry(node, HTML, tag_prefix)

	elif action == 'Pull_Local_Registry' or action == 'Clean_Test_Server_Images':
		if force_local:
			# Do not pull or remove images when running locally. User is supposed to handle image creation & cleanup
			return True
		tag_prefix = test.findtext('tag_prefix') or ""
		images = test.findtext('images').split()
		# hack: for FlexRIC, we need to overwrite the tag to use
		tag = None
		if len(images) == 1 and images[0] == "oai-flexric":
			tag = CONTAINERS.flexricTag
		if action == "Pull_Local_Registry":
			success = CONTAINERS.Pull_Image_from_Registry(HTML, node, images, tag=tag, tag_prefix=tag_prefix)
		if action == "Clean_Test_Server_Images":
			success = CONTAINERS.Clean_Test_Server_Images(HTML, node, images, tag=tag)

	elif action == 'Custom_Command':
		command = test.findtext('command')
		# Allow referencing repository workspace path in XML via %%workspace%%
		command = command.replace("%%workspace%%", CONTAINERS.workspace)
		success = cls_oaicitest.Custom_Command(HTML, node, command)

	elif action == 'Custom_Script':
		script = test.findtext('script')
		args = test.findtext('args')
		# Allow referencing repository workspace path in XML via %%workspace%%
		script = script.replace("%%workspace%%", CONTAINERS.workspace)
		success = cls_oaicitest.Custom_Script(HTML, node, script, args)

	elif action == 'Pull_Cluster_Image':
		tag_prefix = test.findtext('tag_prefix') or ""
		images = test.findtext('images').split()
		success = CLUSTER.PullClusterImage(HTML, node, images, tag_prefix=tag_prefix)

	elif action == 'AnalyzeRTStats':
		yaml = test.findtext('stats_cfg')
		success = RAN.AnalyzeRTStats(HTML, node, ctx, yaml)

	elif action == 'AnalyzeRTStats_Object':
		yaml = test.findtext('stats_cfg')
		service = test.findtext('service')
		stats_files = (test.findtext('stats_file') or '').split()
		success = CONTAINERS.AnalyzeRTStatsObject(HTML, node, ctx, yaml, service, stats_files)

	else:
		logging.warning(f"unknown action {action}, skip step")
		success = True # by default, we skip the step and print a warning

	return success

test_runner_abort = False
def receive_signal(signum, frame):
    global test_runner_abort
    if not test_runner_abort:
        logging.warning("received signal, canceling steps")
        logging.info("send signal again to exit immediately")
        test_runner_abort = True
    else:
        logging.warning("received signal again, exiting")
        sys.exit(1)

def ShowTestID(ctx, desc, file, line):
    logging.info(f'\u001B[1m----------------------------------------\u001B[0m')
    logging.info(f'\u001B[1m Test #{ctx.test_idx} ({file}:{line})   \u001B[0m')
    logging.info(f'\u001B[1m {desc}                                 \u001B[0m')
    logging.info(f'\u001B[1m----------------------------------------\u001B[0m')

#-----------------------------------------------------------
# MAIN PART
#-----------------------------------------------------------

#loading xml action list from yaml
import yaml
xml_class_list_file='xml_class_list.yml'
if (os.path.isfile(xml_class_list_file)):
	yaml_file=xml_class_list_file
elif (os.path.isfile('ci-scripts/'+xml_class_list_file)):
	yaml_file='ci-scripts/'+xml_class_list_file
else:
	logging.error("XML action list yaml file cannot be found")
	sys.exit("XML action list yaml file cannot be found")

with open(yaml_file,'r') as f:
    # The FullLoader parameter handles the conversion-$
    #from YAML scalar values to Python dictionary format$
    xml_class_list = yaml.load(f,Loader=yaml.FullLoader)

mode = ''

CiTestObj = cls_oaicitest.OaiCiTest()
 
RAN = ran.RANManagement()
HTML = cls_oai_html.HTMLManagement()
CONTAINERS = cls_containerize.Containerize()
CLUSTER = cls_cluster.Cluster()

#-----------------------------------------------------------
# Parsing Command Line Arguments
#-----------------------------------------------------------

import args_parse
# Force local execution, move all execution targets to localhost
force_local = False
mode, force_local, date_fmt = args_parse.ArgsParse(sys.argv,CiTestObj,RAN,HTML,CONTAINERS,CLUSTER)
fmt = "%(levelname)8s: %(message)s"
if date_fmt:
    fmt = "[%(asctime)s] %(levelname)s %(message)s"
logging.basicConfig(level=logging.DEBUG, stream=sys.stdout, format=fmt, datefmt=date_fmt,)


#-----------------------------------------------------------
# mode amd XML class (action) analysis
#-----------------------------------------------------------
cwd = os.getcwd()

if re.match('^TerminateeNB$', mode, re.IGNORECASE):
	logging.warning("Option TerminateeNB ignored")
elif re.match('^TerminateHSS$', mode, re.IGNORECASE):
	logging.warning("Option TerminateHSS ignored")
elif re.match('^TerminateMME$', mode, re.IGNORECASE):
	logging.warning("Option TerminateMME ignored")
elif re.match('^TerminateSPGW$', mode, re.IGNORECASE):
	logging.warning("Option TerminateSPGW ignored")
elif re.match('^LogCollectBuild$', mode, re.IGNORECASE):
	logging.warning("Option LogCollectBuild ignored")
elif re.match('^LogCollecteNB$', mode, re.IGNORECASE):
	logging.warning("Option LogCollecteNB ignored")
elif re.match('^LogCollectHSS$', mode, re.IGNORECASE):
	logging.warning("Option LogCollectHSS ignored")
elif re.match('^LogCollectMME$', mode, re.IGNORECASE):
	logging.warning("Option LogCollectMME ignored")
elif re.match('^LogCollectSPGW$', mode, re.IGNORECASE):
	logging.warning("Option LogCollectSPGW ignored")
elif re.match('^LogCollectPing$', mode, re.IGNORECASE):
	logging.warning("Option LogCollectPing ignored")
elif re.match('^LogCollectIperf$', mode, re.IGNORECASE):
	logging.warning("Option LogCollectIperf ignored")
elif re.match('^LogCollectOAIUE$', mode, re.IGNORECASE):
	logging.warning("Option LogCollectOAIUE ignored")
elif re.match('^InitiateHtml$', mode, re.IGNORECASE):
	count = 0
	foundCount = 0
	while (count < HTML.nbTestXMLfiles):
		#xml_test_file = cwd + "/" + CiTestObj.testXMLfiles[count]
		xml_test_file = sys.path[0] + "/" + CiTestObj.testXMLfiles[count]
		if (os.path.isfile(xml_test_file)):
			try:
				xmlTree = ET.parse(xml_test_file)
			except Exception as e:
				print(f"Error: {e} while parsing file: {xml_test_file}.")
			xmlRoot = xmlTree.getroot()
			HTML.htmlTabRefs.append(xmlRoot.findtext('htmlTabRef',default='test-tab-' + str(count)))
			HTML.htmlTabNames.append(xmlRoot.findtext('htmlTabName',default='test-tab-' + str(count)))
			HTML.htmlTabIcons.append(xmlRoot.findtext('htmlTabIcon',default='info-sign'))
			foundCount += 1
		count += 1
	if foundCount != HTML.nbTestXMLfiles:
		HTML.nbTestXMLfiles=foundCount
	
	HTML.CreateHtmlHeader()
elif re.match('^FinalizeHtml$', mode, re.IGNORECASE):
	logging.info('\u001B[1m----------------------------------------\u001B[0m')
	logging.info('\u001B[1m  Creating HTML footer \u001B[0m')
	logging.info('\u001B[1m----------------------------------------\u001B[0m')

	HTML.CreateHtmlFooter(CiTestObj.finalStatus)
elif re.match('^TesteNB$', mode, re.IGNORECASE) or re.match('^TestUE$', mode, re.IGNORECASE):
	logging.info('\u001B[1m----------------------------------------\u001B[0m')
	logging.info('\u001B[1m  Starting Scenario: ' + CiTestObj.testXMLfiles[0] + '\u001B[0m')
	logging.info('\u001B[1m----------------------------------------\u001B[0m')
	if re.match('^TesteNB$', mode, re.IGNORECASE):
		if RAN.repository == '' or RAN.branch == '' or RAN.workspace == '':
			sys.exit(f'Insufficient Parameters: {RAN.repository=}, {RAN.branch=}, {RAN.workspace=}')
	else:
		if CiTestObj.repository == '' or CiTestObj.branch == '':
			sys.exit('UE: Insufficient Parameter')

	#read test_case_list.xml file
	# if no parameters for XML file, use default value
	if (HTML.nbTestXMLfiles != 1):
		xml_test_file = cwd + "/test_case_list.xml"
	else:
		xml_test_file = cwd + "/" + CiTestObj.testXMLfiles[0]

	# directory where all log artifacts will be placed
	logPath = f"{cwd}/../cmake_targets/log/{CiTestObj.testXMLfiles[0].split('/')[-1]}.d"
	# we run from within ci-scripts, but the logPath is absolute, so replace
	# the ci-scripts/..; if it does not exist, nothing will happen
	logPath = logPath.replace(r'/ci-scripts/..', '')
	logging.info(f"placing all artifacts for this run in {logPath}/")
	with cls_cmd.LocalCmd() as c:
		c.run(f"rm -rf {logPath}")
		c.run(f"mkdir -p {logPath}")

	xmlTree = ET.parse(xml_test_file)
	xmlRoot = xmlTree.getroot()

	if (HTML.nbTestXMLfiles == 1):
		HTML.htmlTabRefs.append(xmlRoot.findtext('htmlTabRef',default='test-tab-0'))
		HTML.htmlTabNames.append(xmlRoot.findtext('htmlTabName',default='Test-0'))
	all_tests=xmlRoot.findall('testCase')

	signal.signal(signal.SIGINT, receive_signal)

	HTML.CreateHtmlTabHeader()

	task_set_succeeded = True
	HTML.startTime=int(round(time.time() * 1000))

	for index, test in enumerate(all_tests, start=1):
		if test_runner_abort:
			task_set_succeeded = False
		test_case_idx = f"{index:06d}"
		ctx = TestCaseCtx(int(test_case_idx), logPath)
		HTML.testCaseIdx = test_case_idx
		desc = test.findtext('desc')
		node = test.findtext('node') if not force_local else 'localhost'
		always_exec = test.findtext('always_exec') in ['True', 'true', 'Yes', 'yes']
		may_fail = test.findtext('may_fail') in ['True', 'true', 'Yes', 'yes']
		HTML.desc = desc
		action = test.findtext('class')
		if not CheckClassValidity(xml_class_list, action, test_case_idx):
			task_set_succeeded = False
			continue
		file = os.path.basename(xml_test_file)
		line = test.find('class').sourceline
		ShowTestID(ctx, desc, file, line)
		if not task_set_succeeded and not always_exec:
			msg = f"skipping test due to prior error"
			logging.warning(msg)
			HTML.CreateHtmlTestRowQueue(msg, "SKIP", [])
			continue
		try:
			test_succeeded = ExecuteActionWithParam(action, ctx, node)
			if not test_succeeded and may_fail:
				logging.warning(f"test ID {test_case_idx} action {action} may or may not fail, proceeding despite error")
			elif not test_succeeded:
				logging.error(f"test ID {test_case_idx} action {action} failed ({test_succeeded}), skipping next tests")
				task_set_succeeded = False
		except Exception as e:
			s = traceback.format_exc()
			logging.error(f'while running CI, an exception occurred:\n{s}')
			HTML.CreateHtmlTestRowQueue("N/A", 'KO', [f"CI test code encountered an exception:\n{s}"])
			task_set_succeeded = False
			continue

	if not task_set_succeeded:
		logging.error('\u001B[1;37;41mScenario failed\u001B[0m')
		HTML.CreateHtmlTabFooter(False)
		sys.exit('Failed Scenario')
	else:
		logging.info('\u001B[1;37;42mScenario passed\u001B[0m')
		HTML.CreateHtmlTabFooter(True)
elif re.match('^LoadParams$', mode, re.IGNORECASE):
	pass
else:
	sys.exit(f'Invalid mode {mode}')
sys.exit(0)
