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

#include "ran_func_pdcp.h"

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

#include "openair2/E2AP/flexric/test/rnd/fill_rnd_data_pdcp.h"
#include "common/ran_context.h"
#include "openair2/RRC/NR/rrc_gNB_UE_context.h"
#include "openair2/LAYER2/nr_pdcp/nr_pdcp_oai_api.h"
#include "openair2/LAYER2/nr_rlc/nr_rlc_oai_api.h"
#include "openair2/LAYER2/NR_MAC_gNB/mac_proto.h"
#include "openair2/E2AP/flexric/src/util/time_now_us.h"


static
const int mod_id = 0;

static
    uint32_t num_act_rb(NR_UEs_t* UE_info)
{
  assert(UE_info!= NULL);

  uint32_t act_rb = 0;
  UE_iterator(UE_info->list, UE) {
    uint16_t const rnti = UE->rnti;
    for(int rb_id = 1; rb_id < 6; ++rb_id ){
      nr_rlc_statistics_t rlc = {0};
      const int srb_flag = 0;
      const bool rc = nr_rlc_get_statistics(rnti, srb_flag, rb_id, &rlc);
      if(rc) ++act_rb;
    }
  }
  return act_rb;
}

void read_pdcp_sm(void* data)
{
  assert(data != NULL);
  //assert(data->type == PDCP_STATS_V0);

  pdcp_ind_data_t* pdcp = (pdcp_ind_data_t*)data;
  //fill_pdcp_ind_data(pdcp);
  // TODO: Need to improve, not good
  if (NODE_IS_CU(RC.nrrrc[mod_id]->node_type)) {

    uint32_t act_rb = 0;
    struct rrc_gNB_ue_context_s *ue_context_p1 = NULL;
    RB_FOREACH(ue_context_p1, rrc_nr_ue_tree_s, &RC.nrrrc[mod_id]->rrc_ue_head) {
      uint16_t const rnti = ue_context_p1->ue_context.rnti;
      for(int rb_id = 1; rb_id < 6; ++rb_id ){
        nr_pdcp_statistics_t rb_pdcp = {0};
        const int srb_flag = 0;
        const bool rc = nr_pdcp_get_statistics(rnti, srb_flag, rb_id, &rb_pdcp);
        if(rc) ++act_rb;
      }
    }

    pdcp->msg.tstamp = time_now_us();

    pdcp->msg.len = act_rb;
    if (pdcp->msg.len > 0) {
      pdcp->msg.rb = calloc(pdcp->msg.len, sizeof(pdcp_radio_bearer_stats_t));
      assert(pdcp->msg.rb != NULL && "Memory exhausted!");
    }

    size_t i = 0;
    struct rrc_gNB_ue_context_s *ue_context_p2 = NULL;
    RB_FOREACH(ue_context_p2, rrc_nr_ue_tree_s, &RC.nrrrc[mod_id]->rrc_ue_head) {
      // TODO: Need to handel multiple UEs
      uint16_t const rnti = ue_context_p2->ue_context.rnti;

      for (size_t rb_id = 1; rb_id < 6; ++rb_id) {
        nr_pdcp_statistics_t rb_pdcp = {0};

        const int srb_flag = 0;
        const bool rc = nr_pdcp_get_statistics(rnti, srb_flag, rb_id, &rb_pdcp);

        if (!rc) continue;

        pdcp_radio_bearer_stats_t *rd = &pdcp->msg.rb[i];

        rd->txpdu_pkts = rb_pdcp.txpdu_pkts;     /* aggregated number of tx packets */
        rd->txpdu_bytes = rb_pdcp.txpdu_bytes;    /* aggregated bytes of tx packets */
        rd->txpdu_sn = rb_pdcp.txpdu_sn;       /* current sequence number of last tx packet (or TX_NEXT) */
        rd->rxpdu_pkts = rb_pdcp.rxpdu_pkts;     /* aggregated number of rx packets */
        rd->rxpdu_bytes = rb_pdcp.rxpdu_bytes;    /* aggregated bytes of rx packets */
        rd->rxpdu_sn = rb_pdcp.rxpdu_sn;       /* current sequence number of last rx packet (or  RX_NEXT) */
        rd->rxpdu_oo_pkts = rb_pdcp.rxpdu_oo_pkts;       /* aggregated number of out-of-order rx pkts  (or RX_REORD) */
        rd->rxpdu_oo_bytes = rb_pdcp.rxpdu_oo_bytes; /* aggregated amount of out-of-order rx bytes */
        rd->rxpdu_dd_pkts = rb_pdcp.rxpdu_dd_pkts;  /* aggregated number of duplicated discarded packets (or dropped packets because of other reasons such as integrity failure) (or RX_DELIV) */
        rd->rxpdu_dd_bytes = rb_pdcp.rxpdu_dd_bytes; /* aggregated amount of discarded packets' bytes */
        rd->rxpdu_ro_count = rb_pdcp.rxpdu_ro_count; /* this state variable indicates the COUNT value following the COUNT value associated with the PDCP Data PDU which triggered t-Reordering. (RX_REORD) */
        rd->txsdu_pkts = rb_pdcp.txsdu_pkts;     /* number of SDUs delivered */
        rd->txsdu_bytes = rb_pdcp.txsdu_bytes;    /* number of bytes of SDUs delivered */
        rd->rxsdu_pkts = rb_pdcp.rxsdu_pkts;     /* number of SDUs received */
        rd->rxsdu_bytes = rb_pdcp.rxsdu_bytes;    /* number of bytes of SDUs received */
        rd->rnti = rnti;
        rd->mode = rb_pdcp.mode;               /* 0: PDCP AM, 1: PDCP UM, 2: PDCP TM */
        rd->rbid = rb_id;

        ++i;
      }
    }
  } else {

    //assert(0!=0 && "Calling PDCP");
    // for the moment and while we don't have a split base station, we use the
    // MAC structures to obtain the RNTIs which we use to query the PDCP
    NR_UEs_t *UE_info = &RC.nrmac[mod_id]->UE_info;
    uint32_t const act_rb = num_act_rb(UE_info);

    pdcp->msg.tstamp = time_now_us();

    pdcp->msg.len = act_rb;
    if (pdcp->msg.len > 0) {
      pdcp->msg.rb = calloc(pdcp->msg.len, sizeof(pdcp_radio_bearer_stats_t));
      assert(pdcp->msg.rb != NULL && "Memory exhausted!");
    }

    size_t i = 0;
    UE_iterator(UE_info->list, UE)
    {

      const int rnti = UE->rnti;
      for (size_t rb_id = 1; rb_id < 6; ++rb_id) {
        nr_pdcp_statistics_t rb_pdcp = {0};

        const int srb_flag = 0;
        const bool rc = nr_pdcp_get_statistics(rnti, srb_flag, rb_id, &rb_pdcp);

        if (!rc) continue;

        pdcp_radio_bearer_stats_t *rd = &pdcp->msg.rb[i];

        rd->txpdu_pkts = rb_pdcp.txpdu_pkts;     /* aggregated number of tx packets */
        rd->txpdu_bytes = rb_pdcp.txpdu_bytes;    /* aggregated bytes of tx packets */
        rd->txpdu_sn = rb_pdcp.txpdu_sn;       /* current sequence number of last tx packet (or TX_NEXT) */
        rd->rxpdu_pkts = rb_pdcp.rxpdu_pkts;     /* aggregated number of rx packets */
        rd->rxpdu_bytes = rb_pdcp.rxpdu_bytes;    /* aggregated bytes of rx packets */
        rd->rxpdu_sn = rb_pdcp.rxpdu_sn;       /* current sequence number of last rx packet (or  RX_NEXT) */
        rd->rxpdu_oo_pkts = rb_pdcp.rxpdu_oo_pkts;       /* aggregated number of out-of-order rx pkts  (or RX_REORD) */
        rd->rxpdu_oo_bytes = rb_pdcp.rxpdu_oo_bytes; /* aggregated amount of out-of-order rx bytes */
        rd->rxpdu_dd_pkts = rb_pdcp.rxpdu_dd_pkts;  /* aggregated number of duplicated discarded packets (or dropped packets because of other reasons such as integrity failure) (or RX_DELIV) */
        rd->rxpdu_dd_bytes = rb_pdcp.rxpdu_dd_bytes; /* aggregated amount of discarded packets' bytes */
        rd->rxpdu_ro_count = rb_pdcp.rxpdu_ro_count; /* this state variable indicates the COUNT value following the COUNT value associated with the PDCP Data PDU which triggered t-Reordering. (RX_REORD) */
        rd->txsdu_pkts = rb_pdcp.txsdu_pkts;     /* number of SDUs delivered */
        rd->txsdu_bytes = rb_pdcp.txsdu_bytes;    /* number of bytes of SDUs delivered */
        rd->rxsdu_pkts = rb_pdcp.rxsdu_pkts;     /* number of SDUs received */
        rd->rxsdu_bytes = rb_pdcp.rxsdu_bytes;    /* number of bytes of SDUs received */
        rd->rnti = rnti;
        rd->mode = rb_pdcp.mode;               /* 0: PDCP AM, 1: PDCP UM, 2: PDCP TM */
        rd->rbid = rb_id;

        ++i;
      }
    }
  }
}

void read_pdcp_setup_sm(void* data)
{
  assert(data != NULL);
//  assert(data->type == PDCP_AGENT_IF_E2_SETUP_ANS_V0 );

  assert(0 !=0 && "Not supported");
}

sm_ag_if_ans_t write_ctrl_pdcp_sm(void const* data)
{
  assert(data != NULL);
//  assert(data->type == PDCP_CTRL_REQ_V0 );
  assert(0 !=0 && "Not supported");
  sm_ag_if_ans_t ans = {0};
  return ans;
}


