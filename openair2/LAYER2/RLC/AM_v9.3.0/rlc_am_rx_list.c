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
#define RLC_AM_RX_LIST_C 1
//-----------------------------------------------------------------------------
#include "platform_types.h"
//-----------------------------------------------------------------------------
#include "assertions.h"
#include "list.h"
#include "rlc_am.h"
#include "LAYER2/MAC/mac_extern.h"
#include "common/utils/LOG/log.h"


boolean_t rlc_am_rx_check_vr_reassemble(
		  const protocol_ctxt_t* const ctxt_pP,
		  const rlc_am_entity_t* const rlc_pP)
{
	mem_block_t*       cursor_p                    = rlc_pP->receiver_buffer.head;
	rlc_am_rx_pdu_management_t * pdu_cursor_mgnt_p = NULL;
	sdu_size_t          next_waited_so = 0;
	boolean_t reassemble = FALSE;

	if (cursor_p != NULL) {

		rlc_am_pdu_info_t* pdu_info_p = &((rlc_am_rx_pdu_management_t*)(cursor_p->data))->pdu_info;
		rlc_usn_t sn_ref = pdu_info_p->sn;

		if (sn_ref != rlc_pP->vr_r) {
			/* Case vrR has advanced from head : most likely case */

			reassemble = TRUE;
			/* Handle first SN if it is made of PDU segments : set them all to be reassembled */
			if (pdu_info_p->rf) {
				pdu_cursor_mgnt_p = (rlc_am_rx_pdu_management_t *) (cursor_p->data);
				next_waited_so = 0;
//Assertion(eNB)_PRAN_DesignDocument_annex No.785
                if(pdu_cursor_mgnt_p->all_segments_received <= 0)
                {
                   LOG_E(RLC, "AM Rx Check Reassembly head SN=%d with PDU segments != vrR=%d should be fully received LCID=%d\n",
                         sn_ref,rlc_pP->vr_r,rlc_pP->channel_id);
                   return FALSE;
                }
/*
				AssertFatal(pdu_cursor_mgnt_p->all_segments_received > 0,"AM Rx Check Reassembly head SN=%d with PDU segments != vrR=%d should be fully received LCID=%d\n",
						sn_ref,rlc_pP->vr_r,rlc_pP->channel_id);
*/
				while ((cursor_p != NULL) && (pdu_info_p->sn == sn_ref) && (pdu_info_p->so == next_waited_so)) {
					if (pdu_cursor_mgnt_p->segment_reassembled == RLC_AM_RX_PDU_SEGMENT_REASSEMBLE_NO) {
						pdu_cursor_mgnt_p->segment_reassembled = RLC_AM_RX_PDU_SEGMENT_REASSEMBLE_PENDING;
					}
					next_waited_so += pdu_info_p->payload_size;
					cursor_p = cursor_p->next;
					if (cursor_p != NULL) {
						pdu_cursor_mgnt_p = (rlc_am_rx_pdu_management_t *) (cursor_p->data);
						pdu_info_p = &((rlc_am_rx_pdu_management_t*)(cursor_p->data))->pdu_info;
					}
				}
			}

			/* Now jump up to vrR */
			while ((RLC_AM_DIFF_SN(pdu_info_p->sn,sn_ref) < RLC_AM_DIFF_SN(rlc_pP->vr_r,sn_ref)) && (cursor_p != NULL)) {
				cursor_p = cursor_p->next;
				if (cursor_p != NULL) {
					pdu_info_p = &((rlc_am_rx_pdu_management_t*)(cursor_p->data))->pdu_info;
				}
			}

			/* Handle vrR if it is made of incomplete PDU Segments */
			if ((cursor_p != NULL) && (pdu_info_p->sn == rlc_pP->vr_r)) {
				pdu_cursor_mgnt_p = (rlc_am_rx_pdu_management_t *) (cursor_p->data);
				next_waited_so = 0;
//Assertion(eNB)_PRAN_DesignDocument_annex No.786
              if(pdu_cursor_mgnt_p->all_segments_received != 0)
              {
                 LOG_E(RLC, "AM Rx Check Reassembly vr=%d should be partly received SNHead=%d LCID=%d\n",
                       rlc_pP->vr_r,sn_ref,rlc_pP->channel_id);
                 return FALSE;
              }
/*
				AssertFatal(pdu_cursor_mgnt_p->all_segments_received == 0,"AM Rx Check Reassembly vr=%d should be partly received SNHead=%d LCID=%d\n",
						rlc_pP->vr_r,sn_ref,rlc_pP->channel_id);
*/
				while ((cursor_p != NULL) && (pdu_info_p->sn == rlc_pP->vr_r) && (pdu_info_p->so == next_waited_so)) {
					if (pdu_cursor_mgnt_p->segment_reassembled == RLC_AM_RX_PDU_SEGMENT_REASSEMBLE_NO) {
						pdu_cursor_mgnt_p->segment_reassembled = RLC_AM_RX_PDU_SEGMENT_REASSEMBLE_PENDING;
					}
					next_waited_so += pdu_info_p->payload_size;
					cursor_p = cursor_p->next;
					if (cursor_p != NULL) {
						pdu_cursor_mgnt_p = (rlc_am_rx_pdu_management_t *) (cursor_p->data);
						pdu_info_p = &((rlc_am_rx_pdu_management_t*)(cursor_p->data))->pdu_info;
					}
				}
			}
		} /* end sn_ref != rlc_pP->vr_r */
		else {
			/* case vrR = partially received */

			pdu_cursor_mgnt_p = (rlc_am_rx_pdu_management_t *) (cursor_p->data);
			next_waited_so = 0;
//Assertion(eNB)_PRAN_DesignDocument_annex No.787
            if(pdu_cursor_mgnt_p->all_segments_received != 0)
            {
               LOG_E(RLC, "AM Rx Check Reassembly SNHead=vr=%d should be partly received LCID=%d\n",
                     rlc_pP->vr_r,rlc_pP->channel_id);
               return FALSE;
            }
/*
			AssertFatal(pdu_cursor_mgnt_p->all_segments_received == 0,"AM Rx Check Reassembly SNHead=vr=%d should be partly received LCID=%d\n",
					rlc_pP->vr_r,rlc_pP->channel_id);
*/
			while ((cursor_p != NULL) && (pdu_info_p->sn == rlc_pP->vr_r) && (pdu_info_p->so == next_waited_so)) {
				if (pdu_cursor_mgnt_p->segment_reassembled == RLC_AM_RX_PDU_SEGMENT_REASSEMBLE_NO) {
					pdu_cursor_mgnt_p->segment_reassembled = RLC_AM_RX_PDU_SEGMENT_REASSEMBLE_PENDING;
					reassemble = TRUE;
				}
				next_waited_so += pdu_info_p->payload_size;
				cursor_p = cursor_p->next;
				if (cursor_p != NULL) {
					pdu_cursor_mgnt_p = (rlc_am_rx_pdu_management_t *) (cursor_p->data);
					pdu_info_p = &((rlc_am_rx_pdu_management_t*)(cursor_p->data))->pdu_info;
				}
			}
		} /*end sn_ref == rlc_pP->vr_r */
	}
	return reassemble;
}

