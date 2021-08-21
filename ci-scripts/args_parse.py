
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
import yaml
import constants as CONST

#-----------------------------------------------------------
# Parsing Command Line Arguements
#-----------------------------------------------------------


def ArgsParse(argvs,CiTestObj,RAN,HTML,EPC,ldpc,CONTAINERS,HELP,SCA,PHYSIM):


    py_param_file_present = False
    py_params={}

    while len(argvs) > 1:
        myArgv = argvs.pop(1)	# 0th is this file's name

	    #--help
        if re.match('^\-\-help$', myArgv, re.IGNORECASE):
            HELP.GenericHelp(CONST.Version)
            sys.exit(0)

	    #--apply=<filename> as parameters file, to replace inline parameters
        elif re.match('^\-\-Apply=(.+)$', myArgv, re.IGNORECASE):
            matchReg = re.match('^\-\-Apply=(.+)$', myArgv, re.IGNORECASE)
            py_params_file = matchReg.group(1)
            with open(py_params_file,'r') as file:
          	# The FullLoader parameter handles the conversion from YAML
        	# scalar values to Python dictionary format
                py_params = yaml.load(file,Loader=yaml.FullLoader)
                py_param_file_present = True #to be removed once validated
	    		#AssignParams(py_params) #to be uncommented once validated

	    #consider inline parameters
        elif re.match('^\-\-mode=(.+)$', myArgv, re.IGNORECASE):
            matchReg = re.match('^\-\-mode=(.+)$', myArgv, re.IGNORECASE)
            mode = matchReg.group(1)
        elif re.match('^\-\-eNBRepository=(.+)$|^\-\-ranRepository(.+)$', myArgv, re.IGNORECASE):
            if re.match('^\-\-eNBRepository=(.+)$', myArgv, re.IGNORECASE):
                matchReg = re.match('^\-\-eNBRepository=(.+)$', myArgv, re.IGNORECASE)
            else:
                matchReg = re.match('^\-\-ranRepository=(.+)$', myArgv, re.IGNORECASE)
            CiTestObj.ranRepository = matchReg.group(1)
            RAN.ranRepository=matchReg.group(1)
            HTML.ranRepository=matchReg.group(1)
            ldpc.ranRepository=matchReg.group(1)
            CONTAINERS.ranRepository=matchReg.group(1)
            SCA.ranRepository=matchReg.group(1)
            PHYSIM.ranRepository=matchReg.group(1)
        elif re.match('^\-\-eNB_AllowMerge=(.+)$|^\-\-ranAllowMerge=(.+)$', myArgv, re.IGNORECASE):
            if re.match('^\-\-eNB_AllowMerge=(.+)$', myArgv, re.IGNORECASE):
                matchReg = re.match('^\-\-eNB_AllowMerge=(.+)$', myArgv, re.IGNORECASE)
            else:
                matchReg = re.match('^\-\-ranAllowMerge=(.+)$', myArgv, re.IGNORECASE)
            doMerge = matchReg.group(1)
            ldpc.ranAllowMerge=matchReg.group(1)
            if ((doMerge == 'true') or (doMerge == 'True')):
                CiTestObj.ranAllowMerge = True
                RAN.ranAllowMerge=True
                HTML.ranAllowMerge=True
                CONTAINERS.ranAllowMerge=True
                SCA.ranAllowMerge=True
                PHYSIM.ranAllowMerge=True
        elif re.match('^\-\-eNBBranch=(.+)$|^\-\-ranBranch=(.+)$', myArgv, re.IGNORECASE):
            if re.match('^\-\-eNBBranch=(.+)$', myArgv, re.IGNORECASE):
                matchReg = re.match('^\-\-eNBBranch=(.+)$', myArgv, re.IGNORECASE)
            else:
                matchReg = re.match('^\-\-ranBranch=(.+)$', myArgv, re.IGNORECASE)
            CiTestObj.ranBranch = matchReg.group(1)
            RAN.ranBranch=matchReg.group(1)
            HTML.ranBranch=matchReg.group(1)
            ldpc.ranBranch=matchReg.group(1)
            CONTAINERS.ranBranch=matchReg.group(1)
            SCA.ranBranch=matchReg.group(1)
            PHYSIM.ranBranch=matchReg.group(1)
        elif re.match('^\-\-eNBCommitID=(.*)$|^\-\-ranCommitID=(.*)$', myArgv, re.IGNORECASE):
            if re.match('^\-\-eNBCommitID=(.*)$', myArgv, re.IGNORECASE):
                matchReg = re.match('^\-\-eNBCommitID=(.*)$', myArgv, re.IGNORECASE)
            else:
                matchReg = re.match('^\-\-ranCommitID=(.*)$', myArgv, re.IGNORECASE)
            CiTestObj.ranCommitID = matchReg.group(1)
            RAN.ranCommitID=matchReg.group(1)
            HTML.ranCommitID=matchReg.group(1)
            ldpc.ranCommitID=matchReg.group(1)
            CONTAINERS.ranCommitID=matchReg.group(1)
            SCA.ranCommitID=matchReg.group(1)
            PHYSIM.ranCommitID=matchReg.group(1)
        elif re.match('^\-\-eNBTargetBranch=(.*)$|^\-\-ranTargetBranch=(.*)$', myArgv, re.IGNORECASE):
            if re.match('^\-\-eNBTargetBranch=(.*)$', myArgv, re.IGNORECASE):
                matchReg = re.match('^\-\-eNBTargetBranch=(.*)$', myArgv, re.IGNORECASE)
            else:
                matchReg = re.match('^\-\-ranTargetBranch=(.*)$', myArgv, re.IGNORECASE)
            CiTestObj.ranTargetBranch = matchReg.group(1)
            RAN.ranTargetBranch=matchReg.group(1)
            HTML.ranTargetBranch=matchReg.group(1)
            ldpc.ranTargetBranch=matchReg.group(1)
            CONTAINERS.ranTargetBranch=matchReg.group(1)
            SCA.ranTargetBranch=matchReg.group(1)
            PHYSIM.ranTargetBranch=matchReg.group(1)
        elif re.match('^\-\-eNBIPAddress=(.+)$|^\-\-eNB[1-2]IPAddress=(.+)$', myArgv, re.IGNORECASE):
            if re.match('^\-\-eNBIPAddress=(.+)$', myArgv, re.IGNORECASE):
                matchReg = re.match('^\-\-eNBIPAddress=(.+)$', myArgv, re.IGNORECASE)
                RAN.eNBIPAddress=matchReg.group(1)
                ldpc.eNBIpAddr=matchReg.group(1)
                CONTAINERS.eNBIPAddress=matchReg.group(1)
                SCA.eNBIPAddress=matchReg.group(1)
                PHYSIM.eNBIPAddress=matchReg.group(1)
            elif re.match('^\-\-eNB1IPAddress=(.+)$', myArgv, re.IGNORECASE):
                matchReg = re.match('^\-\-eNB1IPAddress=(.+)$', myArgv, re.IGNORECASE)
                RAN.eNB1IPAddress=matchReg.group(1)
                CONTAINERS.eNB1IPAddress=matchReg.group(1)
            elif re.match('^\-\-eNB2IPAddress=(.+)$', myArgv, re.IGNORECASE):
                matchReg = re.match('^\-\-eNB2IPAddress=(.+)$', myArgv, re.IGNORECASE)
                RAN.eNB2IPAddress=matchReg.group(1)
                CONTAINERS.eNB2IPAddress=matchReg.group(1)
        elif re.match('^\-\-eNBUserName=(.+)$|^\-\-eNB[1-2]UserName=(.+)$', myArgv, re.IGNORECASE):
            if re.match('^\-\-eNBUserName=(.+)$', myArgv, re.IGNORECASE):
                matchReg = re.match('^\-\-eNBUserName=(.+)$', myArgv, re.IGNORECASE)
                RAN.eNBUserName=matchReg.group(1)
                ldpc.eNBUserName=matchReg.group(1)
                CONTAINERS.eNBUserName=matchReg.group(1)
                SCA.eNBUserName=matchReg.group(1)
                PHYSIM.eNBUserName=matchReg.group(1)
            elif re.match('^\-\-eNB1UserName=(.+)$', myArgv, re.IGNORECASE):
                matchReg = re.match('^\-\-eNB1UserName=(.+)$', myArgv, re.IGNORECASE)
                RAN.eNB1UserName=matchReg.group(1)
                CONTAINERS.eNB1UserName=matchReg.group(1)
            elif re.match('^\-\-eNB2UserName=(.+)$', myArgv, re.IGNORECASE):
                matchReg = re.match('^\-\-eNB2UserName=(.+)$', myArgv, re.IGNORECASE)
                RAN.eNB2UserName=matchReg.group(1)
                CONTAINERS.eNB2UserName=matchReg.group(1)
        elif re.match('^\-\-eNBPassword=(.+)$|^\-\-eNB[1-2]Password=(.+)$', myArgv, re.IGNORECASE):
            if re.match('^\-\-eNBPassword=(.+)$', myArgv, re.IGNORECASE):
                matchReg = re.match('^\-\-eNBPassword=(.+)$', myArgv, re.IGNORECASE)
                RAN.eNBPassword=matchReg.group(1)
                ldpc.eNBPassWord=matchReg.group(1)
                CONTAINERS.eNBPassword=matchReg.group(1)
                SCA.eNBPassword=matchReg.group(1)
                PHYSIM.eNBPassword=matchReg.group(1)
            elif re.match('^\-\-eNB1Password=(.+)$', myArgv, re.IGNORECASE):
                matchReg = re.match('^\-\-eNB1Password=(.+)$', myArgv, re.IGNORECASE)
                RAN.eNB1Password=matchReg.group(1)
                CONTAINERS.eNB1Password=matchReg.group(1)
            elif re.match('^\-\-eNB2Password=(.+)$', myArgv, re.IGNORECASE):
                matchReg = re.match('^\-\-eNB2Password=(.+)$', myArgv, re.IGNORECASE)
                RAN.eNB2Password=matchReg.group(1)
                CONTAINERS.eNB2Password=matchReg.group(1)
        elif re.match('^\-\-eNBSourceCodePath=(.+)$|^\-\-eNB[1-2]SourceCodePath=(.+)$', myArgv, re.IGNORECASE):
            if re.match('^\-\-eNBSourceCodePath=(.+)$', myArgv, re.IGNORECASE):
                matchReg = re.match('^\-\-eNBSourceCodePath=(.+)$', myArgv, re.IGNORECASE)
                RAN.eNBSourceCodePath=matchReg.group(1)
                ldpc.eNBSourceCodePath=matchReg.group(1)
                CONTAINERS.eNBSourceCodePath=matchReg.group(1)
                SCA.eNBSourceCodePath=matchReg.group(1)
                PHYSIM.eNBSourceCodePath=matchReg.group(1)
            elif re.match('^\-\-eNB1SourceCodePath=(.+)$', myArgv, re.IGNORECASE):
                matchReg = re.match('^\-\-eNB1SourceCodePath=(.+)$', myArgv, re.IGNORECASE)
                RAN.eNB1SourceCodePath=matchReg.group(1)
                CONTAINERS.eNB1SourceCodePath=matchReg.group(1)
            elif re.match('^\-\-eNB2SourceCodePath=(.+)$', myArgv, re.IGNORECASE):
                matchReg = re.match('^\-\-eNB2SourceCodePath=(.+)$', myArgv, re.IGNORECASE)
                RAN.eNB2SourceCodePath=matchReg.group(1)
                CONTAINERS.eNB2SourceCodePath=matchReg.group(1)
        elif re.match('^\-\-EPCIPAddress=(.+)$', myArgv, re.IGNORECASE):
            matchReg = re.match('^\-\-EPCIPAddress=(.+)$', myArgv, re.IGNORECASE)
            EPC.IPAddress=matchReg.group(1)
        elif re.match('^\-\-EPCUserName=(.+)$', myArgv, re.IGNORECASE):
            matchReg = re.match('^\-\-EPCUserName=(.+)$', myArgv, re.IGNORECASE)
            EPC.UserName=matchReg.group(1)
        elif re.match('^\-\-EPCPassword=(.+)$', myArgv, re.IGNORECASE):
            matchReg = re.match('^\-\-EPCPassword=(.+)$', myArgv, re.IGNORECASE)
            EPC.Password=matchReg.group(1)
        elif re.match('^\-\-EPCSourceCodePath=(.+)$', myArgv, re.IGNORECASE):
            matchReg = re.match('^\-\-EPCSourceCodePath=(.+)$', myArgv, re.IGNORECASE)
            EPC.SourceCodePath=matchReg.group(1)
        elif re.match('^\-\-EPCType=(.+)$', myArgv, re.IGNORECASE):
            matchReg = re.match('^\-\-EPCType=(.+)$', myArgv, re.IGNORECASE)
            if re.match('OAI', matchReg.group(1), re.IGNORECASE) or re.match('ltebox', matchReg.group(1), re.IGNORECASE) or re.match('OAI-Rel14-CUPS', matchReg.group(1), re.IGNORECASE) or re.match('OAI-Rel14-Docker', matchReg.group(1), re.IGNORECASE):
                EPC.Type=matchReg.group(1)
            else:
                sys.exit('Invalid EPC Type: ' + matchReg.group(1) + ' -- (should be OAI or ltebox or OAI-Rel14-CUPS or OAI-Rel14-Docker)')
        elif re.match('^\-\-EPCContainerPrefix=(.+)$', myArgv, re.IGNORECASE):
            matchReg = re.match('^\-\-EPCContainerPrefix=(.+)$', myArgv, re.IGNORECASE)
            EPC.ContainerPrefix=matchReg.group(1)
        elif re.match('^\-\-ADBIPAddress=(.+)$', myArgv, re.IGNORECASE):
            matchReg = re.match('^\-\-ADBIPAddress=(.+)$', myArgv, re.IGNORECASE)
            CiTestObj.ADBIPAddress = matchReg.group(1)
        elif re.match('^\-\-ADBUserName=(.+)$', myArgv, re.IGNORECASE):
            matchReg = re.match('^\-\-ADBUserName=(.+)$', myArgv, re.IGNORECASE)
            CiTestObj.ADBUserName = matchReg.group(1)
        elif re.match('^\-\-ADBType=(.+)$', myArgv, re.IGNORECASE):
            matchReg = re.match('^\-\-ADBType=(.+)$', myArgv, re.IGNORECASE)
            if re.match('centralized', matchReg.group(1), re.IGNORECASE) or re.match('distributed', matchReg.group(1), re.IGNORECASE):
                if re.match('distributed', matchReg.group(1), re.IGNORECASE):
                    CiTestObj.ADBCentralized = False
                else:
                    CiTestObj.ADBCentralized = True
            else:
                sys.exit('Invalid ADB Type: ' + matchReg.group(1) + ' -- (should be centralized or distributed)')
        elif re.match('^\-\-ADBPassword=(.+)$', myArgv, re.IGNORECASE):
            matchReg = re.match('^\-\-ADBPassword=(.+)$', myArgv, re.IGNORECASE)
            CiTestObj.ADBPassword = matchReg.group(1)
        elif re.match('^\-\-XMLTestFile=(.+)$', myArgv, re.IGNORECASE):
            matchReg = re.match('^\-\-XMLTestFile=(.+)$', myArgv, re.IGNORECASE)
            CiTestObj.testXMLfiles.append(matchReg.group(1))
            HTML.testXMLfiles.append(matchReg.group(1))
            HTML.nbTestXMLfiles=HTML.nbTestXMLfiles+1
        elif re.match('^\-\-UEIPAddress=(.+)$', myArgv, re.IGNORECASE):
            matchReg = re.match('^\-\-UEIPAddress=(.+)$', myArgv, re.IGNORECASE)
            CiTestObj.UEIPAddress = matchReg.group(1)
        elif re.match('^\-\-UEUserName=(.+)$', myArgv, re.IGNORECASE):
            matchReg = re.match('^\-\-UEUserName=(.+)$', myArgv, re.IGNORECASE)
            CiTestObj.UEUserName = matchReg.group(1)
        elif re.match('^\-\-UEPassword=(.+)$', myArgv, re.IGNORECASE):
            matchReg = re.match('^\-\-UEPassword=(.+)$', myArgv, re.IGNORECASE)
            CiTestObj.UEPassword = matchReg.group(1)
        elif re.match('^\-\-UESourceCodePath=(.+)$', myArgv, re.IGNORECASE):
            matchReg = re.match('^\-\-UESourceCodePath=(.+)$', myArgv, re.IGNORECASE)
            CiTestObj.UESourceCodePath = matchReg.group(1)
        elif re.match('^\-\-finalStatus=(.+)$', myArgv, re.IGNORECASE):
            matchReg = re.match('^\-\-finalStatus=(.+)$', myArgv, re.IGNORECASE)
            finalStatus = matchReg.group(1)
            if ((finalStatus == 'true') or (finalStatus == 'True')):
                CiTestObj.finalStatus = True
        elif re.match('^\-\-OCUserName=(.+)$', myArgv, re.IGNORECASE):
            matchReg = re.match('^\-\-OCUserName=(.+)$', myArgv, re.IGNORECASE)
            PHYSIM.OCUserName = matchReg.group(1)
        elif re.match('^\-\-OCPassword=(.+)$', myArgv, re.IGNORECASE):
            matchReg = re.match('^\-\-OCPassword=(.+)$', myArgv, re.IGNORECASE)
            PHYSIM.OCPassword = matchReg.group(1)
        elif re.match('^\-\-OCProjectName=(.+)$', myArgv, re.IGNORECASE):
            matchReg = re.match('^\-\-OCProjectName=(.+)$', myArgv, re.IGNORECASE)
            PHYSIM.OCProjectName = matchReg.group(1)
        elif re.match('^\-\-BuildId=(.+)$', myArgv, re.IGNORECASE):
            matchReg = re.match('^\-\-BuildId=(.+)$', myArgv, re.IGNORECASE)
            RAN.BuildId = matchReg.group(1)
        else:
            HELP.GenericHelp(CONST.Version)
            sys.exit('Invalid Parameter: ' + myArgv)

    return py_param_file_present, py_params, mode
