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

/**********************************************************************
*
* FILENAME    :  dmrs.h
*
* MODULE      :  demodulation reference signals
*
* DESCRIPTION :  generation of dmrs sequences for NR 5G
*                3GPP TS 38.211
*
************************************************************************/

#ifndef DMRS_NR_H
#define DMRS_NR_H

#include "PHY/defs_nr_UE.h"
#include "PHY/types.h"
#include "PHY/NR_REFSIG/ss_pbch_nr.h"
#include "PHY/NR_REFSIG/pss_nr.h"
#include "PHY/NR_REFSIG/sss_nr.h"

/************** CODE GENERATION ***********************************/

/************** DEFINE ********************************************/


/************* STRUCTURES *****************************************/


/************** VARIABLES *****************************************/

/************** FUNCTION ******************************************/

int pseudo_random_sequence(int M_PN, uint32_t *c, uint32_t cinit);
void lte_gold_new(LTE_DL_FRAME_PARMS *frame_parms, uint32_t lte_gold_table[20][2][14], uint16_t Nid_cell);
void generate_dmrs_pbch(uint32_t dmrs_pbch_bitmap[DMRS_PBCH_I_SSB][DMRS_PBCH_N_HF][DMRS_BITMAP_SIZE], uint16_t Nid_cell);
uint16_t get_dmrs_freq_idx_ul(uint16_t n, uint8_t k_prime, uint8_t delta, uint8_t dmrs_type);

#undef EXTERN

#endif /* DMRS_NR_H */


