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

/*! \file rlc_am_init.h
* \brief This file defines the prototypes of the functions initializing a RLC AM protocol instance.
* \author GAUTHIER Lionel
* \date 2010-2011
* \version
* \note
* \bug
* \warning
*/
/** @defgroup _rlc_am_init_impl_ RLC AM Init Reference Implementation
* @ingroup _rlc_am_impl_
* @{
*/
#ifndef __RLC_AM_INIT_H__
#    define __RLC_AM_INIT_H__

#    include "UTIL/MEM/mem_block.h"
//-----------------------------------------------------------------------------
#        ifdef RLC_AM_INIT_C
#            define private_rlc_am_init(x)    x
#            define protected_rlc_am_init(x)  x
#            define public_rlc_am_init(x)     x
#        else
#            ifdef RLC_AM_MODULE
#                define private_rlc_am_init(x)
#                define protected_rlc_am_init(x)  extern x
#                define public_rlc_am_init(x)     extern x
#            else
#                define private_rlc_am_init(x)
#                define protected_rlc_am_init(x)
#                define public_rlc_am_init(x)     extern x
#            endif
#        endif
//-----------------------------------------------------------------------------
#include "platform_types.h"
#include "platform_constants.h"
#include "PHY/defs.h"


/*! \struct  rlc_am_info_t
* \brief Structure containing RLC AM configuration parameters.
*/
typedef volatile struct {
  uint16_t max_retx_threshold;  /*!< \brief Maximum number of retransmissions for one RLC AM PDU. */
  uint16_t poll_pdu;            /*!< \brief Generate a status each poll_pdu pdu sent. */
  uint16_t poll_byte;           /*!< \brief Generate a status each time poll_byte bytes have been sent. */
  uint32_t t_poll_retransmit;   /*!< \brief t-PollRetransmit timer initial value. */
  uint32_t t_reordering;        /*!< \brief t-Reordering timer initial value. */
  uint32_t t_status_prohibit;   /*!< \brief t-StatusProhibit timer initial value. */
} rlc_am_info_t;

//-----------------------------------------------------------------------------
/*! \fn void rlc_am_init   (const protocol_ctxt_t* const ctxtP, rlc_am_entity_t * const rlc_pP)
* \brief    Initialize the RLC AM protocol instance, reset variables, allocate buffers, lists, then, the next step in order have a running RLC AM instance is to configure and set debug informations for this RLC instance.
* \param[in]  ctxtP                     Running context.
* \param[in]  rlc_pP                    RLC AM protocol instance pointer.
*/
public_rlc_am_init( void rlc_am_init   (const protocol_ctxt_t* const ctxtP, rlc_am_entity_t* rlc_pP);)

/*! \fn void rlc_am_cleanup(rlc_am_entity_t *const rlc_pP)
* \brief    Free all memory resources allocated and kept by this RLC AM instance.
* \param[in]  rlc_pP                    RLC AM protocol instance pointer.
*/
public_rlc_am_init( void rlc_am_cleanup(rlc_am_entity_t* rlc_pP);)

/*! \fn void rlc_am_configure(const protocol_ctxt_t* const ctxtP, rlc_am_entity_t * const rlc_pP,  uint16_t max_retx_thresholdP, uint16_t poll_pduP, uint16_t poll_byteP, uint32_t t_poll_retransmitP, uint32_t t_reorderingP, uint32_t t_status_prohibitP)
* \brief    Set RLC AM protocol parameters.
* \param[in]  ctxtP                     Running context.
* \param[in]  rlc_pP                    RLC AM protocol instance pointer.
* \param[in]  max_retx_thresholdP       Limit the number of retransmissions of an
AMD PDU.
* \param[in]  poll_pduP                 Trigger a poll for every poll_pduP PDUs.
* \param[in]  poll_byteP                Trigger a poll for every pollByte bytes.
* \param[in]  t_poll_retransmitP        This timer is used by the transmitting side of an AM RLC entity in order to retransmit a poll, value in frames.
* \param[in]  t_reorderingP             This timer is used by the receiving side of an AM RLC entity in order to detect loss of RLC PDUs at lower layer, value in frames.
* \param[in]  t_status_prohibitP        This timer is used by the receiving side of an AM RLC entity in order to prohibit transmission of a STATUS PDU, value in frames.
*/
public_rlc_am_init( void rlc_am_configure(const protocol_ctxt_t* const ctxtP,
                    rlc_am_entity_t * const rlc_pP,
                    const uint16_t max_retx_thresholdP,
                    const uint16_t poll_pduP,
                    const uint16_t poll_byteP,
                    const uint32_t t_poll_retransmitP,
                    const uint32_t t_reorderingP,
                    const uint32_t t_status_prohibitP);)


/*! \fn void rlc_am_set_debug_infos(const protocol_ctxt_t* const ctxtP, rlc_am_entity_t * const rlc_pP, const srb_flag_t srb_flagP, const rb_id_t rb_idP)
* \brief    Set informations that will be displayed in traces, helping the debug process.
* \param[in]  ctxtP                     Running context.
* \param[in]  rlc_pP                    RLC AM protocol instance pointer.
* \param[in]  srb_flagP                 Flag to indicate signalling radio bearer (1) or data radio bearer (0).
* \param[in]  rb_idP                    Radio bearer identifier.
* \param[in]  chan_idP                  Transport channel identifier.
*/
public_rlc_am_init( void rlc_am_set_debug_infos(const protocol_ctxt_t* const ctxtP, rlc_am_entity_t * const rlc_pP, const srb_flag_t srb_flagP, const rb_id_t rb_idP, const logical_chan_id_t chan_idP);)
/** @} */
#endif
