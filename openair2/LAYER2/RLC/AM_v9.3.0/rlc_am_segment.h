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

/*! \file rlc_am_segment.h
* \brief This file defines the prototypes of the functions dealing with the segmentation of PDCP SDUs.
* \author GAUTHIER Lionel
* \date 2010-2011
* \version
* \note
* \bug
* \warning
*/
/** @defgroup _rlc_am_internal_segment_impl_ RLC AM Segmentation Internal Reference Implementation
* @ingroup _rlc_am_internal_impl_
* @{
*/
#    ifndef __RLC_AM_SEGMENT_H__
#        define __RLC_AM_SEGMENT_H__
//-----------------------------------------------------------------------------

/*! \fn void rlc_am_pdu_polling (const protocol_ctxt_t* const  ctxt_pP, rlc_am_entity_t *const rlcP, rlc_am_pdu_sn_10_t *pduP, int16_t payload_sizeP, bool is_new_pdu)
* \brief      Set or not the poll bit in the PDU header depending on RLC AM protocol variables.
* \param[in]  ctxt_pP          Running context.
* \param[in]  rlcP           RLC AM protocol instance pointer.
* \param[in]  pduP           Pointer on the header of the PDU in order to be able to set the poll bit if necessary.
* \param[in]  payload_sizeP  Size of the payload of the PDU.
*/
void rlc_am_pdu_polling (const protocol_ctxt_t* const  ctxt_pP, rlc_am_entity_t *const rlcP, rlc_am_pdu_sn_10_t *pduP, int16_t payload_sizeP, bool is_new_pdu);

/*! \fn void rlc_am_segment_10 (const protocol_ctxt_t* const  ctxt_pP, rlc_am_entity_t * const rlcP)
* \brief      Segment a PDU with 10 bits sequence number, based on segmentation information given by MAC (size to transmit).
* \param[in]  ctxt_pP        Running context.
* \param[in]  rlcP         RLC AM protocol instance pointer.
*/
void rlc_am_segment_10 (const protocol_ctxt_t* const  ctxt_pP,rlc_am_entity_t *const rlcP);
/** @} */
#    endif
