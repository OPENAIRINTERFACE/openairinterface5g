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

/*! \file rlc_um_control_primitives.h
* \brief This file defines the prototypes of the functions dealing with the control primitives and initialization.
* \author GAUTHIER Lionel
* \date 2010-2011
* \version
* \note
* \bug
* \warning
*/
/** @defgroup _rlc_um_init_impl_ RLC UM Init Reference Implementation
* @ingroup _rlc_um_impl_
* @{
*/
#    ifndef __RLC_UM_CONTROL_PRIMITIVES_H__
#        define __RLC_UM_CONTROL_PRIMITIVES_H__
//-----------------------------------------------------------------------------
#        include "rlc_um_entity.h"
#        include "mem_block.h"
#        include "rlc_um_structs.h"
#        include "rlc_um_constants.h"
#        include "platform_types.h"
#        include "LTE_DL-UM-RLC.h"
#        include "LTE_UL-UM-RLC.h"
//-----------------------------------------------------------------------------

typedef volatile struct {
  uint32_t             is_uplink_downlink;
  uint32_t             timer_reordering;
  uint32_t             sn_field_length; // 5 or 10
  uint32_t             is_mXch; // boolean, true if configured for MTCH or MCCH
} rlc_um_info_t;


/*! \fn void config_req_rlc_um (
 * const protocol_ctxt_t* const ctxt_pP,
 * const srb_flag_t srb_flagP,
 * const rlc_um_info_t * const config_umP,
 * const rb_id_t rb_idP)
* \brief    Allocate memory for RLC UM instance, reset protocol variables, and set protocol parameters. After this configuration the RLC UM protocol instance will be in RLC_DATA_TRANSFER_READY_STATE state.
* \param[in]  ctxt_pP                   Running context.
* \param[in]  srb_flagP                 Flag to indicate signalling radio bearer (1) or data radio bearer (0).
* \param[in]  config_umP                Configuration parameters for RLC UM instance.
* \param[in]  rb_idP                    Radio bearer identifier.
* \param[in]  chan_idP                  Transport channel identifier.
*/
void config_req_rlc_um (
                                      const protocol_ctxt_t* const ctxt_pP,
                                      const srb_flag_t srb_flagP,
                                      const rlc_um_info_t * const config_umP,
                                      const rb_id_t rb_idP,
                                      const logical_chan_id_t chan_idP); 

/*! \fn void config_req_rlc_um_asn1 (
 *     const protocol_ctxt_t* const ctxt_pP,
 *     const srb_flag_t srb_flagP,
 *     const MBMS_flag_t mbms_flagP,
 *     const mbms_session_id_t  mbms_session_idP,
 *     const mbms_service_id_t  mbms_service_idP,
 *     const UL_UM_RLC_t* const ul_rlc_pP,
 *     const DL_UM_RLC_t* const dl_rlc_pP,
 *     const rb_id_t rb_idP)
* \brief    Allocate memory for RLC UM instance, reset protocol variables, and set protocol parameters. After this configuration the RLC UM protocol instance will be in RLC_DATA_TRANSFER_READY_STATE state.
* \param[in]  ctxt_pP                   Running context.
* \param[in]  srb_flagP                 Flag to indicate signalling radio bearer (1) or data radio bearer (0).
* \param[in]  mbms_flagP                Flag to indicate if this RLC is configured for MBMS.
* \param[in]  mbms_session_idP          MBMS session id.
* \param[in]  mbms_service_idP          MBMS service id.
* \param[in]  ul_rlc_pP                 Configuration parameters for RLC UM UL instance.
* \param[in]  dl_rlc_pP                 Configuration parameters for RLC UM DL instance.
* \param[in]  rb_idP                    Radio bearer identifier.
* \param[in]  chan_idP                  Transport channel identifier.
*/
void config_req_rlc_um_asn1 (
                                      const protocol_ctxt_t* const ctxt_pP,
                                      const srb_flag_t         srb_flagP,
                                      const MBMS_flag_t        mbms_flagP,
                                      const mbms_session_id_t  mbms_session_idP,
                                      const mbms_service_id_t  mbms_service_idP,
                                      const LTE_UL_UM_RLC_t  * const ul_rlc_pP,
                                      const LTE_DL_UM_RLC_t  * const dl_rlc_pP,
                                      const rb_id_t            rb_idP,
                                      const logical_chan_id_t  chan_idP
#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))
                                      ,const uint32_t          sourceL2Id
                                      ,const uint32_t          destinationL2Id
#endif
                                      );

