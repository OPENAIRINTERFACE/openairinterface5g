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

/*! \file l2_nr_interface.c
 * \brief layer 2 interface, used to support different RRC sublayer
 * \author Raymond Knopp and Navid Nikaein, WEI-TAI CHEN
 * \date 2010-2014, 2018
 * \version 1.0
 * \company Eurecom, NTUST
 * \email: raymond.knopp@eurecom.fr, kroempa@gmail.com
 */

#include "platform_types.h"
#include "nr_rrc_defs.h"
#include "nr_rrc_extern.h"
#include "common/utils/LOG/log.h"
#include "common/ran_context.h"
#include "LAYER2/NR_MAC_COMMON/nr_mac_common.h"
#include "LAYER2/NR_MAC_COMMON/nr_mac_extern.h"
#include "LAYER2/NR_MAC_gNB/nr_mac_gNB.h"
#include "LAYER2/NR_MAC_gNB/mac_proto.h"
#include "LAYER2/nr_rlc/nr_rlc_oai_api.h"

#include "intertask_interface.h"

#include "uper_encoder.h"

#include "NR_MIB.h"
#include "NR_BCCH-BCH-Message.h"
#include "rrc_gNB_UE_context.h"
#include <openair2/RRC/NR/MESSAGES/asn1_msg.h>
#include "nr_pdcp/nr_pdcp_oai_api.h"


extern RAN_CONTEXT_t RC;

void nr_rrc_mac_remove_ue(rnti_t rntiMaybeUEid)
{
  nr_rlc_remove_ue(rntiMaybeUEid);

  gNB_MAC_INST *nrmac = RC.nrmac[0];
  NR_SCHED_LOCK(&nrmac->sched_lock);
  mac_remove_nr_ue(nrmac, rntiMaybeUEid);
  NR_SCHED_UNLOCK(&nrmac->sched_lock);
}

void nr_rrc_mac_update_cellgroup(rnti_t rntiMaybeUEid, NR_CellGroupConfig_t *cgc)
{
  gNB_MAC_INST *nrmac = RC.nrmac[0];
  NR_SCHED_LOCK(&nrmac->sched_lock);
  nr_mac_update_cellgroup(nrmac, rntiMaybeUEid, cgc);
  NR_SCHED_UNLOCK(&nrmac->sched_lock);
}

uint16_t mac_rrc_nr_data_req(const module_id_t Mod_idP,
                             const int         CC_id,
                             const frame_t     frameP,
                             const rb_id_t     Srb_id,
                             const rnti_t      rnti,
                             const uint8_t     Nb_tb,
                             uint8_t *const    buffer_pP)
{

  LOG_D(RRC,"[eNB %d] mac_rrc_data_req to SRB ID=%ld\n",Mod_idP,Srb_id);

  // MIBCH
  if ((Srb_id & RAB_OFFSET) == MIBCH) {

    int encode_size = 3;
    rrc_gNB_carrier_data_t *carrier = &RC.nrrrc[Mod_idP]->carrier;
    int encoded = encode_MIB_NR(carrier->mib, frameP, buffer_pP, encode_size);
    DevAssert(encoded == encode_size);
    LOG_D(NR_RRC, "MIB PDU buffer_pP[0]=%x , buffer_pP[1]=%x, buffer_pP[2]=%x\n", buffer_pP[0], buffer_pP[1],
          buffer_pP[2]);
    return encode_size;
  }

  if ((Srb_id & RAB_OFFSET) == BCCH) {
    memcpy(&buffer_pP[0], RC.nrrrc[Mod_idP]->carrier.SIB1, RC.nrrrc[Mod_idP]->carrier.sizeof_SIB1);
    return RC.nrrrc[Mod_idP]->carrier.sizeof_SIB1;
  }

  // CCCH
  if ((Srb_id & RAB_OFFSET) == CCCH) {
    AssertFatal(0, "CCCH is managed by rlc of srb 0, not anymore by mac_rrc_nr_data_req\n");
  }

  return 0;
}

int8_t nr_mac_rrc_bwp_switch_req(const module_id_t     module_idP,
                                 const frame_t         frameP,
                                 const sub_frame_t     sub_frameP,
                                 const rnti_t          rntiP,
                                 const int             dl_bwp_id,
                                 const int             ul_bwp_id) {
  rrc_gNB_ue_context_t *ue_context_p = rrc_gNB_get_ue_context_by_rnti(RC.nrrrc[module_idP], rntiP);

  protocol_ctxt_t ctxt;
  PROTOCOL_CTXT_SET_BY_MODULE_ID(&ctxt, module_idP, GNB_FLAG_YES, rntiP, frameP, sub_frameP, 0);
  nr_rrc_reconfiguration_req(ue_context_p, &ctxt, dl_bwp_id, ul_bwp_id);

  return 0;
}
