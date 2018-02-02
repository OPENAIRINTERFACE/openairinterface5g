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

/*! \file rlc_um_entity.h
* \brief This file defines the RLC UM variables stored in a struct called rlc_um_entity_t.
* \author GAUTHIER Lionel
* \date 2010-2011
* \version
* \note The rlc_um_entity_t structure store protocol variables, statistic variables, allocation variables, buffers and other miscellaneous variables.
* \bug
* \warning
*/
#    ifndef __RLC_UM_ENTITY_H__
#        define __RLC_UM_ENTITY_H__

#        include <pthread.h>
#        include "platform_types.h"
#        include "platform_constants.h"
#        include "list.h"
#        include "rlc_primitives.h"
#        include "rlc_def.h"

typedef struct rlc_um_timer_s {
  uint32_t  ms_time_out;   /*!< \brief When set, indicate the time the timer will time-out. */
  uint32_t  ms_start;      /*!< \brief indicate the time the timer has been started. */
  uint32_t  ms_duration;   /*!< \brief Configured timer duration in frames. */
  uint32_t  running:1;     /*!< \brief The timer is running. */
  uint32_t  timed_out:1;   /*!< \brief The timer has timed-out. */
  uint32_t  dummy:30;      /*!< \brief Bits not used. */
} rlc_um_timer_t ;


/*! \struct  rlc_um_entity_s
* \brief Structure containing a RLC UM instance protocol variables, statistic variables, allocation variables, buffers and other miscellaneous variables.
*/