mem_block_t * create_new_segment_from_pdu(
		mem_block_t* const tb_pP,
		uint16_t so_offset, /* offset from the data part of the PDU to copy */
		sdu_size_t	data_length_to_copy)
{
	rlc_am_pdu_info_t* pdu_rx_info_p	= &((rlc_am_rx_pdu_management_t*)(tb_pP->data))->pdu_info;
	rlc_am_pdu_info_t* pdu_new_segment_info_p = NULL;
	mem_block_t *	new_segment_p		= NULL;
	int16_t  new_li_list[RLC_AM_MAX_SDU_IN_PDU];
	int16_t header_size = 0;
	uint8_t	num_li = 0;
	boolean_t fi_start, fi_end, lsf;

	/* Init some PDU Segment header fixed parameters */
	fi_start = !((pdu_rx_info_p->fi & 0x2) >> 1);
	fi_end = !(pdu_rx_info_p->fi & 0x1);
	lsf = ((pdu_rx_info_p->lsf == 1) || (pdu_rx_info_p->rf == 0));

	/* Handle NO Li case fist */
	if (pdu_rx_info_p->num_li == 0) {

		header_size = RLC_AM_PDU_SEGMENT_HEADER_MIN_SIZE;

		if (so_offset) {
			fi_start = FALSE;
		}
		if (so_offset + data_length_to_copy != pdu_rx_info_p->payload_size) {
			fi_end = FALSE;
			lsf = FALSE;
		}
	} // end no LI in original segment
	else {

		uint8_t	li_index = 0;
		uint16_t li_sum = 0;
		num_li = pdu_rx_info_p->num_li;

		/* set LSF to false if we know that end of the original segment will not be copied */
		if (so_offset + data_length_to_copy != pdu_rx_info_p->payload_size) {
			lsf = FALSE;
		}

		/* catch the first LI containing so_offset */
		while ((li_index < pdu_rx_info_p->num_li) && (li_sum + pdu_rx_info_p->li_list[li_index] <= so_offset)) {
			li_sum += pdu_rx_info_p->li_list[li_index];
			num_li --;
			li_index ++;
		}

		/* set FI start if so_offset = LI sum and at least one LI have been read  */
		if ((li_index) && (so_offset == li_sum)) {
			fi_start = TRUE;
		}

		/* Fill LI until the end */
		if (num_li) {
			sdu_size_t	remaining_size = data_length_to_copy;
			uint8_t	j = 0;
			new_li_list[0] = li_sum + pdu_rx_info_p->li_list[li_index] - so_offset;
			if (data_length_to_copy <= new_li_list[0]) {
				num_li = 0;
			}
			else {
				remaining_size -= new_li_list[0];
				j++;
				li_index ++;
				while ((li_index < pdu_rx_info_p->num_li) && (remaining_size >= pdu_rx_info_p->li_list[li_index])) {
					remaining_size -= pdu_rx_info_p->li_list[li_index];
					new_li_list[j] = pdu_rx_info_p->li_list[li_index];
					j++;
					li_index ++;
				}

				/* update number of LI in the segment */
				num_li = j;
				/* set FI End if remaining size = 0  */
				if (remaining_size == 0) {
					fi_end = TRUE;
				}
			}
		}

		/* compute header size */
		header_size = RLC_AM_PDU_SEGMENT_HEADER_SIZE(num_li);

	} // end LIs in original segment

	/* Allocate new buffer */
	new_segment_p = get_free_mem_block(sizeof (mac_rlc_max_rx_header_size_t) + header_size + data_length_to_copy, __func__);

	/* Fill PDU  Segment Infos and Header */
	if (new_segment_p != NULL) {
		pdu_new_segment_info_p	= &((rlc_am_rx_pdu_management_t*)(new_segment_p->data))->pdu_info;
		rlc_am_rx_pdu_management_t * pdu_cursor_mgnt_p = (rlc_am_rx_pdu_management_t *) (new_segment_p->data);
		uint8_t   *pdu_segment_header_p        	= (uint8_t *)&(new_segment_p->data[sizeof (mac_rlc_max_rx_header_size_t)]);

		pdu_cursor_mgnt_p->segment_reassembled	= RLC_AM_RX_PDU_SEGMENT_REASSEMBLE_NO; //to be updated after if SN = vrR

		pdu_new_segment_info_p->d_c 			= pdu_rx_info_p->d_c;
		pdu_new_segment_info_p->sn				= pdu_rx_info_p->sn;
		pdu_new_segment_info_p->p				= pdu_rx_info_p->p;
		pdu_new_segment_info_p->rf				= 1;
		pdu_new_segment_info_p->fi				= (((fi_start ? 0: 1) << 1) | (fi_end ? 0: 1));
		pdu_new_segment_info_p->num_li			= num_li;
		pdu_new_segment_info_p->e				= (num_li ? 1: 0);
		pdu_new_segment_info_p->lsf				= (lsf ? 1: 0);
		pdu_new_segment_info_p->so				= pdu_rx_info_p->so + so_offset;
		pdu_new_segment_info_p->payload			= pdu_segment_header_p + header_size;
		pdu_new_segment_info_p->header_size 	= header_size;
		pdu_new_segment_info_p->payload_size	= data_length_to_copy;
		pdu_new_segment_info_p->hidden_size		= data_length_to_copy;
		for (int i=0; i < num_li; i++) {
			pdu_new_segment_info_p->li_list[i] = new_li_list[i];
			pdu_new_segment_info_p->hidden_size -= new_li_list[i];
		}

		/* Fill Header part in the buffer */
		/* Content is supposed to be init with 0 so with FIStart=FIEnd=TRUE */
		/* copy first two bytes from original: D/C + RF + FI + E+ SN*/
		memset(pdu_segment_header_p, 0, header_size);
		RLC_AM_PDU_SET_D_C(*pdu_segment_header_p);
		RLC_AM_PDU_SET_RF(*pdu_segment_header_p);
		if (pdu_new_segment_info_p->p) {
			RLC_AM_PDU_SET_POLL(*pdu_segment_header_p);
		}
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
		/* E */
		if (pdu_new_segment_info_p->e) {
			RLC_AM_PDU_SET_E(*pdu_segment_header_p);
		}
		/* SN */
		(*pdu_segment_header_p) |= ((pdu_new_segment_info_p->sn >> 8) & 0x3);
		*(pdu_segment_header_p + 1) = (pdu_new_segment_info_p->sn & 0xFF);

		pdu_segment_header_p += 2;

		/* Last Segment Flag (LSF) */
		if (lsf)
		{
			RLC_AM_PDU_SET_LSF(*pdu_segment_header_p);
		}
		/* Store SO bytes */
		* (pdu_segment_header_p )  		|= ((pdu_new_segment_info_p->so >> 8) & 0x7F);
		* (pdu_segment_header_p + 1)  	= pdu_new_segment_info_p->so & 0xFF;

		if (num_li) {
			uint16_t index = 0;
			uint16_t temp = 0;
			uint8_t li_bit_offset = 4; /* toggle between 0 and 4 */
			uint8_t li_jump_offset = 1; /* toggle between 1 and 2 */

			/* loop on nb of LIs */
			pdu_segment_header_p += 2;

			while (index < num_li)
			{
				/* Set E bit for next LI if present */
				if (index < num_li - 1)
					RLC_SET_BIT(temp,li_bit_offset + RLC_AM_LI_BITS);
				/* Set LI */
				RLC_AM_PDU_SET_LI(temp,new_li_list[index],li_bit_offset);
				*pdu_segment_header_p = temp >> 8;
				*(pdu_segment_header_p + 1) = temp & 0xFF;
				pdu_segment_header_p += li_jump_offset;
				li_bit_offset ^= 0x4;
				li_jump_offset ^= 0x3;

				temp = ((*pdu_segment_header_p) << 8) | (*(pdu_segment_header_p + 1));
				index ++;
			}
		}

		/* copy data part */
		/* Fill mem_block contexts */
		((struct mac_tb_ind *) (new_segment_p->data))->first_bit = 0;
		((struct mac_tb_ind *) (new_segment_p->data))->data_ptr = (uint8_t*)&new_segment_p->data[sizeof (mac_rlc_max_rx_header_size_t)];
		((struct mac_tb_ind *) (new_segment_p->data))->size = data_length_to_copy + header_size;
		memcpy(pdu_new_segment_info_p->payload,pdu_rx_info_p->payload + so_offset,data_length_to_copy);
	}

	return new_segment_p;
}