/*! \fn void rlc_um_init (const protocol_ctxt_t* const ctxt_pP, rlc_um_entity_t * const rlc_pP)
* \brief    Initialize a RLC UM protocol instance, initialize all variables, lists, allocate buffers for making this instance ready to be configured with protocol configuration parameters. After this initialization the RLC UM protocol instance will be in RLC_NULL_STATE state.
* \param[in]  ctxtP                     Running context.
* \param[in]  rlc_pP                    RLC UM protocol instance pointer.
*/
void rlc_um_init (
                                      const protocol_ctxt_t* const ctxt_pP,
                                      rlc_um_entity_t * const rlc_pP);

/*! \fn void rlc_um_reset_state_variables (const protocol_ctxt_t* const ctxt_pP, rlc_um_entity_t * const rlc_pP)
* \brief    Reset protocol variables and state variables to initial values.
* \param[in]  ctxtP                     Running context.
* \param[in]  rlc_pP                    RLC UM protocol instance pointer.
*/
void rlc_um_reset_state_variables (
                                      const protocol_ctxt_t* const ctxt_pP,
                                      rlc_um_entity_t * const rlc_pP);

/*! \fn void rlc_um_cleanup(rlc_um_entity_t * const rlc_pP)
* \brief    Free all allocated memory (lists and buffers) previously allocated by this RLC UM instance.
* \param[in]  rlc_pP                    RLC UM protocol instance pointer.
*/
void rlc_um_cleanup(
                                      rlc_um_entity_t * const rlc_pP);

/*! \fn void rlc_um_configure(
 *     const protocol_ctxt_t* const ctxt_pP,
 *     rlc_um_entity_t * const rlc_pP,
 *     const uint32_t timer_reorderingP,
 *     const uint32_t rx_sn_field_lengthP,
 *     const uint32_t tx_sn_field_lengthP,
 *     const uint32_t is_mXchP)
* \brief    Configure RLC UM protocol parameters.
* \param[in]  ctxtP                     Running context.
* \param[in]  rlc_pP                    RLC UM protocol instance pointer.
* \param[in]  timer_reorderingP         t-Reordering timer initialization value, units in frameP.
* \param[in]  rx_sn_field_lengthP       Length of the sequence number, 5 or 10 bits in reception.
* \param[in]  tx_sn_field_lengthP       Length of the sequence number, 5 or 10 bits in transmission.
* \param[in]  is_mXchP                  Is the radio bearer for MCCH, MTCH.
*/
void rlc_um_configure(
                                      const protocol_ctxt_t* const ctxt_pP,
                                      rlc_um_entity_t *const  rlc_pP,
                                      const uint32_t timer_reorderingP,
                                      const uint32_t rx_sn_field_lengthP,
                                      const uint32_t tx_sn_field_lengthP,
                                      const uint32_t is_mXchP);

/*! \fn void rlc_um_set_debug_infos(
*     const protocol_ctxt_t* const ctxt_pP,
*     rlc_um_entity_t * const rlc_pP,
*     const srb_flag_t  srb_flagP,
*     const rb_id_t     rb_idP)
* \brief    Set debug informations for a RLC UM protocol instance, these informations are only for trace purpose.
* \param[in]  ctxtP                     Running context.
* \param[in]  rlc_pP                    RLC UM protocol instance pointer.
* \param[in]  srb_flagP                 Flag to indicate signalling radio bearer (1) or data radio bearer (0).
* \param[in]  rb_idP                    Radio bearer identifier.
* \param[in]  chan_idP                  Transport channel identifier.
*/
void rlc_um_set_debug_infos(
                                      const protocol_ctxt_t* const ctxt_pP,
                                      rlc_um_entity_t * const rlc_pP,
                                      const srb_flag_t srb_flagP,
                                      const rb_id_t rb_idP,
                                      const logical_chan_id_t chan_idP); 
/** @} */
#    endif
