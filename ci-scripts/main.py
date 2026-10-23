# SPDX-License-Identifier: LicenseRef-CSSL-1.0

#---------------------------------------------------------------------
# Python for CI of OAI-eNB + COTS-UE
#
#   Required Python Version
#     Python 3.x
#---------------------------------------------------------------------



#-----------------------------------------------------------
# Import Components
#-----------------------------------------------------------



import cls_oaicitest		 #main class for OAI CI test framework
import cls_containerize	 #class Containerize for all container-based operations on RAN/UE objects
import cls_cluster		 # class for building/deploying on cluster
import cls_native        # class for all native/source-based operations
from cls_ci_helper import TestCaseCtx

import ran
import cls_cmd
import cls_oai_html
import args_parse


#-----------------------------------------------------------
# Import Libs
#-----------------------------------------------------------
import sys		# arg
import time		# sleep
import os
import lxml.etree as ET
from collections import namedtuple
import logging
import signal
import traceback


#-----------------------------------------------------------
# General Functions
#-----------------------------------------------------------

def ExecuteActionWithParam(action, test, ctx, node, oc, HTML):
	if action == 'Build_eNB' or action == 'Build_Image' or action == "Build_Cluster_Image" or action == "Build_Run_Tests":
		args = test.findtext('Build_eNB_args')
		image_kind = test.findtext('kind')
		dockerfile = test.findtext('dockerfile') or ''
		runtime_opt = test.findtext('runtime-opt') or ''
		ctest_opt = test.findtext('ctest-opt') or ''
		if action == 'Build_eNB':
			success = cls_native.Native.Build(ctx, node, HTML, ctx.g.workspace, args)
		elif action == 'Build_Image':
			success = cls_containerize.Containerize.BuildImage(ctx, node, HTML, image_kind)
		elif action == 'Build_Cluster_Image':
			success = cls_cluster.Cluster.BuildClusterImage(ctx, oc, node, HTML)
		elif action == 'Build_Run_Tests':
			success = cls_containerize.Containerize.BuildRunTests(ctx, node, dockerfile, runtime_opt, ctest_opt, HTML)

	elif action == 'Initialize_eNB':
		args = test.findtext('Initialize_eNB_args')
		cmd_prefix = test.findtext('cmd_prefix')
		success = ran.RAN.InitializeeNB(ctx, node, HTML, args, cmd_prefix)

	elif action == 'Terminate_eNB':
		services = []
		analysis = test.find("analysis")
		if analysis is not None:
			# services: multiple services to analyse, separated by whitespace
			services = analysis.findtext("services", default="").split()
			# service: individual services to analyze, in case they have whitespace
			services = services + [s.text for s in analysis.findall("service")]
		success = ran.RAN.TerminateeNB(ctx, node, HTML, services)

	elif action == 'Initialize_UE' or action == 'Attach_UE' or action == 'Detach_UE' or action == 'Terminate_UE' or action == 'CheckStatusUE' or action == 'DataEnable_UE' or action == 'DataDisable_UE':
		ue_ids = test.findtext('id').split(' ')
		if action == 'Initialize_UE':
			success = cls_oaicitest.OaiCiTest.InitializeUE(node, HTML, ue_ids)
		elif action == 'Attach_UE':
			success = cls_oaicitest.OaiCiTest.AttachUE(node, HTML, ue_ids)
		elif action == 'Detach_UE':
			success = cls_oaicitest.OaiCiTest.DetachUE(node, HTML, ue_ids)
		elif action == 'Terminate_UE':
			success = cls_oaicitest.OaiCiTest.TerminateUE(ctx, node, HTML, ue_ids)
		elif action == 'CheckStatusUE':
			success = cls_oaicitest.OaiCiTest.CheckStatusUE(node, HTML, ue_ids)
		elif action == 'DataEnable_UE':
			success = cls_oaicitest.OaiCiTest.DataEnableUE(node, HTML, ue_ids)
		elif action == 'DataDisable_UE':
			success = cls_oaicitest.OaiCiTest.DataDisableUE(node, HTML, ue_ids)

	elif action == 'Ping':
		args = test.findtext('ping_args')
		packetloss_threshold = float(test.findtext('ping_packetloss_threshold'))
		ping_rttavg_threshold = test.findtext('ping_rttavg_threshold') or '' # was ignored
		opt = cls_oaicitest.PingOpt(args, packetloss_threshold, float('nan'))
		svr_node = 'localhost' if force_local and test.findtext('svr_node') else test.findtext('svr_node')
		ep = cls_oaicitest.TrafficEndpoints.create(test.findtext('id'), node, test.findtext('svr_id'), svr_node)
		success = cls_oaicitest.OaiCiTest.Ping(ctx, HTML, opt, ep)

	elif action == 'Iperf' or action == 'Iperf2_Unidir':
		args = test.findtext('iperf_args')
		packetloss_threshold = float(test.findtext('iperf_packetloss_threshold') or 'nan') # only for UDP
		bitrate_threshold = float(test.findtext('iperf_bitrate_threshold') or '90')
		profile = test.findtext('iperf_profile') or 'balanced'
		tcp_rate_target = float(test.findtext('iperf_tcp_rate_target') or 'nan')
		if profile != 'balanced' and profile != 'unbalanced' and profile != 'single-ue':
			logging.error(f'test-case has wrong profile {profile}, forcing balanced')
			profile = 'balanced'
		opt = cls_oaicitest.IperfOpt(args, packetloss_threshold, bitrate_threshold, profile, tcp_rate_target)
		svr_node = 'localhost' if force_local and test.findtext('svr_node') else test.findtext('svr_node')
		ep = cls_oaicitest.TrafficEndpoints.create(test.findtext('id'), node, test.findtext('svr_id'), svr_node)
		if action == 'Iperf':
			success = cls_oaicitest.OaiCiTest.Iperf(ctx, HTML, opt, ep)
		elif action == 'Iperf2_Unidir':
			success = cls_oaicitest.OaiCiTest.Iperf2_Unidir(ctx, HTML, opt, ep)

	elif action == 'IdleSleep':
		st = test.findtext('idle_sleep_time_in_sec') or "5"
		success = cls_oaicitest.IdleSleep(HTML, int(st))

	elif action == 'Deploy_Run_OC_PhySim':
		oc_release = test.findtext('oc_release')
		script = "scripts/oc-deploy-physims.sh"
		image_tag = ctx.g.branch
		options = f"oaicicd-core-for-ci-ran {oc_release} {image_tag} {ctx.g.workspace}"
		workdir = ctx.g.workspace
		success = cls_oaicitest.Deploy_Physim(ctx, HTML, node, workdir, script, options)

	elif action == 'Build_Deploy_PhySim':
		ctest_opt = test.findtext('ctest-opt') or ''
		script = test.findtext('script')
		options = f"{ctx.g.workspace} {ctest_opt}"
		workdir = ctx.g.workspace
		success = cls_oaicitest.Deploy_Physim(ctx, HTML, node, workdir, script, options)

	elif action == 'DeployCoreNetwork' or action == 'UndeployCoreNetwork':
		cn_id = test.findtext('cn_id')
		core_op = getattr(cls_oaicitest.OaiCiTest, action)
		success = core_op(cn_id, ctx, HTML)

	elif action == 'Deploy_Object' or action == 'Undeploy_Object' or action == "Create_Workspace" or action == "Stop_Object":
		yaml_path = test.findtext('yaml_path')
		services = test.findtext('services') or ''
		num_attempts = int(test.findtext('num_attempts') or 1)
		opt = cls_containerize.DeploymentOpt(yaml_path, services, num_attempts)
		if action == 'Deploy_Object':
			success = cls_containerize.Containerize.DeployObject(ctx, node, HTML, opt)
		elif action == 'Stop_Object':
			success = cls_containerize.Containerize.StopObject(ctx, node, HTML, opt)
		elif action == 'Undeploy_Object':
			analysis = test.find("analysis")
			services = []
			if analysis is not None:
				# services: multiple services to analyse, separated by whitespace
				services = analysis.findtext("services", default="").split()
				# service: individual services to analyze, in case they have whitespace
				services = services + [s.text for s in analysis.findall("service")]
			success = cls_containerize.Containerize.UndeployObject(ctx, node, HTML, opt, services)
		elif action == 'Create_Workspace':
			if force_local:
				# Do not create a working directory when running locally. Current repo directory will be used
				return True
			success = cls_containerize.Containerize.Create_Workspace(ctx, node, HTML)

	elif action == 'Push_Local_Registry':
		tag_prefix = test.findtext('tag_prefix') or ""
		success = cls_containerize.Containerize.Push_Image_to_Local_Registry(ctx, node, HTML, tag_prefix)

	elif action == 'Pull_Local_Registry' or action == 'Clean_Test_Server_Images':
		if force_local:
			# Do not pull or remove images when running locally. User is supposed to handle image creation & cleanup
			return True
		tag_prefix = test.findtext('tag_prefix') or ""
		images = test.findtext('images').split()
		# hack: for FlexRIC, we need to overwrite the tag to use
		tag = None
		if len(images) == 1 and images[0] == "oai-flexric":
			tag = ctx.g.flexric_tag
		if action == "Pull_Local_Registry":
			success = cls_containerize.Containerize.Pull_Image_from_Registry(ctx, HTML, node, images, tag=tag, tag_prefix=tag_prefix)
		if action == "Clean_Test_Server_Images":
			success = cls_containerize.Containerize.Clean_Test_Server_Images(ctx, HTML, node, images, tag=tag)

	elif action == 'Custom_Command':
		command = test.findtext('command')
		# Allow referencing repository workspace path in XML via %%workspace%%
		command = command.replace("%%workspace%%", ctx.g.workspace)
		success = cls_oaicitest.Custom_Command(HTML, node, command)

	elif action == 'Custom_Script':
		script = test.findtext('script')
		options = test.findtext('options') or ''
		timeout = int(test.findtext('timeout') or 600)
		# Allow referencing repository workspace path and image tag in XML
		script = script.replace("%%workspace%%", ctx.g.workspace)
		options = options.replace("%%workspace%%", ctx.g.workspace)
		options = options.replace("%%image_tag%%", ctx.g.branch)
		success = cls_oaicitest.Custom_Script(HTML, ctx, node, script, options, timeout)

	elif action == 'Pull_Cluster_Image':
		tag_prefix = test.findtext('tag_prefix') or ""
		images = test.findtext('images').split()
		success = cls_cluster.Cluster.PullClusterImage(ctx, oc, HTML, node, images, tag_prefix=tag_prefix)

	elif action == 'AnalyzeRTStats':
		yaml = test.findtext('stats_cfg')
		success = ran.RAN.AnalyzeRTStats(HTML, node, ctx, yaml)

	elif action == 'AnalyzeRTStats_Object':
		yaml_path = test.findtext('yaml_path')
		yaml = test.findtext('stats_cfg')
		service = test.findtext('service')
		stats_files = (test.findtext('stats_file') or '').split()
		success = cls_containerize.Containerize.AnalyzeRTStatsObject(HTML, node, ctx, yaml_path, yaml, service, stats_files)

	else:
		logging.warning(f"unknown action {action}, CI run marked as failure")
		success = False

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

