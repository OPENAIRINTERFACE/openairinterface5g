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
Source    esm_msg.c

Version   0.1

Date    2012/09/27

Product   NAS stack

Subsystem EPS Session Management

Author    Frederic Maurel, Sebastien Roux

Description Defines EPS Session Management messages

*****************************************************************************/

#include "esm_msg.h"
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

static int _esm_msg_decode_header(esm_msg_header_t *header, const uint8_t *buffer, uint32_t len);
static int _esm_msg_encode_header(const esm_msg_header_t *header, uint8_t *buffer, uint32_t len);

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

/****************************************************************************
 **                                                                        **
 ** Name:  esm_msg_decode()                                          **
 **                                                                        **
 ** Description: Decode EPS Session Management messages                    **
 **                                                                        **
 ** Inputs:  buffer:  Pointer to the buffer containing the ESM   **
 **       message                                    **
 **      len:   Number of bytes that should be decoded     **
 **    Others:  None                                       **
 **                                                                        **
 ** Outputs:   msg:   The ESM message structure to be filled     **
 **      Return:  The number of bytes in the buffer if data  **
 **       have been successfully decoded;            **
 **       A negative error code otherwise.           **
 **    Others:  None                                       **
 **                                                                        **
 ***************************************************************************/
int esm_msg_decode(ESM_msg *msg, uint8_t *buffer, uint32_t len)
{
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

  LOG_FUNC_IN;

  /* First decode the ESM message header */
  header_result = _esm_msg_decode_header(&msg->header, buffer, len);

  if (header_result < 0) {
    LOG_TRACE(ERROR, "ESM-MSG   - Failed to decode ESM message header "
              "(%d)", header_result);
    LOG_FUNC_RETURN(header_result);
  }

  buffer += header_result;
  len -= header_result;

  switch(msg->header.message_type) {
  case PDN_DISCONNECT_REQUEST:
    decode_result = decode_pdn_disconnect_request(&msg->pdn_disconnect_request, buffer, len);
    break;

  case DEACTIVATE_EPS_BEARER_CONTEXT_ACCEPT:
    decode_result = decode_deactivate_eps_bearer_context_accept(&msg->deactivate_eps_bearer_context_accept, buffer, len);
    break;

  case BEARER_RESOURCE_ALLOCATION_REQUEST:
    decode_result = decode_bearer_resource_allocation_request(&msg->bearer_resource_allocation_request, buffer, len);
    break;

  case ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_ACCEPT:
    decode_result = decode_activate_default_eps_bearer_context_accept(&msg->activate_default_eps_bearer_context_accept, buffer, len);
    break;

  case PDN_CONNECTIVITY_REJECT:
    decode_result = decode_pdn_connectivity_reject(&msg->pdn_connectivity_reject, buffer, len);
    break;

  case MODIFY_EPS_BEARER_CONTEXT_REJECT:
    decode_result = decode_modify_eps_bearer_context_reject(&msg->modify_eps_bearer_context_reject, buffer, len);
    break;

  case ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_REJECT:
    decode_result = decode_activate_dedicated_eps_bearer_context_reject(&msg->activate_dedicated_eps_bearer_context_reject, buffer, len);
    break;

  case MODIFY_EPS_BEARER_CONTEXT_ACCEPT:
    decode_result = decode_modify_eps_bearer_context_accept(&msg->modify_eps_bearer_context_accept, buffer, len);
    break;

  case DEACTIVATE_EPS_BEARER_CONTEXT_REQUEST:
    decode_result = decode_deactivate_eps_bearer_context_request(&msg->deactivate_eps_bearer_context_request, buffer, len);
    break;

  case ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_ACCEPT:
    decode_result = decode_activate_dedicated_eps_bearer_context_accept(&msg->activate_dedicated_eps_bearer_context_accept, buffer, len);
    break;

  case ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REJECT:
    decode_result = decode_activate_default_eps_bearer_context_reject(&msg->activate_default_eps_bearer_context_reject, buffer, len);
    break;

  case MODIFY_EPS_BEARER_CONTEXT_REQUEST:
    decode_result = decode_modify_eps_bearer_context_request(&msg->modify_eps_bearer_context_request, buffer, len);
    break;

  case PDN_DISCONNECT_REJECT:
    decode_result = decode_pdn_disconnect_reject(&msg->pdn_disconnect_reject, buffer, len);
    break;

  case ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_REQUEST:
    decode_result = decode_activate_dedicated_eps_bearer_context_request(&msg->activate_dedicated_eps_bearer_context_request, buffer, len);
    break;

  case BEARER_RESOURCE_MODIFICATION_REJECT:
    decode_result = decode_bearer_resource_modification_reject(&msg->bearer_resource_modification_reject, buffer, len);
    break;

  case BEARER_RESOURCE_ALLOCATION_REJECT:
    decode_result = decode_bearer_resource_allocation_reject(&msg->bearer_resource_allocation_reject, buffer, len);
    break;

  case ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST:
    decode_result = decode_activate_default_eps_bearer_context_request(&msg->activate_default_eps_bearer_context_request, buffer, len);
    break;

  case PDN_CONNECTIVITY_REQUEST:
    decode_result = decode_pdn_connectivity_request(&msg->pdn_connectivity_request, buffer, len);
    break;

  case ESM_INFORMATION_RESPONSE:
    decode_result = decode_esm_information_response(&msg->esm_information_response, buffer, len);
    break;

  case BEARER_RESOURCE_MODIFICATION_REQUEST:
    decode_result = decode_bearer_resource_modification_request(&msg->bearer_resource_modification_request, buffer, len);
    break;

  case ESM_INFORMATION_REQUEST:
    decode_result = decode_esm_information_request(&msg->esm_information_request, buffer, len);
    break;

  case ESM_STATUS:
    decode_result = decode_esm_status(&msg->esm_status, buffer, len);
    break;

  default:
    LOG_TRACE(ERROR, "ESM-MSG   - Unexpected message type: 0x%x",
              msg->header.message_type);
    decode_result = TLV_DECODE_WRONG_MESSAGE_TYPE;
    break;
  }

  if (decode_result < 0) {
    LOG_TRACE(ERROR, "ESM-MSG   - Failed to decode L3 ESM message 0x%x "
              "(%u)", msg->header.message_type, decode_result);
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
 ** Name:  esm_msg_encode()                                          **
 **                                                                        **
 ** Description: Encode EPS Session Management messages                    **
 **                                                                        **
 ** Inputs:  msg:   The ESM message structure to encode        **
 **      length:  Maximal capacity of the output buffer      **
 **    Others:  None                                       **
 **                                                                        **
 ** Outputs:   buffer:  Pointer to the encoded data buffer         **
 **      Return:  The number of bytes in the buffer if data  **
 **       have been successfully encoded;            **
 **       A negative error code otherwise.           **
 **    Others:  None                                       **
 **                                                                        **
 ***************************************************************************/
int esm_msg_encode(ESM_msg *msg, uint8_t *buffer, uint32_t len)
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

  /* First encode the ESM message header */
  header_result = _esm_msg_encode_header(&msg->header, buffer, len);

  if (header_result < 0) {
    LOG_TRACE(ERROR, "ESM-MSG   - Failed to encode ESM message header "
              "(%d)", header_result);
    LOG_FUNC_RETURN(header_result);
  }

  LOG_TRACE(INFO, "ESM-MSG   - Encoded ESM message header "
            "(%d)", header_result);

  buffer += header_result;
  len -= header_result;

  switch(msg->header.message_type) {
  case PDN_DISCONNECT_REQUEST:
    encode_result = encode_pdn_disconnect_request(&msg->pdn_disconnect_request, buffer, len);
    break;

  case DEACTIVATE_EPS_BEARER_CONTEXT_ACCEPT:
    encode_result = encode_deactivate_eps_bearer_context_accept(&msg->deactivate_eps_bearer_context_accept, buffer, len);
    break;

  case BEARER_RESOURCE_ALLOCATION_REQUEST:
    encode_result = encode_bearer_resource_allocation_request(&msg->bearer_resource_allocation_request, buffer, len);
    break;

  case ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_ACCEPT:
    encode_result = encode_activate_default_eps_bearer_context_accept(&msg->activate_default_eps_bearer_context_accept, buffer, len);
    break;

  case PDN_CONNECTIVITY_REJECT:
    encode_result = encode_pdn_connectivity_reject(&msg->pdn_connectivity_reject, buffer, len);
    break;

  case MODIFY_EPS_BEARER_CONTEXT_REJECT:
    encode_result = encode_modify_eps_bearer_context_reject(&msg->modify_eps_bearer_context_reject, buffer, len);
    break;

  case ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_REJECT:
    encode_result = encode_activate_dedicated_eps_bearer_context_reject(&msg->activate_dedicated_eps_bearer_context_reject, buffer, len);
    break;

  case MODIFY_EPS_BEARER_CONTEXT_ACCEPT:
    encode_result = encode_modify_eps_bearer_context_accept(&msg->modify_eps_bearer_context_accept, buffer, len);
    break;

  case DEACTIVATE_EPS_BEARER_CONTEXT_REQUEST:
    encode_result = encode_deactivate_eps_bearer_context_request(&msg->deactivate_eps_bearer_context_request, buffer, len);
    break;

  case ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_ACCEPT:
    encode_result = encode_activate_dedicated_eps_bearer_context_accept(&msg->activate_dedicated_eps_bearer_context_accept, buffer, len);
    break;

  case ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REJECT:
    encode_result = encode_activate_default_eps_bearer_context_reject(&msg->activate_default_eps_bearer_context_reject, buffer, len);
    break;

  case MODIFY_EPS_BEARER_CONTEXT_REQUEST:
    encode_result = encode_modify_eps_bearer_context_request(&msg->modify_eps_bearer_context_request, buffer, len);
    break;

  case PDN_DISCONNECT_REJECT:
    encode_result = encode_pdn_disconnect_reject(&msg->pdn_disconnect_reject, buffer, len);
    break;

  case ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_REQUEST:
    encode_result = encode_activate_dedicated_eps_bearer_context_request(&msg->activate_dedicated_eps_bearer_context_request, buffer, len);
    break;

  case BEARER_RESOURCE_MODIFICATION_REJECT:
    encode_result = encode_bearer_resource_modification_reject(&msg->bearer_resource_modification_reject, buffer, len);
    break;

  case BEARER_RESOURCE_ALLOCATION_REJECT:
    encode_result = encode_bearer_resource_allocation_reject(&msg->bearer_resource_allocation_reject, buffer, len);
    break;

  case ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST:
    encode_result = encode_activate_default_eps_bearer_context_request(&msg->activate_default_eps_bearer_context_request, buffer, len);
    break;

  case PDN_CONNECTIVITY_REQUEST:
    encode_result = encode_pdn_connectivity_request(&msg->pdn_connectivity_request, buffer, len);
    break;

  case ESM_INFORMATION_RESPONSE:
    encode_result = encode_esm_information_response(&msg->esm_information_response, buffer, len);
    break;

  case BEARER_RESOURCE_MODIFICATION_REQUEST:
    encode_result = encode_bearer_resource_modification_request(&msg->bearer_resource_modification_request, buffer, len);
    break;

  case ESM_INFORMATION_REQUEST:
    encode_result = encode_esm_information_request(&msg->esm_information_request, buffer, len);
    break;

  case ESM_STATUS:
    encode_result = encode_esm_status(&msg->esm_status, buffer, len);
    break;

  default:
    LOG_TRACE(ERROR, "ESM-MSG   - Unexpected message type: 0x%x",
              msg->header.message_type);
    encode_result = TLV_ENCODE_WRONG_MESSAGE_TYPE;
    break;
  }

  if (encode_result < 0) {
    LOG_TRACE(ERROR, "ESM-MSG   - Failed to encode L3 ESM message 0x%x "
              "(%d)", msg->header.message_type, encode_result);
  } else {
#if ((defined(NAS_BUILT_IN_EPC) && defined(NAS_MME)) || (defined(ENABLE_NAS_UE_LOGGING) && defined(NAS_BUILT_IN_UE) && defined(NAS_UE)))
    nas_itti_plain_msg((char *) buffer_log, (nas_message_t *) msg, header_result + encode_result, down_link);
#endif
  }

  LOG_FUNC_RETURN (header_result + encode_result);
}

/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/

/****************************************************************************
 **                                                                        **
 ** Name:  _esm_msg_decode_header()                                  **
 **                                                                        **
 ** Description: Decode header of EPS Mobility Management message.         **
 **    The protocol discriminator and the security header type   **
 **    have already been decoded.                                **
 **                                                                        **
 ** Inputs:  buffer:  Pointer to the buffer containing the ESM   **
 **       message                                    **
 **      len:   Number of bytes that should be decoded     **
 **    Others:  None                                       **
 **                                                                        **
 ** Outputs:   header:  The ESM message header to be filled        **
 **      Return:  The size of the header if data have been   **
 **       successfully decoded;                      **
 **       A negative error code otherwise.           **
 **    Others:  None                                       **
 **                                                                        **
 ***************************************************************************/
static int _esm_msg_decode_header(esm_msg_header_t *header,
                                  const uint8_t *buffer, uint32_t len)
{
  int size = 0;

  /* Check the buffer length */
  if (len < sizeof(esm_msg_header_t)) {
    return (TLV_DECODE_BUFFER_TOO_SHORT);
  }

  /* Decode the EPS bearer identity and the protocol discriminator */
  DECODE_U8(buffer + size, *(uint8_t*)(header), size);
  /* Decode the procedure transaction identity */
  DECODE_U8(buffer + size, header->procedure_transaction_identity, size);
  /* Decode the message type */
  DECODE_U8(buffer + size, header->message_type, size);

  /* Check the protocol discriminator */
  if (header->protocol_discriminator != EPS_SESSION_MANAGEMENT_MESSAGE) {
    LOG_TRACE(ERROR, "ESM-MSG   - Unexpected protocol discriminator: 0x%x",
              header->protocol_discriminator);
    return (TLV_DECODE_PROTOCOL_NOT_SUPPORTED);
  }

  return (size);
}

/****************************************************************************
 **                                                                        **
 ** Name:  _esm_msg_encode_header()                                  **
 **                                                                        **
 **    The protocol discriminator and the security header type   **
 **    have already been encoded.                                **
 **                                                                        **
 ** Inputs:  header:  The ESM message header to encode           **
 **      len:   Maximal capacity of the output buffer      **
 **    Others:  None                                       **
 **                                                                        **
 ** Outputs:   buffer:  Pointer to the encoded data buffer         **
 **      Return:  The number of bytes in the buffer if data  **
 **       have been successfully encoded;            **
 **       A negative error code otherwise.           **
 **    Others:  None                                       **
 **                                                                        **
 ***************************************************************************/
static int _esm_msg_encode_header(const esm_msg_header_t *header,
                                  uint8_t *buffer, uint32_t len)
{
  int size = 0;

  /* Check the buffer length */
  if (len < sizeof(esm_msg_header_t)) {
    LOG_TRACE(ERROR, "ESM-MSG   - buffer too short");
    return (TLV_ENCODE_BUFFER_TOO_SHORT);
  }
  /* Check the protocol discriminator */
  else if (header->protocol_discriminator != EPS_SESSION_MANAGEMENT_MESSAGE) {
    LOG_TRACE(ERROR, "ESM-MSG   - Unexpected protocol discriminator: 0x%x",
              header->protocol_discriminator);
    return (TLV_ENCODE_PROTOCOL_NOT_SUPPORTED);
  }

  /* Encode the EPS bearer identity and the protocol discriminator */
  ENCODE_U8(buffer + size, *(uint8_t*)(header), size);
  /* Encode the procedure transaction identity */
  ENCODE_U8(buffer + size, header->procedure_transaction_identity, size);
  /* Encode the message type */
  ENCODE_U8(buffer + size, header->message_type, size);

  return (size);
}

