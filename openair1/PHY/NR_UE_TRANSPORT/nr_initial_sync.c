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

/*! \file PHY/LTE_TRANSPORT/initial_sync.c
* \brief Routines for initial UE synchronization procedure (PSS,SSS,PBCH and frame format detection)
* \author R. Knopp, F. Kaltenberger
* \date 2011
* \version 0.1
* \company Eurecom
* \email: knopp@eurecom.fr,kaltenberger@eurecom.fr
* \note
* \warning
*/
#include "PHY/types.h"
#include "PHY/defs_nr_UE.h"
#include "PHY/MODULATION/modulation_UE.h"
#include "nr_transport_proto_ue.h"
#include "PHY/NR_UE_ESTIMATION/nr_estimation.h"
#include "SCHED_NR_UE/defs.h"
#include "common/utils/LOG/vcd_signal_dumper.h"
#include "common/utils/nr/nr_common.h"

#include "common_lib.h"
#include <math.h>

#include "PHY/NR_REFSIG/pss_nr.h"
#include "PHY/NR_REFSIG/sss_nr.h"
#include "PHY/NR_REFSIG/refsig_defs_ue.h"

extern openair0_config_t openair0_cfg[];
//static  nfapi_nr_config_request_t config_t;
//static  nfapi_nr_config_request_t* config =&config_t;
int cnt=0;

#define DEBUG_INITIAL_SYNCH


// create a new node of SSB structure
NR_UE_SSB* create_ssb_node(uint8_t  i, uint8_t  h) {

  NR_UE_SSB *new_node = (NR_UE_SSB*)malloc(sizeof(NR_UE_SSB));
  new_node->i_ssb = i;
  new_node->n_hf = h;
  new_node->c_re = 0;
  new_node->c_im = 0;
  new_node->metric = 0;
  new_node->next_ssb = NULL;

  return new_node;
}


// insertion of the structure in the ordered list (highest metric first)
NR_UE_SSB* insert_into_list(NR_UE_SSB *head, NR_UE_SSB *node) {

  if (node->metric > head->metric) {
    node->next_ssb = head;
    head = node;
    return head;
  }

  NR_UE_SSB *current = head;
  while (current->next_ssb !=NULL) {
    NR_UE_SSB *temp=current->next_ssb;
    if(node->metric > temp->metric) {
      node->next_ssb = temp;
      current->next_ssb = node;
      return head;
    }
    else
      current = temp;
  }
  current->next_ssb = node;

  return head;
}


void free_list(NR_UE_SSB *node) {
  if (node->next_ssb != NULL)
    free_list(node->next_ssb);
  free(node);
}


