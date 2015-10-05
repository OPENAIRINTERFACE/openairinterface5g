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
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>

#include "assertions.h"
#include "queue.h"

#include "mme_config.h"
#include "intertask_interface.h"

#include "timer.h"

#include "NwLog.h"
#include "NwGtpv2c.h"
#include "NwGtpv2cIe.h"
#include "NwGtpv2cMsg.h"

#include "sgw_lite_ie_defs.h"

#include "s11_common.h"
#include "s11_sgw.h"
#include "s11_sgw_bearer_manager.h"
#include "s11_sgw_session_manager.h"

static NwGtpv2cStackHandleT s11_sgw_stack_handle;

/* ULP callback for the GTPv2-C stack */
static
NwRcT s11_sgw_ulp_process_stack_req_cb(
  NwGtpv2cUlpHandleT hUlp, NwGtpv2cUlpApiT *pUlpApi)
{
  int ret = 0;

  DevAssert(pUlpApi != NULL);

  switch (pUlpApi->apiType) {
  case NW_GTPV2C_ULP_API_INITIAL_REQ_IND:
    S11_DEBUG("Received initial req indication\n");

    switch (pUlpApi->apiInfo.initialReqIndInfo.msgType) {
    case NW_GTP_CREATE_SESSION_REQ:
      ret = s11_sgw_handle_create_session_request(
              &s11_sgw_stack_handle, pUlpApi);
      break;

    case NW_GTP_MODIFY_BEARER_REQ:
      ret = s11_sgw_handle_modify_bearer_request(
              &s11_sgw_stack_handle, pUlpApi);
      break;

    case NW_GTP_DELETE_SESSION_REQ:
      ret = s11_sgw_handle_delete_session_request(
              &s11_sgw_stack_handle, pUlpApi);
      break;

    default:
      S11_WARN("Received unhandled message type %d\n",
               pUlpApi->apiInfo.initialReqIndInfo.msgType);
      break;
    }

    break;

  default:
    S11_ERROR("Received unknown stack req message %d\n",
              pUlpApi->apiType);
    break;
  }

  return ret == -1 ? NW_FAILURE : NW_OK;
}

static
NwRcT s11_sgw_send_udp_msg(
  NwGtpv2cUdpHandleT udpHandle,
  uint8_t *buffer,
  uint32_t buffer_len,
  uint32_t peerIpAddr,
  uint32_t peerPort)
{
  // Create and alloc new message
  MessageDef     *message_p;
  udp_data_req_t *udp_data_req_p;
  int ret = 0;

  message_p = itti_alloc_new_message(TASK_S11, UDP_DATA_REQ);

  udp_data_req_p = &message_p->ittiMsg.udp_data_req;

  udp_data_req_p->peer_address  = peerIpAddr;
  udp_data_req_p->peer_port     = peerPort;
  udp_data_req_p->buffer        = buffer;
  udp_data_req_p->buffer_length = buffer_len;

  ret = itti_send_msg_to_task(TASK_UDP, INSTANCE_DEFAULT, message_p);

  return ret == 0 ? NW_OK : NW_FAILURE;
}

static
NwRcT s11_sgw_log_wrapper(NwGtpv2cLogMgrHandleT hLogMgr,
                          uint32_t logLevel,
                          NwCharT* file,
                          uint32_t line,
                          NwCharT* logStr)
{
  S11_DEBUG("%s\n", logStr);
  return NW_OK;
}

static
NwRcT s11_sgw_start_timer_wrapper(
  NwGtpv2cTimerMgrHandleT tmrMgrHandle,
  uint32_t                  timeoutSec,
  uint32_t                  timeoutUsec,
  uint32_t                  tmrType,
  void                   *timeoutArg,
  NwGtpv2cTimerHandleT   *hTmr)
{
  long        timer_id;
  int         ret = 0;

  if (tmrType == NW_GTPV2C_TMR_TYPE_REPETITIVE) {
    ret = timer_setup(timeoutSec,
                      timeoutUsec,
                      TASK_S11,
                      INSTANCE_DEFAULT,
                      TIMER_PERIODIC,
                      timeoutArg,
                      &timer_id);
  } else {
    ret = timer_setup(timeoutSec,
                      timeoutUsec,
                      TASK_S11,
                      INSTANCE_DEFAULT,
                      TIMER_ONE_SHOT,
                      timeoutArg,
                      &timer_id);
  }

  return ret == 0 ? NW_OK : NW_FAILURE;
}

static
NwRcT s11_sgw_stop_timer_wrapper(
  NwGtpv2cTimerMgrHandleT tmrMgrHandle,
  NwGtpv2cTimerHandleT    tmrHandle)
{
  int ret;
  long timer_id;

  timer_id = (long)tmrHandle;

  ret = timer_remove(timer_id); //TODO

  return ret == 0 ? NW_OK : NW_FAILURE;
}