rlc_am_rx_pdu_status_t rlc_am_rx_list_handle_pdu_segment(
		const protocol_ctxt_t* const  ctxt_pP,
		rlc_am_entity_t* const rlc_pP,
		mem_block_t* const tb_pP)
{
	  rlc_am_pdu_info_t* pdu_rx_info_p                  = &((rlc_am_rx_pdu_management_t*)(tb_pP->data))->pdu_info;
	  rlc_am_pdu_info_t* pdu_info_cursor_p           = NULL;
	  rlc_am_pdu_info_t* pdu_info_previous_cursor_p  = NULL;
	  mem_block_t*       cursor_p                    = rlc_pP->receiver_buffer.head;
	  mem_block_t*       previous_cursor_p           = NULL;
	  mem_block_t*       next_cursor_p           	 = NULL;
	  uint16_t so_start_min = 0;
	  uint16_t so_end = 0;
	  uint16_t so_start_segment = pdu_rx_info_p->so;
	  uint16_t so_end_segment = pdu_rx_info_p->so + pdu_rx_info_p->payload_size - 1;

	  /*****************************************************/
	  // 1) Find previous cursor to the PDU to insert
	  /*****************************************************/
//Assertion(eNB)_PRAN_DesignDocument_annex No.791
      if(cursor_p == NULL)
      {
        LOG_E(RLC, "AM Rx PDU Error, received buffer empty LcID=%d\n",rlc_pP->channel_id);
        return RLC_AM_DATA_PDU_STATUS_HEADER_ERROR;
      }
/*
	  AssertFatal(cursor_p != NULL,"AM Rx PDU Error, received buffer empty LcID=%d\n",rlc_pP->channel_id);
*/
	  do {
		  pdu_info_cursor_p = &((rlc_am_rx_pdu_management_t*)(cursor_p->data))->pdu_info;

		  // Stop if Cursor SN >= Received SN
		  if (RLC_AM_DIFF_SN(pdu_info_cursor_p->sn,rlc_pP->vr_r) >= RLC_AM_DIFF_SN(pdu_rx_info_p->sn,rlc_pP->vr_r)) {
			  break;
		  }

          previous_cursor_p = cursor_p;
          pdu_info_previous_cursor_p = pdu_info_cursor_p;
          cursor_p = cursor_p->next;
	  } while (cursor_p != NULL);

	  /*****************************************************/
	  // 2) Store the received Segment
	  /*****************************************************/
	  // First case : cursor_p is NULL or its SN is different from the received one, it means the SN is received for the first time
	  // Insert PDU after previous_cursor_p
	  if ((cursor_p == NULL) || (pdu_info_cursor_p->sn != pdu_rx_info_p->sn)) {
          if (previous_cursor_p != NULL) {
                LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[PROCESS RX PDU SEGMENT SN=%d] PDU SEGMENT INSERTED AFTER PDU SN=%d\n",
                            PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),pdu_rx_info_p->sn,
							pdu_info_previous_cursor_p->sn);
              list2_insert_after_element(tb_pP, previous_cursor_p, &rlc_pP->receiver_buffer);
          }
          else { /* SN of head of Rx PDU list is higher than received PDU SN */
              LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[PROCESS RX PDU SEGMENT SN=%d] PDU SEGMENT INSERTED BEFORE PDU SN=%d\n",
                            PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),pdu_rx_info_p->sn,
                            pdu_info_cursor_p->sn);
              list2_insert_before_element(tb_pP, cursor_p, &rlc_pP->receiver_buffer);
          }

		  return RLC_AM_DATA_PDU_STATUS_OK;
	  }

	  /********************************************/
	  /* Now handle case cursor->sn = received SN */
	  /********************************************/

	  rlc_am_rx_pdu_management_t * pdu_cursor_mgnt_p = (rlc_am_rx_pdu_management_t *) (cursor_p->data);

	  // Filter out SN duplicate
	  if (pdu_cursor_mgnt_p->all_segments_received) {
		  return RLC_AM_DATA_PDU_STATUS_AM_SEGMENT_DUPLICATE;
	  }

	  // Try to catch a segment duplicate
	  next_cursor_p = cursor_p;
	  while ((next_cursor_p != NULL) && (pdu_info_cursor_p->sn == pdu_rx_info_p->sn)) {
		  if ((so_start_segment >= pdu_info_cursor_p->so) && (so_end_segment <= pdu_info_cursor_p->so + pdu_info_cursor_p->payload_size - 1)) {
			  LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[PROCESS RX PDU SEGMENT]  DISCARD : DUPLICATE SEGMENT SN=%d\n",
							  PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),pdu_rx_info_p->sn);
			  return RLC_AM_DATA_PDU_STATUS_AM_SEGMENT_DUPLICATE;
		  }
		  next_cursor_p = next_cursor_p->next;
		  if (next_cursor_p != NULL) {
			  pdu_info_cursor_p = &((rlc_am_rx_pdu_management_t*)(next_cursor_p->data))->pdu_info;
		  }
	  }

	  // Reset pdu_info_cursor_p because of the loop before
	  pdu_info_cursor_p = &((rlc_am_rx_pdu_management_t*)(cursor_p->data))->pdu_info;

	  // Try to Handle the most likely cases first
	  if (pdu_info_cursor_p->so == 0) {

		  /* Loop on stored segments and find the stored segment containing received SOStart */
		  previous_cursor_p = cursor_p;
		  pdu_info_previous_cursor_p = pdu_info_cursor_p;
		  while ((cursor_p != NULL) && (pdu_info_cursor_p->sn == pdu_rx_info_p->sn)
				  && ((pdu_info_cursor_p->so + pdu_info_cursor_p->payload_size) <= so_start_segment)) {
			  previous_cursor_p = cursor_p;
			  pdu_info_previous_cursor_p = pdu_info_cursor_p;
			  cursor_p = cursor_p->next;
			  if (cursor_p != NULL) {
				  pdu_info_cursor_p = &((rlc_am_rx_pdu_management_t*)(cursor_p->data))->pdu_info;
			  }
		  }

		  // Most likely case : no duplicate, the segment is put after all stored segments which are contiguous
		  if ((cursor_p == NULL) || (pdu_info_cursor_p->sn != pdu_rx_info_p->sn) || (pdu_info_cursor_p->so > so_end_segment)) {
              LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[PROCESS RX PDU SEGMENT SN=%d SOSTART=%d] PDU SEGMENT INSERTED AFTER PDU SEGMENT WITH SOEND=%d\n",
                          PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),pdu_rx_info_p->sn,so_start_segment,
						  pdu_info_previous_cursor_p->so + pdu_info_previous_cursor_p->payload_size - 1);

			  list2_insert_after_element(tb_pP, previous_cursor_p, &rlc_pP->receiver_buffer);
			  return RLC_AM_DATA_PDU_STATUS_OK;
		  }

		  // Duplicate case : Resume contiguous scan and update previous_cursor_p
		  so_start_min = pdu_info_previous_cursor_p->so;
		  so_end = so_start_min + pdu_info_previous_cursor_p->payload_size;
		  cursor_p = previous_cursor_p->next;
		  if (cursor_p != NULL) {
			  pdu_info_cursor_p = &((rlc_am_rx_pdu_management_t*)(cursor_p->data))->pdu_info;
		  }
		  while ((cursor_p != NULL) && (pdu_info_cursor_p->sn == pdu_rx_info_p->sn)
				  && (pdu_info_cursor_p->so == so_end)) {
			  previous_cursor_p = cursor_p;
			  pdu_info_previous_cursor_p = pdu_info_cursor_p;
			  so_end += pdu_info_cursor_p->payload_size;
			  cursor_p = cursor_p->next;
			  if (cursor_p != NULL) {
				  pdu_info_cursor_p = &((rlc_am_rx_pdu_management_t*)(cursor_p->data))->pdu_info;
			  }
		  }

		  /* Now discard the PDU segment if it is within so_start_min and so_end */
		  if ((so_start_min <= so_start_segment) && (so_end_segment <= so_end - 1)) {
			  LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[PROCESS RX PDU SEGMENT]  DISCARD : DUPLICATE SEGMENT SN=%d\n",
							  PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),pdu_rx_info_p->sn);
			  return RLC_AM_DATA_PDU_STATUS_AM_SEGMENT_DUPLICATE;
		  }

		  // Discard potential embedded segments in the received PDU segment
		  // The first one is discontigous
		  next_cursor_p = cursor_p;
		  while ((next_cursor_p != NULL) && (pdu_info_cursor_p->sn == pdu_rx_info_p->sn) &&
				  (so_end_segment >= pdu_info_cursor_p->so + pdu_info_cursor_p->payload_size - 1)) {
			  /* Discard the segment */
			  cursor_p = next_cursor_p;
			  next_cursor_p = next_cursor_p->next;
			  list2_remove_element (cursor_p, &rlc_pP->receiver_buffer);
			  free_mem_block(cursor_p, __func__);

			  if (next_cursor_p != NULL) {
				  pdu_info_cursor_p = &((rlc_am_rx_pdu_management_t*)(next_cursor_p->data))->pdu_info;
			  }
		  }
		  cursor_p = next_cursor_p;

		  //Remove duplicate at the begining
		  if (so_start_segment < pdu_info_previous_cursor_p->so) {
			  so_start_segment = pdu_info_previous_cursor_p->so + pdu_info_previous_cursor_p->payload_size;
		  }
		  else if (so_start_segment < pdu_info_previous_cursor_p->so + pdu_info_previous_cursor_p->payload_size){
			  so_start_segment += (pdu_info_previous_cursor_p->so + pdu_info_previous_cursor_p->payload_size - so_start_segment);
		  }

		  // Now remove duplicate at the end, only valid if cursor_p SN has the same received SN
		  if (cursor_p != NULL) {
			  pdu_info_cursor_p = &((rlc_am_rx_pdu_management_t*)(next_cursor_p->data))->pdu_info;

			  if ((pdu_info_cursor_p->sn == pdu_rx_info_p->sn) && (so_end_segment >= pdu_info_cursor_p->so)) {
				  so_end_segment = pdu_info_cursor_p->so - 1;
			  }
		  }

