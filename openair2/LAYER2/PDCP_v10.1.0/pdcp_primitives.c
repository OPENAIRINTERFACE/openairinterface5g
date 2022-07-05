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

/*! \file pdcp_primitives.c
* \brief PDCP PDU buffer dissector code
* \author Baris Demiray
* \date 2011
* \version 0.1
*/

#include "common/utils/LOG/log.h"
#include "platform_types.h"
#include "platform_constants.h"
#include "pdcp.h"
#include "pdcp_primitives.h"

extern bool util_mark_nth_bit_of_octet(uint8_t* octet, uint8_t index);

/*
 * Parses data/control field out of buffer of User Plane PDCP Data PDU with
 * long PDCP SN (12-bit)
 *
 * @param pdu_buffer PDCP PDU buffer
 * @return 1 bit dc
 */
uint8_t pdcp_get_dc_filed(unsigned char* pdu_buffer)
{
  uint8_t dc = 0x00;

  if (pdu_buffer == NULL) {
    return 0;
  }

  dc = (uint8_t)pdu_buffer[0] & 0xF0; // Reset D/C field
  dc >>= 8; // FIXME this is broken!!! returns 0 all the time

  return dc;
}

/*
 * Parses sequence number out of buffer of User Plane PDCP Data PDU with
 * long PDCP SN (12-bit)
 *
 * @param pdu_buffer PDCP PDU buffer
 * @return 12-bit sequence number
 */
uint16_t pdcp_get_sequence_number_of_pdu_with_long_sn(unsigned char* pdu_buffer)
{
  uint16_t sequence_number = 0x00;

  if (pdu_buffer == NULL) {
    return 0;
  }

  /*
   * First octet carries the first 4 bits of SN (see 6.2.3)
   */
  sequence_number = (uint8_t)pdu_buffer[0] & 0x0F; // Reset D/C field
  sequence_number <<= 8;
  /*
   * Second octet carries the second part (8-bit) of SN (see 6.2.3)
   */
  sequence_number |= (uint8_t)pdu_buffer[1] & 0xFF;

  return sequence_number;
}

/*
 * Parses sequence number out of buffer of User Plane PDCP Data PDU with
 * short PDCP SN (7-bit)
 *
 * @param pdu_buffer PDCP PDU buffer
 * @return 7-bit sequence number
 */
uint8_t pdcp_get_sequence_number_of_pdu_with_short_sn(unsigned char* pdu_buffer)
{
  if (pdu_buffer == NULL) {
    return 0;
  }

  /*
   * First octet carries all 7 bits of SN (see 6.2.4)
   */
  return (uint8_t)pdu_buffer[0] & 0x7F; // Reset D/C field
}
/*
 * Parses sequence number out of buffer of Control Plane PDCP Data PDU with
 * short PDCP SN (5-bit)
 *
 * @param pdu_buffer PDCP PDU buffer
 * @return 5-bit sequence number
 */
uint8_t pdcp_get_sequence_number_of_pdu_with_SRB_sn(unsigned char* pdu_buffer)
{
  if (pdu_buffer == NULL) {
    return 0;
  }

  /*
   * First octet carries all 5 bits of SN (see 6.2.4)
   */
  return (uint8_t)pdu_buffer[0] & 0x1F;
}
/*
 * Fills the incoming buffer with the fields of the header for srb sn
 *
 * @param pdu_buffer PDCP PDU buffer
 * @return true on success, false otherwise
 */
bool pdcp_serialize_control_plane_data_pdu_with_SRB_sn_buffer(unsigned char* pdu_buffer,
                                                              pdcp_control_plane_data_pdu_header* pdu)
{
  if (pdu_buffer == NULL || pdu == NULL) {
    return false;
  }

  /*
   * Fill the Sequence Number field
   */
  uint8_t sequence_number = pdu->sn;
  pdu_buffer[0] = sequence_number & 0x1F; // 5bit sn

  return true;
}

/*
 * Fills the incoming buffer with the fields of the header for long sn
 *
 * @param pdu_buffer PDCP PDU buffer
 * @return true on success, false otherwise
 */
bool pdcp_serialize_user_plane_data_pdu_with_long_sn_buffer(unsigned char* pdu_buffer,
                                                            pdcp_user_plane_data_pdu_header_with_long_sn* pdu)
{
  if (pdu_buffer == NULL || pdu == NULL) {
    return false;
  }

  /*
   * Fill the Sequence Number field
   */
  uint16_t sequence_number = pdu->sn;
  pdu_buffer[1] = sequence_number & 0xFF;
  sequence_number >>= 8;
  pdu_buffer[0] = sequence_number & 0xFF;

  /*
   * Fill Data or Control field
   */
  if (pdu->dc == PDCP_DATA_PDU_BIT_SET) {
    LOG_D(PDCP, "Setting PDU as a DATA PDU\n");
    pdu_buffer[0] |= 0x80; // set the first bit as 1
  }

  return true;
}

/*
 * Fills the incoming PDU buffer with with given control PDU
 *
 * @param pdu_buffer The buffer that PDU will be serialized into
 * @param pdu A status report header
 * @return true on success, false otherwise
 */
bool pdcp_serialize_control_pdu_for_pdcp_status_report(unsigned char* pdu_buffer,
                                                       uint8_t bitmap[512],
                                                       pdcp_control_pdu_for_pdcp_status_report* pdu)
{
  if (pdu_buffer == NULL || pdu == NULL) {
    return false;
  }

  /*
   * Data or Control field and PDU type (already 0x00, noop)
   *
   * Set leftmost bit to set this PDU as `Control PDU'
   */
  util_mark_nth_bit_of_octet(((uint8_t*)&pdu_buffer[0]), 1);

  /*
   * Fill `First Missing PDU SN' field
   */
  pdu_buffer[0] |= ((pdu->first_missing_sn >> 8) & 0xFF);
  pdu_buffer[1] |= (pdu->first_missing_sn & 0xFF);

  /*
   * Append `bitmap'
   */
  memcpy(pdu_buffer + 2, bitmap, 512);

  return true;
}

