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

#define RLC_UM_MODULE 1
#define RLC_UM_CONTROL_PRIMITIVES_C 1
#include "platform_types.h"
#include "assertions.h"
//-----------------------------------------------------------------------------
#include "rlc_um.h"
#include "rlc_primitives.h"
#include "list.h"
#include "LAYER2/MAC/mac_extern.h"
#include "common/utils/LOG/log.h"

#include "rlc_um_control_primitives.h"
#include "LTE_T-Reordering.h"
#include "msc.h"

//-----------------------------------------------------------------------------
void config_req_rlc_um (
  const protocol_ctxt_t *const ctxt_pP,
  const srb_flag_t      srb_flagP,
  const rlc_um_info_t   *const config_um_pP,
  const rb_id_t           rb_idP,
  const logical_chan_id_t chan_idP) {
  rlc_union_t     *rlc_union_p  = NULL;
  rlc_um_entity_t *rlc_p        = NULL;
  hash_key_t       key          = RLC_COLL_KEY_VALUE(ctxt_pP->module_id, ctxt_pP->rnti, ctxt_pP->enb_flag, rb_idP, srb_flagP);
  hashtable_rc_t   h_rc;
  h_rc = hashtable_get(rlc_coll_p, key, (void **)&rlc_union_p);

  if (h_rc == HASH_TABLE_OK) {
    rlc_p = &rlc_union_p->rlc.um;
    LOG_D(RLC, PROTOCOL_RLC_UM_CTXT_FMT" CONFIG_REQ timer_reordering=%d sn_field_length=%d is_mXch=%d RB %ld\n",
          PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP,rlc_p),
          config_um_pP->timer_reordering,
          config_um_pP->sn_field_length,
          config_um_pP->is_mXch,
          rb_idP);
    rlc_um_init(ctxt_pP, rlc_p);

    if (rlc_um_fsm_notify_event (ctxt_pP, rlc_p, RLC_UM_RECEIVE_CRLC_CONFIG_REQ_ENTER_DATA_TRANSFER_READY_STATE_EVENT)) {
      rlc_um_set_debug_infos(ctxt_pP, rlc_p, srb_flagP, rb_idP, chan_idP);
      rlc_um_configure(
        ctxt_pP,
        rlc_p,
        config_um_pP->timer_reordering,
        config_um_pP->sn_field_length,
        config_um_pP->sn_field_length,
        config_um_pP->is_mXch);
    }
  } else {
    LOG_E(RLC, PROTOCOL_RLC_UM_CTXT_FMT"  CONFIG_REQ RB %ld  RLC UM NOT FOUND\n",
          PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP,rlc_p),
          rb_idP);
  }
}
//-----------------------------------------------------------------------------
const uint32_t t_Reordering_tab[32] = {0,5,10,15,20,25,30,35,40,45,50,55,60,65,70,75,80,85,90,95,100,110,120,130,140,150,160,170,180,190,200,1600};


