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

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "asn1_utils.h"
#include "nr_pdcp_ue_manager.h"
#include "nr_pdcp_timer_thread.h"
#include "NR_RadioBearerConfig.h"
#include "NR_RLC-BearerConfig.h"
#include "NR_RLC-Config.h"
#include "NR_CellGroupConfig.h"
#include "openair2/RRC/NR/nr_rrc_proto.h"
#include <stdint.h>

/* from OAI */
#include "pdcp.h"
#include "LAYER2/nr_rlc/nr_rlc_oai_api.h"
#include <openair3/ocp-gtpu/gtp_itf.h>

#define TODO do { \
    printf("%s:%d:%s: todo\n", __FILE__, __LINE__, __FUNCTION__); \
    exit(1); \
  } while (0)

static nr_pdcp_ue_manager_t *nr_pdcp_ue_manager;

/* TODO: handle time a bit more properly */
static uint64_t nr_pdcp_current_time;
static int      nr_pdcp_current_time_last_frame;
static int      nr_pdcp_current_time_last_subframe;

/* necessary globals for OAI, not used internally */
hash_table_t  *pdcp_coll_p;
static uint64_t pdcp_optmask;

ngran_node_t node_type = ngran_gNB;
uint8_t proto_agent_flag = 0;

/****************************************************************************/
/* rlc_data_req queue - begin                                               */
/****************************************************************************/


#include <pthread.h>

/* NR PDCP and RLC both use "big locks". In some cases a thread may do
 * lock(rlc) followed by lock(pdcp) (typically when running 'rx_sdu').
 * Another thread may first do lock(pdcp) and then lock(rlc) (typically
 * the GTP module calls 'pdcp_data_req' that, in a previous implementation
 * was indirectly calling 'rlc_data_req' which does lock(rlc)).
 * To avoid the resulting deadlock it is enough to ensure that a call
 * to lock(pdcp) will never be followed by a call to lock(rlc). So,
 * here we chose to have a separate thread that deals with rlc_data_req,
 * out of the PDCP lock. Other solutions may be possible.
 * So instead of calling 'rlc_data_req' directly we have a queue and a
 * separate thread emptying it.
 */

typedef struct {
  protocol_ctxt_t ctxt_pP;
  srb_flag_t      srb_flagP;
  MBMS_flag_t     MBMS_flagP;
  rb_id_t         rb_idP;
  mui_t           muiP;
  confirm_t       confirmP;
  sdu_size_t      sdu_sizeP;
  mem_block_t     *sdu_pP;
} rlc_data_req_queue_item;

#define RLC_DATA_REQ_QUEUE_SIZE 10000

typedef struct {
  rlc_data_req_queue_item q[RLC_DATA_REQ_QUEUE_SIZE];
  volatile int start;
  volatile int length;
  pthread_mutex_t m;
  pthread_cond_t c;
} rlc_data_req_queue;

static rlc_data_req_queue q;

extern rlc_op_status_t nr_rrc_rlc_config_asn1_req (const protocol_ctxt_t   * const ctxt_pP,
    const NR_SRB_ToAddModList_t   * const srb2add_listP,
    const NR_DRB_ToAddModList_t   * const drb2add_listP,
    const NR_DRB_ToReleaseList_t  * const drb2release_listP,
    const LTE_PMCH_InfoList_r9_t * const pmch_InfoList_r9_pP,
    struct NR_CellGroupConfig__rlc_BearerToAddModList *rlc_bearer2add_list);

static void *rlc_data_req_thread(void *_)
{
  int i;

  pthread_setname_np(pthread_self(), "RLC queue");
  while (1) {
    if (pthread_mutex_lock(&q.m) != 0) abort();
    while (q.length == 0)
      if (pthread_cond_wait(&q.c, &q.m) != 0) abort();
    i = q.start;
    if (pthread_mutex_unlock(&q.m) != 0) abort();

    rlc_data_req(&q.q[i].ctxt_pP,
                 q.q[i].srb_flagP,
                 q.q[i].MBMS_flagP,
                 q.q[i].rb_idP,
                 q.q[i].muiP,
                 q.q[i].confirmP,
                 q.q[i].sdu_sizeP,
                 q.q[i].sdu_pP,
                 NULL,
                 NULL);

    if (pthread_mutex_lock(&q.m) != 0) abort();

    q.length--;
    q.start = (q.start + 1) % RLC_DATA_REQ_QUEUE_SIZE;

    if (pthread_cond_signal(&q.c) != 0) abort();
    if (pthread_mutex_unlock(&q.m) != 0) abort();
  }
}

static void init_nr_rlc_data_req_queue(void)
{
  pthread_t t;

  pthread_mutex_init(&q.m, NULL);
  pthread_cond_init(&q.c, NULL);

  if (pthread_create(&t, NULL, rlc_data_req_thread, NULL) != 0) {
    LOG_E(PDCP, "%s:%d:%s: fatal\n", __FILE__, __LINE__, __FUNCTION__);
    exit(1);
  }
}

static void enqueue_rlc_data_req(const protocol_ctxt_t *const ctxt_pP,
                                 const srb_flag_t   srb_flagP,
                                 const MBMS_flag_t  MBMS_flagP,
                                 const rb_id_t      rb_idP,
                                 const mui_t        muiP,
                                 confirm_t    confirmP,
                                 sdu_size_t   sdu_sizeP,
                                 mem_block_t *sdu_pP,
                                 void *_unused1, void *_unused2)
{
  int i;
  int logged = 0;

  if (pthread_mutex_lock(&q.m) != 0) abort();
  while (q.length == RLC_DATA_REQ_QUEUE_SIZE) {
    if (!logged) {
      logged = 1;
      LOG_W(PDCP, "%s: rlc_data_req queue is full\n", __FUNCTION__);
    }
    if (pthread_cond_wait(&q.c, &q.m) != 0) abort();
  }

  i = (q.start + q.length) % RLC_DATA_REQ_QUEUE_SIZE;
  q.length++;

  q.q[i].ctxt_pP    = *ctxt_pP;
  q.q[i].srb_flagP  = srb_flagP;
  q.q[i].MBMS_flagP = MBMS_flagP;
  q.q[i].rb_idP     = rb_idP;
  q.q[i].muiP       = muiP;
  q.q[i].confirmP   = confirmP;
  q.q[i].sdu_sizeP  = sdu_sizeP;
  q.q[i].sdu_pP     = sdu_pP;

  if (pthread_cond_signal(&q.c) != 0) abort();
  if (pthread_mutex_unlock(&q.m) != 0) abort();
}

void du_rlc_data_req(const protocol_ctxt_t *const ctxt_pP,
                     const srb_flag_t   srb_flagP,
                     const MBMS_flag_t  MBMS_flagP,
                     const rb_id_t      rb_idP,
                     const mui_t        muiP,
                     confirm_t    confirmP,
                     sdu_size_t   sdu_sizeP,
                     mem_block_t *sdu_pP)
{
  enqueue_rlc_data_req(ctxt_pP,
                       srb_flagP,
                       MBMS_flagP,
                       rb_idP, muiP,
                       confirmP,
                       sdu_sizeP,
                       sdu_pP,
                       NULL,
                       NULL);
}

