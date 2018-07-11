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

/*! \file rlc_am_reassembly.h
* \brief This file defines the prototypes of the functions dealing with the reassembly of segments.
* \author GAUTHIER Lionel
* \date 2010-2011
* \version
* \note
* \bug
* \warning
*/
/** @defgroup _rlc_am_internal_reassembly_impl_ RLC AM Reassembly Internal Reference Implementation
* @ingroup _rlc_am_internal_impl_
* @{
*/
#ifndef __RLC_AM_REASSEMBLY_H__
#    define __RLC_AM_REASSEMBLY_H__
/*! \fn void rlc_am_clear_rx_sdu (const protocol_ctxt_t* const ctxtP, rlc_am_entity_t * const rlc_pP)
* \brief    Reset the data cursor index in the output SDU buffer to zero.
* \param[in]  ctxtP                       Running context.
* \param[in]  rlc_pP                      RLC AM protocol instance pointer.
*/
void rlc_am_clear_rx_sdu (const protocol_ctxt_t* const ctxtP, rlc_am_entity_t * const rlc_pP);

/*! \fn void rlc_am_reassembly   (const protocol_ctxt_t* const ctxtP, rlc_am_entity_t * const rlc_pPuint8_t * srcP, int32_t lengthP)
* \brief    Concatenate datas at the tail of the output SDU in construction. This SDU in construction will be sent to higher layer.
* \param[in]  ctxtP                       Running context.
* \param[in]  rlc_pP                      RLC AM protocol instance pointer.
* \param[in]  srcP                        Pointer on data to be reassemblied.
* \param[in]  lengthP                     Length of data to be reassemblied.
*/
void rlc_am_reassembly   (const protocol_ctxt_t* const ctxtP, rlc_am_entity_t * const rlc_pP, uint8_t * srcP, int32_t lengthP);

/*! \fn void rlc_am_send_sdu     (rlc_am_entity_t *rlc_pP,frame_t frameP)
* \brief    Send the output SDU in construction to higher layer.
* \param[in]  ctxtP                       Running context.
* \param[in]  rlc_pP                      RLC AM protocol instance pointer.
*/
void rlc_am_send_sdu     (const protocol_ctxt_t* const ctxtP, rlc_am_entity_t * const rlc_pP);

/*! \fn void rlc_am_reassemble_pdu(const protocol_ctxt_t* const ctxtP, rlc_am_entity_t * const rlc_pP, const  mem_block_t* const tb_pP,boolean_t free_rlc_pdu)
* \brief    Reassembly a RLC AM PDU, depending of the content of this PDU, data will be reassemblied to the current output SDU, the current will be sent to higher layers or not, after or before the reassembly, or no send of SDU will be triggered, depending on FI field in PDU header.
* \param[in]  ctxtP                       Running context.
* \param[in]  rlc_pP                      RLC AM protocol instance pointer.
* \param[in]  tb_pP                       RLC AM PDU embedded in a mem_block_t.
* \param[in]  free_rlc_pdu                Flag for freeing RLC AM PDU after reassembly.
*/
void rlc_am_reassemble_pdu(const protocol_ctxt_t* const ctxtP, rlc_am_entity_t * const rlc_pP, mem_block_t* const tb_pP,boolean_t free_rlc_pdu);
/** @} */
#endif

