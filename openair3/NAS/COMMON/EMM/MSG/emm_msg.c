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

/*****************************************************************************

Source      emm_msg.c

Version     0.1

Date        2012/09/27

Product     NAS stack

Subsystem   EPS Mobility Management

Author      Frederic Maurel, Sebastien Roux

Description Defines EPS Mobility Management messages

*****************************************************************************/

#include "emm_msg.h"
#include "commonDef.h"
#include "nas_log.h"

#include "TLVDecoder.h"
#include "TLVEncoder.h"

#if ((defined(NAS_BUILT_IN_EPC) && defined(NAS_MME)) || (defined(ENABLE_NAS_UE_LOGGING) && defined(NAS_BUILT_IN_UE) && defined(NAS_UE)))
# include "nas_itti_messaging.h"
#endif

/****************************************************************************/
/****************  E X T E R N A L    D E F I N I T I O N S  ****************/
/****************************************************************************/

/****************************************************************************/
/*******************  L O C A L    D E F I N I T I O N S  *******************/
/****************************************************************************/

static int _emm_msg_decode_header(emm_msg_header_t *header,
                                  const uint8_t *buffer, uint32_t len);
static int _emm_msg_encode_header(const emm_msg_header_t *header,
                                  uint8_t *buffer, uint32_t len);

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

