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

/*! \file gtpv1u_task.c
* \brief
* \author Sebastien ROUX, Lionel Gauthier
* \company Eurecom
* \email: lionel.gauthier@eurecom.fr
*/
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>

#include "mme_config.h"

#include "assertions.h"
#include "intertask_interface.h"
#include "timer.h"

#include "gtpv1u.h"
#include "NwGtpv1u.h"
#include "NwGtpv1uMsg.h"
#include "NwLog.h"
#include "gtpv1u_sgw_defs.h"
#include "NwGtpv1uPrivate.h"
#include "msc.h"

//static NwGtpv1uStackHandleT gtpv1u_stack = 0;
static gtpv1u_data_t        gtpv1u_sgw_data;


static int gtpv1u_create_s1u_tunnel(Gtpv1uCreateTunnelReq *create_tunnel_reqP);
static int gtpv1u_delete_s1u_tunnel(Teid_t context_teidP, Teid_t S1U_teidP);
static int gtpv1u_update_s1u_tunnel(Gtpv1uUpdateTunnelReq *reqP);
static void *gtpv1u_thread(void *args);

NwGtpv1uRcT gtpv1u_send_udp_msg(
  NwGtpv1uUdpHandleT udpHandle,
  uint8_t *buffer,
  uint32_t buffer_len,
  uint32_t buffer_offset,
  uint32_t peerIpAddr,
  uint32_t peerPort);

NwGtpv1uRcT gtpv1u_log_request(
  NwGtpv1uLogMgrHandleT hLogMgr,
  uint32_t logLevel,
  NwCharT *file,
  uint32_t line,
  NwCharT *logStr);

NwGtpv1uRcT gtpv1u_process_stack_req(
  NwGtpv1uUlpHandleT hUlp,
  NwGtpv1uUlpApiT *pUlpApi);


//-----------------------------------------------------------------------------
void gtpu_print_hex_octets(unsigned char* dataP, unsigned long sizeP)
//-----------------------------------------------------------------------------
{
  unsigned long octet_index = 0;
  unsigned long buffer_marker = 0;
  unsigned char aindex;
#define GTPU_2_PRINT_BUFFER_LEN 8000
  char gtpu_2_print_buffer[GTPU_2_PRINT_BUFFER_LEN];
  struct timeval tv;
  struct timezone tz;
  char timeofday[64];
  unsigned int h,m,s;

  if (dataP == NULL) {
    return;
  }

  gettimeofday(&tv, &tz);
  h = tv.tv_sec/3600/24;
  m = (tv.tv_sec / 60) % 60;
  s = tv.tv_sec % 60;
  snprintf(timeofday, 64, "%02d:%02d:%02d.%06d", h,m,s,tv.tv_usec);

  GTPU_DEBUG("%s------+-------------------------------------------------|\n",timeofday);
  GTPU_DEBUG("%s      |  0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f |\n",timeofday);
  GTPU_DEBUG("%s------+-------------------------------------------------|\n",timeofday);

  for (octet_index = 0; octet_index < sizeP; octet_index++) {
    if (GTPU_2_PRINT_BUFFER_LEN < (buffer_marker + 32))  {
      buffer_marker+=snprintf(&gtpu_2_print_buffer[buffer_marker], GTPU_2_PRINT_BUFFER_LEN - buffer_marker,
                              "... (print buffer overflow)");
      GTPU_DEBUG("%s%s",timeofday,gtpu_2_print_buffer);
      return;
    }

    if ((octet_index % 16) == 0) {
      if (octet_index != 0) {
        buffer_marker+=snprintf(&gtpu_2_print_buffer[buffer_marker], GTPU_2_PRINT_BUFFER_LEN - buffer_marker, " |\n");
        GTPU_DEBUG("%s%s",timeofday, gtpu_2_print_buffer);
        buffer_marker = 0;
      }

      buffer_marker+=snprintf(&gtpu_2_print_buffer[buffer_marker], GTPU_2_PRINT_BUFFER_LEN - buffer_marker, " %04ld |", octet_index);
    }

    /*
     * Print every single octet in hexadecimal form
     */
    buffer_marker+=snprintf(&gtpu_2_print_buffer[buffer_marker], GTPU_2_PRINT_BUFFER_LEN - buffer_marker, " %02x", dataP[octet_index]);
    /*
     * Align newline and pipes according to the octets in groups of 2
     */
  }

  /*
   * Append enough spaces and put final pipe
   */
  for (aindex = octet_index; aindex < 16; ++aindex)
    buffer_marker+=snprintf(&gtpu_2_print_buffer[buffer_marker], GTPU_2_PRINT_BUFFER_LEN - buffer_marker, "   ");

  //GTPU_DEBUG("   ");
  buffer_marker+=snprintf(&gtpu_2_print_buffer[buffer_marker], GTPU_2_PRINT_BUFFER_LEN - buffer_marker, " |\n");
  GTPU_DEBUG("%s%s",timeofday,gtpu_2_print_buffer);
}


