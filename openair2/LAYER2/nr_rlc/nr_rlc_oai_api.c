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
#include "LAYER2/nr_pdcp/nr_pdcp_oai_api.h"

/* from nr rlc module */
#include "nr_rlc_asn1_utils.h"
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
#include "openair2/F1AP/f1ap_ids.h"

extern RAN_CONTEXT_t RC;

#include <stdint.h>

#include <executables/softmodem-common.h>

static nr_rlc_ue_manager_t *nr_rlc_ue_manager;

/* TODO: handle time a bit more properly */
static uint64_t nr_rlc_current_time;
static int      nr_rlc_current_time_last_frame;
static int      nr_rlc_current_time_last_subframe;

static void release_rlc_entity_from_lcid(nr_rlc_ue_t *ue, logical_chan_id_t channel_id)
{
  AssertFatal(channel_id != 0, "LCID = 0 shouldn't be handled here\n");
  nr_rlc_rb_t *rb = &ue->lcid2rb[channel_id - 1];
  if (rb->type == NR_RLC_NONE)
    return;
  if (rb->type == NR_RLC_SRB) {
    int id = rb->choice.srb_id - 1;
    AssertFatal(id >= 0, "logic bug: impossible to have srb0 here\n");
    if (ue->srb[id]) {
      ue->srb[id]->delete_entity(ue->srb[id]);
      ue->srb[id] = NULL;
    }
    else
      LOG_E(RLC, "Trying to release a non-established enity with LCID %d\n", channel_id);
  }
  else {
    AssertFatal(rb->type == NR_RLC_DRB,
                "Invalid RB type\n");
    int id = rb->choice.drb_id - 1;
    if (ue->drb[id]) {
      ue->drb[id]->delete_entity(ue->drb[id]);
      ue->drb[id] = NULL;
    }
    else
      LOG_E(RLC, "Trying to release a non-established enity with LCID %d\n", channel_id);
  }
}

static nr_rlc_entity_t *get_rlc_entity_from_lcid(nr_rlc_ue_t *ue, logical_chan_id_t channel_id)
{
  if (channel_id == 0)
    return ue->srb0;
  nr_rlc_rb_t *rb = &ue->lcid2rb[channel_id - 1];
  if (rb->type == NR_RLC_NONE)
    return NULL;
  if (rb->type == NR_RLC_SRB) {
    AssertFatal(rb->choice.srb_id > 0, "logic bug: impossible to have srb0 here\n");
    return ue->srb[rb->choice.srb_id - 1];
  } else {
    AssertFatal(rb->type == NR_RLC_DRB,
                "Invalid RB type\n");
    return ue->drb[rb->choice.drb_id - 1];
  }
}

void nr_rlc_release_entity(int rnti, logical_chan_id_t channel_id)
{
  nr_rlc_manager_lock(nr_rlc_ue_manager);
  nr_rlc_ue_t *ue = nr_rlc_manager_get_ue(nr_rlc_ue_manager, rnti);
  if (channel_id == 0) {
    if (ue->srb0 != NULL) {
      free(ue->srb0->deliver_sdu_data);
      ue->srb0->delete_entity(ue->srb0);
      ue->srb0 = NULL;
    } else
      LOG_E(RLC, "Trying to release a non-established enity with LCID %d\n", channel_id);
  } else {
    release_rlc_entity_from_lcid(ue, channel_id);
  }
  nr_rlc_manager_unlock(nr_rlc_ue_manager);
}

void mac_rlc_data_ind(const module_id_t  module_idP,
                      const rnti_t rntiP,
                      const eNB_index_t eNB_index,
                      const frame_t  rameP,
                      const eNB_flag_t enb_flagP,
                      const MBMS_flag_t MBMS_flagP,
                      const logical_chan_id_t channel_idP,
                      char *buffer_pP,
                      const tb_size_t tb_sizeP,
                      num_tb_t num_tbP,
                      crc_t *crcs_pP)
{
  if (module_idP != 0 || eNB_index != 0 || /*enb_flagP != 1 ||*/ MBMS_flagP != 0) {
    LOG_E(RLC, "%s:%d:%s: fatal\n", __FILE__, __LINE__, __FUNCTION__);
    exit(1);
  }

  if (enb_flagP)
    T(T_ENB_RLC_MAC_UL, T_INT(module_idP), T_INT(rntiP),
      T_INT(channel_idP), T_INT(tb_sizeP));

  nr_rlc_manager_lock(nr_rlc_ue_manager);
  nr_rlc_ue_t *ue = nr_rlc_manager_get_ue(nr_rlc_ue_manager, rntiP);

  if(ue == NULL)
    LOG_I(RLC, "RLC instance for the given UE was not found \n");

  nr_rlc_entity_t *rb = get_rlc_entity_from_lcid(ue, channel_idP);

  if (rb != NULL) {
    LOG_D(RLC, "RB found! (channel ID %d) \n", channel_idP);
    rb->set_time(rb, nr_rlc_current_time);
    rb->recv_pdu(rb, buffer_pP, tb_sizeP);
  } else {
    LOG_E(RLC, "Fatal: no RB found (channel ID %d RNTI %d)\n",
          channel_idP, rntiP);
    // exit(1);
  }

  nr_rlc_manager_unlock(nr_rlc_ue_manager);
}

