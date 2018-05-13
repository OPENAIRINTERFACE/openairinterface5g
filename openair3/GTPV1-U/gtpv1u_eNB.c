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

/*! \file gtpv1u_eNB.c
 * \brief
 * \author Sebastien ROUX, Lionel GAUTHIER, Navid Nikaein
 * \version 1.0
 * \company Eurecom
 * \email: lionel.gauthier@eurecom.fr
 */
#include <stdio.h>
#include <errno.h>

#include "mme_config.h"

#include "assertions.h"
#include "intertask_interface.h"
#include "timer.h"
#include "msc.h"

#include "gtpv1u.h"
#include "NwGtpv1u.h"
#include "NwGtpv1uMsg.h"
#include "NwGtpv1uPrivate.h"
#include "NwLog.h"
#include "gtpv1u_eNB_defs.h"
#include "gtpv1_u_messages_types.h"
#include "udp_eNB_task.h"
#include "UTIL/LOG/log.h"
#include "COMMON/platform_types.h"
#include "COMMON/platform_constants.h"
#include "UTIL/LOG/vcd_signal_dumper.h"
#include "common/ran_context.h"
#include "gtpv1u_eNB_defs.h"

#undef GTP_DUMP_SOCKET

/*
extern boolean_t pdcp_data_req(
  const protocol_ctxt_t* const  ctxt_pP,
  const srb_flag_t     srb_flagP,
  const rb_id_t        rb_idP,
  const mui_t          muiP,
  const confirm_t      confirmP,
  const sdu_size_t     sdu_buffer_sizeP,
  unsigned char *const sdu_buffer_pP,
  const pdcp_transmission_mode_t modeP);
*/
extern unsigned char NB_eNB_INST;
extern RAN_CONTEXT_t RC;

static int
gtpv1u_eNB_send_init_udp(
  uint16_t port_number);

NwGtpv1uRcT
gtpv1u_eNB_log_request(
  NwGtpv1uLogMgrHandleT   hLogMgr,
  uint32_t                  logLevel,
  NwCharT                *file,
  uint32_t                  line,
  NwCharT                *logStr);

NwGtpv1uRcT
gtpv1u_eNB_send_udp_msg(
  NwGtpv1uUdpHandleT      udpHandle,
  uint8_t                  *buffer,
  uint32_t                  buffer_len,
  uint32_t                  buffer_offset,
  uint32_t                  peerIpAddr,
  uint16_t                  peerPort);

NwGtpv1uRcT
gtpv1u_eNB_process_stack_req(
  NwGtpv1uUlpHandleT hUlp,
  NwGtpv1uUlpApiT   *pUlpApi);

int
data_recv_callback(
  uint16_t  portP,
  uint32_t  address,
  uint8_t  *buffer,
  uint32_t  length,
  void     *arg_p);
//int
//gtpv1u_create_tunnel_endpoint(
//    gtpv1u_data_t *gtpv1u_data_pP,
//    uint8_t        ue_idP,
//    uint8_t        rab_idP,
//    char          *sgw_ip_addr_pP,
//    uint16_t       portP);
static NwGtpv1uRcT
gtpv1u_start_timer_wrapper(
  NwGtpv1uTimerMgrHandleT tmrMgrHandle,
  uint32_t                  timeoutSec,
  uint32_t                  timeoutUsec,
  uint32_t                  tmrType,
  void                   *timeoutArg,
  NwGtpv1uTimerHandleT   *hTmr);

static NwGtpv1uRcT
gtpv1u_stop_timer_wrapper(
  NwGtpv1uTimerMgrHandleT     tmrMgrHandle,
  NwGtpv1uTimerHandleT         hTmr);

int
gtpv1u_initial_req(
  gtpv1u_data_t *gtpv1u_data_pP,
  teid_t         teidP,
  tcp_udp_port_t portP,
  uint32_t       address);

int
gtpv1u_new_data_req(
  uint8_t  enb_module_idP,
  rnti_t   ue_rntiP,
  uint8_t  rab_idP,
  uint8_t *buffer_pP,
  uint32_t buf_lenP,
  uint32_t buf_offsetP
);

int
gtpv1u_create_s1u_tunnel(
  const instance_t instanceP,
  const gtpv1u_enb_create_tunnel_req_t *  const create_tunnel_req_pP,
        gtpv1u_enb_create_tunnel_resp_t * const create_tunnel_resp_pP);

static int
gtpv1u_delete_s1u_tunnel(
  const instance_t instanceP,
  const gtpv1u_enb_delete_tunnel_req_t * const req_pP);

static int
gtpv1u_eNB_init(void);

void *
gtpv1u_eNB_task(void *args);

//static gtpv1u_data_t gtpv1u_data_g;

#if defined(GTP_DUMP_SOCKET) && GTP_DUMP_SOCKET > 0
#include <linux/if.h>
static int           gtpv1u_dump_socket_g;

//-----------------------------------------------------------------------------
int gtpv1u_eNB_create_dump_socket(void)
{
  struct ifreq ifr;
  int          hdrincl=1;

  if ((gtpv1u_dump_socket_g = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0 ) {
    LOG_E(GTPU, "Could not create dump socket %d:%s\n", errno, strerror(errno));
    return -1;
  }

  if (setsockopt(gtpv1u_dump_socket_g,
                 IPPROTO_IP,
                 IP_HDRINCL,
                 &hdrincl,
                 sizeof(hdrincl))==-1) {
    LOG_E(GTPU, "%s:%d set IP_HDRINCL %d:%s\n",
          __FILE__, __LINE__, errno, strerror(errno));
  }

  memset(&ifr, 0, sizeof(ifr));
  snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "lo");

  if (setsockopt(gtpv1u_dump_socket_g, SOL_SOCKET, SO_BINDTODEVICE, (void *)&ifr, sizeof(ifr)) < 0) {
    LOG_E(GTPU, "%s:%d  setsockopt SO_BINDTODEVICE %d:%s\n",
          __FILE__, __LINE__, errno, strerror(errno));
    close(gtpv1u_dump_socket_g);
    return -1;
  }
}

//-----------------------------------------------------------------------------
static void gtpv1u_eNB_write_dump_socket(uint8_t *buffer_pP, uint32_t buffer_lengthP)
{
  struct sockaddr_in sin;
  memset(&sin, 0, sizeof(sin));
  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = INADDR_LOOPBACK;

  if (sendto(gtpv1u_dump_socket_g, buffer_pP, (size_t)buffer_lengthP, 0, (struct sockaddr *)&sin, sizeof(struct sockaddr)) < 0)  {
    LOG_E(GTPU, "%s:%s:%d  sendto %d:%s\n",
          __FILE__, __FUNCTION__, __LINE__, errno, strerror(errno));
  }
}

