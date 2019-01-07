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

/*! \file rlc_am_structs.h
* \brief This file defines structures used inside the RLC AM.
* \author GAUTHIER Lionel
* \date 2010-2011
* \version
* \note
* \bug
* \warning
*/
#    ifndef __RLC_AM_STRUCTS_H__
#        define __RLC_AM_STRUCTS_H__

#        include "platform_types.h"
#        include "platform_constants.h"
#        include "list.h"
#        include "mem_block.h"
#        include "rlc_am_constants.h"
#        include "mac_rlc_primitives.h"
#        include "mac_primitives.h"
#        include "rlc_primitives.h"
/**
* @addtogroup _rlc_am_internal_input_sdu_impl_
* @{
*/
//-----------------------------------------------------------------------------
// SDU MANAGEMENT
//-----------------------------------------------------------------------------
/*! \struct  sdu_management_flags_t
* \brief Structure containing SDU bit-field flags (for memory optimization purpose) related to its segmentation and transmission.
*/
typedef struct sdu_management_flags {
  uint8_t discarded:1;                          /*!< \brief Boolean telling if this SDU is discarded, actually unused. */
  uint8_t segmented:1;                          /*!< \brief Boolean telling if the segmentation of this SDU is completed, actually unused. */
  uint8_t segmentation_in_progress:1;           /*!< \brief Boolean telling if the segmentation of this SDU is started but not completed, actually unused. */
  uint8_t no_new_sdu_segmented_in_last_pdu:1;   /*!< \brief Actually unused. */
  uint8_t transmitted_successfully:1;           /*!< \brief SDU acknowledged by peer RLC AM. */
  uint8_t dummy:3;                              /*!< \brief Free bits. */
} sdu_management_flags_t;

/*! \struct  rlc_am_tx_sdu_management_t
* \brief Structure containing SDU variables related to its segmentation and transmission.
*/
typedef struct rlc_am_tx_sdu_management {
  mem_block_t            *mem_block;          /*!< \brief SDU embedded in a mem_block_t. */
  uint8_t                   *first_byte;         /*!< \brief Pointer on SDU payload. */
  frame_t                 sdu_creation_time;  /*!< \brief Time stamped with mac_xface->frame. */
  mui_t                   mui;                /*!< \brief Message Unit Identifier, still here for historical reasons (no LTE-PDCP compliant now). */
  sdu_size_t              sdu_remaining_size; /*!< \brief Remaining size in bytes to be filled in a PDU. */
  sdu_size_t              sdu_segmented_size; /*!< \brief Bytes already segmented in a/several PDU(s). */
  sdu_size_t              sdu_size;           /*!< \brief SDU size in bytes. */
  int16_t                   pdus_index[RLC_AM_MAX_SDU_FRAGMENTS];  /*!< \brief Array containing the sequence numbers of the PDU in which the SDU has been segmented. */
  uint8_t                    nb_pdus;  /*!< \brief Number of referenced PDUs in pdus_index[] array. */
  uint8_t                    nb_pdus_ack;   /*!< \brief Number of referenced PDUs in pdus_index[] array that have been acknowledged. (used for confirmation and MaxDat discard)*/
  sdu_management_flags_t  flags;        /*!< \brief bit-field flags related to SDU segmentation and transmission */
} rlc_am_tx_sdu_management_t;

/** @} */

/**
* @addtogroup _rlc_am_internal_retransmit_impl_
* @{
*/

/*! \struct  pdu_management_flags_t
* \brief Structure containing PDU variables related to its retransmission.
*/
typedef struct pdu_management_flags {
  uint8_t transmitted:1;            /*!< \brief Boolean telling that this PDU is not empty and has been at least transmitted once. */
  uint8_t ack:1;            /*!< \brief Boolean telling that this PDU has been acknowledged. */
  uint8_t nack:1;            /*!< \brief Boolean telling that this PDU has been acknowledged negatively. */
  uint8_t retransmit:1;       /*!< \brief Boolean telling a retransmission is scheduled for this PDU. */
  uint8_t max_retransmit:1;       /*!< \brief Boolean telling max retransmission has been hit for this PDU. */
  uint8_t dummy:3;            /*!< \brief Free bits. */
} pdu_management_flags_t;



