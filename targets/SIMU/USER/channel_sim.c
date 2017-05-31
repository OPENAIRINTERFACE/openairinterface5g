/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.0  (the "License"); you may not use this file
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

#include "SIMULATION/TOOLS/defs.h"
#include "SIMULATION/RF/defs.h"
#include "PHY/types.h"
#include "PHY/defs.h"
#include "PHY/extern.h"

#ifdef OPENAIR2
#include "LAYER2/MAC/defs.h"
#include "LAYER2/MAC/extern.h"
#include "UTIL/LOG/log_if.h"
#include "UTIL/LOG/log_extern.h"
#include "RRC/LITE/extern.h"
#include "PHY_INTERFACE/extern.h"
#include "UTIL/OCG/OCG.h"
#include "UTIL/OPT/opt.h" // to test OPT
#endif

#include "UTIL/FIFO/types.h"

#ifdef IFFT_FPGA
#include "PHY/LTE_REFSIG/mod_table.h"
#endif

#include "SCHED/defs.h"
#include "SCHED/extern.h"

#ifdef XFORMS
#include "forms.h"
#include "phy_procedures_sim_form.h"
#endif

#include "oaisim.h"

#define RF
#define DEBUG_SIM

int number_rb_ul;
int first_rbUL ;

extern Signal_buffers_t *signal_buffers_g;


double r_re_DL[NUMBER_OF_UE_MAX][2][30720];
double r_im_DL[NUMBER_OF_UE_MAX][2][30720];
double r_re_UL[NUMBER_OF_eNB_MAX][2][30720];
double r_im_UL[NUMBER_OF_eNB_MAX][2][30720];
int eNB_output_mask[NUMBER_OF_UE_MAX];
int UE_output_mask[NUMBER_OF_eNB_MAX];
pthread_mutex_t eNB_output_mutex[NUMBER_OF_UE_MAX];
pthread_mutex_t UE_output_mutex[NUMBER_OF_eNB_MAX];

