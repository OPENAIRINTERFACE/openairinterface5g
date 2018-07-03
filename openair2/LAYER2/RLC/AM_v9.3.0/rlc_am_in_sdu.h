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

/*! \file rlc_am_in_sdu.h
* \brief This file defines the prototypes of the utility functions manipulating the incoming SDU buffer.
* \author GAUTHIER Lionel
* \date 2010-2011
* \version
* \note
* \bug
* \warning
*/
/** @defgroup _rlc_am_internal_input_sdu_impl_ RLC AM Input SDU buffer Internal Reference Implementation
* @ingroup _rlc_am_internal_impl_
* @{
*/
#    ifndef __RLC_AM_IN_SDU_H__
#        define __RLC_AM_IN_SDU_H__
/*! \fn void rlc_am_free_in_sdu (const protocol_ctxt_t* const  ctxt_pP, rlc_am_entity_t *rlcP, unsigned int index_in_bufferP)
* \brief    Free a higher layer SDU stored in input_sdus[] buffer.
* \param[in]  ctxtP                     Running context.
* \param[in]  rlcP                      RLC AM protocol instance pointer.
* \param[in]  index_in_bufferP          Position index of the SDU.
* \note Update also the RLC AM instance variables nb_sdu, current_sdu_index, nb_sdu_no_segmented.
*/
void rlc_am_free_in_sdu      (const protocol_ctxt_t* const  ctxt_pP, rlc_am_entity_t *rlcP, unsigned int index_in_bufferP);


/*! \fn void rlc_am_free_in_sdu_data (const protocol_ctxt_t* const  ctxt_pP, rlc_am_entity_t *rlcP, unsigned int index_in_bufferP)
* \brief    Free a higher layer SDU data part, the SDU is stored in input_sdus[] buffer.
* \param[in]  ctxtP                     Running context.
* \param[in]  rlcP                      RLC AM protocol instance pointer.
* \param[in]  index_in_bufferP          Position index of the SDU.
* \note This procedure is called when the SDU segmentation is done for this SDU. Update also the RLC AM instance variable nb_sdu_no_segmented.
*/
void rlc_am_free_in_sdu_data (const protocol_ctxt_t* const  ctxt_pP, rlc_am_entity_t *rlcP, unsigned int index_in_bufferP);


/*! \fn signed int rlc_am_in_sdu_is_empty(const protocol_ctxt_t* const  ctxt_pP, rlc_am_entity_t *rlcP)
* \brief    Indicates if the input SDU buffer for incoming higher layer SDUs is empty or not.
* \param[in]  ctxtP                     Running context.
* \param[in]  rlcP                      RLC AM protocol instance pointer.
* \return 1 if the buffer is empty, else 0.
*/
signed int rlc_am_in_sdu_is_empty(const protocol_ctxt_t* const  ctxt_pP, rlc_am_entity_t *rlcP);

/*! \fn void rlc_am_pdu_sdu_data_cnf(const protocol_ctxt_t* const ctxt_pP,rlc_am_entity_t* const       rlc_pP,const rlc_sn_t           snP)
* \brief    Process SDU cnf of a ACKED PDU for all SDUs concatenated in this PDU.
* \param[in]  ctxtP                     Running context.
* \param[in]  rlcP                      RLC AM protocol instance pointer.
* \param[in]  snP                       Sequence number of the PDU.
*/
void rlc_am_pdu_sdu_data_cnf(const protocol_ctxt_t* const ctxt_pP,rlc_am_entity_t* const       rlc_pP,const rlc_sn_t           snP);
/** @} */
#    endif
