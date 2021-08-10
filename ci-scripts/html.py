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
import time
import subprocess
from multiprocessing import Process, Lock, SimpleQueue

import constants as CONST

#-----------------------------------------------------------
# Class Declaration
#-----------------------------------------------------------
class HTMLManagement():

	def __init__(self):

		self.htmlFile = ''
		self.htmlHeaderCreated = False
		self.htmlFooterCreated = False

		self.ranRepository = ''
		self.ranBranch = ''
		self.ranCommitID = ''
		self.ranAllowMerge = False
		self.ranTargetBranch = ''

		self.nbTestXMLfiles = 0
		self.htmlTabRefs = []
		self.htmlTabNames = []
		self.htmlTabIcons = []
		self.testXMLfiles = []

		self.testUnstable = False
		self.testMinStableId = '999999'
		self.testStabilityPointReached = False

		self.htmleNBFailureMsg = ''
		self.htmlUEFailureMsg = ''

		self.startTime = int(round(time.time() * 1000))
		self.testCase_id = ''
		self.desc = ''

		self.OsVersion = ['', '']
		self.KernelVersion = ['', '']
		self.UhdVersion = ['', '']
		self.UsrpBoard = ['', '']
		self.CpuNb = ['', '']
		self.CpuModel = ['', '']
		self.CpuMHz = ['', '']

#-----------------------------------------------------------
# Setters and Getters
#-----------------------------------------------------------

	def SethtmlUEConnected(self, nbUEs):
		if nbUEs > 0:
			self.htmlUEConnected = nbUEs
		else:
			self.htmlUEConnected = 1



