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

/*! \file pdcp_util.c
* \brief PDCP Util Methods
* \author Baris Demiray
* \date 2012
*/

#include <assert.h>

#include "UTIL/LOG/log.h"
#include "pdcp_util.h"

/*
 * Prints incoming byte stream in hexadecimal and readable form
 *
 * @param component Utilised as with macros defined in UTIL/LOG/log.h
 * @param data unsigned char* pointer for data buffer
 * @param size Number of octets in data buffer
 * @return none
 */
void util_print_hex_octets(comp_name_t component, unsigned char* data, unsigned long size)
{
  if (data == NULL) {
    LOG_W(component, "Incoming buffer is NULL! Ignoring...\n");
    return;
  }

  unsigned long octet_index = 0;

  LOG_T(component, "     |  0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f |\n");
  LOG_T(component, "-----+-------------------------------------------------|\n");
  LOG_T(component, " 000 |");

  for (octet_index = 0; octet_index < size; ++octet_index) {
    /*
     * Print every single octet in hexadecimal form
     */
    LOG_T(component, "%02x.", data[octet_index]);

    /*
     * Align newline and pipes according to the octets in groups of 2
     */
    if (octet_index != 0 && (octet_index + 1) % 16 == 0) {
      LOG_T(component, " |\n");
      LOG_T(component, " %03lu |", octet_index);
    }
  }

  /*
   * Append enough spaces and put final pipe
   */
  unsigned char index;

  for (index = octet_index; index < 16; ++index) {
    LOG_T(component, "   ");
  }

  LOG_T(component, " \n");
}

void util_flush_hex_octets(comp_name_t component, unsigned char* data, unsigned long size)
{
  if (data == NULL) {
    LOG_W(component, "Incoming buffer is NULL! Ignoring...\n");
    return;
  }

  printf("[PDCP]");

  unsigned long octet_index = 0;

  for (octet_index = 0; octet_index < size; ++octet_index) {
    //LOG_T(component, "%02x.", data[octet_index]);
    printf("%02x.", data[octet_index]);
  }

  //LOG_T(component, " \n");
  printf(" \n");
}

/*
 * Prints binary representation of given octet prepending
 * passed log message
 *
 * @param Octet as an unsigned character
 * @return None
 */
void util_print_binary_representation(unsigned char* message, uint8_t octet)
{
  unsigned char index = 0;
  unsigned char mask = 0x80;

  LOG_T(PDCP, "%s", message);

  for (index = 0; index < 8; ++index) {
    if (octet & mask) {
      LOG_T(PDCP, "1");
    } else {
      LOG_T(PDCP, "0");
    }

    mask /= 2;
  }

  LOG_T(PDCP, "\n");
}

/*
 * Sets the bit of given octet at `index'
 *
 * @param octet 8-bit data
 * @param index Index of bit to be set
 * @return TRUE on success, FALSE otherwise
 */
boolean_t util_mark_nth_bit_of_octet(uint8_t* octet, uint8_t index)
{
  uint8_t mask = 0x80;

  assert(octet != NULL);

  /*
   * Prepare mask
   */
  mask >>= index;

  /*
   * Set relevant bit
   */
  *octet |= mask;

  return TRUE;
}

