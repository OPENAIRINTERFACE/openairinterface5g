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
#include "ServiceRequest.h"

int decode_service_request(service_request_msg *service_request, uint8_t *buffer, uint32_t len)
{
  uint32_t decoded = 0;
  int decoded_result = 0;

  LOG_FUNC_IN;
  // Check if we got a NULL pointer and if buffer length is >= minimum length expected for the message.
  CHECK_PDU_POINTER_AND_LENGTH_DECODER(buffer, SERVICE_REQUEST_MINIMUM_LENGTH, len);

  /* Decoding mandatory fields */
  if ((decoded_result = decode_ksi_and_sequence_number(&service_request->ksiandsequencenumber, 0, buffer + decoded, len - decoded)) < 0)
    LOG_FUNC_RETURN(decoded_result);
  else
    decoded += decoded_result;

  if ((decoded_result = decode_short_mac(&service_request->messageauthenticationcode, 0, buffer + decoded, len - decoded)) < 0)
    LOG_FUNC_RETURN(decoded_result);
  else
    decoded += decoded_result;

  LOG_FUNC_RETURN(decoded);
}

int encode_service_request(service_request_msg *service_request, uint8_t *buffer, uint32_t len)
{
  int encoded = 0;
  int encode_result = 0;

  LOG_FUNC_IN;
  /* Checking IEI and pointer */
  CHECK_PDU_POINTER_AND_LENGTH_ENCODER(buffer, SERVICE_REQUEST_MINIMUM_LENGTH, len);

  if ((encode_result =
         encode_ksi_and_sequence_number(&service_request->ksiandsequencenumber,
                                        0, buffer + encoded, len - encoded)) < 0)        //Return in case of error
    LOG_FUNC_RETURN(encode_result);
  else
    encoded += encode_result;

  if ((encode_result =
         encode_short_mac(&service_request->messageauthenticationcode, 0,
                          buffer + encoded, len - encoded)) < 0)        //Return in case of error
    LOG_FUNC_RETURN(encode_result);
  else
    encoded += encode_result;

  LOG_FUNC_RETURN(encoded);
}

