# SPDX-License-Identifier: LicenseRef-CSSL-1.0

#---------------------------------------------------------------------
# Python for CI of OAI-eNB + COTS-UE
#
#   Required Python Version
#     Python 3.x
#---------------------------------------------------------------------

#-----------------------------------------------------------
# Import
#-----------------------------------------------------------
import re               # reg
import fileinput
import time
from datetime import datetime, timezone
import subprocess

def tabRef(idx):
	# implicit, unique HTML anchor for the tab of the idx'th scenario
	return f"test-tab-{idx}"

#-----------------------------------------------------------
# Class Declaration
#-----------------------------------------------------------
class HTMLManagement():

	def __init__(self):

		self.startTime = int(round(time.time() * 1000))
		self.testCaseIdx = ''
		self.desc = ''

#-----------------------------------------------------------
# HTML structure creation functions
#-----------------------------------------------------------


	def CreateHtmlHeader(self, repository, branch, xmls):
		with open('test_results.html', 'w') as f:
			f.write('<!DOCTYPE html>\n')
			f.write('<html class="no-js" lang="en-US">\n')
			f.write('<head>\n')
			f.write('  <meta name="viewport" content="width=device-width, initial-scale=1">\n')
			f.write('  <link rel="stylesheet" href="https://maxcdn.bootstrapcdn.com/bootstrap/3.3.7/css/bootstrap.min.css">\n')
			f.write('  <script src="https://ajax.googleapis.com/ajax/libs/jquery/3.3.1/jquery.min.js"></script>\n')
			f.write('  <script src="https://maxcdn.bootstrapcdn.com/bootstrap/3.3.7/js/bootstrap.min.js"></script>\n')
			f.write('  <title>Test Results for TEMPLATE_JOB_NAME job build #TEMPLATE_BUILD_ID</title>\n')
			f.write('</head>\n')
			f.write('<body><div class="container-fluid" style="margin-left:1em; margin-right:1em">\n')
			f.write('  <br>\n')
			f.write('  <table style="border-collapse: collapse; border: none;">\n')
			f.write('    <tr style="border-collapse: collapse; border: none;">\n')
			f.write('      <td style="border-collapse: collapse; border: none;">\n')
			f.write('        <a href="http://www.openairinterface.org/">\n')
			f.write('           <img src="https://raw.githubusercontent.com/duranta-project/governance/main/logos/Duranta-Logo-Color.png" alt="" border="none" style="margin-right: 2rem;" width=150>\n')
			f.write('           </img>\n')
			f.write('        </a>\n')
			f.write('      </td>\n')
			f.write('      <td style="border-collapse: collapse; border: none; vertical-align: center;">\n')
			f.write('        <b><font size = "6">TEMPLATE_JOB_NAME -- Build-ID: TEMPLATE_BUILD_ID</font></b>\n')
			f.write('      </td>\n')
			f.write('    </tr>\n')
			f.write('  </table>\n')
			f.write('  <br>\n')
			f.write('  <table border = "1">\n')
			f.write('     <tr>\n')
			f.write('       <td bgcolor = "lightcyan" > <span class="glyphicon glyphicon-time"></span> Build Start Time (UTC) </td>\n')
			now = datetime.now(timezone.utc).strftime("%Y-%m-%d %H:%M:%S%z")
			f.write(f'       <td>{now}</td>\n')
			f.write('     </tr>\n')
			f.write('     <tr>\n')
			f.write('       <td bgcolor = "lightcyan" > <span class="glyphicon glyphicon-cloud-upload"></span> GIT Repository </td>\n')
			f.write('       <td><a href="' + repository + '">' + repository + '</a></td>\n')
			f.write('     </tr>\n')
			f.write('     <tr>\n')
			f.write('       <td bgcolor = "lightcyan" > <span class="glyphicon glyphicon-log-out"></span> Test Branch </td>\n')
			f.write('       <td>' + branch + '</td>\n')
			f.write('     </tr>\n')
			commit_id = subprocess.check_output("git log -n1 --pretty=format:\"%H\" ", shell=True, universal_newlines=True)
			commit_id = commit_id.strip()
			f.write('     <tr>\n')
			f.write('       <td bgcolor = "lightcyan" > <span class="glyphicon glyphicon-tag"></span> Commit ID </td>\n')
			f.write('       <td>' + commit_id + '</td>\n')
			f.write('     </tr>\n')
			commit_message = subprocess.check_output("git log -n1 --pretty=format:\"%s\" ", shell=True, universal_newlines=True)
			commit_message = commit_message.strip()
			f.write('     <tr>\n')
			f.write('       <td bgcolor = "lightcyan" > <span class="glyphicon glyphicon-comment"></span> Commit Message </td>\n')
			f.write('       <td>' + commit_message + '</td>\n')
			f.write('     </tr>\n')
			f.write('  </table>\n')

			f.write('  <br>\n')
			f.write('  <ul class="nav nav-pills" style="display: flex; flex-wrap: wrap;">\n')
			for i, xml in enumerate(xmls):
				active = ' class="active"' if i == 0 else ''
				f.write(f'    <li{active}><a data-toggle="pill" href="#{tabRef(i)}">__STATE_{i}__{xml.title}</a></li>\n')
			f.write('  </ul>\n')
			f.write('  <div class="tab-content">\n')

	def CreateHtmlTabHeader(self, xml, idx):
		with open('test_results.html', 'a') as f:
			f.write(f'  <div id="{tabRef(idx)}" class="tab-pane fade">\n')
			f.write(f'  <h3>Test Summary for <span class="glyphicon glyphicon-file"></span> {xml}</h3>\n')
			f.write('  <table class="table" border = "1">\n')
			f.write('      <tr bgcolor = "#33CCFF" >\n')
			f.write('        <th style="width:5%">Relative Time (s)</th>\n')
			f.write('        <th style="width:5%">Test Index</th>\n')
			f.write('        <th>Test Desc</th>\n')
			f.write('        <th>Test Options</th>\n')
			f.write('        <th style="width:5%">Test Status</th>\n')

			f.write('        <th>Info</th>\n')
			f.write('      </tr>\n')

	def CreateHtmlTabFooter(self, passStatus, idx):
		with open('test_results.html', 'a') as f:
			f.write('      <tr>\n')
			f.write('        <th bgcolor = "#33CCFF" colspan="3">Final Tab Status</th>\n')
			if passStatus:
				f.write('        <th bgcolor = "green" colspan="3"><font color="white">PASS <span class="glyphicon glyphicon-ok"></span> </font></th>\n')
			else:
				f.write('        <th bgcolor = "red" colspan="3"><font color="white">FAIL <span class="glyphicon glyphicon-remove"></span> </font></th>\n')
			f.write('      </tr>\n')
			f.write('  </table>\n')
			f.write('  </div>\n')
		if passStatus:
			cmd = f"sed -i -e 's/__STATE_{idx}__//' test_results.html"
			subprocess.run(cmd, shell=True)
		else:
			cmd = f"sed -i -e 's/__STATE_{idx}" + r"__/<span class=\"glyphicon glyphicon-remove\"><\/span>/' test_results.html"
			subprocess.run(cmd, shell=True)

	def CreateHtmlFooter(self, passStatus):
		# Tagging the 1st tab as active so it is automatically opened.
		firstTabFound = False
		for line in fileinput.FileInput("test_results.html", inplace=1):
			if re.search('tab-pane fade', line) and not firstTabFound:
				firstTabFound = True
				print(line.replace('tab-pane fade', 'tab-pane fade in active'), end ='')
			else:
				print(line, end ='')
		with open('test_results.html', 'a') as f:
			f.write('</div>\n')
			f.write('  <p></p>\n')
			f.write('  <table class="table table-condensed">\n')

			f.write('      <tr>\n')
			f.write('        <th colspan="5" bgcolor = "#33CCFF">Final Status</th>\n')
			if passStatus:
				f.write('        <th colspan="3" bgcolor="green"><font color="white">PASS <span class="glyphicon glyphicon-ok"></span></font></th>\n')
			else:
				f.write('        <th colspan="3" bgcolor="red"><font color="white">FAIL <span class="glyphicon glyphicon-remove"></span> </font></th>\n')
			f.write('      </tr>\n')
			f.write('  </table>\n')
			f.write('  <p></p>\n')
			f.write('  <div class="well well-lg">End of Test Report -- Copyright <span class="glyphicon glyphicon-copyright-mark"></span> 2026 <a href="http://www.openairinterface.org/">OpenAirInterface</a>. All Rights Reserved.</div>\n')
			f.write('</div></body>\n')
			f.write('</html>\n')

	#for the moment it is limited to 4 columns, to be made generic later
	def CreateHtmlDataLogTable(self, DataLog, filename):
		with open('test_results.html', 'a') as f:
			# TabHeader
			f.write('      <tr bgcolor = "#F0F0F0" >\n')
			f.write(f'        <td colspan="6"><b> ---- Processing Time from {filename} ---- </b></td>\n')
			f.write('      </tr>\n')
			f.write('      <tr bgcolor = "#33CCFF" >\n')
			f.write('        <th colspan="3">'+ DataLog['ColNames'][0] +'</th>\n')
			f.write('        <th colspan="2">' + DataLog['ColNames'][1] + '</th>\n')
			f.write('        <th colspan="2">'+ DataLog['ColNames'][2] +'</th>\n')
			f.write('      </tr>\n')

			for k in DataLog['Data']:
				# TestRow
				avg = DataLog['Data'][k][0]
				maxval = DataLog['Data'][k][1]
				count = DataLog['Data'][k][2]
				valnorm = float(DataLog['Data'][k][3])
				dev = DataLog['DeviationThreshold'][k]
				ref = DataLog['Ref'][k]
				f.write('      <tr>\n')
				f.write('        <td colspan="3" bgcolor = "lightcyan" >' + k  + ' </td>\n')
				f.write(f'        <td colspan="2" bgcolor = "lightcyan" >{avg}; {maxval}; {count}</td>\n')
				if valnorm > 1.0 + dev or valnorm < 1.0 - dev:
					f.write(f'        <th bgcolor = "red" >{valnorm} (Avg over Ref = {avg} over {ref}; max allowed deviation = {dev})</th>\n')
				else:
					f.write(f'        <th bgcolor = "green" ><font color="white">{valnorm} (Avg over Ref = {avg} over {ref}; max allowed deviation = {dev})</font></th>\n')
				f.write('      </tr>\n')


	def CreateHtmlTestRowQueue(self, options, status, infoList):
		with open('test_results.html', 'a') as f:
			currentTime = int(round(time.time() * 1000)) - self.startTime
			addOrangeBK = False
			f.write('      <tr>\n')
			f.write('        <td bgcolor = "lightcyan" >' + format(currentTime / 1000, '.1f') + '</td>\n')
			f.write('        <td bgcolor = "lightcyan" >' + self.testCaseIdx  + '</td>\n')
			f.write('        <td>' + self.desc  + '</td>\n')
			f.write('        <td>' + str(options)  + '</td>\n')
			if (str(status) == 'OK'):
				f.write(f'        <td bgcolor = "lightgreen" >{status}</td>\n')
			elif (str(status) == 'KO'):
				f.write(f'        <td bgcolor = "lightcoral" >{status}</td>\n')
			elif str(status) == 'SKIP':
				f.write(f'        <td bgcolor = "lightgray" >{status}</td>\n')
			else:
				addOrangeBK = True
				f.write(f'        <td bgcolor = "orange" >{status}</td>\n')
			if (addOrangeBK):
				f.write('        <td bgcolor = "orange" >')
			else:
				f.write('        <td>')
			for i in infoList: # add custom style to have elements side-by-side to reduce need for vertical space
				f.write(f'         <pre style="display: inline flow-root list-item; margin: 0 3px 0 3px; min-width: 24em;">{i}</pre>')

			f.write('                </td>')
			f.write('      </tr>\n')

	def CreateHtmlTestRowPhySimTestResult(self, testSummary, testResult):
		with open('test_results.html', 'a') as f:
			if bool(testResult) == False and bool(testSummary) == False:
				f.write('      <tr bgcolor = "red" >\n')
				f.write('        <td colspan="6"><b> ----PHYSIM TESTING FAILED - Unable to recover the test logs ---- </b></td>\n')
				f.write('      </tr>\n')
			else:
			# Tab header
				f.write('      <tr bgcolor = "#F0F0F0" >\n')
				f.write('        <td colspan="6"><b> ---- PHYSIM TEST SUMMARY---- </b></td>\n')
				f.write('      </tr>\n')
				f.write('      <tr bgcolor = "#33CCFF" >\n')
				f.write('        <th colspan="2">LogFile Name</th>\n')
				f.write('        <th colspan="2">Nb Tests</th>\n')
				f.write('        <th>Nb Failure</th>\n')
				f.write('        <th>Nb Pass</th>\n')
				f.write('      </tr>\n')
				f.write('      <tr>\n')
				f.write('        <td colspan="2" bgcolor = "lightcyan" > physim_log.txt  </td>\n')
				f.write('        <td colspan="2" bgcolor = "lightcyan" >' + str(testSummary['Nbtests']) + ' </td>\n')
				if testSummary['Nbfail'] == 0:
					f.write('        <td bgcolor = "lightcyan" >' + str(testSummary['Nbfail']) + '</td>\n')
				else:
					f.write('        <td bgcolor = "red" ><font color="white">' + str(testSummary['Nbfail']) + '</font></td>\n')
				f.write('        <td bgcolor = "lightcyan" >' + str(testSummary['Nbpass']) + ' </td>\n')
				f.write('      </tr>\n')
				f.write('      <tr bgcolor = "#F0F0F0" >\n')
				f.write('        <td colspan="6"><b> ---- PHYSIM TEST DETAIL INFO---- </b></td>\n')
				f.write('      </tr>\n')
				f.write('      <tr bgcolor = "#33CCFF" >\n')
				f.write('        <th colspan="2">Test Name</th>\n')
				f.write('        <th colspan="2">Test Description</th>\n')
				f.write('        <th>Test Status</th>\n')
				f.write('        <th>Info</th>\n')
				f.write('      </tr>\n')
				y = ''
				for key, value in testResult.items():
					x = key.split(".")
					if x[2] != y:
						f.write('      <tr bgcolor = "lightgreen" >\n')
						f.write('        <td style="text-align: center;" colspan="6"><b>"' + x[2] + '" series </b></td>\n')
						f.write('      </tr>\n')
						y = x[2]
					f.write('      <tr>\n')
					f.write('        <td colspan="2" bgcolor = "lightcyan" >' + key  + ' </td>\n')
					f.write('        <td colspan="2" bgcolor = "lightcyan" >' + value[0]  + '</td>\n')
					if 'PASS' in value:
						f.write('        <td bgcolor = "green" ><font color="white"><b>' + value[2]  + '</b></font></td>\n')
					else:
						f.write('        <td bgcolor = "red" ><font color="white"><b>' + value[2]  + '</b></font></td>\n')
					f.write(f'        <td colspan="2" bgcolor = "lightcyan"><pre style="display: inline flow-root list-item; margin: 0 3px 0 3px; min-width: 24em;">{value[1]}</pre></td>\n')

