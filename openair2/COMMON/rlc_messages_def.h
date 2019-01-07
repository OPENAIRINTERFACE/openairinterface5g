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