NwGtpv1uRcT gtpv1u_log_request(NwGtpv1uLogMgrHandleT hLogMgr,
                               uint32_t logLevel,
                               NwCharT *file,
                               uint32_t line,
                               NwCharT *logStr)
{
  GTPU_DEBUG("%s\n", logStr);
  return NW_GTPV1U_OK;
}

NwGtpv1uRcT gtpv1u_send_udp_msg(
  NwGtpv1uUdpHandleT udpHandle,
  uint8_t *buffer,
  uint32_t buffer_len,
  uint32_t buffer_offset,
  uint32_t peerIpAddr,
  uint32_t peerPort)
{
  // Create and alloc new message
  MessageDef     *message_p;
  udp_data_req_t *udp_data_req_p;

  message_p = itti_alloc_new_message(TASK_GTPV1_U, UDP_DATA_REQ);

  udp_data_req_p = &message_p->ittiMsg.udp_data_req;

  udp_data_req_p->peer_address  = peerIpAddr;
  udp_data_req_p->peer_port     = peerPort;
  udp_data_req_p->buffer        = buffer;
  udp_data_req_p->buffer_length = buffer_len;
  udp_data_req_p->buffer_offset = buffer_offset;

  return itti_send_msg_to_task(TASK_UDP, INSTANCE_DEFAULT, message_p);
}

/* Callback called when a gtpv1u message arrived on UDP interface */
NwGtpv1uRcT gtpv1u_process_stack_req(
  NwGtpv1uUlpHandleT hUlp,
  NwGtpv1uUlpApiT *pUlpApi)
{
  switch(pUlpApi->apiType) {
    /* Here there are two type of messages handled:
     * - T-PDU
     * - END-MARKER
     */
  case NW_GTPV1U_ULP_API_RECV_TPDU: {
    //uint8_t              buffer[4096];
    //uint32_t             buffer_len;
    MessageDef          *message_p  = NULL;
    Gtpv1uTunnelDataInd *data_ind_p = NULL;

    /* Nw-gptv1u stack has processed a PDU. we can forward it to IPV4
     * task for transmission.
     */
    /*if (NW_GTPV1U_OK != nwGtpv1uMsgGetTpdu(pUlpApi->apiInfo.recvMsgInfo.hMsg,
        buffer, (uint32_t *)&buffer_len)) {
        GTPU_ERROR("Error while retrieving T-PDU\n");
    }*/
    GTPU_DEBUG("Received TPDU from gtpv1u stack %u with size %d\n",
               pUlpApi->apiInfo.recvMsgInfo.teid,
               ((NwGtpv1uMsgT*)pUlpApi->apiInfo.recvMsgInfo.hMsg)->msgBufLen);

    message_p = itti_alloc_new_message(TASK_GTPV1_U, GTPV1U_TUNNEL_DATA_IND);

    if (message_p == NULL) {
      return -1;
    }

    data_ind_p                       = &message_p->ittiMsg.gtpv1uTunnelDataInd;
    data_ind_p->buffer               = ((NwGtpv1uMsgT*)pUlpApi->apiInfo.recvMsgInfo.hMsg)->msgBuf;
    data_ind_p->length               = ((NwGtpv1uMsgT*)pUlpApi->apiInfo.recvMsgInfo.hMsg)->msgBufLen;
    data_ind_p->offset               = ((NwGtpv1uMsgT*)pUlpApi->apiInfo.recvMsgInfo.hMsg)->msgBufOffset;
    data_ind_p->local_S1u_teid       = pUlpApi->apiInfo.recvMsgInfo.teid;

    if (data_ind_p->buffer == NULL) {
      GTPU_ERROR("Failed to allocate new buffer\n");
      itti_free(ITTI_MSG_ORIGIN_ID(message_p), message_p);
      message_p = NULL;
    } else {
      //memcpy(data_ind_p->buffer, buffer, buffer_len);
      //data_ind_p->length = buffer_len;
      if (itti_send_msg_to_task(TASK_FW_IP, INSTANCE_DEFAULT, message_p) < 0) {
        GTPU_ERROR("Failed to send message to task\n");
        itti_free(ITTI_MSG_ORIGIN_ID(message_p), message_p);
        message_p = NULL;
      }
    }
  }
  break;

  case NW_GTPV1U_ULP_API_CREATE_TUNNEL_ENDPOINT: {
  }
  break;

  default: {
    GTPU_ERROR("Received undefined UlpApi (%02x) from gtpv1u stack!\n",
               pUlpApi->apiType);
  }
  }

  return NW_GTPV1U_OK;
}