/*! \struct  rlc_am_tx_data_pdu_management_t
* \brief Structure containing PDU variables related to its acknowlegment, non acknowlegment, resegmentation and retransmission.
*/
typedef struct rlc_am_tx_data_pdu_management {
  mem_block_t     *mem_block;                          /*!< \brief PDU embedded in a mem_block_t struct  */
  uint8_t         *first_byte;                        /*!< \brief Pointer on the PDU  including header, LIs, data */
  uint8_t         *payload;                           /*!< \brief Pointer on the PDU  payload */
  int16_t          sdus_index[RLC_AM_MAX_SDU_IN_PDU]; /*!< \brief Index of SDU(s) having segments in this pdu (index in rlc_am_entity.input_sdus[]) */
  frame_t          last_nack_time;                    /*!< \brief Last frame this PDU was negative acknowledged, for not nacking several times in the same frame */
  sdu_size_t       hole_so_start  [RLC_AM_MAX_HOLES_REPORT_PER_PDU]; /*!< \brief Array containing the start segment offsets for marking a hole (negative acknowledged area) in the PDU. */
  sdu_size_t       hole_so_stop   [RLC_AM_MAX_HOLES_REPORT_PER_PDU]; /*!< \brief Array containing the stop segment offsets for marking a hole (negative acknowledged area) in the PDU. */
  uint8_t          num_holes;                         /*!< \brief Number of registereg holes in hole_so_start[], hole_so_stop[]. */
  uint8_t          retx_hole_index;                   /*!< \brief Next index of registered holes to retransmit. */
  sdu_size_t       header_and_payload_size;           /*!< \brief Size of the PDU in bytes, including header and payload. */
  sdu_size_t       payload_size;                      /*!< \brief Size of the PDU payload in bytes. */
  sdu_size_t	   retx_payload_size;				  /*!< \brief Size of the PDU payload to be retransmitted in bytes including all Segment portions. */
  rlc_sn_t         sn;                                /*!< \brief Sequence number of the PDU. */
  sdu_size_t       nack_so_start; /*!< \brief Lowest NACK start segment offset, must be set to 0 if global NACK. */
  sdu_size_t       nack_so_stop;  /*!< \brief Highest NACK stop segment offset, must be set to data_size - 1 if global NACK */

  int8_t           nb_sdus;       /*!< \brief Number of sdu having segments in this pdu. */
  int8_t           retx_count;    /*!< \brief Counts the number of already occurred retransmissions of an AMD PDU (see subclause 5.2.1). */
  int8_t           retx_count_next;    /*!< \brief Counts the number of already occurred retransmissions plus the latest pending one. */

  pdu_management_flags_t  flags; /*!< \brief PDU variables related to its retransmission. */
} rlc_am_tx_data_pdu_management_t;

/** @} */

/**
* @addtogroup _rlc_am_status_report_impl_
* @{
*/

/*! \struct  nack_sn_t
* \brief Structure containing Status PDU information element.
*/
typedef struct nack_sn {
  uint16_t nack_sn:10;  /*!< \brief NACK sequence number field. */
  uint16_t e1:1;        /*!< \brief Extension bit 1 field. */
  uint16_t e2:1;        /*!< \brief Extension bit 2 field. */
  uint32_t so_start:15; /*!< \brief Optional SOstart field (depending on e2 bit). */
  uint32_t so_end:15;   /*!< \brief Optional SOend field (depending on e2 bit). */
} nack_sn_t;

/*! \struct  rlc_am_control_pdu_info_t
* \brief Structure containing Status PDU information element.
*/
typedef struct rlc_am_control_pdu_info {
  uint16_t      d_c:1;       /*!< \brief Data/Control field. */
  uint16_t      cpt:3;       /*!< \brief Control PDU type. */
  uint16_t      ack_sn:10;   /*!< \brief Acknowledgment SN field. */
  uint16_t      e1:1;        /*!< \brief Extension bit 1 field. */
  uint16_t      dummy:2;     /*!< \brief Unused bits. */
  uint16_t      num_nack;    /*!< \brief Number of nack_sn_t fields in nack_list[]. */
  nack_sn_t  nack_list[RLC_AM_MAX_NACK_IN_STATUS_PDU]; /*!< \brief List of NACK_SN fields. */
} rlc_am_control_pdu_info_t ;

/*! \struct  rlc_am_e_li_t
* \brief Structure helping coding and decoding LI and e bits in AMD PDUs and AMD PDU segments.
*/
typedef struct rlc_am_e_li {
  uint8_t  b1; /*!< \brief 1st byte. */
  uint8_t  b2; /*!< \brief 2nd byte. */
  uint8_t  b3; /*!< \brief 3rd byte. */
} rlc_am_e_li_t;

/** @} */

/**
* @addtogroup _rlc_am_internal_impl_
* @{
*/
/*! \struct  rlc_am_pdu_sn_10_t
* \brief Structure helping coding and decoding the first 2 bytes of a AMD PDU or AMD PDU segment.
*/
typedef struct rlc_am_pdu_sn_10 {
  uint8_t  b1;     /*!< \brief 1st byte. */
  uint8_t  b2;     /*!< \brief 2nd byte. */
  uint8_t  data[2];/*!< \brief Following bytes. */
} __attribute__((__packed__)) rlc_am_pdu_sn_10_t ;
/** @} */


