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
 * nas_messages_def.h
 *
 *  Created on: Jan 07, 2014
 *      Author: winckel
 */

#if defined(ENABLE_USE_MME) && defined(ENABLE_NAS_UE_LOGGING)
//-------------------------------------------------------------------------------------------//
// Messages for NAS logging
MESSAGE_DEF(NAS_DL_EMM_RAW_MSG,                 MESSAGE_PRIORITY_MED,   nas_raw_msg_t,              nas_dl_emm_raw_msg)
MESSAGE_DEF(NAS_UL_EMM_RAW_MSG,                 MESSAGE_PRIORITY_MED,   nas_raw_msg_t,              nas_ul_emm_raw_msg)

MESSAGE_DEF(NAS_DL_EMM_PLAIN_MSG,               MESSAGE_PRIORITY_MED,   nas_emm_plain_msg_t,        nas_dl_emm_plain_msg)
MESSAGE_DEF(NAS_UL_EMM_PLAIN_MSG,               MESSAGE_PRIORITY_MED,   nas_emm_plain_msg_t,        nas_ul_emm_plain_msg)
MESSAGE_DEF(NAS_DL_EMM_PROTECTED_MSG,           MESSAGE_PRIORITY_MED,   nas_emm_protected_msg_t,    nas_dl_emm_protected_msg)
MESSAGE_DEF(NAS_UL_EMM_PROTECTED_MSG,           MESSAGE_PRIORITY_MED,   nas_emm_protected_msg_t,    nas_ul_emm_protected_msg)

MESSAGE_DEF(NAS_DL_ESM_RAW_MSG,                 MESSAGE_PRIORITY_MED,   nas_raw_msg_t,              nas_dl_esm_raw_msg)
MESSAGE_DEF(NAS_UL_ESM_RAW_MSG,                 MESSAGE_PRIORITY_MED,   nas_raw_msg_t,              nas_ul_esm_raw_msg)

MESSAGE_DEF(NAS_DL_ESM_PLAIN_MSG,               MESSAGE_PRIORITY_MED,   nas_esm_plain_msg_t,        nas_dl_esm_plain_msg)
MESSAGE_DEF(NAS_UL_ESM_PLAIN_MSG,               MESSAGE_PRIORITY_MED,   nas_esm_plain_msg_t,        nas_ul_esm_plain_msg)
MESSAGE_DEF(NAS_DL_ESM_PROTECTED_MSG,           MESSAGE_PRIORITY_MED,   nas_esm_protected_msg_t,    nas_dl_esm_protected_msg)
MESSAGE_DEF(NAS_UL_ESM_PROTECTED_MSG,           MESSAGE_PRIORITY_MED,   nas_esm_protected_msg_t,    nas_ul_esm_protected_msg)

//-------------------------------------------------------------------------------------------//
#endif /* ENABLE_USE_MME */
