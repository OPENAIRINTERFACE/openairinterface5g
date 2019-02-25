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

#include "PHY/defs_UE.h"
#include "PHY/defs_nr_UE.h"
#include "modulation_UE.h"
#include "PHY/LTE_ESTIMATION/lte_estimation.h"
#include "PHY/NR_UE_ESTIMATION/nr_estimation.h"

//#define DEBUG_FEP

#define SOFFSET 0

/*#ifdef LOG_I
#undef LOG_I
#define LOG_I(A,B...) printf(A)
#endif*/

int nr_slot_fep(PHY_VARS_NR_UE *ue,
		unsigned char l,
		unsigned char Ns,
		int sample_offset,
		int no_prefix,
		int reset_freq_est,
		NR_CHANNEL_EST_t channel)
{
  NR_DL_FRAME_PARMS *frame_parms = &ue->frame_parms;
  NR_UE_COMMON *common_vars   = &ue->common_vars;
  uint8_t eNB_id = 0;//ue_common_vars->eNb_id;
  unsigned char aa;
  unsigned char symbol = l;//+((7-frame_parms->Ncp)*(Ns&1)); ///symbol within sub-frame
  unsigned int nb_prefix_samples = (no_prefix ? 0 : frame_parms->nb_prefix_samples);
  unsigned int nb_prefix_samples0 = (no_prefix ? 0 : frame_parms->nb_prefix_samples0);
  //unsigned int subframe_offset;//,subframe_offset_F;
  unsigned int slot_offset;
  //int i;
  unsigned int frame_length_samples = frame_parms->samples_per_subframe * 10;
  unsigned int rx_offset;
  NR_UE_PDCCH *pdcch_vars  = ue->pdcch_vars[ue->current_thread_id[Ns]][0];
  uint16_t coreset_start_subcarrier = frame_parms->first_carrier_offset;//+((int)floor(frame_parms->ssb_start_subcarrier/NR_NB_SC_PER_RB)+pdcch_vars->coreset[0].rb_offset)*NR_NB_SC_PER_RB;
  uint16_t nb_rb_coreset = 0;
  uint16_t bwp_start_subcarrier = frame_parms->first_carrier_offset;//+516;
  uint16_t nb_rb_pdsch = 50;
  uint8_t p=0;
  uint8_t l0 = pdcch_vars->coreset[0].duration;
  uint64_t coreset_freq_dom  = pdcch_vars->coreset[0].frequencyDomainResources;
  for (int i = 0; i < 45; i++) {
    if (((coreset_freq_dom & 0x1FFFFFFFFFFF) >> i) & 0x1) nb_rb_coreset++;
  }
  nb_rb_coreset = 6 * nb_rb_coreset; 
  //printf("corset duration %d nb_rb_coreset %d\n", l0, nb_rb_coreset);

  void (*dft)(int16_t *,int16_t *, int);
  int tmp_dft_in[8192] __attribute__ ((aligned (32)));  // This is for misalignment issues for 6 and 15 PRBs

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

  case 4096:
    dft = dft4096;
    break;

  case 8192:
    dft = dft8192;
    break;

  default:
    dft = dft512;
    break;
  }

  if (no_prefix) {
    slot_offset = frame_parms->ofdm_symbol_size * (frame_parms->symbols_per_slot) * (Ns);
  } else {
    slot_offset = (frame_parms->samples_per_slot) * (Ns);
  }

  /*if (l<0 || l>=7-frame_parms->Ncp) {
    printf("slot_fep: l must be between 0 and %d\n",7-frame_parms->Ncp);
    return(-1);
    }*/

  if (Ns<0 || Ns>=20) {
    printf("slot_fep: Ns must be between 0 and 19\n");
    return(-1);
  }



  for (aa=0; aa<frame_parms->nb_antennas_rx; aa++) {
    memset(&common_vars->common_vars_rx_data_per_thread[ue->current_thread_id[Ns]].rxdataF[aa][frame_parms->ofdm_symbol_size*symbol],0,frame_parms->ofdm_symbol_size*sizeof(int));

    rx_offset = sample_offset + slot_offset + nb_prefix_samples0 - SOFFSET;
    // Align with 256 bit
    //    rx_offset = rx_offset&0xfffffff8;

#ifdef DEBUG_FEP
      //  if (ue->frame <100)
    /*LOG_I(PHY,*/printf("slot_fep: frame %d: slot %d, symbol %d, nb_prefix_samples %d, nb_prefix_samples0 %d, slot_offset %d,  sample_offset %d,rx_offset %d, frame_length_samples %d\n", ue->proc.proc_rxtx[(Ns)&1].frame_rx,Ns, symbol,
          nb_prefix_samples,nb_prefix_samples0,slot_offset,sample_offset,rx_offset,frame_length_samples);
#endif

    if (l==0) {

      if (rx_offset > (frame_length_samples - frame_parms->ofdm_symbol_size))
        memcpy((short *)&common_vars->rxdata[aa][frame_length_samples],
               (short *)&common_vars->rxdata[aa][0],
               frame_parms->ofdm_symbol_size*sizeof(int));

      if ((rx_offset&7)!=0) {  // if input to dft is not 256-bit aligned, issue for size 6,15 and 25 PRBs
        memcpy((void *)tmp_dft_in,
               (void *)&common_vars->rxdata[aa][rx_offset % frame_length_samples],
               frame_parms->ofdm_symbol_size*sizeof(int));
        dft((int16_t *)tmp_dft_in,
            (int16_t *)&common_vars->common_vars_rx_data_per_thread[ue->current_thread_id[Ns]].rxdataF[aa][frame_parms->ofdm_symbol_size*symbol],1);
      } else { // use dft input from RX buffer directly
#if UE_TIMING_TRACE
          start_meas(&ue->rx_dft_stats);
#endif

        dft((int16_t *)&common_vars->rxdata[aa][(rx_offset) % frame_length_samples],
            (int16_t *)&common_vars->common_vars_rx_data_per_thread[ue->current_thread_id[Ns]].rxdataF[aa][frame_parms->ofdm_symbol_size*symbol],1);
#if UE_TIMING_TRACE
        stop_meas(&ue->rx_dft_stats);
#endif
      }
    } else {
      rx_offset += (frame_parms->ofdm_symbol_size+nb_prefix_samples)*l;// +
      //                   (frame_parms->ofdm_symbol_size+nb_prefix_samples)*(l-1);

      if (rx_offset > (frame_length_samples - frame_parms->ofdm_symbol_size))
        memcpy((void *)&common_vars->rxdata[aa][frame_length_samples],
               (void *)&common_vars->rxdata[aa][0],
               frame_parms->ofdm_symbol_size*sizeof(int));
#if UE_TIMING_TRACE
      start_meas(&ue->rx_dft_stats);
#endif

      if ((rx_offset&7)!=0) {  // if input to dft is not 128-bit aligned, issue for size 6 and 15 PRBs
        memcpy((void *)tmp_dft_in,
               (void *)&common_vars->rxdata[aa][(rx_offset) % frame_length_samples],
               frame_parms->ofdm_symbol_size*sizeof(int));
        dft((int16_t *)tmp_dft_in,
            (int16_t *)&common_vars->common_vars_rx_data_per_thread[ue->current_thread_id[Ns]].rxdataF[aa][frame_parms->ofdm_symbol_size*symbol],1);
      } else { // use dft input from RX buffer directly

        dft((int16_t *)&common_vars->rxdata[aa][(rx_offset) % frame_length_samples],
            (int16_t *)&common_vars->common_vars_rx_data_per_thread[ue->current_thread_id[Ns]].rxdataF[aa][frame_parms->ofdm_symbol_size*symbol],1);
      }
#if UE_TIMING_TRACE
      stop_meas(&ue->rx_dft_stats);
#endif


    }

    #ifdef DEBUG_FEP
        //  if (ue->frame <100)
        printf("slot_fep: frame %d: symbol %d rx_offset %d\n", ue->proc.proc_rxtx[(Ns)&1].frame_rx, symbol,rx_offset);
    #endif
  }

  if (ue->perfect_ce == 0) {

  switch(channel){
  case NR_PBCH_EST:

#ifdef DEBUG_FEP
    printf("Channel estimation eNB %d, slot %d, symbol %d\n",eNB_id,Ns,l);
#endif
#if UE_TIMING_TRACE
    start_meas(&ue->dlsch_channel_estimation_stats);
#endif
    nr_pbch_channel_estimation(ue,eNB_id,0,
			       Ns,
			       l,
			       symbol);
      //}
#if UE_TIMING_TRACE
        stop_meas(&ue->dlsch_channel_estimation_stats);
#endif

      // do frequency offset estimation here!
      // use channel estimates from current symbol (=ch_t) and last symbol (ch_{t-1})
#ifdef DEBUG_FEP
      printf("Frequency offset estimation\n");
#endif

      if (l==(4-frame_parms->Ncp)) {

#if UE_TIMING_TRACE
          start_meas(&ue->dlsch_freq_offset_estimation_stats);
#endif

        /*lte_est_freq_offset(common_vars->common_vars_rx_data_per_thread[ue->current_thread_id[Ns>>1]].dl_ch_estimates[0],
                            frame_parms,
                            l,
                            &common_vars->freq_offset,
			    reset_freq_est);*/
#if UE_TIMING_TRACE
        stop_meas(&ue->dlsch_freq_offset_estimation_stats);
#endif

      }

  break;

  case NR_PDCCH_EST:

#ifdef DEBUG_FEP
    printf("PDCCH Channel estimation eNB %d, aatx %d, slot %d, symbol %d start_sc %d\n",eNB_id,aa,Ns,l,coreset_start_subcarrier);
#endif
#if UE_TIMING_TRACE
    start_meas(&ue->dlsch_channel_estimation_stats);
#endif
    nr_pdcch_channel_estimation(ue,eNB_id,0,
				Ns,
				l,
				symbol,
				coreset_start_subcarrier,
				nb_rb_coreset);
#if UE_TIMING_TRACE
    stop_meas(&ue->dlsch_channel_estimation_stats);
#endif
    
    break;
    
  case NR_PDSCH_EST:
#ifdef DEBUG_FEP
    printf("Channel estimation eNB %d, aatx %d, slot %d, symbol %d\n",eNB_id,aa,Ns,l);
#endif
#if UE_TIMING_TRACE
    start_meas(&ue->dlsch_channel_estimation_stats);
#endif

  ue->frame_parms.nushift =  (p>>1)&1;;

    if (symbol ==l0)
    nr_pdsch_channel_estimation(ue,eNB_id,0,
				Ns,
				p,
				l,
				symbol,
				bwp_start_subcarrier,
				nb_rb_pdsch);
				
#if UE_TIMING_TRACE
    stop_meas(&ue->dlsch_channel_estimation_stats);
#endif
    
    break;
    
  case NR_SSS_EST:
  break;

  default:
    LOG_E(PHY,"[UE][FATAL] Unknown channel format %d\n",channel);
    return(-1);
    break;
  }

  }

#ifdef DEBUG_FEP
  printf("slot_fep: done\n");
#endif
  return(0);
}

