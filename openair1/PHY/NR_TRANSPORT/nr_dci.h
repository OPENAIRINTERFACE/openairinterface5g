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

#ifndef __PHY_NR_TRANSPORT_DCI__H
#define __PHY_NR_TRANSPORT_DCI__H

#include "PHY/defs_gNB.h"
#include "PHY/NR_REFSIG/nr_refsig.h"

uint16_t nr_get_dci_size(nfapi_nr_dci_format_e format,
                         nfapi_nr_rnti_type_e rnti_type,
                         uint16_t N_RB);

void nr_generate_dci_top(processingData_L1tx_t *msgTx,
                         int slot,
                         int32_t *txdataF,
                         int16_t amp,
                         NR_DL_FRAME_PARMS *frame_parms);

void nr_pdcch_scrambling(uint32_t *in,
                         uint32_t size,
                         uint32_t Nid,
                         uint32_t n_RNTI,
                         uint32_t *out);

int16_t find_nr_pdcch(int frame,int slot, PHY_VARS_gNB *gNB,find_type_t type);

void nr_fill_dci(PHY_VARS_gNB *gNB,
                 int frame,
                 int slot,
		 nfapi_nr_dl_tti_pdcch_pdu *pdcch_pdu);

int16_t find_nr_ul_dci(int frame,int slot, PHY_VARS_gNB *gNB,find_type_t type);

void nr_fill_ul_dci(PHY_VARS_gNB *gNB,
		    int frame,
		    int slot,
		    nfapi_nr_ul_dci_request_pdus_t *pdcch_pdu);

void nr_fill_cce_list(nr_cce_t cce_list[MAX_DCI_CORESET][NR_MAX_PDCCH_AGG_LEVEL], nfapi_nr_dl_tti_pdcch_pdu_rel15_t *pdcch_pdu_rel15);

#endif //__PHY_NR_TRANSPORT_DCI__H
