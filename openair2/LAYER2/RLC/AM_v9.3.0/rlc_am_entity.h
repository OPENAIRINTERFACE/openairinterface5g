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

/*! \file rlc_am_entity.h
* \brief This file defines the RLC AM variables stored in a struct called rlc_am_entity_t.
* \author GAUTHIER Lionel
* \date 2010-2011
* \version
* \note The rlc_am_entity_t structure store protocol variables, statistic variables, allocation variables, buffers and other miscellaneous variables.
* \bug
* \warning
*/
/** @defgroup _rlc_am_internal_impl_ RLC AM Layer Internal Reference Implementation
* @ingroup _rlc_am_impl_
* @{
*/

#    ifndef __RLC_AM_ENTITY_H__
#        define __RLC_AM_ENTITY_H__
//-----------------------------------------------------------------------------
#        include <pthread.h>
#        include "platform_types.h"
#        include "platform_constants.h"
#        include "list.h"
#        include "rlc_primitives.h"
#        include "rlc_def_lte.h"
#        include "rlc_def.h"
#        include "rlc_am_structs.h"
#        include "rlc_am_constants.h"
//-----------------------------------------------------------------------------
/*! \struct  rlc_am_entity_t
* \brief Structure containing a RLC AM instance protocol variables, statistic variables, allocation variables, buffers and other miscellaneous variables.
*/
typedef struct rlc_am_entity_s {


  rb_id_t           rb_id;                              /*!< \brief Radio bearer identifier, for statistics and trace purpose. */
  logical_chan_id_t channel_id;                         /*!< \brief Transport channel identifier. */
  bool              is_data_plane;                      /*!< \brief To know if the RLC belongs to a data radio bearer or a signalling radio bearer, for statistics and trace purpose. */

  rlc_buffer_occupancy_t sdu_buffer_occupancy;               /*!< \brief Number of bytes of unsegmented SDUs. */
  rlc_buffer_occupancy_t status_buffer_occupancy;            /*!< \brief Number of bytes of control PDUs waiting for transmission. */

  rlc_am_control_pdu_info_t  control_pdu_info;

  //---------------------------------------------------------------------
  // TX BUFFERS
  //---------------------------------------------------------------------
  //pthread_spinlock_t lock_input_sdus;
  pthread_mutex_t      lock_input_sdus;
  rlc_am_tx_sdu_management_t   *input_sdus;           /*!< \brief Input SDU buffer (for SDUs coming from upper layers). */
  signed int      nb_sdu;                             /*!< \brief Total number of valid rlc_am_tx_sdu_management_t in input_sdus[]. */
  signed int      nb_sdu_no_segmented;                /*!< \brief Total number of SDUs not segmented and partially segmented. nb_sdu_no_segmented = next_sdu_index - current_sdu_index */
  signed int      next_sdu_index;                     /*!< \brief Next SDU index in input_sdus array where for a new incoming SDU. */
  signed int      current_sdu_index;                  /*!< \brief Current SDU index in input_sdus array to be segmented which is not segmented or partially segmented. */


  rlc_am_tx_data_pdu_management_t *tx_data_pdu_buffer;       /*!< \brief Transmission PDU data buffer. Used also for retransmissions */
  signed int      retrans_num_pdus;                          /*!< \brief Number of PDUs in the retransmission buffer. */
  signed int      retrans_num_bytes_to_retransmit;           /*!< \brief Number of bytes in the retransmission buffer to be retransmitted. Only payload is taken into account */
  bool            force_poll;                                /*!< \brief force poll due to t_poll_retransmit time-out. */

  //---------------------------------------------------------------------
  // RX BUFFERS
  //---------------------------------------------------------------------
  list2_t         receiver_buffer;                           /*!< \brief Receiver buffer implemented with a list. */
  mem_block_t     *output_sdu_in_construction;               /*!< \brief Memory area where a complete SDU is reassemblied before being send to upper layers. */
  sdu_size_t       output_sdu_size_to_write;                  /*!< \brief Size of the reassemblied SDU. */


  //---------------------------------------------------------------------
  // PROTOCOL VARIABLES
  //---------------------------------------------------------------------
  rlc_protocol_state_t protocol_state; /*!< \brief Protocol state, can be RLC_NULL_STATE, RLC_DATA_TRANSFER_READY_STATE. */
  //-----------------------------
  // TX STATE VARIABLES
  //-----------------------------
  rlc_usn_t
  vt_a;         /*!< \brief Acknowledgement state variable. This state variable holds the value of the SN of the next AMD PDU for which a positive acknowledgment is to be received in-sequence, and it serves as the lower edge of the transmitting window. It is initially set to 0, and is updated whenever the AM RLC entity receives a positive acknowledgment for an AMD PDU with SN = VT(A).*/
  rlc_usn_t           vt_ms;        /*!< \brief Maximum send state variable. This state variable equals VT(A) + AM_Window_Size, and it serves as the higher edge of the transmitting window. */
  rlc_usn_t
  vt_s;         /*!< \brief  Send state variable. This state variable holds the value of the SN to be assigned for the next newly generated AMD PDU. It is initially set to 0, and is updated whenever the AM RLC entity delivers an AMD PDU with SN = VT(S).*/
  rlc_usn_t
  poll_sn;      /*!< \brief  Poll send state variable. This state variable holds the value of VT(S)-1 upon the most recent transmission of a RLC data PDU with the poll bit set to “1”. It is initially set to 0.*/

  //-----------------------------
  // RX STATE VARIABLES
  //-----------------------------
  rlc_usn_t
  vr_r;         /*!< \brief Receive state variable. This state variable holds the value of the SN following the last in-sequence completely received AMD PDU, and it serves as the lower edge of the receiving window. It is initially set to 0, and is updated whenever the AM RLC entity receives an AMD PDU with SN = VR(R). */
  rlc_usn_t
  vr_mr;        /*!< \brief Maximum acceptable receive state variable. This state variable equals VR(R) + AM_Window_Size, and it holds the value of the SN of the first AMD PDU that is beyond the receiving window and serves as the higher edge of the receiving window. */
  rlc_usn_t           vr_x;         /*!< \brief t-Reordering state variable. This state variable holds the value of the SN following the SN of the RLC data PDU which triggered t-Reordering. */
  rlc_usn_t
  vr_ms;        /*!< \brief Maximum STATUS transmit state variable. This state variable holds the highest possible value of the SN which can be indicated by “ACK_SN” when a STATUS PDU needs to be constructed. It is initially set to 0. */
  rlc_usn_t
  vr_h;         /*!< \brief Highest received state variable. This state variable holds the value of the SN following the SN of the RLC data PDU with the highest SN among received RLC data PDUs. It is initially set to 0. */

  //-----------------------------
  // TIMERS CONFIGURED BY RRC
  //-----------------------------
  rlc_am_timer_t  t_poll_retransmit; /*!< \brief This timer is used by the transmitting side of an AM RLC entity in order to retransmit a poll. */
  rlc_am_timer_t
  t_reordering;      /*!< \brief This timer is used by the receiving side of an AM RLC entity and receiving UM RLC entity in order to detect loss of RLC PDUs at lower layer (see sub clauses 5.1.2.2 and 5.1.3.2). If t-Reordering is running, t-Reordering shall not be started additionally, i.e. only one t-Reordering per RLC entity is running at a given time. */
  rlc_am_timer_t  t_status_prohibit; /*!< \brief This timer is used by the receiving side of an AM RLC entity in order to prohibit transmission of a STATUS PDU. */

  //-----------------------------
  // COUNTERS
  //-----------------------------
  unsigned int    c_pdu_without_poll; /*!< \brief This counter is initially set to 0. It counts the number of AMD PDUs sent since the most recent poll bit was transmitted. */
  unsigned int    c_byte_without_poll;/*!< \brief This counter is initially set to 0. It counts the number of data bytes sent since the most recent poll bit was transmitted. */

  //-----------------------------
  // PARAMETERS CONFIGURED BY RRC
  //-----------------------------
  uint16_t           max_retx_threshold; /*!< \brief This parameter is used by the transmitting side of each AM RLC entity to limit the number of retransmissions of an AMD PDU. */
  uint16_t           poll_pdu;           /*!< \brief This parameter is used by the transmitting side of each AM RLC entity to trigger a poll for every pollPDU PDUs. */
  uint32_t           poll_byte;          /*!< \brief This parameter is used by the transmitting side of each AM RLC entity to trigger a poll for every pollByte bytes. */

  //---------------------------------------------------------------------
  // STATISTICS
  //---------------------------------------------------------------------
  unsigned int stat_tx_pdcp_sdu;                         /*!< \brief Number of SDUs received from upper layers. */
  unsigned int stat_tx_pdcp_bytes;                       /*!< \brief Number of SDU bytes received from upper layers. */
  unsigned int stat_tx_pdcp_sdu_discarded;               /*!< \brief Number of SDUs received from upper layers that have been discarded. */
  unsigned int stat_tx_pdcp_bytes_discarded;             /*!< \brief Number of SDU bytes received from upper layers that have been discarded. */

  unsigned int stat_tx_data_pdu;                         /*!< \brief For statistic report, number of transmitted PDUs to lower layers. */
  unsigned int stat_tx_data_bytes;                       /*!< \brief For statistic report, number of transmitted bytes to lower layers. */
  unsigned int stat_tx_retransmit_pdu_by_status;         /*!< \brief Not updated. */
  unsigned int stat_tx_retransmit_bytes_by_status;       /*!< \brief Not updated. */
  unsigned int stat_tx_retransmit_pdu;                   /*!< \brief Not updated. */
  unsigned int stat_tx_retransmit_bytes;                 /*!< \brief Not updated. */
  unsigned int stat_tx_control_pdu;                      /*!< \brief Not updated. */
  unsigned int stat_tx_control_bytes;                    /*!< \brief Not updated. */

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
  unsigned int stat_rx_control_pdu;                      /*!< \brief Number of control PDUs received. */
  unsigned int stat_rx_control_bytes;                    /*!< \brief Number of control bytes received. */

  unsigned int stat_timer_reordering_timed_out;
  unsigned int stat_timer_poll_retransmit_timed_out;
  unsigned int stat_timer_status_prohibit_timed_out;

  //---------------------------------------------------------------------
  // OUTPUTS
  //---------------------------------------------------------------------
  sdu_size_t        nb_bytes_requested_by_mac;  /*!< \brief Number of remaining bytes available for transmission of any RLC PDU indicated by lower layer */
  list_t            pdus_to_mac_layer;          /*!< \brief PDUs buffered for transmission to MAC layer. */
  list_t            control_pdu_list;           /*!< \brief Control PDUs buffered for transmission to MAC layer. */
  list_t            segmentation_pdu_list;      /*!< \brief List of "freshly" segmented PDUs. */

  uint8_t           status_requested;             /*!< \brief Status bitmap requested by peer. */
  rlc_sn_t		    sn_status_triggered_delayed;   /*!< \brief SN of the last received poll for which Status is delayed until SN is out of Rx Window. */
  uint32_t          last_absolute_subframe_status_indication; /*!< \brief The last absolute subframe number a  MAC status indication has been received by RLC. */
  //-----------------------------
  // buffer occupancy measurements sent to MAC
  //-----------------------------
  // note occupancy of other buffers is deducted from nb elements in lists
  rlc_buffer_occupancy_t  buffer_occupancy_retransmission_buffer;   /*!< \brief Number of PDUs. */

  bool                    initialized;                               /*!< \brief Boolean for rlc_am_entity_t struct initialization. */
  bool                    configured;                               /*!< \brief Boolean for rlc_am_entity_t struct  configuration. */
} rlc_am_entity_t;
/** @} */
#    endif
