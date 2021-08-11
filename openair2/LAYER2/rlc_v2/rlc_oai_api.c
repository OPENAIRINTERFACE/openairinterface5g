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

/* from openair */
#include "rlc.h"
#include "pdcp.h"

/* from new rlc module */
#include "asn1_utils.h"
#include "rlc_ue_manager.h"
#include "rlc_entity.h"

#include <stdint.h>

static rlc_ue_manager_t *rlc_ue_manager;

/* TODO: handle time a bit more properly */
static uint64_t rlc_current_time;
static int      rlc_current_time_last_frame;
static int      rlc_current_time_last_subframe;

void mac_rlc_data_ind     (
  const module_id_t         module_idP,
  const rnti_t              rntiP,
  const eNB_index_t         eNB_index,
  const frame_t             frameP,
  const eNB_flag_t          enb_flagP,
  const MBMS_flag_t         MBMS_flagP,
  const logical_chan_id_t   channel_idP,
  char                     *buffer_pP,
  const tb_size_t           tb_sizeP,
  num_tb_t                  num_tbP,
  crc_t                    *crcs_pP)
{
  rlc_ue_t *ue;
  rlc_entity_t *rb;
  int rnti;
  int channel_id;

  if (enb_flagP == 1 && module_idP != 0) {
    LOG_E(RLC, "%s:%d:%s: fatal, module_id must be 0 for eNB\n",
          __FILE__, __LINE__, __FUNCTION__);
    exit(1);
  }

  if (/*module_idP != 0 ||*/ eNB_index != 0 /*|| enb_flagP != 1 || MBMS_flagP != 0*/) {
    LOG_E(RLC, "%s:%d:%s: fatal\n", __FILE__, __LINE__, __FUNCTION__);
    exit(1);
  }

  if (enb_flagP)
    T(T_ENB_RLC_MAC_UL, T_INT(module_idP), T_INT(rntiP),
      T_INT(channel_idP), T_INT(tb_sizeP));

  /* TODO: better handle mbms, maybe we should not change rnti here */
  if (!enb_flagP && MBMS_flagP) {
    rnti = 0xfffd;
    /* TODO: handle channel_id properly */
    if (channel_idP != 5) {
      LOG_E(RLC, "%s:%d:%s: fatal\n", __FILE__, __LINE__, __FUNCTION__);
      exit(1);
    }
    channel_id = 7;
  } else {
    rnti = rntiP;
    channel_id = channel_idP;
  }

  rlc_manager_lock(rlc_ue_manager);
  ue = rlc_manager_get_ue(rlc_ue_manager, rnti);

  switch (channel_id) {
  case 1 ... 2: rb = ue->srb[channel_id - 1]; break;
  case 3 ... 7: rb = ue->drb[channel_id - 3]; break;
  default:      rb = NULL;                    break;
  }

  if (rb != NULL) {
    rb->set_time(rb, rlc_current_time);
    rb->recv_pdu(rb, buffer_pP, tb_sizeP);
  } else {
    LOG_E(RLC, "%s:%d:%s: fatal: no RB found (rnti %d channel ID %d)\n",
          __FILE__, __LINE__, __FUNCTION__, rnti, channel_id);
    exit(1);
  }

  rlc_manager_unlock(rlc_ue_manager);
}

tbs_size_t mac_rlc_data_req(
  const module_id_t       module_idP,
  const rnti_t            rntiP,
  const eNB_index_t       eNB_index,
  const frame_t           frameP,
  const eNB_flag_t        enb_flagP,
  const MBMS_flag_t       MBMS_flagP,
  const logical_chan_id_t channel_idP,
  const tb_size_t         tb_sizeP,
  char             *buffer_pP,
  const uint32_t sourceL2Id,
  const uint32_t destinationL2Id
   )
{
  int ret;
  rlc_ue_t *ue;
  rlc_entity_t *rb;
  int maxsize;

  rlc_manager_lock(rlc_ue_manager);
  ue = rlc_manager_get_ue(rlc_ue_manager, rntiP);

  switch (channel_idP) {
  case 1 ... 2: rb = ue->srb[channel_idP - 1]; break;
  case 3 ... 7: rb = ue->drb[channel_idP - 3]; break;
  default:      rb = NULL;                     break;
  }

  if (MBMS_flagP == MBMS_FLAG_YES) {
    if (channel_idP >= 1 && channel_idP <= 5)
      rb = ue->drb[channel_idP - 1];
    else
      rb = NULL;
  }


  if (rb != NULL) {
    rb->set_time(rb, rlc_current_time);
    maxsize = tb_sizeP;
    ret = rb->generate_pdu(rb, buffer_pP, maxsize);
  } else {
    LOG_E(RLC, "%s:%d:%s: fatal: data req for unknown RB\n", __FILE__, __LINE__, __FUNCTION__);
    exit(1);
    ret = 0;
  }

  rlc_manager_unlock(rlc_ue_manager);

  if (enb_flagP)
    T(T_ENB_RLC_MAC_DL, T_INT(module_idP), T_INT(rntiP),
      T_INT(channel_idP), T_INT(ret));

  return ret;
}

