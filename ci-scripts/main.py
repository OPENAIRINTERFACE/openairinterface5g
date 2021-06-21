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
# Import Components
#-----------------------------------------------------------

import helpreadme as HELP
import constants as CONST


import cls_oaicitest            #main class for OAI CI test framework
import cls_physim               #class PhySim for physical simulators build and test
import cls_cots_ue              #class CotsUe for Airplane mode control
import cls_containerize         #class Containerize for all container-based operations on RAN/UE objects
import cls_static_code_analysis #class for static code analysis
import cls_ci_ueinfra			#class defining the multi Ue infrastrucure
import cls_physim1          #class PhySim for physical simulators deploy and run

import sshconnection 
import epc
import ran
import html


#-----------------------------------------------------------
# Import Libs
#-----------------------------------------------------------
import sys		# arg
import re		# reg
import pexpect	# pexpect
import time		# sleep
import os
import subprocess
import xml.etree.ElementTree as ET
import logging
import datetime
import signal
from multiprocessing import Process, Lock, SimpleQueue
logging.basicConfig(
	level=logging.DEBUG,
	format="[%(asctime)s] %(name)s:%(levelname)s: %(message)s"
)




#-----------------------------------------------------------
# General Functions
#-----------------------------------------------------------



def CheckClassValidity(xml_class_list,action,id):
	if action not in xml_class_list:
		logging.debug('ERROR: test-case ' + id + ' has unlisted class ' + action + ' ##CHECK xml_class_list.yml')
		resp=False
	else:
		resp=True
	return resp


#assigning parameters to object instance attributes (even if the attributes do not exist !!)
def AssignParams(params_dict):

	for key,value in params_dict.items():
		setattr(CiTestObj, key, value)
		setattr(RAN, key, value)
		setattr(HTML, key, value)
		setattr(ldpc, key, value)