/****************************************************************************/
/* rlc_data_req queue - end                                                 */
/****************************************************************************/

/****************************************************************************/
/* hacks to be cleaned up at some point - begin                             */
/****************************************************************************/

#include "LAYER2/MAC/mac_extern.h"

static void reblock_tun_socket(void)
{
  extern int nas_sock_fd[];
  int f;

  f = fcntl(nas_sock_fd[0], F_GETFL, 0);
  f &= ~(O_NONBLOCK);
  if (fcntl(nas_sock_fd[0], F_SETFL, f) == -1) {
    LOG_E(PDCP, "reblock_tun_socket failed\n");
    exit(1);
  }
}

static void *enb_tun_read_thread(void *_)
{
  extern int nas_sock_fd[];
  char rx_buf[NL_MAX_PAYLOAD];
  int len;
  int rnti;
  protocol_ctxt_t ctxt;

  int rb_id = 1;
  pthread_setname_np( pthread_self(),"enb_tun_read");

  while (1) {
    len = read(nas_sock_fd[0], &rx_buf, NL_MAX_PAYLOAD);
    if (len == -1) {
      LOG_E(PDCP, "%s:%d:%s: fatal\n", __FILE__, __LINE__, __FUNCTION__);
      exit(1);
    }

    LOG_D(PDCP, "%s(): nas_sock_fd read returns len %d\n", __func__, len);

    nr_pdcp_manager_lock(nr_pdcp_ue_manager);
    rnti = nr_pdcp_get_first_rnti(nr_pdcp_ue_manager);
    nr_pdcp_manager_unlock(nr_pdcp_ue_manager);

    if (rnti == -1) continue;

    ctxt.module_id = 0;
    ctxt.enb_flag = 1;
    ctxt.instance = 0;
    ctxt.frame = 0;
    ctxt.subframe = 0;
    ctxt.eNB_index = 0;
    ctxt.configured = 1;
    ctxt.brOption = 0;

    ctxt.rnti = rnti;

    pdcp_data_req(&ctxt, SRB_FLAG_NO, rb_id, RLC_MUI_UNDEFINED,
                  RLC_SDU_CONFIRM_NO, len, (unsigned char *)rx_buf,
                  PDCP_TRANSMISSION_MODE_DATA, NULL, NULL);
  }

  return NULL;
}

static void *ue_tun_read_thread(void *_)
{
  extern int nas_sock_fd[];
  char rx_buf[NL_MAX_PAYLOAD];
  int len;
  int rnti;
  protocol_ctxt_t ctxt;

  int rb_id = 1;
  pthread_setname_np( pthread_self(),"ue_tun_read"); 
  while (1) {
    len = read(nas_sock_fd[0], &rx_buf, NL_MAX_PAYLOAD);
    if (len == -1) {
      LOG_E(PDCP, "%s:%d:%s: fatal\n", __FILE__, __LINE__, __FUNCTION__);
      exit(1);
    }

    LOG_D(PDCP, "%s(): nas_sock_fd read returns len %d\n", __func__, len);
    char buffer[1024];
    hexdump(rx_buf, len, buffer, sizeof(buffer));
    LOG_I(MAC, "Melissa Elkadi, this is hexdump of pdu %s from tunnel interface\n", buffer);

    nr_pdcp_manager_lock(nr_pdcp_ue_manager);
    rnti = nr_pdcp_get_first_rnti(nr_pdcp_ue_manager);
    nr_pdcp_manager_unlock(nr_pdcp_ue_manager);

    if (rnti == -1) continue;

    ctxt.module_id = 0;
    ctxt.enb_flag = 0;
    ctxt.instance = 0;
    ctxt.frame = 0;
    ctxt.subframe = 0;
    ctxt.eNB_index = 0;
    ctxt.configured = 1;
    ctxt.brOption = 0;

    ctxt.rnti = rnti;

    pdcp_data_req(&ctxt, SRB_FLAG_NO, rb_id, RLC_MUI_UNDEFINED,
                  RLC_SDU_CONFIRM_NO, len, (unsigned char *)rx_buf,
                  PDCP_TRANSMISSION_MODE_DATA, NULL, NULL);
  }

  return NULL;
}

static void start_pdcp_tun_enb(void)
{
  pthread_t t;

  reblock_tun_socket();

  if (pthread_create(&t, NULL, enb_tun_read_thread, NULL) != 0) {
    LOG_E(PDCP, "%s:%d:%s: fatal\n", __FILE__, __LINE__, __FUNCTION__);
    exit(1);
  }
}

static void start_pdcp_tun_ue(void)
{
  pthread_t t;

  reblock_tun_socket();

  if (pthread_create(&t, NULL, ue_tun_read_thread, NULL) != 0) {
    LOG_E(PDCP, "%s:%d:%s: fatal\n", __FILE__, __LINE__, __FUNCTION__);
    exit(1);
  }
}

/****************************************************************************/
/* hacks to be cleaned up at some point - end                               */
/****************************************************************************/

int pdcp_fifo_flush_sdus(const protocol_ctxt_t *const ctxt_pP)
{
  return 0;
}

void pdcp_layer_init(void)
{
  /* hack: be sure to initialize only once */
  static pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
  static int initialized = 0;
  if (pthread_mutex_lock(&m) != 0) abort();
  if (initialized) {
    if (pthread_mutex_unlock(&m) != 0) abort();
    return;
  }
  initialized = 1;
  if (pthread_mutex_unlock(&m) != 0) abort();

  nr_pdcp_ue_manager = new_nr_pdcp_ue_manager(1);

  if ((RC.nrrrc == NULL) || (!NODE_IS_CU(RC.nrrrc[0]->node_type))) {
    init_nr_rlc_data_req_queue();
  }

  nr_pdcp_init_timer_thread(nr_pdcp_ue_manager);
}

#include "nfapi/oai_integration/vendor_ext.h"
#include "targets/RT/USER/lte-softmodem.h"
#include "openair2/RRC/NAS/nas_config.h"