/****************************************************************************
 **                                                                        **
 ** Name:    emm_msg_decode()                                          **
 **                                                                        **
 ** Description: Decode EPS Mobility Management messages                   **
 **                                                                        **
 ** Inputs:  buffer:    Pointer to the buffer containing the EMM   **
 **             message data                               **
 **          len:       Number of bytes that should be decoded     **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     msg:       The EMM message structure to be filled     **
 **          Return:    The number of bytes in the buffer if data  **
 **             have been successfully decoded;            **
 **             A negative error code otherwise.           **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int emm_msg_decode(EMM_msg *msg, uint8_t *buffer, uint32_t len)
{
  LOG_FUNC_IN;

  int header_result;
  int decode_result;

#if ((defined(NAS_BUILT_IN_EPC) && defined(NAS_MME)) || (defined(ENABLE_NAS_UE_LOGGING) && defined(NAS_BUILT_IN_UE) && defined(NAS_UE)))
  uint8_t *buffer_log = buffer;
  uint32_t len_log = len;
  int down_link;

# if ((defined(NAS_BUILT_IN_EPC) && defined(NAS_MME)))
  down_link = 0;
# else
  down_link = 1;
# endif
#endif

  /* First decode the EMM message header */
  header_result = _emm_msg_decode_header(&msg->header, buffer, len);

  if (header_result < 0) {
    LOG_TRACE(ERROR, "EMM-MSG   - Failed to decode EMM message header "
              "(%d)", header_result);
    LOG_FUNC_RETURN(header_result);
  }

  buffer += header_result;
  len -= header_result;

  LOG_TRACE(INFO, "EMM-MSG   - Message Type 0x%02x", msg->header.message_type);

  switch(msg->header.message_type) {
  case EMM_INFORMATION:
    decode_result = decode_emm_information(&msg->emm_information, buffer, len);
    break;

  case UPLINK_NAS_TRANSPORT:
    decode_result = decode_uplink_nas_transport(&msg->uplink_nas_transport, buffer,
                    len);
    break;

  case AUTHENTICATION_REJECT:
    decode_result = decode_authentication_reject(&msg->authentication_reject,
                    buffer, len);
    break;

  case AUTHENTICATION_FAILURE:
    decode_result = decode_authentication_failure(&msg->authentication_failure,
                    buffer, len);
    break;

  case DETACH_ACCEPT:
    decode_result = decode_detach_accept(&msg->detach_accept, buffer, len);
    break;

  case SERVICE_REJECT:
    decode_result = decode_service_reject(&msg->service_reject, buffer, len);
    break;

  case AUTHENTICATION_REQUEST:
    decode_result = decode_authentication_request(&msg->authentication_request,
                    buffer, len);
    break;

  case TRACKING_AREA_UPDATE_REQUEST:
    decode_result = decode_tracking_area_update_request(
                      &msg->tracking_area_update_request, buffer, len);
    break;

  case ATTACH_REQUEST:
    decode_result = decode_attach_request(&msg->attach_request, buffer, len);
    break;

  case EMM_STATUS:
    decode_result = decode_emm_status(&msg->emm_status, buffer, len);
    break;

  case IDENTITY_RESPONSE:
    decode_result = decode_identity_response(&msg->identity_response, buffer, len);
    break;

  case IDENTITY_REQUEST:
    decode_result = decode_identity_request(&msg->identity_request, buffer, len);
    break;

  case GUTI_REALLOCATION_COMMAND:
    decode_result = decode_guti_reallocation_command(&msg->guti_reallocation_command,
                    buffer, len);
    break;

  case TRACKING_AREA_UPDATE_REJECT:
    decode_result = decode_tracking_area_update_reject(
                      &msg->tracking_area_update_reject, buffer, len);
    break;

  case ATTACH_ACCEPT:
    decode_result = decode_attach_accept(&msg->attach_accept, buffer, len);
    break;

  case SECURITY_MODE_COMPLETE:
    decode_result = decode_security_mode_complete(&msg->security_mode_complete,
                    buffer, len);
    break;

  case TRACKING_AREA_UPDATE_ACCEPT:
    decode_result = decode_tracking_area_update_accept(
                      &msg->tracking_area_update_accept, buffer, len);
    break;

  case ATTACH_REJECT:
    decode_result = decode_attach_reject(&msg->attach_reject, buffer, len);
    break;

  case ATTACH_COMPLETE:
    decode_result = decode_attach_complete(&msg->attach_complete, buffer, len);
    break;

  case TRACKING_AREA_UPDATE_COMPLETE:
    decode_result = decode_tracking_area_update_complete(
                      &msg->tracking_area_update_complete, buffer, len);
    break;

  case CS_SERVICE_NOTIFICATION:
    decode_result = decode_cs_service_notification(&msg->cs_service_notification,
                    buffer, len);
    break;

  case SECURITY_MODE_REJECT:
    decode_result = decode_security_mode_reject(&msg->security_mode_reject, buffer,
                    len);
    break;

  case DETACH_REQUEST:
    decode_result = decode_detach_request(&msg->detach_request, buffer, len);
    break;

  case GUTI_REALLOCATION_COMPLETE:
    decode_result = decode_guti_reallocation_complete(
                      &msg->guti_reallocation_complete, buffer, len);
    break;

  case SECURITY_MODE_COMMAND:
    decode_result = decode_security_mode_command(&msg->security_mode_command,
                    buffer, len);
    break;

  case DOWNLINK_NAS_TRANSPORT:
    decode_result = decode_downlink_nas_transport(&msg->downlink_nas_transport,
                    buffer, len);
    break;

  case EXTENDED_SERVICE_REQUEST:
    decode_result = decode_extended_service_request(&msg->extended_service_request,
                    buffer, len);
    break;

  case AUTHENTICATION_RESPONSE:
    decode_result = decode_authentication_response(&msg->authentication_response,
                    buffer, len);
    break;

  default:
    LOG_TRACE(ERROR, "EMM-MSG   - Unexpected message type: 0x%x",
              msg->header.message_type);
    decode_result = TLV_DECODE_WRONG_MESSAGE_TYPE;
    /* TODO: Handle not standard layer 3 messages: SERVICE_REQUEST */
  }

  if (decode_result < 0) {
    LOG_TRACE(ERROR, "EMM-MSG   - Failed to decode L3 EMM message 0x%x "
              "(%d)", msg->header.message_type, decode_result);
    LOG_FUNC_RETURN (decode_result);
  } else {
#if ((defined(NAS_BUILT_IN_EPC) && defined(NAS_MME)) || (defined(ENABLE_NAS_UE_LOGGING) && defined(NAS_BUILT_IN_UE) && defined(NAS_UE)))
    /* Message has been decoded and security header removed, handle it has a plain message */
    nas_itti_plain_msg((char *) buffer_log, (nas_message_t *) msg, len_log, down_link);
#endif
  }

  LOG_FUNC_RETURN (header_result + decode_result);
}