def GetParametersFromXML(action):
	if action == 'Build_eNB' or action == 'Build_Image':
		RAN.Build_eNB_args=test.findtext('Build_eNB_args')
		CONTAINERS.imageKind=test.findtext('kind')
		forced_workspace_cleanup = test.findtext('forced_workspace_cleanup')
		if (forced_workspace_cleanup is None):
			RAN.Build_eNB_forced_workspace_cleanup=False
			CONTAINERS.forcedWorkspaceCleanup=False
		else:
			if re.match('true', forced_workspace_cleanup, re.IGNORECASE):
				RAN.Build_eNB_forced_workspace_cleanup=True
				CONTAINERS.forcedWorkspaceCleanup=True
			else:
				RAN.Build_eNB_forced_workspace_cleanup=True
				CONTAINERS.forcedWorkspaceCleanup=False
		eNB_instance=test.findtext('eNB_instance')
		if (eNB_instance is None):
			RAN.eNB_instance=0
			CONTAINERS.eNB_instance=0
		else:
			RAN.eNB_instance=int(eNB_instance)
			CONTAINERS.eNB_instance=int(eNB_instance)
		eNB_serverId=test.findtext('eNB_serverId')
		if (eNB_serverId is None):
			RAN.eNB_serverId[RAN.eNB_instance]='0'
			CONTAINERS.eNB_serverId[RAN.eNB_instance]='0'
		else:
			RAN.eNB_serverId[RAN.eNB_instance]=eNB_serverId
			CONTAINERS.eNB_serverId[CONTAINERS.eNB_instance]=eNB_serverId
		xmlBgBuildField = test.findtext('backgroundBuild')
		if (xmlBgBuildField is None):
			RAN.backgroundBuild=False
		else:
			if re.match('true', xmlBgBuildField, re.IGNORECASE):
				RAN.backgroundBuild=True
			else:
				RAN.backgroundBuild=False

	elif action == 'WaitEndBuild_eNB':
		RAN.Build_eNB_args=test.findtext('Build_eNB_args')
		eNB_instance=test.findtext('eNB_instance')
		if (eNB_instance is None):
			RAN.eNB_instance=0
		else:
			RAN.eNB_instance=int(eNB_instance)
		eNB_serverId=test.findtext('eNB_serverId')
		if (eNB_serverId is None):
			RAN.eNB_serverId[RAN.eNB_instance]='0'
		else:
			RAN.eNB_serverId[RAN.eNB_instance]=eNB_serverId

	elif action == 'Initialize_eNB':
		RAN.eNB_Trace=test.findtext('eNB_Trace')
		RAN.Initialize_eNB_args=test.findtext('Initialize_eNB_args')
		eNB_instance=test.findtext('eNB_instance')
		if (eNB_instance is None):
			RAN.eNB_instance=0
		else:
			RAN.eNB_instance=int(eNB_instance)
		eNB_serverId=test.findtext('eNB_serverId')
		if (eNB_serverId is None):
			RAN.eNB_serverId[RAN.eNB_instance]='0'
		else:
			RAN.eNB_serverId[RAN.eNB_instance]=eNB_serverId
			
		#local variable air_interface
		air_interface = test.findtext('air_interface')		
		if (air_interface is None) or (air_interface.lower() not in ['nr','lte','ocp']):
			RAN.air_interface[RAN.eNB_instance] = 'lte-softmodem'
		elif (air_interface.lower() in ['nr','lte']):
			RAN.air_interface[RAN.eNB_instance] = air_interface.lower() +'-softmodem'
		else :
			RAN.air_interface[RAN.eNB_instance] = 'ocp-enb'

	elif action == 'Terminate_eNB':
		eNB_instance=test.findtext('eNB_instance')
		if (eNB_instance is None):
			RAN.eNB_instance=0
		else:
			RAN.eNB_instance=int(eNB_instance)
		eNB_serverId=test.findtext('eNB_serverId')
		if (eNB_serverId is None):
			RAN.eNB_serverId[RAN.eNB_instance]='0'
		else:
			RAN.eNB_serverId[RAN.eNB_instance]=eNB_serverId

		#local variable air_interface
		air_interface = test.findtext('air_interface')		
		if (air_interface is None) or (air_interface.lower() not in ['nr','lte','ocp']):
			RAN.air_interface[RAN.eNB_instance] = 'lte-softmodem'
		elif (air_interface.lower() in ['nr','lte']):
			RAN.air_interface[RAN.eNB_instance] = air_interface.lower() +'-softmodem'
		else :
			RAN.air_interface[RAN.eNB_instance] = 'ocp-enb'

	elif action == 'Initialize_UE':
		ue_id = test.findtext('id')
		CiTestObj.ue_trace=test.findtext('UE_Trace')#temporary variable, to be passed to Module_UE in Initialize_UE call
		if (ue_id is None):
			CiTestObj.ue_id = ""
		else:
			CiTestObj.ue_id = ue_id

	elif action == 'Detach_UE':
		ue_id = test.findtext('id')
		if (ue_id is None):
			CiTestObj.ue_id = ""
		else:
			CiTestObj.ue_id = ue_id

	elif action == 'Attach_UE':
		ue_id = test.findtext('id')
		if (ue_id is None):
			CiTestObj.ue_id = ""
		else:
			CiTestObj.ue_id = ue_id
		nbMaxUEtoAttach = test.findtext('nbMaxUEtoAttach')
		if (nbMaxUEtoAttach is None):
			CiTestObj.nbMaxUEtoAttach = -1
		else:
			CiTestObj.nbMaxUEtoAttach = int(nbMaxUEtoAttach)

	elif action == 'Terminate_UE':
		ue_id = test.findtext('id')
		if (ue_id is None):
			CiTestObj.ue_id = ""
		else:
			CiTestObj.ue_id = ue_id


	elif action == 'CheckStatusUE':
		expectedNBUE = test.findtext('expectedNbOfConnectedUEs')
		if (expectedNBUE is None):
			CiTestObj.expectedNbOfConnectedUEs = -1
		else:
			CiTestObj.expectedNbOfConnectedUEs = int(expectedNBUE)

	elif action == 'Build_OAI_UE':
		CiTestObj.Build_OAI_UE_args = test.findtext('Build_OAI_UE_args')
		CiTestObj.clean_repository = test.findtext('clean_repository')
		if (CiTestObj.clean_repository == 'false'):
			CiTestObj.clean_repository = False
		else:
			CiTestObj.clean_repository = True

	elif action == 'Initialize_OAI_UE':
		CiTestObj.Initialize_OAI_UE_args = test.findtext('Initialize_OAI_UE_args')
		UE_instance = test.findtext('UE_instance')
		if (UE_instance is None):
			CiTestObj.UE_instance = 0
		else:
			CiTestObj.UE_instance = UE_instance
			
		#local variable air_interface
		air_interface = test.findtext('air_interface')		
		if (air_interface is None) or (air_interface.lower() not in ['nr','lte','ocp']):
			CiTestObj.air_interface = 'lte-uesoftmodem'
		elif (air_interface.lower() in ['nr','lte']):
			CiTestObj.air_interface = air_interface.lower() +'-uesoftmodem'
		else :
			#CiTestObj.air_interface = 'ocp-enb'
			logging.error('OCP UE -- NOT SUPPORTED')

	elif action == 'Terminate_OAI_UE':
		UE_instance=test.findtext('UE_instance')
		if (UE_instance is None):
			CiTestObj.UE_instance = '0'
		else:
			CiTestObj.UE_instance = int(UE_instance)
		
		#local variable air_interface
		air_interface = test.findtext('air_interface')		
		if (air_interface is None) or (air_interface.lower() not in ['nr','lte','ocp']):
			CiTestObj.air_interface = 'lte-uesoftmodem'
		elif (air_interface.lower() in ['nr','lte']):
			CiTestObj.air_interface = air_interface.lower() +'-uesoftmodem'
		else :
			#CiTestObj.air_interface = 'ocp-enb'
			logging.error('OCP UE -- NOT SUPPORTED')

	elif (action == 'Ping') or (action == 'Ping_CatM_module'):
		CiTestObj.ping_args = test.findtext('ping_args')
		CiTestObj.ping_packetloss_threshold = test.findtext('ping_packetloss_threshold')
		ue_id = test.findtext('id')
		if (ue_id is None):
			CiTestObj.ue_id = ""
		else:
			CiTestObj.ue_id = ue_id

	elif action == 'Iperf':
		CiTestObj.iperf_args = test.findtext('iperf_args')
		ue_id = test.findtext('id')
		if (ue_id is None):
			CiTestObj.ue_id = ""
		else:
			CiTestObj.ue_id = ue_id
		CiTestObj.iperf_direction = test.findtext('direction')#used for modules only	
		CiTestObj.iperf_packetloss_threshold = test.findtext('iperf_packetloss_threshold')
		CiTestObj.iperf_profile = test.findtext('iperf_profile')
		if (CiTestObj.iperf_profile is None):
			CiTestObj.iperf_profile = 'balanced'
		else:
			if CiTestObj.iperf_profile != 'balanced' and CiTestObj.iperf_profile != 'unbalanced' and CiTestObj.iperf_profile != 'single-ue':
				logging.debug('ERROR: test-case has wrong profile ' + CiTestObj.iperf_profile)
				CiTestObj.iperf_profile = 'balanced'
		CiTestObj.iperf_options = test.findtext('iperf_options')
		if (CiTestObj.iperf_options is None):
			CiTestObj.iperf_options = 'check'
		else:
			if CiTestObj.iperf_options != 'check' and CiTestObj.iperf_options != 'sink':
				logging.debug('ERROR: test-case has wrong option ' + CiTestObj.iperf_options)
				CiTestObj.iperf_options = 'check'

	elif action == 'IdleSleep':
		string_field = test.findtext('idle_sleep_time_in_sec')
		if (string_field is None):
			CiTestObj.idle_sleep_time = 5
		else:
			CiTestObj.idle_sleep_time = int(string_field)

	elif action == 'Perform_X2_Handover':
		string_field = test.findtext('x2_ho_options')
		if (string_field is None):
			CiTestObj.x2_ho_options = 'network'
		else:
			if string_field != 'network':
				logging.error('ERROR: test-case has wrong option ' + string_field)
				CiTestObj.x2_ho_options = 'network'
			else:
				CiTestObj.x2_ho_options = string_field

	elif action == 'Build_PhySim':
		ldpc.buildargs  = test.findtext('physim_build_args')
		forced_workspace_cleanup = test.findtext('forced_workspace_cleanup')
		if (forced_workspace_cleanup is None):
			ldpc.forced_workspace_cleanup=False
		else:
			if re.match('true', forced_workspace_cleanup, re.IGNORECASE):
				ldpc.forced_workspace_cleanup=True
			else:
				ldpc.forced_workspace_cleanup=False

	elif action == 'Initialize_MME':
		string_field = test.findtext('option')
		if (string_field is not None):
			EPC.mmeConfFile = string_field

	elif action == 'Deploy_EPC':
		string_field = test.findtext('parameters')
		if (string_field is not None):
			EPC.yamlPath = string_field

	elif action == 'Deploy_Object' or action == 'Undeploy_Object':
		eNB_instance=test.findtext('eNB_instance')
		if (eNB_instance is None):
			CONTAINERS.eNB_instance=0
		else:
			CONTAINERS.eNB_instance=int(eNB_instance)
		eNB_serverId=test.findtext('eNB_serverId')
		if (eNB_serverId is None):
			CONTAINERS.eNB_serverId[CONTAINERS.eNB_instance]='0'
		else:
			CONTAINERS.eNB_serverId[CONTAINERS.eNB_instance]=eNB_serverId
		string_field = test.findtext('yaml_path')
		if (string_field is not None):
			CONTAINERS.yamlPath[CONTAINERS.eNB_instance] = string_field


	else: # ie action == 'Run_PhySim':
		ldpc.runargs = test.findtext('physim_run_args')
		

