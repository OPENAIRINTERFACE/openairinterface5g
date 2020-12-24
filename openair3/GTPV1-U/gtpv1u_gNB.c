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
#include "gtpv1_u_messages_types.h"
#include "udp_eNB_task.h"
#include "common/utils/LOG/log.h"
#include "COMMON/platform_types.h"
#include "COMMON/platform_constants.h"
#include "common/utils/LOG/vcd_signal_dumper.h"
#include "common/ran_context.h"
#include "gtpv1u_eNB_defs.h"
#include "gtpv1u_eNB_task.h"
#include "gtpv1u_gNB_task.h"
#include "rrc_eNB_GTPV1U.h"

#undef GTP_DUMP_SOCKET

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

