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

/*! \file rlc_tm_init.h
* \brief This file defines the prototypes of the functions dealing with the control primitives and initialization.
* \author GAUTHIER Lionel
* \date 2010-2011
* \version
* \note
* \bug
* \warning
*/
/** @defgroup _rlc_tm_init_impl_ RLC TM Init Reference Implementation
* @ingroup _rlc_tm_impl_
* @{
*/
#    ifndef __RLC_TM_INIT_H__
#        define __RLC_TM_INIT_H__
//-----------------------------------------------------------------------------
#        include "rlc_tm_entity.h"
#        include "mem_block.h"
//#        include "rrm_config_structs.h"
#        include "rlc_tm_structs.h"
//#        include "rlc.h"
#        include "platform_types.h"
//-----------------------------------------------------------------------------

typedef volatile struct rlc_tm_info_s {
  uint8_t             is_uplink_downlink;
} rlc_tm_info_t;


/*! \fn void config_req_rlc_tm (void config_req_rlc_tm (
    const protocol_ctxt_t* const  ctxt_pP,
    const srb_flag_t  srb_flagP,
    const rlc_tm_info_t * const config_tmP,
    const rb_id_t     rb_idP,
    const srb_flag_t  srb_flagP)
* \brief    Allocate memory for RLC TM instance, reset protocol variables, and set protocol parameters.
* \param[in]  srb_flagP                 Flag to indicate SRB (1) or DRB (0)
* \param[in]  config_tmP                Configuration parameters for RLC TM instance.
* \param[in]  rb_idP                    Radio bearer identifier.
* \param[in]  chan_idP                  Transport channel identifier.
*/
void config_req_rlc_tm (
                        const protocol_ctxt_t* const  ctxt_pP,
                        const srb_flag_t  srb_flagP,
                        const rlc_tm_info_t * const config_tmP,
                        const rb_id_t     rb_idP,
                        const logical_chan_id_t chan_idP); 

/*! \fn void rlc_tm_init (const protocol_ctxt_t* const  ctxt_pP, rlc_tm_entity_t * const rlcP)
* \brief    Initialize a RLC TM protocol instance, initialize all variables, lists, allocate buffers for making this instance ready to be configured with protocol configuration parameters. After this initialization the RLC TM protocol instance will be in RLC_NULL_STATE state.
* \param[in]  rlcP                      RLC TM protocol instance pointer.
*/
void rlc_tm_init (
                        const protocol_ctxt_t* const  ctxt_pP,
                        rlc_tm_entity_t * const rlcP);

/*! \fn void rlc_tm_reset_state_variables (const protocol_ctxt_t* const  ctxt_pP, rlc_tm_entity_t * const rlcP)
* \brief    Reset protocol variables and state variables to initial values.
* \param[in]  rlcP                      RLC TM protocol instance pointer.
*/
void rlc_tm_reset_state_variables (
                        const protocol_ctxt_t* const  ctxt_pP,
                        rlc_tm_entity_t * const rlcP);

/*! \fn void rlc_tm_cleanup(rlc_tm_entity_t * const rlcP)
* \brief    Free all allocated memory (lists and buffers) previously allocated by this RLC TM instance.
* \param[in]  rlcP                      RLC TM protocol instance pointer.
*/
void rlc_tm_cleanup(
                        rlc_tm_entity_t * const rlcP);

/*! \fn void rlc_tm_configure(const protocol_ctxt_t* const  ctxt_pP,rlc_tm_entity_t * const rlcP, const boolean_t is_uplink_downlinkP)
* \brief    Configure RLC TM protocol parameters.
* \param[in]  rlcP                      RLC TM protocol instance pointer.
* \param[in]  is_uplink_downlinkP       Is this instance is TRANSMITTER_ONLY,
RECEIVER_ONLY, or TRANSMITTER_AND_RECEIVER.
*/
void rlc_tm_configure(
                        const protocol_ctxt_t* const  ctxt_pP,
                        rlc_tm_entity_t * const rlcP,
                        const boolean_t is_uplink_downlinkP);

/*! \fn void rlc_tm_set_debug_infos(const protocol_ctxt_t* const  ctxt_pP, rlc_tm_entity_t * const rlcP, const rb_id_t rb_idP, const srb_flag_t srb_flagP)
* \brief    Set debug informations for a RLC TM protocol instance, these informations are only for trace purpose.
* \param[in]  rlcP                      RLC TM protocol instance pointer.
* \param[in]  srb_flagP                 Flag to indicate signalling radio bearer (1) or data radio bearer (0).
* \param[in]  rb_idP                    Radio bearer identifier.
* \param[in]  chan_idP                  Transport channel identifier
*/
void rlc_tm_set_debug_infos(
                        const protocol_ctxt_t* const  ctxt_pP,
                        rlc_tm_entity_t * const       rlcP,
                        const srb_flag_t              srb_flagP,
                        const rb_id_t                 rb_idP,
                        const logical_chan_id_t chan_idP);

/** @} */
#    endif
