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

#include "PHY/defs.h"
#include "PHY/extern.h"
#include "defs.h"
//#define DEBUG_FEP

int slot_fep_ul(LTE_DL_FRAME_PARMS *frame_parms,
                LTE_eNB_COMMON *eNB_common_vars,
                unsigned char l,
                unsigned char Ns,
                unsigned char eNB_id,
                int no_prefix)
{
#ifdef DEBUG_FEP
  char fname[40], vname[40];
#endif
  unsigned char aa;
  unsigned char symbol = l+((7-frame_parms->Ncp)*(Ns&1)); ///symbol within sub-frame
  unsigned int nb_prefix_samples = (no_prefix ? 0 : frame_parms->nb_prefix_samples);
  unsigned int nb_prefix_samples0 = (no_prefix ? 0 : frame_parms->nb_prefix_samples0);
  //  unsigned int subframe_offset;
  unsigned int slot_offset;

  void (*dft)(int16_t *,int16_t *, int);

  int tmp_dft_in[2048] __attribute__ ((aligned (32)));  // This is for misalignment issues for 6 and 15 PRBs
  unsigned int frame_length_samples = frame_parms->samples_per_tti * 10;
  unsigned int rx_offset;

  switch (frame_parms->ofdm_symbol_size) {
  case 128:
    dft = dft128;
    break;

  case 256:
    dft = dft256;
    break;

  case 512:
    dft = dft512;
    break;

  case 1024:
    dft = dft1024;
    break;

  case 1536:
    dft = dft1536;
    break;

  case 2048:
    dft = dft2048;
    break;

  default:
    dft = dft512;
    break;
  }

  if (no_prefix) {
    //    subframe_offset = frame_parms->ofdm_symbol_size * frame_parms->symbols_per_tti * (Ns>>1);
    slot_offset = frame_parms->ofdm_symbol_size * (frame_parms->symbols_per_tti>>1) * (Ns&1);
  } else {
    //    subframe_offset = frame_parms->samples_per_tti * (Ns>>1);
    slot_offset = (frame_parms->samples_per_tti>>1) * (Ns&1);
  }

  if (l<0 || l>=7-frame_parms->Ncp) {
    LOG_E(PHY,"slot_fep: l must be between 0 and %d\n",7-frame_parms->Ncp);
    return(-1);
  }

  if (Ns<0 || Ns>=20) {
    LOG_E(PHY,"slot_fep: Ns must be between 0 and 19\n");
    return(-1);
  }

#ifdef DEBUG_FEP
  LOG_D(PHY,"slot_fep: Ns %d offset %d, symbol %d, nb_prefix_samples %d\n",Ns,slot_offset,symbol, nb_prefix_samples);
#endif

  for (aa=0; aa<frame_parms->nb_antennas_rx; aa++) {
    rx_offset = slot_offset +nb_prefix_samples0;
    if (l==0) {

      dft( (int16_t *)&eNB_common_vars->rxdata_7_5kHz[eNB_id][aa][rx_offset],
           (int16_t *)&eNB_common_vars->rxdataF[eNB_id][aa][frame_parms->ofdm_symbol_size*symbol],
           1
         );
    } else {
      
      rx_offset += (frame_parms->ofdm_symbol_size+nb_prefix_samples)*l;
      /* should never happen for eNB
      if(rx_offset > (frame_length_samples - frame_parms->ofdm_symbol_size))
      {
        memcpy((void *)&eNB_common_vars->rxdata_7_5kHz[eNB_id][aa][frame_length_samples],
               (void *)&eNB_common_vars->rxdata_7_5kHz[eNB_id][aa][0],
               frame_parms->ofdm_symbol_size*sizeof(int));
      }
      */

      // check for 256-bit alignment of input buffer and do DFT directly, else do via intermediate buffer
      if( (rx_offset & 15) != 0){
        memcpy((void *)&tmp_dft_in,
	       (void *)&eNB_common_vars->rxdata_7_5kHz[eNB_id][aa][(rx_offset % frame_length_samples)],
	       frame_parms->ofdm_symbol_size*sizeof(int));
        dft( (short *) tmp_dft_in,
             (short*)  &eNB_common_vars->rxdataF[eNB_id][aa][frame_parms->ofdm_symbol_size*symbol],
             1
           );
      }
      else{
      dft( (short *)&eNB_common_vars->rxdata_7_5kHz[eNB_id][aa][rx_offset],
           (short*)&eNB_common_vars->rxdataF[eNB_id][aa][frame_parms->ofdm_symbol_size*symbol],
           1
         );
      }
    }
  }

#ifdef DEBUG_FEP
  LOG_D(PHY,"slot_fep: done\n");
#endif
  return(0);
}