uint64_t nr_pdcp_module_init(uint64_t _pdcp_optmask, int id)
{
  /* hack: be sure to initialize only once */
  static pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
  static int initialized = 0;
  if (pthread_mutex_lock(&m) != 0) abort();
  if (initialized) {
    abort();
  }
  initialized = 1;
  if (pthread_mutex_unlock(&m) != 0) abort();

#if 0
  pdcp_optmask = _pdcp_optmask;
  return pdcp_optmask;
#endif
  /* temporary enforce netlink when UE_NAS_USE_TUN is set,
     this is while switching from noS1 as build option
     to noS1 as config option                               */
  if ( _pdcp_optmask & UE_NAS_USE_TUN_BIT) {
    pdcp_optmask = pdcp_optmask | PDCP_USE_NETLINK_BIT ;
  }

  pdcp_optmask = pdcp_optmask | _pdcp_optmask ;
  LOG_I(PDCP, "pdcp init,%s %s\n",
        ((LINK_ENB_PDCP_TO_GTPV1U)?"usegtp":""),
        ((PDCP_USE_NETLINK)?"usenetlink":""));

  if (PDCP_USE_NETLINK) {
    nas_getparams();

    if(UE_NAS_USE_TUN) {
      /* Melissa TODO: Brute force changes made below to allow nr-UE to have unique tunnel interfaces.
         When the NODE_NUMBER param is not used to determine functionality and LTE tunnel
         interfaces, we should update the netlink_init_tun() and nas_config() calls below as well. */
      int num_if = (NFAPI_MODE == NFAPI_UE_STUB_PNF || IS_SOFTMODEM_SIML1 || NFAPI_MODE == NFAPI_MODE_STANDALONE_PNF)? MAX_MOBILES_PER_ENB : 1;
      netlink_init_tun("nrue", num_if, id);
      //Add --nr-ip-over-lte option check for next line
      if (IS_SOFTMODEM_NOS1)
          nas_config(1, 1, 3, "nrue");
      LOG_I(PDCP, "UE pdcp will use tun interface\n");
      start_pdcp_tun_ue();
    } else if(ENB_NAS_USE_TUN) {
      netlink_init_tun("gnb", 1, id);
      nas_config(1, 1, 1, "gnb");
      LOG_I(PDCP, "ENB pdcp will use tun interface\n");
      start_pdcp_tun_enb();
    } else {
      LOG_I(PDCP, "pdcp will use kernel modules\n");
      abort();
      netlink_init();
    }
  }
  return pdcp_optmask ;
}

static void deliver_sdu_drb(void *_ue, nr_pdcp_entity_t *entity,
                            char *buf, int size)
{
  extern int nas_sock_fd[];
  int len;
  nr_pdcp_ue_t *ue = _ue;
  MessageDef  *message_p;
  uint8_t     *gtpu_buffer_p;
  int rb_id;
  int i;

  if (IS_SOFTMODEM_NOS1 || UE_NAS_USE_TUN) {
    LOG_D(PDCP, "IP packet received, to be sent to TUN interface");

    /* In 5G there is a new layer called SDAP which adds one byte
       to the PDU data. TUN interfaces care about the actual data
       we are sending. If we do not remove the instance ID from
       the buffer, it will start with 00, and the write will fail
       with errno = 22 (EINVAL) */
    char *data_buffer = buf + 1;
    int size_to_write = size - 1;

    len = write(nas_sock_fd[0], data_buffer, size_to_write);
    LOG_D(PDCP, "len = %d bytes to tunnel interface %d\n", len, nas_sock_fd[0]);
    if (len != size_to_write) {
      LOG_E(PDCP, "%s:%d:%s: fatal error %d: %s\n", __FILE__, __LINE__, __FUNCTION__, errno, strerror(errno));
    }
  }
  else{
    for (i = 0; i < 5; i++) {
        if (entity == ue->drb[i]) {
          rb_id = i+1;
          goto rb_found;
        }
      }

      LOG_E(PDCP, "%s:%d:%s: fatal, no RB found for ue %d\n",
            __FILE__, __LINE__, __FUNCTION__, ue->rnti);
      exit(1);

    rb_found:
    {
      int offset=0;
      if (entity->has_sdap == 1 && entity->has_sdapULheader == 1) offset = 1; // this is the offset of the SDAP header in bytes

      gtpu_buffer_p = itti_malloc(TASK_PDCP_ENB, TASK_GTPV1_U,
                                  size + GTPU_HEADER_OVERHEAD_MAX - offset);
      AssertFatal(gtpu_buffer_p != NULL, "OUT OF MEMORY");
      memcpy(&gtpu_buffer_p[GTPU_HEADER_OVERHEAD_MAX], buf+offset, size-offset);
      message_p = itti_alloc_new_message(TASK_PDCP_ENB, 0, GTPV1U_GNB_TUNNEL_DATA_REQ);
      AssertFatal(message_p != NULL, "OUT OF MEMORY");
      GTPV1U_GNB_TUNNEL_DATA_REQ(message_p).buffer              = gtpu_buffer_p;
      GTPV1U_GNB_TUNNEL_DATA_REQ(message_p).length              = size-offset;
      GTPV1U_GNB_TUNNEL_DATA_REQ(message_p).offset              = GTPU_HEADER_OVERHEAD_MAX;
      GTPV1U_GNB_TUNNEL_DATA_REQ(message_p).rnti                = ue->rnti;
      GTPV1U_GNB_TUNNEL_DATA_REQ(message_p).pdusession_id       = entity->pdusession_id;
      if (offset==1) LOG_I(PDCP, "%s() (drb %d) SDAP header %2x\n",__func__, rb_id, buf[0]);
      LOG_D(PDCP, "%s() (drb %d) sending message to gtp size %d\n", __func__, rb_id, size-offset);
      itti_send_msg_to_task(TASK_VARIABLE, INSTANCE_DEFAULT, message_p);
   }
  }
}

static void deliver_pdu_drb(void *_ue, nr_pdcp_entity_t *entity,
                            char *buf, int size, int sdu_id)
{
  nr_pdcp_ue_t *ue = _ue;
  int rb_id;
  protocol_ctxt_t ctxt;
  int i;
  mem_block_t *memblock;

  for (i = 0; i < 5; i++) {
    if (entity == ue->drb[i]) {
      rb_id = i+1;
      goto rb_found;
    }
  }

  LOG_E(PDCP, "%s:%d:%s: fatal, no RB found for ue %d\n",
        __FILE__, __LINE__, __FUNCTION__, ue->rnti);
  exit(1);

rb_found:
  ctxt.module_id = 0;
  ctxt.enb_flag = 1;
  ctxt.instance = 0;
  ctxt.frame = 0;
  ctxt.subframe = 0;
  ctxt.eNB_index = 0;
  ctxt.configured = 1;
  ctxt.brOption = 0;

  ctxt.rnti = ue->rnti;

  memblock = get_free_mem_block(size, __FUNCTION__);
  memcpy(memblock->data, buf, size);

  LOG_D(PDCP, "%s(): (srb %d) calling rlc_data_req size %d\n", __func__, rb_id, size);
  //for (i = 0; i < size; i++) printf(" %2.2x", (unsigned char)memblock->data[i]);
  //printf("\n");
  enqueue_rlc_data_req(&ctxt, 0, MBMS_FLAG_NO, rb_id, sdu_id, 0, size, memblock, NULL, NULL);
}