//Assertion(eNB)_PRAN_DesignDocument_annex No.792
          if((so_start_segment > so_end_segment) || (pdu_rx_info_p->so > so_start_segment) ||
		     (so_end_segment > pdu_rx_info_p->so + pdu_rx_info_p->payload_size - 1))
          {
            LOG_E(RLC, " AM RX PDU Segment Duplicate elimination error FirstSO=0 OldSOStart=%d OldSOEnd=%d newSOStart=%d newSOEnd =%d SN=%d\n",
			      pdu_rx_info_p->so,pdu_rx_info_p->so + pdu_rx_info_p->payload_size - 1,so_start_segment,so_end_segment,pdu_rx_info_p->sn);
            return RLC_AM_DATA_PDU_STATUS_AM_SEGMENT_DUPLICATE;
          }
/*
		  AssertFatal((so_start_segment <= so_end_segment) && (pdu_rx_info_p->so <= so_start_segment) &&
				  (so_end_segment <= pdu_rx_info_p->so + pdu_rx_info_p->payload_size - 1),
				  " AM RX PDU Segment Duplicate elimination error FirstSO=0 OldSOStart=%d OldSOEnd=%d newSOStart=%d newSOEnd =%d SN=%d\n",
				  pdu_rx_info_p->so,pdu_rx_info_p->so + pdu_rx_info_p->payload_size - 1,so_start_segment,so_end_segment,pdu_rx_info_p->sn);
*/
	  } // end pdu_info_cursor_p->so == 0
	  else {
		  // Handle most likely case : PDU Segment without duplicate is inserted before first stored PDU segment
		  if (so_end_segment < pdu_info_cursor_p->so) {
              LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[PROCESS RX PDU SEGMENT SN=%d SOSTART=%d SOEND=%d] PDU SEGMENT INSERTED BEFORE PDU SEGMENT WITH SOSTART=%d\n",
                            PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),pdu_rx_info_p->sn,so_start_segment,so_end_segment,
							pdu_info_cursor_p->so);

			  list2_insert_before_element(tb_pP, cursor_p, &rlc_pP->receiver_buffer);
			  return RLC_AM_DATA_PDU_STATUS_OK;
		  }

		  // Handle duplicate case
		  if (so_start_segment < pdu_info_cursor_p->so) {
			  // First Case : only duplicate at the end
			  // Scan for embedded segments to be discarded
			  next_cursor_p = cursor_p;
			  while ((next_cursor_p != NULL) && (pdu_info_cursor_p->sn == pdu_rx_info_p->sn) &&
					  (so_end_segment >= pdu_info_cursor_p->so + pdu_info_cursor_p->payload_size - 1)) {
				  /* Discard the segment */
				  cursor_p = next_cursor_p;
				  next_cursor_p = next_cursor_p->next;
				  list2_remove_element (cursor_p, &rlc_pP->receiver_buffer);
				  free_mem_block(cursor_p, __func__);

				  if (next_cursor_p != NULL) {
					  pdu_info_cursor_p = &((rlc_am_rx_pdu_management_t*)(next_cursor_p->data))->pdu_info;
				  }
			  }
			  cursor_p = next_cursor_p;

			  // Now remove duplicate at the end, only valid if cursor_p SN has the same received SN
			  if (cursor_p != NULL) {
				  pdu_info_cursor_p = &((rlc_am_rx_pdu_management_t*)(next_cursor_p->data))->pdu_info;

				  if ((pdu_info_cursor_p->sn == pdu_rx_info_p->sn) && (so_end_segment >= pdu_info_cursor_p->so)) {
					  so_end_segment = pdu_info_cursor_p->so - 1;
				  }
			  }
//Assertion(eNB)_PRAN_DesignDocument_annex No.793
              if((so_start_segment > so_end_segment) ||
                 (so_end_segment > pdu_rx_info_p->so + pdu_rx_info_p->payload_size - 1))
              {
                 LOG_E(RLC, " AM RX PDU Segment Duplicate elimination at the end error FirstSO!=0 SOStart=%d OldSOEnd=%d newSOEnd =%d SN=%d\n",
                       pdu_rx_info_p->so,pdu_rx_info_p->so + pdu_rx_info_p->payload_size - 1,so_end_segment,pdu_rx_info_p->sn);
                 return RLC_AM_DATA_PDU_STATUS_AM_SEGMENT_DUPLICATE;
              }
/*
			  AssertFatal((so_start_segment <= so_end_segment) &&
					  (so_end_segment <= pdu_rx_info_p->so + pdu_rx_info_p->payload_size - 1),
					  " AM RX PDU Segment Duplicate elimination at the end error FirstSO!=0 SOStart=%d OldSOEnd=%d newSOEnd =%d SN=%d\n",
					  pdu_rx_info_p->so,pdu_rx_info_p->so + pdu_rx_info_p->payload_size - 1,so_end_segment,pdu_rx_info_p->sn);
*/
		  }
		  else {
			  // Second Case: Duplicate at the begining and potentially at the end
			  /* Loop on stored segments and find the stored segment containing received SOStart */

			  previous_cursor_p = cursor_p;
			  pdu_info_previous_cursor_p = pdu_info_cursor_p;
			  while ((cursor_p != NULL) && (pdu_info_cursor_p->sn == pdu_rx_info_p->sn)
					  && ((pdu_info_cursor_p->so + pdu_info_cursor_p->payload_size) <= so_start_segment)) {
				  previous_cursor_p = cursor_p;
				  pdu_info_previous_cursor_p = pdu_info_cursor_p;
				  cursor_p = cursor_p->next;
				  if (cursor_p != NULL) {
					  pdu_info_cursor_p = &((rlc_am_rx_pdu_management_t*)(cursor_p->data))->pdu_info;
				  }
			  }

			  // Most likely case : no duplicate, the segment is put after all stored segments which are contiguous
			  if ((cursor_p == NULL) || (pdu_info_cursor_p->sn != pdu_rx_info_p->sn) || (pdu_info_cursor_p->so > so_end_segment)) {
	              LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[PROCESS RX PDU SEGMENT SN=%d SOSTART=%d] PDU SEGMENT INSERTED AFTER PDU SEGMENT WITH SOEND=%d\n",
	                          PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),pdu_rx_info_p->sn,so_start_segment,
							  pdu_info_previous_cursor_p->so + pdu_info_previous_cursor_p->payload_size - 1);

				  list2_insert_after_element(tb_pP, previous_cursor_p, &rlc_pP->receiver_buffer);
				  return RLC_AM_DATA_PDU_STATUS_OK;
			  }

			  // Now look for contiguous segments to check whether the received segment is not fully duplicate
			  so_start_min = pdu_info_previous_cursor_p->so;
			  so_end = so_start_min + pdu_info_previous_cursor_p->payload_size;
			  cursor_p = previous_cursor_p->next;
			  if (cursor_p != NULL) {
				  pdu_info_cursor_p = &((rlc_am_rx_pdu_management_t*)(cursor_p->data))->pdu_info;
			  }
			  while ((cursor_p != NULL) && (pdu_info_cursor_p->sn == pdu_rx_info_p->sn)
					  && (pdu_info_cursor_p->so == so_end)) {
				  previous_cursor_p = cursor_p;
				  pdu_info_previous_cursor_p = pdu_info_cursor_p;
				  so_end += pdu_info_cursor_p->payload_size;
				  cursor_p = cursor_p->next;
				  if (cursor_p != NULL) {
					  pdu_info_cursor_p = &((rlc_am_rx_pdu_management_t*)(cursor_p->data))->pdu_info;
				  }
			  }

			  /* Now discard the PDU segment if it is within so_start_min and so_end */
			  if ((so_start_min <= so_start_segment) && (so_end_segment < so_end)) {
				  LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[PROCESS RX PDU SEGMENT]  DISCARD : DUPLICATE SEGMENT SN=%d\n",
								  PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),pdu_rx_info_p->sn);
				  return RLC_AM_DATA_PDU_STATUS_AM_SEGMENT_DUPLICATE;
			  }

			  //Remove duplicate at the begining
			  if (so_start_segment < pdu_info_previous_cursor_p->so) {
				  so_start_segment = pdu_info_previous_cursor_p->so + pdu_info_previous_cursor_p->payload_size;
			  }
			  else if (so_start_segment < pdu_info_previous_cursor_p->so + pdu_info_previous_cursor_p->payload_size){
				  so_start_segment += (pdu_info_previous_cursor_p->so + pdu_info_previous_cursor_p->payload_size - so_start_segment);
			  }


			  // Now Scan for embedded segments to be discarded
			  next_cursor_p = cursor_p;
			  while ((next_cursor_p != NULL) && (pdu_info_cursor_p->sn == pdu_rx_info_p->sn) &&
					  (so_end_segment >= pdu_info_cursor_p->so + pdu_info_cursor_p->payload_size - 1)) {
				  /* Discard the segment */
				  cursor_p = next_cursor_p;
				  next_cursor_p = next_cursor_p->next;
				  list2_remove_element (cursor_p, &rlc_pP->receiver_buffer);
				  free_mem_block(cursor_p, __func__);

				  if (next_cursor_p != NULL) {
					  pdu_info_cursor_p = &((rlc_am_rx_pdu_management_t*)(next_cursor_p->data))->pdu_info;
				  }
			  }
			  cursor_p = next_cursor_p;

			  // Now remove duplicate at the end, only valid if cursor_p SN has the same received SN
			  if (cursor_p != NULL) {
				  pdu_info_cursor_p = &((rlc_am_rx_pdu_management_t*)(next_cursor_p->data))->pdu_info;

				  if ((pdu_info_cursor_p->sn == pdu_rx_info_p->sn) && (so_end_segment >= pdu_info_cursor_p->so)) {
					  so_end_segment = pdu_info_cursor_p->so - 1;
				  }
			  }
