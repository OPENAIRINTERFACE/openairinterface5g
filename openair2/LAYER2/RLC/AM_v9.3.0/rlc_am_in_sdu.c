/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.0  (the "License"); you may not use this file
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
    if (rlcP->input_sdus[index_in_bufferP].mem_block != NULL) {
      free_mem_block(rlcP->input_sdus[index_in_bufferP].mem_block);
      rlcP->input_sdus[index_in_bufferP].mem_block = NULL;
      rlcP->nb_sdu_no_segmented -= 1;
      rlcP->input_sdus[index_in_bufferP].sdu_remaining_size = 0;
    }

    rlcP->nb_sdu -= 1;
    memset(&rlcP->input_sdus[index_in_bufferP], 0, sizeof(rlc_am_tx_sdu_management_t));
    rlcP->input_sdus[index_in_bufferP].flags.transmitted_successfully = 1;

    if (rlcP->current_sdu_index == index_in_bufferP) {
      rlcP->current_sdu_index = (rlcP->current_sdu_index + 1) % RLC_AM_SDU_CONTROL_BUFFER_SIZE;
    }

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
      free_mem_block(rlcP->input_sdus[index_in_bufferP].mem_block);
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