static void deliver_sdu_srb(void *_ue, nr_pdcp_entity_t *entity,
                            char *buf, int size)
{
  nr_pdcp_ue_t *ue = _ue;
  int srb_id;
  int i;

  for (i = 0; i < sizeofArray(ue->srb) ; i++) {
    if (entity == ue->srb[i]) {
      srb_id = i+1;
      goto srb_found;
    }
  }

  LOG_E(PDCP, "%s:%d:%s: fatal, no SRB found for ue %d\n",
	__FILE__, __LINE__, __FUNCTION__, ue->rnti);
  exit(1);

 srb_found:
  {
       uint8_t *rrc_buffer_p = entity->is_gnb ?
					itti_malloc(TASK_PDCP_ENB, TASK_RRC_GNB, size):
                                        itti_malloc(TASK_PDCP_UE, TASK_RRC_NRUE, size);
       MessageDef  *message_p;

       AssertFatal(rrc_buffer_p != NULL, "OUT OF MEMORY");
       memcpy(rrc_buffer_p, buf, size);
       message_p = entity->is_gnb ?
                            itti_alloc_new_message(TASK_PDCP_ENB, 0, NR_RRC_DCCH_DATA_IND):
                            itti_alloc_new_message(TASK_PDCP_UE, 0, NR_RRC_DCCH_DATA_IND);

       AssertFatal(message_p != NULL, "OUT OF MEMORY");
       NR_RRC_DCCH_DATA_IND(message_p).dcch_index = srb_id;
       NR_RRC_DCCH_DATA_IND(message_p).sdu_p = rrc_buffer_p;
       NR_RRC_DCCH_DATA_IND(message_p).sdu_size = size;
       NR_RRC_DCCH_DATA_IND(message_p).rnti = ue->rnti;

       itti_send_msg_to_task(entity->is_gnb ? TASK_RRC_GNB : TASK_RRC_NRUE, 0, message_p);
    }
}

static void deliver_pdu_srb(void *_ue, nr_pdcp_entity_t *entity,
                            char *buf, int size, int sdu_id)
{
  nr_pdcp_ue_t *ue = _ue;
  int srb_id;
  protocol_ctxt_t ctxt;
  int i;
  mem_block_t *memblock;

  for (i = 0; i < sizeofArray(ue->srb) ; i++) {
    if (entity == ue->srb[i]) {
      srb_id = i+1;
      goto srb_found;
    }
  }

  LOG_E(PDCP, "%s:%d:%s: fatal, no SRB found for ue %d\n",
        __FILE__, __LINE__, __FUNCTION__, ue->rnti);
  exit(1);

srb_found:


  LOG_D(PDCP, "%s(): (srb %d) calling rlc_data_req size %d\n", __func__, srb_id, size);
  //for (i = 0; i < size; i++) printf(" %2.2x", (unsigned char)memblock->data[i]);
  //printf("\n");
  if ((RC.nrrrc == NULL) || (!NODE_IS_CU(RC.nrrrc[0]->node_type))) {
    ctxt.module_id = 0;
    ctxt.enb_flag = 1;
    ctxt.instance = 0;
    ctxt.frame = 0;
    ctxt.subframe = 0;
    ctxt.eNB_index = 0;
    ctxt.configured = 1;
    ctxt.brOption = 0;

    ctxt.rnti = ue->rnti;

    memblock = get_free_mem_block(size, __FUNCTION__);
    memcpy(memblock->data, buf, size);
    enqueue_rlc_data_req(&ctxt, 1, MBMS_FLAG_NO, srb_id, sdu_id, 0, size, memblock, NULL, NULL);
  }
  else {
    MessageDef  *message_p = itti_alloc_new_message (TASK_RRC_GNB, 0, F1AP_DL_RRC_MESSAGE);
    F1AP_DL_RRC_MESSAGE (message_p).rrc_container        = (uint8_t*)buf;
    F1AP_DL_RRC_MESSAGE (message_p).rrc_container_length = size;
    F1AP_DL_RRC_MESSAGE (message_p).gNB_CU_ue_id         = 0;
    F1AP_DL_RRC_MESSAGE (message_p).gNB_DU_ue_id         = 0;
    F1AP_DL_RRC_MESSAGE (message_p).old_gNB_DU_ue_id     = 0xFFFFFFFF; // unknown
    F1AP_DL_RRC_MESSAGE (message_p).rnti                 = ue->rnti;
    F1AP_DL_RRC_MESSAGE (message_p).srb_id               = srb_id;
    F1AP_DL_RRC_MESSAGE (message_p).execute_duplication  = 1;
    F1AP_DL_RRC_MESSAGE (message_p).RAT_frequency_priority_information.en_dc = 0;
    itti_send_msg_to_task (TASK_CU_F1, 0, message_p);
    LOG_D(PDCP, "Send F1AP_DL_RRC_MESSAGE with ITTI\n");
  }
}

boolean_t pdcp_data_ind(
  const protocol_ctxt_t *const  ctxt_pP,
  const srb_flag_t srb_flagP,
  const MBMS_flag_t MBMS_flagP,
  const rb_id_t rb_id,
  const sdu_size_t sdu_buffer_size,
  mem_block_t *const sdu_buffer)
{
  nr_pdcp_ue_t *ue;
  nr_pdcp_entity_t *rb;
  int rnti = ctxt_pP->rnti;

  if (ctxt_pP->module_id != 0 ||
      //ctxt_pP->enb_flag != 1 ||
      ctxt_pP->instance != 0 ||
      ctxt_pP->eNB_index != 0 ||
      ctxt_pP->configured != 1 ||
      ctxt_pP->brOption != 0) {
    LOG_E(PDCP, "%s:%d:%s: fatal\n", __FILE__, __LINE__, __FUNCTION__);
    exit(1);
  }

  if (ctxt_pP->enb_flag)
    T(T_ENB_PDCP_UL, T_INT(ctxt_pP->module_id), T_INT(rnti),
      T_INT(rb_id), T_INT(sdu_buffer_size));

  nr_pdcp_manager_lock(nr_pdcp_ue_manager);
  ue = nr_pdcp_manager_get_ue(nr_pdcp_ue_manager, rnti);

  if (srb_flagP == 1) {
    if (rb_id < 1 || rb_id > 2)
      rb = NULL;
    else
      rb = ue->srb[rb_id - 1];
  } else {
    if (rb_id < 1 || rb_id > 5)
      rb = NULL;
    else
      rb = ue->drb[rb_id - 1];
  }

  if (rb != NULL) {
    rb->recv_pdu(rb, (char *)sdu_buffer->data, sdu_buffer_size);
  } else {
    LOG_E(PDCP, "%s:%d:%s: fatal: no RB found (rb_id %ld, srb_flag %d)\n",
          __FILE__, __LINE__, __FUNCTION__, rb_id, srb_flagP);
    exit(1);
  }

  nr_pdcp_manager_unlock(nr_pdcp_ue_manager);

  free_mem_block(sdu_buffer, __FUNCTION__);

  return 1;
}

