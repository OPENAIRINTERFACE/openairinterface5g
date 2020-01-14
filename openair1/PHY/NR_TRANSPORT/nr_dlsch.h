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

/*! \file PHY/LTE_TRANSPORT/defs.h
* \brief data structures for PDSCH/DLSCH/PUSCH/ULSCH physical and transport channel descriptors (TX/RX)
* \author R. Knopp
* \date 2011
* \version 0.1
* \company Eurecom
* \email: raymond.knopp@eurecom.fr, florian.kaltenberger@eurecom.fr, oscar.tonelli@yahoo.it
* \note
* \warning
*/

#ifndef __NR_DLSCH__H
#define __NR_DLSCH__H

#include "PHY/defs_gNB.h"

void nr_get_time_domain_allocation_type(nfapi_nr_config_request_t config,
                                        nfapi_nr_dl_tti_pdcch_pdu dci_pdu,
                                        nfapi_nr_dl_tti_pdsch_pdu *pdsch_pdu);

void nr_check_time_alloc(uint8_t S, uint8_t L,nfapi_nr_dl_tti_pdsch_pdu_rel15_t *rel15,nfapi_nr_config_request_t *cfg);

uint16_t get_RIV(uint16_t rb_start, uint16_t L, uint16_t N_RB);

uint16_t get_SLIV(uint8_t S, uint8_t L);

uint8_t nr_get_S(uint8_t row_idx, uint8_t CP, uint8_t time_alloc_type, uint8_t dmrs_typeA_position);

void nr_get_rbg_parms(NR_BWP_PARMS* bwp, uint8_t config_type);

void nr_get_rbg_list(uint32_t bitmap, uint8_t n_rbg, uint8_t* rbg_list);


uint8_t nr_get_Qm(uint8_t Imcs, uint8_t table_idx);

uint32_t nr_get_code_rate(uint8_t Imcs, uint8_t table_idx);

void nr_pdsch_codeword_scrambling(uint8_t *in,
                                  uint32_t size,
                                  uint8_t q,
                                  uint32_t Nid,
                                  uint32_t n_RNTI,
                                  uint32_t* out);

void nr_fill_dlsch(PHY_VARS_gNB *gNB,
                   int frame,
                   int slot,
                   nfapi_nr_dl_tti_pdsch_pdu *pdsch_pdu,
                   unsigned char *sdu); 

uint8_t nr_generate_pdsch(NR_gNB_DLSCH_t *dlsch,
                          uint32_t ***pdsch_dmrs,
                          int32_t** txdataF,
                          int16_t amp,
                          int frame,
                          uint8_t slot,
                          NR_DL_FRAME_PARMS *frame_parms,
			  int xOverhead,
                          time_stats_t *dlsch_encoding_stats,
                          time_stats_t *dlsch_scrambling_stats,
                          time_stats_t *dlsch_modulation_stats,
			  time_stats_t *tinput,
			  time_stats_t *tprep,
			  time_stats_t *tparity,
			  time_stats_t *toutput,
			  time_stats_t *dlsch_rate_matching_stats,
			  time_stats_t *dlsch_interleaving_stats,
			  time_stats_t *dlsch_segmentation_stats);



void free_gNB_dlsch(NR_gNB_DLSCH_t **dlschptr,uint16_t N_RB);

void clean_gNB_dlsch(NR_gNB_DLSCH_t *dlsch);

void clean_gNB_ulsch(NR_gNB_ULSCH_t *ulsch);

int16_t find_nr_dlsch(uint16_t rnti, PHY_VARS_gNB *gNB,find_type_t type);

int nr_dlsch_encoding(unsigned char *a,int frame,
		      uint8_t slot,
		      NR_gNB_DLSCH_t *dlsch,
		      NR_DL_FRAME_PARMS* frame_parms,
		      time_stats_t *tinput,
		      time_stats_t *tprep,
		      time_stats_t *tparity,
		      time_stats_t *toutput,
		      time_stats_t *dlsch_rate_matching_stats,
		      time_stats_t *dlsch_interleaving_stats,
		      time_stats_t *dlsch_segmentation_stats);


void nr_emulate_dlsch_payload(uint8_t* payload, uint16_t size);

#endif
