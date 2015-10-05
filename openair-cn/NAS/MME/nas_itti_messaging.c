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
#include <string.h>

#include "intertask_interface.h"
#include "nas_itti_messaging.h"
#include "msc.h"


#define TASK_ORIGIN  TASK_NAS_MME


#if defined(NAS_BUILT_IN_EPC)
static const uint8_t emm_message_ids[] = {
  ATTACH_REQUEST,
  ATTACH_ACCEPT,
  ATTACH_COMPLETE,
  ATTACH_REJECT,
  DETACH_REQUEST,
  DETACH_ACCEPT,
  TRACKING_AREA_UPDATE_REQUEST,
  TRACKING_AREA_UPDATE_ACCEPT,
  TRACKING_AREA_UPDATE_COMPLETE,
  TRACKING_AREA_UPDATE_REJECT,
  EXTENDED_SERVICE_REQUEST,
  SERVICE_REQUEST,
  SERVICE_REJECT,
  GUTI_REALLOCATION_COMMAND,
  GUTI_REALLOCATION_COMPLETE,
  AUTHENTICATION_REQUEST,
  AUTHENTICATION_RESPONSE,
  AUTHENTICATION_REJECT,
  AUTHENTICATION_FAILURE,
  IDENTITY_REQUEST,
  IDENTITY_RESPONSE,
  SECURITY_MODE_COMMAND,
  SECURITY_MODE_COMPLETE,
  SECURITY_MODE_REJECT,
  EMM_STATUS,
  EMM_INFORMATION,
  DOWNLINK_NAS_TRANSPORT,
  UPLINK_NAS_TRANSPORT,
  CS_SERVICE_NOTIFICATION,
};

static const uint8_t esm_message_ids[] = {
  ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST,
  ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_ACCEPT,
  ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REJECT,
  ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_REQUEST,
  ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_ACCEPT,
  ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_REJECT,
  MODIFY_EPS_BEARER_CONTEXT_REQUEST,
  MODIFY_EPS_BEARER_CONTEXT_ACCEPT,
  MODIFY_EPS_BEARER_CONTEXT_REJECT,
  DEACTIVATE_EPS_BEARER_CONTEXT_REQUEST,
  DEACTIVATE_EPS_BEARER_CONTEXT_ACCEPT,
  PDN_CONNECTIVITY_REQUEST,
  PDN_CONNECTIVITY_REJECT,
  PDN_DISCONNECT_REQUEST,
  PDN_DISCONNECT_REJECT,
  BEARER_RESOURCE_ALLOCATION_REQUEST,
  BEARER_RESOURCE_ALLOCATION_REJECT,
  BEARER_RESOURCE_MODIFICATION_REQUEST,
  BEARER_RESOURCE_MODIFICATION_REJECT,
  ESM_INFORMATION_REQUEST,
  ESM_INFORMATION_RESPONSE,
  ESM_STATUS,
};

static int _nas_find_message_index(const uint8_t message_id, const uint8_t *message_ids, const int ids_number)
{
  int i;

  for(i = 0; i < ids_number; i ++) {
    if (message_id == message_ids[i]) {
      return (2 + i);
    }
  }

  return (1);
}

