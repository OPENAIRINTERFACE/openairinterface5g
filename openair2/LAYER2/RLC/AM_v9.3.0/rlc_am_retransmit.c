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

#define RLC_AM_MODULE 1
#define RLC_AM_RETRANSMIT_C 1
//-----------------------------------------------------------------------------
//#include "rtos_header.h"
//-----------------------------------------------------------------------------
#include "rlc_am.h"
#include "rlc.h"
#include "LAYER2/MAC/extern.h"
#include "UTIL/LOG/log.h"
#include "msc.h"
//-----------------------------------------------------------------------------
boolean_t rlc_am_nack_pdu (
  const protocol_ctxt_t* const  ctxt_pP,
  rlc_am_entity_t *const rlc_pP,
  const rlc_sn_t snP,
  const rlc_sn_t prev_nack_snP,
  sdu_size_t so_startP,
  sdu_size_t so_endP)
{
  // 5.2.1 Retransmission
  // ...
  // When an AMD PDU or a portion of an AMD PDU is considered for retransmission, the transmitting side of the AM
  // RLC entity shall:
  //     - if the AMD PDU is considered for retransmission for the first time:
  //         - set the RETX_COUNT associated with the AMD PDU to zero;
  //     - else, if it (the AMD PDU or the portion of the AMD PDU that is considered for retransmission) is not pending
  //            for retransmission already, or a portion of it is not pending for retransmission already:
  //         - increment the RETX_COUNT;
  //     - if RETX_COUNT = maxRetxThreshold:
  //         - indicate to upper layers that max retransmission has been reached.


  mem_block_t* mb_p         = rlc_pP->tx_data_pdu_buffer[snP % RLC_AM_WINDOW_SIZE].mem_block;
  rlc_am_tx_data_pdu_management_t *tx_data_pdu_buffer_p = &rlc_pP->tx_data_pdu_buffer[snP % RLC_AM_WINDOW_SIZE];
  //int          pdu_sdu_index;
  //int          sdu_index;
  boolean_t status = TRUE;
  boolean_t retx_count_increment = FALSE;
  sdu_size_t pdu_data_to_retx = 0;

  if (mb_p != NULL) {
    assert(so_startP <= so_endP);

    // Handle full PDU NACK first
    if ((so_startP == 0) && (so_endP == 0x7FFF)) {
    	if ((prev_nack_snP != snP) && (tx_data_pdu_buffer_p->flags.ack == 0) && (tx_data_pdu_buffer_p->flags.max_retransmit == 0)) {
    		pdu_data_to_retx = tx_data_pdu_buffer_p->payload_size;
            /* Increment VtReTxNext if this is the first NACK or if some segments have already been transmitted */
            if ((tx_data_pdu_buffer_p->flags.retransmit == 0) || (tx_data_pdu_buffer_p->nack_so_start))
            {
            	retx_count_increment = TRUE;
            }

            tx_data_pdu_buffer_p->nack_so_start = 0;
            tx_data_pdu_buffer_p->num_holes     = 0;
            tx_data_pdu_buffer_p->retx_hole_index = 0;
            tx_data_pdu_buffer_p->nack_so_stop  = tx_data_pdu_buffer_p->payload_size - 1;
        #if TRACE_RLC_AM_HOLE
            LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[HOLE] SN %04d GLOBAL NACK 0->%05d\n",
                  PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
                  snP,
                  so_stopP);
        #endif
            assert(tx_data_pdu_buffer_p->nack_so_start < tx_data_pdu_buffer_p->payload_size);
    	}
    	else {
    		status = FALSE;
    	}
      }
    else if (tx_data_pdu_buffer_p->flags.max_retransmit == 0) {
    	// Handle Segment offset
		if (so_endP == 0x7FFF) {
			so_endP = tx_data_pdu_buffer_p->payload_size - 1;
		}

		// Check consistency
		if ((so_startP <= so_endP) && (so_endP < tx_data_pdu_buffer_p->payload_size)) {
	    	if (prev_nack_snP != snP) {
	    		/* New NACK_SN with SO */
                /* check whether a new segment is to be placed in Retransmission Buffer, then increment vrReTx */
                if ((tx_data_pdu_buffer_p->flags.retransmit == 0) || (so_startP < tx_data_pdu_buffer_p->nack_so_start)) {
                	retx_count_increment = TRUE;
                }

	            tx_data_pdu_buffer_p->num_holes     = 1;
	            tx_data_pdu_buffer_p->retx_hole_index = 0;
	            tx_data_pdu_buffer_p->hole_so_start[0] = so_startP;
	            tx_data_pdu_buffer_p->hole_so_stop[0] = so_endP;
	            tx_data_pdu_buffer_p->nack_so_start = so_startP;
	            tx_data_pdu_buffer_p->nack_so_stop = so_endP;
	            pdu_data_to_retx = so_endP - so_startP + 1;

	    	}
	    	else if ((tx_data_pdu_buffer_p->num_holes) && (tx_data_pdu_buffer_p->num_holes < RLC_AM_MAX_HOLES_REPORT_PER_PDU)) {
	    		/* New SOStart/SOEnd for the same NACK_SN than before */
	    		/* check discontinuity */
	    		if (so_startP > tx_data_pdu_buffer_p->hole_so_stop[tx_data_pdu_buffer_p->num_holes - 1]) {
		            tx_data_pdu_buffer_p->hole_so_start[tx_data_pdu_buffer_p->num_holes] = so_startP;
		            tx_data_pdu_buffer_p->hole_so_stop[tx_data_pdu_buffer_p->num_holes] = so_endP;
		            tx_data_pdu_buffer_p->nack_so_stop = so_endP;
		            tx_data_pdu_buffer_p->num_holes ++;
		            pdu_data_to_retx = so_endP - so_startP + 1;
	    		}
	    		else {
	    			status = FALSE;
	    		}
	    	}
	    	else {
	    		status = FALSE;
	    	}
		}
		else {
			status = FALSE;
		}
    }
    else {
    	status = FALSE;
    }

    if (status) {
    	tx_data_pdu_buffer_p->flags.nack = 1;
    	if ((retx_count_increment) && (tx_data_pdu_buffer_p->retx_count == tx_data_pdu_buffer_p->retx_count_next)) {
    		tx_data_pdu_buffer_p->retx_count_next ++;
    	}
    	if (tx_data_pdu_buffer_p->flags.retransmit == 1) {
            if (prev_nack_snP != snP) {
                /* if first process of this NACK_SN and data already pending for retx */
            	rlc_pP->retrans_num_bytes_to_retransmit += (pdu_data_to_retx - tx_data_pdu_buffer_p->retx_payload_size);
            	tx_data_pdu_buffer_p->retx_payload_size = pdu_data_to_retx;
            }
            else if (tx_data_pdu_buffer_p->num_holes > 1) {
                /* Segment case : SOStart and SOEnd already received for same NACK_SN */
                /* filter case where a NACK_SN is received twice with SO first time and no SO second time */
            	rlc_pP->retrans_num_bytes_to_retransmit += pdu_data_to_retx;
            	tx_data_pdu_buffer_p->retx_payload_size += pdu_data_to_retx;
            }
    	}
        else {
        	tx_data_pdu_buffer_p->flags.retransmit = 1;
        	rlc_pP->retrans_num_bytes_to_retransmit += pdu_data_to_retx;
        	tx_data_pdu_buffer_p->retx_payload_size = pdu_data_to_retx;
        	rlc_pP->retrans_num_pdus ++;
        }
    }

    /* TODO: Move this part in UL SCH processing */
#if 0
    if (rlc_pP->tx_data_pdu_buffer[snP].retx_count >= rlc_pP->max_retx_threshold) {
      for (pdu_sdu_index = 0; pdu_sdu_index < rlc_pP->tx_data_pdu_buffer[snP].nb_sdus; pdu_sdu_index++) {
        sdu_index = rlc_pP->tx_data_pdu_buffer[snP].sdus_index[pdu_sdu_index];
        assert(pdu_sdu_index < RLC_AM_MAX_SDU_IN_PDU);
        assert(sdu_index < RLC_AM_SDU_CONTROL_BUFFER_SIZE);
        rlc_pP->input_sdus[sdu_index].nb_pdus_ack += 1;

        if (rlc_pP->input_sdus[sdu_index].nb_pdus_ack == rlc_pP->input_sdus[sdu_index].nb_pdus) {
#if TEST_RLC_AM
          rlc_am_v9_3_0_test_data_conf (rlc_pP->module_id, rlc_pP->rb_id, rlc_pP->input_sdus[sdu_index].mui, RLC_SDU_CONFIRM_NO);
#else
          rlc_data_conf(ctxt_pP, rlc_pP->rb_id, rlc_pP->input_sdus[sdu_index].mui, RLC_SDU_CONFIRM_NO, rlc_pP->is_data_plane);
#endif
          rlc_pP->stat_tx_pdcp_sdu_discarded   += 1;
          rlc_pP->stat_tx_pdcp_bytes_discarded += rlc_pP->input_sdus[sdu_index].sdu_size;
          MSC_LOG_EVENT((ctxt_pP->enb_flag == ENB_FLAG_YES) ? MSC_RLC_ENB:MSC_RLC_UE,\
                        "0 "PROTOCOL_RLC_AM_MSC_FMT" Dropped SDU mui %u cause max_retx %u reached",\
                        PROTOCOL_RLC_AM_MSC_ARGS(ctxt_pP,rlc_pP),
                        rlc_pP->input_sdus[sdu_index].mui,
                        rlc_pP->max_retx_threshold);

          rlc_am_free_in_sdu(ctxt_pP, rlc_pP, sdu_index);

        }
      }
    }
#endif
  } else {
    LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[NACK-PDU] ERROR NACK MISSING PDU SN %05d\n",
          PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
          snP);
    status = FALSE;
  }

  return status;
}
//-----------------------------------------------------------------------------
void rlc_am_ack_pdu (
  const protocol_ctxt_t* const  ctxt_pP,
  rlc_am_entity_t *const rlc_pP,
  const rlc_sn_t snP,
  boolean_t free_pdu)
{
  mem_block_t* mb_p         = rlc_pP->tx_data_pdu_buffer[snP % RLC_AM_WINDOW_SIZE].mem_block;
  rlc_am_tx_data_pdu_management_t *tx_data_pdu_buffer = &rlc_pP->tx_data_pdu_buffer[snP % RLC_AM_WINDOW_SIZE];

  tx_data_pdu_buffer->flags.retransmit = 0;

  if (mb_p != NULL) {
    if (free_pdu) {
    free_mem_block(mb_p, __func__);
    tx_data_pdu_buffer->mem_block = NULL;
    LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[ACK-PDU] ACK PDU SN %05d previous retx_count %d \n",
          PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
          snP,
		  tx_data_pdu_buffer->retx_count);
    }

    if (tx_data_pdu_buffer->retx_payload_size) {
    	AssertFatal (tx_data_pdu_buffer->flags.ack == 0,
    			"RLC AM Rx Status Report sn=%d acked twice but is pending for Retx vtA=%d vtS=%d LcId=%d\n",
				snP, rlc_pP->vt_a,rlc_pP->vt_s,rlc_pP->channel_id);
      rlc_pP->retrans_num_bytes_to_retransmit -= tx_data_pdu_buffer->retx_payload_size;
      tx_data_pdu_buffer->retx_payload_size = 0;
      tx_data_pdu_buffer->num_holes = 0;
      rlc_pP->retrans_num_pdus --;
    }

  } else {
    LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[ACK-PDU] WARNING ACK PDU SN %05d -> NO PDU TO ACK\n",
          PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
          snP);

    if (mb_p != NULL) {
      free_mem_block(mb_p, __func__);
      tx_data_pdu_buffer->mem_block = NULL;
    }
  }
  tx_data_pdu_buffer->flags.ack = 1;
  tx_data_pdu_buffer->flags.transmitted = 0;
  tx_data_pdu_buffer->flags.retransmit = 0;

}
//-----------------------------------------------------------------------------
mem_block_t* rlc_am_retransmit_get_copy (
  const protocol_ctxt_t* const  ctxt_pP,
  rlc_am_entity_t *const rlc_pP,
  const rlc_sn_t snP)
{
  mem_block_t* mb_original_p = rlc_pP->tx_data_pdu_buffer[snP % RLC_AM_WINDOW_SIZE].mem_block;

  AssertFatal (mb_original_p != NULL, "RLC AM PDU Copy Error: Empty block sn=%d vtA=%d vtS=%d LcId=%d !\n",
		  snP,rlc_pP->vt_a,rlc_pP->vt_s,rlc_pP->channel_id);

  rlc_am_tx_data_pdu_management_t *pdu_mngt = &rlc_pP->tx_data_pdu_buffer[snP % RLC_AM_WINDOW_SIZE];

  /* We need to allocate a new buffer and copy to it because header content may change for Polling bit */
  int size             = pdu_mngt->header_and_payload_size + sizeof(struct mac_tb_req);
  mem_block_t* mb_copy = get_free_mem_block(size, __func__);
  memcpy(mb_copy->data, mb_original_p->data, size);

  rlc_am_pdu_sn_10_t *pdu_p                         = (rlc_am_pdu_sn_10_t*) (&mb_copy->data[sizeof(struct mac_tb_req)]);
  ((struct mac_tb_req*)(mb_copy->data))->data_ptr = (uint8_t*)pdu_p;

  return mb_copy;
}