//Assertion(eNB)_PRAN_DesignDocument_annex No.794
      if((so_start_segment > so_end_segment) || (pdu_rx_info_p->so > so_start_segment) ||
         (so_end_segment > pdu_rx_info_p->so + pdu_rx_info_p->payload_size - 1))
      {
         LOG_E(RLC, " AM RX PDU Segment Duplicate elimination error FirstSO!=0 OldSOStart=%d OldSOEnd=%d newSOStart=%d newSOEnd =%d SN=%d\n",
              pdu_rx_info_p->so,pdu_rx_info_p->so + pdu_rx_info_p->payload_size - 1,so_start_segment,so_end_segment,pdu_rx_info_p->sn);
         return RLC_AM_DATA_PDU_STATUS_AM_SEGMENT_DUPLICATE;
      }
/*
			  AssertFatal((so_start_segment <= so_end_segment) && (pdu_rx_info_p->so <= so_start_segment) &&
					  (so_end_segment <= pdu_rx_info_p->so + pdu_rx_info_p->payload_size - 1),
					  " AM RX PDU Segment Duplicate elimination error FirstSO!=0 OldSOStart=%d OldSOEnd=%d newSOStart=%d newSOEnd =%d SN=%d\n",
					  pdu_rx_info_p->so,pdu_rx_info_p->so + pdu_rx_info_p->payload_size - 1,so_start_segment,so_end_segment,pdu_rx_info_p->sn);
*/
		  }

	  } // end pdu_info_cursor_p->so != 0


	  /* Last step :  duplicate bytes had been removed, build a new PDU segment */
//Assertion(eNB)_PRAN_DesignDocument_annex No.795
      if((pdu_rx_info_p->so == so_start_segment) && (so_end_segment == pdu_rx_info_p->so + pdu_rx_info_p->payload_size - 1))
      {
         LOG_E(RLC, " AM RX PDU Segment Duplicate elimination error FirstSO!=0 OldSOStart=%d OldSOEnd=%d newSOStart=%d newSOEnd =%d SN=%d\n",
              pdu_rx_info_p->so,pdu_rx_info_p->so + pdu_rx_info_p->payload_size - 1,so_start_segment,so_end_segment,pdu_rx_info_p->sn);
         return RLC_AM_DATA_PDU_STATUS_AM_SEGMENT_DUPLICATE;
      }
/*
	  AssertFatal((pdu_rx_info_p->so != so_start_segment) || (so_end_segment != pdu_rx_info_p->so + pdu_rx_info_p->payload_size - 1),
			  " AM RX PDU Segment Duplicate elimination error FirstSO!=0 OldSOStart=%d OldSOEnd=%d newSOStart=%d newSOEnd =%d SN=%d\n",
			  pdu_rx_info_p->so,pdu_rx_info_p->so + pdu_rx_info_p->payload_size - 1,so_start_segment,so_end_segment,pdu_rx_info_p->sn);
*/
	  mem_block_t* trunc_segment = create_new_segment_from_pdu(tb_pP,so_start_segment - pdu_rx_info_p->so,so_end_segment - so_start_segment + 1);
	  if (trunc_segment != NULL) {
		  LOG_I(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[PROCESS RX PDU SEGMENT]  CREATE SEGMENT FROM SEGMENT OFFSET=%d DATA LENGTH=%d SN=%d\n",
						  PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),so_start_segment - pdu_rx_info_p->so,so_end_segment - so_start_segment + 1,pdu_rx_info_p->sn);

		  if (previous_cursor_p != NULL) {
			  list2_insert_after_element(trunc_segment, previous_cursor_p, &rlc_pP->receiver_buffer);
		  }
		  else {
			  list2_insert_before_element(trunc_segment, rlc_pP->receiver_buffer.head, &rlc_pP->receiver_buffer);
		  }

		  /* Free original PDU Segment */
		  free_mem_block(tb_pP, __func__);

		  return RLC_AM_DATA_PDU_STATUS_OK;
	  }
	  else {
		  return RLC_AM_DATA_PDU_STATUS_BUFFER_FULL;
	  }

}

