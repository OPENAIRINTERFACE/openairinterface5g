/*******************************************************************************
    OpenAirInterface
    Copyright(c) 1999 - 2014 Eurecom

    OpenAirInterface is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.


    OpenAirInterface is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with OpenAirInterface.The full GNU General Public License is
   included in this distribution in the file called "COPYING". If not,
   see <http://www.gnu.org/licenses/>.

  Contact Information
  OpenAirInterface Admin: openair_admin@eurecom.fr
  OpenAirInterface Tech : openair_tech@eurecom.fr
  OpenAirInterface Dev  : openair4g-devel@lists.eurecom.fr

  Address      : Eurecom, Campus SophiaTech, 450 Route des Chappes, CS 50193 - 06904 Biot Sophia Antipolis cedex, FRANCE

*******************************************************************************/

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

// UE: NAS -> RRC messages
MESSAGE_DEF(NAS_CELL_SELECTION_REQ,     MESSAGE_PRIORITY_MED,       NasCellSelectionReq,        nas_cell_selection_req)
MESSAGE_DEF(NAS_CONN_ESTABLI_REQ,       MESSAGE_PRIORITY_MED,       NasConnEstabliReq,          nas_conn_establi_req)
MESSAGE_DEF(NAS_UPLINK_DATA_REQ,        MESSAGE_PRIORITY_MED,       NasUlDataReq,               nas_ul_data_req)

MESSAGE_DEF(NAS_RAB_ESTABLI_RSP,        MESSAGE_PRIORITY_MED,       NasRabEstRsp,               nas_rab_est_rsp)

// UE: RRC -> NAS messages
MESSAGE_DEF(NAS_CELL_SELECTION_CNF,     MESSAGE_PRIORITY_MED,       NasCellSelectionCnf,        nas_cell_selection_cnf)
MESSAGE_DEF(NAS_CELL_SELECTION_IND,     MESSAGE_PRIORITY_MED,       NasCellSelectionInd,        nas_cell_selection_ind)
MESSAGE_DEF(NAS_PAGING_IND,             MESSAGE_PRIORITY_MED,       NasPagingInd,               nas_paging_ind)
MESSAGE_DEF(NAS_CONN_ESTABLI_CNF,       MESSAGE_PRIORITY_MED,       NasConnEstabCnf,            nas_conn_establi_cnf)
MESSAGE_DEF(NAS_CONN_RELEASE_IND,       MESSAGE_PRIORITY_MED,       NasConnReleaseInd,          nas_conn_release_ind)
MESSAGE_DEF(NAS_UPLINK_DATA_CNF,        MESSAGE_PRIORITY_MED,       NasUlDataCnf,               nas_ul_data_cnf)
MESSAGE_DEF(NAS_DOWNLINK_DATA_IND,      MESSAGE_PRIORITY_MED,       NasDlDataInd,               nas_dl_data_ind)
