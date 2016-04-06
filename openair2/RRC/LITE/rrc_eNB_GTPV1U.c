/*******************************************************************************
    OpenAirInterface
    Copyright(c) 1999 - 2014 Eurecom

    OpenAirInterface is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.


    OpenAirInterface is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with OpenAirInterface.The full GNU General Public License is
    included in this distribution in the file called "COPYING". If not,
    see <http://www.gnu.org/licenses/>.

  Contact Information
  OpenAirInterface Admin: openair_admin@eurecom.fr
  OpenAirInterface Tech : openair_tech@eurecom.fr
  OpenAirInterface Dev  : openair4g-devel@lists.eurecom.fr

  Address      : Eurecom, Campus SophiaTech, 450 Route des Chappes, CS 50193 - 06904 Biot Sophia Antipolis cedex, FRANCE

*******************************************************************************/
/*! \file rrc_eNB_GTPV1U.c
 * \brief rrc GTPV1U procedures for eNB
 * \author Lionel GAUTHIER
 * \version 1.0
 * \company Eurecom
 * \email: lionel.gauthier@eurecom.fr
 */

#if defined(ENABLE_USE_MME)
# include "defs.h"
# include "extern.h"
# include "RRC/LITE/MESSAGES/asn1_msg.h"
# include "rrc_eNB_GTPV1U.h"
# include "rrc_eNB_UE_context.h"
# include "msc.h"

# if defined(ENABLE_ITTI)
#   include "asn1_conversions.h"
#   include "intertask_interface.h"
# endif

int
rrc_eNB_process_GTPV1U_CREATE_TUNNEL_RESP(
  const protocol_ctxt_t* const ctxt_pP,
  const gtpv1u_enb_create_tunnel_resp_t * const create_tunnel_resp_pP
)
{
  rnti_t                         rnti;
  int                            i;
  struct rrc_eNB_ue_context_s*   ue_context_p = NULL;

  if (create_tunnel_resp_pP) {
    LOG_D(RRC, PROTOCOL_RRC_CTXT_UE_FMT" RX CREATE_TUNNEL_RESP num tunnels %u \n",
          PROTOCOL_RRC_CTXT_UE_ARGS(ctxt_pP),
          create_tunnel_resp_pP->num_tunnels);

    rnti = create_tunnel_resp_pP->rnti;
    ue_context_p = rrc_eNB_get_ue_context(
                     &eNB_rrc_inst[ctxt_pP->module_id],
                     ctxt_pP->rnti);

    for (i = 0; i < create_tunnel_resp_pP->num_tunnels; i++) {
      
      ue_context_p->ue_context.enb_gtp_teid[i+ue_context_p->ue_context.setup_e_rabs]  = create_tunnel_resp_pP->enb_S1u_teid[i];
      ue_context_p->ue_context.enb_gtp_addrs[i+ue_context_p->ue_context.setup_e_rabs] = create_tunnel_resp_pP->enb_addr;
      ue_context_p->ue_context.enb_gtp_ebi[i+ue_context_p->ue_context.setup_e_rabs]   = create_tunnel_resp_pP->eps_bearer_id[i];
      
      LOG_I(RRC, PROTOCOL_RRC_CTXT_UE_FMT" rrc_eNB_process_GTPV1U_CREATE_TUNNEL_RESP tunnel (%u, %u) bearer UE context index %u, msg index %u, id %u, gtp addr len %d \n",
            PROTOCOL_RRC_CTXT_UE_ARGS(ctxt_pP),
            create_tunnel_resp_pP->enb_S1u_teid[i],
	    ue_context_p->ue_context.enb_gtp_teid[i+ue_context_p->ue_context.setup_e_rabs],
            i+ue_context_p->ue_context.setup_e_rabs, 
	    i,
            create_tunnel_resp_pP->eps_bearer_id[i],
	    create_tunnel_resp_pP->enb_addr.length);
    }
	MSC_LOG_RX_MESSAGE(
			  MSC_RRC_ENB,
			  MSC_GTPU_ENB,
			  NULL,0,
			  MSC_AS_TIME_FMT" CREATE_TUNNEL_RESP RNTI %"PRIx16" ntuns %u ebid %u enb-s1u teid %u",
			  0,0,rnti,
			  create_tunnel_resp_pP->num_tunnels,
			  ue_context_p->ue_context.enb_gtp_ebi[0],
			  ue_context_p->ue_context.enb_gtp_teid[0]);
        (void)rnti; /* avoid gcc warning "set but not used" */
    return 0;
  } else {
    return -1;
  }
}
#endif
