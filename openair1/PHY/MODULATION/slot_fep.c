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


int slot_fep(PHY_VARS_UE *ue,
             unsigned char l,
             unsigned char Ns,
             int sample_offset,
             int no_prefix,
	     int reset_freq_est)
{

  LTE_DL_FRAME_PARMS *frame_parms = &ue->frame_parms;
  LTE_UE_COMMON *common_vars   = &ue->common_vars;
  uint8_t eNB_id = 0;//ue_common_vars->eNb_id;
  unsigned char aa;
  unsigned char symbol = l+((7-frame_parms->Ncp)*(Ns&1)); ///symbol within sub-frame
  unsigned int nb_prefix_samples = (no_prefix ? 0 : frame_parms->nb_prefix_samples);
  unsigned int nb_prefix_samples0 = (no_prefix ? 0 : frame_parms->nb_prefix_samples0);
  unsigned int subframe_offset;//,subframe_offset_F;
  unsigned int slot_offset;
  int i;
  unsigned int frame_length_samples = frame_parms->samples_per_tti * 10;
  unsigned int rx_offset;

  /*LTE_UE_DLSCH_t **dlsch_ue = phy_vars_ue->dlsch_ue[eNB_id];
  unsigned char harq_pid = dlsch_ue[0]->current_harq_pid; 
  LTE_DL_UE_HARQ_t *dlsch0_harq = dlsch_ue[0]->harq_processes[harq_pid];
  int uespec_pilot[9][1200];*/

  void (*dft)(int16_t *,int16_t *, int);
  int tmp_dft_in[2048] __attribute__ ((aligned (32)));  // This is for misalignment issues for 6 and 15 PRBs

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
    subframe_offset = frame_parms->ofdm_symbol_size * frame_parms->symbols_per_tti * (Ns>>1);
    slot_offset = frame_parms->ofdm_symbol_size * (frame_parms->symbols_per_tti>>1) * (Ns%2);
  } else {
    subframe_offset = frame_parms->samples_per_tti * (Ns>>1);
    slot_offset = (frame_parms->samples_per_tti>>1) * (Ns%2);
  }

  //  subframe_offset_F = frame_parms->ofdm_symbol_size * frame_parms->symbols_per_tti * (Ns>>1);


  if (l<0 || l>=7-frame_parms->Ncp) {
    printf("slot_fep: l must be between 0 and %d\n",7-frame_parms->Ncp);
    return(-1);
  }

  if (Ns<0 || Ns>=20) {
    printf("slot_fep: Ns must be between 0 and 19\n");
    return(-1);
  }



  for (aa=0; aa<frame_parms->nb_antennas_rx; aa++) {
    memset(&common_vars->common_vars_rx_data_per_thread[ue->current_thread_id[Ns>>1]].rxdataF[aa][frame_parms->ofdm_symbol_size*symbol],0,frame_parms->ofdm_symbol_size*sizeof(int));

    rx_offset = sample_offset + slot_offset + nb_prefix_samples0 + subframe_offset - SOFFSET;
    // Align with 256 bit
    //    rx_offset = rx_offset&0xfffffff8;

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
            (int16_t *)&common_vars->common_vars_rx_data_per_thread[ue->current_thread_id[Ns>>1]].rxdataF[aa][frame_parms->ofdm_symbol_size*symbol],1);
      } else { // use dft input from RX buffer directly
#if UE_TIMING_TRACE
          start_meas(&ue->rx_dft_stats);
#endif

        dft((int16_t *)&common_vars->rxdata[aa][(rx_offset) % frame_length_samples],
            (int16_t *)&common_vars->common_vars_rx_data_per_thread[ue->current_thread_id[Ns>>1]].rxdataF[aa][frame_parms->ofdm_symbol_size*symbol],1);
#if UE_TIMING_TRACE
        stop_meas(&ue->rx_dft_stats);
#endif

      }
    } else {
      rx_offset += (frame_parms->ofdm_symbol_size+nb_prefix_samples)*l;// +
      //                   (frame_parms->ofdm_symbol_size+nb_prefix_samples)*(l-1);

#ifdef DEBUG_FEP
      //  if (ue->frame <100)
      LOG_I(PHY,"slot_fep: frame %d: slot %d, symbol %d, nb_prefix_samples %d, nb_prefix_samples0 %d, slot_offset %d, subframe_offset %d, sample_offset %d,rx_offset %d, frame_length_samples %d\n", ue->proc.proc_rxtx[(Ns>>1)&1].frame_rx,Ns, symbol,
          nb_prefix_samples,nb_prefix_samples0,slot_offset,subframe_offset,sample_offset,rx_offset,frame_length_samples);
#endif

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
            (int16_t *)&common_vars->common_vars_rx_data_per_thread[ue->current_thread_id[Ns>>1]].rxdataF[aa][frame_parms->ofdm_symbol_size*symbol],1);
      } else { // use dft input from RX buffer directly

        dft((int16_t *)&common_vars->rxdata[aa][(rx_offset) % frame_length_samples],
            (int16_t *)&common_vars->common_vars_rx_data_per_thread[ue->current_thread_id[Ns>>1]].rxdataF[aa][frame_parms->ofdm_symbol_size*symbol],1);
      }
