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
#define RLC_TM_C 1
//-----------------------------------------------------------------------------
#include "platform_types.h"
#include "platform_constants.h"
//-----------------------------------------------------------------------------
#include "rlc_tm.h"
#include "mac_primitives.h"
#include "rlc_primitives.h"
#include "list.h"
#include "LAYER2/MAC/mac_extern.h"
//-----------------------------------------------------------------------------
void
rlc_tm_send_sdu (
  const protocol_ctxt_t* const  ctxt_pP,
  rlc_tm_entity_t * const rlc_pP,
  const boolean_t         error_indicationP,
  uint8_t * const         srcP,
  const sdu_size_t        length_in_bitsP)
{
  int             length_in_bytes;
#if DEBUG_RLC_TM_DISPLAY_ASCII_DATA
  int             index;
#endif
#if DEBUG_RLC_TM_REASSEMBLY
  LOG_D(RLC, PROTOCOL_RLC_TM_CTXT_FMT"[SEND_SDU] %d bits\n",
        PROTOCOL_RLC_TM_CTXT_ARGS(ctxt_pP, rlc_pP),
        length_in_bitsP);
#endif
  length_in_bytes = (length_in_bitsP + 7) >> 3;

  if (rlc_pP->output_sdu_in_construction == NULL) {
    rlc_pP->output_sdu_in_construction = get_free_mem_block (length_in_bytes, __func__);
  }

  if ((rlc_pP->output_sdu_in_construction)) {
#if DEBUG_RLC_TM_DISPLAY_ASCII_DATA
    LOG_D(RLC, PROTOCOL_RLC_TM_CTXT_FMT"[SEND_SDU] DATA :",
          PROTOCOL_RLC_TM_CTXT_ARGS(ctxt_pP, rlc_pP));

    for (index = 0; index < length_in_bytes; index++) {
      LOG_T (RLC,"%c", srcP[index]);
    }

    LOG_T (RLC,"\n");
#endif

    memcpy (&rlc_pP->output_sdu_in_construction->data[rlc_pP->output_sdu_size_to_write], srcP, length_in_bytes);

    rlc_data_ind (
      ctxt_pP,
      BOOL_NOT(rlc_pP->is_data_plane),
      MBMS_FLAG_NO,
      rlc_pP->rb_id,
      length_in_bytes,
      rlc_pP->output_sdu_in_construction);
    rlc_pP->output_sdu_in_construction = NULL;
  } else {
    LOG_D (RLC,"[RLC_TM %p][SEND_SDU] ERROR  OUTPUT SDU IS NULL\n", rlc_pP);
  }
}
//-----------------------------------------------------------------------------
void
rlc_tm_no_segment (
  const protocol_ctxt_t* const  ctxt_pP,
  rlc_tm_entity_t *const rlc_pP
)
{
  mem_block_t                     *pdu_p               = NULL;
  struct rlc_tm_tx_sdu_management *sdu_mngt_p          = NULL;
  struct rlc_tm_tx_pdu_management *pdu_mngt_p          = NULL;
  int                              nb_pdu_to_transmit  = 1;

  // only one SDU per TTI
  while ((rlc_pP->input_sdus[rlc_pP->current_sdu_index]) && (nb_pdu_to_transmit > 0)) {

    sdu_mngt_p = ((struct rlc_tm_tx_sdu_management *) (rlc_pP->input_sdus[rlc_pP->current_sdu_index]->data));

    if (!(pdu_p = get_free_mem_block (((rlc_pP->rlc_pdu_size + 7) >> 3) + sizeof (struct rlc_tm_tx_data_pdu_struct) + GUARD_CRC_LIH_SIZE, __func__))) {
      LOG_D(RLC, PROTOCOL_RLC_TM_CTXT_FMT"[SEGMENT] ERROR COULD NOT GET NEW PDU, EXIT\n",
            PROTOCOL_RLC_TM_CTXT_ARGS(ctxt_pP, rlc_pP));
      return;
    }

    // SHOULD BE OPTIMIZED...SOON
    pdu_mngt_p = (struct rlc_tm_tx_pdu_management *) (pdu_p->data);
    memset (pdu_p->data, 0, sizeof (struct rlc_tm_tx_pdu_management));
    pdu_mngt_p->first_byte = (uint8_t*)&pdu_p->data[sizeof (struct rlc_tm_tx_data_pdu_struct)];

    memcpy (pdu_mngt_p->first_byte, sdu_mngt_p->first_byte, ((rlc_pP->rlc_pdu_size + 7) >> 3));
    ((struct mac_tb_req *) (pdu_p->data))->rlc = NULL;
    ((struct mac_tb_req *) (pdu_p->data))->data_ptr = pdu_mngt_p->first_byte;
    ((struct mac_tb_req *) (pdu_p->data))->first_bit = 0;
    ((struct mac_tb_req *) (pdu_p->data))->tb_size = rlc_pP->rlc_pdu_size >> 3;
    list_add_tail_eurecom (pdu_p, &rlc_pP->pdus_to_mac_layer);

    rlc_pP->buffer_occupancy -= (sdu_mngt_p->sdu_size >> 3);
    free_mem_block (rlc_pP->input_sdus[rlc_pP->current_sdu_index], __func__);
    rlc_pP->input_sdus[rlc_pP->current_sdu_index] = NULL;
    rlc_pP->current_sdu_index = (rlc_pP->current_sdu_index + 1) % rlc_pP->size_input_sdus_buffer;
    rlc_pP->nb_sdu -= 1;
  }
}
//-----------------------------------------------------------------------------
void
rlc_tm_rx (
  const protocol_ctxt_t* const  ctxt_pP,
  void * const         argP,
  struct mac_data_ind data_indP)
{

