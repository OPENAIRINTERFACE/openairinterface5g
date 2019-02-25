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

typedef unsigned __int128 uint128_t;

uint16_t nr_get_dci_size(nfapi_nr_dci_format_e format,
                         nfapi_nr_rnti_type_e rnti_type,
                         uint16_t N_RB,
                         nfapi_nr_config_request_t *config);

uint8_t nr_generate_dci_top(NR_gNB_PDCCH pdcch_vars,
                            uint32_t **gold_pdcch_dmrs,
                            int32_t *txdataF,
                            int16_t amp,
                            NR_DL_FRAME_PARMS frame_parms,
                            nfapi_nr_config_request_t config);

void nr_pdcch_scrambling(uint32_t *in,
                         uint16_t size,
                         uint32_t Nid,
                         uint32_t n_RNTI,
                         uint32_t *out);

void nr_fill_dci_and_dlsch(PHY_VARS_gNB *gNB,
                           int frame,
                           int subframe,
                           gNB_L1_rxtx_proc_t *proc,
                           NR_gNB_DCI_ALLOC_t *dci_alloc,
                           nfapi_nr_dl_config_dci_dl_pdu *pdu,
                           nfapi_nr_dl_config_dlsch_pdu *dlsch_pdu);

void nr_fill_cce_list(NR_gNB_DCI_ALLOC_t *dci_alloc, uint16_t n_shift, uint8_t m);


#endif //__PHY_NR_TRANSPORT_DCI__H