def run_tests(g_ctx, logPath, HTML, all_tests, filename):
	task_set_succeeded = True
	for index, test in enumerate(all_tests, start=1):
		if test_runner_abort:
			task_set_succeeded = False
		test_case_idx = f"{index:06d}"
		ctx = TestCaseCtx(int(test_case_idx), logPath, g_ctx)
		desc = test.findtext('desc')
		node = test.findtext('node') if not force_local else 'localhost'
		always_exec = test.findtext('always_exec') in ['True', 'true', 'Yes', 'yes']
		may_fail = test.findtext('may_fail') in ['True', 'true', 'Yes', 'yes']
		HTML.testCaseIdx = test_case_idx
		HTML.desc = desc
		action = test.findtext('class')
		file = os.path.basename(filename)
		line = test.find('class').sourceline
		ShowTestID(ctx, desc, file, line)
		if not task_set_succeeded and not always_exec:
			msg = f"skipping test due to prior error"
			logging.warning(msg)
			HTML.CreateHtmlTestRowQueue(msg, "SKIP", [])
			continue
		try:
			test_succeeded = ExecuteActionWithParam(action, test, ctx, node, oc, HTML)
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
	return task_set_succeeded

#-----------------------------------------------------------
# MAIN PART
#-----------------------------------------------------------
if __name__ == "__main__":
	force_local, date_fmt, xmls, g_ctx, oc = args_parse.ArgsParse()
	fmt = "%(levelname)8s: %(message)s"
	if date_fmt:
		fmt = "[%(asctime)s] %(levelname)s %(message)s"
	logging.basicConfig(level=logging.DEBUG, stream=sys.stdout, format=fmt, datefmt=date_fmt,)

	if g_ctx.repository == '' or g_ctx.branch == '' or g_ctx.workspace == '':
		sys.exit(f'Insufficient Parameters: {g_ctx.repository=}, {g_ctx.branch=}, {g_ctx.workspace=}')

	cwd = os.getcwd()
	logDir = f"{cwd}/../cmake_targets/log"
	logging.info(f"placing all artifacts for this run in {logDir}/")
	TestXML = namedtuple("TestXML", ["filename", "logPath", "title"])
	test_xmls = []
	for x in xmls:
		logging.info(f"open and parse file {x}")
		if not os.path.isfile(x):
			logging.error(f"no such file {x}")
			sys.exit(1)
		try:
			xmlTree = ET.parse(x)
		except Exception as e:
			logging.error(f"while parsing file {x}: {e}")
			sys.exit(1)
		logPath = f"{logDir}/{os.path.basename(x)}.d"
		with cls_cmd.LocalCmd() as c:
			c.run(f"rm -rf {logPath}")
			ret = c.run(f"mkdir -p {logPath}")
			if ret.returncode != 0:
				logging.error(f"error while creating logPath {logPath}")
				sys.exit(1)
		root = xmlTree.getroot()
		t = TestXML(x, logPath, root.findtext('htmlTabName'))
		test_xmls.append(t)

	HTML = cls_oai_html.HTMLManagement()
	HTML.CreateHtmlHeader(g_ctx.repository, g_ctx.branch, test_xmls)

	signal.signal(signal.SIGINT, receive_signal)

	final_status = True
	for i, xml in enumerate(test_xmls):
		logging.info('\u001B[1m----------------------------------------\u001B[0m')
		logging.info(f'\u001B[1m {xml}\u001B[0m')
		logging.info('\u001B[1m----------------------------------------\u001B[0m')

		xmlTree = ET.parse(xml.filename)
		xmlRoot = xmlTree.getroot()
		all_tests = xmlRoot.findall('testCase')

		HTML.CreateHtmlTabHeader(xml.filename, i)
		HTML.startTime=int(round(time.time() * 1000))

		success = run_tests(g_ctx, xml.logPath, HTML, all_tests, xml.filename)

		if not success:
			logging.error(f'\u001B[1;37;41mScenario {xml.filename} failed\u001B[0m')
			HTML.CreateHtmlTabFooter(False, i)
			final_status = False
		else:
			logging.info(f'\u001B[1;37;42mScenario {xml.filename} passed\u001B[0m')
			HTML.CreateHtmlTabFooter(True, i)

	HTML.CreateHtmlFooter(final_status)
	ret = 0 if final_status else 1
	sys.exit(ret)