  rlc_tm_entity_t     * const rlc_p = (rlc_tm_entity_t *) argP;
  mem_block_t         *tb_p;
  uint8_t             *first_byte_p;

  rlc_p->output_sdu_size_to_write = 0;      // size of sdu reassemblied

  while ((tb_p = list_remove_head (&data_indP.data))) {
    first_byte_p = ((struct mac_tb_ind *) (tb_p->data))->data_ptr;

    ((struct rlc_tm_rx_pdu_management *) (tb_p->data))->first_byte = first_byte_p;

    rlc_tm_send_sdu (ctxt_pP, rlc_p, (((struct mac_tb_ind *) (tb_p->data))->error_indication), first_byte_p, data_indP.tb_size);
    free_mem_block (tb_p, __func__);
  }
}

//-----------------------------------------------------------------------------
struct mac_status_resp
rlc_tm_mac_status_indication (
  const protocol_ctxt_t* const  ctxt_pP,
  void * const                  rlc_pP,
  const tb_size_t               tb_sizeP,
  struct mac_status_ind         tx_statusP)
{
  struct mac_status_resp status_resp;

  ((rlc_tm_entity_t *) rlc_pP)->rlc_pdu_size = tb_sizeP;

  status_resp.buffer_occupancy_in_bytes = ((rlc_tm_entity_t *) rlc_pP)->buffer_occupancy;
  status_resp.buffer_occupancy_in_pdus = status_resp.buffer_occupancy_in_bytes / ((rlc_tm_entity_t *) rlc_pP)->rlc_pdu_size;
  status_resp.rlc_info.rlc_protocol_state = ((rlc_tm_entity_t *) rlc_pP)->protocol_state;
  return status_resp;
}

//-----------------------------------------------------------------------------
struct mac_data_req
rlc_tm_mac_data_request (
  const protocol_ctxt_t* const  ctxt_pP,
  void * const rlc_pP)
{
  rlc_tm_entity_t*    rlc_p = (rlc_tm_entity_t*) rlc_pP;
  struct mac_data_req data_req;

  rlc_tm_no_segment (ctxt_pP, rlc_p);
  list_init (&data_req.data, NULL);
  list_add_list (&rlc_p->pdus_to_mac_layer, &data_req.data);
  data_req.buffer_occupancy_in_bytes = rlc_p->buffer_occupancy;
  data_req.buffer_occupancy_in_pdus = data_req.buffer_occupancy_in_bytes / rlc_p->rlc_pdu_size;
  data_req.rlc_info.rlc_protocol_state = rlc_p->protocol_state;

  if (data_req.data.nb_elements > 0) {
    LOG_D(RLC, PROTOCOL_RLC_TM_CTXT_FMT" MAC_DATA_REQUEST %d TBs\n",
          PROTOCOL_RLC_TM_CTXT_ARGS(ctxt_pP, rlc_p),
          data_req.data.nb_elements);
  }

  return data_req;
}

//-----------------------------------------------------------------------------
void
rlc_tm_mac_data_indication (
  const protocol_ctxt_t* const  ctxt_pP,
  void * const        rlc_pP,
  struct mac_data_ind data_indP)
{

  if (data_indP.data.nb_elements > 0) {
    LOG_D(RLC, PROTOCOL_RLC_TM_CTXT_FMT" MAC_DATA_IND %d TBs\n",
          PROTOCOL_RLC_TM_CTXT_ARGS(ctxt_pP, ((rlc_tm_entity_t*) rlc_pP)),
          data_indP.data.nb_elements);
  }

  rlc_tm_rx (ctxt_pP, rlc_pP, data_indP);
}

//-----------------------------------------------------------------------------
void
rlc_tm_data_req (
  const protocol_ctxt_t* const  ctxt_pP,
  void *const rlc_pP,
  mem_block_t *const sdu_pP)
{
  rlc_tm_entity_t *rlc_p = (rlc_tm_entity_t *) rlc_pP;

#if DEBUG_RLC_TM_DATA_REQUEST
  LOG_D (RLC, PROTOCOL_RLC_TM_CTXT_FMT" RLC_TM_DATA_REQ size %d Bytes, BO %ld , NB SDU %d current_sdu_index=%d next_sdu_index=%d\n",
         PROTOCOL_RLC_TM_CTXT_ARGS(ctxt_pP, rlc_p),
         ((struct rlc_um_data_req *) (sdu_pP->data))->data_size,
         rlc_p->buffer_occupancy,
         rlc_p->nb_sdu,
         rlc_p->current_sdu_index,
         rlc_p->next_sdu_index);
#endif

  // not in 3GPP specification but the buffer may be full if not correctly configured
  if (rlc_p->input_sdus[rlc_p->next_sdu_index] == NULL) {
    ((struct rlc_tm_tx_sdu_management *) (sdu_pP->data))->sdu_size = ((struct rlc_tm_data_req *) (sdu_pP->data))->data_size;
    rlc_p->buffer_occupancy += ((struct rlc_tm_tx_sdu_management *) (sdu_pP->data))->sdu_size >> 3;
    rlc_p->nb_sdu += 1;
    ((struct rlc_tm_tx_sdu_management *) (sdu_pP->data))->first_byte = (uint8_t*)&sdu_pP->data[sizeof (struct rlc_tm_data_req_alloc)];
    rlc_p->input_sdus[rlc_p->next_sdu_index] = sdu_pP;
    rlc_p->next_sdu_index = (rlc_p->next_sdu_index + 1) % rlc_p->size_input_sdus_buffer;
  } else {
    free_mem_block (sdu_pP, __func__);
  }
}
