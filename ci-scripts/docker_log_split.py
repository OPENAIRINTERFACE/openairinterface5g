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
# Import Libs
#-----------------------------------------------------------
import sys		# arg
import re		# reg
import os
import subprocess


class SplitReport():
	def __init__(self):
		self.logfilename = ''
		self.destinationFolder = ''

	def split(self):
		self.destinationFolder = self.logfilename.replace(".log","")
		if os.path.isfile(self.logfilename):
			newImageLog = open(self.logfilename + '.new', 'w')
			copyFlag = True
			with open(self.logfilename, 'r') as imageLog:
				for line in imageLog:
					header = False
					ret = re.search('====== Start of log for ([0-9\.A-Za-z\-\_]+) ======', line)
					if ret is not None:
						copyFlag = False
						header = True
						detailedLogFile = open(self.destinationFolder + '/' + ret.group(1), 'w')
					if copyFlag:
						newImageLog.write(line)
					ret = re.search('====== End of log for ([0-9\.A-Za-z\-\_]+) ======', line)
					if ret is not None:
						copyFlag = True
						detailedLogFile.close()
					elif not copyFlag and not header:
						detailedLogFile.write(line)
			imageLog.close()
			newImageLog.close()
			os.rename(self.logfilename + '.new', self.logfilename)
		else:
			print('Cannot split unfound file')

#--------------------------------------------------------------------------------------------------------
#
# Start of main
#
#--------------------------------------------------------------------------------------------------------

argvs = sys.argv
argc = len(argvs)

SP = SplitReport()

while len(argvs) > 1:
	myArgv = argvs.pop(1)
	if re.match('^\-\-logfilename=(.+)$', myArgv, re.IGNORECASE):
		matchReg = re.match('^\-\-logfilename=(.+)$', myArgv, re.IGNORECASE)
		SP.logfilename = matchReg.group(1)

SP.split()

sys.exit(0)