/****************************************************************************
 **                                                                        **
 ** Name:    emm_msg_encode()                                          **
 **                                                                        **
 ** Description: Encode EPS Mobility Management messages                   **
 **                                                                        **
 ** Inputs:  msg:       The EMM message structure to encode        **
 **          length:    Maximal capacity of the output buffer      **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     buffer:    Pointer to the encoded data buffer         **
 **          Return:    The number of bytes in the buffer if data  **
 **             have been successfully encoded;            **
 **             A negative error code otherwise.           **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int emm_msg_encode(EMM_msg *msg, uint8_t *buffer, uint32_t len)
{
  LOG_FUNC_IN;

  int header_result;
  int encode_result;

#if ((defined(NAS_BUILT_IN_EPC) && defined(NAS_MME)) || (defined(ENABLE_NAS_UE_LOGGING) && defined(NAS_BUILT_IN_UE) && defined(NAS_UE)))
  uint8_t *buffer_log = buffer;
  int down_link;

# if ((defined(NAS_BUILT_IN_EPC) && defined(NAS_MME)))
  down_link = 1;
# else
  down_link = 0;
# endif
#endif

  /* First encode the EMM message header */
  header_result = _emm_msg_encode_header(&msg->header, buffer, len);

  if (header_result < 0) {
    LOG_TRACE(ERROR, "EMM-MSG   - Failed to encode EMM message header "
              "(%d)", header_result);
    LOG_FUNC_RETURN(header_result);
  }

  buffer += header_result;
  len -= header_result;

  switch(msg->header.message_type) {
  case EMM_INFORMATION:
    encode_result = encode_emm_information(&msg->emm_information, buffer, len);
    break;

  case UPLINK_NAS_TRANSPORT:
    encode_result = encode_uplink_nas_transport(&msg->uplink_nas_transport, buffer,
                    len);
    break;

  case AUTHENTICATION_REJECT:
    encode_result = encode_authentication_reject(&msg->authentication_reject,
                    buffer, len);
    break;

  case AUTHENTICATION_FAILURE:
    encode_result = encode_authentication_failure(&msg->authentication_failure,
                    buffer, len);
    break;

  case DETACH_ACCEPT:
    encode_result = encode_detach_accept(&msg->detach_accept, buffer, len);
    break;

  case SERVICE_REJECT:
    encode_result = encode_service_reject(&msg->service_reject, buffer, len);
    break;

  case AUTHENTICATION_REQUEST:
    encode_result = encode_authentication_request(&msg->authentication_request,
                    buffer, len);
    break;

  case TRACKING_AREA_UPDATE_REQUEST:
    encode_result = encode_tracking_area_update_request(
                      &msg->tracking_area_update_request, buffer, len);
    break;

  case ATTACH_REQUEST:
    encode_result = encode_attach_request(&msg->attach_request, buffer, len);
    break;

  case EMM_STATUS:
    encode_result = encode_emm_status(&msg->emm_status, buffer, len);
    break;

  case IDENTITY_RESPONSE:
    encode_result = encode_identity_response(&msg->identity_response, buffer, len);
    break;

  case IDENTITY_REQUEST:
    encode_result = encode_identity_request(&msg->identity_request, buffer, len);
    break;

  case GUTI_REALLOCATION_COMMAND:
    encode_result = encode_guti_reallocation_command(&msg->guti_reallocation_command,
                    buffer, len);
    break;

  case TRACKING_AREA_UPDATE_REJECT:
    encode_result = encode_tracking_area_update_reject(
                      &msg->tracking_area_update_reject, buffer, len);
    break;

  case ATTACH_ACCEPT:
    encode_result = encode_attach_accept(&msg->attach_accept, buffer, len);
    break;

  case SECURITY_MODE_COMPLETE:
    encode_result = encode_security_mode_complete(&msg->security_mode_complete,
                    buffer, len);
    break;

  case TRACKING_AREA_UPDATE_ACCEPT:
    encode_result = encode_tracking_area_update_accept(
                      &msg->tracking_area_update_accept, buffer, len);
    break;

  case ATTACH_REJECT:
    encode_result = encode_attach_reject(&msg->attach_reject, buffer, len);
    break;

  case ATTACH_COMPLETE:
    encode_result = encode_attach_complete(&msg->attach_complete, buffer, len);
    break;

  case TRACKING_AREA_UPDATE_COMPLETE:
    encode_result = encode_tracking_area_update_complete(
                      &msg->tracking_area_update_complete, buffer, len);
    break;

  case CS_SERVICE_NOTIFICATION:
    encode_result = encode_cs_service_notification(&msg->cs_service_notification,
                    buffer, len);
    break;

  case SECURITY_MODE_REJECT:
    encode_result = encode_security_mode_reject(&msg->security_mode_reject, buffer,
                    len);
    break;

  case DETACH_REQUEST:
    encode_result = encode_detach_request(&msg->detach_request, buffer, len);
    break;

  case GUTI_REALLOCATION_COMPLETE:
    encode_result = encode_guti_reallocation_complete(
                      &msg->guti_reallocation_complete, buffer, len);
    break;

  case SECURITY_MODE_COMMAND:
    encode_result = encode_security_mode_command(&msg->security_mode_command,
                    buffer, len);
    break;

  case DOWNLINK_NAS_TRANSPORT:
    encode_result = encode_downlink_nas_transport(&msg->downlink_nas_transport,
                    buffer, len);
    break;

  case EXTENDED_SERVICE_REQUEST:
    encode_result = encode_extended_service_request(&msg->extended_service_request,
                    buffer, len);
    break;

  case AUTHENTICATION_RESPONSE:
    encode_result = encode_authentication_response(&msg->authentication_response,
                    buffer, len);
    break;

  case SERVICE_REQUEST:
    encode_result = encode_service_request(&msg->service_request, buffer, len);
    break;

  default:
    LOG_TRACE(ERROR, "EMM-MSG   - Unexpected message type: 0x%x",
              msg->header.message_type);
    encode_result = TLV_ENCODE_WRONG_MESSAGE_TYPE;
    /* TODO: Handle not standard layer 3 messages: SERVICE_REQUEST */
  }

  if (encode_result < 0) {
    LOG_TRACE(ERROR, "EMM-MSG   - Failed to encode L3 EMM message 0x%x "
              "(%d)", msg->header.message_type, encode_result);
  } else {
#if ((defined(NAS_BUILT_IN_EPC) && defined(NAS_MME)) || (defined(ENABLE_NAS_UE_LOGGING) && defined(NAS_BUILT_IN_UE) && defined(NAS_UE)))
    nas_itti_plain_msg((char *) buffer_log, (nas_message_t *) msg, header_result + encode_result, down_link);
#endif
  }

  if (encode_result < 0)
    LOG_FUNC_RETURN (encode_result);

  LOG_FUNC_RETURN (header_result + encode_result);
}