void do_DL_sig(channel_desc_t *eNB2UE[NUMBER_OF_eNB_MAX][NUMBER_OF_UE_MAX][MAX_NUM_CCs],
	       node_desc_t *enb_data[NUMBER_OF_eNB_MAX],
	       node_desc_t *ue_data[NUMBER_OF_UE_MAX],
	       uint16_t subframe,uint8_t abstraction_flag,LTE_DL_FRAME_PARMS *frame_parms,
	       uint8_t UE_id,
	       int CC_id)
{

  int32_t att_eNB_id=-1;
  int32_t **txdata,**rxdata;

  uint8_t eNB_id=0;
  double tx_pwr;
  double rx_pwr;
  int32_t rx_pwr2;
  uint32_t i,aa;
  uint32_t sf_offset;

  double min_path_loss=-200;
  uint8_t hold_channel=0;
  uint8_t nb_antennas_rx = eNB2UE[0][0][CC_id]->nb_rx; // number of rx antennas at UE
  uint8_t nb_antennas_tx = eNB2UE[0][0][CC_id]->nb_tx; // number of tx antennas at eNB

  double s_re0[30720];//PHY_vars_UE_g[UE_id][CC_id]->frame_parms.samples_per_tti];
  double s_re1[30720];//PHY_vars_UE_g[UE_id][CC_id]->frame_parms.samples_per_tti];
  double *s_re[2];
  double s_im0[30720];//PHY_vars_UE_g[UE_id][CC_id]->frame_parms.samples_per_tti];
  double s_im1[30720];//PHY_vars_UE_g[UE_id][CC_id]->frame_parms.samples_per_tti];
  double *s_im[2];
  double r_re00[30720];//PHY_vars_UE_g[UE_id][CC_id]->frame_parms.samples_per_tti];
  double r_re01[30720];//PHY_vars_UE_g[UE_id][CC_id]->frame_parms.samples_per_tti];
  double *r_re0[2];
  double r_im00[30720];//PHY_vars_UE_g[UE_id][CC_id]->frame_parms.samples_per_tti];
  double r_im01[30720];//PHY_vars_UE_g[UE_id][CC_id]->frame_parms.samples_per_tti];
  double *r_im0[2];

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

  if (abstraction_flag != 0) {
    //for (UE_id=0;UE_id<NB_UE_INST;UE_id++) {

    if (!hold_channel) {
      // calculate the random channel from each eNB
      for (eNB_id=0; eNB_id<NB_eNB_INST; eNB_id++) {

        random_channel(eNB2UE[eNB_id][UE_id][CC_id],abstraction_flag);
        /*
        for (i=0;i<eNB2UE[eNB_id][UE_id]->nb_taps;i++)
        printf("eNB2UE[%d][%d]->a[0][%d] = (%f,%f)\n",eNB_id,UE_id,i,eNB2UE[eNB_id][UE_id]->a[0][i].x,eNB2UE[eNB_id][UE_id]->a[0][i].y);
        */
        freq_channel(eNB2UE[eNB_id][UE_id][CC_id], frame_parms->N_RB_DL,frame_parms->N_RB_DL*12+1);
      }

      // find out which eNB the UE is attached to
      for (eNB_id=0; eNB_id<NB_eNB_INST; eNB_id++) {
        if (find_ue(PHY_vars_UE_g[UE_id][CC_id]->pdcch_vars[0][0]->crnti,PHY_vars_eNB_g[eNB_id][CC_id])>=0) {
          // UE with UE_id is connected to eNb with eNB_id
          att_eNB_id=eNB_id;
          LOG_D(OCM,"A: UE attached to eNB (UE%d->eNB%d)\n",UE_id,eNB_id);
        }
      }

      // if UE is not attached yet, find assume its the eNB with the smallest pathloss
      if (att_eNB_id<0) {
        for (eNB_id=0; eNB_id<NB_eNB_INST; eNB_id++) {
          if (min_path_loss<eNB2UE[eNB_id][UE_id][CC_id]->path_loss_dB) {
            min_path_loss = eNB2UE[eNB_id][UE_id][CC_id]->path_loss_dB;
            att_eNB_id=eNB_id;
            LOG_D(OCM,"B: UE attached to eNB (UE%d->eNB%d)\n",UE_id,eNB_id);
          }
        }
      }

      if (att_eNB_id<0) {
        LOG_E(OCM,"Cannot find eNB for UE %d, return\n",UE_id);
        return; //exit(-1);
      }

#ifdef DEBUG_SIM
      rx_pwr = signal_energy_fp2(eNB2UE[att_eNB_id][UE_id][CC_id]->ch[0],
                                 eNB2UE[att_eNB_id][UE_id][CC_id]->channel_length)*eNB2UE[att_eNB_id][UE_id][CC_id]->channel_length;
      LOG_D(OCM,"Channel (CCid %d) eNB %d => UE %d : tx_power %d dBm, path_loss %f dB\n",
            CC_id,att_eNB_id,UE_id,
            frame_parms->pdsch_config_common.referenceSignalPower,
            eNB2UE[att_eNB_id][UE_id][CC_id]->path_loss_dB);
#endif

      //dlsch_abstraction(PHY_vars_UE_g[UE_id]->sinr_dB, rb_alloc, 8);
      // fill in perfect channel estimates
      channel_desc_t *desc1 = eNB2UE[att_eNB_id][UE_id][CC_id];
      int32_t **dl_channel_est = PHY_vars_UE_g[UE_id][CC_id]->common_vars.common_vars_rx_data_per_thread[subframe&0x1].dl_ch_estimates[0];
      //      double scale = pow(10.0,(enb_data[att_eNB_id]->tx_power_dBm + eNB2UE[att_eNB_id][UE_id]->path_loss_dB + (double) PHY_vars_UE_g[UE_id]->rx_total_gain_dB)/20.0);
      double scale = pow(10.0,(frame_parms->pdsch_config_common.referenceSignalPower+eNB2UE[att_eNB_id][UE_id][CC_id]->path_loss_dB + (double) PHY_vars_UE_g[UE_id][CC_id]->rx_total_gain_dB)/20.0);
      LOG_D(OCM,"scale =%lf (%d dB)\n",scale,(int) (20*log10(scale)));
      // freq_channel(desc1,frame_parms->N_RB_DL,nb_samples);
      //write_output("channel.m","ch",desc1->ch[0],desc1->channel_length,1,8);
      //write_output("channelF.m","chF",desc1->chF[0],nb_samples,1,8);
      int count,count1,a_rx,a_tx;

      for(a_tx=0; a_tx<nb_antennas_tx; a_tx++) {
        for (a_rx=0; a_rx<nb_antennas_rx; a_rx++) {
          //for (count=0;count<frame_parms->symbols_per_tti/2;count++)
          for (count=0; count<1; count++) {
            for (count1=0; count1<frame_parms->N_RB_DL*12; count1++) {
              ((int16_t *) dl_channel_est[(a_tx<<1)+a_rx])[2*count1+(count*frame_parms->ofdm_symbol_size+LTE_CE_FILTER_LENGTH)*2]=(int16_t)(desc1->chF[a_rx+(a_tx*nb_antennas_rx)][count1].x*scale);
              ((int16_t *) dl_channel_est[(a_tx<<1)+a_rx])[2*count1+1+(count*frame_parms->ofdm_symbol_size+LTE_CE_FILTER_LENGTH)*2]=(int16_t)(desc1->chF[a_rx+(a_tx*nb_antennas_rx)][count1].y*scale) ;
            }
          }
        }
      }

      // calculate the SNR for the attached eNB (this assumes eNB always uses PMI stored in eNB_UE_stats; to be improved)
      init_snr(eNB2UE[att_eNB_id][UE_id][CC_id], enb_data[att_eNB_id], ue_data[UE_id], PHY_vars_UE_g[UE_id][CC_id]->sinr_dB, &PHY_vars_UE_g[UE_id][CC_id]->N0,
               PHY_vars_UE_g[UE_id][CC_id]->transmission_mode[att_eNB_id], PHY_vars_eNB_g[att_eNB_id][CC_id]->UE_stats[UE_id].DL_pmi_single,
	       PHY_vars_eNB_g[att_eNB_id][CC_id]->mu_mimo_mode[UE_id].dl_pow_off,PHY_vars_eNB_g[att_eNB_id][CC_id]->frame_parms.N_RB_DL);

      // calculate sinr here
      for (eNB_id = 0; eNB_id < NB_eNB_INST; eNB_id++) {
        if (att_eNB_id != eNB_id) {
          calculate_sinr(eNB2UE[eNB_id][UE_id][CC_id], enb_data[eNB_id], ue_data[UE_id], PHY_vars_UE_g[UE_id][CC_id]->sinr_dB,PHY_vars_eNB_g[att_eNB_id][CC_id]->frame_parms.N_RB_DL);
        }
      }
    } // hold channel
  }
  else { //abstraction_flag


    pthread_mutex_lock(&eNB_output_mutex[UE_id]);
 
    if (eNB_output_mask[UE_id] == 0) {  //  This is the first eNodeB for this UE, clear the buffer
      
      for (aa=0; aa<nb_antennas_rx; aa++) {
	memset((void*)r_re_DL[UE_id][aa],0,(frame_parms->samples_per_tti)*sizeof(double));
	memset((void*)r_im_DL[UE_id][aa],0,(frame_parms->samples_per_tti)*sizeof(double));
      }
    }
    pthread_mutex_unlock(&eNB_output_mutex[UE_id]);

    for (eNB_id=0; eNB_id<NB_eNB_INST; eNB_id++) {
      txdata = PHY_vars_eNB_g[eNB_id][CC_id]->common_vars.txdata[0];
      sf_offset = subframe*frame_parms->samples_per_tti;
      tx_pwr = dac_fixed_gain(s_re,
                              s_im,
                              txdata,
                              sf_offset,
                              nb_antennas_tx,
                              frame_parms->samples_per_tti,
                              sf_offset,
                              frame_parms->ofdm_symbol_size,
                              14,
                              frame_parms->pdsch_config_common.referenceSignalPower, // dBm/RE
                              frame_parms->N_RB_DL*12);

#ifdef DEBUG_SIM
      LOG_D(OCM,"[SIM][DL] eNB %d (CCid %d): tx_pwr %.1f dBm/RE (target %d dBm/RE), for subframe %d\n",
            eNB_id,CC_id,
            10*log10(tx_pwr),
            frame_parms->pdsch_config_common.referenceSignalPower,
            subframe);

#endif
      //eNB2UE[eNB_id][UE_id]->path_loss_dB = 0;
      multipath_channel(eNB2UE[eNB_id][UE_id][CC_id],s_re,s_im,r_re0,r_im0,
                        frame_parms->samples_per_tti,hold_channel);
#ifdef DEBUG_SIM
      rx_pwr = signal_energy_fp2(eNB2UE[eNB_id][UE_id][CC_id]->ch[0],
                                 eNB2UE[eNB_id][UE_id][CC_id]->channel_length)*eNB2UE[eNB_id][UE_id][CC_id]->channel_length;
      LOG_D(OCM,"[SIM][DL] Channel eNB %d => UE %d (CCid %d): Channel gain %f dB (%f)\n",eNB_id,UE_id,CC_id,10*log10(rx_pwr),rx_pwr);
#endif


#ifdef DEBUG_SIM

      for (i=0; i<eNB2UE[eNB_id][UE_id][CC_id]->channel_length; i++)
        LOG_D(OCM,"channel(%d,%d)[%d] : (%f,%f)\n",eNB_id,UE_id,i,eNB2UE[eNB_id][UE_id][CC_id]->ch[0][i].x,eNB2UE[eNB_id][UE_id][CC_id]->ch[0][i].y);

#endif

      LOG_D(OCM,"[SIM][DL] Channel eNB %d => UE %d (CCid %d): tx_power %.1f dBm/RE, path_loss %1.f dB\n",
            eNB_id,UE_id,CC_id,
            (double)frame_parms->pdsch_config_common.referenceSignalPower,
            //         enb_data[eNB_id]->tx_power_dBm,
            eNB2UE[eNB_id][UE_id][CC_id]->path_loss_dB);

#ifdef DEBUG_SIM
      rx_pwr = signal_energy_fp(r_re0,r_im0,nb_antennas_rx,
                                frame_parms->ofdm_symbol_size,
                                sf_offset)/(12.0*frame_parms->N_RB_DL);
      LOG_D(OCM,"[SIM][DL] UE %d : rx_pwr %f dBm/RE (%f dBm RSSI)for subframe %d\n",UE_id,
            10*log10(rx_pwr),
            10*log10(rx_pwr*(double)frame_parms->N_RB_DL*12),subframe);
      LOG_D(OCM,"[SIM][DL] UE %d : rx_pwr (noise) -132 dBm/RE (N0fs = %.1f dBm, N0B = %.1f dBm) for subframe %d\n",
            UE_id,
            10*log10(eNB2UE[eNB_id][UE_id][CC_id]->sampling_rate*1e6)-174,
            10*log10(eNB2UE[eNB_id][UE_id][CC_id]->sampling_rate*1e6*12*frame_parms->N_RB_DL/(double)frame_parms->ofdm_symbol_size)-174,
            subframe);
#endif

      if (eNB2UE[eNB_id][UE_id][CC_id]->first_run == 1)
        eNB2UE[eNB_id][UE_id][CC_id]->first_run = 0;


      // RF model
#ifdef DEBUG_SIM
      LOG_D(OCM,"[SIM][DL] UE %d (CCid %d): rx_gain %d dB (-ADC %f) for subframe %d\n",UE_id,CC_id,PHY_vars_UE_g[UE_id][CC_id]->rx_total_gain_dB,
            PHY_vars_UE_g[UE_id][CC_id]->rx_total_gain_dB-66.227,subframe);
#endif

      rf_rx_simple(r_re0,
                   r_im0,
                   nb_antennas_rx,
                   frame_parms->samples_per_tti,
                   1e3/eNB2UE[eNB_id][UE_id][CC_id]->sampling_rate,  // sampling time (ns)
                   (double)PHY_vars_UE_g[UE_id][CC_id]->rx_total_gain_dB - 66.227);   // rx_gain (dB) (66.227 = 20*log10(pow2(11)) = gain from the adc that will be applied later)

#ifdef DEBUG_SIM
      rx_pwr = signal_energy_fp(r_re0,r_im0,
                                nb_antennas_rx,
                                frame_parms->ofdm_symbol_size,
                                sf_offset)/(12.0*frame_parms->N_RB_DL);
      LOG_D(OCM,"[SIM][DL] UE %d : ADC in (eNB %d) %f dBm/RE for subframe %d\n",
            UE_id,eNB_id,
            10*log10(rx_pwr),subframe);
#endif
      
      pthread_mutex_lock(&eNB_output_mutex[UE_id]);
      for (i=0; i<frame_parms->samples_per_tti; i++) {
        for (aa=0; aa<nb_antennas_rx; aa++) {
          r_re_DL[UE_id][aa][i]+=r_re0[aa][i];
          r_im_DL[UE_id][aa][i]+=r_im0[aa][i];
        }
      }
      eNB_output_mask[UE_id] |= (1<<eNB_id);
      if (eNB_output_mask[UE_id] == (1<<NB_eNB_INST)-1) {
	eNB_output_mask[UE_id]=0;
      

	double *r_re_p[2] = {r_re_DL[UE_id][0],r_re_DL[UE_id][1]};
	double *r_im_p[2] = {r_im_DL[UE_id][0],r_im_DL[UE_id][1]};

#ifdef DEBUG_SIM
	rx_pwr = signal_energy_fp(r_re_p,r_im_p,nb_antennas_rx,frame_parms->ofdm_symbol_size,sf_offset)/(12.0*frame_parms->N_RB_DL);
	LOG_D(OCM,"[SIM][DL] UE %d : ADC in %f dBm for subframe %d\n",UE_id,10*log10(rx_pwr),subframe);
#endif
	
	rxdata = PHY_vars_UE_g[UE_id][CC_id]->common_vars.rxdata;
	sf_offset = subframe*frame_parms->samples_per_tti;


	adc(r_re_p,
	    r_im_p,
	    0,
	    sf_offset,
	    rxdata,
	    nb_antennas_rx,
	    frame_parms->samples_per_tti,
	    12);
	
#ifdef DEBUG_SIM
	rx_pwr2 = signal_energy(rxdata[0]+sf_offset,frame_parms->ofdm_symbol_size)/(12.0*frame_parms->N_RB_DL);
	LOG_D(OCM,"[SIM][DL] UE %d : rx_pwr (ADC out) %f dB/RE (%d) for subframe %d, writing to %p\n",UE_id, 10*log10((double)rx_pwr2),rx_pwr2,subframe,rxdata);
#else
	UNUSED_VARIABLE(rx_pwr2);
	UNUSED_VARIABLE(tx_pwr);
	UNUSED_VARIABLE(rx_pwr);
#endif
		
      } // eNB_output_mask
      pthread_mutex_unlock(&eNB_output_mutex[UE_id]);      
    } // eNB_id

  }

}


