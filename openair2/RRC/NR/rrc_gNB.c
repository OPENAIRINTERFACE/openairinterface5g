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

/*! \file rrc_gNB.c
 * \brief rrc procedures for gNB
 * \author Navid Nikaein and  Raymond Knopp , WEI-TAI CHEN
 * \date 2011 - 2014 , 2018
 * \version 1.0
 * \company Eurecom, NTUST
 * \email: navid.nikaein@eurecom.fr and raymond.knopp@eurecom.fr, kroempa@gmail.com
 */
#define RRC_GNB_C
#define RRC_GNB_C

#include <netinet/in.h>
#include <netinet/sctp.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "5g_platform_types.h"
#include "openair2/RRC/NR/nr_rrc_proto.h"
#include "openair2/RRC/NR/rrc_gNB_UE_context.h"
#include "openair3/SECU/key_nas_deriver.h"
#include "openair3/ocp-gtpu/gtp_itf.h"
#include "BIT_STRING.h"
#include "F1AP_CauseProtocol.h"
#include "F1AP_CauseRadioNetwork.h"
#include "NGAP_CauseRadioNetwork.h"
#include "openair2/LAYER2/NR_MAC_COMMON/nr_mac.h"
#include "OCTET_STRING.h"
#include "RRC/NR/MESSAGES/asn1_msg.h"
#include "RRC/NR/mac_rrc_dl.h"
#include "RRC/NR/nr_rrc_common.h"
#include "SIMULATION/TOOLS/sim.h"
#include "T.h"
#include "asn_codecs.h"
#include "asn_internal.h"
#include "assertions.h"
#include "byte_array.h"
#include "common/ngran_types.h"
#include "common/openairinterface5g_limits.h"
#include "common/platform_constants.h"
#include "common/ran_context.h"
#include "common_lib.h"
#include "constr_SEQUENCE.h"
#include "constr_TYPE.h"
#include "cucp_cuup_if.h"
#include "e1ap_messages_types.h"
#include "executables/softmodem-common.h"
#include "f1ap_messages_types.h"
#include "gtpv1_u_messages_types.h"
#include "intertask_interface.h"
#include "linear_alloc.h"
#include "ngap_messages_types.h"
#include "nr_pdcp/nr_pdcp_entity.h"
#include "nr_pdcp/nr_pdcp_oai_api.h"
#include "nr_rrc_defs.h"
#include "oai_asn1.h"
#include "openair2/F1AP/f1ap_common.h"
#include "openair2/F1AP/f1ap_ids.h"
#include "openair2/F1AP/lib/f1ap_rrc_message_transfer.h"
#include "openair2/F1AP/lib/f1ap_interface_management.h"
#include "openair2/F1AP/lib/f1ap_ue_context.h"
#include "rrc_gNB_NGAP.h"
#include "rrc_gNB_du.h"
#include "rrc_gNB_mobility.h"
#include "rrc_gNB_radio_bearers.h"
#include "rrc_messages_types.h"
#include "rrc_gNB_asn1.h"
#include "seq_arr.h"
#include "tree.h"
#include "uper_decoder.h"
#include "uper_encoder.h"
#include "utils.h"
#include "x2ap_messages_types.h"
#include "xer_encoder.h"
#include "E1AP/lib/e1ap_bearer_context_management.h"
#include "E1AP/lib/e1ap_interface_management.h"
#include "NR_DL-DCCH-Message.h"
#include "ds/byte_array.h"
#include "alg/find.h"
#include "NR_HandoverCommand.h"
#include "openair2/SDAP/nr_sdap/nr_sdap_configuration.h"

#ifdef E2_AGENT
#include "openair2/E2AP/RAN_FUNCTION/O-RAN/ran_func_rc_extern.h"
#define E2_AGENT_SIGNAL_DL_DCCH_RRC_MSG(BUF, LEN, ID)    \
  do {                                                   \
    byte_array_t buffer_ba = {.len = LEN};               \
    buffer_ba.buf = (uint8_t *)BUF;                      \
    signal_rrc_msg(DL_DCCH_NR_RRC_CLASS, ID, buffer_ba); \
  } while (0)
#endif

mui_t rrc_gNB_mui = 0;

/* Per-transaction max_delays counter to limit retry attempts */
#define MAX_DELAYS 100

/** @brief clone and re-enqueue an NGAP message after delaying
 * delays the ongoing transaction (in msg_p) by setting a timer to wait
 * 10ms; upon expiry, delivers to RRC, which sends the message to itself */
static void delay_transaction(MessageDef *msg_p, int wait_us)
{
  MessagesIds id = ITTI_MSG_ID(msg_p);
  AssertFatal(id == NGAP_PDUSESSION_SETUP_REQ || id == NGAP_PDUSESSION_RELEASE_COMMAND,
              "delay_transaction(): unsupported message id %d\n",
              id);

  MessageDef *new = itti_alloc_new_message(TASK_RRC_GNB, 0, id);

  // Copy only the specific message struct, not the entire union.
  // The union (msg_t) contains all message types and is much larger than
  // the allocated space (which is sized for the specific message type only).
  if (id == NGAP_PDUSESSION_SETUP_REQ) {
    NGAP_PDUSESSION_SETUP_REQ(new) = NGAP_PDUSESSION_SETUP_REQ(msg_p);
  } else if (id == NGAP_PDUSESSION_RELEASE_COMMAND) {
    NGAP_PDUSESSION_RELEASE_COMMAND(new) = NGAP_PDUSESSION_RELEASE_COMMAND(msg_p);
  }

  int instance = msg_p->ittiMsgHeader.originInstance;
  long timer_id;
  timer_setup(0, wait_us, TASK_RRC_GNB, instance, TIMER_ONE_SHOT, new, &timer_id);
}

static void reset_delayed_action(delayed_action_state_t *delayed_action)
{
  delayed_action->ongoing_transaction = false;
  delayed_action->max_delays = 0;
}

void init_delayed_action(delayed_action_state_t *delayed_action)
{
  delayed_action->ongoing_transaction = true;
  delayed_action->max_delays = MAX_DELAYS;
}

/* \brief checks if any transaction is ongoing for any xid of this UE */
static bool transaction_ongoing(const gNB_RRC_UE_t *UE)
{
  for (int xid = 0; xid < NR_RRC_TRANSACTION_IDENTIFIER_NUMBER; ++xid) {
    if (UE->xids[xid] != RRC_ACTION_NONE)
      return true;
  }
  return false;
}

/** @brief delay control: returns true if delayed, false if should proceed
 * This is a hack. We observed that with some UEs, PDU session requests might
 * come in quick succession, faster than the RRC reconfiguration for the PDU
 * session requests can be carried out (UE is doing reconfig, and second PDU
 * session request arrives). We don't have currently the means to "queue up"
 * these transactions, which would probably involve some rework of the RRC.
 * To still allow these requests to come in and succeed, we below check and delay transactions
 * for 10ms. However, to not accidentally end up in infinite loops, the
 * maximum number is capped on a per-UE basis as indicated in variable
 * max_delays_pdu_session. See commit 277f8da0 for more details. */
static bool rrc_delay_transaction(instance_t instance, MessageDef *msg_p)
{
  uint32_t cu_ue_id = 0;
  if (ITTI_MSG_ID(msg_p) == NGAP_PDUSESSION_SETUP_REQ) {
    cu_ue_id = NGAP_PDUSESSION_SETUP_REQ(msg_p).gNB_ue_ngap_id;
  } else if (ITTI_MSG_ID(msg_p) == NGAP_PDUSESSION_RELEASE_COMMAND) {
    cu_ue_id = NGAP_PDUSESSION_RELEASE_COMMAND(msg_p).gNB_ue_ngap_id;
  }
  AssertFatal(cu_ue_id > 0, "cu_ue_id not found in message %s\n", ITTI_MSG_NAME(msg_p));

  rrc_gNB_ue_context_t *ue_context_p = rrc_gNB_get_ue_context(RC.nrrrc[instance], cu_ue_id);
  DevAssert(ue_context_p);

  gNB_RRC_UE_t *UE = &ue_context_p->ue_context;
  bool delay = UE->delayed_action.ongoing_transaction && UE->delayed_action.max_delays > 0;
  /* Check if any PDU session action is ongoing */
  if (delay || transaction_ongoing(UE)) {
    int wait_us = 10000;
    LOG_I(NR_RRC,
          "UE %d: ongoing transaction, delaying incoming transaction by %d us\n",
          UE->rrc_ue_id,
          wait_us);
    delay_transaction(msg_p, wait_us);
    UE->delayed_action.max_delays--;
    return true; /* delayed */
  }
  LOG_D(NR_RRC, "UE %d: no delayed action ongoing, proceeding with incoming transaction\n", UE->rrc_ue_id);
  return false; /* not delayed */
}

typedef struct deliver_ue_ctxt_release_data_t {
  gNB_RRC_INST *rrc;
  f1ap_ue_context_rel_cmd_t *release_cmd;
  sctp_assoc_t assoc_id;
} deliver_ue_ctxt_release_data_t;

const NR_RedCapParameters_r17_t *get_redcapparam_r17(NR_UE_NR_Capability_t *UE_Capability_nr)
{
  if (UE_Capability_nr && UE_Capability_nr->nonCriticalExtension && UE_Capability_nr->nonCriticalExtension->nonCriticalExtension
      && UE_Capability_nr->nonCriticalExtension->nonCriticalExtension->nonCriticalExtension
      && UE_Capability_nr->nonCriticalExtension->nonCriticalExtension->nonCriticalExtension->nonCriticalExtension
      && UE_Capability_nr->nonCriticalExtension->nonCriticalExtension->nonCriticalExtension->nonCriticalExtension
             ->nonCriticalExtension
      && UE_Capability_nr->nonCriticalExtension->nonCriticalExtension->nonCriticalExtension->nonCriticalExtension
             ->nonCriticalExtension->nonCriticalExtension
      && UE_Capability_nr->nonCriticalExtension->nonCriticalExtension->nonCriticalExtension->nonCriticalExtension
             ->nonCriticalExtension->nonCriticalExtension->nonCriticalExtension
      && UE_Capability_nr->nonCriticalExtension->nonCriticalExtension->nonCriticalExtension->nonCriticalExtension
             ->nonCriticalExtension->nonCriticalExtension->nonCriticalExtension->nonCriticalExtension
      && UE_Capability_nr->nonCriticalExtension->nonCriticalExtension->nonCriticalExtension->nonCriticalExtension
             ->nonCriticalExtension->nonCriticalExtension->nonCriticalExtension->nonCriticalExtension->nonCriticalExtension
      && UE_Capability_nr->nonCriticalExtension->nonCriticalExtension->nonCriticalExtension->nonCriticalExtension
             ->nonCriticalExtension->nonCriticalExtension->nonCriticalExtension->nonCriticalExtension->nonCriticalExtension
             ->nonCriticalExtension
      && UE_Capability_nr->nonCriticalExtension->nonCriticalExtension->nonCriticalExtension->nonCriticalExtension
             ->nonCriticalExtension->nonCriticalExtension->nonCriticalExtension->nonCriticalExtension->nonCriticalExtension
             ->nonCriticalExtension->redCapParameters_r17) {
    return UE_Capability_nr->nonCriticalExtension->nonCriticalExtension->nonCriticalExtension->nonCriticalExtension
        ->nonCriticalExtension->nonCriticalExtension->nonCriticalExtension->nonCriticalExtension->nonCriticalExtension
        ->nonCriticalExtension->redCapParameters_r17;
  } else
    return NULL;
}

static void rrc_deliver_ue_ctxt_release_cmd(void *deliver_pdu_data, ue_id_t ue_id, int srb_id, char *buf, int size, int sdu_id)
{
  DevAssert(deliver_pdu_data != NULL);
  deliver_ue_ctxt_release_data_t *data = deliver_pdu_data;
  byte_array_t rrc_cont = {.buf = (uint8_t *)buf, .len = size};
  data->release_cmd->rrc_container = &rrc_cont;
  data->rrc->mac_rrc.ue_context_release_command(data->assoc_id, data->release_cmd);
}

static bool eq_cell_id(const void *vval, const void *vit)
{
  const int *cell_id = (const int *)vval;
  const neighbour_cell_configuration_t *cell = (const neighbour_cell_configuration_t *)vit;
  return cell->nr_cell_id == *cell_id;
}

const neighbour_cell_configuration_t *get_neighbour_cell_config(const gNB_RRC_INST *rrc, int cell_id)
{
  if (!rrc->neighbour_cell_configuration)
    return NULL;
  seq_arr_t *head = rrc->neighbour_cell_configuration;
  LOG_D(NR_RRC, "Number of neighbour cell configurations: %ld\n", head->size);
  elm_arr_t e = find_if(head, &cell_id, eq_cell_id);
  if (e.found) {
    const neighbour_cell_configuration_t *cell = (const neighbour_cell_configuration_t *)e.it;
    LOG_D(NR_RRC, "Found matching neighbour cell with Cell ID %ld\n", cell->nr_cell_id);
    return cell;
  }
  return NULL;
}

static bool eq_pci(const void *vval, const void *vit)
{
  const int *pci = (const int *)vval;
  const nr_neighbour_cell_t *neighbour = (const nr_neighbour_cell_t *)vit;
  return neighbour->physicalCellId == *pci;
}

const nr_neighbour_cell_t *get_neighbour_cell_by_pci(const neighbour_cell_configuration_t *cell, int pci)
{
  seq_arr_t *head = cell->neighbour_cells;
  DevAssert(head != NULL);
  LOG_D(NR_RRC, "Number of neighbour cells: %ld\n", head->size);
  elm_arr_t e = find_if(head, &pci, eq_pci);
  if (e.found) {
    const nr_neighbour_cell_t *neighbour = (const nr_neighbour_cell_t *)e.it;
    LOG_D(NR_RRC, "Found matching neighbour cell with PCI %d and Cell ID %ld\n", neighbour->physicalCellId, neighbour->nrcell_id);
    return neighbour;
  }
  LOG_E(NR_RRC, "No matching neighbour cell found for Physical Cell ID: %d\n", pci);
  return NULL;
}

typedef struct deliver_dl_rrc_message_data_s {
  const gNB_RRC_INST *rrc;
  f1ap_dl_rrc_message_t *dl_rrc;
  sctp_assoc_t assoc_id;
} deliver_dl_rrc_message_data_t;
static void rrc_deliver_dl_rrc_message(void *deliver_pdu_data, ue_id_t ue_id, int srb_id, char *buf, int size, int sdu_id)
{
  DevAssert(deliver_pdu_data != NULL);
  deliver_dl_rrc_message_data_t *data = (deliver_dl_rrc_message_data_t *)deliver_pdu_data;
  data->dl_rrc->rrc_container = (uint8_t *)buf;
  data->dl_rrc->rrc_container_length = size;
  DevAssert(data->dl_rrc->srb_id == srb_id);
  data->rrc->mac_rrc.dl_rrc_message_transfer(data->assoc_id, data->dl_rrc);
}

static void nr_rrc_transfer_protected_rrc_message(const gNB_RRC_INST *rrc,
                                                  const gNB_RRC_UE_t *ue_p,
                                                  uint8_t srb_id,
                                                  const uint32_t message_id,
                                                  const uint8_t *buffer,
                                                  int size)
{
  DevAssert(size > 0);
  f1_ue_data_t ue_data = cu_get_f1_ue_data(ue_p->rrc_ue_id);
  RETURN_IF_INVALID_ASSOC_ID(ue_data.du_assoc_id);
  f1ap_dl_rrc_message_t dl_rrc = {.gNB_CU_ue_id = ue_p->rrc_ue_id, .gNB_DU_ue_id = ue_data.secondary_ue, .srb_id = srb_id};
  deliver_dl_rrc_message_data_t data = {.rrc = rrc, .dl_rrc = &dl_rrc, .assoc_id = ue_data.du_assoc_id};
  nr_pdcp_data_req_srb(ue_p->rrc_ue_id,
                       srb_id,
                       rrc_gNB_mui++,
                       size,
                       (unsigned char *const)buffer,
                       rrc_deliver_dl_rrc_message,
                       &data);

#ifdef E2_AGENT
  E2_AGENT_SIGNAL_DL_DCCH_RRC_MSG(buffer, size, message_id);
#endif
}

static void rrc_gNB_CU_DU_init(gNB_RRC_INST *rrc)
{
  switch (rrc->node_type) {
    case ngran_gNB_CUCP:
      mac_rrc_dl_f1ap_init(&rrc->mac_rrc);
      cucp_cuup_message_transfer_e1ap_init(rrc);
      break;
    case ngran_gNB_CU:
      mac_rrc_dl_f1ap_init(&rrc->mac_rrc);
      cucp_cuup_message_transfer_direct_init(rrc);
      break;
    case ngran_gNB:
      mac_rrc_dl_direct_init(&rrc->mac_rrc);
      cucp_cuup_message_transfer_direct_init(rrc);
       break;
    case ngran_gNB_DU:
      /* silently drop this, as we currently still need the RRC at the DU. As
       * soon as this is not the case anymore, we can add the AssertFatal() */
      //AssertFatal(1==0,"nothing to do for DU\n");
      break;
    default:
      AssertFatal(0 == 1, "Unknown node type %d\n", rrc->node_type);
      break;
  }
  cu_init_f1_ue_data();
}

void openair_rrc_gNB_configuration(gNB_RRC_INST *rrc, gNB_RrcConfigurationReq *configuration)
{
  AssertFatal(rrc != NULL, "RC.nrrrc not initialized!");
  AssertFatal(MAX_MOBILES_PER_GNB < (module_id_t)0xFFFFFFFFFFFFFFFF, " variable overflow");
  AssertFatal(configuration!=NULL,"configuration input is null\n");
  rrc->module_id = 0;
  rrc_gNB_CU_DU_init(rrc);
  uid_linear_allocator_init(&rrc->uid_allocator);
  RB_INIT(&rrc->rrc_ue_head);
  RB_INIT(&rrc->cuups);
  RB_INIT(&rrc->dus);
  rrc->configuration = *configuration;
}

static void rrc_gNB_process_AdditionRequestInformation(const module_id_t gnb_mod_idP, x2ap_ENDC_sgnb_addition_req_t *m)
{
  gNB_RRC_INST *rrc = RC.nrrrc[gnb_mod_idP];
  AssertFatal(NODE_IS_MONOLITHIC(rrc->node_type), "NSA, phy_test, and do_ra only work in monolithic\n");
  sctp_assoc_t assoc_id = -1; // monolithic gNB
  rrc_add_nsa_user(rrc, m, assoc_id);
}

//-----------------------------------------------------------------------------
unsigned int rrc_gNB_get_next_transaction_identifier(module_id_t gnb_mod_idP)
//-----------------------------------------------------------------------------
{
  static unsigned int transaction_id[NUMBER_OF_gNB_MAX] = {0};
  // used also in NGAP thread, so need thread safe operation
  unsigned int tmp = __atomic_add_fetch(&transaction_id[gnb_mod_idP], 1, __ATOMIC_SEQ_CST);
  tmp %= NR_RRC_TRANSACTION_IDENTIFIER_NUMBER;
  LOG_T(NR_RRC, "generated xid is %d\n", tmp);
  return tmp;
}

/** @brief Create srb-ToAddModList for RRCSetup and RRCReconfiguration messages
  * @param reestablish bitmap to indicates whether PDCP should be re-established
  * for the SRB1 and/or SRB2. For convenience the bitmap is 0-based, with index 1
  * corresponding to SRB1, index 2 to SRB2. 3GPP TS 38.331 RadioBearerConfig
  * specifies that PDCP shall be re-established whenever the security key used
  * for the radio bearer changes, with some expections for SRB1 (i.e. when resuming
  * an RRC connection, or at the first reconfiguration after RRC connection
  * reestablishment in NR, do not re-establish PDCP) */
NR_SRB_ToAddModList_t *createSRBlist(gNB_RRC_UE_t *ue, uint8_t reestablish)
{
  if (!ue->Srb[SRB1].Active) {
    LOG_E(NR_RRC, "Call SRB list while SRB1 doesn't exist\n");
    return NULL;
  }
  NR_SRB_ToAddModList_t *list = CALLOC(sizeof(*list), 1);
  for (int i = 0; i < NR_NUM_SRB; i++)
    if (ue->Srb[i].Active) {
      asn1cSequenceAdd(list->list, NR_SRB_ToAddMod_t, srb);
      srb->srb_Identity = i;
      /* Based on the bitmap, set reestablishPDCP for SRB1 and SRB2 */
      if ((i == SRB1 || i == SRB2) && (reestablish & (1 << i))) {
        asn1cCallocOne(srb->reestablishPDCP, NR_SRB_ToAddMod__reestablishPDCP_true);
      } else {
        DevAssert(!(reestablish & (1 << i)));
      }
    }
  return list;
}

static NR_SDAP_Config_t *nr_rrc_build_sdap_config_ie(const int pdusession_id,
                                                     uint8_t n_flows,
                                                     pdusession_level_qos_parameter_t *qos,
                                                     const nr_sdap_configuration_t *sdap_config)
{
  DevAssert(n_flows < MAX_QOS_FLOWS);
  // SDAP
  NR_SDAP_Config_t *sc = calloc_or_fail(1, sizeof(*sc));
  sc->defaultDRB = true;
  sc->pdu_Session = pdusession_id;
  sc->sdap_HeaderDL = sdap_config->header_dl_absent ? NR_SDAP_Config__sdap_HeaderDL_absent : NR_SDAP_Config__sdap_HeaderDL_present;
  sc->sdap_HeaderUL = sdap_config->header_ul_absent ? NR_SDAP_Config__sdap_HeaderUL_absent : NR_SDAP_Config__sdap_HeaderUL_present;
  // QoS
  asn1cCalloc(sc->mappedQoS_FlowsToAdd, mappedQoS_FlowsToAdd);
  for (uint8_t i = 0; i < n_flows; ++i) {
    DevAssert(i < MAX_QOS_FLOWS);
    NR_QFI_t *qfi = calloc_or_fail(1, sizeof(*qfi));
    *qfi = qos[i].qfi;
    LOG_D(NR_RRC, "Adding QFI %ld to PDU Session %d\n", *qfi, pdusession_id);
    asn1cSeqAdd(&mappedQoS_FlowsToAdd->list, qfi);
  }
  return sc;
}

NR_DRB_ToAddModList_t *createDRBlist(gNB_RRC_UE_t *ue, bool reestablish, bool do_integrity, bool do_ciphering)
{
  NR_DRB_ToAddModList_t *DRB_configList = CALLOC(sizeof(*DRB_configList), 1);

  FOR_EACH_SEQ_ARR(drb_t *, drb, &ue->drbs) {
    rrc_pdu_session_param_t *pduSession = find_pduSession(&ue->pduSessions, drb->pdusession_id);
    if (!pduSession) {
      LOG_D(NR_RRC, "PDU Session %d not found, skip\n", drb->pdusession_id);
      continue;
    }
    if (pduSession->status > PDU_SESSION_STATUS_TOMODIFY) {
      LOG_D(NR_RRC, "PDU Session %d is not to add/mod\n", drb->pdusession_id);
      continue;
    }
    pdusession_t *session = &pduSession->param;
    NR_DRB_ToAddMod_t *drb_ToAddMod = calloc_or_fail(1, sizeof(*drb_ToAddMod));
    drb_ToAddMod->drb_Identity = drb->drb_id;
    // PDCP config
    drb_ToAddMod->pdcp_Config = nr_rrc_build_pdcp_config_ie(do_integrity, do_ciphering, &drb->pdcp_config);
    if (reestablish) {
      asn1cCallocOne(drb_ToAddMod->reestablishPDCP, NR_DRB_ToAddMod__reestablishPDCP_true);
    }
    // cn-association: SDAP config
    // Get all QoS flows mapped to this DRB
    pdusession_level_qos_parameter_t flows_to_add[MAX_QOS_FLOWS] = {0};
    uint8_t n_flows = 0;
    FOR_EACH_SEQ_ARR(nr_rrc_qos_t *, q, &session->qos)
    {
      if (q->drb_id == drb->drb_id) {
        DevAssert(n_flows < MAX_QOS_FLOWS);
        flows_to_add[n_flows++] = q->qos;
      }
    }
    asn1cCalloc(drb_ToAddMod->cnAssociation, cn_association);
    cn_association->present = NR_DRB_ToAddMod__cnAssociation_PR_sdap_Config;
    cn_association->choice.sdap_Config = nr_rrc_build_sdap_config_ie(drb->pdusession_id, n_flows, flows_to_add, &session->sdap_config);
    asn1cSeqAdd(&DRB_configList->list, drb_ToAddMod);
  }
  if (DRB_configList->list.count == 0) {
    free(DRB_configList);
    return NULL;
  }
  return DRB_configList;
}