//-----------------------------------------------------------------------------
mem_block_t* rlc_am_retransmit_get_am_segment(
  const protocol_ctxt_t* const  ctxt_pP,
  rlc_am_entity_t *const rlc_pP,
  rlc_am_tx_data_pdu_management_t *const pdu_mngt,
  sdu_size_t * const payload_sizeP /* in-out*/)
{
	int16_t          sdus_segment_size[RLC_AM_MAX_SDU_IN_PDU];
	mem_block_t*   mb_original_p  = pdu_mngt->mem_block;
	mem_block_t*   mem_pdu_segment_p = NULL;
	uint8_t              *pdu_original_header_p        = NULL;
	uint8_t              *pdu_segment_header_p        = NULL;
	sdu_size_t     retx_so_start,retx_so_stop; //starting and ending SO for retransmission in this PDU
	rlc_sn_t sn = pdu_mngt->sn;
	uint16_t	header_so_part;
	boolean_t fi_start, fi_end;
	uint8_t sdu_index = 0;
	uint8_t sdu_segment_index = 0;
	uint8_t num_LIs_pdu_segment = pdu_mngt->nb_sdus - 1;
	uint8_t li_bit_offset = 4; /* toggle between 0 and 4 */
	uint8_t li_jump_offset = 1; /* toggle between 1 and 2 */


	AssertFatal (mb_original_p != NULL, "RLC AM PDU Segment Error: Empty block sn=%d vtA=%d vtS=%d LcId=%d !\n",
			sn,rlc_pP->vt_a,rlc_pP->vt_s,rlc_pP->channel_id);


	AssertFatal (pdu_mngt->payload == mb_original_p->data + sizeof(struct mac_tb_req) + pdu_mngt->header_and_payload_size - pdu_mngt->payload_size,
			"RLC AM PDU Segment Error: Inconsistent data pointers p1=%p p2=%p sn = %d total size = %d data size = %d LcId=%d !\n",
			pdu_mngt->payload,mb_original_p->data + sizeof(struct mac_tb_req),pdu_mngt->header_and_payload_size,pdu_mngt->payload_size,sn,rlc_pP->channel_id);

	/* Init ReTx Hole list if not configured, ie the whole PDU has to be retransmitted */
	if (pdu_mngt->num_holes == 0)
	{
		AssertFatal (pdu_mngt->retx_payload_size == pdu_mngt->payload_size,"RLC AM PDU ReTx Segment: Expecting full PDU size ReTxSize=%d DataSize=%d sn=%d vtA=%d vtS=%d LcId=%d !\n",
				pdu_mngt->retx_payload_size,pdu_mngt->payload_size,sn,rlc_pP->vt_a,rlc_pP->vt_s,rlc_pP->channel_id);
		pdu_mngt->retx_hole_index = 0;
		pdu_mngt->hole_so_start[0] = 0;
		pdu_mngt->hole_so_stop[0] = pdu_mngt->payload_size - 1;
		pdu_mngt->num_holes = 1;
	}

	/* Init SO Start and SO Stop */
	retx_so_start = pdu_mngt->hole_so_start[pdu_mngt->retx_hole_index];
	retx_so_stop = pdu_mngt->hole_so_stop[pdu_mngt->retx_hole_index];

	AssertFatal ((retx_so_start <= retx_so_stop) && (retx_so_stop - retx_so_start + 1 <= pdu_mngt->payload_size),
			"RLC AM Tx PDU Segment Data SO Error: retx_so_start=%d retx_so_stop=%d OriginalPDUDataLength=%d sn=%d LcId=%d!\n",
			retx_so_start,retx_so_stop,pdu_mngt->payload_size,sn,rlc_pP->channel_id);

	/* Init FI to the same value as original PDU */
	fi_start = (!(RLC_AM_PDU_GET_FI_START(*(pdu_mngt->first_byte))));
	fi_end = (!(RLC_AM_PDU_GET_FI_END(*(pdu_mngt->first_byte))));

	/* Handle no LI case first */
	if (num_LIs_pdu_segment == 0)
	{
		/* Bound retx_so_stop to available TBS */
		if (retx_so_stop - retx_so_start + 1 + RLC_AM_PDU_SEGMENT_HEADER_MIN_SIZE > rlc_pP->nb_bytes_requested_by_mac)
		{
			retx_so_stop = retx_so_start + rlc_pP->nb_bytes_requested_by_mac - RLC_AM_PDU_SEGMENT_HEADER_MIN_SIZE - 1;
		}

		*payload_sizeP = retx_so_stop - retx_so_start + 1;

		mem_pdu_segment_p = get_free_mem_block((*payload_sizeP + RLC_AM_PDU_SEGMENT_HEADER_MIN_SIZE + sizeof(struct mac_tb_req)), __func__);
		pdu_segment_header_p        = (uint8_t *)&mem_pdu_segment_p->data[sizeof(struct mac_tb_req)];
		((struct mac_tb_req*)(mem_pdu_segment_p->data))->data_ptr = pdu_segment_header_p;
		((struct mac_tb_req*)(mem_pdu_segment_p->data))->tb_size = RLC_AM_PDU_SEGMENT_HEADER_MIN_SIZE + *payload_sizeP;

		/* clear all PDU segment */
		memset(pdu_segment_header_p, 0, *payload_sizeP + RLC_AM_PDU_SEGMENT_HEADER_MIN_SIZE);
		/* copy data part */
		memcpy(pdu_segment_header_p + RLC_AM_PDU_SEGMENT_HEADER_MIN_SIZE, pdu_mngt->payload + retx_so_start, *payload_sizeP);

		/* Set FI part to false if SO Start and SO End are different from PDU boundaries */
		if (retx_so_start)
		{
			fi_start = FALSE;
		}
		if (retx_so_stop < pdu_mngt->payload_size - 1)
		{
			fi_end = FALSE;
		}

		/* Header content is filled at the end */
	}
	else
	{
		/* Step 1 */
		/* Find the SDU index in the original PDU containing retx_so_start */
		sdu_size_t sdu_size = 0;
		sdu_size_t data_size = 0;
		*payload_sizeP = 0;
		sdu_size_t header_segment_length = RLC_AM_PDU_SEGMENT_HEADER_MIN_SIZE;
		pdu_original_header_p = pdu_mngt->first_byte + 2;
		li_bit_offset = 4; /* toggle between 0 and 4 */
		li_jump_offset = 1; /* toggle between 1 and 2 */
		uint16_t temp_read = ((*pdu_original_header_p) << 8) | (*(pdu_original_header_p + 1));


		/* Read first LI */
		sdu_size = RLC_AM_PDU_GET_LI(temp_read,li_bit_offset);
		pdu_original_header_p += li_jump_offset;
		li_bit_offset ^= 0x4;
		li_jump_offset ^= 0x3;
		data_size += sdu_size;
		sdu_index = 1;

		/* Loop on all original LIs */
		while ((data_size < retx_so_start + 1) && (sdu_index < pdu_mngt->nb_sdus))
		{
			if (sdu_index < pdu_mngt->nb_sdus - 1)
			{
				temp_read = ((*pdu_original_header_p) << 8) | (*(pdu_original_header_p + 1));
				sdu_size = RLC_AM_PDU_GET_LI(temp_read,li_bit_offset);
				pdu_original_header_p += li_jump_offset;
				li_bit_offset ^= 0x4;
				li_jump_offset ^= 0x3;
				data_size += sdu_size;
			}
			else
			{
				/* if retx_so_start is still not included then set data_size with full original PDU data size */
				/* Set fi_start to FALSE in this case */
				data_size = pdu_mngt->payload_size;
			}
			sdu_index ++;
		}

		if (retx_so_start == data_size)
		{
			/* Set FI Start if retx_so_start = cumulated data size */
			fi_start = TRUE;
			/* there must be at least one SDU more */
			AssertFatal (sdu_index < pdu_mngt->nb_sdus, "RLC AM Tx PDU Segment Error: sdu_index=%d nb_sdus=%d sn=%d LcId=%d !\n",
					sdu_index,pdu_mngt->nb_sdus,sn,rlc_pP->channel_id);
			if (sdu_index < pdu_mngt->nb_sdus - 1)
			{
				temp_read = ((*pdu_original_header_p) << 8) | (*(pdu_original_header_p + 1));
				sdu_size = RLC_AM_PDU_GET_LI(temp_read,li_bit_offset);
				pdu_original_header_p += li_jump_offset;
				li_bit_offset ^= 0x4;
				li_jump_offset ^= 0x3;
				data_size += sdu_size;
			}
			else
			{
				/* It was the last LI, then set data_size to full original PDU size */
				data_size = pdu_mngt->payload_size;
			}
			/* Go to next SDU */
			sdu_index ++;
		}
		else if (retx_so_start != 0)
		{
			/* in all other cases set fi_start to FALSE if it SO Start is not 0 */
			fi_start = FALSE;
		}

		/* Set first SDU portion of the segment */
		sdus_segment_size[0] = data_size - retx_so_start;

		/* Check if so end is in the first SDU portion */
		if (sdus_segment_size[0] >= retx_so_stop - retx_so_start + 1)
		{
			sdus_segment_size[0] = retx_so_stop - retx_so_start + 1;
			*payload_sizeP = sdus_segment_size[0];
			num_LIs_pdu_segment = 0;
		}

		/* Bound first SDU segment to available TBS if necessary */
		if (sdus_segment_size[0] + RLC_AM_PDU_SEGMENT_HEADER_MIN_SIZE >= rlc_pP->nb_bytes_requested_by_mac)
		{
			sdus_segment_size[0] = rlc_pP->nb_bytes_requested_by_mac - RLC_AM_PDU_SEGMENT_HEADER_MIN_SIZE;
			*payload_sizeP = sdus_segment_size[0];
			num_LIs_pdu_segment = 0;
		}


		/* Now look for the end if it was not set previously */
		if (*payload_sizeP == 0)
		{
			sdu_segment_index ++;
			while ((sdu_index < pdu_mngt->nb_sdus) && (data_size < retx_so_stop + 1))
			{
				if (sdu_index < pdu_mngt->nb_sdus - 1)
				{
					temp_read = ((*pdu_original_header_p) << 8) | (*(pdu_original_header_p + 1));
					sdu_size = RLC_AM_PDU_GET_LI(temp_read,li_bit_offset);
					pdu_original_header_p += li_jump_offset;
					li_bit_offset ^= 0x4;
					li_jump_offset ^= 0x3;
					data_size += sdu_size;
				}
				else
				{
					sdu_size = pdu_mngt->payload_size - data_size;
					data_size = pdu_mngt->payload_size;
				}

				sdus_segment_size[sdu_segment_index] = sdu_size;
				sdu_index ++;
				sdu_segment_index ++;
			}


			if (data_size > retx_so_stop + 1)
			{
				sdus_segment_size[sdu_segment_index - 1] = retx_so_stop - (data_size - sdu_size) + 1;
			}

			/* Set number of LIs in the segment */
			num_LIs_pdu_segment = sdu_segment_index - 1;

			AssertFatal (num_LIs_pdu_segment <=  pdu_mngt->nb_sdus - 1, "RLC AM Tx PDU Segment Data Error: nbLISegment=%d nbLIPDU=%d sn=%d LcId=%d !\n",
					num_LIs_pdu_segment,pdu_mngt->nb_sdus - 1,sn,rlc_pP->channel_id);

			/* Bound to available TBS taking into account min PDU segment header*/
			sdu_segment_index = 0;
			while ((sdu_segment_index < num_LIs_pdu_segment + 1) && (rlc_pP->nb_bytes_requested_by_mac > *payload_sizeP + RLC_AM_PDU_SEGMENT_HEADER_SIZE(sdu_segment_index)))
			{
				AssertFatal (sdus_segment_size[sdu_segment_index] > 0, "RLC AM Tx PDU Segment Data Error: EMpty LI index=%d numLISegment=%d numLIPDU=%d PDULength=%d SOStart=%d SOStop=%d sn=%d LcId=%d !\n",
						sdu_segment_index,num_LIs_pdu_segment,pdu_mngt->nb_sdus - 1,pdu_mngt->payload_size,retx_so_start,retx_so_stop,sn,rlc_pP->channel_id);

				/* Add next sdu_segment_index to data part */
				if (RLC_AM_PDU_SEGMENT_HEADER_SIZE(sdu_segment_index) + (*payload_sizeP) + sdus_segment_size[sdu_segment_index] <= rlc_pP->nb_bytes_requested_by_mac)
				{
					(*payload_sizeP) += sdus_segment_size[sdu_segment_index];
				}
				else
				{
					/* bound to available TBS size */
					sdus_segment_size[sdu_segment_index] = rlc_pP->nb_bytes_requested_by_mac - RLC_AM_PDU_SEGMENT_HEADER_SIZE(sdu_segment_index) - (*payload_sizeP);
					(*payload_sizeP) += sdus_segment_size[sdu_segment_index];
				}
				header_segment_length = RLC_AM_PDU_SEGMENT_HEADER_SIZE(sdu_segment_index);
				sdu_segment_index ++;
			}

			num_LIs_pdu_segment = sdu_segment_index - 1;
		}


		/* update retx_so_stop */
		retx_so_stop = retx_so_start + (*payload_sizeP) - 1;

		AssertFatal ((retx_so_stop <= pdu_mngt->payload_size - 1) && (retx_so_stop - retx_so_start + 1 < pdu_mngt->payload_size),
				"RLC AM Tx PDU Segment Data Error: retx_so_stop=%d OriginalPDUDataLength=%d SOStart=%d SegmentLength=%d numLISegment=%d numLIPDU=%d sn=%d LcId=%d !\n",
				retx_so_stop,pdu_mngt->payload_size,retx_so_start,*payload_sizeP,num_LIs_pdu_segment,pdu_mngt->nb_sdus - 1,sn,rlc_pP->channel_id);

		/* init FI End to FALSE if retx_so_stop is not end of PDU */
		if (retx_so_stop != pdu_mngt->payload_size - 1)
		{
			fi_end = FALSE;
		}

		/* Check consistency between sdus_segment_size and payload_sizeP */
		/* And Set FI End if retx_so_stop = cumulated data size and this is not last SDU */
		data_size = 0;
		for (int i = 0; i < num_LIs_pdu_segment + 1; i++)
		{
			data_size += sdus_segment_size[i];
			if ((retx_so_stop == data_size - 1) && (i < num_LIs_pdu_segment))
			{
				fi_end = TRUE;
			}
		}

		AssertFatal (data_size == *payload_sizeP, "RLC AM Tx PDU Segment Data Error: SduSum=%d Data=%d sn=%d LcId=%d !\n",
				data_size,*payload_sizeP,sn,rlc_pP->channel_id);



		/* Allocation */
		AssertFatal (header_segment_length + *payload_sizeP <= pdu_mngt->header_and_payload_size + 2, "RLC AM PDU Segment Error: Hdr=%d Data=%d Original Hdr+Data =%d sn=%d LcId=%d !\n",
				header_segment_length,*payload_sizeP,pdu_mngt->header_and_payload_size,sn,rlc_pP->channel_id);
		mem_pdu_segment_p = get_free_mem_block((*payload_sizeP + header_segment_length + sizeof(struct mac_tb_req)), __func__);
		pdu_segment_header_p        = (uint8_t *)&mem_pdu_segment_p->data[sizeof(struct mac_tb_req)];
		((struct mac_tb_req*)(mem_pdu_segment_p->data))->data_ptr = pdu_segment_header_p;
		((struct mac_tb_req*)(mem_pdu_segment_p->data))->tb_size = header_segment_length + *payload_sizeP;

		/* clear all PDU segment */
		memset(pdu_segment_header_p, 0, *payload_sizeP + header_segment_length);
		/* copy data part */
		memcpy(pdu_segment_header_p + header_segment_length, pdu_mngt->payload + retx_so_start, *payload_sizeP);
	}

	/* Last step : update contexts and fill PDU Segment Header */
	if (mem_pdu_segment_p != NULL)
	{
		/* Update PDU Segment contexts */
		if (*payload_sizeP == pdu_mngt->hole_so_stop[pdu_mngt->retx_hole_index] - pdu_mngt->hole_so_start[pdu_mngt->retx_hole_index] + 1)
		{
			/* All data in the segment are transmitted : switch to next one */
			pdu_mngt->retx_hole_index ++;
			if (pdu_mngt->retx_hole_index < pdu_mngt->num_holes)
			{
				/* Set min SOStart to the value of next hole : assumption is holes are ordered by increasing SOStart */
				pdu_mngt->nack_so_start = pdu_mngt->hole_so_start[pdu_mngt->retx_hole_index];
			}
			else
			{
				/* no more scheduled Retx: reset values */
				/* Retx size is reset in the calling function */
				pdu_mngt->num_holes = 0;
				pdu_mngt->retx_hole_index = 0;
				pdu_mngt->nack_so_start = 0;
			}
		}
		else
		{
			/* not all segment data could be transmitted, just update SoStart */
			pdu_mngt->hole_so_start[pdu_mngt->retx_hole_index] += (*payload_sizeP);
			pdu_mngt->nack_so_start = pdu_mngt->hole_so_start[pdu_mngt->retx_hole_index];
		}

		/* Content is supposed to be init with 0 so with FIStart=FIEnd=TRUE */
		RLC_AM_PDU_SET_D_C(*pdu_segment_header_p);
		RLC_AM_PDU_SET_RF(*pdu_segment_header_p);
		/* Change FI */
		if (!fi_start)
		{
			// Set to not starting
			(*pdu_segment_header_p) |= (1 << (RLC_AM_PDU_FI_OFFSET + 1));
		}
		if (!fi_end)
		{
			// Set to not starting
			(*pdu_segment_header_p) |= (1 << (RLC_AM_PDU_FI_OFFSET));
		}
		/* Set SN */
		(*pdu_segment_header_p) |= ((sn >> 8) & 0x3);
		(*(pdu_segment_header_p + 1)) |= (sn & 0xFF);

		/* Segment Offset */
		header_so_part = retx_so_start;

		/* Last Segment Flag (LSF) */
		if (retx_so_stop == pdu_mngt->payload_size - 1)
		{
			RLC_AM_PDU_SET_LSF(header_so_part);
		}

		/* Store SO bytes */
		* (pdu_segment_header_p + 2)  = (header_so_part >> 8) & 0xFF;
		* (pdu_segment_header_p + 3)  = header_so_part & 0xFF;

		/* Fill LI part */
		if (num_LIs_pdu_segment)
		{
			uint16_t index = 0;
			uint16_t temp = 0;
			/* Set Extension bit in first byte */
			RLC_AM_PDU_SET_E(*pdu_segment_header_p);

			/* loop on nb of LIs */
			pdu_segment_header_p += RLC_AM_PDU_SEGMENT_HEADER_MIN_SIZE;
			li_bit_offset = 4; /* toggle between 0 and 4 */
			li_jump_offset = 1; /* toggle between 1 and 2 */

			while (index < num_LIs_pdu_segment)
			{
				/* Set E bit for next LI if present */
				if (index < num_LIs_pdu_segment - 1)
					RLC_SET_BIT(temp,li_bit_offset + RLC_AM_LI_BITS);
				/* Set LI */
				RLC_AM_PDU_SET_LI(temp,sdus_segment_size[index],li_bit_offset);
				*pdu_segment_header_p = temp >> 8;
				*(pdu_segment_header_p + 1) = temp & 0xFF;
				pdu_segment_header_p += li_jump_offset;
				li_bit_offset ^= 0x4;
				li_jump_offset ^= 0x3;

				temp = ((*pdu_segment_header_p) << 8) | (*(pdu_segment_header_p + 1));
				index ++;
			}
		}
	}
	else
	{
		LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[RE-SEGMENT] OUT OF MEMORY PDU SN %04d\n",
		              PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
					  sn);
	}

	return mem_pdu_segment_p;
}

