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
 * mac_messages_def.h
 *
 *  Created on: Oct 24, 2013
 *      Author: L. winckel and Navid Nikaein
 */

//-------------------------------------------------------------------------------------------//
// Messages between RRC and MAC layers
MESSAGE_DEF(RRC_MAC_IN_SYNC_IND,        MESSAGE_PRIORITY_MED_PLUS, RrcMacInSyncInd,             rrc_mac_in_sync_ind)
MESSAGE_DEF(RRC_MAC_OUT_OF_SYNC_IND,    MESSAGE_PRIORITY_MED_PLUS, RrcMacOutOfSyncInd,          rrc_mac_out_of_sync_ind)

MESSAGE_DEF(RRC_MAC_BCCH_DATA_REQ,      MESSAGE_PRIORITY_MED_PLUS, RrcMacBcchDataReq,           rrc_mac_bcch_data_req)
MESSAGE_DEF(RRC_MAC_BCCH_DATA_IND,      MESSAGE_PRIORITY_MED_PLUS, RrcMacBcchDataInd,           rrc_mac_bcch_data_ind)

MESSAGE_DEF(RRC_MAC_BCCH_MBMS_DATA_REQ,      MESSAGE_PRIORITY_MED_PLUS, RrcMacBcchMbmsDataReq,           rrc_mac_bcch_mbms_data_req)
MESSAGE_DEF(RRC_MAC_BCCH_MBMS_DATA_IND,      MESSAGE_PRIORITY_MED_PLUS, RrcMacBcchMbmsDataInd,           rrc_mac_bcch_mbms_data_ind)

MESSAGE_DEF(RRC_MAC_CCCH_DATA_REQ,      MESSAGE_PRIORITY_MED_PLUS, RrcMacCcchDataReq,           rrc_mac_ccch_data_req)
MESSAGE_DEF(RRC_MAC_CCCH_DATA_CNF,      MESSAGE_PRIORITY_MED_PLUS, RrcMacCcchDataCnf,           rrc_mac_ccch_data_cnf)
MESSAGE_DEF(RRC_MAC_CCCH_DATA_IND,      MESSAGE_PRIORITY_MED_PLUS, RrcMacCcchDataInd,           rrc_mac_ccch_data_ind)


MESSAGE_DEF(RRC_MAC_MCCH_DATA_REQ,      MESSAGE_PRIORITY_MED_PLUS, RrcMacMcchDataReq,           rrc_mac_mcch_data_req)
MESSAGE_DEF(RRC_MAC_MCCH_DATA_IND,      MESSAGE_PRIORITY_MED_PLUS, RrcMacMcchDataInd,           rrc_mac_mcch_data_ind)

MESSAGE_DEF(RRC_MAC_PCCH_DATA_REQ,      MESSAGE_PRIORITY_MED_PLUS, RrcMacPcchDataReq,           rrc_mac_pcch_data_req)

/* RRC configures DRX context (MAC timers) of a UE */
MESSAGE_DEF(RRC_MAC_DRX_CONFIG_REQ, MESSAGE_PRIORITY_MED, rrc_mac_drx_config_req_t, rrc_mac_drx_config_req)

// gNB
MESSAGE_DEF(NR_RRC_MAC_CCCH_DATA_IND,    MESSAGE_PRIORITY_MED_PLUS, NRRrcMacCcchDataInd,           nr_rrc_mac_ccch_data_ind)
MESSAGE_DEF(NR_RRC_MAC_BCCH_DATA_IND,    MESSAGE_PRIORITY_MED_PLUS, NRRrcMacBcchDataInd,           nr_rrc_mac_bcch_data_ind)