rlc_am_rx_pdu_status_t rlc_am_rx_list_handle_pdu(
		const protocol_ctxt_t* const  ctxt_pP,
		rlc_am_entity_t* const rlc_pP,
		mem_block_t* const tb_pP)
{
	  rlc_am_pdu_info_t* pdu_rx_info_p                  = &((rlc_am_rx_pdu_management_t*)(tb_pP->data))->pdu_info;
	  rlc_am_pdu_info_t* pdu_info_cursor_p           = NULL;
	  mem_block_t*       cursor_p                    = rlc_pP->receiver_buffer.head;
	  mem_block_t*       previous_cursor_p           = NULL;
	  rlc_am_rx_pdu_status_t pdu_status = RLC_AM_DATA_PDU_STATUS_OK;
	  // it is assumed this pdu is in rx window


	  /*****************************************************/
	  // 1) Find previous cursor to the PDU to insert
	  /*****************************************************/
//Assertion(eNB)_PRAN_DesignDocument_annex No.788
      if(cursor_p == NULL)
      {
         LOG_E(RLC, "AM Rx PDU Error, received buffer empty LcID=%d\n",rlc_pP->channel_id);
         return RLC_AM_DATA_PDU_STATUS_HEADER_ERROR;
      }
/*
	  AssertFatal(cursor_p != NULL,"AM Rx PDU Error, received buffer empty LcID=%d\n",rlc_pP->channel_id);
*/
	  do {
		  pdu_info_cursor_p = &((rlc_am_rx_pdu_management_t*)(cursor_p->data))->pdu_info;

		  // Stop if Cursor SN >= Received SN
		  if (RLC_AM_DIFF_SN(pdu_info_cursor_p->sn,rlc_pP->vr_r) >= RLC_AM_DIFF_SN(pdu_rx_info_p->sn,rlc_pP->vr_r)) {
			  break;
		  }

          previous_cursor_p = cursor_p;
          cursor_p = cursor_p->next;
	  } while (cursor_p != NULL);

	  /*****************************************************/
	  // 2) Insert PDU by removing byte duplicate if required
	  /*****************************************************/
	  // First case : cursor_p is NULL or SN are different, it means the SN is received for the first time
	  // Insert PDU after previous_cursor_p
	  if ((cursor_p == NULL) || (pdu_info_cursor_p->sn != pdu_rx_info_p->sn)) {
	      if (previous_cursor_p != NULL) {
	            LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[PROCESS RX PDU SN=%d] PDU INSERTED AFTER PDU SN=%d\n",
	                        PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),pdu_rx_info_p->sn,
	                        ((rlc_am_rx_pdu_management_t*)(previous_cursor_p->data))->pdu_info.sn);
	          list2_insert_after_element(tb_pP, previous_cursor_p, &rlc_pP->receiver_buffer);
	      }
	      else { /* SN of head of Rx PDU list is higher than received PDU SN */
              LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[PROCESS RX PDU SN=%d] PDU INSERTED BEFORE PDU SN=%d\n",
                            PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),pdu_rx_info_p->sn,
                            pdu_info_cursor_p->sn);
	          list2_insert_before_element(tb_pP, cursor_p, &rlc_pP->receiver_buffer);
	      }

		  return pdu_status;
	  }

	  // Filter out SN duplicate
	  // SN of received PDU has already data stored
	  rlc_am_rx_pdu_management_t * pdu_cursor_mgnt_p = (rlc_am_rx_pdu_management_t *) (cursor_p->data);

	  if (pdu_cursor_mgnt_p->all_segments_received) {
		  return RLC_AM_DATA_PDU_STATUS_SN_DUPLICATE;
	  }

	  /* check if the received PDU is equal to vrR */
	  if ((pdu_rx_info_p->sn != rlc_pP->vr_r) || (pdu_info_cursor_p->so != 0)) {
		  /* The full received PDU can replace the allready received PDU Segments. */
		  /* clean them and append this PDU */
		  mem_block_t*       cursor_next_p = cursor_p;
		  while (cursor_next_p) {
			  cursor_p = cursor_next_p;
			  cursor_next_p = cursor_next_p->next;
			  list2_remove_element (cursor_p, &rlc_pP->receiver_buffer);
			  free_mem_block(cursor_p, __func__);
			  if (cursor_next_p != NULL) {
				  pdu_info_cursor_p = &((rlc_am_rx_pdu_management_t*)(cursor_next_p->data))->pdu_info;
				  if (pdu_info_cursor_p->sn != pdu_rx_info_p->sn) {
					  break;
				  }
			  }
		  }

	      LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[PROCESS RX PDU]  PDU REPLACES STORED PDU SEGMENTS SN=%d\n",
	              PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),pdu_rx_info_p->sn);

	      if (previous_cursor_p != NULL) {
	          list2_insert_after_element(tb_pP, previous_cursor_p, &rlc_pP->receiver_buffer);
	      }
	      else {
	          list2_insert_before_element(tb_pP, cursor_next_p, &rlc_pP->receiver_buffer);
	      }

		  return pdu_status;
	  } // End SN != vrR or SO != 0
	  else {
		  /* First update cursor until discontinuity */
		  previous_cursor_p = cursor_p;
//Assertion(eNB)_PRAN_DesignDocument_annex No.789
          if(pdu_info_cursor_p->rf == 0)
          {
             LOG_E(RLC, "AM Rx PDU Error, stored SN=%d should be a PDU segment\n",pdu_info_cursor_p->sn);
             return RLC_AM_DATA_PDU_STATUS_HEADER_ERROR;
          }
/*
		  AssertFatal(pdu_info_cursor_p->rf != 0,"AM Rx PDU Error, stored SN=%d should be a PDU segment\n",pdu_info_cursor_p->sn);
*/
//Assertion(eNB)_PRAN_DesignDocument_annex No.790
          if(((rlc_am_rx_pdu_management_t *) (cursor_p->data))->all_segments_received != 0)
          {
             LOG_E(RLC, "AM Rx PDU Error, stored SN=%d already fully received\n",pdu_info_cursor_p->sn);
             return RLC_AM_DATA_PDU_STATUS_SN_DUPLICATE;
          }
/*
		  AssertFatal(((rlc_am_rx_pdu_management_t *) (cursor_p->data))->all_segments_received == 0,
				  "AM Rx PDU Error, stored SN=%d already fully received\n",pdu_info_cursor_p->sn);
*/
		  sdu_size_t          next_waited_so = 0;
		  while ((cursor_p != NULL) && (pdu_info_cursor_p->sn == pdu_rx_info_p->sn) && (pdu_info_cursor_p->so == next_waited_so)) {
			  next_waited_so += pdu_info_cursor_p->payload_size;
	          previous_cursor_p = cursor_p;
	          cursor_p = cursor_p->next;
	          if (cursor_p != NULL) {
	        	  pdu_info_cursor_p = &((rlc_am_rx_pdu_management_t*)(cursor_p->data))->pdu_info;
	          }
		  }
		  /* Create a new PDU segment by removing first next_waited_so bytes */
		  mem_block_t* trunc_pdu = create_new_segment_from_pdu(tb_pP,next_waited_so,pdu_rx_info_p->payload_size - next_waited_so);

		  if (trunc_pdu != NULL) {
			  /* Insert PDU Segment */
			  list2_insert_after_element(trunc_pdu, previous_cursor_p, &rlc_pP->receiver_buffer);

		      LOG_I(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[PROCESS RX PDU]  CREATE PDU SEGMENT FROM PDU OFFSET =%d SN=%d\n",
		              PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),next_waited_so,pdu_rx_info_p->sn);

			  /* clean previous stored segments in duplicate */
			  if ((cursor_p != NULL) && (pdu_info_cursor_p->sn == pdu_rx_info_p->sn)) {
				  mem_block_t*       cursor_next_p = cursor_p;
				  while (cursor_next_p) {
					  cursor_p = cursor_next_p;
					  cursor_next_p = cursor_next_p->next;
					  list2_remove_element (cursor_p, &rlc_pP->receiver_buffer);
					  free_mem_block(cursor_p, __func__);
					  if (cursor_next_p != NULL) {
						  pdu_info_cursor_p = &((rlc_am_rx_pdu_management_t*)(cursor_next_p->data))->pdu_info;
						  if (pdu_info_cursor_p->sn != pdu_rx_info_p->sn) {
							  break;
						  }
					  }
				  }
			  }

			  /* Free original PDU */
			  free_mem_block(tb_pP, __func__);

			  return pdu_status;
		  }
		  else {
			  return RLC_AM_DATA_PDU_STATUS_BUFFER_FULL;
		  }
	  }
}

// returns 0 if success
// returns neg value if failure
//-----------------------------------------------------------------------------
rlc_am_rx_pdu_status_t
rlc_am_rx_list_check_duplicate_insert_pdu(
  const protocol_ctxt_t* const  ctxt_pP,
  rlc_am_entity_t* const rlc_pP,
  mem_block_t* const tb_pP)
{
	  rlc_am_pdu_info_t* pdu_rx_info_p                  = &((rlc_am_rx_pdu_management_t*)(tb_pP->data))->pdu_info;
	  mem_block_t*       cursor_p                    = NULL;
	  cursor_p = rlc_pP->receiver_buffer.head;
	  rlc_am_rx_pdu_status_t pdu_status = RLC_AM_DATA_PDU_STATUS_OK;
	  // it is assumed this pdu is in rx window

	  /* Init Reassembly status */
	  ((rlc_am_rx_pdu_management_t*)(tb_pP->data))->segment_reassembled = RLC_AM_RX_PDU_SEGMENT_REASSEMBLE_NO;

	  if (cursor_p == NULL)  {
		    LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[INSERT PDU] LINE %d RX PDU SN %04d (only inserted)\n",
		          PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
		          __LINE__,
				  pdu_rx_info_p->sn);
		    list2_add_head(tb_pP, &rlc_pP->receiver_buffer);
		    return pdu_status;
	  }


	  if (pdu_rx_info_p->rf == 0) { // Case normal PDU received
		  pdu_status = rlc_am_rx_list_handle_pdu(ctxt_pP,rlc_pP,tb_pP);
	  }
	  else {
		  pdu_status = rlc_am_rx_list_handle_pdu_segment(ctxt_pP,rlc_pP,tb_pP);
	  }

	  return pdu_status;
}

