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

/*
                                rlc.c
                             -------------------
  AUTHOR  : Lionel GAUTHIER
  COMPANY : EURECOM
  EMAIL   : Lionel.Gauthier at eurecom dot fr
*/
#define RLC_C
#include "rlc.h"
#include "mem_block.h"
#include "../MAC/mac_extern.h"
#include "LAYER2/RLC/UM_v9.3.0/rlc_um.h"
#include "common/utils/LOG/log.h"
#include "UTIL/OCG/OCG_vars.h"
#include "common/utils/LOG/vcd_signal_dumper.h"
#include "targets/COMMON/openairinterface5g_limits.h"
#include "assertions.h"

#include "common/ran_context.h"
extern RAN_CONTEXT_t RC;

extern boolean_t pdcp_data_ind(
  const protocol_ctxt_t *const ctxt_pP,
  const srb_flag_t srb_flagP,
  const MBMS_flag_t MBMS_flagP,
  const rb_id_t rb_idP,
  const sdu_size_t sdu_buffer_sizeP,
  mem_block_t *const sdu_buffer_pP);

#define DEBUG_RLC_PDCP_INTERFACE 1
//#define TRACE_RLC_PAYLOAD 1
#define DEBUG_RLC_DATA_REQ 1

//-----------------------------------------------------------------------------
void rlc_util_print_hex_octets(comp_name_t componentP, unsigned char *dataP, const signed long sizeP)
//-----------------------------------------------------------------------------
{
  unsigned long octet_index = 0;

  if (dataP == NULL) {
    return;
  }

  LOG_T(componentP, "+-----+-------------------------------------------------+\n");
  LOG_T(componentP, "|     |  0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f |\n");
  LOG_T(componentP, "+-----+-------------------------------------------------+\n");

  for (octet_index = 0; octet_index < sizeP; octet_index++) {
    if ((octet_index % 16) == 0) {
      if (octet_index != 0) {
        LOG_T(componentP, " |\n");
      }

      LOG_T(componentP, " %04lu |", octet_index);
    }

    /*
     * Print every single octet in hexadecimal form
     */
    LOG_T(componentP, " %02x", dataP[octet_index]);
    /*
     * Align newline and pipes according to the octets in groups of 2
     */
  }

  /*
   * Append enough spaces and put final pipe
   */
  unsigned char index;

  for (index = octet_index; index < 16; ++index) {
    LOG_T(componentP, "   ");
  }

  LOG_T(componentP, " |\n");
}

