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
# Version
#-----------------------------------------------------------
Version = '0.2'

#-----------------------------------------------------------
# Constants
#-----------------------------------------------------------
ALL_PROCESSES_OK = 0
ENB_PROCESS_FAILED = -1
ENB_PROCESS_OK = +1
ENB_PROCESS_SEG_FAULT = -11
ENB_PROCESS_ASSERTION = -12
ENB_PROCESS_REALTIME_ISSUE = -13
ENB_PROCESS_NOLOGFILE_TO_ANALYZE = -14
ENB_PROCESS_SLAVE_RRU_NOT_SYNCED = -15
ENB_REAL_TIME_PROCESSING_ISSUE = -16
ENB_RETX_ISSUE = -17
ENB_SHUTDOWN_NO_BYE = -18
HSS_PROCESS_FAILED = -2
HSS_PROCESS_OK = +2
MME_PROCESS_FAILED = -3
MME_PROCESS_OK = +3
SPGW_PROCESS_FAILED = -4
SPGW_PROCESS_OK = +4
UE_IP_ADDRESS_ISSUE = -5
OAI_UE_PROCESS_NOLOGFILE_TO_ANALYZE = -20
OAI_UE_PROCESS_COULD_NOT_SYNC = -21
OAI_UE_PROCESS_ASSERTION = -22
OAI_UE_PROCESS_FAILED = -23
OAI_UE_PROCESS_NO_TUNNEL_INTERFACE = -24
OAI_UE_PROCESS_SEG_FAULT = -25
OAI_UE_PROCESS_NO_MBMS_MSGS = -26
OAI_UE_PROCESS_OK = +6
INVALID_PARAMETER = -50
PHYSIM_IMAGE_ABSENT = -60
OC_LOGIN_FAIL = -61
OC_PROJECT_FAIL = -62
OC_IS_FAIL = -63
OC_PHYSIM_DEPLOY_FAIL = -64

UE_STATUS_DETACHED = 0
UE_STATUS_DETACHING = 1
UE_STATUS_ATTACHING = 2
UE_STATUS_ATTACHED = 3

X2_HO_REQ_STATE__IDLE = 0
X2_HO_REQ_STATE__TARGET_RECEIVES_REQ = 1
X2_HO_REQ_STATE__TARGET_RRC_RECFG_COMPLETE = 2
X2_HO_REQ_STATE__TARGET_SENDS_SWITCH_REQ = 3
X2_HO_REQ_STATE__SOURCE_RECEIVES_REQ_ACK = 10

# placeholder for a "don't care" password to gradually transition to
# passwordless CI
CI_NO_PASSWORD = "CIPASSWORDDONTCARE"
