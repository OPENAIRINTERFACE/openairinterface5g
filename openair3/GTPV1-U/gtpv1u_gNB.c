/* Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
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

/*! \file gtpv1u_gNB.c
 * \brief
 * \author Sebastien ROUX, Lionel GAUTHIER, Navid Nikaein, Panos MATZAKOS
 * \version 1.0
 * \company Eurecom
 * \email: lionel.gauthier@eurecom.fr
 */
#include <stdio.h>
#include <errno.h>

#include "mme_config.h"
#include "intertask_interface.h"
#include "msc.h"

#include "gtpv1u.h"
#include "NwGtpv1u.h"
#include "NwGtpv1uMsg.h"
#include "NwGtpv1uPrivate.h"
#include "NwLog.h"
#include "gtpv1u_eNB_defs.h"
#include "gtpv1u_gNB_defs.h"
#include "gtpv1_u_messages_types.h"
#include "udp_eNB_task.h"
#include "common/utils/LOG/log.h"
#include "COMMON/platform_types.h"
#include "COMMON/platform_constants.h"
#include "common/utils/LOG/vcd_signal_dumper.h"
#include "common/ran_context.h"
#include "gtpv1u_eNB_task.h"
#include "gtpv1u_gNB_task.h"
#include "rrc_eNB_GTPV1U.h"

#undef GTP_DUMP_SOCKET

#undef GTPV1U_BEARER_OFFSET
#define GTPV1U_BEARER_OFFSET 1

extern unsigned char NB_eNB_INST;

extern RAN_CONTEXT_t RC;

extern NwGtpv1uRcT gtpv1u_eNB_send_udp_msg(
		  NwGtpv1uUdpHandleT udpHandle,
		  uint8_t *buffer,
		  uint32_t buffer_len,
		  uint32_t buffer_offset,
		  uint32_t peerIpAddr,
		  uint16_t peerPort);

extern NwGtpv1uRcT gtpv1u_eNB_log_request(NwGtpv1uLogMgrHandleT hLogMgr,
        uint32_t logLevel,
        NwCharT *file,
        uint32_t line,
        NwCharT *logStr);

