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

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

#include "ran_func_rlc.h"
#include "common/ran_context.h"
#include "openair2/LAYER2/NR_MAC_gNB/mac_proto.h"
#include "openair2/LAYER2/nr_rlc/nr_rlc_oai_api.h"
#include "openair2/E2AP/flexric/src/util/time_now_us.h"

static
const int mod_id = 0;

static
uint32_t num_act_rb(NR_UEs_t* const UE_info)
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

static
void active_avg_to_tx(NR_UEs_t* const UE_info)
{
  assert(UE_info!= NULL);

  UE_iterator(UE_info->list, UE) {
    uint16_t const rnti = UE->rnti;
    for(int rb_id = 1; rb_id < 6; ++rb_id ){
      nr_rlc_statistics_t rlc = {0};
      const int srb_flag = 0;
      const bool rc = nr_rlc_get_statistics(rnti, srb_flag, rb_id, &rlc);
      if(rc) nr_rlc_activate_avg_time_to_tx(rnti, rb_id+3, 1);   // rb_id (DRB ID) mapping to logical channel
    }
  }
}

bool read_rlc_sm(void* data)
{
  assert(data != NULL);
//  assert(data->type ==  RLC_STATS_V0);

  rlc_ind_data_t* rlc = (rlc_ind_data_t*)data;
  //fill_rlc_ind_data(rlc);
  // use MAC structures to get RNTIs
  NR_UEs_t *UE_info = &RC.nrmac[mod_id]->UE_info;
  uint32_t const act_rb = num_act_rb(UE_info);

  //assert(0!=0 && "Read RLC called");

  // activate the rlc to calculate the average tx time
  active_avg_to_tx(UE_info);

  rlc->msg.len = act_rb;
  if(rlc->msg.len > 0){
    rlc->msg.rb = calloc(rlc->msg.len, sizeof(rlc_radio_bearer_stats_t));
    assert(rlc->msg.rb != NULL && "Memory exhausted");
  }

  rlc->msg.tstamp = time_now_us();

  uint32_t i = 0;
  UE_iterator(UE_info->list, UE) {
    uint16_t const rnti = UE->rnti;
    //for every LC ID
    for(int rb_id = 1; rb_id < 6; ++rb_id ){

      nr_rlc_statistics_t rb_rlc = {0};
      const int srb_flag = 0;
      const bool rc = nr_rlc_get_statistics(rnti, srb_flag, rb_id, &rb_rlc);
      if(!rc) continue;
      rlc_radio_bearer_stats_t* sm_rb = &rlc->msg.rb[i];

      /* TX */
      sm_rb->txpdu_pkts = rb_rlc.txpdu_pkts;
      sm_rb->txpdu_bytes =  rb_rlc.txpdu_bytes;        /* aggregated amount of transmitted bytes in RLC PDUs */

      /* TODO? */
      /* (TO BE DISCUSSED) HOL delay of the packet to be transmitted (RLC delay + MAC delay) */
      // sm_rb->txpdu_wt_ms += rb_rlc.txsdu_avg_time_to_tx;

      sm_rb->txpdu_dd_pkts = rb_rlc.txpdu_dd_pkts;      /* aggregated number of dropped or discarded tx packets by RLC */
      sm_rb->txpdu_dd_bytes = rb_rlc.txpdu_dd_bytes;     /* aggregated amount of bytes dropped or discarded tx packets by RLC */
      sm_rb->txpdu_retx_pkts = rb_rlc.txpdu_retx_pkts;    /* aggregated number of tx pdus/pkts to be re-transmitted (only applicable to RLC AM) */
      sm_rb->txpdu_retx_bytes = rb_rlc.txpdu_retx_bytes;   /* aggregated amount of bytes to be re-transmitted (only applicable to RLC AM) */
      sm_rb->txpdu_segmented = rb_rlc.txpdu_segmented;    /* aggregated number of segmentations */
      sm_rb->txpdu_status_pkts = rb_rlc.txpdu_status_pkts;  /* aggregated number of tx status pdus/pkts (only applicable to RLC AM) */
      sm_rb->txpdu_status_bytes = rb_rlc.txpdu_status_bytes; /* aggregated amount of tx status bytes  (only applicable to RLC AM) */
      sm_rb->txbuf_occ_bytes = rb_rlc.txbuf_occ_bytes;    /* (IMPLEMENTED) transmitting bytes currently in buffer */
      sm_rb->txbuf_occ_pkts = rb_rlc.txbuf_occ_pkts;     /* current tx buffer occupancy in terms of number of packets (average: NOT IMPLEMENTED) */

      /* RX */
      sm_rb->rxpdu_pkts = rb_rlc.rxpdu_pkts;         /* aggregated number of received RLC PDUs */
      sm_rb->rxpdu_bytes = rb_rlc.rxpdu_bytes;        /* amount of bytes received by the RLC */
      sm_rb->rxpdu_dup_pkts = rb_rlc.rxpdu_dup_pkts;     /* aggregated number of duplicate packets */
      sm_rb->rxpdu_dup_bytes = rb_rlc.rxpdu_dup_bytes;    /* aggregated amount of duplicated bytes */
      sm_rb->rxpdu_dd_pkts = rb_rlc.rxpdu_dd_pkts;      /* aggregated number of rx packets dropped or discarded by RLC */
      sm_rb->rxpdu_dd_bytes = rb_rlc.rxpdu_dd_bytes;     /* aggregated amount of rx bytes dropped or discarded by RLC */
      sm_rb->rxpdu_ow_pkts = rb_rlc.rxpdu_ow_pkts;      /* aggregated number of out of window received RLC pdu */
      sm_rb->rxpdu_ow_bytes = rb_rlc.rxpdu_ow_bytes;     /* aggregated number of out of window bytes received RLC pdu */
      sm_rb->rxpdu_status_pkts = rb_rlc.rxpdu_status_pkts;  /* aggregated number of rx status pdus/pkts (only applicable to RLC AM) */
      sm_rb->rxpdu_status_bytes = rb_rlc.rxpdu_status_bytes; /* aggregated amount of rx status bytes  (only applicable to RLC AM) */

      sm_rb->rxbuf_occ_bytes = rb_rlc.rxbuf_occ_bytes;    /* (IMPLEMENTED) received bytes currently in buffer */
      sm_rb->rxbuf_occ_pkts = rb_rlc.rxbuf_occ_pkts;     /* current rx buffer occupancy in terms of number of packets (average: NOT IMPLEMENTED) */

      /* TX */
      sm_rb->txsdu_pkts = rb_rlc.txsdu_pkts;         /* number of SDUs delivered */
      sm_rb->txsdu_bytes = rb_rlc.txsdu_bytes;        /* (UPDATED) number of SDUs bytes successfully transmitted so far (counter) */

      sm_rb->txsdu_avg_time_to_tx = rb_rlc.txsdu_avg_time_to_tx;    /* (100ms-windowed) per-packet sojourn (SDU to PDU) in microseconds */
      sm_rb->txsdu_wt_us = rb_rlc.txsdu_wt_us;           /* HOL delay of the current radio bearer */

      /* RX */
      sm_rb->rxsdu_pkts = rb_rlc.rxsdu_pkts;         /* number of SDUs received */
      sm_rb->rxsdu_bytes = rb_rlc.rxsdu_bytes;        /* (UPDATED) number of SDUs bytes arrived so far (counter) */
      sm_rb->rxsdu_dd_pkts = rb_rlc.rxsdu_dd_pkts;      /* number of dropped or discarded SDUs */
      sm_rb->rxsdu_dd_bytes = rb_rlc.rxsdu_dd_bytes;     /* number of bytes of SDUs dropped or discarded */

      sm_rb->mode = rb_rlc.mode;               /* 0: RLC AM, 1: RLC UM, 2: RLC TM */
      sm_rb->rnti = rnti;
      sm_rb->rbid = rb_id;

      LOG_D(RLC, "[E2-AGENT] SDU Goodput & HOL-waittime/200ms-sojourn: %lu/%lu (bytes) & %u/%f (microsecs);\n\
                             Current Buffer: received %u, transmitting %u (bytes).\n",
            sm_rb->txsdu_bytes, sm_rb->rxsdu_bytes, sm_rb->txsdu_wt_us, sm_rb->txsdu_avg_time_to_tx,
            sm_rb->rxbuf_occ_bytes, sm_rb->txbuf_occ_bytes);

      ++i;
    }
  }

  return act_rb > 0;
}

void read_rlc_setup_sm(void* data)
{
  assert(data != NULL);
//  assert(data->type == RLC_AGENT_IF_E2_SETUP_ANS_V0 );
  assert(0 !=0 && "Not supported");
}

sm_ag_if_ans_t write_ctrl_rlc_sm(void const* data)
{
  (void)data;
  assert(0!=0 && "Not supported");
}