mac_rlc_status_resp_t mac_rlc_status_ind(
  const module_id_t       module_idP,
  const rnti_t            rntiP,
  const eNB_index_t       eNB_index,
  const frame_t           frameP,
  const sub_frame_t       subframeP,
  const eNB_flag_t        enb_flagP,
  const MBMS_flag_t       MBMS_flagP,
  const logical_chan_id_t channel_idP,
  const uint32_t sourceL2Id,
  const uint32_t destinationL2Id
  )
{
  rlc_ue_t *ue;
  mac_rlc_status_resp_t ret;
  rlc_entity_t *rb;

  rlc_manager_lock(rlc_ue_manager);
  ue = rlc_manager_get_ue(rlc_ue_manager, rntiP);

  switch (channel_idP) {
  case 1 ... 2: rb = ue->srb[channel_idP - 1]; break;
  case 3 ... 7: rb = ue->drb[channel_idP - 3]; break;
  default:      rb = NULL;                     break;
  }

  if (MBMS_flagP == MBMS_FLAG_YES) {
    if (channel_idP >= 1 && channel_idP <= 5)
      rb = ue->drb[channel_idP - 1];
    else
      rb = NULL;
  }

  if (rb != NULL) {
    rlc_entity_buffer_status_t buf_stat;
    rb->set_time(rb, rlc_current_time);
    /* 36.321 deals with BSR values up to 3000000 bytes, after what it
     * reports '> 3000000' (table 6.1.3.1-2). Passing 4000000 is thus
     * more than enough.
     */
    buf_stat = rb->buffer_status(rb, 4000000);
    ret.bytes_in_buffer = buf_stat.status_size
                        + buf_stat.retx_size
                        + buf_stat.tx_size;
  } else {
    ret.bytes_in_buffer = 0;
  }

  rlc_manager_unlock(rlc_ue_manager);

  ret.pdus_in_buffer = 0;
  /* TODO: creation time may be important (unit: frame, as it seems) */
  ret.head_sdu_creation_time = 0;
  ret.head_sdu_remaining_size_to_send = 0;
  ret.head_sdu_is_segmented = 0;
  return ret;
}

rlc_buffer_occupancy_t mac_rlc_get_buffer_occupancy_ind(
  const module_id_t       module_idP,
  const rnti_t            rntiP,
  const eNB_index_t       eNB_index,
  const frame_t           frameP,
  const sub_frame_t       subframeP,
  const eNB_flag_t        enb_flagP,
  const logical_chan_id_t channel_idP)
{
  rlc_ue_t *ue;
  rlc_buffer_occupancy_t ret;
  rlc_entity_t *rb;

  if (enb_flagP) {
    LOG_E(RLC, "Tx mac_rlc_get_buffer_occupancy_ind function is not implemented for eNB LcId=%u\n", channel_idP);
    exit(1);
  }

  rlc_manager_lock(rlc_ue_manager);
  ue = rlc_manager_get_ue(rlc_ue_manager, rntiP);

  switch (channel_idP) {
  case 1 ... 2: rb = ue->srb[channel_idP - 1]; break;
  case 3 ... 7: rb = ue->drb[channel_idP - 3]; break;
  default:      rb = NULL;                     break;
  }

  if (rb != NULL) {
    rlc_entity_buffer_status_t buf_stat;
    rb->set_time(rb, rlc_current_time);
    /* 36.321 deals with BSR values up to 3000000 bytes, after what it
     * reports '> 3000000' (table 6.1.3.1-2). Passing 4000000 is thus
     * more than enough.
     */
    buf_stat = rb->buffer_status(rb, 4000000);
    ret = buf_stat.status_size
        + buf_stat.retx_size
        + buf_stat.tx_size;
  } else {
    ret = 0;
  }

  rlc_manager_unlock(rlc_ue_manager);

  return ret;
}


rlc_op_status_t rlc_data_req     (const protocol_ctxt_t *const ctxt_pP,
                                  const srb_flag_t   srb_flagP,
                                  const MBMS_flag_t  MBMS_flagP,
                                  const rb_id_t      rb_idP,
                                  const mui_t        muiP,
                                  confirm_t    confirmP,
                                  sdu_size_t   sdu_sizeP,
                                  mem_block_t *sdu_pP,
                                  const uint32_t *const sourceL2Id,
                                  const uint32_t *const destinationL2Id
                                 )
{
  int rnti = ctxt_pP->rnti;
  rlc_ue_t *ue;
  rlc_entity_t *rb;

  if (MBMS_flagP == MBMS_FLAG_YES)
    rnti = 0xfffd;

  LOG_D(RLC, "%s rnti %d srb_flag %d rb_id %d mui %d confirm %d sdu_size %d MBMS_flag %d\n",
        __FUNCTION__, rnti, srb_flagP, (int)rb_idP, muiP, confirmP, sdu_sizeP,
        MBMS_flagP);

  if (ctxt_pP->enb_flag)
    T(T_ENB_RLC_DL, T_INT(ctxt_pP->module_id),
      T_INT(ctxt_pP->rnti), T_INT(rb_idP), T_INT(sdu_sizeP));

  rlc_manager_lock(rlc_ue_manager);
  ue = rlc_manager_get_ue(rlc_ue_manager, rnti);

  rb = NULL;

  if (srb_flagP) {
    if (rb_idP >= 1 && rb_idP <= 2)
      rb = ue->srb[rb_idP - 1];
  } else {
    if (rb_idP >= 1 && rb_idP <= 5)
      rb = ue->drb[rb_idP - 1];
  }

  if( MBMS_flagP == MBMS_FLAG_YES) {
    if (rb_idP >= 1 && rb_idP <= 5)
      rb = ue->drb[rb_idP - 1];
  }

  if (rb != NULL) {
    rb->set_time(rb, rlc_current_time);
    rb->recv_sdu(rb, (char *)sdu_pP->data, sdu_sizeP, muiP);
  } else {
    LOG_E(RLC, "%s:%d:%s: fatal: SDU sent to unknown RB\n", __FILE__, __LINE__, __FUNCTION__);
    exit(1);
  }

  rlc_manager_unlock(rlc_ue_manager);

  free_mem_block(sdu_pP, __func__);

  return RLC_OP_STATUS_OK;
}

