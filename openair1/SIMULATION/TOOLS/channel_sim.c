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

#include <string.h>
#include <math.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include "SIMULATION/TOOLS/sim.h"
#include "SIMULATION/RF/rf.h"
#include "PHY/types.h"
#include "PHY/defs_eNB.h"
#include "PHY/phy_extern.h"
#include "PHY/phy_extern_ue.h"

#include "common/utils/LOG/log.h"
#include "PHY_INTERFACE/phy_interface_extern.h"
#include "UTIL/OPT/opt.h" // to test OPT
#include "common/ran_context.h"

#define RF

//#define DEBUG_SIM 1


void do_DL_sig(sim_t *sim,
               uint16_t subframe,
               uint32_t offset,
               uint32_t length,
               uint8_t abstraction_flag,
               LTE_DL_FRAME_PARMS *ue_frame_parms,
               uint8_t UE_id,
               int CC_id)
{
  int32_t **txdata,**rxdata;
  uint32_t ru_id=0;
  double tx_pwr;
  double rx_pwr;
  int32_t rx_pwr2;
  uint32_t i,aa;
  uint32_t sf_offset;

  uint8_t hold_channel=0;
  uint8_t nb_antennas_rx = sim->RU2UE[0][0][CC_id]->nb_rx; // number of rx antennas at UE
  uint8_t nb_antennas_tx = sim->RU2UE[0][0][CC_id]->nb_tx; // number of tx antennas at eNB

  double s_re0[30720];
  double s_re1[30720];
  double *s_re[NB_ANTENNAS_TX];
  double s_im0[30720];
  double s_im1[30720];
  double *s_im[NB_ANTENNAS_TX];
  double r_re00[30720];
  double r_re01[30720];
  double *r_re0[NB_ANTENNAS_RX];
  double r_im00[30720];
  double r_im01[30720];
  double *r_im0[NB_ANTENNAS_RX];
  LTE_DL_FRAME_PARMS *frame_parms;

  s_re[0] = s_re0;
  s_im[0] = s_im0;
  s_re[1] = s_re1;
  s_im[1] = s_im1;

  r_re0[0] = r_re00;
  r_im0[0] = r_im00;
  r_re0[1] = r_re01;
  r_im0[1] = r_im01;

  if (subframe==0)
    hold_channel = 0;
  else
    hold_channel = 1;

  pthread_mutex_lock(&sim->RU_output_mutex[UE_id]);
  
  if (sim->RU_output_mask[UE_id] == 0) {  //  This is the first eNodeB for this UE, clear the buffer
    for (aa=0; aa<nb_antennas_rx; aa++) {
      memset((void*)sim->r_re_DL[UE_id][aa],0,(RC.ru[0]->frame_parms->samples_per_tti)*sizeof(double));
      memset((void*)sim->r_im_DL[UE_id][aa],0,(RC.ru[0]->frame_parms->samples_per_tti)*sizeof(double));
    }
  }
  pthread_mutex_unlock(&sim->RU_output_mutex[UE_id]);
  
  for (ru_id=0; ru_id<RC.nb_RU; ru_id++) {
    txdata = RC.ru[ru_id]->common.txdata;
    frame_parms = RC.ru[ru_id]->frame_parms;
    
    //    sf_offset = (subframe*frame_parms->samples_per_tti) + offset;
    sf_offset = (subframe*frame_parms->samples_per_tti);
    for (int aid=0;aid<RC.ru[ru_id]->nb_tx;aid++)
      LOG_D(OCM,">>>>>>>>>>>>>>>>>TXPATH: RU %d : DL_sig reading TX%d for subframe %d (sf_offset %d, length %d) from %p\n",ru_id,aid,subframe,sf_offset,length,txdata[0]+sf_offset); 
    int length_meas = frame_parms->ofdm_symbol_size;
    if (sf_offset+length <= frame_parms->samples_per_tti*10) {
      
      tx_pwr = dac_fixed_gain(s_re,
			      s_im,
			      txdata,
			      sf_offset+offset,
			      nb_antennas_tx,
			      length,
			      sf_offset,
			      length_meas,
			      14,
			      frame_parms->pdsch_config_common.referenceSignalPower, // dBm/RE
			      0,
			      &sim->ru_amp[ru_id],
			      frame_parms->N_RB_DL*12);
      
    }
    else {
      tx_pwr = dac_fixed_gain(s_re,
			      s_im,
			      txdata,
			      sf_offset,
			      nb_antennas_tx,
			      (frame_parms->samples_per_tti*10)-sf_offset,
			      sf_offset,
			      length_meas,
			      14,
			      frame_parms->pdsch_config_common.referenceSignalPower, // dBm/RE
			      0,
			      &sim->ru_amp[ru_id],
			      frame_parms->N_RB_DL*12);
      
      tx_pwr = dac_fixed_gain(s_re,
			      s_im,
			      txdata,
			      sf_offset,
			      nb_antennas_tx,
			      length+sf_offset-(frame_parms->samples_per_tti*10),
			      sf_offset,
			      length_meas,
			      14,
			      frame_parms->pdsch_config_common.referenceSignalPower, // dBm/RE
			      0,
			      &sim->ru_amp[ru_id],
			      frame_parms->N_RB_DL*12);
    }
#ifdef DEBUG_SIM
    for (int aid=0;aid<RC.ru[ru_id]->nb_tx;aid++)
      LOG_D(OCM,"[SIM][DL] subframe %d: txp%d (time) %d dB\n",
	    subframe,aid,dB_fixed(signal_energy(&txdata[aid][sf_offset],length_meas)));
    
    LOG_D(OCM,"[SIM][DL] RU %d (CCid %d): tx_pwr %.1f dBm/RE (target %d dBm/RE), for subframe %d\n",
	  ru_id,CC_id,
	  10*log10(tx_pwr),
	  frame_parms->pdsch_config_common.referenceSignalPower,
	  subframe);
    
#endif
    tx_pwr = signal_energy_fp(s_re,s_im,nb_antennas_tx,
			      length<length_meas?length:length_meas,
			      0)/(12.0*frame_parms->N_RB_DL);
    
    //RU2UE[eNB_id][UE_id]->path_loss_dB = 0;
    multipath_channel(sim->RU2UE[ru_id][UE_id][CC_id],s_re,s_im,r_re0,r_im0,
		      length,hold_channel,0);
#ifdef DEBUG_SIM
    rx_pwr = signal_energy_fp2(sim->RU2UE[ru_id][UE_id][CC_id]->ch[0],
			       sim->RU2UE[ru_id][UE_id][CC_id]->channel_length)*sim->RU2UE[ru_id][UE_id][CC_id]->channel_length;
    LOG_D(OCM,"[SIM][DL] Channel RU %d => UE %d (CCid %d): Channel gain %f dB (%f)\n",ru_id,UE_id,CC_id,10*log10(rx_pwr),rx_pwr);
#endif
    
    
#ifdef DEBUG_SIM
    
    for (i=0; i<sim->RU2UE[ru_id][UE_id][CC_id]->channel_length; i++)
      LOG_D(OCM,"channel(%d,%d)[%d] : (%f,%f)\n",ru_id,UE_id,i,sim->RU2UE[ru_id][UE_id][CC_id]->ch[0][i].x,sim->RU2UE[ru_id][UE_id][CC_id]->ch[0][i].y);
    
#endif
    
    LOG_D(OCM,"[SIM][DL] Channel RU %d => UE %d (CCid %d): tx_power %.1f dBm/RE, path_loss %1.f dB\n",
	  ru_id,UE_id,CC_id,
	  (double)frame_parms->pdsch_config_common.referenceSignalPower,
	  sim->RU2UE[ru_id][UE_id][CC_id]->path_loss_dB);
    
#ifdef DEBUG_SIM
    rx_pwr = signal_energy_fp(r_re0,r_im0,nb_antennas_rx,
			      length<length_meas?length:length_meas,
			      0)/(12.0*frame_parms->N_RB_DL);
    LOG_D(OCM,"[SIM][DL] UE %d : rx_pwr %f dBm/RE (%f dBm RSSI,tx %f dB)for subframe %d (length %d)\n",UE_id,
	  10*log10(rx_pwr),
	  10*log10(rx_pwr*(double)frame_parms->N_RB_DL*12),
	  10*log10(tx_pwr),subframe,
	  length<length_meas?length:length_meas);

    
    LOG_D(OCM,"[SIM][DL] UE %d : rx_pwr (noise) -132 dBm/RE (N0fs = %.1f dBm, N0B = %.1f dBm) for subframe %d\n",
	  UE_id,
	  10*log10(sim->RU2UE[ru_id][UE_id][CC_id]->sampling_rate*1e6)-174,
	  10*log10(sim->RU2UE[ru_id][UE_id][CC_id]->sampling_rate*1e6*12*frame_parms->N_RB_DL/(double)frame_parms->ofdm_symbol_size)-174,
	  subframe);
#endif
    
    if (sim->RU2UE[ru_id][UE_id][CC_id]->first_run == 1)
      sim->RU2UE[ru_id][UE_id][CC_id]->first_run = 0;
    
    
    // RF model
#ifdef DEBUG_SIM
    LOG_D(OCM,"[SIM][DL] UE %d (CCid %d): rx_gain %d dB (-ADC %f) for subframe %d\n",UE_id,CC_id,PHY_vars_UE_g[UE_id][CC_id]->rx_total_gain_dB,
	  PHY_vars_UE_g[UE_id][CC_id]->rx_total_gain_dB-66.227,subframe);
#endif
    
    rf_rx_simple(r_re0,
		 r_im0,
		 nb_antennas_rx,
		 length,
		 1e3/sim->RU2UE[ru_id][UE_id][CC_id]->sampling_rate,  // sampling time (ns)
		 (double)PHY_vars_UE_g[UE_id][CC_id]->rx_total_gain_dB - 66.227);   // rx_gain (dB) (66.227 = 20*log10(pow2(11)) = gain from the adc that will be applied later)
    
#ifdef DEBUG_SIM
    rx_pwr = signal_energy_fp(r_re0,r_im0,
			      nb_antennas_rx,
			      length<length_meas?length:length_meas,
			      0)/(12.0*frame_parms->N_RB_DL);
    LOG_D(OCM,"[SIM][DL] UE %d : ADC in (RU %d) %f dBm/RE for subframe %d\n",
	  UE_id,ru_id,
	  10*log10(rx_pwr),subframe);
#endif
    
    
    pthread_mutex_lock(&sim->RU_output_mutex[UE_id]);
    for (i=0; i<frame_parms->samples_per_tti; i++) {
      for (aa=0; aa<nb_antennas_rx; aa++) {
	sim->r_re_DL[UE_id][aa][i]+=r_re0[aa][i];
	sim->r_im_DL[UE_id][aa][i]+=r_im0[aa][i];
      }
    }
    sim->RU_output_mask[UE_id] |= (1<<ru_id);
    if (sim->RU_output_mask[UE_id] == (1<<RC.nb_RU)-1) {
      sim->RU_output_mask[UE_id]=0;
      
      
      
      double *r_re_p[2] = {sim->r_re_DL[UE_id][0],sim->r_re_DL[UE_id][1]};
      double *r_im_p[2] = {sim->r_im_DL[UE_id][0],sim->r_im_DL[UE_id][1]};
      
#ifdef DEBUG_SIM
      rx_pwr = signal_energy_fp(r_re_p,r_im_p,nb_antennas_rx,length<length_meas?length:length_meas,0)/(12.0*frame_parms->N_RB_DL);
      LOG_D(OCM,"[SIM][DL] UE %d : ADC in %f dBm/RE for subframe %d\n",UE_id,10*log10(rx_pwr),subframe);
#endif
      
      rxdata = PHY_vars_UE_g[UE_id][CC_id]->common_vars.rxdata;
      sf_offset = (subframe*frame_parms->samples_per_tti)+offset;
      
      
      adc(r_re_p,
	  r_im_p,
	  0,
	  sf_offset,
	  rxdata,
	  nb_antennas_rx,
	  length,
	  12);
      
#ifdef DEBUG_SIM
      rx_pwr2 = signal_energy(rxdata[0]+sf_offset,length<length_meas?length:length_meas)/(12.0*frame_parms->N_RB_DL);
      LOG_D(OCM,"[SIM][DL] UE %d : rx_pwr (ADC out) %f dB/RE (%d) for subframe %d, writing to %p, length %d\n",UE_id, 10*log10((double)rx_pwr2),rx_pwr2,subframe,rxdata,length<length_meas?length:length_meas);
      LOG_D(OCM,"[SIM][DL] UE %d : rx_pwr (ADC out) %f dB for subframe %d\n",UE_id,10*log10((double)rx_pwr2*12*frame_parms->N_RB_DL) ,subframe);
#else
      UNUSED_VARIABLE(rx_pwr2);
      UNUSED_VARIABLE(tx_pwr);
      UNUSED_VARIABLE(rx_pwr);
#endif
	
    } // RU_output_mask
    pthread_mutex_unlock(&sim->RU_output_mutex[UE_id]);      
  } // ru_id
  
}