void do_UL_sig(channel_desc_t *UE2eNB[NUMBER_OF_UE_MAX][NUMBER_OF_eNB_MAX][MAX_NUM_CCs],
               node_desc_t *enb_data[NUMBER_OF_eNB_MAX],node_desc_t *ue_data[NUMBER_OF_UE_MAX],
	       uint16_t subframe,uint8_t abstraction_flag,LTE_DL_FRAME_PARMS *frame_parms, 
	       uint32_t frame,int eNB_id,uint8_t CC_id)
{

  int32_t **txdata,**rxdata;
#ifdef PHY_ABSTRACTION_UL
  int32_t att_eNB_id=-1;
#endif
  uint8_t UE_id=0;

  uint8_t nb_antennas_rx = UE2eNB[0][0][CC_id]->nb_rx; // number of rx antennas at eNB
  uint8_t nb_antennas_tx = UE2eNB[0][0][CC_id]->nb_tx; // number of tx antennas at UE

  double tx_pwr, rx_pwr;
  int32_t rx_pwr2;
  uint32_t i,aa;
  uint32_t sf_offset;

  uint8_t hold_channel=0;

#ifdef PHY_ABSTRACTION_UL
  double min_path_loss=-200;
  uint16_t ul_nb_rb=0 ;
  uint16_t ul_fr_rb=0;
  int ulnbrb2 ;
  int ulfrrb2 ;
  uint8_t harq_pid;
#endif
  double s_re0[30720];//PHY_vars_UE_g[UE_id][CC_id]->frame_parms.samples_per_tti];
  double s_re1[30720];//PHY_vars_UE_g[UE_id][CC_id]->frame_parms.samples_per_tti];
  double *s_re[2];
  double s_im0[30720];//PHY_vars_UE_g[UE_id][CC_id]->frame_parms.samples_per_tti];
  double s_im1[30720];//PHY_vars_UE_g[UE_id][CC_id]->frame_parms.samples_per_tti];
  double *s_im[2];
  double r_re00[30720];//PHY_vars_UE_g[UE_id][CC_id]->frame_parms.samples_per_tti];
  double r_re01[30720];//PHY_vars_UE_g[UE_id][CC_id]->frame_parms.samples_per_tti];
  double *r_re0[2];
  double r_im00[30720];//PHY_vars_UE_g[UE_id][CC_id]->frame_parms.samples_per_tti];
  double r_im01[30720];//PHY_vars_UE_g[UE_id][CC_id]->frame_parms.samples_per_tti];
  double *r_im0[2];

  s_re[0] = s_re0;
  s_im[0] = s_im0;
  s_re[1] = s_re1;
  s_im[1] = s_im1;

  r_re0[0] = r_re00;
  r_im0[0] = r_im00;
  r_re0[1] = r_re01;
  r_im0[1] = r_im01;

  if (abstraction_flag!=0)  {
#ifdef PHY_ABSTRACTION_UL

    for (UE_id=0; UE_id<NB_UE_INST; UE_id++) {
      if (!hold_channel) {
	random_channel(UE2eNB[UE_id][eNB_id][CC_id],abstraction_flag);
	freq_channel(UE2eNB[UE_id][eNB_id][CC_id], frame_parms->N_RB_UL,frame_parms->N_RB_UL*12+1);
	
	// REceived power at the eNB
	rx_pwr = signal_energy_fp2(UE2eNB[UE_id][eNB_id][CC_id]->ch[0],
				   UE2eNB[UE_id][eNB_id][CC_id]->channel_length)*UE2eNB[UE_id][att_eNB_id][CC_id]->channel_length; // calculate the rx power at the eNB
      }
      
      //  write_output("SINRch.m","SINRch",PHY_vars_eNB_g[att_eNB_id]->sinr_dB_eNB,frame_parms->N_RB_UL*12+1,1,1);
      if(subframe>1 && subframe <5) {
	harq_pid = subframe2harq_pid(frame_parms,frame,subframe);
	ul_nb_rb = PHY_vars_eNB_g[att_eNB_id][CC_id]->ulsch_eNB[(uint8_t)UE_id]->harq_processes[harq_pid]->nb_rb;
	ul_fr_rb = PHY_vars_eNB_g[att_eNB_id][CC_id]->ulsch_eNB[(uint8_t)UE_id]->harq_processes[harq_pid]->first_rb;
      }
      
      if(ul_nb_rb>1 && (ul_fr_rb < 25 && ul_fr_rb > -1)) {
	number_rb_ul = ul_nb_rb;
	first_rbUL = ul_fr_rb;
	init_snr_up(UE2eNB[UE_id][att_eNB_id][CC_id],enb_data[att_eNB_id], ue_data[UE_id],PHY_vars_eNB_g[att_eNB_id][CC_id]->sinr_dB,&PHY_vars_UE_g[att_eNB_id][CC_id]->N0,ul_nb_rb,ul_fr_rb);
	
      }
    } //UE_id

#else

#endif
  } else { //without abstraction

    pthread_mutex_lock(&UE_output_mutex[eNB_id]);
    // Clear RX signal for eNB = eNB_id
    for (i=0; i<frame_parms->samples_per_tti; i++) {
      for (aa=0; aa<nb_antennas_rx; aa++) {
	r_re_UL[eNB_id][aa][i]=0.0;
	r_im_UL[eNB_id][aa][i]=0.0;
      }
    }
    pthread_mutex_unlock(&UE_output_mutex[eNB_id]);

    // Compute RX signal for eNB = eNB_id
    for (UE_id=0; UE_id<NB_UE_INST; UE_id++) {
      
      txdata = PHY_vars_UE_g[UE_id][CC_id]->common_vars.txdata;
      sf_offset = subframe*frame_parms->samples_per_tti;
      
      if (((double)PHY_vars_UE_g[UE_id][CC_id]->tx_power_dBm[subframe] +
	   UE2eNB[UE_id][eNB_id][CC_id]->path_loss_dB) <= -125.0) {
	// don't simulate a UE that is too weak
	LOG_D(OCM,"[SIM][UL] UE %d tx_pwr %d dBm (num_RE %d) for subframe %d (sf_offset %d)\n",
	      UE_id,
	      PHY_vars_UE_g[UE_id][CC_id]->tx_power_dBm[subframe],
	      PHY_vars_UE_g[UE_id][CC_id]->tx_total_RE[subframe],
	      subframe,sf_offset);	
      } else {
	tx_pwr = dac_fixed_gain((double**)s_re,
				(double**)s_im,
				txdata,
				sf_offset,
				nb_antennas_tx,
				frame_parms->samples_per_tti,
				sf_offset,
				frame_parms->ofdm_symbol_size,
				14,
				(double)PHY_vars_UE_g[UE_id][CC_id]->tx_power_dBm[subframe]-10*log10((double)PHY_vars_UE_g[UE_id][CC_id]->tx_total_RE[subframe]),
				PHY_vars_UE_g[UE_id][CC_id]->tx_total_RE[subframe]);  // This make the previous argument the total power
	LOG_D(OCM,"[SIM][UL] UE %d tx_pwr %f dBm (target %d dBm, num_RE %d) for subframe %d (sf_offset %d)\n",
	      UE_id,
	      10*log10(tx_pwr),
	      PHY_vars_UE_g[UE_id][CC_id]->tx_power_dBm[subframe],
	      PHY_vars_UE_g[UE_id][CC_id]->tx_total_RE[subframe],
	      subframe,sf_offset);
       
		
	multipath_channel(UE2eNB[UE_id][eNB_id][CC_id],s_re,s_im,r_re0,r_im0,
			  frame_parms->samples_per_tti,hold_channel);
	

	rx_pwr = signal_energy_fp2(UE2eNB[UE_id][eNB_id][CC_id]->ch[0],
				   UE2eNB[UE_id][eNB_id][CC_id]->channel_length)*UE2eNB[UE_id][eNB_id][CC_id]->channel_length;

	LOG_D(OCM,"[SIM][UL] subframe %d Channel UE %d => eNB %d : %f dB (hold %d,length %d, PL %f)\n",subframe,UE_id,eNB_id,10*log10(rx_pwr),
	      hold_channel,UE2eNB[UE_id][eNB_id][CC_id]->channel_length,
	      UE2eNB[UE_id][eNB_id][CC_id]->path_loss_dB);

	rx_pwr = signal_energy_fp(r_re0,r_im0,nb_antennas_rx,frame_parms->samples_per_tti,0);
	LOG_D(OCM,"[SIM][UL] eNB %d : rx_pwr %f dBm (%f) for subframe %d, sptti %d\n",
	      eNB_id,10*log10(rx_pwr),rx_pwr,subframe,frame_parms->samples_per_tti);
	
	
	if (UE2eNB[UE_id][eNB_id][CC_id]->first_run == 1)
	  UE2eNB[UE_id][eNB_id][CC_id]->first_run = 0;
	
	
	pthread_mutex_lock(&UE_output_mutex[eNB_id]);
	for (aa=0; aa<nb_antennas_rx; aa++) {
	  for (i=0; i<frame_parms->samples_per_tti; i++) {
	    r_re_UL[eNB_id][aa][i]+=r_re0[aa][i];
	    r_im_UL[eNB_id][aa][i]+=r_im0[aa][i];
	  }
	}
	pthread_mutex_unlock(&UE_output_mutex[eNB_id]);
      }
    } //UE_id
    
    double *r_re_p[2] = {r_re_UL[eNB_id][0],r_re_UL[eNB_id][1]};
    double *r_im_p[2] = {r_im_UL[eNB_id][0],r_im_UL[eNB_id][1]};

    rf_rx_simple(r_re_p,
		 r_im_p,
		 nb_antennas_rx,
		 frame_parms->samples_per_tti,
		 1e3/UE2eNB[0][eNB_id][CC_id]->sampling_rate,  // sampling time (ns)
		 (double)PHY_vars_eNB_g[eNB_id][CC_id]->rx_total_gain_dB - 66.227);   // rx_gain (dB) (66.227 = 20*log10(pow2(11)) = gain from the adc that will be applied later)
    
#ifdef DEBUG_SIM
    rx_pwr = signal_energy_fp(r_re_p,r_im_p,nb_antennas_rx,frame_parms->samples_per_tti,0)*(double)frame_parms->ofdm_symbol_size/(12.0*frame_parms->N_RB_DL);
    LOG_D(OCM,"[SIM][UL] rx_pwr (ADC in) %f dB for subframe %d\n",10*log10(rx_pwr),subframe);
#endif
    
    rxdata = PHY_vars_eNB_g[eNB_id][CC_id]->common_vars.rxdata[0];
    sf_offset = subframe*frame_parms->samples_per_tti;

    
    adc(r_re_p,
	r_im_p,
	0,
	sf_offset,
	rxdata,
	nb_antennas_rx,
	frame_parms->samples_per_tti,
	12);
    
#ifdef DEBUG_SIM
    rx_pwr2 = signal_energy(rxdata[0]+sf_offset,frame_parms->samples_per_tti)*(double)frame_parms->ofdm_symbol_size/(12.0*frame_parms->N_RB_DL);
    LOG_D(OCM,"[SIM][UL] eNB %d rx_pwr (ADC out) %f dB (%d) for subframe %d (offset %d)\n",eNB_id,10*log10((double)rx_pwr2),rx_pwr2,subframe,sf_offset);
#else
    UNUSED_VARIABLE(tx_pwr);
    UNUSED_VARIABLE(rx_pwr);
    UNUSED_VARIABLE(rx_pwr2);
#endif
    
  } // abstraction_flag==0

}


void init_channel_vars(LTE_DL_FRAME_PARMS *frame_parms, double ***s_re,double ***s_im,double ***r_re,double ***r_im,double ***r_re0,double ***r_im0)
{

  int i;

  memset(eNB_output_mask,0,sizeof(int)*NUMBER_OF_UE_MAX);
  for (i=0;i<NB_UE_INST;i++)
    pthread_mutex_init(&eNB_output_mutex[i],NULL);

  memset(UE_output_mask,0,sizeof(int)*NUMBER_OF_eNB_MAX);
  for (i=0;i<NB_eNB_INST;i++)
    pthread_mutex_init(&UE_output_mutex[i],NULL);

}

