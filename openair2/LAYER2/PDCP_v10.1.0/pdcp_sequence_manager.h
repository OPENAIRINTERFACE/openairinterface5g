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

/*! \file pdcp_sequence_manager.h
* \brief PDCP Sequence Numbering Methods
* \author Baris Demiray
* \date 2011
*/

#ifndef PDCP_SEQUENCE_MANAGER_H
#define PDCP_SEQUENCE_MANAGER_H

#include "pdcp.h"

/**
 * Initializes sequence numbering state
 * @param pdcp_entity The PDCP entity to be initialized
 * @return none
 */
boolean_t pdcp_init_seq_numbers(pdcp_t* pdcp_entity);
/**
 * Checks if incoming PDCP entitiy pointer and relevant sequence number size is valid
 * @return TRUE (0x01) if it is valid, FALSE (0x00) otherwise
 */
boolean_t pdcp_is_seq_num_size_valid(pdcp_t* pdcp_entity);
/**
 * Check if SN number is in the range according to SN size
 * @return TRUE if it is valid, FALSE otherwise
 */
boolean_t pdcp_is_seq_num_valid(uint16_t seq_num, uint8_t seq_num_size);
/**
 * Returns the maximum allowed sequence number value for given size of SN field
 * @return Max sequence number value
 */
uint16_t pdcp_calculate_max_seq_num_for_given_size(uint8_t seq_num_size);
/**
 * Returns the next TX sequence number for given PDCP entity
 */
uint16_t pdcp_get_next_tx_seq_number(pdcp_t* pdcp_entity);
/**
 * Advances the RX window state of given PDCP entity upon successfull receipt of a SDU
 */
boolean_t pdcp_advance_rx_window(pdcp_t* pdcp_entity);
/**
 * Checks if incoming PDU has a sequence number in accordance with the RX window
 * @return TRUE if it is valid, FALSE otherwise
 */
boolean_t pdcp_is_rx_seq_number_valid(uint16_t seq_num, pdcp_t* pdcp_entity,srb_flag_t srb_flagP);
/**
* Updates missing PDU bitmap with incoming sequence number
* @return TRUE if successful, FALSE otherwise
*/
boolean_t pdcp_mark_current_pdu_as_received(uint16_t seq_num, pdcp_t* pdcp_entity);

#endif
