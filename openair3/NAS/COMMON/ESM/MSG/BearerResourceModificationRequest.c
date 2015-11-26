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
#include "BearerResourceModificationRequest.h"

int decode_bearer_resource_modification_request(bearer_resource_modification_request_msg *bearer_resource_modification_request, uint8_t *buffer, uint32_t len)
{
  uint32_t decoded = 0;
  int decoded_result = 0;

  // Check if we got a NULL pointer and if buffer length is >= minimum length expected for the message.
  CHECK_PDU_POINTER_AND_LENGTH_DECODER(buffer, BEARER_RESOURCE_MODIFICATION_REQUEST_MINIMUM_LENGTH, len);

  /* Decoding mandatory fields */
  if ((decoded_result = decode_u8_linked_eps_bearer_identity(&bearer_resource_modification_request->epsbeareridentityforpacketfilter, 0, *(buffer + decoded) >> 4, len - decoded)) < 0)
    return decoded_result;

  decoded++;

  if ((decoded_result = decode_traffic_flow_aggregate_description(&bearer_resource_modification_request->trafficflowaggregate, 0, buffer + decoded, len - decoded)) < 0)
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
    case BEARER_RESOURCE_MODIFICATION_REQUEST_REQUIRED_TRAFFIC_FLOW_QOS_IEI:
      if ((decoded_result =
             decode_eps_quality_of_service(&bearer_resource_modification_request->requiredtrafficflowqos,
                                           BEARER_RESOURCE_MODIFICATION_REQUEST_REQUIRED_TRAFFIC_FLOW_QOS_IEI,
                                           buffer + decoded, len - decoded)) <= 0)
        return decoded_result;

      decoded += decoded_result;
      /* Set corresponding mask to 1 in presencemask */
      bearer_resource_modification_request->presencemask |= BEARER_RESOURCE_MODIFICATION_REQUEST_REQUIRED_TRAFFIC_FLOW_QOS_PRESENT;
      break;

    case BEARER_RESOURCE_MODIFICATION_REQUEST_ESM_CAUSE_IEI:
      if ((decoded_result =
             decode_esm_cause(&bearer_resource_modification_request->esmcause,
                              BEARER_RESOURCE_MODIFICATION_REQUEST_ESM_CAUSE_IEI, buffer
                              + decoded, len - decoded)) <= 0)
        return decoded_result;

      decoded += decoded_result;
      /* Set corresponding mask to 1 in presencemask */
      bearer_resource_modification_request->presencemask |= BEARER_RESOURCE_MODIFICATION_REQUEST_ESM_CAUSE_PRESENT;
      break;

    case BEARER_RESOURCE_MODIFICATION_REQUEST_PROTOCOL_CONFIGURATION_OPTIONS_IEI:
      if ((decoded_result =
             decode_protocol_configuration_options(&bearer_resource_modification_request->protocolconfigurationoptions,
                 BEARER_RESOURCE_MODIFICATION_REQUEST_PROTOCOL_CONFIGURATION_OPTIONS_IEI,
                 buffer + decoded, len - decoded)) <= 0)
        return decoded_result;

      decoded += decoded_result;
      /* Set corresponding mask to 1 in presencemask */
      bearer_resource_modification_request->presencemask |= BEARER_RESOURCE_MODIFICATION_REQUEST_PROTOCOL_CONFIGURATION_OPTIONS_PRESENT;
      break;

    default:
      errorCodeDecoder = TLV_DECODE_UNEXPECTED_IEI;
      return TLV_DECODE_UNEXPECTED_IEI;
    }
  }

  return decoded;
}

int encode_bearer_resource_modification_request(bearer_resource_modification_request_msg *bearer_resource_modification_request, uint8_t *buffer, uint32_t len)
{
  int encoded = 0;
  int encode_result = 0;

  /* Checking IEI and pointer */
  CHECK_PDU_POINTER_AND_LENGTH_ENCODER(buffer, BEARER_RESOURCE_MODIFICATION_REQUEST_MINIMUM_LENGTH, len);

  *(buffer + encoded) = ((encode_u8_linked_eps_bearer_identity(&bearer_resource_modification_request->epsbeareridentityforpacketfilter) & 0x0f) << 4) | 0x00;
  encoded++;

  if ((encode_result =
         encode_traffic_flow_aggregate_description(&bearer_resource_modification_request->trafficflowaggregate,
             0, buffer + encoded, len - encoded)) < 0)        //Return in case of error
    return encode_result;
  else
    encoded += encode_result;

  if ((bearer_resource_modification_request->presencemask & BEARER_RESOURCE_MODIFICATION_REQUEST_REQUIRED_TRAFFIC_FLOW_QOS_PRESENT)
      == BEARER_RESOURCE_MODIFICATION_REQUEST_REQUIRED_TRAFFIC_FLOW_QOS_PRESENT) {
    if ((encode_result =
           encode_eps_quality_of_service(&bearer_resource_modification_request->requiredtrafficflowqos,
                                         BEARER_RESOURCE_MODIFICATION_REQUEST_REQUIRED_TRAFFIC_FLOW_QOS_IEI,
                                         buffer + encoded, len - encoded)) < 0)
      // Return in case of error
      return encode_result;
    else
      encoded += encode_result;
  }

  if ((bearer_resource_modification_request->presencemask & BEARER_RESOURCE_MODIFICATION_REQUEST_ESM_CAUSE_PRESENT)
      == BEARER_RESOURCE_MODIFICATION_REQUEST_ESM_CAUSE_PRESENT) {
    if ((encode_result =
           encode_esm_cause(&bearer_resource_modification_request->esmcause,
                            BEARER_RESOURCE_MODIFICATION_REQUEST_ESM_CAUSE_IEI, buffer +
                            encoded, len - encoded)) < 0)
      // Return in case of error
      return encode_result;
    else
      encoded += encode_result;
  }

  if ((bearer_resource_modification_request->presencemask & BEARER_RESOURCE_MODIFICATION_REQUEST_PROTOCOL_CONFIGURATION_OPTIONS_PRESENT)
      == BEARER_RESOURCE_MODIFICATION_REQUEST_PROTOCOL_CONFIGURATION_OPTIONS_PRESENT) {
    if ((encode_result =
           encode_protocol_configuration_options(&bearer_resource_modification_request->protocolconfigurationoptions,
               BEARER_RESOURCE_MODIFICATION_REQUEST_PROTOCOL_CONFIGURATION_OPTIONS_IEI,
               buffer + encoded, len - encoded)) < 0)
      // Return in case of error
      return encode_result;
    else
      encoded += encode_result;
  }

  return encoded;
}

