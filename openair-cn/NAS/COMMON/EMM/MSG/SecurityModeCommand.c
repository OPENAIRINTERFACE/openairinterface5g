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
  OpenAirInterface Dev  : openair4g-devel@eurecom.fr

  Address      : Eurecom, Compus SophiaTech 450, route des chappes, 06451 Biot, France.

 *******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>


#include "TLVEncoder.h"
#include "TLVDecoder.h"
#include "NASSecurityModeCommand.h"

int decode_security_mode_command(security_mode_command_msg *security_mode_command, uint8_t *buffer, uint32_t len)
{
  uint32_t decoded = 0;
  int decoded_result = 0;

  // Check if we got a NULL pointer and if buffer length is >= minimum length expected for the message.
  CHECK_PDU_POINTER_AND_LENGTH_DECODER(buffer, SECURITY_MODE_COMMAND_MINIMUM_LENGTH, len);

  /* Decoding mandatory fields */
  if ((decoded_result = decode_nas_security_algorithms(&security_mode_command->selectednassecurityalgorithms, 0, buffer + decoded, len - decoded)) < 0)
    return decoded_result;
  else
    decoded += decoded_result;

  if ((decoded_result = decode_u8_nas_key_set_identifier(&security_mode_command->naskeysetidentifier, 0, *(buffer + decoded) & 0x0f, len - decoded)) < 0)
    return decoded_result;

  decoded++;

  if ((decoded_result = decode_ue_security_capability(&security_mode_command->replayeduesecuritycapabilities, 0, buffer + decoded, len - decoded)) < 0)
    return decoded_result;
  else
    decoded += decoded_result;

  /* Decoding optional fields */
  while(len - decoded > 0) {
    uint8_t ieiDecoded = *(buffer + decoded);

    /* Type | value iei are below 0x80 so just return the first 4 bits */
    if (ieiDecoded >= 0x80)
      ieiDecoded = ieiDecoded & 0xf0;

    switch(ieiDecoded) {
    case SECURITY_MODE_COMMAND_IMEISV_REQUEST_IEI:
      if ((decoded_result =
             decode_imeisv_request(&security_mode_command->imeisvrequest,
                                   SECURITY_MODE_COMMAND_IMEISV_REQUEST_IEI, buffer + decoded,
                                   len - decoded)) <= 0)
        return decoded_result;

      decoded += decoded_result;
      /* Set corresponding mask to 1 in presencemask */
      security_mode_command->presencemask |= SECURITY_MODE_COMMAND_IMEISV_REQUEST_PRESENT;
      break;

    case SECURITY_MODE_COMMAND_REPLAYED_NONCEUE_IEI:
      if ((decoded_result =
             decode_nonce(&security_mode_command->replayednonceue,
                          SECURITY_MODE_COMMAND_REPLAYED_NONCEUE_IEI, buffer +
                          decoded, len - decoded)) <= 0)
        return decoded_result;

      decoded += decoded_result;
      /* Set corresponding mask to 1 in presencemask */
      security_mode_command->presencemask |= SECURITY_MODE_COMMAND_REPLAYED_NONCEUE_PRESENT;
      break;

    case SECURITY_MODE_COMMAND_NONCEMME_IEI:
      if ((decoded_result =
             decode_nonce(&security_mode_command->noncemme,
                          SECURITY_MODE_COMMAND_NONCEMME_IEI, buffer + decoded, len -
                          decoded)) <= 0)
        return decoded_result;

      decoded += decoded_result;
      /* Set corresponding mask to 1 in presencemask */
      security_mode_command->presencemask |= SECURITY_MODE_COMMAND_NONCEMME_PRESENT;
      break;

    default:
      errorCodeDecoder = TLV_DECODE_UNEXPECTED_IEI;
      return TLV_DECODE_UNEXPECTED_IEI;
    }
  }

  return decoded;
}

int encode_security_mode_command(security_mode_command_msg *security_mode_command, uint8_t *buffer, uint32_t len)
{
  int encoded = 0;
  int encode_result = 0;

  /* Checking IEI and pointer */
  CHECK_PDU_POINTER_AND_LENGTH_ENCODER(buffer, SECURITY_MODE_COMMAND_MINIMUM_LENGTH, len);

  if ((encode_result =
         encode_nas_security_algorithms(&security_mode_command->selectednassecurityalgorithms,
                                        0, buffer + encoded, len - encoded)) < 0)        //Return in case of error
    return encode_result;
  else
    encoded += encode_result;

  *(buffer + encoded) = (encode_u8_nas_key_set_identifier(&security_mode_command->naskeysetidentifier) & 0x0f);
  encoded++;

  if ((encode_result =
         encode_ue_security_capability(&security_mode_command->replayeduesecuritycapabilities,
                                       0, buffer + encoded, len - encoded)) < 0)        //Return in case of error
    return encode_result;
  else
    encoded += encode_result;

  if ((security_mode_command->presencemask & SECURITY_MODE_COMMAND_IMEISV_REQUEST_PRESENT)
      == SECURITY_MODE_COMMAND_IMEISV_REQUEST_PRESENT) {
    if ((encode_result =
           encode_imeisv_request(&security_mode_command->imeisvrequest,
                                 SECURITY_MODE_COMMAND_IMEISV_REQUEST_IEI, buffer + encoded, len -
                                 encoded)) < 0)
      // Return in case of error
      return encode_result;
    else
      encoded += encode_result;
  }

  if ((security_mode_command->presencemask & SECURITY_MODE_COMMAND_REPLAYED_NONCEUE_PRESENT)
      == SECURITY_MODE_COMMAND_REPLAYED_NONCEUE_PRESENT) {
    if ((encode_result =
           encode_nonce(&security_mode_command->replayednonceue,
                        SECURITY_MODE_COMMAND_REPLAYED_NONCEUE_IEI, buffer + encoded, len
                        - encoded)) < 0)
      // Return in case of error
      return encode_result;
    else
      encoded += encode_result;
  }

  if ((security_mode_command->presencemask & SECURITY_MODE_COMMAND_NONCEMME_PRESENT)
      == SECURITY_MODE_COMMAND_NONCEMME_PRESENT) {
    if ((encode_result = encode_nonce(&security_mode_command->noncemme,
                                      SECURITY_MODE_COMMAND_NONCEMME_IEI, buffer + encoded, len -
                                      encoded)) < 0)
      // Return in case of error
      return encode_result;
    else
      encoded += encode_result;
  }

  return encoded;
}