void freeSRBlist(NR_SRB_ToAddModList_t *l)
{
  ASN_STRUCT_FREE(asn_DEF_NR_SRB_ToAddModList, l);
}

void activate_srb(gNB_RRC_UE_t *UE, int srb_id)
{
  AssertFatal(srb_id == 1 || srb_id == 2, "handling only SRB 1 or 2\n");
  if (UE->Srb[srb_id].Active == 1) {
    LOG_W(RRC, "UE %d SRB %d already activated\n", UE->rrc_ue_id, srb_id);
    return;
  }
  LOG_I(RRC, "activate SRB %d of UE %d\n", srb_id, UE->rrc_ue_id);
  UE->Srb[srb_id].Active = 1;

  NR_SRB_ToAddModList_t *list = CALLOC(sizeof(*list), 1);
  asn1cSequenceAdd(list->list, NR_SRB_ToAddMod_t, srb);
  srb->srb_Identity = srb_id;

  if (srb_id == 1) {
    nr_pdcp_entity_security_keys_and_algos_t null_security_parameters = {0};
    nr_pdcp_add_srbs(true, UE->rrc_ue_id, list, &null_security_parameters);
  } else {
    nr_pdcp_entity_security_keys_and_algos_t security_parameters;
    security_parameters.ciphering_algorithm = UE->ciphering_algorithm;
    security_parameters.integrity_algorithm = UE->integrity_algorithm;
    nr_derive_key(RRC_ENC_ALG, UE->ciphering_algorithm, UE->kgnb, security_parameters.ciphering_key);
    nr_derive_key(RRC_INT_ALG, UE->integrity_algorithm, UE->kgnb, security_parameters.integrity_key);

    nr_pdcp_add_srbs(true,
                     UE->rrc_ue_id,
                     list,
                     &security_parameters);
  }
  freeSRBlist(list);
}

//-----------------------------------------------------------------------------
static void rrc_gNB_generate_RRCSetup(instance_t instance,
                                      rnti_t rnti,
                                      rrc_gNB_ue_context_t *const ue_context_pP,
                                      const uint8_t *masterCellGroup,
                                      int masterCellGroup_len)
//-----------------------------------------------------------------------------
{
  LOG_UE_DL_EVENT(&ue_context_pP->ue_context, "Send RRC Setup\n");

  gNB_RRC_UE_t *ue_p = &ue_context_pP->ue_context;
  gNB_RRC_INST *rrc = RC.nrrrc[instance];
  unsigned char buf[1024];
  uint8_t xid = rrc_gNB_get_next_transaction_identifier(instance);
  ue_p->xids[xid] = RRC_SETUP;
  NR_SRB_ToAddModList_t *SRBs = createSRBlist(ue_p, false);

  int size = do_RRCSetup(buf, sizeof(buf), xid, masterCellGroup, masterCellGroup_len, &rrc->configuration, SRBs);
  AssertFatal(size > 0, "do_RRCSetup failed\n");
  AssertFatal(size <= 1024, "memory corruption\n");

  LOG_DUMPMSG(NR_RRC, DEBUG_RRC, (char *)buf, size, "[MSG] RRC Setup\n");
  freeSRBlist(SRBs);
  f1_ue_data_t ue_data = cu_get_f1_ue_data(ue_p->rrc_ue_id);
  RETURN_IF_INVALID_ASSOC_ID(ue_data.du_assoc_id);
  int srbid = 0;
  f1ap_dl_rrc_message_t dl_rrc = {
    .gNB_CU_ue_id = ue_p->rrc_ue_id,
    .gNB_DU_ue_id = ue_data.secondary_ue,
    .rrc_container = buf,
    .rrc_container_length = size,
    .srb_id = srbid
  };
  rrc->mac_rrc.dl_rrc_message_transfer(ue_data.du_assoc_id, &dl_rrc);
}

static void rrc_gNB_generate_RRCReject(gNB_RRC_INST *rrc, rrc_gNB_ue_context_t *const ue_context_pP)
//-----------------------------------------------------------------------------
{
  gNB_RRC_UE_t *ue_p = &ue_context_pP->ue_context;
  LOG_A(NR_RRC, "Send RRCReject to RNTI %04x\n", ue_p->rnti);

  unsigned char buf[1024];
  int size = do_RRCReject(buf);
  AssertFatal(size > 0, "do_RRCReject failed\n");
  AssertFatal(size <= 1024, "memory corruption\n");

  LOG_DUMPMSG(NR_RRC, DEBUG_RRC,
              (char *)buf,
              size,
              "[MSG] RRCReject \n");
  LOG_I(NR_RRC, " [RAPROC] ue %04x Logical Channel DL-CCCH, Generating NR_RRCReject (bytes %d)\n", ue_p->rnti, size);

  f1_ue_data_t ue_data = cu_get_f1_ue_data(ue_p->rrc_ue_id);
  RETURN_IF_INVALID_ASSOC_ID(ue_data.du_assoc_id);
  int srbid = 0;
  f1ap_dl_rrc_message_t dl_rrc = {
    .gNB_CU_ue_id = ue_p->rrc_ue_id,
    .gNB_DU_ue_id = ue_data.secondary_ue,
    .rrc_container = buf,
    .rrc_container_length = size,
    .srb_id = srbid,
  };
  rrc->mac_rrc.dl_rrc_message_transfer(ue_data.du_assoc_id, &dl_rrc);
  /* release the created UE context, we rejected the UE */
  rrc_remove_ue(rrc, ue_context_pP);
}

//-----------------------------------------------------------------------------
/*
* Process the rrc setup complete message from UE (SRB1 Active)
*/
static void rrc_gNB_process_RRCSetupComplete(gNB_RRC_INST *rrc, gNB_RRC_UE_t *UE, NR_RRCSetupComplete_IEs_t *rrcSetupComplete)
//-----------------------------------------------------------------------------
{
  UE->Srb[1].Active = 1;
  UE->Srb[2].Active = 0;

  rrc_gNB_send_NGAP_NAS_FIRST_REQ(rrc, UE, rrcSetupComplete);
}

static nr_a3_event_t *get_a3_configuration(gNB_RRC_INST *rrc, int pci)
{
  nr_measurement_configuration_t *measurementConfiguration = &rrc->measurementConfiguration;
  if (!measurementConfiguration->a3_event_list)
    return NULL;

  for (uint8_t i = 0; i < measurementConfiguration->a3_event_list->size; i++) {
    nr_a3_event_t *a3_event = (nr_a3_event_t *)seq_arr_at(measurementConfiguration->a3_event_list, i);
    if (a3_event->pci == pci)
      return a3_event;
  }

  if (measurementConfiguration->is_default_a3_configuration_exists)
    return get_a3_configuration(rrc, -1);

  return NULL;
}

static NR_ReportConfigToAddMod_t *prepare_periodic_event_report(const nr_per_event_t *per_event)
{
  NR_ReportConfigToAddMod_t *rc = calloc(1, sizeof(*rc));
  rc->reportConfigId = 1;
  rc->reportConfig.present = NR_ReportConfigToAddMod__reportConfig_PR_reportConfigNR;

  NR_PeriodicalReportConfig_t *prc = calloc(1, sizeof(*prc));
  prc->rsType = NR_NR_RS_Type_ssb;
  prc->reportInterval = NR_ReportInterval_ms1024;
  prc->reportAmount = NR_PeriodicalReportConfig__reportAmount_infinity;
  prc->reportQuantityCell.rsrp = true;
  prc->reportQuantityCell.rsrq = true;
  prc->reportQuantityCell.sinr = true;
  prc->reportQuantityRS_Indexes = calloc(1, sizeof(*prc->reportQuantityRS_Indexes));
  prc->reportQuantityRS_Indexes->rsrp = true;
  prc->reportQuantityRS_Indexes->rsrq = true;
  prc->reportQuantityRS_Indexes->sinr = true;
  asn1cCallocOne(prc->maxNrofRS_IndexesToReport, per_event->maxReportCells);
  prc->maxReportCells = per_event->maxReportCells;
  prc->includeBeamMeasurements = per_event->includeBeamMeasurements;

  NR_ReportConfigNR_t *rcnr = calloc(1, sizeof(*rcnr));
  rcnr->reportType.present = NR_ReportConfigNR__reportType_PR_periodical;
  rcnr->reportType.choice.periodical = prc;

  rc->reportConfig.choice.reportConfigNR = rcnr;
  return rc;
}

static NR_ReportConfigToAddMod_t *prepare_a2_event_report(const nr_a2_event_t *a2_event)
{
  NR_ReportConfigToAddMod_t *rc_A2 = calloc(1, sizeof(*rc_A2));
  rc_A2->reportConfigId = 2;
  rc_A2->reportConfig.present = NR_ReportConfigToAddMod__reportConfig_PR_reportConfigNR;
  NR_EventTriggerConfig_t *etrc_A2 = calloc(1, sizeof(*etrc_A2));
  etrc_A2->eventId.present = NR_EventTriggerConfig__eventId_PR_eventA2;
  etrc_A2->eventId.choice.eventA2 = calloc(1, sizeof(*etrc_A2->eventId.choice.eventA2));
  etrc_A2->eventId.choice.eventA2->a2_Threshold.present = NR_MeasTriggerQuantity_PR_rsrp;
  etrc_A2->eventId.choice.eventA2->a2_Threshold.choice.rsrp = a2_event->threshold_RSRP;
  etrc_A2->eventId.choice.eventA2->reportOnLeave = false;
  etrc_A2->eventId.choice.eventA2->hysteresis = 0;
  etrc_A2->eventId.choice.eventA2->timeToTrigger = a2_event->timeToTrigger;
  etrc_A2->rsType = NR_NR_RS_Type_ssb;
  etrc_A2->reportInterval = NR_ReportInterval_ms480;
  etrc_A2->reportAmount = NR_EventTriggerConfig__reportAmount_r4;
  etrc_A2->reportQuantityCell.rsrp = true;
  etrc_A2->reportQuantityCell.rsrq = true;
  etrc_A2->reportQuantityCell.sinr = true;
  asn1cCallocOne(etrc_A2->maxNrofRS_IndexesToReport, 4);
  etrc_A2->maxReportCells = 1;
  etrc_A2->includeBeamMeasurements = false;
  NR_ReportConfigNR_t *rcnr_A2 = calloc(1, sizeof(*rcnr_A2));
  rcnr_A2->reportType.present = NR_ReportConfigNR__reportType_PR_eventTriggered;
  rcnr_A2->reportType.choice.eventTriggered = etrc_A2;
  rc_A2->reportConfig.choice.reportConfigNR = rcnr_A2;

  return rc_A2;
}

static NR_ReportConfigToAddMod_t *prepare_a3_event_report(const nr_a3_event_t *a3_event, NR_ReportConfigId_t reportConfigId)
{
  NR_ReportConfigToAddMod_t *rc_A3 = calloc(1, sizeof(*rc_A3));
  // 3 is default A3 Report Config ID. So cellId(0) specific Report Config ID
  // starts from 4
  rc_A3->reportConfigId = reportConfigId;
  rc_A3->reportConfig.present = NR_ReportConfigToAddMod__reportConfig_PR_reportConfigNR;
  NR_EventTriggerConfig_t *etrc_A3 = calloc(1, sizeof(*etrc_A3));
  etrc_A3->eventId.present = NR_EventTriggerConfig__eventId_PR_eventA3;
  etrc_A3->eventId.choice.eventA3 = calloc(1, sizeof(*etrc_A3->eventId.choice.eventA3));
  etrc_A3->eventId.choice.eventA3->a3_Offset.present = NR_MeasTriggerQuantityOffset_PR_rsrp;
  etrc_A3->eventId.choice.eventA3->a3_Offset.choice.rsrp = a3_event->a3_offset;
  etrc_A3->eventId.choice.eventA3->reportOnLeave = true;
  etrc_A3->eventId.choice.eventA3->hysteresis = a3_event->hysteresis;
  etrc_A3->eventId.choice.eventA3->timeToTrigger = a3_event->timeToTrigger;
  etrc_A3->rsType = NR_NR_RS_Type_ssb;
  etrc_A3->reportInterval = NR_ReportInterval_ms1024;
  etrc_A3->reportAmount = NR_EventTriggerConfig__reportAmount_r4;
  etrc_A3->reportQuantityCell.rsrp = true;
  etrc_A3->reportQuantityCell.rsrq = true;
  etrc_A3->reportQuantityCell.sinr = true;
  asn1cCallocOne(etrc_A3->maxNrofRS_IndexesToReport, 4);
  etrc_A3->maxReportCells = 4;
  etrc_A3->includeBeamMeasurements = false;
  NR_ReportConfigNR_t *rcnr_A3 = calloc(1, sizeof(*rcnr_A3));
  rcnr_A3->reportType.present = NR_ReportConfigNR__reportType_PR_eventTriggered;
  rcnr_A3->reportType.choice.eventTriggered = etrc_A3;
  rc_A3->reportConfig.choice.reportConfigNR = rcnr_A3;
  return rc_A3;
}

void free_RRCReconfiguration_params(nr_rrc_reconfig_param_t params)
{
  ASN_STRUCT_FREE(asn_DEF_NR_DRB_ToAddModList, params.drb_config_list);
  ASN_STRUCT_FREE(asn_DEF_NR_SRB_ToAddModList, params.srb_config_list);
  ASN_STRUCT_FREE(asn_DEF_NR_SecurityConfig, params.security_config);
  free(params.drb_rel);
  for (int i = 0; i < params.num_nas_msg; i++)
    FREE_AND_ZERO_BYTE_ARRAY(params.dedicated_NAS_msg_list[i]);
}

NR_MeasConfig_t *nr_rrc_get_measconfig(const gNB_RRC_INST *rrc, uint64_t nr_cellid)
{
  nr_rrc_du_container_t *du = get_du_by_cell_id((gNB_RRC_INST *)rrc, nr_cellid);
  DevAssert(du != NULL);
  f1ap_served_cell_info_t *cell_info = &du->setup_req->cell[0].info;

  if (du->mtc != NULL) {
    NR_ReportConfigToAddMod_t *rc_PER = NULL;
    NR_ReportConfigToAddMod_t *rc_A2 = NULL;
    seq_arr_t rc_A3_seq = {0};
    seq_arr_t neigh_seq = {0};
    seq_arr_init(&rc_A3_seq, sizeof(NR_ReportConfigToAddMod_t));
    seq_arr_init(&neigh_seq, sizeof(nr_neighbour_cell_t));
    int scs = get_ssb_scs(cell_info);
    int band = get_dl_band(cell_info);
    const NR_MeasTimingList_t *mtlist = du->mtc->criticalExtensions.choice.c1->choice.measTimingConf->measTiming;
    const NR_MeasTiming_t *mt = mtlist->list.array[0];
    const neighbour_cell_configuration_t *neighbour_config = get_neighbour_cell_config(rrc, cell_info->nr_cellid);
    seq_arr_t *neighbour_cells = NULL;
    if (neighbour_config)
      neighbour_cells = neighbour_config->neighbour_cells;

    if (neighbour_cells && rrc->measurementConfiguration.a3_event_list && rrc->measurementConfiguration.a3_event_list->size > 0) {
      /* Loop through neighbours and find related A3 configuration
         If no related A3 but there is default add the default one.
         If default one added once as a report, no need to add it again && duplication.
      */
      LOG_D(NR_RRC, "Preparing A3 Event Measurement Configuration!\n");
      bool default_a3_added = false; // To ensure that the default configuration is only added once
      for (int i = 0; i < neighbour_cells->size; i++) {
        nr_neighbour_cell_t *neighbourCell = (nr_neighbour_cell_t *)seq_arr_at(neighbour_cells, i);
        if (default_a3_added && neighbourCell->physicalCellId == -1)
          continue;
        seq_arr_push_back(&neigh_seq, neighbourCell, sizeof(nr_neighbour_cell_t));
        const nr_a3_event_t *a3Event = get_a3_configuration((gNB_RRC_INST *)rrc, neighbourCell->physicalCellId);
        if (a3Event) {
          NR_ReportConfigId_t reportConfigId = neighbourCell->physicalCellId == -1 ? 3 : i + 4;
          seq_arr_push_back(&rc_A3_seq, prepare_a3_event_report(a3Event, reportConfigId), sizeof(NR_ReportConfigToAddMod_t));
          if (neighbourCell->physicalCellId == -1)
            default_a3_added = true;
        }
      }
    }
    if (rrc->measurementConfiguration.per_event)
      rc_PER = prepare_periodic_event_report(rrc->measurementConfiguration.per_event);
    if (rrc->measurementConfiguration.a2_event)
      rc_A2 = prepare_a2_event_report(rrc->measurementConfiguration.a2_event);

    NR_MeasConfig_t *result = get_MeasConfig(mt, band, scs, cell_info->nr_pci, rc_PER, rc_A2, &rc_A3_seq, &neigh_seq);

    // Clean up sequence arrays
    seq_arr_free(&rc_A3_seq, NULL);
    seq_arr_free(&neigh_seq, NULL);

    return result;
  }
  return NULL;
}

/** @brief Prepare the instance of RRCReconfigurationParams to be pass to RRC encoding */
nr_rrc_reconfig_param_t get_RRCReconfiguration_params(gNB_RRC_INST *rrc, gNB_RRC_UE_t *UE, uint8_t srb_reest_bitmap, bool drb_reestablish)
{
  uint8_t xid = rrc_gNB_get_next_transaction_identifier(rrc->module_id);

  // Re-establish PDCP for SRB2 only
  bool do_integrity = rrc->security.do_drb_integrity;
  bool do_ciphering = rrc->security.do_drb_ciphering;
  NR_SRB_ToAddModList_t *SRBs = createSRBlist(UE, srb_reest_bitmap);
  NR_DRB_ToAddModList_t *DRBs = createDRBlist(UE, drb_reestablish, do_integrity, do_ciphering);

  nr_rrc_reconfig_param_t params = {.cell_group_config = UE->masterCellGroup,
                                    .transaction_id = xid,
                                    .drb_config_list = DRBs,
                                    .meas_config = UE->measConfig,
                                    .srb_config_list = SRBs};

  FOR_EACH_SEQ_ARR(rrc_pdu_session_param_t *, item, &UE->pduSessions) {
    pdusession_t *session = &item->param;
    // Collect NAS PDUs
    if (session->nas_pdu.len > 0) {
      params.dedicated_NAS_msg_list[params.num_nas_msg++] = session->nas_pdu;
      session->nas_pdu.buf = NULL;
      session->nas_pdu.len = 0;
      LOG_D(NR_RRC, "Transfer NAS info with size %ld to RRCReconfiguration params\n", session->nas_pdu.len);
    }
    // Collect DRBs to release for PDU sessions marked for release
    if (item->status == PDU_SESSION_STATUS_TORELEASE) {
      if (!params.drb_rel)
        params.drb_rel = calloc_or_fail(MAX_DRBS_PER_UE, sizeof(int));
      FOR_EACH_SEQ_ARR (drb_t *, drb, &UE->drbs) {
        if (drb->pdusession_id == session->pdusession_id) {
          if (params.n_drb_rel >= MAX_DRBS_PER_UE) {
            LOG_E(NR_RRC, "UE %d: Too many DRBs to release (max %d)\n", UE->rrc_ue_id, MAX_DRBS_PER_UE);
            break;
          }
          params.drb_rel[params.n_drb_rel++] = drb->drb_id;
          LOG_D(NR_RRC,
                "UE %d: Added DRB %d to release list for PDU session %d\n",
                UE->rrc_ue_id,
                drb->drb_id,
                session->pdusession_id);
        }
      }
    }
  }

  if (UE->nas_pdu.len > 0) {
    params.dedicated_NAS_msg_list[params.num_nas_msg++] = UE->nas_pdu;
    UE->nas_pdu.buf = NULL;
    UE->nas_pdu.len = 0;
    LOG_D(NR_RRC, "Transfer NAS info with size %ld to RRCReconfiguration params\n", UE->nas_pdu.len);
  }

  return params;
}

byte_array_t rrc_gNB_encode_RRCReconfiguration(gNB_RRC_INST *rrc, gNB_RRC_UE_t *UE, nr_rrc_reconfig_param_t params)
{
  byte_array_t msg = do_RRCReconfiguration(&params);
  if (msg.len <= 0) {
    LOG_E(NR_RRC, "UE %d: Failed to generate RRCReconfiguration\n", UE->rrc_ue_id);
    return msg;
  }
  LOG_DUMPMSG(NR_RRC, DEBUG_RRC, msg.buf, msg.len, "[MSG] RRC Reconfiguration\n");
  return msg;
}

static void rrc_gNB_generate_dedicatedRRCReconfiguration(gNB_RRC_INST *rrc, gNB_RRC_UE_t *ue_p)
{
  /* do not re-establish PDCP for any bearer */
  nr_rrc_reconfig_param_t params = get_RRCReconfiguration_params(rrc, ue_p, 0, false);
  FOR_EACH_SEQ_ARR(rrc_pdu_session_param_t *, item, &ue_p->pduSessions) {
    if (item->status == PDU_SESSION_STATUS_NEW) {
      item->status = PDU_SESSION_STATUS_DONE;
      continue;
    }
    // Set xid for all PDU sessions
    item->xid = params.transaction_id;
  }

  // Set xid for RRC transaction
  ue_p->xids[params.transaction_id] = params.n_drb_rel > 0 ? RRC_PDUSESSION_RELEASE : RRC_PDUSESSION_ESTABLISH;

  byte_array_t msg = rrc_gNB_encode_RRCReconfiguration(rrc, ue_p, params);
  if (msg.len <= 0) {
    LOG_E(NR_RRC, "UE %d: Failed to generate RRCReconfiguration\n", ue_p->rrc_ue_id);
    return;
  }

  LOG_UE_DL_EVENT(ue_p, "Generate RRCReconfiguration (bytes %ld, xid %d)\n", msg.len, params.transaction_id);
  const uint32_t msg_id = NR_DL_DCCH_MessageType__c1_PR_rrcReconfiguration;
  nr_rrc_transfer_protected_rrc_message(rrc, ue_p, DL_SCH_LCID_DCCH, msg_id, msg.buf, msg.len);
  free_RRCReconfiguration_params(params);
  free_byte_array(msg);
}