//-----------------------------------------------------------------------------
void
rlc_am_rx_check_all_byte_segments(
  const protocol_ctxt_t* const  ctxt_pP,
  rlc_am_entity_t* const rlc_pP,
  mem_block_t* const tb_pP)
{
  rlc_am_pdu_info_t  *pdu_info_p        = &((rlc_am_rx_pdu_management_t*)(tb_pP->data))->pdu_info;
  mem_block_t        *cursor_p        = NULL;
  mem_block_t        *first_cursor_p  = NULL;
  rlc_sn_t            sn              = pdu_info_p->sn;
  sdu_size_t          next_waited_so;
  sdu_size_t          last_end_so;

  //msg("rlc_am_rx_check_all_byte_segments(%d) @0\n",sn);
  if (pdu_info_p->rf == 0) {
    ((rlc_am_rx_pdu_management_t*)(tb_pP->data))->all_segments_received = 1;
    return;
  }

  // for re-segmented AMD PDUS
  cursor_p = tb_pP;
  //list2_init(&list, NULL);
  //list2_add_head(cursor_p, &list);
  //msg("rlc_am_rx_check_all_byte_segments(%d) @1\n",sn);

  // get all previous PDU with same SN
  while (cursor_p->previous != NULL) {
    if (((rlc_am_rx_pdu_management_t*)(cursor_p->previous->data))->pdu_info.sn == sn) {
      //list2_add_head(cursor_p->previous, &list);
      cursor_p = cursor_p->previous;
      //msg("rlc_am_rx_check_all_byte_segments(%d) @2\n",sn);
    } else {
      break;
    }
  }

  // in case all first segments up to tb_pP are in list
  // the so field of the first PDU should be 0
  //cursor_p = list.head;
  //we start from the first stored PDU segment of this SN
  pdu_info_p = &((rlc_am_rx_pdu_management_t*)(cursor_p->data))->pdu_info;

  // if the first segment does not have SO = 0 then no need to continue
  if (pdu_info_p->so != 0) {
    return;
  }

  //msg("rlc_am_rx_check_all_byte_segments(%d) @3\n",sn);
  next_waited_so = pdu_info_p->payload_size;
  first_cursor_p = cursor_p;
  // then check if all segments are contiguous
  last_end_so = pdu_info_p->payload_size;

  while (cursor_p->next != NULL) {
    //msg("rlc_am_rx_check_all_byte_segments(%d) @4\n",sn);
    cursor_p = cursor_p->next;
    pdu_info_p = &((rlc_am_rx_pdu_management_t*)(cursor_p->data))->pdu_info;

    if (pdu_info_p->sn == sn) {
      // extra check normally not necessary
      if (
        !(pdu_info_p->rf == 1) ||
        !(pdu_info_p->so <= last_end_so)
      ) {
        //msg("rlc_am_rx_check_all_byte_segments(%d) @5 pdu_info_p->rf %d pdu_info_p->so %d\n",sn, pdu_info_p->rf, pdu_info_p->so);
        return;
      } else {
        if (pdu_info_p->so == next_waited_so) {
          next_waited_so = next_waited_so + pdu_info_p->payload_size;
          //msg("rlc_am_rx_check_all_byte_segments(%d) @6\n",sn);
        } else { // assumed pdu_info_p->so + pdu_info_p->payload_size > next_waited_so
          //next_waited_so = (next_waited_so + pdu_info_p->payload_size) - (next_waited_so - pdu_info_p->so);
          //msg("rlc_am_rx_check_all_byte_segments(%d) @7\n",sn);
        	return;
        }

        if (pdu_info_p->lsf > 0) {
          //msg("rlc_am_rx_check_all_byte_segments(%d) @8\n",sn);
          rlc_am_rx_mark_all_segments_received(ctxt_pP, rlc_pP,  first_cursor_p);
          return;
        }
      }

      last_end_so = pdu_info_p->so + pdu_info_p->payload_size;
    } else {
      //msg("rlc_am_rx_check_all_byte_segments(%d) @9\n",sn);
      return;
    }
  }
}
//-----------------------------------------------------------------------------
void
rlc_am_rx_mark_all_segments_received(
  const protocol_ctxt_t* const  ctxt_pP,
  rlc_am_entity_t* const        rlc_pP,
  mem_block_t* const            fisrt_segment_tbP)
{
  rlc_am_pdu_info_t* pdu_info_p          = &((rlc_am_rx_pdu_management_t*)(fisrt_segment_tbP->data))->pdu_info;
  rlc_am_pdu_info_t* pdu_info_cursor_p = NULL;
  mem_block_t*       cursor_p          = NULL;
  rlc_sn_t           sn                = pdu_info_p->sn;

  cursor_p = fisrt_segment_tbP;

  if (cursor_p) {
    LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[PROCESS RX PDU] ALL SEGMENTS RECEIVED SN %04d:\n",
          PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
          sn);

    do {
      pdu_info_cursor_p = &((rlc_am_rx_pdu_management_t*)(cursor_p->data))->pdu_info;

      if (pdu_info_cursor_p->sn == sn) {
        ((rlc_am_rx_pdu_management_t*)(cursor_p->data))->all_segments_received = 1;
      } else {
        return;
      }

      cursor_p = cursor_p->next;
    } while (cursor_p != NULL);
  }
}
//-----------------------------------------------------------------------------
//#define		RLC_AM_DEBUG_REASSEMBLY
void
rlc_am_rx_list_reassemble_rlc_sdus(
  const protocol_ctxt_t* const  ctxt_pP,
  rlc_am_entity_t* const        rlc_pP)
{
  mem_block_t*                cursor_p                     	= NULL;
  rlc_am_rx_pdu_management_t* rlc_am_rx_old_pdu_management 	= NULL;
  rlc_am_pdu_info_t* pdu_info_p								= NULL;

  cursor_p = list2_get_head(&rlc_pP->receiver_buffer);

  if (cursor_p == NULL) {
    return;
  }

  rlc_am_rx_pdu_management_t* rlc_am_rx_pdu_management_p = ((rlc_am_rx_pdu_management_t*)(cursor_p->data));
  pdu_info_p	= &((rlc_am_rx_pdu_management_t*)(cursor_p->data))->pdu_info;
#ifdef RLC_AM_DEBUG_REASSEMBLY
  rlc_usn_t sn_reass_start = pdu_info_p->sn;
#endif

  /* Specific process for the first SN if all PDU segments had been reassembled but not freed */
  if ((rlc_am_rx_pdu_management_p->all_segments_received > 0) && (pdu_info_p->rf != 0)) {
	  rlc_sn_t sn = pdu_info_p->sn;
	  while ((cursor_p != NULL) && (((rlc_am_rx_pdu_management_t*)(cursor_p->data))->segment_reassembled == RLC_AM_RX_PDU_SEGMENT_REASSEMBLED)
			  && ((rlc_am_rx_pdu_management_t*)(cursor_p->data))->pdu_info.sn == sn) {
	      cursor_p = list2_remove_head(&rlc_pP->receiver_buffer);
	      free_mem_block(cursor_p, __func__);
	      cursor_p = list2_get_head(&rlc_pP->receiver_buffer);
	  }

	  /* Reset Management pointers */
	  if (cursor_p != NULL) {
		  rlc_am_rx_pdu_management_p = ((rlc_am_rx_pdu_management_t*)(cursor_p->data));
		  /* Next SN must be the same or SN+1 */
		  if (RLC_AM_DIFF_SN(rlc_am_rx_pdu_management_p->pdu_info.sn,sn) > 1) {
			  return;
		  }
	  }
	  else {
		  return;
	  }
  }

  do {
    if (rlc_am_rx_pdu_management_p->all_segments_received > 0) {
      cursor_p = list2_remove_head(&rlc_pP->receiver_buffer);
      rlc_am_reassemble_pdu(ctxt_pP, rlc_pP, cursor_p,TRUE);
      rlc_am_rx_old_pdu_management = rlc_am_rx_pdu_management_p;
      cursor_p = list2_get_head(&rlc_pP->receiver_buffer);

      if (cursor_p == NULL) {
#ifdef RLC_AM_DEBUG_REASSEMBLY
    	  LOG_D(RLC, "RLC AM REASSEMBLY from sn=%d to ALL, vrR=%d vrMS=%d\n",
    			  sn_reass_start,rlc_pP->vr_r,rlc_pP->vr_ms);
#endif
        return;
      }

      rlc_am_rx_pdu_management_p = ((rlc_am_rx_pdu_management_t*)(cursor_p->data));
    }
    else if (rlc_am_rx_pdu_management_p->segment_reassembled == RLC_AM_RX_PDU_SEGMENT_REASSEMBLE_PENDING) {
    	rlc_am_rx_pdu_management_p->segment_reassembled = RLC_AM_RX_PDU_SEGMENT_REASSEMBLED;

        rlc_am_reassemble_pdu(ctxt_pP, rlc_pP, cursor_p,FALSE);
        rlc_am_rx_old_pdu_management = rlc_am_rx_pdu_management_p;
        cursor_p = cursor_p->next;

        if (cursor_p == NULL) {
#ifdef RLC_AM_DEBUG_REASSEMBLY
      	  LOG_D(RLC, "RLC AM REASSEMBLY from sn=%d to ALL, Last is Segment, vrR=%d vrMS=%d\n",
      			  sn_reass_start,rlc_pP->vr_r,rlc_pP->vr_ms);
#endif
          return;
        }

        rlc_am_rx_pdu_management_p = ((rlc_am_rx_pdu_management_t*)(cursor_p->data));
    }
    else if (rlc_am_rx_pdu_management_p->segment_reassembled == RLC_AM_RX_PDU_SEGMENT_REASSEMBLED) {
        rlc_am_rx_old_pdu_management = rlc_am_rx_pdu_management_p;
        cursor_p = cursor_p->next;

        if (cursor_p == NULL) {
          return;
        }

        rlc_am_rx_pdu_management_p = ((rlc_am_rx_pdu_management_t*)(cursor_p->data));
    }
    else {
#if RLC_STOP_ON_LOST_PDU

      if (list2_get_head(&rlc_pP->receiver_buffer) != cursor_p) {
        AssertFatal( 0 == 1,
                     PROTOCOL_RLC_AM_CTXT_FMT" LOST PDU DETECTED\n",
                     PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP));
      }

#endif
#ifdef RLC_AM_DEBUG_REASSEMBLY
	  LOG_D(RLC, "RLC AM REASSEMBLY from sn=%d to ALL, vrR=%d vrMS=%d\n",
			  sn_reass_start,rlc_pP->vr_r,rlc_pP->vr_ms);
#endif
      return;
    }

  } while (((RLC_AM_DIFF_SN(rlc_am_rx_pdu_management_p->pdu_info.sn,rlc_am_rx_old_pdu_management->pdu_info.sn) < 2) && (rlc_am_rx_old_pdu_management->all_segments_received > 0))
           || ((rlc_am_rx_pdu_management_p->pdu_info.sn == rlc_am_rx_old_pdu_management->pdu_info.sn) && (rlc_am_rx_pdu_management_p->segment_reassembled != RLC_AM_RX_PDU_SEGMENT_REASSEMBLE_NO)));