int rlc_module_init(int enb_flag)
{
  static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
  static int inited = 0;

  if (pthread_mutex_lock(&lock)) abort();

  if (inited) {
    LOG_E(RLC, "%s:%d:%s: fatal\n", __FILE__, __LINE__, __FUNCTION__);
    exit(1);
  }

  inited = 1;

  rlc_ue_manager = new_rlc_ue_manager(enb_flag);

  if (pthread_mutex_unlock(&lock)) abort();

  return 0;
}

void rlc_util_print_hex_octets(comp_name_t componentP, unsigned char *dataP, const signed long sizeP)
{
}

#include "common/ran_context.h"
extern RAN_CONTEXT_t RC;

static void deliver_sdu(void *_ue, rlc_entity_t *entity, char *buf, int size)
{
  rlc_ue_t *ue = _ue;
  int is_srb;
  int rb_id;
  protocol_ctxt_t ctx;
  mem_block_t *memblock;
  int i;
  int is_enb;
  int is_mbms;

  /* TODO: be sure it's fine to check rnti for MBMS */
  is_mbms = ue->rnti == 0xfffd;

  /* is it SRB? */
  for (i = 0; i < 2; i++) {
    if (entity == ue->srb[i]) {
      is_srb = 1;
      rb_id = i+1;
      goto rb_found;
    }
  }

  /* maybe DRB? */
  for (i = 0; i < 5; i++) {
    if (entity == ue->drb[i]) {
      is_srb = 0;
      rb_id = i+1;
      goto rb_found;
    }
  }

  LOG_E(RLC, "%s:%d:%s: fatal, no RB found for ue %d\n",
        __FILE__, __LINE__, __FUNCTION__, ue->rnti);
  exit(1);

rb_found:
  LOG_D(RLC, "%s:%d:%s: delivering SDU (rnti %d is_srb %d rb_id %d) size %d",
        __FILE__, __LINE__, __FUNCTION__, ue->rnti, is_srb, rb_id, size);

  memblock = get_free_mem_block(size, __func__);
  if (memblock == NULL) {
    LOG_E(RLC, "%s:%d:%s: ERROR: get_free_mem_block failed\n", __FILE__, __LINE__, __FUNCTION__);
    exit(1);
  }
  memcpy(memblock->data, buf, size);

  /* unused fields? */
  ctx.instance = ue->module_id;
  ctx.frame = 0;
  ctx.subframe = 0;
  ctx.eNB_index = 0;
  ctx.configured = 1;
  ctx.brOption = 0;

  /* used fields? */
  ctx.module_id = ue->module_id;
  ctx.rnti = ue->rnti;

  is_enb = rlc_manager_get_enb_flag(rlc_ue_manager);
  ctx.enb_flag = is_enb;

  if (is_enb) {
    T(T_ENB_RLC_UL,
      T_INT(0 /*ctxt_pP->module_id*/),
      T_INT(ue->rnti), T_INT(rb_id), T_INT(size));

    const ngran_node_t type = RC.rrc[0 /*ctxt_pP->module_id*/]->node_type;
    AssertFatal(type != ngran_eNB_CU && type != ngran_ng_eNB_CU && type != ngran_gNB_CU,
                "Can't be CU, bad node type %d\n", type);

    if (NODE_IS_DU(type) && is_srb == 1) {
      MessageDef *msg = itti_alloc_new_message(TASK_RLC_ENB, 0, F1AP_UL_RRC_MESSAGE);
      F1AP_UL_RRC_MESSAGE(msg).rnti = ue->rnti;
      F1AP_UL_RRC_MESSAGE(msg).srb_id = rb_id;
      F1AP_UL_RRC_MESSAGE(msg).rrc_container = (unsigned char *)buf;
      F1AP_UL_RRC_MESSAGE(msg).rrc_container_length = size;
      itti_send_msg_to_task(TASK_DU_F1, ENB_MODULE_ID_TO_INSTANCE(0 /*ctxt_pP->module_id*/), msg);
      return;
    }
  }

  if (!get_pdcp_data_ind_func()(&ctx, is_srb, is_mbms, rb_id, size, memblock, NULL, NULL)) {
    LOG_E(RLC, "%s:%d:%s: ERROR: pdcp_data_ind failed (is_srb %d rb_id %d rnti %d)\n",
          __FILE__, __LINE__, __FUNCTION__,
          is_srb, rb_id, ue->rnti);
    /* what to do in case of failure? for the moment: nothing */
  }
}