static NwGtpv1uRcT gtpv1u_start_timer_wrapper(
  NwGtpv1uTimerMgrHandleT tmrMgrHandle,
  uint32_t                  timeoutSec,
  uint32_t                  timeoutUsec,
  uint32_t                  tmrType,
  void                   *timeoutArg,
  NwGtpv1uTimerHandleT   *hTmr) {
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


static NwGtpv1uRcT
gtpv1u_stop_timer_wrapper(
  NwGtpv1uTimerMgrHandleT tmrMgrHandle,
  NwGtpv1uTimerHandleT hTmr) {
  NwGtpv1uRcT rc = NW_GTPV1U_OK;
  return rc;
}

/* Callback called when a gtpv1u message arrived on UDP interface */
NwGtpv1uRcT gtpv1u_gNB_process_stack_req(
  NwGtpv1uUlpHandleT hUlp,
  NwGtpv1uUlpApiT   *pUlpApi) {
  boolean_t           result             = FALSE;
  teid_t              teid               = 0;
  hashtable_rc_t      hash_rc            = HASH_TABLE_KEY_NOT_EXISTS;
  gtpv1u_teid_data_t *gtpv1u_teid_data_p = NULL;
  protocol_ctxt_t     ctxt;
  NwGtpv1uRcT         rc;

  switch(pUlpApi->apiType) {
    /* Here there are two type of messages handled:
     * - T-PDU
     * - END-MARKER
     */
    case NW_GTPV1U_ULP_API_RECV_TPDU: {
      uint8_t              buffer[4096];
      uint32_t             buffer_len;
      //uint16_t             msgType = NW_GTP_GPDU;
      //NwGtpv1uMsgT     *pMsg = NULL;
      /* Nw-gptv1u stack has processed a PDU. we can schedule it to PDCP
       * for transmission.
       */
      teid = pUlpApi->apiInfo.recvMsgInfo.teid;
      //pMsg = (NwGtpv1uMsgT *) pUlpApi->apiInfo.recvMsgInfo.hMsg;
      //msgType = pMsg->msgType;

      if (NW_GTPV1U_OK != nwGtpv1uMsgGetTpdu(pUlpApi->apiInfo.recvMsgInfo.hMsg,
                                             buffer, &buffer_len)) {
        LOG_E(GTPU, "Error while retrieving T-PDU");
      }

      itti_free(TASK_UDP, ((NwGtpv1uMsgT *)pUlpApi->apiInfo.recvMsgInfo.hMsg)->msgBuf);
#if defined(GTP_DUMP_SOCKET) && GTP_DUMP_SOCKET > 0
      gtpv1u_eNB_write_dump_socket(buffer,buffer_len);
#endif
      rc = nwGtpv1uMsgDelete(RC.gtpv1u_data_g->gtpv1u_stack,
                             pUlpApi->apiInfo.recvMsgInfo.hMsg);

      if (rc != NW_GTPV1U_OK) {
        LOG_E(GTPU, "nwGtpv1uMsgDelete failed: 0x%x\n", rc);
      }

      hash_rc = hashtable_get(RC.gtpv1u_data_g->teid_mapping, teid, (void **)&gtpv1u_teid_data_p);

      if (hash_rc == HASH_TABLE_OK) {
#if defined(LOG_GTPU) && LOG_GTPU > 0
        LOG_D(GTPU, "Received T-PDU from gtpv1u stack teid  %u size %d -> enb module id %u ue module id %u rab id %u\n",
              teid,
              buffer_len,
              gtpv1u_teid_data_p->enb_id,
              gtpv1u_teid_data_p->ue_id,
              gtpv1u_teid_data_p->eps_bearer_id);
#endif
        //warning "LG eps bearer mapping to DRB id to do (offset -4)"
        PROTOCOL_CTXT_SET_BY_MODULE_ID(&ctxt, gtpv1u_teid_data_p->enb_id, ENB_FLAG_YES,  gtpv1u_teid_data_p->ue_id, 0, 0,gtpv1u_teid_data_p->enb_id);
        MSC_LOG_TX_MESSAGE(
          MSC_GTPU_ENB,
          MSC_PDCP_ENB,
          NULL,0,
          MSC_AS_TIME_FMT" DATA-REQ rb %u size %u",
          0,0,
          (gtpv1u_teid_data_p->eps_bearer_id) ? gtpv1u_teid_data_p->eps_bearer_id - 4: 5-4,
          buffer_len);

        result = pdcp_data_req(
                   &ctxt,
                   SRB_FLAG_NO,
                   (gtpv1u_teid_data_p->eps_bearer_id) ? gtpv1u_teid_data_p->eps_bearer_id - 4: 5-4,
                   0, // mui
                   SDU_CONFIRM_NO, // confirm
                   buffer_len,
                   buffer,
                   PDCP_TRANSMISSION_MODE_DATA,NULL, NULL
                 );

        if ( result == FALSE ) {
          if (ctxt.configured == FALSE )
            LOG_W(GTPU, "gNB node PDCP data request failed, cause: [UE:%x]RB is not configured!\n", ctxt.rnti) ;
          else
            LOG_W(GTPU, "PDCP data request failed\n");

          return NW_GTPV1U_FAILURE;
        }
      } else {
        LOG_W(GTPU, "Received T-PDU from gtpv1u stack teid %u unknown size %u", teid, buffer_len);
      }
    }
    break;

    default: {
      LOG_E(GTPU, "Received undefined UlpApi (%02x) from gtpv1u stack!\n",
            pUlpApi->apiType);
    }
  } // end of switch

  return NW_GTPV1U_OK;
}

int gtpv1u_gNB_init(void) {
  NwGtpv1uRcT             rc = NW_GTPV1U_FAILURE;
  NwGtpv1uUlpEntityT      ulp;
  NwGtpv1uUdpEntityT      udp;
  NwGtpv1uLogMgrEntityT   log;
  NwGtpv1uTimerMgrEntityT tmr;
  //  enb_properties_p = enb_config_get()->properties[0];
  RC.gtpv1u_data_g = (gtpv1u_data_t *)calloc(sizeof(gtpv1u_data_t),1);
  LOG_I(GTPU, "Initializing GTPU stack %p\n",&RC.gtpv1u_data_g);
  //gtpv1u_data_g.gtpv1u_stack;
  /* Initialize UE hashtable */
  RC.gtpv1u_data_g->ue_mapping      = hashtable_create (32, NULL, NULL);
  AssertFatal(RC.gtpv1u_data_g->ue_mapping != NULL, " ERROR Initializing TASK_GTPV1_U task interface: in hashtable_create returned %p\n", RC.gtpv1u_data_g->ue_mapping);
  RC.gtpv1u_data_g->teid_mapping    = hashtable_create (256, NULL, NULL);
  AssertFatal(RC.gtpv1u_data_g->teid_mapping != NULL, " ERROR Initializing TASK_GTPV1_U task interface: in hashtable_create\n");
  //  RC.gtpv1u_data_g.enb_ip_address_for_S1u_S12_S4_up         = enb_properties_p->enb_ipv4_address_for_S1U;
  //gtpv1u_data_g.udp_data;
  RC.gtpv1u_data_g->seq_num         = 0;
  RC.gtpv1u_data_g->restart_counter = 0;

  /* Initializing GTPv1-U stack */
  if ((rc = nwGtpv1uInitialize(&RC.gtpv1u_data_g->gtpv1u_stack, GTPU_STACK_ENB)) != NW_GTPV1U_OK) {
    LOG_E(GTPU, "Failed to setup nwGtpv1u stack %x\n", rc);
    return -1;
  }

  if ((rc = nwGtpv1uSetLogLevel(RC.gtpv1u_data_g->gtpv1u_stack,
                                NW_LOG_LEVEL_DEBG)) != NW_GTPV1U_OK) {
    LOG_E(GTPU, "Failed to setup loglevel for stack %x\n", rc);
    return -1;
  }

  /* Set the ULP API callback. Called once message have been processed by the
   * nw-gtpv1u stack.
   */
  ulp.ulpReqCallback = gtpv1u_gNB_process_stack_req;
  memset((void *)&(ulp.hUlp), 0, sizeof(NwGtpv1uUlpHandleT));

  if ((rc = nwGtpv1uSetUlpEntity(RC.gtpv1u_data_g->gtpv1u_stack, &ulp)) != NW_GTPV1U_OK) {
    LOG_E(GTPU, "nwGtpv1uSetUlpEntity: %x", rc);
    return -1;
  }

  /* nw-gtpv1u stack requires an udp callback to send data over UDP.
   * We provide a wrapper to UDP task.
   */
  udp.udpDataReqCallback = gtpv1u_eNB_send_udp_msg;
  memset((void *)&(udp.hUdp), 0, sizeof(NwGtpv1uUdpHandleT));

  if ((rc = nwGtpv1uSetUdpEntity(RC.gtpv1u_data_g->gtpv1u_stack, &udp)) != NW_GTPV1U_OK) {
    LOG_E(GTPU, "nwGtpv1uSetUdpEntity: %x", rc);
    return -1;
  }

  log.logReqCallback = gtpv1u_eNB_log_request;
  memset((void *)&(log.logMgrHandle), 0, sizeof(NwGtpv1uLogMgrHandleT));

  if ((rc = nwGtpv1uSetLogMgrEntity(RC.gtpv1u_data_g->gtpv1u_stack, &log)) != NW_GTPV1U_OK) {
    LOG_E(GTPU, "nwGtpv1uSetLogMgrEntity: %x", rc);
    return -1;
  }

  /* Timer interface is more complicated as both wrappers doesn't send a message
   * to the timer task but call the timer API functions start/stop timer.
   */
  tmr.tmrMgrHandle     = 0;
  tmr.tmrStartCallback = gtpv1u_start_timer_wrapper;
  tmr.tmrStopCallback  = gtpv1u_stop_timer_wrapper;

  if ((rc = nwGtpv1uSetTimerMgrEntity(RC.gtpv1u_data_g->gtpv1u_stack, &tmr)) != NW_GTPV1U_OK) {
    LOG_E(GTPU, "nwGtpv1uSetTimerMgrEntity: %x", rc);
    return -1;
  }

#if defined(GTP_DUMP_SOCKET) && GTP_DUMP_SOCKET > 0

  if ((ret = gtpv1u_eNB_create_dump_socket()) < 0) {
    return -1;
  }

#endif
  LOG_D(GTPU, "Initializing GTPV1U interface for eNB: DONE\n");
  return 0;
}

void *gtpv1u_gNB_task(void *args) {
  int rc = 0;
  rc = gtpv1u_gNB_init();
  AssertFatal(rc == 0, "gtpv1u_eNB_init Failed");
  itti_mark_task_ready(TASK_GTPV1_U);
  MSC_START_USE();

  while(1) {
    (void) gtpv1u_eNB_process_itti_msg (NULL);
  }

  return NULL;
}

/* Callback called when a gtpv1u message arrived on UDP interface */
NwGtpv1uRcT nr_gtpv1u_gNB_process_stack_req(
  NwGtpv1uUlpHandleT hUlp,
  NwGtpv1uUlpApiT   *pUlpApi) {
  boolean_t              result             = FALSE;
  teid_t                 teid               = 0;
  hashtable_rc_t         hash_rc            = HASH_TABLE_KEY_NOT_EXISTS;
  nr_gtpv1u_teid_data_t *gtpv1u_teid_data_p = NULL;
  protocol_ctxt_t        ctxt;
  NwGtpv1uRcT            rc;

  switch(pUlpApi->apiType) {
    /* Here there are two type of messages handled:
     * - T-PDU
     * - END-MARKER
     */
    case NW_GTPV1U_ULP_API_RECV_TPDU: {
      uint8_t              buffer[4096];
      uint32_t             buffer_len;
      //uint16_t             msgType = NW_GTP_GPDU;
      //NwGtpv1uMsgT     *pMsg = NULL;
      /* Nw-gptv1u stack has processed a PDU. we can schedule it to PDCP
       * for transmission.
       */
      teid = pUlpApi->apiInfo.recvMsgInfo.teid;
      //pMsg = (NwGtpv1uMsgT *) pUlpApi->apiInfo.recvMsgInfo.hMsg;
      //msgType = pMsg->msgType;

      if (NW_GTPV1U_OK != nwGtpv1uMsgGetTpdu(pUlpApi->apiInfo.recvMsgInfo.hMsg,
                                             buffer, &buffer_len)) {
        LOG_E(GTPU, "Error while retrieving T-PDU");
      }

      itti_free(TASK_UDP, ((NwGtpv1uMsgT *)pUlpApi->apiInfo.recvMsgInfo.hMsg)->msgBuf);
#if defined(GTP_DUMP_SOCKET) && GTP_DUMP_SOCKET > 0
      gtpv1u_eNB_write_dump_socket(buffer,buffer_len);
#endif
      rc = nwGtpv1uMsgDelete(RC.nr_gtpv1u_data_g->gtpv1u_stack,
                             pUlpApi->apiInfo.recvMsgInfo.hMsg);

      if (rc != NW_GTPV1U_OK) {
        LOG_E(GTPU, "nwGtpv1uMsgDelete failed: 0x%x\n", rc);
      }

      hash_rc = hashtable_get(RC.nr_gtpv1u_data_g->teid_mapping, teid, (void **)&gtpv1u_teid_data_p);

      if (hash_rc == HASH_TABLE_OK) {
// #if defined(LOG_GTPU) && LOG_GTPU > 0
        LOG_D(GTPU, "Received T-PDU from gtpv1u stack teid  %u size %d -> gnb module id %u ue module id %u pdu session id %u\n",
              teid,
              buffer_len,
              gtpv1u_teid_data_p->gnb_id,
              gtpv1u_teid_data_p->ue_id,
              gtpv1u_teid_data_p->pdu_session_id);
// #endif
        //warning "LG eps bearer mapping to DRB id to do (offset -4)"
        PROTOCOL_CTXT_SET_BY_MODULE_ID(&ctxt, gtpv1u_teid_data_p->gnb_id, GNB_FLAG_YES,  gtpv1u_teid_data_p->ue_id, 0, 0,gtpv1u_teid_data_p->gnb_id);
        // MSC_LOG_TX_MESSAGE(
        //   MSC_GTPU_ENB,
        //   MSC_PDCP_ENB,
        //   NULL,0,
        //   MSC_AS_TIME_FMT" DATA-REQ rb %u size %u",
        //   0,0,
        //   (gtpv1u_teid_data_p->eps_bearer_id) ? gtpv1u_teid_data_p->eps_bearer_id - 4: 5-4,
        //   buffer_len);

        result = pdcp_data_req(
                   &ctxt,
                   SRB_FLAG_NO,
                   1,
                   0, // mui
                   SDU_CONFIRM_NO, // confirm
                   buffer_len,
                   buffer,
                   PDCP_TRANSMISSION_MODE_DATA,NULL, NULL
                 );

        if ( result == FALSE ) {
          if (ctxt.configured == FALSE )
            LOG_W(GTPU, "gNB node PDCP data request failed, cause: [UE:%x]RB is not configured!\n", ctxt.rnti) ;
          else
            LOG_W(GTPU, "PDCP data request failed\n");

          return NW_GTPV1U_FAILURE;
        }
      } else {
        LOG_W(GTPU, "Received T-PDU from gtpv1u stack teid %u unknown size %u", teid, buffer_len);
      }
    }
    break;

    default: {
      LOG_E(GTPU, "Received undefined UlpApi (%02x) from gtpv1u stack!\n",
            pUlpApi->apiType);
    }
  } // end of switch

  return NW_GTPV1U_OK;
}

int nr_gtpv1u_gNB_init(void) {
  NwGtpv1uRcT             rc = NW_GTPV1U_FAILURE;
  NwGtpv1uUlpEntityT      ulp;
  NwGtpv1uUdpEntityT      udp;
  NwGtpv1uLogMgrEntityT   log;
  NwGtpv1uTimerMgrEntityT tmr;
  //  enb_properties_p = enb_config_get()->properties[0];
  RC.nr_gtpv1u_data_g = (nr_gtpv1u_data_t *)calloc(sizeof(nr_gtpv1u_data_t),1);
  LOG_I(GTPU, "Initializing GTPU stack %p\n",&RC.nr_gtpv1u_data_g);

  /* Initialize UE hashtable */
  RC.nr_gtpv1u_data_g->ue_mapping      = hashtable_create (32, NULL, NULL);
  AssertFatal(RC.nr_gtpv1u_data_g->ue_mapping != NULL, " ERROR Initializing TASK_GTPV1_U task interface: in hashtable_create returned %p\n", RC.gtpv1u_data_g->ue_mapping);
  RC.nr_gtpv1u_data_g->teid_mapping    = hashtable_create (256, NULL, NULL);
  AssertFatal(RC.nr_gtpv1u_data_g->teid_mapping != NULL, " ERROR Initializing TASK_GTPV1_U task interface: in hashtable_create\n");
  //  RC.gtpv1u_data_g.enb_ip_address_for_S1u_S12_S4_up         = enb_properties_p->enb_ipv4_address_for_S1U;
  //gtpv1u_data_g.udp_data;
  RC.nr_gtpv1u_data_g->seq_num         = 0;
  RC.nr_gtpv1u_data_g->restart_counter = 0;

  /* Initializing GTPv1-U stack */
  if ((rc = nwGtpv1uInitialize(&RC.nr_gtpv1u_data_g->gtpv1u_stack, GTPU_STACK_ENB)) != NW_GTPV1U_OK) {
    LOG_E(GTPU, "Failed to setup nwGtpv1u stack %x\n", rc);
    return -1;
  }

  if ((rc = nwGtpv1uSetLogLevel(RC.nr_gtpv1u_data_g->gtpv1u_stack,
                                NW_LOG_LEVEL_DEBG)) != NW_GTPV1U_OK) {
    LOG_E(GTPU, "Failed to setup loglevel for stack %x\n", rc);
    return -1;
  }

  /* Set the ULP API callback. Called once message have been processed by the
   * nw-gtpv1u stack.
   */
  ulp.ulpReqCallback = nr_gtpv1u_gNB_process_stack_req;
  memset((void *)&(ulp.hUlp), 0, sizeof(NwGtpv1uUlpHandleT));

  if ((rc = nwGtpv1uSetUlpEntity(RC.nr_gtpv1u_data_g->gtpv1u_stack, &ulp)) != NW_GTPV1U_OK) {
    LOG_E(GTPU, "nwGtpv1uSetUlpEntity: %x", rc);
    return -1;
  }

  /* nw-gtpv1u stack requires an udp callback to send data over UDP.
   * We provide a wrapper to UDP task.
   */
  udp.udpDataReqCallback = gtpv1u_eNB_send_udp_msg;
  memset((void *)&(udp.hUdp), 0, sizeof(NwGtpv1uUdpHandleT));

  if ((rc = nwGtpv1uSetUdpEntity(RC.nr_gtpv1u_data_g->gtpv1u_stack, &udp)) != NW_GTPV1U_OK) {
    LOG_E(GTPU, "nwGtpv1uSetUdpEntity: %x", rc);
    return -1;
  }

  log.logReqCallback = gtpv1u_eNB_log_request;
  memset((void *)&(log.logMgrHandle), 0, sizeof(NwGtpv1uLogMgrHandleT));

  if ((rc = nwGtpv1uSetLogMgrEntity(RC.nr_gtpv1u_data_g->gtpv1u_stack, &log)) != NW_GTPV1U_OK) {
    LOG_E(GTPU, "nwGtpv1uSetLogMgrEntity: %x", rc);
    return -1;
  }

  /* Timer interface is more complicated as both wrappers doesn't send a message
   * to the timer task but call the timer API functions start/stop timer.
   */
  tmr.tmrMgrHandle     = 0;
  tmr.tmrStartCallback = gtpv1u_start_timer_wrapper;
  tmr.tmrStopCallback  = gtpv1u_stop_timer_wrapper;

  if ((rc = nwGtpv1uSetTimerMgrEntity(RC.nr_gtpv1u_data_g->gtpv1u_stack, &tmr)) != NW_GTPV1U_OK) {
    LOG_E(GTPU, "nwGtpv1uSetTimerMgrEntity: %x", rc);
    return -1;
  }

#if defined(GTP_DUMP_SOCKET) && GTP_DUMP_SOCKET > 0

  if ((ret = gtpv1u_eNB_create_dump_socket()) < 0) {
    return -1;
  }

#endif
  LOG_D(GTPU, "Initializing GTPV1U interface for eNB: DONE\n");
  return 0;
}

//-----------------------------------------------------------------------------
int
gtpv1u_create_ngu_tunnel(
  const instance_t                              instanceP,
  const gtpv1u_gnb_create_tunnel_req_t *const  create_tunnel_req_pP,
  gtpv1u_gnb_create_tunnel_resp_t *const create_tunnel_resp_pP)
{
  /* Create a new nw-gtpv1-u stack req using API */
  NwGtpv1uUlpApiT          stack_req;
  NwGtpv1uRcT              rc                   = NW_GTPV1U_FAILURE;
  /* Local tunnel end-point identifier */
  teid_t                   ngu_teid             = 0;
  nr_gtpv1u_teid_data_t   *gtpv1u_teid_data_p   = NULL;
  nr_gtpv1u_ue_data_t     *gtpv1u_ue_data_p     = NULL;
  //MessageDef              *message_p            = NULL;
  hashtable_rc_t           hash_rc              = HASH_TABLE_KEY_NOT_EXISTS;
  int                      i;
  pdusessionid_t           pdusession_id        = 0;
  //    int                      ipv4_addr            = 0;
  int                      ip_offset            = 0;
  in_addr_t                in_addr;
  int                      addrs_length_in_bytes= 0;
  int                      loop_counter         = 0;
  int                      ret                  = 0;
  MSC_LOG_RX_MESSAGE(
    MSC_GTPU_GNB,
    MSC_RRC_GNB,
    NULL,0,
    MSC_AS_TIME_FMT" CREATE_TUNNEL_REQ RNTI %"PRIx16" inst %u ntuns %u psid %u upf-ngu teid %u",
    0,0,create_tunnel_req_pP->rnti, instanceP,
    create_tunnel_req_pP->num_tunnels, create_tunnel_req_pP->pdusession_id[0],
    create_tunnel_req_pP->outgoing_teid[0]);
  create_tunnel_resp_pP->rnti        = create_tunnel_req_pP->rnti;
  create_tunnel_resp_pP->status      = 0;
  create_tunnel_resp_pP->num_tunnels = 0;

  for (i = 0; i < create_tunnel_req_pP->num_tunnels; i++) {
    ip_offset               = 0;
    loop_counter            = 0;
    pdusession_id = create_tunnel_req_pP->pdusession_id[i];
    LOG_D(GTPU, "Rx GTPV1U_GNB_CREATE_TUNNEL_REQ ue rnti %x pdu session id %u\n",
          create_tunnel_req_pP->rnti, pdusession_id);
    memset(&stack_req, 0, sizeof(NwGtpv1uUlpApiT));
    stack_req.apiType = NW_GTPV1U_ULP_API_CREATE_TUNNEL_ENDPOINT;

    do {
      ngu_teid = gtpv1u_new_teid();
      LOG_D(GTPU, "gtpv1u_create_ngu_tunnel() 0x%x %u(dec)\n", ngu_teid, ngu_teid);
      stack_req.apiInfo.createTunnelEndPointInfo.teid          = ngu_teid;
      stack_req.apiInfo.createTunnelEndPointInfo.hUlpSession   = 0;
      stack_req.apiInfo.createTunnelEndPointInfo.hStackSession = 0;
      rc = nwGtpv1uProcessUlpReq(RC.nr_gtpv1u_data_g->gtpv1u_stack, &stack_req);
      LOG_D(GTPU, ".\n");
      loop_counter++;
    } while (rc != NW_GTPV1U_OK && loop_counter < 10);

    if ( rc != NW_GTPV1U_OK && loop_counter == 10 ) {
      LOG_E(GTPU,"NwGtpv1uCreateTunnelEndPoint failed 10 times,start next loop\n");
      ret = -1;
      continue;
    }

    //-----------------------
    // PDCP->GTPV1U mapping
    //-----------------------
    hash_rc = hashtable_get(RC.nr_gtpv1u_data_g->ue_mapping, create_tunnel_req_pP->rnti, (void **)&gtpv1u_ue_data_p);

    if ((hash_rc == HASH_TABLE_KEY_NOT_EXISTS) || (hash_rc == HASH_TABLE_OK)) {
      if (hash_rc == HASH_TABLE_KEY_NOT_EXISTS) {
        gtpv1u_ue_data_p = calloc (1, sizeof(gtpv1u_ue_data_t));
        hash_rc = hashtable_insert(RC.nr_gtpv1u_data_g->ue_mapping, create_tunnel_req_pP->rnti, gtpv1u_ue_data_p);
        AssertFatal(hash_rc == HASH_TABLE_OK, "Error inserting ue_mapping in GTPV1U hashtable");
      }

      gtpv1u_ue_data_p->ue_id       = create_tunnel_req_pP->rnti;
      gtpv1u_ue_data_p->instance_id = 0; // TO DO
      memcpy(&create_tunnel_resp_pP->gnb_addr.buffer,
             &RC.nr_gtpv1u_data_g->gnb_ip_address_for_NGu_up,
             sizeof (in_addr_t));
     
      LOG_I(GTPU,"Configured GTPu address : %x\n",RC.nr_gtpv1u_data_g->gnb_ip_address_for_NGu_up);
      create_tunnel_resp_pP->gnb_addr.length = sizeof (in_addr_t);
      addrs_length_in_bytes = create_tunnel_req_pP->dst_addr[i].length / 8;
      AssertFatal((addrs_length_in_bytes == 4) ||
                  (addrs_length_in_bytes == 16) ||
                  (addrs_length_in_bytes == 20),
                  "Bad transport layer address length %d (bits) %d (bytes)",
                  create_tunnel_req_pP->dst_addr[i].length, addrs_length_in_bytes);

      if ((addrs_length_in_bytes == 4) ||
          (addrs_length_in_bytes == 20)) {
        in_addr = *((in_addr_t *)create_tunnel_req_pP->dst_addr[i].buffer);
        ip_offset = 4;
        gtpv1u_ue_data_p->bearers[pdusession_id - GTPV1U_BEARER_OFFSET].upf_ip_addr = in_addr;
      }

      if ((addrs_length_in_bytes == 16) ||
          (addrs_length_in_bytes == 20)) {
        memcpy(gtpv1u_ue_data_p->bearers[pdusession_id - GTPV1U_BEARER_OFFSET].upf_ip6_addr.s6_addr,
               &create_tunnel_req_pP->dst_addr[i].buffer[ip_offset],
               16);
      }

      gtpv1u_ue_data_p->bearers[pdusession_id - GTPV1U_BEARER_OFFSET].state                  = BEARER_IN_CONFIG;
      gtpv1u_ue_data_p->bearers[pdusession_id - GTPV1U_BEARER_OFFSET].teid_gNB               = ngu_teid;
      gtpv1u_ue_data_p->bearers[pdusession_id - GTPV1U_BEARER_OFFSET].teid_gNB_stack_session = stack_req.apiInfo.createTunnelEndPointInfo.hStackSession;
      gtpv1u_ue_data_p->bearers[pdusession_id - GTPV1U_BEARER_OFFSET].teid_upf               = create_tunnel_req_pP->outgoing_teid[i];
      gtpv1u_ue_data_p->num_bearers++;
      create_tunnel_resp_pP->gnb_NGu_teid[i] = ngu_teid;

      LOG_I(GTPU,"Copied to create_tunnel_resp tunnel: index %d target gNB ip %d.%d.%d.%d length %d gtp teid %u\n",
        i,
        create_tunnel_resp_pP->gnb_addr.buffer[0],
        create_tunnel_resp_pP->gnb_addr.buffer[1],
        create_tunnel_resp_pP->gnb_addr.buffer[2],
        create_tunnel_resp_pP->gnb_addr.buffer[3],
        create_tunnel_resp_pP->gnb_addr.length,
        create_tunnel_resp_pP->gnb_NGu_teid[i]);
    } else {
      create_tunnel_resp_pP->gnb_NGu_teid[i] = 0;
      create_tunnel_resp_pP->status         = 0xFF;
    }

    create_tunnel_resp_pP->pdusession_id[i] = pdusession_id;
    create_tunnel_resp_pP->num_tunnels      += 1;
    //-----------------------
    // GTPV1U->PDCP mapping
    //-----------------------
    hash_rc = hashtable_get(RC.nr_gtpv1u_data_g->teid_mapping, ngu_teid, (void **)&gtpv1u_teid_data_p);

    if (hash_rc == HASH_TABLE_KEY_NOT_EXISTS) {
      gtpv1u_teid_data_p = calloc (1, sizeof(nr_gtpv1u_teid_data_t));
      gtpv1u_teid_data_p->gnb_id        = 0; // TO DO
      gtpv1u_teid_data_p->ue_id         = create_tunnel_req_pP->rnti;
      gtpv1u_teid_data_p->pdu_session_id = pdusession_id;
      hash_rc = hashtable_insert(RC.nr_gtpv1u_data_g->teid_mapping, ngu_teid, gtpv1u_teid_data_p);
      AssertFatal(hash_rc == HASH_TABLE_OK, "Error inserting teid mapping in GTPV1U hashtable");
    } else {
      create_tunnel_resp_pP->gnb_NGu_teid[i] = 0;
      create_tunnel_resp_pP->status         = 0xFF;
    }
  }

  MSC_LOG_TX_MESSAGE(
    MSC_GTPU_GNB,
    MSC_RRC_GNB,
    NULL,0,
    "0 GTPV1U_GNB_CREATE_TUNNEL_RESP rnti %x teid %x",
    create_tunnel_resp_pP->rnti,
    ngu_teid);
  LOG_D(GTPU, "Tx GTPV1U_GNB_CREATE_TUNNEL_RESP ue rnti %x status %d\n",
        create_tunnel_req_pP->rnti,
        create_tunnel_resp_pP->status);
  //return 0;
  return ret;
}

int gtpv1u_update_ngu_tunnel(
  const instance_t                              instanceP,
  const gtpv1u_gnb_create_tunnel_req_t *const  create_tunnel_req_pP,
  const rnti_t                                  prior_rnti
) {
  /* Local tunnel end-point identifier */
  teid_t                      ngu_teid             = 0;
  nr_gtpv1u_teid_data_t      *gtpv1u_teid_data_p   = NULL;
  nr_gtpv1u_ue_data_t        *gtpv1u_ue_data_p     = NULL;
  nr_gtpv1u_ue_data_t        *gtpv1u_ue_data_new_p     = NULL;
  //MessageDef              *message_p            = NULL;
  hashtable_rc_t           hash_rc              = HASH_TABLE_KEY_NOT_EXISTS;
  int                      i,j;
  uint8_t                  bearers_num = 0,bearers_total = 0;
  //-----------------------
  // PDCP->GTPV1U mapping
  //-----------------------
  hash_rc = hashtable_get(RC.nr_gtpv1u_data_g->ue_mapping, prior_rnti, (void **)&gtpv1u_ue_data_p);

  if(hash_rc != HASH_TABLE_OK) {
    LOG_E(GTPU,"Error get ue_mapping(rnti=%x) from GTPV1U hashtable error\n", prior_rnti);
    return -1;
  }

  gtpv1u_ue_data_new_p = calloc (1, sizeof(nr_gtpv1u_ue_data_t));
  memcpy(gtpv1u_ue_data_new_p,gtpv1u_ue_data_p,sizeof(nr_gtpv1u_ue_data_t));
  gtpv1u_ue_data_new_p->ue_id       = create_tunnel_req_pP->rnti;
  hash_rc = hashtable_insert(RC.nr_gtpv1u_data_g->ue_mapping, create_tunnel_req_pP->rnti, gtpv1u_ue_data_new_p);

  //AssertFatal(hash_rc == HASH_TABLE_OK, "Error inserting ue_mapping in GTPV1U hashtable");
  if ( hash_rc != HASH_TABLE_OK ) {
    LOG_E(GTPU,"Failed to insert ue_mapping(rnti=%x) in GTPV1U hashtable\n",create_tunnel_req_pP->rnti);
    return -1;
  } else {
    LOG_I(GTPU, "inserting ue_mapping(rnti=%x) in GTPV1U hashtable\n",
          create_tunnel_req_pP->rnti);
  }

  hash_rc = hashtable_remove(RC.nr_gtpv1u_data_g->ue_mapping, prior_rnti);
  LOG_I(GTPU, "hashtable_remove ue_mapping(rnti=%x) in GTPV1U hashtable\n",
        prior_rnti);
  //-----------------------
  // GTPV1U->PDCP mapping
  //-----------------------
  bearers_total =gtpv1u_ue_data_new_p->num_bearers;

  for(j = 0; j<GTPV1U_MAX_BEARERS_ID; j++) {
    if(gtpv1u_ue_data_new_p->bearers[j].state != BEARER_IN_CONFIG)
      continue;

    bearers_num++;

    for (i = 0; i < create_tunnel_req_pP->num_tunnels; i++) {
      if(j == (create_tunnel_req_pP->pdusession_id[i]-GTPV1U_BEARER_OFFSET))
        break;
    }

    if(i < create_tunnel_req_pP->num_tunnels) {
      ngu_teid = gtpv1u_ue_data_new_p->bearers[j].teid_gNB;
      hash_rc = hashtable_get(RC.nr_gtpv1u_data_g->teid_mapping, ngu_teid, (void **)&gtpv1u_teid_data_p);

      if (hash_rc == HASH_TABLE_OK) {
        gtpv1u_teid_data_p->ue_id         = create_tunnel_req_pP->rnti;
        gtpv1u_teid_data_p->pdu_session_id = create_tunnel_req_pP->pdusession_id[i];
        LOG_I(GTPU, "updata teid_mapping te_id %u (prior_rnti %x rnti %x) in GTPV1U hashtable\n",
                    ngu_teid,prior_rnti,create_tunnel_req_pP->rnti);
      } else {
        LOG_W(GTPU, "Error get teid mapping(s1u_teid=%u) from GTPV1U hashtable", ngu_teid);
      }
    } else {
      ngu_teid = gtpv1u_ue_data_new_p->bearers[j].teid_gNB;
      hash_rc = hashtable_remove(RC.nr_gtpv1u_data_g->teid_mapping, ngu_teid);

      if (hash_rc != HASH_TABLE_OK) {
        LOG_D(GTPU, "Removed user rnti %x , enb S1U teid %u not found\n", prior_rnti, ngu_teid);
      }

      gtpv1u_ue_data_new_p->bearers[j].state = BEARER_DOWN;
      gtpv1u_ue_data_new_p->num_bearers--;
      LOG_I(GTPU, "delete teid_mapping te_id %u (rnti%x) bearer_id %d in GTPV1U hashtable\n",
            ngu_teid,prior_rnti,j+GTPV1U_BEARER_OFFSET);;
    }

    if(bearers_num > bearers_total)
      break;
  }

  return 0;
}

//-----------------------------------------------------------------------------
int gtpv1u_delete_ngu_tunnel(
  const instance_t                             instanceP,
  const gtpv1u_gnb_delete_tunnel_req_t *const req_pP) {
  NwGtpv1uUlpApiT          stack_req;
  NwGtpv1uRcT              rc                   = NW_GTPV1U_FAILURE;
  MessageDef              *message_p = NULL;
  nr_gtpv1u_ue_data_t     *gtpv1u_ue_data_p     = NULL;
  hashtable_rc_t           hash_rc              = HASH_TABLE_KEY_NOT_EXISTS;
  teid_t                   teid_gNB             = 0;
  int                      pdusession_index     = 0;
  message_p = itti_alloc_new_message(TASK_GTPV1_U, 0, GTPV1U_GNB_DELETE_TUNNEL_RESP);
  GTPV1U_GNB_DELETE_TUNNEL_RESP(message_p).rnti     = req_pP->rnti;
  GTPV1U_GNB_DELETE_TUNNEL_RESP(message_p).status       = 0;
  hash_rc = hashtable_get(RC.nr_gtpv1u_data_g->ue_mapping, req_pP->rnti, (void **)&gtpv1u_ue_data_p);

  if (hash_rc == HASH_TABLE_OK) {
    for (pdusession_index = 0; pdusession_index < req_pP->num_pdusession; pdusession_index++) {
      teid_gNB = gtpv1u_ue_data_p->bearers[req_pP->pdusession_id[pdusession_index] - GTPV1U_BEARER_OFFSET].teid_gNB;
      LOG_D(GTPU, "Rx GTPV1U_ENB_DELETE_TUNNEL user rnti %x eNB S1U teid %u eps bearer id %u\n",
            req_pP->rnti, teid_gNB, req_pP->pdusession_id[pdusession_index]);
      {
        memset(&stack_req, 0, sizeof(NwGtpv1uUlpApiT));
        stack_req.apiType = NW_GTPV1U_ULP_API_DESTROY_TUNNEL_ENDPOINT;
        LOG_D(GTPU, "gtpv1u_delete_ngu_tunnel pdusession %u  %u\n",
              req_pP->pdusession_id[pdusession_index],
              teid_gNB);
        stack_req.apiInfo.destroyTunnelEndPointInfo.hStackSessionHandle =
            gtpv1u_ue_data_p->bearers[req_pP->pdusession_id[pdusession_index] - GTPV1U_BEARER_OFFSET].teid_gNB_stack_session;
        rc = nwGtpv1uProcessUlpReq(RC.nr_gtpv1u_data_g->gtpv1u_stack, &stack_req);
        LOG_D(GTPU, ".\n");
      }

      if (rc != NW_GTPV1U_OK) {
        GTPV1U_GNB_DELETE_TUNNEL_RESP(message_p).status       |= 0xFF;
        LOG_E(GTPU, "NW_GTPV1U_ULP_API_DESTROY_TUNNEL_ENDPOINT failed");
      }

      //-----------------------
      // PDCP->GTPV1U mapping
      //-----------------------
      gtpv1u_ue_data_p->bearers[req_pP->pdusession_id[pdusession_index] - GTPV1U_BEARER_OFFSET].state       = BEARER_DOWN;
      gtpv1u_ue_data_p->bearers[req_pP->pdusession_id[pdusession_index] - GTPV1U_BEARER_OFFSET].teid_gNB    = 0;
      gtpv1u_ue_data_p->bearers[req_pP->pdusession_id[pdusession_index] - GTPV1U_BEARER_OFFSET].teid_upf    = 0;
      gtpv1u_ue_data_p->bearers[req_pP->pdusession_id[pdusession_index] - GTPV1U_BEARER_OFFSET].upf_ip_addr = 0;
      gtpv1u_ue_data_p->num_bearers -= 1;

      if (gtpv1u_ue_data_p->num_bearers == 0) {
        hash_rc = hashtable_remove(RC.nr_gtpv1u_data_g->ue_mapping, req_pP->rnti);
        LOG_D(GTPU, "Removed user rnti %x,no more bearers configured\n", req_pP->rnti);
      }

      //-----------------------
      // GTPV1U->PDCP mapping
      //-----------------------
      hash_rc = hashtable_remove(RC.nr_gtpv1u_data_g->teid_mapping, teid_gNB);

      if (hash_rc != HASH_TABLE_OK) {
        LOG_D(GTPU, "Removed user rnti %x , gNB NGU teid %u not found\n", req_pP->rnti, teid_gNB);
      }
    }
  }// else silently do nothing

  LOG_D(GTPU, "Tx GTPV1U_GNB_DELETE_TUNNEL_RESP user rnti %x gNB NGU teid %u status %u\n",
        GTPV1U_GNB_DELETE_TUNNEL_RESP(message_p).rnti,
        GTPV1U_GNB_DELETE_TUNNEL_RESP(message_p).gnb_NGu_teid,
        GTPV1U_GNB_DELETE_TUNNEL_RESP(message_p).status);
  MSC_LOG_TX_MESSAGE(
    MSC_GTPU_GNB,
    MSC_RRC_GNB,
    NULL,0,
    "0 GTPV1U_GNB_DELETE_TUNNEL_RESP rnti %x teid %x",
    GTPV1U_GNB_DELETE_TUNNEL_RESP(message_p).rnti,
    teid_gNB);
  return itti_send_msg_to_task(TASK_RRC_GNB, instanceP, message_p);
}

//-----------------------------------------------------------------------------
static int gtpv1u_gNB_send_init_udp(const Gtpv1uNGReq *req) {
  // Create and alloc new message
  MessageDef *message_p;
  struct in_addr addr= {0};
  message_p = itti_alloc_new_message(TASK_GTPV1_U, 0, UDP_INIT);

  if (message_p == NULL) {
    return -1;
  }

  UDP_INIT(message_p).port = req->gnb_port_for_NGu_up;
  addr.s_addr = req->gnb_ip_address_for_NGu_up;
  UDP_INIT(message_p).address = inet_ntoa(addr);
  LOG_I(GTPU, "Tx UDP_INIT IP addr %s (%x)\n", UDP_INIT(message_p).address,UDP_INIT(message_p).port);
  MSC_LOG_EVENT(
    MSC_GTPU_ENB,
    "0 UDP bind  %s:%u",
    UDP_INIT(message_p).address,
    UDP_INIT(message_p).port);
  return itti_send_msg_to_task(TASK_UDP, INSTANCE_DEFAULT, message_p);
}

static int gtpv1u_ng_req(
  const instance_t                             instanceP,
  const Gtpv1uNGReq *const req) {
  memcpy(&RC.nr_gtpv1u_data_g->gnb_ip_address_for_NGu_up,
         &req->gnb_ip_address_for_NGu_up,
         sizeof (req->gnb_ip_address_for_NGu_up));
  gtpv1u_gNB_send_init_udp(req);
  return 0;
}

static int gtpv1u_gnb_tunnel_data_req(gtpv1u_gnb_tunnel_data_req_t *gnb_tunnel_data_req) {
  gtpv1u_gnb_tunnel_data_req_t *data_req_p           = NULL;
  NwGtpv1uUlpApiT               stack_req;
  NwGtpv1uRcT                   rc                   = NW_GTPV1U_FAILURE;
  hashtable_rc_t                hash_rc              = HASH_TABLE_KEY_NOT_EXISTS;
  nr_gtpv1u_ue_data_t          *gtpv1u_ue_data_p     = NULL;
  teid_t                        gnb_ngu_teid         = 0;
  teid_t                        outgoing_teid         = 0;
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_GTPV1U_PROCESS_TUNNEL_DATA_REQ, VCD_FUNCTION_IN);
  data_req_p = gnb_tunnel_data_req;

  memset(&stack_req, 0, sizeof(NwGtpv1uUlpApiT));
  hash_rc = hashtable_get(RC.nr_gtpv1u_data_g->ue_mapping, (uint64_t)data_req_p->rnti, (void **)&gtpv1u_ue_data_p);

  if (hash_rc == HASH_TABLE_KEY_NOT_EXISTS) {
    LOG_E(GTPU, "nwGtpv1uProcessUlpReq failed: while getting ue rnti %x in hashtable ue_mapping\n", data_req_p->rnti);
  } else {
    if ((data_req_p->pdusession_id >= GTPV1U_BEARER_OFFSET) && (data_req_p->pdusession_id < max_val_NR_DRB_Identity)) {
      gnb_ngu_teid                        = gtpv1u_ue_data_p->bearers[data_req_p->pdusession_id - GTPV1U_BEARER_OFFSET].teid_gNB;
      outgoing_teid                        = gtpv1u_ue_data_p->bearers[data_req_p->pdusession_id - GTPV1U_BEARER_OFFSET].teid_upf;
      stack_req.apiType                   = NW_GTPV1U_ULP_API_SEND_TPDU;
      stack_req.apiInfo.sendtoInfo.teid   = outgoing_teid;
      stack_req.apiInfo.sendtoInfo.ipAddr = gtpv1u_ue_data_p->bearers[data_req_p->pdusession_id - GTPV1U_BEARER_OFFSET].upf_ip_addr;
      rc = nwGtpv1uGpduMsgNew(
              RC.nr_gtpv1u_data_g->gtpv1u_stack,
              outgoing_teid,
              NW_FALSE,
              RC.nr_gtpv1u_data_g->seq_num++,
              data_req_p->buffer,
              data_req_p->length,
              data_req_p->offset,
              &(stack_req.apiInfo.sendtoInfo.hMsg));

      if (rc != NW_GTPV1U_OK) {
        LOG_E(GTPU, "nwGtpv1uGpduMsgNew failed: 0x%x\n", rc);
        MSC_LOG_EVENT(MSC_GTPU_GNB,"0 Failed send G-PDU ltid %u rtid %u size %u",
                      gnb_ngu_teid,outgoing_teid,data_req_p->length);
        (void)gnb_ngu_teid; /* avoid gcc warning "set but not used" */
      } else {
        rc = nwGtpv1uProcessUlpReq(RC.nr_gtpv1u_data_g->gtpv1u_stack, &stack_req);

        if (rc != NW_GTPV1U_OK) {
          LOG_E(GTPU, "nwGtpv1uProcessUlpReq failed: 0x%x\n", rc);
          MSC_LOG_EVENT(MSC_GTPU_GNB,"0 Failed send G-PDU ltid %u rtid %u size %u",
                        gnb_ngu_teid,outgoing_teid,data_req_p->length);
        } else {
          MSC_LOG_TX_MESSAGE(
            MSC_GTPU_GNB,
            MSC_GTPU_SGW,
            NULL,
            0,
            MSC_AS_TIME_FMT" G-PDU ltid %u rtid %u size %u",
            0,0,
            gnb_ngu_teid,
            outgoing_teid,
            data_req_p->length);
        }

        rc = nwGtpv1uMsgDelete(RC.nr_gtpv1u_data_g->gtpv1u_stack,
                                stack_req.apiInfo.sendtoInfo.hMsg);

        if (rc != NW_GTPV1U_OK) {
          LOG_E(GTPU, "nwGtpv1uMsgDelete failed: 0x%x\n", rc);
        }
      }
    }
  }

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_GTPV1U_PROCESS_TUNNEL_DATA_REQ, VCD_FUNCTION_OUT);
  /* Buffer still needed, do not free it */
  //itti_free(ITTI_MSG_ORIGIN_ID(received_message_p), data_req_p->buffer);

  return 0;
}