#if UE_TIMING_TRACE
      stop_meas(&ue->rx_dft_stats);
#endif


    }

    #ifdef DEBUG_FEP
        //  if (ue->frame <100)
        printf("slot_fep: frame %d: symbol %d rx_offset %d\n", ue->proc.proc_rxtx[(Ns>>1)&1].frame_rx, symbol,rx_offset);
    #endif
  }

  if (ue->perfect_ce == 0) {
    if ((l==0) || (l==(4-frame_parms->Ncp))) {
      for (aa=0; aa<frame_parms->nb_antenna_ports_eNB; aa++) {

#ifdef DEBUG_FEP
        printf("Channel estimation eNB %d, aatx %d, slot %d, symbol %d\n",eNB_id,aa,Ns,l);
#endif
#if UE_TIMING_TRACE
        start_meas(&ue->dlsch_channel_estimation_stats);
#endif
        lte_dl_channel_estimation(ue,eNB_id,0,
                                  Ns,
                                  aa,
                                  l,
                                  symbol);
#if UE_TIMING_TRACE
        stop_meas(&ue->dlsch_channel_estimation_stats);
#endif

        for (i=0; i<ue->measurements.n_adj_cells; i++) {
          lte_dl_channel_estimation(ue,eNB_id,i+1,
                                    Ns,
                                    aa,
                                    l,
                                    symbol);
        }
      }


      // do frequency offset estimation here!
      // use channel estimates from current symbol (=ch_t) and last symbol (ch_{t-1})
#ifdef DEBUG_FEP
      printf("Frequency offset estimation\n");
#endif

      if (l==(4-frame_parms->Ncp)) {

#if UE_TIMING_TRACE
          start_meas(&ue->dlsch_freq_offset_estimation_stats);
#endif

        lte_est_freq_offset(common_vars->common_vars_rx_data_per_thread[ue->current_thread_id[Ns>>1]].dl_ch_estimates[0],
                            frame_parms,
                            l,
                            &common_vars->freq_offset,
			    reset_freq_est);
#if UE_TIMING_TRACE
        stop_meas(&ue->dlsch_freq_offset_estimation_stats);
#endif

      }
    }

  }

#ifdef DEBUG_FEP
  printf("slot_fep: done\n");
#endif
  return(0);
}

