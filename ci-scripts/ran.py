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
# Import
#-----------------------------------------------------------
import re               # reg
import logging
import os
import time

#-----------------------------------------------------------
# OAI Testing modules
#-----------------------------------------------------------
import cls_cmd
import cls_analysis
from cls_ci_helper import archiveArtifact

#-----------------------------------------------------------
# Class Declaration
#-----------------------------------------------------------
class RAN():

#-----------------------------------------------------------
# RAN management functions
#-----------------------------------------------------------

	def InitializeeNB(ctx, node, HTML, args, cmd_prefix):
		if not node:
			raise ValueError(f"{node=}")
		logging.debug('Starting eNB/gNB on server: ' + node)

		lSourcePath = ctx.g.workspace
		cmd = cls_cmd.getConnection(node)
		
		# args usually start with -O and followed by the location in repository
		full_config_file = args.replace('-O ','')
		extra_options = ''
		extIdx = full_config_file.find('.conf')
		if (extIdx <= 0):
			raise ValueError(f"no config file in {args}")
		extra_options = full_config_file[extIdx + 5:]
		full_config_file = full_config_file[:extIdx + 5]
		config_path, config_file = os.path.split(full_config_file)

		logfile = f'{lSourcePath}/cmake_targets/enb.log'
		cmd.cd(f"{lSourcePath}/cmake_targets/") # important: set wd so nrL1_stats.log etc are logged here
		cmd.run(f'sudo -E stdbuf -o0 {cmd_prefix} {lSourcePath}/cmake_targets/ran_build/build/nr-softmodem -O {lSourcePath}/{full_config_file} {extra_options} > {logfile} 2>&1 &')

		enbDidSync = False
		for _ in range(10):
			time.sleep(5)
			ret = cmd.run(f'grep --text -E --color=never -i "wait|sync|Starting|Started" {logfile}', reportNonZero=False)
			result = re.search('got sync|Starting F1AP at CU', ret.stdout)
			if result is not None:
				enbDidSync = True
				break
		if not enbDidSync:
			cmd.run(f'sudo killall -9 nr-softmodem') # in case it did not stop automatically
			archiveArtifact(cmd, ctx, logfile)

		cmd.close()

		msg = f'{cmd_prefix} nr-softmodem -O {config_file} {extra_options}'
		if enbDidSync:
			logging.debug('\u001B[1m Initialize eNB/gNB Completed\u001B[0m')
			HTML.CreateHtmlTestRowQueue(msg, 'OK', [])
		else:
			logging.error('\u001B[1;37;41m eNB/gNB logging system did not show got sync! \u001B[0m')
			HTML.CreateHtmlTestRowQueue(msg, 'KO', [])

		return enbDidSync

	def TerminateeNB(ctx, node, HTML, to_analyze):
		logging.debug('Stopping eNB/gNB on server: ' + node)
		lSourcePath = ctx.g.workspace
		cmd = cls_cmd.getConnection(node)
		ret = cmd.run('ps -aux | grep --color=never -e softmodem | grep -v grep')
		result = re.search('-softmodem', ret.stdout)
		if result is not None:
			cmd.run('sudo -S killall --signal SIGINT -r .*-softmodem')
			time.sleep(6)

		ret = cmd.run('ps -aux | grep --color=never -e softmodem | grep -v grep')
		result = re.search('-softmodem', ret.stdout)
		if result is not None:
			cmd.run('sudo -S killall --signal SIGKILL -r .*-softmodem')
			time.sleep(5)
		HTML.CreateHtmlTestRowQueue(node, 'OK', ['Undeployment successful'])

		# see InitializeeNB()
		logfile = f'{lSourcePath}/cmake_targets/enb.log'
		logdir = os.path.dirname(logfile)

		file = archiveArtifact(cmd, ctx, logfile)
		cmd.close()
		if file is None:
			logging.debug('\u001B[1;37;41m Could not copy xNB logfile to analyze it! \u001B[0m')
			msg = 'Could not copy xNB logfile to analyze it!'
			HTML.CreateHtmlTestRowQueue('N/A', 'KO', [msg])
			return False

		logging.debug('\u001B[1m Analyzing xNB logfile \u001B[0m ' + file)
		service_desc = {}
		service_desc["nr-softmodem"] = {'returncode': 0, 'logfile': file}
		success = cls_analysis.AnalyzeServices(HTML, service_desc, to_analyze)
		return success

	def AnalyzeRTStats(HTML, node, ctx, thresholds):
		logging.info(f'Analyzing realtime stats from server: {node}')
		lSourcePath = ctx.g.workspace

		logdir = f'{lSourcePath}/cmake_targets'
		with cls_cmd.getConnection(node) as cmd:
			l1_file = archiveArtifact(cmd, ctx, f"{logdir}/nrL1_stats.log")
			mac_file = archiveArtifact(cmd, ctx, f"{logdir}/nrMAC_stats.log")

		logging.info(f"check against thresholds from {thresholds}")
		success, datalog_rt_stats = cls_analysis.Analysis.analyze_rt_stats(thresholds, [l1_file, mac_file])
		HTML.CreateHtmlDataLogTable(datalog_rt_stats, thresholds)
		return success