typedef struct rlc_um_entity_s {
  boolean_t            initialized;        /*!< \brief Boolean for rlc_am_entity_t struct initialization. */
  boolean_t            is_uplink_downlink; /*!< \brief Is this instance is a transmitter, a receiver or both? */
  rlc_protocol_state_t protocol_state;     /*!< \brief Protocol state, can be RLC_NULL_STATE, RLC_DATA_TRANSFER_READY_STATE, RLC_LOCAL_SUSPEND_STATE. */
  boolean_t            is_data_plane;      /*!< \brief To know if the RLC belongs to a data radio bearer or a signalling radio bearer, for statistics and trace purpose. */
  //boolean_t            is_enb;             /*!< \brief To know if the RLC belongs to a eNB or UE. */
  boolean_t            is_mxch;            /*!< \brief To know if the RLC belongs to a MBMS bearer. */
  //-----------------------------
  // PROTOCOL VARIABLES
  //-----------------------------
  rlc_usn_t
  vt_us; /*!< \brief This state variable holds the value of the SN to be assigned for the next newly generated UMD PDU. It is initially set to 0, and is updated whenever the UM RLC entity delivers an UMD PDU with SN = VT(US). */
  rlc_usn_t
  vr_ur; /*!< \brief UM receive state variable. This state variable holds the value of the SN of the earliest UMD PDU that is still considered for reordering. It is initially set to 0. */
  rlc_usn_t            vr_ux; /*!< \brief UM t-Reordering state variable. This state variable holds the value of the SN following the SN of the UMD PDU which triggered t-Reordering. */
  rlc_usn_t
  vr_uh; /*!< \brief UM highest received state variable. This state variable holds the value of the SN following the SN of the UMD PDU with the highest SN among received UMD PDUs, and it serves as the higher edge of the reordering window. It is initially set to 0. */
  //-----------------------------
  // TIMERS
  //-----------------------------
  struct rlc_um_timer_s  t_reordering;
  //*****************************************************************************
  // CONFIGURATION PARAMETERS
  //*****************************************************************************
  uint8_t              tx_sn_length;                     /*!< \brief Length of sequence number in bits, can be 5 or 10. */
  uint8_t              rx_sn_length;                     /*!< \brief Length of sequence number in bits, can be 5 or 10. */
  uint8_t              tx_header_min_length_in_bytes;    /*!< \brief Length of PDU header, can be 1 or 2 bytes. */
  uint8_t              rx_header_min_length_in_bytes;    /*!< \brief Length of PDU header, can be 1 or 2 bytes. */
  rlc_sn_t             tx_sn_modulo;                     /*!< \brief Module of the sequence number of PDU, can be RLC_UM_SN_5_BITS_MODULO or RLC_UM_SN_10_BITS_MODULO. */
  rlc_sn_t             rx_sn_modulo;                     /*!< \brief Module of the sequence number of PDU, can be RLC_UM_SN_5_BITS_MODULO or RLC_UM_SN_10_BITS_MODULO. */
  rlc_sn_t             rx_um_window_size;
  rlc_sn_t             tx_um_window_size;
  //-----------------------------
  // tranmission
  //-----------------------------
  // sdu communication;
  //pthread_spinlock_t lock_input_sdus;
  pthread_mutex_t      lock_input_sdus;
  list_t               input_sdus;                /*!< \brief Input SDU buffer (for SDUs coming from upper layers). Should be accessed as an array. */
  rlc_buffer_occupancy_t buffer_occupancy;          /*!< \brief Number of bytes contained in input_sdus buffer.*/
  uint32_t             nb_bytes_requested_by_mac; /*!< \brief Number of bytes requested by lower layer for next transmission. */
  list_t               pdus_to_mac_layer;         /*!< \brief PDUs buffered for transmission to MAC layer. */
  //*****************************************************************************
  // RECEIVER
  //*****************************************************************************
  mem_block_t      *output_sdu_in_construction;     /*!< \brief Memory area where a complete SDU is reassemblied before being send to upper layers. */
  sdu_size_t        output_sdu_size_to_write;       /*!< \brief Size of the reassemblied SDU. */


  //pthread_spinlock_t lock_dar_buffer;
  pthread_mutex_t      lock_dar_buffer;
  mem_block_t     **dar_buffer;                     /*!< \brief Array of rx PDUs. */
  list_t            pdus_from_mac_layer;            /*!< \brief Not Used. */

  logical_chan_id_t channel_id;                     /*!< \brief Transport channel identifier. */
  rb_id_t           rb_id;                          /*!< \brief Radio bearer identifier, for statistics and trace purpose. */
  rlc_usn_t         last_reassemblied_sn;           /*!< \brief Sequence number of the last reassemblied PDU. */
  rlc_usn_t         last_reassemblied_missing_sn;   /*!< \brief Sequence number of the last found missing PDU. */
  rlc_usn_t         reassembly_missing_sn_detected; /*!< \brief Act as a boolean, set if a hole in the sequence numbering of received PDUs has been found. */
  //-----------------------------
  // STATISTICS
  //-----------------------------
  boolean_t         first_pdu;                        /*!< \brief Act as a boolean, tells if the next PDU is the first PDU to be received. */

  unsigned int stat_tx_pdcp_sdu;                      /*!< \brief Number of SDUs received from upper layers. */
  unsigned int stat_tx_pdcp_bytes;                    /*!< \brief Number of SDU bytes received from upper layers. */
  unsigned int stat_tx_pdcp_sdu_discarded;            /*!< \brief Number of SDUs received from upper layers that have been discarded. */
  unsigned int stat_tx_pdcp_bytes_discarded;          /*!< \brief Number of SDU bytes received from upper layers that have been discarded. */

  unsigned int stat_tx_data_pdu;                         /*!< \brief For statistic report, number of transmitted PDUs to lower layers. */
  unsigned int stat_tx_data_bytes;                       /*!< \brief For statistic report, number of transmitted bytes to lower layers. */
  unsigned int stat_rx_pdcp_sdu;                         /*!< \brief For statistic report, number of reassemblied SDUs, sent to upper layers. */
  unsigned int stat_rx_pdcp_bytes;                       /*!< \brief For statistic report, number of reassemblied bytes, sent to upper layers. */
  unsigned int stat_rx_data_pdus_duplicate;              /*!< \brief For statistic report, number of received duplicated PDUs from lower layers. */
  unsigned int stat_rx_data_bytes_duplicate;             /*!< \brief For statistic report, number of received duplicated bytes from lower layers. */
  unsigned int stat_rx_data_pdu;                         /*!< \brief For statistic report, number of received PDUs from lower layers. */
  unsigned int stat_rx_data_bytes;                       /*!< \brief For statistic report, number of received bytes from lower layers. */
  unsigned int stat_rx_data_pdu_dropped;                 /*!< \brief For statistic report, number of dropped received PDUs from lower layers. Does not include out of window stat. */
  unsigned int stat_rx_data_bytes_dropped;               /*!< \brief For statistic report, number of dropped received bytes from lower layers. Does not include out of window stat. */
  unsigned int stat_rx_data_pdu_out_of_window;           /*!< \brief Number of data PDUs received out of the receive window. */
  unsigned int stat_rx_data_bytes_out_of_window;         /*!< \brief Number of data bytes received out of the receive window. */
  unsigned int stat_timer_reordering_timed_out;
} rlc_um_entity_t;
/** @} */
#    endif