int front_end_fft(PHY_VARS_UE *ue,
             unsigned char l,
             unsigned char Ns,
             int sample_offset,
             int no_prefix)
{
  LTE_DL_FRAME_PARMS *frame_parms = &ue->frame_parms;
  LTE_UE_COMMON *common_vars   = &ue->common_vars;
  unsigned char aa;
  unsigned char symbol = l+((7-frame_parms->Ncp)*(Ns&1)); ///symbol within sub-frame
  unsigned int nb_prefix_samples = (no_prefix ? 0 : frame_parms->nb_prefix_samples);
  unsigned int nb_prefix_samples0 = (no_prefix ? 0 : frame_parms->nb_prefix_samples0);
  unsigned int subframe_offset;//,subframe_offset_F;
  unsigned int slot_offset;
  unsigned int frame_length_samples = frame_parms->samples_per_tti * 10;
  unsigned int rx_offset;
  uint8_t  threadId;

  /*LTE_UE_DLSCH_t **dlsch_ue = phy_vars_ue->dlsch_ue[eNB_id];
  unsigned char harq_pid = dlsch_ue[0]->current_harq_pid;
  LTE_DL_UE_HARQ_t *dlsch0_harq = dlsch_ue[0]->harq_processes[harq_pid];
  int uespec_pilot[9][1200];*/

  void (*dft)(int16_t *,int16_t *, int);
  int tmp_dft_in[2048] __attribute__ ((aligned (32)));  // This is for misalignment issues for 6 and 15 PRBs

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
    subframe_offset = frame_parms->ofdm_symbol_size * frame_parms->symbols_per_tti * (Ns>>1);
    slot_offset = frame_parms->ofdm_symbol_size * (frame_parms->symbols_per_tti>>1) * (Ns%2);
  } else {
    subframe_offset = frame_parms->samples_per_tti * (Ns>>1);
    slot_offset = (frame_parms->samples_per_tti>>1) * (Ns%2);
  }

  //  subframe_offset_F = frame_parms->ofdm_symbol_size * frame_parms->symbols_per_tti * (Ns>>1);


  if (l<0 || l>=7-frame_parms->Ncp) {
    printf("slot_fep: l must be between 0 and %d\n",7-frame_parms->Ncp);
    return(-1);
  }

  if (Ns<0 || Ns>=20) {
    printf("slot_fep: Ns must be between 0 and 19\n");
    return(-1);
  }



  threadId = ue->current_thread_id[Ns>>1];
  for (aa=0; aa<frame_parms->nb_antennas_rx; aa++) {
      // change thread index
    memset(&common_vars->common_vars_rx_data_per_thread[threadId].rxdataF[aa][frame_parms->ofdm_symbol_size*symbol],0,frame_parms->ofdm_symbol_size*sizeof(int));

    rx_offset = sample_offset + slot_offset + nb_prefix_samples0 + subframe_offset - SOFFSET;
    // Align with 256 bit
    //    rx_offset = rx_offset&0xfffffff8;

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
            (int16_t *)&common_vars->common_vars_rx_data_per_thread[threadId].rxdataF[aa][frame_parms->ofdm_symbol_size*symbol],1);
      } else { // use dft input from RX buffer directly
        start_meas(&ue->rx_dft_stats);

        dft((int16_t *)&common_vars->rxdata[aa][(rx_offset) % frame_length_samples],
            (int16_t *)&common_vars->common_vars_rx_data_per_thread[threadId].rxdataF[aa][frame_parms->ofdm_symbol_size*symbol],1);
        stop_meas(&ue->rx_dft_stats);

      }
    } else {
      rx_offset += (frame_parms->ofdm_symbol_size+nb_prefix_samples)*l;// +
      //                   (frame_parms->ofdm_symbol_size+nb_prefix_samples)*(l-1);

#ifdef DEBUG_FEP
      //  if (ue->frame <100)
      LOG_I(PHY,"slot_fep: frame %d: slot %d, threadId %d, symbol %d, nb_prefix_samples %d, nb_prefix_samples0 %d, slot_offset %d, subframe_offset %d, sample_offset %d,rx_offset %d, frame_length_samples %d\n",
              ue->proc.proc_rxtx[threadId].frame_rx,Ns, threadId,symbol,
          nb_prefix_samples,nb_prefix_samples0,slot_offset,subframe_offset,sample_offset,rx_offset,frame_length_samples);
#endif

      if (rx_offset > (frame_length_samples - frame_parms->ofdm_symbol_size))
        memcpy((void *)&common_vars->rxdata[aa][frame_length_samples],
               (void *)&common_vars->rxdata[aa][0],
               frame_parms->ofdm_symbol_size*sizeof(int));

      start_meas(&ue->rx_dft_stats);

      if ((rx_offset&7)!=0) {  // if input to dft is not 128-bit aligned, issue for size 6 and 15 PRBs
        memcpy((void *)tmp_dft_in,
               (void *)&common_vars->rxdata[aa][(rx_offset) % frame_length_samples],
               frame_parms->ofdm_symbol_size*sizeof(int));
        dft((int16_t *)tmp_dft_in,
            (int16_t *)&common_vars->common_vars_rx_data_per_thread[threadId].rxdataF[aa][frame_parms->ofdm_symbol_size*symbol],1);
      } else { // use dft input from RX buffer directly

        dft((int16_t *)&common_vars->rxdata[aa][(rx_offset) % frame_length_samples],
            (int16_t *)&common_vars->common_vars_rx_data_per_thread[threadId].rxdataF[aa][frame_parms->ofdm_symbol_size*symbol],1);
      }

      stop_meas(&ue->rx_dft_stats);


    }

    #ifdef DEBUG_FEP
        //  if (ue->frame <100)
        printf("slot_fep: frame %d: symbol %d rx_offset %d\n", ue->proc.proc_rxtx[threadId].frame_rx, symbol,rx_offset);
    #endif
  }
  return(0);
}

