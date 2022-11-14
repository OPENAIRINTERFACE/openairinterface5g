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

/*! \file common/utils/websrv/websrv_websockets.c
 * \brief: implementation of web/websockets API
 * \author Francois TABURET
 * \date 2022
 * \version 0.1
 * \company NOKIA BellLabs France
 * \email: francois.taburet@nokia-bell-labs.com
 * \note
 * \warning
 */

#include <gnutls/gnutls.h>
#include <gnutls/x509.h>
#include <arpa/inet.h>
#include <errno.h>
#include <jansson.h>
#include <ulfius.h>
#include "common/utils/LOG/log.h"
#include "common/utils/websrv/websrv.h"

#include "time.h"

void websrv_websocket_send_message(char msg_src, char msg_type, char *msg_data, struct _websocket_manager *websocket_manager)
{
  websrv_msg_t msg;
  int st;
  msg.header.src = msg_src;
  msg.header.msgtype = msg_type;
  sprintf(msg.data, "%s", msg_data);
  int len = strlen(msg.data) + WEBSOCK_HEADSIZE;
  st = ulfius_websocket_send_message(websocket_manager, U_WEBSOCKET_OPCODE_BINARY, len, (char *)&msg);
  if (st != U_OK)
    LOG_I(UTIL, "Error sending scope message, length %i status %i\n", len, st);
}
/* websocket callbacks as set in callback_websocket, the initial url endpoint which triggers the websocket init */
/* function executed by ulfius when websocket is closed */
void websrv_websocket_onclose_callback(const struct _u_request *request, struct _websocket_manager *websocket_manager, void *websocket_onclose_user_data)
{
  websrv_params_t *websrvparams = (websrv_params_t *)websocket_onclose_user_data;
  websrv_dump_request("websocket close ", request, websrvparams->dbglvl);
  websrvparams->wm = NULL;
  websrv_scope_stop();
}

void websrv_close_ws(websrv_params_t *websrvparams, char *source)
{
  LOG_W(UTIL, "[websrv] Websocket re-init, from %s....\n", source);
  ulfius_websocket_send_close_signal(websrvparams->wm);
}

/* function executed by ulfius in a dedicated thread, should not terminate while client connection is up */
void websrv_websocket_manager_callback(const struct _u_request *request, struct _websocket_manager *websocket_manager, void *websocket_manager_user_data)
{
  websrv_params_t *websrvparams = (websrv_params_t *)websocket_manager_user_data;
  websrv_dump_request("websocket manager ", request, websrvparams->dbglvl);
  while (websrvparams->wm != NULL) {
    LOG_I(UTIL, "[websrv] Waitting a previous websocket instance to close...\n");
    usleep(100000);
  }
  websrvparams->wm = websocket_manager;
  uint64_t lcount = 0;
  int st = U_WEBSOCKET_STATUS_OPEN;
  while (st == U_WEBSOCKET_STATUS_OPEN) {
    st = ulfius_websocket_status(websocket_manager /* ms */);
    if (st == U_WEBSOCKET_STATUS_OPEN) {
      if (websrv_scope_manager(lcount, websrvparams) < 0) {
        websrv_close_ws(websrvparams, "ws scope manager");
      };
      lcount++;
    } else if (st == U_WEBSOCKET_STATUS_ERROR) {
      LOG_I(UTIL, "[websrv] manager: Websocket error, errno %s\n", strerror(errno));
      break;
    } else {
      LOG_I(UTIL, "[websrv] manager: Websocket has been  closed\n");
      break;
    }
    usleep(100000);
  } /* while */
  LOG_I(UTIL, "Closing websocket_manager_callback...\n");
}

void websrv_websocket_incoming_message_callback(const struct _u_request *request,
                                                struct _websocket_manager *websocket_manager,
                                                const struct _websocket_message *last_message,
                                                void *websocket_incoming_message_user_data)
{
  LOG_I(UTIL, "Incoming message,  opcode: 0x%02x, mask: %d, len: %zu\n", last_message->opcode, last_message->has_mask, last_message->data_len);
  websrv_params_t *websrvparams = (websrv_params_t *)websocket_incoming_message_user_data;
  if (websrvparams->wm != NULL && websrvparams->wm != websocket_manager) {
    websrv_close_ws(websrvparams, "ws incoming message callback");
  }
  if (last_message->opcode == U_WEBSOCKET_OPCODE_TEXT) {
    LOG_I(UTIL, "[websrv] text payload '%.*s'", (int)last_message->data_len, last_message->data);
  } else if (last_message->opcode == U_WEBSOCKET_OPCODE_BINARY) {
    websrv_msg_t *msg = (websrv_msg_t *)last_message->data;
    LOG_I(UTIL, "[websrv] binary payload from %c type %i\n", msg->header.src, (int)msg->header.msgtype);
    switch (msg->header.src) {
      case 's':
        websrv_websocket_process_scopemessage(msg->header.msgtype, msg->data, websocket_manager);
        break;
      default:
        LOG_W(UTIL, "[websrv] Unknown message source: %c\n", msg->header.src);
        break;
    }
  } else if (last_message->opcode == U_WEBSOCKET_OPCODE_CLOSE) {
    LOG_I(UTIL, "[websrv] websocket closed\n");

  } else {
    LOG_I(UTIL, "[websrv] message ignored\n");
  }
}

/**
 * callback function, called when the url corresponding to the endpoint set in
 * websrv_init_websocket is requested. that simply set  the websocket callbacks
 */
int websrv_callback_websocket(const struct _u_request *request, struct _u_response *response, void *user_data)
{
  int ret;

  websrv_params_t *websrvparams = (websrv_params_t *)user_data;
  websrv_dump_request("websocket ", request, websrvparams->dbglvl);
  if (websrvparams->wm != NULL) {
    websrv_close_ws(websrvparams, "ws url callback");
  }
  if ((ret = ulfius_set_websocket_response(
           response, NULL, NULL, websrv_websocket_manager_callback, user_data, websrv_websocket_incoming_message_callback, user_data, websrv_websocket_onclose_callback, user_data))
      == U_OK) {
    return U_CALLBACK_COMPLETE;
  } else {
    return U_CALLBACK_ERROR;
  }
}

int websrv_init_websocket(websrv_params_t *websrvparams, char *module)
{
  int status = ulfius_add_endpoint_by_val(&(websrvparams->instance), "GET", NULL, module, 1, &websrv_callback_websocket, websrvparams);
  return status;
}