static void successful_delivery(void *_ue, rlc_entity_t *entity, int sdu_id)
{
  rlc_ue_t *ue = _ue;
  int i;
  int is_srb;
  int rb_id;
  MessageDef *msg;
  int is_enb;

  /* is it SRB? */
  for (i = 0; i < 2; i++) {
    if (entity == ue->srb[i]) {
      is_srb = 1;
      rb_id = i+1;
      goto rb_found;
    }
  }

  /* maybe DRB? */
  for (i = 0; i < 5; i++) {
    if (entity == ue->drb[i]) {
      is_srb = 0;
      rb_id = i+1;
      goto rb_found;
    }
  }

  LOG_E(RLC, "%s:%d:%s: fatal, no RB found for ue %d\n",
        __FILE__, __LINE__, __FUNCTION__, ue->rnti);
  exit(1);

rb_found:
  LOG_D(RLC, "sdu %d was successfully delivered on %s %d\n",
        sdu_id,
        is_srb ? "SRB" : "DRB",
        rb_id);

  /* TODO: do something for DRBs? */
  if (is_srb == 0)
    return;

  is_enb = rlc_manager_get_enb_flag(rlc_ue_manager);
  if (!is_enb)
    return;

  msg = itti_alloc_new_message(TASK_RLC_ENB, 0, RLC_SDU_INDICATION);
  RLC_SDU_INDICATION(msg).rnti          = ue->rnti;
  RLC_SDU_INDICATION(msg).is_successful = 1;
  RLC_SDU_INDICATION(msg).srb_id        = rb_id;
  RLC_SDU_INDICATION(msg).message_id    = sdu_id;
  /* TODO: accept more than 1 instance? here we send to instance id 0 */
  itti_send_msg_to_task(TASK_RRC_ENB, 0, msg);
}

static void max_retx_reached(void *_ue, rlc_entity_t *entity)
{
  rlc_ue_t *ue = _ue;
  int i;
  int is_srb;
  int rb_id;
  MessageDef *msg;
  int is_enb;

  /* is it SRB? */
  for (i = 0; i < 2; i++) {
    if (entity == ue->srb[i]) {
      is_srb = 1;
      rb_id = i+1;
      goto rb_found;
    }
  }

  /* maybe DRB? */
  for (i = 0; i < 5; i++) {
    if (entity == ue->drb[i]) {
      is_srb = 0;
      rb_id = i+1;
      goto rb_found;
    }
  }

  LOG_E(RLC, "%s:%d:%s: fatal, no RB found for ue %d\n",
        __FILE__, __LINE__, __FUNCTION__, ue->rnti);
  exit(1);

rb_found:
  LOG_D(RLC, "max RETX reached on %s %d\n",
        is_srb ? "SRB" : "DRB",
        rb_id);

  /* TODO: do something for DRBs? */
  if (is_srb == 0)
    return;

  is_enb = rlc_manager_get_enb_flag(rlc_ue_manager);
  if (!is_enb)
    return;

  msg = itti_alloc_new_message(TASK_RLC_ENB, 0, RLC_SDU_INDICATION);
  RLC_SDU_INDICATION(msg).rnti          = ue->rnti;
  RLC_SDU_INDICATION(msg).is_successful = 0;
  RLC_SDU_INDICATION(msg).srb_id        = rb_id;
  RLC_SDU_INDICATION(msg).message_id    = -1;
  /* TODO: accept more than 1 instance? here we send to instance id 0 */
  itti_send_msg_to_task(TASK_RRC_ENB, 0, msg);
}

