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
#include "UTIL/LOG/log_if.h"
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
#if 0
  LOG_D(PDCP, "Advancing RX window...\n");
#endif

  if (pdcp_entity->next_pdcp_rx_sn == pdcp_calculate_max_seq_num_for_given_size(pdcp_entity->seq_num_size)) {
    pdcp_entity->next_pdcp_rx_sn = 0;
    pdcp_entity->rx_hfn++;
  } else {
    pdcp_entity->next_pdcp_rx_sn++;
  }

  return TRUE;
}

/**
 * Checks if incoming PDU has a sequence number in accordance with the RX window
 * @return 1 if SN is okay, 0 otherwise
 * XXX Reordering window should also be handled here
 */
boolean_t pdcp_is_rx_seq_number_valid(uint16_t seq_num, pdcp_t* pdcp_entity,srb_flag_t srb_flagP)
{

  uint16_t  reordering_window = 0;

#if 0
  LOG_D(PDCP, "Incoming RX Sequence number is %04d\n", seq_num);
#endif

  if (pdcp_is_seq_num_size_valid(pdcp_entity) == FALSE || pdcp_is_seq_num_valid(seq_num, pdcp_entity->seq_num_size) == FALSE) {
    return FALSE;
  }

  /*
   * Mark received sequence numbers to keep track of missing ones
   * (and to build PDCP Control PDU for PDCP status report)
   */
  if (pdcp_mark_current_pdu_as_received(seq_num, pdcp_entity) == TRUE) {
#if 0
    LOG_I(PDCP, "Received sequence number successfuly marked\n");
#endif
  } else {
    LOG_W(PDCP, "Cannot mark received sequence number on the bitmap!\n");
  }

  /*
   * RX Procedures for SRB and DRBs as described in sec 5.1.2 of 36.323
   */

  if (srb_flagP) { // SRB

    if (seq_num < pdcp_entity->next_pdcp_rx_sn) {
      // decipher and verify the integrity of the PDU (if applicable) using COUNT based on RX_HFN + 1 and the received PDCP SN
      pdcp_entity->rx_hfn++;
      pdcp_entity->rx_hfn_offset   = 0;
    } else {
      // decipher and verify the integrity of the PDU (if applicable) using COUNT based using COUNT based on RX_HFN and the received PDCP SN
      pdcp_entity->rx_hfn_offset   = 0;
    }

    // Assume  that integrity verification is applicable and the integrity verification is passed successfully;
    // or assume that  integrity verification is not applicable:

    // same the old next_pdcp_rx_sn to revert otherwise
    pdcp_entity->next_pdcp_rx_sn_before_integrity = pdcp_entity->next_pdcp_rx_sn;
#if 0

    if (seq_num != pdcp_entity->next_pdcp_rx_sn) {
      LOG_D(PDCP,"Re-adjusting the sequence number to %d\n", seq_num);
    }

#endif
    //set Next_PDCP_RX_SN to the received PDCP SN +1 ;
    pdcp_entity->next_pdcp_rx_sn = seq_num;
    pdcp_advance_rx_window(pdcp_entity);  // + 1, and check if it is larger than Maximum_PDCP_SN:

  } else { // DRB

    if (pdcp_entity->seq_num_size == PDCP_SN_7BIT) {
      reordering_window = REORDERING_WINDOW_SN_7BIT;
    } else {
      reordering_window = REORDERING_WINDOW_SN_12BIT;
    }

    switch (pdcp_entity->rlc_mode) {
    case RLC_MODE_AM:
      if ((seq_num - pdcp_entity->last_submitted_pdcp_rx_sn > reordering_window)  ||
          ((0 <= pdcp_entity->last_submitted_pdcp_rx_sn - seq_num) &&
           (pdcp_entity->last_submitted_pdcp_rx_sn - seq_num < reordering_window)  )) {

        if (seq_num  > pdcp_entity->next_pdcp_rx_sn) {
          /*
           * decipher the PDCP PDU as specified in the subclause 5.6, using COUNT based on RX_HFN - 1 and the received PDCP SN;
           */
          pdcp_entity->rx_hfn_offset   =  -1;
        } else  {
          /*
           *  decipher the PDCP PDU as specified in the subclause 5.6, using COUNT based on RX_HFN and the received PDCP SN;
           */
          pdcp_entity->rx_hfn_offset   = 0;
        }

        // discard this PDCP SDU;
        LOG_W(PDCP, "Out of the reordering window (Incoming SN:%d, Expected SN:%d): discard this PDCP SDU\n",
              seq_num, pdcp_entity->next_pdcp_rx_sn);
        return FALSE;
      } else if (pdcp_entity->next_pdcp_rx_sn - seq_num > reordering_window) {
        pdcp_entity->rx_hfn++;
        // use COUNT based on RX_HFN and the received PDCP SN for deciphering the PDCP PDU;
        pdcp_entity->rx_hfn_offset   = 0;
        pdcp_entity->next_pdcp_rx_sn++;
      } else if (seq_num - pdcp_entity->next_pdcp_rx_sn  >= reordering_window ) {
        //  use COUNT based on RX_HFN – 1 and the received PDCP SN for deciphering the PDCP PDU;
        pdcp_entity->rx_hfn_offset   = -1;
      } else if (seq_num  >= pdcp_entity->next_pdcp_rx_sn ) {
        // use COUNT based on RX_HFN and the received PDCP SN for deciphering the PDCP PDU;
        pdcp_entity->rx_hfn_offset = 0;
        //set Next_PDCP_RX_SN to the received PDCP SN +1 ;
        pdcp_entity->next_pdcp_rx_sn = seq_num;
        pdcp_advance_rx_window(pdcp_entity);  // + 1, anc check if it is larger than Maximum_PDCP_SN:
#if 0
        LOG_D(PDCP,"Re-adjusting the sequence number to %d\n", seq_num);
#endif
      } else if (seq_num < pdcp_entity->next_pdcp_rx_sn) {
        // use COUNT based on RX_HFN and the received PDCP SN for deciphering the PDCP PDU;
        pdcp_entity->rx_hfn_offset = 0;
      }

      break;

    case RLC_MODE_UM :
      if (seq_num <  pdcp_entity->next_pdcp_rx_sn) {
        pdcp_entity->rx_hfn++;
      }

      // decipher the PDCP Data PDU using COUNT based on RX_HFN and the received PDCP SN as specified in the subclause 5.6;
      //set Next_PDCP_RX_SN to the received PDCP SN +1 ;
      pdcp_entity->next_pdcp_rx_sn = seq_num;
      pdcp_advance_rx_window(pdcp_entity);  // + 1, and check if it is larger than Maximum_PDCP_SN:

      break;

    case RLC_MODE_TM :
    default:
      LOG_W(PDCP,"RLC mode %d not supported\n",pdcp_entity->rlc_mode);
      return FALSE;
    }
  }

  /*
  if (seq_num == pdcp_entity->next_pdcp_rx_sn) {
    LOG_I(PDCP, "Next expected SN (%d) arrived, advancing RX window\n", seq_num);

    return pdcp_advance_rx_window(pdcp_entity);
  } else {
    LOG_E(PDCP, "Incoming SN is not the one we expected to receive! (Incoming:%d, Expected:%d)\n", \
        seq_num, pdcp_entity->next_pdcp_rx_sn);


    // Update first missing PDU (used in PDCP Control PDU for PDCP status report, see 6.2.6)
    if (pdcp_entity->first_missing_pdu != -1)
      pdcp_entity->first_missing_pdu = pdcp_entity->next_pdcp_rx_sn;

    return FALSE;
  }
  */
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
#if 0
  LOG_D(PDCP, "Marking %d. bit of %d. octet of status bitmap\n", (seq_num % 8) + 1, octet_index);
#endif
  util_mark_nth_bit_of_octet(&pdcp_entity->missing_pdu_bitmap[octet_index], seq_num % 8);
#if 0
  util_print_binary_representation((uint8_t*)"Current state of relevant octet: ", pdcp_entity->missing_pdu_bitmap[octet_index]);
#endif
  return TRUE;
}