/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/

/****************************************************************************
 **                                                                        **
 ** Name:    _emm_msg_decode_header()                                  **
 **                                                                        **
 ** Description: Decode header of EPS Mobility Management message.         **
 **      The protocol discriminator and the security header type   **
 **      have already been decoded.                                **
 **                                                                        **
 ** Inputs:  buffer:    Pointer to the buffer containing the EMM   **
 **             message                                    **
 **          len:       Number of bytes that should be decoded     **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     header:    The EMM message header to be filled        **
 **          Return:    The size of the header if data have been   **
 **             successfully decoded;                      **
 **             A negative error code otherwise.           **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
static int _emm_msg_decode_header(emm_msg_header_t *header,
                                  const uint8_t *buffer, uint32_t len)
{
  int size = 0;

  /* Check the buffer length */
  if (len < sizeof(emm_msg_header_t)) {
    return (TLV_DECODE_BUFFER_TOO_SHORT);
  }

  /* Decode the security header type and the protocol discriminator */
  DECODE_U8(buffer + size, *(uint8_t *)(header), size);
  /* Decode the message type */
  DECODE_U8(buffer + size, header->message_type, size);

  /* Check the protocol discriminator */
  if (header->protocol_discriminator != EPS_MOBILITY_MANAGEMENT_MESSAGE) {
    LOG_TRACE(ERROR, "ESM-MSG   - Unexpected protocol discriminator: 0x%x",
              header->protocol_discriminator);
    return (TLV_DECODE_PROTOCOL_NOT_SUPPORTED);
  }

  return (size);
}

/****************************************************************************
 **                                                                        **
 ** Name:    _emm_msg_encode_header()                                  **
 **                                                                        **
 **      The protocol discriminator and the security header type   **
 **      have already been encoded.                                **
 **                                                                        **
 ** Inputs:  header:    The EMM message header to encode           **
 **          len:       Maximal capacity of the output buffer      **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     buffer:    Pointer to the encoded data buffer         **
 **          Return:    The number of bytes in the buffer if data  **
 **             have been successfully encoded;            **
 **             A negative error code otherwise.           **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
static int _emm_msg_encode_header(const emm_msg_header_t *header,
                                  uint8_t *buffer, uint32_t len)
{
  int size = 0;

  /* Check the buffer length */
  if (len < sizeof(emm_msg_header_t)) {
    return (TLV_ENCODE_BUFFER_TOO_SHORT);
  }

  /* Check the protocol discriminator */
  if (header->protocol_discriminator != EPS_MOBILITY_MANAGEMENT_MESSAGE) {
    LOG_TRACE(ERROR, "ESM-MSG   - Unexpected protocol discriminator: 0x%x",
              header->protocol_discriminator);
    return (TLV_ENCODE_PROTOCOL_NOT_SUPPORTED);
  }

  /* Encode the security header type and the protocol discriminator */
  ENCODE_U8(buffer + size, *(uint8_t *)(header), size);
  /* Encode the message type */
  ENCODE_U8(buffer + size, header->message_type, size);

  return (size);
}

