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

/*! \file rlc_tm_entity.h
* \brief This file defines the RLC TM variables stored in a struct called rlc_tm_entity_t.
* \author GAUTHIER Lionel
* \date 2010-2011
* \version
* \note The rlc_tm_entity_t structure store protocol variables, statistic variables, allocation variables, buffers and other miscellaneous variables.
* \bug
* \warning
*/
#    ifndef __RLC_TM_ENTITY_H__
#        define __RLC_TM_ENTITY_H__
//-----------------------------------------------------------------------------
#        include "platform_types.h"
#        include "platform_constants.h"
#        include "rlc_tm_structs.h"
#        include "rlc_def.h"
//-----------------------------------------------------------------------------
/*! \struct  rlc_tm_entity_t
* \brief Structure containing a RLC TM instance protocol variables, allocation variables, buffers and other miscellaneous variables.
*/
typedef struct rlc_tm_entity {
  boolean_t            allocation;         /*!< \brief Boolean for rlc_tm_entity_t struct allocation. */
  rlc_protocol_state_t protocol_state;     /*!< \brief Protocol state, can be RLC_NULL_STATE, RLC_DATA_TRANSFER_READY_STATE, RLC_LOCAL_SUSPEND_STATE. */
  boolean_t            is_uplink_downlink; /*!< \brief Is this instance is a transmitter, a receiver or both? */
  boolean_t            is_data_plane;      /*!< \brief To know if the RLC belongs to a data radio bearer or a signalling radio bearer, for statistics and trace purpose. */
  // for stats and trace purpose :
  logical_chan_id_t    channel_id;         /*!< \brief Transport channel identifier. */
  rb_id_t              rb_id;              /*!< \brief Radio bearer identifier, for statistics and trace purpose. */
  //boolean_t            is_enb;             /*!< \brief To know if the RLC belongs to a eNB or UE. */
  //-----------------------------
  // tranmission
  //-----------------------------
  // sdu communication;
  mem_block_t     **input_sdus;              /*!< \brief Input SDU buffer (for SDUs coming from upper layers). Should be accessed as an array. */
  mem_block_t      *input_sdus_alloc;        /*!< \brief Allocated memory for the input SDU buffer (for SDUs coming from upper layers). */
  uint16_t             size_input_sdus_buffer;  /*!< \brief Size of the input SDU buffer. */
  uint16_t             nb_sdu;                  /*!< \brief Total number of SDUs in input_sdus[] */
  uint16_t             next_sdu_index;          /*!< \brief Next SDU index for a new incomin SDU in input_sdus[]. */
  uint16_t             current_sdu_index;       /*!< \brief Current SDU index in input_sdus array to be segmented. */
  list_t            pdus_to_mac_layer;       /*!< \brief PDUs buffered for transmission to MAC layer. */
  sdu_size_t        rlc_pdu_size;
  rlc_buffer_occupancy_t buffer_occupancy;        /*!< \brief Number of bytes contained in input_sdus buffer.*/
  //-----------------------------
  // receiver
  //-----------------------------
  unsigned int      output_sdu_size_to_write;     /*!< \brief Size of the reassemblied SDU. */
  mem_block_t      *output_sdu_in_construction;   /*!< \brief Memory area where a complete SDU is reassemblied before being send to upper layers. */
} rlc_tm_entity_t;
/** @} */
#    endif