//-----------------------------------------------------------------------------
rlc_op_status_t rlc_stat_req     (
  const protocol_ctxt_t *const ctxt_pP,
  const srb_flag_t    srb_flagP,
  const rb_id_t       rb_idP,
  unsigned int *stat_rlc_mode,
  unsigned int *stat_tx_pdcp_sdu,
  unsigned int *stat_tx_pdcp_bytes,
  unsigned int *stat_tx_pdcp_sdu_discarded,
  unsigned int *stat_tx_pdcp_bytes_discarded,
  unsigned int *stat_tx_data_pdu,
  unsigned int *stat_tx_data_bytes,
  unsigned int *stat_tx_retransmit_pdu_by_status,
  unsigned int *stat_tx_retransmit_bytes_by_status,
  unsigned int *stat_tx_retransmit_pdu,
  unsigned int *stat_tx_retransmit_bytes,
  unsigned int *stat_tx_control_pdu,
  unsigned int *stat_tx_control_bytes,
  unsigned int *stat_rx_pdcp_sdu,
  unsigned int *stat_rx_pdcp_bytes,
  unsigned int *stat_rx_data_pdus_duplicate,
  unsigned int *stat_rx_data_bytes_duplicate,
  unsigned int *stat_rx_data_pdu,
  unsigned int *stat_rx_data_bytes,
  unsigned int *stat_rx_data_pdu_dropped,
  unsigned int *stat_rx_data_bytes_dropped,
  unsigned int *stat_rx_data_pdu_out_of_window,
  unsigned int *stat_rx_data_bytes_out_of_window,
  unsigned int *stat_rx_control_pdu,
  unsigned int *stat_rx_control_bytes,
  unsigned int *stat_timer_reordering_timed_out,
  unsigned int *stat_timer_poll_retransmit_timed_out,
  unsigned int *stat_timer_status_prohibit_timed_out) {
  //-----------------------------------------------------------------------------
  rlc_mode_t             rlc_mode        = RLC_MODE_NONE;
  rlc_union_t           *rlc_union_p     = NULL;
  hash_key_t             key             = HASHTABLE_NOT_A_KEY_VALUE;
  hashtable_rc_t         h_rc;

  //AssertFatal (rb_idP < NB_RB_MAX, "RB id is too high (%u/%d)!\n", rb_idP, NB_RB_MAX);
  if(rb_idP >= NB_RB_MAX) {
    LOG_E(RLC, "RB id is too high (%u/%d)!\n", rb_idP, NB_RB_MAX);
    return RLC_OP_STATUS_BAD_PARAMETER;
  }

  key = RLC_COLL_KEY_VALUE(ctxt_pP->module_id, ctxt_pP->rnti, ctxt_pP->enb_flag, rb_idP, srb_flagP);
  h_rc = hashtable_get(rlc_coll_p, key, (void **)&rlc_union_p);

  if (h_rc == HASH_TABLE_OK) {
    rlc_mode = rlc_union_p->mode;
  }

  *stat_rlc_mode                     = rlc_mode;

  switch (rlc_mode) {
    case RLC_MODE_NONE:
      *stat_tx_pdcp_sdu                     = 0;
      *stat_tx_pdcp_bytes                   = 0;
      *stat_tx_pdcp_sdu_discarded           = 0;
      *stat_tx_pdcp_bytes_discarded         = 0;
      *stat_tx_data_pdu                     = 0;
      *stat_tx_data_bytes                   = 0;
      *stat_tx_retransmit_pdu_by_status     = 0;
      *stat_tx_retransmit_bytes_by_status   = 0;
      *stat_tx_retransmit_pdu               = 0;
      *stat_tx_retransmit_bytes             = 0;
      *stat_tx_control_pdu                  = 0;
      *stat_tx_control_bytes                = 0;
      *stat_rx_pdcp_sdu                     = 0;
      *stat_rx_pdcp_bytes                   = 0;
      *stat_rx_data_pdus_duplicate          = 0;
      *stat_rx_data_bytes_duplicate         = 0;
      *stat_rx_data_pdu                     = 0;
      *stat_rx_data_bytes                   = 0;
      *stat_rx_data_pdu_dropped             = 0;
      *stat_rx_data_bytes_dropped           = 0;
      *stat_rx_data_pdu_out_of_window       = 0;
      *stat_rx_data_bytes_out_of_window     = 0;
      *stat_rx_control_pdu                  = 0;
      *stat_rx_control_bytes                = 0;
      *stat_timer_reordering_timed_out      = 0;
      *stat_timer_poll_retransmit_timed_out = 0;
      *stat_timer_status_prohibit_timed_out = 0;
      return RLC_OP_STATUS_BAD_PARAMETER;
      break;

    case RLC_MODE_AM:
      rlc_am_stat_req(ctxt_pP,
                      &rlc_union_p->rlc.am,
                      stat_tx_pdcp_sdu,
                      stat_tx_pdcp_bytes,
                      stat_tx_pdcp_sdu_discarded,
                      stat_tx_pdcp_bytes_discarded,
                      stat_tx_data_pdu,
                      stat_tx_data_bytes,
                      stat_tx_retransmit_pdu_by_status,
                      stat_tx_retransmit_bytes_by_status,
                      stat_tx_retransmit_pdu,
                      stat_tx_retransmit_bytes,
                      stat_tx_control_pdu,
                      stat_tx_control_bytes,
                      stat_rx_pdcp_sdu,
                      stat_rx_pdcp_bytes,
                      stat_rx_data_pdus_duplicate,
                      stat_rx_data_bytes_duplicate,
                      stat_rx_data_pdu,
                      stat_rx_data_bytes,
                      stat_rx_data_pdu_dropped,
                      stat_rx_data_bytes_dropped,
                      stat_rx_data_pdu_out_of_window,
                      stat_rx_data_bytes_out_of_window,
                      stat_rx_control_pdu,
                      stat_rx_control_bytes,
                      stat_timer_reordering_timed_out,
                      stat_timer_poll_retransmit_timed_out,
                      stat_timer_status_prohibit_timed_out);
      return RLC_OP_STATUS_OK;
      break;

    case RLC_MODE_UM:
      *stat_tx_retransmit_pdu_by_status     = 0;
      *stat_tx_retransmit_bytes_by_status   = 0;
      *stat_tx_retransmit_pdu               = 0;
      *stat_tx_retransmit_bytes             = 0;
      *stat_tx_control_pdu                  = 0;
      *stat_tx_control_bytes                = 0;
      *stat_rx_data_pdu_dropped             = 0;
      *stat_rx_data_bytes_dropped           = 0;
      *stat_rx_data_pdu_out_of_window       = 0;
      *stat_rx_data_bytes_out_of_window     = 0;
      *stat_timer_poll_retransmit_timed_out = 0;
      *stat_timer_status_prohibit_timed_out = 0;
      rlc_um_stat_req (&rlc_union_p->rlc.um,
                       stat_tx_pdcp_sdu,
                       stat_tx_pdcp_bytes,
                       stat_tx_pdcp_sdu_discarded,
                       stat_tx_pdcp_bytes_discarded,
                       stat_tx_data_pdu,
                       stat_tx_data_bytes,
                       stat_rx_pdcp_sdu,
                       stat_rx_pdcp_bytes,
                       stat_rx_data_pdus_duplicate,
                       stat_rx_data_bytes_duplicate,
                       stat_rx_data_pdu,
                       stat_rx_data_bytes,
                       stat_rx_data_pdu_dropped,
                       stat_rx_data_bytes_dropped,
                       stat_rx_data_pdu_out_of_window,
                       stat_rx_data_bytes_out_of_window,
                       stat_timer_reordering_timed_out);
      return RLC_OP_STATUS_OK;
      break;

    case RLC_MODE_TM:
      *stat_tx_pdcp_sdu                     = 0;
      *stat_tx_pdcp_bytes                   = 0;
      *stat_tx_pdcp_sdu_discarded           = 0;
      *stat_tx_pdcp_bytes_discarded         = 0;
      *stat_tx_data_pdu                     = 0;
      *stat_tx_data_bytes                   = 0;
      *stat_tx_retransmit_pdu_by_status     = 0;
      *stat_tx_retransmit_bytes_by_status   = 0;
      *stat_tx_retransmit_pdu               = 0;
      *stat_tx_retransmit_bytes             = 0;
      *stat_tx_control_pdu                  = 0;
      *stat_tx_control_bytes                = 0;
      *stat_rx_pdcp_sdu                     = 0;
      *stat_rx_pdcp_bytes                   = 0;
      *stat_rx_data_pdus_duplicate          = 0;
      *stat_rx_data_bytes_duplicate         = 0;
      *stat_rx_data_pdu                     = 0;
      *stat_rx_data_bytes                   = 0;
      *stat_rx_data_pdu_dropped             = 0;
      *stat_rx_data_bytes_dropped           = 0;
      *stat_rx_data_pdu_out_of_window       = 0;
      *stat_rx_data_bytes_out_of_window     = 0;
      *stat_rx_control_pdu                  = 0;
      *stat_rx_control_bytes                = 0;
      *stat_timer_reordering_timed_out      = 0;
      *stat_timer_poll_retransmit_timed_out = 0;
      *stat_timer_status_prohibit_timed_out = 0;
      return RLC_OP_STATUS_BAD_PARAMETER;
      break;

    default:
      *stat_tx_pdcp_sdu                     = 0;
      *stat_tx_pdcp_bytes                   = 0;
      *stat_tx_pdcp_sdu_discarded           = 0;
      *stat_tx_pdcp_bytes_discarded         = 0;
      *stat_tx_data_pdu                     = 0;
      *stat_tx_data_bytes                   = 0;
      *stat_tx_retransmit_pdu_by_status     = 0;
      *stat_tx_retransmit_bytes_by_status   = 0;
      *stat_tx_retransmit_pdu               = 0;
      *stat_tx_retransmit_bytes             = 0;
      *stat_tx_control_pdu                  = 0;
      *stat_tx_control_bytes                = 0;
      *stat_rx_pdcp_sdu                     = 0;
      *stat_rx_pdcp_bytes                   = 0;
      *stat_rx_data_pdus_duplicate          = 0;
      *stat_rx_data_bytes_duplicate         = 0;
      *stat_rx_data_pdu                     = 0;
      *stat_rx_data_bytes                   = 0;
      *stat_rx_data_pdu_dropped             = 0;
      *stat_rx_data_bytes_dropped           = 0;
      *stat_rx_data_pdu_out_of_window       = 0;
      *stat_rx_data_bytes_out_of_window     = 0;
      *stat_rx_control_pdu                  = 0;
      *stat_rx_control_bytes                = 0;
      *stat_timer_poll_retransmit_timed_out = 0;
      *stat_timer_status_prohibit_timed_out = 0;
      return RLC_OP_STATUS_BAD_PARAMETER;
  }
}

