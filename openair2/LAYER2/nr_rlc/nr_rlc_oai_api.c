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

/* from nr rlc module */
#include "asn1_utils.h"
#include "nr_rlc_ue_manager.h"
#include "nr_rlc_entity.h"
#include "nr_rlc_oai_api.h"
#include "NR_RLC-BearerConfig.h"
#include "NR_DRB-ToAddMod.h"
#include "NR_DRB-ToAddModList.h"
#include "NR_SRB-ToAddModList.h"
#include "NR_DRB-ToReleaseList.h"
#include "NR_CellGroupConfig.h"
#include "NR_RLC-Config.h"
#include "common/ran_context.h"
#include "NR_UL-CCCH-Message.h"

#include "openair2/F1AP/f1ap_du_rrc_message_transfer.h"

#include "openair2/LAYER2/PROTO_AGENT/proto_agent.h"

extern RAN_CONTEXT_t RC;

#include <stdint.h>

#include <executables/softmodem-common.h>

static nr_rlc_ue_manager_t *nr_rlc_ue_manager;

/* TODO: handle time a bit more properly */
static uint64_t nr_rlc_current_time;
static int      nr_rlc_current_time_last_frame;
static int      nr_rlc_current_time_last_subframe;


void nr_rlc_bearer_init(NR_RLC_BearerConfig_t *RLC_BearerConfig, NR_RLC_BearerConfig__servedRadioBearer_PR rb_type){

  RLC_BearerConfig->servedRadioBearer                      = calloc(1, sizeof(*RLC_BearerConfig->servedRadioBearer));
  RLC_BearerConfig->reestablishRLC                         = calloc(1, sizeof(*RLC_BearerConfig->reestablishRLC));
  RLC_BearerConfig->rlc_Config                             = calloc(1, sizeof(*RLC_BearerConfig->rlc_Config));
  RLC_BearerConfig->mac_LogicalChannelConfig               = calloc(1, sizeof(*RLC_BearerConfig->mac_LogicalChannelConfig));

  *RLC_BearerConfig->reestablishRLC                        = NR_RLC_BearerConfig__reestablishRLC_true;
  if(rb_type == NR_RLC_BearerConfig__servedRadioBearer_PR_drb_Identity){
    RLC_BearerConfig->logicalChannelIdentity                 = 4;
    RLC_BearerConfig->servedRadioBearer->present             = NR_RLC_BearerConfig__servedRadioBearer_PR_drb_Identity;
    RLC_BearerConfig->servedRadioBearer->choice.drb_Identity = 1;
  }
  else{
    RLC_BearerConfig->logicalChannelIdentity                 = 1;
    RLC_BearerConfig->servedRadioBearer->present             = NR_RLC_BearerConfig__servedRadioBearer_PR_srb_Identity;
    RLC_BearerConfig->servedRadioBearer->choice.srb_Identity = 1;
  }

}

void nr_rlc_bearer_init_ul_spec(struct NR_LogicalChannelConfig *mac_LogicalChannelConfig){

  mac_LogicalChannelConfig->ul_SpecificParameters                              = calloc(1, sizeof(*mac_LogicalChannelConfig->ul_SpecificParameters));
  mac_LogicalChannelConfig->ul_SpecificParameters->priority                    = 1;
  mac_LogicalChannelConfig->ul_SpecificParameters->prioritisedBitRate          = NR_LogicalChannelConfig__ul_SpecificParameters__prioritisedBitRate_infinity;
  mac_LogicalChannelConfig->ul_SpecificParameters->bucketSizeDuration          = NR_LogicalChannelConfig__ul_SpecificParameters__bucketSizeDuration_ms50;
  mac_LogicalChannelConfig->ul_SpecificParameters->allowedServingCells         = NULL;
  mac_LogicalChannelConfig->ul_SpecificParameters->allowedSCS_List             = NULL;
  mac_LogicalChannelConfig->ul_SpecificParameters->maxPUSCH_Duration           = NULL;
  mac_LogicalChannelConfig->ul_SpecificParameters->configuredGrantType1Allowed = NULL;

  mac_LogicalChannelConfig->ul_SpecificParameters->logicalChannelGroup                = calloc(1,sizeof(*mac_LogicalChannelConfig->ul_SpecificParameters->logicalChannelGroup));
  *mac_LogicalChannelConfig->ul_SpecificParameters->logicalChannelGroup               = 1;
  mac_LogicalChannelConfig->ul_SpecificParameters->schedulingRequestID                = calloc(1,sizeof(*mac_LogicalChannelConfig->ul_SpecificParameters->schedulingRequestID));
  *mac_LogicalChannelConfig->ul_SpecificParameters->schedulingRequestID               = 0;
  mac_LogicalChannelConfig->ul_SpecificParameters->logicalChannelSR_Mask              = false;
  mac_LogicalChannelConfig->ul_SpecificParameters->logicalChannelSR_DelayTimerApplied = false;
  mac_LogicalChannelConfig->ul_SpecificParameters->bitRateQueryProhibitTimer          = NULL;

}

