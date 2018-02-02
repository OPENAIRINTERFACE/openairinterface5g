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

/***************************************************************************
                          rlc_am_in_sdu.c  -
                             -------------------
  AUTHOR  : Lionel GAUTHIER
  COMPANY : EURECOM
  EMAIL   : Lionel.Gauthier@eurecom.fr
 ***************************************************************************/
#define RLC_AM_MODULE 1
#define RLC_AM_IN_SDU_C 1
//-----------------------------------------------------------------------------
#include "rlc_am.h"
#include "LAYER2/MAC/extern.h"
#include "UTIL/LOG/log.h"

#define TRACE_RLC_AM_FREE_SDU 0
//-----------------------------------------------------------------------------
void rlc_am_free_in_sdu(
  const protocol_ctxt_t* const  ctxt_pP,
  rlc_am_entity_t *const        rlcP,
  const unsigned int            index_in_bufferP)
{
  if (index_in_bufferP <= RLC_AM_SDU_CONTROL_BUFFER_SIZE) {
	/* BugFix:  SDU shall have been already freed during initial PDU segmentation or concatenation !! */
	  AssertFatal(rlcP->input_sdus[index_in_bufferP].mem_block == NULL, "RLC AM Tx SDU Conf: Data Part is not empty index=%d LcId=%d\n",
				index_in_bufferP,rlcP->channel_id);
	/*
    if (rlcP->input_sdus[index_in_bufferP].mem_block != NULL) {
      free_mem_block(rlcP->input_sdus[index_in_bufferP].mem_block, __func__);
      rlcP->input_sdus[index_in_bufferP].mem_block = NULL;
      rlcP->nb_sdu_no_segmented -= 1;
      rlcP->input_sdus[index_in_bufferP].sdu_remaining_size = 0;
    }
    */

    rlcP->nb_sdu -= 1;
    memset(&rlcP->input_sdus[index_in_bufferP], 0, sizeof(rlc_am_tx_sdu_management_t));
    rlcP->input_sdus[index_in_bufferP].flags.transmitted_successfully = 1;

    // case when either one SDU needs to be removed from segmentation or SDU buffer is full
    if (rlcP->current_sdu_index == index_in_bufferP) {
      rlcP->current_sdu_index = (rlcP->current_sdu_index + 1) % RLC_AM_SDU_CONTROL_BUFFER_SIZE;
    }

    // wrapping and reset current_sdu_index to next_sdu_index when all transmitted SDUs have been acknowledged
    while ((rlcP->current_sdu_index != rlcP->next_sdu_index) &&
           (rlcP->input_sdus[rlcP->current_sdu_index].flags.transmitted_successfully == 1)) {
      rlcP->current_sdu_index = (rlcP->current_sdu_index + 1) % RLC_AM_SDU_CONTROL_BUFFER_SIZE;
    }
  }

#if TRACE_RLC_AM_FREE_SDU
  LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[FREE SDU] SDU INDEX %03u current_sdu_index=%u next_sdu_index=%u nb_sdu_no_segmented=%u\n",
        PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlcP),
        index_in_bufferP,
        rlcP->current_sdu_index,
        rlcP->next_sdu_index,
        rlcP->nb_sdu_no_segmented);
#endif
}
// called when segmentation is done
//-----------------------------------------------------------------------------
void
rlc_am_free_in_sdu_data(
  const protocol_ctxt_t* const ctxt_pP,
  rlc_am_entity_t* const       rlcP,
  const unsigned int           index_in_bufferP)
{
  if (index_in_bufferP <= RLC_AM_SDU_CONTROL_BUFFER_SIZE) {
    if (rlcP->input_sdus[index_in_bufferP].mem_block != NULL) {
      free_mem_block(rlcP->input_sdus[index_in_bufferP].mem_block, __func__);
      rlcP->input_sdus[index_in_bufferP].mem_block = NULL;
      rlcP->input_sdus[index_in_bufferP].sdu_remaining_size = 0;
      rlcP->nb_sdu_no_segmented -= 1;
    }
  }
}
//-----------------------------------------------------------------------------
signed int
rlc_am_in_sdu_is_empty(
  const protocol_ctxt_t* const ctxt_pP,
  rlc_am_entity_t       *const rlcP)
{
  if (rlcP->nb_sdu == 0) {
    return 1;
  }

  return 0;
}

// called when PDU is ACKED
//-----------------------------------------------------------------------------
void
rlc_am_pdu_sdu_data_cnf(
  const protocol_ctxt_t* const ctxt_pP,
  rlc_am_entity_t* const       rlc_pP,
  const rlc_sn_t           snP)
{
	  int          pdu_sdu_index;
	  int          sdu_index;

      for (pdu_sdu_index = 0; pdu_sdu_index < rlc_pP->tx_data_pdu_buffer[snP % RLC_AM_WINDOW_SIZE].nb_sdus; pdu_sdu_index++) {
        sdu_index = rlc_pP->tx_data_pdu_buffer[snP % RLC_AM_WINDOW_SIZE].sdus_index[pdu_sdu_index];
        assert(sdu_index >= 0);
        assert(sdu_index < RLC_AM_SDU_CONTROL_BUFFER_SIZE);
        rlc_pP->input_sdus[sdu_index].nb_pdus_ack += 1;

        if ((rlc_pP->input_sdus[sdu_index].nb_pdus_ack == rlc_pP->input_sdus[sdu_index].nb_pdus) &&
            (rlc_pP->input_sdus[sdu_index].sdu_remaining_size == 0)) {
  #if TEST_RLC_AM
          rlc_am_v9_3_0_test_data_conf (
            rlc_pP->module_id,
            rlc_pP->rb_id,
            rlc_pP->input_sdus[sdu_index].mui,
            RLC_SDU_CONFIRM_YES);
  #else
          rlc_data_conf(
            ctxt_pP,
            rlc_pP->rb_id,
            rlc_pP->input_sdus[sdu_index].mui,
            RLC_SDU_CONFIRM_YES,
            rlc_pP->is_data_plane);
  #endif
          rlc_am_free_in_sdu(ctxt_pP, rlc_pP, sdu_index);
        }
      }
}
