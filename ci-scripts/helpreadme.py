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
# Functions Declaration
#-----------------------------------------------------------

def GenericHelp(vers):
	print('----------------------------------------------------------------------------------------------------------------------')
	print('main.py Ver: ' + vers)
	print('----------------------------------------------------------------------------------------------------------------------')
	print('python main.py [options]')
	print('  --help  Show this help.')
	print('  --mode=[Mode]')
	print('      TesteNB')
	print('      InitiateHtml, FinalizeHtml')
	print('      TerminateeNB, TerminateHSS, TerminateMME, TerminateSPGW')
	print('      LogCollectBuild, LogCollecteNB, LogCollectHSS, LogCollectMME, LogCollectSPGW, LogCollectPing, LogCollectIperf')

def GitSrvHelp(repository,branch,commit,mergeallow,targetbranch):
	print('  --ranRepository=[OAI RAN Repository URL]                                      -- ' + repository)
	print('  --ranBranch=[OAI RAN Repository Branch]                                       -- ' + branch)
	print('  --ranCommitID=[OAI RAN Repository Commit SHA-1]                               -- ' + commit)
	print('  --ranAllowMerge=[Allow Merge Request (with target branch) (true or false)]    -- ' + mergeallow)
	print('  --ranTargetBranch=[Target Branch in case of a Merge Request]                  -- ' + targetbranch)

def eNBSrvHelp(ipaddr, username, password, sourcepath):
	print('  --eNBIPAddress=[eNB\'s IP Address]                       -- ' + ipaddr)
	print('  --eNBUserName=[eNB\'s Login User Name]                   -- ' + username)
	print('  --eNBPassword=[eNB\'s Login Password]                    -- ' + password)
	print('  --eNBSourceCodePath=[eNB\'s Source Code Path]            -- ' + sourcepath)

def OAIUESrvHelp(ipaddr, username, password, sourcepath):
	print('  --UEIPAddress=[UE\'s IP Address]                         -- ' + ipaddr)
	print('  --UEUserName=[UE\'s Login User Name]                     -- ' + username)
	print('  --UEPassword=[UE\'s Login Password]                      -- ' + password)
	print('  --UESourceCodePath=[UE\'s Source Code Path]              -- ' + sourcepath)
		
def EPCSrvHelp(ipaddr, username, password, sourcepath, epctype):
	print('  --EPCIPAddress=[EPC\'s IP Address]                       -- ' + ipaddr)
	print('  --EPCUserName=[EPC\'s Login User Name]                   -- ' + username)
	print('  --EPCPassword=[EPC\'s Login Password]                    -- ' + password)
	print('  --EPCSourceCodePath=[EPC\'s Source Code Path]            -- ' + sourcepath)
	print('  --EPCType=[EPC\'s Type: OAI or ltebox or OC-OAI-CN5G]    -- ' + epctype)

def XmlHelp(filename):
	print('  --XMLTestFile=[XML Test File to be run]                  -- ' + filename)
	print('	Note: multiple xml files can be specified (--XMLFile=File1 ... --XMLTestFile=FileN) when HTML headers are created ("InitiateHtml" mode)')