static void add_srb(int rnti, int module_id, struct LTE_SRB_ToAddMod *s)
{
  rlc_entity_t            *rlc_am;
  rlc_ue_t                *ue;

  struct LTE_SRB_ToAddMod__rlc_Config *r = s->rlc_Config;
  struct LTE_SRB_ToAddMod__logicalChannelConfig *l = s->logicalChannelConfig;
  int srb_id = s->srb_Identity;
  int logical_channel_group;

  int t_reordering;
  int t_status_prohibit;
  int t_poll_retransmit;
  int poll_pdu;
  int poll_byte;
  int max_retx_threshold;

  if (srb_id != 1 && srb_id != 2) {
    LOG_E(RLC, "%s:%d:%s: fatal, bad srb id %d\n",
          __FILE__, __LINE__, __FUNCTION__, srb_id);
    exit(1);
  }

  switch (l->present) {
  case LTE_SRB_ToAddMod__logicalChannelConfig_PR_explicitValue:
    logical_channel_group = *l->choice.explicitValue.ul_SpecificParameters->logicalChannelGroup;
    break;
  case LTE_SRB_ToAddMod__logicalChannelConfig_PR_defaultValue:
    /* default value from 36.331 9.2.1 */
    logical_channel_group = 0;
    break;
  default:
    LOG_E(RLC, "%s:%d:%s: fatal error\n", __FILE__, __LINE__, __FUNCTION__);
    exit(1);
  }

  /* TODO: accept other values? */
  if (logical_channel_group != 0) {
    LOG_E(RLC, "%s:%d:%s: fatal error\n", __FILE__, __LINE__, __FUNCTION__);
    exit(1);
  }

  switch (r->present) {
  case LTE_SRB_ToAddMod__rlc_Config_PR_explicitValue: {
    struct LTE_RLC_Config__am *am;
    if (r->choice.explicitValue.present != LTE_RLC_Config_PR_am) {
      LOG_E(RLC, "%s:%d:%s: fatal error, must be RLC AM\n",
            __FILE__, __LINE__, __FUNCTION__);
      exit(1);
    }
    am = &r->choice.explicitValue.choice.am;
    t_reordering       = decode_t_reordering(am->dl_AM_RLC.t_Reordering);
    t_status_prohibit  = decode_t_status_prohibit(am->dl_AM_RLC.t_StatusProhibit);
    t_poll_retransmit  = decode_t_poll_retransmit(am->ul_AM_RLC.t_PollRetransmit);
    poll_pdu           = decode_poll_pdu(am->ul_AM_RLC.pollPDU);
    poll_byte          = decode_poll_byte(am->ul_AM_RLC.pollByte);
    max_retx_threshold = decode_max_retx_threshold(am->ul_AM_RLC.maxRetxThreshold);
    break;
  }
  case LTE_SRB_ToAddMod__rlc_Config_PR_defaultValue:
    /* default values from 36.331 9.2.1 */
    t_reordering       = 35;
    t_status_prohibit  = 0;
    t_poll_retransmit  = 45;
    poll_pdu           = -1;
    poll_byte          = -1;
    max_retx_threshold = 4;
    break;
  default:
    LOG_E(RLC, "%s:%d:%s: fatal error\n", __FILE__, __LINE__, __FUNCTION__);
    exit(1);
  }

  rlc_manager_lock(rlc_ue_manager);
  ue = rlc_manager_get_ue(rlc_ue_manager, rnti);
  ue->module_id = module_id;
  if (ue->srb[srb_id-1] != NULL) {
    LOG_D(RLC, "%s:%d:%s: warning SRB %d already exist for ue %d, do nothing\n",
          __FILE__, __LINE__, __FUNCTION__, srb_id, rnti);
  } else {
    rlc_am = new_rlc_entity_am(100000,
                               100000,
                               deliver_sdu, ue,
                               successful_delivery, ue,
                               max_retx_reached, ue,
                               t_reordering, t_status_prohibit,
                               t_poll_retransmit,
                               poll_pdu, poll_byte, max_retx_threshold);
    rlc_ue_add_srb_rlc_entity(ue, srb_id, rlc_am);

    LOG_D(RLC, "%s:%d:%s: added SRB %d to UE RNTI %x\n", __FILE__, __LINE__, __FUNCTION__, srb_id, rnti);
  }
  rlc_manager_unlock(rlc_ue_manager);
}

static void add_drb_am(int rnti, int module_id, struct LTE_DRB_ToAddMod *s)
{
  rlc_entity_t            *rlc_am;
  rlc_ue_t                *ue;

  struct LTE_RLC_Config *r = s->rlc_Config;
  struct LTE_LogicalChannelConfig *l = s->logicalChannelConfig;
  int drb_id = s->drb_Identity;
  int channel_id = *s->logicalChannelIdentity;
  int logical_channel_group;

  int t_reordering;
  int t_status_prohibit;
  int t_poll_retransmit;
  int poll_pdu;
  int poll_byte;
  int max_retx_threshold;

  if (!(drb_id >= 1 && drb_id <= 5)) {
    LOG_E(RLC, "%s:%d:%s: fatal, bad srb id %d\n",
          __FILE__, __LINE__, __FUNCTION__, drb_id);
    exit(1);
  }

  if (channel_id != drb_id + 2) {
    LOG_E(RLC, "%s:%d:%s: todo, remove this limitation\n",
          __FILE__, __LINE__, __FUNCTION__);
    exit(1);
  }

  logical_channel_group = *l->ul_SpecificParameters->logicalChannelGroup;

  /* TODO: accept other values? */
  if (logical_channel_group != 1) {
    LOG_E(RLC, "%s:%d:%s: fatal error\n", __FILE__, __LINE__, __FUNCTION__);
    exit(1);
  }

  switch (r->present) {
  case LTE_RLC_Config_PR_am: {
    struct LTE_RLC_Config__am *am;
    am = &r->choice.am;
    t_reordering       = decode_t_reordering(am->dl_AM_RLC.t_Reordering);
    t_status_prohibit  = decode_t_status_prohibit(am->dl_AM_RLC.t_StatusProhibit);
    t_poll_retransmit  = decode_t_poll_retransmit(am->ul_AM_RLC.t_PollRetransmit);
    poll_pdu           = decode_poll_pdu(am->ul_AM_RLC.pollPDU);
    poll_byte          = decode_poll_byte(am->ul_AM_RLC.pollByte);
    max_retx_threshold = decode_max_retx_threshold(am->ul_AM_RLC.maxRetxThreshold);
    break;
  }
  default:
    LOG_E(RLC, "%s:%d:%s: fatal error\n", __FILE__, __LINE__, __FUNCTION__);
    exit(1);
  }

  rlc_manager_lock(rlc_ue_manager);
  ue = rlc_manager_get_ue(rlc_ue_manager, rnti);
  ue->module_id = module_id;
  if (ue->drb[drb_id-1] != NULL) {
    LOG_D(RLC, "%s:%d:%s: warning DRB %d already exist for ue %d, do nothing\n",
          __FILE__, __LINE__, __FUNCTION__, drb_id, rnti);
  } else {
    rlc_am = new_rlc_entity_am(1000000,
                               1000000,
                               deliver_sdu, ue,
                               successful_delivery, ue,
                               max_retx_reached, ue,
                               t_reordering, t_status_prohibit,
                               t_poll_retransmit,
                               poll_pdu, poll_byte, max_retx_threshold);
    rlc_ue_add_drb_rlc_entity(ue, drb_id, rlc_am);

    LOG_D(RLC, "%s:%d:%s: added DRB %d to UE RNTI %x\n", __FILE__, __LINE__, __FUNCTION__, drb_id, rnti);
  }
  rlc_manager_unlock(rlc_ue_manager);
}