#if 0
//-----------------------------------------------------------------------------
mem_block_t* rlc_am_retransmit_get_subsegment(
  const protocol_ctxt_t* const  ctxt_pP,
  rlc_am_entity_t *const rlc_pP,
  const rlc_sn_t snP,
  sdu_size_t * const sizeP /* in-out*/)
{

  // 5.2 ARQ procedures
  // ARQ procedures are only performed by an AM RLC entity.
  // 5.2.1 Retransmission
  // The transmitting side of an AM RLC entity can receive a negative acknowledgement (notification of reception failure
  // by its peer AM RLC entity) for an AMD PDU or a portion of an AMD PDU by the following:
  // - STATUS PDU from its peer AM RLC entity.
  //
  // When receiving a negative acknowledgement for an AMD PDU or a portion of an AMD PDU by a STATUS PDU from
  // its peer AM RLC entity, the transmitting side of the AM RLC entity shall:
  //     - if the SN of the corresponding AMD PDU falls within the range VT(A) <= SN < VT(S):
  //         - consider the AMD PDU or the portion of the AMD PDU for which a negative acknowledgement was
  //           received for retransmission.
  //
  // When an AMD PDU or a portion of an AMD PDU is considered for retransmission, the transmitting side of the AM
  // RLC entity shall:
  //     - if the AMD PDU is considered for retransmission for the first time:
  //         - set the RETX_COUNT associated with the AMD PDU to zero;
  //     - else, if it (the AMD PDU or the portion of the AMD PDU that is considered for retransmission) is not pending
  //       for retransmission already, or a portion of it is not pending for retransmission already:
  //         - increment the RETX_COUNT;
  //     - if RETX_COUNT = maxRetxThreshold:
  //         - indicate to upper layers that max retransmission has been reached.
  //
  // When retransmitting an AMD PDU, the transmitting side of an AM RLC entity shall:
  //     - if the AMD PDU can entirely fit within the total size of RLC PDU(s) indicated by lower layer at the particular
  //       transmission opportunity:
  //         - deliver the AMD PDU as it is except for the P field (the P field should be set according to sub clause 5.2.2) to
  //           lower layer;
  //     - otherwise:
  //         - segment the AMD PDU, form a new AMD PDU segment which will fit within the total size of RLC PDU(s)
  //           indicated by lower layer at the particular transmission opportunity and deliver the new AMD PDU segment
  //           to lower layer.
  //
  // When retransmitting a portion of an AMD PDU, the transmitting side of an AM RLC entity shall:
  //    - segment the portion of the AMD PDU as necessary, form a new AMD PDU segment which will fit within the
  //    total size of RLC PDU(s) indicated by lower layer at the particular transmission opportunity and deliver the new
  //    AMD PDU segment to lower layer.
  //
  // When forming a new AMD PDU segment, the transmitting side of an AM RLC entity shall:
  //    - only map the Data field of the original AMD PDU to the Data field of the new AMD PDU segment;
  //    - set the header of the new AMD PDU segment in accordance with the description in sub clause 6.;
  //    - set the P field according to sub clause 5.2.2.

  mem_block_t*           mb_original_p     = rlc_pP->tx_data_pdu_buffer[snP].mem_block;

  if (mb_original_p != NULL) {
    mem_block_t*           mb_sub_segment_p  = get_free_mem_block(*sizeP + sizeof(struct mac_tb_req), __func__);
    rlc_am_pdu_sn_10_t*    pdu_original_p    = (rlc_am_pdu_sn_10_t*) (&mb_original_p->data[sizeof(struct mac_tb_req)]);
    rlc_am_pdu_sn_10_t*    pdu_sub_segment_p = (rlc_am_pdu_sn_10_t*) (&mb_sub_segment_p->data[sizeof(struct mac_tb_req)]);
    rlc_am_pdu_info_t      pdu_info;
    int                    max_copy_payload_size;
    //LG avoid WARNING int                    test_max_copy_payload_size;
    int                    test_pdu_copy_size          = 0;

    ((struct mac_tb_req*)(mb_sub_segment_p->data))->data_ptr         = (uint8_t*)&(mb_sub_segment_p->data[sizeof(struct mac_tb_req)]);

    if (rlc_am_get_data_pdu_infos(ctxt_pP, rlc_pP, pdu_original_p, rlc_pP->tx_data_pdu_buffer[snP].header_and_payload_size, &pdu_info) >= 0) {
      int li_index = 0;
      int start_offset       = rlc_pP->tx_data_pdu_buffer[snP].nack_so_start;
      int stop_offset        = rlc_pP->tx_data_pdu_buffer[snP].nack_so_stop;

      LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[RE-SEGMENT] ORIGINAL PDU SN %04d:\n",
            PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
            snP);
      rlc_am_display_data_pdu_infos(ctxt_pP, rlc_pP, &pdu_info);

      // all 15 bits set to 1 (indicate that the missing portion of the AMD PDU includes all bytes
      // to the last byte of the AMD PDU)
      if (stop_offset == 0x7FFF) {
        rlc_pP->tx_data_pdu_buffer[snP].nack_so_stop = rlc_pP->tx_data_pdu_buffer[snP].payload_size - 1;
        stop_offset = rlc_pP->tx_data_pdu_buffer[snP].nack_so_stop;
        LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[RE-SEGMENT] UPDATED RETRANS PDU SN %04d nack_so_stop FROM 0x7FFF to %05d\n",
              PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
              snP,
              stop_offset);
      }

      // FIXED PART AMD PDU SEGMENT HEADER
      // 6.2.1.5 AMD PDU segment
      // AMD PDU segment consists of a Data field and an AMD PDU segment header.
      //
      // AMD PDU segment header consists of a fixed part (fields that are present for every AMD PDU segment) and an
      // extension part (fields that are present for an AMD PDU segment when necessary). The fixed part of the AMD PDU
      // segment header itself is byte aligned and consists of a D/C, a RF, a P, a FI, an E, a SN, a LSF and a SO. The extension
      // part of the AMD PDU segment header itself is byte aligned and consists of E(s) and LI(s).
      //
      // An AMD PDU segment header consists of an extension part only when more than one Data field elements are present
      // in the AMD PDU segment, in which case an E and a LI are present for every Data field element except the last.
      // Furthermore, when an AMD PDU segment header consists of an odd number of LI(s), four padding bits follow after the
      // last LI.

      pdu_sub_segment_p->b1 = (pdu_original_p->b1 & 0x83) | 0x40;
      pdu_sub_segment_p->b2 = pdu_original_p->b2;
      pdu_sub_segment_p->data[0] = ((uint8_t)(start_offset >> 8));
      pdu_sub_segment_p->data[1] = ((uint8_t)(start_offset & 0xFF));

      *sizeP = *sizeP - 4;


      int            continue_fill_pdu_with_pdu  = 1;
      int            test_pdu_remaining_size     = *sizeP;
      int            test_num_li                 = 0;
      int            fill_num_li                    ;
      int            test_li_sum                 = 0;
      int            not_test_fi                 = 0; // by default not 1st byte and not last byte af a SDU
      int            test_start_offset;
      unsigned int   test_li_length_in_bytes     = 1;
      int16_t          test_li_list[RLC_AM_MAX_SDU_IN_PDU];
      uint8_t*          fill_payload_p;
      //int            test_fi_last_byte_pdu_is_last_byte_sdu = 0;
      //int            test_fi_first_byte_pdu_is_first_byte_sdu = 0;

      rlc_am_e_li_t* e_li_sub_segment            = (rlc_am_e_li_t*)(&pdu_sub_segment_p->data[2]);
      //int            first_enter_in_start_offset_lt_li_sum = 1;
      int            not_fi_original                 = ((pdu_original_p->b1 & 0x18) >> 3) ^ 3;

      //-------------------------------------------------------
      // set MAX payload size that can be copied
      // first constraint : the size of the hole to retransmit
      rlc_am_get_next_hole(ctxt_pP, rlc_pP, snP, &start_offset, &stop_offset);
      max_copy_payload_size = stop_offset - start_offset + 1;
      assert(max_copy_payload_size > 0);
      assert(test_pdu_remaining_size > 0);
      LOG_T(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[RE-SEGMENT] HOLE FOUND SO %d -> %d\n",
            PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
            start_offset,
            stop_offset);
      LOG_T(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[RE-SEGMENT] ORIGINAL FI %d\n",
            PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
            (pdu_original_p->b1 & 0x18) >> 3);

      // second constraint the size of the pdu_p requested by MAC layer
      if (max_copy_payload_size > test_pdu_remaining_size) {
        LOG_T(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[RE-SEGMENT] CUT max_copy_payload_size with test_pdu_remaining_size %d -> %d\n",
              PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
              max_copy_payload_size,
              test_pdu_remaining_size);
        max_copy_payload_size = test_pdu_remaining_size;
      }


      if (start_offset == 0) {
        not_test_fi = (not_fi_original & 0x02);
      }

      test_start_offset = start_offset;

      //.find the li corresponding to the nack_so_start (start_offset)
      if (pdu_info.num_li > 0) {
        LOG_T(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[RE-SEGMENT] ORIGINAL NUM LI %d\n",
              PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
              pdu_info.num_li);

        while ((li_index < pdu_info.num_li) && (continue_fill_pdu_with_pdu)) {
          LOG_T(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[RE-SEGMENT] FIND LI %d\n",
                PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
                pdu_info.li_list[li_index]);

          if (max_copy_payload_size > test_pdu_remaining_size) {
            LOG_T(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[RE-SEGMENT] CUT max_copy_payload_size with test_pdu_remaining_size %d -> %d\n",
                  PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
                  max_copy_payload_size,
                  test_pdu_remaining_size);
            max_copy_payload_size = test_pdu_remaining_size;
          }

          assert(max_copy_payload_size >= 0);
          assert(test_pdu_remaining_size >= 0);

          test_li_sum               += pdu_info.li_list[li_index];

          //---------------------------------------------------------------
          if (test_start_offset < test_li_sum) {

            LOG_T(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[RE-SEGMENT] test_start_offset < test_li_sum  %d < %d\n",
                  PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
                  test_start_offset,
                  test_li_sum);
            /*if (test_max_copy_payload_size > (test_li_sum - test_start_offset)) {
                #if TRACE_RLC_AM_RESEGMENT
                LOG_D(RLC, "[FRAME %5u][%s][RLC_AM][MOD %u/%u][RB %u][RE-SEGMENT] CUT test_max_copy_payload_size with test_li_sum - test_start_offset %d -> %d\n",ctxt_pP->frame, rlc_pP->module_id, rlc_pP->rb_id,  test_max_copy_payload_size, test_li_sum - test_start_offset);
                #endif
                test_max_copy_payload_size = test_li_sum - test_start_offset;
            }*/

            if ((max_copy_payload_size + test_start_offset) < test_li_sum) {
              LOG_T(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[RE-SEGMENT] (max_copy_payload_size %d + test_start_offset %d) < test_li_sum %d\n",
                    PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
                    max_copy_payload_size,
                    test_start_offset,
                    test_li_sum);
              LOG_T(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[RE-SEGMENT] COPY SO %d -> %d  %d BYTES\n",
                    PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
                    test_start_offset ,
                    test_start_offset + max_copy_payload_size - 1,
                    max_copy_payload_size );
              assert(max_copy_payload_size > 0);
              continue_fill_pdu_with_pdu = 0;
              test_pdu_copy_size         = test_pdu_copy_size + max_copy_payload_size;
              test_start_offset          = test_start_offset + max_copy_payload_size;
              not_test_fi                = not_test_fi & 0x02;  // clear b0, last byte does not correspond to last byte of a SDU
              max_copy_payload_size      = 0;

            } else if ((max_copy_payload_size + test_start_offset) == test_li_sum) {
              LOG_T(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[RE-SEGMENT] (max_copy_payload_size + test_start_offset) == test_li_sum %d == %d\n",
                    PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
                    (max_copy_payload_size + test_start_offset) ,
                    test_li_sum);
              LOG_T(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[RE-SEGMENT] COPY SO %d -> %d  %d BYTES\n",
                    PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
                    test_start_offset ,
                    test_start_offset + max_copy_payload_size - 1,
                    max_copy_payload_size );
              assert(max_copy_payload_size > 0);
              continue_fill_pdu_with_pdu = 0;
              test_pdu_copy_size         = test_pdu_copy_size + max_copy_payload_size;
              test_start_offset          = test_start_offset + max_copy_payload_size;
              not_test_fi                = not_test_fi | 0x01;// set b0, last byte does correspond to last byte of a SDU
              max_copy_payload_size      = 0;

            } else if ((max_copy_payload_size + test_start_offset - (test_li_length_in_bytes ^ 3)) > test_li_sum) {
              LOG_T(RLC,
                    PROTOCOL_RLC_AM_CTXT_FMT"[RE-SEGMENT] (max_copy_payload_size + test_start_offset - (test_li_length_in_bytes ^ 3)) > test_li_sum %d > %d\n SET LI %d\n",
                    PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
                    (max_copy_payload_size + test_start_offset)  + (test_li_length_in_bytes ^ 3),
                    test_li_sum,
                    test_li_sum - test_start_offset);
              LOG_T(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[RE-SEGMENT] COPY SO %d -> %d  %d BYTES\n",
                    PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
                    test_start_offset ,
                    test_li_sum - 1,
                    test_li_sum - test_start_offset );
              assert((test_li_sum - test_start_offset) > 0);
              test_li_list[test_num_li++] = test_li_sum - test_start_offset;
              test_pdu_copy_size          = test_pdu_copy_size + test_li_sum - test_start_offset;
              test_li_length_in_bytes      = test_li_length_in_bytes ^ 3;

              test_pdu_remaining_size     = test_pdu_remaining_size - test_li_sum + test_start_offset -
                                            test_li_length_in_bytes;

              max_copy_payload_size       = max_copy_payload_size - test_li_sum + test_start_offset - test_li_length_in_bytes;
              LOG_T(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[RE-SEGMENT] NOW max_copy_payload_size %d BYTES test_start_offset %d\n",
                    PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
                    max_copy_payload_size,
                    test_li_sum);
              // normally the next while itereation will add bytes to PDU
              //not_test_fi = not_test_fi | 0x01;  // set b0, last byte does correspond to last byte of a SDU
              test_start_offset           = test_li_sum;

            } else {
              LOG_T(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[RE-SEGMENT] (test_max_copy_payload_size + test_start_offset ) > test_li_sum %d > %d\n NO REMAINING SIZE FOR LI",
                    PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
                    (max_copy_payload_size + test_start_offset),
                    test_li_sum);
              LOG_T(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[RE-SEGMENT] COPY SO %d -> %d  %d BYTES\n",
                    PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
                    test_start_offset ,
                    test_li_sum - 1,
                    test_li_sum - test_start_offset );
              assert((test_li_sum - test_start_offset) > 0);
              continue_fill_pdu_with_pdu = 0;
              test_pdu_copy_size         = test_pdu_copy_size + test_li_sum - test_start_offset;

              test_pdu_remaining_size    = test_pdu_remaining_size - test_li_sum + test_start_offset;

              max_copy_payload_size      = max_copy_payload_size - test_li_sum + test_start_offset;
              test_start_offset          = test_li_sum;
              not_test_fi                = not_test_fi | 0x01;  // set b0, last byte does correspond to last byte of a SDU
            }

            //---------------------------------------------------------------
            // start offset start at the begining of a SDU
            // and it cant be the first data field of the original PDU
          } else if (test_start_offset == test_li_sum) {
            LOG_T(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[RE-SEGMENT] (test_start_offset == test_li_sum) %d == %d\n",
                  PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
                  test_start_offset ,
                  test_li_sum);

            if ((test_num_li == 0) && (test_pdu_copy_size == 0)) {
              not_test_fi = not_test_fi | 0x02;  // set b1, first byte does correspond to first byte of a SDU
            }
          }

          li_index = li_index + 1;
        }

        if ((continue_fill_pdu_with_pdu > 0) &&
            (li_index    == pdu_info.num_li) &&
            (pdu_info.hidden_size       > 0) &&
            (test_pdu_remaining_size    > 0) &&
            (max_copy_payload_size      > 0) ) {

          if (max_copy_payload_size > test_pdu_remaining_size) {
            LOG_T(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[RE-SEGMENT] TRYING HIDDEN SIZE...CUT max_copy_payload_size with test_pdu_remaining_size %d -> %d\n",
                  PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
                  max_copy_payload_size,
                  test_pdu_remaining_size);
            max_copy_payload_size = test_pdu_remaining_size;
          }

          // remaining bytes to fill, redundant check, but ...
          if  ((max_copy_payload_size + test_start_offset) >= (pdu_info.hidden_size + test_li_sum)) {
            test_pdu_copy_size += (pdu_info.hidden_size  + test_li_sum - test_start_offset);
            LOG_T(RLC,
                  PROTOCOL_RLC_AM_CTXT_FMT"[RE-SEGMENT] COPYING WHOLE REMAINING SIZE %d (max_copy_payload_size %d, test_start_offset %d, pdu_info.hidden_size %d test_li_sum %d test_pdu_copy_size %d)\n",
                  PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
                  pdu_info.hidden_size  + test_li_sum - test_start_offset,
                  max_copy_payload_size,
                  test_start_offset,
                  pdu_info.hidden_size,
                  test_li_sum,
                  test_pdu_copy_size);
            test_start_offset   = pdu_info.hidden_size   + test_li_sum;
            not_test_fi = (not_test_fi & 0x2) | (not_fi_original & 0x1);  // set b0 idendical to the b0 of the non segmented PDU
          } else {
            LOG_T(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[RE-SEGMENT] COPYING REMAINING SIZE %d (/%d)\n",
                  PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
                  max_copy_payload_size,
                  pdu_info.hidden_size);
            test_pdu_copy_size += max_copy_payload_size;
            test_start_offset = test_start_offset + max_copy_payload_size;
            not_test_fi = not_test_fi & 0x2;  // clear b0 because no SDU ending in this PDU
          }
        }
      } else { // num_li == 0
        LOG_T(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[RE-SEGMENT] (num_li == 0)\n",
              PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP));
        test_pdu_copy_size = max_copy_payload_size;

        if ((stop_offset ==  (start_offset + max_copy_payload_size - 1)) && (stop_offset == rlc_pP->tx_data_pdu_buffer[snP].payload_size - 1)) {
          not_test_fi = (not_test_fi & 0x2) | (not_fi_original & 0x1);  // set b0 idendical to the b0 of the non segmented PDU
        } else {
          not_test_fi = not_test_fi & 0x2;  // clear b0 because no SDU ending in this PDU
        }
      }

      //---------------------------------------------------------------
      /*if (stop_offset == (rlc_pP->tx_data_pdu_buffer[snP].payload_size - 1)) {
              test_fi = (test_fi & 0x02) | (fi_original & 0x01);
      }*/
      //---------------------------------------------------------------
      // write FI field in header
      //---------------------------------------------------------------
      pdu_sub_segment_p->b1 = pdu_sub_segment_p->b1 | (((not_test_fi << 3) ^ 0x18) & 0x18);
      //---------------------------------------------------------------
      // fill the segment pdu_p with Lis and data
      //---------------------------------------------------------------
      LOG_T(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[RE-SEGMENT] fill the segment pdu_p with Lis and data, test_num_li %d\n",
            PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP), test_num_li);

      if (test_num_li > 0) {
        pdu_sub_segment_p->b1 = pdu_sub_segment_p->b1 | 0x04; // set E bit
        test_li_length_in_bytes = 1;

        for (fill_num_li=0; fill_num_li < test_num_li; fill_num_li++) {
          test_li_length_in_bytes = test_li_length_in_bytes ^ 3;

          if (test_li_length_in_bytes  == 2) {
            if (fill_num_li == (test_num_li - 1)) {
              e_li_sub_segment->b1 = 0;
            } else {
              e_li_sub_segment->b1 =  0x80;
            }

            e_li_sub_segment->b1 = e_li_sub_segment->b1 | (test_li_list[fill_num_li] >> 4);
            e_li_sub_segment->b2 = test_li_list[fill_num_li] << 4;
            fill_payload_p         = (uint8_t*)(&e_li_sub_segment->b3);
            *sizeP               = *sizeP - 2;
          } else {
            if (fill_num_li != (test_num_li - 1)) {
              e_li_sub_segment->b2  = e_li_sub_segment->b2 | 0x08;
            }

            e_li_sub_segment->b2 = e_li_sub_segment->b2 | (test_li_list[fill_num_li] >> 8);
            e_li_sub_segment->b3 = test_li_list[fill_num_li] & 0xFF;
            e_li_sub_segment++;
            fill_payload_p         = (uint8_t*)e_li_sub_segment;
            *sizeP               = *sizeP - 1;
          }

          LOG_T(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[RE-SEGMENT] ADD LI %d\n",
                PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
                test_li_list[fill_num_li]);
        }
      } else {
        LOG_T(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[RE-SEGMENT] ADD NO LI\n",
              PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP));
        fill_payload_p = (uint8_t*)e_li_sub_segment;
      }

      //---------------------------------------------------------------
      // copy payload to retransmit
      //---------------------------------------------------------------
      memcpy(fill_payload_p,
             &rlc_pP->tx_data_pdu_buffer[snP].payload[start_offset],
             test_pdu_copy_size);

      ((struct mac_tb_req*)(mb_sub_segment_p->data))->tb_size  = (tb_size_t)(((uint64_t)fill_payload_p)+ test_pdu_copy_size) - ((uint64_t)(&pdu_sub_segment_p->b1));

      // set LSF
      if ((test_pdu_copy_size + start_offset) == rlc_pP->tx_data_pdu_buffer[snP].payload_size) {
        pdu_sub_segment_p->data[0] = pdu_sub_segment_p->data[0] | 0x80;

        rlc_pP->tx_data_pdu_buffer[snP].flags.retransmit = 0;

        LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[RE-SEGMENT] RE-SEND DATA PDU SN %04d SO %d %d BYTES PAYLOAD %d BYTES LSF!\n",
              PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
              snP,
              start_offset,
              ((struct mac_tb_req*)(mb_sub_segment_p->data))->tb_size,
              test_pdu_copy_size);
      } else {
        LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[RE-SEGMENT] RE-SEND DATA PDU SN %04d SO %d %d BYTES PAYLOAD %d BYTES\n",
              PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
              snP,
              start_offset,
              ((struct mac_tb_req*)(mb_sub_segment_p->data))->tb_size,
              test_pdu_copy_size);
      }

      LOG_T(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[RE-SEGMENT] *sizeP %d = *sizeP %d - test_pdu_copy_size %d\n",
            PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
            *sizeP - test_pdu_copy_size, *sizeP,
            test_pdu_copy_size);

      *sizeP = *sizeP - test_pdu_copy_size;
      //---------------------------------------------------------------
      // update nack_so_start
      //---------------------------------------------------------------
      rlc_am_remove_hole(ctxt_pP, rlc_pP, snP, start_offset, test_pdu_copy_size+start_offset - 1);
      //rlc_pP->tx_data_pdu_buffer[snP].nack_so_start = rlc_pP->tx_data_pdu_buffer[snP].nack_so_start + test_pdu_copy_size;
      LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[RE-SEGMENT] RE-SEND DATA PDU SN %04d NOW nack_so_start %d nack_so_stop %d\n",
            PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
            snP,
            rlc_pP->tx_data_pdu_buffer[snP].nack_so_start,
            rlc_pP->tx_data_pdu_buffer[snP].nack_so_stop);
      /*if (rlc_pP->tx_data_pdu_buffer[snP].nack_so_start == rlc_pP->tx_data_pdu_buffer[snP].nack_so_stop) {
          rlc_pP->tx_data_pdu_buffer[snP].nack_so_start = 0;
          rlc_pP->tx_data_pdu_buffer[snP].nack_so_stop  = 0x7FFF;
      }*/
    } else {
      LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[RE-SEGMENT] COULD NOT GET INFO FOR DATA PDU SN %04d -> RETURN NULL\n",
            PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
            snP);
      return NULL;
    }
    return mb_sub_segment_p;
  } else {
    LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[RE-SEGMENT] RE-SEND DATA PDU SN %04d BUT NO PDU AVAILABLE -> RETURN NULL\n",
          PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
          snP);
    assert(3==4);
    return NULL;
  }
}
#endif
//-----------------------------------------------------------------------------
void rlc_am_tx_buffer_display (
  const protocol_ctxt_t* const  ctxt_pP,
  rlc_am_entity_t* const rlc_pP,
  char* const message_pP)
{
  rlc_sn_t       sn = rlc_pP->vt_a;
  int            i, loop = 0;
  rlc_am_tx_data_pdu_management_t *tx_data_pdu_buffer_p;

  if (message_pP) {
    LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT" Retransmission buffer %s VT(A)=%04d VT(S)=%04d:",
          PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
          message_pP,
          rlc_pP->vt_a,
          rlc_pP->vt_s);
  } else {
    LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT" Retransmission buffer VT(A)=%04d VT(S)=%04d:",
          PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
          rlc_pP->vt_a,
          rlc_pP->vt_s);
  }

  while (rlc_pP->vt_s != sn) {
	  tx_data_pdu_buffer_p = &rlc_pP->tx_data_pdu_buffer[sn % RLC_AM_WINDOW_SIZE];
    if (tx_data_pdu_buffer_p->mem_block) {
      if ((loop % 1) == 0) {
        LOG_D(RLC, "\nTX SN:\t");
      }

      if (tx_data_pdu_buffer_p->flags.retransmit) {
        LOG_D(RLC, "%04d %d/%d Bytes (NACK RTX:%02d ",sn, tx_data_pdu_buffer_p->header_and_payload_size, tx_data_pdu_buffer_p->payload_size,
        		tx_data_pdu_buffer_p->retx_count);
      } else {
        LOG_D(RLC, "%04d %d/%d Bytes (RTX:%02d ",sn, tx_data_pdu_buffer_p->header_and_payload_size, tx_data_pdu_buffer_p->payload_size,
        		tx_data_pdu_buffer_p->retx_count);
      }

      if (tx_data_pdu_buffer_p->num_holes == 0) {
        LOG_D(RLC, "SO:%04d->%04d)\t", tx_data_pdu_buffer_p->nack_so_start, tx_data_pdu_buffer_p->nack_so_stop);
      } else {
        for (i=0; i<tx_data_pdu_buffer_p->num_holes; i++) {
          assert(i < RLC_AM_MAX_HOLES_REPORT_PER_PDU);
          LOG_D(RLC, "SO:%04d->%04d)\t", tx_data_pdu_buffer_p->hole_so_start[i], tx_data_pdu_buffer_p->hole_so_stop[i]);
        }
      }

      loop++;
    }

    sn = (sn + 1) & RLC_AM_SN_MASK;
  }

  LOG_D(RLC, "\n");
}

