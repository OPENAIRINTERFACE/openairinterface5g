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

/*! \file nr_dlsch.h
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

void nr_pdsch_codeword_scrambling(uint8_t *in,
                                  uint32_t size,
                                  uint8_t q,
                                  uint32_t Nid,
                                  uint32_t n_RNTI,
                                  uint32_t* out);

void nr_fill_dlsch(processingData_L1tx_t *msgTx,
                   nfapi_nr_dl_tti_pdsch_pdu *pdsch_pdu,
                   unsigned char *sdu); 

void nr_generate_pdsch(processingData_L1tx_t *msgTx,
                       int frame,
                       int slot);

void clean_gNB_dlsch(NR_gNB_DLSCH_t *dlsch);

int16_t find_nr_dlsch(uint16_t rnti, PHY_VARS_gNB *gNB,find_type_t type);

NR_gNB_SCH_STATS_t *find_nr_dlsch_stats(uint16_t rnti, PHY_VARS_gNB *gNB,find_type_t type);

int nr_dlsch_encoding(PHY_VARS_gNB *gNB,
		      int frame,
		      uint8_t slot,
		      NR_DL_gNB_HARQ_t *harq,
		      NR_DL_FRAME_PARMS* frame_parms,
		      unsigned char * output,
		      time_stats_t *tinput,
		      time_stats_t *tprep,
		      time_stats_t *tparity,
		      time_stats_t *toutput,
		      time_stats_t *dlsch_rate_matching_stats,
		      time_stats_t *dlsch_interleaving_stats,
		      time_stats_t *dlsch_segmentation_stats);

void init_dlsch_tpool(uint8_t nun_dlsch_threads);
void nr_emulate_dlsch_payload(uint8_t* payload, uint16_t size);

void dump_pdsch_stats(FILE *fd,PHY_VARS_gNB *gNB);

#endif