void config_req_rlc_um_asn1 (
  const protocol_ctxt_t *const ctxt_pP,
  const srb_flag_t          srb_flagP,
  const MBMS_flag_t         mbms_flagP,
  const mbms_session_id_t   mbms_session_idP,
  const mbms_service_id_t   mbms_service_idP,
  const LTE_UL_UM_RLC_t    *const ul_rlc_pP,
  const LTE_DL_UM_RLC_t    *const dl_rlc_pP,
  const rb_id_t             rb_idP,
  const logical_chan_id_t   chan_idP,
  const uint32_t            sourceL2Id,
  const uint32_t            destinationL2Id
) {
  uint32_t         ul_sn_FieldLength   = 0;
  uint32_t         dl_sn_FieldLength   = 0;
  uint32_t         t_Reordering        = 0;
  rlc_union_t     *rlc_union_p         = NULL;
  rlc_um_entity_t *rlc_p               = NULL;
  hash_key_t       key                 = RLC_COLL_KEY_VALUE(ctxt_pP->module_id, ctxt_pP->rnti, ctxt_pP->enb_flag, rb_idP, srb_flagP);
  hashtable_rc_t   h_rc;

  if (mbms_flagP) {
    //AssertFatal(dl_rlc_pP, "No RLC UM DL config");
    if(dl_rlc_pP == NULL) {
      LOG_E(RLC, "No RLC UM DL config\n");
      return;
    }

    //AssertFatal(ul_rlc_pP == NULL, "RLC UM UL config present");
    if(ul_rlc_pP != NULL) {
      LOG_E(RLC, "RLC UM UL config present\n");
      return;
    }

    key = RLC_COLL_KEY_MBMS_VALUE(ctxt_pP->module_id, ctxt_pP->rnti, ctxt_pP->enb_flag, mbms_service_idP, mbms_session_idP);
    h_rc = hashtable_get(rlc_coll_p, key, (void **)&rlc_union_p);

    //AssertFatal (h_rc == HASH_TABLE_OK, "RLC NOT FOUND enb id %u rnti %i enb flag %u service id %u, session id %u",
    //             ctxt_pP->module_id,
    //             ctxt_pP->rnti,
    //             ctxt_pP->enb_flag,
    //             mbms_service_idP,
    //             mbms_session_idP);
    if(h_rc != HASH_TABLE_OK) {
      LOG_E(RLC, "RLC NOT FOUND enb id %u rnti %i enb flag %u service id %u, session id %u\n",
            ctxt_pP->module_id, ctxt_pP->rnti, ctxt_pP->enb_flag, mbms_service_idP, mbms_session_idP);
      return;
    }

    rlc_p = &rlc_union_p->rlc.um;
  } else if ((sourceL2Id >0 ) && (destinationL2Id >0)) {
    key = RLC_COLL_KEY_SOURCE_DEST_VALUE(ctxt_pP->module_id, ctxt_pP->rnti, ctxt_pP->enb_flag, rb_idP, sourceL2Id, destinationL2Id, srb_flagP);
  } else {
    key  = RLC_COLL_KEY_VALUE(ctxt_pP->module_id, ctxt_pP->rnti, ctxt_pP->enb_flag, rb_idP, srb_flagP);
  }

  h_rc = hashtable_get(rlc_coll_p, key, (void **)&rlc_union_p);

  //AssertFatal (h_rc == HASH_TABLE_OK, "RLC NOT FOUND enb id %u ue id %i enb flag %u rb id %u, srb flag %u",
  //             ctxt_pP->module_id,
  //             ctxt_pP->rnti,
  //             ctxt_pP->enb_flag,
  //             rb_idP,
  //             srb_flagP);
  if(h_rc != HASH_TABLE_OK) {
    LOG_E(RLC, "RLC NOT FOUND enb id %u ue id %i enb flag %u rb id %ld, srb flag %u\n",
          ctxt_pP->module_id, ctxt_pP->rnti, ctxt_pP->enb_flag, rb_idP, srb_flagP);
    return;
  }

  rlc_p = &rlc_union_p->rlc.um;
  //-----------------------------------------------------------------------------
  LOG_D(RLC, PROTOCOL_RLC_UM_CTXT_FMT"  CONFIG_REQ timer_reordering=%dms sn_field_length=   RB %ld \n",
        PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP,rlc_p),
        (dl_rlc_pP && dl_rlc_pP->t_Reordering<31)?t_Reordering_tab[dl_rlc_pP->t_Reordering]:-1,
        rb_idP);
  rlc_um_init(ctxt_pP, rlc_p);

  if (rlc_um_fsm_notify_event (ctxt_pP, rlc_p, RLC_UM_RECEIVE_CRLC_CONFIG_REQ_ENTER_DATA_TRANSFER_READY_STATE_EVENT)) {
    rlc_um_set_debug_infos(ctxt_pP,rlc_p, srb_flagP, rb_idP, chan_idP);

    if (ul_rlc_pP != NULL) {
      switch (ul_rlc_pP->sn_FieldLength) {
        case LTE_SN_FieldLength_size5:
          ul_sn_FieldLength = 5;
          break;

        case LTE_SN_FieldLength_size10:
          ul_sn_FieldLength = 10;
          break;

        default:
          LOG_E(RLC,PROTOCOL_RLC_UM_CTXT_FMT" [CONFIGURE] RB %ld INVALID UL sn_FieldLength %ld, RLC NOT CONFIGURED\n",
                PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP,rlc_p),
                rlc_p->rb_id,
                ul_rlc_pP->sn_FieldLength);
          MSC_LOG_RX_DISCARDED_MESSAGE(
            (ctxt_pP->enb_flag == ENB_FLAG_YES) ? MSC_RLC_ENB:MSC_RLC_UE,
            (ctxt_pP->enb_flag == ENB_FLAG_YES) ? MSC_RRC_ENB:MSC_RRC_UE,
            NULL,
            0,
            MSC_AS_TIME_FMT" "PROTOCOL_RLC_AM_MSC_FMT" CONFIG-REQ UL sn_FieldLength %u",
            MSC_AS_TIME_ARGS(ctxt_pP),
            PROTOCOL_RLC_AM_MSC_ARGS(ctxt_pP, rlc_p),
            ul_rlc_pP->sn_FieldLength);
          return;
      }
    }

    if (dl_rlc_pP != NULL) {
      switch (dl_rlc_pP->sn_FieldLength) {
        case LTE_SN_FieldLength_size5:
          dl_sn_FieldLength = 5;
          break;

        case LTE_SN_FieldLength_size10:
          dl_sn_FieldLength = 10;
          break;

        default:
          LOG_E(RLC,PROTOCOL_RLC_UM_CTXT_FMT" [CONFIGURE] RB %ld INVALID DL sn_FieldLength %ld, RLC NOT CONFIGURED\n",
                PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP,rlc_p),
                rlc_p->rb_id,
                dl_rlc_pP->sn_FieldLength);
          MSC_LOG_RX_DISCARDED_MESSAGE(
            (ctxt_pP->enb_flag == ENB_FLAG_YES) ? MSC_RLC_ENB:MSC_RLC_UE,
            (ctxt_pP->enb_flag == ENB_FLAG_YES) ? MSC_RRC_ENB:MSC_RRC_UE,
            NULL,
            0,
            MSC_AS_TIME_FMT" "PROTOCOL_RLC_AM_MSC_FMT" CONFIG-REQ DL sn_FieldLength %u",
            MSC_AS_TIME_ARGS(ctxt_pP),
            PROTOCOL_RLC_AM_MSC_ARGS(ctxt_pP, rlc_p),
            dl_rlc_pP->sn_FieldLength);
          return;
      }

      if (dl_rlc_pP->t_Reordering<32) {
        t_Reordering = t_Reordering_tab[dl_rlc_pP->t_Reordering];
      } else {
        LOG_E(RLC,PROTOCOL_RLC_UM_CTXT_FMT" [CONFIGURE] RB %ld INVALID T_Reordering %ld, RLC NOT CONFIGURED\n",
              PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP,rlc_p),
              rlc_p->rb_id,
              dl_rlc_pP->t_Reordering);
        MSC_LOG_RX_DISCARDED_MESSAGE(
          (ctxt_pP->enb_flag == ENB_FLAG_YES) ? MSC_RLC_ENB:MSC_RLC_UE,
          (ctxt_pP->enb_flag == ENB_FLAG_YES) ? MSC_RRC_ENB:MSC_RRC_UE,
          NULL,
          0,
          MSC_AS_TIME_FMT" "PROTOCOL_RLC_AM_MSC_FMT" CONFIG-REQ t_Reord %u",
          MSC_AS_TIME_ARGS(ctxt_pP),
          PROTOCOL_RLC_AM_MSC_ARGS(ctxt_pP, rlc_p),
          dl_rlc_pP->t_Reordering);
        return;
      }
    }

    if (ctxt_pP->enb_flag > 0) {
      rlc_um_configure(ctxt_pP,rlc_p,
                       t_Reordering,
                       ul_sn_FieldLength,
                       dl_sn_FieldLength,
                       mbms_flagP);
      MSC_LOG_RX_MESSAGE(
        (ctxt_pP->enb_flag == ENB_FLAG_YES) ? MSC_RLC_ENB:MSC_RLC_UE,
        (ctxt_pP->enb_flag == ENB_FLAG_YES) ? MSC_RRC_ENB:MSC_RRC_UE,
        NULL,
        0,
        MSC_AS_TIME_FMT" "PROTOCOL_RLC_AM_MSC_FMT" CONFIG-REQ t_Reord %u rx snfl %u tx snfl %u",
        MSC_AS_TIME_ARGS(ctxt_pP),
        PROTOCOL_RLC_AM_MSC_ARGS(ctxt_pP, rlc_p),
        t_Reordering,
        ul_sn_FieldLength,
        dl_sn_FieldLength);
    } else {
      rlc_um_configure(ctxt_pP,rlc_p,
                       t_Reordering,
                       dl_sn_FieldLength,
                       ul_sn_FieldLength,
                       mbms_flagP);
      MSC_LOG_RX_MESSAGE(
        (ctxt_pP->enb_flag == ENB_FLAG_YES) ? MSC_RLC_ENB:MSC_RLC_UE,
        (ctxt_pP->enb_flag == ENB_FLAG_YES) ? MSC_RRC_ENB:MSC_RRC_UE,
        NULL,
        0,
        MSC_AS_TIME_FMT" "PROTOCOL_RLC_AM_MSC_FMT" CONFIG-REQ t_Reord %u rx snfl %u tx snfl %u",
        MSC_AS_TIME_ARGS(ctxt_pP),
        PROTOCOL_RLC_AM_MSC_ARGS(ctxt_pP, rlc_p),
        t_Reordering,
        dl_sn_FieldLength,
        ul_sn_FieldLength);
    }
  }
}
//-----------------------------------------------------------------------------
void
rlc_um_init (
  const protocol_ctxt_t *const ctxt_pP,
  rlc_um_entity_t *const rlc_pP
) {
  //AssertFatal(rlc_pP, "Bad RLC UM pointer (NULL)");
  if(rlc_pP == NULL) {
    LOG_E(RLC, "Bad RLC UM pointer (NULL)\n");
    return;
  }

  if (rlc_pP->initialized) {
    LOG_D(RLC,PROTOCOL_RLC_UM_CTXT_FMT" [INIT] ALREADY DONE, DOING NOTHING\n",
          PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP,rlc_pP));
  } else {
    LOG_D(RLC, PROTOCOL_RLC_UM_CTXT_FMT" [INIT] STATE VARIABLES, BUFFERS, LISTS\n",
          PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP,rlc_pP));
    memset (rlc_pP, 0, sizeof (rlc_um_entity_t));
    // TX SIDE
    list_init (&rlc_pP->pdus_to_mac_layer, NULL);
    pthread_mutex_init(&rlc_pP->lock_input_sdus, NULL);
    list_init (&rlc_pP->input_sdus, NULL);
    rlc_pP->protocol_state = RLC_NULL_STATE;
    //rlc_pP->vt_us = 0;
    // RX SIDE
    list_init (&rlc_pP->pdus_from_mac_layer, NULL);
    //rlc_pP->vr_ur = 0;
    //rlc_pP->vr_ux = 0;
    //rlc_pP->vr_uh = 0;
    //rlc_pP->output_sdu_size_to_write = 0;
    //rlc_pP->output_sdu_in_construction = NULL;
    rlc_pP->rx_sn_length          = 10;
    rlc_pP->rx_header_min_length_in_bytes = 2;
    rlc_pP->tx_sn_length          = 10;
    rlc_pP->tx_header_min_length_in_bytes = 2;
    pthread_mutex_init(&rlc_pP->lock_dar_buffer, NULL);

    if (rlc_pP->dar_buffer == NULL) {
      rlc_pP->dar_buffer = calloc (1, 1024 * sizeof (void *));
    }

    rlc_pP->first_pdu = 1;
    rlc_pP->initialized = TRUE;
  }
}
//-----------------------------------------------------------------------------
void
rlc_um_reset_state_variables (
  const protocol_ctxt_t *const ctxt_pP,
  rlc_um_entity_t *const rlc_pP
) {
  rlc_pP->buffer_occupancy = 0;
  // TX SIDE
  rlc_pP->vt_us = 0;
  // RX SIDE
  rlc_pP->vr_ur = 0;
  rlc_pP->vr_ux = 0;
  rlc_pP->vr_uh = 0;
}
//-----------------------------------------------------------------------------
void
rlc_um_cleanup (
  rlc_um_entity_t *const rlc_pP) {
  int             index;
  // TX SIDE
  list_free (&rlc_pP->pdus_to_mac_layer);
  pthread_mutex_destroy(&rlc_pP->lock_input_sdus);
  list_free (&rlc_pP->input_sdus);
  // RX SIDE
  list_free (&rlc_pP->pdus_from_mac_layer);

  if ((rlc_pP->output_sdu_in_construction)) {
    free_mem_block (rlc_pP->output_sdu_in_construction, __func__);
    rlc_pP->output_sdu_in_construction = NULL;
  }

  if (rlc_pP->dar_buffer) {
    for (index = 0; index < 1024; index++) {
      if (rlc_pP->dar_buffer[index]) {
        free_mem_block (rlc_pP->dar_buffer[index], __func__);
      }
    }

    free (rlc_pP->dar_buffer);
    rlc_pP->dar_buffer = NULL;
  }

  pthread_mutex_destroy(&rlc_pP->lock_dar_buffer);
  memset(rlc_pP, 0, sizeof(rlc_um_entity_t));
}