static int gtpv1u_create_s1u_tunnel(Gtpv1uCreateTunnelReq *create_tunnel_reqP)
{
  /* Create a new nw-gtpv1-u stack req using API */
  NwGtpv1uUlpApiT          stack_req;
  NwGtpv1uRcT              rc;
  /* Local tunnel end-point identifier */
  uint32_t                 s1u_teid             = 0;
  gtpv1u_teid2enb_info_t  *gtpv1u_teid2enb_info = NULL;
  MessageDef              *message_p            = NULL;
  hashtable_rc_t           hash_rc;

  GTPU_DEBUG("Rx GTPV1U_CREATE_TUNNEL_REQ Context %d\n", create_tunnel_reqP->context_teid);
  memset(&stack_req, 0, sizeof(NwGtpv1uUlpApiT));

  stack_req.apiType = NW_GTPV1U_ULP_API_CREATE_TUNNEL_ENDPOINT;

  do {
    s1u_teid = gtpv1u_new_teid();
    GTPU_DEBUG("gtpv1u_create_s1u_tunnel() %u\n", s1u_teid);
    stack_req.apiInfo.createTunnelEndPointInfo.teid          = s1u_teid;
    stack_req.apiInfo.createTunnelEndPointInfo.hUlpSession   = 0;
    stack_req.apiInfo.createTunnelEndPointInfo.hStackSession = 0;

    rc = nwGtpv1uProcessUlpReq(gtpv1u_sgw_data.gtpv1u_stack, &stack_req);
    GTPU_DEBUG(".\n");
  } while (rc != NW_GTPV1U_OK);

  gtpv1u_teid2enb_info = malloc (sizeof(gtpv1u_teid2enb_info_t));
  memset(gtpv1u_teid2enb_info, 0, sizeof(gtpv1u_teid2enb_info_t));
  gtpv1u_teid2enb_info->state       = BEARER_IN_CONFIG;

  //#warning !!! hack because missing modify session request, so force enb address
  //    gtpv1u_teid2enb_info->enb_ip_addr.pdn_type = IPv4;
  //    gtpv1u_teid2enb_info->enb_ip_addr.address.ipv4_address[0] = 192;
  //    gtpv1u_teid2enb_info->enb_ip_addr.address.ipv4_address[1] = 168;
  //    gtpv1u_teid2enb_info->enb_ip_addr.address.ipv4_address[2] = 1;
  //    gtpv1u_teid2enb_info->enb_ip_addr.address.ipv4_address[3] = 2;


  message_p = itti_alloc_new_message(TASK_GTPV1_U, GTPV1U_CREATE_TUNNEL_RESP);
  message_p->ittiMsg.gtpv1uCreateTunnelResp.S1u_teid      = s1u_teid;
  message_p->ittiMsg.gtpv1uCreateTunnelResp.context_teid  = create_tunnel_reqP->context_teid;
  message_p->ittiMsg.gtpv1uCreateTunnelResp.eps_bearer_id = create_tunnel_reqP->eps_bearer_id;

  hash_rc = hashtable_is_key_exists(gtpv1u_sgw_data.S1U_mapping, s1u_teid);

  if (hash_rc == HASH_TABLE_KEY_NOT_EXISTS) {
    hash_rc = hashtable_insert(gtpv1u_sgw_data.S1U_mapping, s1u_teid, gtpv1u_teid2enb_info);
    message_p->ittiMsg.gtpv1uCreateTunnelResp.status       = 0;
  } else {
    message_p->ittiMsg.gtpv1uCreateTunnelResp.status       = 0xFF;
  }

  GTPU_DEBUG("Tx GTPV1U_CREATE_TUNNEL_RESP Context %u teid %u eps bearer id %u status %d\n",
             message_p->ittiMsg.gtpv1uCreateTunnelResp.context_teid,
             message_p->ittiMsg.gtpv1uCreateTunnelResp.S1u_teid,
             message_p->ittiMsg.gtpv1uCreateTunnelResp.eps_bearer_id,
             message_p->ittiMsg.gtpv1uCreateTunnelResp.status);
  return itti_send_msg_to_task(TASK_SPGW_APP, INSTANCE_DEFAULT, message_p);
}



