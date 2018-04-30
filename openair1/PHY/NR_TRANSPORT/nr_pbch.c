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

/*! \file PHY/LTE_TRANSPORT/pbch.c
* \brief Top-level routines for generating and decoding  the PBCH/BCH physical/transport channel V8.6 2009-03
* \author R. Knopp, F. Kaltenberger
* \date 2011
* \version 0.1
* \company Eurecom
* \email: knopp@eurecom.fr,florian.kaltenberger.fr
* \note
* \warning
*/
#include "PHY/defs.h"
#include "PHY/CODING/extern.h"
#include "PHY/CODING/lte_interleaver_inline.h"
#include "defs.h"
#include "extern.h"
#include "PHY/extern.h"
#include "PHY/sse_intrin.h"

#define DEBUG_PBCH

short nr_mod_table[MOD_TABLE_SIZE_SHORT] = {0,0,23170,23170,-23170,-23170};

int nr_generate_pbch_dmrs(uint32_t *gold_pbch_dmrs,
                          int32_t **txdataF,
                          int16_t amp,
                          uint8_t ssb_start_symbol,
                          nfapi_config_request_t* config,
                          NR_DL_FRAME_PARMS *frame_parms)
{
  int m,k,l;
  int a, aa;
  uint16_t mod_dmrs[2 * NR_PBCH_DMRS_LENGTH];


  /// BPSK modulation
  for (m=0; m<NR_PBCH_DMRS_LENGTH; m++) {
    mod_dmrs[2*m] = nr_mod_table[2*(1 + (*(gold_pbch_dmrs + m/32))&(1<<(m&0xf)) )];
    mod_dmrs[2*m + 1] = nr_mod_table[2*(1 + (*(gold_pbch_dmrs + m/32))&(1<<(m&0xf)) ) + 1];
#ifdef DEBUG_PBCH
  printf("m %d  mod_dmrs %d %d", m, mod_dmrs[2*m], mod_dmrs[2*m + 1]);
#endif
  }

  /// Resource mapping
  a = (config->rf_config.tx_antenna_ports.value == 1) ? amp : (amp*ONE_OVER_SQRT2_Q15)>>15;

  for (aa = 0; aa < config->rf_config.tx_antenna_ports.value; aa++)
  {

    // PBCH DMRS are mapped
    k = frame_parms->first_carrier_offset + frame_parms->ssb_start_subcarrier + 56; //and
    l = ssb_start_symbol + 1;

    for (m = 0; m < NR_PBCH_DMRS_LENGTH; m++) {
      ((int16_t*)txdataF[aa])[2*(l*frame_parms->ofdm_symbol_size + k)] = (a * mod_dmrs[2*m]) >> 15;
      ((int16_t*)txdataF[aa])[2*(l*frame_parms->ofdm_symbol_size + k) + 1] = (a * mod_dmrs[2*m + 1]) >> 15;
#ifdef DEBUG_PBCH
  int idx = 2*(l*frame_parms->ofdm_symbol_size + k);
  printf("aa %d m %d  txdataF  %d %d %d", aa, m, txdataF[aa][idx], txdataF[aa][idx+1]);
#endif
      k++;

      if (k >= frame_parms->ofdm_symbol_size)
        k-=frame_parms->ofdm_symbol_size;
    }
  }
  return (0);
}