void do_UL_sig(sim_t *sim, uint16_t subframe, uint8_t abstraction_flag, LTE_DL_FRAME_PARMS *frame_parms, uint32_t frame, int ru_id, uint8_t CC_id, int NB_UE_INST)
{
  int32_t **txdata,**rxdata;
  uint8_t UE_id=0;

  uint8_t nb_antennas_rx = sim->UE2RU[0][0][CC_id]->nb_rx; // number of rx antennas at eNB
  uint8_t nb_antennas_tx = sim->UE2RU[0][0][CC_id]->nb_tx; // number of tx antennas at UE

  double tx_pwr, rx_pwr;
  int32_t rx_pwr2;
  uint32_t i,aa;
  uint32_t sf_offset,sf_offset_tdd;

  uint8_t hold_channel=0;

  double s_re0[30720];
  double s_re1[30720];
  double *s_re[NB_ANTENNAS_TX];
  double s_im0[30720];
  double s_im1[30720];
  double *s_im[NB_ANTENNAS_TX];
  double r_re00[30720];
  double r_re01[30720];
  double *r_re0[NB_ANTENNAS_RX];
  double r_im00[30720];
  double r_im01[30720];
  double *r_im0[NB_ANTENNAS_RX];

  s_re[0] = s_re0;
  s_im[0] = s_im0;
  s_re[1] = s_re1;
  s_im[1] = s_im1;

  r_re0[0] = r_re00;
  r_im0[0] = r_im00;
  r_re0[1] = r_re01;
  r_im0[1] = r_im01;
  
  pthread_mutex_lock(&sim->UE_output_mutex[ru_id]);
  // Clear RX signal for eNB = eNB_id
  for (i=0; i<frame_parms->samples_per_tti; i++) {
    for (aa=0; aa<nb_antennas_rx; aa++) {
      sim->r_re_UL[ru_id][aa][i]=0.0;
      sim->r_im_UL[ru_id][aa][i]=0.0;
    }
  }
  pthread_mutex_unlock(&sim->UE_output_mutex[ru_id]);
  
  // Compute RX signal for eNB = eNB_id
  for (UE_id=0; UE_id<NB_UE_INST; UE_id++) {
    
    txdata = PHY_vars_UE_g[UE_id][CC_id]->common_vars.txdata;
    AssertFatal(txdata != NULL,"txdata is null\n");

    sf_offset = subframe*frame_parms->samples_per_tti;
    if (subframe>0) sf_offset_tdd = sf_offset - PHY_vars_UE_g[UE_id][CC_id]->N_TA_offset;
    else            sf_offset_tdd = sf_offset;

    LOG_D(OCM,"txdata for subframe %d (%d), power %d\n",subframe,sf_offset_tdd,dB_fixed(signal_energy(&txdata[0][sf_offset_tdd],frame_parms->samples_per_tti)));

    if (((double)PHY_vars_UE_g[UE_id][CC_id]->tx_power_dBm[subframe] +
	 sim->UE2RU[UE_id][ru_id][CC_id]->path_loss_dB) <= -125.0) {
      // don't simulate a UE that is too weak
      LOG_D(OCM,"[SIM][UL] ULPOWERS UE %d tx_pwr %d dBm (num_RE %d) for subframe %d (sf_offset %d,sf_offset_tdd %d)\n",
	    UE_id,
	    PHY_vars_UE_g[UE_id][CC_id]->tx_power_dBm[subframe],
	    PHY_vars_UE_g[UE_id][CC_id]->tx_total_RE[subframe],
	    subframe,sf_offset,sf_offset_tdd);	
    } else {
      tx_pwr = dac_fixed_gain((double**)s_re,
			      (double**)s_im,
			      txdata,
			      sf_offset_tdd,
			      nb_antennas_tx,
			      frame_parms->samples_per_tti,
			      sf_offset_tdd,
			      frame_parms->ofdm_symbol_size,
			      14,
			      (double)PHY_vars_UE_g[UE_id][CC_id]->tx_power_dBm[subframe]-10*log10((double)PHY_vars_UE_g[UE_id][CC_id]->tx_total_RE[subframe]),
			      1,
			      NULL,
			      PHY_vars_UE_g[UE_id][CC_id]->tx_total_RE[subframe]);  // This make the previous argument the total power
      LOG_D(OCM,"[SIM][UL] ULPOWERS UE %d tx_pwr %f dBm (target %d dBm, num_RE %d) for subframe %d (sf_offset %d,sf_offset_tdd %d)\n",
	    UE_id,
	    10*log10(tx_pwr*PHY_vars_UE_g[UE_id][CC_id]->tx_total_RE[subframe]),
	    PHY_vars_UE_g[UE_id][CC_id]->tx_power_dBm[subframe],
	    PHY_vars_UE_g[UE_id][CC_id]->tx_total_RE[subframe],
	    subframe,sf_offset,sf_offset_tdd);
      
      
      multipath_channel(sim->UE2RU[UE_id][ru_id][CC_id],s_re,s_im,r_re0,r_im0,
			frame_parms->samples_per_tti,hold_channel,0);
      
      
      rx_pwr = signal_energy_fp2(sim->UE2RU[UE_id][ru_id][CC_id]->ch[0],
				 sim->UE2RU[UE_id][ru_id][CC_id]->channel_length)*sim->UE2RU[UE_id][ru_id][CC_id]->channel_length;
      
      LOG_D(OCM,"[SIM][UL] subframe %d Channel UE %d => RU %d : %f dB (hold %d,length %d, PL %f)\n",subframe,UE_id,ru_id,10*log10(rx_pwr),
	    hold_channel,sim->UE2RU[UE_id][ru_id][CC_id]->channel_length,
	    sim->UE2RU[UE_id][ru_id][CC_id]->path_loss_dB);
      
      rx_pwr = signal_energy_fp(r_re0,r_im0,nb_antennas_rx,frame_parms->samples_per_tti,0);
      LOG_D(OCM,"[SIM][UL] RU %d (%d/%d rx antennas) : rx_pwr %f dBm (tx_pwr - PL %f) for subframe %d, sptti %d\n",
	    ru_id,nb_antennas_rx,sim->UE2RU[UE_id][ru_id][CC_id]->nb_rx,10*log10(rx_pwr),10*log10(tx_pwr*PHY_vars_UE_g[UE_id][CC_id]->tx_total_RE[subframe])+sim->UE2RU[UE_id][ru_id][CC_id]->path_loss_dB,subframe,frame_parms->samples_per_tti);
      /*	
		if (abs(10*log10(rx_pwr)-10*log10(tx_pwr*PHY_vars_UE_g[UE_id][CC_id]->tx_total_RE[subframe])-UE2RU[UE_id][ru_id][CC_id]->path_loss_dB)>3) {
		write_output("txsig_re.m","s_re",s_re[0],frame_parms->samples_per_tti,1,7);
		write_output("txsig_im.m","s_im",s_im[0],frame_parms->samples_per_tti,1,7);
		write_output("rxsig_re.m","r_re",r_re0[0],frame_parms->samples_per_tti,1,7);
		write_output("rxsig_im.m","r_im",r_im0[0],frame_parms->samples_per_tti,1,7);
		exit(-1);
		}*/
      
      if (sim->UE2RU[UE_id][ru_id][CC_id]->first_run == 1)
	sim->UE2RU[UE_id][ru_id][CC_id]->first_run = 0;
      
      
      pthread_mutex_lock(&sim->UE_output_mutex[ru_id]);
      for (aa=0; aa<nb_antennas_rx; aa++) {
	for (i=0; i<frame_parms->samples_per_tti; i++) {
	  sim->r_re_UL[ru_id][aa][i]+=r_re0[aa][i];
	  sim->r_im_UL[ru_id][aa][i]+=r_im0[aa][i];
	}
      }
      pthread_mutex_unlock(&sim->UE_output_mutex[ru_id]);
    }
  } //UE_id
  
  double *r_re_p[2] = {sim->r_re_UL[ru_id][0],sim->r_re_UL[ru_id][1]};
  double *r_im_p[2] = {sim->r_im_UL[ru_id][0],sim->r_im_UL[ru_id][1]};
  
  rx_pwr = signal_energy_fp(r_re_p,r_im_p,nb_antennas_rx,frame_parms->samples_per_tti,0);
  LOG_D(OCM,"[SIM][UL] RU %d (%d/%d rx antennas) : rx_pwr %f dBm (before RF) for subframe %d, gain %f\n",
	ru_id,nb_antennas_rx,nb_antennas_rx,10*log10(rx_pwr),subframe,
	(double)RC.ru[ru_id]->max_rxgain-(double)RC.ru[ru_id]->att_rx - 66.227);
  rf_rx_simple(r_re_p,
	       r_im_p,
	       nb_antennas_rx,
	       frame_parms->samples_per_tti,
	       1e3/sim->UE2RU[0][ru_id][CC_id]->sampling_rate,  // sampling time (ns)
	       (double)RC.ru[ru_id]->max_rxgain-(double)RC.ru[ru_id]->att_rx - 66.227);   // rx_gain (dB) (66.227 = 20*log10(pow2(11)) = gain from the adc that will be applied later)
  
#ifdef DEBUG_SIM
  rx_pwr = signal_energy_fp(r_re_p,r_im_p,nb_antennas_rx,frame_parms->samples_per_tti,0);//*(double)frame_parms->ofdm_symbol_size/(12.0*frame_parms->N_RB_DL;
  LOG_D(OCM,"[SIM][UL] rx_pwr (ADC in) %f dB for subframe %d (rx_gain %f)\n",10*log10(rx_pwr),subframe,
	(double)RC.ru[ru_id]->max_rxgain-(double)RC.ru[ru_id]->att_rx);
#endif
  
  rxdata = RC.ru[ru_id]->common.rxdata;
  sf_offset = subframe*frame_parms->samples_per_tti;
  if (subframe>0) sf_offset_tdd = sf_offset - RC.ru[ru_id]->N_TA_offset;
  else            sf_offset_tdd = sf_offset;
  
  
  adc(r_re_p,
      r_im_p,
      0,
      sf_offset_tdd,
      rxdata,
      nb_antennas_rx,
      frame_parms->samples_per_tti,
      12);
  
#ifdef DEBUG_SIM
  rx_pwr2 = signal_energy(rxdata[0]+sf_offset_tdd,frame_parms->samples_per_tti)*(double)frame_parms->ofdm_symbol_size/(12.0*frame_parms->N_RB_DL);
  LOG_D(OCM,"[SIM][UL] RU %d rx_pwr (ADC out) %f dB (%d) for subframe %d (offset %d) = %p\n",ru_id,10*log10((double)rx_pwr2),rx_pwr2,subframe,sf_offset,rxdata[0]+sf_offset_tdd);
#else
  UNUSED_VARIABLE(tx_pwr);
  UNUSED_VARIABLE(rx_pwr);
  UNUSED_VARIABLE(rx_pwr2);
#endif
}
