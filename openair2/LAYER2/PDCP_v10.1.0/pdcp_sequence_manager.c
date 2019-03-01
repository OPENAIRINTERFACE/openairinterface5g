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

/*! \file pdcp_sequence_manager.c
* \brief PDCP Sequence Numbering Methods
* \author Baris Demiray and Navid Nikaein
* \email navid.nikaein@eurecom.fr
* \date 2014
*/

#include "pdcp_sequence_manager.h"
#include "common/utils//LOG/log_if.h"
#include "pdcp_util.h"

/*
 * Initializes sequence numbering state
 * @param pdcp_entity The PDCP entity to be initialized
 * @return boolean_t TRUE on success, FALSE otherwise
 */
boolean_t pdcp_init_seq_numbers(pdcp_t* pdcp_entity)
{
  if (pdcp_entity == NULL) {
    return FALSE;
  }

  /* Sequence number state variables */

  // TX and RX window
  pdcp_entity->next_pdcp_tx_sn = 0;
  pdcp_entity->next_pdcp_rx_sn = 0;

  // TX and RX Hyper Frame Numbers
  pdcp_entity->tx_hfn = 0;
  pdcp_entity->rx_hfn = 0;

  // SN of the last PDCP SDU delivered to upper layers
  // Shall UE and eNB behave differently on initialization? (see 7.1.e)
  pdcp_entity->last_submitted_pdcp_rx_sn = 4095;

  return TRUE;
}

boolean_t pdcp_is_seq_num_size_valid(pdcp_t* pdcp_entity)
{
  if (pdcp_entity == NULL) {
    return FALSE;
  }

  // Check if the size of SN is valid (see 3GPP TS 36.323 v10.1.0 item 6.3.2)
  if (pdcp_entity->seq_num_size != 5 && pdcp_entity->seq_num_size != 7 && pdcp_entity->seq_num_size != 12) {
    LOG_W(PDCP, "Incoming SN size is invalid! (Expected: {5 | 7 | 12}, Received: %d\n", pdcp_entity->seq_num_size);
    return FALSE;
  }

  return TRUE;
}

/**
 * Check if SN number is in the range according to SN size
 */
boolean_t pdcp_is_seq_num_valid(uint16_t seq_num, uint8_t seq_num_size)
{
  if (seq_num >= 0 && seq_num <= pdcp_calculate_max_seq_num_for_given_size(seq_num_size)) {
    return TRUE;
  }

  return FALSE;
}

uint16_t pdcp_calculate_max_seq_num_for_given_size(uint8_t seq_num_size)
{
  uint16_t max_seq_num = 1;

  max_seq_num <<= seq_num_size;

  return max_seq_num - 1;
}

uint16_t pdcp_get_next_tx_seq_number(pdcp_t* pdcp_entity)
{
  if (pdcp_is_seq_num_size_valid(pdcp_entity) == FALSE) {
    return -1;
  }

  // Sequence number should be incremented after it is assigned for a PDU
  uint16_t pdcp_seq_num = pdcp_entity->next_pdcp_tx_sn;

  /*
   * Update sequence numbering state and Hyper Frame Number if SN has already reached
   * its max value (see 5.1 PDCP Data Transfer Procedures)
   */
  if (pdcp_entity->next_pdcp_tx_sn == pdcp_calculate_max_seq_num_for_given_size(pdcp_entity->seq_num_size)) {
    pdcp_entity->next_pdcp_tx_sn = 0;
    pdcp_entity->tx_hfn++;
    LOG_D(PDCP,"Reseting the PDCP sequence number\n");
  } else {
    pdcp_entity->next_pdcp_tx_sn++;
  }

  return pdcp_seq_num;
}

boolean_t pdcp_advance_rx_window(pdcp_t* pdcp_entity)
{
  if (pdcp_is_seq_num_size_valid(pdcp_entity) == FALSE) {
    return FALSE;
  }

  /*
   * Update sequence numbering state and Hyper Frame Number if SN has already reached
   * its max value (see 5.1 PDCP Data Transfer Procedures)
   */
  LOG_D(PDCP, "Advancing RX window...\n");

  if (pdcp_entity->next_pdcp_rx_sn == pdcp_calculate_max_seq_num_for_given_size(pdcp_entity->seq_num_size)) {
    pdcp_entity->next_pdcp_rx_sn = 0;
    pdcp_entity->rx_hfn++;
  } else {
    pdcp_entity->next_pdcp_rx_sn++;
  }

  return TRUE;
}

boolean_t pdcp_mark_current_pdu_as_received(uint16_t seq_num, pdcp_t* pdcp_entity)
{
  /*
   * Incoming sequence number and PDCP entity were already
   * validated in pdcp_is_rx_seq_number_valid() so we don't
   * check here
   */

  /*
   * Find relevant octet
   */
  uint16_t octet_index = seq_num / 8;
  /*
   * Set relevant bit
   */
  LOG_D(PDCP, "Marking %d. bit of %d. octet of status bitmap\n", (seq_num % 8) + 1, octet_index);
  util_mark_nth_bit_of_octet(&pdcp_entity->missing_pdu_bitmap[octet_index], seq_num % 8);
  util_print_binary_representation((uint8_t*)"Current state of relevant octet: ", pdcp_entity->missing_pdu_bitmap[octet_index]);
  return TRUE;
}
