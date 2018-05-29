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

#include "defs.h"

void nr_init_pbch_dmrs(PHY_VARS_gNB* gNB)
{

  unsigned int n, x1, x2;
  unsigned char Nid, i_ssb, i_ssb2;
  unsigned char Lmax, l, n_hf, N_hf;
  nfapi_config_request_t *cfg = &gNB->gNB_config;
  NR_DL_FRAME_PARMS *fp = &gNB->frame_parms;

  Nid = cfg->sch_config.physical_cell_id.value;

  Lmax = (fp->dl_CarrierFreq < 3e9)? 4:8;
  N_hf = (Lmax == 4)? 2:1;

  for (n_hf = 0; n_hf < N_hf; n_hf++) {

    for (l = 0; l < Lmax ; l++) {
      i_ssb = l & (Lmax-1);
      i_ssb2 = (i_ssb<<2) + n_hf;

      x1 = 1 + (1<<31);
      x2 = (1<<11) * (i_ssb2 + 1) * ((Nid>>2) + 1) + (1<<6) * (i_ssb2 + 1) + (Nid&3);
      x2 = x2 ^ ((x2 ^ (x2>>1) ^ (x2>>2) ^ (x2>>3))<<31);

      // skip first 50 double words (1600 bits)
      for (n = 1; n < 50; n++) {
        x1 = (x1>>1) ^ (x1>>4);
        x1 = x1 ^ (x1<<31) ^ (x1<<28);
        x2 = (x2>>1) ^ (x2>>2) ^ (x2>>3) ^ (x2>>4);
        x2 = x2 ^ (x2<<31) ^ (x2<<30) ^ (x2<<29) ^ (x2<<28);
      }

      for (n=0; n<NR_PBCH_DMRS_LENGTH_DWORD; n++) {
        x1 = (x1>>1) ^ (x1>>4);
        x1 = x1 ^ (x1<<31) ^ (x1<<28);
        x2 = (x2>>1) ^ (x2>>2) ^ (x2>>3) ^ (x2>>4);
        x2 = x2 ^ (x2<<31) ^ (x2<<30) ^ (x2<<29) ^ (x2<<28);
        gNB->nr_gold_pbch_dmrs[n_hf][l][n] = x1 ^ x2;
      }

    }
  }

}