#endif

//-----------------------------------------------------------------------------
static int gtpv1u_eNB_send_init_udp(uint16_t port_number)
{
  // Create and alloc new message
  MessageDef *message_p;
  struct in_addr addr;

  message_p = itti_alloc_new_message(TASK_GTPV1_U, UDP_INIT);

  if (message_p == NULL) {
    return -1;
  }

  UDP_INIT(message_p).port = port_number;
  //LG UDP_INIT(message_p).address = "0.0.0.0"; //ANY address

  addr.s_addr = RC.gtpv1u_data_g->enb_ip_address_for_S1u_S12_S4_up;
  UDP_INIT(message_p).address = inet_ntoa(addr);
  LOG_I(GTPU, "Tx UDP_INIT IP addr %s (%x)\n", UDP_INIT(message_p).address,RC.gtpv1u_data_g->enb_ip_address_for_S1u_S12_S4_up);

  MSC_LOG_EVENT(
	  MSC_GTPU_ENB,
	  "0 UDP bind  %s:%u",
	  UDP_INIT(message_p).address,
	  UDP_INIT(message_p).port);
  return itti_send_msg_to_task(TASK_UDP, INSTANCE_DEFAULT, message_p);
}

//-----------------------------------------------------------------------------
NwGtpv1uRcT gtpv1u_eNB_log_request(NwGtpv1uLogMgrHandleT hLogMgr,
                                   uint32_t logLevel,
                                   NwCharT *file,
                                   uint32_t line,
                                   NwCharT *logStr)
{
  LOG_D(GTPU, "%s\n", logStr);
  return NW_GTPV1U_OK;
}

//-----------------------------------------------------------------------------
NwGtpv1uRcT gtpv1u_eNB_send_udp_msg(
  NwGtpv1uUdpHandleT udpHandle,
  uint8_t *buffer,
  uint32_t buffer_len,
  uint32_t buffer_offset,
  uint32_t peerIpAddr,
  uint16_t peerPort)
{
  // Create and alloc new message
  MessageDef     *message_p       = NULL;
  udp_data_req_t *udp_data_req_p  = NULL;

  message_p = itti_alloc_new_message(TASK_GTPV1_U, UDP_DATA_REQ);

  if (message_p) {
#if defined(LOG_GTPU) && LOG_GTPU > 0
    LOG_D(GTPU, "Sending UDP_DATA_REQ length %u offset %u", buffer_len, buffer_offset);
#endif
    udp_data_req_p = &message_p->ittiMsg.udp_data_req;
    udp_data_req_p->peer_address  = peerIpAddr;
    udp_data_req_p->peer_port     = peerPort;
    udp_data_req_p->buffer        = buffer;
    udp_data_req_p->buffer_length = buffer_len;
    udp_data_req_p->buffer_offset = buffer_offset;
    return itti_send_msg_to_task(TASK_UDP, INSTANCE_DEFAULT, message_p);
  } else {
    return NW_GTPV1U_FAILURE;
  }
}