void rrc_gNB_modify_dedicatedRRCReconfiguration(gNB_RRC_INST *rrc, gNB_RRC_UE_t *ue_p)
{
  int qos_flow_index = 0;
  nr_rrc_reconfig_param_t params = get_RRCReconfiguration_params(rrc, ue_p, 0, false);
  ue_p->xids[params.transaction_id] = RRC_PDUSESSION_MODIFY;

  FOR_EACH_SEQ_ARR(rrc_pdu_session_param_t *, item, &ue_p->pduSessions) {
    pdusession_t *session = &item->param;
    // bypass the new and already configured pdu sessions
    if (item->status >= PDU_SESSION_STATUS_DONE) {
      item->xid = params.transaction_id;
      continue;
    }

    if (item->cause.type != NGAP_CAUSE_NOTHING) {
      // set xid of failure pdu session
      item->xid = params.transaction_id;
      item->status = PDU_SESSION_STATUS_FAILED;
      continue;
    }

    // Reference TS23501 Table 5.7.4-1: Standardized 5QI to QoS characteristics mapping
    FOR_EACH_SEQ_ARR(nr_rrc_qos_t *, qos, &session->qos) {
      switch (qos->qos.fiveQI) {
        case 1: //100ms
        case 2: //150ms
        case 3: //50ms
        case 4: //300ms
        case 5: //100ms
        case 6: //300ms
        case 7: //100ms
        case 8: //300ms
        case 9: //300ms Video (Buffered Streaming)TCP-based (e.g., www, e-mail, chat, ftp, p2p file sharing, progressive video, etc.)
          // TODO
          break;

        default:
          LOG_E(NR_RRC, "not supported 5qi %lu\n", qos->qos.fiveQI);
          ngap_cause_t cause = {.type = NGAP_CAUSE_RADIO_NETWORK, .value = NGAP_CauseRadioNetwork_not_supported_5QI_value};
          item->status = PDU_SESSION_STATUS_FAILED;
          item->xid = params.transaction_id;
          item->cause = cause;
          continue;
      }
      LOG_I(NR_RRC, "PDU Session ID %d, QOS flow %d, 5QI %ld \n", session->pdusession_id, qos_flow_index, qos->qos.fiveQI);
    }
    item->status = PDU_SESSION_STATUS_DONE;
    item->xid = params.transaction_id;
  }

  byte_array_t msg = do_RRCReconfiguration(&params);
  if (msg.len <= 0) {
    LOG_E(NR_RRC, "UE %d: Failed to generate RRCReconfiguration\n", ue_p->rrc_ue_id);
    return;
  }
  LOG_DUMPMSG(NR_RRC, DEBUG_RRC, msg.buf, msg.len, "[MSG] RRC Reconfiguration\n");
  LOG_I(NR_RRC, "UE %d: Generate RRCReconfiguration (bytes %ld)\n", ue_p->rrc_ue_id, msg.len);
  const uint32_t msg_id = NR_DL_DCCH_MessageType__c1_PR_rrcReconfiguration;
  nr_rrc_transfer_protected_rrc_message(rrc, ue_p, DL_SCH_LCID_DCCH, msg_id, msg.buf, msg.len);
  free_RRCReconfiguration_params(params);
  free_byte_array(msg);
}

static void rrc_gNB_send_f1_drb_release_request(gNB_RRC_INST *rrc, gNB_RRC_UE_t *ue_p, int *drb_to_release, int n_drb_to_release)
{
  f1_ue_data_t ue_data = cu_get_f1_ue_data(ue_p->rrc_ue_id);
  RETURN_IF_INVALID_ASSOC_ID(ue_data.du_assoc_id);
  f1ap_ue_context_mod_req_t req = {
      .gNB_CU_ue_id = ue_p->rrc_ue_id,
      .gNB_DU_ue_id = ue_data.secondary_ue,
      .servCellIndex = 0,
  };
  req.plmn = malloc_or_fail(sizeof(*req.plmn));
  *req.plmn = rrc->configuration.plmn[0];
  req.nr_cellid = malloc_or_fail(sizeof(*req.nr_cellid));
  *req.nr_cellid = rrc->nr_cellid;
  req.drbs_rel = malloc_or_fail(sizeof(*req.drbs_rel));
  req.drbs_rel_len = n_drb_to_release;
  memcpy(req.drbs_rel, drb_to_release, n_drb_to_release * sizeof(int));

  /* send UE Context Modification to DU without attaching any RRC container */
  rrc->mac_rrc.ue_context_modification_request(ue_data.du_assoc_id, &req);
  LOG_I(NR_RRC, "UE %d: send F1 UE Context Modification Request with DRB release (%d DRBs)\n", ue_p->rrc_ue_id, req.drbs_rel_len);
  free_ue_context_mod_req(&req);
}

static void fill_security_info(gNB_RRC_INST *rrc, gNB_RRC_UE_t *UE, security_information_t *secInfo)
{
  secInfo->cipheringAlgorithm = rrc->security.do_drb_ciphering ? UE->ciphering_algorithm : 0;
  secInfo->integrityProtectionAlgorithm = rrc->security.do_drb_integrity ? UE->integrity_algorithm : 0;
  nr_derive_key(UP_ENC_ALG, secInfo->cipheringAlgorithm, UE->kgnb, (uint8_t *)secInfo->encryptionKey);
  nr_derive_key(UP_INT_ALG, secInfo->integrityProtectionAlgorithm, UE->kgnb, (uint8_t *)secInfo->integrityProtectionKey);
}

/** @brief Add DRB modification to E1AP bearer modification request
 * Finds PDU session by DRB ID, creates/updates PDU session modification entry
 * and appends DRB modification
 * @param req E1AP bearer modification request
 * @param ue UE context
 * @param drb_id DRB ID to find PDU session for
 * @param drb_to_mod DRB modification data to append
 * @return true if successful, false if PDU session not found */
static bool append_e1_drb_mod_req(e1ap_bearer_mod_req_t *req,
                                  gNB_RRC_UE_t *ue,
                                  const int drb_id,
                                  const DRB_nGRAN_to_mod_t *drb_to_mod)
{
  // Find PDU session by DRB ID
  rrc_pdu_session_param_t *pdu = find_pduSession_from_drbId(ue, drb_id);
  if (!pdu) {
    LOG_E(NR_RRC, "UE %d: Failed to append E1 DRB mod, no PDU session found (DRB=%d)\n", ue->rrc_ue_id, drb_id);
    return false;
  }

  int pdu_id = pdu->param.pdusession_id;

  // Find existing PDU session modification entry or create new one
  pdu_session_to_mod_t *pdu_mod = NULL;
  for (int i = 0; i < req->numPDUSessionsMod; ++i) {
    if (req->pduSessionMod[i].sessionId == pdu_id) {
      pdu_mod = &req->pduSessionMod[i];
      break;
    }
  }

  // Create new PDU session modification entry if not found
  if (!pdu_mod) {
    DevAssert(req->numPDUSessionsMod < E1AP_MAX_NUM_PDU_SESSIONS);
    int new_index = req->numPDUSessionsMod++;
    req->pduSessionMod[new_index].sessionId = pdu_id;
    pdu_mod = &req->pduSessionMod[new_index];
  }

  // Append DRB modification
  DevAssert(pdu_mod->numDRB2Modify < E1AP_MAX_NUM_DRBS);
  pdu_mod->DRBnGRanModList[pdu_mod->numDRB2Modify++] = *drb_to_mod;

  return true;
}

/** @brief Create DRB modification for reestablishment from existing DRB */
static DRB_nGRAN_to_mod_t get_e1_drb_mod_reestablishment(const drb_t *drb, const bearer_context_pdcp_config_t *pdcp_config)
{
  DRB_nGRAN_to_mod_t drb_e1 = {0};
  drb_e1.id = drb->drb_id;
  drb_e1.numDlUpParam = 1;
  memcpy(&drb_e1.DlUpParamList[0].tl_info.tlAddress, &drb->du_tunnel_config.addr.buffer, sizeof(uint8_t) * 4);
  drb_e1.DlUpParamList[0].tl_info.teId = drb->du_tunnel_config.teid;
  /* PDCP configuration */
  drb_e1.pdcp_config = malloc_or_fail(sizeof(*drb_e1.pdcp_config));
  *drb_e1.pdcp_config = *pdcp_config;
  drb_e1.pdcp_config->pDCP_Reestablishment = true;
  return drb_e1;
}

/**
 * @brief Notify E1 re-establishment to CU-UP
 */
static void cuup_notify_reestablishment(gNB_RRC_INST *rrc, gNB_RRC_UE_t *ue_p)
{
  e1ap_bearer_mod_req_t req = {
      .gNB_cu_cp_ue_id = ue_p->rrc_ue_id,
      .gNB_cu_up_ue_id = ue_p->rrc_ue_id,
  };
  // Quit re-establishment notification if no CU-UP is associated
  if (!is_cuup_associated(rrc) || !ue_associated_to_cuup(rrc, ue_p)) {
    return;
  }

  bool um_on_default_drb = rrc->configuration.um_on_default_drb;
  /* loop through active DRBs */
  FOR_EACH_SEQ_ARR(drb_t *, drb, &ue_p->drbs) {
    bearer_context_pdcp_config_t pdcp = set_bearer_context_pdcp_config(drb->pdcp_config, um_on_default_drb, ue_p->redcap_cap);
    DRB_nGRAN_to_mod_t drb_e1 = get_e1_drb_mod_reestablishment(drb, &pdcp);
    if (!append_e1_drb_mod_req(&req, ue_p, drb->drb_id, &drb_e1)) {
      LOG_W(NR_RRC, "UE %d: Failed to append E1 DRB mod for reestablishment (DRB=%d)\n", ue_p->rrc_ue_id, drb->drb_id);
    }
  }

  /* During reestablishment, for DRB integrity protection security keys change (KgNB* is derived),
   * so security information MUST be sent to CU-UP to update DRB security keys. */
  req.secInfo = malloc_or_fail(sizeof(*req.secInfo));
  fill_security_info(rrc, ue_p, req.secInfo);

  /* Send E1 Bearer Context Modification Request (3GPP TS 38.463) */
  sctp_assoc_t assoc_id = get_existing_cuup_for_ue(rrc, ue_p);
  rrc->cucp_cuup.bearer_context_mod(assoc_id, &req);
  free_e1ap_context_mod_request(&req);
}

/**
 * @brief RRCReestablishment message
 *        Direction: Network to UE
 */
static void rrc_gNB_generate_RRCReestablishment(rrc_gNB_ue_context_t *ue_context_pP,
                                                const uint8_t *masterCellGroup_from_DU,
                                                const rnti_t old_rnti,
                                                const nr_rrc_du_container_t *du)
{
  module_id_t module_id = 0;
  gNB_RRC_INST *rrc = RC.nrrrc[module_id];
  gNB_RRC_UE_t *ue_p = &ue_context_pP->ue_context;
  uint8_t buffer[NR_RRC_BUF_SIZE] = {0};
  uint8_t xid = rrc_gNB_get_next_transaction_identifier(module_id);
  ue_p->xids[xid] = RRC_REESTABLISH;
  const f1ap_served_cell_info_t *cell_info = &du->setup_req->cell[0].info;
  uint32_t ssb_arfcn = get_ssb_arfcn(du);
  LOG_I(NR_RRC, "Reestablishment update key pci=%d, earfcn_dl=%u\n", cell_info->nr_pci, ssb_arfcn);
  nr_derive_key_ng_ran_star(cell_info->nr_pci, ssb_arfcn, ue_p->nh_ncc > 0 ? ue_p->nh : ue_p->kgnb, ue_p->kgnb);
  int size = do_RRCReestablishment(ue_context_pP->ue_context.nh_ncc, buffer, NR_RRC_BUF_SIZE, xid);

  LOG_A(NR_RRC, "Send RRCReestablishment [%d bytes] to RNTI %04x\n", size, ue_p->rnti);

  /* Ciphering and Integrity according to TS 33.501 */
  nr_pdcp_entity_security_keys_and_algos_t security_parameters;
  /* Derive the keys from kgnb */
  nr_derive_key(RRC_ENC_ALG, ue_p->ciphering_algorithm, ue_p->kgnb, security_parameters.ciphering_key);
  nr_derive_key(RRC_INT_ALG, ue_p->integrity_algorithm, ue_p->kgnb, security_parameters.integrity_key);
  LOG_I(NR_RRC,
        "Set PDCP security UE %d RNTI %04x nea %ld nia %d in RRCReestablishment\n",
        ue_p->rrc_ue_id,
        ue_p->rnti,
        ue_p->ciphering_algorithm,
        ue_p->integrity_algorithm);
  /* RRCReestablishment is integrity protected but not ciphered,
   * so let's configure only integrity protection right now.
   * Ciphering is enabled below, after generating RRCReestablishment.
   */
  security_parameters.integrity_algorithm = ue_p->integrity_algorithm;
  security_parameters.ciphering_algorithm = 0;

  /* SRBs */
  for (int srb_id = 1; srb_id < NR_NUM_SRB; srb_id++) {
    if (ue_p->Srb[srb_id].Active)
      nr_pdcp_config_set_security(ue_p->rrc_ue_id, srb_id, true, &security_parameters);
  }
  /* Re-establish PDCP for SRB1, according to 5.3.7.4 of 3GPP TS 38.331 */
  nr_pdcp_reestablishment(ue_p->rrc_ue_id,
                          1,
                          true,
                          &security_parameters);
  /* F1AP DL RRC Message Transfer */
  f1_ue_data_t ue_data = cu_get_f1_ue_data(ue_p->rrc_ue_id);
  RETURN_IF_INVALID_ASSOC_ID(ue_data.du_assoc_id);
  uint32_t old_gNB_DU_ue_id = old_rnti;
  f1ap_dl_rrc_message_t dl_rrc = {.gNB_CU_ue_id = ue_p->rrc_ue_id,
                                  .gNB_DU_ue_id = ue_data.secondary_ue,
                                  .srb_id = DL_SCH_LCID_DCCH,
                                  .old_gNB_DU_ue_id = &old_gNB_DU_ue_id};
  deliver_dl_rrc_message_data_t data = {.rrc = rrc, .dl_rrc = &dl_rrc, .assoc_id = ue_data.du_assoc_id};
  nr_pdcp_data_req_srb(ue_p->rrc_ue_id, DL_SCH_LCID_DCCH, rrc_gNB_mui++, size, (unsigned char *const)buffer, rrc_deliver_dl_rrc_message, &data);

#ifdef E2_AGENT
  E2_AGENT_SIGNAL_DL_DCCH_RRC_MSG(buffer, size, NR_DL_DCCH_MessageType__c1_PR_rrcReestablishment);
#endif

  /* RRCReestablishment has been generated, let's enable ciphering now. */
  security_parameters.ciphering_algorithm = ue_p->ciphering_algorithm;
  /* SRBs */
  for (int srb_id = 1; srb_id < NR_NUM_SRB; srb_id++) {
    if (ue_p->Srb[srb_id].Active)
      nr_pdcp_config_set_security(ue_p->rrc_ue_id, srb_id, true, &security_parameters);
  }
}

/// @brief Function tha processes RRCReestablishmentComplete message sent by the UE, after RRCReestasblishment request.
static void rrc_gNB_process_RRCReestablishmentComplete(gNB_RRC_INST *rrc, gNB_RRC_UE_t *ue_p, const uint8_t xid)
{
  LOG_I(NR_RRC, "UE %d Processing NR_RRCReestablishmentComplete from UE\n", ue_p->rrc_ue_id);

  int i = 0;

  ue_p->xids[xid] = RRC_ACTION_NONE;

  ue_p->Srb[1].Active = 1;

  NR_CellGroupConfig_t *cellGroupConfig = calloc(1, sizeof(NR_CellGroupConfig_t));

  cellGroupConfig->spCellConfig = ue_p->masterCellGroup->spCellConfig;
  cellGroupConfig->mac_CellGroupConfig = ue_p->masterCellGroup->mac_CellGroupConfig;
  cellGroupConfig->physicalCellGroupConfig = ue_p->masterCellGroup->physicalCellGroupConfig;
  cellGroupConfig->rlc_BearerToReleaseList = NULL;
  cellGroupConfig->rlc_BearerToAddModList = calloc(1, sizeof(*cellGroupConfig->rlc_BearerToAddModList));

  /*
   * Get SRB2, DRB configuration from the existing UE context,
   * also start from SRB2 (i=1) and not from SRB1 (i=0).
   */
  for (i = 1; i < ue_p->masterCellGroup->rlc_BearerToAddModList->list.count; ++i)
    asn1cSeqAdd(&cellGroupConfig->rlc_BearerToAddModList->list, ue_p->masterCellGroup->rlc_BearerToAddModList->list.array[i]);

  /*
   * At this stage, PDCP entity are re-established and reestablishRLC is flagged
   * with RRCReconfiguration to complete RLC re-establishment of remaining bearers
   */
  for (i = 0; i < cellGroupConfig->rlc_BearerToAddModList->list.count; i++) {
    asn1cCallocOne(cellGroupConfig->rlc_BearerToAddModList->list.array[i]->reestablishRLC,
                   NR_RLC_BearerConfig__reestablishRLC_true);
  }

  /* TODO: remove a reconfigurationWithSync, we don't need it for
   * reestablishment. The whole reason why this might be here is that we store
   * the CellGroupConfig (after handover), and simply reuse it for
   * reestablishment, instead of re-requesting the CellGroupConfig from the DU.
   * Hence, add below hack; the solution would be to request the
   * CellGroupConfig from the DU when doing reestablishment. */
  if (cellGroupConfig->spCellConfig && cellGroupConfig->spCellConfig->reconfigurationWithSync) {
    ASN_STRUCT_FREE(asn_DEF_NR_ReconfigurationWithSync, cellGroupConfig->spCellConfig->reconfigurationWithSync);
    cellGroupConfig->spCellConfig->reconfigurationWithSync = NULL;
  }

  /* Re-establish SRB2 according to clause 5.3.5.6.3 of 3GPP TS 38.331
   * (SRB1 is re-established with RRCReestablishment message)
   */
  int srb_id = 2;
  if (ue_p->Srb[srb_id].Active) {
    nr_pdcp_entity_security_keys_and_algos_t security_parameters;
    security_parameters.ciphering_algorithm = ue_p->ciphering_algorithm;
    security_parameters.integrity_algorithm = ue_p->integrity_algorithm;
    nr_derive_key(RRC_ENC_ALG, ue_p->ciphering_algorithm, ue_p->kgnb, security_parameters.ciphering_key);
    nr_derive_key(RRC_INT_ALG, ue_p->integrity_algorithm, ue_p->kgnb, security_parameters.integrity_key);

    nr_pdcp_reestablishment(ue_p->rrc_ue_id, srb_id, true, &security_parameters);
  }
  /* PDCP Reestablishment of DRBs according to 5.3.5.6.5 of 3GPP TS 38.331 (over E1) */
  cuup_notify_reestablishment(rrc, ue_p);

  /* Create srb-ToAddModList, 3GPP TS 38.331 RadioBearerConfig:
   * do not re-establish PDCP for SRB1, when resuming an RRC connection,
   * or at the first reconfiguration after RRC connection reestablishment in NR */
  NR_SRB_ToAddModList_t *SRBs = createSRBlist(ue_p, 1 << SRB2); // Re-establish PDCP for SRB2 only
  /* Create drb-ToAddModList */
  bool do_integrity = rrc->security.do_drb_integrity;
  bool do_ciphering = rrc->security.do_drb_ciphering;
  NR_DRB_ToAddModList_t *DRBs = createDRBlist(ue_p, true, do_integrity, do_ciphering);

  uint8_t new_xid = rrc_gNB_get_next_transaction_identifier(rrc->module_id);
  ue_p->xids[new_xid] = RRC_REESTABLISH_COMPLETE;
  nr_rrc_reconfig_param_t params = {.cell_group_config = cellGroupConfig,
                                    .meas_config = ue_p->measConfig,
                                    .drb_config_list = DRBs,
                                    .srb_config_list = SRBs,
                                    .transaction_id = new_xid};
  byte_array_t msg = do_RRCReconfiguration(&params);
  if (msg.len <= 0) {
    LOG_E(NR_RRC, "UE %d: Failed to generate RRCReconfiguration\n", ue_p->rrc_ue_id);
    return;
  }
  LOG_DUMPMSG(NR_RRC, DEBUG_RRC, msg.buf, msg.len, "[MSG] RRC Reconfiguration\n");
  LOG_I(NR_RRC,
        "UE %d RNTI %04x: Generate NR_RRCReconfiguration after reestablishment complete (bytes %ld)\n",
        ue_p->rrc_ue_id,
        ue_p->rnti,
        msg.len);
  const uint32_t msg_id = NR_DL_DCCH_MessageType__c1_PR_rrcReconfiguration;
  nr_rrc_transfer_protected_rrc_message(rrc, ue_p, DL_SCH_LCID_DCCH, msg_id, msg.buf, msg.len);
  free_RRCReconfiguration_params(params);
  free_byte_array(msg);
}

int nr_rrc_reconfiguration_req(gNB_RRC_INST *rrc, gNB_RRC_UE_t *ue_p, const int dl_bwp_id, const int ul_bwp_id)
{
  uint8_t xid = rrc_gNB_get_next_transaction_identifier(rrc->module_id);
  ue_p->xids[xid] = RRC_DEDICATED_RECONF;
  ue_p->ongoing_reconfiguration = true;

  NR_CellGroupConfig_t *masterCellGroup = ue_p->masterCellGroup;
  if (dl_bwp_id > 0) {
    *masterCellGroup->spCellConfig->spCellConfigDedicated->firstActiveDownlinkBWP_Id = dl_bwp_id;
    *masterCellGroup->spCellConfig->spCellConfigDedicated->defaultDownlinkBWP_Id = dl_bwp_id;
  }
  if (ul_bwp_id > 0) {
    *masterCellGroup->spCellConfig->spCellConfigDedicated->uplinkConfig->firstActiveUplinkBWP_Id = ul_bwp_id;
  }

  nr_rrc_reconfig_param_t params = {.cell_group_config = masterCellGroup, .transaction_id = xid};
  byte_array_t msg = do_RRCReconfiguration(&params);
  if (msg.len <= 0) {
    LOG_E(NR_RRC, "UE %d: Failed to generate RRCReconfiguration\n", ue_p->rrc_ue_id);
    return -1;
  }
  const uint32_t msg_id = NR_DL_DCCH_MessageType__c1_PR_rrcReconfiguration;
  nr_rrc_transfer_protected_rrc_message(rrc, ue_p, DL_SCH_LCID_DCCH, msg_id, msg.buf, msg.len);
  free_byte_array(msg);
  free_RRCReconfiguration_params(params);
  return 0;
}

static void rrc_handle_RRCSetupRequest(gNB_RRC_INST *rrc,
                                       sctp_assoc_t assoc_id,
                                       const NR_RRCSetupRequest_IEs_t *rrcSetupRequest,
                                       const f1ap_initial_ul_rrc_message_t *msg)
{
  rrc_gNB_ue_context_t *ue_context_p = NULL;
  if (NR_InitialUE_Identity_PR_randomValue == rrcSetupRequest->ue_Identity.present) {
    /* randomValue                         BIT STRING (SIZE (39)) */
    if (rrcSetupRequest->ue_Identity.choice.randomValue.size != 5) { // 39-bit random value
      LOG_E(NR_RRC,
            "wrong InitialUE-Identity randomValue size, expected 5, provided %lu",
            (long unsigned int)rrcSetupRequest->ue_Identity.choice.randomValue.size);
      return;
    }
    uint64_t random_value = 0;
    memcpy(((uint8_t *)&random_value) + 3,
           rrcSetupRequest->ue_Identity.choice.randomValue.buf,
           rrcSetupRequest->ue_Identity.choice.randomValue.size);

    ue_context_p = rrc_gNB_create_ue_context(assoc_id, msg->crnti, rrc, random_value, msg->gNB_DU_ue_id);
  } else if (NR_InitialUE_Identity_PR_ng_5G_S_TMSI_Part1 == rrcSetupRequest->ue_Identity.present) {
    /* <5G-S-TMSI> = <AMF Set ID><AMF Pointer><5G-TMSI> 48-bit */
    /* ng-5G-S-TMSI-Part1                  BIT STRING (SIZE (39)) */
    if (rrcSetupRequest->ue_Identity.choice.ng_5G_S_TMSI_Part1.size != 5) {
      LOG_E(NR_RRC,
            "wrong ng_5G_S_TMSI_Part1 size, expected 5, provided %lu \n",
            (long unsigned int)rrcSetupRequest->ue_Identity.choice.ng_5G_S_TMSI_Part1.size);
      return;
    }

    uint64_t s_tmsi_part1 = BIT_STRING_to_uint64(&rrcSetupRequest->ue_Identity.choice.ng_5G_S_TMSI_Part1);
    LOG_I(NR_RRC, "Received UE 5G-S-TMSI-Part1 %ld\n", s_tmsi_part1);

    ue_context_p = rrc_gNB_create_ue_context(assoc_id, msg->crnti, rrc, s_tmsi_part1, msg->gNB_DU_ue_id);
    AssertFatal(ue_context_p != NULL, "out of memory\n");
    gNB_RRC_UE_t *UE = &ue_context_p->ue_context;
    UE->Initialue_identity_5g_s_TMSI.presence = true;
    UE->ng_5G_S_TMSI_Part1 = s_tmsi_part1;
  } else {
    uint64_t random_value = 0;
    memcpy(((uint8_t *)&random_value) + 3,
           rrcSetupRequest->ue_Identity.choice.randomValue.buf,
           rrcSetupRequest->ue_Identity.choice.randomValue.size);

    ue_context_p = rrc_gNB_create_ue_context(assoc_id, msg->crnti, rrc, random_value, msg->gNB_DU_ue_id);
    LOG_E(NR_RRC, "RRCSetupRequest without random UE identity or S-TMSI not supported, let's reject the UE %04x\n", msg->crnti);
    rrc_gNB_generate_RRCReject(rrc, ue_context_p);
    return;
  }

  // If the DU to CU RRC Container IE is not included in the INITIAL UL RRC MESSAGE TRANSFER,
  // the gNB-CU should reject the UE under the assumption that the gNB-DU is not able to serve such UE
  if (msg->du2cu_rrc_container == NULL) {
    // this will remove the UE context
    rrc_gNB_generate_RRCReject(rrc, ue_context_p);
    return;
  }
  NR_CellGroupConfig_t *cellGroupConfig = NULL;
  asn_dec_rval_t dec_rval = uper_decode_complete(NULL,
                                                 &asn_DEF_NR_CellGroupConfig,
                                                 (void **)&cellGroupConfig,
                                                 msg->du2cu_rrc_container,
                                                 msg->du2cu_rrc_container_length);
  AssertFatal(dec_rval.code == RC_OK && dec_rval.consumed > 0, "Cell group config decode error\n");

  gNB_RRC_UE_t *UE = &ue_context_p->ue_context;
  UE = &ue_context_p->ue_context;
  UE->establishment_cause = rrcSetupRequest->establishmentCause;
  UE->nr_cellid = msg->nr_cellid;
  UE->masterCellGroup = cellGroupConfig;
  UE->ongoing_reconfiguration = false;
  UE->measConfig = nr_rrc_get_measconfig(rrc, UE->nr_cellid);
  activate_srb(UE, 1);
  rrc_gNB_generate_RRCSetup(0, msg->crnti, ue_context_p, msg->du2cu_rrc_container, msg->du2cu_rrc_container_length);
}