void pdcp_run(const protocol_ctxt_t *const  ctxt_pP)
{
  MessageDef      *msg_p;
  int             result;
  protocol_ctxt_t ctxt={.module_id=0,
                        .enb_flag=1,
                        .instance=0,
                        .rnti=0,
                        .frame=-1,
                        .subframe=-1,
                        .eNB_index=0,
                        .configured=true,
                        .brOption=false
                       };


  while (1) {
    itti_poll_msg(ctxt_pP->enb_flag ? TASK_PDCP_ENB : TASK_PDCP_UE, &msg_p);
    if (msg_p == NULL){
     break;
    }
    switch (ITTI_MSG_ID(msg_p)) {
    case RRC_DCCH_DATA_REQ:
      LOG_D(PDCP, "Received RRC_DCCH_DATA_REQ type at PDCP task \n");
      PROTOCOL_CTXT_SET_BY_MODULE_ID(
          &ctxt,
          RRC_DCCH_DATA_REQ(msg_p).module_id,
          RRC_DCCH_DATA_REQ(msg_p).enb_flag,
          RRC_DCCH_DATA_REQ(msg_p).rnti,
          RRC_DCCH_DATA_REQ(msg_p).frame,
          0,
          RRC_DCCH_DATA_REQ(msg_p).eNB_index);
      result = pdcp_data_req(&ctxt,
                             SRB_FLAG_YES,
                             RRC_DCCH_DATA_REQ(msg_p).rb_id,
                             RRC_DCCH_DATA_REQ(msg_p).muip,
                             RRC_DCCH_DATA_REQ(msg_p).confirmp,
                             RRC_DCCH_DATA_REQ(msg_p).sdu_size,
                             RRC_DCCH_DATA_REQ(msg_p).sdu_p,
                             RRC_DCCH_DATA_REQ(msg_p).mode,
                             NULL, NULL);

      if (result != TRUE)
        LOG_E(PDCP, "PDCP data request failed!\n");
      result = itti_free(ITTI_MSG_ORIGIN_ID(msg_p), RRC_DCCH_DATA_REQ(msg_p).sdu_p);
      AssertFatal(result == EXIT_SUCCESS, "Failed to free memory (%d)!\n", result);
      break;
    default:
      LOG_E(PDCP, "Received unexpected message %s\n", ITTI_MSG_NAME(msg_p));
      break;
    }
  }
}

static void add_srb(int is_gnb, int rnti, struct NR_SRB_ToAddMod *s)
{
  nr_pdcp_entity_t *pdcp_srb;
  nr_pdcp_ue_t *ue;
  int t_Reordering=3000;

  int srb_id = s->srb_Identity;
  if (s->pdcp_Config == NULL ||
      s->pdcp_Config->t_Reordering == NULL) t_Reordering = 3000;
  else t_Reordering = decode_t_reordering(*s->pdcp_Config->t_Reordering);

  nr_pdcp_manager_lock(nr_pdcp_ue_manager);
  ue = nr_pdcp_manager_get_ue(nr_pdcp_ue_manager, rnti);
  if (ue->srb[srb_id-1] != NULL) {
    LOG_D(PDCP, "%s:%d:%s: warning SRB %d already exist for ue %d, do nothing\n",
          __FILE__, __LINE__, __FUNCTION__, srb_id, rnti);
  } else {
    pdcp_srb = new_nr_pdcp_entity(NR_PDCP_SRB, is_gnb, srb_id,
                                  0, 0, 0, 0, // sdap parameters
                                  deliver_sdu_srb, ue, deliver_pdu_srb, ue,
                                  12, t_Reordering, -1,
                                  0, 0,
                                  NULL, NULL);
    nr_pdcp_ue_add_srb_pdcp_entity(ue, srb_id, pdcp_srb);

    LOG_D(PDCP, "%s:%d:%s: added srb %d to ue rnti %x\n", __FILE__, __LINE__, __FUNCTION__, srb_id, rnti);
  }
  nr_pdcp_manager_unlock(nr_pdcp_ue_manager);
}

static void add_drb_am(int is_gnb, int rnti, struct NR_DRB_ToAddMod *s,
                       int ciphering_algorithm,
                       int integrity_algorithm,
                       unsigned char *ciphering_key,
                       unsigned char *integrity_key)
{
  nr_pdcp_entity_t *pdcp_drb;
  nr_pdcp_ue_t *ue;

  int drb_id = s->drb_Identity;
  int sn_size_ul = decode_sn_size_ul(*s->pdcp_Config->drb->pdcp_SN_SizeUL);
  int sn_size_dl = decode_sn_size_dl(*s->pdcp_Config->drb->pdcp_SN_SizeDL);
  int discard_timer = decode_discard_timer(*s->pdcp_Config->drb->discardTimer);

  /* if pdcp_Config->t_Reordering is not present, it means infinity (-1) */
  int t_reordering = -1;
  if (s->pdcp_Config->t_Reordering != NULL) {
    t_reordering = decode_t_reordering(*s->pdcp_Config->t_Reordering);
  }

  if ((!s->cnAssociation) || s->cnAssociation->present == NR_DRB_ToAddMod__cnAssociation_PR_NOTHING) {
    LOG_E(PDCP,"%s:%d:%s: fatal, cnAssociation is missing or present is NR_DRB_ToAddMod__cnAssociation_PR_NOTHING\n",__FILE__,__LINE__,__FUNCTION__);
    exit(-1);
  }

  int pdusession_id;
  int has_sdap = 0;
  int has_sdapULheader=0;
  int has_sdapDLheader=0;
  if (s->cnAssociation->present == NR_DRB_ToAddMod__cnAssociation_PR_eps_BearerIdentity)
     pdusession_id = s->cnAssociation->choice.eps_BearerIdentity;
  else {
    if (!s->cnAssociation->choice.sdap_Config) {
      LOG_E(PDCP,"%s:%d:%s: fatal, sdap_Config is null",__FILE__,__LINE__,__FUNCTION__);
      exit(-1);
    }
    pdusession_id = s->cnAssociation->choice.sdap_Config->pdu_Session;
    has_sdap = 1;
    has_sdapULheader = s->cnAssociation->choice.sdap_Config->sdap_HeaderUL == NR_SDAP_Config__sdap_HeaderUL_present ? 1 : 0;
    has_sdapDLheader = s->cnAssociation->choice.sdap_Config->sdap_HeaderDL == NR_SDAP_Config__sdap_HeaderDL_present ? 1 : 0;
    if (has_sdapDLheader==1) {
      LOG_E(PDCP,"%s:%d:%s: fatal, no support for SDAP DL yet\n",__FILE__,__LINE__,__FUNCTION__);
      exit(-1);
    }
  }
  /* TODO(?): accept different UL and DL SN sizes? */
  if (sn_size_ul != sn_size_dl) {
    LOG_E(PDCP, "%s:%d:%s: fatal, bad SN sizes, must be same. ul=%d, dl=%d\n",
          __FILE__, __LINE__, __FUNCTION__, sn_size_ul, sn_size_dl);
    exit(1);
  }

  if (drb_id != 1) {
    LOG_E(PDCP, "%s:%d:%s: fatal, bad drb id %d\n",
          __FILE__, __LINE__, __FUNCTION__, drb_id);
    exit(1);
  }

  nr_pdcp_manager_lock(nr_pdcp_ue_manager);
  ue = nr_pdcp_manager_get_ue(nr_pdcp_ue_manager, rnti);
  if (ue->drb[drb_id-1] != NULL) {
    LOG_I(PDCP, "Melissa Elkadi %s:%d:%s: warning DRB %d already exist for ue %d, do nothing\n",
          __FILE__, __LINE__, __FUNCTION__, drb_id, rnti);
  } else {
    LOG_I(PDCP, "%s:%d:%s: Melissa Elkadi, we will call deliver_sdu_drb\n", __FILE__, __LINE__, __FUNCTION__);
    pdcp_drb = new_nr_pdcp_entity(NR_PDCP_DRB_AM, is_gnb, drb_id,pdusession_id,has_sdap,
                                  has_sdapULheader,has_sdapDLheader,
                                  deliver_sdu_drb, ue, deliver_pdu_drb, ue,
                                  sn_size_dl, t_reordering, discard_timer,
                                  ciphering_algorithm, integrity_algorithm,
                                  ciphering_key, integrity_key);
    nr_pdcp_ue_add_drb_pdcp_entity(ue, drb_id, pdcp_drb);

    LOG_D(PDCP, "%s:%d:%s: added drb %d to ue rnti %x\n", __FILE__, __LINE__, __FUNCTION__, drb_id, rnti);
  }
  nr_pdcp_manager_unlock(nr_pdcp_ue_manager);
}

