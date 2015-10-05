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
#include "EmmInformation.h"

int decode_emm_information(emm_information_msg *emm_information, uint8_t *buffer, uint32_t len)
{
  uint32_t decoded = 0;
  int decoded_result = 0;

  // Check if we got a NULL pointer and if buffer length is >= minimum length expected for the message.
  CHECK_PDU_POINTER_AND_LENGTH_DECODER(buffer, EMM_INFORMATION_MINIMUM_LENGTH, len);

  /* Decoding mandatory fields */
  /* Decoding optional fields */
  while(len - decoded > 0) {
    uint8_t ieiDecoded = *(buffer + decoded);

    /* Type | value iei are below 0x80 so just return the first 4 bits */
    if (ieiDecoded >= 0x80)
      ieiDecoded = ieiDecoded & 0xf0;

    switch(ieiDecoded) {
    case EMM_INFORMATION_FULL_NAME_FOR_NETWORK_IEI:
      if ((decoded_result =
             decode_network_name(&emm_information->fullnamefornetwork,
                                 EMM_INFORMATION_FULL_NAME_FOR_NETWORK_IEI, buffer +
                                 decoded, len - decoded)) <= 0)
        return decoded_result;

      decoded += decoded_result;
      /* Set corresponding mask to 1 in presencemask */
      emm_information->presencemask |= EMM_INFORMATION_FULL_NAME_FOR_NETWORK_PRESENT;
      break;

    case EMM_INFORMATION_SHORT_NAME_FOR_NETWORK_IEI:
      if ((decoded_result =
             decode_network_name(&emm_information->shortnamefornetwork,
                                 EMM_INFORMATION_SHORT_NAME_FOR_NETWORK_IEI, buffer +
                                 decoded, len - decoded)) <= 0)
        return decoded_result;

      decoded += decoded_result;
      /* Set corresponding mask to 1 in presencemask */
      emm_information->presencemask |= EMM_INFORMATION_SHORT_NAME_FOR_NETWORK_PRESENT;
      break;

    case EMM_INFORMATION_LOCAL_TIME_ZONE_IEI:
      if ((decoded_result =
             decode_time_zone(&emm_information->localtimezone,
                              EMM_INFORMATION_LOCAL_TIME_ZONE_IEI, buffer + decoded, len
                              - decoded)) <= 0)
        return decoded_result;

      decoded += decoded_result;
      /* Set corresponding mask to 1 in presencemask */
      emm_information->presencemask |= EMM_INFORMATION_LOCAL_TIME_ZONE_PRESENT;
      break;

    case EMM_INFORMATION_UNIVERSAL_TIME_AND_LOCAL_TIME_ZONE_IEI:
      if ((decoded_result =
             decode_time_zone_and_time(&emm_information->universaltimeandlocaltimezone,
                                       EMM_INFORMATION_UNIVERSAL_TIME_AND_LOCAL_TIME_ZONE_IEI,
                                       buffer + decoded, len - decoded)) <= 0)
        return decoded_result;

      decoded += decoded_result;
      /* Set corresponding mask to 1 in presencemask */
      emm_information->presencemask |= EMM_INFORMATION_UNIVERSAL_TIME_AND_LOCAL_TIME_ZONE_PRESENT;
      break;

    case EMM_INFORMATION_NETWORK_DAYLIGHT_SAVING_TIME_IEI:
      if ((decoded_result =
             decode_daylight_saving_time(&emm_information->networkdaylightsavingtime,
                                         EMM_INFORMATION_NETWORK_DAYLIGHT_SAVING_TIME_IEI, buffer +
                                         decoded, len - decoded)) <= 0)
        return decoded_result;

      decoded += decoded_result;
      /* Set corresponding mask to 1 in presencemask */
      emm_information->presencemask |= EMM_INFORMATION_NETWORK_DAYLIGHT_SAVING_TIME_PRESENT;
      break;

    default:
      errorCodeDecoder = TLV_DECODE_UNEXPECTED_IEI;
      return TLV_DECODE_UNEXPECTED_IEI;
    }
  }

  return decoded;
}

int encode_emm_information(emm_information_msg *emm_information, uint8_t *buffer, uint32_t len)
{
  int encoded = 0;
  int encode_result = 0;

  /* Checking IEI and pointer */
  CHECK_PDU_POINTER_AND_LENGTH_ENCODER(buffer, EMM_INFORMATION_MINIMUM_LENGTH, len);

  if ((emm_information->presencemask & EMM_INFORMATION_FULL_NAME_FOR_NETWORK_PRESENT)
      == EMM_INFORMATION_FULL_NAME_FOR_NETWORK_PRESENT) {
    if ((encode_result =
           encode_network_name(&emm_information->fullnamefornetwork,
                               EMM_INFORMATION_FULL_NAME_FOR_NETWORK_IEI, buffer + encoded, len -
                               encoded)) < 0)
      // Return in case of error
      return encode_result;
    else
      encoded += encode_result;
  }

  if ((emm_information->presencemask & EMM_INFORMATION_SHORT_NAME_FOR_NETWORK_PRESENT)
      == EMM_INFORMATION_SHORT_NAME_FOR_NETWORK_PRESENT) {
    if ((encode_result =
           encode_network_name(&emm_information->shortnamefornetwork,
                               EMM_INFORMATION_SHORT_NAME_FOR_NETWORK_IEI, buffer + encoded, len
                               - encoded)) < 0)
      // Return in case of error
      return encode_result;
    else
      encoded += encode_result;
  }

  if ((emm_information->presencemask & EMM_INFORMATION_LOCAL_TIME_ZONE_PRESENT)
      == EMM_INFORMATION_LOCAL_TIME_ZONE_PRESENT) {
    if ((encode_result = encode_time_zone(&emm_information->localtimezone,
                                          EMM_INFORMATION_LOCAL_TIME_ZONE_IEI, buffer + encoded, len -
                                          encoded)) < 0)
      // Return in case of error
      return encode_result;
    else
      encoded += encode_result;
  }

  if ((emm_information->presencemask & EMM_INFORMATION_UNIVERSAL_TIME_AND_LOCAL_TIME_ZONE_PRESENT)
      == EMM_INFORMATION_UNIVERSAL_TIME_AND_LOCAL_TIME_ZONE_PRESENT) {
    if ((encode_result =
           encode_time_zone_and_time(&emm_information->universaltimeandlocaltimezone,
                                     EMM_INFORMATION_UNIVERSAL_TIME_AND_LOCAL_TIME_ZONE_IEI, buffer +
                                     encoded, len - encoded)) < 0)
      // Return in case of error
      return encode_result;
    else
      encoded += encode_result;
  }

  if ((emm_information->presencemask & EMM_INFORMATION_NETWORK_DAYLIGHT_SAVING_TIME_PRESENT)
      == EMM_INFORMATION_NETWORK_DAYLIGHT_SAVING_TIME_PRESENT) {
    if ((encode_result =
           encode_daylight_saving_time(&emm_information->networkdaylightsavingtime,
                                       EMM_INFORMATION_NETWORK_DAYLIGHT_SAVING_TIME_IEI, buffer +
                                       encoded, len - encoded)) < 0)
      // Return in case of error
      return encode_result;
    else
      encoded += encode_result;
  }

  return encoded;
}

