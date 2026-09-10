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
import fileinput
import logging
import os
import time
import subprocess

import constants as CONST

#-----------------------------------------------------------
# Class Declaration
#-----------------------------------------------------------
class HTMLManagement():

	def __init__(self):

		self.htmlFile = ''
		self.htmlHeaderCreated = False
		self.htmlFooterCreated = False

		self.nbTestXMLfiles = 0
		self.htmlTabRefs = []
		self.htmlTabNames = []
		self.htmlTabIcons = []
		self.testXMLfiles = []

		self.htmleNBFailureMsg = ''
		self.htmlUEFailureMsg = ''

		self.startTime = int(round(time.time() * 1000))
		self.testCaseIdx = ''
		self.desc = ''

#-----------------------------------------------------------
# HTML structure creation functions
#-----------------------------------------------------------


	def CreateHtmlHeader(self, repository, branch):
		if (not self.htmlHeaderCreated):
			logging.info('\u001B[1m----------------------------------------\u001B[0m')
			logging.info('\u001B[1m  Creating HTML header \u001B[0m')
			logging.info('\u001B[1m----------------------------------------\u001B[0m')
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
			self.htmlFile.write('<body><div class="container-fluid" style="margin-left:1em; margin-right:1em">\n')
			self.htmlFile.write('  <br>\n')
			self.htmlFile.write('  <table style="border-collapse: collapse; border: none;">\n')
			self.htmlFile.write('    <tr style="border-collapse: collapse; border: none;">\n')
			self.htmlFile.write('      <td style="border-collapse: collapse; border: none;">\n')
			self.htmlFile.write('        <a href="http://www.openairinterface.org/">\n')
			self.htmlFile.write('           <img src="https://raw.githubusercontent.com/duranta-project/governance/main/logos/Duranta-Logo-Color.png" alt="" border="none" style="margin-right: 2rem;" width=150>\n')
			self.htmlFile.write('           </img>\n')
			self.htmlFile.write('        </a>\n')
			self.htmlFile.write('      </td>\n')
			self.htmlFile.write('      <td style="border-collapse: collapse; border: none; vertical-align: center;">\n')
			self.htmlFile.write('        <b><font size = "6">TEMPLATE_JOB_NAME -- Build-ID: TEMPLATE_BUILD_ID</font></b>\n')
			self.htmlFile.write('      </td>\n')
			self.htmlFile.write('    </tr>\n')
			self.htmlFile.write('  </table>\n')
			self.htmlFile.write('  <br>\n')
			self.htmlFile.write('  <table border = "1">\n')
			self.htmlFile.write('     <tr>\n')
			self.htmlFile.write('       <td bgcolor = "lightcyan" > <span class="glyphicon glyphicon-time"></span> Build Start Time (UTC) </td>\n')
			self.htmlFile.write('       <td>TEMPLATE_BUILD_TIME</td>\n')
			self.htmlFile.write('     </tr>\n')
			self.htmlFile.write('     <tr>\n')
			self.htmlFile.write('       <td bgcolor = "lightcyan" > <span class="glyphicon glyphicon-cloud-upload"></span> GIT Repository </td>\n')
			self.htmlFile.write('       <td><a href="' + repository + '">' + repository + '</a></td>\n')
			self.htmlFile.write('     </tr>\n')
			self.htmlFile.write('     <tr>\n')
			self.htmlFile.write('       <td bgcolor = "lightcyan" > <span class="glyphicon glyphicon-log-out"></span> Test Branch </td>\n')
			self.htmlFile.write('       <td>' + branch + '</td>\n')
			self.htmlFile.write('     </tr>\n')
			commit_id = subprocess.check_output("git log -n1 --pretty=format:\"%H\" ", shell=True, universal_newlines=True)
			commit_id = commit_id.strip()
			self.htmlFile.write('     <tr>\n')
			self.htmlFile.write('       <td bgcolor = "lightcyan" > <span class="glyphicon glyphicon-tag"></span> Commit ID </td>\n')
			self.htmlFile.write('       <td>' + commit_id + '</td>\n')
			self.htmlFile.write('     </tr>\n')
			commit_message = subprocess.check_output("git log -n1 --pretty=format:\"%s\" ", shell=True, universal_newlines=True)
			commit_message = commit_message.strip()
			self.htmlFile.write('     <tr>\n')
			self.htmlFile.write('       <td bgcolor = "lightcyan" > <span class="glyphicon glyphicon-comment"></span> Commit Message </td>\n')
			self.htmlFile.write('       <td>' + commit_message + '</td>\n')
			self.htmlFile.write('     </tr>\n')
			self.htmlFile.write('  </table>\n')

			self.htmlFile.write('  <br>\n')
			self.htmlFile.write('  <ul class="nav nav-pills">\n')
			count = 0
			while (count < self.nbTestXMLfiles):
				if count == 0:
					pillMsg = '    <li class="active"><a data-toggle="pill" href="#'
				else:
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
			self.htmlFile.write('        <th style="width:5%">Relative Time (s)</th>\n')
			self.htmlFile.write('        <th style="width:5%">Test Index</th>\n')
			self.htmlFile.write('        <th>Test Desc</th>\n')
			self.htmlFile.write('        <th>Test Options</th>\n')
			self.htmlFile.write('        <th style="width:5%">Test Status</th>\n')

			self.htmlFile.write('        <th>Info</th>\n')
			self.htmlFile.write('      </tr>\n')
			self.htmlFile.close()
		self.htmlHeaderCreated = True

	def CreateHtmlTabFooter(self, passStatus):
		if ((not self.htmlFooterCreated) and (self.htmlHeaderCreated)):
			self.htmlFile = open('test_results.html', 'a')
			self.htmlFile.write('      <tr>\n')
			self.htmlFile.write('        <th bgcolor = "#33CCFF" colspan="3">Final Tab Status</th>\n')
			if passStatus:
				self.htmlFile.write('        <th bgcolor = "green" colspan="3"><font color="white">PASS <span class="glyphicon glyphicon-ok"></span> </font></th>\n')
			else:
				self.htmlFile.write('        <th bgcolor = "red" colspan="3"><font color="white">FAIL <span class="glyphicon glyphicon-remove"></span> </font></th>\n')
			self.htmlFile.write('      </tr>\n')
			self.htmlFile.write('  </table>\n')
			self.htmlFile.write('  </div>\n')
			self.htmlFile.close()
			if passStatus:
				cmd = "sed -i -e 's/__STATE_" + self.htmlTabNames[0] + "__//' test_results.html"
				subprocess.run(cmd, shell=True)
			else:
				cmd = "sed -i -e 's/__STATE_" + self.htmlTabNames[0] + r"__/<span class=\"glyphicon glyphicon-remove\"><\/span>/' test_results.html"
				subprocess.run(cmd, shell=True)
		self.htmlFooterCreated = False

	def CreateHtmlFooter(self, passStatus):
		if (os.path.isfile('test_results.html')):
			# Tagging the 1st tab as active so it is automatically opened.
			firstTabFound = False
			for line in fileinput.FileInput("test_results.html", inplace=1):
				if re.search('tab-pane fade', line) and not firstTabFound:
					firstTabFound = True
					print(line.replace('tab-pane fade', 'tab-pane fade in active'), end ='')
				else:
					print(line, end ='')

			self.htmlFile = open('test_results.html', 'a')
			self.htmlFile.write('</div>\n')
			self.htmlFile.write('  <p></p>\n')
			self.htmlFile.write('  <table class="table table-condensed">\n')

			self.htmlFile.write('      <tr>\n')
			self.htmlFile.write('        <th colspan="5" bgcolor = "#33CCFF">Final Status</th>\n')
			if passStatus:
				self.htmlFile.write('        <th colspan="3" bgcolor="green"><font color="white">PASS <span class="glyphicon glyphicon-ok"></span></font></th>\n')
			else:
				self.htmlFile.write('        <th colspan="3" bgcolor="red"><font color="white">FAIL <span class="glyphicon glyphicon-remove"></span> </font></th>\n')
			self.htmlFile.write('      </tr>\n')
			self.htmlFile.write('  </table>\n')
			self.htmlFile.write('  <p></p>\n')
			self.htmlFile.write('  <div class="well well-lg">End of Test Report -- Copyright <span class="glyphicon glyphicon-copyright-mark"></span> 2026 <a href="http://www.openairinterface.org/">OpenAirInterface</a>. All Rights Reserved.</div>\n')
			self.htmlFile.write('</div></body>\n')
			self.htmlFile.write('</html>\n')
			self.htmlFile.close()

	def CreateHtmlTestRow(self, options, status, processesStatus, machine='eNB'):
		if (self.htmlFooterCreated or (not self.htmlHeaderCreated)):
			return
		self.htmlFile = open('test_results.html', 'a')
		currentTime = int(round(time.time() * 1000)) - self.startTime
		self.htmlFile.write('      <tr>\n')
		self.htmlFile.write('        <td bgcolor = "lightcyan" >' + format(currentTime / 1000, '.1f') + '</td>\n')
		self.htmlFile.write('        <td bgcolor = "lightcyan" >' + self.testCaseIdx  + '</td>\n')
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
			self.htmlFile.write('        <td bgcolor = "' + cellBgColor + '" colspan="1"><pre style="background-color:' + cellBgColor + '">' + self.htmleNBFailureMsg + '</pre></td>\n')
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
			self.htmlFile.write('        <td bgcolor = "' + cellBgColor + '" colspan="1"><pre style="background-color:' + cellBgColor + '">' + self.htmlUEFailureMsg + '</pre></td>\n')
			self.htmlUEFailureMsg = ''
		else:
			self.htmlFile.write('        <td>-</td>\n')
		self.htmlFile.write('      </tr>\n')
		self.htmlFile.close()

	#for the moment it is limited to 4 columns, to be made generic later
	def CreateHtmlDataLogTable(self, DataLog, filename):
		if (self.htmlFooterCreated or (not self.htmlHeaderCreated)):
			return
		self.htmlFile = open('test_results.html', 'a')
		
        # TabHeader 
		self.htmlFile.write('      <tr bgcolor = "#F0F0F0" >\n')
		self.htmlFile.write(f'        <td colspan="6"><b> ---- Processing Time from {filename} ---- </b></td>\n')
		self.htmlFile.write('      </tr>\n')
		self.htmlFile.write('      <tr bgcolor = "#33CCFF" >\n')
		self.htmlFile.write('        <th colspan="3">'+ DataLog['ColNames'][0] +'</th>\n')
		self.htmlFile.write('        <th colspan="2">' + DataLog['ColNames'][1] + '</th>\n')
		self.htmlFile.write('        <th colspan="2">'+ DataLog['ColNames'][2] +'</th>\n')
		self.htmlFile.write('      </tr>\n')

		for k in DataLog['Data']:
			# TestRow 
			avg = DataLog['Data'][k][0]
			maxval = DataLog['Data'][k][1]
			count = DataLog['Data'][k][2]
			valnorm = float(DataLog['Data'][k][3])
			dev = DataLog['DeviationThreshold'][k]
			ref = DataLog['Ref'][k]
			self.htmlFile.write('      <tr>\n')
			self.htmlFile.write('        <td colspan="3" bgcolor = "lightcyan" >' + k  + ' </td>\n')				
			self.htmlFile.write(f'        <td colspan="2" bgcolor = "lightcyan" >{avg}; {maxval}; {count}</td>\n')
			if valnorm > 1.0 + dev or valnorm < 1.0 - dev:
				self.htmlFile.write(f'        <th bgcolor = "red" >{valnorm} (Avg over Ref = {avg} over {ref}; max allowed deviation = {dev})</th>\n')
			else:
				self.htmlFile.write(f'        <th bgcolor = "green" ><font color="white">{valnorm} (Avg over Ref = {avg} over {ref}; max allowed deviation = {dev})</font></th>\n')
			self.htmlFile.write('      </tr>\n')
		self.htmlFile.close()


	def CreateHtmlTestRowQueue(self, options, status, infoList):
		if ((not self.htmlFooterCreated) and (self.htmlHeaderCreated)):
			self.htmlFile = open('test_results.html', 'a')
			currentTime = int(round(time.time() * 1000)) - self.startTime
			addOrangeBK = False
			self.htmlFile.write('      <tr>\n')
			self.htmlFile.write('        <td bgcolor = "lightcyan" >' + format(currentTime / 1000, '.1f') + '</td>\n')
			self.htmlFile.write('        <td bgcolor = "lightcyan" >' + self.testCaseIdx  + '</td>\n')
			self.htmlFile.write('        <td>' + self.desc  + '</td>\n')
			self.htmlFile.write('        <td>' + str(options)  + '</td>\n')
			if (str(status) == 'OK'):
				self.htmlFile.write(f'        <td bgcolor = "lightgreen" >{status}</td>\n')
			elif (str(status) == 'KO'):
				self.htmlFile.write(f'        <td bgcolor = "lightcoral" >{status}</td>\n')
			elif str(status) == 'SKIP':
				self.htmlFile.write(f'        <td bgcolor = "lightgray" >{status}</td>\n')
			else:
				addOrangeBK = True
				self.htmlFile.write(f'        <td bgcolor = "orange" >{status}</td>\n')
			if (addOrangeBK):
				self.htmlFile.write('        <td bgcolor = "orange" >')
			else:
				self.htmlFile.write('        <td>')
			for i in infoList: # add custom style to have elements side-by-side to reduce need for vertical space
				self.htmlFile.write(f'         <pre style="display: inline flow-root list-item; margin: 0 3px 0 3px; min-width: 24em;">{i}</pre>')

			self.htmlFile.write('                </td>')
			self.htmlFile.write('      </tr>\n')
			self.htmlFile.close()

	def CreateHtmlTestRowPhySimTestResult(self, testSummary, testResult):
		if (self.htmlFooterCreated or (not self.htmlHeaderCreated)):
			return
		self.htmlFile = open('test_results.html', 'a')
		if bool(testResult) == False and bool(testSummary) == False:
			self.htmlFile.write('      <tr bgcolor = "red" >\n')
			self.htmlFile.write('        <td colspan="6"><b> ----PHYSIM TESTING FAILED - Unable to recover the test logs ---- </b></td>\n')
			self.htmlFile.write('      </tr>\n')
		else:
		# Tab header
			self.htmlFile.write('      <tr bgcolor = "#F0F0F0" >\n')
			self.htmlFile.write('        <td colspan="6"><b> ---- PHYSIM TEST SUMMARY---- </b></td>\n')
			self.htmlFile.write('      </tr>\n')
			self.htmlFile.write('      <tr bgcolor = "#33CCFF" >\n')
			self.htmlFile.write('        <th colspan="2">LogFile Name</th>\n')
			self.htmlFile.write('        <th colspan="2">Nb Tests</th>\n')
			self.htmlFile.write('        <th>Nb Failure</th>\n')
			self.htmlFile.write('        <th>Nb Pass</th>\n')
			self.htmlFile.write('      </tr>\n')
			self.htmlFile.write('      <tr>\n')
			self.htmlFile.write('        <td colspan="2" bgcolor = "lightcyan" > physim_log.txt  </td>\n')
			self.htmlFile.write('        <td colspan="2" bgcolor = "lightcyan" >' + str(testSummary['Nbtests']) + ' </td>\n')
			if testSummary['Nbfail'] == 0:
				self.htmlFile.write('        <td bgcolor = "lightcyan" >' + str(testSummary['Nbfail']) + '</td>\n')
			else:
				self.htmlFile.write('        <td bgcolor = "red" ><font color="white">' + str(testSummary['Nbfail']) + '</font></td>\n')
			self.htmlFile.write('        <td bgcolor = "lightcyan" >' + str(testSummary['Nbpass']) + ' </td>\n')
			self.htmlFile.write('      </tr>\n')
			self.htmlFile.write('      <tr bgcolor = "#F0F0F0" >\n')
			self.htmlFile.write('        <td colspan="6"><b> ---- PHYSIM TEST DETAIL INFO---- </b></td>\n')
			self.htmlFile.write('      </tr>\n')
			self.htmlFile.write('      <tr bgcolor = "#33CCFF" >\n')
			self.htmlFile.write('        <th colspan="2">Test Name</th>\n')
			self.htmlFile.write('        <th colspan="2">Test Description</th>\n')
			self.htmlFile.write('        <th>Test Status</th>\n')
			self.htmlFile.write('        <th>Info</th>\n')
			self.htmlFile.write('      </tr>\n')
			y = ''
			for key, value in testResult.items():
				x = key.split(".")
				if x[2] != y:
					self.htmlFile.write('      <tr bgcolor = "lightgreen" >\n')
					self.htmlFile.write('        <td style="text-align: center;" colspan="6"><b>"' + x[2] + '" series </b></td>\n')
					self.htmlFile.write('      </tr>\n')
					y = x[2]
				self.htmlFile.write('      <tr>\n')
				self.htmlFile.write('        <td colspan="2" bgcolor = "lightcyan" >' + key  + ' </td>\n')
				self.htmlFile.write('        <td colspan="2" bgcolor = "lightcyan" >' + value[0]  + '</td>\n')
				if 'PASS' in value:
					self.htmlFile.write('        <td bgcolor = "green" ><font color="white"><b>' + value[2]  + '</b></font></td>\n')
				else:
					self.htmlFile.write('        <td bgcolor = "red" ><font color="white"><b>' + value[2]  + '</b></font></td>\n')
				self.htmlFile.write(f'        <td colspan="2" bgcolor = "lightcyan"><pre style="display: inline flow-root list-item; margin: 0 3px 0 3px; min-width: 24em;">{value[1]}</pre></td>\n')

		self.htmlFile.close()