static void add_drb(int is_gnb, int rnti, struct NR_DRB_ToAddMod *s,
                    NR_RLC_Config_t *rlc_Config,
                    int ciphering_algorithm,
                    int integrity_algorithm,
                    unsigned char *ciphering_key,
                    unsigned char *integrity_key)
{
  switch (rlc_Config->present) {
  case NR_RLC_Config_PR_am:
    add_drb_am(is_gnb, rnti, s, ciphering_algorithm, integrity_algorithm,
               ciphering_key, integrity_key);
    break;
  case NR_RLC_Config_PR_um_Bi_Directional:
    //add_drb_um(rnti, s);
    /* hack */
    add_drb_am(is_gnb, rnti, s, ciphering_algorithm, integrity_algorithm,
               ciphering_key, integrity_key);
    break;
  default:
    LOG_E(PDCP, "%s:%d:%s: fatal: unhandled DRB type\n",
          __FILE__, __LINE__, __FUNCTION__);
    exit(1);
  }
  LOG_I(PDCP, "%s:%s:%d: added DRB for UE RNTI %x\n", __FILE__, __FUNCTION__, __LINE__, rnti);
}

boolean_t nr_rrc_pdcp_config_asn1_req(
  const protocol_ctxt_t *const  ctxt_pP,
  NR_SRB_ToAddModList_t  *const srb2add_list,
  NR_DRB_ToAddModList_t  *const drb2add_list,
  NR_DRB_ToReleaseList_t *const drb2release_list,
  const uint8_t                   security_modeP,
  uint8_t                  *const kRRCenc,
  uint8_t                  *const kRRCint,
  uint8_t                  *const kUPenc,
  uint8_t                  *const kUPint
#if (LTE_RRC_VERSION >= MAKE_VERSION(9, 0, 0))
  ,LTE_PMCH_InfoList_r9_t  *pmch_InfoList_r9
#endif
  ,rb_id_t                 *const defaultDRB,
  struct NR_CellGroupConfig__rlc_BearerToAddModList *rlc_bearer2add_list)
  //struct NR_RLC_Config     *rlc_Config)
{
  int rnti = ctxt_pP->rnti;
  int i;

  if (//ctxt_pP->enb_flag != 1 ||
      ctxt_pP->module_id != 0 ||
      ctxt_pP->instance != 0 ||
      ctxt_pP->eNB_index != 0 ||
      //ctxt_pP->configured != 2 ||
      //srb2add_list == NULL ||
      //drb2add_list != NULL ||
      //drb2release_list != NULL ||
      //security_modeP != 255 ||
      //kRRCenc != NULL ||
      //kRRCint != NULL ||
      //kUPenc != NULL ||
      pmch_InfoList_r9 != NULL /*||
      defaultDRB != NULL */) {
    LOG_I(PDCP,"Releasing DRBs, oops\n");
    TODO;
  }

  if (srb2add_list != NULL) {
    for (i = 0; i < srb2add_list->list.count; i++) {
      add_srb(ctxt_pP->enb_flag,rnti, srb2add_list->list.array[i]);
    }
  }

  if (drb2add_list != NULL) {
    for (i = 0; i < drb2add_list->list.count; i++) {
      add_drb(ctxt_pP->enb_flag, rnti, drb2add_list->list.array[i],
              rlc_bearer2add_list->list.array[i]->rlc_Config,
              security_modeP & 0x0f, (security_modeP >> 4) & 0x0f,
              kUPenc, kUPint);
    }
  }

  /* update security */
  if (kRRCint != NULL) {
    /* todo */
  }

  if (drb2release_list != NULL) {
    // TODO
  }

  free(kRRCenc);
  free(kRRCint);
  free(kUPenc);
  free(kUPint);

  return 0;
}

/* Dummy function due to dependency from LTE libraries */
boolean_t rrc_pdcp_config_asn1_req(
  const protocol_ctxt_t *const  ctxt_pP,
  LTE_SRB_ToAddModList_t  *const srb2add_list,
  LTE_DRB_ToAddModList_t  *const drb2add_list,
  LTE_DRB_ToReleaseList_t *const drb2release_list,
  const uint8_t                   security_modeP,
  uint8_t                  *const kRRCenc,
  uint8_t                  *const kRRCint,
  uint8_t                  *const kUPenc
#if (LTE_RRC_VERSION >= MAKE_VERSION(9, 0, 0))
  ,LTE_PMCH_InfoList_r9_t  *pmch_InfoList_r9
#endif
  ,rb_id_t                 *const defaultDRB)
{
  return 0;
}

