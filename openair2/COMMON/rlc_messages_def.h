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
