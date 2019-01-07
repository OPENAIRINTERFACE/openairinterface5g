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
#include "ActivateDefaultEpsBearerContextRequest.h"

int decode_activate_default_eps_bearer_context_request(activate_default_eps_bearer_context_request_msg *activate_default_eps_bearer_context_request, uint8_t *buffer, uint32_t len)
{
  uint32_t decoded = 0;
  int decoded_result = 0;

  // Check if we got a NULL pointer and if buffer length is >= minimum length expected for the message.
  CHECK_PDU_POINTER_AND_LENGTH_DECODER(buffer, ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_MINIMUM_LENGTH, len);

  /* Decoding mandatory fields */
  if ((decoded_result = decode_eps_quality_of_service(&activate_default_eps_bearer_context_request->epsqos, 0, buffer + decoded, len - decoded)) < 0)
    return decoded_result;
  else
    decoded += decoded_result;

  if ((decoded_result = decode_access_point_name(&activate_default_eps_bearer_context_request->accesspointname, 0, buffer + decoded, len - decoded)) < 0)
    return decoded_result;
  else
    decoded += decoded_result;

  if ((decoded_result = decode_pdn_address(&activate_default_eps_bearer_context_request->pdnaddress, 0, buffer + decoded, len - decoded)) < 0)
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
    case ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_TRANSACTION_IDENTIFIER_IEI:
      if ((decoded_result =
             decode_transaction_identifier(&activate_default_eps_bearer_context_request->transactionidentifier,
                                           ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_TRANSACTION_IDENTIFIER_IEI,
                                           buffer + decoded, len - decoded)) <= 0)
        return decoded_result;

      decoded += decoded_result;
      /* Set corresponding mask to 1 in presencemask */
      activate_default_eps_bearer_context_request->presencemask |= ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_TRANSACTION_IDENTIFIER_PRESENT;
      break;

    case ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_NEGOTIATED_QOS_IEI:
      if ((decoded_result =
             decode_quality_of_service(&activate_default_eps_bearer_context_request->negotiatedqos,
                                       ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_NEGOTIATED_QOS_IEI,
                                       buffer + decoded, len - decoded)) <= 0)
        return decoded_result;

      decoded += decoded_result;
      /* Set corresponding mask to 1 in presencemask */
      activate_default_eps_bearer_context_request->presencemask |= ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_NEGOTIATED_QOS_PRESENT;
      break;

    case ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_NEGOTIATED_LLC_SAPI_IEI:
      if ((decoded_result =
             decode_llc_service_access_point_identifier(&activate_default_eps_bearer_context_request->negotiatedllcsapi,
                 ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_NEGOTIATED_LLC_SAPI_IEI,
                 buffer + decoded, len - decoded)) <= 0)
        return decoded_result;

      decoded += decoded_result;
      /* Set corresponding mask to 1 in presencemask */
      activate_default_eps_bearer_context_request->presencemask |= ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_NEGOTIATED_LLC_SAPI_PRESENT;
      break;

    case ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_RADIO_PRIORITY_IEI:
      if ((decoded_result =
             decode_radio_priority(&activate_default_eps_bearer_context_request->radiopriority,
                                   ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_RADIO_PRIORITY_IEI,
                                   buffer + decoded, len - decoded)) <= 0)
        return decoded_result;

      decoded += decoded_result;
      /* Set corresponding mask to 1 in presencemask */
      activate_default_eps_bearer_context_request->presencemask |= ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_RADIO_PRIORITY_PRESENT;
      break;

    case ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_PACKET_FLOW_IDENTIFIER_IEI:
      if ((decoded_result =
             decode_packet_flow_identifier(&activate_default_eps_bearer_context_request->packetflowidentifier,
                                           ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_PACKET_FLOW_IDENTIFIER_IEI,
                                           buffer + decoded, len - decoded)) <= 0)
        return decoded_result;

      decoded += decoded_result;
      /* Set corresponding mask to 1 in presencemask */
      activate_default_eps_bearer_context_request->presencemask |= ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_PACKET_FLOW_IDENTIFIER_PRESENT;
      break;

    case ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_APNAMBR_IEI:
      if ((decoded_result =
             decode_apn_aggregate_maximum_bit_rate(&activate_default_eps_bearer_context_request->apnambr,
                 ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_APNAMBR_IEI,
                 buffer + decoded, len - decoded)) <= 0)
        return decoded_result;

      decoded += decoded_result;
      /* Set corresponding mask to 1 in presencemask */
      activate_default_eps_bearer_context_request->presencemask |= ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_APNAMBR_PRESENT;
      break;

    case ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_ESM_CAUSE_IEI:
      if ((decoded_result =
             decode_esm_cause(&activate_default_eps_bearer_context_request->esmcause,
                              ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_ESM_CAUSE_IEI,
                              buffer + decoded, len - decoded)) <= 0)
        return decoded_result;

      decoded += decoded_result;
      /* Set corresponding mask to 1 in presencemask */
      activate_default_eps_bearer_context_request->presencemask |= ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_ESM_CAUSE_PRESENT;
      break;

    case ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_PROTOCOL_CONFIGURATION_OPTIONS_IEI:
      if ((decoded_result =
             decode_protocol_configuration_options(&activate_default_eps_bearer_context_request->protocolconfigurationoptions,
                 ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_PROTOCOL_CONFIGURATION_OPTIONS_IEI,
                 buffer + decoded, len - decoded)) <= 0)
        return decoded_result;

      decoded += decoded_result;
      /* Set corresponding mask to 1 in presencemask */
      activate_default_eps_bearer_context_request->presencemask |= ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_PROTOCOL_CONFIGURATION_OPTIONS_PRESENT;
      break;

    default:
      errorCodeDecoder = TLV_DECODE_UNEXPECTED_IEI;
      return TLV_DECODE_UNEXPECTED_IEI;
    }
  }

  return decoded;
}