//-----------------------------------------------------------------------------
rlc_op_status_t rlc_data_req     (const protocol_ctxt_t *const ctxt_pP,
                                  const srb_flag_t   srb_flagP,
                                  const MBMS_flag_t  MBMS_flagP,
                                  const rb_id_t      rb_idP,
                                  const mui_t        muiP,
                                  confirm_t    confirmP,
                                  sdu_size_t   sdu_sizeP,
                                  mem_block_t *sdu_pP
#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))
  ,const uint32_t *const sourceL2Id
  ,const uint32_t *const destinationL2Id
#endif
                                 ) {
  //-----------------------------------------------------------------------------
  mem_block_t           *new_sdu_p    = NULL;
  rlc_mode_t             rlc_mode     = RLC_MODE_NONE;
  rlc_union_t           *rlc_union_p = NULL;
  hash_key_t             key         = HASHTABLE_NOT_A_KEY_VALUE;
  hashtable_rc_t         h_rc;
#if (LTE_RRC_VERSION >= MAKE_VERSION(10, 0, 0))
  rlc_mbms_id_t         *mbms_id_p  = NULL;
  logical_chan_id_t      log_ch_id  = 0;
#endif
#ifdef DEBUG_RLC_DATA_REQ
  LOG_D(RLC,PROTOCOL_CTXT_FMT"rlc_data_req:  rb_id %u (MAX %d), muip %d, confirmP %d, sdu_sizeP %d, sdu_pP %p\n",
        PROTOCOL_CTXT_ARGS(ctxt_pP),
        rb_idP,
        NB_RAB_MAX,
        muiP,
        confirmP,
        sdu_sizeP,
        sdu_pP);
#endif
#if (LTE_RRC_VERSION >= MAKE_VERSION(10, 0, 0))
#else
  AssertFatal(MBMS_flagP == 0, "MBMS_flagP %u", MBMS_flagP);
#endif
#if T_TRACER

  if (ctxt_pP->enb_flag)
    T(T_ENB_RLC_DL, T_INT(ctxt_pP->module_id), T_INT(ctxt_pP->rnti), T_INT(rb_idP), T_INT(sdu_sizeP));

#endif

  if (MBMS_flagP) {
    //AssertFatal (rb_idP < NB_RB_MBMS_MAX, "RB id is too high (%u/%d)!\n", rb_idP, NB_RB_MBMS_MAX);
    if(rb_idP >= NB_RB_MBMS_MAX) {
      LOG_E(RLC, "RB id is too high (%u/%d)!\n", rb_idP, NB_RB_MBMS_MAX);
      return RLC_OP_STATUS_BAD_PARAMETER;
    }
  } else {
    //AssertFatal (rb_idP < NB_RB_MAX, "RB id is too high (%u/%d)!\n", rb_idP, NB_RB_MAX);
    if(rb_idP >= NB_RB_MAX) {
      LOG_E(RLC, "RB id is too high (%u/%d)!\n", rb_idP, NB_RB_MAX);
      return RLC_OP_STATUS_BAD_PARAMETER;
    }
  }

  //DevAssert(sdu_pP != NULL);
  if(sdu_pP == NULL) {
    LOG_E(RLC, "sdu_pP == NULL\n");
    return RLC_OP_STATUS_BAD_PARAMETER;
  }

  //DevCheck(sdu_sizeP > 0, sdu_sizeP, 0, 0);
  if(sdu_sizeP <= 0) {
    LOG_E(RLC, "sdu_sizeP %d, file %s, line %d\n", sdu_sizeP, __FILE__,__LINE__);
    return RLC_OP_STATUS_BAD_PARAMETER;
  }

#if (LTE_RRC_VERSION < MAKE_VERSION(10, 0, 0))
  DevCheck(MBMS_flagP == 0, MBMS_flagP, 0, 0);
#endif
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_RLC_DATA_REQ,VCD_FUNCTION_IN);
#if (LTE_RRC_VERSION >= MAKE_VERSION(10, 0, 0))

  if (MBMS_flagP == TRUE) {
    if (ctxt_pP->enb_flag) {
      log_ch_id = rlc_mbms_enb_get_lcid_by_rb_id(ctxt_pP->module_id,rb_idP);
      mbms_id_p = &rlc_mbms_lcid2service_session_id_eNB[ctxt_pP->module_id][log_ch_id];
    } else {
      log_ch_id = rlc_mbms_ue_get_lcid_by_rb_id(ctxt_pP->rnti,rb_idP);
      mbms_id_p = &rlc_mbms_lcid2service_session_id_ue[ctxt_pP->rnti][log_ch_id];
    }

    key = RLC_COLL_KEY_MBMS_VALUE(ctxt_pP->module_id, ctxt_pP->rnti, ctxt_pP->enb_flag, mbms_id_p->service_id, mbms_id_p->session_id);
  }else

  if (sourceL2Id && destinationL2Id) {
    LOG_D (RLC, "RLC_COLL_KEY_VALUE: ctxt_pP->module_id: %d, ctxt_pP->rnti: %d, ctxt_pP->enb_flag: %d, rb_idP:%d, srb_flagP: %d \n \n", ctxt_pP->module_id, ctxt_pP->rnti, ctxt_pP->enb_flag, rb_idP,
           srb_flagP);
    key = RLC_COLL_KEY_VALUE(ctxt_pP->module_id, ctxt_pP->rnti, ctxt_pP->enb_flag, rb_idP, srb_flagP);
    //Thinh's line originally uncommented
    //key = RLC_COLL_KEY_SOURCE_DEST_VALUE(ctxt_pP->module_id, ctxt_pP->rnti, ctxt_pP->enb_flag, rb_idP, *sourceL2Id, *destinationL2Id, srb_flagP);
    //key_lcid = RLC_COLL_KEY_LCID_SOURCE_DEST_VALUE(ctxt_pP->module_id, ctxt_pP->rnti, ctxt_pP->enb_flag, chan_idP, *sourceL2Id, *destinationL2Id, srb_flagP);
  } else