//-----------------------------------------------------------------------------
/* Callback called when a gtpv1u message arrived on UDP interface */
NwGtpv1uRcT gtpv1u_eNB_process_stack_req(
  NwGtpv1uUlpHandleT hUlp,
  NwGtpv1uUlpApiT   *pUlpApi)
{
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
    /* Nw-gptv1u stack has processed a PDU. we can schedule it to PDCP
     * for transmission.
     */
    teid = pUlpApi->apiInfo.recvMsgInfo.teid;

    if (NW_GTPV1U_OK != nwGtpv1uMsgGetTpdu(pUlpApi->apiInfo.recvMsgInfo.hMsg,
                                           buffer, &buffer_len)) {
      LOG_E(GTPU, "Error while retrieving T-PDU");
    }

    itti_free(TASK_UDP, ((NwGtpv1uMsgT*)pUlpApi->apiInfo.recvMsgInfo.hMsg)->msgBuf);
#if defined(GTP_DUMP_SOCKET) && GTP_DUMP_SOCKET > 0
    gtpv1u_eNB_write_dump_socket(buffer,buffer_len);
#endif

    rc = nwGtpv1uMsgDelete(RC.gtpv1u_data_g->gtpv1u_stack,
                           pUlpApi->apiInfo.recvMsgInfo.hMsg);
    if (rc != NW_GTPV1U_OK) {
      LOG_E(GTPU, "nwGtpv1uMsgDelete failed: 0x%x\n", rc);
    }

    //-----------------------
    // GTPV1U->PDCP mapping
    //-----------------------
    hash_rc = hashtable_get(RC.gtpv1u_data_g->teid_mapping, teid, (void**)&gtpv1u_teid_data_p);

    if (hash_rc == HASH_TABLE_OK) {
#if defined(LOG_GTPU) && LOG_GTPU > 0
      LOG_D(GTPU, "Received T-PDU from gtpv1u stack teid  %u size %d -> enb module id %u ue module id %u rab id %u\n",
            teid,
            buffer_len,
            gtpv1u_teid_data_p->enb_id,
            gtpv1u_teid_data_p->ue_id,
            gtpv1u_teid_data_p->eps_bearer_id);
#endif

//#warning "LG eps bearer mapping to DRB id to do (offset -4)"
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
			     PDCP_TRANSMISSION_MODE_DATA);
      
      
      if ( result == FALSE ) {
	
	if (ctxt.configured == FALSE )
	  LOG_W(GTPU, "PDCP data request failed, cause: RB is not configured!\n") ;
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


//-----------------------------------------------------------------------------
int data_recv_callback(uint16_t  portP,
                       uint32_t  address,
                       uint8_t  *buffer,
                       uint32_t  length,
                       void     *arg_p)
{
  gtpv1u_data_t        *gtpv1u_data_p;

  if (arg_p == NULL) {
    return -1;
  }

  gtpv1u_data_p = (gtpv1u_data_t *)arg_p;

  return nwGtpv1uProcessUdpReq(gtpv1u_data_p->gtpv1u_stack,
                               buffer,
                               length,
                               portP,
                               address);
}

//int
//gtpv1u_create_tunnel_endpoint(
//    gtpv1u_data_t *gtpv1u_data_pP,
//    uint8_t        ue_idP,
//    uint8_t        rab_idP,
//    char          *sgw_ip_addr_pP,
//    uint16_t       portP)
//{
//    uint32_t                     teid;
//    uint8_t                      max_attempt = 100;
//    NwGtpv1uRcT                  rc          = NW_GTPV1U_FAILURE;
//    NwGtpv1uUlpApiT              ulp_req;
//    struct gtpv1u_ue_data_s     *new_ue_p    = NULL;
//    struct gtpv1u_bearer_s      *bearer_p    = NULL;
//    hashtable_rc_t               hash_rc     = HASH_TABLE_KEY_NOT_EXISTS;;
//
//    if (rab_idP > GTPV1U_MAX_BEARERS_PER_UE) {
//        LOG_E(GTPU, "Could not use rab_id %d > max %d\n",
//              rab_idP, GTPV1U_MAX_BEARERS_PER_UE);
//        return -1;
//    }
//
//
//    if ((hash_rc = hashtable_get(gtpv1u_data_pP->ue_mapping, (uint64_t)ue_idP, (void**)&new_ue_p)) == HASH_TABLE_OK) {
//        /* A context for this UE already exist in the tree, use it */
//        /* We check that the tunnel is not already configured */
//        if (new_ue_p->bearers[rab_idP].state != BEARER_DOWN) {
//            LOG_E(GTPU, "Cannot create new end-point over already existing tunnel\n");
//            return -1;
//        }
//    } else {
//        /* Context doesn't exist, create it */
//        if (rab_idP != 0) {
//            /* UE should first establish Default bearer before trying to setup
//             * additional bearers.
//             */
//            LOG_E(GTPU, "UE context is not known and rab_id != 0\n");
//            return -1;
//        }
//        new_ue_p = calloc(1, sizeof(struct gtpv1u_ue_data_s));
//        new_ue_p->ue_id = ue_idP;
//
//        hash_rc = hashtable_insert(gtpv1u_data_pP->ue_mapping, (uint64_t)ue_idP, new_ue_p);
//
//        if ((hash_rc != HASH_TABLE_OK) && (hash_rc != HASH_TABLE_INSERT_OVERWRITTEN_DATA)) {
//            LOG_E(GTPU, "Failed to insert new UE context\n");
//            free(new_ue_p);
//            return -1;
//        }
//    }
//
//    bearer_p = &new_ue_p->bearers[rab_idP];
//
//    /* Configure the bearer */
//    bearer_p->state       = BEARER_IN_CONFIG;
//    bearer_p->sgw_ip_addr = inet_addr(sgw_ip_addr_pP);
//    bearer_p->port        = portP;
//
//    /* Create the new stack api request */
//    memset(&ulp_req, 0, sizeof(NwGtpv1uUlpApiT));
//    ulp_req.apiType = NW_GTPV1U_ULP_API_CREATE_TUNNEL_ENDPOINT;
//
//    /* Try to create new tunnel-endpoint.
//     * If teid generated is already present in the stack, just peek another random
//     * teid. This could be ok for small number of tunnel but more errors could be
//     * thrown if we reached high number of tunnels.
//     * TODO: find a solution for teid
//     */
//    do {
//        /* Request for a new random TEID */
//        teid = gtpv1u_new_teid();
//        ulp_req.apiInfo.createTunnelEndPointInfo.teid = teid;
//
//        rc = nwGtpv1uProcessUlpReq(gtpv1u_data_pP->gtpv1u_stack, &ulp_req);
//
//        if (rc == NW_GTPV1U_OK) {
////             LOG_D(GTPU, "Successfully created new tunnel endpoint for teid 0x%x\n",
////                   teid);
//            bearer_p->teid_eNB = teid;
////             gtpv1u_initial_req(gtpv1u_data_pP, teid, GTPV1U_UDP_PORT,
////                                inet_addr("192.168.56.101"));
//            LOG_I(GTPU, "Created eNB tunnel endpoint %u for ue id %u, rab id %u\n", teid, ue_idP, rab_idP);
//            return 0;
//        } else {
//            LOG_W(GTPU, "Teid %u already in use... %s\n",
//                  teid, (max_attempt > 1) ? "Trying another one" : "Last chance");
//        }
//    } while(max_attempt-- && rc != NW_GTPV1U_OK);
//
//    bearer_p->state = BEARER_DOWN;
//    LOG_I(GTPU, "Failed to created eNB tunnel endpoint %u for ue id %u, rab id %u, bearer down\n", teid, ue_idP, rab_idP);
//
//    return -1;
//}


//-----------------------------------------------------------------------------
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


//-----------------------------------------------------------------------------
static NwGtpv1uRcT
gtpv1u_stop_timer_wrapper(
  NwGtpv1uTimerMgrHandleT tmrMgrHandle,
  NwGtpv1uTimerHandleT hTmr)
{

  NwGtpv1uRcT rc = NW_GTPV1U_OK;

  return rc;
}


//-----------------------------------------------------------------------------
int
gtpv1u_initial_req(
  gtpv1u_data_t *gtpv1u_data_pP,
  teid_t         teidP,
  tcp_udp_port_t portP,
  uint32_t       address)
{
  NwGtpv1uUlpApiT ulp_req;
  NwGtpv1uRcT     rc = NW_GTPV1U_FAILURE;

  memset(&ulp_req, 0, sizeof(NwGtpv1uUlpApiT));

  ulp_req.apiType = NW_GTPV1U_ULP_API_INITIAL_REQ;
  ulp_req.apiInfo.initialReqInfo.teid     = teidP;
  ulp_req.apiInfo.initialReqInfo.peerPort = portP;
  ulp_req.apiInfo.initialReqInfo.peerIp   = address;

  rc = nwGtpv1uProcessUlpReq(gtpv1u_data_pP->gtpv1u_stack, &ulp_req);

  if (rc == NW_GTPV1U_OK) {
    LOG_D(GTPU, "Successfully sent initial req for teid %u\n", teidP);
  } else {
    LOG_W(GTPU, "Could not send initial req for teid %u\n", teidP);
  }

  return (rc == NW_GTPV1U_OK) ? 0 : -1;
}

//-----------------------------------------------------------------------------
int
gtpv1u_new_data_req(
  uint8_t  enb_module_idP,
  rnti_t   ue_rntiP,
  uint8_t  rab_idP,
  uint8_t *buffer_pP,
  uint32_t buf_lenP,
  uint32_t buf_offsetP
)
{

  NwGtpv1uUlpApiT          stack_req;
  NwGtpv1uRcT              rc            = NW_GTPV1U_FAILURE;
  struct gtpv1u_ue_data_s  ue;
  struct gtpv1u_ue_data_s *ue_inst_p     = NULL;
  struct gtpv1u_bearer_s  *bearer_p      = NULL;
  hashtable_rc_t           hash_rc       = HASH_TABLE_KEY_NOT_EXISTS;;
  gtpv1u_data_t           *gtpv1u_data_p = NULL;

  memset(&ue, 0, sizeof(struct gtpv1u_ue_data_s));
  ue.ue_id = ue_rntiP;

  AssertFatal(enb_module_idP >=0, "Bad parameter enb module id %u\n", enb_module_idP);
  AssertFatal((rab_idP - GTPV1U_BEARER_OFFSET)< GTPV1U_MAX_BEARERS_ID, "Bad parameter rab id %u\n", rab_idP);
  AssertFatal((rab_idP - GTPV1U_BEARER_OFFSET) >= 0 , "Bad parameter rab id %u\n", rab_idP);

  gtpv1u_data_p = RC.gtpv1u_data_g;
  /* Check that UE context is present in ue map. */
  hash_rc = hashtable_get(gtpv1u_data_p->ue_mapping, (uint64_t)ue_rntiP, (void**)&ue_inst_p);

  if (hash_rc ==  HASH_TABLE_KEY_NOT_EXISTS ) {
    LOG_E(GTPU, "[UE %d] Trying to send data on non-existing UE context\n", ue_rntiP);
    return -1;
  }

  bearer_p = &ue_inst_p->bearers[rab_idP - GTPV1U_BEARER_OFFSET];

  /* Ensure the bearer in ready.
   * TODO: handle the cases where the bearer is in HANDOVER state.
   * In such case packets should be placed in FIFO.
   */
  if (bearer_p->state != BEARER_UP) {
    LOG_W(GTPU, "Trying to send data over bearer with state(%u) != BEARER_UP\n",
          bearer_p->state);
//#warning  LG: HACK WHILE WAITING FOR NAS, normally return -1

    if (bearer_p->state != BEARER_IN_CONFIG)
      return -1;
  }

  memset(&stack_req, 0, sizeof(NwGtpv1uUlpApiT));

  stack_req.apiType                   = NW_GTPV1U_ULP_API_SEND_TPDU;
  stack_req.apiInfo.sendtoInfo.teid   = bearer_p->teid_sgw;
  stack_req.apiInfo.sendtoInfo.ipAddr = bearer_p->sgw_ip_addr;

  LOG_D(GTPU, "TX TO TEID %u addr 0x%x\n",bearer_p->teid_sgw, bearer_p->sgw_ip_addr);
  rc = nwGtpv1uGpduMsgNew(gtpv1u_data_p->gtpv1u_stack,
                          bearer_p->teid_sgw,
                          NW_FALSE,
                          gtpv1u_data_p->seq_num++,
                          buffer_pP,
                          buf_lenP,
                          buf_offsetP,
                          &(stack_req.apiInfo.sendtoInfo.hMsg));

  if (rc != NW_GTPV1U_OK) {
    LOG_E(GTPU, "nwGtpv1uGpduMsgNew failed: 0x%x\n", rc);
    return -1;
  }

  rc = nwGtpv1uProcessUlpReq(gtpv1u_data_p->gtpv1u_stack,
                             &stack_req);

  if (rc != NW_GTPV1U_OK) {
    LOG_E(GTPU, "nwGtpv1uProcessUlpReq failed: 0x%x\n", rc);
    return -1;
  }

  rc = nwGtpv1uMsgDelete(gtpv1u_data_p->gtpv1u_stack,
                         stack_req.apiInfo.sendtoInfo.hMsg);

  if (rc != NW_GTPV1U_OK) {
    LOG_E(GTPU, "nwGtpv1uMsgDelete failed: 0x%x\n", rc);
    return -1;
  }

  LOG_D(GTPU, "%s() return code OK\n", __FUNCTION__);
  return 0;
}

//-----------------------------------------------------------------------------
int
gtpv1u_create_s1u_tunnel(
  const instance_t                              instanceP,
  const gtpv1u_enb_create_tunnel_req_t * const  create_tunnel_req_pP,
        gtpv1u_enb_create_tunnel_resp_t * const create_tunnel_resp_pP
  )
{
  /* Create a new nw-gtpv1-u stack req using API */
  NwGtpv1uUlpApiT          stack_req;
  NwGtpv1uRcT              rc                   = NW_GTPV1U_FAILURE;
  /* Local tunnel end-point identifier */
  teid_t                   s1u_teid             = 0;
  gtpv1u_teid_data_t      *gtpv1u_teid_data_p   = NULL;
  gtpv1u_ue_data_t        *gtpv1u_ue_data_p     = NULL;
  //MessageDef              *message_p            = NULL;
  hashtable_rc_t           hash_rc              = HASH_TABLE_KEY_NOT_EXISTS;
  int                      i;
  ebi_t                    eps_bearer_id        = 0;
  //    int                      ipv4_addr            = 0;
  int                      ip_offset            = 0;
  in_addr_t                in_addr;
  int                      addrs_length_in_bytes= 0;


  MSC_LOG_RX_MESSAGE(
		  MSC_GTPU_ENB,
		  MSC_RRC_ENB,
		  NULL,0,
		  MSC_AS_TIME_FMT" CREATE_TUNNEL_REQ RNTI %"PRIx16" inst %u ntuns %u ebid %u sgw-s1u teid %u",
		  0,0,create_tunnel_req_pP->rnti, instanceP,
		  create_tunnel_req_pP->num_tunnels, create_tunnel_req_pP->eps_bearer_id[0],
		  create_tunnel_req_pP->sgw_S1u_teid[0]);

  create_tunnel_resp_pP->rnti        = create_tunnel_req_pP->rnti;
  create_tunnel_resp_pP->status      = 0;
  create_tunnel_resp_pP->num_tunnels = 0;

  for (i = 0; i < create_tunnel_req_pP->num_tunnels; i++) {
    ip_offset               = 0;
    eps_bearer_id = create_tunnel_req_pP->eps_bearer_id[i];
    LOG_D(GTPU, "Rx GTPV1U_ENB_CREATE_TUNNEL_REQ ue rnti %x eps bearer id %u\n",
          create_tunnel_req_pP->rnti, eps_bearer_id);
    memset(&stack_req, 0, sizeof(NwGtpv1uUlpApiT));

    stack_req.apiType = NW_GTPV1U_ULP_API_CREATE_TUNNEL_ENDPOINT;

    do {
      s1u_teid = gtpv1u_new_teid();
      LOG_D(GTPU, "gtpv1u_create_s1u_tunnel() 0x%x %u(dec)\n", s1u_teid, s1u_teid);
      stack_req.apiInfo.createTunnelEndPointInfo.teid          = s1u_teid;
      stack_req.apiInfo.createTunnelEndPointInfo.hUlpSession   = 0;
      stack_req.apiInfo.createTunnelEndPointInfo.hStackSession = 0;

      rc = nwGtpv1uProcessUlpReq(RC.gtpv1u_data_g->gtpv1u_stack, &stack_req);
      LOG_D(GTPU, ".\n");
    } while (rc != NW_GTPV1U_OK);

    //-----------------------
    // PDCP->GTPV1U mapping
    //-----------------------
    hash_rc = hashtable_get(RC.gtpv1u_data_g->ue_mapping, create_tunnel_req_pP->rnti, (void **)&gtpv1u_ue_data_p);

    if ((hash_rc == HASH_TABLE_KEY_NOT_EXISTS) || (hash_rc == HASH_TABLE_OK)) {

      if (hash_rc == HASH_TABLE_KEY_NOT_EXISTS) {
        gtpv1u_ue_data_p = calloc (1, sizeof(gtpv1u_ue_data_t));
        hash_rc = hashtable_insert(RC.gtpv1u_data_g->ue_mapping, create_tunnel_req_pP->rnti, gtpv1u_ue_data_p);
        AssertFatal(hash_rc == HASH_TABLE_OK, "Error inserting ue_mapping in GTPV1U hashtable");
      }

      gtpv1u_ue_data_p->ue_id       = create_tunnel_req_pP->rnti;
      gtpv1u_ue_data_p->instance_id = 0; // TO DO
      memcpy(&create_tunnel_resp_pP->enb_addr.buffer,
             &RC.gtpv1u_data_g->enb_ip_address_for_S1u_S12_S4_up,
             sizeof (in_addr_t));
      create_tunnel_resp_pP->enb_addr.length = sizeof (in_addr_t);

      addrs_length_in_bytes = create_tunnel_req_pP->sgw_addr[i].length / 8;
      AssertFatal((addrs_length_in_bytes == 4) ||
                  (addrs_length_in_bytes == 16) ||
                  (addrs_length_in_bytes == 20),
                  "Bad transport layer address length %d (bits) %d (bytes)",
                  create_tunnel_req_pP->sgw_addr[i].length, addrs_length_in_bytes);

      if ((addrs_length_in_bytes == 4) ||
          (addrs_length_in_bytes == 20)) {
        in_addr = *((in_addr_t*)create_tunnel_req_pP->sgw_addr[i].buffer);
        ip_offset = 4;
        gtpv1u_ue_data_p->bearers[eps_bearer_id - GTPV1U_BEARER_OFFSET].sgw_ip_addr = in_addr;
      }

      if ((addrs_length_in_bytes == 16) ||
          (addrs_length_in_bytes == 20)) {
        memcpy(gtpv1u_ue_data_p->bearers[eps_bearer_id - GTPV1U_BEARER_OFFSET].sgw_ip6_addr.s6_addr,
               &create_tunnel_req_pP->sgw_addr[i].buffer[ip_offset],
               16);
      }

      gtpv1u_ue_data_p->bearers[eps_bearer_id - GTPV1U_BEARER_OFFSET].state                  = BEARER_IN_CONFIG;
      gtpv1u_ue_data_p->bearers[eps_bearer_id - GTPV1U_BEARER_OFFSET].teid_eNB               = s1u_teid;
      gtpv1u_ue_data_p->bearers[eps_bearer_id - GTPV1U_BEARER_OFFSET].teid_eNB_stack_session = stack_req.apiInfo.createTunnelEndPointInfo.hStackSession;
      gtpv1u_ue_data_p->bearers[eps_bearer_id - GTPV1U_BEARER_OFFSET].teid_sgw               = create_tunnel_req_pP->sgw_S1u_teid[i];
      gtpv1u_ue_data_p->num_bearers++;
      create_tunnel_resp_pP->enb_S1u_teid[i] = s1u_teid;

    } else {
      create_tunnel_resp_pP->enb_S1u_teid[i] = 0;
      create_tunnel_resp_pP->status         = 0xFF;
    }

    create_tunnel_resp_pP->eps_bearer_id[i] = eps_bearer_id;
    create_tunnel_resp_pP->num_tunnels      += 1;

    //-----------------------
    // GTPV1U->PDCP mapping
    //-----------------------
    hash_rc = hashtable_get(RC.gtpv1u_data_g->teid_mapping, s1u_teid, (void**)&gtpv1u_teid_data_p);

    if (hash_rc == HASH_TABLE_KEY_NOT_EXISTS) {
      gtpv1u_teid_data_p = calloc (1, sizeof(gtpv1u_teid_data_t));
      gtpv1u_teid_data_p->enb_id        = 0; // TO DO
      gtpv1u_teid_data_p->ue_id         = create_tunnel_req_pP->rnti;
      gtpv1u_teid_data_p->eps_bearer_id = eps_bearer_id;
      hash_rc = hashtable_insert(RC.gtpv1u_data_g->teid_mapping, s1u_teid, gtpv1u_teid_data_p);
      AssertFatal(hash_rc == HASH_TABLE_OK, "Error inserting teid mapping in GTPV1U hashtable");
    } else {
      create_tunnel_resp_pP->enb_S1u_teid[i] = 0;
      create_tunnel_resp_pP->status         = 0xFF;
    }
  }
  MSC_LOG_TX_MESSAGE(
		  MSC_GTPU_ENB,
		  MSC_RRC_ENB,
		  NULL,0,
		  "0 GTPV1U_ENB_CREATE_TUNNEL_RESP rnti %x teid %x",
		  create_tunnel_resp_pP->rnti,
		  s1u_teid);

  LOG_D(GTPU, "Tx GTPV1U_ENB_CREATE_TUNNEL_RESP ue rnti %x status %d\n",
        create_tunnel_req_pP->rnti,
        create_tunnel_resp_pP->status);
  return 0;
}

int gtpv1u_update_s1u_tunnel(
    const instance_t                              instanceP,
    const gtpv1u_enb_create_tunnel_req_t * const  create_tunnel_req_pP,
    const rnti_t                                  prior_rnti
    )
{

  /* Local tunnel end-point identifier */
  teid_t                   s1u_teid             = 0;
  gtpv1u_teid_data_t      *gtpv1u_teid_data_p   = NULL;
  gtpv1u_ue_data_t        *gtpv1u_ue_data_p     = NULL;
  gtpv1u_ue_data_t        *gtpv1u_ue_data_new_p     = NULL;
  //MessageDef              *message_p            = NULL;
  hashtable_rc_t           hash_rc              = HASH_TABLE_KEY_NOT_EXISTS;
  int                      i,j;
  uint8_t                  bearers_num = 0,bearers_total = 0;

  //-----------------------
  // PDCP->GTPV1U mapping
  //-----------------------
  hash_rc = hashtable_get(RC.gtpv1u_data_g->ue_mapping, prior_rnti, (void **)&gtpv1u_ue_data_p);
  if(hash_rc != HASH_TABLE_OK){
    LOG_E(GTPU,"Error get ue_mapping(rnti=%x) from GTPV1U hashtable error\n", prior_rnti);
    return -1;
  }

  gtpv1u_ue_data_new_p = calloc (1, sizeof(gtpv1u_ue_data_t));
  memcpy(gtpv1u_ue_data_new_p,gtpv1u_ue_data_p,sizeof(gtpv1u_ue_data_t));
  gtpv1u_ue_data_new_p->ue_id       = create_tunnel_req_pP->rnti;

  hash_rc = hashtable_insert(RC.gtpv1u_data_g->ue_mapping, create_tunnel_req_pP->rnti, gtpv1u_ue_data_new_p);
  AssertFatal(hash_rc == HASH_TABLE_OK, "Error inserting ue_mapping in GTPV1U hashtable");
  LOG_I(GTPU, "inserting ue_mapping(rnti=%x) in GTPV1U hashtable\n",
      create_tunnel_req_pP->rnti);

  hash_rc = hashtable_remove(RC.gtpv1u_data_g->ue_mapping, prior_rnti);
  LOG_I(GTPU, "hashtable_remove ue_mapping(rnti=%x) in GTPV1U hashtable\n",
		  prior_rnti);
  //-----------------------
  // GTPV1U->PDCP mapping
  //-----------------------
  bearers_total =gtpv1u_ue_data_new_p->num_bearers;
  for(j = 0;j<GTPV1U_MAX_BEARERS_ID;j++){

    if(gtpv1u_ue_data_new_p->bearers[j].state != BEARER_IN_CONFIG)
      continue;

    bearers_num++;
    for (i = 0; i < create_tunnel_req_pP->num_tunnels; i++) {
      if(j == (create_tunnel_req_pP->eps_bearer_id[i]-GTPV1U_BEARER_OFFSET))
        break;
    }
    if(i < create_tunnel_req_pP->num_tunnels){
      s1u_teid = gtpv1u_ue_data_new_p->bearers[j].teid_eNB;
      hash_rc = hashtable_get(RC.gtpv1u_data_g->teid_mapping, s1u_teid, (void**)&gtpv1u_teid_data_p);
      if (hash_rc == HASH_TABLE_OK) {
        gtpv1u_teid_data_p->ue_id         = create_tunnel_req_pP->rnti;
        gtpv1u_teid_data_p->eps_bearer_id = create_tunnel_req_pP->eps_bearer_id[i];

        LOG_I(GTPU, "updata teid_mapping te_id %u (prior_rnti %x rnti %x) in GTPV1U hashtable\n",
              s1u_teid,prior_rnti,create_tunnel_req_pP->rnti);
      }else{
        LOG_W(GTPU, "Error get teid mapping(s1u_teid=%u) from GTPV1U hashtable", s1u_teid);
      }
    }else{
      s1u_teid = gtpv1u_ue_data_new_p->bearers[j].teid_eNB;
      hash_rc = hashtable_remove(RC.gtpv1u_data_g->teid_mapping, s1u_teid);

      if (hash_rc != HASH_TABLE_OK) {
        LOG_D(GTPU, "Removed user rnti %x , enb S1U teid %u not found\n", prior_rnti, s1u_teid);
      }
      gtpv1u_ue_data_new_p->bearers[j].state = BEARER_DOWN;
      gtpv1u_ue_data_new_p->num_bearers--;
      LOG_I(GTPU, "delete teid_mapping te_id %u (rnti%x) bearer_id %d in GTPV1U hashtable\n",
            s1u_teid,prior_rnti,j+GTPV1U_BEARER_OFFSET);;
    }
    if(bearers_num > bearers_total)
      break;
  }
  return 0;

}

//-----------------------------------------------------------------------------
static int gtpv1u_delete_s1u_tunnel(
  const instance_t                             instanceP,
  const gtpv1u_enb_delete_tunnel_req_t * const req_pP)
{
  NwGtpv1uUlpApiT          stack_req;
  NwGtpv1uRcT              rc                   = NW_GTPV1U_FAILURE;
  MessageDef              *message_p = NULL;
  gtpv1u_ue_data_t        *gtpv1u_ue_data_p     = NULL;
  hashtable_rc_t           hash_rc              = HASH_TABLE_KEY_NOT_EXISTS;
  teid_t                   teid_eNB             = 0;
  int                      erab_index           = 0;

  message_p = itti_alloc_new_message(TASK_GTPV1_U, GTPV1U_ENB_DELETE_TUNNEL_RESP);

  GTPV1U_ENB_DELETE_TUNNEL_RESP(message_p).rnti     = req_pP->rnti;
  GTPV1U_ENB_DELETE_TUNNEL_RESP(message_p).status       = 0;


  hash_rc = hashtable_get(RC.gtpv1u_data_g->ue_mapping, req_pP->rnti, (void**)&gtpv1u_ue_data_p);

  if (hash_rc == HASH_TABLE_OK) {

    for (erab_index = 0; erab_index < req_pP->num_erab; erab_index++) {
      teid_eNB = gtpv1u_ue_data_p->bearers[req_pP->eps_bearer_id[erab_index] - GTPV1U_BEARER_OFFSET].teid_eNB;
      LOG_D(GTPU, "Rx GTPV1U_ENB_DELETE_TUNNEL user rnti %x eNB S1U teid %u eps bearer id %u\n",
            req_pP->rnti, teid_eNB, req_pP->eps_bearer_id[erab_index]);

      {
        memset(&stack_req, 0, sizeof(NwGtpv1uUlpApiT));
        stack_req.apiType = NW_GTPV1U_ULP_API_DESTROY_TUNNEL_ENDPOINT;
        LOG_D(GTPU, "gtpv1u_delete_s1u_tunnel erab %u  %u\n",
              req_pP->eps_bearer_id[erab_index],
              teid_eNB);
        stack_req.apiInfo.destroyTunnelEndPointInfo.hStackSessionHandle   = gtpv1u_ue_data_p->bearers[req_pP->eps_bearer_id[erab_index] - GTPV1U_BEARER_OFFSET].teid_eNB_stack_session;

        rc = nwGtpv1uProcessUlpReq(RC.gtpv1u_data_g->gtpv1u_stack, &stack_req);
        LOG_D(GTPU, ".\n");
      }

      if (rc != NW_GTPV1U_OK) {
        GTPV1U_ENB_DELETE_TUNNEL_RESP(message_p).status       |= 0xFF;
        LOG_E(GTPU, "NW_GTPV1U_ULP_API_DESTROY_TUNNEL_ENDPOINT failed");
      }

      //-----------------------
      // PDCP->GTPV1U mapping
      //-----------------------
      gtpv1u_ue_data_p->bearers[req_pP->eps_bearer_id[erab_index] - GTPV1U_BEARER_OFFSET].state       = BEARER_DOWN;
      gtpv1u_ue_data_p->bearers[req_pP->eps_bearer_id[erab_index] - GTPV1U_BEARER_OFFSET].teid_eNB    = 0;
      gtpv1u_ue_data_p->bearers[req_pP->eps_bearer_id[erab_index] - GTPV1U_BEARER_OFFSET].teid_sgw    = 0;
      gtpv1u_ue_data_p->bearers[req_pP->eps_bearer_id[erab_index] - GTPV1U_BEARER_OFFSET].sgw_ip_addr = 0;
      gtpv1u_ue_data_p->num_bearers -= 1;

      if (gtpv1u_ue_data_p->num_bearers == 0) {
        hash_rc = hashtable_remove(RC.gtpv1u_data_g->ue_mapping, req_pP->rnti);
        LOG_D(GTPU, "Removed user rnti %x,no more bearers configured\n", req_pP->rnti);
      }

      //-----------------------
      // GTPV1U->PDCP mapping
      //-----------------------
      hash_rc = hashtable_remove(RC.gtpv1u_data_g->teid_mapping, teid_eNB);

      if (hash_rc != HASH_TABLE_OK) {
        LOG_D(GTPU, "Removed user rnti %x , enb S1U teid %u not found\n", req_pP->rnti, teid_eNB);
      }
    }
  }// else silently do nothing


  LOG_D(GTPU, "Tx GTPV1U_ENB_DELETE_TUNNEL_RESP user rnti %x eNB S1U teid %u status %u\n",
        GTPV1U_ENB_DELETE_TUNNEL_RESP(message_p).rnti,
        GTPV1U_ENB_DELETE_TUNNEL_RESP(message_p).enb_S1u_teid,
        GTPV1U_ENB_DELETE_TUNNEL_RESP(message_p).status);

  MSC_LOG_TX_MESSAGE(
		  MSC_GTPU_ENB,
		  MSC_RRC_ENB,
		  NULL,0,
		  "0 GTPV1U_ENB_DELETE_TUNNEL_RESP rnti %x teid %x",
		  GTPV1U_ENB_DELETE_TUNNEL_RESP(message_p).rnti,
		  teid_eNB);

  return itti_send_msg_to_task(TASK_RRC_ENB, instanceP, message_p);
}


//-----------------------------------------------------------------------------
static int gtpv1u_eNB_init(void)
{
  int                     ret;
  NwGtpv1uRcT             rc = NW_GTPV1U_FAILURE;
  NwGtpv1uUlpEntityT      ulp;
  NwGtpv1uUdpEntityT      udp;
  NwGtpv1uLogMgrEntityT   log;
  NwGtpv1uTimerMgrEntityT tmr;


  //  enb_properties_p = enb_config_get()->properties[0];
  RC.gtpv1u_data_g = (gtpv1u_data_t*)malloc(sizeof(gtpv1u_data_t));
  memset(RC.gtpv1u_data_g, 0, sizeof(gtpv1u_data_t));

  RCconfig_gtpu();


  LOG_I(GTPU, "Initializing GTPU stack %p\n",&RC.gtpv1u_data_g);
  //gtpv1u_data_g.gtpv1u_stack;
  /* Initialize UE hashtable */
  RC.gtpv1u_data_g->ue_mapping      = hashtable_create (32, NULL, NULL);
  AssertFatal(RC.gtpv1u_data_g->ue_mapping != NULL, " ERROR Initializing TASK_GTPV1_U task interface: in hashtable_create returned %p\n", RC.gtpv1u_data_g->ue_mapping);
  RC.gtpv1u_data_g->teid_mapping    = hashtable_create (256, NULL, NULL);
  AssertFatal(RC.gtpv1u_data_g->teid_mapping != NULL, " ERROR Initializing TASK_GTPV1_U task interface: in hashtable_create\n");
//  RC.gtpv1u_data_g.enb_ip_address_for_S1u_S12_S4_up         = enb_properties_p->enb_ipv4_address_for_S1U;
  RC.gtpv1u_data_g->ip_addr         = NULL;

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
  ulp.ulpReqCallback = gtpv1u_eNB_process_stack_req;

  if ((rc = nwGtpv1uSetUlpEntity(RC.gtpv1u_data_g->gtpv1u_stack, &ulp)) != NW_GTPV1U_OK) {
    LOG_E(GTPU, "nwGtpv1uSetUlpEntity: %x", rc);
    return -1;
  }

  /* nw-gtpv1u stack requires an udp callback to send data over UDP.
   * We provide a wrapper to UDP task.
   */
  udp.udpDataReqCallback = gtpv1u_eNB_send_udp_msg;

  if ((rc = nwGtpv1uSetUdpEntity(RC.gtpv1u_data_g->gtpv1u_stack, &udp)) != NW_GTPV1U_OK) {
    LOG_E(GTPU, "nwGtpv1uSetUdpEntity: %x", rc);
    return -1;
  }

  log.logReqCallback = gtpv1u_eNB_log_request;

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
  ret = gtpv1u_eNB_send_init_udp(RC.gtpv1u_data_g->enb_port_for_S1u_S12_S4_up);

  if (ret < 0) {
    return ret;
  }

  LOG_D(GTPU, "Initializing GTPV1U interface for eNB: DONE\n");
  return 0;
}


//-----------------------------------------------------------------------------
void *gtpv1u_eNB_task(void *args)
{
  int                       rc = 0;
  instance_t                instance;
  //const char               *msg_name_p;

  rc = gtpv1u_eNB_init();
  AssertFatal(rc == 0, "gtpv1u_eNB_init Failed");
  itti_mark_task_ready(TASK_GTPV1_U);
  MSC_START_USE();

  while(1) {
    /* Trying to fetch a message from the message queue.
     * If the queue is empty, this function will block till a
     * message is sent to the task.
     */
    MessageDef *received_message_p = NULL;
    itti_receive_msg(TASK_GTPV1_U, &received_message_p);
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_GTPV1U_ENB_TASK, VCD_FUNCTION_IN);
    DevAssert(received_message_p != NULL);

    instance = ITTI_MSG_INSTANCE(received_message_p);
    //msg_name_p = ITTI_MSG_NAME(received_message_p);

    switch (ITTI_MSG_ID(received_message_p)) {

    case GTPV1U_ENB_DELETE_TUNNEL_REQ: {
      gtpv1u_delete_s1u_tunnel(instance, &received_message_p->ittiMsg.Gtpv1uDeleteTunnelReq);
    }
    break;

    // DATA COMING FROM UDP
    case UDP_DATA_IND: {
      udp_data_ind_t *udp_data_ind_p;
      udp_data_ind_p = &received_message_p->ittiMsg.udp_data_ind;
      nwGtpv1uProcessUdpReq(RC.gtpv1u_data_g->gtpv1u_stack,
                            udp_data_ind_p->buffer,
                            udp_data_ind_p->buffer_length,
                            udp_data_ind_p->peer_port,
                            udp_data_ind_p->peer_address);
      //itti_free(ITTI_MSG_ORIGIN_ID(received_message_p), udp_data_ind_p->buffer);
    }
    break;

    // DATA TO BE SENT TO UDP
    case GTPV1U_ENB_TUNNEL_DATA_REQ: {
      gtpv1u_enb_tunnel_data_req_t *data_req_p           = NULL;
      NwGtpv1uUlpApiT               stack_req;
      NwGtpv1uRcT                   rc                   = NW_GTPV1U_FAILURE;
      hashtable_rc_t                hash_rc              = HASH_TABLE_KEY_NOT_EXISTS;
      gtpv1u_ue_data_t             *gtpv1u_ue_data_p     = NULL;
      teid_t                        enb_s1u_teid         = 0;
      teid_t                        sgw_s1u_teid         = 0;

      VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_GTPV1U_PROCESS_TUNNEL_DATA_REQ, VCD_FUNCTION_IN);
      data_req_p = &GTPV1U_ENB_TUNNEL_DATA_REQ(received_message_p);
      //ipv4_send_data(ipv4_data_p->sd, data_ind_p->buffer, data_ind_p->length);

#if defined(GTP_DUMP_SOCKET) && GTP_DUMP_SOCKET > 0
      gtpv1u_eNB_write_dump_socket(&data_req_p->buffer[data_req_p->offset],data_req_p->length);
#endif
      memset(&stack_req, 0, sizeof(NwGtpv1uUlpApiT));

      hash_rc = hashtable_get(RC.gtpv1u_data_g->ue_mapping, (uint64_t)data_req_p->rnti, (void**)&gtpv1u_ue_data_p);

      if (hash_rc == HASH_TABLE_KEY_NOT_EXISTS) {
        LOG_E(GTPU, "nwGtpv1uProcessUlpReq failed: while getting ue rnti %x in hashtable ue_mapping\n", data_req_p->rnti);
      } else {
        if ((data_req_p->rab_id >= GTPV1U_BEARER_OFFSET) && (data_req_p->rab_id <= max_val_DRB_Identity)) {
          enb_s1u_teid                        = gtpv1u_ue_data_p->bearers[data_req_p->rab_id - GTPV1U_BEARER_OFFSET].teid_eNB;
          sgw_s1u_teid                        = gtpv1u_ue_data_p->bearers[data_req_p->rab_id - GTPV1U_BEARER_OFFSET].teid_sgw;
          stack_req.apiType                   = NW_GTPV1U_ULP_API_SEND_TPDU;
          stack_req.apiInfo.sendtoInfo.teid   = sgw_s1u_teid;
          stack_req.apiInfo.sendtoInfo.ipAddr = gtpv1u_ue_data_p->bearers[data_req_p->rab_id - GTPV1U_BEARER_OFFSET].sgw_ip_addr;

          rc = nwGtpv1uGpduMsgNew(
                 RC.gtpv1u_data_g->gtpv1u_stack,
                 sgw_s1u_teid,
                 NW_FALSE,
                 RC.gtpv1u_data_g->seq_num++,
                 data_req_p->buffer,
                 data_req_p->length,
                 data_req_p->offset,
                 &(stack_req.apiInfo.sendtoInfo.hMsg));

          if (rc != NW_GTPV1U_OK) {
            LOG_E(GTPU, "nwGtpv1uGpduMsgNew failed: 0x%x\n", rc);
            MSC_LOG_EVENT(MSC_GTPU_ENB,"0 Failed send G-PDU ltid %u rtid %u size %u",
            		enb_s1u_teid,sgw_s1u_teid,data_req_p->length);
            (void)enb_s1u_teid; /* avoid gcc warning "set but not used" */
          } else {
            rc = nwGtpv1uProcessUlpReq(RC.gtpv1u_data_g->gtpv1u_stack, &stack_req);

            if (rc != NW_GTPV1U_OK) {
              LOG_E(GTPU, "nwGtpv1uProcessUlpReq failed: 0x%x\n", rc);
              MSC_LOG_EVENT(MSC_GTPU_ENB,"0 Failed send G-PDU ltid %u rtid %u size %u",
              		enb_s1u_teid,sgw_s1u_teid,data_req_p->length);
            } else {
            	  MSC_LOG_TX_MESSAGE(
            			  MSC_GTPU_ENB,
            			  MSC_GTPU_SGW,
            			  NULL,
            			  0,
            			  MSC_AS_TIME_FMT" G-PDU ltid %u rtid %u size %u",
            			  0,0,
            			  enb_s1u_teid,
            			  sgw_s1u_teid,
            			  data_req_p->length);

            }

            rc = nwGtpv1uMsgDelete(RC.gtpv1u_data_g->gtpv1u_stack,
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
    }
    break;

    case TERMINATE_MESSAGE: {
      if (RC.gtpv1u_data_g->ue_mapping != NULL) {
        hashtable_destroy (RC.gtpv1u_data_g->ue_mapping);
      }

      if (RC.gtpv1u_data_g->teid_mapping != NULL) {
        hashtable_destroy (RC.gtpv1u_data_g->teid_mapping);
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
  }

  return NULL;
}

