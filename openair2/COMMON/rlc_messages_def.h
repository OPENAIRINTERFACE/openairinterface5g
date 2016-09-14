/*
 * rlc_messages_def.h
 *
 *  Created on: Jan 15, 2014
 *      Author: Gauthier
 */

//-------------------------------------------------------------------------------------------//
// Messages for RLC logging

#if defined(DISABLE_ITTI_XER_PRINT)

#else
MESSAGE_DEF(RLC_AM_DATA_PDU_IND,                MESSAGE_PRIORITY_MED_PLUS,  IttiMsgText,         rlc_am_data_pdu_ind)
MESSAGE_DEF(RLC_AM_DATA_PDU_REQ,                MESSAGE_PRIORITY_MED_PLUS,  IttiMsgText,         rlc_am_data_pdu_req)
MESSAGE_DEF(RLC_AM_STATUS_PDU_IND,              MESSAGE_PRIORITY_MED_PLUS,  IttiMsgText,         rlc_am_status_pdu_ind)
MESSAGE_DEF(RLC_AM_STATUS_PDU_REQ,              MESSAGE_PRIORITY_MED_PLUS,  IttiMsgText,         rlc_am_status_pdu_req)
MESSAGE_DEF(RLC_AM_SDU_REQ,                     MESSAGE_PRIORITY_MED_PLUS,  IttiMsgText,         rlc_am_sdu_req)
MESSAGE_DEF(RLC_AM_SDU_IND,                     MESSAGE_PRIORITY_MED_PLUS,  IttiMsgText,         rlc_am_sdu_ind)

MESSAGE_DEF(RLC_UM_DATA_PDU_IND,                MESSAGE_PRIORITY_MED_PLUS,  IttiMsgText,         rlc_um_data_pdu_ind)
MESSAGE_DEF(RLC_UM_DATA_PDU_REQ,                MESSAGE_PRIORITY_MED_PLUS,  IttiMsgText,         rlc_um_data_pdu_req)
MESSAGE_DEF(RLC_UM_SDU_REQ,                     MESSAGE_PRIORITY_MED_PLUS,  IttiMsgText,         rlc_um_sdu_req)
MESSAGE_DEF(RLC_UM_SDU_IND,                     MESSAGE_PRIORITY_MED_PLUS,  IttiMsgText,         rlc_um_sdu_ind)
#endif