int front_end_chanEst(PHY_VARS_UE *ue,
             unsigned char l,
             unsigned char Ns,
             int reset_freq_est)
{
  LTE_DL_FRAME_PARMS *frame_parms = &ue->frame_parms;
  LTE_UE_COMMON *common_vars   = &ue->common_vars;
  uint8_t eNB_id = 0;//ue_common_vars->eNb_id;
  unsigned char aa;
  unsigned char symbol = l+((7-frame_parms->Ncp)*(Ns&1)); ///symbol within sub-frame
  int i;

  /*LTE_UE_DLSCH_t **dlsch_ue = phy_vars_ue->dlsch_ue[eNB_id];
  unsigned char harq_pid = dlsch_ue[0]->current_harq_pid;
  LTE_DL_UE_HARQ_t *dlsch0_harq = dlsch_ue[0]->harq_processes[harq_pid];
  int uespec_pilot[9][1200];*/

  if (ue->perfect_ce == 0) {
    if ((l==0) || (l==(4-frame_parms->Ncp))) {
      for (aa=0; aa<frame_parms->nb_antenna_ports_eNB; aa++) {

#ifdef DEBUG_FEP
        printf("Channel estimation eNB %d, aatx %d, slot %d, symbol %d\n",eNB_id,aa,Ns,l);
#endif
        start_meas(&ue->dlsch_channel_estimation_stats);
        lte_dl_channel_estimation(ue,eNB_id,0,
                                  Ns,
                                  aa,
                                  l,
                                  symbol);
        stop_meas(&ue->dlsch_channel_estimation_stats);

        for (i=0; i<ue->measurements.n_adj_cells; i++) {
          lte_dl_channel_estimation(ue,eNB_id,i+1,
                                    Ns,
                                    aa,
                                    l,
                                    symbol);
        }
      }


      // do frequency offset estimation here!
      // use channel estimates from current symbol (=ch_t) and last symbol (ch_{t-1})
#ifdef DEBUG_FEP
      printf("Frequency offset estimation\n");
#endif

      if (l==(4-frame_parms->Ncp)) {
        start_meas(&ue->dlsch_freq_offset_estimation_stats);
        lte_est_freq_offset(common_vars->common_vars_rx_data_per_thread[(Ns>>1)&0x1].dl_ch_estimates[0],
                            frame_parms,
                            l,
                            &common_vars->freq_offset,
                reset_freq_est);
        stop_meas(&ue->dlsch_freq_offset_estimation_stats);

      }
    }

  }
  return(0);
}
