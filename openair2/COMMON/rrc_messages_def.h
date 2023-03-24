/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.1  (the "License"); you may not use this file
 * except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.openairinterface.org/?page_id=698
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

/*
 * rrc_messages_def.h
 *
 *  Created on: Oct 24, 2013
 *      Author: winckel
 */

//-------------------------------------------------------------------------------------------//
// Messages for RRC logging
#if defined(DISABLE_ITTI_XER_PRINT)
MESSAGE_DEF(RRC_DL_BCCH_MESSAGE,        MESSAGE_PRIORITY_MED_PLUS,  RrcDlBcchMessage,           rrc_dl_bcch_message)
MESSAGE_DEF(RRC_DL_CCCH_MESSAGE,        MESSAGE_PRIORITY_MED_PLUS,  RrcDlCcchMessage,           rrc_dl_ccch_message)
MESSAGE_DEF(RRC_DL_DCCH_MESSAGE,        MESSAGE_PRIORITY_MED_PLUS,  RrcDlDcchMessage,           rrc_dl_dcch_message)

MESSAGE_DEF(RRC_UE_EUTRA_CAPABILITY,    MESSAGE_PRIORITY_MED_PLUS,  RrcUeEutraCapability,       rrc_ue_eutra_capability)

MESSAGE_DEF(RRC_UL_CCCH_MESSAGE,        MESSAGE_PRIORITY_MED_PLUS,  RrcUlCcchMessage,           rrc_ul_ccch_message)
MESSAGE_DEF(RRC_UL_DCCH_MESSAGE,        MESSAGE_PRIORITY_MED_PLUS,  RrcUlDcchMessage,           rrc_ul_dcch_message)
#else
MESSAGE_DEF(RRC_DL_BCCH,                MESSAGE_PRIORITY_MED_PLUS,  IttiMsgText,                rrc_dl_bcch)
MESSAGE_DEF(RRC_DL_CCCH,                MESSAGE_PRIORITY_MED_PLUS,  IttiMsgText,                rrc_dl_ccch)
MESSAGE_DEF(RRC_DL_DCCH,                MESSAGE_PRIORITY_MED_PLUS,  IttiMsgText,                rrc_dl_dcch)
MESSAGE_DEF(RRC_DL_MCCH,                MESSAGE_PRIORITY_MED_PLUS,  IttiMsgText,                rrc_dl_mcch)

MESSAGE_DEF(RRC_UE_EUTRA_CAPABILITY,    MESSAGE_PRIORITY_MED_PLUS,  IttiMsgText,                rrc_ue_eutra_capability)

MESSAGE_DEF(RRC_UL_CCCH,                MESSAGE_PRIORITY_MED_PLUS,  IttiMsgText,                rrc_ul_ccch)
MESSAGE_DEF(RRC_UL_DCCH,                MESSAGE_PRIORITY_MED_PLUS,  IttiMsgText,                rrc_ul_dcch)
#endif

MESSAGE_DEF(RRC_STATE_IND,              MESSAGE_PRIORITY_MED,       RrcStateInd,                rrc_state_ind)

//-------------------------------------------------------------------------------------------//
// eNB: ENB_APP -> RRC messages
MESSAGE_DEF(RRC_CONFIGURATION_REQ,      MESSAGE_PRIORITY_MED,       RrcConfigurationReq,        rrc_configuration_req)
MESSAGE_DEF(NBIOTRRC_CONFIGURATION_REQ, MESSAGE_PRIORITY_MED,       NbIoTRrcConfigurationReq,   nbiotrrc_configuration_req)
MESSAGE_DEF(NRRRC_CONFIGURATION_REQ,    MESSAGE_PRIORITY_MED,       gNB_RrcConfigurationReq,    nrrrc_configuration_req)

// UE: NAS -> RRC messages
MESSAGE_DEF(NAS_KENB_REFRESH_REQ,       MESSAGE_PRIORITY_MED,       NasKenbRefreshReq,          nas_kenb_refresh_req)
MESSAGE_DEF(NAS_CELL_SELECTION_REQ,     MESSAGE_PRIORITY_MED,       NasCellSelectionReq,        nas_cell_selection_req)
MESSAGE_DEF(NAS_CONN_ESTABLI_REQ,       MESSAGE_PRIORITY_MED,       NasConnEstabliReq,          nas_conn_establi_req)
MESSAGE_DEF(NAS_UPLINK_DATA_REQ,        MESSAGE_PRIORITY_MED,       NasUlDataReq,               nas_ul_data_req)

MESSAGE_DEF(NAS_RAB_ESTABLI_RSP,        MESSAGE_PRIORITY_MED,       NasRabEstRsp,               nas_rab_est_rsp)

MESSAGE_DEF(NAS_OAI_TUN_NSA,            MESSAGE_PRIORITY_MED,       NasOaiTunNsa,               nas_oai_tun_nsa)

// UE: RRC -> NAS messages
MESSAGE_DEF(NAS_CELL_SELECTION_CNF,     MESSAGE_PRIORITY_MED,       NasCellSelectionCnf,        nas_cell_selection_cnf)
MESSAGE_DEF(NAS_CELL_SELECTION_IND,     MESSAGE_PRIORITY_MED,       NasCellSelectionInd,        nas_cell_selection_ind)
MESSAGE_DEF(NAS_PAGING_IND,             MESSAGE_PRIORITY_MED,       NasPagingInd,               nas_paging_ind)
MESSAGE_DEF(NAS_CONN_ESTABLI_CNF,       MESSAGE_PRIORITY_MED,       NasConnEstabCnf,            nas_conn_establi_cnf)
MESSAGE_DEF(NAS_CONN_RELEASE_IND,       MESSAGE_PRIORITY_MED,       NasConnReleaseInd,          nas_conn_release_ind)
MESSAGE_DEF(NAS_UPLINK_DATA_CNF,        MESSAGE_PRIORITY_MED,       NasUlDataCnf,               nas_ul_data_cnf)
MESSAGE_DEF(NAS_DOWNLINK_DATA_IND,      MESSAGE_PRIORITY_MED,       NasDlDataInd,               nas_dl_data_ind)

// eNB: realtime -> RRC messages
MESSAGE_DEF(RRC_SUBFRAME_PROCESS,       MESSAGE_PRIORITY_MED,       RrcSubframeProcess,         rrc_subframe_process)

// eNB: RLC -> RRC messages
MESSAGE_DEF(RLC_SDU_INDICATION,         MESSAGE_PRIORITY_MED,       RlcSduIndication,           rlc_sdu_indication)
