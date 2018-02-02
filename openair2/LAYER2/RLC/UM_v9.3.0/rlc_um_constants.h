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

/*! \file rlc_um_constants.h
* \brief This file defines constant values used inside RLC UM.
* \author GAUTHIER Lionel
* \date 2010-2011
* \version
* \note
* \bug
* \warning
*/
/**
* @addtogroup _rlc_um_internal_impl_
* @{
*/
#    ifndef __RLC_UM_CONSTANTS_H__
#        define __RLC_UM_CONSTANTS_H__

/** The sequence numbering modulo (10 bits). */
#        define RLC_UM_SN_10_BITS_MODULO                    1024
/** The sequence numbering binary mask (10 bits). */
#        define RLC_UM_SN_10_BITS_MASK                      0x03FF
/** UM_Window_Size = 512 when a 10 bit SN is configured. */
#        define RLC_UM_WINDOW_SIZE_SN_10_BITS               512
/** The sequence numbering modulo (5 bits). */
#        define RLC_UM_SN_5_BITS_MODULO                     32
/** The sequence numbering binary mask (5 bits). */
#        define RLC_UM_SN_5_BITS_MASK                       0x1F
/** UM_Window_Size = 16 when a 5 bit SN is configured. */
#        define RLC_UM_WINDOW_SIZE_SN_5_BITS                16
// This constant is used by the receiving UM RLC entity to define SNs of those
// UMD PDUs that can be received without causing an advancement of the
// receiving window. UM_Window_Size = 16 when a 5 bit SN is configured,
// UM_Window_Size = 512 when a 10 bit SN is configured and UM_Window_Size = 0
// when the receiving UM RLC entity is configured for MCCH or MTCH.
// li field (values shifted 1 bit left)
#        define RLC_UM_SEGMENT_NB_MAX_LI_PER_PDU            128
//----------------------------------------------------------
// Events defined for state model of the acknowledged mode entity
/** Internal event defined for state model of the RLC UM. */
#        define RLC_UM_RECEIVE_CRLC_CONFIG_REQ_ENTER_NULL_STATE_EVENT                 0x00

/** Internal event defined for state model of the RLC UM. */
#        define RLC_UM_RECEIVE_CRLC_CONFIG_REQ_ENTER_DATA_TRANSFER_READY_STATE_EVENT  0x01

/** Internal event defined for state model of the RLC UM. */
#        define RLC_UM_RECEIVE_CRLC_SUSPEND_REQ_EVENT                                 0x10

/** Internal event defined for state model of the RLC UM. */
#        define RLC_UM_TRANSMIT_CRLC_SUSPEND_CNF_EVENT                                0x11

/** Internal event defined for state model of the RLC UM. */
#        define RLC_UM_RECEIVE_CRLC_RESUME_REQ_EVENT                                  0x12

/** Marker in RX buffer meaning that a PDU has been received. */
#        define RLC_UM_PDU_PREVIOUSLY_RECEIVED 0x01
/** @} */
#    endif