//-----------------------------------------------------------------------------
mem_block_t * rlc_am_get_pdu_to_retransmit(
  const protocol_ctxt_t* const  ctxt_pP,
  rlc_am_entity_t* const rlc_pP)
{
	  rlc_sn_t             sn          = rlc_pP->vt_a;
	  rlc_sn_t             sn_end      = rlc_pP->vt_s;
	  mem_block_t*         pdu_p        = NULL;
	  rlc_am_tx_data_pdu_management_t* tx_data_pdu_management;

	  AssertFatal ((rlc_pP->retrans_num_pdus > 0) && (rlc_pP->vt_a !=  rlc_pP->vt_s), "RLC AM ReTx start process Error: NbPDUtoRetx=%d vtA=%d vtS=%d  LcId=%d !\n",
			  rlc_pP->retrans_num_pdus,rlc_pP->vt_a,rlc_pP->vt_s,rlc_pP->channel_id);

	  do
	  {
		  tx_data_pdu_management = &rlc_pP->tx_data_pdu_buffer[sn % RLC_AM_WINDOW_SIZE];
		  if ((tx_data_pdu_management->flags.retransmit) && (tx_data_pdu_management->flags.max_retransmit == 0))
		  {
			  AssertFatal (tx_data_pdu_management->sn == sn, "RLC AM ReTx PDU Error: SN Error pdu_sn=%d sn=%d vtA=%d vtS=%d LcId=%d !\n",
					  tx_data_pdu_management->sn,sn,rlc_pP->vt_a,rlc_pP->vt_s,rlc_pP->channel_id);
			  AssertFatal (tx_data_pdu_management->flags.transmitted == 1, "RLC AM ReTx PDU Error: State Error sn=%d vtA=%d vtS=%d LcId=%d !\n",
			  					  sn,rlc_pP->vt_a,rlc_pP->vt_s,rlc_pP->channel_id);
			  AssertFatal (tx_data_pdu_management->retx_payload_size > 0, "RLC AM ReTx PDU Error: No Data to Retx sn=%d vtA=%d vtS=%d LcId=%d !\n",
					  sn,rlc_pP->vt_a,rlc_pP->vt_s,rlc_pP->channel_id);

			  /* Either the whole RLC PDU is to be transmitted and there is enough MAC TBS or there is minimum TBS size for transmitting 1 AM PDU segment */
			  if ((tx_data_pdu_management->retx_payload_size == tx_data_pdu_management->payload_size) && (rlc_pP->nb_bytes_requested_by_mac >= tx_data_pdu_management->header_and_payload_size))
			  {
				  /* check maxretx is not hit */
				  if (tx_data_pdu_management->retx_count_next <= rlc_pP->max_retx_threshold)
				  {
					  pdu_p = rlc_am_retransmit_get_copy(ctxt_pP, rlc_pP, sn);

					  if (pdu_p != NULL)
					  {
						  rlc_pP->retrans_num_bytes_to_retransmit -= tx_data_pdu_management->retx_payload_size;
						  rlc_pP->retrans_num_pdus --;
						  tx_data_pdu_management->retx_payload_size = 0;
						  tx_data_pdu_management->flags.retransmit = 0;

				    	  // update stats
				          rlc_pP->stat_tx_data_pdu                   += 1;
				          rlc_pP->stat_tx_retransmit_pdu             += 1;
				          rlc_pP->stat_tx_retransmit_pdu_by_status   += 1;
				          rlc_pP->stat_tx_data_bytes                 += tx_data_pdu_management->payload_size;
				          rlc_pP->stat_tx_retransmit_bytes           += tx_data_pdu_management->payload_size;
				          rlc_pP->stat_tx_retransmit_bytes_by_status += tx_data_pdu_management->payload_size;

					  }
				  }
				  else
				  {
					  // TO DO : RLC Notification to RRC + ReEstablishment procedure
					  tx_data_pdu_management->flags.max_retransmit = 1;
					  LOG_W(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[RLC AM MAX RETX=%d] SN %04d\n",
					                PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
									tx_data_pdu_management->retx_count_next,
					                sn);
				  }
			  }
			  else if (rlc_pP->nb_bytes_requested_by_mac >= 5)
			  {
				  /* Resegmentation case */
				  /* check maxretx is not hit */
				  if (tx_data_pdu_management->retx_count_next <= rlc_pP->max_retx_threshold)
				  {
					  sdu_size_t pdu_data_size = 0;

					  pdu_p = rlc_am_retransmit_get_am_segment(ctxt_pP, rlc_pP, tx_data_pdu_management,&pdu_data_size);

					  if (pdu_p != NULL)
					  {
						  AssertFatal ((tx_data_pdu_management->retx_payload_size >= pdu_data_size) && (rlc_pP->retrans_num_bytes_to_retransmit >= pdu_data_size), "RLC AM ReTx PDU Segment Error: DataSize=%d PDUReTxsize=%d TotalReTxsize=%d sn=%d LcId=%d !\n",
								  pdu_data_size,tx_data_pdu_management->retx_payload_size,rlc_pP->retrans_num_bytes_to_retransmit,sn,rlc_pP->channel_id);

						  tx_data_pdu_management->retx_payload_size -= pdu_data_size;
						  rlc_pP->retrans_num_bytes_to_retransmit -= pdu_data_size;
						  if (tx_data_pdu_management->retx_payload_size == 0)
						  {
							  rlc_pP->retrans_num_pdus --;
							  tx_data_pdu_management->retx_payload_size = 0;
							  tx_data_pdu_management->flags.retransmit = 0;
						  }

				    	  // update stats
				          rlc_pP->stat_tx_data_pdu                   += 1;
				          rlc_pP->stat_tx_retransmit_pdu             += 1;
				          rlc_pP->stat_tx_retransmit_pdu_by_status   += 1;
				          rlc_pP->stat_tx_data_bytes                 += pdu_data_size;
				          rlc_pP->stat_tx_retransmit_bytes           += pdu_data_size;
				          rlc_pP->stat_tx_retransmit_bytes_by_status += pdu_data_size;

					  }
				  }
				  else
				  {
					  // TO DO : RLC Notification to RRC + ReEstablishment procedure
					  tx_data_pdu_management->flags.max_retransmit = 1;
					  LOG_W(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[RLC AM MAX RETX=%d] SN %04d\n",
					  					                PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
					  									tx_data_pdu_management->retx_count_next,
					  					                sn);
				  }
			  }

			  if (pdu_p != NULL)
			  {
				  /* check polling */
				  rlc_am_pdu_sn_10_t* pdu_header_p   = (rlc_am_pdu_sn_10_t*) (&pdu_p->data[sizeof(struct mac_tb_req)]);
				  rlc_am_pdu_polling(ctxt_pP, rlc_pP, pdu_header_p, tx_data_pdu_management->payload_size,false);

				  tx_data_pdu_management->retx_count = tx_data_pdu_management->retx_count_next;

				  break;
			  }

		  }

		  sn = RLC_AM_NEXT_SN(sn);
	  } while((sn != sn_end) && (rlc_pP->retrans_num_pdus > 0));

	  return pdu_p;
}