void nr_DRB_preconfiguration(uint16_t crnti)
{

  NR_RadioBearerConfig_t             *rbconfig = NULL;
  struct NR_CellGroupConfig__rlc_BearerToAddModList *Rlc_Bearer_ToAdd_list = NULL;
  protocol_ctxt_t ctxt;
  //fill_default_rbconfig(rb_config, 5, 1);
  rbconfig = calloc(1, sizeof(*rbconfig));

  rbconfig->srb_ToAddModList = NULL;
  rbconfig->srb3_ToRelease = NULL;
  rbconfig->drb_ToAddModList = calloc(1,sizeof(*rbconfig->drb_ToAddModList));
  NR_DRB_ToAddMod_t *drb_ToAddMod = calloc(1,sizeof(*drb_ToAddMod));
  drb_ToAddMod->cnAssociation = calloc(1,sizeof(*drb_ToAddMod->cnAssociation));
  drb_ToAddMod->cnAssociation->present = NR_DRB_ToAddMod__cnAssociation_PR_eps_BearerIdentity;
  drb_ToAddMod->cnAssociation->choice.eps_BearerIdentity= 5;
  drb_ToAddMod->drb_Identity = 1;
  drb_ToAddMod->reestablishPDCP = NULL;
  drb_ToAddMod->recoverPDCP = NULL;
  drb_ToAddMod->pdcp_Config = calloc(1,sizeof(*drb_ToAddMod->pdcp_Config));
  drb_ToAddMod->pdcp_Config->drb = calloc(1,sizeof(*drb_ToAddMod->pdcp_Config->drb));
  drb_ToAddMod->pdcp_Config->drb->discardTimer = calloc(1,sizeof(*drb_ToAddMod->pdcp_Config->drb->discardTimer));
  *drb_ToAddMod->pdcp_Config->drb->discardTimer=NR_PDCP_Config__drb__discardTimer_infinity;
  drb_ToAddMod->pdcp_Config->drb->pdcp_SN_SizeUL = calloc(1,sizeof(*drb_ToAddMod->pdcp_Config->drb->pdcp_SN_SizeUL));
  *drb_ToAddMod->pdcp_Config->drb->pdcp_SN_SizeUL = NR_PDCP_Config__drb__pdcp_SN_SizeUL_len12bits;
  drb_ToAddMod->pdcp_Config->drb->pdcp_SN_SizeDL = calloc(1,sizeof(*drb_ToAddMod->pdcp_Config->drb->pdcp_SN_SizeDL));
  *drb_ToAddMod->pdcp_Config->drb->pdcp_SN_SizeDL = NR_PDCP_Config__drb__pdcp_SN_SizeDL_len12bits;
  drb_ToAddMod->pdcp_Config->drb->headerCompression.present = NR_PDCP_Config__drb__headerCompression_PR_notUsed;
  drb_ToAddMod->pdcp_Config->drb->headerCompression.choice.notUsed = 0;

  drb_ToAddMod->pdcp_Config->drb->integrityProtection=NULL;
  drb_ToAddMod->pdcp_Config->drb->statusReportRequired=NULL;
  drb_ToAddMod->pdcp_Config->drb->outOfOrderDelivery=NULL;
  drb_ToAddMod->pdcp_Config->moreThanOneRLC = NULL;

  drb_ToAddMod->pdcp_Config->t_Reordering = calloc(1,sizeof(*drb_ToAddMod->pdcp_Config->t_Reordering));
  *drb_ToAddMod->pdcp_Config->t_Reordering = NR_PDCP_Config__t_Reordering_ms0;
  drb_ToAddMod->pdcp_Config->ext1 = NULL;

  ASN_SEQUENCE_ADD(&rbconfig->drb_ToAddModList->list,drb_ToAddMod);

  rbconfig->drb_ToReleaseList = NULL;

  rbconfig->securityConfig = calloc(1,sizeof(*rbconfig->securityConfig));
  rbconfig->securityConfig->securityAlgorithmConfig = calloc(1,sizeof(*rbconfig->securityConfig->securityAlgorithmConfig));
  rbconfig->securityConfig->securityAlgorithmConfig->cipheringAlgorithm = NR_CipheringAlgorithm_nea0;
  rbconfig->securityConfig->securityAlgorithmConfig->integrityProtAlgorithm=NULL;
  rbconfig->securityConfig->keyToUse = calloc(1,sizeof(*rbconfig->securityConfig->keyToUse));
  *rbconfig->securityConfig->keyToUse = NR_SecurityConfig__keyToUse_master;

  if ( LOG_DEBUGFLAG(DEBUG_ASN1) ) {
    xer_fprint(stdout, &asn_DEF_NR_RadioBearerConfig, (const void*)rbconfig);
  }

  NR_RLC_BearerConfig_t *RLC_BearerConfig = calloc(1,sizeof(*RLC_BearerConfig));
  nr_rlc_bearer_init(RLC_BearerConfig,NR_RLC_BearerConfig__servedRadioBearer_PR_drb_Identity);
  nr_drb_config(RLC_BearerConfig->rlc_Config, NR_RLC_Config_PR_am);
  nr_rlc_bearer_init_ul_spec(RLC_BearerConfig->mac_LogicalChannelConfig);

  Rlc_Bearer_ToAdd_list = calloc(1,sizeof(*Rlc_Bearer_ToAdd_list));
  ASN_SEQUENCE_ADD(&Rlc_Bearer_ToAdd_list->list, RLC_BearerConfig);

  if (ENB_NAS_USE_TUN){
    PROTOCOL_CTXT_SET_BY_MODULE_ID(&ctxt, 0, ENB_FLAG_YES, crnti, 0, 0, 0);
  }
  else{
    PROTOCOL_CTXT_SET_BY_MODULE_ID(&ctxt, 0, ENB_FLAG_NO, crnti, 0, 0,0);
  }

  nr_rrc_pdcp_config_asn1_req(
    &ctxt,
    (NR_SRB_ToAddModList_t *) NULL,
    rbconfig->drb_ToAddModList ,
    rbconfig->drb_ToReleaseList,
    0,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    Rlc_Bearer_ToAdd_list);

  nr_rrc_rlc_config_asn1_req (&ctxt,
      (NR_SRB_ToAddModList_t *) NULL,
      rbconfig->drb_ToAddModList,
      rbconfig->drb_ToReleaseList,
      (LTE_PMCH_InfoList_r9_t *) NULL,
      Rlc_Bearer_ToAdd_list);

  LOG_D(PDCP, "%s:%d: done RRC PDCP/RLC ASN1 request for UE rnti %x\n", __FUNCTION__, __LINE__, ctxt.rnti);

}

uint64_t get_pdcp_optmask(void)
{
  return pdcp_optmask;
}

boolean_t pdcp_remove_UE(
  const protocol_ctxt_t *const  ctxt_pP)
{
  int rnti = ctxt_pP->rnti;

  nr_pdcp_manager_lock(nr_pdcp_ue_manager);
  nr_pdcp_manager_remove_ue(nr_pdcp_ue_manager, rnti);
  nr_pdcp_manager_unlock(nr_pdcp_ue_manager);

  return 1;
}