int encode_activate_default_eps_bearer_context_request(activate_default_eps_bearer_context_request_msg *activate_default_eps_bearer_context_request, uint8_t *buffer, uint32_t len)
{
  int encoded = 0;
  int encode_result = 0;

  /* Checking IEI and pointer */
  CHECK_PDU_POINTER_AND_LENGTH_ENCODER(buffer, ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_MINIMUM_LENGTH, len);

  if ((encode_result =
         encode_eps_quality_of_service(&activate_default_eps_bearer_context_request->epsqos,
                                       0, buffer + encoded, len - encoded)) < 0) {       //Return in case of error
    LOG_TRACE(ERROR, "ESM  ENCODE epsqos");
    return encode_result;
  } else
    encoded += encode_result;

  if ((encode_result =
         encode_access_point_name(&activate_default_eps_bearer_context_request->accesspointname,
                                  0, buffer + encoded, len - encoded)) < 0) {       //Return in case of error
    LOG_TRACE(ERROR, "ESM  ENCODE accesspointname");
    return encode_result;
  } else
    encoded += encode_result;

  if ((encode_result =
         encode_pdn_address(&activate_default_eps_bearer_context_request->pdnaddress,
                            0, buffer + encoded, len - encoded)) < 0) {       //Return in case of error
    LOG_TRACE(ERROR, "ESM  ENCODE pdnaddress");
    return encode_result;
  } else
    encoded += encode_result;

  if ((activate_default_eps_bearer_context_request->presencemask & ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_TRANSACTION_IDENTIFIER_PRESENT)
      == ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_TRANSACTION_IDENTIFIER_PRESENT) {
    if ((encode_result =
           encode_transaction_identifier(&activate_default_eps_bearer_context_request->transactionidentifier,
                                         ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_TRANSACTION_IDENTIFIER_IEI,
                                         buffer + encoded, len - encoded)) < 0) {
      LOG_TRACE(ERROR, "ESM  ENCODE transactionidentifier");
      // Return in case of error
      return encode_result;
    } else
      encoded += encode_result;
  }

  if ((activate_default_eps_bearer_context_request->presencemask & ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_NEGOTIATED_QOS_PRESENT)
      == ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_NEGOTIATED_QOS_PRESENT) {
    if ((encode_result =
           encode_quality_of_service(&activate_default_eps_bearer_context_request->negotiatedqos,
                                     ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_NEGOTIATED_QOS_IEI,
                                     buffer + encoded, len - encoded)) < 0) {
      LOG_TRACE(ERROR, "ESM  ENCODE negotiatedqos");
      // Return in case of error
      return encode_result;
    } else
      encoded += encode_result;
  }

  if ((activate_default_eps_bearer_context_request->presencemask & ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_NEGOTIATED_LLC_SAPI_PRESENT)
      == ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_NEGOTIATED_LLC_SAPI_PRESENT) {
    if ((encode_result =
           encode_llc_service_access_point_identifier(&activate_default_eps_bearer_context_request->negotiatedllcsapi,
               ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_NEGOTIATED_LLC_SAPI_IEI,
               buffer + encoded, len - encoded)) < 0) {
      LOG_TRACE(ERROR, "ESM  ENCODE negotiatedllcsapi");
      // Return in case of error
      return encode_result;
    } else
      encoded += encode_result;
  }

  if ((activate_default_eps_bearer_context_request->presencemask & ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_RADIO_PRIORITY_PRESENT)
      == ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_RADIO_PRIORITY_PRESENT) {
    if ((encode_result =
           encode_radio_priority(&activate_default_eps_bearer_context_request->radiopriority,
                                 ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_RADIO_PRIORITY_IEI,
                                 buffer + encoded, len - encoded)) < 0) {
      LOG_TRACE(ERROR, "ESM  ENCODE radiopriority");
      // Return in case of error
      return encode_result;
    } else
      encoded += encode_result;
  }

  if ((activate_default_eps_bearer_context_request->presencemask & ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_PACKET_FLOW_IDENTIFIER_PRESENT)
      == ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_PACKET_FLOW_IDENTIFIER_PRESENT) {
    if ((encode_result =
           encode_packet_flow_identifier(&activate_default_eps_bearer_context_request->packetflowidentifier,
                                         ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_PACKET_FLOW_IDENTIFIER_IEI,
                                         buffer + encoded, len - encoded)) < 0) {
      LOG_TRACE(ERROR, "ESM  ENCODE packetflowidentifier");
      // Return in case of error
      return encode_result;
    } else
      encoded += encode_result;
  }

  if ((activate_default_eps_bearer_context_request->presencemask & ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_APNAMBR_PRESENT)
      == ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_APNAMBR_PRESENT) {
    if ((encode_result =
           encode_apn_aggregate_maximum_bit_rate(&activate_default_eps_bearer_context_request->apnambr,
               ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_APNAMBR_IEI, buffer +
               encoded, len - encoded)) < 0) {
      LOG_TRACE(ERROR, "ESM  ENCODE apnambr");
      // Return in case of error
      return encode_result;
    } else
      encoded += encode_result;
  }

  if ((activate_default_eps_bearer_context_request->presencemask & ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_ESM_CAUSE_PRESENT)
      == ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_ESM_CAUSE_PRESENT) {
    if ((encode_result =
           encode_esm_cause(&activate_default_eps_bearer_context_request->esmcause,
                            ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_ESM_CAUSE_IEI, buffer
                            + encoded, len - encoded)) < 0) {
      LOG_TRACE(ERROR, "ESM  ENCODE esmcause");
      // Return in case of error
      return encode_result;
    } else
      encoded += encode_result;
  }

  if ((activate_default_eps_bearer_context_request->presencemask & ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_PROTOCOL_CONFIGURATION_OPTIONS_PRESENT)
      == ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_PROTOCOL_CONFIGURATION_OPTIONS_PRESENT) {
    if ((encode_result =
           encode_protocol_configuration_options(&activate_default_eps_bearer_context_request->protocolconfigurationoptions,
               ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_PROTOCOL_CONFIGURATION_OPTIONS_IEI,
               buffer + encoded, len - encoded)) < 0) {
      LOG_TRACE(ERROR, "ESM  ENCODE protocolconfigurationoptions");
      // Return in case of error
      return encode_result;
    } else
      encoded += encode_result;
  }

  LOG_TRACE(INFO, "ESM  ENCODED activate_default_eps_bearer_context_request");
  return encoded;
}