static const char *get_reestab_cause(NR_ReestablishmentCause_t c)
{
  switch (c) {
    case NR_ReestablishmentCause_otherFailure:
      return "Other Failure";
    case NR_ReestablishmentCause_handoverFailure:
      return "Handover Failure";
    case NR_ReestablishmentCause_reconfigurationFailure:
      return "Reconfiguration Failure";
    default:
      break;
  }
  return "UNKNOWN Failure (ASN.1 decoder error?)";
}

static rrc_gNB_ue_context_t *rrc_gNB_get_ue_context_source_cell(gNB_RRC_INST *rrc_instance_pP, long pci, rnti_t rntiP)
{
  rrc_gNB_ue_context_t *ue_context_p;
  RB_FOREACH(ue_context_p, rrc_nr_ue_tree_s, &rrc_instance_pP->rrc_ue_head) {
    gNB_RRC_UE_t *ue = &ue_context_p->ue_context;
    if (!ue->ho_context || !ue->ho_context->source)
      continue;
    nr_ho_source_cu_t *source_ctx = ue->ho_context->source;
    if (source_ctx->old_rnti == rntiP && source_ctx->du->setup_req->cell[0].info.nr_pci == pci)
      return ue_context_p;
  }
  return NULL;
}

/**
 * @brief Rollback F1-U DL TL and TEID in RRC
 */
static void f1u_dl_gtp_rollback(gNB_RRC_UE_t *UE)
{
  DevAssert(UE != NULL);
  DevAssert(UE->ho_context->source != NULL);

  FOR_EACH_SEQ_ARR(drb_t *, drb, &UE->drbs) {
    drb->du_tunnel_config = UE->ho_context->source->old_du_tunnel_config;
    LOG_W(NR_RRC, "DRB id %d rollback to tunnel TEID %x\n", drb->drb_id, drb->du_tunnel_config.teid);
  }
}

static void rrc_handle_RRCReestablishmentRequest(gNB_RRC_INST *rrc,
                                                 sctp_assoc_t assoc_id,
                                                 const NR_RRCReestablishmentRequest_IEs_t *req,
                                                 const f1ap_initial_ul_rrc_message_t *msg)
{
  uint64_t random_value = 0;
  const char *scause = get_reestab_cause(req->reestablishmentCause);
  const long physCellId = req->ue_Identity.physCellId;
  long ngap_cause = NGAP_CAUSE_RADIO_NETWORK_UNSPECIFIED; /* cause in case of NGAP release req */
  rrc_gNB_ue_context_t *ue_context_p = NULL;
  LOG_I(NR_RRC,
        "Reestablishment RNTI %04x req C-RNTI %04lx physCellId %ld cause %s\n",
        msg->crnti,
        req->ue_Identity.c_RNTI,
        physCellId,
        scause);

  const nr_rrc_du_container_t *du = get_du_by_assoc_id(rrc, assoc_id);
  if (du == NULL) {
    LOG_E(RRC, "received CCCH message, but no corresponding DU found\n");
    return;
  }

  // 3GPP TS 38.321 version 15.13.0 Section 7.1 Table 7.1-1: RNTI values
  if (req->ue_Identity.c_RNTI < 0x1 || req->ue_Identity.c_RNTI > 0xffef) {
    /* c_RNTI range error should not happen */
    LOG_E(NR_RRC, "NR_RRCReestablishmentRequest c_RNTI %04lx range error, fallback to RRC setup\n", req->ue_Identity.c_RNTI);
    goto fallback_rrc_setup;
  }

  if (du->mib == NULL || du->sib1 == NULL) {
    /* we don't have MIB/SIB1 of the DU, and therefore cannot generate the
     * Reestablishment (as we would need the SSB's ARFCN, which we cannot
     * compute). So generate RRC Setup instead */
    LOG_E(NR_RRC, "Reestablishment request: no MIB/SIB1 of DU present, cannot do reestablishment, force setup request\n");
    goto fallback_rrc_setup;
  }

  if (du->mtc == NULL) {
    // some UEs don't send MeasurementTimingConfiguration, so we don't know the
    // SSB ARFCN and can't do reestablishment. handle it gracefully by doing
    // RRC setup procedure instead
    LOG_E(NR_RRC, "no MeasurementTimingConfiguration for this cell, cannot perform reestablishment\n");
    ngap_cause = NGAP_CAUSE_RADIO_NETWORK_RELEASE_DUE_TO_NGRAN_GENERATED_REASON;
    goto fallback_rrc_setup;
  }

  rnti_t old_rnti = req->ue_Identity.c_RNTI;
  ue_context_p = rrc_gNB_get_ue_context_by_rnti(rrc, assoc_id, old_rnti);
  if (ue_context_p == NULL) {
    ue_context_p = rrc_gNB_get_ue_context_source_cell(rrc, physCellId, old_rnti);
    if (ue_context_p == NULL) {
      LOG_E(NR_RRC, "NR_RRCReestablishmentRequest without UE context, fallback to RRC setup\n");
      goto fallback_rrc_setup;
    }
  }
  gNB_RRC_UE_t *UE = &ue_context_p->ue_context;

  /* should check phys cell ID to identify the correct cell */
  const f1ap_served_cell_info_t *cell_info = &du->setup_req->cell[0].info;
  const f1ap_served_cell_info_t *previous_cell_info = get_cell_information_by_phycellId(physCellId);

  nr_ho_source_cu_t *source_ctx = UE->ho_context ? UE->ho_context->source : NULL;
  DevAssert(!source_ctx || source_ctx->du->setup_req->num_cells_available == 1);
  bool ho_reestab_on_source = source_ctx && previous_cell_info->nr_cellid == source_ctx->du->setup_req->cell[0].info.nr_cellid;

  if (ho_reestab_on_source) {
    /* the UE came back on the source DU while doing handover, release at
     * target DU and and update the association to the initial DU one */
    LOG_W(NR_RRC, "handover for UE %d/RNTI %04x failed, rollback to original cell\n", UE->rrc_ue_id, UE->rnti);

    // find the transaction of handover (the corresponding reconfig) and abort it
    for (int i = 0; i < NR_RRC_TRANSACTION_IDENTIFIER_NUMBER; ++i) {
      if (UE->xids[i] == RRC_DEDICATED_RECONF)
        UE->xids[i] = RRC_ACTION_NONE;
    }

    source_ctx->ho_cancel(rrc, UE);
    f1u_dl_gtp_rollback(UE);

    /* we need the original CellGroupConfig */
    ASN_STRUCT_FREE(asn_DEF_NR_CellGroupConfig, UE->masterCellGroup);
    UE->masterCellGroup = source_ctx->old_cellGroupConfig;
    source_ctx->old_cellGroupConfig = NULL;

    /* update to old DU assoc id -- RNTI + secondary DU UE ID further below */
    f1_ue_data_t ue_data = cu_get_f1_ue_data(UE->rrc_ue_id);
    ue_data.du_assoc_id = source_ctx->du->assoc_id;
    bool success = cu_update_f1_ue_data(UE->rrc_ue_id, &ue_data);
    DevAssert(success);
    nr_rrc_finalize_ho(UE);
  } else if (physCellId != cell_info->nr_pci) {
    /* UE was moving from previous cell so quickly that RRCReestablishment for previous cell was received in this cell */
    LOG_I(NR_RRC,
          "RRC Reestablishment Request from different physCellId (%ld) than current physCellId (%d), fallback to RRC setup\n",
          physCellId,
          cell_info->nr_pci);
    ngap_cause = NGAP_CAUSE_RADIO_NETWORK_RELEASE_DUE_TO_NGRAN_GENERATED_REASON;
    /* 38.401 8.7: "If the UE accessed from a gNB-DU other than the original
     * one, the gNB-CU should trigger the UE Context Setup procedure". Let's
     * assume that the old DU will trigger a release request, also freeing the
     * ongoing context at the CU. Hence, create new below */
    goto fallback_rrc_setup;
  }

  if (!UE->as_security_active) {
    /* no active security context, need to restart entire connection */
    LOG_E(NR_RRC, "UE requested Reestablishment without activated AS security\n");
    ngap_cause = NGAP_CAUSE_RADIO_NETWORK_RELEASE_DUE_TO_NGRAN_GENERATED_REASON;
    goto fallback_rrc_setup;
  }

  /* TODO: start timer in ITTI and drop UE if it does not come back */

  // update with new RNTI, and update secondary UE association
  UE->rnti = msg->crnti;
  UE->nr_cellid = msg->nr_cellid;
  f1_ue_data_t ue_data = cu_get_f1_ue_data(UE->rrc_ue_id);
  ue_data.secondary_ue = msg->gNB_DU_ue_id;
  bool success = cu_update_f1_ue_data(UE->rrc_ue_id, &ue_data);
  DevAssert(success);

  rrc_gNB_generate_RRCReestablishment(ue_context_p, msg->du2cu_rrc_container, old_rnti, du);
  return;

fallback_rrc_setup:
  fill_random(&random_value, sizeof(random_value));
  random_value = random_value & 0x7fffffffff; /* random value is 39 bits */

  ngap_cause_t cause = {.type = NGAP_CAUSE_RADIO_NETWORK, .value = ngap_cause};
  /* request release of the "old" UE in case it exists */
  if (ue_context_p != NULL)
    rrc_gNB_send_NGAP_UE_CONTEXT_RELEASE_REQ(0, ue_context_p, cause);

  rrc_gNB_ue_context_t *new = rrc_gNB_create_ue_context(assoc_id, msg->crnti, rrc, random_value, msg->gNB_DU_ue_id);
  activate_srb(&new->ue_context, 1);
  rrc_gNB_generate_RRCSetup(0, msg->crnti, new, msg->du2cu_rrc_container, msg->du2cu_rrc_container_length);
  return;
}

static void process_Periodical_Measurement_Report(gNB_RRC_UE_t *ue_ctxt, NR_MeasurementReport_t *measurementReport)
{
  ASN_STRUCT_FREE(asn_DEF_NR_MeasResults, ue_ctxt->measResults);
  ue_ctxt->measResults = NULL;

  const NR_MeasId_t id = measurementReport->criticalExtensions.choice.measurementReport->measResults.measId;
  AssertFatal(id, "unexpected MeasResult for MeasurementId %ld received\n", id);
  asn1cCallocOne(ue_ctxt->measResults, measurementReport->criticalExtensions.choice.measurementReport->measResults);
  /* we "keep" the measurement report, so set to 0 */
  free(measurementReport->criticalExtensions.choice.measurementReport);
  measurementReport->criticalExtensions.choice.measurementReport = NULL;
}

static void process_Event_Based_Measurement_Report(gNB_RRC_INST *rrc,
                                                   gNB_RRC_UE_t *ue,
                                                   NR_ReportConfigNR_t *report,
                                                   NR_MeasurementReport_t *measurementReport)
{
  NR_EventTriggerConfig_t *event_triggered = report->reportType.choice.eventTriggered;

  int servingCellRSRP = 0;
  int neighbourCellRSRP = 0;
  int scell_pci = -1;
  int best_rsrp = -10000;

  switch (event_triggered->eventId.present) {
    case NR_EventTriggerConfig__eventId_PR_eventA2:
      LOG_I(NR_RRC, "HO LOG: Event A2 (Serving becomes worse than threshold)\n");
      break;

    case NR_EventTriggerConfig__eventId_PR_eventA3: {
      LOG_W(NR_RRC, "HO LOG: Event A3 Report for UE %d - Neighbour Becomes Better than Serving!\n", ue->rrc_ue_id);
      if (!measurementReport->criticalExtensions.choice.measurementReport) {
        LOG_E(NR_RRC, "HO LOG: Event A3 Report: measurementReport is null\n");
        break;
      }
      const NR_MeasResults_t *measResults = &measurementReport->criticalExtensions.choice.measurementReport->measResults;

      for (int serving_cell_idx = 0; serving_cell_idx < measResults->measResultServingMOList.list.count; serving_cell_idx++) {
        const NR_MeasResultServMO_t *meas_result_serv_MO = measResults->measResultServingMOList.list.array[serving_cell_idx];
        scell_pci = *(meas_result_serv_MO->measResultServingCell.physCellId);
        if (meas_result_serv_MO->measResultServingCell.measResult.cellResults.resultsSSB_Cell) {
          servingCellRSRP = *(meas_result_serv_MO->measResultServingCell.measResult.cellResults.resultsSSB_Cell->rsrp) - 157;
        } else {
          servingCellRSRP = *(meas_result_serv_MO->measResultServingCell.measResult.cellResults.resultsCSI_RS_Cell->rsrp) - 157;
        }
        LOG_D(NR_RRC, "Serving Cell RSRP: %d\n", servingCellRSRP);
      }

      if (measResults->measResultNeighCells == NULL ||
          measResults->measResultNeighCells->present != NR_MeasResults__measResultNeighCells_PR_measResultListNR) {
        LOG_D(NR_RRC, "HO LOG: No neighbor cell measurements available\n");
        break;
      }

      const NR_MeasResultListNR_t *measResultListNR = measResults->measResultNeighCells->choice.measResultListNR;
      for (int neigh_meas_idx = 0; neigh_meas_idx < measResultListNR->list.count; neigh_meas_idx++) {
        const NR_MeasResultNR_t *meas_result_neigh_cell = (measResultListNR->list.array[neigh_meas_idx]);
        const int neighbour_pci = *(meas_result_neigh_cell->physCellId);

        // TS 138 133 Table 10.1.6.1-1: SS-RSRP and CSI-RSRP measurement report mapping
        const struct NR_MeasResultNR__measResult__cellResults *cellResults = &(meas_result_neigh_cell->measResult.cellResults);

        if (cellResults->resultsSSB_Cell) {
          neighbourCellRSRP = *(cellResults->resultsSSB_Cell->rsrp) - 157;
        } else {
          neighbourCellRSRP = *(cellResults->resultsCSI_RS_Cell->rsrp) - 157;
        }

        LOG_I(NR_RRC,
              "HO LOG: Measurement Report for the neighbour %d with RSRP: %d\n",
              neighbour_pci,
              neighbourCellRSRP);

        const f1ap_served_cell_info_t *neigh_cell = get_cell_information_by_phycellId(neighbour_pci);
        const f1ap_served_cell_info_t *serving_cell = get_cell_information_by_phycellId(scell_pci);
        const neighbour_cell_configuration_t *cell = get_neighbour_cell_config(rrc, serving_cell->nr_cellid);
        const nr_neighbour_cell_t *neighbour = get_neighbour_cell_by_pci(cell, neighbour_pci);
        // CU does not have f1 connection with neighbour cell context. So  check does serving cell has this phyCellId as a
        // neighbour.
        if (!neigh_cell && neighbour) {
          // No F1 connection but static neighbour configuration is available
          const nr_a3_event_t *a3_event_configuration = get_a3_configuration(rrc, neighbour->physicalCellId);
          // Additional check - This part can be modified according to additional cell specific Handover Margin
          // a3-Offset: The actual value is field value * 0.5 dB.
          if (a3_event_configuration
              && ((a3_event_configuration->a3_offset * 0.5 + a3_event_configuration->hysteresis)
                  < (neighbourCellRSRP - servingCellRSRP))) {
            if (neighbourCellRSRP > best_rsrp) {
              // UE can send multiple neighbour cells A3 event report in 1 Meas Report. So, we need to find the best neighbour
              best_rsrp = neighbourCellRSRP;
              LOG_I(NR_RRC, "HO LOG: Serving Cell RSRP: %d - Best Neighbor RSRP: %d ! Trigger N2 HO\n", servingCellRSRP, best_rsrp);
              nr_rrc_trigger_n2_ho(rrc, ue, scell_pci, neighbour);
            }
            LOG_D(NR_RRC, "HO LOG: Trigger N2 HO for the neighbour gnb: %u cell: %lu\n", neighbour->gNB_ID, neighbour->nrcell_id);
          }
        } else if (neigh_cell && neighbour) {
          /* we know the cell and are connected to the DU! */
          nr_rrc_du_container_t *source_du = get_du_by_cell_id(rrc, serving_cell->nr_cellid);
          DevAssert(source_du);
          nr_rrc_du_container_t *target_du = get_du_by_cell_id(rrc, neigh_cell->nr_cellid);
          nr_rrc_trigger_f1_ho(rrc, ue, source_du, target_du);
        } else {
          LOG_W(NR_RRC, "UE %d: received A3 event for stronger neighbor PCI %d, but no such neighbour in configuration\n", ue->rrc_ue_id, neighbour_pci);
        }
      }
    } break;
    default:
      LOG_D(NR_RRC, "NR_EventTriggerConfig__eventId_PR_NOTHING or Other event report\n");
      break;
  }
}

static void rrc_gNB_process_MeasurementReport(gNB_RRC_INST *rrc, gNB_RRC_UE_t *UE, NR_MeasurementReport_t *measurementReport)
{
  NR_MeasurementReport__criticalExtensions_PR p = measurementReport->criticalExtensions.present;
  if (p != NR_MeasurementReport__criticalExtensions_PR_measurementReport
      || measurementReport->criticalExtensions.choice.measurementReport == NULL) {
    LOG_E(NR_RRC, "UE %d: expected presence of MeasurementReport, but has %d (%p)\n", UE->rrc_ue_id, p, measurementReport->criticalExtensions.choice.measurementReport);
    return;
  }

  if (LOG_DEBUGFLAG(DEBUG_ASN1))
    xer_fprint(stdout, &asn_DEF_NR_MeasurementReport, (void *)measurementReport);

  NR_MeasConfig_t *meas_config = UE->measConfig;
  if (meas_config == NULL) {
    LOG_I(NR_RRC, "Unexpected Measurement Report from UE with id: %d\n", UE->rrc_ue_id);
    return;
  }

  NR_MeasurementReport_IEs_t *measurementReport_IEs = measurementReport->criticalExtensions.choice.measurementReport;
  const NR_MeasId_t measId = measurementReport_IEs->measResults.measId;

  NR_MeasIdToAddMod_t *meas_id_s = NULL;
  for (int meas_idx = 0; meas_idx < meas_config->measIdToAddModList->list.count; meas_idx++) {
    if (measId == meas_config->measIdToAddModList->list.array[meas_idx]->measId) {
      meas_id_s = meas_config->measIdToAddModList->list.array[meas_idx];
      break;
    }
  }

  if (meas_id_s == NULL) {
    LOG_E(NR_RRC, "Incoming Meas ID with id: %d Can not Found!\n", (int)measId);
    return;
  }

  LOG_D(NR_RRC, "HO LOG: Meas Id is found: %d\n", (int)meas_id_s->measId);

  struct NR_ReportConfigToAddMod__reportConfig *report_config = NULL;
  for (int rep_id = 0; rep_id < meas_config->reportConfigToAddModList->list.count; rep_id++) {
    if (meas_id_s->reportConfigId == meas_config->reportConfigToAddModList->list.array[rep_id]->reportConfigId) {
      report_config = &meas_config->reportConfigToAddModList->list.array[rep_id]->reportConfig;
    }
  }

  if (report_config == NULL || report_config->choice.reportConfigNR == NULL) {
    LOG_E(NR_RRC, "There is no related report configuration for this measId!\n");
    return;
  }

  if (report_config->choice.reportConfigNR->reportType.present == NR_ReportConfigNR__reportType_PR_periodical)
    return process_Periodical_Measurement_Report(UE, measurementReport);

  if (report_config->choice.reportConfigNR->reportType.present == NR_ReportConfigNR__reportType_PR_eventTriggered)
    return process_Event_Based_Measurement_Report(rrc, UE, report_config->choice.reportConfigNR, measurementReport);

  LOG_E(NR_RRC, "Incoming Report Type: %d is not supported! \n", report_config->choice.reportConfigNR->reportType.present);
}

static int handle_rrcReestablishmentComplete(gNB_RRC_INST *rrc, gNB_RRC_UE_t *UE, const NR_RRCReestablishmentComplete_t *cplt)
{
  NR_RRCReestablishmentComplete__criticalExtensions_PR p = cplt->criticalExtensions.present;
  if (p != NR_RRCReestablishmentComplete__criticalExtensions_PR_rrcReestablishmentComplete) {
    LOG_E(NR_RRC, "UE %d: expected presence of rrcReestablishmentComplete, but message has %d\n", UE->rrc_ue_id, p);
    return -1;
  }
  rrc_gNB_process_RRCReestablishmentComplete(rrc, UE, cplt->rrc_TransactionIdentifier);

  UE->ue_reestablishment_counter++;
  return 0;
}

/**
 * @brief Forward stored NAS PDU to UE (3GPP TS 38.413)
 *        - 8.2.1.2: If the NAS-PDU IE is included in the PDU SESSION RESOURCE SETUP REQUEST message,
 *        the NG-RAN node shall pass it to the UE.
 *        - 8.3.1.2: If the NAS-PDU IE is included in the INITIAL CONTEXT SETUP REQUEST message,
 *        the NG-RAN node shall pass it transparently towards the UE.
 *        - 8.6.2: The NAS-PDU IE contains an AMF–UE message that is transferred without interpretation in the NG-RAN node.
 */
void rrc_forward_ue_nas_message(gNB_RRC_INST *rrc, gNB_RRC_UE_t *UE)
{
  if (UE->nas_pdu.buf == NULL || UE->nas_pdu.len == 0)
    return; // no problem: the UE will re-request a NAS PDU

  LOG_UE_DL_EVENT(UE, "Send DL Information Transfer [%ld bytes]\n", UE->nas_pdu.len);

  uint8_t buffer[4096];
  unsigned int xid = rrc_gNB_get_next_transaction_identifier(rrc->module_id);
  uint32_t length = do_NR_DLInformationTransfer(buffer, sizeof(buffer), xid, UE->nas_pdu.len, UE->nas_pdu.buf);
  LOG_DUMPMSG(NR_RRC, DEBUG_RRC, buffer, length, "[MSG] RRC DL Information Transfer\n");
  rb_id_t srb_id = UE->Srb[2].Active ? DL_SCH_LCID_DCCH1 : DL_SCH_LCID_DCCH;
  const uint32_t msg_id = NR_DL_DCCH_MessageType__c1_PR_dlInformationTransfer;
  nr_rrc_transfer_protected_rrc_message(rrc, UE, srb_id, msg_id, buffer, length);
  // no need to free UE->nas_pdu.buf, do_NR_DLInformationTransfer() did that
  UE->nas_pdu.buf = NULL;
  UE->nas_pdu.len = 0;
}