static void add_drb_um(int rnti, int module_id, struct LTE_DRB_ToAddMod *s)
{
  rlc_entity_t            *rlc_um;
  rlc_ue_t                *ue;

  struct LTE_RLC_Config *r = s->rlc_Config;
  struct LTE_LogicalChannelConfig *l = s->logicalChannelConfig;
  int drb_id = s->drb_Identity;
  int channel_id = *s->logicalChannelIdentity;
  int logical_channel_group;

  int t_reordering;
  int sn_field_length;

  if (!(drb_id >= 1 && drb_id <= 5)) {
    LOG_E(RLC, "%s:%d:%s: fatal, bad srb id %d\n",
          __FILE__, __LINE__, __FUNCTION__, drb_id);
    exit(1);
  }

  if (channel_id != drb_id + 2) {
    LOG_E(RLC, "%s:%d:%s: todo, remove this limitation\n",
          __FILE__, __LINE__, __FUNCTION__);
    exit(1);
  }

  logical_channel_group = *l->ul_SpecificParameters->logicalChannelGroup;

  /* TODO: accept other values? */
  if (logical_channel_group != 1) {
    LOG_E(RLC, "%s:%d:%s: fatal error\n", __FILE__, __LINE__, __FUNCTION__);
    exit(1);
  }

  switch (r->present) {
  case LTE_RLC_Config_PR_um_Bi_Directional: {
    struct LTE_RLC_Config__um_Bi_Directional *um;
    um = &r->choice.um_Bi_Directional;
    t_reordering    = decode_t_reordering(um->dl_UM_RLC.t_Reordering);
    if (um->dl_UM_RLC.sn_FieldLength != um->ul_UM_RLC.sn_FieldLength) {
      LOG_E(RLC, "%s:%d:%s: fatal\n", __FILE__, __LINE__, __FUNCTION__);
      exit(1);
    }
    sn_field_length = decode_sn_field_length(um->dl_UM_RLC.sn_FieldLength);
    break;
  }
  default:
    LOG_E(RLC, "%s:%d:%s: fatal error\n", __FILE__, __LINE__, __FUNCTION__);
    exit(1);
  }

  rlc_manager_lock(rlc_ue_manager);
  ue = rlc_manager_get_ue(rlc_ue_manager, rnti);
  ue->module_id = module_id;
  if (ue->drb[drb_id-1] != NULL) {
    LOG_D(RLC, "%s:%d:%s: warning DRB %d already exist for ue %d, do nothing\n",
          __FILE__, __LINE__, __FUNCTION__, drb_id, rnti);
  } else {
    rlc_um = new_rlc_entity_um(1000000,
                               1000000,
                               deliver_sdu, ue,
                               t_reordering,
                               sn_field_length);
    rlc_ue_add_drb_rlc_entity(ue, drb_id, rlc_um);

    LOG_D(RLC, "%s:%d:%s: added DRB %d to UE RNTI %x\n", __FILE__, __LINE__, __FUNCTION__, drb_id, rnti);
  }
  rlc_manager_unlock(rlc_ue_manager);
}

static void add_drb(int rnti, int module_id, struct LTE_DRB_ToAddMod *s)
{
  switch (s->rlc_Config->present) {
  case LTE_RLC_Config_PR_am:
    add_drb_am(rnti, module_id, s);
    break;
  case LTE_RLC_Config_PR_um_Bi_Directional:
    add_drb_um(rnti, module_id, s);
    break;
  default:
    LOG_E(RLC, "%s:%d:%s: fatal: unhandled DRB type\n",
          __FILE__, __LINE__, __FUNCTION__);
    exit(1);
  }
}

