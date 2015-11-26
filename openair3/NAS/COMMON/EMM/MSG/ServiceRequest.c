/*
 * Copyright (c) 2015, EURECOM (www.eurecom.fr)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are those
 * of the authors and should not be interpreted as representing official policies,
 * either expressed or implied, of the FreeBSD Project.
 */
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