static void handle_ueCapabilityInformation(gNB_RRC_INST *rrc, gNB_RRC_UE_t *UE, const NR_UECapabilityInformation_t *ue_cap_info)
{
  int xid = ue_cap_info->rrc_TransactionIdentifier;
  rrc_action_t a = UE->xids[xid];
  UE->xids[xid] = RRC_ACTION_NONE;
  if (a != RRC_UECAPABILITY_ENQUIRY) {
    LOG_E(NR_RRC, "UE %d: received unsolicited UE Capability Information, aborting procedure\n", UE->rrc_ue_id);
    return;
  }

  int eutra_index = -1;

  if (ue_cap_info->criticalExtensions.present == NR_UECapabilityInformation__criticalExtensions_PR_ueCapabilityInformation) {
    const NR_UE_CapabilityRAT_ContainerList_t *ue_CapabilityRAT_ContainerList =
        ue_cap_info->criticalExtensions.choice.ueCapabilityInformation->ue_CapabilityRAT_ContainerList;

    /* Encode UE-CapabilityRAT-ContainerList for sending to the DU */
    FREE_AND_ZERO_BYTE_ARRAY(UE->ue_cap_buffer);
    UE->ue_cap_buffer.len = uper_encode_to_new_buffer(&asn_DEF_NR_UE_CapabilityRAT_ContainerList,
                                                      NULL,
                                                      ue_CapabilityRAT_ContainerList,
                                                      (void **)&UE->ue_cap_buffer.buf);
    if (UE->ue_cap_buffer.len <= 0) {
      LOG_E(RRC, "could not encode UE-CapabilityRAT-ContainerList, abort handling capabilities\n");
      return;
    }
    LOG_UE_UL_EVENT(UE, "Received UE capabilities\n");

    for (int i = 0; i < ue_CapabilityRAT_ContainerList->list.count; i++) {
      const NR_UE_CapabilityRAT_Container_t *ue_cap_container = ue_CapabilityRAT_ContainerList->list.array[i];
      if (ue_cap_container->rat_Type == NR_RAT_Type_nr) {
        if (UE->UE_Capability_nr) {
          ASN_STRUCT_FREE(asn_DEF_NR_UE_NR_Capability, UE->UE_Capability_nr);
          UE->UE_Capability_nr = 0;
        }

        UE->UE_Capability_nr = decode_nr_ue_capability(UE->rnti, ue_CapabilityRAT_ContainerList);
        if (!UE->UE_Capability_nr) {
          LOG_E(NR_RRC, "UE %d: NR capability decoding failed\n", UE->rrc_ue_id);
          return;
        }

        if (LOG_DEBUGFLAG(DEBUG_ASN1)) {
          xer_fprint(stdout, &asn_DEF_NR_UE_NR_Capability, UE->UE_Capability_nr);
        }

        const NR_RedCapParameters_r17_t *redcap_p = get_redcapparam_r17(UE->UE_Capability_nr);
        if (redcap_p) {
          UE->redcap_cap = calloc_or_fail(1, sizeof(*UE->redcap_cap));
          UE->redcap_cap->support_of_redcap_r17 = redcap_p->supportOfRedCap_r17 != NULL;
          UE->redcap_cap->support_of_16drb_redcap_r17 = redcap_p->supportOf16DRB_RedCap_r17 != NULL;
          if (UE->UE_Capability_nr->pdcp_Parameters.ext2)
            UE->redcap_cap->pdcp_drb_long_sn_redcap_r17 = UE->UE_Capability_nr->pdcp_Parameters.ext2->longSN_RedCap_r17 != NULL;
          if (UE->UE_Capability_nr->rlc_Parameters && UE->UE_Capability_nr->rlc_Parameters->ext2)
            UE->redcap_cap->rlc_am_drb_long_sn_redcap_r17 =
                UE->UE_Capability_nr->rlc_Parameters->ext2->am_WithLongSN_RedCap_r17 != NULL;
        }

        UE->UE_Capability_size = ue_cap_container->ue_CapabilityRAT_Container.size;
        if (eutra_index != -1) {
          LOG_E(NR_RRC, "fatal: more than 1 eutra capability\n");
          return;
        }
        eutra_index = i;
      }

      if (ue_cap_container->rat_Type == NR_RAT_Type_eutra_nr) {
        if (UE->UE_Capability_MRDC) {
          ASN_STRUCT_FREE(asn_DEF_NR_UE_MRDC_Capability, UE->UE_Capability_MRDC);
          UE->UE_Capability_MRDC = 0;
        }
        asn_dec_rval_t dec_rval = uper_decode(NULL,
                                              &asn_DEF_NR_UE_MRDC_Capability,
                                              (void **)&UE->UE_Capability_MRDC,
                                              ue_cap_container->ue_CapabilityRAT_Container.buf,
                                              ue_cap_container->ue_CapabilityRAT_Container.size,
                                              0,
                                              0);

        if (LOG_DEBUGFLAG(DEBUG_ASN1)) {
          xer_fprint(stdout, &asn_DEF_NR_UE_MRDC_Capability, UE->UE_Capability_MRDC);
        }

        if ((dec_rval.code != RC_OK) && (dec_rval.consumed == 0)) {
          LOG_E(NR_RRC, "UE %d: Failed to decode nr UE capabilities (%zu bytes)\n", UE->rrc_ue_id, dec_rval.consumed);
          ASN_STRUCT_FREE(asn_DEF_NR_UE_MRDC_Capability, UE->UE_Capability_MRDC);
          UE->UE_Capability_MRDC = 0;
        }
        UE->UE_MRDC_Capability_size = ue_cap_container->ue_CapabilityRAT_Container.size;
      }

      if (ue_cap_container->rat_Type == NR_RAT_Type_eutra) {
        // TODO
      }
    }

    if (eutra_index == -1)
      return;
  }

  rrc_gNB_send_NGAP_UE_CAPABILITIES_IND(rrc, UE, ue_cap_info);

  if (UE->n_initial_pdu > 0) {
    /* there were PDU sessions with the NG UE Context setup, but we had to set
     * up security and request capabilities, so trigger PDU sessions now. The
     * UE NAS message will be forwarded in the corresponding reconfiguration,
     * the Initial context setup response after reconfiguration complete. */
    if (!trigger_bearer_setup(rrc, UE, UE->n_initial_pdu, UE->initial_pdus, 0)) {
      LOG_W(NR_RRC, "Failed to setup bearers for UE %d: send Initial Context Setup Response\n", UE->rrc_ue_id);
      rrc_gNB_send_NGAP_INITIAL_CONTEXT_SETUP_RESP(rrc, UE);
      rrc_forward_ue_nas_message(rrc, UE);
      return;
    }
  } else {
    rrc_gNB_send_NGAP_INITIAL_CONTEXT_SETUP_RESP(rrc, UE);
    rrc_forward_ue_nas_message(rrc, UE);
  }

  return;
}

static void handle_rrcSetupComplete(gNB_RRC_INST *rrc, gNB_RRC_UE_t *UE, const NR_RRCSetupComplete_t *setup_complete)
{
  uint8_t xid = setup_complete->rrc_TransactionIdentifier;
  UE->xids[xid] = RRC_ACTION_NONE;

  if (setup_complete->criticalExtensions.present != NR_RRCSetupComplete__criticalExtensions_PR_rrcSetupComplete) {
    LOG_E(NR_RRC, "malformed RRCSetupComplete received from UE %d\n", UE->rrc_ue_id);
    return;
  }

  NR_RRCSetupComplete_IEs_t *setup_complete_ies = setup_complete->criticalExtensions.choice.rrcSetupComplete;

  if (setup_complete_ies->ng_5G_S_TMSI_Value != NULL) {
    uint64_t fiveg_s_TMSI = 0;
    if (setup_complete_ies->ng_5G_S_TMSI_Value->present == NR_RRCSetupComplete_IEs__ng_5G_S_TMSI_Value_PR_ng_5G_S_TMSI_Part2) {
      const BIT_STRING_t *part2 = &setup_complete_ies->ng_5G_S_TMSI_Value->choice.ng_5G_S_TMSI_Part2;
      if (part2->size != 2) {
        LOG_E(NR_RRC, "wrong ng_5G_S_TMSI_Part2 size, expected 2, provided %lu", part2->size);
        return;
      }

      if (UE->Initialue_identity_5g_s_TMSI.presence) {
        uint16_t stmsi_part2 = BIT_STRING_to_uint16(part2);
        LOG_I(RRC, "s_tmsi part2 %d (%02x %02x)\n", stmsi_part2, part2->buf[0], part2->buf[1]);
        // Part2 is leftmost 9, Part1 is rightmost 39 bits of 5G-S-TMSI
        fiveg_s_TMSI = ((uint64_t) stmsi_part2) << 39 | UE->ng_5G_S_TMSI_Part1;
      } else {
        LOG_W(RRC, "UE %d received 5G-S-TMSI-Part2, but no 5G-S-TMSI-Part1 present, won't send 5G-S-TMSI to core\n", UE->rrc_ue_id);
        UE->Initialue_identity_5g_s_TMSI.presence = false;
      }
    } else if (setup_complete_ies->ng_5G_S_TMSI_Value->present == NR_RRCSetupComplete_IEs__ng_5G_S_TMSI_Value_PR_ng_5G_S_TMSI) {
      const NR_NG_5G_S_TMSI_t *bs_stmsi = &setup_complete_ies->ng_5G_S_TMSI_Value->choice.ng_5G_S_TMSI;
      if (bs_stmsi->size != 6) {
        LOG_E(NR_RRC, "wrong ng_5G_S_TMSI size, expected 6, provided %lu", bs_stmsi->size);
        return;
      }

      fiveg_s_TMSI = BIT_STRING_to_uint64(bs_stmsi);
      UE->Initialue_identity_5g_s_TMSI.presence = true;
    }

    if (UE->Initialue_identity_5g_s_TMSI.presence) {
      uint16_t amf_set_id = fiveg_s_TMSI >> 38;
      uint8_t amf_pointer = (fiveg_s_TMSI >> 32) & 0x3F;
      uint32_t fiveg_tmsi = (uint32_t) fiveg_s_TMSI;
      LOG_I(NR_RRC,
            "5g_s_TMSI: 0x%lX, amf_set_id: 0x%X (%d), amf_pointer: 0x%X (%d), 5g TMSI: 0x%X \n",
            fiveg_s_TMSI,
            amf_set_id,
            amf_set_id,
            amf_pointer,
            amf_pointer,
            fiveg_tmsi);
      UE->Initialue_identity_5g_s_TMSI.amf_set_id = amf_set_id;
      UE->Initialue_identity_5g_s_TMSI.amf_pointer = amf_pointer;
      UE->Initialue_identity_5g_s_TMSI.fiveg_tmsi = fiveg_tmsi;

      // update random identity with 5G-S-TMSI, which only contained Part1 of it
      UE->random_ue_identity = fiveg_s_TMSI;
    }
  }

#ifdef E2_AGENT
  const uint32_t msg_id = NR_UL_DCCH_MessageType__c1_PR_rrcSetupComplete;
  signal_ue_id(UE, UL_DCCH_NR_RRC_CLASS, msg_id);
#endif

  rrc_gNB_process_RRCSetupComplete(rrc, UE, setup_complete->criticalExtensions.choice.rrcSetupComplete);
  return;
}

static void handle_rrcReconfigurationComplete(gNB_RRC_INST *rrc, gNB_RRC_UE_t *UE, const NR_RRCReconfigurationComplete_t *reconfig_complete)
{
  uint8_t xid = reconfig_complete->rrc_TransactionIdentifier;
  UE->ue_reconfiguration_counter++;
  UE->ongoing_reconfiguration = false;

  switch (UE->xids[xid]) {
    case RRC_PDUSESSION_RELEASE: {
      rrc_gNB_send_NGAP_PDUSESSION_RELEASE_RESPONSE(rrc, UE, xid);
      reset_delayed_action(&UE->delayed_action);
    } break;
    case RRC_PDUSESSION_ESTABLISH:
      if (UE->n_initial_pdu > 0) {
        /* PDU sessions through initial UE context setup */
        rrc_gNB_send_NGAP_INITIAL_CONTEXT_SETUP_RESP(rrc, UE);
        UE->n_initial_pdu = 0;
        free(UE->initial_pdus);
        UE->initial_pdus = NULL;
      } else if (seq_arr_size(&UE->pduSessions) > 0)
        rrc_gNB_send_NGAP_PDUSESSION_SETUP_RESP(rrc, UE, xid);
      else
        LOG_W(NR_RRC,
              "UE %d: RRC Reconfiguration Complete for PDU session establishment, but no PDU sessions were setup\n",
              UE->rrc_ue_id);
      reset_delayed_action(&UE->delayed_action);
      break;
    case RRC_PDUSESSION_MODIFY:
      rrc_gNB_send_NGAP_PDUSESSION_MODIFY_RESP(rrc, UE, xid);
      break;
    case RRC_REESTABLISH_COMPLETE:
    case RRC_DEDICATED_RECONF:
      /* do nothing */
      break;
    case RRC_ACTION_NONE:
      LOG_E(RRC, "UE %d: Received RRC Reconfiguration Complete with xid %d while no transaction is ongoing\n", UE->rrc_ue_id, xid);
      break;
    default:
      LOG_E(RRC, "UE %d: Received unexpected transaction type %d for xid %d\n", UE->rrc_ue_id, UE->xids[xid], xid);
      break;
  }

  UE->xids[xid] = RRC_ACTION_NONE;
  for (int i = 0; i < NR_RRC_TRANSACTION_IDENTIFIER_NUMBER; ++i) {
    if (UE->xids[i] != RRC_ACTION_NONE) {
      LOG_I(RRC, "UE %d: transaction %d still ongoing for action %d\n", UE->rrc_ue_id, i, UE->xids[i]);
    }
  }

  if (UE->ho_context != NULL) {
    LOG_A(NR_RRC, "handover for UE %d/RNTI %04x complete!\n", UE->rrc_ue_id, UE->rnti);
    DevAssert(UE->ho_context->target != NULL);

    UE->ho_context->target->ho_success(rrc, UE);
    nr_rrc_finalize_ho(UE);
  }

  f1_ue_data_t ue_data = cu_get_f1_ue_data(UE->rrc_ue_id);
  RETURN_IF_INVALID_ASSOC_ID(ue_data.du_assoc_id);
  ReconfigurationCompl_t rc = RRCreconf_success;
  f1ap_ue_context_mod_req_t req = {
      .gNB_CU_ue_id = UE->rrc_ue_id,
      .gNB_DU_ue_id = ue_data.secondary_ue,
      .reconfig_compl = &rc,
  };
  rrc->mac_rrc.ue_context_modification_request(ue_data.du_assoc_id, &req);
  /* nothing to be freed */
}

static void rrc_gNB_generate_UECapabilityEnquiry(gNB_RRC_INST *rrc, gNB_RRC_UE_t *ue)
{
  uint8_t buffer[100];

  T(T_ENB_RRC_UE_CAPABILITY_ENQUIRY, T_INT(rrc->module_id), T_INT(0), T_INT(0), T_INT(ue->rrc_ue_id));
  uint8_t xid = rrc_gNB_get_next_transaction_identifier(rrc->module_id);
  ue->xids[xid] = RRC_UECAPABILITY_ENQUIRY;
  int size = do_NR_SA_UECapabilityEnquiry(buffer, xid);
  LOG_I(NR_RRC, "UE %d: Logical Channel DL-DCCH, Generate NR UECapabilityEnquiry (bytes %d, xid %d)\n", ue->rrc_ue_id, size, xid);

  AssertFatal(!NODE_IS_DU(rrc->node_type), "illegal node type DU!\n");

  const uint32_t msg_id = NR_DL_DCCH_MessageType__c1_PR_ueCapabilityEnquiry;
  nr_rrc_transfer_protected_rrc_message(rrc, ue, DL_SCH_LCID_DCCH, msg_id, buffer, size);
}

static int rrc_gNB_decode_dcch(gNB_RRC_INST *rrc, const f1ap_ul_rrc_message_t *msg)
{
  /* we look up by CU UE ID! Do NOT change back to RNTI! */
  rrc_gNB_ue_context_t *ue_context_p = rrc_gNB_get_ue_context(rrc, msg->gNB_CU_ue_id);
  if (!ue_context_p) {
    LOG_E(RRC, "could not find UE context for CU UE ID %u, aborting transaction\n", msg->gNB_CU_ue_id);
    return -1;
  }
  gNB_RRC_UE_t *UE = &ue_context_p->ue_context;

  if (msg->srb_id < 1 || msg->srb_id > 2) {
    LOG_E(NR_RRC, "Received message on SRB %d, discarding message\n", msg->srb_id);
    return -1;
  }

  LOG_D(NR_RRC, "UE %d: Decoding DCCH %d size %d\n", UE->rrc_ue_id, msg->srb_id, msg->rrc_container_length);
  LOG_DUMPMSG(RRC, DEBUG_RRC, (char *)msg->rrc_container, msg->rrc_container_length, "[MSG] RRC UL Information Transfer \n");
  NR_UL_DCCH_Message_t *ul_dcch_msg = NULL;
  asn_dec_rval_t dec_rval =
      uper_decode(NULL, &asn_DEF_NR_UL_DCCH_Message, (void **)&ul_dcch_msg, msg->rrc_container, msg->rrc_container_length, 0, 0);

  if ((dec_rval.code != RC_OK) && (dec_rval.consumed == 0)) {
    LOG_E(NR_RRC, "UE %d: Failed to decode UL-DCCH (%zu bytes)\n", UE->rrc_ue_id, dec_rval.consumed);
    return -1;
  }

  if (LOG_DEBUGFLAG(DEBUG_ASN1)) {
    xer_fprint(stdout, &asn_DEF_NR_UL_DCCH_Message, (void *)ul_dcch_msg);
  }

  if (ul_dcch_msg->message.present == NR_UL_DCCH_MessageType_PR_c1) {
#ifdef E2_AGENT
    // 38.331 Sec 6.2.1: message index of UL-DCCH-Message
    const uint32_t rrc_msg_id = ul_dcch_msg->message.choice.c1->present;
    byte_array_t buffer_ba = {.len = msg->rrc_container_length};
    buffer_ba.buf = msg->rrc_container;
    signal_rrc_msg(UL_DCCH_NR_RRC_CLASS, rrc_msg_id, buffer_ba);
#endif
    switch (ul_dcch_msg->message.choice.c1->present) {
      case NR_UL_DCCH_MessageType__c1_PR_NOTHING:
        LOG_I(NR_RRC, "Received PR_NOTHING on UL-DCCH-Message\n");
        break;

      case NR_UL_DCCH_MessageType__c1_PR_rrcReconfigurationComplete:
        LOG_UE_UL_EVENT(UE, "Received RRCReconfigurationComplete\n");
        handle_rrcReconfigurationComplete(rrc, UE, ul_dcch_msg->message.choice.c1->choice.rrcReconfigurationComplete);
        break;

      case NR_UL_DCCH_MessageType__c1_PR_rrcSetupComplete:
        LOG_UE_UL_EVENT(UE, "Received RRCSetupComplete (RRC_CONNECTED reached)\n");
        handle_rrcSetupComplete(rrc, UE, ul_dcch_msg->message.choice.c1->choice.rrcSetupComplete);
        break;

      case NR_UL_DCCH_MessageType__c1_PR_measurementReport:
        if (ul_dcch_msg->message.choice.c1->choice.measurementReport != NULL) {
          rrc_gNB_process_MeasurementReport(rrc, UE, ul_dcch_msg->message.choice.c1->choice.measurementReport);
        } else {
          LOG_E(NR_RRC, "UE %d: No measurementReport CHOICE is given\n", ue_context_p->ue_context.rrc_ue_id);
        }
        break;

      case NR_UL_DCCH_MessageType__c1_PR_ulInformationTransfer:
        LOG_UE_UL_EVENT(UE, "Received RRC UL Information Transfer [%d bytes]\n", msg->rrc_container_length);
        rrc_gNB_send_NGAP_UPLINK_NAS(rrc, UE, ul_dcch_msg);
        break;

      case NR_UL_DCCH_MessageType__c1_PR_securityModeComplete:
        LOG_UE_UL_EVENT(UE, "Received Security Mode Complete\n");
        /* configure ciphering */
        nr_rrc_pdcp_config_security(UE, true);
        UE->as_security_active = true;

        /* trigger UE capability enquiry if we don't have them yet */
        if (UE->ue_cap_buffer.len == 0) {
          rrc_gNB_generate_UECapabilityEnquiry(rrc, UE);
          /* else blocks are executed after receiving UE capability info */
        } else if (UE->n_initial_pdu > 0) {
          /* there were PDU sessions with the NG UE Context setup, but we had
           * to set up security, so trigger PDU sessions now. The UE NAS
           * message will be forwarded in the corresponding reconfiguration,
           * the Initial context setup response after reconfiguration complete. */
          if (!trigger_bearer_setup(rrc, UE, UE->n_initial_pdu, UE->initial_pdus, 0)) {
            LOG_W(NR_RRC, "Failed to setup bearers for UE %d: send Initial Context Setup Response\n", UE->rrc_ue_id);
            rrc_gNB_send_NGAP_INITIAL_CONTEXT_SETUP_RESP(rrc, UE);
            rrc_forward_ue_nas_message(rrc, UE);
          }
        } else {
          /* we already have capabilities, and no PDU sessions to setup, ack
           * this UE */
          rrc_gNB_send_NGAP_INITIAL_CONTEXT_SETUP_RESP(rrc, UE);
          rrc_forward_ue_nas_message(rrc, UE);
        }
        break;

      case NR_UL_DCCH_MessageType__c1_PR_securityModeFailure:
        LOG_E(NR_RRC, "UE %d: received securityModeFailure\n", ue_context_p->ue_context.rrc_ue_id);
        LOG_W(NR_RRC, "Cannot continue as no AS security is activated (implementation missing)\n");
        break;

      case NR_UL_DCCH_MessageType__c1_PR_ueCapabilityInformation:
        handle_ueCapabilityInformation(rrc, UE, ul_dcch_msg->message.choice.c1->choice.ueCapabilityInformation);
        break;

      case NR_UL_DCCH_MessageType__c1_PR_rrcReestablishmentComplete:
        LOG_UE_UL_EVENT(UE, "Received RRCReestablishmentComplete\n");
        handle_rrcReestablishmentComplete(rrc, UE, ul_dcch_msg->message.choice.c1->choice.rrcReestablishmentComplete);
        break;

      default:
        break;
    }
  }
  ASN_STRUCT_FREE(asn_DEF_NR_UL_DCCH_Message, ul_dcch_msg);
  return 0;
}