void pdcp_config_set_security(
        const protocol_ctxt_t* const  ctxt_pP,
        pdcp_t *const pdcp_pP,
        const rb_id_t rb_id,
        const uint16_t lc_idP,
        const uint8_t security_modeP,
        uint8_t *const kRRCenc_pP,
        uint8_t *const kRRCint_pP,
        uint8_t *const kUPenc_pP)
{
  nr_pdcp_ue_t *ue;
  nr_pdcp_entity_t *rb;
  int rnti = ctxt_pP->rnti;
  int integrity_algorithm;
  int ciphering_algorithm;

  nr_pdcp_manager_lock(nr_pdcp_ue_manager);

  ue = nr_pdcp_manager_get_ue(nr_pdcp_ue_manager, rnti);

  /* TODO: proper handling of DRBs, for the moment only SRBs are handled */

  if (rb_id >= 1 && rb_id <= 2) {
    rb = ue->srb[rb_id - 1];

    if (rb == NULL) {
      LOG_E(PDCP, "%s:%d:%s: no SRB found (rnti %d, rb_id %ld)\n",
            __FILE__, __LINE__, __FUNCTION__, rnti, rb_id);
      nr_pdcp_manager_unlock(nr_pdcp_ue_manager);
      return;
    }

    integrity_algorithm = (security_modeP>>4) & 0xf;
    ciphering_algorithm = security_modeP & 0x0f;
    rb->set_security(rb, integrity_algorithm, (char *)kRRCint_pP,
                     ciphering_algorithm, (char *)kRRCenc_pP);
  } else {
    LOG_E(PDCP, "%s:%d:%s: TODO\n", __FILE__, __LINE__, __FUNCTION__);
    exit(1);
  }

  nr_pdcp_manager_unlock(nr_pdcp_ue_manager);
}

static boolean_t pdcp_data_req_srb(
  protocol_ctxt_t  *ctxt_pP,
  const rb_id_t rb_id,
  const mui_t muiP,
  const confirm_t confirmP,
  const sdu_size_t sdu_buffer_size,
  unsigned char *const sdu_buffer)
{
  LOG_D(PDCP, "%s() called, size %d\n", __func__, sdu_buffer_size);
  nr_pdcp_ue_t *ue;
  nr_pdcp_entity_t *rb;
  int rnti = ctxt_pP->rnti;

  if (ctxt_pP->module_id != 0 ||
      //ctxt_pP->enb_flag != 1 ||
      ctxt_pP->instance != 0 ||
      ctxt_pP->eNB_index != 0 /*||
      ctxt_pP->configured != 1 ||
      ctxt_pP->brOption != 0*/) {
    LOG_E(PDCP, "%s:%d:%s: fatal\n", __FILE__, __LINE__, __FUNCTION__);
    exit(1);
  }

  nr_pdcp_manager_lock(nr_pdcp_ue_manager);

  ue = nr_pdcp_manager_get_ue(nr_pdcp_ue_manager, rnti);

  if (rb_id < 1 || rb_id > 2)
    rb = NULL;
  else
    rb = ue->srb[rb_id - 1];

  if (rb == NULL) {
    LOG_E(PDCP, "%s:%d:%s: no SRB found (rnti %d, rb_id %ld)\n",
          __FILE__, __LINE__, __FUNCTION__, rnti, rb_id);
    nr_pdcp_manager_unlock(nr_pdcp_ue_manager);
    return 0;
  }

  rb->recv_sdu(rb, (char *)sdu_buffer, sdu_buffer_size, muiP);

  nr_pdcp_manager_unlock(nr_pdcp_ue_manager);

  return 1;
}


static boolean_t pdcp_data_req_drb(
  protocol_ctxt_t  *ctxt_pP,
  const rb_id_t rb_id,
  const mui_t muiP,
  const confirm_t confirmP,
  const sdu_size_t sdu_buffer_size,
  unsigned char *const sdu_buffer)
{
  LOG_D(PDCP, "%s() called, size %d\n", __func__, sdu_buffer_size);
  nr_pdcp_ue_t *ue;
  nr_pdcp_entity_t *rb;
  int rnti = ctxt_pP->rnti;

  if (ctxt_pP->module_id != 0 ||
      //ctxt_pP->enb_flag != 1 ||
      ctxt_pP->instance != 0 ||
      ctxt_pP->eNB_index != 0 /*||
      ctxt_pP->configured != 1 ||
      ctxt_pP->brOption != 0*/) {
    LOG_E(PDCP, "%s:%d:%s: fatal\n", __FILE__, __LINE__, __FUNCTION__);
    exit(1);
  }

  nr_pdcp_manager_lock(nr_pdcp_ue_manager);

  ue = nr_pdcp_manager_get_ue(nr_pdcp_ue_manager, rnti);

  if (rb_id < 1 || rb_id > 5)
    rb = NULL;
  else
    rb = ue->drb[rb_id - 1];

  if (rb == NULL) {
    LOG_E(PDCP, "%s:%d:%s: no DRB found (rnti %d, rb_id %ld)\n",
          __FILE__, __LINE__, __FUNCTION__, rnti, rb_id);
    nr_pdcp_manager_unlock(nr_pdcp_ue_manager);
    return 0;
  }
  char buffer[1024];
  hexdump((char *)sdu_buffer, sdu_buffer_size, buffer, sizeof(buffer));
  LOG_I(PDCP, "Melissa Elkadi in %s, this is hexdump of pdu %s from tunnel interface in PDCP layer\n",
        __FUNCTION__, buffer);
  rb->recv_sdu(rb, (char *)sdu_buffer, sdu_buffer_size, muiP);

  nr_pdcp_manager_unlock(nr_pdcp_ue_manager);

  return 1;
}

boolean_t pdcp_data_req(
  protocol_ctxt_t  *ctxt_pP,
  const srb_flag_t srb_flagP,
  const rb_id_t rb_id,
  const mui_t muiP,
  const confirm_t confirmP,
  const sdu_size_t sdu_buffer_size,
  unsigned char *const sdu_buffer,
  const pdcp_transmission_mode_t mode
#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))
  ,const uint32_t *const sourceL2Id
  ,const uint32_t *const destinationL2Id
#endif
  )
{
  if (srb_flagP) {
   return pdcp_data_req_srb(ctxt_pP, rb_id, muiP, confirmP, sdu_buffer_size, sdu_buffer);
  }
  else{
    return pdcp_data_req_drb(ctxt_pP, rb_id, muiP, confirmP, sdu_buffer_size, sdu_buffer);
  }
}

void pdcp_set_pdcp_data_ind_func(pdcp_data_ind_func_t pdcp_data_ind)
{
  /* nothing to do */
}

void pdcp_set_rlc_data_req_func(send_rlc_data_req_func_t send_rlc_data_req)
{
  /* nothing to do */
}

//Dummy function needed due to LTE dependencies
void
pdcp_mbms_run ( const protocol_ctxt_t *const  ctxt_pP){
  /* nothing to do */
}

void nr_pdcp_tick(int frame, int subframe)
{
  if (frame != nr_pdcp_current_time_last_frame ||
      subframe != nr_pdcp_current_time_last_subframe) {
    nr_pdcp_current_time_last_frame = frame;
    nr_pdcp_current_time_last_subframe = subframe;
    nr_pdcp_current_time++;
    nr_pdcp_wakeup_timer_thread(nr_pdcp_current_time);
  }
}