#check if given test is in list
#it is in list if one of the strings in 'list' is at the beginning of 'test'
def test_in_list(test, list):
	for check in list:
		check=check.replace('+','')
		if (test.startswith(check)):
			return True
	return False

def receive_signal(signum, frame):
	sys.exit(1)






#-----------------------------------------------------------
# MAIN PART
#-----------------------------------------------------------

#loading xml action list from yaml
import yaml
xml_class_list_file='xml_class_list.yml'
if (os.path.isfile(xml_class_list_file)):
	yaml_file=xml_class_list_file
elif (os.path.isfile('ci-scripts/'+xml_class_list_file)):
	yaml_file='ci-scripts/'+xml_class_list_file
else:
	logging.error("XML action list yaml file cannot be found")
	sys.exit("XML action list yaml file cannot be found")

with open(yaml_file,'r') as f:
    # The FullLoader parameter handles the conversion-$
    #from YAML scalar values to Python dictionary format$
    xml_class_list = yaml.load(f,Loader=yaml.FullLoader)



#loading UE infrastructure from yaml
ue_infra_file='ci_ueinfra.yaml'
if (os.path.isfile(ue_infra_file)):
	yaml_file=ue_infra_file
elif (os.path.isfile('ci-scripts/'+ue_infra_file)):
	yaml_file='ci-scripts/'+ue_infra_file