void rrc_gNB_process_initial_ul_rrc_message(sctp_assoc_t assoc_id, const f1ap_initial_ul_rrc_message_t *ul_rrc)
{
  AssertFatal(assoc_id != 0, "illegal assoc_id == 0: should be -1 (monolithic) or >0 (split)\n");

  gNB_RRC_INST *rrc = RC.nrrrc[0];
  LOG_I(NR_RRC, "Decoding CCCH: RNTI %04x, payload_size %d\n", ul_rrc->crnti, ul_rrc->rrc_container_length);
  NR_UL_CCCH_Message_t *ul_ccch_msg = NULL;
  asn_dec_rval_t dec_rval = uper_decode(NULL,
                                        &asn_DEF_NR_UL_CCCH_Message,
                                        (void **)&ul_ccch_msg,
                                        ul_rrc->rrc_container,
                                        ul_rrc->rrc_container_length,
                                        0,
                                        0);
  if (dec_rval.code != RC_OK || dec_rval.consumed == 0) {
    LOG_E(NR_RRC, " FATAL Error in receiving CCCH\n");
    return;
  }

  if (ul_ccch_msg->message.present == NR_UL_CCCH_MessageType_PR_c1) {
    switch (ul_ccch_msg->message.choice.c1->present) {
      case NR_UL_CCCH_MessageType__c1_PR_NOTHING:
        LOG_W(NR_RRC, "Received PR_NOTHING on UL-CCCH-Message, ignoring message\n");
        break;

      case NR_UL_CCCH_MessageType__c1_PR_rrcSetupRequest:
        LOG_D(NR_RRC, "Received RRCSetupRequest on UL-CCCH-Message (UE rnti %04x)\n", ul_rrc->crnti);
        rrc_handle_RRCSetupRequest(rrc, assoc_id, &ul_ccch_msg->message.choice.c1->choice.rrcSetupRequest->rrcSetupRequest, ul_rrc);
        break;

      case NR_UL_CCCH_MessageType__c1_PR_rrcResumeRequest:
        LOG_E(NR_RRC, "Received rrcResumeRequest message, but handling is not implemented\n");
        break;

      case NR_UL_CCCH_MessageType__c1_PR_rrcReestablishmentRequest: {
        LOG_D(NR_RRC, "Received RRCReestablishmentRequest on UL-CCCH-Message (UE RNTI %04x)\n", ul_rrc->crnti);
        rrc_handle_RRCReestablishmentRequest(
            rrc,
            assoc_id,
            &ul_ccch_msg->message.choice.c1->choice.rrcReestablishmentRequest->rrcReestablishmentRequest,
            ul_rrc);
      } break;

      case NR_UL_CCCH_MessageType__c1_PR_rrcSystemInfoRequest:
        LOG_I(NR_RRC, "UE %04x receive rrcSystemInfoRequest message \n", ul_rrc->crnti);
        /* TODO */
        break;

      default:
        LOG_E(NR_RRC, "UE %04x Unknown message\n", ul_rrc->crnti);
        break;
    }
  }
  ASN_STRUCT_FREE(asn_DEF_NR_UL_CCCH_Message, ul_ccch_msg);
}

static void rrc_gNB_trigger_nsa_release(module_id_t mod_id, int ue_id)
{
  gNB_RRC_INST *rrc = RC.nrrrc[mod_id];
  rrc_gNB_ue_context_t *ue_context = rrc_gNB_get_ue_context(rrc, ue_id);
  if (!ue_context) {
    LOG_E(NR_RRC, "could not find UE for ID %d\n", ue_id);
    return;
  }
  rrc_release_nsa_user(rrc, ue_context);
}

void rrc_gNB_process_release_request(const module_id_t gnb_mod_idP, x2ap_ENDC_sgnb_release_request_t *m)
{
  /* it's not the RNTI, it's the UE ID ... */
  rrc_gNB_trigger_nsa_release(gnb_mod_idP, m->rnti);
}

void rrc_gNB_process_dc_overall_timeout(const module_id_t gnb_mod_idP, x2ap_ENDC_dc_overall_timeout_t *m)
{
  /* it's not the RNTI, it's the UE ID ... */
  rrc_gNB_trigger_nsa_release(gnb_mod_idP, m->rnti);
}

/* \brief fill E1 bearer modification's DRB from F1 DRB
 * \param drb_e1 pointer to a DRB inside an E1 bearer modification message
 * \param drb_f1 pointer to a DRB inside an F1 UE Ctxt modification Response */
static DRB_nGRAN_to_mod_t fill_e1_bearer_modif_from_f1(const f1ap_drb_setup_t *drb_f1)
{
  DRB_nGRAN_to_mod_t drb_e1 = {0};
  drb_e1.id = drb_f1->id;
  drb_e1.numDlUpParam = drb_f1->up_dl_tnl_len;
  drb_e1.DlUpParamList[0].tl_info.tlAddress = drb_f1->up_dl_tnl[0].tl_address;
  drb_e1.DlUpParamList[0].tl_info.teId = drb_f1->up_dl_tnl[0].teid;
  return drb_e1;
}

static gtpu_tunnel_t f1u_gtp_update(uint32_t teid, const in_addr_t addr)
{
  gtpu_tunnel_t out = {0};
  out.teid = teid;
  memcpy(&out.addr.buffer, &addr, 4);
  out.addr.length = sizeof(addr);
  return out;
}

/**
 * @brief Update DRB TEID information in RRC storage from received DRB list
 */
static void store_du_f1u_tunnel(const f1ap_drb_setup_t *drbs, int n, gNB_RRC_UE_t *ue)
{
  for (int i = 0; i < n; i++) {
    const f1ap_drb_setup_t *drb_f1 = &drbs[i];
    AssertFatal(drb_f1->up_dl_tnl_len == 1, "can handle only one UP param\n");
    AssertFatal(drb_f1->id < MAX_DRBS_PER_UE, "illegal DRB ID %d\n", drb_f1->id);
    drb_t *drb = get_drb(&ue->drbs, drb_f1->id);
    DevAssert(drb);
    drb->du_tunnel_config = f1u_gtp_update(drb_f1->up_dl_tnl[0].teid, drb_f1->up_dl_tnl[0].tl_address);
  }
}

static DRB_nGRAN_to_mod_t get_e1_drb_mod_pdcp_status(const drb_t *drb,
                                                     bearer_context_pdcp_config_t *pdcp_config,
                                                     const ngap_drb_status_t *drb_status)
{
  DevAssert(drb_status);
  DevAssert(pdcp_config);
  DevAssert(drb);
  DRB_nGRAN_to_mod_t drb_to_mod = {0};
  drb_to_mod.id = drb->drb_id;
  drb_to_mod.pdcp_sn_status_requested = false;
  // PDCP SN Status Information
  drb_to_mod.pdcp_config = calloc_or_fail(1, sizeof(*drb_to_mod.pdcp_config));
  *drb_to_mod.pdcp_config = *pdcp_config;
  drb_to_mod.pdcp_status = calloc_or_fail(1, sizeof(*drb_to_mod.pdcp_status));
  drb_to_mod.pdcp_status->dl_count.hfn = drb_status->dl_count.hfn;
  drb_to_mod.pdcp_status->dl_count.sn = drb_status->dl_count.pdcp_sn;
  drb_to_mod.pdcp_status->ul_count.hfn = drb_status->ul_count.hfn;
  drb_to_mod.pdcp_status->ul_count.sn = drb_status->ul_count.pdcp_sn;
  return drb_to_mod;
}

/** @brief Send E1 bearer context modification request to CU-UP */
static void e1_send_bearer_modification_request(gNB_RRC_INST *rrc, gNB_RRC_UE_t *UE, e1ap_bearer_mod_req_t *req)
{
  if (req->numPDUSessionsMod == 0) {
    LOG_W(NR_RRC, "UE %d: No PDU sessions to modify in E1 bearer update\n", UE->rrc_ue_id);
    return;
  }

  DevAssert(req->numPDUSessions == 0);
  req->secInfo = malloc_or_fail(sizeof(*req->secInfo));
  fill_security_info(rrc, UE, req->secInfo);

  sctp_assoc_t assoc_id = get_existing_cuup_for_ue(rrc, UE);
  rrc->cucp_cuup.bearer_context_mod(assoc_id, req);
}

/** @brief Send E1 bearer updates for DRBs to setup from F1 UE Context Modification Response */
static void e1_send_bearer_updates(gNB_RRC_INST *rrc, gNB_RRC_UE_t *UE, int n, f1ap_drb_setup_t *drbs)
{
  if (!is_cuup_associated(rrc) || n <= 0)
    return;

  e1ap_bearer_mod_req_t req = {
    .gNB_cu_cp_ue_id = UE->rrc_ue_id,
    .gNB_cu_up_ue_id = UE->rrc_ue_id,
  };

  for (int i = 0; i < n; i++) {
    const f1ap_drb_setup_t *drb_f1 = &drbs[i];
    int drb_id = drb_f1->id;
    rrc_pdu_session_param_t *pdu = find_pduSession_from_drbId(UE, drb_id);
    if (!pdu)
      continue;
    DRB_nGRAN_to_mod_t drb_to_mod = fill_e1_bearer_modif_from_f1(drb_f1);
    if (!append_e1_drb_mod_req(&req, UE, drb_id, &drb_to_mod)) {
      LOG_W(NR_RRC, "UE %d: Failed to append E1 DRB mod from F1 (DRB=%d)\n", UE->rrc_ue_id, drb_id);
    }
  }

  e1_send_bearer_modification_request(rrc, UE, &req);
  free_e1ap_context_mod_request(&req);
}

/** @brief Request PDCP status from CU-UP during inter-CU handover */
static void e1_request_pdcp_status(gNB_RRC_INST *rrc, gNB_RRC_UE_t *UE)
{
  if (!is_cuup_associated(rrc))
    return;

  e1ap_bearer_mod_req_t req = {
      .gNB_cu_cp_ue_id = UE->rrc_ue_id,
      .gNB_cu_up_ue_id = UE->rrc_ue_id,
  };

  FOR_EACH_SEQ_ARR(drb_t *, drb, &UE->drbs) {
    DRB_nGRAN_to_mod_t drb_to_mod = {.id = drb->drb_id, .pdcp_sn_status_requested = true};
    LOG_D(NR_RRC, "PDCP Status requested (drb_id=%d)\n", drb->drb_id);
    if (!append_e1_drb_mod_req(&req, UE, drb->drb_id, &drb_to_mod)) {
      LOG_W(NR_RRC, "UE %d: Failed to append E1 DRB mod for PDCP status request (DRB=%d)\n", UE->rrc_ue_id, drb->drb_id);
    }
  }

  e1_send_bearer_modification_request(rrc, UE, &req);
  free_e1ap_context_mod_request(&req);
}

/** @brief Notify CU-UP with PDCP status during handover */
void e1_notify_pdcp_status(gNB_RRC_INST *rrc, gNB_RRC_UE_t *UE, const ngap_drb_status_t *drb_status)
{
  if (!is_cuup_associated(rrc) || !drb_status)
    return;

  e1ap_bearer_mod_req_t req = {
      .gNB_cu_cp_ue_id = UE->rrc_ue_id,
      .gNB_cu_up_ue_id = UE->rrc_ue_id,
  };

  FOR_EACH_SEQ_ARR(drb_t *, drb, &UE->drbs) {
    LOG_I(NR_RRC, "Forward PDCP Status to CU-UP (drb_id=%d)\n", drb->drb_id);
    bearer_context_pdcp_config_t pdcp_config = set_bearer_context_pdcp_config(drb->pdcp_config, rrc->configuration.um_on_default_drb, UE->redcap_cap);
    DRB_nGRAN_to_mod_t drb_to_mod = get_e1_drb_mod_pdcp_status(drb, &pdcp_config, drb_status);
    append_e1_drb_mod_req(&req, UE, drb->drb_id, &drb_to_mod);
  }

  e1_send_bearer_modification_request(rrc, UE, &req);
  free_e1ap_context_mod_request(&req);
}

static void rrc_CU_process_ue_context_setup_response(MessageDef *msg_p, instance_t instance)
{
  f1ap_ue_context_setup_resp_t *resp = &F1AP_UE_CONTEXT_SETUP_RESP(msg_p);
  gNB_RRC_INST *rrc = RC.nrrrc[instance];
  rrc_gNB_ue_context_t *ue_context_p = rrc_gNB_get_ue_context(rrc, resp->gNB_CU_ue_id);
  if (!ue_context_p) {
    LOG_E(RRC, "could not find UE context for CU UE ID %u, aborting transaction\n", resp->gNB_CU_ue_id);
    return;
  }
  gNB_RRC_UE_t *UE = &ue_context_p->ue_context;
  UE->f1_ue_context_active = true;

  NR_CellGroupConfig_t *cellGroupConfig = NULL;
  byte_array_t *cgc = &resp->du_to_cu_rrc_info.cell_group_config;
  asn_dec_rval_t dec_rval =
      uper_decode_complete(NULL, &asn_DEF_NR_CellGroupConfig, (void **)&cellGroupConfig, (uint8_t *)cgc->buf, cgc->len);
  AssertFatal(dec_rval.code == RC_OK && dec_rval.consumed > 0, "Cell group config decode error\n");

  if (resp->du_to_cu_rrc_info.meas_gap_config) {
    NR_MeasGapConfig_t *measGapConfig = NULL;
    asn_dec_rval_t dec_rval_mgc = uper_decode_complete(NULL,
                                                       &asn_DEF_NR_MeasGapConfig,
                                                       (void **)&measGapConfig,
                                                       (uint8_t *)resp->du_to_cu_rrc_info.meas_gap_config->buf,
                                                       resp->du_to_cu_rrc_info.meas_gap_config->len);
    AssertFatal(dec_rval_mgc.code == RC_OK && dec_rval_mgc.consumed > 0, "measGapConfig decode error\n");
    UE->measConfig->measGapConfig = measGapConfig;
  }

  if (UE->masterCellGroup) {
    ASN_STRUCT_FREE(asn_DEF_NR_CellGroupConfig, UE->masterCellGroup);
    LOG_I(RRC, "UE %04x replacing existing CellGroupConfig with new one received from DU\n", UE->rnti);
  }
  UE->masterCellGroup = cellGroupConfig;
  if (LOG_DEBUGFLAG(DEBUG_ASN1))
    xer_fprint(stdout, &asn_DEF_NR_CellGroupConfig, UE->masterCellGroup);

  if (!IS_SA_MODE(get_softmodem_params())) {
    rrc_add_nsa_user_resp(rrc, UE, resp);
    return;
  }

  if (resp->drbs_len > 0) {
    int num_drb = seq_arr_size(&UE->drbs);
    DevAssert(num_drb == 0 || num_drb == resp->drbs_len);

    /* Note: we would ideally check that SRB2 is acked, but at least LiteOn DU
     * seems buggy and does not ack, so simply check that locally we activated */
    AssertFatal(UE->Srb[1].Active && UE->Srb[2].Active, "SRBs 1 and 2 must be active during DRB Establishment");
    store_du_f1u_tunnel(resp->drbs, resp->drbs_len, UE);
    if (num_drb == 0)
      e1_send_bearer_updates(rrc, UE, resp->drbs_len, resp->drbs);
    else
      cuup_notify_reestablishment(rrc, UE);
  }

  if (UE->ho_context == NULL) {
    rrc_gNB_generate_dedicatedRRCReconfiguration(rrc, UE);
  } else {
    // case of handover
    // handling of "target CU" information
    DevAssert(UE->ho_context->target != NULL);
    DevAssert(resp->crnti != NULL);
    UE->ho_context->target->du_ue_id = resp->gNB_DU_ue_id;
    UE->ho_context->target->new_rnti = *resp->crnti;
    UE->ho_context->target->ho_req_ack(rrc, UE);
  }
}

static void rrc_CU_process_ue_context_release_request(MessageDef *msg_p, sctp_assoc_t assoc_id)
{
  const int instance = 0;
  f1ap_ue_context_rel_req_t *req = &F1AP_UE_CONTEXT_RELEASE_REQ(msg_p);
  gNB_RRC_INST *rrc = RC.nrrrc[instance];
  rrc_gNB_ue_context_t *ue_context_p = rrc_gNB_get_ue_context(rrc, req->gNB_CU_ue_id);
  int srbid = 1;
  // valid AMF UE NGAP ID range is 0..2^40-1
  if (!ue_context_p || ue_context_p->ue_context.amf_ue_ngap_id >= (1LL << 40)) {
    const char *reason = !ue_context_p ? "could not find UE context" : "no AMF";
    LOG_W(RRC, "%s for CU UE ID %u: auto-generate release command\n", reason, req->gNB_CU_ue_id);
    uint8_t buffer[NR_RRC_BUF_SIZE] = {0};
    int size = do_NR_RRCRelease(buffer, NR_RRC_BUF_SIZE, rrc_gNB_get_next_transaction_identifier(0));
    RETURN_IF_INVALID_ASSOC_ID(assoc_id);
    f1ap_ue_context_rel_cmd_t ue_context_release_cmd = {
        .gNB_CU_ue_id = req->gNB_CU_ue_id,
        .gNB_DU_ue_id = req->gNB_DU_ue_id,
        .cause = F1AP_CAUSE_RADIO_NETWORK,
        .cause_value = 10, // 10 = F1AP_CauseRadioNetwork_normal_release
        .srb_id = &srbid, // C-ifRRCContainer => is added below
    };
    deliver_ue_ctxt_release_data_t data = {.rrc = rrc, .release_cmd = &ue_context_release_cmd, .assoc_id = assoc_id};
    nr_pdcp_data_req_srb(req->gNB_CU_ue_id, srbid, rrc_gNB_mui++, size, buffer, rrc_deliver_ue_ctxt_release_cmd, &data);
    return;
  }

  gNB_RRC_UE_t *UE = &ue_context_p->ue_context;
  if (UE->ho_context != NULL) {
    nr_ho_source_cu_t *source_ctx = UE->ho_context->source;
    bool from_source_du = source_ctx && source_ctx->du->assoc_id == assoc_id;
    if (from_source_du) {
      // we received release request from the source DU, but HO is still
      // ongoing; free the UE, and remove the HO context.
      LOG_W(NR_RRC, "UE %d: release request from source DU ID %ld during HO, marking HO as complete\n", UE->rrc_ue_id, source_ctx->du->setup_req->gNB_DU_id);
      RETURN_IF_INVALID_ASSOC_ID(source_ctx->du->assoc_id);
      f1ap_ue_context_rel_cmd_t cmd = {
          .gNB_CU_ue_id = UE->rrc_ue_id,
          .gNB_DU_ue_id = source_ctx->du_ue_id,
          .cause = F1AP_CAUSE_RADIO_NETWORK,
          .cause_value = 5, // 5 = F1AP_CauseRadioNetwork_interaction_with_other_procedure
      };
      rrc->mac_rrc.ue_context_release_command(assoc_id, &cmd);
      nr_rrc_finalize_ho(UE);
      return;
    }
    // if we receive the release request from the target DU (regardless if
    // successful), we assume it is "genuine" and ask the AMF to release
    nr_rrc_finalize_ho(UE);
  }

  /* TODO: marshall types correctly */
  LOG_I(NR_RRC, "received UE Context Release Request for UE %u, forwarding to AMF\n", req->gNB_CU_ue_id);
  ngap_cause_t cause = {.type = NGAP_CAUSE_RADIO_NETWORK, .value = NGAP_CAUSE_RADIO_NETWORK_RADIO_CONNECTION_WITH_UE_LOST};
  rrc_gNB_send_NGAP_UE_CONTEXT_RELEASE_REQ(instance, ue_context_p, cause);
}

static void rrc_delete_ue_data(gNB_RRC_UE_t *UE)
{
  /* Clean up handover context if it exists */
  if (UE->ho_context)
    nr_rrc_finalize_ho(UE);
  ASN_STRUCT_FREE(asn_DEF_NR_UE_NR_Capability, UE->UE_Capability_nr);
  ASN_STRUCT_FREE(asn_DEF_NR_CellGroupConfig, UE->masterCellGroup);
  ASN_STRUCT_FREE(asn_DEF_NR_MeasResults, UE->measResults);
  FREE_AND_ZERO_BYTE_ARRAY(UE->ue_cap_buffer);
  free_MeasConfig(UE->measConfig);
  free(UE->redcap_cap);
  UE->redcap_cap = NULL;
  seq_arr_free(&UE->pduSessions, free_pdusession);
  seq_arr_free(&UE->drbs, free_drb);
}

void rrc_remove_ue(gNB_RRC_INST *rrc, rrc_gNB_ue_context_t *ue_context_p)
{
  gNB_RRC_UE_t *UE = &ue_context_p->ue_context;
  /* we call nr_pdcp_remove_UE() in the handler of E1 bearer release, but if we
   * are in E1, we also need to free the UE in the CU-CP, so call it twice to
   * cover all cases */
  nr_pdcp_remove_UE(UE->rrc_ue_id);
  LOG_I(NR_RRC, "removed UE CU UE ID %u/RNTI %04x \n", UE->rrc_ue_id, UE->rnti);
  rrc_delete_ue_data(UE);
  rrc_gNB_remove_ue_context(rrc, ue_context_p);
}

static void rrc_CU_process_ue_context_release_complete(MessageDef *msg_p)
{
  const int instance = 0;
  f1ap_ue_context_rel_cplt_t *complete = &F1AP_UE_CONTEXT_RELEASE_COMPLETE(msg_p);
  gNB_RRC_INST *rrc = RC.nrrrc[instance];
  rrc_gNB_ue_context_t *ue_context_p = rrc_gNB_get_ue_context(rrc, complete->gNB_CU_ue_id);
  if (!ue_context_p) {
    LOG_E(RRC, "could not find UE context for CU UE ID %u, aborting transaction\n", complete->gNB_CU_ue_id);
    return;
  }

  gNB_RRC_UE_t *UE = &ue_context_p->ue_context;
  if (UE->an_release) {
    /* only trigger release if it has been requested by core
     * otherwise, it might be CU that requested release on a DU during normal
     * operation (i.e, handover) */
    rrc_gNB_send_NGAP_UE_CONTEXT_RELEASE_COMPLETE(0, UE->rrc_ue_id, &UE->pduSessions);
    rrc_remove_ue(RC.nrrrc[0], ue_context_p);
  }
}

static void rrc_CU_process_ue_context_modification_response(MessageDef *msg_p, instance_t instance)
{
  f1ap_ue_context_mod_resp_t *resp = &F1AP_UE_CONTEXT_MODIFICATION_RESP(msg_p);
  gNB_RRC_INST *rrc = RC.nrrrc[instance];
  rrc_gNB_ue_context_t *ue_context_p = rrc_gNB_get_ue_context(rrc, resp->gNB_CU_ue_id);
  if (!ue_context_p) {
    LOG_E(RRC, "could not find UE context for CU UE ID %u, aborting transaction\n", resp->gNB_CU_ue_id);
    return;
  }
  gNB_RRC_UE_t *UE = &ue_context_p->ue_context;

  bool is_inter_cu_ho = UE->ho_context && UE->ho_context->source && !UE->ho_context->target;
  if (resp->drbs_len > 0) { // DRB to setup
    store_du_f1u_tunnel(resp->drbs, resp->drbs_len, UE);
    e1_send_bearer_updates(rrc, UE, resp->drbs_len, resp->drbs);
  } else if (is_inter_cu_ho) { // PDCP status request
    e1_request_pdcp_status(rrc, UE);
  }

  if (resp->du_to_cu_rrc_info) {
    NR_CellGroupConfig_t *cellGroupConfig = NULL;
    byte_array_t *cgc = &resp->du_to_cu_rrc_info->cell_group_config;
    asn_dec_rval_t dec_rval =
        uper_decode_complete(NULL, &asn_DEF_NR_CellGroupConfig, (void **)&cellGroupConfig, (uint8_t *)cgc->buf, cgc->len);
    AssertFatal(dec_rval.code == RC_OK && dec_rval.consumed > 0, "Cell group config decode error\n");

    /* hack for interoperability with srsRAN: ignore cell group config if empty */
    /* cell group config is empty if buffer length is 2 and content is all 0 */
    bool cgc_is_empty = cgc->len == 2 && cgc->buf[0] == 0 && cgc->buf[1] == 0;
    if (!cgc_is_empty) {
      if (UE->masterCellGroup) {
        ASN_STRUCT_FREE(asn_DEF_NR_CellGroupConfig, UE->masterCellGroup);
        LOG_I(RRC, "UE %04x replacing existing CellGroupConfig with new one received from DU\n", UE->rnti);
      }
      UE->masterCellGroup = cellGroupConfig;
      rrc_gNB_generate_dedicatedRRCReconfiguration(rrc, UE);
    } else {
      LOG_W(NR_RRC, "hack: UE %d: ignore empty CellGroupConfig in UEContextModificationResponse\n", UE->rrc_ue_id);
    }
  }

  // Reconfiguration should have been sent to the UE, so it will attempt the
  // handover. In the F1 case, update with new RNTI, and update secondary UE
  // association, so we can receive the new UE from the target DU (in N2/Xn,
  // nothing is to be done, we wait for confirmation to release the UE in the
  // CU/DU)
  if (UE->ho_context && UE->ho_context->target && UE->ho_context->source) {
    nr_ho_target_cu_t *target_ctx = UE->ho_context->target;
    f1_ue_data_t ue_data = cu_get_f1_ue_data(UE->rrc_ue_id);
    ue_data.secondary_ue = target_ctx->du_ue_id;
    ue_data.du_assoc_id = target_ctx->du->assoc_id;
    bool success = cu_update_f1_ue_data(UE->rrc_ue_id, &ue_data);
    DevAssert(success);
    LOG_I(NR_RRC, "UE %d handover: update RNTI from %04x to %04x\n", UE->rrc_ue_id, UE->rnti, target_ctx->new_rnti);
    nr_ho_source_cu_t *source_ctx = UE->ho_context->source;
    DevAssert(source_ctx->old_rnti == UE->rnti);
    UE->rnti = target_ctx->new_rnti;
    UE->nr_cellid = target_ctx->du->setup_req->cell[0].info.nr_cellid;
  }
}