static void *s11_sgw_thread(void *args)
{
  itti_mark_task_ready(TASK_S11);

  while(1) {
    MessageDef *received_message_p = NULL;
    itti_receive_msg(TASK_S11, &received_message_p);
    assert(received_message_p != NULL);

    switch (ITTI_MSG_ID(received_message_p)) {
    case UDP_DATA_IND: {
      /* We received new data to handle from the UDP layer */
      NwRcT rc;
      udp_data_ind_t *udp_data_ind;

      udp_data_ind = &received_message_p->ittiMsg.udp_data_ind;

      S11_DEBUG("Processing new data indication from UDP\n");

      rc = nwGtpv2cProcessUdpReq(s11_sgw_stack_handle,
                                 udp_data_ind->buffer,
                                 udp_data_ind->buffer_length,
                                 udp_data_ind->peer_port,
                                 udp_data_ind->peer_address);

      DevAssert(rc == NW_OK);
    }
    break;

    case SGW_CREATE_SESSION_RESPONSE: {
      S11_DEBUG("Received create session response from S-PGW APP\n");
      s11_sgw_handle_create_session_response(
        &s11_sgw_stack_handle,
        &received_message_p->ittiMsg.sgwCreateSessionResponse);
    }
    break;

    case SGW_MODIFY_BEARER_RESPONSE: {
      S11_DEBUG("Received modify bearer response from S-PGW APP\n");
      s11_sgw_handle_modify_bearer_response(
        &s11_sgw_stack_handle,
        &received_message_p->ittiMsg.sgwModifyBearerResponse);
    }
    break;

    case SGW_DELETE_SESSION_RESPONSE: {
      S11_DEBUG("Received delete session response from S-PGW APP\n");
      s11_sgw_handle_delete_session_response(
        &s11_sgw_stack_handle,
        &received_message_p->ittiMsg.sgwDeleteSessionResponse);
    }
    break;

    case TIMER_HAS_EXPIRED: {
      S11_DEBUG("Processing timeout for timer_id 0x%lx and arg %p\n",
                received_message_p->ittiMsg.timer_has_expired.timer_id,
                received_message_p->ittiMsg.timer_has_expired.arg);
      DevAssert(nwGtpv2cProcessTimeout(received_message_p->ittiMsg.timer_has_expired.arg) == NW_OK);
    }
    break;

    default: {
      S11_ERROR("Unkwnon message ID %d:%s\n",
                ITTI_MSG_ID(received_message_p), ITTI_MSG_NAME(received_message_p));
    }
    break;
    }

    itti_free(ITTI_MSG_ORIGIN_ID(received_message_p), received_message_p);
    received_message_p = NULL;
  }

  return NULL;
}

static int s11_send_init_udp(char *address, uint16_t port_number)
{
  MessageDef *message_p;

  message_p = itti_alloc_new_message(TASK_S11, UDP_INIT);

  if (message_p == NULL) {
    return -1;
  }

  message_p->ittiMsg.udp_init.port = port_number;
  //LG message_p->ittiMsg.udpInit.address = "0.0.0.0"; //ANY address
  message_p->ittiMsg.udp_init.address = address;

  S11_DEBUG("Tx UDP_INIT IP addr %s\n", message_p->ittiMsg.udp_init.address);

  return itti_send_msg_to_task(TASK_UDP, INSTANCE_DEFAULT, message_p);
}

int s11_sgw_init(const mme_config_t *mme_config_p)
{
  int ret = 0;
  NwGtpv2cUlpEntityT      ulp;
  NwGtpv2cUdpEntityT      udp;
  NwGtpv2cTimerMgrEntityT tmrMgr;
  NwGtpv2cLogMgrEntityT   logMgr;
  struct in_addr addr;
  char *s11_address_str = NULL;

  S11_DEBUG("Initializing S11 interface\n");

  if (nwGtpv2cInitialize(&s11_sgw_stack_handle) != NW_OK) {
    S11_ERROR("Failed to initialize gtpv2-c stack\n");
    goto fail;
  }

  /* Set ULP entity */
  ulp.hUlp           = (NwGtpv2cUlpHandleT)NULL;
  ulp.ulpReqCallback = s11_sgw_ulp_process_stack_req_cb;
  DevAssert(NW_OK == nwGtpv2cSetUlpEntity(s11_sgw_stack_handle, &ulp));

  /* Set UDP entity */
  udp.hUdp               = (NwGtpv2cUdpHandleT)NULL;
  udp.udpDataReqCallback = s11_sgw_send_udp_msg;
  DevAssert(NW_OK == nwGtpv2cSetUdpEntity(s11_sgw_stack_handle, &udp));

  /* Set Timer entity */
  tmrMgr.tmrMgrHandle = (NwGtpv2cTimerMgrHandleT)NULL;
  tmrMgr.tmrStartCallback = s11_sgw_start_timer_wrapper;
  tmrMgr.tmrStopCallback  = s11_sgw_stop_timer_wrapper;
  DevAssert(NW_OK == nwGtpv2cSetTimerMgrEntity(s11_sgw_stack_handle, &tmrMgr));

  logMgr.logMgrHandle    = 0;
  logMgr.logReqCallback  = s11_sgw_log_wrapper;

  DevAssert(NW_OK == nwGtpv2cSetLogMgrEntity(s11_sgw_stack_handle, &logMgr));

  if (itti_create_task(TASK_S11, &s11_sgw_thread, NULL) < 0) {
    S11_ERROR("gtpv1u phtread_create: %s\n", strerror(errno));
    goto fail;
  }

  DevAssert(NW_OK == nwGtpv2cSetLogLevel(s11_sgw_stack_handle,
                                         NW_LOG_LEVEL_DEBG));

  config_read_lock(&mme_config);
  addr.s_addr = mme_config.ipv4.sgw_ip_address_for_S11;
  config_unlock(&mme_config);

  s11_address_str = inet_ntoa(addr);

  DevAssert(s11_address_str != NULL);

  s11_send_init_udp(s11_address_str, 2123);

  S11_DEBUG("Initializing S11 interface: DONE\n");

  return ret;

fail:
  S11_DEBUG("Initializing S11 interface: FAILURE\n");
  return -1;
}