else:
	logging.error("UE infrastructure yaml file cannot be found")
	sys.exit("UE infrastructure file cannot be found")
InfraUE=cls_ci_ueinfra.InfraUE() #initialize UE infrastructure class
InfraUE.Get_UE_Infra(yaml_file) #read the UE infra, filename is hardcoded and unique for the moment but should be passed as parameter from the test suite



mode = ''

CiTestObj = cls_oaicitest.OaiCiTest()
 
SSH = sshconnection.SSHConnection()
EPC = epc.EPCManagement()
RAN = ran.RANManagement()
HTML = html.HTMLManagement()
CONTAINERS = cls_containerize.Containerize()
SCA = cls_static_code_analysis.StaticCodeAnalysis()
PHYSIM = cls_physim1.PhySim()

ldpc=cls_physim.PhySim()    #create an instance for LDPC test using GPU or CPU build


#-----------------------------------------------------------
# Parsing Command Line Arguments
#-----------------------------------------------------------

import args_parse
py_param_file_present, py_params, mode = args_parse.ArgsParse(sys.argv,CiTestObj,RAN,HTML,EPC,ldpc,CONTAINERS,HELP,SCA,PHYSIM)



#-----------------------------------------------------------
# TEMPORARY params management (UNUSED)
#-----------------------------------------------------------
#temporary solution for testing:
if py_param_file_present == True:
	AssignParams(py_params)


#-----------------------------------------------------------
# COTS UE instanciation
#-----------------------------------------------------------
#COTS_UE instanciation and ADB server init
#ue id and ue mode are retrieved from xml
COTS_UE=cls_cots_ue.CotsUe(CiTestObj.ADBIPAddress, CiTestObj.ADBUserName,CiTestObj.ADBPassword)


#-----------------------------------------------------------
# mode amd XML class (action) analysis
#-----------------------------------------------------------
cwd = os.getcwd()

if re.match('^TerminateeNB$', mode, re.IGNORECASE):
	if RAN.eNBIPAddress == '' or RAN.eNBUserName == '' or RAN.eNBPassword == '':
		HELP.GenericHelp(CONST.Version)
		sys.exit('Insufficient Parameter')
	RAN.eNB_instance=0
	RAN.eNB_serverId[0]='0'
	RAN.eNBSourceCodePath='/tmp/'
	RAN.TerminateeNB(HTML, EPC)
elif re.match('^TerminateUE$', mode, re.IGNORECASE):
	if (CiTestObj.ADBIPAddress == '' or CiTestObj.ADBUserName == '' or CiTestObj.ADBPassword == ''):
		HELP.GenericHelp(CONST.Version)
		sys.exit('Insufficient Parameter')
	signal.signal(signal.SIGUSR1, receive_signal)
	CiTestObj.TerminateUE(HTML,COTS_UE)
elif re.match('^TerminateOAIUE$', mode, re.IGNORECASE):
	if CiTestObj.UEIPAddress == '' or CiTestObj.UEUserName == '' or CiTestObj.UEPassword == '':
		HELP.GenericHelp(CONST.Version)
		sys.exit('Insufficient Parameter')
	signal.signal(signal.SIGUSR1, receive_signal)
	CiTestObj.TerminateOAIUE(HTML,RAN,COTS_UE,EPC,InfraUE)
elif re.match('^TerminateHSS$', mode, re.IGNORECASE):
	if EPC.IPAddress == '' or EPC.UserName == '' or EPC.Password == '' or EPC.Type == '' or EPC.SourceCodePath == '':
		HELP.GenericHelp(CONST.Version)
		sys.exit('Insufficient Parameter')
	EPC.TerminateHSS(HTML)
elif re.match('^TerminateMME$', mode, re.IGNORECASE):
	if EPC.IPAddress == '' or EPC.UserName == '' or EPC.Password == '' or EPC.Type == '' or EPC.SourceCodePath == '':
		HELP.GenericHelp(CONST.Version)
		sys.exit('Insufficient Parameter')
	EPC.TerminateMME(HTML)
elif re.match('^TerminateSPGW$', mode, re.IGNORECASE):
	if EPC.IPAddress == '' or EPC.UserName == '' or EPC.Password == '' or EPC.Type == '' or EPC.SourceCodePath== '':
		HELP.GenericHelp(CONST.Version)
		sys.exit('Insufficient Parameter')
	EPC.TerminateSPGW(HTML)
