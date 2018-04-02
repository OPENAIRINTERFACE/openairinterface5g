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

#include "../defs_NR.h"

#define NR_PSS_DEBUG

short nr_mod_table[MOD_TABLE_SIZE_SHORT] = {0,0,768,768,-768,-768};

int nr_generate_pss(  int16_t *d_pss,
                      int32_t **txdataF,
                      int16_t amp,
                      int16_t ssb_first_subcarrier,
                      uint8_t slot_offset,
                      nfapi_param_t nfapi_params,
                      NR_DL_FRAME_PARMS *frame_parms)
{
  int i,n,m,k;
  int16_t a, aa;
  int16_t x[NR_PSS_LENGTH];
  int16_t pss_mod[2* NR_PSS_LENGTH];
  const int x_initial[7] = {0, 1, 1 , 0, 1, 1, 1};

  uint8_t Nid2 = nfapi_params.sch_config.physical_cell_id.value % 3;
  uint8_t Nsymb = (nfapi_params.subframe_config.dl_cyclic_prefix_type.value == 0)? 14 : 12;

  // Binary sequence generation
  for (i=0; i < 7; i++)
    x[i] = x_initial[i];

  for (i=0; i < (NR_PSS_LENGTH - 7); i++) {
    x[i+7] = (x[i + 4] + x[i]) %2;
  }

  for (n=0; n < NR_PSS_LENGTH; n++) {
    m = (n + 43*Nid2)%(NR_PSS_LENGTH);
    d_pss[n] = x[m]; // 1 - 2*x[m] is taken into account in the mod_table (binary input)
  }

  // BPSK modulation and resource mapping
  a = (nfapi_params.rf_config.tx_antenna_ports.value == 1) ? amp : (amp*ONE_OVER_SQRT2_Q15)>>15;
  for (i = 0; i <  NR_PSS_LENGTH; i++)
  {
    pss_mod[2*i] =  nr_mod_table[ 2 * (MOD_TABLE_BPSK_OFFSET + d_pss[i]) ];
    pss_mod[2*i + 1] = nr_mod_table[ 2 * (MOD_TABLE_BPSK_OFFSET + d_pss[i]) + 1];
  }

  for (aa = 0; aa < nfapi_params.rf_config.tx_antenna_ports.value; aa++)
  {

    // PSS occupies a predefined position (symbol 0, subcarriers 56-182) within the SSB block starting from
    k = frame_parms->first_carrier_offset + ssb_first_subcarrier + 56; // to be retrieved from ssb scheduling function

    for (m = 0; m < NR_PSS_LENGTH; m++) {
      ((int16_t*)txdataF[aa])[2*(slot_offset*Nsymb*frame_parms->ofdm_symbol_size + k)] = (a * pss_mod[2*m]) >> 15;
      ((int16_t*)txdataF[aa])[2*(slot_offset*Nsymb*frame_parms->ofdm_symbol_size + k) + 1] = (a * pss_mod[2*m + 1]) >> 15;
      k+=1;

      if (k >= frame_parms->ofdm_symbol_size) {
        k++; //skip DC
        k-=frame_parms->ofdm_symbol_size;
      }
    }
  }

  return (0);
}
