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
import sys              # arg
import re               # reg
import logging
import os
from pathlib import Path
import time
from multiprocessing import Process, Lock, SimpleQueue

#-----------------------------------------------------------
# OAI Testing modules
#-----------------------------------------------------------
import helpreadme as HELP
import constants as CONST
import cls_cmd
from cls_containerize import CreateWorkspace

#-----------------------------------------------------------
# Class Declaration
#-----------------------------------------------------------
class CppCheckResults():

	def __init__(self):

		self.variants = ['bionic', 'focal']
		self.versions = ['','']
		self.nbErrors = [0,0]
		self.nbWarnings = [0,0]
		self.nbNullPtrs = [0,0]
		self.nbMemLeaks = [0,0]
		self.nbUninitVars = [0,0]
		self.nbInvalidPrintf = [0,0]
		self.nbModuloAlways = [0,0]
		self.nbTooManyBitsShift = [0,0]
		self.nbIntegerOverflow = [0,0]
		self.nbWrongScanfArg = [0,0]
		self.nbPtrAddNotNull = [0,0]
		self.nbOppoInnerCondition = [0,0]

class StaticCodeAnalysis():

	def __init__(self):

		self.ranRepository = ''
		self.ranBranch = ''
		self.ranAllowMerge = False
		self.ranCommitID = ''
		self.ranTargetBranch = ''
		self.eNBIPAddress = ''
		self.eNBUserName = ''
		self.eNBPassword = ''
		self.eNBSourceCodePath = ''

	def CppCheckAnalysis(self, HTML):
		if self.ranRepository == '' or self.ranBranch == '' or self.ranCommitID == '':
			HELP.GenericHelp(CONST.Version)
			sys.exit('Insufficient Parameter')
		lIpAddr = self.eNBIPAddress
		lUserName = self.eNBUserName
		lPassWord = self.eNBPassword
		lSourcePath = self.eNBSourceCodePath

		if lIpAddr == '' or lUserName == '' or lPassWord == '' or lSourcePath == '':
			HELP.GenericHelp(CONST.Version)
			sys.exit('Insufficient Parameter')
		logging.debug('Building on server: ' + lIpAddr)
		cmd = cls_cmd.getConnection(lIpAddr)
		self.testCase_id = HTML.testCase_id
		# on RedHat/CentOS .git extension is mandatory
		result = re.search('([a-zA-Z0-9\:\-\.\/])+\.git', self.ranRepository)
		if result is not None:
			full_ran_repo_name = self.ranRepository.replace('git/', 'git')
		else:
			full_ran_repo_name = self.ranRepository + '.git'

		CreateWorkspace(cmd, lSourcePath, full_ran_repo_name, self.ranCommitID, self.ranTargetBranch, self.ranAllowMerge)

		logDir = f'{lSourcePath}/cmake_targets/build_log_{self.testCase_id}'
		cmd.run(f'mkdir -p {logDir}')
		cmd.run('docker image rm oai-cppcheck:bionic oai-cppcheck:focal')
		cmd.run(f'sed -e "s@xenial@bionic@" {lSourcePath}/ci-scripts/docker/Dockerfile.cppcheck.xenial > {lSourcePath}/ci-scripts/docker/Dockerfile.cppcheck.bionic')
		cmd.run(f'docker build --tag oai-cppcheck:bionic --file {lSourcePath}/ci-scripts/docker/Dockerfile.cppcheck.bionic . > {logDir}/cppcheck-bionic.txt 2>&1')
		cmd.run(f'sed -e "s@xenial@focal@" {lSourcePath}/ci-scripts/docker/Dockerfile.cppcheck.xenial > {lSourcePath}/ci-scripts/docker/Dockerfile.cppcheck.focal')
		cmd.run(f'docker build --tag oai-cppcheck:focal --file {lSourcePath}/ci-scripts/docker/Dockerfile.cppcheck.focal . > {logDir}/cppcheck-focal.txt 2>&1')
		cmd.run('docker image rm oai-cppcheck:bionic oai-cppcheck:focal')

		# Analyzing the logs
		cmd.copyin(f'{logDir}/cppcheck-bionic.txt', 'cppcheck-bionic.txt')
		cmd.copyin(f'{logDir}/cppcheck-focal.txt', 'cppcheck-focal.txt')
		cmd.close()

		CCR = CppCheckResults()
		CCR_ref = CppCheckResults()
		vId = 0
		for variant in CCR.variants:
			refAvailable = False
			if self.ranAllowMerge:
				refFolder = str(Path.home()) + '/cppcheck-references'
				if (os.path.isfile(refFolder + '/cppcheck-'+ variant + '.txt')):
					refAvailable = True
					with open(refFolder + '/cppcheck-'+ variant + '.txt', 'r') as refFile:
						for line in refFile:
							ret = re.search(' (?P<nb_errors>[0-9\.]+) errors', str(line))
							if ret is not None:
								CCR_ref.nbErrors[vId] = int(ret.group('nb_errors'))
							ret = re.search(' (?P<nb_warnings>[0-9\.]+) warnings', str(line))
							if ret is not None:
								CCR_ref.nbWarnings[vId] = int(ret.group('nb_warnings'))
			if (os.path.isfile('./cppcheck-'+ variant + '.txt')):
				xmlStart = False
				with open('./cppcheck-'+ variant + '.txt', 'r') as logfile:
					for line in logfile:
						ret = re.search('cppcheck version="(?P<version>[0-9\.]+)"', str(line))
						if ret is not None:
						   CCR.versions[vId] = ret.group('version')
						if re.search('RUN cat cmake_targets/log/cppcheck.xml', str(line)) is not None:
							xmlStart = True
						if xmlStart:
							if re.search('severity="error"', str(line)) is not None:
								CCR.nbErrors[vId] += 1
							if re.search('severity="warning"', str(line)) is not None:
								CCR.nbWarnings[vId] += 1
							if re.search('id="memleak"', str(line)) is not None:
								CCR.nbMemLeaks[vId] += 1
							if re.search('id="nullPointer"', str(line)) is not None:
								CCR.nbNullPtrs[vId] += 1
							if re.search('id="uninitvar"', str(line)) is not None:
								CCR.nbUninitVars[vId] += 1
							if re.search('id="invalidPrintfArgType_sint"|id="invalidPrintfArgType_uint"', str(line)) is not None:
								CCR.nbInvalidPrintf[vId] += 1
							if re.search('id="moduloAlwaysTrueFalse"', str(line)) is not None:
								CCR.nbModuloAlways[vId] += 1
							if re.search('id="shiftTooManyBitsSigned"', str(line)) is not None:
								CCR.nbTooManyBitsShift[vId] += 1
							if re.search('id="integerOverflow"', str(line)) is not None:
								CCR.nbIntegerOverflow[vId] += 1
							if re.search('id="wrongPrintfScanfArgNum"|id="invalidScanfArgType_int"', str(line)) is not None:
								CCR.nbWrongScanfArg[vId] += 1
							if re.search('id="pointerAdditionResultNotNull"', str(line)) is not None:
								CCR.nbPtrAddNotNull[vId] += 1
							if re.search('id="oppositeInnerCondition"', str(line)) is not None:
								CCR.nbOppoInnerCondition[vId] += 1
			vMsg  = ''
			vMsg += '========  Variant ' + variant + ' - ' + CCR.versions[vId] + ' ========\n'
			vMsg += '   ' + str(CCR.nbErrors[vId]) + ' errors\n'
			vMsg += '   ' + str(CCR.nbWarnings[vId]) + ' warnings\n'
			vMsg += '  -- Details --\n'
			vMsg += '   Memory leak:                     ' + str(CCR.nbMemLeaks[vId]) + '\n'
			vMsg += '   Possible null pointer deference: ' + str(CCR.nbNullPtrs[vId]) + '\n'
			vMsg += '   Uninitialized variable:          ' + str(CCR.nbUninitVars[vId]) + '\n'
			vMsg += '   Undefined behaviour shifting:    ' + str(CCR.nbTooManyBitsShift[vId]) + '\n'
			vMsg += '   Signed integer overflow:         ' + str(CCR.nbIntegerOverflow[vId]) + '\n'
			vMsg += '\n'
			vMsg += '   Printf formatting issue:         ' + str(CCR.nbInvalidPrintf[vId]) + '\n'
			vMsg += '   Modulo result is predetermined:  ' + str(CCR.nbModuloAlways[vId]) + '\n'
			vMsg += '   Opposite Condition -> dead code: ' + str(CCR.nbOppoInnerCondition[vId]) + '\n'
			vMsg += '   Wrong Scanf Nb Args:             ' + str(CCR.nbWrongScanfArg[vId]) + '\n'
			for vLine in vMsg.split('\n'):
				logging.debug(vLine)
			if self.ranAllowMerge and refAvailable:
				if CCR_ref.nbErrors[vId] == CCR.nbErrors[vId]:
					logging.debug('   No change in number of errors')
				elif CCR_ref.nbErrors[vId] > CCR.nbErrors[vId]:
					logging.debug('   Good! Decrease in number of errors')
				else:
					logging.debug('   Bad! increase in number of errors')
				if CCR_ref.nbWarnings[vId] == CCR.nbWarnings[vId]:
					logging.debug('   No change in number of warnings')
				elif CCR_ref.nbWarnings[vId] > CCR.nbWarnings[vId]:
					logging.debug('   Good! Decrease in number of warnings')
				else:
					logging.debug('   Bad! increase in number of warnings')
			# Create new reference file
			if not self.ranAllowMerge:
				refFolder = str(Path.home()) + '/cppcheck-references'
				if not os.path.isdir(refFolder):
					os.mkdir(refFolder)
				with open(refFolder + '/cppcheck-'+ variant + '.txt', 'w') as refFile:
					refFile.write(vMsg)
			vId += 1

		HTML.CreateHtmlTestRow('N/A', 'OK', CONST.ALL_PROCESSES_OK)
		HTML.CreateHtmlTestRowCppCheckResults(CCR)
		logging.info('\u001B[1m Static Code Analysis Pass\u001B[0m')

		return 0

	def LicenceAndFormattingCheck(self, HTML):
		if self.ranRepository == '' or self.ranBranch == '' or self.ranCommitID == '':
			HELP.GenericHelp(CONST.Version)
			sys.exit('Insufficient Parameter')
		lIpAddr = self.eNBIPAddress
		lUserName = self.eNBUserName
		lPassWord = self.eNBPassword
		lSourcePath = self.eNBSourceCodePath

		if lIpAddr == '' or lUserName == '' or lPassWord == '' or lSourcePath == '':
			HELP.GenericHelp(CONST.Version)
			sys.exit('Insufficient Parameter')
		logging.debug('Building on server: ' + lIpAddr)
		cmd = cls_cmd.getConnection(lIpAddr)
		self.testCase_id = HTML.testCase_id
		# on RedHat/CentOS .git extension is mandatory
		result = re.search('([a-zA-Z0-9\:\-\.\/])+\.git', self.ranRepository)
		if result is not None:
			full_ran_repo_name = self.ranRepository.replace('git/', 'git')
		else:
			full_ran_repo_name = self.ranRepository + '.git'

		CreateWorkspace(cmd, lSourcePath, full_ran_repo_name, self.ranCommitID, self.ranTargetBranch, self.ranAllowMerge)
		check_options = ''
		if self.ranAllowMerge:
			check_options = f'--build-arg MERGE_REQUEST=true --build-arg SRC_BRANCH={self.ranBranch}'
			if self.ranTargetBranch == '':
				if self.ranBranch != 'develop' and self.ranBranch != 'origin/develop':
					check_options += ' --build-arg TARGET_BRANCH=develop'
			else:
				check_options += f' --build-arg TARGET_BRANCH={self.ranTargetBranch}'

		logDir = f'{lSourcePath}/cmake_targets/build_log_{self.testCase_id}'
		cmd.run(f'mkdir -p {logDir}')
		cmd.run('docker image rm oai-formatting-check:latest')
		cmd.run(f'docker build --target oai-formatting-check --tag oai-formatting-check:latest {check_options} --file {lSourcePath}/ci-scripts/docker/Dockerfile.formatting.bionic . > {logDir}/oai-formatting-check.txt 2>&1')

		cmd.run('docker image rm oai-formatting-check:latest')
		cmd.run('docker image prune --force')
		cmd.run('docker volume prune --force')

		# Analyzing the logs
		cmd.copyin(f'{logDir}/oai-formatting-check.txt', 'oai-formatting-check.txt')
		cmd.close()

		finalStatus = 0
		if (os.path.isfile('./oai-formatting-check.txt')):
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
			with open('./oai-formatting-check.txt', 'r') as logfile:
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

		return finalStatus
