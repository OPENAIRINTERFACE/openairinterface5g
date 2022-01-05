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
#
#   Required Python Version
#     Python 3.x
#
#---------------------------------------------------------------------

#to use isfile
import os
import sys
import logging
#to create a SSH object locally in the methods
import sshconnection
#time.sleep
import time
#to load ue infrastructure dictionary
import yaml

class InfraUE:
	def __init__(self):
		self.ci_ue_infra ={}


#-----------------$
#PUBLIC Methods$
#-----------------$

	#This method reads the yaml file describing the multi-UE infrastructure
	#and stores the infra permanently in the related class attribute self.ci_ue_infra
	def Get_UE_Infra(self,ue_infra_filename):
		f_yaml=ue_infra_filename
		with open(f_yaml,'r') as file:
			logging.debug('Loading UE infrastructure from file '+f_yaml)
			#load it permanently in the class attribute
			self.ci_ue_infra = yaml.load(file,Loader=yaml.FullLoader)



