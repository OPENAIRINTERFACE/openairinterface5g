/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.0  (the "License"); you may not use this file
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

/*! \file rlc_am_constants.h
* \brief This file defines constant values used in RLC AM.
* \author GAUTHIER Lionel
* \date 2010-2011
* \version
* \note
* \bug
* \warning
*/
/**
* @addtogroup _rlc_am_internal_impl_
* @{
*/
#ifndef __RLC_AM_CONSTANT_H__
#    define __RLC_AM_CONSTANT_H__

/** The sequence numbering modulo (10 bits). */
#    define RLC_AM_SN_MODULO                      1024

/** The sequence numbering binary mask (10 bits). */
#    define RLC_AM_SN_MASK                        0x3FF

/** FROM Spec: This constant is used by both the transmitting side and the receiving side of each AM RLC entity to calculate VT(MS) from VT(A), and VR(MR) from VR(R). AM_Window_Size = 512.. */
#    define RLC_AM_WINDOW_SIZE                    512

/** Max number of bytes of incoming SDUs from upper layer that can be buffered in a RLC AM protocol instance. */
#    define RLC_AM_SDU_DATA_BUFFER_SIZE           64*1024

/** Max number of incoming SDUs from upper layer that can be buffered in a RLC AM protocol instance. */
#    define RLC_AM_SDU_CONTROL_BUFFER_SIZE        128

/** Size of the retransmission buffer (number of PDUs). */
#    define RLC_AM_PDU_RETRANSMISSION_BUFFER_SIZE RLC_AM_WINDOW_SIZE

/** PDU minimal header size in bytes. */
#    define RLC_AM_HEADER_MIN_SIZE                2

/** If we want to send a segment of a PDU, then the min transport block size requested by MAC should be this amount. */
#    define RLC_AM_MIN_SEGMENT_SIZE_REQUEST       8

/** Max SDUs that can fit in a PDU. */
#    define RLC_AM_MAX_SDU_IN_PDU                 32

/** Max fragments for a SDU. */
#    define RLC_AM_MAX_SDU_FRAGMENTS              32

/** Max Negative Acknowledgment SN (NACK_SN) fields in a STATUS PDU. */
#    define RLC_AM_MAX_NACK_IN_STATUS_PDU         1023

/** Max holes created by NACK_SN with segment offsets for a PDU in the retransmission buffer. */
#    define RLC_AM_MAX_HOLES_REPORT_PER_PDU       16
/** @} */

/* Common to Data and Status PDU */
#define RLC_AM_SN_BITS                         10
#define RLC_AM_PDU_D_C_BITS                	   1
#define RLC_AM_PDU_E_BITS                	   1

/* STATUS PDU */
#define RLC_AM_STATUS_PDU_CPT_STATUS           0

#define RLC_AM_STATUS_PDU_CPT_OFFSET           4
#define RLC_AM_STATUS_PDU_CPT_LENGTH           3

#define RLC_AM_STATUS_PDU_ACK_SN_OFFSET        2

#define RLC_AM_STATUS_PDU_SO_LENGTH            15

#define RLC_AM_STATUS_PDU_SO_END_ALL_BYTES     0x7FFF

/* MACRO DEFINITIONS */
#define RLC_AM_NEXT_SN(sn)         (((sn)+1) & ((RLC_AM_SN_MODULO)-1))
#define RLC_AM_PREV_SN(sn)         (((sn)+(RLC_AM_SN_MODULO)-1) & ((RLC_AM_SN_MODULO)-1))

#define RLC_DIFF_SN(sn,snref,modulus)  ((sn+(modulus)-snref) & ((modulus)-1))
#define RLC_SN_IN_WINDOW(sn,snref,modulus)  ((RLC_DIFF_SN(sn,snref,modulus)) < ((modulus) >> 1))

#define RLC_AM_DIFF_SN(sn,snref)       (RLC_DIFF_SN(sn,snref,RLC_AM_SN_MODULO))
#define RLC_AM_SN_IN_WINDOW(sn,snref)  (RLC_SN_IN_WINDOW(sn,snref,RLC_AM_SN_MODULO))

#define RLC_SET_EVENT(x,event)         ((x) |= (event))
#define RLC_GET_EVENT(x,event)         ((x) & (event))
#define RLC_CLEAR_EVENT(x,event)       ((x) &= (~(event)))

/* Uplink STATUS PDU trigger events */
#define RLC_AM_STATUS_NOT_TRIGGERED               0
#define RLC_AM_STATUS_TRIGGERED_POLL              0x01    /* Status Report is triggered by a received poll */
#define RLC_AM_STATUS_TRIGGERED_T_REORDERING      0x02    /* Status Report is triggered by Timer Reordering Expiry */
#define RLC_AM_STATUS_TRIGGERED_DELAYED           0x10    /* Status is delayed until SN(receivedPoll) < VR(MS) */
#define RLC_AM_STATUS_PROHIBIT                    0x20    /* TimerStatusProhibit still running */
#define RLC_AM_STATUS_NO_TX_MASK				  (RLC_AM_STATUS_PROHIBIT | RLC_AM_STATUS_TRIGGERED_DELAYED)

/* Status triggered (bit 5-7) will be concatenated with Poll triggered (bit 0-4) for RLCdec. RLC_AM_STATUS_TRIGGERED_DELAYED is not recorded. */
#define RLC_AM_SET_STATUS(x,event)     (RLC_SET_EVENT(x,event))
#define RLC_AM_GET_STATUS(x,event)     (RLC_GET_EVENT(x,event))
#define RLC_AM_CLEAR_STATUS(x,event)   (RLC_CLEAR_EVENT(x,event))
#define RLC_AM_CLEAR_ALL_STATUS(x)     ((x) = (RLC_AM_STATUS_NOT_TRIGGERED))


#endif
