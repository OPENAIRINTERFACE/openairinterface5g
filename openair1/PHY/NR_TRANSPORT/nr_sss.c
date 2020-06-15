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

#include "PHY/NR_TRANSPORT/nr_transport_proto.h"

//#define NR_SSS_DEBUG

int nr_generate_sss(  int16_t *d_sss,
                      int32_t *txdataF,
                      int16_t amp,
                      uint8_t ssb_start_symbol,
                      nfapi_nr_config_request_scf_t* config,
                      NR_DL_FRAME_PARMS *frame_parms)
{
  int i,k,l;
  int m0, m1;
  int Nid, Nid1, Nid2;
  //int16_t a;
  int16_t x0[NR_SSS_LENGTH], x1[NR_SSS_LENGTH];
  const int x0_initial[7] = { 1, 0, 0, 0, 0, 0, 0 };
  const int x1_initial[7] = { 1, 0, 0, 0, 0, 0, 0 };

  /// Sequence generation
  Nid = config->cell_config.phy_cell_id.value;
  Nid2 = Nid % 3;
  Nid1 = (Nid - Nid2)/3;

  for ( i=0 ; i < 7 ; i++) {
    x0[i] = x0_initial[i];
    x1[i] = x1_initial[i];
  }

  for ( i=0 ; i < NR_SSS_LENGTH - 7 ; i++) {
    x0[i+7] = (x0[i + 4] + x0[i]) % 2;
    x1[i+7] = (x1[i + 1] + x1[i]) % 2;
  }

  m0 = 15*(Nid1/112) + (5*Nid2);
  m1 = Nid1 % 112;

  for (i = 0; i < NR_SSS_LENGTH ; i++) {
    d_sss[i] = (1 - 2*x0[(i + m0) % NR_SSS_LENGTH] ) * (1 - 2*x1[(i + m1) % NR_SSS_LENGTH] ) * 23170;
  }

#ifdef NR_SSS_DEBUG
  write_output("d_sss.m", "d_sss", (void*)d_sss, NR_SSS_LENGTH, 1, 1);
#endif

  /// Resource mapping

    // SSS occupies a predefined position (subcarriers 56-182, symbol 2) within the SSB block starting from
    k = frame_parms->first_carrier_offset + frame_parms->ssb_start_subcarrier + 56; //and
    l = ssb_start_symbol + 2;

    for (int m = 0; m < NR_SSS_LENGTH; m++) {
      ((int16_t*)txdataF)[2*(l*frame_parms->ofdm_symbol_size + k)] = (amp * d_sss[m]) >> 15;
      k++;

      if (k >= frame_parms->ofdm_symbol_size)
        k-=frame_parms->ofdm_symbol_size;
    }
#ifdef NR_SSS_DEBUG
  //  write_output("sss_0.m", "sss_0", (void*)txdataF[0][l*frame_parms->ofdm_symbol_size], frame_parms->ofdm_symbol_size, 1, 1);
#endif

  return 0;
}
