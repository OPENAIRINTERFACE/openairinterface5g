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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>


#include "TLVEncoder.h"
#include "TLVDecoder.h"
#include "ModifyEpsBearerContextRequest.h"

int decode_modify_eps_bearer_context_request(modify_eps_bearer_context_request_msg *modify_eps_bearer_context_request, uint8_t *buffer, uint32_t len)
{
  uint32_t decoded = 0;
  int decoded_result = 0;

  // Check if we got a NULL pointer and if buffer length is >= minimum length expected for the message.
  CHECK_PDU_POINTER_AND_LENGTH_DECODER(buffer, MODIFY_EPS_BEARER_CONTEXT_REQUEST_MINIMUM_LENGTH, len);

  /* Decoding mandatory fields */
  /* Decoding optional fields */
  while(len - decoded > 0) {
    uint8_t ieiDecoded = *(buffer + decoded);

    /* Type | value iei are below 0x80 so just return the first 4 bits */
    if (ieiDecoded >= 0x80)
      ieiDecoded = ieiDecoded & 0xf0;

    switch(ieiDecoded) {
    case MODIFY_EPS_BEARER_CONTEXT_REQUEST_NEW_EPS_QOS_IEI:
      if ((decoded_result =
             decode_eps_quality_of_service(&modify_eps_bearer_context_request->newepsqos,
                                           MODIFY_EPS_BEARER_CONTEXT_REQUEST_NEW_EPS_QOS_IEI, buffer +
                                           decoded, len - decoded)) <= 0)
        return decoded_result;

      decoded += decoded_result;
      /* Set corresponding mask to 1 in presencemask */
      modify_eps_bearer_context_request->presencemask |= MODIFY_EPS_BEARER_CONTEXT_REQUEST_NEW_EPS_QOS_PRESENT;
      break;

    case MODIFY_EPS_BEARER_CONTEXT_REQUEST_TFT_IEI:
      if ((decoded_result =
             decode_traffic_flow_template(&modify_eps_bearer_context_request->tft,
                                          MODIFY_EPS_BEARER_CONTEXT_REQUEST_TFT_IEI, buffer +
                                          decoded, len - decoded)) <= 0)
        return decoded_result;

      decoded += decoded_result;
      /* Set corresponding mask to 1 in presencemask */
      modify_eps_bearer_context_request->presencemask |= MODIFY_EPS_BEARER_CONTEXT_REQUEST_TFT_PRESENT;
      break;

    case MODIFY_EPS_BEARER_CONTEXT_REQUEST_NEW_QOS_IEI:
      if ((decoded_result =
             decode_quality_of_service(&modify_eps_bearer_context_request->newqos,
                                       MODIFY_EPS_BEARER_CONTEXT_REQUEST_NEW_QOS_IEI, buffer +
                                       decoded, len - decoded)) <= 0)
        return decoded_result;

      decoded += decoded_result;
      /* Set corresponding mask to 1 in presencemask */
      modify_eps_bearer_context_request->presencemask |= MODIFY_EPS_BEARER_CONTEXT_REQUEST_NEW_QOS_PRESENT;
      break;

    case MODIFY_EPS_BEARER_CONTEXT_REQUEST_NEGOTIATED_LLC_SAPI_IEI:
      if ((decoded_result =
             decode_llc_service_access_point_identifier(&modify_eps_bearer_context_request->negotiatedllcsapi,
                 MODIFY_EPS_BEARER_CONTEXT_REQUEST_NEGOTIATED_LLC_SAPI_IEI,
                 buffer + decoded, len - decoded)) <= 0)
        return decoded_result;

      decoded += decoded_result;
      /* Set corresponding mask to 1 in presencemask */
      modify_eps_bearer_context_request->presencemask |= MODIFY_EPS_BEARER_CONTEXT_REQUEST_NEGOTIATED_LLC_SAPI_PRESENT;
      break;

    case MODIFY_EPS_BEARER_CONTEXT_REQUEST_RADIO_PRIORITY_IEI:
      if ((decoded_result =
             decode_radio_priority(&modify_eps_bearer_context_request->radiopriority,
                                   MODIFY_EPS_BEARER_CONTEXT_REQUEST_RADIO_PRIORITY_IEI,
                                   buffer + decoded, len - decoded)) <= 0)
        return decoded_result;

      decoded += decoded_result;
      /* Set corresponding mask to 1 in presencemask */
      modify_eps_bearer_context_request->presencemask |= MODIFY_EPS_BEARER_CONTEXT_REQUEST_RADIO_PRIORITY_PRESENT;
      break;

    case MODIFY_EPS_BEARER_CONTEXT_REQUEST_PACKET_FLOW_IDENTIFIER_IEI:
      if ((decoded_result =
             decode_packet_flow_identifier(&modify_eps_bearer_context_request->packetflowidentifier,
                                           MODIFY_EPS_BEARER_CONTEXT_REQUEST_PACKET_FLOW_IDENTIFIER_IEI,
                                           buffer + decoded, len - decoded)) <= 0)
        return decoded_result;

      decoded += decoded_result;
      /* Set corresponding mask to 1 in presencemask */
      modify_eps_bearer_context_request->presencemask |= MODIFY_EPS_BEARER_CONTEXT_REQUEST_PACKET_FLOW_IDENTIFIER_PRESENT;
      break;

    case MODIFY_EPS_BEARER_CONTEXT_REQUEST_APNAMBR_IEI:
      if ((decoded_result =
             decode_apn_aggregate_maximum_bit_rate(&modify_eps_bearer_context_request->apnambr,
                 MODIFY_EPS_BEARER_CONTEXT_REQUEST_APNAMBR_IEI, buffer +
                 decoded, len - decoded)) <= 0)
        return decoded_result;

      decoded += decoded_result;
      /* Set corresponding mask to 1 in presencemask */
      modify_eps_bearer_context_request->presencemask |= MODIFY_EPS_BEARER_CONTEXT_REQUEST_APNAMBR_PRESENT;
      break;

    case MODIFY_EPS_BEARER_CONTEXT_REQUEST_PROTOCOL_CONFIGURATION_OPTIONS_IEI:
      if ((decoded_result =
             decode_protocol_configuration_options(&modify_eps_bearer_context_request->protocolconfigurationoptions,
                 MODIFY_EPS_BEARER_CONTEXT_REQUEST_PROTOCOL_CONFIGURATION_OPTIONS_IEI,
                 buffer + decoded, len - decoded)) <= 0)
        return decoded_result;

      decoded += decoded_result;
      /* Set corresponding mask to 1 in presencemask */
      modify_eps_bearer_context_request->presencemask |= MODIFY_EPS_BEARER_CONTEXT_REQUEST_PROTOCOL_CONFIGURATION_OPTIONS_PRESENT;
      break;

    default:
      errorCodeDecoder = TLV_DECODE_UNEXPECTED_IEI;
      return TLV_DECODE_UNEXPECTED_IEI;
    }
  }

  return decoded;
}

