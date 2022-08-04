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

#define RLC_TM_MODULE 1
#define RLC_TM_INIT_C 1
//-----------------------------------------------------------------------------
#include "rlc_tm.h"
#include "LAYER2/MAC/mac_extern.h"
//-----------------------------------------------------------------------------
void config_req_rlc_tm (
  const protocol_ctxt_t* const  ctxt_pP,
  const srb_flag_t  srb_flagP,
  const rlc_tm_info_t * const config_tmP,
  const rb_id_t rb_idP,
  const logical_chan_id_t chan_idP
)
{
  rlc_union_t     *rlc_union_p  = NULL;
  rlc_tm_entity_t *rlc_p        = NULL;
  hash_key_t       key          = RLC_COLL_KEY_VALUE(ctxt_pP->module_id, ctxt_pP->rnti, ctxt_pP->enb_flag, rb_idP, srb_flagP);
  hashtable_rc_t   h_rc;

  h_rc = hashtable_get(rlc_coll_p, key, (void**)&rlc_union_p);

  if (h_rc == HASH_TABLE_OK) {
    rlc_p = &rlc_union_p->rlc.tm;
    LOG_D(RLC, PROTOCOL_RLC_TM_CTXT_FMT" CONFIG_REQ (is_uplink_downlink=%d) RB %ld\n",
          PROTOCOL_RLC_TM_CTXT_ARGS(ctxt_pP, rlc_p),
          config_tmP->is_uplink_downlink,
          rb_idP);

    rlc_tm_init(ctxt_pP, rlc_p);
    rlc_p->protocol_state = RLC_DATA_TRANSFER_READY_STATE;
    rlc_tm_set_debug_infos(ctxt_pP, rlc_p, srb_flagP, rb_idP, chan_idP);
    rlc_tm_configure(ctxt_pP, rlc_p, config_tmP->is_uplink_downlink);
  } else {
    LOG_E(RLC, PROTOCOL_RLC_TM_CTXT_FMT" CONFIG_REQ RB %ld RLC NOT FOUND\n",
          PROTOCOL_RLC_TM_CTXT_ARGS(ctxt_pP, rlc_p),
          rb_idP);
  }
}

//-----------------------------------------------------------------------------
void rlc_tm_init (
  const protocol_ctxt_t* const  ctxt_pP,
  rlc_tm_entity_t * const rlcP
)
{
  int saved_allocation = rlcP->allocation;
  memset (rlcP, 0, sizeof (struct rlc_tm_entity));
  rlcP->allocation = saved_allocation;
  // TX SIDE
  list_init (&rlcP->pdus_to_mac_layer, NULL);

  rlcP->protocol_state    = RLC_NULL_STATE;
  rlcP->nb_sdu            = 0;
  rlcP->next_sdu_index    = 0;
  rlcP->current_sdu_index = 0;

  rlcP->output_sdu_size_to_write = 0;
  rlcP->buffer_occupancy  = 0;

  // SPARE : not 3GPP
  rlcP->size_input_sdus_buffer = 16;

  if ((rlcP->input_sdus_alloc == NULL) && (rlcP->size_input_sdus_buffer > 0)) {
    rlcP->input_sdus_alloc = get_free_mem_block (rlcP->size_input_sdus_buffer * sizeof (void *), __func__);
    if(rlcP->input_sdus_alloc == NULL) return;
    rlcP->input_sdus = (mem_block_t **) (rlcP->input_sdus_alloc->data);
    memset (rlcP->input_sdus, 0, rlcP->size_input_sdus_buffer * sizeof (void *));
  }
}

//-----------------------------------------------------------------------------
void
rlc_tm_reset_state_variables (
  const protocol_ctxt_t* const  ctxt_pP,
  struct rlc_tm_entity * const rlcP
)
{
  rlcP->output_sdu_size_to_write = 0;
  rlcP->buffer_occupancy = 0;
  rlcP->nb_sdu = 0;
  rlcP->next_sdu_index = 0;
  rlcP->current_sdu_index = 0;
}
//-----------------------------------------------------------------------------
void
rlc_tm_cleanup (
  rlc_tm_entity_t * const rlcP
)
{
  int             index;
  // TX SIDE
  list_free (&rlcP->pdus_to_mac_layer);

  if (rlcP->input_sdus_alloc) {
    for (index = 0; index < rlcP->size_input_sdus_buffer; index++) {
      if (rlcP->input_sdus[index]) {
        free_mem_block (rlcP->input_sdus[index], __func__);
      }
    }

    free_mem_block (rlcP->input_sdus_alloc, __func__);
    rlcP->input_sdus_alloc = NULL;
  }

  // RX SIDE
  if ((rlcP->output_sdu_in_construction)) {
    free_mem_block (rlcP->output_sdu_in_construction, __func__);
    rlcP->output_sdu_in_construction = NULL;
  }

  memset(rlcP, 0, sizeof(rlc_tm_entity_t));
}

//-----------------------------------------------------------------------------
void rlc_tm_configure(const protocol_ctxt_t* const  ctxt_pP,
                      rlc_tm_entity_t * const rlcP,
                      const bool is_uplink_downlinkP)
{
  rlcP->is_uplink_downlink = is_uplink_downlinkP;
  rlc_tm_reset_state_variables (ctxt_pP, rlcP);
}

//-----------------------------------------------------------------------------
void rlc_tm_set_debug_infos(
  const protocol_ctxt_t* const  ctxt_pP,
  rlc_tm_entity_t * const rlcP,
  const srb_flag_t  srb_flagP,
  const rb_id_t     rb_idP,
  const logical_chan_id_t chan_idP) 
{
  rlcP->rb_id      = rb_idP;
  rlcP->channel_id = chan_idP;

  if (srb_flagP) {
    rlcP->is_data_plane = 0;
  } else {
    rlcP->is_data_plane = 1;
  }
}
