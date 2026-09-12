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
import sys              # arg
import re               # reg
import logging
import os
from pathlib import Path

#-----------------------------------------------------------
# OAI Testing modules
#-----------------------------------------------------------
import constants as CONST
import cls_cmd
from cls_ci_helper import archiveArtifact

class StaticCodeAnalysis():

	def LicenceAndFormattingCheck(ctx, node, HTML):
		# Workspace is no longer recreated from scratch.
		# It implies that this method shall be called last within a build pipeline
		# where workspace is already created

		d = ctx.g.workspace
		if not node or not d:
			raise ValueError(f"{d=} {node=}")
		logging.debug('Building on server: ' + node)
		cmd = cls_cmd.getConnection(node)
		check_options = ''
		if ctx.g.merge:
			branch = ctx.g.branch
			check_options = f'--build-arg MERGE_REQUEST=true --build-arg SRC_BRANCH={branch}'
			if ctx.g.targetBranch == '':
				if branch != 'develop' and branch != 'origin/develop':
					check_options += ' --build-arg TARGET_BRANCH=develop'
			else:
				check_options += f' --build-arg TARGET_BRANCH={ctx.g.targetBranch}'

		logDir = f'{d}/cmake_targets/log/'
		cmd.run(f'mkdir -p {logDir}')
		cmd.run('docker image rm oai-formatting-check:latest')
		cmd.run(f'docker build --target oai-formatting-check --tag oai-formatting-check:latest {check_options} --file {d}/ci-scripts/docker/Dockerfile.formatting.ubuntu {d} > {logDir}/oai-formatting-check.txt 2>&1')

		cmd.run('docker image rm oai-formatting-check:latest')
		cmd.run('docker image prune --force')
		cmd.run('docker volume prune --force')

		file = archiveArtifact(cmd, ctx, f'{logDir}/oai-formatting-check.txt')
		cmd.close()

		finalStatus = 0
		if (os.path.isfile(file)):
			analyzed = False
			nbFilesNotFormatted = 0
			listFiles = False
			listFilesNotFormatted = []
			circularHeaderDependency = False
			circularHeaderDependencyFiles = []
			gnuGplLicence = False
			gnuGplLicenceFiles = []
			suspectLicence = False
			suspectLicenceFiles = []
			with open(file, 'r') as logfile:
				for line in logfile:
					ret = re.search('./ci-scripts/checkCodingFormattingRules.sh', str(line))
					if ret is not None:
						analyzed = True
					if analyzed:
						if re.search('=== Files with incorrect define protection ===', str(line)) is not None:
							circularHeaderDependency = True
						if circularHeaderDependency:
							if re.search('DONE', str(line)) is not None:
								circularHeaderDependency = False
							elif re.search('Running in|Files with incorrect define protection', str(line)) is not None:
								pass
							else:
								circularHeaderDependencyFiles.append(str(line).strip())

						if re.search('=== Files with a GNU GPL licence Banner ===', str(line)) is not None:
							gnuGplLicence = True
						if gnuGplLicence:
							if re.search('DONE', str(line)) is not None:
								gnuGplLicence = False
							elif re.search('Running in|Files with a GNU GPL licence Banner', str(line)) is not None:
								pass
							else:
								gnuGplLicenceFiles.append(str(line).strip())

						if re.search('=== Files with a suspect Banner ===', str(line)) is not None:
							suspectLicence = True
						if suspectLicence:
							if re.search('DONE', str(line)) is not None:
								suspectLicence = False
							elif re.search('Running in|Files with a suspect Banner', str(line)) is not None:
								pass
							else:
								suspectLicenceFiles.append(str(line).strip())

				logfile.close()
			if analyzed:
				logging.debug('files not formatted properly: ' + str(nbFilesNotFormatted))
				if nbFilesNotFormatted == 0:
					HTML.CreateHtmlTestRow('File(s) Format', 'OK', CONST.ALL_PROCESSES_OK)
				else:
					html_cell = f'Number of files not following OAI Rules: {nbFilesNotFormatted}\n'
					for nFile in listFilesNotFormatted:
						html_cell += str(nFile).strip() + '\n'
					HTML.CreateHtmlTestRowQueue('File(s) Format', 'KO', [html_cell])
					del(html_cell)

				logging.debug('header files not respecting the circular dependency protection: ' + str(len(circularHeaderDependencyFiles)))
				if len(circularHeaderDependencyFiles) == 0:
					HTML.CreateHtmlTestRow('Header Circular Dependency', 'OK', CONST.ALL_PROCESSES_OK)
				else:
					html_cell = f'Number of files not respecting: {len(circularHeaderDependencyFiles)}\n'
					for nFile in circularHeaderDependencyFiles:
						html_cell += str(nFile).strip() + '\n'
					HTML.CreateHtmlTestRowQueue('Header Circular Dependency', 'KO', [html_cell])
					del(html_cell)
					finalStatus = -1

				logging.debug('files with a GNU GPL license: ' + str(len(gnuGplLicenceFiles)))
				if len(gnuGplLicenceFiles) == 0:
					HTML.CreateHtmlTestRow('Files w/ GNU GPL License', 'OK', CONST.ALL_PROCESSES_OK)
				else:
					html_cell = f'Number of files not respecting: {len(gnuGplLicenceFiles)}\n'
					for nFile in gnuGplLicenceFiles:
						html_cell += str(nFile).strip() + '\n'
					HTML.CreateHtmlTestRowQueue('Files w/ GNU GPL License', 'KO', [html_cell])
					del(html_cell)
					finalStatus = -1

				logging.debug('files with a suspect license: ' + str(len(suspectLicenceFiles)))
				if len(suspectLicenceFiles) == 0:
					HTML.CreateHtmlTestRow('Files with suspect license', 'OK', CONST.ALL_PROCESSES_OK)
				else:
					html_cell = f'Number of files not respecting: {len(suspectLicenceFiles)}\n'
					for nFile in suspectLicenceFiles:
						html_cell += str(nFile).strip() + '\n'
					HTML.CreateHtmlTestRowQueue('Files with suspect license', 'KO', [html_cell])
					del(html_cell)
					finalStatus = -1

			else:
				finalStatus = -1
				HTML.htmleNBFailureMsg = 'Could not fully analyze oai-formatting-check.txt file'
				HTML.CreateHtmlTestRow('N/A', 'KO', CONST.ENB_PROCESS_NOLOGFILE_TO_ANALYZE)
		else:
			finalStatus = -1
			HTML.htmleNBFailureMsg = 'Could not access oai-formatting-check.txt file'
			HTML.CreateHtmlTestRow('N/A', 'KO', CONST.ENB_PROCESS_NOLOGFILE_TO_ANALYZE)

		return finalStatus == 0