static void rrc_CU_process_ue_modification_required(MessageDef *msg_p, instance_t instance, sctp_assoc_t assoc_id)
{
  gNB_RRC_INST *rrc = RC.nrrrc[instance];
  f1ap_ue_context_modif_required_t *required = &F1AP_UE_CONTEXT_MODIFICATION_REQUIRED(msg_p);
  rrc_gNB_ue_context_t *ue_context_p = rrc_gNB_get_ue_context(rrc, required->gNB_CU_ue_id);
  if (ue_context_p == NULL) {
    LOG_E(RRC, "Could not find UE context for CU UE ID %d, cannot handle UE context modification request\n", required->gNB_CU_ue_id);
    f1ap_ue_context_modif_refuse_t refuse = {
      .gNB_CU_ue_id = required->gNB_CU_ue_id,
      .gNB_DU_ue_id = required->gNB_DU_ue_id,
      .cause = F1AP_CAUSE_RADIO_NETWORK,
      .cause_value = F1AP_CauseRadioNetwork_unknown_or_already_allocated_gnb_cu_ue_f1ap_id,
    };
    rrc->mac_rrc.ue_context_modification_refuse(msg_p->ittiMsgHeader.originInstance, &refuse);
    return;
  }

  gNB_RRC_UE_t *UE = &ue_context_p->ue_context;
  if (UE->ho_context && UE->ho_context->source && UE->ho_context->source->du && UE->ho_context->source->du->assoc_id == assoc_id) {
    LOG_W(NR_RRC, "UE %d: UE Context Modification Required during handover, ignoring message\n", UE->rrc_ue_id);
    return;
  }

  if (required->du_to_cu_rrc_information && required->du_to_cu_rrc_information->cellGroupConfig) {
    gNB_RRC_UE_t *UE = &ue_context_p->ue_context;
    LOG_I(RRC,
          "UE Context Modification Required: new CellGroupConfig for UE ID %d/RNTI %04x, triggering reconfiguration\n",
          UE->rrc_ue_id,
          UE->rnti);

    NR_CellGroupConfig_t *cellGroupConfig = NULL;
    asn_dec_rval_t dec_rval = uper_decode_complete(NULL,
                                                   &asn_DEF_NR_CellGroupConfig,
                                                   (void **)&cellGroupConfig,
                                                   (uint8_t *)required->du_to_cu_rrc_information->cellGroupConfig,
                                                   required->du_to_cu_rrc_information->cellGroupConfig_length);
    if (dec_rval.code != RC_OK && dec_rval.consumed == 0) {
      LOG_E(RRC, "Cell group config decode error, refusing reconfiguration\n");
      f1ap_ue_context_modif_refuse_t refuse = {
        .gNB_CU_ue_id = required->gNB_CU_ue_id,
        .gNB_DU_ue_id = required->gNB_DU_ue_id,
        .cause = F1AP_CAUSE_PROTOCOL,
        .cause_value = F1AP_CauseProtocol_transfer_syntax_error,
      };
      rrc->mac_rrc.ue_context_modification_refuse(msg_p->ittiMsgHeader.originInstance, &refuse);
      return;
    }

    if (UE->masterCellGroup) {
      ASN_STRUCT_FREE(asn_DEF_NR_CellGroupConfig, UE->masterCellGroup);
      LOG_I(RRC, "UE %d/RNTI %04x replacing existing CellGroupConfig with new one received from DU\n", UE->rrc_ue_id, UE->rnti);
    }
    UE->masterCellGroup = cellGroupConfig;
    if (LOG_DEBUGFLAG(DEBUG_ASN1))
      xer_fprint(stdout, &asn_DEF_NR_CellGroupConfig, UE->masterCellGroup);

    /* trigger reconfiguration */
    if (!UE->ongoing_reconfiguration)
      nr_rrc_reconfiguration_req(rrc, UE, 0, 0);
    return;
  }
  LOG_W(RRC,
        "nothing to be done after UE Context Modification Required for UE ID %d/RNTI %04x\n",
        required->gNB_CU_ue_id,
        required->gNB_DU_ue_id);
}

unsigned int mask_flip(unsigned int x) {
  return((((x>>8) + (x<<8))&0xffff)>>6);
}

/* \bref return F1AP QoS characteristics based on Qos flow parameters */
f1ap_qos_flow_param_t get_qos_char_from_qos_flow_param(const pdusession_level_qos_parameter_t *qos_param)
{
  f1ap_qos_flow_param_t qos_char = {0};
  if (qos_param->fiveQI_type == DYNAMIC) {
    qos_char.qos_type = DYNAMIC;
    qos_char.dyn.prio = qos_param->qos_priority;
    qos_char.dyn.pdb = 100; // TODO
    qos_char.dyn.per.scalar = 10; // TODO
    qos_char.dyn.per.exponent = 100; // TODO
  } else {
    qos_char.qos_type = NON_DYNAMIC;
    qos_char.nondyn.fiveQI = qos_param->fiveQI;
  }
  const qos_arp_t *a = &qos_param->arp;
  qos_char.arp.prio = a->priority_level;
  qos_char.arp.preempt_cap =
      a->pre_emp_capability == PEC_MAY_TRIGGER_PREEMPTION ? MAY_TRIGGER_PREEMPTION : SHALL_NOT_TRIGGER_PREEMPTION;
  qos_char.arp.preempt_vuln = a->pre_emp_vulnerability == PEV_PREEMPTABLE ? PREEMPTABLE : NOT_PREEMPTABLE;
  return qos_char;
}

/* \brief fills a list of DRBs to be setup from a number of PDU sessions in E1
 * bearer setup response */
static int fill_drb_to_be_setup_from_e1_resp(const gNB_RRC_INST *rrc,
                                             gNB_RRC_UE_t *UE,
                                             const pdu_session_setup_t *pduSession,
                                             int numPduSession,
                                             f1ap_drb_to_setup_t drbs[32])
{
  int nb_drb = 0;
  for (int p = 0; p < numPduSession; ++p) {
    rrc_pdu_session_param_t *pdu = find_pduSession(&UE->pduSessions, pduSession[p].id);
    DevAssert(pdu);
    for (int i = 0; i < pduSession[p].numDRBSetup; i++) {
      const DRB_nGRAN_setup_t *drb_config = &pduSession[p].DRBnGRanList[i];
      drb_t *rrc_drb = get_drb(&UE->drbs, pduSession[p].DRBnGRanList[i].id);
      nr_pdcp_configuration_t pdcp = rrc_drb->pdcp_config;
      DevAssert(rrc_drb);

      DevAssert(nb_drb < MAX_DRBS_PER_UE);
      f1ap_drb_to_setup_t *drb = &drbs[nb_drb];
      drb->id = rrc_drb->drb_id;

      drb->qos_choice = F1AP_QOS_CHOICE_NR;

      /* pass QoS info to MAC */
      int nb_qos_flows = drb_config->numQosFlowSetup;
      AssertFatal(nb_qos_flows > 0, "must map at least one flow to a DRB\n");
      drb->nr.flows_len = nb_qos_flows;
      drb->nr.flows = calloc_or_fail(nb_qos_flows, sizeof(*drb->nr.flows));
      for (int j = 0; j < nb_qos_flows; j++) {
        int qfi = drb_config->qosFlows[j].qfi;
        /* find the QoS characteristics stored at RRC based on QFI returned
         * from E1. We expect this to correspond to a QFI we passed to E1
         * previously, so we expect it to exist. */
        nr_rrc_qos_t *qos_param = find_qos(&pdu->param.qos, qfi);
        DevAssert(qos_param);
        f1ap_drb_flows_mapped_t *flow = &drb->nr.flows[j];
        flow->qfi = qfi;
        flow->param = get_qos_char_from_qos_flow_param(&qos_param->qos);
      }
      /* the DRB QoS parameters: we just reuse the ones from the first flow */
      drb->nr.drb_qos = drb->nr.flows[0].param;

      /* pass NSSAI info to MAC */
      drb->nr.nssai = pdu->param.nssai;

      drb->up_ul_tnl[0].tl_address = drb_config->UpParamList[0].tl_info.tlAddress;
      drb->up_ul_tnl[0].teid = drb_config->UpParamList[0].tl_info.teId;
      drb->up_ul_tnl_len = 1;
      drb->rlc_mode = rrc->configuration.um_on_default_drb ? F1AP_RLC_MODE_UM_BIDIR : F1AP_RLC_MODE_AM;
      DevAssert(pdcp.drb.sn_size == 18 || pdcp.drb.sn_size == 12);
      drb->dl_pdcp_sn_len = malloc_or_fail(sizeof(*drb->dl_pdcp_sn_len));
      *drb->dl_pdcp_sn_len = pdcp.drb.sn_size == 18 ? F1AP_PDCP_SN_18B : F1AP_PDCP_SN_12B;
      drb->ul_pdcp_sn_len = malloc_or_fail(sizeof(*drb->ul_pdcp_sn_len));
      *drb->ul_pdcp_sn_len = pdcp.drb.sn_size == 18 ? F1AP_PDCP_SN_18B : F1AP_PDCP_SN_12B;

      nb_drb++;
    }
  }
  return nb_drb;
}

/**
 * @brief E1AP Bearer Context Setup Response processing on CU-CP
*/
void rrc_gNB_process_e1_bearer_context_setup_resp(e1ap_bearer_setup_resp_t *resp, instance_t instance)
{
  gNB_RRC_INST *rrc = RC.nrrrc[0];
  rrc_gNB_ue_context_t *ue_context_p = rrc_gNB_get_ue_context(rrc, resp->gNB_cu_cp_ue_id);
  AssertFatal(ue_context_p != NULL, "did not find UE with CU UE ID %d\n", resp->gNB_cu_cp_ue_id);
  gNB_RRC_UE_t *UE = &ue_context_p->ue_context;

  // currently: we don't have "infrastructure" to save the CU-UP UE ID, so we
  // assume (and below check) that CU-UP UE ID == CU-CP UE ID
  AssertFatal(resp->gNB_cu_cp_ue_id == resp->gNB_cu_up_ue_id,
              "cannot handle CU-UP UE ID different from CU-CP UE ID (%d vs %d)\n",
              resp->gNB_cu_cp_ue_id,
              resp->gNB_cu_up_ue_id);

  // save the tunnel address for the PDU sessions
  for (int i = 0; i < resp->numPDUSessions; i++) {
    pdu_session_setup_t *e1_pdu = &resp->pduSession[i];
    rrc_pdu_session_param_t *rrc_pdu = find_pduSession(&UE->pduSessions, e1_pdu->id);
    if (rrc_pdu == NULL) {
      LOG_W(RRC, "E1: received setup for PDU session %ld, but has not been requested\n", e1_pdu->id);
      continue;
    }
    rrc_pdu->param.n3_outgoing = f1u_gtp_update(e1_pdu->tl_info.teId, e1_pdu->tl_info.tlAddress);

    // save the tunnel address for the DRBs
    for (int j = 0; j < e1_pdu->numDRBSetup; j++) {
      DRB_nGRAN_setup_t *drb_config = &e1_pdu->DRBnGRanList[j];
      // numUpParam only relevant in F1, but not monolithic
      AssertFatal(drb_config->numUpParam <= 1, "can only up to one UP param\n");
      drb_t *drb = get_drb(&UE->drbs, drb_config->id);
      if (!drb) {
        LOG_E(RRC, "E1: DRB %ld not found for PDU session %ld\n", drb_config->id, e1_pdu->id);
        continue;
      }
      UP_TL_information_t *tl_info = &drb_config->UpParamList[0].tl_info;
      drb->cuup_tunnel_config = f1u_gtp_update(tl_info->teId, tl_info->tlAddress);
    }
  }

  // If HO Preparation Info is stored, N2 handover is ongoing
  if (UE->ho_context) {
    LOG_I(NR_RRC, "Received Bearer Context Setup Response for UE %d with valid HO Context\n", UE->rrc_ue_id);
    UE->ho_context->target->ho_trigger(rrc, UE);
    return;
  }

  if (!UE->f1_ue_context_active)
    rrc_gNB_generate_UeContextSetupRequest(rrc, ue_context_p, resp);
  else
    rrc_gNB_generate_UeContextModificationRequest(rrc, ue_context_p, resp, 0, NULL);
}

/** @brief E1AP Bearer Context Setup Failure processing on CU-CP */
void rrc_gNB_process_e1_bearer_context_setup_failure(e1ap_bearer_context_setup_failure_t *msg)
{
  LOG_E(RRC,
        "Received E1AP Bearer Context Setup Failure for UE CU-CP ID %d with cause (%d, %d)\n",
        msg->gNB_cu_cp_ue_id,
        msg->cause.type,
        msg->cause.value);
}

/**
 * @brief E1AP Bearer Context Modification Response processing on CU-CP
 */
void rrc_gNB_process_e1_bearer_context_modif_resp(const e1ap_bearer_modif_resp_t *resp)
{
  LOG_I(NR_RRC, "Received E1AP Bearer Context Modification Response for UE CU-CP ID %d\n", resp->gNB_cu_cp_ue_id);
  gNB_RRC_INST *rrc = RC.nrrrc[0];
  rrc_gNB_ue_context_t *ue_context_p = rrc_gNB_get_ue_context(rrc, resp->gNB_cu_cp_ue_id);
  if (ue_context_p == NULL) {
    LOG_E(RRC, "no UE with CU-CP UE ID %d found\n", resp->gNB_cu_cp_ue_id);
    return;
  }
  gNB_RRC_UE_t *ue = &ue_context_p->ue_context;

  int n_drb_mod = 0;
  int drb_ids[MAX_DRBS_PER_UE] = {0};
  int drb_to_release[MAX_DRBS_PER_UE] = {0};
  int n_drb_to_release = 0;
  e1_pdcp_status_info_t pdcp_status[MAX_DRBS_PER_UE] = {0};
  for (int i = 0; i < resp->numPDUSessionsMod; ++i) {
    const pdu_session_modif_t *pdu = &resp->pduSessionMod[i];
    LOG_I(RRC, "UE %d: PDU session ID %ld modified %d bearers\n", resp->gNB_cu_cp_ue_id, pdu->id, pdu->numDRBModified);
    for (int  j = 0; j < pdu->numDRBModified; j++) {
      // Trigger UL RAN Status Transfer
      if (pdu->DRBnGRanModList[j].pdcp_status) {
        DevAssert(n_drb_mod < MAX_DRBS_PER_UE);
        drb_ids[n_drb_mod] = pdu->DRBnGRanModList[j].id;
        pdcp_status[n_drb_mod++] = *pdu->DRBnGRanModList[j].pdcp_status;
      }
    }
    // Collect DRBs to release for PDU sessions marked for release
    rrc_pdu_session_param_t *pdu_session = find_pduSession(&ue->pduSessions, pdu->id);
    if (pdu_session && pdu_session->status == PDU_SESSION_STATUS_TORELEASE) {
      FOR_EACH_SEQ_ARR(drb_t *, drb, &ue->drbs) {
        if (drb->pdusession_id == pdu->id) {
          DevAssert(n_drb_to_release < MAX_DRBS_PER_UE);
          drb_to_release[n_drb_to_release++] = drb->drb_id;
        }
      }
      if (n_drb_to_release == 0) {
        LOG_E(NR_RRC, "UE %d: no DRBs to release for PDU session %ld\n", ue->rrc_ue_id, pdu->id);
      }
    }
  }

  if (n_drb_mod) {
    LOG_I(NR_RRC, "UE %d: received PDU Status Info - send UL RAN Status Transfer\n", resp->gNB_cu_cp_ue_id);
    if (ue->ho_context && ue->ho_context->source)
      ue->ho_context->source->ho_status_transfer(rrc, ue, n_drb_mod, drb_ids, pdcp_status);
  }

  // Send F1 UE Context Modification Request with DRB release
  if (n_drb_to_release) {
    LOG_I(NR_RRC, "Send F1 UE Context Modification Request with DRB to release\n");
    rrc_gNB_send_f1_drb_release_request(rrc, ue, drb_to_release, n_drb_to_release);
  }
}

/** @brief E1AP Bearer Context Modification Failure processing on CU-CP */
static void rrc_gNB_process_e1_bearer_context_modif_fail(const e1ap_bearer_context_mod_failure_t *fail)
{
  gNB_RRC_INST *rrc = RC.nrrrc[0];
  rrc_gNB_ue_context_t *ue_context_p = rrc_gNB_get_ue_context(rrc, fail->gNB_cu_cp_ue_id);
  if (ue_context_p == NULL) {
    LOG_E(NR_RRC, "No UE with CU-CP UE ID %d found (Bearer Context Modification Failure)\n", fail->gNB_cu_cp_ue_id);
    return;
  }

  LOG_W(NR_RRC,
        "Bearer Context Modification Failure received for UE ID %d (CU-UP ID %d), cause: %d\n",
        fail->gNB_cu_cp_ue_id,
        fail->gNB_cu_up_ue_id,
        fail->cause.value);
}

/**
 * @brief E1AP Bearer Context Release processing
 */
void rrc_gNB_process_e1_bearer_context_release_cplt(const e1ap_bearer_release_cplt_t *cplt)
{
  // there is not really anything to do here as of now
  // note that we don't check for the UE: it does not exist anymore if the F1
  // UE context release complete arrived from the DU first, after which we free
  // the UE context
  LOG_I(RRC, "UE %d: received bearer release complete\n", cplt->gNB_cu_cp_ue_id);
}

static void print_meas_result_quantity(FILE *f, const NR_MeasQuantityResults_t *mqr)
{
  fprintf(f, "      resultSSB:");
  if (!mqr) {
    fprintf(f, " NOT PROVIDED\n");
    return;
  }
  if (mqr->rsrp) {
    const long rrsrp = *mqr->rsrp - 156;
    fprintf(f, " RSRP %ld dBm", rrsrp);
  } else {
    fprintf(f, " RSRP not provided");
  }
  if (mqr->rsrq) {
    const float rrsrq = (float) (*mqr->rsrq - 87) / 2.0f;
    fprintf(f, " RSRQ %.1f dB", rrsrq);
  } else {
    fprintf(f, " RSRQ not provided");
  }
  if (mqr->sinr) {
    const float rsinr = (float) (*mqr->sinr - 46) / 2.0f;
    fprintf(f, " SINR %.1f dB", rsinr);
  } else {
    fprintf(f, " SINR not provided");
  }
  fprintf(f, "\n");
}

static void print_rrc_meas(FILE *f, const NR_MeasResults_t *measresults)
{
  DevAssert(measresults->measResultServingMOList.list.count >= 1);
  if (measresults->measResultServingMOList.list.count > 1)
    LOG_W(RRC, "Received %d MeasResultServMO, but handling only 1!\n", measresults->measResultServingMOList.list.count);

  NR_MeasResultServMO_t *measresultservmo = measresults->measResultServingMOList.list.array[0];
  NR_MeasResultNR_t *measresultnr = &measresultservmo->measResultServingCell;
  if (measresultnr->physCellId)
    fprintf(f,
            "    servingCellId %ld MeasResultNR for phyCellId %ld:\n",
            measresultservmo->servCellId,
            *measresultnr->physCellId);
  print_meas_result_quantity(f, measresultnr->measResult.cellResults.resultsSSB_Cell);

  if (measresults->measResultNeighCells
      && measresults->measResultNeighCells->present == NR_MeasResults__measResultNeighCells_PR_measResultListNR) {
    NR_MeasResultListNR_t *meas_neigh = measresults->measResultNeighCells->choice.measResultListNR;
    for (int i = 0; i < meas_neigh->list.count; ++i) {
      NR_MeasResultNR_t *measresultneigh = meas_neigh->list.array[i];
      if (measresultneigh->physCellId)
        fprintf(f, "    neighboring cell for phyCellId %ld:\n", *measresultneigh->physCellId);
      print_meas_result_quantity(f, measresultneigh->measResult.cellResults.resultsSSB_Cell);
    }
  }
}

static const char *get_pdusession_status_text(pdu_session_status_t status)
{
  switch (status) {
    case PDU_SESSION_STATUS_NEW: return "new";
    case PDU_SESSION_STATUS_DONE: return "done";
    case PDU_SESSION_STATUS_ESTABLISHED: return "established";
    case PDU_SESSION_STATUS_REESTABLISHED: return "reestablished";
    case PDU_SESSION_STATUS_TOMODIFY: return "to-modify";
    case PDU_SESSION_STATUS_FAILED: return "failed";
    case PDU_SESSION_STATUS_TORELEASE: return "to-release";
    default: AssertFatal(false, "illegal PDU status code %d\n", status); return "illegal";
  }
  return "illegal";
}

static bool write_rrc_stats(const gNB_RRC_INST *rrc)
{
  const char *filename = "nrRRC_stats.log";
  FILE *f = fopen(filename, "w");
  if (f == NULL) {
    LOG_W(NR_RRC, "cannot open %s for writing: %d, %s\n", filename, errno, strerror(errno));
    return false;
  }

  time_t now = time(NULL);
  int i = 0;
  rrc_gNB_ue_context_t *ue_context_p = NULL;
  /* cast is necessary to eliminate warning "discards ‘const’ qualifier" */
  RB_FOREACH(ue_context_p, rrc_nr_ue_tree_s, &((gNB_RRC_INST *)rrc)->rrc_ue_head)
  {
    const gNB_RRC_UE_t *ue_ctxt = &ue_context_p->ue_context;
    f1_ue_data_t ue_data = cu_get_f1_ue_data(ue_ctxt->rrc_ue_id);

    fprintf(f,
            "UE %d CU UE ID %u DU UE ID %d RNTI %04x random identity %016lx",
            i,
            ue_ctxt->rrc_ue_id,
            ue_data.secondary_ue,
            ue_ctxt->rnti,
            ue_ctxt->random_ue_identity);
    if (ue_ctxt->Initialue_identity_5g_s_TMSI.presence)
      fprintf(f, " S-TMSI %x", ue_ctxt->Initialue_identity_5g_s_TMSI.fiveg_tmsi);
    fprintf(f, ":\n");

    time_t last_seen = now - ue_ctxt->last_seen;
    fprintf(f, "    last RRC activity: %ld seconds ago\n", last_seen);

    FOR_EACH_SEQ_ARR(rrc_pdu_session_param_t *, pdu, &ue_ctxt->pduSessions) {
      fprintf(f, "    PDU session ID %d status %s\n", pdu->param.pdusession_id, get_pdusession_status_text(pdu->status));
    }

    fprintf(f, "    associated DU: ");
    if (ue_data.du_assoc_id == -1)
      fprintf(f, " (local/integrated CU-DU)");
    else if (ue_data.du_assoc_id == 0)
      fprintf(f, " DU offline/unavailable");
    else
      fprintf(f, " DU assoc ID %d", ue_data.du_assoc_id);
    fprintf(f, "\n");

    if (ue_ctxt->measResults)
      print_rrc_meas(f, ue_ctxt->measResults);
    ++i;
  }

  fprintf(f, "\n");
  dump_du_info(rrc, f);

  fclose(f);
  return true;
}