int encode_modify_eps_bearer_context_request(modify_eps_bearer_context_request_msg *modify_eps_bearer_context_request, uint8_t *buffer, uint32_t len)
{
  int encoded = 0;
  int encode_result = 0;

  /* Checking IEI and pointer */
  CHECK_PDU_POINTER_AND_LENGTH_ENCODER(buffer, MODIFY_EPS_BEARER_CONTEXT_REQUEST_MINIMUM_LENGTH, len);

  if ((modify_eps_bearer_context_request->presencemask & MODIFY_EPS_BEARER_CONTEXT_REQUEST_NEW_EPS_QOS_PRESENT)
      == MODIFY_EPS_BEARER_CONTEXT_REQUEST_NEW_EPS_QOS_PRESENT) {
    if ((encode_result =
           encode_eps_quality_of_service(&modify_eps_bearer_context_request->newepsqos,
                                         MODIFY_EPS_BEARER_CONTEXT_REQUEST_NEW_EPS_QOS_IEI, buffer +
                                         encoded, len - encoded)) < 0)
      // Return in case of error
      return encode_result;
    else
      encoded += encode_result;
  }

  if ((modify_eps_bearer_context_request->presencemask & MODIFY_EPS_BEARER_CONTEXT_REQUEST_TFT_PRESENT)
      == MODIFY_EPS_BEARER_CONTEXT_REQUEST_TFT_PRESENT) {
    if ((encode_result =
           encode_traffic_flow_template(&modify_eps_bearer_context_request->tft,
                                        MODIFY_EPS_BEARER_CONTEXT_REQUEST_TFT_IEI, buffer + encoded, len -
                                        encoded)) < 0)
      // Return in case of error
      return encode_result;
    else
      encoded += encode_result;
  }

  if ((modify_eps_bearer_context_request->presencemask & MODIFY_EPS_BEARER_CONTEXT_REQUEST_NEW_QOS_PRESENT)
      == MODIFY_EPS_BEARER_CONTEXT_REQUEST_NEW_QOS_PRESENT) {
    if ((encode_result =
           encode_quality_of_service(&modify_eps_bearer_context_request->newqos,
                                     MODIFY_EPS_BEARER_CONTEXT_REQUEST_NEW_QOS_IEI, buffer + encoded,
                                     len - encoded)) < 0)
      // Return in case of error
      return encode_result;
    else
      encoded += encode_result;
  }

  if ((modify_eps_bearer_context_request->presencemask & MODIFY_EPS_BEARER_CONTEXT_REQUEST_NEGOTIATED_LLC_SAPI_PRESENT)
      == MODIFY_EPS_BEARER_CONTEXT_REQUEST_NEGOTIATED_LLC_SAPI_PRESENT) {
    if ((encode_result =
           encode_llc_service_access_point_identifier(&modify_eps_bearer_context_request->negotiatedllcsapi,
               MODIFY_EPS_BEARER_CONTEXT_REQUEST_NEGOTIATED_LLC_SAPI_IEI, buffer
               + encoded, len - encoded)) < 0)
      // Return in case of error
      return encode_result;
    else
      encoded += encode_result;
  }

  if ((modify_eps_bearer_context_request->presencemask & MODIFY_EPS_BEARER_CONTEXT_REQUEST_RADIO_PRIORITY_PRESENT)
      == MODIFY_EPS_BEARER_CONTEXT_REQUEST_RADIO_PRIORITY_PRESENT) {
    if ((encode_result =
           encode_radio_priority(&modify_eps_bearer_context_request->radiopriority,
                                 MODIFY_EPS_BEARER_CONTEXT_REQUEST_RADIO_PRIORITY_IEI, buffer +
                                 encoded, len - encoded)) < 0)
      // Return in case of error
      return encode_result;
    else
      encoded += encode_result;
  }

  if ((modify_eps_bearer_context_request->presencemask & MODIFY_EPS_BEARER_CONTEXT_REQUEST_PACKET_FLOW_IDENTIFIER_PRESENT)
      == MODIFY_EPS_BEARER_CONTEXT_REQUEST_PACKET_FLOW_IDENTIFIER_PRESENT) {
    if ((encode_result =
           encode_packet_flow_identifier(&modify_eps_bearer_context_request->packetflowidentifier,
                                         MODIFY_EPS_BEARER_CONTEXT_REQUEST_PACKET_FLOW_IDENTIFIER_IEI,
                                         buffer + encoded, len - encoded)) < 0)
      // Return in case of error
      return encode_result;
    else
      encoded += encode_result;
  }

  if ((modify_eps_bearer_context_request->presencemask & MODIFY_EPS_BEARER_CONTEXT_REQUEST_APNAMBR_PRESENT)
      == MODIFY_EPS_BEARER_CONTEXT_REQUEST_APNAMBR_PRESENT) {
    if ((encode_result =
           encode_apn_aggregate_maximum_bit_rate(&modify_eps_bearer_context_request->apnambr,
               MODIFY_EPS_BEARER_CONTEXT_REQUEST_APNAMBR_IEI, buffer + encoded,
               len - encoded)) < 0)
      // Return in case of error
      return encode_result;
    else
      encoded += encode_result;
  }

  if ((modify_eps_bearer_context_request->presencemask & MODIFY_EPS_BEARER_CONTEXT_REQUEST_PROTOCOL_CONFIGURATION_OPTIONS_PRESENT)
      == MODIFY_EPS_BEARER_CONTEXT_REQUEST_PROTOCOL_CONFIGURATION_OPTIONS_PRESENT) {
    if ((encode_result =
           encode_protocol_configuration_options(&modify_eps_bearer_context_request->protocolconfigurationoptions,
               MODIFY_EPS_BEARER_CONTEXT_REQUEST_PROTOCOL_CONFIGURATION_OPTIONS_IEI,
               buffer + encoded, len - encoded)) < 0)
      // Return in case of error
      return encode_result;
    else
      encoded += encode_result;
  }

  return encoded;
}

