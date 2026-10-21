/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

/*!
 * \brief rrc GTPV1U procedures for eNB
 */

# include "rrc_defs.h"
# include "rrc_extern.h"
# include "RRC/LTE/MESSAGES/asn1_msg.h"
# include "rrc_eNB_GTPV1U.h"
# include "rrc_eNB_UE_context.h"


#include "oai_asn1.h"
#include "intertask_interface.h"


# include "common/ran_context.h"
#include <openair3/ocp-gtpu/gtp_itf.h>

extern RAN_CONTEXT_t RC;

int
rrc_eNB_process_GTPV1U_CREATE_TUNNEL_RESP(
  const protocol_ctxt_t *const ctxt_pP,
  const gtpv1u_enb_create_tunnel_resp_t *const create_tunnel_resp_pP,
  uint8_t                         *inde_list
) {
  rnti_t                         rnti;
  int                            i;
  struct rrc_eNB_ue_context_s   *ue_context_p = NULL;

  if (create_tunnel_resp_pP) {
    LOG_D(RRC, PROTOCOL_RRC_CTXT_UE_FMT" RX CREATE_TUNNEL_RESP num tunnels %u \n",
          PROTOCOL_RRC_CTXT_UE_ARGS(ctxt_pP),
          create_tunnel_resp_pP->num_tunnels);
    rnti = create_tunnel_resp_pP->rnti;
    ue_context_p = rrc_eNB_get_ue_context(RC.rrc[ctxt_pP->module_id], ctxt_pP->rntiMaybeUEid);

    for (i = 0; i < create_tunnel_resp_pP->num_tunnels; i++) {
      ue_context_p->ue_context.enb_gtp_teid[inde_list[i]]  = create_tunnel_resp_pP->enb_S1u_teid[i];
      ue_context_p->ue_context.enb_gtp_addrs[inde_list[i]] = create_tunnel_resp_pP->enb_addr;
      ue_context_p->ue_context.enb_gtp_ebi[inde_list[i]]   = create_tunnel_resp_pP->eps_bearer_id[i];
      LOG_I(RRC, PROTOCOL_RRC_CTXT_UE_FMT" rrc_eNB_process_GTPV1U_CREATE_TUNNEL_RESP tunnel (%u, %u) bearer UE context index %u, msg index %u, id %u, gtp addr len %d \n",
            PROTOCOL_RRC_CTXT_UE_ARGS(ctxt_pP),
            create_tunnel_resp_pP->enb_S1u_teid[i],
            ue_context_p->ue_context.enb_gtp_teid[inde_list[i]],            
            inde_list[i],
	    i,
            create_tunnel_resp_pP->eps_bearer_id[i],
	    create_tunnel_resp_pP->enb_addr.length);
    }

        (void)rnti; /* avoid gcc warning "set but not used" */
    return 0;
  } else {
    return -1;
  }
}

void rrc_eNB_send_GTPV1U_ENB_DELETE_TUNNEL_REQ(
  module_id_t enb_mod_idP,
  rrc_eNB_ue_context_t*  ue_context_pP
)
{
  if (!ue_context_pP) {
    LOG_W(RRC, "[eNB] In %s: invalid UE\n", __func__);
    return;
  }
  eNB_RRC_UE_t *ue = &ue_context_pP->ue_context;
  gtpv1u_enb_delete_tunnel_req_t tmp={0};

  tmp.rnti = ue->rnti;
  tmp.from_gnb = 0;
  tmp.num_erab = ue->nb_of_e_rabs;
  for (int e_rab = 0; e_rab < ue->nb_of_e_rabs; e_rab++) {
    const rb_id_t gtp_ebi = ue->enb_gtp_ebi[e_rab];
    tmp.eps_bearer_id[e_rab] = gtp_ebi;
  }
  gtpv1u_delete_s1u_tunnel(enb_mod_idP,&tmp);
  if ( ue->ue_release_timer_rrc > 0
       && (ue->handover_info == NULL ||
	   (ue->handover_info->state != HO_RELEASE &&
	    ue->handover_info->state != HO_CANCEL
	   )
       )
    )
    ue->ue_release_timer_rrc = ue->ue_release_timer_thres_rrc;
  
}


