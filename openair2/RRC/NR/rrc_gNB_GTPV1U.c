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

/*! \file rrc_gNB_GTPV1U.c
 * \brief rrc GTPV1U procedures for gNB
 * \author Lionel GAUTHIER, Panos MATZAKOS
 * \version 1.0
 * \company Eurecom
 * \email: lionel.gauthier@eurecom.fr, panagiotis.matzakos@eurecom.fr
 */

# include "rrc_defs.h"
# include "rrc_extern.h"
# include "RRC/LTE/MESSAGES/asn1_msg.h"
# include "rrc_eNB_GTPV1U.h"
# include "rrc_eNB_UE_context.h"
# include "openair2/RRC/NR/rrc_gNB_UE_context.h"

//# if defined(ENABLE_ITTI)
#   include "oai_asn1.h"
#   include "intertask_interface.h"
//#endif

# include "common/ran_context.h"
#include "openair2/RRC/NR/rrc_gNB_GTPV1U.h"

extern RAN_CONTEXT_t RC;

int rrc_gNB_process_GTPV1U_CREATE_TUNNEL_RESP(const protocol_ctxt_t *const ctxt_pP, const gtpv1u_enb_create_tunnel_resp_t *const create_tunnel_resp_pP, uint8_t *inde_list)
{
  if (!create_tunnel_resp_pP) {
    LOG_E(NR_RRC, "create_tunnel_resp_pP error\n");
    return -1;
  }

  LOG_D(RRC, PROTOCOL_RRC_CTXT_UE_FMT " RX CREATE_TUNNEL_RESP num tunnels %u \n", PROTOCOL_RRC_CTXT_UE_ARGS(ctxt_pP), create_tunnel_resp_pP->num_tunnels);
  /* we look up by CU UE ID! Do NOT change back to RNTI! */
  rrc_gNB_ue_context_t *ue_context_p = rrc_gNB_get_ue_context(RC.nrrrc[ctxt_pP->module_id], ctxt_pP->rntiMaybeUEid);
  if (!ue_context_p) {
    LOG_E(NR_RRC, "UE table error\n");
    return -1;
  }
  for (int i = 0; i < create_tunnel_resp_pP->num_tunnels; i++) {
    ue_context_p->ue_context.nsa_gtp_teid[inde_list[i]] = create_tunnel_resp_pP->enb_S1u_teid[i];
    ue_context_p->ue_context.nsa_gtp_addrs[inde_list[i]] = create_tunnel_resp_pP->enb_addr;
    ue_context_p->ue_context.nsa_gtp_ebi[inde_list[i]] = create_tunnel_resp_pP->eps_bearer_id[i];
    LOG_I(RRC,
          PROTOCOL_RRC_CTXT_UE_FMT " rrc_eNB_process_GTPV1U_CREATE_TUNNEL_RESP tunnel (%u, %u) bearer UE context index %u, msg index %u, id %u, gtp addr len %d \n",
          PROTOCOL_RRC_CTXT_UE_ARGS(ctxt_pP),
          create_tunnel_resp_pP->enb_S1u_teid[i],
          ue_context_p->ue_context.nsa_gtp_teid[inde_list[i]],
          inde_list[i],
          i,
          create_tunnel_resp_pP->eps_bearer_id[i],
          create_tunnel_resp_pP->enb_addr.length);
  }

  return 0;
}