#ifdef RLC_AM_DEBUG_REASSEMBLY
  mem_block_t* cursor_head_p = list2_get_head(&rlc_pP->receiver_buffer);
  if (cursor_head_p == NULL) {
      return;
    }

  rlc_am_pdu_info_t* pdu_info_head_p	= &((rlc_am_rx_pdu_management_t*)(cursor_head_p->data))->pdu_info;

  LOG_D(RLC, "RLC AM REASSEMBLY from sn=%d to sn=%d, next_sn=%d head sn=%d vrR=%d vrMS=%d\n",
		  sn_reass_start,rlc_am_rx_old_pdu_management->pdu_info.sn,rlc_am_rx_pdu_management_p->pdu_info.sn,pdu_info_head_p->sn,rlc_pP->vr_r,rlc_pP->vr_ms);
#endif

}
//-----------------------------------------------------------------------------
mem_block_t *
list2_insert_before_element (
  mem_block_t * element_to_insert_pP,
  mem_block_t * element_pP,
  list2_t * list_pP)
{
  if ((element_to_insert_pP != NULL) && (element_pP != NULL)) {
    list_pP->nb_elements = list_pP->nb_elements + 1;
    mem_block_t *previous = element_pP->previous;
    element_to_insert_pP->previous = previous;
    element_to_insert_pP->next     = element_pP;
    element_pP->previous           = element_to_insert_pP;

    if (previous != NULL) {
      previous->next = element_to_insert_pP;
    } else if (list_pP->head == element_pP) {
      list_pP->head = element_to_insert_pP;
    }

    return element_to_insert_pP;
  } else {
    //assert(2==1);
    LOG_E(RLC, "list2_insert_before_element error. element_to_insert_pP %p, element_pP %p\n", element_to_insert_pP,element_pP);
    return NULL;
  }
}
//-----------------------------------------------------------------------------
mem_block_t *
list2_insert_after_element (
  mem_block_t * element_to_insert_pP,
  mem_block_t * element_pP,
  list2_t * list_pP)
{

  if ((element_to_insert_pP != NULL) && (element_pP != NULL)) {
    list_pP->nb_elements = list_pP->nb_elements + 1;
    mem_block_t *next = element_pP->next;
    element_to_insert_pP->previous = element_pP;
    element_to_insert_pP->next     = next;
    element_pP->next               = element_to_insert_pP;

    if (next != NULL) {
      next->previous = element_to_insert_pP;
    } else if (list_pP->tail == element_pP) {
      list_pP->tail = element_to_insert_pP;
    }

    return element_to_insert_pP;
  } else {
    //assert(2==1);
    LOG_E(RLC, "list2_insert_after_element error. element_to_insert_pP %p, element_pP %p\n", element_to_insert_pP,element_pP);
    return NULL;
  }
}
//-----------------------------------------------------------------------------
void
rlc_am_rx_list_display (
  const rlc_am_entity_t* const rlc_pP,
  char* message_pP)
{
  mem_block_t      *cursor_p = NULL;
  unsigned int      loop     = 0;

  cursor_p = rlc_pP->receiver_buffer.head;

  if (message_pP) {
    LOG_T(RLC, "Display list %s %s VR(R)=%04d:\n", rlc_pP->receiver_buffer.name, message_pP, rlc_pP->vr_r);
  } else {
    LOG_T(RLC, "Display list %s VR(R)=%04d:\n", rlc_pP->receiver_buffer.name, rlc_pP->vr_r);
  }

  if (cursor_p) {
    // almost one element
    while (cursor_p != NULL) {
      //if (((loop % 16) == 0) && (loop > 0)) {
      if ((loop % 4) == 0) {
        LOG_T(RLC, "\nRX SN:\t");
      }

      if (((rlc_am_rx_pdu_management_t*)(cursor_p->data))->pdu_info.rf) {
        if (((rlc_am_rx_pdu_management_t*)(cursor_p->data))->pdu_info.lsf) {
          LOG_T(RLC, "%04d (%04d->%04d LSF)\t",
                ((rlc_am_rx_pdu_management_t*)(cursor_p->data))->pdu_info.sn,
                ((rlc_am_rx_pdu_management_t*)(cursor_p->data))->pdu_info.so,
                ((rlc_am_rx_pdu_management_t*)(cursor_p->data))->pdu_info.so + ((rlc_am_rx_pdu_management_t*)(cursor_p->data))->pdu_info.payload_size - 1);
        } else {
          LOG_T(RLC, "%04d (%04d->%04d)\t",
                ((rlc_am_rx_pdu_management_t*)(cursor_p->data))->pdu_info.sn,
                ((rlc_am_rx_pdu_management_t*)(cursor_p->data))->pdu_info.so,
                ((rlc_am_rx_pdu_management_t*)(cursor_p->data))->pdu_info.so + ((rlc_am_rx_pdu_management_t*)(cursor_p->data))->pdu_info.payload_size - 1);
        }
      } else {
        LOG_T(RLC, "%04d (%04d NOSEG)\t",
              ((rlc_am_rx_pdu_management_t*)(cursor_p->data))->pdu_info.sn,
              ((rlc_am_rx_pdu_management_t*)(cursor_p->data))->pdu_info.payload_size);
      }

      //if (cursor_p == cursor_p->next) {
      //    rlc_am_v9_3_0_test_print_trace();
      //}
      //assert(cursor_p != cursor_p->next);
      if(cursor_p == cursor_p->next)
      {
        LOG_E(RLC, "rlc_am_rx_list_display error. cursor_p %p, cursor_p->next %p\n", cursor_p, cursor_p->next);
        break;
      }
      cursor_p = cursor_p->next;
      loop++;
    }

    LOG_T(RLC, "\n");
  } else {
    LOG_T(RLC, "\nNO ELEMENTS\n");
  }
}
