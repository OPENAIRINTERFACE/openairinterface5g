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

#include "nr_pdcp_ue_manager.h"

/* from OAI */
#include "pdcp.h"

#define TODO do { \
    printf("%s:%d:%s: todo\n", __FILE__, __LINE__, __FUNCTION__); \
    exit(1); \
  } while (0)

static nr_pdcp_ue_manager_t *nr_pdcp_ue_manager;

/* necessary globals for OAI, not used internally */
hash_table_t  *pdcp_coll_p;
static uint64_t pdcp_optmask;

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

static void *rlc_data_req_thread(void *_)
{
  int i;

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

/****************************************************************************/
/* rlc_data_req queue - end                                                 */
/****************************************************************************/

/****************************************************************************/
/* hacks to be cleaned up at some point - begin                             */
/****************************************************************************/

#include "LAYER2/MAC/mac_extern.h"

void nr_ip_over_LTE_DRB_preconfiguration(void)
{
          // Addition for the use-case of 4G stack on top of 5G-NR.
          // We need to configure pdcp and rlc instances without having an actual
          // UE RRC Connection. In order to be able to test the NR PHY with some injected traffic
          // on top of the LTE stack.
          protocol_ctxt_t ctxt;
          LTE_DRB_ToAddModList_t*                DRB_configList=NULL;
          DRB_configList = CALLOC(1, sizeof(LTE_DRB_ToAddModList_t));
          struct LTE_LogicalChannelConfig        *DRB_lchan_config                                 = NULL;
          struct LTE_RLC_Config                  *DRB_rlc_config                   = NULL;
          struct LTE_PDCP_Config                 *DRB_pdcp_config                  = NULL;
          struct LTE_PDCP_Config__rlc_UM         *PDCP_rlc_UM                      = NULL;

          struct LTE_DRB_ToAddMod                *DRB_config                       = NULL;
          struct LTE_LogicalChannelConfig__ul_SpecificParameters *DRB_ul_SpecificParameters        = NULL;
          long  *logicalchannelgroup_drb;


          //Static preconfiguration of DRB
          DRB_config = CALLOC(1, sizeof(*DRB_config));

          DRB_config->eps_BearerIdentity = CALLOC(1, sizeof(long));
          // allowed value 5..15, value : x+4
          *(DRB_config->eps_BearerIdentity) = 1; //ue_context_pP->ue_context.e_rab[i].param.e_rab_id;//+ 4; // especial case generation
          //   DRB_config->drb_Identity =  1 + drb_identity_index + e_rab_done;// + i ;// (DRB_Identity_t) ue_context_pP->ue_context.e_rab[i].param.e_rab_id;
          // 1 + drb_identiy_index;
          DRB_config->drb_Identity = 1;
          DRB_config->logicalChannelIdentity = CALLOC(1, sizeof(long));
          *(DRB_config->logicalChannelIdentity) = DRB_config->drb_Identity + 2; //(long) (ue_context_pP->ue_context.e_rab[i].param.e_rab_id + 2); // value : x+2

          DRB_rlc_config = CALLOC(1, sizeof(*DRB_rlc_config));
          DRB_config->rlc_Config = DRB_rlc_config;

          DRB_pdcp_config = CALLOC(1, sizeof(*DRB_pdcp_config));
          DRB_config->pdcp_Config = DRB_pdcp_config;
          DRB_pdcp_config->discardTimer = CALLOC(1, sizeof(long));
          *DRB_pdcp_config->discardTimer = LTE_PDCP_Config__discardTimer_infinity;
          DRB_pdcp_config->rlc_AM = NULL;
          DRB_pdcp_config->rlc_UM = NULL;

          DRB_rlc_config->present = LTE_RLC_Config_PR_um_Bi_Directional;
          DRB_rlc_config->choice.um_Bi_Directional.ul_UM_RLC.sn_FieldLength = LTE_SN_FieldLength_size10;
          DRB_rlc_config->choice.um_Bi_Directional.dl_UM_RLC.sn_FieldLength = LTE_SN_FieldLength_size10;
          DRB_rlc_config->choice.um_Bi_Directional.dl_UM_RLC.t_Reordering = LTE_T_Reordering_ms35;
          // PDCP
          PDCP_rlc_UM = CALLOC(1, sizeof(*PDCP_rlc_UM));
          DRB_pdcp_config->rlc_UM = PDCP_rlc_UM;
          PDCP_rlc_UM->pdcp_SN_Size = LTE_PDCP_Config__rlc_UM__pdcp_SN_Size_len12bits;

          DRB_pdcp_config->headerCompression.present = LTE_PDCP_Config__headerCompression_PR_notUsed;

          DRB_lchan_config = CALLOC(1, sizeof(*DRB_lchan_config));
          DRB_config->logicalChannelConfig = DRB_lchan_config;
          DRB_ul_SpecificParameters = CALLOC(1, sizeof(*DRB_ul_SpecificParameters));
          DRB_lchan_config->ul_SpecificParameters = DRB_ul_SpecificParameters;

          DRB_ul_SpecificParameters->priority= 4;

          DRB_ul_SpecificParameters->prioritisedBitRate = LTE_LogicalChannelConfig__ul_SpecificParameters__prioritisedBitRate_kBps8;
          //LogicalChannelConfig__ul_SpecificParameters__prioritisedBitRate_infinity;
          DRB_ul_SpecificParameters->bucketSizeDuration =
          LTE_LogicalChannelConfig__ul_SpecificParameters__bucketSizeDuration_ms50;

          logicalchannelgroup_drb = CALLOC(1, sizeof(long));
          *logicalchannelgroup_drb = 1;//(i+1) % 3;
          DRB_ul_SpecificParameters->logicalChannelGroup = logicalchannelgroup_drb;

          ASN_SEQUENCE_ADD(&DRB_configList->list,DRB_config);

          if (ENB_NAS_USE_TUN){
                  PROTOCOL_CTXT_SET_BY_MODULE_ID(&ctxt, 0, ENB_FLAG_YES, 0x1234, 0, 0,0);
          }
          else{
                  PROTOCOL_CTXT_SET_BY_MODULE_ID(&ctxt, 0, ENB_FLAG_NO, 0x1234, 0, 0,0);
          }

          rrc_pdcp_config_asn1_req(&ctxt,
                       (LTE_SRB_ToAddModList_t *) NULL,
                       DRB_configList,
                       (LTE_DRB_ToReleaseList_t *) NULL,
                       0xff, NULL, NULL, NULL
                       , (LTE_PMCH_InfoList_r9_t *) NULL,
                       &DRB_config->drb_Identity);

        rrc_rlc_config_asn1_req(&ctxt,
                       (LTE_SRB_ToAddModList_t*)NULL,
                       DRB_configList,
                       (LTE_DRB_ToReleaseList_t*)NULL
        //#if (RRC_VERSION >= MAKE_VERSION(10, 0, 0))
                       ,(LTE_PMCH_InfoList_r9_t *)NULL
                       , 0, 0
        //#endif
                 );
}

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

  while (1) {
    len = read(nas_sock_fd[0], &rx_buf, NL_MAX_PAYLOAD);
    if (len == -1) {
      LOG_E(PDCP, "%s:%d:%s: fatal\n", __FILE__, __LINE__, __FUNCTION__);
      exit(1);
    }

printf("\n\n\n########## nas_sock_fd read returns len %d\n", len);

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

  while (1) {
    len = read(nas_sock_fd[0], &rx_buf, NL_MAX_PAYLOAD);
    if (len == -1) {
      LOG_E(PDCP, "%s:%d:%s: fatal\n", __FILE__, __LINE__, __FUNCTION__);
      exit(1);
    }

printf("\n\n\n########## nas_sock_fd read returns len %d\n", len);

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
  init_nr_rlc_data_req_queue();
}

#include "nfapi/oai_integration/vendor_ext.h"
#include "targets/RT/USER/lte-softmodem.h"
#include "openair2/RRC/NAS/nas_config.h"

uint64_t pdcp_module_init(uint64_t _pdcp_optmask)
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
      int num_if = (NFAPI_MODE == NFAPI_UE_STUB_PNF || IS_SOFTMODEM_SIML1 )?MAX_NUMBER_NETIF:1;
      netlink_init_tun("ue",num_if);
      //Add --nr-ip-over-lte option check for next line
      if (IS_SOFTMODEM_NOS1)
          nas_config(1, 1, 2, "ue");
      LOG_I(PDCP, "UE pdcp will use tun interface\n");
      start_pdcp_tun_ue();
    } else if(ENB_NAS_USE_TUN) {
      netlink_init_tun("enb",1);
      nas_config(1, 1, 1, "enb");
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

  len = write(nas_sock_fd[0], buf, size);
  if (len != size) {
    LOG_E(PDCP, "%s:%d:%s: fatal\n", __FILE__, __LINE__, __FUNCTION__);
    exit(1);
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

printf("!!!!!!! deliver_pdu_drb (srb %d) calling rlc_data_req size %d: ", rb_id, size);
//for (i = 0; i < size; i++) printf(" %2.2x", (unsigned char)memblock->data[i]);
printf("\n");
  enqueue_rlc_data_req(&ctxt, 0, MBMS_FLAG_NO, rb_id, sdu_id, 0, size, memblock, NULL, NULL);
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
  protocol_ctxt_t ctxt;

  while (1) {
    itti_poll_msg(ctxt_pP->enb_flag ? TASK_PDCP_ENB : TASK_PDCP_UE, &msg_p);
    if (msg_p == NULL)
      break;
    switch (ITTI_MSG_ID(msg_p)) {
    case RRC_DCCH_DATA_REQ:
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

static void add_srb(int rnti, struct LTE_SRB_ToAddMod *s)
{
  TODO;
}

static void add_drb_am(int rnti, struct NR_DRB_ToAddMod *s)
{
  nr_pdcp_entity_t *pdcp_drb;
  nr_pdcp_ue_t *ue;

  int drb_id = s->drb_Identity;

printf("\n\n################# rnti %d add drb %d\n\n\n", rnti, drb_id);

  if (drb_id != 1) {
    LOG_E(PDCP, "%s:%d:%s: fatal, bad drb id %d\n",
          __FILE__, __LINE__, __FUNCTION__, drb_id);
    exit(1);
  }

  nr_pdcp_manager_lock(nr_pdcp_ue_manager);
  ue = nr_pdcp_manager_get_ue(nr_pdcp_ue_manager, rnti);
  if (ue->drb[drb_id-1] != NULL) {
    LOG_D(PDCP, "%s:%d:%s: warning DRB %d already exist for ue %d, do nothing\n",
          __FILE__, __LINE__, __FUNCTION__, drb_id, rnti);
  } else {
    pdcp_drb = new_nr_pdcp_entity_drb_am(drb_id, deliver_sdu_drb, ue, deliver_pdu_drb, ue);
    nr_pdcp_ue_add_drb_pdcp_entity(ue, drb_id, pdcp_drb);

    LOG_D(PDCP, "%s:%d:%s: added drb %d to ue %d\n",
          __FILE__, __LINE__, __FUNCTION__, drb_id, rnti);
  }
  nr_pdcp_manager_unlock(nr_pdcp_ue_manager);
}

static void add_drb(int rnti, struct NR_DRB_ToAddMod *s, NR_RLC_Config_t *rlc_Config)
{
  switch (rlc_Config->present) {
  case NR_RLC_Config_PR_am:
    add_drb_am(rnti, s);
    break;
  case NR_RLC_Config_PR_um_Bi_Directional:
    //add_drb_um(rnti, s);
    /* hack */
    add_drb_am(rnti, s);
    break;
  default:
    LOG_E(PDCP, "%s:%d:%s: fatal: unhandled DRB type\n",
          __FILE__, __LINE__, __FUNCTION__);
    exit(1);
  }
}

boolean_t nr_rrc_pdcp_config_asn1_req(
  const protocol_ctxt_t *const  ctxt_pP,
  NR_SRB_ToAddModList_t  *const srb2add_list,
  NR_DRB_ToAddModList_t  *const drb2add_list,
  NR_DRB_ToReleaseList_t *const drb2release_list,
  const uint8_t                   security_modeP,
  uint8_t                  *const kRRCenc,
  uint8_t                  *const kRRCint,
  uint8_t                  *const kUPenc
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
      drb2release_list != NULL ||
      security_modeP != 255 ||
      //kRRCenc != NULL ||
      //kRRCint != NULL ||
      //kUPenc != NULL ||
      pmch_InfoList_r9 != NULL /*||
      defaultDRB != NULL */) {
    TODO;
  }

  if (srb2add_list != NULL) {
    for (i = 0; i < srb2add_list->list.count; i++) {
      add_srb(rnti, srb2add_list->list.array[i]);
    }
  }

  if (drb2add_list != NULL) {
    for (i = 0; i < drb2add_list->list.count; i++) {
      LOG_I(PDCP, "Before calling add_drb \n");
      add_drb(rnti, drb2add_list->list.array[i], rlc_bearer2add_list->list.array[i]->rlc_Config);
    }
  }

  /* update security */
  if (kRRCint != NULL) {
    /* todo */
  }

  free(kRRCenc);
  free(kRRCint);
  free(kUPenc);

  return 0;
}

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
  int rnti = ctxt_pP->rnti;
  int i;

  if (//ctxt_pP->enb_flag != 1 ||
      ctxt_pP->module_id != 0 ||
      ctxt_pP->instance != 0 ||
      ctxt_pP->eNB_index != 0 ||
      //ctxt_pP->configured != 2 ||
      //srb2add_list == NULL ||
      //drb2add_list != NULL ||
      drb2release_list != NULL ||
      security_modeP != 255 ||
      //kRRCenc != NULL ||
      //kRRCint != NULL ||
      //kUPenc != NULL ||
      pmch_InfoList_r9 != NULL /*||
      defaultDRB != NULL */) {
    TODO;
  }

  if (srb2add_list != NULL) {
    for (i = 0; i < srb2add_list->list.count; i++) {
      add_srb(rnti, srb2add_list->list.array[i]);
    }
  }

  if (drb2add_list != NULL) {
    for (i = 0; i < drb2add_list->list.count; i++) {
      add_drb(rnti, drb2add_list->list.array[i], NULL);
    }
  }

  /* update security */
  if (kRRCint != NULL) {
    /* todo */
  }

  free(kRRCenc);
  free(kRRCint);
  free(kUPenc);

  return 0;
}

uint64_t get_pdcp_optmask(void)
{
  return pdcp_optmask;
}

boolean_t pdcp_remove_UE(
  const protocol_ctxt_t *const  ctxt_pP)
{
  TODO;
  return 1;
}

void pdcp_config_set_security(const protocol_ctxt_t* const  ctxt_pP, pdcp_t *pdcp_pP, rb_id_t rb_id,
                              uint16_t lc_idP, uint8_t security_modeP, uint8_t *kRRCenc_pP, uint8_t *kRRCint_pP, uint8_t *kUPenc_pP)
{
  TODO;
}

static boolean_t pdcp_data_req_drb(
  protocol_ctxt_t  *ctxt_pP,
  const rb_id_t rb_id,
  const mui_t muiP,
  const confirm_t confirmP,
  const sdu_size_t sdu_buffer_size,
  unsigned char *const sdu_buffer)
{
printf("pdcp_data_req called size %d\n", sdu_buffer_size);
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
    LOG_E(PDCP, "%s:%d:%s: fatal: no DRB found (rnti %d, rb_id %ld)\n",
          __FILE__, __LINE__, __FUNCTION__, rnti, rb_id);
    exit(1);
  }

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
  if (srb_flagP) { TODO; }
  return pdcp_data_req_drb(ctxt_pP, rb_id, muiP, confirmP, sdu_buffer_size,
                           sdu_buffer);
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
