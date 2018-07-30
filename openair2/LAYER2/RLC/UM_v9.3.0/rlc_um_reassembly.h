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

/*! \file rlc_um_reassembly.h
* \brief This file defines the prototypes of the functions dealing with the reassembly of segments.
* \author GAUTHIER Lionel
* \date 2010-2011
* \version
* \note
* \bug
* \warning
*/
/** @defgroup _rlc_um_receiver_impl_ RLC UM Receiver Implementation
* @ingroup _rlc_um_impl_
* @{
*/
#    ifndef __RLC_UM_REASSEMBLY_PROTO_EXTERN_H__
#        define __RLC_UM_REASSEMBLY_PROTO_EXTERN_H__
//-----------------------------------------------------------------------------
#        include "rlc_um_entity.h"
//-----------------------------------------------------------------------------
/*! \fn void rlc_um_clear_rx_sdu (const protocol_ctxt_t* const ctxtP, rlc_um_entity_t *rlcP)
* \brief    Erase the SDU in construction.
* \param[in]  ctxtP       Running context.
* \param[in]  rlcP        RLC UM protocol instance pointer.
*/
void rlc_um_clear_rx_sdu (const protocol_ctxt_t* const ctxtP, rlc_um_entity_t *rlcP);

/*! \fn void rlc_um_reassembly (uint8_t * srcP, int32_t lengthP, rlc_um_entity_t *rlcP, frame_t frame)
* \brief    Reassembly lengthP bytes to the end of the SDU in construction.
* \param[in]  ctxtP       Running context.
* \param[in]  rlcP        RLC UM protocol instance pointer.
* \param[in]  srcP        Pointer on data to be reassemblied.
* \param[in]  lengthP     Length to reassembly.
*/
void     rlc_um_reassembly (const protocol_ctxt_t* const ctxtP, rlc_um_entity_t *rlcP, uint8_t * srcP, int32_t lengthP);

/*! \fn void rlc_um_send_sdu (const protocol_ctxt_t* const ctxtP, rlc_um_entity_t *rlcP)
* \brief    Send SDU if any reassemblied to upper layer.
* \param[in]  ctxtP       Running context.
* \param[in]  rlcP        RLC UM protocol instance pointer.
*/
void     rlc_um_send_sdu (const protocol_ctxt_t* const ctxtP, rlc_um_entity_t *rlcP);
/** @} */
#    endif
