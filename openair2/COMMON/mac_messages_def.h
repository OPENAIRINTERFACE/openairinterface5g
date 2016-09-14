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

MESSAGE_DEF(RRC_MAC_CCCH_DATA_REQ,      MESSAGE_PRIORITY_MED_PLUS, RrcMacCcchDataReq,           rrc_mac_ccch_data_req)
MESSAGE_DEF(RRC_MAC_CCCH_DATA_CNF,      MESSAGE_PRIORITY_MED_PLUS, RrcMacCcchDataCnf,           rrc_mac_ccch_data_cnf)
MESSAGE_DEF(RRC_MAC_CCCH_DATA_IND,      MESSAGE_PRIORITY_MED_PLUS, RrcMacCcchDataInd,           rrc_mac_ccch_data_ind)

MESSAGE_DEF(RRC_MAC_MCCH_DATA_REQ,      MESSAGE_PRIORITY_MED_PLUS, RrcMacMcchDataReq,           rrc_mac_mcch_data_req)
MESSAGE_DEF(RRC_MAC_MCCH_DATA_IND,      MESSAGE_PRIORITY_MED_PLUS, RrcMacMcchDataInd,           rrc_mac_mcch_data_ind)