#if 0
//-----------------------------------------------------------------------------
void rlc_am_retransmit_any_pdu(
  const protocol_ctxt_t* const  ctxt_pP,
  rlc_am_entity_t* const rlc_pP)
{
  rlc_sn_t             sn           = (rlc_pP->vt_s - 1) & RLC_AM_SN_MASK;
  rlc_sn_t             sn_end       = (rlc_pP->vt_a - 1) & RLC_AM_SN_MASK;
  int                  found_pdu    = 0;
  rlc_sn_t             found_pdu_sn = 0; // avoid warning
  mem_block_t*         pdu_p        = NULL;
  rlc_am_pdu_sn_10_t*  pdu_sn_10_p  = NULL;

  LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[FORCE-TRAFFIC] rlc_am_retransmit_any_pdu()\n",
        PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP));

  while (sn != sn_end) {
    if (rlc_pP->tx_data_pdu_buffer[sn].mem_block != NULL) {
      if (!found_pdu) {
        found_pdu = 1;
        found_pdu_sn = sn;
      }

      if (rlc_pP->tx_data_pdu_buffer[sn].header_and_payload_size <= rlc_pP->nb_bytes_requested_by_mac) {
        LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[FORCE-TRAFFIC] RE-SEND DATA PDU SN %04d\n",
              PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
              sn);
#if MESSAGE_CHART_GENERATOR_RLC_MAC
        MSC_LOG_EVENT((ctxt_pP->enb_flag == ENB_FLAG_YES) ? MSC_RLC_ENB:MSC_RLC_UE,\
                               "0 "PROTOCOL_RLC_AM_MSC_FMT" RTX any pdu found SN %u",\
                               PROTOCOL_RLC_AM_MSC_ARGS(ctxt_pP,rlc_pP), sn);
#endif
        rlc_am_nack_pdu (ctxt_pP, rlc_pP, sn, 0, 0x7FFF);
        // no need for update rlc_pP->nb_bytes_requested_by_mac
        pdu_p = rlc_am_retransmit_get_copy(ctxt_pP, rlc_pP, sn);
        pdu_sn_10_p = (rlc_am_pdu_sn_10_t*) (&pdu_p->data[sizeof(struct mac_tb_req)]);
        rlc_am_pdu_polling(ctxt_pP, rlc_pP, pdu_sn_10_p, rlc_pP->tx_data_pdu_buffer[sn].header_and_payload_size,false);
        //BugFix: polling is checked and done in function above !
        //pdu_sn_10_p->b1 = pdu_sn_10_p->b1 | 0x20;
        //BugFix : pdu_without_poll and byte_without_poll are reset only if a Poll is transmitted
        //rlc_pP->c_pdu_without_poll     = 0;
        //rlc_pP->c_byte_without_poll    = 0;
        //rlc_pP->poll_sn = (rlc_pP->vt_s -1) & RLC_AM_SN_MASK;
        rlc_pP->stat_tx_data_pdu                   += 1;
        rlc_pP->stat_tx_retransmit_pdu             += 1;
        rlc_pP->stat_tx_data_bytes                 += ((struct mac_tb_req*)(pdu_p->data))->tb_size;
        rlc_pP->stat_tx_retransmit_bytes           += ((struct mac_tb_req*)(pdu_p->data))->tb_size;
        list_add_tail_eurecom (pdu_p, &rlc_pP->pdus_to_mac_layer);
        return;
      }
    }

    sn = (sn - 1) & RLC_AM_SN_MASK;
  }

  // no pdu_p with correct size has been found
  // so re-segment a pdu_p if possible
  if (found_pdu) {
    LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[FORCE-TRAFFIC] SEND SEGMENT OF DATA PDU SN %04d\n",
          PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
          found_pdu_sn);

    if (rlc_pP->nb_bytes_requested_by_mac > 4) {
#if MESSAGE_CHART_GENERATOR_RLC_MAC
        MSC_LOG_EVENT((ctxt_pP->enb_flag == ENB_FLAG_YES) ? MSC_RLC_ENB:MSC_RLC_UE,\
                               "0 "PROTOCOL_RLC_AM_MSC_FMT" RTX any pdu found SN %u (subseg)",\
                               PROTOCOL_RLC_AM_MSC_ARGS(ctxt_pP,rlc_pP), found_pdu_sn);
#endif
      rlc_am_nack_pdu (ctxt_pP, rlc_pP, found_pdu_sn, 0, 0x7FFF);
      pdu_p = rlc_am_retransmit_get_subsegment(ctxt_pP, rlc_pP, found_pdu_sn, &rlc_pP->nb_bytes_requested_by_mac);
      pdu_sn_10_p = (rlc_am_pdu_sn_10_t*) (&pdu_p->data[sizeof(struct mac_tb_req)]);
      rlc_am_pdu_polling(ctxt_pP, rlc_pP, pdu_sn_10_p, rlc_pP->tx_data_pdu_buffer[found_pdu_sn].header_and_payload_size,false);
      pdu_sn_10_p->b1 = pdu_sn_10_p->b1 | 0x20;
      rlc_pP->c_pdu_without_poll     = 0;
      rlc_pP->c_byte_without_poll    = 0;
      //rlc_pP->poll_sn = (rlc_pP->vt_s -1) & RLC_AM_SN_MASK;
      rlc_pP->stat_tx_data_pdu                   += 1;
      rlc_pP->stat_tx_retransmit_pdu             += 1;
      rlc_pP->stat_tx_data_bytes                 += ((struct mac_tb_req*)(pdu_p->data))->tb_size;
      rlc_pP->stat_tx_retransmit_bytes           += ((struct mac_tb_req*)(pdu_p->data))->tb_size;
      list_add_tail_eurecom (pdu_p, &rlc_pP->pdus_to_mac_layer);
      return;
    } else {
      LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[FORCE-TRAFFIC] ... BUT NOT ENOUGH BYTES ALLOWED BY MAC %0d\n",
            PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
            rlc_pP->nb_bytes_requested_by_mac);
    }
  }
}
#endif