int nr_pbch_detection(UE_nr_rxtx_proc_t * proc, PHY_VARS_NR_UE *ue, int pbch_initial_symbol)
{
  NR_DL_FRAME_PARMS *frame_parms=&ue->frame_parms;
  int ret =-1;

  NR_UE_SSB *best_ssb = NULL;
  NR_UE_SSB *current_ssb;

#ifdef DEBUG_INITIAL_SYNCH
  LOG_I(PHY,"[UE%d] Initial sync: starting PBCH detection (rx_offset %d)\n",ue->Mod_id,
        ue->rx_offset);
#endif

  uint8_t  N_L = (frame_parms->Lmax == 4)? 4:8;
  uint8_t  N_hf = (frame_parms->Lmax == 4)? 2:1;

  // loops over possible pbch dmrs cases to retrive best estimated i_ssb (and n_hf for Lmax=4) for multiple ssb detection
  for (int hf = 0; hf < N_hf; hf++) {
    for (int l = 0; l < N_L ; l++) {

      // initialization of structure
      current_ssb = create_ssb_node(l,hf);

      start_meas(&ue->dlsch_channel_estimation_stats);
      // computing correlation between received DMRS symbols and transmitted sequence for current i_ssb and n_hf
      for(int i=pbch_initial_symbol; i<pbch_initial_symbol+3;i++)
          nr_pbch_dmrs_correlation(ue,proc,0,0,i,i-pbch_initial_symbol,current_ssb);
      stop_meas(&ue->dlsch_channel_estimation_stats);
      
      current_ssb->metric = current_ssb->c_re*current_ssb->c_re + current_ssb->c_im*current_ssb->c_im;
      
      // generate a list of SSB structures
      if (best_ssb == NULL)
        best_ssb = current_ssb;
      else
        best_ssb = insert_into_list(best_ssb,current_ssb);

    }
  }

  NR_UE_SSB *temp_ptr=best_ssb;
  while (ret!=0 && temp_ptr != NULL) {

    start_meas(&ue->dlsch_channel_estimation_stats);
  // computing channel estimation for selected best ssb
    for(int i=pbch_initial_symbol; i<pbch_initial_symbol+3;i++)
      nr_pbch_channel_estimation(ue,proc,0,0,i,i-pbch_initial_symbol,temp_ptr->i_ssb,temp_ptr->n_hf);
    stop_meas(&ue->dlsch_channel_estimation_stats);

    ret = nr_rx_pbch(ue,
                     proc,
                     ue->pbch_vars[0],
                     frame_parms,
                     0,
                     temp_ptr->i_ssb,
                     SISO);

    temp_ptr=temp_ptr->next_ssb;
  }

  free_list(best_ssb);

  
  if (ret==0) {
    
    frame_parms->nb_antenna_ports_gNB = 1; //pbch_tx_ant;
    
    // set initial transmission mode to 1 or 2 depending on number of detected TX antennas
    //frame_parms->mode1_flag = (pbch_tx_ant==1);
    // openair_daq_vars.dlsch_transmission_mode = (pbch_tx_ant>1) ? 2 : 1;


    // flip byte endian on 24-bits for MIB
    //    dummy = ue->pbch_vars[0]->decoded_output[0];
    //    ue->pbch_vars[0]->decoded_output[0] = ue->pbch_vars[0]->decoded_output[2];
    //    ue->pbch_vars[0]->decoded_output[2] = dummy;

#ifdef DEBUG_INITIAL_SYNCH
    LOG_I(PHY,"[UE%d] Initial sync: pbch decoded sucessfully\n",ue->Mod_id);
#endif
    return(0);
  } else {
    return(-1);
  }

}

char duplex_string[2][4] = {"FDD","TDD"};
char prefix_string[2][9] = {"NORMAL","EXTENDED"};