int nas_itti_plain_msg(const char *buffer, const nas_message_t *msg, const int length, const int down_link)
{
  MessageDef *message_p;
  int data_length = length < NAS_DATA_LENGHT_MAX ? length : NAS_DATA_LENGHT_MAX;
  int message_type = -1;
  MessagesIds messageId_raw = -1;
  MessagesIds messageId_plain = -1;

  /* Define message ids */
  if (msg->header.protocol_discriminator == EPS_MOBILITY_MANAGEMENT_MESSAGE) {
    message_type    = 0;
    messageId_raw   = down_link ? NAS_DL_EMM_RAW_MSG : NAS_UL_EMM_RAW_MSG;
    messageId_plain = down_link ? NAS_DL_EMM_PLAIN_MSG : NAS_UL_EMM_PLAIN_MSG;
  } else {
    if (msg->header.protocol_discriminator == EPS_SESSION_MANAGEMENT_MESSAGE) {
      message_type    = 1;
      messageId_raw   = down_link ? NAS_DL_ESM_RAW_MSG : NAS_UL_ESM_RAW_MSG;
      messageId_plain = down_link ? NAS_DL_ESM_PLAIN_MSG : NAS_UL_ESM_PLAIN_MSG;
    }
  }

  if (message_type >= 0) {
    /* Create and send the RAW message */
    message_p = itti_alloc_new_message(TASK_ORIGIN, messageId_raw);

    NAS_DL_EMM_RAW_MSG(message_p).lenght = length;
    memset ((void *) &(NAS_DL_EMM_RAW_MSG(message_p).data), 0, NAS_DATA_LENGHT_MAX);
    memcpy ((void *) &(NAS_DL_EMM_RAW_MSG(message_p).data), buffer, data_length);

    itti_send_msg_to_task(TASK_UNKNOWN, INSTANCE_DEFAULT, message_p);

    /* Create and send the plain message */
    if (message_type == 0) {
      message_p = itti_alloc_new_message(TASK_ORIGIN, messageId_plain);

      NAS_DL_EMM_PLAIN_MSG(message_p).present = _nas_find_message_index(msg->plain.emm.header.message_type, emm_message_ids, sizeof(emm_message_ids) / sizeof(emm_message_ids[0]));
      memcpy ((void *) &(NAS_DL_EMM_PLAIN_MSG(message_p).choice), &msg->plain.emm, sizeof (EMM_msg));
    } else {
      message_p = itti_alloc_new_message(TASK_ORIGIN, messageId_plain);

      NAS_DL_ESM_PLAIN_MSG(message_p).present = _nas_find_message_index(msg->plain.esm.header.message_type, esm_message_ids, sizeof(esm_message_ids) / sizeof(esm_message_ids[0]));
      memcpy ((void *) &(NAS_DL_ESM_PLAIN_MSG(message_p).choice), &msg->plain.esm, sizeof (ESM_msg));
    }

    return itti_send_msg_to_task(TASK_UNKNOWN, INSTANCE_DEFAULT, message_p);
  }

  return EXIT_FAILURE;
}

int nas_itti_protected_msg(const char *buffer, const nas_message_t *msg, const int length, const int down_link)
{
  MessageDef *message_p = NULL;

  if (msg->header.protocol_discriminator == EPS_MOBILITY_MANAGEMENT_MESSAGE) {
    message_p = itti_alloc_new_message(TASK_ORIGIN, down_link ? NAS_DL_EMM_PROTECTED_MSG : NAS_UL_EMM_PROTECTED_MSG);

    memcpy ((void *) &(NAS_DL_EMM_PROTECTED_MSG(message_p).header), &msg->header, sizeof (nas_message_security_header_t));
    NAS_DL_EMM_PROTECTED_MSG(message_p).present = _nas_find_message_index(msg->security_protected.plain.emm.header.message_type, emm_message_ids, sizeof(emm_message_ids) / sizeof(emm_message_ids[0]));
    memcpy ((void *) &(NAS_DL_EMM_PROTECTED_MSG(message_p).choice), &msg->security_protected.plain.emm, sizeof (EMM_msg));
  } else {
    if (msg->header.protocol_discriminator == EPS_SESSION_MANAGEMENT_MESSAGE) {
      message_p = itti_alloc_new_message(TASK_ORIGIN, down_link ? NAS_DL_ESM_PROTECTED_MSG : NAS_UL_ESM_PROTECTED_MSG);

      memcpy ((void *) &(NAS_DL_ESM_PROTECTED_MSG(message_p).header), &msg->header, sizeof (nas_message_security_header_t));
      NAS_DL_ESM_PROTECTED_MSG(message_p).present =  _nas_find_message_index(msg->security_protected.plain.esm.header.message_type, esm_message_ids, sizeof(esm_message_ids) / sizeof(esm_message_ids[0]));
      memcpy ((void *) &(NAS_DL_ESM_PROTECTED_MSG(message_p).choice), &msg->security_protected.plain.esm, sizeof (ESM_msg));
    }
  }

  if (message_p != NULL) {
    return itti_send_msg_to_task(TASK_UNKNOWN, INSTANCE_DEFAULT, message_p);
  }

  return EXIT_FAILURE;
}
#endif

#if defined(NAS_BUILT_IN_EPC)
int nas_itti_dl_data_req(const uint32_t ue_id, void *const data,
                         const uint32_t length)
{
  MessageDef *message_p;

  message_p = itti_alloc_new_message(TASK_NAS_MME, NAS_DOWNLINK_DATA_REQ);

  NAS_DL_DATA_REQ(message_p).UEid          = ue_id;
  NAS_DL_DATA_REQ(message_p).nasMsg.data   = data;
  NAS_DL_DATA_REQ(message_p).nasMsg.length = length;

  MSC_LOG_TX_MESSAGE(
  	  	MSC_NAS_MME,
  	  	MSC_S1AP_MME,
  	  	NULL,0,
  	  	"0 NAS_DOWNLINK_DATA_REQ ue id "NAS_UE_ID_FMT" len %u", ue_id, length);

  return itti_send_msg_to_task(TASK_S1AP, INSTANCE_DEFAULT, message_p);
}

#endif