//-----------------------------------------------------------------------------
void *gtpv1u_gNB_process_itti_msg(void *notUsed) {
  /* Trying to fetch a message from the message queue.
   * If the queue is empty, this function will block till a
   * message is sent to the task.
   */
  instance_t  instance;
  MessageDef *received_message_p = NULL;
  int         rc = 0;
  itti_receive_msg(TASK_GTPV1_U, &received_message_p);
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_GTPV1U_ENB_TASK, VCD_FUNCTION_IN);
  DevAssert(received_message_p != NULL);
  instance = received_message_p->ittiMsgHeader.originInstance;

  switch (ITTI_MSG_ID(received_message_p)) {
    case GTPV1U_GNB_NG_REQ:
      gtpv1u_ng_req(instance, &received_message_p->ittiMsg.gtpv1uNGReq);
      break;

    case GTPV1U_GNB_DELETE_TUNNEL_REQ:
      gtpv1u_delete_ngu_tunnel(instance, &received_message_p->ittiMsg.NRGtpv1uDeleteTunnelReq);
      break;

    // DATA COMING FROM UDP
    case UDP_DATA_IND: {
      udp_data_ind_t *udp_data_ind_p;
      udp_data_ind_p = &received_message_p->ittiMsg.udp_data_ind;
      nwGtpv1uProcessUdpReq(RC.nr_gtpv1u_data_g->gtpv1u_stack,
                            udp_data_ind_p->buffer,
                            udp_data_ind_p->buffer_length,
                            udp_data_ind_p->peer_port,
                            udp_data_ind_p->peer_address);
    }
    break;

    // DATA TO BE SENT TO UDP
    case GTPV1U_GNB_TUNNEL_DATA_REQ:
      LOG_I(GTPU, "Received message %s\n", ITTI_MSG_NAME(received_message_p));
      gtpv1u_gnb_tunnel_data_req(&GTPV1U_GNB_TUNNEL_DATA_REQ(received_message_p));
      break;

    case TERMINATE_MESSAGE: {
      if (RC.nr_gtpv1u_data_g->ue_mapping != NULL) {
        hashtable_destroy (&(RC.nr_gtpv1u_data_g->ue_mapping));
      }

      if (RC.nr_gtpv1u_data_g->teid_mapping != NULL) {
        hashtable_destroy (&(RC.nr_gtpv1u_data_g->teid_mapping));
      }

      LOG_W(GTPU, " *** Exiting GTPU thread\n");
      itti_exit_task();
    }
    break;

    case TIMER_HAS_EXPIRED:
      nwGtpv1uProcessTimeout(&received_message_p->ittiMsg.timer_has_expired.arg);
      break;

    default: {
      LOG_E(GTPU, "Unkwnon message ID %d:%s\n",
            ITTI_MSG_ID(received_message_p),
            ITTI_MSG_NAME(received_message_p));
    }
    break;
  }

  rc = itti_free(ITTI_MSG_ORIGIN_ID(received_message_p), received_message_p);
  AssertFatal(rc == EXIT_SUCCESS, "Failed to free memory (%d)!\n", rc);
  received_message_p = NULL;
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_GTPV1U_ENB_TASK, VCD_FUNCTION_OUT);
  return NULL;
}

void *nr_gtpv1u_gNB_task(void *args) {
  int rc = 0;
  rc = nr_gtpv1u_gNB_init();
  AssertFatal(rc == 0, "gtpv1u_gNB_init Failed");
  itti_mark_task_ready(TASK_GTPV1_U);
  MSC_START_USE();

  while(1) {
    (void) gtpv1u_gNB_process_itti_msg (NULL);
  }

  return NULL;
}