#-----------------------------------------------------------
# HTML structure creation functions
#-----------------------------------------------------------


	def CreateHtmlHeader(self, ADBIPAddress):
		if (not self.htmlHeaderCreated):
			logging.debug('\u001B[1m----------------------------------------\u001B[0m')
			logging.debug('\u001B[1m  Creating HTML header \u001B[0m')
			logging.debug('\u001B[1m----------------------------------------\u001B[0m')
			self.htmlFile = open('test_results.html', 'w')
			self.htmlFile.write('<!DOCTYPE html>\n')
			self.htmlFile.write('<html class="no-js" lang="en-US">\n')
			self.htmlFile.write('<head>\n')
			self.htmlFile.write('  <meta name="viewport" content="width=device-width, initial-scale=1">\n')
			self.htmlFile.write('  <link rel="stylesheet" href="https://maxcdn.bootstrapcdn.com/bootstrap/3.3.7/css/bootstrap.min.css">\n')
			self.htmlFile.write('  <script src="https://ajax.googleapis.com/ajax/libs/jquery/3.3.1/jquery.min.js"></script>\n')
			self.htmlFile.write('  <script src="https://maxcdn.bootstrapcdn.com/bootstrap/3.3.7/js/bootstrap.min.js"></script>\n')
			self.htmlFile.write('  <title>Test Results for TEMPLATE_JOB_NAME job build #TEMPLATE_BUILD_ID</title>\n')
			self.htmlFile.write('</head>\n')
			self.htmlFile.write('<body><div class="container">\n')
			self.htmlFile.write('  <br>\n')
			self.htmlFile.write('  <table style="border-collapse: collapse; border: none;">\n')
			self.htmlFile.write('    <tr style="border-collapse: collapse; border: none;">\n')
			self.htmlFile.write('      <td style="border-collapse: collapse; border: none;">\n')
			self.htmlFile.write('        <a href="http://www.openairinterface.org/">\n')
			self.htmlFile.write('           <img src="http://www.openairinterface.org/wp-content/uploads/2016/03/cropped-oai_final_logo2.png" alt="" border="none" height=50 width=150>\n')
			self.htmlFile.write('           </img>\n')
			self.htmlFile.write('        </a>\n')
			self.htmlFile.write('      </td>\n')
			self.htmlFile.write('      <td style="border-collapse: collapse; border: none; vertical-align: center;">\n')
			self.htmlFile.write('        <b><font size = "6">Job Summary -- Job: TEMPLATE_JOB_NAME -- Build-ID: TEMPLATE_BUILD_ID</font></b>\n')
			self.htmlFile.write('      </td>\n')
			self.htmlFile.write('    </tr>\n')
			self.htmlFile.write('  </table>\n')
			self.htmlFile.write('  <br>\n')
			self.htmlFile.write('  <div class="alert alert-info"><strong> <span class="glyphicon glyphicon-dashboard"></span> TEMPLATE_STAGE_NAME</strong></div>\n')
			self.htmlFile.write('  <table border = "1">\n')
			self.htmlFile.write('     <tr>\n')
			self.htmlFile.write('       <td bgcolor = "lightcyan" > <span class="glyphicon glyphicon-time"></span> Build Start Time (UTC) </td>\n')
			self.htmlFile.write('       <td>TEMPLATE_BUILD_TIME</td>\n')
			self.htmlFile.write('     </tr>\n')
			self.htmlFile.write('     <tr>\n')
			self.htmlFile.write('       <td bgcolor = "lightcyan" > <span class="glyphicon glyphicon-cloud-upload"></span> GIT Repository </td>\n')
			self.htmlFile.write('       <td><a href="' + self.ranRepository + '">' + self.ranRepository + '</a></td>\n')
			self.htmlFile.write('     </tr>\n')
			self.htmlFile.write('     <tr>\n')
			self.htmlFile.write('       <td bgcolor = "lightcyan" > <span class="glyphicon glyphicon-wrench"></span> Job Trigger </td>\n')
			if (self.ranAllowMerge):
				self.htmlFile.write('       <td>Merge-Request</td>\n')
			else:
				self.htmlFile.write('       <td>Push to Branch</td>\n')
			self.htmlFile.write('     </tr>\n')
			self.htmlFile.write('     <tr>\n')
			if (self.ranAllowMerge):
				self.htmlFile.write('       <td bgcolor = "lightcyan" > <span class="glyphicon glyphicon-log-out"></span> Source Branch </td>\n')
			else:
				self.htmlFile.write('       <td bgcolor = "lightcyan" > <span class="glyphicon glyphicon-tree-deciduous"></span> Branch</td>\n')
			self.htmlFile.write('       <td>' + self.ranBranch + '</td>\n')
			self.htmlFile.write('     </tr>\n')
			self.htmlFile.write('     <tr>\n')
			if (self.ranAllowMerge):
				self.htmlFile.write('       <td bgcolor = "lightcyan" > <span class="glyphicon glyphicon-tag"></span> Source Commit ID </td>\n')
			else:
				self.htmlFile.write('       <td bgcolor = "lightcyan" > <span class="glyphicon glyphicon-tag"></span> Commit ID </td>\n')
			self.htmlFile.write('       <td>' + self.ranCommitID + '</td>\n')
			self.htmlFile.write('     </tr>\n')
			if self.ranAllowMerge != '':
				commit_message = subprocess.check_output("git log -n1 --pretty=format:\"%s\" " + self.ranCommitID, shell=True, universal_newlines=True)
				commit_message = commit_message.strip()
				self.htmlFile.write('     <tr>\n')
				if (self.ranAllowMerge):
					self.htmlFile.write('       <td bgcolor = "lightcyan" > <span class="glyphicon glyphicon-comment"></span> Source Commit Message </td>\n')
				else:
					self.htmlFile.write('       <td bgcolor = "lightcyan" > <span class="glyphicon glyphicon-comment"></span> Commit Message </td>\n')
				self.htmlFile.write('       <td>' + commit_message + '</td>\n')
				self.htmlFile.write('     </tr>\n')
			if (self.ranAllowMerge):
				self.htmlFile.write('     <tr>\n')
				self.htmlFile.write('       <td bgcolor = "lightcyan" > <span class="glyphicon glyphicon-log-in"></span> Target Branch </td>\n')
				if (self.ranTargetBranch == ''):
					self.htmlFile.write('       <td>develop</td>\n')
				else:
					self.htmlFile.write('       <td>' + self.ranTargetBranch + '</td>\n')
				self.htmlFile.write('     </tr>\n')
			self.htmlFile.write('  </table>\n')

			if (ADBIPAddress != 'none') and (ADBIPAddress != 'modules'):
				self.htmlFile.write('  <h2><span class="glyphicon glyphicon-phone"></span> <span class="glyphicon glyphicon-menu-right"></span> ' + str(self.htmlNb_Smartphones) + ' UE(s) is(are) connected to ADB bench server</h2>\n')
				self.htmlFile.write('  <h2><span class="glyphicon glyphicon-phone"></span> <span class="glyphicon glyphicon-menu-right"></span> ' + str(self.htmlNb_CATM_Modules) + ' CAT-M UE(s) is(are) connected to bench server</h2>\n')
			elif (ADBIPAddress == 'modules'):
				self.htmlUEConnected = 1
				self.htmlFile.write('  <h2><span class="glyphicon glyphicon-phone"></span> <span class="glyphicon glyphicon-menu-right"></span> 1 commercial module is connected to CI bench</h2>\n')
			else:
				self.htmlUEConnected = 1
				self.htmlFile.write('  <h2><span class="glyphicon glyphicon-phone"></span> <span class="glyphicon glyphicon-menu-right"></span> 1 OAI UE(s) is(are) connected to CI bench</h2>\n')
			self.htmlFile.write('  <br>\n')
			self.htmlFile.write('  <ul class="nav nav-pills">\n')
			count = 0
			while (count < self.nbTestXMLfiles):
				pillMsg = '    <li><a data-toggle="pill" href="#'
				pillMsg += self.htmlTabRefs[count]
				pillMsg += '">'
				pillMsg += '__STATE_' + self.htmlTabNames[count] + '__'
				pillMsg += self.htmlTabNames[count]
				pillMsg += ' <span class="glyphicon glyphicon-'
				pillMsg += self.htmlTabIcons[count]
				pillMsg += '"></span></a></li>\n'
				self.htmlFile.write(pillMsg)
				count += 1
			self.htmlFile.write('  </ul>\n')
			self.htmlFile.write('  <div class="tab-content">\n')
			self.htmlFile.close()

	def CreateHtmlTabHeader(self):
		if (not self.htmlHeaderCreated):
			if (not os.path.isfile('test_results.html')):
				self.CreateHtmlHeader('none')
			self.htmlFile = open('test_results.html', 'a')
			if (self.nbTestXMLfiles == 1):
				self.htmlFile.write('  <div id="' + self.htmlTabRefs[0] + '" class="tab-pane fade">\n')
				self.htmlFile.write('  <h3>Test Summary for <span class="glyphicon glyphicon-file"></span> ' + self.testXMLfiles[0] + '</h3>\n')
			else:
				self.htmlFile.write('  <div id="build-tab" class="tab-pane fade">\n')
			self.htmlFile.write('  <table class="table" border = "1">\n')
			self.htmlFile.write('      <tr bgcolor = "#33CCFF" >\n')
			self.htmlFile.write('        <th>Relative Time (s)</th>\n')
			self.htmlFile.write('        <th>Test Id</th>\n')
			self.htmlFile.write('        <th>Test Desc</th>\n')
			self.htmlFile.write('        <th>Test Options</th>\n')
			self.htmlFile.write('        <th>Test Status</th>\n')

			i = 0
			while (i < self.htmlUEConnected):
				self.htmlFile.write('        <th>UE' + str(i) + ' Status</th>\n')
				i += 1
			self.htmlFile.write('      </tr>\n')
			self.htmlFile.close()
		self.htmlHeaderCreated = True

	def CreateHtmlTabFooter(self, passStatus):
		if ((not self.htmlFooterCreated) and (self.htmlHeaderCreated)):
			testOkEvenIfUnstable = False
			if self.testUnstable and not passStatus:
				if self.testStabilityPointReached or self.testMinStableId == '999999':
					testOkEvenIfUnstable = True
			self.htmlFile = open('test_results.html', 'a')
			self.htmlFile.write('      <tr>\n')
			self.htmlFile.write('        <th bgcolor = "#33CCFF" colspan=3>Final Tab Status</th>\n')
			if passStatus:
				self.htmlFile.write('        <th bgcolor = "green" colspan=' + str(2 + self.htmlUEConnected) + '><font color="white">PASS <span class="glyphicon glyphicon-ok"></span> </font></th>\n')
			else:
				if testOkEvenIfUnstable:
					self.htmlFile.write('        <th bgcolor = "orange" colspan=' + str(2 + self.htmlUEConnected) + '><font color="white">KNOWN UNSTABLE SCENARIO <span class="glyphicon glyphicon-exclamation-sign"></span> </font></th>\n')
				else:
					self.htmlFile.write('        <th bgcolor = "red" colspan=' + str(2 + self.htmlUEConnected) + '><font color="white">FAIL <span class="glyphicon glyphicon-remove"></span> </font></th>\n')
			self.htmlFile.write('      </tr>\n')
			self.htmlFile.write('  </table>\n')
			self.htmlFile.write('  </div>\n')
			self.htmlFile.close()
			time.sleep(1)
			if passStatus:
				cmd = "sed -i -e 's/__STATE_" + self.htmlTabNames[0] + "__//' test_results.html"
				subprocess.run(cmd, shell=True)
			else:
				if testOkEvenIfUnstable:
					cmd = "sed -i -e 's/__STATE_" + self.htmlTabNames[0] + "__/<span class=\"glyphicon glyphicon-exclamation-sign\"><\/span>/' test_results.html"
				else:
					cmd = "sed -i -e 's/__STATE_" + self.htmlTabNames[0] + "__/<span class=\"glyphicon glyphicon-remove\"><\/span>/' test_results.html"
				subprocess.run(cmd, shell=True)
		self.htmlFooterCreated = False

	def CreateHtmlFooter(self, passStatus):
		if (os.path.isfile('test_results.html')):
			self.htmlFile = open('test_results.html', 'a')
			self.htmlFile.write('</div>\n')
			self.htmlFile.write('  <p></p>\n')
			self.htmlFile.write('  <table class="table table-condensed">\n')

			machines = [ 'eNB', 'UE' ]
			for machine in machines:
				if machine == 'eNB':
					idx = 0
				else:
					idx = 1
				if self.OsVersion[idx] == '':
					continue

				self.htmlFile.write('      <tr>\n')
				self.htmlFile.write('        <th colspan=8>' + str(machine) + ' Server Characteristics</th>\n')
				self.htmlFile.write('      </tr>\n')
				self.htmlFile.write('      <tr>\n')
				self.htmlFile.write('        <td>OS Version</td>\n')
				self.htmlFile.write('        <td><span class="label label-default">' + self.OsVersion[idx] + '</span></td>\n')
				self.htmlFile.write('        <td>Kernel Version</td>\n')
				self.htmlFile.write('        <td><span class="label label-default">' + self.KernelVersion[idx] + '</span></td>\n')
				self.htmlFile.write('        <td>UHD Version</td>\n')
				self.htmlFile.write('        <td><span class="label label-default">' + self.UhdVersion[idx] + '</span></td>\n')
				self.htmlFile.write('        <td>USRP Board</td>\n')
				self.htmlFile.write('        <td><span class="label label-default">' + self.UsrpBoard[idx] + '</span></td>\n')
				self.htmlFile.write('      </tr>\n')
				self.htmlFile.write('      <tr>\n')
				self.htmlFile.write('        <td>Nb CPUs</td>\n')
				self.htmlFile.write('        <td><span class="label label-default">' + self.CpuNb[idx] + '</span></td>\n')
				self.htmlFile.write('        <td>CPU Model Name</td>\n')
				self.htmlFile.write('        <td><span class="label label-default">' + self.CpuModel[idx] + '</span></td>\n')
				self.htmlFile.write('        <td>CPU Frequency</td>\n')
				self.htmlFile.write('        <td><span class="label label-default">' + self.CpuMHz[idx] + '</span></td>\n')
				self.htmlFile.write('        <td></td>\n')
				self.htmlFile.write('        <td></td>\n')
				self.htmlFile.write('      </tr>\n')

			self.htmlFile.write('      <tr>\n')
			self.htmlFile.write('        <th colspan=5 bgcolor = "#33CCFF">Final Status</th>\n')
			if passStatus:
				self.htmlFile.write('        <th colspan=3 bgcolor="green"><font color="white">PASS <span class="glyphicon glyphicon-ok"></span></font></th>\n')
			else:
				self.htmlFile.write('        <th colspan=3 bgcolor="red"><font color="white">FAIL <span class="glyphicon glyphicon-remove"></span> </font></th>\n')
			self.htmlFile.write('      </tr>\n')
			self.htmlFile.write('  </table>\n')
			self.htmlFile.write('  <p></p>\n')
			self.htmlFile.write('  <div class="well well-lg">End of Test Report -- Copyright <span class="glyphicon glyphicon-copyright-mark"></span> 2018 <a href="http://www.openairinterface.org/">OpenAirInterface</a>. All Rights Reserved.</div>\n')
			self.htmlFile.write('</div></body>\n')
			self.htmlFile.write('</html>\n')
			self.htmlFile.close()

	def CreateHtmlRetrySeparator(self, cntnumfails):
		if ((not self.htmlFooterCreated) and (self.htmlHeaderCreated)):
			self.htmlFile = open('test_results.html', 'a')
			self.htmlFile.write('      <tr bgcolor = "#F0F0F0" >\n')
			self.htmlFile.write('        <td colspan=' + str(5+self.htmlUEConnected) + '><b> ---- Try Run #' + str(cntnumfails) + ' ---- </b></td>\n')
			self.htmlFile.write('      </tr>\n')
			self.htmlFile.close()

	def CreateHtmlTestRow(self, options, status, processesStatus, machine='eNB'):
		if (self.htmlFooterCreated or (not self.htmlHeaderCreated)):
			return
		self.htmlFile = open('test_results.html', 'a')
		currentTime = int(round(time.time() * 1000)) - self.startTime
		self.htmlFile.write('      <tr>\n')
		self.htmlFile.write('        <td bgcolor = "lightcyan" >' + format(currentTime / 1000, '.1f') + '</td>\n')
		self.htmlFile.write('        <td bgcolor = "lightcyan" >' + self.testCase_id  + '</td>\n')
		self.htmlFile.write('        <td>' + self.desc  + '</td>\n')
		self.htmlFile.write('        <td>' + str(options)  + '</td>\n')
		if (str(status) == 'OK'):
			self.htmlFile.write('        <td bgcolor = "lightgreen" >' + str(status)  + '</td>\n')
		elif (str(status) == 'KO'):
			if (processesStatus == 0):
				self.htmlFile.write('        <td bgcolor = "lightcoral" >' + str(status)  + '</td>\n')
			elif (processesStatus == CONST.ENB_PROCESS_FAILED):
				self.htmlFile.write('        <td bgcolor = "lightcoral" >KO - eNB process not found</td>\n')
			elif (processesStatus == CONST.OAI_UE_PROCESS_FAILED):
				self.htmlFile.write('        <td bgcolor = "lightcoral" >KO - OAI UE process not found</td>\n')
			elif (processesStatus == CONST.ENB_PROCESS_SEG_FAULT) or (processesStatus == CONST.OAI_UE_PROCESS_SEG_FAULT):
				self.htmlFile.write('        <td bgcolor = "lightcoral" >KO - ' + machine + ' process ended in Segmentation Fault</td>\n')
			elif (processesStatus == CONST.ENB_PROCESS_ASSERTION) or (processesStatus == CONST.OAI_UE_PROCESS_ASSERTION):
				self.htmlFile.write('        <td bgcolor = "lightcoral" >KO - ' + machine + ' process ended in Assertion</td>\n')
			elif (processesStatus == CONST.ENB_PROCESS_REALTIME_ISSUE):
				self.htmlFile.write('        <td bgcolor = "lightcoral" >KO - ' + machine + ' process faced Real Time issue(s)</td>\n')
			elif (processesStatus == CONST.ENB_PROCESS_NOLOGFILE_TO_ANALYZE) or (processesStatus == CONST.OAI_UE_PROCESS_NOLOGFILE_TO_ANALYZE):
				self.htmlFile.write('        <td bgcolor = "orange" >OK?</td>\n')
			elif (processesStatus == CONST.ENB_PROCESS_SLAVE_RRU_NOT_SYNCED):
				self.htmlFile.write('        <td bgcolor = "lightcoral" >KO - ' + machine + ' Slave RRU could not synch</td>\n')
			elif (processesStatus == CONST.OAI_UE_PROCESS_COULD_NOT_SYNC):
				self.htmlFile.write('        <td bgcolor = "lightcoral" >KO - UE could not sync</td>\n')
			elif (processesStatus == CONST.HSS_PROCESS_FAILED):
				self.htmlFile.write('        <td bgcolor = "lightcoral" >KO - HSS process not found</td>\n')
			elif (processesStatus == CONST.MME_PROCESS_FAILED):
				self.htmlFile.write('        <td bgcolor = "lightcoral" >KO - MME process not found</td>\n')
			elif (processesStatus == CONST.SPGW_PROCESS_FAILED):
				self.htmlFile.write('        <td bgcolor = "lightcoral" >KO - SPGW process not found</td>\n')
			elif (processesStatus == CONST.UE_IP_ADDRESS_ISSUE):
				self.htmlFile.write('        <td bgcolor = "lightcoral" >KO - Could not retrieve UE IP address</td>\n')
			elif (processesStatus == CONST.PHYSIM_IMAGE_ABSENT):
				self.htmlFile.write('        <td bgcolor = "lightcoral" >KO - No such image oai-physim</td>\n')
			elif (processesStatus == CONST.OC_LOGIN_FAIL):
				self.htmlFile.write('        <td bgcolor = "lightcoral" >KO - Could not log onto cluster</td>\n')
			elif (processesStatus == CONST.OC_PROJECT_FAIL):
				self.htmlFile.write('        <td bgcolor = "lightcoral" >KO - Could not register into cluster project</td>\n')
			elif (processesStatus == CONST.OC_IS_FAIL):
				self.htmlFile.write('        <td bgcolor = "lightcoral" >KO - Could not create Image Stream</td>\n')
			elif (processesStatus == CONST.OC_PHYSIM_DEPLOY_FAIL):
				self.htmlFile.write('        <td bgcolor = "lightcoral" >KO - Could not properly deploy physim on cluster</td>\n')
			else:
				self.htmlFile.write('        <td bgcolor = "lightcoral" >' + str(status)  + '</td>\n')
		else:
			self.htmlFile.write('        <td bgcolor = "orange" >' + str(status)  + '</td>\n')
		if (len(str(self.htmleNBFailureMsg)) > 2):
			cellBgColor = 'white'
			result = re.search('ended with|faced real time issues', self.htmleNBFailureMsg)
			if result is not None:
				cellBgColor = 'red'
			else:
				result = re.search('showed|Reestablishment|Could not copy eNB logfile', self.htmleNBFailureMsg)
				if result is not None:
					cellBgColor = 'orange'
			self.htmlFile.write('        <td bgcolor = "' + cellBgColor + '" colspan=' + str(self.htmlUEConnected) + '><pre style="background-color:' + cellBgColor + '">' + self.htmleNBFailureMsg + '</pre></td>\n')
			self.htmleNBFailureMsg = ''
		elif (len(str(self.htmlUEFailureMsg)) > 2):
			cellBgColor = 'white'
			result = re.search('ended with|faced real time issues', self.htmlUEFailureMsg)
			if result is not None:
				cellBgColor = 'red'
			else:
				result = re.search('showed|Could not copy UE logfile|oaitun_ue1 interface is either NOT mounted or NOT configured', self.htmlUEFailureMsg)
				if result is not None:
					cellBgColor = 'orange'
			self.htmlFile.write('        <td bgcolor = "' + cellBgColor + '" colspan=' + str(self.htmlUEConnected) + '><pre style="background-color:' + cellBgColor + '">' + self.htmlUEFailureMsg + '</pre></td>\n')
			self.htmlUEFailureMsg = ''
		else:
			i = 0
			while (i < self.htmlUEConnected):
				self.htmlFile.write('        <td>-</td>\n')
				i += 1
		self.htmlFile.write('      </tr>\n')
		self.htmlFile.close()


	def CreateHtmlNextTabHeaderTestRow(self, collectInfo, allImagesSize, machine='eNB'):
		if (self.htmlFooterCreated or (not self.htmlHeaderCreated)):
			return
		self.htmlFile = open('test_results.html', 'a')
		if bool(collectInfo) == False:
			self.htmlFile.write('      <tr bgcolor = "red" >\n')
			self.htmlFile.write('        <td colspan=' + str(5+self.htmlUEConnected) + '><b> ----IMAGES BUILDING FAILED - Unable to recover the image logs ---- </b></td>\n')
			self.htmlFile.write('      </tr>\n')
		else:
			for image in collectInfo:
				files = collectInfo[image]
        		# TabHeader for image logs on built shared and target images
				if allImagesSize[image].count('unknown') > 0:
					self.htmlFile.write('      <tr bgcolor = "orange" >\n')
				elif allImagesSize[image].count('Build Failed') > 0:
					self.htmlFile.write('      <tr bgcolor = "red" >\n')
				else:
					self.htmlFile.write('      <tr bgcolor = "#F0F0F0" >\n')
				self.htmlFile.write('        <td colspan=' + str(5+self.htmlUEConnected) + '><b> ---- ' + image  + ' IMAGE STATUS ----> Size ' + allImagesSize[image] + ' </b></td>\n')
				self.htmlFile.write('      </tr>\n')
				self.htmlFile.write('      <tr bgcolor = "#33CCFF" >\n')
				self.htmlFile.write('        <th colspan="2">Element</th>\n')
				self.htmlFile.write('        <th>Nb Errors</th>\n')
				self.htmlFile.write('        <th>Nb Warnings</th>\n')
				self.htmlFile.write('        <th colspan=' + str(1+self.htmlUEConnected) + '>Status</th>\n')
				self.htmlFile.write('      </tr>\n')

				for fil in files:
					parameters = files[fil]
					# TestRow for image logs on built shared and target images
					self.htmlFile.write('      <tr>\n')
					self.htmlFile.write('        <td colspan="2" bgcolor = "lightcyan" >' + fil  + ' </td>\n')
					if (parameters['errors'] == 0):
						self.htmlFile.write('        <td bgcolor = "green" >' + str(parameters['errors'])  + '</td>\n')
					else:
						self.htmlFile.write('        <td bgcolor = "red" >' + str(parameters['errors'])  + '</td>\n')
					if (parameters['errors'] > 0):
						self.htmlFile.write('        <td bgcolor = "red" >' + str(parameters['warnings'])  + '</td>\n')
					elif (parameters['warnings'] == 0):
						self.htmlFile.write('        <td bgcolor = "green" >' + str(parameters['warnings'])  + '</td>\n')
					elif ((parameters['warnings'] > 0) and (parameters['warnings'] <= 20)):
						self.htmlFile.write('        <td bgcolor = "orange" >' + str(parameters['warnings'])  + '</td>\n')
					else:
						self.htmlFile.write('        <td bgcolor = "red" >' + str(parameters['warnings'])  + '</td>\n')
					if (parameters['errors'] == 0) and (parameters['warnings'] == 0):
						self.htmlFile.write('        <th colspan=' + str(1+self.htmlUEConnected) + ' bgcolor = "green" ><font color="white">OK </font></th>\n')
					elif (parameters['errors'] == 0) and ((parameters['warnings'] > 0) and (parameters['warnings'] <= 20)):
						self.htmlFile.write('        <th colspan=' + str(1+self.htmlUEConnected) + ' bgcolor = "orange" ><font color="white">OK </font></th>\n')
					else:
						self.htmlFile.write('        <th colspan=' + str(1+self.htmlUEConnected) + ' bgcolor = "red" > NOT OK  </th>\n')
					self.htmlFile.write('      </tr>\n')
		self.htmlFile.close()

	#for the moment it is limited to 4 columns, to be made generic later
	def CreateHtmlDataLogTable(self, DataLog):
		if (self.htmlFooterCreated or (not self.htmlHeaderCreated)):
			return
		self.htmlFile = open('test_results.html', 'a')
		
        # TabHeader 
		self.htmlFile.write('      <tr bgcolor = "#F0F0F0" >\n')
		self.htmlFile.write('        <td colspan=' + str(5+self.htmlUEConnected) + '><b> ---- ' + DataLog['Title'] + ' ---- </b></td>\n')
		self.htmlFile.write('      </tr>\n')
		self.htmlFile.write('      <tr bgcolor = "#33CCFF" >\n')
		self.htmlFile.write('        <th colspan="3">'+ DataLog['ColNames'][0] +'</th>\n')
		self.htmlFile.write('        <th>' + DataLog['ColNames'][1] + '</th>\n')
		self.htmlFile.write('        <th>' + DataLog['ColNames'][2] + '</th>\n')
		self.htmlFile.write('        <th colspan=' + str(1+self.htmlUEConnected) + '>'+ DataLog['ColNames'][3] +'</th>\n')
		self.htmlFile.write('      </tr>\n')

		for k in DataLog['Data']:
			# TestRow 
			self.htmlFile.write('      <tr>\n')
			self.htmlFile.write('        <td colspan="3" bgcolor = "lightcyan" >' + k  + ' </td>\n')				
			self.htmlFile.write('        <td bgcolor = "lightcyan" >' + DataLog['Data'][k][0]  + ' </td>\n')
			self.htmlFile.write('        <td bgcolor = "lightcyan" >' + DataLog['Data'][k][1]  + ' </td>\n')
			if float(DataLog['Data'][k][2])> DataLog['Threshold'][k]:
				self.htmlFile.write('        <th bgcolor = "red" >' + DataLog['Data'][k][2]  + ' (Ref = ' + str(DataLog['Ref'][k]) + ' ; Thres = '   +str(DataLog['Threshold'][k])+') ' + '</th>\n')
			else:
				self.htmlFile.write('        <th bgcolor = "green" ><font color="white">' + DataLog['Data'][k][2]  + ' (Ref = ' + str(DataLog['Ref'][k]) + ' ; Thres = '   +str(DataLog['Threshold'][k])+') ' + '</th>\n')					
			self.htmlFile.write('      </tr>\n')
		self.htmlFile.close()


	def CreateHtmlTestRowQueue(self, options, status, ue_status, ue_queue):
		if ((not self.htmlFooterCreated) and (self.htmlHeaderCreated)):
			self.htmlFile = open('test_results.html', 'a')
			currentTime = int(round(time.time() * 1000)) - self.startTime
			addOrangeBK = False
			self.htmlFile.write('      <tr>\n')
			self.htmlFile.write('        <td bgcolor = "lightcyan" >' + format(currentTime / 1000, '.1f') + '</td>\n')
			self.htmlFile.write('        <td bgcolor = "lightcyan" >' + self.testCase_id  + '</td>\n')
			self.htmlFile.write('        <td>' + self.desc  + '</td>\n')
			self.htmlFile.write('        <td>' + str(options)  + '</td>\n')
			if (str(status) == 'OK'):
				self.htmlFile.write('        <td bgcolor = "lightgreen" >' + str(status)  + '</td>\n')
			elif (str(status) == 'KO'):
				self.htmlFile.write('        <td bgcolor = "lightcoral" >' + str(status)  + '</td>\n')
			else:
				addOrangeBK = True
				self.htmlFile.write('        <td bgcolor = "orange" >' + str(status)  + '</td>\n')
			i = 0
			while (i < self.htmlUEConnected):
				if (i < ue_status):
					if (not ue_queue.empty()):
						if (addOrangeBK):
							self.htmlFile.write('        <td bgcolor = "orange" >' + str(ue_queue.get()).replace('white', 'orange') + '</td>\n')
						else:
							self.htmlFile.write('        <td>' + str(ue_queue.get()) + '</td>\n')
					else:
						self.htmlFile.write('        <td>-</td>\n')
				else:
					self.htmlFile.write('        <td>-</td>\n')
				i += 1
			self.htmlFile.write('      </tr>\n')
			self.htmlFile.close()

	def CreateHtmlTestRowCppCheckResults(self, CCR):
		if (self.htmlFooterCreated or (not self.htmlHeaderCreated)):
			return
		self.htmlFile = open('test_results.html', 'a')
		vId = 0
		for version in CCR.versions:
			self.htmlFile.write('      <tr bgcolor = "#F0F0F0" >\n')
			self.htmlFile.write('        <td colspan=' + str(5+self.htmlUEConnected) + '><b> Results for cppcheck v ' + version + ' </b></td>\n')
			self.htmlFile.write('      </tr>\n')
			self.htmlFile.write('      <tr>\n')
			self.htmlFile.write('        <td></td>\n')
			self.htmlFile.write('        <td colspan=2 bgcolor = "lightcyan" ><b> NB ERRORS</b></td>\n')
			if CCR.nbErrors[vId] == 0:
				myColor = 'lightgreen'
			elif CCR.nbErrors[vId] < 20:
				myColor = 'orange'
			else:
				myColor = 'lightcoral'
			self.htmlFile.write('        <td colspan=' + str(2+self.htmlUEConnected) + ' bgcolor = "' + myColor + '"><b>' + str(CCR.nbErrors[vId]) + '</b></td>\n')
			self.htmlFile.write('      </tr>\n')
			self.htmlFile.write('      <tr>\n')
			self.htmlFile.write('        <td></td>\n')
			self.htmlFile.write('        <td colspan=2 bgcolor = "lightcyan" ><b> NB WARNINGS</b></td>\n')
			if CCR.nbWarnings[vId] == 0:
				myColor = 'lightgreen'
			elif CCR.nbWarnings[vId] < 20:
				myColor = 'orange'
			else:
				myColor = 'lightcoral'
			self.htmlFile.write('        <td colspan=' + str(2+self.htmlUEConnected) + ' bgcolor = "' + myColor + '"><b>' + str(CCR.nbWarnings[vId]) + '</b></td>\n')
			self.htmlFile.write('      </tr>\n')
			self.htmlFile.write('      <tr bgcolor = "#F0F0F0" >\n')
			self.htmlFile.write('        <td colspan=' + str(5+self.htmlUEConnected) + '> ----------------- </td>\n')
			self.htmlFile.write('      </tr>\n')
			self.htmlFile.write('      <tr>\n')
			self.htmlFile.write('        <td></td>\n')
			self.htmlFile.write('        <td colspan=2 bgcolor = "lightcyan" ><b> Memory leak</b></td>\n')
			self.htmlFile.write('        <td colspan=' + str(2+self.htmlUEConnected) + '>' + str(CCR.nbMemLeaks[vId]) + '</td>\n')
			self.htmlFile.write('      </tr>\n')
			self.htmlFile.write('      <tr>\n')
			self.htmlFile.write('        <td></td>\n')
			self.htmlFile.write('        <td colspan=2 bgcolor = "lightcyan" ><b> Possible null pointer deference</b></td>\n')
			self.htmlFile.write('        <td colspan=' + str(2+self.htmlUEConnected) + '>' + str(CCR.nbNullPtrs[vId]) + '</td>\n')
			self.htmlFile.write('      </tr>\n')
			self.htmlFile.write('      <tr>\n')
			self.htmlFile.write('        <td></td>\n')
			self.htmlFile.write('        <td colspan=2 bgcolor = "lightcyan" ><b> Uninitialized variable</b></td>\n')
			self.htmlFile.write('        <td colspan=' + str(2+self.htmlUEConnected) + '>' + str(CCR.nbUninitVars[vId]) + '</td>\n')
			self.htmlFile.write('      </tr>\n')
			self.htmlFile.write('      <tr>\n')
			self.htmlFile.write('        <td></td>\n')
			self.htmlFile.write('        <td colspan=2 bgcolor = "lightcyan" ><b> Undefined behaviour shifting</b></td>\n')
			self.htmlFile.write('        <td colspan=' + str(2+self.htmlUEConnected) + '>' + str(CCR.nbTooManyBitsShift[vId]) + '</td>\n')
			self.htmlFile.write('      </tr>\n')
			self.htmlFile.write('      <tr>\n')
			self.htmlFile.write('        <td></td>\n')
			self.htmlFile.write('        <td colspan=2 bgcolor = "lightcyan" ><b> Signed integer overflow</b></td>\n')
			self.htmlFile.write('        <td colspan=' + str(2+self.htmlUEConnected) + '>' + str(CCR.nbIntegerOverflow[vId]) + '</td>\n')
			self.htmlFile.write('      </tr>\n')
			self.htmlFile.write('      <tr bgcolor = "#F0F0F0" >\n')
			self.htmlFile.write('        <td colspan=' + str(5+self.htmlUEConnected) + '> </td>\n')
			self.htmlFile.write('      </tr>\n')
			self.htmlFile.write('      <tr>\n')
			self.htmlFile.write('        <td></td>\n')
			self.htmlFile.write('        <td colspan=2 bgcolor = "lightcyan" ><b> Printf formatting issues</b></td>\n')
			self.htmlFile.write('        <td colspan=' + str(2+self.htmlUEConnected) + '>' + str(CCR.nbInvalidPrintf[vId]) + '</td>\n')
			self.htmlFile.write('      </tr>\n')
			self.htmlFile.write('      <tr>\n')
			self.htmlFile.write('        <td></td>\n')
			self.htmlFile.write('        <td colspan=2 bgcolor = "lightcyan" ><b> Modulo result is predetermined</b></td>\n')
			self.htmlFile.write('        <td colspan=' + str(2+self.htmlUEConnected) + '>' + str(CCR.nbModuloAlways[vId]) + '</td>\n')
			self.htmlFile.write('      </tr>\n')
			self.htmlFile.write('      <tr>\n')
			self.htmlFile.write('        <td></td>\n')
			self.htmlFile.write('        <td colspan=2 bgcolor = "lightcyan" ><b> Opposite Condition -> dead code</b></td>\n')
			self.htmlFile.write('        <td colspan=' + str(2+self.htmlUEConnected) + '>' + str(CCR.nbOppoInnerCondition[vId]) + '</td>\n')
			self.htmlFile.write('      </tr>\n')
			self.htmlFile.write('      <tr>\n')
			self.htmlFile.write('        <td></td>\n')
			self.htmlFile.write('        <td colspan=2 bgcolor = "lightcyan" ><b> Wrong Scanf Nb Args</b></td>\n')
			self.htmlFile.write('        <td colspan=' + str(2+self.htmlUEConnected) + '>' + str(CCR.nbWrongScanfArg[vId]) + '</td>\n')
			self.htmlFile.write('      </tr>\n')
			vId += 1

	def CreateHtmlTestRowPhySimTestResult(self, testSummary, testResult):
		if (self.htmlFooterCreated or (not self.htmlHeaderCreated)):
			return
		self.htmlFile = open('test_results.html', 'a')
		if bool(testResult) == False and bool(testSummary) == False:
			self.htmlFile.write('      <tr bgcolor = "red" >\n')
			self.htmlFile.write('        <td colspan=' + str(5+self.htmlUEConnected) + '><b> ----PHYSIM TESTING FAILED - Unable to recover the test logs ---- </b></td>\n')
			self.htmlFile.write('      </tr>\n')
		else:
		# Tab header
			self.htmlFile.write('      <tr bgcolor = "#F0F0F0" >\n')
			self.htmlFile.write('        <td colspan=' + str(5+self.htmlUEConnected) + '><b> ---- PHYSIM TEST SUMMARY---- </b></td>\n')
			self.htmlFile.write('      </tr>\n')
			self.htmlFile.write('      <tr bgcolor = "#33CCFF" >\n')
			self.htmlFile.write('        <th colspan="2">LogFile Name</th>\n')
			self.htmlFile.write('        <th colspan="2">Nb Tests</th>\n')
			self.htmlFile.write('        <th>Nb Failure</th>\n')
			self.htmlFile.write('        <th>Nb Pass</th>\n')
			self.htmlFile.write('      </tr>\n')
			self.htmlFile.write('      <tr>\n')
			self.htmlFile.write('        <td colspan="2" bgcolor = "lightcyan" > physim_test.txt  </td>\n')
			self.htmlFile.write('        <td colspan="2" bgcolor = "lightcyan" >' + str(testSummary['Nbtests']) + ' </td>\n')
			if testSummary['Nbfail'] == 0:
				self.htmlFile.write('        <td bgcolor = "lightcyan" >' + str(testSummary['Nbfail']) + ' </td>\n')
			else:
				self.htmlFile.write('        <td bgcolor = "red" >' + str(testSummary['Nbfail']) + ' </td>\n')
			self.htmlFile.write('        <td gcolor = "lightcyan" >' + str(testSummary['Nbpass']) + ' </td>\n')
			self.htmlFile.write('      </tr>\n')
			self.htmlFile.write('      <tr bgcolor = "#F0F0F0" >\n')
			self.htmlFile.write('        <td colspan=' + str(5+self.htmlUEConnected) + '><b> ---- PHYSIM TEST DETAIL INFO---- </b></td>\n')
			self.htmlFile.write('      </tr>\n')
			self.htmlFile.write('      <tr bgcolor = "#33CCFF" >\n')
			self.htmlFile.write('        <th colspan="2">Test Name</th>\n')
			self.htmlFile.write('        <th colspan="2">Test Description</th>\n')
			self.htmlFile.write('        <th colspan=' + str(1+self.htmlUEConnected) + '>Result</th>\n')
			self.htmlFile.write('      </tr>\n')
			y = ''
			for key, value in testResult.items():
				x = key.split(".")
				if x[0] != y:
					self.htmlFile.write('      <tr bgcolor = "lightgreen" >\n')
					self.htmlFile.write('        <td style="text-align: center;" colspan=' + str(5+self.htmlUEConnected) + '><b>"' + x[0] + '" series </b></td>\n')
					self.htmlFile.write('      </tr>\n')
					y = x[0]
				self.htmlFile.write('      <tr>\n')
				self.htmlFile.write('        <td colspan="2" bgcolor = "lightcyan" >' + key  + ' </td>\n')
				self.htmlFile.write('        <td colspan="2" bgcolor = "lightcyan" >' + value[0]  + '</td>\n')
				if 'PASS' in value:
					self.htmlFile.write('        <td colspan=' + str(1+self.htmlUEConnected) + ' bgcolor = "green" >' + value[1]  + '</td>\n')
				else:
					self.htmlFile.write('        <td colspan=' + str(1+self.htmlUEConnected) + ' bgcolor = "red" >' + value[1]  + '</td>\n')

		self.htmlFile.close()