elif re.match('^LogCollectBuild$', mode, re.IGNORECASE):
	if (RAN.eNBIPAddress == '' or RAN.eNBUserName == '' or RAN.eNBPassword == '' or RAN.eNBSourceCodePath == '') and (CiTestObj.UEIPAddress == '' or CiTestObj.UEUserName == '' or CiTestObj.UEPassword == '' or CiTestObj.UESourceCodePath == ''):
		HELP.GenericHelp(CONST.Version)
		sys.exit('Insufficient Parameter')
	CiTestObj.LogCollectBuild(RAN)
elif re.match('^LogCollecteNB$', mode, re.IGNORECASE):
	if RAN.eNBIPAddress == '' or RAN.eNBUserName == '' or RAN.eNBPassword == '' or RAN.eNBSourceCodePath == '':
		HELP.GenericHelp(CONST.Version)
		sys.exit('Insufficient Parameter')
	RAN.LogCollecteNB()
elif re.match('^LogCollectHSS$', mode, re.IGNORECASE):
	if EPC.IPAddress == '' or EPC.UserName == '' or EPC.Password == '' or EPC.Type == '' or EPC.SourceCodePath == '':
		HELP.GenericHelp(CONST.Version)
		sys.exit('Insufficient Parameter')
	EPC.LogCollectHSS()
elif re.match('^LogCollectMME$', mode, re.IGNORECASE):
	if EPC.IPAddress == '' or EPC.UserName == '' or EPC.Password == '' or EPC.Type == '' or EPC.SourceCodePath == '':
		HELP.GenericHelp(CONST.Version)
		sys.exit('Insufficient Parameter')
	EPC.LogCollectMME()
elif re.match('^LogCollectSPGW$', mode, re.IGNORECASE):
	if EPC.IPAddress == '' or EPC.UserName == '' or EPC.Password == '' or EPC.Type == '' or EPC.SourceCodePath == '':
		HELP.GenericHelp(CONST.Version)
		sys.exit('Insufficient Parameter')
	EPC.LogCollectSPGW()
elif re.match('^LogCollectPing$', mode, re.IGNORECASE):
	if EPC.IPAddress == '' or EPC.UserName == '' or EPC.Password == '' or EPC.SourceCodePath == '':
		HELP.GenericHelp(CONST.Version)
		sys.exit('Insufficient Parameter')
	CiTestObj.LogCollectPing(EPC)
elif re.match('^LogCollectIperf$', mode, re.IGNORECASE):
	if EPC.IPAddress == '' or EPC.UserName == '' or EPC.Password == '' or EPC.SourceCodePath == '':
		HELP.GenericHelp(CONST.Version)
		sys.exit('Insufficient Parameter')
	CiTestObj.LogCollectIperf(EPC)
elif re.match('^LogCollectOAIUE$', mode, re.IGNORECASE):
	if CiTestObj.UEIPAddress == '' or CiTestObj.UEUserName == '' or CiTestObj.UEPassword == '' or CiTestObj.UESourceCodePath == '':
		HELP.GenericHelp(CONST.Version)
		sys.exit('Insufficient Parameter')
	CiTestObj.LogCollectOAIUE()
elif re.match('^InitiateHtml$', mode, re.IGNORECASE):
	if (CiTestObj.ADBIPAddress == '' or CiTestObj.ADBUserName == '' or CiTestObj.ADBPassword == ''):
		HELP.GenericHelp(CONST.Version)
		sys.exit('Insufficient Parameter')
	count = 0
	foundCount = 0
	while (count < HTML.nbTestXMLfiles):
		#xml_test_file = cwd + "/" + CiTestObj.testXMLfiles[count]
		xml_test_file = sys.path[0] + "/" + CiTestObj.testXMLfiles[count]
		if (os.path.isfile(xml_test_file)):
			try:
				xmlTree = ET.parse(xml_test_file)
			except:
				print("Error while parsing file: " + xml_test_file)
			xmlRoot = xmlTree.getroot()
			HTML.htmlTabRefs.append(xmlRoot.findtext('htmlTabRef',default='test-tab-' + str(count)))
			HTML.htmlTabNames.append(xmlRoot.findtext('htmlTabName',default='test-tab-' + str(count)))
			HTML.htmlTabIcons.append(xmlRoot.findtext('htmlTabIcon',default='info-sign'))
			foundCount += 1
		count += 1
	if foundCount != HTML.nbTestXMLfiles:
		HTML.nbTestXMLfiles=foundCount
	
	if (CiTestObj.ADBIPAddress != 'none'):
		terminate_ue_flag = False
		CiTestObj.GetAllUEDevices(terminate_ue_flag)
		CiTestObj.GetAllCatMDevices(terminate_ue_flag)
		HTML.SethtmlUEConnected(len(CiTestObj.UEDevices) + len(CiTestObj.CatMDevices))
		HTML.htmlNb_Smartphones=len(CiTestObj.UEDevices)
		HTML.htmlNb_CATM_Modules=len(CiTestObj.CatMDevices)
	HTML.CreateHtmlHeader(CiTestObj.ADBIPAddress)