tbs_size_t mac_rlc_data_req(const module_id_t  module_idP,
                            const rnti_t rntiP,
                            const eNB_index_t eNB_index,
                            const frame_t frameP,
                            const eNB_flag_t enb_flagP,
                            const MBMS_flag_t MBMS_flagP,
                            const logical_chan_id_t channel_idP,
                            const tb_size_t tb_sizeP,
                            char *buffer_pP,
                            const uint32_t sourceL2Id,
                            const uint32_t destinationL2Id)
{
  int ret;
  int maxsize;

  nr_rlc_manager_lock(nr_rlc_ue_manager);
  nr_rlc_ue_t *ue = nr_rlc_manager_get_ue(nr_rlc_ue_manager, rntiP);
  nr_rlc_entity_t *rb = get_rlc_entity_from_lcid(ue, channel_idP);

  if (rb != NULL) {
    LOG_D(RLC, "MAC PDU to get created for channel_idP:%d \n", channel_idP);
    rb->set_time(rb, nr_rlc_current_time);
    maxsize = tb_sizeP;
    ret = rb->generate_pdu(rb, buffer_pP, maxsize);
  } else {
    LOG_D(RLC, "MAC PDU failed to get created for channel_idP:%d \n", channel_idP);
    ret = 0;
  }

  nr_rlc_manager_unlock(nr_rlc_ue_manager);

  if (enb_flagP)
    T(T_ENB_RLC_MAC_DL, T_INT(module_idP), T_INT(rntiP),
      T_INT(channel_idP), T_INT(ret));

  return ret;
}

mac_rlc_status_resp_t mac_rlc_status_ind(const module_id_t module_idP,
                                         const rnti_t rntiP,
                                         const eNB_index_t eNB_index,
                                         const frame_t frameP,
                                         const sub_frame_t subframeP,
                                         const eNB_flag_t enb_flagP,
                                         const MBMS_flag_t MBMS_flagP,
                                         const logical_chan_id_t channel_idP,
                                         const uint32_t sourceL2Id,
                                         const uint32_t destinationL2Id)
{
  mac_rlc_status_resp_t ret;

  nr_rlc_manager_lock(nr_rlc_ue_manager);
  nr_rlc_ue_t *ue = nr_rlc_manager_get_ue(nr_rlc_ue_manager, rntiP);
  nr_rlc_entity_t *rb = get_rlc_entity_from_lcid(ue, channel_idP);

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
    if (!(frameP%128) || channel_idP == 0) //to suppress this warning message
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

rlc_buffer_occupancy_t mac_rlc_get_buffer_occupancy_ind(const module_id_t module_idP,
                                                        const rnti_t rntiP,
                                                        const eNB_index_t eNB_index,
                                                        const frame_t frameP,
                                                        const sub_frame_t subframeP,
                                                        const eNB_flag_t enb_flagP,
                                                        const logical_chan_id_t channel_idP)
{
  rlc_buffer_occupancy_t ret;

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
  nr_rlc_ue_t *ue = nr_rlc_manager_get_ue(nr_rlc_ue_manager, rntiP);
  nr_rlc_entity_t *rb = get_rlc_entity_from_lcid(ue, channel_idP);

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
    if (!(frameP%128)) //to suppress this warning message
      LOG_W(RLC, "[%s] Radio Bearer (channel ID %d) is NULL for UE with rntiP %x\n", __FUNCTION__, channel_idP, rntiP);
    ret = 0;
  }

  nr_rlc_manager_unlock(nr_rlc_ue_manager);

  return ret;
}

rlc_op_status_t rlc_data_req(const protocol_ctxt_t *const ctxt_pP,
                             const srb_flag_t srb_flagP,
                             const MBMS_flag_t MBMS_flagP,
                             const rb_id_t rb_idP,
                             const mui_t muiP,
                             confirm_t confirmP,
                             sdu_size_t sdu_sizeP,
                             uint8_t *sdu_pP,
                             const uint32_t *const sourceL2Id,
                             const uint32_t *const destinationL2Id)
{
  int rnti = ctxt_pP->rntiMaybeUEid;
  nr_rlc_ue_t *ue;
  nr_rlc_entity_t *rb;

  LOG_D(RLC, "rnti %d srb_flag %d rb_id %ld mui %d confirm %d sdu_size %d MBMS_flag %d\n",
        rnti, srb_flagP, rb_idP, muiP, confirmP, sdu_sizeP, MBMS_flagP);

  if (ctxt_pP->enb_flag)
    T(T_ENB_RLC_DL, T_INT(ctxt_pP->module_id), T_INT(ctxt_pP->rntiMaybeUEid), T_INT(rb_idP), T_INT(sdu_sizeP));

  nr_rlc_manager_lock(nr_rlc_ue_manager);
  ue = nr_rlc_manager_get_ue(nr_rlc_ue_manager, rnti);

  rb = NULL;

  if (srb_flagP) {
    if (rb_idP >= 1 && rb_idP <= 2)
      rb = ue->srb[rb_idP - 1];
  } else {
    if (rb_idP >= 1 && rb_idP <= MAX_DRBS_PER_UE)
      rb = ue->drb[rb_idP - 1];
  }

  if (rb != NULL) {
    rb->set_time(rb, nr_rlc_current_time);
    rb->recv_sdu(rb, (char *)sdu_pP, sdu_sizeP, muiP);
  } else {
    LOG_E(RLC, "%s:%d:%s: fatal: SDU sent to unknown RB\n", __FILE__, __LINE__, __FUNCTION__);
  }

  nr_rlc_manager_unlock(nr_rlc_ue_manager);

  free(sdu_pP);

  return RLC_OP_STATUS_OK;
}