void *rrc_gnb_task(void *args_p) {
  MessageDef *msg_p;
  instance_t                         instance;
  int                                result;

  long stats_timer_id = 1;
  if (!IS_SOFTMODEM_NOSTATS) {
    /* timer to write stats to file */
    timer_setup(1, 0, TASK_RRC_GNB, 0, TIMER_PERIODIC, NULL, &stats_timer_id);
  }

  itti_mark_task_ready(TASK_RRC_GNB);
  LOG_I(NR_RRC,"Entering main loop of NR_RRC message task\n");

  while (1) {
    // Wait for a message
    itti_receive_msg(TASK_RRC_GNB, &msg_p);
    const char *msg_name_p = ITTI_MSG_NAME(msg_p);
    instance = ITTI_MSG_DESTINATION_INSTANCE(msg_p);
    LOG_D(NR_RRC,
          "RRC GNB Task Received %s for instance %ld from task %s\n",
          ITTI_MSG_NAME(msg_p),
          ITTI_MSG_DESTINATION_INSTANCE(msg_p),
          ITTI_MSG_ORIGIN_NAME(msg_p));
    switch (ITTI_MSG_ID(msg_p)) {
      case TERMINATE_MESSAGE:
        LOG_W(NR_RRC, " *** Exiting NR_RRC thread\n");
        timer_remove(stats_timer_id);
        itti_exit_task();
        break;

      case MESSAGE_TEST:
        LOG_I(NR_RRC, "[gNB %ld] Received %s\n", instance, msg_name_p);
        break;

      case TIMER_HAS_EXPIRED:
        if (TIMER_HAS_EXPIRED(msg_p).timer_id == stats_timer_id) {
          if (!write_rrc_stats(RC.nrrrc[0]))
            timer_remove(stats_timer_id);
        } else {
          itti_send_msg_to_task(TASK_RRC_GNB, 0, TIMER_HAS_EXPIRED(msg_p).arg); /* see rrc_gNB_process_NGAP_PDUSESSION_SETUP_REQ() */
        }
        break;

      case F1AP_INITIAL_UL_RRC_MESSAGE:
        AssertFatal(NODE_IS_CU(RC.nrrrc[instance]->node_type) || NODE_IS_MONOLITHIC(RC.nrrrc[instance]->node_type),
                    "should not receive F1AP_INITIAL_UL_RRC_MESSAGE, need call by CU!\n");
        rrc_gNB_process_initial_ul_rrc_message(msg_p->ittiMsgHeader.originInstance, &F1AP_INITIAL_UL_RRC_MESSAGE(msg_p));
        free_initial_ul_rrc_message_transfer(&F1AP_INITIAL_UL_RRC_MESSAGE(msg_p));
        break;

      /* Messages from PDCP */
      /* From DU -> CU */
      case F1AP_UL_RRC_MESSAGE:
        rrc_gNB_decode_dcch(RC.nrrrc[instance], &F1AP_UL_RRC_MESSAGE(msg_p));
        free_ul_rrc_message_transfer(&F1AP_UL_RRC_MESSAGE(msg_p));
        break;

      case NGAP_DOWNLINK_NAS:
        rrc_gNB_process_NGAP_DOWNLINK_NAS(msg_p, instance, &rrc_gNB_mui);
        break;

      case NGAP_PDUSESSION_SETUP_REQ:
        if (!rrc_delay_transaction(instance, msg_p))
          rrc_gNB_process_NGAP_PDUSESSION_SETUP_REQ(msg_p, instance);
        break;

      case NGAP_PDUSESSION_MODIFY_REQ:
        rrc_gNB_process_NGAP_PDUSESSION_MODIFY_REQ(msg_p, instance);
        break;

      case NGAP_PDUSESSION_RELEASE_COMMAND:
        if (!rrc_delay_transaction(instance, msg_p))
          rrc_gNB_process_NGAP_PDUSESSION_RELEASE_COMMAND(&NGAP_PDUSESSION_RELEASE_COMMAND(msg_p), RC.nrrrc[instance]);
        break;

      case NGAP_DL_RAN_STATUS_TRANSFER:
        rrc_gNB_process_NGAP_DL_RAN_STATUS_TRANSFER(msg_p, instance);
        break;

      /* Messages from F1AP task */
      case F1AP_SETUP_REQ:
        AssertFatal(!NODE_IS_DU(RC.nrrrc[instance]->node_type), "should not receive F1AP_SETUP_REQUEST in DU!\n");
        rrc_gNB_process_f1_setup_req(&F1AP_SETUP_REQ(msg_p), msg_p->ittiMsgHeader.originInstance);
        free_f1ap_setup_request(&F1AP_SETUP_REQ(msg_p));
        break;

      case F1AP_UE_CONTEXT_SETUP_RESP:
        rrc_CU_process_ue_context_setup_response(msg_p, instance);
        free_ue_context_setup_resp(&F1AP_UE_CONTEXT_SETUP_RESP(msg_p));
        break;

      case F1AP_UE_CONTEXT_MODIFICATION_RESP:
        rrc_CU_process_ue_context_modification_response(msg_p, instance);
        free_ue_context_mod_resp(&F1AP_UE_CONTEXT_MODIFICATION_RESP(msg_p));
        break;

      case F1AP_UE_CONTEXT_MODIFICATION_REQUIRED:
        rrc_CU_process_ue_modification_required(msg_p, instance, msg_p->ittiMsgHeader.originInstance);
        break;

      case F1AP_UE_CONTEXT_RELEASE_REQ:
        rrc_CU_process_ue_context_release_request(msg_p, msg_p->ittiMsgHeader.originInstance);
        free_ue_context_rel_req(&F1AP_UE_CONTEXT_RELEASE_REQ(msg_p));
        break;

      case F1AP_UE_CONTEXT_RELEASE_COMPLETE:
        rrc_CU_process_ue_context_release_complete(msg_p);
        free_ue_context_rel_cplt(&F1AP_UE_CONTEXT_RELEASE_COMPLETE(msg_p));
        break;

      case F1AP_LOST_CONNECTION:
        rrc_CU_process_f1_lost_connection(RC.nrrrc[0], &F1AP_LOST_CONNECTION(msg_p), msg_p->ittiMsgHeader.originInstance);
        break;

      case F1AP_GNB_DU_CONFIGURATION_UPDATE:
        AssertFatal(!NODE_IS_DU(RC.nrrrc[instance]->node_type), "should not receive F1AP_SETUP_REQUEST in DU!\n");
        rrc_gNB_process_f1_du_configuration_update(&F1AP_GNB_DU_CONFIGURATION_UPDATE(msg_p), msg_p->ittiMsgHeader.originInstance);
        free_f1ap_du_configuration_update(&F1AP_GNB_DU_CONFIGURATION_UPDATE(msg_p));
        break;

      case F1AP_GNB_CU_CONFIGURATION_UPDATE_ACKNOWLEDGE:
        AssertFatal(!NODE_IS_DU(RC.nrrrc[instance]->node_type),
                    "should not receive F1AP_GNB_CU_CONFIGURATION_UPDATE_ACKNOWLEDGE in DU!\n");
        LOG_E(NR_RRC, "Handling of F1AP_GNB_CU_CONFIGURATION_UPDATE_ACKNOWLEDGE not implemented\n");
        break;

      case F1AP_RESET_ACK:
        LOG_I(NR_RRC, "received F1AP reset acknowledgement\n");
        free_f1ap_reset_ack(&F1AP_RESET_ACK(msg_p));
        break;

      /* Messages from X2AP */
      case X2AP_ENDC_SGNB_ADDITION_REQ:
        LOG_I(NR_RRC, "Received ENDC sgNB addition request from X2AP \n");
        rrc_gNB_process_AdditionRequestInformation(instance, &X2AP_ENDC_SGNB_ADDITION_REQ(msg_p));
        break;

      case X2AP_ENDC_SGNB_RECONF_COMPLETE:
        LOG_A(NR_RRC, "Handling of reconfiguration complete message at RRC gNB is pending \n");
        break;

      case NGAP_INITIAL_CONTEXT_SETUP_REQ:
        rrc_gNB_process_NGAP_INITIAL_CONTEXT_SETUP_REQ(msg_p, instance);
        break;

      case X2AP_ENDC_SGNB_RELEASE_REQUEST:
        LOG_I(NR_RRC, "Received ENDC sgNB release request from X2AP \n");
        rrc_gNB_process_release_request(instance, &X2AP_ENDC_SGNB_RELEASE_REQUEST(msg_p));
        break;

      case X2AP_ENDC_DC_OVERALL_TIMEOUT:
        rrc_gNB_process_dc_overall_timeout(instance, &X2AP_ENDC_DC_OVERALL_TIMEOUT(msg_p));
        break;

      case NGAP_UE_CONTEXT_RELEASE_COMMAND:
        rrc_gNB_process_NGAP_UE_CONTEXT_RELEASE_COMMAND(msg_p, instance);
        break;

      case E1AP_SETUP_REQ:
        rrc_gNB_process_e1_setup_req(msg_p->ittiMsgHeader.originInstance, &E1AP_SETUP_REQ(msg_p));
        free_e1ap_cuup_setup_request(&E1AP_SETUP_REQ(msg_p));
        break;

      case E1AP_BEARER_CONTEXT_SETUP_RESP:
        rrc_gNB_process_e1_bearer_context_setup_resp(&E1AP_BEARER_CONTEXT_SETUP_RESP(msg_p), instance);
        free_e1ap_context_setup_response(&E1AP_BEARER_CONTEXT_SETUP_RESP(msg_p));
        break;

      case E1AP_BEARER_CONTEXT_SETUP_FAILURE:
        rrc_gNB_process_e1_bearer_context_setup_failure(&E1AP_BEARER_CONTEXT_SETUP_FAILURE(msg_p));
        free_e1_bearer_context_setup_failure(&E1AP_BEARER_CONTEXT_SETUP_FAILURE(msg_p));
        break;

      case E1AP_BEARER_CONTEXT_MODIFICATION_RESP:
        rrc_gNB_process_e1_bearer_context_modif_resp(&E1AP_BEARER_CONTEXT_MODIFICATION_RESP(msg_p));
        free_e1ap_context_mod_response(&E1AP_BEARER_CONTEXT_MODIFICATION_RESP(msg_p));
        break;

      case E1AP_BEARER_CONTEXT_MODIFICATION_FAIL:
        rrc_gNB_process_e1_bearer_context_modif_fail(&E1AP_BEARER_CONTEXT_MODIFICATION_FAIL(msg_p));
        break;

      case E1AP_BEARER_CONTEXT_RELEASE_CPLT:
        rrc_gNB_process_e1_bearer_context_release_cplt(&E1AP_BEARER_CONTEXT_RELEASE_CPLT(msg_p));
        break;

      case E1AP_LOST_CONNECTION: /* CUCP */
        rrc_gNB_process_e1_lost_connection(RC.nrrrc[0], &E1AP_LOST_CONNECTION(msg_p), msg_p->ittiMsgHeader.originInstance);
        break;

      case NGAP_PAGING_IND:
        rrc_gNB_process_PAGING_IND(msg_p, instance);
        break;

      case NGAP_HANDOVER_REQUEST:
        rrc_gNB_process_Handover_Request(RC.nrrrc[instance], instance, &NGAP_HANDOVER_REQUEST(msg_p));
        rrc_gNB_free_Handover_Request(&NGAP_HANDOVER_REQUEST(msg_p)); // Free transfered NG message
        break;

      case NGAP_HANDOVER_COMMAND:
        rrc_gNB_process_HandoverCommand(RC.nrrrc[instance], &NGAP_HANDOVER_COMMAND(msg_p));
        rrc_gNB_free_Handover_Command(&NGAP_HANDOVER_COMMAND(msg_p)); // Free transfered NG message
        break;

      default:
        LOG_E(NR_RRC, "[gNB %ld] Received unexpected message %s\n", instance, msg_name_p);
        break;
    }

    result = itti_free(ITTI_MSG_ORIGIN_ID(msg_p), msg_p);
    AssertFatal(result == EXIT_SUCCESS, "Failed to free memory (%d)!\n", result);
    msg_p = NULL;
  }
}

//-----------------------------------------------------------------------------
void rrc_gNB_generate_SecurityModeCommand(gNB_RRC_INST *rrc, gNB_RRC_UE_t *ue_p)
//-----------------------------------------------------------------------------
{
  uint8_t buffer[100];
  AssertFatal(!ue_p->as_security_active, "logic error: security already active\n");

  T(T_ENB_RRC_SECURITY_MODE_COMMAND, T_INT(0), T_INT(0), T_INT(0), T_INT(ue_p->rrc_ue_id));
  NR_IntegrityProtAlgorithm_t integrity_algorithm = (NR_IntegrityProtAlgorithm_t)ue_p->integrity_algorithm;
  int size = do_NR_SecurityModeCommand(buffer,
                                       rrc_gNB_get_next_transaction_identifier(rrc->module_id),
                                       ue_p->ciphering_algorithm,
                                       integrity_algorithm);
  LOG_DUMPMSG(NR_RRC, DEBUG_RRC, (char *)buffer, size, "[MSG] RRC Security Mode Command\n");
  LOG_I(NR_RRC, "UE %u Logical Channel DL-DCCH, Generate SecurityModeCommand (bytes %d)\n", ue_p->rrc_ue_id, size);

  const uint32_t msg_id = NR_DL_DCCH_MessageType__c1_PR_securityModeCommand;
  nr_rrc_transfer_protected_rrc_message(rrc, ue_p, DL_SCH_LCID_DCCH, msg_id, buffer, size);
}

//-----------------------------------------------------------------------------
/*
* Generate the RRC Connection Release to UE.
* If received, UE should switch to RRC_IDLE mode.
*/
void rrc_gNB_generate_RRCRelease(gNB_RRC_INST *rrc, gNB_RRC_UE_t *UE)
{
  uint8_t buffer[NR_RRC_BUF_SIZE] = {0};
  int size = do_NR_RRCRelease(buffer, NR_RRC_BUF_SIZE, rrc_gNB_get_next_transaction_identifier(rrc->module_id));

  LOG_UE_DL_EVENT(UE, "Send RRC Release\n");
  f1_ue_data_t ue_data = cu_get_f1_ue_data(UE->rrc_ue_id);
  RETURN_IF_INVALID_ASSOC_ID(ue_data.du_assoc_id);
  int srbid = 1;
  f1ap_ue_context_rel_cmd_t ue_context_release_cmd = {
    .gNB_CU_ue_id = UE->rrc_ue_id,
    .gNB_DU_ue_id = ue_data.secondary_ue,
    .cause = F1AP_CAUSE_RADIO_NETWORK,
    .cause_value = 10, // 10 = F1AP_CauseRadioNetwork_normal_release
    .srb_id = &srbid, // C-ifRRCContainer => is added below
  };
  deliver_ue_ctxt_release_data_t data = {.rrc = rrc, .release_cmd = &ue_context_release_cmd, .assoc_id = ue_data.du_assoc_id};
  nr_pdcp_data_req_srb(UE->rrc_ue_id, DL_SCH_LCID_DCCH, rrc_gNB_mui++, size, buffer, rrc_deliver_ue_ctxt_release_cmd, &data);

#ifdef E2_AGENT
  E2_AGENT_SIGNAL_DL_DCCH_RRC_MSG(buffer, size, NR_DL_DCCH_MessageType__c1_PR_rrcRelease);
#endif
}

int rrc_gNB_generate_pcch_msg(sctp_assoc_t assoc_id, const NR_SIB1_t *sib1, uint32_t tmsi, uint8_t paging_drx)
{
  instance_t instance = 0;
  const unsigned int Ttab[4] = {32,64,128,256};
  uint8_t Tc;
  uint8_t Tue;
  uint32_t pfoffset;
  uint32_t N;  /* N: min(T,nB). total count of PF in one DRX cycle */
  uint32_t Ns = 0;  /* Ns: max(1,nB/T) */
  uint32_t T;  /* DRX cycle */
  uint8_t buffer[NR_RRC_BUF_SIZE];

  /* get default DRX cycle from configuration */
  Tc = sib1->servingCellConfigCommon->downlinkConfigCommon.pcch_Config.defaultPagingCycle;

  Tue = paging_drx;
  /* set T = min(Tc,Tue) */
  T = Tc < Tue ? Ttab[Tc] : Ttab[Tue];
  /* set N = PCCH-Config->nAndPagingFrameOffset */
  switch (sib1->servingCellConfigCommon->downlinkConfigCommon.pcch_Config.nAndPagingFrameOffset.present) {
    case NR_PCCH_Config__nAndPagingFrameOffset_PR_oneT:
      N = T;
      pfoffset = 0;
      break;
    case NR_PCCH_Config__nAndPagingFrameOffset_PR_halfT:
      N = T/2;
      pfoffset = 1;
      break;
    case NR_PCCH_Config__nAndPagingFrameOffset_PR_quarterT:
      N = T/4;
      pfoffset = 3;
      break;
    case NR_PCCH_Config__nAndPagingFrameOffset_PR_oneEighthT:
      N = T/8;
      pfoffset = 7;
      break;
    case NR_PCCH_Config__nAndPagingFrameOffset_PR_oneSixteenthT:
      N = T/16;
      pfoffset = 15;
      break;
    default:
      LOG_E(RRC, "[gNB %ld] In rrc_gNB_generate_pcch_msg:  pfoffset error (pfoffset %d)\n",
            instance, sib1->servingCellConfigCommon->downlinkConfigCommon.pcch_Config.nAndPagingFrameOffset.present);
      return (-1);

  }

  switch (sib1->servingCellConfigCommon->downlinkConfigCommon.pcch_Config.ns) {
    case NR_PCCH_Config__ns_four:
      if(*sib1->servingCellConfigCommon->downlinkConfigCommon.initialDownlinkBWP.pdcch_ConfigCommon->choice.setup->pagingSearchSpace == 0){
        LOG_E(RRC, "[gNB %ld] In rrc_gNB_generate_pcch_msg:  ns error only 1 or 2 is allowed when pagingSearchSpace is 0\n",
              instance);
        return (-1);
      } else {
        Ns = 4;
      }
      break;
    case NR_PCCH_Config__ns_two:
      Ns = 2;
      break;
    case NR_PCCH_Config__ns_one:
      Ns = 1;
      break;
    default:
      LOG_E(RRC, "[gNB %ld] In rrc_gNB_generate_pcch_msg: ns error (ns %ld)\n",
            instance, sib1->servingCellConfigCommon->downlinkConfigCommon.pcch_Config.ns);
      return (-1);
  }

  (void) N; /* not used, suppress warning */
  (void) Ns; /* not used, suppress warning */
  (void) pfoffset; /* not used, suppress warning */

  /* Create message for PDCP (DLInformationTransfer_t) */
  int length = do_NR_Paging(instance, buffer, tmsi);

  if (length == -1) {
    LOG_I(NR_RRC, "do_Paging error\n");
    return -1;
  }
  // TODO, send message to pdcp
  (void) assoc_id;

  return 0;
}

/* F1AP UE Context Management Procedures */

//-----------------------------------------------------------------------------
void rrc_gNB_generate_UeContextSetupRequest(const gNB_RRC_INST *rrc,
                                            rrc_gNB_ue_context_t *const ue_context_pP,
                                            const e1ap_bearer_setup_resp_t *resp)
//-----------------------------------------------------------------------------
{
  gNB_RRC_UE_t *ue_p = &ue_context_pP->ue_context;
  AssertFatal(!ue_p->f1_ue_context_active, "logic error: ue context already active\n");

  AssertFatal(!NODE_IS_DU(rrc->node_type), "illegal node type DU!\n");

  f1ap_cu_to_du_rrc_info_t cu2du = {0};
  if (ue_p->ue_cap_buffer.len > 0) {
    cu2du.ue_cap = malloc_or_fail(sizeof(byte_array_t));
    *cu2du.ue_cap = copy_byte_array(ue_p->ue_cap_buffer);
  }

  nr_rrc_du_container_t *du = get_du_for_ue((gNB_RRC_INST *)rrc, ue_p->rrc_ue_id);
  cu2du.meas_timing_config = get_meas_timing_config(du->mtc, ue_p->measConfig);

  int nb_srb = 1;
  f1ap_srb_to_setup_t *srbs = calloc_or_fail(nb_srb, sizeof(*srbs));
  srbs[0].id = 2;
  activate_srb(ue_p, srbs[0].id);

  /* Instruction towards the DU for DRB configuration and tunnel creation */
  f1ap_drb_to_setup_t *drbs = calloc_or_fail(32, sizeof(*drbs)); // maximum DRB can be 32
  int n_drbs = fill_drb_to_be_setup_from_e1_resp(rrc, ue_p, resp->pduSession, resp->numPDUSessions, drbs);

  /* gNB-DU UE Aggregate Maximum Bit Rate Uplink is C- ifDRBSetup */
  uint64_t *agg_mbr = malloc_or_fail(sizeof(*agg_mbr));
  *agg_mbr = 1000000000 /* bps */;

  f1_ue_data_t ue_data = cu_get_f1_ue_data(ue_p->rrc_ue_id);
  RETURN_IF_INVALID_ASSOC_ID(ue_data.du_assoc_id);
  uint32_t *du_ue_id = malloc_or_fail(sizeof(*du_ue_id));
  *du_ue_id = ue_data.secondary_ue;
  f1ap_ue_context_setup_req_t ue_context_setup_req = {
      .gNB_CU_ue_id = ue_p->rrc_ue_id,
      .gNB_DU_ue_id = du_ue_id,
      .plmn = ue_p->serving_plmn,
      .nr_cellid = rrc->nr_cellid,
      .servCellIndex = 0, /* TODO: correct value? */
      .srbs_len = nb_srb,
      .srbs = srbs,
      .drbs_len = n_drbs,
      .drbs = drbs,
      .cu_to_du_rrc_info = cu2du,
      .gnb_du_ue_agg_mbr_ul = agg_mbr,
  };

  rrc->mac_rrc.ue_context_setup_request(ue_data.du_assoc_id, &ue_context_setup_req);
  free_ue_context_setup_req(&ue_context_setup_req);
  LOG_I(RRC, "UE %d trigger UE context setup request with %d DRBs\n", ue_p->rrc_ue_id, n_drbs);
}

//-----------------------------------------------------------------------------
void rrc_gNB_generate_UeContextModificationRequest(const gNB_RRC_INST *rrc,
                                                   rrc_gNB_ue_context_t *const ue_context_pP,
                                                   const e1ap_bearer_setup_resp_t *resp,
                                                   int n_rel_drbs,
                                                   const f1ap_drb_to_release_t *rel_drbs)
//-----------------------------------------------------------------------------
{
  gNB_RRC_UE_t *ue_p = &ue_context_pP->ue_context;
  AssertFatal(ue_p->f1_ue_context_active, "logic error: calling ue context modification when context not established\n");
  AssertFatal(ue_p->Srb[1].Active && ue_p->Srb[2].Active, "SRBs should already be active\n");

  AssertFatal(!NODE_IS_DU(rrc->node_type), "illegal node type DU!\n");

  f1ap_cu_to_du_rrc_info_t *cu2du = NULL;
  if (ue_p->ue_cap_buffer.len > 0) {
    cu2du = calloc_or_fail(1, sizeof(*cu2du));
    cu2du->ue_cap = calloc_or_fail(1, sizeof(*cu2du->ue_cap));
    *cu2du->ue_cap = copy_byte_array(ue_p->ue_cap_buffer);
  }

  /* Instruction towards the DU for DRB configuration and tunnel creation */
  f1ap_drb_to_setup_t *drbs = calloc_or_fail(32, sizeof(*drbs)); // maximum DRB can be 32
  int n_drbs = fill_drb_to_be_setup_from_e1_resp(rrc, ue_p, resp->pduSession, resp->numPDUSessions, drbs);

  f1_ue_data_t ue_data = cu_get_f1_ue_data(ue_p->rrc_ue_id);
  RETURN_IF_INVALID_ASSOC_ID(ue_data.du_assoc_id);
  f1ap_ue_context_mod_req_t ue_context_modif_req = {
      .gNB_CU_ue_id = ue_p->rrc_ue_id,
      .gNB_DU_ue_id = ue_data.secondary_ue,
      .drbs_len = n_drbs,
      .drbs = drbs,
      .drbs_rel_len = n_rel_drbs,
      .drbs_rel = (f1ap_drb_to_release_t *)rel_drbs,
      .cu_to_du_rrc_info = cu2du,
  };
  rrc->mac_rrc.ue_context_modification_request(ue_data.du_assoc_id, &ue_context_modif_req);
  // avoid attempt to release rel_drbs
  ue_context_modif_req.drbs_rel_len = 0;
  ue_context_modif_req.drbs_rel = NULL;
  free_ue_context_mod_req(&ue_context_modif_req);
  LOG_I(RRC, "UE %d trigger UE context modification request with %d DRBs\n", ue_p->rrc_ue_id, n_drbs);
}