static int gtpv1u_delete_s1u_tunnel(Teid_t context_teidP, Teid_t S1U_teidP)
{
  /* Local tunnel end-point identifier */
  MessageDef              *message_p;

  GTPU_DEBUG("Rx GTPV1U_DELETE_TUNNEL Context %u S1U teid %u\n", context_teidP, S1U_teidP);
  message_p = itti_alloc_new_message(TASK_GTPV1_U, GTPV1U_DELETE_TUNNEL_RESP);

  message_p->ittiMsg.gtpv1uDeleteTunnelResp.S1u_teid     = S1U_teidP;
  message_p->ittiMsg.gtpv1uDeleteTunnelResp.context_teid = context_teidP;

  if (hashtable_remove(gtpv1u_sgw_data.S1U_mapping, S1U_teidP) == HASH_TABLE_OK ) {
    message_p->ittiMsg.gtpv1uDeleteTunnelResp.status       = 0;
  } else {
    message_p->ittiMsg.gtpv1uDeleteTunnelResp.status       = -1;
  }

  GTPU_DEBUG("Tx SGW_S1U_ENDPOINT_CREATED Context %u teid %u status %d\n", context_teidP, S1U_teidP, message_p->ittiMsg.gtpv1uDeleteTunnelResp.status);
  return itti_send_msg_to_task(TASK_SPGW_APP, INSTANCE_DEFAULT, message_p);
}


static int gtpv1u_update_s1u_tunnel(Gtpv1uUpdateTunnelReq *reqP)
{
  hashtable_rc_t           hash_rc;
  gtpv1u_teid2enb_info_t  *gtpv1u_teid2enb_info;
  MessageDef              *message_p;

  GTPU_DEBUG("Rx GTPV1U_UPDATE_TUNNEL_REQ Context %u, S-GW S1U teid %u, eNB S1U teid %u\n",
             reqP->context_teid,
             reqP->sgw_S1u_teid,
             reqP->enb_S1u_teid);
  message_p = itti_alloc_new_message(TASK_GTPV1_U, GTPV1U_UPDATE_TUNNEL_RESP);

  hash_rc = hashtable_get(gtpv1u_sgw_data.S1U_mapping, reqP->sgw_S1u_teid, (void**)&gtpv1u_teid2enb_info);

  if (hash_rc == HASH_TABLE_OK) {
    gtpv1u_teid2enb_info->teid_enb    = reqP->enb_S1u_teid;
    gtpv1u_teid2enb_info->enb_ip_addr = reqP->enb_ip_address_for_S1u;
    gtpv1u_teid2enb_info->state       = BEARER_UP;
    gtpv1u_teid2enb_info->port        = GTPV1U_UDP_PORT;
    message_p->ittiMsg.gtpv1uUpdateTunnelResp.status = 0;           ///< Status (Failed = 0xFF or Success = 0x0)
  } else {
    GTPU_ERROR("Mapping not found\n");
    message_p->ittiMsg.gtpv1uUpdateTunnelResp.status = 0xFF;           ///< Status (Failed = 0xFF or Success = 0x0)
  }

  message_p->ittiMsg.gtpv1uUpdateTunnelResp.context_teid  = reqP->context_teid;
  message_p->ittiMsg.gtpv1uUpdateTunnelResp.sgw_S1u_teid  = reqP->sgw_S1u_teid;
  message_p->ittiMsg.gtpv1uUpdateTunnelResp.enb_S1u_teid  = reqP->enb_S1u_teid;
  message_p->ittiMsg.gtpv1uUpdateTunnelResp.eps_bearer_id = reqP->eps_bearer_id;
  return itti_send_msg_to_task(TASK_SPGW_APP, INSTANCE_DEFAULT, message_p);
}


