/*******************************************************************************
    OpenAirInterface
    Copyright(c) 1999 - 2014 Eurecom

    OpenAirInterface is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.


    OpenAirInterface is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with OpenAirInterface.The full GNU General Public License is
   included in this distribution in the file called "COPYING". If not,
   see <http://www.gnu.org/licenses/>.

  Contact Information
  OpenAirInterface Admin: openair_admin@eurecom.fr
  OpenAirInterface Tech : openair_tech@eurecom.fr
  OpenAirInterface Dev  : openair4g-devel@lists.eurecom.fr

  Address      : Eurecom, Campus SophiaTech, 450 Route des Chappes, CS 50193 - 06904 Biot Sophia Antipolis cedex, FRANCE

 *******************************************************************************/
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