int nr_rlc_get_available_tx_space(const rnti_t rntiP,
                                  const logical_chan_id_t channel_idP)
{
  int ret;

  nr_rlc_manager_lock(nr_rlc_ue_manager);
  nr_rlc_ue_t *ue = nr_rlc_manager_get_ue(nr_rlc_ue_manager, rntiP);
  nr_rlc_entity_t *rb = get_rlc_entity_from_lcid(ue, channel_idP);

  if (rb != NULL) {
    ret = rb->available_tx_space(rb);
  } else {
    LOG_E(RLC, "[%s] Radio Bearer (channel ID %d) is NULL for UE with rntiP %x\n", __FUNCTION__, channel_idP, rntiP);
    ret = -1;
  }

  nr_rlc_manager_unlock(nr_rlc_ue_manager);

  return ret;
}

int rlc_module_init(int enb_flag)
{
  static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
  static int inited = 0;
  static int inited_ue = 0;

  if (pthread_mutex_lock(&lock)) abort();

  if (enb_flag == 1 && inited) {
    LOG_E(RLC, "%s:%d:%s: fatal, inited already 1\n", __FILE__, __LINE__, __FUNCTION__);
    exit(1);
  }

  if (enb_flag == 0 && inited_ue) {
    LOG_E(RLC, "%s:%d:%s: fatal, inited_ue already 1\n", __FILE__, __LINE__, __FUNCTION__);
    exit(1);
  }

  if (enb_flag == 1) inited = 1;
  if (enb_flag == 0) inited_ue = 1;

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
  uint8_t *memblock;
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
  LOG_D(RLC, "%s:%d:%s: delivering SDU (rnti %d is_srb %d rb_id %d) size %d\n",
        __FILE__, __LINE__, __FUNCTION__, ue->rnti, is_srb, rb_id, size);

  /* unused fields? */
  ctx.instance = 0;
  ctx.frame = 0;
  ctx.subframe = 0;
  ctx.eNB_index = 0;
  ctx.brOption = 0;

  is_enb = nr_rlc_manager_get_enb_flag(nr_rlc_ue_manager);

  /* used fields? */
  ctx.module_id = 0;
  /* CU (PDCP, RRC, SDAP) use a different ID than RNTI, so below set the CU UE
   * ID if in gNB, else use RNTI normally */
  ctx.rntiMaybeUEid = ue->rnti;
  if (is_enb) {
    f1_ue_data_t ue_data = du_get_f1_ue_data(ue->rnti);
    ctx.rntiMaybeUEid = ue_data.secondary_ue;
  }

  ctx.enb_flag = is_enb;

  if (is_enb) {
    T(T_ENB_RLC_UL,
      T_INT(0 /*ctxt_pP->module_id*/),
      T_INT(ue->rnti), T_INT(rb_id), T_INT(size));

    const ngran_node_t type = RC.nrrrc[0 /*ctxt_pP->module_id*/]->node_type;
    AssertFatal(!NODE_IS_CU(type),
                "Can't be CU, bad node type %d\n", type);

    // if (NODE_IS_DU(type) && is_srb == 0) {
    //   LOG_D(RLC, "call proto_agent_send_pdcp_data_ind() \n");
    //   proto_agent_send_pdcp_data_ind(&ctx, is_srb, 0, rb_id, size, memblock);
    //   return;
    // }

    if (NODE_IS_DU(type)) {
      if(is_srb) {
	MessageDef *msg;
	msg = itti_alloc_new_message(TASK_RLC_ENB, 0, F1AP_UL_RRC_MESSAGE);
	uint8_t *message_buffer = itti_malloc (TASK_RLC_ENB, TASK_DU_F1, size);
	memcpy (message_buffer, buf, size);
        F1AP_UL_RRC_MESSAGE(msg).gNB_CU_ue_id = ctx.rntiMaybeUEid;
	F1AP_UL_RRC_MESSAGE(msg).gNB_DU_ue_id = ue->rnti;
	F1AP_UL_RRC_MESSAGE(msg).srb_id = rb_id;
	F1AP_UL_RRC_MESSAGE(msg).rrc_container = message_buffer;
	F1AP_UL_RRC_MESSAGE(msg).rrc_container_length = size;
	itti_send_msg_to_task(TASK_DU_F1, ENB_MODULE_ID_TO_INSTANCE(0 /*ctxt_pP->module_id*/), msg);
	return;
      } else {
	MessageDef *msg = itti_alloc_new_message_sized(TASK_RLC_ENB, 0, GTPV1U_TUNNEL_DATA_REQ,
						       sizeof(gtpv1u_tunnel_data_req_t) + size);
	gtpv1u_tunnel_data_req_t *req=&GTPV1U_TUNNEL_DATA_REQ(msg);
	req->buffer=(uint8_t*)(req+1);
	memcpy(req->buffer,buf,size);
	req->length=size;
	req->offset=0;
	req->ue_id=ue->rnti; // use RNTI here, which GTP will use to look up TEID
	req->bearer_id=rb_id;
	LOG_D(RLC, "Received uplink user-plane traffic at RLC-DU to be sent to the CU, size %d \n", size);
	extern instance_t DUuniqInstance;
	itti_send_msg_to_task(TASK_GTPV1_U, DUuniqInstance, msg);
	return;
      }
    }
  }

  /* UE or monolithic gNB */
  memblock = malloc16(size);
  if (memblock == NULL) {
    LOG_E(RLC, "%s:%d:%s: ERROR: malloc16 failed\n", __FILE__, __LINE__, __FUNCTION__);
    exit(1);
  }
  memcpy(memblock, buf, size);
  LOG_D(PDCP, "Calling PDCP layer from RLC in %s\n", __FUNCTION__);
  if (!pdcp_data_ind(&ctx, is_srb, 0, rb_id, size, memblock, NULL, NULL)) {
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
  for (i = 0; i < MAX_DRBS_PER_UE; i++) {
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
  for (i = 0; i < MAX_DRBS_PER_UE; i++) {
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

void nr_rlc_reestablish_entity(int rnti, int lc_id)
{
  nr_rlc_manager_lock(nr_rlc_ue_manager);
  nr_rlc_ue_t *ue = nr_rlc_manager_get_ue(nr_rlc_ue_manager, rnti);

  if (ue == NULL)
    LOG_E(RLC, "RLC instance for the given UE was not found \n");

  nr_rlc_entity_t *rb = get_rlc_entity_from_lcid(ue, lc_id);

  if (rb != NULL) {
    LOG_D(RLC, "RB found! (channel ID %d) \n", lc_id);
    rb->reestablishment(rb);
  } else {
    LOG_E(RLC, "no RLC entity found (channel ID %d) for reestablishment\n", lc_id);
  }
  nr_rlc_manager_unlock(nr_rlc_ue_manager);
}

void nr_rlc_reconfigure_entity(int rnti, int lc_id, NR_RLC_Config_t *rlc_Config)
{
  nr_rlc_manager_lock(nr_rlc_ue_manager);
  nr_rlc_ue_t *ue = nr_rlc_manager_get_ue(nr_rlc_ue_manager, rnti);

  if (ue == NULL)
    LOG_E(RLC, "RLC instance for the given UE was not found \n");

  nr_rlc_entity_t *rb = get_rlc_entity_from_lcid(ue, lc_id);
  if (rlc_Config) {
    AssertFatal(rb->stats.mode != NR_RLC_TM, "Cannot reconfigure TM mode\n");
    if (rb->stats.mode == NR_RLC_AM) {
      AssertFatal(rlc_Config->present == NR_RLC_Config_PR_am, "Invalid RLC Config type\n");
      struct NR_RLC_Config__am *am = rlc_Config->choice.am;
      int t_reassembly = decode_t_reassembly(am->dl_AM_RLC.t_Reassembly);
      int t_status_prohibit = decode_t_status_prohibit(am->dl_AM_RLC.t_StatusProhibit);
      int t_poll_retransmit = decode_t_poll_retransmit(am->ul_AM_RLC.t_PollRetransmit);
      int poll_pdu = decode_poll_pdu(am->ul_AM_RLC.pollPDU);
      int poll_byte = decode_poll_byte(am->ul_AM_RLC.pollByte);
      int max_retx_threshold = decode_max_retx_threshold(am->ul_AM_RLC.maxRetxThreshold);
      int _sn_field_length;
      int *sn_field_length = NULL;
      if (am->dl_AM_RLC.sn_FieldLength) {
        AssertFatal(am->ul_AM_RLC.sn_FieldLength != NULL, "Cannot handle different sn_FieldLength for DL and UL\n");
        if (am->ul_AM_RLC.sn_FieldLength) {
          AssertFatal(*am->dl_AM_RLC.sn_FieldLength == *am->ul_AM_RLC.sn_FieldLength,
                      "Cannot handle different sn_FieldLength for DL and UL\n");
          _sn_field_length = decode_sn_field_length_am(*am->dl_AM_RLC.sn_FieldLength);
          sn_field_length = &_sn_field_length;
        }
      } else
        AssertFatal(am->ul_AM_RLC.sn_FieldLength == NULL, "Cannot handle different sn_FieldLength for DL and UL\n");
      nr_rlc_entity_am_reconfigure(rb,
                                   t_poll_retransmit,
                                   t_reassembly,
                                   t_status_prohibit,
                                   poll_pdu,
                                   poll_byte,
                                   max_retx_threshold,
                                   sn_field_length);
    } else { // UM
      AssertFatal(rlc_Config->present == NR_RLC_Config_PR_um_Bi_Directional, "Invalid RLC Config type\n");
      struct NR_RLC_Config__um_Bi_Directional *um = rlc_Config->choice.um_Bi_Directional;
      int t_reassembly = decode_t_reassembly(um->dl_UM_RLC.t_Reassembly);
      int _sn_field_length;
      int *sn_field_length = NULL;
      if (um->dl_UM_RLC.sn_FieldLength) {
        AssertFatal(um->ul_UM_RLC.sn_FieldLength != NULL, "Cannot handle different sn_FieldLength for DL and UL\n");
        if (um->ul_UM_RLC.sn_FieldLength) {
          AssertFatal(*um->dl_UM_RLC.sn_FieldLength == *um->ul_UM_RLC.sn_FieldLength,
                      "Cannot handle different sn_FieldLength for DL and UL\n");
          _sn_field_length = decode_sn_field_length_um(*um->dl_UM_RLC.sn_FieldLength);
          sn_field_length = &_sn_field_length;
        }
      } else
        AssertFatal(um->ul_UM_RLC.sn_FieldLength == NULL, "Cannot handle different sn_FieldLength for DL and UL\n");
      nr_rlc_entity_um_reconfigure(rb, t_reassembly, sn_field_length);
    }
  }
  nr_rlc_manager_unlock(nr_rlc_ue_manager);
}

void nr_rlc_add_srb(int rnti, int srb_id, const NR_RLC_BearerConfig_t *rlc_BearerConfig)
{
  struct NR_RLC_Config *r = rlc_BearerConfig->rlc_Config;
  int t_status_prohibit;
  int t_poll_retransmit;
  int poll_pdu;
  int poll_byte;
  int max_retx_threshold;
  int t_reassembly;
  int sn_field_length;

  LOG_D(RLC, "Trying to add SRB %d\n", srb_id);
  AssertFatal(srb_id > 0 && srb_id < 4,
              "Invalid srb id %d\n", srb_id);

  if (r && r->present == NR_RLC_Config_PR_am) {
    struct NR_RLC_Config__am *am;
    am = r->choice.am;
    t_reassembly = decode_t_reassembly(am->dl_AM_RLC.t_Reassembly);
    t_status_prohibit = decode_t_status_prohibit(am->dl_AM_RLC.t_StatusProhibit);
    t_poll_retransmit = decode_t_poll_retransmit(am->ul_AM_RLC.t_PollRetransmit);
    poll_pdu = decode_poll_pdu(am->ul_AM_RLC.pollPDU);
    poll_byte = decode_poll_byte(am->ul_AM_RLC.pollByte);
    max_retx_threshold = decode_max_retx_threshold(am->ul_AM_RLC.maxRetxThreshold);
    if (*am->dl_AM_RLC.sn_FieldLength != *am->ul_AM_RLC.sn_FieldLength) {
      LOG_E(RLC, "%s:%d:%s: fatal\n", __FILE__, __LINE__, __FUNCTION__);
      exit(1);
    }
    sn_field_length = decode_sn_field_length_am(*am->dl_AM_RLC.sn_FieldLength);
  } else {
    // default values as in 9.2.1 of 38.331
    t_reassembly = 35;
    t_status_prohibit = 0;
    t_poll_retransmit = 45;
    poll_pdu = -1;
    poll_byte = -1;
    max_retx_threshold = 8;
    sn_field_length = 12;
  }

  nr_rlc_manager_lock(nr_rlc_ue_manager);
  nr_rlc_ue_t *ue = nr_rlc_manager_get_ue(nr_rlc_ue_manager, rnti);
  AssertFatal(rlc_BearerConfig->servedRadioBearer &&
              (rlc_BearerConfig->servedRadioBearer->present ==
              NR_RLC_BearerConfig__servedRadioBearer_PR_srb_Identity),
              "servedRadioBearer for SRB mandatory present when setting up an SRB RLC entity\n");
  int local_id = rlc_BearerConfig->logicalChannelIdentity - 1; // LCID 0 for SRB 0 not mapped
  ue->lcid2rb[local_id].type = NR_RLC_SRB;
  ue->lcid2rb[local_id].choice.srb_id = rlc_BearerConfig->servedRadioBearer->choice.srb_Identity;
  if (ue->srb[srb_id-1] != NULL) {
    LOG_E(RLC, "%s:%d:%s: SRB %d already exists for UE with RNTI %04x, do nothing\n", __FILE__, __LINE__, __FUNCTION__, srb_id, rnti);
  } else {
    nr_rlc_entity_t *nr_rlc_am = new_nr_rlc_entity_am(RLC_RX_MAXSIZE,
                                                      RLC_TX_MAXSIZE,
                                                      deliver_sdu, ue,
                                                      successful_delivery, ue,
                                                      max_retx_reached, ue,
                                                      t_poll_retransmit,
                                                      t_reassembly, t_status_prohibit,
                                                      poll_pdu, poll_byte, max_retx_threshold,
                                                      sn_field_length);
    nr_rlc_ue_add_srb_rlc_entity(ue, srb_id, nr_rlc_am);

    LOG_I(RLC, "Added srb %d to UE with RNTI 0x%x\n", srb_id, rnti);
  }
  nr_rlc_manager_unlock(nr_rlc_ue_manager);
}

static void add_drb_am(int rnti, int drb_id, const NR_RLC_BearerConfig_t *rlc_BearerConfig)
{
  struct NR_RLC_Config *r = rlc_BearerConfig->rlc_Config;

  int t_status_prohibit;
  int t_poll_retransmit;
  int poll_pdu;
  int poll_byte;
  int max_retx_threshold;
  int t_reassembly;
  int sn_field_length;

  AssertFatal(drb_id > 0 && drb_id <= MAX_DRBS_PER_UE,
              "Invalid DRB ID %d\n", drb_id);

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
  nr_rlc_ue_t *ue = nr_rlc_manager_get_ue(nr_rlc_ue_manager, rnti);
  AssertFatal(rlc_BearerConfig->servedRadioBearer &&
              (rlc_BearerConfig->servedRadioBearer->present ==
              NR_RLC_BearerConfig__servedRadioBearer_PR_drb_Identity),
              "servedRadioBearer for DRB mandatory present when setting up an SRB RLC entity\n");
  int local_id = rlc_BearerConfig->logicalChannelIdentity - 1; // LCID 0 for SRB 0 not mapped
  ue->lcid2rb[local_id].type = NR_RLC_DRB;
  ue->lcid2rb[local_id].choice.drb_id = rlc_BearerConfig->servedRadioBearer->choice.drb_Identity;
  if (ue->drb[drb_id-1] != NULL) {
    LOG_E(RLC, "%s:%d:%s: DRB %d already exists for UE with RNTI %04x, do nothing\n", __FILE__, __LINE__, __FUNCTION__, drb_id, rnti);
  } else {
    nr_rlc_entity_t *nr_rlc_am = new_nr_rlc_entity_am(RLC_RX_MAXSIZE,
                                                      RLC_TX_MAXSIZE,
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

static void add_drb_um(int rnti, int drb_id, const NR_RLC_BearerConfig_t *rlc_BearerConfig)
{
  struct NR_RLC_Config *r = rlc_BearerConfig->rlc_Config;

  int sn_field_length;
  int t_reassembly;

  AssertFatal(drb_id > 0 && drb_id <= MAX_DRBS_PER_UE,
              "Invalid DRB ID %d\n", drb_id);

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
  nr_rlc_ue_t *ue = nr_rlc_manager_get_ue(nr_rlc_ue_manager, rnti);
  AssertFatal(rlc_BearerConfig->servedRadioBearer &&
              (rlc_BearerConfig->servedRadioBearer->present ==
              NR_RLC_BearerConfig__servedRadioBearer_PR_drb_Identity),
              "servedRadioBearer for DRB mandatory present when setting up an SRB RLC entity\n");
  int local_id = rlc_BearerConfig->logicalChannelIdentity - 1; // LCID 0 for SRB 0 not mapped
  ue->lcid2rb[local_id].type = NR_RLC_DRB;
  ue->lcid2rb[local_id].choice.drb_id = rlc_BearerConfig->servedRadioBearer->choice.drb_Identity;
  if (ue->drb[drb_id-1] != NULL) {
    LOG_E(RLC, "DEBUG add_drb_um %s:%d:%s: warning DRB %d already exist for ue %d, do nothing\n", __FILE__, __LINE__, __FUNCTION__, drb_id, rnti);
  } else {
    nr_rlc_entity_t *nr_rlc_um = new_nr_rlc_entity_um(RLC_RX_MAXSIZE,
                                                      RLC_TX_MAXSIZE,
                                                      deliver_sdu, ue,
                                                      t_reassembly,
                                                      sn_field_length);
    nr_rlc_ue_add_drb_rlc_entity(ue, drb_id, nr_rlc_um);

    LOG_D(RLC, "%s:%d:%s: added drb %d to UE with RNTI 0x%x\n", __FILE__, __LINE__, __FUNCTION__, drb_id, rnti);
  }
  nr_rlc_manager_unlock(nr_rlc_ue_manager);
}

void nr_rlc_add_drb(int rnti, int drb_id, const NR_RLC_BearerConfig_t *rlc_BearerConfig)
{
  switch (rlc_BearerConfig->rlc_Config->present) {
  case NR_RLC_Config_PR_am:
    add_drb_am(rnti, drb_id, rlc_BearerConfig);
    break;
  case NR_RLC_Config_PR_um_Bi_Directional:
    add_drb_um(rnti, drb_id, rlc_BearerConfig);
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

struct srb0_data {
  int rnti;
  void *data;
  void (*send_initial_ul_rrc_message)(int                    rnti,
                                      const uint8_t         *sdu,
                                      sdu_size_t             sdu_len,
                                      void                  *data);
};

void deliver_sdu_srb0(void *deliver_sdu_data, struct nr_rlc_entity_t *entity,
                      char *buf, int size)
{
  struct srb0_data *s0 = (struct srb0_data *)deliver_sdu_data;
  s0->send_initial_ul_rrc_message(s0->rnti, (unsigned char *)buf, size, s0->data);
}

void nr_rlc_activate_srb0(int rnti, void *data,
                          void (*send_initial_ul_rrc_message)(
                                     int                    rnti,
                                     const uint8_t         *sdu,
                                     sdu_size_t             sdu_len,
                                     void                  *data))
{
  nr_rlc_entity_t            *nr_rlc_tm;
  nr_rlc_ue_t                *ue;
  struct srb0_data           *srb0_data;

  srb0_data = calloc(1, sizeof(struct srb0_data));
  AssertFatal(srb0_data != NULL, "out of memory\n");

  srb0_data->rnti      = rnti;
  srb0_data->data      = data;
  srb0_data->send_initial_ul_rrc_message = send_initial_ul_rrc_message;

  nr_rlc_manager_lock(nr_rlc_ue_manager);
  ue = nr_rlc_manager_get_ue(nr_rlc_ue_manager, rnti);
  if (ue->srb0 != NULL) {
    LOG_W(RLC, "SRB0 already exists for UE with RNTI 0x%x, do nothing\n", rnti);
    free(srb0_data);
    nr_rlc_manager_unlock(nr_rlc_ue_manager);
    return;
  }

  nr_rlc_tm = new_nr_rlc_entity_tm(10000,
                                   deliver_sdu_srb0, srb0_data);
  nr_rlc_ue_add_srb_rlc_entity(ue, 0, nr_rlc_tm);

  LOG_I(RLC, "activated srb0 for UE with RNTI 0x%x\n", rnti);
  nr_rlc_manager_unlock(nr_rlc_ue_manager);
}

rlc_op_status_t rrc_rlc_config_req   (
  const protocol_ctxt_t* const ctxt_pP,
  const srb_flag_t      srb_flagP,
  const MBMS_flag_t     mbms_flagP,
  const config_action_t actionP,
  const rb_id_t         rb_idP)
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
      (!srb_flagP && !(rb_idP >= 1 && rb_idP <= MAX_DRBS_PER_UE))) {
    LOG_E(RLC, "%s:%d:%s: bad rb_id (%ld) (is_srb %d)\n", __FILE__, __LINE__, __FUNCTION__, rb_idP, srb_flagP);
    exit(1);
  }
  nr_rlc_manager_lock(nr_rlc_ue_manager);
  LOG_D(RLC, "%s:%d:%s: remove rb %ld (is_srb %d) for UE RNTI %lx\n", __FILE__, __LINE__, __FUNCTION__, rb_idP, srb_flagP, ctxt_pP->rntiMaybeUEid);
  ue = nr_rlc_manager_get_ue(nr_rlc_ue_manager, ctxt_pP->rntiMaybeUEid);
  if (srb_flagP) {
    if (ue->srb[rb_idP-1] != NULL) {
      ue->srb[rb_idP-1]->delete_entity(ue->srb[rb_idP-1]);
      ue->srb[rb_idP-1] = NULL;
    } else
      LOG_W(RLC, "removing non allocated SRB %ld, do nothing\n", rb_idP);
  } else {
    if (ue->drb[rb_idP-1] != NULL) {
      ue->drb[rb_idP-1]->delete_entity(ue->drb[rb_idP-1]);
      ue->drb[rb_idP-1] = NULL;
    } else
      LOG_W(RLC, "removing non allocated DRB %ld, do nothing\n", rb_idP);
  }
  /* remove UE if it has no more RB configured */
  for (i = 0; i < 2; i++)
    if (ue->srb[i] != NULL)
      break;
  if (i == 2) {
    for (i = 0; i < MAX_DRBS_PER_UE; i++)
      if (ue->drb[i] != NULL)
        break;
    if (i == MAX_DRBS_PER_UE)
      nr_rlc_manager_remove_ue(nr_rlc_ue_manager, ctxt_pP->rntiMaybeUEid);
  }
  nr_rlc_manager_unlock(nr_rlc_ue_manager);
  return RLC_OP_STATUS_OK;
}

void nr_rlc_remove_ue(int rnti)
{
  LOG_W(RLC, "remove UE %x\n", rnti);
  nr_rlc_manager_lock(nr_rlc_ue_manager);
  nr_rlc_manager_remove_ue(nr_rlc_ue_manager, rnti);
  nr_rlc_manager_unlock(nr_rlc_ue_manager);
}

rlc_op_status_t rrc_rlc_remove_ue (const protocol_ctxt_t* const x)
{
  nr_rlc_remove_ue(x->rntiMaybeUEid);
  return RLC_OP_STATUS_OK;
}

bool nr_rlc_update_rnti(int from_rnti, int to_rnti)
{
  nr_rlc_manager_lock(nr_rlc_ue_manager);
  nr_rlc_ue_t *ue = nr_rlc_manager_get_ue(nr_rlc_ue_manager, from_rnti);
  if (ue == NULL) {
    nr_rlc_manager_unlock(nr_rlc_ue_manager);
    LOG_E(RLC, "Cannot find RLC entity for UE %04x\n", from_rnti);
    return false;
  }
  ue->rnti = to_rnti;
  LOG_I(RLC, "Update old UE RNTI %04x context to RNTI %04x\n", from_rnti, to_rnti);
  for (int i = 0; i < sizeof(ue->srb) / sizeof(ue->srb[0]); ++i)
    if (ue->srb[i])
      ue->srb[i]->reestablishment(ue->srb[i]);
  for (int i = 0; i < sizeof(ue->drb) / sizeof(ue->drb[0]); ++i)
    if (ue->drb[i])
      ue->drb[i]->reestablishment(ue->drb[i]);
  nr_rlc_manager_unlock(nr_rlc_ue_manager);
  return true;
}

/* This function is for testing purposes. At least on a COTS UE, it will
 * trigger a reestablishment. */
void nr_rlc_test_trigger_reestablishment(int rnti)
{
  nr_rlc_manager_lock(nr_rlc_ue_manager);
  nr_rlc_ue_t *ue = nr_rlc_manager_get_ue(nr_rlc_ue_manager, rnti);
  if (ue == NULL) {
    nr_rlc_manager_unlock(nr_rlc_ue_manager);
    LOG_E(RLC, "Cannot find RLC entity for UE %04x\n", rnti);
    return;
  }
  /* we simply assume the SRB exists, because the scheduler creates it as soon
   * as the UE context is created. */
  nr_rlc_entity_t *ent = ue->srb[0];
  ent->reestablishment(ent);
  nr_rlc_manager_unlock(nr_rlc_ue_manager);
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
 * TODO: remove it. The solution is to cleanup CMakeLists.txt
 */
void rlc_tick(int a, int b)
{
  LOG_E(RLC, "%s:%d:%s: this code should not be reached\n",
        __FILE__, __LINE__, __FUNCTION__);
  exit(1);
}

void nr_rlc_activate_avg_time_to_tx(
  const rnti_t            rnti,
  const logical_chan_id_t channel_id,
  const bool              is_on)
{
  nr_rlc_ue_t *ue;
  nr_rlc_entity_t *rb;

  nr_rlc_manager_lock(nr_rlc_ue_manager);
  ue = nr_rlc_manager_get_ue(nr_rlc_ue_manager, rnti);

  switch (channel_id) {
  case 1 ... 3: rb = ue->srb[channel_id - 1]; break;
  case 4 ... 8: rb = ue->drb[channel_id - 4]; break;
  default:      rb = NULL;                    break;
  }

  if (rb != NULL) {
    rb->avg_time_is_on = is_on;
    time_average_reset(rb->txsdu_avg_time_to_tx);
  } else {
    LOG_E(RLC, "[%s] Radio Bearer (channel ID %d) is NULL for UE with rnti %x\n", __FUNCTION__, channel_id, rnti);
  }

  nr_rlc_manager_unlock(nr_rlc_ue_manager);
}

/* returns false in case of error, true if everything ok */
bool nr_rlc_get_statistics(
  int rnti,
  int srb_flag,
  int rb_id,
  nr_rlc_statistics_t *out)
{
  nr_rlc_ue_t     *ue;
  nr_rlc_entity_t *rb;
  bool             ret;

  nr_rlc_manager_lock(nr_rlc_ue_manager);
  ue = nr_rlc_manager_get_ue(nr_rlc_ue_manager, rnti);

  rb = NULL;

  if (srb_flag) {
    if (rb_id >= 1 && rb_id <= 2)
      rb = ue->srb[rb_id - 1];
  } else {
    if (rb_id >= 1 && rb_id <= 5)
      rb = ue->drb[rb_id - 1];
  }

  if (rb != NULL) {
    rb->get_stats(rb, out);
    ret = true;

    // Patch buffer status using OAI results (no need to change anything in the RB)
    // rb->set_time(rb, nr_rlc_current_time);
    nr_rlc_entity_buffer_status_t oai_stat = rb->buffer_status(rb, 1000*1000);
    out->rxbuf_occ_bytes = oai_stat.status_size;
    out->txbuf_occ_bytes = oai_stat.tx_size + oai_stat.retx_size;
  } else {
    ret = false;
  }

  nr_rlc_manager_unlock(nr_rlc_ue_manager);

  return ret;
}

void nr_rlc_srb_recv_sdu(const int rnti, const logical_chan_id_t channel_id, unsigned char *buf, int size)
{
  nr_rlc_ue_t *ue;
  nr_rlc_entity_t *rb;

  T(T_ENB_RLC_DL, T_INT(0), T_INT(rnti), T_INT(0), T_INT(size));

  nr_rlc_manager_lock(nr_rlc_ue_manager);
  ue = nr_rlc_manager_get_ue(nr_rlc_ue_manager, rnti);

  if (channel_id == 0) {
    rb = ue->srb0;
  } else {
    rb = ue->srb[channel_id - 1];
  }

  AssertFatal(rb != NULL, "SDU sent to unknown RB RNTI %04x SRB %d\n", rnti, channel_id);

  rb->set_time(rb, nr_rlc_current_time);
  rb->recv_sdu(rb, (char *)buf, size, -1);

  nr_rlc_manager_unlock(nr_rlc_ue_manager);
}
