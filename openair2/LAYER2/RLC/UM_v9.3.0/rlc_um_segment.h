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

/*! \file rlc_um_segment.h
* \brief This file defines the prototypes of the functions dealing with the segmentation of PDCP SDUs.
* \author GAUTHIER Lionel
* \date 2010-2011
* \version
* \note
* \bug
* \warning
*/
/** @defgroup _rlc_um_segment_impl_ RLC UM Segmentation Implementation
* @ingroup _rlc_um_impl_
* @{
*/
#    ifndef __RLC_UM_SEGMENT_PROTO_EXTERN_H__
#        define __RLC_UM_SEGMENT_PROTO_EXTERN_H__
//-----------------------------------------------------------------------------
#        include "rlc_um_entity.h"
#        include "rlc_um_structs.h"
#        include "rlc_um_constants.h"
#        include "list.h"
//-----------------------------------------------------------------------------
/*! \fn void rlc_um_segment_10 (const protocol_ctxt_t* const ctxtP, rlc_um_entity_t *rlcP)
* \brief    Segmentation procedure with 10 bits sequence number, segment the first SDU in buffer and create a PDU of the size (nb_bytes_to_transmit) requested by MAC if possible and put it in the list "pdus_to_mac_layer".
* \param[in]  ctxtP       Running context.
* \param[in]  rlcP        RLC UM protocol instance pointer.
*/
void rlc_um_segment_10 (const protocol_ctxt_t* const ctxtP, rlc_um_entity_t *rlcP);


/*! \fn void rlc_um_segment_5 (const protocol_ctxt_t* const ctxtP, rlc_um_entity_t *rlcP)
* \brief    Segmentation procedure with 5 bits sequence number, segment the first SDU in buffer and create a PDU of the size (nb_bytes_to_transmit) requested by MAC if possible and put it in the list "pdus_to_mac_layer".
* \param[in]  ctxtP       Running context.
* \param[in]  rlcP        RLC UM protocol instance pointer.
*/
void rlc_um_segment_5 (const protocol_ctxt_t* const ctxtP, rlc_um_entity_t *rlcP);
/** @} */
#    endif