int nr_initial_sync(UE_nr_rxtx_proc_t *proc,
                    PHY_VARS_NR_UE *ue,
                    int n_frames, int sa,
                    int dlsch_parallel)
{

  int32_t sync_pos, sync_pos_frame; // k_ssb, N_ssb_crb, sync_pos2,
  int32_t accumulated_freq_offset = 0;
  int32_t metric_tdd_ncp=0;
  uint8_t phase_tdd_ncp;
  double im, re;
  int is;

  NR_DL_FRAME_PARMS *fp = &ue->frame_parms;
  int ret=-1;
  int rx_power=0; //aarx,
  
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_NR_INITIAL_UE_SYNC, VCD_FUNCTION_IN);


  LOG_D(PHY,"nr_initial sync ue RB_DL %d\n", fp->N_RB_DL);

  /*   Initial synchronisation
   *
  *                                 1 radio frame = 10 ms
  *     <--------------------------------------------------------------------------->
  *     -----------------------------------------------------------------------------
  *     |                                 Received UE data buffer                    |
  *     ----------------------------------------------------------------------------
  *                     --------------------------
  *     <-------------->| pss | pbch | sss | pbch |
  *                     --------------------------
  *          sync_pos            SS/PBCH block
  */

  cnt++;
  if (1){ // (cnt>100)
   cnt =0;

   // initial sync performed on two successive frames, if pbch passes on first frame, no need to process second frame 
   // only one frame is used for symulation tools
   for(is=0; is<n_frames;is++) {

    /* process pss search on received buffer */
    sync_pos = pss_synchro_nr(ue, is, NO_RATE_CHANGE);

    if (sync_pos >= fp->nb_prefix_samples)
      ue->ssb_offset = sync_pos - fp->nb_prefix_samples;
    else
      ue->ssb_offset = sync_pos + (fp->samples_per_subframe * 10) - fp->nb_prefix_samples;

#ifdef DEBUG_INITIAL_SYNCH
    LOG_I(PHY,"[UE%d] Initial sync : Estimated PSS position %d, Nid2 %d\n", ue->Mod_id, sync_pos,ue->common_vars.eNb_id);
    LOG_I(PHY,"sync_pos %d ssb_offset %d \n",sync_pos,ue->ssb_offset);
#endif

    // digital compensation of FFO for SSB symbols
    if (ue->UE_fo_compensation){
      double s_time = 1/(1.0e3*fp->samples_per_subframe);  // sampling time
      double off_angle = -2*M_PI*s_time*(ue->common_vars.freq_offset);  // offset rotation angle compensation per sample
      accumulated_freq_offset += ue->common_vars.freq_offset;

      // In SA we need to perform frequency offset correction until the end of buffer because we need to decode SIB1
      // and we do not know yet in which slot it goes.

      // start for offset correction
      int start = sa ? is*fp->samples_per_frame : is*fp->samples_per_frame + ue->ssb_offset;

      // loop over samples
      int end = sa ? n_frames*fp->samples_per_frame-1 : start + NR_N_SYMBOLS_SSB*(fp->ofdm_symbol_size + fp->nb_prefix_samples);

      for(int n=start; n<end; n++){
        for (int ar=0; ar<fp->nb_antennas_rx; ar++) {
          re = ((double)(((short *)ue->common_vars.rxdata[ar]))[2*n]);
          im = ((double)(((short *)ue->common_vars.rxdata[ar]))[2*n+1]);
          ((short *)ue->common_vars.rxdata[ar])[2*n] = (short)(round(re*cos(n*off_angle) - im*sin(n*off_angle)));
          ((short *)ue->common_vars.rxdata[ar])[2*n+1] = (short)(round(re*sin(n*off_angle) + im*cos(n*off_angle)));
        }
      }
    }

    /* check that SSS/PBCH block is continuous inside the received buffer */
    if (sync_pos < (NR_NUMBER_OF_SUBFRAMES_PER_FRAME*fp->samples_per_subframe - (NB_SYMBOLS_PBCH * fp->ofdm_symbol_size))) {

    /* slop_fep function works for lte and takes into account begining of frame with prefix for subframe 0 */
    /* for NR this is not the case but slot_fep is still used for computing FFT of samples */
    /* in order to achieve correct processing for NR prefix samples is forced to 0 and then restored after function call */
    /* symbol number are from beginning of SS/PBCH blocks as below:  */
    /*    Signal            PSS  PBCH  SSS  PBCH                     */
    /*    symbol number      0     1    2    3                       */
    /* time samples in buffer rxdata are used as input of FFT -> FFT results are stored in the frequency buffer rxdataF */
    /* rxdataF stores SS/PBCH from beginning of buffers in the same symbol order as in time domain */

      for(int i=0; i<4;i++)
        nr_slot_fep_init_sync(ue,
                              proc,
                              i,
                              0,
                              is*fp->samples_per_frame+ue->ssb_offset);

#ifdef DEBUG_INITIAL_SYNCH
      LOG_I(PHY,"Calling sss detection (normal CP)\n");
#endif

      int freq_offset_sss = 0;
      rx_sss_nr(ue, proc, &metric_tdd_ncp, &phase_tdd_ncp, &freq_offset_sss);

      accumulated_freq_offset += freq_offset_sss;

      // digital compensation of FFO for SSB symbols
      if (ue->UE_fo_compensation){
        double s_time = 1/(1.0e3*fp->samples_per_subframe);  // sampling time
        double off_angle = -2*M_PI*s_time*freq_offset_sss;   // offset rotation angle compensation per sample

        // In SA we need to perform frequency offset correction until the end of buffer because we need to decode SIB1
        // and we do not know yet in which slot it goes.

        // start for offset correction
        int start = sa ? is*fp->samples_per_frame : is*fp->samples_per_frame + ue->ssb_offset;

        // loop over samples
        int end = sa ? n_frames*fp->samples_per_frame-1 : start + NR_N_SYMBOLS_SSB*(fp->ofdm_symbol_size + fp->nb_prefix_samples);

        for(int n=start; n<end; n++){
          for (int ar=0; ar<fp->nb_antennas_rx; ar++) {
            re = ((double)(((short *)ue->common_vars.rxdata[ar]))[2*n]);
            im = ((double)(((short *)ue->common_vars.rxdata[ar]))[2*n+1]);
            ((short *)ue->common_vars.rxdata[ar])[2*n] = (short)(round(re*cos(n*off_angle) - im*sin(n*off_angle)));
            ((short *)ue->common_vars.rxdata[ar])[2*n+1] = (short)(round(re*sin(n*off_angle) + im*cos(n*off_angle)));
          }
        }
      }

      nr_gold_pbch(ue);
      ret = nr_pbch_detection(proc, ue, 1);  // start pbch detection at first symbol after pss

      if (ret == 0) {
        // sync at symbol ue->symbol_offset
        // computing the offset wrt the beginning of the frame
        int mu = fp->numerology_index;
        // number of symbols with different prefix length
        // every 7*(1<<mu) symbols there is a different prefix length (38.211 5.3.1)
        int n_symb_prefix0 = (ue->symbol_offset/(7*(1<<mu)))+1;
        sync_pos_frame = n_symb_prefix0*(fp->ofdm_symbol_size + fp->nb_prefix_samples0)+(ue->symbol_offset-n_symb_prefix0)*(fp->ofdm_symbol_size + fp->nb_prefix_samples);
        // for a correct computation of frame number to sync with the one decoded at MIB we need to take into account in which of the n_frames we got sync
        ue->init_sync_frame = n_frames - 1 - is;
        // we also need to take into account the shift by samples_per_frame in case the if is true
        if (ue->ssb_offset < sync_pos_frame){
          ue->rx_offset = fp->samples_per_frame - sync_pos_frame + ue->ssb_offset;
          ue->init_sync_frame += 1;
        }
        else
          ue->rx_offset = ue->ssb_offset - sync_pos_frame;
      }   

    /*
    int nb_prefix_samples0 = fp->nb_prefix_samples0;
    fp->nb_prefix_samples0 = fp->nb_prefix_samples;
	  
    nr_slot_fep(ue, proc, 0, 0, ue->ssb_offset, 0, NR_PDCCH_EST);
    nr_slot_fep(ue, proc, 1, 0, ue->ssb_offset, 0, NR_PDCCH_EST);
    fp->nb_prefix_samples0 = nb_prefix_samples0;	

    LOG_I(PHY,"[UE  %d] AUTOTEST Cell Sync : frame = %d, rx_offset %d, freq_offset %d \n",
              ue->Mod_id,
              ue->proc.proc_rxtx[0].frame_rx,
              ue->rx_offset,
              ue->common_vars.freq_offset );
    */


#ifdef DEBUG_INITIAL_SYNCH
      LOG_I(PHY,"TDD Normal prefix: CellId %d metric %d, phase %d, pbch %d\n",
            fp->Nid_cell,metric_tdd_ncp,phase_tdd_ncp,ret);
#endif

      }
      else {
#ifdef DEBUG_INITIAL_SYNCH
       LOG_I(PHY,"TDD Normal prefix: SSS error condition: sync_pos %d\n", sync_pos);
#endif
      }
      if (ret == 0) break;
    }
  }
  else {
    ret = -1;
  }

  /* Consider this is a false detection if the offset is > 1000 Hz 
     Not to be used now that offest estimation is in place
  if( (abs(ue->common_vars.freq_offset) > 150) && (ret == 0) )
  {
	  ret=-1;
	  LOG_E(HW, "Ignore MIB with high freq offset [%d Hz] estimation \n",ue->common_vars.freq_offset);
  }*/

  if (ret==0) {  // PBCH found so indicate sync to higher layers and configure frame parameters

    //#ifdef DEBUG_INITIAL_SYNCH

    LOG_I(PHY, "[UE%d] In synch, rx_offset %d samples\n",ue->Mod_id, ue->rx_offset);

    //#endif

    if (ue->UE_scan_carrier == 0) {

    #if UE_AUTOTEST_TRACE
      LOG_I(PHY,"[UE  %d] AUTOTEST Cell Sync : rx_offset %d, freq_offset %d \n",
              ue->Mod_id,
              ue->rx_offset,
              ue->common_vars.freq_offset );
    #endif

// send sync status to higher layers later when timing offset converge to target timing

      ue->pbch_vars[0]->pdu_errors_conseq=0;

    }

    LOG_I(PHY, "[UE %d] RRC Measurements => rssi %3.1f dBm (dig %3.1f dB, gain %d), N0 %d dBm,  rsrp %3.1f dBm/RE, rsrq %3.1f dB\n",ue->Mod_id,
	  10*log10(ue->measurements.rssi)-ue->rx_total_gain_dB,
	  10*log10(ue->measurements.rssi),
	  ue->rx_total_gain_dB,
	  ue->measurements.n0_power_tot_dBm,
	  10*log10(ue->measurements.rsrp[0])-ue->rx_total_gain_dB,
	  (10*log10(ue->measurements.rsrq[0])));

/*    LOG_I(PHY, "[UE %d] Frame %d MIB Information => %s, %s, NidCell %d, N_RB_DL %d, PHICH DURATION %d, PHICH RESOURCE %s, TX_ANT %d\n",
	  ue->Mod_id,
	  ue->proc.proc_rxtx[0].frame_rx,
	  duplex_string[fp->frame_type],
	  prefix_string[fp->Ncp],
	  fp->Nid_cell,
	  fp->N_RB_DL,
	  fp->phich_config_common.phich_duration,
	  phich_string[fp->phich_config_common.phich_resource],
	  fp->nb_antenna_ports_gNB);*/

#if defined(OAI_USRP) || defined(EXMIMO) || defined(OAI_BLADERF) || defined(OAI_LMSSDR) || defined(OAI_ADRV9371_ZC706)
    LOG_I(PHY, "[UE %d] Measured Carrier Frequency %.0f Hz (offset %d Hz)\n",
	  ue->Mod_id,
	  openair0_cfg[0].rx_freq[0]+ue->common_vars.freq_offset,
	  ue->common_vars.freq_offset);
#endif
  } else {
#ifdef DEBUG_INITIAL_SYNC
    LOG_I(PHY,"[UE%d] Initial sync : PBCH not ok\n",ue->Mod_id);
    LOG_I(PHY,"[UE%d] Initial sync : Estimated PSS position %d, Nid2 %d\n",ue->Mod_id,sync_pos,ue->common_vars.eNb_id);
    LOG_I(PHY,"[UE%d] Initial sync : Estimated Nid_cell %d, Frame_type %d\n",ue->Mod_id,
          frame_parms->Nid_cell,frame_parms->frame_type);
#endif

    ue->UE_mode[0] = NOT_SYNCHED;
    ue->pbch_vars[0]->pdu_errors_last=ue->pbch_vars[0]->pdu_errors;
    ue->pbch_vars[0]->pdu_errors++;
    ue->pbch_vars[0]->pdu_errors_conseq++;

  }

  // gain control
  if (ret!=0) { //we are not synched, so we cannot use rssi measurement (which is based on channel estimates)
    rx_power = 0;

    // do a measurement on the best guess of the PSS
    //for (aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++)
    //  rx_power += signal_energy(&ue->common_vars.rxdata[aarx][sync_pos2],
	//			frame_parms->ofdm_symbol_size+frame_parms->nb_prefix_samples);

    /*
    // do a measurement on the full frame
    for (aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++)
      rx_power += signal_energy(&ue->common_vars.rxdata[aarx][0],
				frame_parms->samples_per_subframe*10);
    */

    // we might add a low-pass filter here later
    ue->measurements.rx_power_avg[0] = rx_power/fp->nb_antennas_rx;

    ue->measurements.rx_power_avg_dB[0] = dB_fixed(ue->measurements.rx_power_avg[0]);

#ifdef DEBUG_INITIAL_SYNCH
  LOG_I(PHY,"[UE%d] Initial sync : Estimated power: %d dB\n",ue->Mod_id,ue->measurements.rx_power_avg_dB[0] );
#endif

#ifndef OAI_USRP
#ifndef OAI_BLADERF
#ifndef OAI_LMSSDR
#ifndef OAI_ADRV9371_ZC706
  //phy_adjust_gain(ue,ue->measurements.rx_power_avg_dB[0],0);
#endif
#endif
#endif
#endif

  }
  else {

#ifndef OAI_USRP
#ifndef OAI_BLADERF
#ifndef OAI_LMSSDR
#ifndef OAI_ADRV9371_ZC706
  //phy_adjust_gain(ue,dB_fixed(ue->measurements.rssi),0);
#endif
#endif
#endif
#endif

  }

  // if stand alone and sync on ssb do sib1 detection as part of initial sync
  if (sa==1 && ret==0) {
    bool dec = false;
    NR_UE_PDCCH *pdcch_vars  = ue->pdcch_vars[proc->thread_id][0];
    int gnb_id = 0; //FIXME
    int coreset_nb_rb=0;
    int coreset_start_rb=0;

    for(int n_ss = 0; n_ss<pdcch_vars->nb_search_space; n_ss++) {
      uint8_t nb_symb_pdcch = pdcch_vars->pdcch_config[n_ss].coreset.duration;
      int start_symb = pdcch_vars->pdcch_config[n_ss].coreset.StartSymbolIndex;
      get_coreset_rballoc(pdcch_vars->pdcch_config[n_ss].coreset.frequency_domain_resource,&coreset_nb_rb,&coreset_start_rb);
      for (uint16_t l=start_symb; l<start_symb+nb_symb_pdcch; l++) {
        nr_slot_fep_init_sync(ue,
                              proc,
                              l, // the UE PHY has no notion of the symbols to be monitored in the search space
                              pdcch_vars->slot,
                              is*fp->samples_per_frame+pdcch_vars->sfn*fp->samples_per_frame+ue->rx_offset);

        if (coreset_nb_rb > 0)
          nr_pdcch_channel_estimation(ue,
                                      proc,
                                      0,
                                      pdcch_vars->slot,
                                      l,
                                      fp->first_carrier_offset+(pdcch_vars->pdcch_config[n_ss].BWPStart + coreset_start_rb)*12,
                                      coreset_nb_rb);

      }
      int  dci_cnt = nr_ue_pdcch_procedures(gnb_id, ue, proc, n_ss);
      if (dci_cnt>0){
        NR_UE_DLSCH_t *dlsch = ue->dlsch_SI[gnb_id];
        if (dlsch && (dlsch->active == 1)) {
          uint8_t harq_pid = dlsch->current_harq_pid;
          NR_DL_UE_HARQ_t *dlsch0_harq = dlsch->harq_processes[harq_pid];
          uint16_t nb_symb_sch = dlsch0_harq->nb_symbols;
          uint16_t start_symb_sch = dlsch0_harq->start_symbol;

          for (uint16_t m=start_symb_sch;m<(nb_symb_sch+start_symb_sch) ; m++){
            nr_slot_fep_init_sync(ue,
                                  proc,
                                  m,
                                  pdcch_vars->slot,  // same slot and offset as pdcch
                                  is*fp->samples_per_frame+pdcch_vars->sfn*fp->samples_per_frame+ue->rx_offset);
          }

          int ret = nr_ue_pdsch_procedures(ue,
                                           proc,
                                           gnb_id,
                                           SI_PDSCH,
                                           ue->dlsch_SI[gnb_id],
                                           NULL);
          if (ret >= 0)
            dec = nr_ue_dlsch_procedures(ue,
                                         proc,
                                         gnb_id,
                                         SI_PDSCH,
                                         ue->dlsch_SI[gnb_id],
                                         NULL,
                                         &ue->dlsch_SI_errors[gnb_id],
                                         dlsch_parallel);

          // deactivate dlsch once dlsch proc is done
          ue->dlsch_SI[gnb_id]->active = 0;
        }
      }
    }
    if (dec == false) // sib1 not decoded
      ret = -1;

    ue->common_vars.freq_offset = accumulated_freq_offset;
  }
  //  exit_fun("debug exit");
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_NR_INITIAL_UE_SYNC, VCD_FUNCTION_OUT);
  return ret;
}

