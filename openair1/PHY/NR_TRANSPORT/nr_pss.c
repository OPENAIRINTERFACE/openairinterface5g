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

//#define NR_PSS_DEBUG

int nr_generate_pss(  c16_t *txdataF,
                      int16_t amp,
                      uint8_t ssb_start_symbol,
                      nfapi_nr_config_request_scf_t* config,
                      NR_DL_FRAME_PARMS *frame_parms)
{
  int16_t x[NR_PSS_LENGTH];
  const int x_initial[7] = {0, 1, 1 , 0, 1, 1, 1};

  /// Sequence generation
  for (int i=0; i < 7; i++)
    x[i] = x_initial[i];

  for (int i=0; i < (NR_PSS_LENGTH - 7); i++) {
    x[i+7] = (x[i + 4] + x[i]) %2;
  }

#ifdef NR_PSS_DEBUG
  write_output("d_pss.m", "d_pss", (void*)d_pss, NR_PSS_LENGTH, 1, 0);
  printf("PSS: ofdm_symbol_size %d, first_carrier_offset %d\n",frame_parms->ofdm_symbol_size,frame_parms->first_carrier_offset);
#endif

  /// Resource mapping

  // PSS occupies a predefined position (subcarriers 56-182, symbol 0) within the SSB block starting from
  int k = frame_parms->first_carrier_offset + frame_parms->ssb_start_subcarrier + 56; //and
  if (k>= frame_parms->ofdm_symbol_size) k-=frame_parms->ofdm_symbol_size;

  int l = ssb_start_symbol;

  uint8_t Nid2 = config->cell_config.phy_cell_id.value % 3;
  for (int i = 0; i < NR_PSS_LENGTH; i++) {
    int m = (i + 43*Nid2)%(NR_PSS_LENGTH);
    int16_t d_pss = (1 - 2*x[m]) * 23170;
    //      printf("pss: writing position k %d / %d\n",k,frame_parms->ofdm_symbol_size);
    ((int16_t*)txdataF)[2*(l*frame_parms->ofdm_symbol_size + k)] = (((int16_t)amp) * d_pss) >> 15;
    k++;

    if (k >= frame_parms->ofdm_symbol_size)
      k-=frame_parms->ofdm_symbol_size;
  }

#ifdef NR_PSS_DEBUG
  LOG_M("pss_0.m", "pss_0", 
	(void*)&txdataF[0][ssb_start_symbol*frame_parms->ofdm_symbol_size], 
	frame_parms->ofdm_symbol_size, 1, 1);
#endif

  return 0;
}
