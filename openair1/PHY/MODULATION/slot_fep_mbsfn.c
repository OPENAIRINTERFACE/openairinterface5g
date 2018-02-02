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
#include "defs.h"
//#define DEBUG_FEP

#define SOFFSET 0

int slot_fep_mbsfn(PHY_VARS_UE *ue,
                   unsigned char l,
                   int subframe,
                   int sample_offset,
                   int no_prefix)
{
 
  LTE_DL_FRAME_PARMS *frame_parms = &ue->frame_parms;
  LTE_UE_COMMON *common_vars   = &ue->common_vars;
  uint8_t eNB_id = 0;//ue_common_vars->eNb_id;

  unsigned char aa;
  unsigned char frame_type = frame_parms->frame_type; // Frame Type: 0 - FDD, 1 - TDD;
  unsigned int nb_prefix_samples = frame_parms->ofdm_symbol_size>>2;//(no_prefix ? 0 : frame_parms->nb_prefix_samples);
  unsigned int nb_prefix_samples0 = frame_parms->ofdm_symbol_size>>2;//(no_prefix ? 0 : frame_parms->nb_prefix_samples0);
  unsigned int subframe_offset;

  //   int i;
  unsigned int frame_length_samples = frame_parms->samples_per_tti * 10;
  void (*dft)(int16_t *,int16_t *, int);

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
    subframe_offset = frame_parms->ofdm_symbol_size * frame_parms->symbols_per_tti * subframe;

  } else {
    subframe_offset = frame_parms->samples_per_tti * subframe;

  }


  if (l<0 || l>=12) {
    LOG_E(PHY,"slot_fep_mbsfn: l must be between 0 and 11\n");
    return(-1);
  }

  if (((subframe == 0) || (subframe == 5) ||    // SFn 0,4,5,9;
       (subframe == 4) || (subframe == 9))
      && (frame_type==FDD) )    {   //check for valid MBSFN subframe
    LOG_E(PHY,"slot_fep_mbsfn: Subframe must be 1,2,3,6,7,8 for FDD, Got %d \n",subframe);
    return(-1);
  } else if (((subframe == 0) || (subframe == 1) || (subframe==2) ||  // SFn 0,4,5,9;
              (subframe == 5) || (subframe == 6))
             && (frame_type==TDD) )   {   //check for valid MBSFN subframe
    LOG_E(PHY,"slot_fep_mbsfn: Subframe must be 3,4,7,8,9 for TDD, Got %d \n",subframe);
    return(-1);
  }

#ifdef DEBUG_FEP
  LOG_D(PHY,"slot_fep_mbsfn: subframe %d, symbol %d, nb_prefix_samples %d, nb_prefix_samples0 %d, subframe_offset %d, sample_offset %d\n", subframe, l, nb_prefix_samples,nb_prefix_samples0,subframe_offset,
      sample_offset);
#endif

  for (aa=0; aa<frame_parms->nb_antennas_rx; aa++) {
    memset(&common_vars->common_vars_rx_data_per_thread[ue->current_thread_id[subframe]].rxdataF[aa][frame_parms->ofdm_symbol_size*l],0,frame_parms->ofdm_symbol_size*sizeof(int));
    if (l==0) {
#if UE_TIMING_TRACE
        start_meas(&ue->rx_dft_stats);
#endif
      dft((int16_t *)&common_vars->rxdata[aa][(sample_offset +
          nb_prefix_samples0 +
          subframe_offset -
          SOFFSET) % frame_length_samples],
          (int16_t *)&common_vars->common_vars_rx_data_per_thread[ue->current_thread_id[subframe]].rxdataF[aa][frame_parms->ofdm_symbol_size*l],1);
#if UE_TIMING_TRACE
      stop_meas(&ue->rx_dft_stats);
#endif
    } else {
      if ((sample_offset +
           (frame_parms->ofdm_symbol_size+nb_prefix_samples0+nb_prefix_samples) +
           (frame_parms->ofdm_symbol_size+nb_prefix_samples)*(l-1) +
           subframe_offset-
           SOFFSET) > (frame_length_samples - frame_parms->ofdm_symbol_size))
        memcpy((short *)&common_vars->rxdata[aa][frame_length_samples],
               (short *)&common_vars->rxdata[aa][0],
               frame_parms->ofdm_symbol_size*sizeof(int));

#if UE_TIMING_TRACE
      start_meas(&ue->rx_dft_stats);
#endif
      dft((int16_t *)&common_vars->rxdata[aa][(sample_offset +
          (frame_parms->ofdm_symbol_size+nb_prefix_samples0+nb_prefix_samples) +
          (frame_parms->ofdm_symbol_size+nb_prefix_samples)*(l-1) +
          subframe_offset-
          SOFFSET) % frame_length_samples],
          (int16_t *)&common_vars->common_vars_rx_data_per_thread[ue->current_thread_id[subframe]].rxdataF[aa][frame_parms->ofdm_symbol_size*l],1);
#if UE_TIMING_TRACE
      stop_meas(&ue->rx_dft_stats);
#endif
    }
  }



  //if ((l==0) || (l==(4-frame_parms->Ncp))) {
  // changed to invoke MBSFN channel estimation in symbols 2,6,10
  if ((l==2)||(l==6)||(l==10)) {
    for (aa=0; aa<frame_parms->nb_antennas_tx; aa++) {
      if (ue->perfect_ce == 0) {
#ifdef DEBUG_FEP
        LOG_D(PHY,"Channel estimation eNB %d, aatx %d, subframe %d, symbol %d\n",eNB_id,aa,subframe,l);
#endif

        lte_dl_mbsfn_channel_estimation(ue,
                                        eNB_id,
                                        0,
                                        subframe,
                                        l);
        /*   for (i=0;i<ue->PHY_measurements.n_adj_cells;i++) {
        lte_dl_mbsfn_channel_estimation(ue,
               eNB_id,
             i+1,
               subframe,
               l);
             lte_dl_channel_estimation(ue,eNB_id,0,
           Ns,
           aa,
           l,
           symbol);
           for (i=0;i<ue->PHY_measurements.n_adj_cells;i++) {
        lte_dl_channel_estimation(ue,eNB_id,i+1,
             Ns,
             aa,
             l,
             symbol); */
        //  }

        // do frequency offset estimation here!
        // use channel estimates from current symbol (=ch_t) and last symbol (ch_{t-1})
#ifdef DEBUG_FEP
        LOG_D(PHY,"Frequency offset estimation\n");
#endif
        // if ((l == 0) || (l==(4-frame_parms->Ncp)))
        /*    if ((l==2)||(l==6)||(l==10))
          lte_mbsfn_est_freq_offset(common_vars->dl_ch_estimates[0],
                  frame_parms,
                  l,
                  &common_vars->freq_offset); */
      }
    }
  }

#ifdef DEBUG_FEP
  LOG_D(PHY,"slot_fep_mbsfn: done\n");
#endif
  return(0);
}