//-----------------------------------------------------------------------------
void rlc_um_configure(
  const protocol_ctxt_t *const ctxt_pP,
  rlc_um_entity_t *const rlc_pP,
  const uint32_t         timer_reorderingP,
  const uint32_t         rx_sn_field_lengthP,
  const uint32_t         tx_sn_field_lengthP,
  const uint32_t         is_mXchP) {
  if (rx_sn_field_lengthP == 10) {
    rlc_pP->rx_sn_length                  = 10;
    rlc_pP->rx_sn_modulo                  = RLC_UM_SN_10_BITS_MODULO;
    rlc_pP->rx_um_window_size             = RLC_UM_WINDOW_SIZE_SN_10_BITS;
    rlc_pP->rx_header_min_length_in_bytes = 2;
  } else if (rx_sn_field_lengthP == 5) {
    rlc_pP->rx_sn_length                  = 5;
    rlc_pP->rx_sn_modulo                  = RLC_UM_SN_5_BITS_MODULO;
    rlc_pP->rx_um_window_size             = RLC_UM_WINDOW_SIZE_SN_5_BITS;
    rlc_pP->rx_header_min_length_in_bytes = 1;
  } else if (rx_sn_field_lengthP != 0) {
    LOG_E(RLC, PROTOCOL_RLC_UM_CTXT_FMT" [CONFIGURE] RB %ld INVALID RX SN LENGTH %d BITS NOT IMPLEMENTED YET, RLC NOT CONFIGURED\n",
          PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP,rlc_pP),
          rlc_pP->rb_id,
          rx_sn_field_lengthP);
    return;
  }

  if (tx_sn_field_lengthP == 10) {
    rlc_pP->tx_sn_length                  = 10;
    rlc_pP->tx_sn_modulo                  = RLC_UM_SN_10_BITS_MODULO;
    rlc_pP->tx_um_window_size             = RLC_UM_WINDOW_SIZE_SN_10_BITS;
    rlc_pP->tx_header_min_length_in_bytes = 2;
  } else if (tx_sn_field_lengthP == 5) {
    rlc_pP->tx_sn_length                  = 5;
    rlc_pP->tx_sn_modulo                  = RLC_UM_SN_5_BITS_MODULO;
    rlc_pP->tx_um_window_size             = RLC_UM_WINDOW_SIZE_SN_5_BITS;
    rlc_pP->tx_header_min_length_in_bytes = 1;
  } else if (tx_sn_field_lengthP != 0) {
    LOG_E(RLC, PROTOCOL_RLC_UM_CTXT_FMT" [CONFIGURE] RB %ld INVALID RX SN LENGTH %d BITS NOT IMPLEMENTED YET, RLC NOT CONFIGURED\n",
          PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP,rlc_pP),
          rlc_pP->rb_id,
          tx_sn_field_lengthP);
    return;
  }

  if (is_mXchP > 0) {
    rlc_pP->tx_um_window_size = 0;
    rlc_pP->rx_um_window_size = 0;
  }

  rlc_pP->is_mxch = is_mXchP;
  rlc_pP->last_reassemblied_sn  = rlc_pP->rx_sn_modulo - 1;
  rlc_pP->last_reassemblied_missing_sn  = rlc_pP->rx_sn_modulo - 1;
  rlc_pP->reassembly_missing_sn_detected = 0;
  // timers
  rlc_um_init_timer_reordering(ctxt_pP,rlc_pP, timer_reorderingP);
  rlc_pP->first_pdu = 1;
  rlc_um_reset_state_variables (ctxt_pP,rlc_pP);
}
//-----------------------------------------------------------------------------
void rlc_um_set_debug_infos(
  const protocol_ctxt_t *const ctxt_pP,
  rlc_um_entity_t *const rlc_pP,
  const srb_flag_t        srb_flagP,
  const rb_id_t           rb_idP,
  const logical_chan_id_t chan_idP) {
  LOG_D(RLC, PROTOCOL_RLC_UM_CTXT_FMT" [SET DEBUG INFOS] rb_id %ld srb_flag %d\n",
        PROTOCOL_RLC_UM_CTXT_ARGS(ctxt_pP,rlc_pP),
        rb_idP,
        srb_flagP);
  rlc_pP->rb_id      = rb_idP;
  rlc_pP->channel_id = chan_idP;

  if (srb_flagP) {
    rlc_pP->is_data_plane = 0;
  } else {
    rlc_pP->is_data_plane = 1;
  }
}