#endif
  {
    LOG_D (RLC, "RLC_COLL_KEY_VALUE: ctxt_pP->module_id: %d, ctxt_pP->rnti: %d, ctxt_pP->enb_flag: %d, rb_idP:%d, srb_flagP: %d \n \n", ctxt_pP->module_id, ctxt_pP->rnti, ctxt_pP->enb_flag, rb_idP,
           srb_flagP);
    key = RLC_COLL_KEY_VALUE(ctxt_pP->module_id, ctxt_pP->rnti, ctxt_pP->enb_flag, rb_idP, srb_flagP);
  }

  h_rc = hashtable_get(rlc_coll_p, key, (void **)&rlc_union_p);

  if (h_rc == HASH_TABLE_OK) {
    rlc_mode = rlc_union_p->mode;
  } else {
    rlc_mode = RLC_MODE_NONE;
    //AssertFatal (0 , "RLC not configured key %ju\n", key);
    LOG_E(RLC, "not configured key %lu\n", key);
    return RLC_OP_STATUS_OUT_OF_RESSOURCES;
  }

  if (MBMS_flagP == 0) {
    LOG_D(RLC, PROTOCOL_CTXT_FMT"[RB %u] Display of rlc_data_req:\n",
          PROTOCOL_CTXT_ARGS(ctxt_pP),
          rb_idP);
#if defined(TRACE_RLC_PAYLOAD)
    rlc_util_print_hex_octets(RLC, (unsigned char *)sdu_pP->data, sdu_sizeP);
#endif
#ifdef DEBUG_RLC_DATA_REQ
    LOG_D(RLC,"RLC_TYPE : %d\n", rlc_mode);
#endif

    switch (rlc_mode) {
      case RLC_MODE_NONE:
        free_mem_block(sdu_pP, __func__);
        LOG_E(RLC, PROTOCOL_CTXT_FMT" Received RLC_MODE_NONE as rlc_type for rb_id %u\n",
              PROTOCOL_CTXT_ARGS(ctxt_pP),
              rb_idP);
        VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_RLC_DATA_REQ,VCD_FUNCTION_OUT);
        return RLC_OP_STATUS_BAD_PARAMETER;

      case RLC_MODE_AM:
#ifdef DEBUG_RLC_DATA_REQ
        LOG_D(RLC,"RLC_MODE_AM\n");
#endif
        new_sdu_p = get_free_mem_block (sdu_sizeP + sizeof (struct rlc_am_data_req_alloc), __func__);

        if (new_sdu_p != NULL) {
          // PROCESS OF COMPRESSION HERE:
          memset (new_sdu_p->data, 0, sizeof (struct rlc_am_data_req_alloc));
          memcpy (&new_sdu_p->data[sizeof (struct rlc_am_data_req_alloc)], &sdu_pP->data[0], sdu_sizeP);
          ((struct rlc_am_data_req *) (new_sdu_p->data))->data_size = sdu_sizeP;
          ((struct rlc_am_data_req *) (new_sdu_p->data))->conf = confirmP;
          ((struct rlc_am_data_req *) (new_sdu_p->data))->mui  = muiP;
          ((struct rlc_am_data_req *) (new_sdu_p->data))->data_offset = sizeof (struct rlc_am_data_req_alloc);
          free_mem_block(sdu_pP, __func__);
          rlc_am_data_req(ctxt_pP, &rlc_union_p->rlc.am, new_sdu_p);
          VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_RLC_DATA_REQ,VCD_FUNCTION_OUT);
          return RLC_OP_STATUS_OK;
        } else {
          VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_RLC_DATA_REQ,VCD_FUNCTION_OUT);
          free_mem_block(sdu_pP, __func__);
          return RLC_OP_STATUS_INTERNAL_ERROR;
        }

        break;

      case RLC_MODE_UM:

        /* TODO: this is a hack, needs better solution. Let's not use too
         * much memory and store at maximum 5 millions bytes.
         */
        /* look for HACK_RLC_UM_LIMIT for others places related to the hack. Please do not remove this comment. */
        if (rlc_um_get_buffer_occupancy(&rlc_union_p->rlc.um) > 5000000) {
          free_mem_block(sdu_pP, __func__);
          return RLC_OP_STATUS_OUT_OF_RESSOURCES;
        }

        new_sdu_p = get_free_mem_block (sdu_sizeP + sizeof (struct rlc_um_data_req_alloc), __func__);

        if (new_sdu_p != NULL) {
          // PROCESS OF COMPRESSION HERE:
          memset (new_sdu_p->data, 0, sizeof (struct rlc_um_data_req_alloc));
          memcpy (&new_sdu_p->data[sizeof (struct rlc_um_data_req_alloc)], &sdu_pP->data[0], sdu_sizeP);
          ((struct rlc_um_data_req *) (new_sdu_p->data))->data_size = sdu_sizeP;
          ((struct rlc_um_data_req *) (new_sdu_p->data))->data_offset = sizeof (struct rlc_um_data_req_alloc);
          free_mem_block(sdu_pP, __func__);
          rlc_um_data_req(ctxt_pP, &rlc_union_p->rlc.um, new_sdu_p);
          //free_mem_block(new_sdu, __func__);
          VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_RLC_DATA_REQ,VCD_FUNCTION_OUT);
          return RLC_OP_STATUS_OK;
        } else {
          VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_RLC_DATA_REQ,VCD_FUNCTION_OUT);
          free_mem_block(sdu_pP, __func__);
          return RLC_OP_STATUS_INTERNAL_ERROR;
        }

        break;

      case RLC_MODE_TM:
        new_sdu_p = get_free_mem_block (sdu_sizeP + sizeof (struct rlc_tm_data_req_alloc), __func__);

        if (new_sdu_p != NULL) {
          // PROCESS OF COMPRESSION HERE:
          memset (new_sdu_p->data, 0, sizeof (struct rlc_tm_data_req_alloc));
          memcpy (&new_sdu_p->data[sizeof (struct rlc_tm_data_req_alloc)], &sdu_pP->data[0], sdu_sizeP);
          ((struct rlc_tm_data_req *) (new_sdu_p->data))->data_size = sdu_sizeP;
          ((struct rlc_tm_data_req *) (new_sdu_p->data))->data_offset = sizeof (struct rlc_tm_data_req_alloc);
          free_mem_block(sdu_pP, __func__);
          rlc_tm_data_req(ctxt_pP, &rlc_union_p->rlc.tm, new_sdu_p);
          VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_RLC_DATA_REQ,VCD_FUNCTION_OUT);
          return RLC_OP_STATUS_OK;
        } else {
          //handle_event(ERROR,"FILE %s FONCTION rlc_data_req() LINE %s : out of memory\n", __FILE__, __LINE__);
          VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_RLC_DATA_REQ,VCD_FUNCTION_OUT);
          free_mem_block(sdu_pP, __func__);
          return RLC_OP_STATUS_INTERNAL_ERROR;
        }

        break;

      default:
        free_mem_block(sdu_pP, __func__);
        VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_RLC_DATA_REQ,VCD_FUNCTION_OUT);
        return RLC_OP_STATUS_INTERNAL_ERROR;
    }