void nr_drb_config(struct NR_RLC_Config *rlc_Config, NR_RLC_Config_PR rlc_config_pr){

  switch (rlc_config_pr){
    case NR_RLC_Config_PR_um_Bi_Directional:
      // RLC UM Bi-directional Bearer configuration
      rlc_Config->choice.um_Bi_Directional                            = calloc(1, sizeof(*rlc_Config->choice.um_Bi_Directional));
      rlc_Config->choice.um_Bi_Directional->ul_UM_RLC.sn_FieldLength  = calloc(1, sizeof(*rlc_Config->choice.um_Bi_Directional->ul_UM_RLC.sn_FieldLength));
      *rlc_Config->choice.um_Bi_Directional->ul_UM_RLC.sn_FieldLength = NR_SN_FieldLengthUM_size12;
      rlc_Config->choice.um_Bi_Directional->dl_UM_RLC.sn_FieldLength  = calloc(1, sizeof(*rlc_Config->choice.um_Bi_Directional->dl_UM_RLC.sn_FieldLength));
      *rlc_Config->choice.um_Bi_Directional->dl_UM_RLC.sn_FieldLength = NR_SN_FieldLengthUM_size12;
      rlc_Config->choice.um_Bi_Directional->dl_UM_RLC.t_Reassembly    = NR_T_Reassembly_ms15;
      break;
    case NR_RLC_Config_PR_am:
      // RLC AM Bearer configuration
      rlc_Config->choice.am                             = calloc(1, sizeof(*rlc_Config->choice.am));
      rlc_Config->choice.am->ul_AM_RLC.sn_FieldLength   = calloc(1, sizeof(*rlc_Config->choice.am->ul_AM_RLC.sn_FieldLength));
      *rlc_Config->choice.am->ul_AM_RLC.sn_FieldLength  = NR_SN_FieldLengthAM_size18;
      rlc_Config->choice.am->ul_AM_RLC.t_PollRetransmit = NR_T_PollRetransmit_ms45;
      rlc_Config->choice.am->ul_AM_RLC.pollPDU          = NR_PollPDU_p64;
      rlc_Config->choice.am->ul_AM_RLC.pollByte         = NR_PollByte_kB500;
      rlc_Config->choice.am->ul_AM_RLC.maxRetxThreshold = NR_UL_AM_RLC__maxRetxThreshold_t32;
      rlc_Config->choice.am->dl_AM_RLC.sn_FieldLength   = calloc(1, sizeof(*rlc_Config->choice.am->dl_AM_RLC.sn_FieldLength));
      *rlc_Config->choice.am->dl_AM_RLC.sn_FieldLength  = NR_SN_FieldLengthAM_size18;
      rlc_Config->choice.am->dl_AM_RLC.t_Reassembly     = NR_T_Reassembly_ms15;
      rlc_Config->choice.am->dl_AM_RLC.t_StatusProhibit = NR_T_StatusProhibit_ms15;
      break;
    default:
      LOG_E (RLC, "Error in %s: RLC config type %d is not handled\n", __FUNCTION__, rlc_config_pr);
      break;
    }

  rlc_Config->present = rlc_config_pr;

}

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
  nr_rlc_ue_t *ue;
  nr_rlc_entity_t *rb;

  if (module_idP != 0 || eNB_index != 0 || /*enb_flagP != 1 ||*/ MBMS_flagP != 0) {
    LOG_E(RLC, "%s:%d:%s: fatal\n", __FILE__, __LINE__, __FUNCTION__);
    exit(1);
  }

  if (enb_flagP)
    T(T_ENB_RLC_MAC_UL, T_INT(module_idP), T_INT(rntiP),
      T_INT(channel_idP), T_INT(tb_sizeP));

  nr_rlc_manager_lock(nr_rlc_ue_manager);
  ue = nr_rlc_manager_get_ue(nr_rlc_ue_manager, rntiP);

  if(ue == NULL)
	  LOG_I(RLC, "RLC instance for the given UE was not found \n");

  switch (channel_idP) {
  case 1 ... 3: rb = ue->srb[channel_idP - 1]; break;
  case 4 ... 8: rb = ue->drb[channel_idP - 4]; break;
  default:      rb = NULL;                     break;
  }

  if (rb != NULL) {
	LOG_D(RLC, "RB found! (channel ID %d) \n", channel_idP);
    rb->set_time(rb, nr_rlc_current_time);
    rb->recv_pdu(rb, buffer_pP, tb_sizeP);
  } else {
    LOG_E(RLC, "%s:%d:%s: fatal: no RB found (channel ID %d)\n",
          __FILE__, __LINE__, __FUNCTION__, channel_idP);
    // exit(1);
  }

  nr_rlc_manager_unlock(nr_rlc_ue_manager);
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
  nr_rlc_ue_t *ue;
  nr_rlc_entity_t *rb;
  int maxsize;

  nr_rlc_manager_lock(nr_rlc_ue_manager);
  ue = nr_rlc_manager_get_ue(nr_rlc_ue_manager, rntiP);

  switch (channel_idP) {
  case 1 ... 3: rb = ue->srb[channel_idP - 1]; break;
  case 4 ... 8: rb = ue->drb[channel_idP - 4]; break;
  default:
  rb = NULL;
  LOG_E(RLC, "In %s:%d:%s: data request for unknown RB with LCID 0x%02x !\n", __FILE__, __LINE__, __FUNCTION__, channel_idP);
  break;
  }

  if (rb != NULL) {
    rb->set_time(rb, nr_rlc_current_time);
    maxsize = tb_sizeP;
    ret = rb->generate_pdu(rb, buffer_pP, maxsize);
  } else {
    ret = 0;
  }

  nr_rlc_manager_unlock(nr_rlc_ue_manager);

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
  nr_rlc_ue_t *ue;
  mac_rlc_status_resp_t ret;
  nr_rlc_entity_t *rb;

  nr_rlc_manager_lock(nr_rlc_ue_manager);
  ue = nr_rlc_manager_get_ue(nr_rlc_ue_manager, rntiP);

  switch (channel_idP) {
  case 1 ... 3: rb = ue->srb[channel_idP - 1]; break;
  case 4 ... 8: rb = ue->drb[channel_idP - 4]; break;
  default:      rb = NULL;                     break;
  }

  if (rb != NULL) {
    nr_rlc_entity_buffer_status_t buf_stat;
    rb->set_time(rb, nr_rlc_current_time);
    /* 38.321 deals with BSR values up to 81338368 bytes, after what it
     * reports '> 81338368' (table 6.1.3.1-2). Passing 100000000 is thus
     * more than enough.
     */
    // Fix me: temproary reduction meanwhile cpu cost of this computation is optimized
    buf_stat = rb->buffer_status(rb, 1000*1000);
    ret.bytes_in_buffer = buf_stat.status_size
                        + buf_stat.retx_size
                        + buf_stat.tx_size;
  } else {
    if (!(frameP%128)) //to supress this warning message
      LOG_W(RLC, "[%s] Radio Bearer (channel ID %d) is NULL for UE with rntiP %x\n", __FUNCTION__, channel_idP, rntiP);
    ret.bytes_in_buffer = 0;
  }

  nr_rlc_manager_unlock(nr_rlc_ue_manager);

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
  nr_rlc_ue_t *ue;
  rlc_buffer_occupancy_t ret;
  nr_rlc_entity_t *rb;

  if (enb_flagP) {
    LOG_E(RLC, "Tx mac_rlc_get_buffer_occupancy_ind function is not implemented for eNB LcId=%u\n", channel_idP);
    exit(1);
  }

  /* TODO: handle time a bit more properly */
  if (nr_rlc_current_time_last_frame != frameP ||
      nr_rlc_current_time_last_subframe != subframeP) {
    nr_rlc_current_time++;
    nr_rlc_current_time_last_frame = frameP;
    nr_rlc_current_time_last_subframe = subframeP;
  }

  nr_rlc_manager_lock(nr_rlc_ue_manager);
  ue = nr_rlc_manager_get_ue(nr_rlc_ue_manager, rntiP);

  switch (channel_idP) {
  case 1 ... 3: rb = ue->srb[channel_idP - 1]; break;
  case 4 ... 8: rb = ue->drb[channel_idP - 4]; break;
  default:      rb = NULL;                     break;
  }

  if (rb != NULL) {
    nr_rlc_entity_buffer_status_t buf_stat;
    rb->set_time(rb, nr_rlc_current_time);
    /* 38.321 deals with BSR values up to 81338368 bytes, after what it
     * reports '> 81338368' (table 6.1.3.1-2). Passing 100000000 is thus
     * more than enough.
     */
    // Fixme : Laurent reduced size for CPU saving
    // Fix me: temproary reduction meanwhile cpu cost of this computation is optimized
    buf_stat = rb->buffer_status(rb, 1000*1000);
    ret = buf_stat.status_size
        + buf_stat.retx_size
        + buf_stat.tx_size;
  } else {
    ret = 0;
  }

  nr_rlc_manager_unlock(nr_rlc_ue_manager);

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
  nr_rlc_ue_t *ue;
  nr_rlc_entity_t *rb;

  LOG_D(RLC, "%s rnti %d srb_flag %d rb_id %ld mui %d confirm %d sdu_size %d MBMS_flag %d\n",
        __FUNCTION__, rnti, srb_flagP, rb_idP, muiP, confirmP, sdu_sizeP,
        MBMS_flagP);

  if (ctxt_pP->enb_flag)
    T(T_ENB_RLC_DL, T_INT(ctxt_pP->module_id),
      T_INT(ctxt_pP->rnti), T_INT(rb_idP), T_INT(sdu_sizeP));

  nr_rlc_manager_lock(nr_rlc_ue_manager);
  ue = nr_rlc_manager_get_ue(nr_rlc_ue_manager, rnti);

  rb = NULL;

  if (srb_flagP) {
    if (rb_idP >= 1 && rb_idP <= 2)
      rb = ue->srb[rb_idP - 1];
  } else {
    if (rb_idP >= 1 && rb_idP <= 5)
      rb = ue->drb[rb_idP - 1];
  }

  if (rb != NULL) {
    rb->set_time(rb, nr_rlc_current_time);
    rb->recv_sdu(rb, (char *)sdu_pP->data, sdu_sizeP, muiP);
  } else {
    LOG_E(RLC, "%s:%d:%s: fatal: SDU sent to unknown RB\n", __FILE__, __LINE__, __FUNCTION__);
    exit(1);
  }

  nr_rlc_manager_unlock(nr_rlc_ue_manager);

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

  nr_rlc_ue_manager = new_nr_rlc_ue_manager(enb_flag);

  if (pthread_mutex_unlock(&lock)) abort();

  return 0;
}

