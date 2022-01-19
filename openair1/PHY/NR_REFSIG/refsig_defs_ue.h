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

/* Definitions for LTE Reference signals */
/* Author R. Knopp / EURECOM / OpenAirInterface.org */
#ifndef __NR_REFSIG_DEFS__H__
#define __NR_REFSIG_DEFS__H__

#include "PHY/defs_nr_UE.h"
#include "PHY/LTE_REFSIG/lte_refsig.h"


/*!\brief This function generates the NR Gold sequence (38-211, Sec 5.2.1) for the PBCH DMRS.
@param PHY_VARS_NR_UE* ue structure provides configuration, frame parameters and the pointers to the 32 bits sequence storage tables
 */
int nr_pbch_dmrs_rx(int dmrss,
                    unsigned int *nr_gold_pbch,
                    int32_t *output);

/*!\brief This function generates the NR Gold sequence (38-211, Sec 5.2.1) for the PDCCH DMRS.
@param PHY_VARS_NR_UE* ue structure provides configuration, frame parameters and the pointers to the 32 bits sequence storage tables
 */
int nr_pdcch_dmrs_rx(PHY_VARS_NR_UE *ue,
                     uint8_t eNB_offset,
                     unsigned int Ns,
                     unsigned int *nr_gold_pdcch,
                     int32_t *output,
                     unsigned short p,
                     unsigned short nb_rb_corset);

int nr_pdsch_dmrs_rx(PHY_VARS_NR_UE *ue,
                     unsigned int Ns,
                     unsigned int *nr_gold_pdsch,
                     int32_t *output,
                     unsigned short p,
                     unsigned char lp,
                     unsigned short nb_pdsch_rb,
                     uint8_t config_type);

void nr_gold_pbch(PHY_VARS_NR_UE* ue);

void nr_gold_pdcch(PHY_VARS_NR_UE* ue,
                   unsigned short n_idDMRS);

void nr_gold_pdsch(PHY_VARS_NR_UE* ue,
                   unsigned short *n_idDMRS);

void nr_init_pusch_dmrs(PHY_VARS_NR_UE* ue,
                        uint16_t *N_n_scid,
                        uint8_t n_scid);

void nr_gold_prs(PHY_VARS_NR_UE* ue);

#endif
