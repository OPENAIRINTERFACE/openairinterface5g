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

/*! \file rlc_um_dar.h
* \brief This file defines the prototypes of the functions dealing with the reassembly buffer.
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
#    ifndef __RLC_UM_DAR_H__
#        define __RLC_UM_DAR_H__
//-----------------------------------------------------------------------------
#        include "rlc_um_entity.h"
#        include "rlc_um_structs.h"
#        include "rlc_um_constants.h"
#        include "list.h"
//-----------------------------------------------------------------------------
/*! \fn signed int rlc_um_get_pdu_infos(const protocol_ctxt_t* const ctxt_pP,const rlc_um_entity_t * const rlc_pP,rlc_um_pdu_sn_10_t* header_pP, int16_t total_sizeP, rlc_um_pdu_info_t* pdu_info_pP, uint8_t sn_lengthP)
* \brief    Extract PDU informations (header fields, data size, etc) from the serialized PDU.
* \param[in]  ctxt_pP              Running context.
* \param[in]  rlc_pP             RLC UM protocol instance pointer..
* \param[in]  header_pP          RLC UM header PDU pointer.
* \param[in]  total_sizeP        Size of RLC UM PDU.
* \param[in]  pdu_info_pP        Structure containing extracted informations from PDU.
* \param[in]  sn_lengthP         Sequence number length in bits in PDU header (5 or 10).
* \return     0 if no error was encountered during the parsing of the PDU, else -1;
*/
signed int rlc_um_get_pdu_infos(
                         const protocol_ctxt_t* const ctxt_pP,
                         const rlc_um_entity_t * const rlc_pP,
                         rlc_um_pdu_sn_10_t  * const header_pP,
                         const sdu_size_t            total_sizeP,
                         rlc_um_pdu_info_t   * const pdu_info_pP,
                         const uint8_t               sn_lengthP);

/*! \fn int rlc_um_read_length_indicators(unsigned char**data_ppP, rlc_um_e_li_t* e_li_pP, unsigned int* li_array_pP, unsigned int *num_li_pP, sdu_size_t *data_size_pP)
* \brief    Reset protocol variables and state variables to initial values.
* \param[in,out]  data_ppP       Pointer on data payload.
* \param[in]      e_li_pP        Pointer on first LI + e bit in PDU.
* \param[in,out]  li_array_pP    Array containing read LI.
* \param[in,out]  num_li_pP      Pointer on number of LI read.
* \param[in,out]  data_size_pP   Pointer on data size.
* \return     0 if no error was encountered during the parsing of the PDU, else -1;
*/
int rlc_um_read_length_indicators(unsigned char**data_ppP, rlc_um_e_li_t* e_li_pP, unsigned int* li_array_pP, unsigned int *num_li_pP, sdu_size_t *data_size_pP);

/*! \fn void rlc_um_try_reassembly      (const protocol_ctxt_t* const ctxt_pP, rlc_um_entity_t * const rlc_pP, rlc_sn_t start_snP, rlc_sn_t end_snP)
* \brief    Try reassembly PDUs from DAR buffer, starting at sequence number snP.
* \param[in]  ctxt_pP       Running context.
* \param[in]  rlc_pP      RLC UM protocol instance pointer.
* \param[in]  frameP      Frame index.
* \param[in]  start_snP   First PDU to be reassemblied if possible.
* \param[in]  end_snP     Last excluded highest sequence number of PDU to be reassemblied.
*/
void rlc_um_try_reassembly      (const protocol_ctxt_t* const ctxt_pP, rlc_um_entity_t * const rlc_pP, const rlc_sn_t start_snP, const rlc_sn_t end_snP);

/*! \fn void rlc_um_check_timer_reordering(rlc_um_entity_t * const rlc_pP,frame_t frameP)
* \brief      Check if timer reordering has timed-out, if so it is stopped and has the status "timed-out".
* \param[in]  ctxt_pP             Running context.
* \param[in]  rlc_pP            RLC UM protocol instance pointer.
* \param[in]  frameP            Frame index
*/
void rlc_um_check_timer_reordering(const protocol_ctxt_t* const ctxt_pP, rlc_um_entity_t  *const rlc_pP);

/*! \fn void rlc_um_stop_and_reset_timer_reordering(const protocol_ctxt_t* const ctxt_pP, rlc_um_entity_t * const rlc_pP)
* \brief      Stop and reset the timer reordering.
* \param[in]  ctxt_pP             Running context.
* \param[in]  rlc_pP            RLC UM protocol instance pointer.
* \param[in]  frameP            Frame index.
*/
void rlc_um_stop_and_reset_timer_reordering(const protocol_ctxt_t* const ctxt_pP, rlc_um_entity_t *const rlc_pP);

/*! \fn void rlc_um_start_timer_reordering(const protocol_ctxt_t* const ctxt_pP, rlc_um_entity_t * const rlc_pP)
* \brief      Re-arm (based on RLC UM config parameter) and start timer reordering.
* \param[in]  ctxt_pP             Running context.
* \param[in]  rlc_pP            RLC UM protocol instance pointer.
* \param[in]  frameP            Frame index.
*/
void rlc_um_start_timer_reordering(const protocol_ctxt_t* const ctxt_pP, rlc_um_entity_t * const rlc_pP);