void rlc_util_print_hex_octets(comp_name_t componentP, unsigned char *dataP, const signed long sizeP)
{
}

static void deliver_sdu(void *_ue, nr_rlc_entity_t *entity, char *buf, int size)
{
  nr_rlc_ue_t *ue = _ue;
  int is_srb;
  int rb_id;
  protocol_ctxt_t ctx;
  mem_block_t *memblock;
  int i;
  int is_enb;

  /* is it SRB? */
  for (i = 0; i < sizeofArray(ue->srb); i++) {
    if (entity == ue->srb[i]) {
      is_srb = 1;
      rb_id = i+1;
      goto rb_found;
    }
  }

  /* maybe DRB? */
  for (i = 0; i < sizeofArray(ue->drb) ; i++) {
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
  ctx.instance = 0;
  ctx.frame = 0;
  ctx.subframe = 0;
  ctx.eNB_index = 0;
  ctx.configured = 1;
  ctx.brOption = 0;

  /* used fields? */
  ctx.module_id = 0;
  ctx.rnti = ue->rnti;

  is_enb = nr_rlc_manager_get_enb_flag(nr_rlc_ue_manager);
  ctx.enb_flag = is_enb;

  if (is_enb) {
    T(T_ENB_RLC_UL,
      T_INT(0 /*ctxt_pP->module_id*/),
      T_INT(ue->rnti), T_INT(rb_id), T_INT(size));

    const ngran_node_t type = RC.nrrrc[0 /*ctxt_pP->module_id*/]->node_type;
    AssertFatal(type != ngran_eNB_CU && type != ngran_ng_eNB_CU && type != ngran_gNB_CU,
                "Can't be CU, bad node type %d\n", type);

    // if (NODE_IS_DU(type) && is_srb == 0) {
    //   LOG_D(RLC, "call proto_agent_send_pdcp_data_ind() \n");
    //   proto_agent_send_pdcp_data_ind(&ctx, is_srb, 0, rb_id, size, memblock);
    //   return;
    // }

    if (NODE_IS_DU(type) && is_srb == 1) {
      MessageDef *msg;
      msg = itti_alloc_new_message(TASK_RLC_ENB, 0, F1AP_UL_RRC_MESSAGE);
      uint8_t *message_buffer = itti_malloc (TASK_RLC_ENB, TASK_DU_F1, size);
      memcpy (message_buffer, buf, size);
      F1AP_UL_RRC_MESSAGE(msg).rnti = ue->rnti;
      F1AP_UL_RRC_MESSAGE(msg).srb_id = rb_id;
      F1AP_UL_RRC_MESSAGE(msg).rrc_container = message_buffer;
      F1AP_UL_RRC_MESSAGE(msg).rrc_container_length = size;
      itti_send_msg_to_task(TASK_DU_F1, ENB_MODULE_ID_TO_INSTANCE(0 /*ctxt_pP->module_id*/), msg);
      return;
    }
    else if(NODE_IS_DU(type) && is_srb == 0) {
      LOG_W(RLC, "Received uplink user-plane traffic at RLC-DU to be sent to the CU, but F1-U implementation is not in place yet \n");
      return;
    }
  }

  if (!pdcp_data_ind(&ctx, is_srb, 0, rb_id, size, memblock)) {
    LOG_E(RLC, "%s:%d:%s: ERROR: pdcp_data_ind failed\n", __FILE__, __LINE__, __FUNCTION__);
    /* what to do in case of failure? for the moment: nothing */
  }
}

static void successful_delivery(void *_ue, nr_rlc_entity_t *entity, int sdu_id)
{
  nr_rlc_ue_t *ue = _ue;
  int i;
  int is_srb;
  int rb_id;
#if 0
  MessageDef *msg;
#endif
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

  is_enb = nr_rlc_manager_get_enb_flag(nr_rlc_ue_manager);
  if (!is_enb)
    return;

#if 0
  msg = itti_alloc_new_message(TASK_RLC_ENB, RLC_SDU_INDICATION);
  RLC_SDU_INDICATION(msg).rnti          = ue->rnti;
  RLC_SDU_INDICATION(msg).is_successful = 1;
  RLC_SDU_INDICATION(msg).srb_id        = rb_id;
  RLC_SDU_INDICATION(msg).message_id    = sdu_id;
  /* TODO: accept more than 1 instance? here we send to instance id 0 */
  itti_send_msg_to_task(TASK_RRC_ENB, 0, msg);
#endif
}

static void max_retx_reached(void *_ue, nr_rlc_entity_t *entity)
{
  nr_rlc_ue_t *ue = _ue;
  int i;
  int is_srb;
  int rb_id;
#if 0
  MessageDef *msg;
#endif
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
  LOG_E(RLC, "max RETX reached on %s %d\n",
        is_srb ? "SRB" : "DRB",
        rb_id);

  /* TODO: do something for DRBs? */
  if (is_srb == 0)
    return;

  is_enb = nr_rlc_manager_get_enb_flag(nr_rlc_ue_manager);
  if (!is_enb)
    return;

#if 0
  msg = itti_alloc_new_message(TASK_RLC_ENB, RLC_SDU_INDICATION);
  RLC_SDU_INDICATION(msg).rnti          = ue->rnti;
  RLC_SDU_INDICATION(msg).is_successful = 0;
  RLC_SDU_INDICATION(msg).srb_id        = rb_id;
  RLC_SDU_INDICATION(msg).message_id    = -1;
  /* TODO: accept more than 1 instance? here we send to instance id 0 */
  itti_send_msg_to_task(TASK_RRC_ENB, 0, msg);
#endif
}

static void add_rlc_srb(int rnti, struct NR_SRB_ToAddMod *s, NR_RLC_BearerConfig_t *rlc_BearerConfig)
{
  nr_rlc_entity_t            *nr_rlc_am;
  nr_rlc_ue_t                *ue;

  struct NR_RLC_Config *r = rlc_BearerConfig->rlc_Config;
  struct NR_LogicalChannelConfig *l = rlc_BearerConfig->mac_LogicalChannelConfig;
  int srb_id = s->srb_Identity;
  int channel_id = rlc_BearerConfig->logicalChannelIdentity;
  int logical_channel_group;

  int t_status_prohibit;
  int t_poll_retransmit;
  int poll_pdu;
  int poll_byte;
  int max_retx_threshold;
  int t_reassembly;
  int sn_field_length;

  LOG_D(RLC,"Trying to add SRB %d\n",srb_id);
  if (srb_id != 1 && srb_id != 2) {
    LOG_E(RLC, "%s:%d:%s: fatal, bad srb id %d\n",
        __FILE__, __LINE__, __FUNCTION__, srb_id);
    exit(1);
  }

  if (channel_id != srb_id) {
    LOG_E(RLC, "%s:%d:%s: todo, remove this limitation\n",
          __FILE__, __LINE__, __FUNCTION__);
    exit(1);
  }

  logical_channel_group = *l->ul_SpecificParameters->logicalChannelGroup;

  /* TODO: accept other values? */
  if (logical_channel_group != 0) {
    LOG_E(RLC, "%s:%d:%s: fatal error\n", __FILE__, __LINE__, __FUNCTION__);
    exit(1);
  }

  switch (r->present) {
  case NR_RLC_Config_PR_am: {
    struct NR_RLC_Config__am *am;
    am = r->choice.am;
    t_reassembly       = decode_t_reassembly(am->dl_AM_RLC.t_Reassembly);
    t_status_prohibit  = decode_t_status_prohibit(am->dl_AM_RLC.t_StatusProhibit);
    t_poll_retransmit  = decode_t_poll_retransmit(am->ul_AM_RLC.t_PollRetransmit);
    poll_pdu           = decode_poll_pdu(am->ul_AM_RLC.pollPDU);
    poll_byte          = decode_poll_byte(am->ul_AM_RLC.pollByte);
    max_retx_threshold = decode_max_retx_threshold(am->ul_AM_RLC.maxRetxThreshold);
    if (*am->dl_AM_RLC.sn_FieldLength != *am->ul_AM_RLC.sn_FieldLength) {
      LOG_E(RLC, "%s:%d:%s: fatal\n", __FILE__, __LINE__, __FUNCTION__);
      exit(1);
    }
    sn_field_length    = decode_sn_field_length_am(*am->dl_AM_RLC.sn_FieldLength);
    break;
  }
  default:
    LOG_E(RLC, "%s:%d:%s: fatal error\n", __FILE__, __LINE__, __FUNCTION__);
    exit(1);
  }

  nr_rlc_manager_lock(nr_rlc_ue_manager);
  ue = nr_rlc_manager_get_ue(nr_rlc_ue_manager, rnti);
  if (ue->srb[srb_id-1] != NULL) {
    LOG_W(RLC, "%s:%d:%s: SRB %d already exists for UE with RNTI 0x%x, do nothing\n", __FILE__, __LINE__, __FUNCTION__, srb_id, rnti);
  } else {
    /* hack: hardcode values for NR */
    t_poll_retransmit = 45;
    t_reassembly = 35;
    t_status_prohibit = 0;
    poll_pdu = -1;
    poll_byte = -1;
    max_retx_threshold = 8;
    sn_field_length = 12;
    nr_rlc_am = new_nr_rlc_entity_am(10000000,
                                     10000000,
                                     deliver_sdu, ue,
                                     successful_delivery, ue,
                                     max_retx_reached, ue,
                                     t_poll_retransmit,
                                     t_reassembly, t_status_prohibit,
                                     poll_pdu, poll_byte, max_retx_threshold,
                                     sn_field_length);
    nr_rlc_ue_add_srb_rlc_entity(ue, srb_id, nr_rlc_am);

    LOG_I(RLC, "%s:%d:%s: added srb %d to UE with RNTI 0x%x\n", __FILE__, __LINE__, __FUNCTION__, srb_id, rnti);
  }
  nr_rlc_manager_unlock(nr_rlc_ue_manager);
}

static void add_drb_am(int rnti, struct NR_DRB_ToAddMod *s, NR_RLC_BearerConfig_t *rlc_BearerConfig)
{
  nr_rlc_entity_t            *nr_rlc_am;
  nr_rlc_ue_t                *ue;

  struct NR_RLC_Config *r = rlc_BearerConfig->rlc_Config;
  struct NR_LogicalChannelConfig *l = rlc_BearerConfig->mac_LogicalChannelConfig;
  int drb_id = s->drb_Identity;
  int channel_id = rlc_BearerConfig->logicalChannelIdentity;
  int logical_channel_group;

  int t_status_prohibit;
  int t_poll_retransmit;
  int poll_pdu;
  int poll_byte;
  int max_retx_threshold;
  int t_reassembly;
  int sn_field_length;

  if (!(drb_id >= 1 && drb_id <= 5)) {
    LOG_E(RLC, "%s:%d:%s: fatal, bad srb id %d\n",
          __FILE__, __LINE__, __FUNCTION__, drb_id);
    exit(1);
  }

  if (channel_id != drb_id + 3) {
    LOG_E(RLC, "%s:%d:%s: todo, remove this limitation\n",
          __FILE__, __LINE__, __FUNCTION__);
    exit(1);
  }

  logical_channel_group = *l->ul_SpecificParameters->logicalChannelGroup;

  /* TODO: accept other values? */
  if (logical_channel_group != 1) {
    LOG_E(RLC, "%s:%d:%s: fatal error\n", __FILE__, __LINE__, __FUNCTION__);
    //exit(1);
  }

  switch (r->present) {
  case NR_RLC_Config_PR_am: {
    struct NR_RLC_Config__am *am;
    am = r->choice.am;
    t_reassembly       = decode_t_reassembly(am->dl_AM_RLC.t_Reassembly);
    t_status_prohibit  = decode_t_status_prohibit(am->dl_AM_RLC.t_StatusProhibit);
    t_poll_retransmit  = decode_t_poll_retransmit(am->ul_AM_RLC.t_PollRetransmit);
    poll_pdu           = decode_poll_pdu(am->ul_AM_RLC.pollPDU);
    poll_byte          = decode_poll_byte(am->ul_AM_RLC.pollByte);
    max_retx_threshold = decode_max_retx_threshold(am->ul_AM_RLC.maxRetxThreshold);
    if (*am->dl_AM_RLC.sn_FieldLength != *am->ul_AM_RLC.sn_FieldLength) {
      LOG_E(RLC, "%s:%d:%s: fatal\n", __FILE__, __LINE__, __FUNCTION__);
      exit(1);
    }
    sn_field_length    = decode_sn_field_length_am(*am->dl_AM_RLC.sn_FieldLength);
    break;
  }
  default:
    LOG_E(RLC, "%s:%d:%s: fatal error\n", __FILE__, __LINE__, __FUNCTION__);
    exit(1);
  }

  nr_rlc_manager_lock(nr_rlc_ue_manager);
  ue = nr_rlc_manager_get_ue(nr_rlc_ue_manager, rnti);
  if (ue->drb[drb_id-1] != NULL) {
    LOG_W(RLC, "%s:%d:%s: DRB %d already exists for UE with RNTI %d, do nothing\n", __FILE__, __LINE__, __FUNCTION__, drb_id, rnti);
  } else {
    nr_rlc_am = new_nr_rlc_entity_am(10000000,
                                     10000000,
                                     deliver_sdu, ue,
                                     successful_delivery, ue,
                                     max_retx_reached, ue,
                                     t_poll_retransmit,
                                     t_reassembly, t_status_prohibit,
                                     poll_pdu, poll_byte, max_retx_threshold,
                                     sn_field_length);
    nr_rlc_ue_add_drb_rlc_entity(ue, drb_id, nr_rlc_am);

    LOG_I(RLC, "%s:%d:%s: added drb %d to UE with RNTI 0x%x\n", __FILE__, __LINE__, __FUNCTION__, drb_id, rnti);
  }
  nr_rlc_manager_unlock(nr_rlc_ue_manager);
}

static void add_drb_um(int rnti, struct NR_DRB_ToAddMod *s, NR_RLC_BearerConfig_t *rlc_BearerConfig)
{
  nr_rlc_entity_t            *nr_rlc_um;
  nr_rlc_ue_t                *ue;

  struct NR_RLC_Config *r = rlc_BearerConfig->rlc_Config;
  struct NR_LogicalChannelConfig *l = rlc_BearerConfig->mac_LogicalChannelConfig;
  int drb_id = s->drb_Identity;
  int channel_id = rlc_BearerConfig->logicalChannelIdentity;
  int logical_channel_group;

  int sn_field_length;
  int t_reassembly;

  if (!(drb_id >= 1 && drb_id <= 5)) {
    LOG_E(RLC, "%s:%d:%s: fatal, bad srb id %d\n",
          __FILE__, __LINE__, __FUNCTION__, drb_id);
    exit(1);
  }

  if (channel_id != drb_id + 3) {
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
  case NR_RLC_Config_PR_um_Bi_Directional: {
    struct NR_RLC_Config__um_Bi_Directional *um;
    um = r->choice.um_Bi_Directional;
    t_reassembly = decode_t_reassembly(um->dl_UM_RLC.t_Reassembly);
    if (*um->dl_UM_RLC.sn_FieldLength != *um->ul_UM_RLC.sn_FieldLength) {
      LOG_E(RLC, "%s:%d:%s: fatal\n", __FILE__, __LINE__, __FUNCTION__);
      exit(1);
    }
    sn_field_length = decode_sn_field_length_um(*um->dl_UM_RLC.sn_FieldLength);
    break;
  }
  default:
    LOG_E(RLC, "%s:%d:%s: fatal error\n", __FILE__, __LINE__, __FUNCTION__);
    exit(1);
  }

  nr_rlc_manager_lock(nr_rlc_ue_manager);
  ue = nr_rlc_manager_get_ue(nr_rlc_ue_manager, rnti);
  if (ue->drb[drb_id-1] != NULL) {
    LOG_W(RLC, "DEBUG add_drb_um %s:%d:%s: warning DRB %d already exist for ue %d, do nothing\n", __FILE__, __LINE__, __FUNCTION__, drb_id, rnti);
  } else {
    nr_rlc_um = new_nr_rlc_entity_um(100000000,
                                     100000000,
                                     deliver_sdu, ue,
                                     t_reassembly,
                                     sn_field_length);
    nr_rlc_ue_add_drb_rlc_entity(ue, drb_id, nr_rlc_um);

    LOG_D(RLC, "%s:%d:%s: added drb %d to UE with RNTI 0x%x\n", __FILE__, __LINE__, __FUNCTION__, drb_id, rnti);
  }
  nr_rlc_manager_unlock(nr_rlc_ue_manager);
}

static void add_drb(int rnti, struct NR_DRB_ToAddMod *s, struct NR_RLC_BearerConfig *rlc_BearerConfig)
{
  switch (rlc_BearerConfig->rlc_Config->present) {
  case NR_RLC_Config_PR_am:
    add_drb_am(rnti, s, rlc_BearerConfig);
    break;
  case NR_RLC_Config_PR_um_Bi_Directional:
    add_drb_um(rnti, s, rlc_BearerConfig);
    break;
  default:
    LOG_E(RLC, "%s:%d:%s: fatal: unhandled DRB type\n",
          __FILE__, __LINE__, __FUNCTION__);
    exit(1);
  }
  LOG_I(RLC, "%s:%s:%d: added DRB to UE with RNTI 0x%x\n", __FILE__, __FUNCTION__, __LINE__, rnti);
}

/* Dummy function due to dependency from LTE libraries */
rlc_op_status_t rrc_rlc_config_asn1_req (const protocol_ctxt_t   * const ctxt_pP,
    const LTE_SRB_ToAddModList_t   * const srb2add_listP,
    const LTE_DRB_ToAddModList_t   * const drb2add_listP,
    const LTE_DRB_ToReleaseList_t  * const drb2release_listP,
    const LTE_PMCH_InfoList_r9_t * const pmch_InfoList_r9_pP,
    const uint32_t sourceL2Id,
    const uint32_t destinationL2Id)
{
  return 0;
}

rlc_op_status_t nr_rrc_rlc_config_asn1_req (const protocol_ctxt_t   * const ctxt_pP,
    const NR_SRB_ToAddModList_t   * const srb2add_listP,
    const NR_DRB_ToAddModList_t   * const drb2add_listP,
    const NR_DRB_ToReleaseList_t  * const drb2release_listP,
    const LTE_PMCH_InfoList_r9_t * const pmch_InfoList_r9_pP,
    struct NR_CellGroupConfig__rlc_BearerToAddModList *rlc_bearer2add_list)
{
  int rnti = ctxt_pP->rnti;
  int i;
  int j;

  if (/*ctxt_pP->enb_flag != 1 ||*/ ctxt_pP->module_id != 0 /*||
      ctxt_pP->instance != 0 || ctxt_pP->eNB_index != 0 ||
      ctxt_pP->configured != 1 || ctxt_pP->brOption != 0 */) {
    LOG_E(RLC, "%s: ctxt_pP not handled (%d %d %ld %d %d %d)\n", __FUNCTION__,
          ctxt_pP->enb_flag , ctxt_pP->module_id, ctxt_pP->instance,
          ctxt_pP->eNB_index, ctxt_pP->configured, ctxt_pP->brOption);
    exit(1);
  }

  if (pmch_InfoList_r9_pP != NULL) {
    LOG_E(RLC, "%s: pmch_InfoList_r9_pP not handled\n", __FUNCTION__);
    exit(1);
  }

  if (drb2release_listP != NULL) {
    LOG_E(RLC, "%s:%d:%s: TODO\n", __FILE__, __LINE__, __FUNCTION__);
    //exit(1);
  }

  if (srb2add_listP != NULL) {
    for (i = 0; i < srb2add_listP->list.count; i++) {
      if (rlc_bearer2add_list != NULL) {
        for(j = 0; j < rlc_bearer2add_list->list.count; j++){
          if(rlc_bearer2add_list->list.array[j]->servedRadioBearer != NULL){
            if(rlc_bearer2add_list->list.array[j]->servedRadioBearer->present == NR_RLC_BearerConfig__servedRadioBearer_PR_srb_Identity){
              if(srb2add_listP->list.array[i]->srb_Identity == rlc_bearer2add_list->list.array[j]->servedRadioBearer->choice.srb_Identity){
                add_rlc_srb(rnti, srb2add_listP->list.array[i], rlc_bearer2add_list->list.array[j]);
              }
            }
          }
        }
      }

    }
  }

  if ((drb2add_listP != NULL) && (rlc_bearer2add_list != NULL)) {
    for (i = 0; i < drb2add_listP->list.count; i++) {
      if (rlc_bearer2add_list != NULL) {
      for(j = 0; j < rlc_bearer2add_list->list.count; j++){
        if(rlc_bearer2add_list->list.array[j]->servedRadioBearer != NULL){
          if(rlc_bearer2add_list->list.array[j]->servedRadioBearer->present == NR_RLC_BearerConfig__servedRadioBearer_PR_drb_Identity){
            if(drb2add_listP->list.array[i]->drb_Identity == rlc_bearer2add_list->list.array[j]->servedRadioBearer->choice.drb_Identity){
              add_drb(rnti, drb2add_listP->list.array[i], rlc_bearer2add_list->list.array[j]);
            }
          }  
        }
      }
      }
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
  nr_rlc_ue_t *ue;
  int      i;

  if (mbms_flagP) {
    LOG_E(RLC, "%s:%d:%s: todo (MBMS NOT supported)\n", __FILE__, __LINE__, __FUNCTION__);
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
    LOG_E(RLC, "%s:%d:%s: bad rb_id (%ld) (is_srb %d)\n", __FILE__, __LINE__, __FUNCTION__, rb_idP, srb_flagP);
    exit(1);
  }
  nr_rlc_manager_lock(nr_rlc_ue_manager);
  LOG_D(RLC, "%s:%d:%s: remove rb %ld (is_srb %d) for UE RNTI %x\n", __FILE__, __LINE__, __FUNCTION__, rb_idP, srb_flagP, ctxt_pP->rnti);
  ue = nr_rlc_manager_get_ue(nr_rlc_ue_manager, ctxt_pP->rnti);
  if (srb_flagP) {
    if (ue->srb[rb_idP-1] != NULL) {
      ue->srb[rb_idP-1]->delete(ue->srb[rb_idP-1]);
      ue->srb[rb_idP-1] = NULL;
    } else
      LOG_W(RLC, "removing non allocated SRB %ld, do nothing\n", rb_idP);
  } else {
    if (ue->drb[rb_idP-1] != NULL) {
      ue->drb[rb_idP-1]->delete(ue->drb[rb_idP-1]);
      ue->drb[rb_idP-1] = NULL;
    } else
      LOG_W(RLC, "removing non allocated DRB %ld, do nothing\n", rb_idP);
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
      nr_rlc_manager_remove_ue(nr_rlc_ue_manager, ctxt_pP->rnti);
  }
  nr_rlc_manager_unlock(nr_rlc_ue_manager);
  return RLC_OP_STATUS_OK;
}

void rrc_rlc_register_rrc (rrc_data_ind_cb_t rrc_data_indP, rrc_data_conf_cb_t rrc_data_confP)
{
  /* nothing to do */
}

rlc_op_status_t rrc_rlc_remove_ue (const protocol_ctxt_t* const x)
{
  LOG_D(RLC, "%s:%d:%s: remove UE %d\n", __FILE__, __LINE__, __FUNCTION__, x->rnti);
  nr_rlc_manager_lock(nr_rlc_ue_manager);
  nr_rlc_manager_remove_ue(nr_rlc_ue_manager, x->rnti);
  nr_rlc_manager_unlock(nr_rlc_ue_manager);

  return RLC_OP_STATUS_OK;
}

void nr_rlc_tick(int frame, int subframe)
{
  if (frame != nr_rlc_current_time_last_frame ||
      subframe != nr_rlc_current_time_last_subframe) {
    nr_rlc_current_time_last_frame = frame;
    nr_rlc_current_time_last_subframe = subframe;
    nr_rlc_current_time++;
  }
}

/* This is a hack, to compile the gNB.
 * TODO: remove it. The solution is to cleanup cmake_targets/CMakeLists.txt
 */
void rlc_tick(int a, int b)
{
  LOG_E(RLC, "%s:%d:%s: this code should not be reached\n",
        __FILE__, __LINE__, __FUNCTION__);
  exit(1);
}
