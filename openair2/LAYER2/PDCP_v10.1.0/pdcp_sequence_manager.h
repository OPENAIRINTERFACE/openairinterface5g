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