rlc_op_status_t rrc_rlc_config_asn1_req (const protocol_ctxt_t   * const ctxt_pP,
    const LTE_SRB_ToAddModList_t   * const srb2add_listP,
    const LTE_DRB_ToAddModList_t   * const drb2add_listP,
    const LTE_DRB_ToReleaseList_t  * const drb2release_listP,
    const LTE_PMCH_InfoList_r9_t * const pmch_InfoList_r9_pP,
    const uint32_t sourceL2Id,
    const uint32_t destinationL2Id
                                        )
{
  int rnti = ctxt_pP->rnti;
  int module_id = ctxt_pP->module_id;
  int i;
  int j;

  if (ctxt_pP->enb_flag == 1 &&
      (ctxt_pP->module_id != 0 || ctxt_pP->instance != 0)) {
    LOG_E(RLC, "%s: module_id != 0 or instance != 0 not handled for eNB\n",
          __FUNCTION__);
    exit(1);
  }

  if (0 /*||
      ctxt_pP->instance != 0 || ctxt_pP->eNB_index != 0 ||
      ctxt_pP->configured != 1 || ctxt_pP->brOption != 0 */) {
    LOG_E(RLC, "%s: ctxt_pP not handled (%d %d %ld %d %d %d)\n", __FUNCTION__,
          ctxt_pP->enb_flag , ctxt_pP->module_id, ctxt_pP->instance,
          ctxt_pP->eNB_index, ctxt_pP->configured, ctxt_pP->brOption);
    exit(1);
  }

  if (pmch_InfoList_r9_pP != NULL) {
    int                           mbms_rnti = 0xfffd;
    LTE_MBMS_SessionInfoList_r9_t *mbms_SessionInfoList_r9_p = NULL;
    LTE_MBMS_SessionInfo_r9_t     *MBMS_SessionInfo_p        = NULL;
    mbms_session_id_t             mbms_session_id;
    mbms_service_id_t             mbms_service_id;
    rb_id_t                       drb_id = 0;
    logical_chan_id_t             lc_id  = 0;
    //LTE_DRB_Identity_t     drb_id          = 0;
    //LTE_DRB_Identity_t*    pdrb_id         = NULL;

    for (i=0; i<pmch_InfoList_r9_pP->list.count; i++) {
      mbms_SessionInfoList_r9_p = &(pmch_InfoList_r9_pP->list.array[i]->mbms_SessionInfoList_r9);

      for (j=0; j<mbms_SessionInfoList_r9_p->list.count; j++) {
        MBMS_SessionInfo_p = mbms_SessionInfoList_r9_p->list.array[j];
        if (0/*MBMS_SessionInfo_p->sessionId_r9*/)
          mbms_session_id  = MBMS_SessionInfo_p->sessionId_r9->buf[0];
        else
          mbms_session_id  = MBMS_SessionInfo_p->logicalChannelIdentity_r9;
        lc_id              = mbms_session_id;
        mbms_service_id    = MBMS_SessionInfo_p->tmgi_r9.serviceId_r9.buf[2]; //serviceId is 3-octet string
//        mbms_service_id    = j;

#if 0
        /* TODO: check if this code should stay there
         * as it is both enb and ue cases do the same thing
         */
        // can set the mch_id = i
        if (ctxt_pP->enb_flag) {
          drb_id =  (mbms_service_id * LTE_maxSessionPerPMCH ) + mbms_session_id;//+ (LTE_maxDRB + 3) * MAX_MOBILES_PER_ENB; // 1
        } else {
          drb_id =  (mbms_service_id * LTE_maxSessionPerPMCH ) + mbms_session_id; // + (LTE_maxDRB + 3); // 15
        }
#endif

        drb_id =  (mbms_service_id * LTE_maxSessionPerPMCH ) + mbms_session_id;

        LOG_I(RLC, PROTOCOL_CTXT_FMT" CONFIG REQ MBMS ASN1 LC ID %u RB ID %u SESSION ID %u SERVICE ID %u, mbms_rnti %x\n",
              PROTOCOL_CTXT_ARGS(ctxt_pP),
              lc_id,
              (int)drb_id,
              mbms_session_id,
              mbms_service_id,
              mbms_rnti
             );

        rlc_entity_t            *rlc_um;
        rlc_ue_t                *ue;

        //drb_id = rb_id;

        rlc_manager_lock(rlc_ue_manager);
        ue = rlc_manager_get_ue(rlc_ue_manager, mbms_rnti);
        if (ue->drb[drb_id-1] != NULL) {
          LOG_D(RLC, "%s:%d:%s: warning DRB %d already exist for ue %d, do nothing\n",
              __FILE__, __LINE__, __FUNCTION__, (int)drb_id, mbms_rnti);
        } else {
          rlc_um = new_rlc_entity_um(1000000,
                                     1000000,
                                     deliver_sdu, ue,
                                     0,//LTE_T_Reordering_ms0,//t_reordering,
                                     5//LTE_SN_FieldLength_size5//sn_field_length
                                    );
          rlc_ue_add_drb_rlc_entity(ue, drb_id, rlc_um);

          LOG_D(RLC, "%s:%d:%s: added DRB %d to UE RNTI %x\n", __FILE__, __LINE__, __FUNCTION__, (int)drb_id, mbms_rnti);
        }
        rlc_manager_unlock(rlc_ue_manager);

      }
    }

  }

  if (drb2release_listP != NULL) {
    LOG_E(RLC, "%s:%d:%s: TODO\n", __FILE__, __LINE__, __FUNCTION__);
    exit(1);
  }

  if (srb2add_listP != NULL) {
    for (i = 0; i < srb2add_listP->list.count; i++) {
      add_srb(rnti, module_id, srb2add_listP->list.array[i]);
    }
  }

  if (drb2add_listP != NULL) {
    for (i = 0; i < drb2add_listP->list.count; i++) {
      add_drb(rnti, module_id, drb2add_listP->list.array[i]);
    }
  }

  return RLC_OP_STATUS_OK;
}