#if (LTE_RRC_VERSION >= MAKE_VERSION(10, 0, 0))
  } else { /* MBMS_flag != 0 */
    //  LOG_I(RLC,"DUY rlc_data_req: mbms_rb_id in RLC instant is: %d\n", mbms_rb_id);
    if (sdu_pP != NULL) {
      if (sdu_sizeP > 0) {
        LOG_I(RLC,"received a packet with size %d for MBMS \n", sdu_sizeP);
        new_sdu_p = get_free_mem_block (sdu_sizeP + sizeof (struct rlc_um_data_req_alloc), __func__);

        if (new_sdu_p != NULL) {
          // PROCESS OF COMPRESSION HERE:
          memset (new_sdu_p->data, 0, sizeof (struct rlc_um_data_req_alloc));
          memcpy (&new_sdu_p->data[sizeof (struct rlc_um_data_req_alloc)], &sdu_pP->data[0], sdu_sizeP);
          ((struct rlc_um_data_req *) (new_sdu_p->data))->data_size = sdu_sizeP;
          ((struct rlc_um_data_req *) (new_sdu_p->data))->data_offset = sizeof (struct rlc_um_data_req_alloc);
          free_mem_block(sdu_pP, __func__);
          rlc_um_data_req(ctxt_pP, &rlc_union_p->rlc.um, new_sdu_p);
          //free_mem_block(new_sdu, __func__);
          VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_RLC_DATA_REQ,VCD_FUNCTION_OUT);
          return RLC_OP_STATUS_OK;
        } else {
          VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_RLC_DATA_REQ,VCD_FUNCTION_OUT);
          free_mem_block(sdu_pP, __func__);
          return RLC_OP_STATUS_BAD_PARAMETER;
        }
      } else {
        VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_RLC_DATA_REQ,VCD_FUNCTION_OUT);
        return RLC_OP_STATUS_BAD_PARAMETER;
      }
    } else {
      VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_RLC_DATA_REQ,VCD_FUNCTION_OUT);
      return RLC_OP_STATUS_BAD_PARAMETER;
    }
  }