/*! \fn void rlc_um_init_timer_reordering(rlc_um_entity_t * const rlc_pP, const uint32_t ms_durationP)
* \brief      Initialize the timer reordering with RLC UM time-out config parameter.
* \param[in]  ctxt_pP             Running context.
* \param[in]  rlc_pP            RLC UM protocol instance pointer.
* \param[in]  ms_durationP      Duration in milliseconds units.
*/
void rlc_um_init_timer_reordering(const protocol_ctxt_t* const ctxt_pP, rlc_um_entity_t * const rlc_pP, const uint32_t ms_durationP);

/*! \fn void rlc_um_check_timer_dar_time_out(const protocol_ctxt_t* const ctxt_pP, rlc_um_entity_t * const rlc_pP,)
* \brief    Check if t-Reordering expires and take the appropriate actions as described in 3GPP specifications.
* \param[in]  ctxt_pP        Running context.
* \param[in]  rlc_pP       RLC UM protocol instance pointer.
*/
void rlc_um_check_timer_dar_time_out(const protocol_ctxt_t* const ctxt_pP, rlc_um_entity_t * const rlc_pP);

/*! \fn mem_block_t *rlc_um_remove_pdu_from_dar_buffer(rlc_um_entity_t * const rlc_pP, uint16_t snP)
* \brief    Remove the PDU with sequence number snP from the DAR buffer and return it.
* \param[in]  ctxt_pP         Running context.
* \param[in]  rlc_pP        RLC UM protocol instance pointer.
* \param[in]  snP           Sequence number.
* \return     The PDU stored in the DAR buffer having sequence number snP, else return NULL.
*/
mem_block_t *rlc_um_remove_pdu_from_dar_buffer(const protocol_ctxt_t* const ctxt_pP, rlc_um_entity_t * const rlc_pP, const uint16_t snP);

/*! \fn mem_block_t *rlc_um_remove_pdu_from_dar_buffer(const protocol_ctxt_t* const ctxt_pP, rlc_um_entity_t * const rlc_pP, const uint16_t snP)
* \brief    Get the PDU with sequence number snP from the DAR buffer.
* \param[in]  ctxt_pP         Running context.
* \param[in]  rlc_pP        RLC UM protocol instance pointer.
* \param[in]  snP           Sequence number.
* \return     The PDU stored in the DAR buffer having sequence number snP, else return NULL.
*/
mem_block_t* rlc_um_get_pdu_from_dar_buffer(const protocol_ctxt_t* const ctxt_pP, rlc_um_entity_t * const rlc_pP, const uint16_t snP);

/*! \fn signed int rlc_um_in_window(const protocol_ctxt_t* const ctxt_pP, rlc_um_entity_t * const rlc_pP,rlc_sn_t lower_boundP, rlc_sn_t snP, rlc_sn_t higher_boundP)
* \brief    Compute if the sequence number of a PDU is in a window .
* \param[in]  ctxt_pP          Running context.
* \param[in]  rlc_pP         RLC UM protocol instance pointer.
* \param[in]  lower_boundP   Lower bound of a window.
* \param[in]  snP            Sequence number of a theorical PDU.
* \param[in]  higher_boundP  Higher bound of a window.
* \return     -2 if lower_boundP  > sn, -1 if higher_boundP < sn, 0 if lower_boundP  < sn < higher_boundP, 1 if lower_boundP  == sn, 2 if higher_boundP == sn, 3 if higher_boundP == sn == lower_boundP.
*/
signed int rlc_um_in_window(const protocol_ctxt_t* const ctxt_pP, rlc_um_entity_t * const rlc_pP, const rlc_sn_t lower_boundP, const rlc_sn_t snP, const rlc_sn_t higher_boundP);

/*! \fn signed int rlc_um_in_reordering_window(const protocol_ctxt_t* const ctxt_pP, rlc_um_entity_t * const rlc_pP, const rlc_sn_t snP)
* \brief    Compute if the sequence number of a PDU is in a window .
* \param[in]  ctxt_pP          Running context.
* \param[in]  rlc_pP         RLC UM protocol instance pointer.
* \param[in]  snP            Sequence number of a theorical PDU.
* \return     0 if snP is in reordering window, else -1.
*/
signed int rlc_um_in_reordering_window(const protocol_ctxt_t* const ctxt_pP, rlc_um_entity_t * const rlc_pP,  const rlc_sn_t snP);

/*! \fn void rlc_um_receive_process_dar (const protocol_ctxt_t* const ctxt_pP, rlc_um_entity_t * const rlc_pP, mem_block_t *pdu_mem_pP,rlc_um_pdu_sn_10_t * const pdu_pP, const sdu_size_t tb_sizeP)
* \brief    Apply the DAR process for a PDU: put it in DAR buffer and try to reassembly or discard it.
* \param[in]  ctxt_pP        Running context.
* \param[in]  rlc_pP     RLC UM protocol instance pointer.
* \param[in]  pdu_mem_pP mem_block_t wrapper for a UM PDU .
* \param[in]  pdu_pP     Pointer on the header of the UM PDU.
* \param[in]  tb_sizeP   Size of the UM PDU.
*/
void rlc_um_receive_process_dar (const protocol_ctxt_t* const ctxt_pP, rlc_um_entity_t * const rlc_pP, mem_block_t * pdu_mem_pP,rlc_um_pdu_sn_10_t * const pdu_pP,
                     const sdu_size_t tb_sizeP);
/** @} */
#    endif