static NwGtpv1uRcT gtpv1u_start_timer_wrapper(
  NwGtpv1uTimerMgrHandleT tmrMgrHandle,
  uint32_t                  timeoutSec,
  uint32_t                  timeoutUsec,
  uint32_t                  tmrType,
  void                   *timeoutArg,
  NwGtpv1uTimerHandleT   *hTmr)
{

  NwGtpv1uRcT rc = NW_GTPV1U_OK;
  long        timer_id;

  if (tmrType == NW_GTPV1U_TMR_TYPE_ONE_SHOT) {
    timer_setup(timeoutSec,
                timeoutUsec,
                TASK_GTPV1_U,
                INSTANCE_DEFAULT,
                TIMER_ONE_SHOT,
                timeoutArg,
                &timer_id);
  } else {
    timer_setup(timeoutSec,
                timeoutUsec,
                TASK_GTPV1_U,
                INSTANCE_DEFAULT,
                TIMER_PERIODIC,
                timeoutArg,
                &timer_id);
  }

  return rc;
}

static NwGtpv1uRcT gtpv1u_stop_timer_wrapper(
  NwGtpv1uTimerMgrHandleT tmrMgrHandle,
  NwGtpv1uTimerHandleT hTmr)
{

  NwGtpv1uRcT rc = NW_GTPV1U_OK;

  return rc;
}

static void *gtpv1u_thread(void *args)
{
  itti_mark_task_ready(TASK_GTPV1_U);
  MSC_START_USE();

  while(1) {
    /* Trying to fetch a message from the message queue.
     * If the queue is empty, this function will block till a
     * message is sent to the task.
     */
    MessageDef *received_message_p = NULL;
    itti_receive_msg(TASK_GTPV1_U, &received_message_p);
    DevAssert(received_message_p != NULL);


    switch (ITTI_MSG_ID(received_message_p)) {

    case TERMINATE_MESSAGE: {
      GTPU_WARN(" *** Exiting GTPU thread\n");
      itti_exit_task();
    }
    break;

    // DATA COMING FROM UDP
    case UDP_DATA_IND: {
      udp_data_ind_t *udp_data_ind_p;
      udp_data_ind_p = &received_message_p->ittiMsg.udp_data_ind;
      nwGtpv1uProcessUdpReq(gtpv1u_sgw_data.gtpv1u_stack,
                             udp_data_ind_p->buffer,
                             udp_data_ind_p->buffer_length,
                             udp_data_ind_p->peer_port,
                             udp_data_ind_p->peer_address);
       //itti_free(ITTI_MSG_ORIGIN_ID(received_message_p), udp_data_ind_p->buffer);
    }
    break;

    case TIMER_HAS_EXPIRED:
      nwGtpv1uProcessTimeout(&received_message_p->ittiMsg.timer_has_expired.arg);
      break;

    default: {
      GTPU_ERROR("Unkwnon message ID %d:%s\n",
                 ITTI_MSG_ID(received_message_p),
                 ITTI_MSG_NAME(received_message_p));
    }
    break;
    }

    itti_free(ITTI_MSG_ORIGIN_ID(received_message_p), received_message_p);
    received_message_p = NULL;
  }

  return NULL;
}

int gtpv1u_init(const mme_config_t *mme_config_p)
{
  int                     ret = 0;
  NwGtpv1uRcT             rc  = 0;
  NwGtpv1uUlpEntityT      ulp;
  NwGtpv1uUdpEntityT      udp;
  NwGtpv1uLogMgrEntityT   log;
  NwGtpv1uTimerMgrEntityT tmr;

  GTPU_DEBUG("Initializing GTPV1U interface\n");

  memset(&gtpv1u_sgw_data, 0, sizeof(gtpv1u_sgw_data));

  gtpv1u_sgw_data.sgw_ip_address_for_S1u_S12_S4_up = mme_config_p->ipv4.sgw_ip_address_for_S1u_S12_S4_up;

  if (itti_create_task(TASK_GTPV1_U, &gtpv1u_thread, NULL) < 0) {
    GTPU_ERROR("gtpv1u phtread_create: %s", strerror(errno));
    return -1;
  }

  GTPU_DEBUG("Initializing GTPV1U interface: DONE\n");

  return ret;
}