/**
* @addtogroup _rlc_am_internal_receiver_impl_
* @{
*/
/*! \struct  rlc_am_pdu_info_t
* \brief Structure for storing decoded informations from the header of a AMD PDU or AMD PDU segment.
*/
typedef struct rlc_am_pdu_info {
  uint32_t  d_c:1;  /*!< \brief Data/Control field. */
  uint32_t  rf:1;   /*!< \brief Resegmentation Flag field. */
  uint32_t  p:1;    /*!< \brief Polling bit field. */
  uint32_t  fi:2;   /*!< \brief Framing Info field. */
  uint32_t  e:1;    /*!< \brief Extension bit field. */
  uint32_t  sn:10;  /*!< \brief Sequence Number field. */
  uint32_t  lsf:1;  /*!< \brief Last Segment Flag field. */
  uint32_t  so:15;  /*!< \brief Segment Offset field. */
  uint16_t  num_li; /*!< \brief Number of Length Indicators. */
  int16_t  li_list[RLC_AM_MAX_SDU_IN_PDU]; /*!< \brief List of Length Indicators. */
  int16_t  hidden_size  /*!< \brief Part of payload size in bytes that is not included in the sum of LI fields. */;
  uint8_t*  payload;     /*!< \brief Pointer on PDU payload. */
  int16_t  payload_size;/*!< \brief Size of payload in bytes. */
  int16_t  header_size; /*!< \brief Size of header in bytes (including SO field and LI fields). */
} rlc_am_pdu_info_t ;
/** @} */


/**
* @addtogroup _rlc_am_timers_impl_
* @{
*/
/*! \struct  rlc_am_timer_t
* \brief Generic structure for implementing a timer.
*/
typedef struct rlc_am_timer {
  uint32_t    ms_time_out;   /*!< \brief When set, indicate the time in milliseconds the timer will time-out. */
  uint32_t    ms_start;      /*!< \brief indicate the time in milliseconds the timer has been started. */
  uint32_t    ms_duration;   /*!< \brief Configured timer duration in milliseconds. */
  uint32_t    running:1;     /*!< \brief The timer is running. */
  uint32_t    timed_out:1;   /*!< \brief The timer has timed-out. */
  uint32_t    dummy:30;      /*!< \brief Bits not used. */
} rlc_am_timer_t ;
/** @} */

//-----------------------------------------------------------------------------
// DATA PDU
//-----------------------------------------------------------------------------
/**
* @addtogroup _rlc_am_internal_receiver_impl_
* @{
*/

typedef enum rlc_am_rx_segment_reassemble_info
{
    /** No Reassembly scheduled */
    RLC_AM_RX_PDU_SEGMENT_REASSEMBLE_NO           = 0,
    /** Reassembly scheduled */
	RLC_AM_RX_PDU_SEGMENT_REASSEMBLE_PENDING      = 1,
    /** Reassembly done */
	RLC_AM_RX_PDU_SEGMENT_REASSEMBLED             = 2

} rlc_am_rx_segment_reassemble_info_t;

/*! \struct  rlc_am_rx_pdu_management_t
* \brief Structure for storing decoded informations from the header of a AMD PDU or AMD PDU segment and information on reassembly.
*/
typedef struct rlc_am_rx_pdu_management {
  rlc_am_pdu_info_t pdu_info; /*!< \brief Field for storing decoded informations from the header of a AMD PDU or AMD PDU segment. */
  uint8_t              all_segments_received; /*!< \brief Is all segments of PDU SN have been received. */
  rlc_am_rx_segment_reassemble_info_t			   segment_reassembled; /*!< \brief if the segment for SN=vrR is reassembled but not discarded yet. */
} rlc_am_rx_pdu_management_t;
/** @} */

typedef enum rlc_am_rx_pdu_status
{
    /** PDU okay. */
    RLC_AM_DATA_PDU_STATUS_OK                     = 0,
    /** SN outside RX window */
	RLC_AM_DATA_PDU_STATUS_SN_OUTSIDE_WINDOW      = 1,
    /** SN already available */
	RLC_AM_DATA_PDU_STATUS_SN_DUPLICATE           = 2,
    /** SN already available */
	RLC_AM_DATA_PDU_STATUS_AM_SEGMENT_DUPLICATE   = 3,
    /** Buffer full */
	RLC_AM_DATA_PDU_STATUS_BUFFER_FULL            = 4,
    /** Header Error (LI,SO...) */
	RLC_AM_DATA_PDU_STATUS_HEADER_ERROR           = 5,
    /** Unknown bearer */
	RLC_AM_DATA_PDU_STATUS_INVALID_BEARER         = 6,
    /** RLC in wrong state */
	RLC_AM_DATA_PDU_STATUS_WRONG_STATE            = 7

} rlc_am_rx_pdu_status_t;

/*! \cond PRIVATE */
//-----------------------------------------------------------------------------
//  interlayers optimizations
//-----------------------------------------------------------------------------
struct rlc_am_tx_data_pdu_allocation {
  union {
    struct rlc_am_tx_data_pdu_management rlc_am_tx_pdu_mngmnt;
    struct mac_tb_req tb_req;
    struct mac_tx_tb_management tb_mngt;
  } dummy;
};

/*struct rlc_am_tx_control_pdu_allocation {
  union {
    struct mac_tb_req tb_req;
    struct mac_tx_tb_management tb_mngt;
  } dummy;
};*/

struct rlc_am_data_req_alloc {  // alloc enought bytes for sdu mngt also
  union {
    struct rlc_am_data_req dummy1;
    struct rlc_am_tx_sdu_management dummy2;
  } dummy;
};
/*! \endcond */
#    endif