elif re.match('^FinalizeHtml$', mode, re.IGNORECASE):
	logging.debug('\u001B[1m----------------------------------------\u001B[0m')
	logging.debug('\u001B[1m  Creating HTML footer \u001B[0m')
	logging.debug('\u001B[1m----------------------------------------\u001B[0m')

	CiTestObj.RetrieveSystemVersion('eNB',HTML,RAN)
	CiTestObj.RetrieveSystemVersion('UE',HTML,RAN)
	HTML.CreateHtmlFooter(CiTestObj.finalStatus)
elif re.match('^TesteNB$', mode, re.IGNORECASE) or re.match('^TestUE$', mode, re.IGNORECASE):
	logging.debug('\u001B[1m----------------------------------------\u001B[0m')
	logging.debug('\u001B[1m  Starting Scenario: ' + CiTestObj.testXMLfiles[0] + '\u001B[0m')
	logging.debug('\u001B[1m----------------------------------------\u001B[0m')
	if re.match('^TesteNB$', mode, re.IGNORECASE):
		if RAN.eNBIPAddress == '' or RAN.ranRepository == '' or RAN.ranBranch == '' or RAN.eNBUserName == '' or RAN.eNBPassword == '' or RAN.eNBSourceCodePath == '' or EPC.IPAddress == '' or EPC.UserName == '' or EPC.Password == '' or EPC.Type == '' or EPC.SourceCodePath == '' or CiTestObj.ADBIPAddress == '' or CiTestObj.ADBUserName == '' or CiTestObj.ADBPassword == '':
			HELP.GenericHelp(CONST.Version)
			if EPC.IPAddress == '' or EPC.UserName == '' or EPC.Password == '' or EPC.SourceCodePath == '' or EPC.Type == '':
				HELP.EPCSrvHelp(EPC.IPAddress, EPC.UserName, EPC.Password, EPC.SourceCodePath, EPC.Type)
			if RAN.ranRepository == '':
				HELP.GitSrvHelp(RAN.ranRepository, RAN.ranBranch, RAN.ranCommitID, RAN.ranAllowMerge, RAN.ranTargetBranch)
			if RAN.eNBIPAddress == ''  or RAN.eNBUserName == '' or RAN.eNBPassword == '' or RAN.eNBSourceCodePath == '':
				HELP.eNBSrvHelp(RAN.eNBIPAddress, RAN.eNBUserName, RAN.eNBPassword, RAN.eNBSourceCodePath)
			sys.exit('Insufficient Parameter')

		if (EPC.IPAddress!= '') and (EPC.IPAddress != 'none'):
			SSH.copyout(EPC.IPAddress, EPC.UserName, EPC.Password, cwd + "/tcp_iperf_stats.awk", "/tmp")
			SSH.copyout(EPC.IPAddress, EPC.UserName, EPC.Password, cwd + "/active_net_interfaces.awk", "/tmp")
	else:
		if CiTestObj.UEIPAddress == '' or CiTestObj.ranRepository == '' or CiTestObj.ranBranch == '' or CiTestObj.UEUserName == '' or CiTestObj.UEPassword == '' or CiTestObj.UESourceCodePath == '':
			HELP.GenericHelp(CONST.Version)
			sys.exit('UE: Insufficient Parameter')

	#read test_case_list.xml file
	# if no parameters for XML file, use default value
	if (HTML.nbTestXMLfiles != 1):
		xml_test_file = cwd + "/test_case_list.xml"
	else:
		xml_test_file = cwd + "/" + CiTestObj.testXMLfiles[0]

	xmlTree = ET.parse(xml_test_file)
	xmlRoot = xmlTree.getroot()

	exclusion_tests=xmlRoot.findtext('TestCaseExclusionList',default='')
	requested_tests=xmlRoot.findtext('TestCaseRequestedList',default='')
	if (HTML.nbTestXMLfiles == 1):
		HTML.htmlTabRefs.append(xmlRoot.findtext('htmlTabRef',default='test-tab-0'))
		HTML.htmlTabNames.append(xmlRoot.findtext('htmlTabName',default='Test-0'))
		repeatCount = xmlRoot.findtext('repeatCount',default='1')
		testStability = xmlRoot.findtext('TestUnstable',default='False')
		CiTestObj.repeatCounts.append(int(repeatCount))
		if testStability == 'True':
			CiTestObj.testUnstable = True
			HTML.testUnstable = True
			CiTestObj.testMinStableId = xmlRoot.findtext('TestMinId',default='999999')
			HTML.testMinStableId = CiTestObj.testMinStableId
			logging.debug('Test is tagged as Unstable -- starting from TestID ' + str(CiTestObj.testMinStableId))
	all_tests=xmlRoot.findall('testCase')

	exclusion_tests=exclusion_tests.split()
	requested_tests=requested_tests.split()

	#check that exclusion tests are well formatted
	#(6 digits or less than 6 digits followed by +)
	for test in exclusion_tests:
		if     (not re.match('^[0-9]{6}$', test) and
				not re.match('^[0-9]{1,5}\+$', test)):
			logging.debug('ERROR: exclusion test is invalidly formatted: ' + test)
			sys.exit(1)
		else:
			logging.debug(test)

	#check that requested tests are well formatted
	#(6 digits or less than 6 digits followed by +)
	#be verbose
	for test in requested_tests:
		if     (re.match('^[0-9]{6}$', test) or
				re.match('^[0-9]{1,5}\+$', test)):
			logging.debug('INFO: test group/case requested: ' + test)
		else:
			logging.debug('ERROR: requested test is invalidly formatted: ' + test)
			sys.exit(1)
	if (EPC.IPAddress != '') and (EPC.IPAddress != 'none'):
		CiTestObj.CheckFlexranCtrlInstallation(RAN,EPC,CONTAINERS)
		EPC.SetMmeIPAddress()

	#get the list of tests to be done
	todo_tests=[]
	for test in requested_tests:
		if    (test_in_list(test, exclusion_tests)):
			logging.debug('INFO: test will be skipped: ' + test)
		else:
			#logging.debug('INFO: test will be run: ' + test)
			todo_tests.append(test)

	signal.signal(signal.SIGUSR1, receive_signal)

	if (CiTestObj.ADBIPAddress != 'none'):
		terminate_ue_flag = False
		CiTestObj.GetAllUEDevices(terminate_ue_flag)
		CiTestObj.GetAllCatMDevices(terminate_ue_flag)
	else:
		CiTestObj.UEDevices.append('OAI-UE')
	HTML.SethtmlUEConnected(len(CiTestObj.UEDevices) + len(CiTestObj.CatMDevices))
	HTML.CreateHtmlTabHeader()

	CiTestObj.FailReportCnt = 0
	RAN.prematureExit=True
	HTML.startTime=int(round(time.time() * 1000))
	while CiTestObj.FailReportCnt < CiTestObj.repeatCounts[0] and RAN.prematureExit:
		RAN.prematureExit=False
		# At every iteratin of the retry loop, a separator will be added
		# pass CiTestObj.FailReportCnt as parameter of HTML.CreateHtmlRetrySeparator
		HTML.CreateHtmlRetrySeparator(CiTestObj.FailReportCnt)
		for test_case_id in todo_tests:
			if RAN.prematureExit:
				break
			for test in all_tests:
				if RAN.prematureExit:
					break
				id = test.get('id')
				if test_case_id != id:
					continue
				CiTestObj.testCase_id = id
				HTML.testCase_id=CiTestObj.testCase_id
				EPC.testCase_id=CiTestObj.testCase_id
				CiTestObj.desc = test.findtext('desc')
				HTML.desc=CiTestObj.desc
				action = test.findtext('class')
				if (CheckClassValidity(xml_class_list, action, id) == False):
					continue
				CiTestObj.ShowTestID()
				GetParametersFromXML(action)
				if action == 'Initialize_UE' or action == 'Attach_UE' or action == 'Detach_UE' or action == 'Ping' or action == 'Iperf' or action == 'Reboot_UE' or action == 'DataDisable_UE' or action == 'DataEnable_UE' or action == 'CheckStatusUE':
					if (CiTestObj.ADBIPAddress != 'none'):
						#in these cases, having no devices is critical, GetAllUEDevices function has to manage it as a critical error, reason why terminate_ue_flag is set to True
						terminate_ue_flag = True 
						# Now we stop properly the test-suite --> clean reporting
						status = CiTestObj.GetAllUEDevices(terminate_ue_flag)
						if not status:
							RAN.prematureExit = True
							break
				if action == 'Build_eNB':
					RAN.BuildeNB(HTML)
				elif action == 'WaitEndBuild_eNB':
					RAN.WaitBuildeNBisFinished(HTML)
				elif action == 'Initialize_eNB':
					check_eNB = False
					check_OAI_UE = False
					RAN.pStatus=CiTestObj.CheckProcessExist(check_eNB, check_OAI_UE,RAN,EPC)
					RAN.InitializeeNB(HTML, EPC)
				elif action == 'Terminate_eNB':
					RAN.TerminateeNB(HTML, EPC)
				elif action == 'Initialize_UE':
					CiTestObj.InitializeUE(HTML,RAN, EPC, COTS_UE, InfraUE, CiTestObj.ue_trace)
				elif action == 'Terminate_UE':
					CiTestObj.TerminateUE(HTML,COTS_UE, InfraUE, CiTestObj.ue_trace)
				elif action == 'Attach_UE':
					CiTestObj.AttachUE(HTML,RAN,EPC,COTS_UE,InfraUE)
				elif action == 'Detach_UE':
					CiTestObj.DetachUE(HTML,RAN,EPC,COTS_UE,InfraUE)
				elif action == 'DataDisable_UE':
					CiTestObj.DataDisableUE(HTML)
				elif action == 'DataEnable_UE':
					CiTestObj.DataEnableUE(HTML)
				elif action == 'CheckStatusUE':
					CiTestObj.CheckStatusUE(HTML,RAN,EPC,COTS_UE,InfraUE)
				elif action == 'Build_OAI_UE':
					CiTestObj.BuildOAIUE(HTML)
				elif action == 'Initialize_OAI_UE':
					CiTestObj.InitializeOAIUE(HTML,RAN,EPC,COTS_UE,InfraUE)
				elif action == 'Terminate_OAI_UE':
					CiTestObj.TerminateOAIUE(HTML,RAN,COTS_UE,EPC,InfraUE)
				elif action == 'Initialize_CatM_module':
					CiTestObj.InitializeCatM(HTML)
				elif action == 'Terminate_CatM_module':
					CiTestObj.TerminateCatM(HTML)
				elif action == 'Attach_CatM_module':
					CiTestObj.AttachCatM(HTML,RAN,COTS_UE,EPC,InfraUE)
				elif action == 'Detach_CatM_module':
					CiTestObj.TerminateCatM(HTML)
				elif action == 'Ping_CatM_module':
					CiTestObj.PingCatM(HTML,RAN,EPC,COTS_UE,EPC,InfraUE)
				elif action == 'Ping':
					CiTestObj.Ping(HTML,RAN,EPC,COTS_UE, InfraUE)
				elif action == 'Iperf':
					CiTestObj.Iperf(HTML,RAN,EPC,COTS_UE, InfraUE)
				elif action == 'Reboot_UE':
					CiTestObj.RebootUE(HTML,RAN,EPC)
				elif action == 'Initialize_HSS':
					EPC.InitializeHSS(HTML)
				elif action == 'Terminate_HSS':
					EPC.TerminateHSS(HTML)
				elif action == 'Initialize_MME':
					EPC.InitializeMME(HTML)
				elif action == 'Terminate_MME':
					EPC.TerminateMME(HTML)
				elif action == 'Initialize_SPGW':
					EPC.InitializeSPGW(HTML)
				elif action == 'Terminate_SPGW':
					EPC.TerminateSPGW(HTML)
				elif action == 'Deploy_EPC':
					EPC.DeployEpc(HTML)
				elif action == 'Undeploy_EPC':
					EPC.UndeployEpc(HTML)
				elif action == 'Initialize_FlexranCtrl':
					CiTestObj.InitializeFlexranCtrl(HTML,RAN,EPC)
				elif action == 'Terminate_FlexranCtrl':
					CiTestObj.TerminateFlexranCtrl(HTML,RAN,EPC)
				elif action == 'IdleSleep':
					CiTestObj.IdleSleep(HTML)
				elif action == 'Perform_X2_Handover':
					CiTestObj.Perform_X2_Handover(HTML,RAN,EPC)
				elif action == 'Build_PhySim':
					HTML=ldpc.Build_PhySim(HTML,CONST)
					if ldpc.exitStatus==1:sys.exit()
				elif action == 'Run_PhySim':
					HTML=ldpc.Run_PhySim(HTML,CONST,id)
				elif action == 'Build_Image':
					CONTAINERS.BuildImage(HTML)
				elif action == 'Deploy_Object':
					CONTAINERS.DeployObject(HTML, EPC)
				elif action == 'Undeploy_Object':
					CONTAINERS.UndeployObject(HTML, RAN)
				elif action == 'Cppcheck_Analysis':
					SCA.CppCheckAnalysis(HTML)
				elif action == 'Deploy_Run_PhySim':
					PHYSIM.Deploy_PhySim(HTML, RAN)
				else:
					sys.exit('Invalid class (action) from xml')
				if not RAN.prematureExit:
					if CiTestObj.testCase_id == CiTestObj.testMinStableId:
						logging.debug('Scenario has reached minimal stability point')
						CiTestObj.testStabilityPointReached = True
						HTML.testStabilityPointReached = True
		CiTestObj.FailReportCnt += 1
	if CiTestObj.FailReportCnt == CiTestObj.repeatCounts[0] and RAN.prematureExit:
		logging.debug('Scenario failed ' + str(CiTestObj.FailReportCnt) + ' time(s)')
		HTML.CreateHtmlTabFooter(False)
		if CiTestObj.testUnstable and (CiTestObj.testStabilityPointReached or CiTestObj.testMinStableId == '999999'):
			logging.debug('Scenario has reached minimal stability point -- Not a Failure')
		else:
			sys.exit('Failed Scenario')
	else:
		logging.info('Scenario passed after ' + str(CiTestObj.FailReportCnt) + ' time(s)')
		HTML.CreateHtmlTabFooter(True)
elif re.match('^LoadParams$', mode, re.IGNORECASE):
	pass
else:
	HELP.GenericHelp(CONST.Version)
	sys.exit('Invalid mode')
sys.exit(0)