rlc_op_status_t rrc_rlc_config_req   (
  const protocol_ctxt_t* const ctxt_pP,
  const srb_flag_t      srb_flagP,
  const MBMS_flag_t     mbms_flagP,
  const config_action_t actionP,
  const rb_id_t         rb_idP,
  const rlc_info_t      rlc_infoP)
{
  rlc_ue_t *ue;
  int      i;

  if (mbms_flagP) {
    LOG_E(RLC, "%s:%d:%s: todo (mbms not supported)\n", __FILE__, __LINE__, __FUNCTION__);
    exit(1);
  }
  if (actionP != CONFIG_ACTION_REMOVE) {
    LOG_E(RLC, "%s:%d:%s: todo (only CONFIG_ACTION_REMOVE supported)\n", __FILE__, __LINE__, __FUNCTION__);
    exit(1);
  }
  if (ctxt_pP->module_id) {
    LOG_E(RLC, "%s:%d:%s: todo (only module_id 0 supported)\n", __FILE__, __LINE__, __FUNCTION__);
    exit(1);
  }
  if ((srb_flagP && !(rb_idP >= 1 && rb_idP <= 2)) ||
      (!srb_flagP && !(rb_idP >= 1 && rb_idP <= 5))) {
    LOG_E(RLC, "%s:%d:%s: bad rb_id (%d) (is_srb %d)\n", __FILE__, __LINE__, __FUNCTION__, (int)rb_idP, srb_flagP);
    exit(1);
  }
  rlc_manager_lock(rlc_ue_manager);
  LOG_D(RLC, "%s:%d:%s: remove rb %d (is_srb %d) for UE %d\n", __FILE__, __LINE__, __FUNCTION__, (int)rb_idP, srb_flagP, ctxt_pP->rnti);
  ue = rlc_manager_get_ue(rlc_ue_manager, ctxt_pP->rnti);
  if (srb_flagP) {
    if (ue->srb[rb_idP-1] != NULL) {
      ue->srb[rb_idP-1]->delete(ue->srb[rb_idP-1]);
      ue->srb[rb_idP-1] = NULL;
    } else
      LOG_W(RLC, "removing non allocated SRB %d, do nothing\n", (int)rb_idP);
  } else {
    if (ue->drb[rb_idP-1] != NULL) {
      ue->drb[rb_idP-1]->delete(ue->drb[rb_idP-1]);
      ue->drb[rb_idP-1] = NULL;
    } else
      LOG_W(RLC, "removing non allocated DRB %d, do nothing\n", (int)rb_idP);
  }
  /* remove UE if it has no more RB configured */
  for (i = 0; i < 2; i++)
    if (ue->srb[i] != NULL)
      break;
  if (i == 2) {
    for (i = 0; i < 5; i++)
      if (ue->drb[i] != NULL)
        break;
    if (i == 5)
      rlc_manager_remove_ue(rlc_ue_manager, ctxt_pP->rnti);
  }
  rlc_manager_unlock(rlc_ue_manager);
  return RLC_OP_STATUS_OK;
}

void rrc_rlc_register_rrc (rrc_data_ind_cb_t rrc_data_indP, rrc_data_conf_cb_t rrc_data_confP)
{
  /* nothing to do */
}

rlc_op_status_t rrc_rlc_remove_ue (const protocol_ctxt_t* const x)
{
  LOG_D(RLC, "%s:%d:%s: remove UE %d\n", __FILE__, __LINE__, __FUNCTION__, x->rnti);
  rlc_manager_lock(rlc_ue_manager);
  rlc_manager_remove_ue(rlc_ue_manager, x->rnti);
  rlc_manager_unlock(rlc_ue_manager);

  return RLC_OP_STATUS_OK;
}

void rlc_tick(int frame, int subframe)
{
  int expected_next_frame;
  int expected_next_subframe;

  if (frame != rlc_current_time_last_frame ||
      subframe != rlc_current_time_last_subframe) {
    /* warn if discontinuity in ticks */
    expected_next_subframe = (rlc_current_time_last_subframe + 1) % 10;
    if (expected_next_subframe == 0)
      expected_next_frame = (rlc_current_time_last_frame + 1) % 1024;
    else
      expected_next_frame = rlc_current_time_last_frame;
    if (expected_next_frame != frame || expected_next_subframe != subframe)
      LOG_W(RLC, "rlc_tick: discontinuity (expected %d.%d, got %d.%d)\n",
            expected_next_frame, expected_next_subframe,
            frame, subframe);

    rlc_current_time++;
    rlc_current_time_last_frame = frame;
    rlc_current_time_last_subframe = subframe;
  }
}

void du_rlc_data_req(const protocol_ctxt_t *const ctxt_pP,
                     const srb_flag_t   srb_flagP,
                     const MBMS_flag_t  MBMS_flagP,
                     const rb_id_t      rb_idP,
                     const mui_t        muiP,
                     confirm_t    confirmP,
                     sdu_size_t   sdu_sizeP,
                     mem_block_t *sdu_pP){

}