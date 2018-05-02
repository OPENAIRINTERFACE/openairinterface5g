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

/*! \file PHY/NR_TRANSPORT/nr_pbch.c
* \brief Top-level routines for generating and decoding  the PBCH/BCH physical/transport channel V15.1 03/2018
* \author G. De Souza
* \date 2018
* \version 0.1
* \company Eurecom
* \email: desouza@eurecom.fr
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
                          uint8_t nu,
                          nfapi_config_request_t* config,
                          NR_DL_FRAME_PARMS *frame_parms)
{
  int m,k,l;
  int a, aa;
  int16_t mod_dmrs[2 * NR_PBCH_DMRS_LENGTH];

  LOG_I(PHY, "PBCH DMRS mapping started at symbol %d shift %d\n", ssb_start_symbol+1, nu);

  /// BPSK modulation
  for (m=0; m<NR_PBCH_DMRS_LENGTH; m++) {
    mod_dmrs[m<<1] = nr_mod_table[((1 + ((gold_pbch_dmrs[m>>5]&(1<<(m&0x1f)))>>(m&0x1f)))<<1)];
    mod_dmrs[m<<1 + 1] = nr_mod_table[((1 + ((gold_pbch_dmrs[m>>5]&(1<<(m&0x1f)))>>(m&0x1f)))<<1) + 1];
#ifdef DEBUG_PBCH
  printf("m %d  mod_dmrs %d %d\n", m, mod_dmrs[2*m], mod_dmrs[2*m + 1]);
#endif
  }

  /// Resource mapping
  a = (config->rf_config.tx_antenna_ports.value == 1) ? amp : (amp*ONE_OVER_SQRT2_Q15)>>15;

  for (aa = 0; aa < config->rf_config.tx_antenna_ports.value; aa++)
  {

    // PBCH DMRS are mapped  within the SSB block on every fourth subcarrier starting from nu of symbols 1, 2, 3
      ///symbol 1  [0+nu:4:236+nu] -- 60 mod symbols
    k = frame_parms->first_carrier_offset + frame_parms->ssb_start_subcarrier + nu;
    l = ssb_start_symbol + 1;

    for (m = 0; m < 60; m++) {
#ifdef DEBUG_PBCH
  printf("Mapping m %d: %d  %d at k %d of l %d\n", m,(a * mod_dmrs[m<<1]) >> 15, (a * mod_dmrs[m<<1 + 1]) >> 15, k, l);
#endif
      ((int16_t*)txdataF[aa])[(l*frame_parms->ofdm_symbol_size + k)<<1] = (a * mod_dmrs[m<<1]) >> 15;
      ((int16_t*)txdataF[aa])[(l*frame_parms->ofdm_symbol_size + k)<<1 + 1] = (a * mod_dmrs[m<<1 + 1]) >> 15;
      k+=4;

      if (k >= frame_parms->ofdm_symbol_size)
        k-=frame_parms->ofdm_symbol_size;
    }

      ///symbol 2  [0+u:4:44+nu ; 192+nu:4:236+nu] -- 24 mod symbols
    k = frame_parms->first_carrier_offset + frame_parms->ssb_start_subcarrier + nu;
    l++;

    for (m = 60; m < 84; m++) {
#ifdef DEBUG_PBCH
  printf("Mapping m %d: %d  %d at k %d of l %d\n", m,(a * mod_dmrs[m<<1]) >> 15, (a * mod_dmrs[m<<1 + 1]) >> 15, k, l);
#endif
      ((int16_t*)txdataF[aa])[(l*frame_parms->ofdm_symbol_size + k)<<1] = (a * mod_dmrs[m<<1]) >> 15;
      ((int16_t*)txdataF[aa])[(l*frame_parms->ofdm_symbol_size + k)<<1 + 1] = (a * mod_dmrs[m<<1 + 1]) >> 15;
      k+=(m==71)?148:4; // Jump from 44+nu to 192+nu

      if (k >= frame_parms->ofdm_symbol_size)
        k-=frame_parms->ofdm_symbol_size;
    }

      ///symbol 3  [0+nu:4:236+nu] -- 60 mod symbols
    k = frame_parms->first_carrier_offset + frame_parms->ssb_start_subcarrier + nu;
    l++;

    for (m = 84; m < NR_PBCH_DMRS_LENGTH; m++) {
#ifdef DEBUG_PBCH
  printf("Mapping m %d: %d  %d at k %d of l %d\n", m,(a * mod_dmrs[m<<1]) >> 15, (a * mod_dmrs[m<<1 + 1]) >> 15, k, l);
#endif
      ((int16_t*)txdataF[aa])[(l*frame_parms->ofdm_symbol_size + k)<<1] = (a * mod_dmrs[m<<1]) >> 15;
      ((int16_t*)txdataF[aa])[(l*frame_parms->ofdm_symbol_size + k)<<1 + 1] = (a * mod_dmrs[m<<1 + 1]) >> 15;
      k+=4;

      if (k >= frame_parms->ofdm_symbol_size)
        k-=frame_parms->ofdm_symbol_size;
    }

  }


#ifdef DEBUG_PBCH
  write_output("txdataF.m", "txdataF", txdataF[0], frame_parms->samples_per_frame_wCP, 1, 1);
#endif
  return (0);
}