#else
  } else { /* MBMS_flag != 0 */
    free_mem_block(sdu_pP, __func__);
    LOG_E(RLC, "MBMS_flag != 0 while Rel10/Rel14 is not defined...\n");
    //handle_event(ERROR,"FILE %s FONCTION rlc_data_req() LINE %s : parameter module_id out of bounds :%d\n", __FILE__, __LINE__, module_idP);
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_RLC_DATA_REQ,VCD_FUNCTION_OUT);
    return RLC_OP_STATUS_BAD_PARAMETER;
  }

#endif
}

//-----------------------------------------------------------------------------
void rlc_data_ind     (
  const protocol_ctxt_t *const ctxt_pP,
  const srb_flag_t  srb_flagP,
  const MBMS_flag_t MBMS_flagP,
  const rb_id_t     rb_idP,
  const sdu_size_t  sdu_sizeP,
  mem_block_t      *sdu_pP) {
  //-----------------------------------------------------------------------------
  LOG_D(RLC, PROTOCOL_CTXT_FMT"[%s %u] Display of rlc_data_ind: size %u\n",
        PROTOCOL_CTXT_ARGS(ctxt_pP),
        (srb_flagP) ? "SRB" : "DRB",
        rb_idP,
        sdu_sizeP);
  rlc_util_print_hex_octets(RLC, (unsigned char *)sdu_pP->data, sdu_sizeP);

  if (ctxt_pP->enb_flag) {
#if T_TRACER
    T(T_ENB_RLC_UL, T_INT(ctxt_pP->module_id), T_INT(ctxt_pP->rnti), T_INT(rb_idP), T_INT(sdu_sizeP));
#endif
    const ngran_node_t type = RC.rrc[ctxt_pP->module_id]->node_type;
    AssertFatal(type != ngran_eNB_CU && type != ngran_ng_eNB_CU && type != ngran_gNB_CU,
                "Can't be CU, bad node type %d\n", type);

    if (NODE_IS_DU(type) && srb_flagP == 1) {
      MessageDef *msg = itti_alloc_new_message(TASK_RLC_ENB, F1AP_UL_RRC_MESSAGE);
      F1AP_UL_RRC_MESSAGE(msg).rnti = ctxt_pP->rnti;
      F1AP_UL_RRC_MESSAGE(msg).srb_id = rb_idP;
      F1AP_UL_RRC_MESSAGE(msg).rrc_container = sdu_pP->data;
      F1AP_UL_RRC_MESSAGE(msg).rrc_container_length = sdu_sizeP;
      itti_send_msg_to_task(TASK_DU_F1, ENB_MODULE_ID_TO_INSTANCE(ctxt_pP->module_id), msg);
      return;
    }
  } // case monolithic eNodeB or UE

  get_pdcp_data_ind_func()(ctxt_pP, srb_flagP, MBMS_flagP, rb_idP, sdu_sizeP, sdu_pP,NULL,NULL);
}
//-----------------------------------------------------------------------------
void rlc_data_conf     (const protocol_ctxt_t *const ctxt_pP,
                        const srb_flag_t      srb_flagP,
                        const rb_id_t         rb_idP,
                        const mui_t           muiP,
                        const rlc_tx_status_t statusP) {
  //-----------------------------------------------------------------------------
  if (srb_flagP) {
    if (rlc_rrc_data_conf != NULL) {
      rlc_rrc_data_conf (ctxt_pP, rb_idP, muiP, statusP);
    }
  }
}
//-----------------------------------------------------------------------------
int
rlc_module_init (void) {
  //-----------------------------------------------------------------------------
  int          k;
  module_id_t  module_id1;
  /* for no gcc warnings */
  (void)k;
  LOG_D(RLC, "MODULE INIT\n");
  rlc_rrc_data_ind  = NULL;
  rlc_rrc_data_conf = NULL;
  rlc_coll_p = hashtable_create ((LTE_maxDRB + 2) * NUMBER_OF_UE_MAX, NULL, rb_free_rlc_union);

  //AssertFatal(rlc_coll_p != NULL, "UNRECOVERABLE error, RLC hashtable_create failed");
  if(rlc_coll_p == NULL) {
    LOG_E(RLC, "UNRECOVERABLE error, RLC hashtable_create failed\n");
    return -1;
  }

  for (module_id1=0; module_id1 < MAX_MOBILES_PER_ENB; module_id1++) {
    for (k=0; k < RLC_MAX_MBMS_LC; k++) {
      rlc_mbms_lcid2service_session_id_ue[module_id1][k].service_id = 0;
      rlc_mbms_lcid2service_session_id_ue[module_id1][k].session_id = 0;
    }

    for (k=0; k < NB_RB_MBMS_MAX; k++) {
      rlc_mbms_rbid2lcid_ue[module_id1][k] = RLC_LC_UNALLOCATED;
    }
  }

  for (k=0; k < RLC_MAX_MBMS_LC; k++) {
    rlc_mbms_lcid2service_session_id_eNB[0][k].service_id = 0;
    rlc_mbms_lcid2service_session_id_eNB[0][k].session_id = 0;
  }

  for (k=0; k < NB_RB_MBMS_MAX; k++) {
    rlc_mbms_rbid2lcid_eNB[0][k] = RLC_LC_UNALLOCATED;
  }

  pool_buffer_init();
  return(0);
}
//-----------------------------------------------------------------------------
void
rlc_module_cleanup (void)
//-----------------------------------------------------------------------------
{
  hashtable_destroy(&rlc_coll_p);
}
//-----------------------------------------------------------------------------
void
rlc_layer_init (void) {
  //-----------------------------------------------------------------------------
}
//-----------------------------------------------------------------------------
void
rlc_layer_cleanup (void)
//-----------------------------------------------------------------------------
{
}

