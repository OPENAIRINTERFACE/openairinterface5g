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

/*! \file PHY/LTE_TRANSPORT/dlsch_demodulation.c
 * \brief Top-level routines for demodulating the PDSCH physical channel from 36-211, V8.6 2009-03
 * \author R. Knopp, F. Kaltenberger,A. Bhamri, S. Aubert, X. Xiang
 * \date 2011
 * \version 0.1
 * \company Eurecom
 * \email: knopp@eurecom.fr,florian.kaltenberger@eurecom.fr,ankit.bhamri@eurecom.fr,sebastien.aubert@eurecom.fr
 * \note
 * \warning
 */
#include "PHY/defs_UE.h"
#include "PHY/phy_extern.h"
#include "SCHED_UE/sched_UE.h"
#include "transport_ue.h"
#include "transport_proto_ue.h"
#include "PHY/sse_intrin.h"
#include "T.h"
#include<stdio.h>
#include<math.h>
#include <stdlib.h>
#include <string.h>
#include <linux/version.h>
#include <lapacke_utils.h>
#include <lapacke.h>
#include "linear_preprocessing_rec.h"

//#define DEBUG_MMSE


/* dynamic shift for LLR computation for TM3/4
 * set as command line argument, see lte-softmodem.c
 * default value: 0
 */
int16_t dlsch_demod_shift = 0;
int16_t interf_unaw_shift = 13;

unsigned char offset_mumimo_llr_drange_fix=0;
//inferference-free case
unsigned char interf_unaw_shift_tm4_mcs[29]= {5, 3, 4, 3, 3, 2, 1, 1, 2, 0, 1, 1, 1, 1, 0, 0,
                                              1, 1, 1, 1, 0, 2, 1, 0, 1, 0, 1, 0, 0
                                             } ;
unsigned char interf_unaw_shift_tm1_mcs[29]= {5, 5, 4, 3, 3, 3, 2, 2, 4, 4, 2, 3, 3, 3, 1, 1,
                                              0, 1, 1, 2, 5, 4, 4, 6, 5, 1, 0, 5, 6
                                             } ; // mcs 21, 26, 28 seem to be errorneous

/*
//original values from sebastion + same hand tuning
unsigned char offset_mumimo_llr_drange[29][3]={{8,8,8},{7,7,7},{7,7,7},{7,7,7},{6,6,6},{6,6,6},{6,6,6},{5,5,5},{4,4,4},{1,2,4}, // QPSK
{5,5,4},{5,5,5},{5,5,5},{3,3,3},{2,2,2},{2,2,2},{2,2,2}, // 16-QAM
{2,2,1},{3,3,3},{3,3,3},{3,3,1},{2,2,2},{2,2,2},{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0}}; //64-QAM
*/
/*
//first optimization try
unsigned char offset_mumimo_llr_drange[29][3]={{7, 8, 7},{6, 6, 7},{6, 6, 7},{6, 6, 6},{5, 6, 6},{5, 5, 6},{5, 5, 6},{4, 5, 4},{4, 3, 4},{3, 2, 2},{6, 5, 5},{5, 4, 4},{5, 5, 4},{3, 3, 2},{2, 2, 1},{2, 1, 1},{2, 2, 2},{3, 3, 3},{3, 3, 2},{3, 3, 2},{3, 2, 1},{2, 2, 2},{2, 2, 2},{0, 0, 0},{0, 0, 0},{0, 0, 0},{0, 0, 0},{0, 0, 0}};
*/
//second optimization try
/*
  unsigned char offset_mumimo_llr_drange[29][3]={{5, 8, 7},{4, 6, 8},{3, 6, 7},{7, 7, 6},{4, 7, 8},{4, 7, 4},{6, 6, 6},{3, 6, 6},{3, 6, 6},{1, 3, 4},{1, 1, 0},{3, 3, 2},{3, 4, 1},{4, 0, 1},{4, 2, 2},{3, 1, 2},{2, 1, 0},{2, 1, 1},{1, 0, 1},{1, 0, 1},{0, 0, 0},{1, 0, 0},{0, 0, 0},{0, 1, 0},{1, 0, 0},{0, 0, 0},{0, 0, 0},{0, 0, 0},{0, 0, 0}};  w
*/
unsigned char offset_mumimo_llr_drange[29][3]= {{0, 6, 5},{0, 4, 5},{0, 4, 5},{0, 5, 4},{0, 5, 6},{0, 5, 3},{0, 4, 4},{0, 4, 4},{0, 3, 3},{0, 1, 2},{1, 1, 0},{1, 3, 2},{3, 4, 1},{2, 0, 0},{2, 2, 2},{1, 1, 1},{2, 1, 0},{2, 1, 1},{1, 0, 1},{1, 0, 1},{0, 0, 0},{1, 0, 0},{0, 0, 0},{0, 1, 0},{1, 0, 0},{0, 0, 0},{0, 0, 0},{0, 0, 0},{0, 0, 0}};


extern void print_shorts(char *s,int16_t *x);


int rx_pdsch(PHY_VARS_UE *ue,
             PDSCH_t type,
             unsigned char eNB_id,
             unsigned char eNB_id_i, //if this == ue->n_connected_eNB, we assume MU interference
             uint32_t frame,
             uint8_t subframe,
             unsigned char symbol,
             unsigned char first_symbol_flag,
             RX_type_t rx_type,
             unsigned char i_mod,
             unsigned char harq_pid) {
  LTE_UE_COMMON *common_vars  = &ue->common_vars;
  LTE_UE_PDSCH **pdsch_vars;
  LTE_DL_FRAME_PARMS *frame_parms    = &ue->frame_parms;
  PHY_MEASUREMENTS *measurements = &ue->measurements;
  LTE_UE_DLSCH_t   **dlsch;
  int avg[4];
  int avg_0[2];
  int avg_1[2];
  unsigned short mmse_flag=0;
#if UE_TIMING_TRACE
  uint8_t slot = 0;
#endif
  unsigned char aatx,aarx;
  unsigned short nb_rb = 0, round;
  int avgs = 0, rb;
  LTE_DL_UE_HARQ_t *dlsch0_harq,*dlsch1_harq = 0;
  uint8_t beamforming_mode;
  uint32_t *rballoc;
  int32_t **rxdataF_comp_ptr;
  int32_t **dl_ch_mag_ptr;
  int32_t codeword_TB0 = -1;
  int32_t codeword_TB1 = -1;

  switch (type) {
    case SI_PDSCH:
      pdsch_vars = &ue->pdsch_vars_SI[eNB_id];
      dlsch = &ue->dlsch_SI[eNB_id];
      dlsch0_harq = dlsch[0]->harq_processes[harq_pid];
      beamforming_mode  = 0;
      break;

    case RA_PDSCH:
      pdsch_vars = &ue->pdsch_vars_ra[eNB_id];
      dlsch = &ue->dlsch_ra[eNB_id];
      dlsch0_harq = dlsch[0]->harq_processes[harq_pid];
      beamforming_mode  = 0;
      break;

    case PDSCH:
      pdsch_vars = ue->pdsch_vars[ue->current_thread_id[subframe]];
      dlsch = ue->dlsch[ue->current_thread_id[subframe]][eNB_id];
      //printf("status TB0 = %d, status TB1 = %d \n", dlsch[0]->harq_processes[harq_pid]->status, dlsch[1]->harq_processes[harq_pid]->status);
      LOG_D(PHY,"AbsSubframe %d.%d / Sym %d harq_pid %d,  harq status %d.%d \n",
            frame,subframe,symbol,harq_pid,
            dlsch[0]->harq_processes[harq_pid]->status,
            dlsch[1]->harq_processes[harq_pid]->status);

      if ((dlsch[0]->harq_processes[harq_pid]->status == ACTIVE) &&
          (dlsch[1]->harq_processes[harq_pid]->status == ACTIVE)) {
        codeword_TB0 = dlsch[0]->harq_processes[harq_pid]->codeword;
        codeword_TB1 = dlsch[1]->harq_processes[harq_pid]->codeword;
        dlsch0_harq = dlsch[codeword_TB0]->harq_processes[harq_pid];
        dlsch1_harq = dlsch[codeword_TB1]->harq_processes[harq_pid];
#ifdef DEBUG_HARQ
        printf("[DEMOD] I am assuming both TBs are active\n");
#endif
      } else if ((dlsch[0]->harq_processes[harq_pid]->status == ACTIVE) &&
                 (dlsch[1]->harq_processes[harq_pid]->status != ACTIVE) ) {
        codeword_TB0 = dlsch[0]->harq_processes[harq_pid]->codeword;
        dlsch0_harq = dlsch[0]->harq_processes[harq_pid];
        dlsch1_harq = NULL;
        codeword_TB1 = -1;
#ifdef DEBUG_HARQ
        printf("[DEMOD] I am assuming only TB0 is active\n");
#endif
      } else if ((dlsch[0]->harq_processes[harq_pid]->status != ACTIVE) &&
                 (dlsch[1]->harq_processes[harq_pid]->status == ACTIVE) ) {
        codeword_TB1 = dlsch[1]->harq_processes[harq_pid]->codeword;
        dlsch0_harq  = dlsch[1]->harq_processes[harq_pid];
        dlsch1_harq  = NULL;
        codeword_TB0 = -1;
#ifdef DEBUG_HARQ
        printf("[DEMOD] I am assuming only TB1 is active, it is in cw %d\n", dlsch0_harq->codeword);
#endif
      } else {
        LOG_E(PHY,"[UE][FATAL] Frame %d subframe %d: no active DLSCH\n",ue->proc.proc_rxtx[0].frame_rx,subframe);
        return(-1);
      }

      beamforming_mode  = ue->transmission_mode[eNB_id]<7?0:ue->transmission_mode[eNB_id];
      break;

    default:
      LOG_E(PHY,"[UE][FATAL] Frame %d subframe %d: Unknown PDSCH format %d\n",ue->proc.proc_rxtx[0].frame_rx,subframe,type);
      return(-1);
      break;
  }

#ifdef DEBUG_HARQ
  printf("[DEMOD] MIMO mode = %d\n", dlsch0_harq->mimo_mode);
  printf("[DEMOD] cw for TB0 = %d, cw for TB1 = %d\n", codeword_TB0, codeword_TB1);
#endif
  DevAssert(dlsch0_harq);
  round = dlsch0_harq->round;
  //printf("round = %d\n", round);

  if (eNB_id > 2) {
    LOG_W(PHY,"dlsch_demodulation.c: Illegal eNB_id %d\n",eNB_id);
    return(-1);
  }

  if (!common_vars) {
    LOG_W(PHY,"dlsch_demodulation.c: Null common_vars\n");
    return(-1);
  }

  if (!dlsch[0]) {
    LOG_W(PHY,"dlsch_demodulation.c: Null dlsch_ue pointer\n");
    return(-1);
  }

  if (!pdsch_vars) {
    LOG_W(PHY,"dlsch_demodulation.c: Null pdsch_vars pointer\n");
    return(-1);
  }

  if (!frame_parms) {
    LOG_W(PHY,"dlsch_demodulation.c: Null frame_parms\n");
    return(-1);
  }

  if (((frame_parms->Ncp == NORMAL) && (symbol>=7)) ||
      ((frame_parms->Ncp == EXTENDED) && (symbol>=6)))
    rballoc = dlsch0_harq->rb_alloc_odd;
  else
    rballoc = dlsch0_harq->rb_alloc_even;

  if (dlsch0_harq->mimo_mode>DUALSTREAM_PUSCH_PRECODING) {
    LOG_E(PHY,"This transmission mode is not yet supported!\n");
    return(-1);
  }

  if ((dlsch0_harq->mimo_mode==LARGE_CDD) || ((dlsch0_harq->mimo_mode>=DUALSTREAM_UNIFORM_PRECODING1) && (dlsch0_harq->mimo_mode<=DUALSTREAM_PUSCH_PRECODING)))  {
    DevAssert(dlsch1_harq);

    if (eNB_id!=eNB_id_i) {
      LOG_E(PHY,"TM3/TM4 requires to set eNB_id==eNB_id_i!\n");
      return(-1);
    }
  }

#if UE_TIMING_TRACE

  if(symbol > ue->frame_parms.symbols_per_tti>>1) {
    slot = 1;
  }

#endif
#ifdef DEBUG_HARQ
  printf("Demod  dlsch0_harq->pmi_alloc %d\n",  dlsch0_harq->pmi_alloc);
#endif

  if (frame_parms->nb_antenna_ports_eNB>1 && beamforming_mode==0) {
#ifdef DEBUG_DLSCH_MOD
    LOG_D(PHY,"dlsch: using pmi %x (%p), rb_alloc %x\n",pmi2hex_2Ar1(dlsch0_harq->pmi_alloc),dlsch[0],dlsch0_harq->rb_alloc_even[0]);
#endif
#if UE_TIMING_TRACE
    start_meas(&ue->generic_stat_bis[ue->current_thread_id[subframe]][slot]);
#endif
    nb_rb = dlsch_extract_rbs_dual(common_vars->common_vars_rx_data_per_thread[ue->current_thread_id[subframe]].rxdataF,
                                   common_vars->common_vars_rx_data_per_thread[ue->current_thread_id[subframe]].dl_ch_estimates[eNB_id],
                                   pdsch_vars[eNB_id]->rxdataF_ext,
                                   pdsch_vars[eNB_id]->dl_ch_estimates_ext,
                                   dlsch0_harq->pmi_alloc,
                                   pdsch_vars[eNB_id]->pmi_ext,
                                   rballoc,
                                   symbol,
                                   subframe,
                                   ue->high_speed_flag,
                                   frame_parms,
                                   dlsch0_harq->mimo_mode);
#ifdef DEBUG_DLSCH_MOD
    printf("dlsch: using pmi %lx, rb_alloc %x, pmi_ext ",pmi2hex_2Ar1(dlsch0_harq->pmi_alloc),*rballoc);

    for (rb=0; rb<nb_rb; rb++)
      printf("%d",pdsch_vars[eNB_id]->pmi_ext[rb]);

    printf("\n");
#endif

    if (rx_type >= rx_IC_single_stream) {
      if (eNB_id_i<ue->n_connected_eNB) // we are in TM5
        nb_rb = dlsch_extract_rbs_dual(common_vars->common_vars_rx_data_per_thread[ue->current_thread_id[subframe]].rxdataF,
                                       common_vars->common_vars_rx_data_per_thread[ue->current_thread_id[subframe]].dl_ch_estimates[eNB_id_i],
                                       pdsch_vars[eNB_id_i]->rxdataF_ext,
                                       pdsch_vars[eNB_id_i]->dl_ch_estimates_ext,
                                       dlsch0_harq->pmi_alloc,
                                       pdsch_vars[eNB_id_i]->pmi_ext,
                                       rballoc,
                                       symbol,
                                       subframe,
                                       ue->high_speed_flag,
                                       frame_parms,
                                       dlsch0_harq->mimo_mode);
      else
        nb_rb = dlsch_extract_rbs_dual(common_vars->common_vars_rx_data_per_thread[ue->current_thread_id[subframe]].rxdataF,
                                       common_vars->common_vars_rx_data_per_thread[ue->current_thread_id[subframe]].dl_ch_estimates[eNB_id],
                                       pdsch_vars[eNB_id_i]->rxdataF_ext,
                                       pdsch_vars[eNB_id_i]->dl_ch_estimates_ext,
                                       dlsch0_harq->pmi_alloc,
                                       pdsch_vars[eNB_id_i]->pmi_ext,
                                       rballoc,
                                       symbol,
                                       subframe,
                                       ue->high_speed_flag,
                                       frame_parms,
                                       dlsch0_harq->mimo_mode);
    }
  } else if (beamforming_mode==0) { //else if nb_antennas_ports_eNB==1 && beamforming_mode == 0
    nb_rb = dlsch_extract_rbs_single(common_vars->common_vars_rx_data_per_thread[ue->current_thread_id[subframe]].rxdataF,
                                     common_vars->common_vars_rx_data_per_thread[ue->current_thread_id[subframe]].dl_ch_estimates[eNB_id],
                                     pdsch_vars[eNB_id]->rxdataF_ext,
                                     pdsch_vars[eNB_id]->dl_ch_estimates_ext,
                                     dlsch0_harq->pmi_alloc,
                                     pdsch_vars[eNB_id]->pmi_ext,
                                     rballoc,
                                     symbol,
                                     subframe,
                                     ue->high_speed_flag,
                                     frame_parms);

    if (rx_type==rx_IC_single_stream) {
      if (eNB_id_i<ue->n_connected_eNB)
        nb_rb = dlsch_extract_rbs_single(common_vars->common_vars_rx_data_per_thread[ue->current_thread_id[subframe]].rxdataF,
                                         common_vars->common_vars_rx_data_per_thread[ue->current_thread_id[subframe]].dl_ch_estimates[eNB_id_i],
                                         pdsch_vars[eNB_id_i]->rxdataF_ext,
                                         pdsch_vars[eNB_id_i]->dl_ch_estimates_ext,
                                         dlsch0_harq->pmi_alloc,
                                         pdsch_vars[eNB_id_i]->pmi_ext,
                                         rballoc,
                                         symbol,
                                         subframe,
                                         ue->high_speed_flag,
                                         frame_parms);
      else
        nb_rb = dlsch_extract_rbs_single(common_vars->common_vars_rx_data_per_thread[ue->current_thread_id[subframe]].rxdataF,
                                         common_vars->common_vars_rx_data_per_thread[ue->current_thread_id[subframe]].dl_ch_estimates[eNB_id],
                                         pdsch_vars[eNB_id_i]->rxdataF_ext,
                                         pdsch_vars[eNB_id_i]->dl_ch_estimates_ext,
                                         dlsch0_harq->pmi_alloc,
                                         pdsch_vars[eNB_id_i]->pmi_ext,
                                         rballoc,
                                         symbol,
                                         subframe,
                                         ue->high_speed_flag,
                                         frame_parms);
    }
  } else if (beamforming_mode==7) { //else if beamforming_mode == 7
    nb_rb = dlsch_extract_rbs_TM7(common_vars->common_vars_rx_data_per_thread[ue->current_thread_id[subframe]].rxdataF,
                                  pdsch_vars[eNB_id]->dl_bf_ch_estimates,
                                  pdsch_vars[eNB_id]->rxdataF_ext,
                                  pdsch_vars[eNB_id]->dl_bf_ch_estimates_ext,
                                  rballoc,
                                  symbol,
                                  subframe,
                                  ue->high_speed_flag,
                                  frame_parms);
  } else if(beamforming_mode>7) {
    LOG_W(PHY,"dlsch_demodulation: beamforming mode not supported yet.\n");
  }

  //printf("nb_rb = %d, eNB_id %d\n",nb_rb,eNB_id);
  if (nb_rb==0) {
    //    LOG_D(PHY,"dlsch_demodulation.c: nb_rb=0\n");
    return(-1);
  }

#if UE_TIMING_TRACE
  stop_meas(&ue->generic_stat_bis[ue->current_thread_id[subframe]][slot]);
  LOG_D(PHY,
        "[AbsSFN %d.%d] Slot%d Symbol %d Flag %d type %d: Pilot/Data extraction  %5.2f \n",
        frame,
        subframe,
        slot,
        symbol,
        ue->high_speed_flag,
        type,
        ue->generic_stat_bis[ue->current_thread_id[subframe]][slot].p_time / (get_cpu_freq_GHz() * 1000.0));
#endif
#if UE_TIMING_TRACE
  start_meas(&ue->generic_stat_bis[ue->current_thread_id[subframe]][slot]);
#endif
  aatx = frame_parms->nb_antenna_ports_eNB;
  aarx = frame_parms->nb_antennas_rx;
  dlsch_scale_channel(pdsch_vars[eNB_id]->dl_ch_estimates_ext,
                      frame_parms,
                      dlsch,
                      symbol,
                      nb_rb);

  if ((dlsch0_harq->mimo_mode<DUALSTREAM_UNIFORM_PRECODING1) &&
      (rx_type==rx_IC_single_stream) &&
      (eNB_id_i==ue->n_connected_eNB) &&
      (dlsch0_harq->dl_power_off==0)
     ) { // TM5 two-user
    dlsch_scale_channel(pdsch_vars[eNB_id_i]->dl_ch_estimates_ext,
                        frame_parms,
                        dlsch,
                        symbol,
                        nb_rb);
  }

#if UE_TIMING_TRACE
  stop_meas(&ue->generic_stat_bis[ue->current_thread_id[subframe]][slot]);
  LOG_D(PHY,
        "[AbsSFN %d.%d] Slot%d Symbol %d: Channel Scale  %5.2f \n",
        frame,
        subframe,
        slot,
        symbol,
        ue->generic_stat_bis[ue->current_thread_id[subframe]][slot].p_time / (get_cpu_freq_GHz() * 1000.0));
  start_meas(&ue->generic_stat_bis[ue->current_thread_id[subframe]][slot]);
#endif

  if (first_symbol_flag==1) {
    if (beamforming_mode==0) {
      if (dlsch0_harq->mimo_mode<LARGE_CDD) {
        dlsch_channel_level(pdsch_vars[eNB_id]->dl_ch_estimates_ext,
                            frame_parms,
                            avg,
                            symbol,
                            nb_rb);
        avgs = 0;

        for (aatx=0; aatx<frame_parms->nb_antenna_ports_eNB; aatx++)
          for (aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++)
            avgs = cmax(avgs,avg[(aatx<<1)+aarx]);

        pdsch_vars[eNB_id]->log2_maxh = (log2_approx(avgs)/2)+1;
      } else if ((dlsch0_harq->mimo_mode == LARGE_CDD) ||
                 ((dlsch0_harq->mimo_mode >=DUALSTREAM_UNIFORM_PRECODING1) &&
                  (dlsch0_harq->mimo_mode <=DUALSTREAM_PUSCH_PRECODING))) {
        dlsch_channel_level_TM34(pdsch_vars[eNB_id]->dl_ch_estimates_ext,
                                 frame_parms,
                                 pdsch_vars[eNB_id]->pmi_ext,
                                 avg_0,
                                 avg_1,
                                 symbol,
                                 nb_rb,
                                 mmse_flag,
                                 dlsch0_harq->mimo_mode);
        LOG_D(PHY,"Channel Level TM34  avg_0 %d, avg_1 %d, rx_type %d, rx_standard %d, dlsch_demod_shift %d \n", avg_0[0],
              avg_1[0], rx_type, rx_standard, dlsch_demod_shift);

        if (rx_type>rx_standard) {
          avg_0[0] = (log2_approx(avg_0[0])/2) + dlsch_demod_shift;// + 2 ;//+ 4;
          avg_1[0] = (log2_approx(avg_1[0])/2) + dlsch_demod_shift;// + 2 ;//+ 4;
          pdsch_vars[eNB_id]->log2_maxh0 = cmax(avg_0[0],0);
          pdsch_vars[eNB_id]->log2_maxh1 = cmax(avg_1[0],0);
          // printf("dlsch_demod_shift  %d\n", dlsch_demod_shift);
        } else {
          avg_0[0] = (log2_approx(avg_0[0])/2) - 13 + interf_unaw_shift;
          avg_1[0] = (log2_approx(avg_1[0])/2) - 13 + interf_unaw_shift;
          pdsch_vars[eNB_id]->log2_maxh0 = cmax(avg_0[0],0);
          pdsch_vars[eNB_id]->log2_maxh1 = cmax(avg_1[0],0);
        }
      } else if (dlsch0_harq->mimo_mode<DUALSTREAM_UNIFORM_PRECODING1) { // single-layer precoding (TM5, TM6)
        if ((rx_type==rx_IC_single_stream) && (eNB_id_i==ue->n_connected_eNB) && (dlsch0_harq->dl_power_off==0)) {
          dlsch_channel_level_TM56(pdsch_vars[eNB_id]->dl_ch_estimates_ext,
                                   frame_parms,
                                   pdsch_vars[eNB_id]->pmi_ext,
                                   avg,
                                   symbol,
                                   nb_rb);
          avg[0] = log2_approx(avg[0]) - 13 + offset_mumimo_llr_drange[dlsch0_harq->mcs][(i_mod>>1)-1];
          pdsch_vars[eNB_id]->log2_maxh = cmax(avg[0],0);
        } else if (dlsch0_harq->dl_power_off==1) { //TM6
          dlsch_channel_level(pdsch_vars[eNB_id]->dl_ch_estimates_ext,
                              frame_parms,
                              avg,
                              symbol,
                              nb_rb);
          avgs = 0;

          for (aatx=0; aatx<frame_parms->nb_antenna_ports_eNB; aatx++)
            for (aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++)
              avgs = cmax(avgs,avg[(aatx<<1)+aarx]);

          pdsch_vars[eNB_id]->log2_maxh = (log2_approx(avgs)/2) + 1;
          pdsch_vars[eNB_id]->log2_maxh++;
        }
      }
    } else if (beamforming_mode==7)
      dlsch_channel_level_TM7(pdsch_vars[eNB_id]->dl_bf_ch_estimates_ext,
                              frame_parms,
                              avg,
                              symbol,
                              nb_rb);

#ifdef UE_DEBUG_TRACE
    LOG_D(PHY,"[DLSCH] AbsSubframe %d.%d log2_maxh = %d [log2_maxh0 %d log2_maxh1 %d] (%d,%d)\n",
          frame%1024,subframe, pdsch_vars[eNB_id]->log2_maxh,
          pdsch_vars[eNB_id]->log2_maxh0,
          pdsch_vars[eNB_id]->log2_maxh1,
          avg[0],avgs);
    //LOG_D(PHY,"[DLSCH] mimo_mode = %d\n", dlsch0_harq->mimo_mode);
#endif
    //wait until pdcch is decoded
    //proc->channel_level = 1;
  }

  /*
  uint32_t wait = 0;
  while(proc->channel_level == 0)
  {
      usleep(1);
      wait++;
  }
  */
#if T_TRACER

  if (type == PDSCH) {
    T(T_UE_PHY_PDSCH_ENERGY, T_INT(eNB_id), T_INT(frame%1024), T_INT(subframe),
      T_INT(avg[0]), T_INT(avg[1]),     T_INT(avg[2]),   T_INT(avg[3]));
  }

#endif
#if UE_TIMING_TRACE
  stop_meas(&ue->generic_stat_bis[ue->current_thread_id[subframe]][slot]);
  LOG_D(PHY,
        "[AbsSFN %d.%d] Slot%d Symbol %d first_symbol_flag %d: Channel Level  %5.2f \n",
        frame,
        subframe,
        slot,
        symbol,
        first_symbol_flag,
        ue->generic_stat_bis[ue->current_thread_id[subframe]][slot].p_time / (get_cpu_freq_GHz() * 1000.0));
  start_meas(&ue->generic_stat_bis[ue->current_thread_id[subframe]][slot]);
#endif

  if (rx_type==rx_IC_dual_stream && mmse_flag==1) {
    precode_channel_est(pdsch_vars[eNB_id]->dl_ch_estimates_ext,
                        frame_parms,
                        pdsch_vars[eNB_id],
                        symbol,
                        nb_rb,
                        dlsch0_harq->mimo_mode);
    mmse_processing_oai(pdsch_vars[eNB_id],
                        frame_parms,
                        measurements,
                        first_symbol_flag,
                        dlsch0_harq->mimo_mode,
                        mmse_flag,
                        0.0,
                        symbol,
                        nb_rb);
  }

  // Now channel compensation
  if (dlsch0_harq->mimo_mode<LARGE_CDD) {
    dlsch_channel_compensation(pdsch_vars[eNB_id]->rxdataF_ext,
                               pdsch_vars[eNB_id]->dl_ch_estimates_ext,
                               pdsch_vars[eNB_id]->dl_ch_mag0,
                               pdsch_vars[eNB_id]->dl_ch_magb0,
                               pdsch_vars[eNB_id]->rxdataF_comp0,
                               (aatx>1) ? pdsch_vars[eNB_id]->rho : NULL,
                               frame_parms,
                               symbol,
                               first_symbol_flag,
                               dlsch0_harq->Qm,
                               nb_rb,
                               pdsch_vars[eNB_id]->log2_maxh,
                               measurements); // log2_maxh+I0_shift

//    if (symbol == 5) {
//      LOG_M("rxF_comp_d.m","rxF_c_d",&pdsch_vars[eNB_id]->rxdataF_comp0[0][symbol*frame_parms->N_RB_DL*12],frame_parms->N_RB_DL*12,1,1);
//    }
//
    if ((rx_type==rx_IC_single_stream) &&
        (eNB_id_i<ue->n_connected_eNB)) {
      dlsch_channel_compensation(pdsch_vars[eNB_id_i]->rxdataF_ext,
                                 pdsch_vars[eNB_id_i]->dl_ch_estimates_ext,
                                 pdsch_vars[eNB_id_i]->dl_ch_mag0,
                                 pdsch_vars[eNB_id_i]->dl_ch_magb0,
                                 pdsch_vars[eNB_id_i]->rxdataF_comp0,
                                 (aatx>1) ? pdsch_vars[eNB_id_i]->rho : NULL,
                                 frame_parms,
                                 symbol,
                                 first_symbol_flag,
                                 i_mod,
                                 nb_rb,
                                 pdsch_vars[eNB_id]->log2_maxh,
                                 measurements); // log2_maxh+I0_shift

      if (symbol == 5) {
        LOG_M("rxF_comp_d.m","rxF_c_d",&pdsch_vars[eNB_id]->rxdataF_comp0[0][symbol*frame_parms->N_RB_DL*12],frame_parms->N_RB_DL*12,1,1);
        LOG_M("rxF_comp_i.m","rxF_c_i",&pdsch_vars[eNB_id_i]->rxdataF_comp0[0][symbol*frame_parms->N_RB_DL*12],frame_parms->N_RB_DL*12,1,1);
      }

      dlsch_dual_stream_correlation(frame_parms,
                                    symbol,
                                    nb_rb,
                                    pdsch_vars[eNB_id]->dl_ch_estimates_ext,
                                    pdsch_vars[eNB_id_i]->dl_ch_estimates_ext,
                                    pdsch_vars[eNB_id]->dl_ch_rho_ext[harq_pid][round],
                                    pdsch_vars[eNB_id]->log2_maxh);
    }
  } else if ((dlsch0_harq->mimo_mode == LARGE_CDD) || ((dlsch0_harq->mimo_mode >=DUALSTREAM_UNIFORM_PRECODING1) &&
             (dlsch0_harq->mimo_mode <=DUALSTREAM_PUSCH_PRECODING))) {
    dlsch_channel_compensation_TM34(frame_parms,
                                    pdsch_vars[eNB_id],
                                    measurements,
                                    eNB_id,
                                    symbol,
                                    dlsch0_harq->Qm,
                                    dlsch1_harq->Qm,
                                    harq_pid,
                                    dlsch0_harq->round,
                                    dlsch0_harq->mimo_mode,
                                    nb_rb,
                                    mmse_flag,
                                    pdsch_vars[eNB_id]->log2_maxh0,
                                    pdsch_vars[eNB_id]->log2_maxh1);

    if (symbol == 5) {
      LOG_M("rxF_comp_d00.m","rxF_c_d00",&pdsch_vars[eNB_id]->rxdataF_comp0[0][symbol*frame_parms->N_RB_DL*12],frame_parms->N_RB_DL*12,1,1);// should be QAM
      LOG_M("rxF_comp_d01.m","rxF_c_d01",&pdsch_vars[eNB_id]->rxdataF_comp0[1][symbol*frame_parms->N_RB_DL*12],frame_parms->N_RB_DL*12,1,1);//should be almost 0
      LOG_M("rxF_comp_d10.m","rxF_c_d10",&pdsch_vars[eNB_id]->rxdataF_comp1[harq_pid][round][0][symbol*frame_parms->N_RB_DL*12],frame_parms->N_RB_DL*12,1,1);//should be almost 0
      LOG_M("rxF_comp_d11.m","rxF_c_d11",&pdsch_vars[eNB_id]->rxdataF_comp1[harq_pid][round][1][symbol*frame_parms->N_RB_DL*12],frame_parms->N_RB_DL*12,1,1);//should be QAM
    }

    // compute correlation between signal and interference channels (rho12 and rho21)
    dlsch_dual_stream_correlation(frame_parms, // this is doing h11'*h12 and h21'*h22
                                  symbol,
                                  nb_rb,
                                  pdsch_vars[eNB_id]->dl_ch_estimates_ext,
                                  &(pdsch_vars[eNB_id]->dl_ch_estimates_ext[2]),
                                  pdsch_vars[eNB_id]->dl_ch_rho2_ext,
                                  pdsch_vars[eNB_id]->log2_maxh0);
    //printf("rho stream1 =%d\n", &pdsch_vars[eNB_id]->dl_ch_rho_ext[harq_pid][round] );
    //to be optimized (just take complex conjugate)
    dlsch_dual_stream_correlation(frame_parms, // this is doing h12'*h11 and h22'*h21
                                  symbol,
                                  nb_rb,
                                  &(pdsch_vars[eNB_id]->dl_ch_estimates_ext[2]),
                                  pdsch_vars[eNB_id]->dl_ch_estimates_ext,
                                  pdsch_vars[eNB_id]->dl_ch_rho_ext[harq_pid][round],
                                  pdsch_vars[eNB_id]->log2_maxh1);

    //  printf("rho stream2 =%d\n",&pdsch_vars[eNB_id]->dl_ch_rho2_ext );
    //printf("TM3 log2_maxh : %d\n",pdsch_vars[eNB_id]->log2_maxh);
    if (symbol == 5) {
      LOG_M("rho0_0.m","rho0_0",&pdsch_vars[eNB_id]->dl_ch_rho_ext[harq_pid][round][0][symbol*frame_parms->N_RB_DL*12],frame_parms->N_RB_DL*12,1,1);// should be QAM
      LOG_M("rho2_0.m","rho2_0",&pdsch_vars[eNB_id]->dl_ch_rho2_ext[0][symbol*frame_parms->N_RB_DL*12],frame_parms->N_RB_DL*12,1,1);//should be almost 0
      LOG_M("rho0_1.m.m","rho0_1",&pdsch_vars[eNB_id]->dl_ch_rho_ext[harq_pid][round][1][symbol*frame_parms->N_RB_DL*12],frame_parms->N_RB_DL*12,1,1);//should be almost 0
      LOG_M("rho2_1.m","rho2_1",&pdsch_vars[eNB_id]->dl_ch_rho2_ext[1][symbol*frame_parms->N_RB_DL*12],frame_parms->N_RB_DL*12,1,1);//should be QAM
    }
  } else if (dlsch0_harq->mimo_mode<DUALSTREAM_UNIFORM_PRECODING1) {// single-layer precoding (TM5, TM6)
    if ((rx_type==rx_IC_single_stream) && (eNB_id_i==ue->n_connected_eNB) && (dlsch0_harq->dl_power_off==0)) {
      dlsch_channel_compensation_TM56(pdsch_vars[eNB_id]->rxdataF_ext,
                                      pdsch_vars[eNB_id]->dl_ch_estimates_ext,
                                      pdsch_vars[eNB_id]->dl_ch_mag0,
                                      pdsch_vars[eNB_id]->dl_ch_magb0,
                                      pdsch_vars[eNB_id]->rxdataF_comp0,
                                      pdsch_vars[eNB_id]->pmi_ext,
                                      frame_parms,
                                      measurements,
                                      eNB_id,
                                      symbol,
                                      dlsch0_harq->Qm,
                                      nb_rb,
                                      pdsch_vars[eNB_id]->log2_maxh,
                                      dlsch0_harq->dl_power_off);

      for (rb=0; rb<nb_rb; rb++) {
        switch(pdsch_vars[eNB_id]->pmi_ext[rb]) {
          case 0:
            pdsch_vars[eNB_id_i]->pmi_ext[rb]=1;
            break;

          case 1:
            pdsch_vars[eNB_id_i]->pmi_ext[rb]=0;
            break;

          case 2:
            pdsch_vars[eNB_id_i]->pmi_ext[rb]=3;
            break;

          case 3:
            pdsch_vars[eNB_id_i]->pmi_ext[rb]=2;
            break;
        }

        //  if (rb==0)
        //    printf("pmi %d, pmi_i %d\n",pdsch_vars[eNB_id]->pmi_ext[rb],pdsch_vars[eNB_id_i]->pmi_ext[rb]);
      }

      dlsch_channel_compensation_TM56(pdsch_vars[eNB_id_i]->rxdataF_ext,
                                      pdsch_vars[eNB_id_i]->dl_ch_estimates_ext,
                                      pdsch_vars[eNB_id_i]->dl_ch_mag0,
                                      pdsch_vars[eNB_id_i]->dl_ch_magb0,
                                      pdsch_vars[eNB_id_i]->rxdataF_comp0,
                                      pdsch_vars[eNB_id_i]->pmi_ext,
                                      frame_parms,
                                      measurements,
                                      eNB_id_i,
                                      symbol,
                                      i_mod,
                                      nb_rb,
                                      pdsch_vars[eNB_id]->log2_maxh,
                                      dlsch0_harq->dl_power_off);

      if (symbol==5) {
        LOG_M("rxF_comp_d.m","rxF_c_d",&pdsch_vars[eNB_id]->rxdataF_comp0[0][symbol*frame_parms->N_RB_DL*12],frame_parms->N_RB_DL*12,1,1);
        LOG_M("rxF_comp_i.m","rxF_c_i",&pdsch_vars[eNB_id_i]->rxdataF_comp0[0][symbol*frame_parms->N_RB_DL*12],frame_parms->N_RB_DL*12,1,1);
      }

      dlsch_dual_stream_correlation(frame_parms,
                                    symbol,
                                    nb_rb,
                                    pdsch_vars[eNB_id]->dl_ch_estimates_ext,
                                    pdsch_vars[eNB_id_i]->dl_ch_estimates_ext,
                                    pdsch_vars[eNB_id]->dl_ch_rho_ext[harq_pid][round],
                                    pdsch_vars[eNB_id]->log2_maxh);
    }  else if (dlsch0_harq->dl_power_off==1)  {
      dlsch_channel_compensation_TM56(pdsch_vars[eNB_id]->rxdataF_ext,
                                      pdsch_vars[eNB_id]->dl_ch_estimates_ext,
                                      pdsch_vars[eNB_id]->dl_ch_mag0,
                                      pdsch_vars[eNB_id]->dl_ch_magb0,
                                      pdsch_vars[eNB_id]->rxdataF_comp0,
                                      pdsch_vars[eNB_id]->pmi_ext,
                                      frame_parms,
                                      measurements,
                                      eNB_id,
                                      symbol,
                                      dlsch0_harq->Qm,
                                      nb_rb,
                                      pdsch_vars[eNB_id]->log2_maxh,
                                      1);
    }
  } else if (dlsch0_harq->mimo_mode==TM7) { //TM7
    dlsch_channel_compensation(pdsch_vars[eNB_id]->rxdataF_ext,
                               pdsch_vars[eNB_id]->dl_bf_ch_estimates_ext,
                               pdsch_vars[eNB_id]->dl_ch_mag0,
                               pdsch_vars[eNB_id]->dl_ch_magb0,
                               pdsch_vars[eNB_id]->rxdataF_comp0,
                               (aatx>1) ? pdsch_vars[eNB_id]->rho : NULL,
                               frame_parms,
                               symbol,
                               first_symbol_flag,
                               get_Qm(dlsch0_harq->mcs),
                               nb_rb,
                               //9,
                               pdsch_vars[eNB_id]->log2_maxh,
                               measurements); // log2_maxh+I0_shift
  }

#if UE_TIMING_TRACE
  stop_meas(&ue->generic_stat_bis[ue->current_thread_id[subframe]][slot]);
  LOG_D(PHY,
        "[AbsSFN %d.%d] Slot%d Symbol %d log2_maxh %d Channel Comp  %5.2f \n",
        frame,
        subframe,
        slot,
        symbol,
        pdsch_vars[eNB_id]->log2_maxh,
        ue->generic_stat_bis[ue->current_thread_id[subframe]][slot].p_time / (get_cpu_freq_GHz() * 1000.0));
  start_meas(&ue->generic_stat_bis[ue->current_thread_id[subframe]][slot]);
#endif

  if (frame_parms->nb_antennas_rx > 1) {
    if ((dlsch0_harq->mimo_mode == LARGE_CDD) ||
        ((dlsch0_harq->mimo_mode >=DUALSTREAM_UNIFORM_PRECODING1) &&
         (dlsch0_harq->mimo_mode <=DUALSTREAM_PUSCH_PRECODING))) { // TM3 or TM4
      if (frame_parms->nb_antenna_ports_eNB == 2) {
        dlsch_detection_mrc_TM34(frame_parms,
                                 pdsch_vars[eNB_id],
                                 harq_pid,
                                 dlsch0_harq->round,
                                 symbol,
                                 nb_rb,
                                 1);

        if (symbol == 5) {
          LOG_M("rho0_mrc.m","rho0_0",&pdsch_vars[eNB_id]->dl_ch_rho_ext[harq_pid][round][0][symbol*frame_parms->N_RB_DL*12],frame_parms->N_RB_DL*12,1,1);// should be QAM
          LOG_M("rho2_mrc.m","rho2_0",&pdsch_vars[eNB_id]->dl_ch_rho2_ext[0][symbol*frame_parms->N_RB_DL*12],frame_parms->N_RB_DL*12,1,1);//should be almost 0
        }
      }
    } else {
      dlsch_detection_mrc(frame_parms,
                          pdsch_vars[eNB_id]->rxdataF_comp0,
                          pdsch_vars[eNB_id_i]->rxdataF_comp0,
                          pdsch_vars[eNB_id]->rho,
                          pdsch_vars[eNB_id]->dl_ch_rho_ext[harq_pid][round],
                          pdsch_vars[eNB_id]->dl_ch_mag0,
                          pdsch_vars[eNB_id]->dl_ch_magb0,
                          pdsch_vars[eNB_id_i]->dl_ch_mag0,
                          pdsch_vars[eNB_id_i]->dl_ch_magb0,
                          symbol,
                          nb_rb,
                          rx_type==rx_IC_single_stream);
    }
  }

  //  printf("Combining");
  if ((dlsch0_harq->mimo_mode == SISO) ||
      ((dlsch0_harq->mimo_mode >= UNIFORM_PRECODING11) &&
       (dlsch0_harq->mimo_mode <= PUSCH_PRECODING0)) ||
      (dlsch0_harq->mimo_mode == TM7)) {
    /*
      dlsch_siso(frame_parms,
      pdsch_vars[eNB_id]->rxdataF_comp,
      pdsch_vars[eNB_id_i]->rxdataF_comp,
      symbol,
      nb_rb);
    */
  } else if (dlsch0_harq->mimo_mode == ALAMOUTI) {
    dlsch_alamouti(frame_parms,
                   pdsch_vars[eNB_id]->rxdataF_comp0,
                   pdsch_vars[eNB_id]->dl_ch_mag0,
                   pdsch_vars[eNB_id]->dl_ch_magb0,
                   symbol,
                   nb_rb);
  }

  //    printf("LLR");
  if ((dlsch0_harq->mimo_mode == LARGE_CDD) ||
      ((dlsch0_harq->mimo_mode >=DUALSTREAM_UNIFORM_PRECODING1) &&
       (dlsch0_harq->mimo_mode <=DUALSTREAM_PUSCH_PRECODING)))  {
    rxdataF_comp_ptr = pdsch_vars[eNB_id]->rxdataF_comp1[harq_pid][round];
    dl_ch_mag_ptr = pdsch_vars[eNB_id]->dl_ch_mag1[harq_pid][round];
  } else {
    rxdataF_comp_ptr = pdsch_vars[eNB_id_i]->rxdataF_comp0;
    dl_ch_mag_ptr = pdsch_vars[eNB_id_i]->dl_ch_mag0;
    //i_mod should have been passed as a parameter
  }

#if UE_TIMING_TRACE
  stop_meas(&ue->generic_stat_bis[ue->current_thread_id[subframe]][slot]);
  LOG_D(PHY,
        "[AbsSFN %d.%d] Slot%d Symbol %d: Channel Combine  %5.2f \n",
        frame,
        subframe,
        slot,
        symbol,
        ue->generic_stat_bis[ue->current_thread_id[subframe]][slot].p_time / (get_cpu_freq_GHz() * 1000.0));
  start_meas(&ue->generic_stat_bis[ue->current_thread_id[subframe]][slot]);
#endif
  //printf("LLR dlsch0_harq->Qm %d rx_type %d cw0 %d cw1 %d symbol %d \n",dlsch0_harq->Qm,rx_type,codeword_TB0,codeword_TB1,symbol);
  // compute LLRs
  // -> // compute @pointer where llrs should filled for this ofdm-symbol
  int8_t  *pllr_symbol_cw0;
  int8_t  *pllr_symbol_cw1;
  uint32_t llr_offset_symbol;
  llr_offset_symbol = pdsch_vars[eNB_id]->llr_offset[symbol];
  pllr_symbol_cw0  = (int8_t *)pdsch_vars[eNB_id]->llr[0];
  pllr_symbol_cw1  = (int8_t *)pdsch_vars[eNB_id]->llr[1];
  pllr_symbol_cw0 += llr_offset_symbol;
  pllr_symbol_cw1 += llr_offset_symbol;

  LOG_D(PHY,"compute LLRs [AbsSubframe %d.%d-%d] NbRB %d Qm %d LLRs-Length %d LLR-Offset %d @LLR Buff %p @LLR Buff(symb) %p energy: %d\n",
             frame, subframe,symbol,
             nb_rb,dlsch0_harq->Qm,
             pdsch_vars[eNB_id]->llr_length[symbol],
             pdsch_vars[eNB_id]->llr_offset[symbol],
             (int16_t*)pdsch_vars[eNB_id]->llr[0],
             pllr_symbol_cw0,
	     signal_energy(pdsch_vars[eNB_id]->rxdataF_comp0[0], 7*2*frame_parms->N_RB_DL*12));
  switch (dlsch0_harq->Qm) {
    case 2 :
      if ((rx_type==rx_standard) || (codeword_TB1 == -1)) {
        dlsch_qpsk_llr(frame_parms,
                       pdsch_vars[eNB_id]->rxdataF_comp0,
                       (int16_t *)pllr_symbol_cw0,
                       symbol,
                       first_symbol_flag,
                       nb_rb,
                       adjust_G2(frame_parms->Ncp,frame_parms->frame_type, frame_parms->N_RB_DL,dlsch0_harq->rb_alloc_even,2,subframe,symbol),
                       beamforming_mode);
      } else if (codeword_TB0 == -1) {
        dlsch_qpsk_llr(frame_parms,
                       pdsch_vars[eNB_id]->rxdataF_comp0,
                       (int16_t *)pllr_symbol_cw1,
                       symbol,
                       first_symbol_flag,
                       nb_rb,
                       adjust_G2(frame_parms->Ncp,frame_parms->frame_type, frame_parms->N_RB_DL,dlsch0_harq->rb_alloc_even,2,subframe,symbol),
                       beamforming_mode);
      } else if (rx_type >= rx_IC_single_stream) {
        if (dlsch1_harq->Qm == 2) {
          dlsch_qpsk_qpsk_llr(frame_parms,
                              pdsch_vars[eNB_id]->rxdataF_comp0,
                              rxdataF_comp_ptr,
                              pdsch_vars[eNB_id]->dl_ch_rho2_ext,
                              pdsch_vars[eNB_id]->llr[0],
                              symbol,first_symbol_flag,nb_rb,
                              adjust_G2(frame_parms->Ncp,frame_parms->frame_type, frame_parms->N_RB_DL,dlsch0_harq->rb_alloc_even,2,subframe,symbol),
                              pdsch_vars[eNB_id]->llr128);

          if (rx_type==rx_IC_dual_stream) {
            dlsch_qpsk_qpsk_llr(frame_parms,
                                rxdataF_comp_ptr,
                                pdsch_vars[eNB_id]->rxdataF_comp0,
                                pdsch_vars[eNB_id]->dl_ch_rho_ext[harq_pid][round],
                                pdsch_vars[eNB_id]->llr[1],
                                symbol,first_symbol_flag,nb_rb,
                                adjust_G2(frame_parms->Ncp,frame_parms->frame_type, frame_parms->N_RB_DL,dlsch1_harq->rb_alloc_even,2,subframe,symbol),
                                pdsch_vars[eNB_id]->llr128_2ndstream);
          }
        } else if (dlsch1_harq->Qm == 4) {
          dlsch_qpsk_16qam_llr(frame_parms,
                               pdsch_vars[eNB_id]->rxdataF_comp0,
                               rxdataF_comp_ptr,//i
                               dl_ch_mag_ptr,//i
                               pdsch_vars[eNB_id]->dl_ch_rho2_ext,
                               pdsch_vars[eNB_id]->llr[0],
                               symbol,first_symbol_flag,nb_rb,
                               adjust_G2(frame_parms->Ncp,frame_parms->frame_type, frame_parms->N_RB_DL,dlsch0_harq->rb_alloc_even,2,subframe,symbol),
                               pdsch_vars[eNB_id]->llr128);

          if (rx_type==rx_IC_dual_stream) {
            dlsch_16qam_qpsk_llr(frame_parms,
                                 rxdataF_comp_ptr,
                                 pdsch_vars[eNB_id]->rxdataF_comp0,//i
                                 dl_ch_mag_ptr,
                                 pdsch_vars[eNB_id]->dl_ch_rho_ext[harq_pid][round],
                                 pdsch_vars[eNB_id]->llr[1],
                                 symbol,first_symbol_flag,nb_rb,
                                 adjust_G2(frame_parms->Ncp,frame_parms->frame_type, frame_parms->N_RB_DL,dlsch1_harq->rb_alloc_even,4,subframe,symbol),
                                 pdsch_vars[eNB_id]->llr128_2ndstream);
          }
        } else {
          dlsch_qpsk_64qam_llr(frame_parms,
                               pdsch_vars[eNB_id]->rxdataF_comp0,
                               rxdataF_comp_ptr,//i
                               dl_ch_mag_ptr,//i
                               pdsch_vars[eNB_id]->dl_ch_rho2_ext,
                               pdsch_vars[eNB_id]->llr[0],
                               symbol,first_symbol_flag,nb_rb,
                               adjust_G2(frame_parms->Ncp,frame_parms->frame_type, frame_parms->N_RB_DL,dlsch0_harq->rb_alloc_even,2,subframe,symbol),
                               pdsch_vars[eNB_id]->llr128);

          if (rx_type==rx_IC_dual_stream) {
            dlsch_64qam_qpsk_llr(frame_parms,
                                 rxdataF_comp_ptr,
                                 pdsch_vars[eNB_id]->rxdataF_comp0,//i
                                 dl_ch_mag_ptr,
                                 pdsch_vars[eNB_id]->dl_ch_rho_ext[harq_pid][round],
                                 pdsch_vars[eNB_id]->llr[1],
                                 symbol,first_symbol_flag,nb_rb,
                                 adjust_G2(frame_parms->Ncp,frame_parms->frame_type, frame_parms->N_RB_DL,dlsch1_harq->rb_alloc_even,6,subframe,symbol),
                                 pdsch_vars[eNB_id]->llr128_2ndstream);
          }
        }
      }

      break;

    case 4 :
      if ((rx_type==rx_standard ) || (codeword_TB1 == -1)) {
        dlsch_16qam_llr(frame_parms,
                        pdsch_vars[eNB_id]->rxdataF_comp0,
                        pdsch_vars[eNB_id]->llr[0],
                        pdsch_vars[eNB_id]->dl_ch_mag0,
                        symbol,first_symbol_flag,nb_rb,
                        adjust_G2(frame_parms->Ncp,frame_parms->frame_type, frame_parms->N_RB_DL,dlsch0_harq->rb_alloc_even,4,subframe,symbol),
                        pdsch_vars[eNB_id]->llr128,
                        beamforming_mode);
      } else if (codeword_TB0 == -1) {
        dlsch_16qam_llr(frame_parms,
                        pdsch_vars[eNB_id]->rxdataF_comp0,
                        pdsch_vars[eNB_id]->llr[1],
                        pdsch_vars[eNB_id]->dl_ch_mag0,
                        symbol,first_symbol_flag,nb_rb,
                        adjust_G2(frame_parms->Ncp,frame_parms->frame_type, frame_parms->N_RB_DL,dlsch0_harq->rb_alloc_even,4,subframe,symbol),
                        pdsch_vars[eNB_id]->llr128_2ndstream,
                        beamforming_mode);
      } else if (rx_type >= rx_IC_single_stream) {
        if (dlsch1_harq->Qm == 2) {
          dlsch_16qam_qpsk_llr(frame_parms,
                               pdsch_vars[eNB_id]->rxdataF_comp0,
                               rxdataF_comp_ptr,//i
                               pdsch_vars[eNB_id]->dl_ch_mag0,
                               pdsch_vars[eNB_id]->dl_ch_rho2_ext,
                               pdsch_vars[eNB_id]->llr[0],
                               symbol,first_symbol_flag,nb_rb,
                               adjust_G2(frame_parms->Ncp,frame_parms->frame_type, frame_parms->N_RB_DL,dlsch0_harq->rb_alloc_even,4,subframe,symbol),
                               pdsch_vars[eNB_id]->llr128);

          if (rx_type==rx_IC_dual_stream) {
            dlsch_qpsk_16qam_llr(frame_parms,
                                 rxdataF_comp_ptr,
                                 pdsch_vars[eNB_id]->rxdataF_comp0,//i
                                 pdsch_vars[eNB_id]->dl_ch_mag0,//i
                                 pdsch_vars[eNB_id]->dl_ch_rho_ext[harq_pid][round],
                                 pdsch_vars[eNB_id]->llr[1],
                                 symbol,first_symbol_flag,nb_rb,
                                 adjust_G2(frame_parms->Ncp,frame_parms->frame_type, frame_parms->N_RB_DL,dlsch1_harq->rb_alloc_even,2,subframe,symbol),
                                 pdsch_vars[eNB_id]->llr128_2ndstream);
          }
        } else if (dlsch1_harq->Qm == 4) {
          dlsch_16qam_16qam_llr(frame_parms,
                                pdsch_vars[eNB_id]->rxdataF_comp0,
                                rxdataF_comp_ptr,//i
                                pdsch_vars[eNB_id]->dl_ch_mag0,
                                dl_ch_mag_ptr,//i
                                pdsch_vars[eNB_id]->dl_ch_rho2_ext,
                                pdsch_vars[eNB_id]->llr[0],
                                symbol,first_symbol_flag,nb_rb,
                                adjust_G2(frame_parms->Ncp,frame_parms->frame_type, frame_parms->N_RB_DL,dlsch0_harq->rb_alloc_even,4,subframe,symbol),
                                pdsch_vars[eNB_id]->llr128);

          if (rx_type==rx_IC_dual_stream) {
            dlsch_16qam_16qam_llr(frame_parms,
                                  rxdataF_comp_ptr,
                                  pdsch_vars[eNB_id]->rxdataF_comp0,//i
                                  dl_ch_mag_ptr,
                                  pdsch_vars[eNB_id]->dl_ch_mag0,//i
                                  pdsch_vars[eNB_id]->dl_ch_rho_ext[harq_pid][round],
                                  pdsch_vars[eNB_id]->llr[1],
                                  symbol,first_symbol_flag,nb_rb,
                                  adjust_G2(frame_parms->Ncp,frame_parms->frame_type, frame_parms->N_RB_DL,dlsch1_harq->rb_alloc_even,4,subframe,symbol),
                                  pdsch_vars[eNB_id]->llr128_2ndstream);
          }
        } else {
          dlsch_16qam_64qam_llr(frame_parms,
                                pdsch_vars[eNB_id]->rxdataF_comp0,
                                rxdataF_comp_ptr,//i
                                pdsch_vars[eNB_id]->dl_ch_mag0,
                                dl_ch_mag_ptr,//i
                                pdsch_vars[eNB_id]->dl_ch_rho2_ext,
                                pdsch_vars[eNB_id]->llr[0],
                                symbol,first_symbol_flag,nb_rb,
                                adjust_G2(frame_parms->Ncp,frame_parms->frame_type, frame_parms->N_RB_DL,dlsch0_harq->rb_alloc_even,4,subframe,symbol),
                                pdsch_vars[eNB_id]->llr128);

          if (rx_type==rx_IC_dual_stream) {
            dlsch_64qam_16qam_llr(frame_parms,
                                  rxdataF_comp_ptr,
                                  pdsch_vars[eNB_id]->rxdataF_comp0,
                                  dl_ch_mag_ptr,
                                  pdsch_vars[eNB_id]->dl_ch_mag0,
                                  pdsch_vars[eNB_id]->dl_ch_rho_ext[harq_pid][round],
                                  pdsch_vars[eNB_id]->llr[1],
                                  symbol,first_symbol_flag,nb_rb,
                                  adjust_G2(frame_parms->Ncp,frame_parms->frame_type, frame_parms->N_RB_DL,dlsch1_harq->rb_alloc_even,6,subframe,symbol),
                                  pdsch_vars[eNB_id]->llr128_2ndstream);
          }
        }
      }

      break;

    case 6 :
      if ((rx_type==rx_standard) || (codeword_TB1 == -1))  {
        dlsch_64qam_llr(frame_parms,
                        pdsch_vars[eNB_id]->rxdataF_comp0,
                        (int16_t *)pllr_symbol_cw0,
                        pdsch_vars[eNB_id]->dl_ch_mag0,
                        pdsch_vars[eNB_id]->dl_ch_magb0,
                        symbol,first_symbol_flag,nb_rb,
                        adjust_G2(frame_parms->Ncp,frame_parms->frame_type, frame_parms->N_RB_DL,dlsch0_harq->rb_alloc_even,6,subframe,symbol),
                        pdsch_vars[eNB_id]->llr_offset[symbol],
                        beamforming_mode);
      } else if (codeword_TB0 == -1) {
        dlsch_64qam_llr(frame_parms,
                        pdsch_vars[eNB_id]->rxdataF_comp0,
                        (int16_t *)pllr_symbol_cw1,
                        pdsch_vars[eNB_id]->dl_ch_mag0,
                        pdsch_vars[eNB_id]->dl_ch_magb0,
                        symbol,first_symbol_flag,nb_rb,
                        adjust_G2(frame_parms->Ncp,frame_parms->frame_type, frame_parms->N_RB_DL,dlsch0_harq->rb_alloc_even,6,subframe,symbol),
                        pdsch_vars[eNB_id]->llr_offset[symbol],
                        beamforming_mode);
      } else if (rx_type >= rx_IC_single_stream) {
        if (dlsch1_harq->Qm == 2) {
          dlsch_64qam_qpsk_llr(frame_parms,
                               pdsch_vars[eNB_id]->rxdataF_comp0,
                               rxdataF_comp_ptr,//i
                               pdsch_vars[eNB_id]->dl_ch_mag0,
                               pdsch_vars[eNB_id]->dl_ch_rho2_ext,
                               pdsch_vars[eNB_id]->llr[0],
                               symbol,first_symbol_flag,nb_rb,
                               adjust_G2(frame_parms->Ncp,frame_parms->frame_type, frame_parms->N_RB_DL,dlsch0_harq->rb_alloc_even,6,subframe,symbol),
                               pdsch_vars[eNB_id]->llr128);

          if (rx_type==rx_IC_dual_stream) {
            dlsch_qpsk_64qam_llr(frame_parms,
                                 rxdataF_comp_ptr,
                                 pdsch_vars[eNB_id]->rxdataF_comp0,//i
                                 pdsch_vars[eNB_id]->dl_ch_mag0,
                                 pdsch_vars[eNB_id]->dl_ch_rho_ext[harq_pid][round],
                                 pdsch_vars[eNB_id]->llr[1],
                                 symbol,first_symbol_flag,nb_rb,
                                 adjust_G2(frame_parms->Ncp,frame_parms->frame_type, frame_parms->N_RB_DL,dlsch1_harq->rb_alloc_even,2,subframe,symbol),
                                 pdsch_vars[eNB_id]->llr128_2ndstream);
          }
        } else if (dlsch1_harq->Qm == 4) {
          dlsch_64qam_16qam_llr(frame_parms,
                                pdsch_vars[eNB_id]->rxdataF_comp0,
                                rxdataF_comp_ptr,//i
                                pdsch_vars[eNB_id]->dl_ch_mag0,
                                dl_ch_mag_ptr,//i
                                pdsch_vars[eNB_id]->dl_ch_rho2_ext,
                                pdsch_vars[eNB_id]->llr[0],
                                symbol,first_symbol_flag,nb_rb,
                                adjust_G2(frame_parms->Ncp,frame_parms->frame_type, frame_parms->N_RB_DL,dlsch0_harq->rb_alloc_even,6,subframe,symbol),
                                pdsch_vars[eNB_id]->llr128);

          if (rx_type==rx_IC_dual_stream) {
            dlsch_16qam_64qam_llr(frame_parms,
                                  rxdataF_comp_ptr,
                                  pdsch_vars[eNB_id]->rxdataF_comp0,//i
                                  dl_ch_mag_ptr,
                                  pdsch_vars[eNB_id]->dl_ch_mag0,//i
                                  pdsch_vars[eNB_id]->dl_ch_rho_ext[harq_pid][round],
                                  pdsch_vars[eNB_id]->llr[1],
                                  symbol,first_symbol_flag,nb_rb,
                                  adjust_G2(frame_parms->Ncp,frame_parms->frame_type, frame_parms->N_RB_DL,dlsch1_harq->rb_alloc_even,4,subframe,symbol),
                                  pdsch_vars[eNB_id]->llr128_2ndstream);
          }
        } else {
          dlsch_64qam_64qam_llr(frame_parms,
                                pdsch_vars[eNB_id]->rxdataF_comp0,
                                rxdataF_comp_ptr,//i
                                pdsch_vars[eNB_id]->dl_ch_mag0,
                                dl_ch_mag_ptr,//i
                                pdsch_vars[eNB_id]->dl_ch_rho2_ext,
                                (int16_t *)pllr_symbol_cw0,
                                symbol,first_symbol_flag,nb_rb,
                                adjust_G2(frame_parms->Ncp,frame_parms->frame_type, frame_parms->N_RB_DL,dlsch0_harq->rb_alloc_even,6,subframe,symbol),
                                pdsch_vars[eNB_id]->llr_offset[symbol]);

          if (rx_type==rx_IC_dual_stream) {
            dlsch_64qam_64qam_llr(frame_parms,
                                  rxdataF_comp_ptr,
                                  pdsch_vars[eNB_id]->rxdataF_comp0,//i
                                  dl_ch_mag_ptr,
                                  pdsch_vars[eNB_id]->dl_ch_mag0,//i
                                  pdsch_vars[eNB_id]->dl_ch_rho_ext[harq_pid][round],
                                  (int16_t *)pllr_symbol_cw1,
                                  symbol,first_symbol_flag,nb_rb,
                                  adjust_G2(frame_parms->Ncp,frame_parms->frame_type, frame_parms->N_RB_DL,dlsch1_harq->rb_alloc_even,6,subframe,symbol),
                                  pdsch_vars[eNB_id]->llr_offset[symbol]);
          }
        }
      }

      break;

    default:
      LOG_W(PHY,"rx_dlsch.c : Unknown mod_order!!!!\n");
      return(-1);
      break;
  }

  if (dlsch1_harq) {
    switch (get_Qm(dlsch1_harq->mcs)) {
      case 2 :
        if (rx_type==rx_standard) {
          dlsch_qpsk_llr(frame_parms,
                         pdsch_vars[eNB_id]->rxdataF_comp0,
                         (int16_t *)pllr_symbol_cw0,
                         symbol,first_symbol_flag,nb_rb,
                         adjust_G2(frame_parms->Ncp,frame_parms->frame_type, frame_parms->N_RB_DL,dlsch0_harq->rb_alloc_even,2,subframe,symbol),
                         beamforming_mode);
        }

        break;

      case 4:
        if (rx_type==rx_standard) {
          dlsch_16qam_llr(frame_parms,
                          pdsch_vars[eNB_id]->rxdataF_comp0,
                          pdsch_vars[eNB_id]->llr[0],
                          pdsch_vars[eNB_id]->dl_ch_mag0,
                          symbol,first_symbol_flag,nb_rb,
                          adjust_G2(frame_parms->Ncp,frame_parms->frame_type, frame_parms->N_RB_DL,dlsch0_harq->rb_alloc_even,4,subframe,symbol),
                          pdsch_vars[eNB_id]->llr128,
                          beamforming_mode);
        }

        break;

      case 6 :
        if (rx_type==rx_standard) {
          dlsch_64qam_llr(frame_parms,
                          pdsch_vars[eNB_id]->rxdataF_comp0,
                          (int16_t *)pllr_symbol_cw0,
                          pdsch_vars[eNB_id]->dl_ch_mag0,
                          pdsch_vars[eNB_id]->dl_ch_magb0,
                          symbol,first_symbol_flag,nb_rb,
                          adjust_G2(frame_parms->Ncp,frame_parms->frame_type, frame_parms->N_RB_DL,dlsch0_harq->rb_alloc_even,6,subframe,symbol),
                          pdsch_vars[eNB_id]->llr_offset[symbol],
                          beamforming_mode);
        }

        break;

      default:
        LOG_W(PHY,"rx_dlsch.c : Unknown mod_order!!!!\n");
        return(-1);
        break;
    }
  }

#if UE_TIMING_TRACE
  stop_meas(&ue->generic_stat_bis[ue->current_thread_id[subframe]][slot]);
  LOG_D(PHY,
        "[AbsSFN %d.%d] Slot%d Symbol %d: LLR Computation  %5.2f \n",
        frame,
        subframe,
        slot,
        symbol,
        ue->generic_stat_bis[ue->current_thread_id[subframe]][slot].p_time / (get_cpu_freq_GHz() * 1000.0));
#endif
  // Please keep it: useful for debugging
#if 0

  if( (symbol == 13) && (subframe==0) && (dlsch0_harq->Qm == 6) /*&& (nb_rb==25)*/) {
    LOG_E(PHY,"Dump Phy Chan Est \n");

    if(1) {
#if 1
      LOG_M("rxdataF0.m", "rxdataF0",             &common_vars->common_vars_rx_data_per_thread[ue->current_thread_id[subframe]].rxdataF[0][0],14*frame_parms->ofdm_symbol_size,1,1);
      //LOG_M("rxdataF1.m"    , "rxdataF1",             &common_vars->common_vars_rx_data_per_thread[ue->current_thread_id[subframe]].rxdataF[0][0],14*frame_parms->ofdm_symbol_size,1,1);
      LOG_M("dl_ch_estimates00.m", "dl_ch_estimates00",   &common_vars->common_vars_rx_data_per_thread[ue->current_thread_id[subframe]].dl_ch_estimates[eNB_id][0][0],14*frame_parms->ofdm_symbol_size,1,1);
      //LOG_M("dl_ch_estimates01.m", "dl_ch_estimates01",   &common_vars->common_vars_rx_data_per_thread[ue->current_thread_id[subframe]].dl_ch_estimates[eNB_id][1][0],14*frame_parms->ofdm_symbol_size,1,1);
      //LOG_M("dl_ch_estimates10.m", "dl_ch_estimates10",   &common_vars->common_vars_rx_data_per_thread[ue->current_thread_id[subframe]].dl_ch_estimates[eNB_id][2][0],14*frame_parms->ofdm_symbol_size,1,1);
      //LOG_M("dl_ch_estimates11.m", "dl_ch_estimates11",   &common_vars->common_vars_rx_data_per_thread[ue->current_thread_id[subframe]].dl_ch_estimates[eNB_id][3][0],14*frame_parms->ofdm_symbol_size,1,1);
      //LOG_M("rxdataF_ext00.m"    , "rxdataF_ext00",       &pdsch_vars[eNB_id]->rxdataF_ext[0][0],14*frame_parms->N_RB_DL*12,1,1);
      //LOG_M("rxdataF_ext01.m"    , "rxdataF_ext01",       &pdsch_vars[eNB_id]->rxdataF_ext[1][0],14*frame_parms->N_RB_DL*12,1,1);
      //LOG_M("rxdataF_ext10.m"    , "rxdataF_ext10",       &pdsch_vars[eNB_id]->rxdataF_ext[2][0],14*frame_parms->N_RB_DL*12,1,1);
      //LOG_M("rxdataF_ext11.m"    , "rxdataF_ext11",       &pdsch_vars[eNB_id]->rxdataF_ext[3][0],14*frame_parms->N_RB_DL*12,1,1);
      LOG_M("dl_ch_estimates_ext00.m", "dl_ch_estimates_ext00", &pdsch_vars[eNB_id]->dl_ch_estimates_ext[0][0],14*frame_parms->N_RB_DL*12,1,1);
      //LOG_M("dl_ch_estimates_ext01.m", "dl_ch_estimates_ext01", &pdsch_vars[eNB_id]->dl_ch_estimates_ext[1][0],14*frame_parms->N_RB_DL*12,1,1);
      //LOG_M("dl_ch_estimates_ext10.m", "dl_ch_estimates_ext10", &pdsch_vars[eNB_id]->dl_ch_estimates_ext[2][0],14*frame_parms->N_RB_DL*12,1,1);
      //LOG_M("dl_ch_estimates_ext11.m", "dl_ch_estimates_ext11", &pdsch_vars[eNB_id]->dl_ch_estimates_ext[3][0],14*frame_parms->N_RB_DL*12,1,1);
      LOG_M("rxdataF_comp00.m","rxdataF_comp00",              &pdsch_vars[eNB_id]->rxdataF_comp0[0][0],14*frame_parms->N_RB_DL*12,1,1);
      //LOG_M("rxdataF_comp01.m","rxdataF_comp01",              &pdsch_vars[eNB_id]->rxdataF_comp0[1][0],14*frame_parms->N_RB_DL*12,1,1);
      //LOG_M("rxdataF_comp10.m","rxdataF_comp10",              &pdsch_vars[eNB_id]->rxdataF_comp1[harq_pid][round][0][0],14*frame_parms->N_RB_DL*12,1,1);
      //LOG_M("rxdataF_comp11.m","rxdataF_comp11",              &pdsch_vars[eNB_id]->rxdataF_comp1[harq_pid][round][1][0],14*frame_parms->N_RB_DL*12,1,1);
#endif
      LOG_M("llr0.m","llr0",  &pdsch_vars[eNB_id]->llr[0][0],(14*nb_rb*12*dlsch1_harq->Qm) - 4*(nb_rb*4*dlsch1_harq->Qm),1,0);
      //LOG_M("llr1.m","llr1",  &pdsch_vars[eNB_id]->llr[1][0],(14*nb_rb*12*dlsch1_harq->Qm) - 4*(nb_rb*4*dlsch1_harq->Qm),1,0);
      AssertFatal(0," ");
    }
  }

#endif
  T(T_UE_PHY_PDSCH_IQ, T_INT(eNB_id), T_INT(frame%1024),
    T_INT(subframe), T_INT(nb_rb),
    T_INT(frame_parms->N_RB_UL), T_INT(frame_parms->symbols_per_tti),
    T_BUFFER(&pdsch_vars[eNB_id]->rxdataF_comp0[eNB_id][0],
             2 * /* ulsch[UE_id]->harq_processes[harq_pid]->nb_rb */ frame_parms->N_RB_UL *12*frame_parms->symbols_per_tti*2));
  return 0;
}

//==============================================================================================
// Pre-processing for LLR computation
//==============================================================================================

void dlsch_channel_compensation(int **rxdataF_ext,
                                int **dl_ch_estimates_ext,
                                int **dl_ch_mag,
                                int **dl_ch_magb,
                                int **rxdataF_comp,
                                int **rho,
                                LTE_DL_FRAME_PARMS *frame_parms,
                                unsigned char symbol,
                                uint8_t first_symbol_flag,
                                unsigned char mod_order,
                                unsigned short nb_rb,
                                unsigned char output_shift,
                                PHY_MEASUREMENTS *measurements) {
  unsigned short rb;
  unsigned char aatx,aarx,symbol_mod,pilots=0;
  simde__m128i *dl_ch128,*dl_ch128_2,*dl_ch_mag128,*dl_ch_mag128b,*rxdataF128,*rxdataF_comp128,*rho128;
  simde__m128i mmtmpD0,mmtmpD1,mmtmpD2,mmtmpD3,QAM_amp128={0};
  symbol_mod = (symbol>=(7-frame_parms->Ncp)) ? symbol-(7-frame_parms->Ncp) : symbol;

  if ((symbol_mod == 0) || (symbol_mod == (4-frame_parms->Ncp))) {
    if (frame_parms->nb_antenna_ports_eNB==1) // 10 out of 12 so don't reduce size
      nb_rb=1+(5*nb_rb/6);
    else
      pilots=1;
  }

  for (aatx=0; aatx<frame_parms->nb_antenna_ports_eNB; aatx++) {
    simde__m128i QAM_amp128b = simde_mm_setzero_si128();

    if (mod_order == 4) {
      QAM_amp128 = simde_mm_set1_epi16(QAM16_n1);  // 2/sqrt(10)
    } else if (mod_order == 6) {
      QAM_amp128  = simde_mm_set1_epi16(QAM64_n1); //
      QAM_amp128b = simde_mm_set1_epi16(QAM64_n2);
    }

    for (aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++) {
      /* TODO: hack to be removed. There is crash for 1 antenna case, so
       * for 1 antenna case, I put back the value 2 as it was before
       * Elena's commit.
       */
      int x = frame_parms->nb_antennas_rx > 1 ? frame_parms->nb_antennas_rx : 2;
      dl_ch128          = (simde__m128i *)&dl_ch_estimates_ext[aatx*x + aarx][symbol*frame_parms->N_RB_DL*12];
      //print_shorts("dl_ch128[0]=",&dl_ch128[0]);*/
      dl_ch_mag128      = (simde__m128i *)&dl_ch_mag[aatx*x + aarx][symbol*frame_parms->N_RB_DL*12];
      dl_ch_mag128b     = (simde__m128i *)&dl_ch_magb[aatx*x + aarx][symbol*frame_parms->N_RB_DL*12];
      rxdataF128        = (simde__m128i *)&rxdataF_ext[aarx][symbol*frame_parms->N_RB_DL*12];
      rxdataF_comp128   = (simde__m128i *)&rxdataF_comp[aatx*x + aarx][symbol*frame_parms->N_RB_DL*12];

      for (rb=0; rb<nb_rb; rb++) {
        if (mod_order>2) {
          // get channel amplitude if not QPSK
          mmtmpD0 = simde_mm_madd_epi16(dl_ch128[0],dl_ch128[0]);
          mmtmpD0 = simde_mm_srai_epi32(mmtmpD0,output_shift);
          mmtmpD1 = simde_mm_madd_epi16(dl_ch128[1],dl_ch128[1]);
          mmtmpD1 = simde_mm_srai_epi32(mmtmpD1,output_shift);
          mmtmpD0 = simde_mm_packs_epi32(mmtmpD0,mmtmpD1);
          // store channel magnitude here in a new field of dlsch
          dl_ch_mag128[0] = simde_mm_unpacklo_epi16(mmtmpD0,mmtmpD0);
          dl_ch_mag128b[0] = dl_ch_mag128[0];
          dl_ch_mag128[0] = simde_mm_mulhi_epi16(dl_ch_mag128[0],QAM_amp128);
          dl_ch_mag128[0] = simde_mm_slli_epi16(dl_ch_mag128[0],1);
          //print_ints("Re(ch):",(int16_t*)&mmtmpD0);
          //print_shorts("QAM_amp:",(int16_t*)&QAM_amp128);
          //print_shorts("mag:",(int16_t*)&dl_ch_mag128[0]);
          dl_ch_mag128[1] = simde_mm_unpackhi_epi16(mmtmpD0,mmtmpD0);
          dl_ch_mag128b[1] = dl_ch_mag128[1];
          dl_ch_mag128[1] = simde_mm_mulhi_epi16(dl_ch_mag128[1],QAM_amp128);
          dl_ch_mag128[1] = simde_mm_slli_epi16(dl_ch_mag128[1],1);

          if (pilots==0) {
            mmtmpD0 = simde_mm_madd_epi16(dl_ch128[2],dl_ch128[2]);
            mmtmpD0 = simde_mm_srai_epi32(mmtmpD0,output_shift);
            mmtmpD1 = simde_mm_packs_epi32(mmtmpD0,mmtmpD0);
            dl_ch_mag128[2] = simde_mm_unpacklo_epi16(mmtmpD1,mmtmpD1);
            dl_ch_mag128b[2] = dl_ch_mag128[2];
            dl_ch_mag128[2] = simde_mm_mulhi_epi16(dl_ch_mag128[2],QAM_amp128);
            dl_ch_mag128[2] = simde_mm_slli_epi16(dl_ch_mag128[2],1);
          }

          dl_ch_mag128b[0] = simde_mm_mulhi_epi16(dl_ch_mag128b[0],QAM_amp128b);
          dl_ch_mag128b[0] = simde_mm_slli_epi16(dl_ch_mag128b[0],1);
          dl_ch_mag128b[1] = simde_mm_mulhi_epi16(dl_ch_mag128b[1],QAM_amp128b);
          dl_ch_mag128b[1] = simde_mm_slli_epi16(dl_ch_mag128b[1],1);

          if (pilots==0) {
            dl_ch_mag128b[2] = simde_mm_mulhi_epi16(dl_ch_mag128b[2],QAM_amp128b);
            dl_ch_mag128b[2] = simde_mm_slli_epi16(dl_ch_mag128b[2],1);
          }
        }

        // multiply by conjugated channel
        mmtmpD0 = simde_mm_madd_epi16(dl_ch128[0],rxdataF128[0]);
        // mmtmpD0 contains real part of 4 consecutive outputs (32-bit)
        mmtmpD1 = simde_mm_shufflelo_epi16(dl_ch128[0], SIMDE_MM_SHUFFLE(2,3,0,1));
        mmtmpD1 = simde_mm_shufflehi_epi16(mmtmpD1, SIMDE_MM_SHUFFLE(2,3,0,1));
        mmtmpD1 = simde_mm_sign_epi16(mmtmpD1,*(simde__m128i *)&conjugate[0]);
        //  print_ints("im",&mmtmpD1);
        mmtmpD1 = simde_mm_madd_epi16(mmtmpD1,rxdataF128[0]);
        // mmtmpD1 contains imag part of 4 consecutive outputs (32-bit)
        mmtmpD0 = simde_mm_srai_epi32(mmtmpD0,output_shift);
        //  print_ints("re(shift)",&mmtmpD0);
        mmtmpD1 = simde_mm_srai_epi32(mmtmpD1,output_shift);
        //  print_ints("im(shift)",&mmtmpD1);
        mmtmpD2 = simde_mm_unpacklo_epi32(mmtmpD0,mmtmpD1);
        mmtmpD3 = simde_mm_unpackhi_epi32(mmtmpD0,mmtmpD1);
        //        print_ints("c0",&mmtmpD2);
        //  print_ints("c1",&mmtmpD3);
        rxdataF_comp128[0] = simde_mm_packs_epi32(mmtmpD2,mmtmpD3);
        //  print_shorts("rx:",rxdataF128);
        //  print_shorts("ch:",dl_ch128);
        //  print_shorts("pack:",rxdataF_comp128);
        // multiply by conjugated channel
        mmtmpD0 = simde_mm_madd_epi16(dl_ch128[1],rxdataF128[1]);
        // mmtmpD0 contains real part of 4 consecutive outputs (32-bit)
        mmtmpD1 = simde_mm_shufflelo_epi16(dl_ch128[1], SIMDE_MM_SHUFFLE(2,3,0,1));
        mmtmpD1 = simde_mm_shufflehi_epi16(mmtmpD1, SIMDE_MM_SHUFFLE(2,3,0,1));
        mmtmpD1 = simde_mm_sign_epi16(mmtmpD1,*(simde__m128i *)conjugate);
        mmtmpD1 = simde_mm_madd_epi16(mmtmpD1,rxdataF128[1]);
        // mmtmpD1 contains imag part of 4 consecutive outputs (32-bit)
        mmtmpD0 = simde_mm_srai_epi32(mmtmpD0,output_shift);
        mmtmpD1 = simde_mm_srai_epi32(mmtmpD1,output_shift);
        mmtmpD2 = simde_mm_unpacklo_epi32(mmtmpD0,mmtmpD1);
        mmtmpD3 = simde_mm_unpackhi_epi32(mmtmpD0,mmtmpD1);
        rxdataF_comp128[1] = simde_mm_packs_epi32(mmtmpD2,mmtmpD3);
        //  print_shorts("rx:",rxdataF128+1);
        //  print_shorts("ch:",dl_ch128+1);
        //  print_shorts("pack:",rxdataF_comp128+1);

        if (pilots==0) {
          // multiply by conjugated channel
          mmtmpD0 = simde_mm_madd_epi16(dl_ch128[2],rxdataF128[2]);
          // mmtmpD0 contains real part of 4 consecutive outputs (32-bit)
          mmtmpD1 = simde_mm_shufflelo_epi16(dl_ch128[2], SIMDE_MM_SHUFFLE(2,3,0,1));
          mmtmpD1 = simde_mm_shufflehi_epi16(mmtmpD1, SIMDE_MM_SHUFFLE(2,3,0,1));
          mmtmpD1 = simde_mm_sign_epi16(mmtmpD1,*(simde__m128i *)conjugate);
          mmtmpD1 = simde_mm_madd_epi16(mmtmpD1,rxdataF128[2]);
          // mmtmpD1 contains imag part of 4 consecutive outputs (32-bit)
          mmtmpD0 = simde_mm_srai_epi32(mmtmpD0,output_shift);
          mmtmpD1 = simde_mm_srai_epi32(mmtmpD1,output_shift);
          mmtmpD2 = simde_mm_unpacklo_epi32(mmtmpD0,mmtmpD1);
          mmtmpD3 = simde_mm_unpackhi_epi32(mmtmpD0,mmtmpD1);
          rxdataF_comp128[2] = simde_mm_packs_epi32(mmtmpD2,mmtmpD3);
          //  print_shorts("rx:",rxdataF128+2);
          //  print_shorts("ch:",dl_ch128+2);
          // print_shorts("pack:",rxdataF_comp128+2);
          dl_ch128+=3;
          dl_ch_mag128+=3;
          dl_ch_mag128b+=3;
          rxdataF128+=3;
          rxdataF_comp128+=3;
        } else { // we have a smaller PDSCH in symbols with pilots so skip last group of 4 REs and increment less
          dl_ch128+=2;
          dl_ch_mag128+=2;
          dl_ch_mag128b+=2;
          rxdataF128+=2;
          rxdataF_comp128+=2;
        }
      }
    }
  }

  if (rho) {
    for (aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++) {
      rho128        = (simde__m128i *)&rho[aarx][symbol*frame_parms->N_RB_DL*12];
      dl_ch128      = (simde__m128i *)&dl_ch_estimates_ext[aarx][symbol*frame_parms->N_RB_DL*12];
      dl_ch128_2    = (simde__m128i *)&dl_ch_estimates_ext[2+aarx][symbol*frame_parms->N_RB_DL*12];

      for (rb=0; rb<nb_rb; rb++) {
        // multiply by conjugated channel
        mmtmpD0 = simde_mm_madd_epi16(dl_ch128[0],dl_ch128_2[0]);
        //  print_ints("re",&mmtmpD0);
        // mmtmpD0 contains real part of 4 consecutive outputs (32-bit)
        mmtmpD1 = simde_mm_shufflelo_epi16(dl_ch128[0], SIMDE_MM_SHUFFLE(2,3,0,1));
        mmtmpD1 = simde_mm_shufflehi_epi16(mmtmpD1, SIMDE_MM_SHUFFLE(2,3,0,1));
        mmtmpD1 = simde_mm_sign_epi16(mmtmpD1,*(simde__m128i *)&conjugate[0]);
        //  print_ints("im",&mmtmpD1);
        mmtmpD1 = simde_mm_madd_epi16(mmtmpD1,dl_ch128_2[0]);
        // mmtmpD1 contains imag part of 4 consecutive outputs (32-bit)
        mmtmpD0 = simde_mm_srai_epi32(mmtmpD0,output_shift);
        //  print_ints("re(shift)",&mmtmpD0);
        mmtmpD1 = simde_mm_srai_epi32(mmtmpD1,output_shift);
        //  print_ints("im(shift)",&mmtmpD1);
        mmtmpD2 = simde_mm_unpacklo_epi32(mmtmpD0,mmtmpD1);
        mmtmpD3 = simde_mm_unpackhi_epi32(mmtmpD0,mmtmpD1);
        //        print_ints("c0",&mmtmpD2);
        //  print_ints("c1",&mmtmpD3);
        rho128[0] = simde_mm_packs_epi32(mmtmpD2,mmtmpD3);
        //print_shorts("rx:",dl_ch128_2);
        //print_shorts("ch:",dl_ch128);
        //print_shorts("pack:",rho128);
        // multiply by conjugated channel
        mmtmpD0 = simde_mm_madd_epi16(dl_ch128[1],dl_ch128_2[1]);
        // mmtmpD0 contains real part of 4 consecutive outputs (32-bit)
        mmtmpD1 = simde_mm_shufflelo_epi16(dl_ch128[1], SIMDE_MM_SHUFFLE(2,3,0,1));
        mmtmpD1 = simde_mm_shufflehi_epi16(mmtmpD1, SIMDE_MM_SHUFFLE(2,3,0,1));
        mmtmpD1 = simde_mm_sign_epi16(mmtmpD1,*(simde__m128i *)conjugate);
        mmtmpD1 = simde_mm_madd_epi16(mmtmpD1,dl_ch128_2[1]);
        // mmtmpD1 contains imag part of 4 consecutive outputs (32-bit)
        mmtmpD0 = simde_mm_srai_epi32(mmtmpD0,output_shift);
        mmtmpD1 = simde_mm_srai_epi32(mmtmpD1,output_shift);
        mmtmpD2 = simde_mm_unpacklo_epi32(mmtmpD0,mmtmpD1);
        mmtmpD3 = simde_mm_unpackhi_epi32(mmtmpD0,mmtmpD1);
        rho128[1] =simde_mm_packs_epi32(mmtmpD2,mmtmpD3);
        //print_shorts("rx:",dl_ch128_2+1);
        //print_shorts("ch:",dl_ch128+1);
        //print_shorts("pack:",rho128+1);
        // multiply by conjugated channel
        mmtmpD0 = simde_mm_madd_epi16(dl_ch128[2],dl_ch128_2[2]);
        // mmtmpD0 contains real part of 4 consecutive outputs (32-bit)
        mmtmpD1 = simde_mm_shufflelo_epi16(dl_ch128[2], SIMDE_MM_SHUFFLE(2,3,0,1));
        mmtmpD1 = simde_mm_shufflehi_epi16(mmtmpD1, SIMDE_MM_SHUFFLE(2,3,0,1));
        mmtmpD1 = simde_mm_sign_epi16(mmtmpD1,*(simde__m128i *)conjugate);
        mmtmpD1 = simde_mm_madd_epi16(mmtmpD1,dl_ch128_2[2]);
        // mmtmpD1 contains imag part of 4 consecutive outputs (32-bit)
        mmtmpD0 = simde_mm_srai_epi32(mmtmpD0,output_shift);
        mmtmpD1 = simde_mm_srai_epi32(mmtmpD1,output_shift);
        mmtmpD2 = simde_mm_unpacklo_epi32(mmtmpD0,mmtmpD1);
        mmtmpD3 = simde_mm_unpackhi_epi32(mmtmpD0,mmtmpD1);
        rho128[2] = simde_mm_packs_epi32(mmtmpD2,mmtmpD3);
        //print_shorts("rx:",dl_ch128_2+2);
        //print_shorts("ch:",dl_ch128+2);
        //print_shorts("pack:",rho128+2);
        dl_ch128+=3;
        dl_ch128_2+=3;
        rho128+=3;
      }

      if (first_symbol_flag==1) {
        measurements->rx_correlation[0][aarx] = signal_energy(&rho[aarx][symbol*frame_parms->N_RB_DL*12],rb*12);
      }
    }
  }

  simde_mm_empty();
  simde_m_empty();
}

void dlsch_channel_compensation_core(int **rxdataF_ext,
                                     int **dl_ch_estimates_ext,
                                     int **dl_ch_mag,
                                     int **dl_ch_magb,
                                     int **rxdataF_comp,
                                     int **rho,
                                     unsigned char n_tx,
                                     unsigned char n_rx,
                                     unsigned char mod_order,
                                     unsigned char output_shift,
                                     int length,
                                     int start_point)

{
  unsigned short ii;
  int length_mod8 = 0;
  int length2;
  simde__m128i *dl_ch128,*dl_ch_mag128,*dl_ch_mag128b, *dl_ch128_2, *rxdataF128,*rxdataF_comp128,*rho128;
  simde__m128i mmtmpD0,mmtmpD1,mmtmpD2,mmtmpD3,QAM_amp128={0};
  int aatx = 0, aarx = 0;

  for (aatx=0; aatx<n_tx; aatx++) {
    simde__m128i QAM_amp128b={0};

    if (mod_order == 4) {
      QAM_amp128 = simde_mm_set1_epi16(QAM16_n1);  // 2/sqrt(10)
      QAM_amp128b = simde_mm_setzero_si128();
    } else if (mod_order == 6) {
      QAM_amp128  = simde_mm_set1_epi16(QAM64_n1); //
      QAM_amp128b = simde_mm_set1_epi16(QAM64_n2);
    }

    for (aarx=0; aarx<n_rx; aarx++) {
      /* TODO: hack to be removed. There is crash for 1 antenna case, so
       * for 1 antenna case, I put back the value 2 as it was before
       * Elena's commit.
       */
      int x = n_rx > 1 ? n_rx : 2;
      dl_ch128          = (simde__m128i *)&dl_ch_estimates_ext[aatx*x + aarx][start_point];
      dl_ch_mag128      = (simde__m128i *)&dl_ch_mag[aatx*x + aarx][start_point];
      dl_ch_mag128b     = (simde__m128i *)&dl_ch_magb[aatx*x + aarx][start_point];
      rxdataF128        = (simde__m128i *)&rxdataF_ext[aarx][start_point];
      rxdataF_comp128   = (simde__m128i *)&rxdataF_comp[aatx*x + aarx][start_point];
      length_mod8 = length&7;

      if (length_mod8 == 0) {
        length2 = length>>3;

        for (ii=0; ii<length2; ++ii) {
          if (mod_order>2) {
            // get channel amplitude if not QPSK
            mmtmpD0 = simde_mm_madd_epi16(dl_ch128[0],dl_ch128[0]);
            mmtmpD0 = simde_mm_srai_epi32(mmtmpD0,output_shift);
            mmtmpD1 = simde_mm_madd_epi16(dl_ch128[1],dl_ch128[1]);
            mmtmpD1 = simde_mm_srai_epi32(mmtmpD1,output_shift);
            mmtmpD0 = simde_mm_packs_epi32(mmtmpD0,mmtmpD1);
            // store channel magnitude here in a new field of dlsch
            dl_ch_mag128[0] = simde_mm_unpacklo_epi16(mmtmpD0,mmtmpD0);
            dl_ch_mag128b[0] = dl_ch_mag128[0];
            dl_ch_mag128[0] = simde_mm_mulhi_epi16(dl_ch_mag128[0],QAM_amp128);
            dl_ch_mag128[0] = simde_mm_slli_epi16(dl_ch_mag128[0],1);
            //print_ints("Re(ch):",(int16_t*)&mmtmpD0);
            //print_shorts("QAM_amp:",(int16_t*)&QAM_amp128);
            //print_shorts("mag:",(int16_t*)&dl_ch_mag128[0]);
            dl_ch_mag128[1] = simde_mm_unpackhi_epi16(mmtmpD0,mmtmpD0);
            dl_ch_mag128b[1] = dl_ch_mag128[1];
            dl_ch_mag128[1] = simde_mm_mulhi_epi16(dl_ch_mag128[1],QAM_amp128);
            dl_ch_mag128[1] = simde_mm_slli_epi16(dl_ch_mag128[1],1);
            dl_ch_mag128b[0] = simde_mm_mulhi_epi16(dl_ch_mag128b[0],QAM_amp128b);
            dl_ch_mag128b[0] = simde_mm_slli_epi16(dl_ch_mag128b[0],1);
            dl_ch_mag128b[1] = simde_mm_mulhi_epi16(dl_ch_mag128b[1],QAM_amp128b);
            dl_ch_mag128b[1] = simde_mm_slli_epi16(dl_ch_mag128b[1],1);
          }

          // multiply by conjugated channel
          mmtmpD0 = simde_mm_madd_epi16(dl_ch128[0],rxdataF128[0]);
          // mmtmpD0 contains real part of 4 consecutive outputs (32-bit)
          mmtmpD1 = simde_mm_shufflelo_epi16(dl_ch128[0], SIMDE_MM_SHUFFLE(2,3,0,1));
          mmtmpD1 = simde_mm_shufflehi_epi16(mmtmpD1, SIMDE_MM_SHUFFLE(2,3,0,1));
          mmtmpD1 = simde_mm_sign_epi16(mmtmpD1,*(simde__m128i *)&conjugate[0]);
          //  print_ints("im",&mmtmpD1);
          mmtmpD1 = simde_mm_madd_epi16(mmtmpD1,rxdataF128[0]);
          // mmtmpD1 contains imag part of 4 consecutive outputs (32-bit)
          mmtmpD0 = simde_mm_srai_epi32(mmtmpD0,output_shift);
          //  print_ints("re(shift)",&mmtmpD0);
          mmtmpD1 = simde_mm_srai_epi32(mmtmpD1,output_shift);
          //  print_ints("im(shift)",&mmtmpD1);
          mmtmpD2 = simde_mm_unpacklo_epi32(mmtmpD0,mmtmpD1);
          mmtmpD3 = simde_mm_unpackhi_epi32(mmtmpD0,mmtmpD1);
          //        print_ints("c0",&mmtmpD2);
          //  print_ints("c1",&mmtmpD3);
          rxdataF_comp128[0] = simde_mm_packs_epi32(mmtmpD2,mmtmpD3);
          //  print_shorts("rx:",rxdataF128);
          //  print_shorts("ch:",dl_ch128);
          //  print_shorts("pack:",rxdataF_comp128);
          // multiply by conjugated channel
          mmtmpD0 = simde_mm_madd_epi16(dl_ch128[1],rxdataF128[1]);
          // mmtmpD0 contains real part of 4 consecutive outputs (32-bit)
          mmtmpD1 = simde_mm_shufflelo_epi16(dl_ch128[1], SIMDE_MM_SHUFFLE(2,3,0,1));
          mmtmpD1 = simde_mm_shufflehi_epi16(mmtmpD1, SIMDE_MM_SHUFFLE(2,3,0,1));
          mmtmpD1 = simde_mm_sign_epi16(mmtmpD1,*(simde__m128i *)conjugate);
          mmtmpD1 = simde_mm_madd_epi16(mmtmpD1,rxdataF128[1]);
          // mmtmpD1 contains imag part of 4 consecutive outputs (32-bit)
          mmtmpD0 = simde_mm_srai_epi32(mmtmpD0,output_shift);
          mmtmpD1 = simde_mm_srai_epi32(mmtmpD1,output_shift);
          mmtmpD2 = simde_mm_unpacklo_epi32(mmtmpD0,mmtmpD1);
          mmtmpD3 = simde_mm_unpackhi_epi32(mmtmpD0,mmtmpD1);
          rxdataF_comp128[1] = simde_mm_packs_epi32(mmtmpD2,mmtmpD3);
          //  print_shorts("rx:",rxdataF128+1);
          //  print_shorts("ch:",dl_ch128+1);
          //print_shorts("pack:",rxdataF_comp128+1);
          dl_ch128+=2;
          dl_ch_mag128+=2;
          dl_ch_mag128b+=2;
          rxdataF128+=2;
          rxdataF_comp128+=2;
        }
      } else {
        printf ("Channel Compensation: Received number of subcarriers is not multiple of 8, \n"
                "need to adapt the code!\n");
      }
    }
  }

  /*This part of code makes sense only for processing in 2x2 blocks*/
  if (rho) {
    for (aarx=0; aarx<n_rx; aarx++) {
      rho128        = (simde__m128i *)&rho[aarx][start_point];
      dl_ch128      = (simde__m128i *)&dl_ch_estimates_ext[aarx][start_point];
      dl_ch128_2    = (simde__m128i *)&dl_ch_estimates_ext[2+aarx][start_point];

      if (length_mod8 == 0) {
        length2 = length>>3;

        for (ii=0; ii<length2; ++ii) {
          // multiply by conjugated channel
          mmtmpD0 = simde_mm_madd_epi16(dl_ch128[0],dl_ch128_2[0]);
          //  print_ints("re",&mmtmpD0);
          // mmtmpD0 contains real part of 4 consecutive outputs (32-bit)
          mmtmpD1 = simde_mm_shufflelo_epi16(dl_ch128[0], SIMDE_MM_SHUFFLE(2,3,0,1));
          mmtmpD1 = simde_mm_shufflehi_epi16(mmtmpD1, SIMDE_MM_SHUFFLE(2,3,0,1));
          mmtmpD1 = simde_mm_sign_epi16(mmtmpD1,*(simde__m128i *)&conjugate[0]);
          //  print_ints("im",&mmtmpD1);
          mmtmpD1 = simde_mm_madd_epi16(mmtmpD1,dl_ch128_2[0]);
          // mmtmpD1 contains imag part of 4 consecutive outputs (32-bit)
          mmtmpD0 = simde_mm_srai_epi32(mmtmpD0,output_shift);
          //  print_ints("re(shift)",&mmtmpD0);
          mmtmpD1 = simde_mm_srai_epi32(mmtmpD1,output_shift);
          //  print_ints("im(shift)",&mmtmpD1);
          mmtmpD2 = simde_mm_unpacklo_epi32(mmtmpD0,mmtmpD1);
          mmtmpD3 = simde_mm_unpackhi_epi32(mmtmpD0,mmtmpD1);
          //        print_ints("c0",&mmtmpD2);
          //  print_ints("c1",&mmtmpD3);
          rho128[0] = simde_mm_packs_epi32(mmtmpD2,mmtmpD3);
          //print_shorts("rx:",dl_ch128_2);
          //print_shorts("ch:",dl_ch128);
          //print_shorts("pack:",rho128);
          // multiply by conjugated channel
          mmtmpD0 = simde_mm_madd_epi16(dl_ch128[1],dl_ch128_2[1]);
          // mmtmpD0 contains real part of 4 consecutive outputs (32-bit)
          mmtmpD1 = simde_mm_shufflelo_epi16(dl_ch128[1], SIMDE_MM_SHUFFLE(2,3,0,1));
          mmtmpD1 = simde_mm_shufflehi_epi16(mmtmpD1, SIMDE_MM_SHUFFLE(2,3,0,1));
          mmtmpD1 = simde_mm_sign_epi16(mmtmpD1,*(simde__m128i *)conjugate);
          mmtmpD1 = simde_mm_madd_epi16(mmtmpD1,dl_ch128_2[1]);
          // mmtmpD1 contains imag part of 4 consecutive outputs (32-bit)
          mmtmpD0 = simde_mm_srai_epi32(mmtmpD0,output_shift);
          mmtmpD1 = simde_mm_srai_epi32(mmtmpD1,output_shift);
          mmtmpD2 = simde_mm_unpacklo_epi32(mmtmpD0,mmtmpD1);
          mmtmpD3 = simde_mm_unpackhi_epi32(mmtmpD0,mmtmpD1);
          rho128[1] =simde_mm_packs_epi32(mmtmpD2,mmtmpD3);
          dl_ch128+=2;
          dl_ch128_2+=2;
          rho128+=2;
        }
      } else {
        printf ("Channel Compensation: Received number of subcarriers is not multiple of 8, \n"
                "need to adapt the code!\n");
      }
    }
  }

  simde_mm_empty();
  simde_m_empty();
}


void prec2A_TM56_128(unsigned char pmi,simde__m128i *ch0,simde__m128i *ch1) {
  simde__m128i amp;
  amp = simde_mm_set1_epi16(ONE_OVER_SQRT2_Q15);

  switch (pmi) {
    case 0 :   // +1 +1
      //    print_shorts("phase 0 :ch0",ch0);
      //    print_shorts("phase 0 :ch1",ch1);
      ch0[0] = simde_mm_adds_epi16(ch0[0],ch1[0]);
      break;

    case 1 :   // +1 -1
      //    print_shorts("phase 1 :ch0",ch0);
      //    print_shorts("phase 1 :ch1",ch1);
      ch0[0] = simde_mm_subs_epi16(ch0[0],ch1[0]);
      //    print_shorts("phase 1 :ch0-ch1",ch0);
      break;

    case 2 :   // +1 +j
      ch1[0] = simde_mm_sign_epi16(ch1[0],*(simde__m128i *)&conjugate[0]);
      ch1[0] = simde_mm_shufflelo_epi16(ch1[0], SIMDE_MM_SHUFFLE(2,3,0,1));
      ch1[0] = simde_mm_shufflehi_epi16(ch1[0], SIMDE_MM_SHUFFLE(2,3,0,1));
      ch0[0] = simde_mm_subs_epi16(ch0[0],ch1[0]);
      break;   // +1 -j

    case 3 :
      ch1[0] = simde_mm_sign_epi16(ch1[0],*(simde__m128i *)&conjugate[0]);
      ch1[0] = simde_mm_shufflelo_epi16(ch1[0], SIMDE_MM_SHUFFLE(2,3,0,1));
      ch1[0] = simde_mm_shufflehi_epi16(ch1[0], SIMDE_MM_SHUFFLE(2,3,0,1));
      ch0[0] = simde_mm_adds_epi16(ch0[0],ch1[0]);
      break;
  }

  ch0[0] = simde_mm_mulhi_epi16(ch0[0],amp);
  ch0[0] = simde_mm_slli_epi16(ch0[0],1);
  simde_mm_empty();
  simde_m_empty();
}
// precoding is stream 0 .5(1,1)  .5(1,-1) .5(1,1)  .5(1,-1)
//              stream 1 .5(1,-1) .5(1,1)  .5(1,-1) .5(1,1)
// store "precoded" channel for stream 0 in ch0, stream 1 in ch1

short TM3_prec[8]__attribute__((aligned(16))) = {1,1,-1,-1,1,1,-1,-1} ;

void prec2A_TM3_128(simde__m128i *ch0,simde__m128i *ch1) {
  simde__m128i amp = simde_mm_set1_epi16(ONE_OVER_SQRT2_Q15);
  simde__m128i tmp0,tmp1;
  //simde_mm_mulhi_epi16
  //  print_shorts("prec2A_TM3 ch0 (before):",ch0);
  //  print_shorts("prec2A_TM3 ch1 (before):",ch1);
  tmp0 = ch0[0];
  tmp1  = simde_mm_sign_epi16(ch1[0],((simde__m128i *)&TM3_prec)[0]);
  //  print_shorts("prec2A_TM3 ch1*s (mid):",(simde__m128i *)TM3_prec);
  ch0[0] = simde_mm_adds_epi16(ch0[0],tmp1);
  ch1[0] = simde_mm_subs_epi16(tmp0,tmp1);
  ch0[0] = simde_mm_mulhi_epi16(ch0[0],amp);
  ch0[0] = simde_mm_slli_epi16(ch0[0],1);
  ch1[0] = simde_mm_mulhi_epi16(ch1[0],amp);
  ch1[0] = simde_mm_slli_epi16(ch1[0],1);
  //  print_shorts("prec2A_TM3 ch0 (mid):",&tmp0);
  //  print_shorts("prec2A_TM3 ch1 (mid):",ch1);
  //ch0[0] = simde_mm_mulhi_epi16(ch0[0],amp);
  //ch0[0] = simde_mm_slli_epi16(ch0[0],1);
  //ch1[0] = simde_mm_mulhi_epi16(ch1[0],amp);
  //ch1[0] = simde_mm_slli_epi16(ch1[0],1);
  //ch0[0] = simde_mm_srai_epi16(ch0[0],1);
  //ch1[0] = simde_mm_srai_epi16(ch1[0],1);
  //  print_shorts("prec2A_TM3 ch0 (after):",ch0);
  //  print_shorts("prec2A_TM3 ch1 (after):",ch1);
  simde_mm_empty();
  simde_m_empty();
}

// pmi = 0 => stream 0 (1,1), stream 1 (1,-1)
// pmi = 1 => stream 0 (1,j), stream 2 (1,-j)

void prec2A_TM4_128(int pmi,simde__m128i *ch0,simde__m128i *ch1) {
  // sqrt(2) is already taken into account in computation sqrt_rho_a, sqrt_rho_b,
  //so divide by 2 is replaced by divide by sqrt(2).
  // printf ("demod pmi=%d\n", pmi);
  simde__m128i amp;
  amp = simde_mm_set1_epi16(ONE_OVER_SQRT2_Q15);
  simde__m128i tmp0,tmp1;

  // print_shorts("prec2A_TM4 ch0 (before):",ch0);
  // print_shorts("prec2A_TM4 ch1 (before):",ch1);

  if (pmi == 0) { //[1 1;1 -1]
    tmp0 = ch0[0];
    tmp1 = ch1[0];
    ch0[0] = simde_mm_adds_epi16(tmp0,tmp1);
    ch1[0] = simde_mm_subs_epi16(tmp0,tmp1);
  } else { //ch0+j*ch1 ch0-j*ch1
    tmp0 = ch0[0];
    tmp1   = simde_mm_sign_epi16(ch1[0],*(simde__m128i *)&conjugate[0]);
    tmp1   = simde_mm_shufflelo_epi16(tmp1, SIMDE_MM_SHUFFLE(2,3,0,1));
    tmp1   = simde_mm_shufflehi_epi16(tmp1, SIMDE_MM_SHUFFLE(2,3,0,1));
    ch0[0] = simde_mm_subs_epi16(tmp0,tmp1);
    ch1[0] = simde_mm_add_epi16(tmp0,tmp1);
  }

  //print_shorts("prec2A_TM4 ch0 (middle):",ch0);
  //print_shorts("prec2A_TM4 ch1 (middle):",ch1);
  ch0[0] = simde_mm_mulhi_epi16(ch0[0],amp);
  ch0[0] = simde_mm_slli_epi16(ch0[0],1);
  ch1[0] = simde_mm_mulhi_epi16(ch1[0],amp);
  ch1[0] = simde_mm_slli_epi16(ch1[0],1);
  // ch0[0] = simde_mm_srai_epi16(ch0[0],1); //divide by 2
  // ch1[0] = simde_mm_srai_epi16(ch1[0],1); //divide by 2
  //print_shorts("prec2A_TM4 ch0 (end):",ch0);
  //print_shorts("prec2A_TM4 ch1 (end):",ch1);
  simde_mm_empty();
  simde_m_empty();
  // print_shorts("prec2A_TM4 ch0 (end):",ch0);
  //print_shorts("prec2A_TM4 ch1 (end):",ch1);
}

void dlsch_channel_compensation_TM56(int **rxdataF_ext,
                                     int **dl_ch_estimates_ext,
                                     int **dl_ch_mag,
                                     int **dl_ch_magb,
                                     int **rxdataF_comp,
                                     unsigned char *pmi_ext,
                                     LTE_DL_FRAME_PARMS *frame_parms,
                                     PHY_MEASUREMENTS *measurements,
                                     int eNB_id,
                                     unsigned char symbol,
                                     unsigned char mod_order,
                                     unsigned short nb_rb,
                                     unsigned char output_shift,
                                     unsigned char dl_power_off) {
  unsigned short rb,Nre;
  simde__m128i *dl_ch0_128,*dl_ch1_128,*dl_ch_mag128,*dl_ch_mag128b,*rxdataF128,*rxdataF_comp128;
  unsigned char aarx=0,symbol_mod,pilots=0;
  int precoded_signal_strength=0;
  simde__m128i mmtmpD0,mmtmpD1,mmtmpD2,mmtmpD3,QAM_amp128={0};
  symbol_mod = (symbol>=(7-frame_parms->Ncp)) ? symbol-(7-frame_parms->Ncp) : symbol;

  if ((symbol_mod == 0) || (symbol_mod == (4-frame_parms->Ncp)))
    pilots=1;

  //printf("comp prec: symbol %d, pilots %d\n",symbol, pilots);
  simde__m128i QAM_amp128b = simde_mm_setzero_si128();

  if (mod_order == 4) {
    QAM_amp128 = simde_mm_set1_epi16(QAM16_n1);
  } else if (mod_order == 6) {
    QAM_amp128  = simde_mm_set1_epi16(QAM64_n1);
    QAM_amp128b = simde_mm_set1_epi16(QAM64_n2);
  }

  for (aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++) {
    dl_ch0_128          = (simde__m128i *)&dl_ch_estimates_ext[aarx][symbol*frame_parms->N_RB_DL*12];
    dl_ch1_128          = (simde__m128i *)&dl_ch_estimates_ext[2+aarx][symbol*frame_parms->N_RB_DL*12];
    dl_ch_mag128      = (simde__m128i *)&dl_ch_mag[aarx][symbol*frame_parms->N_RB_DL*12];
    dl_ch_mag128b     = (simde__m128i *)&dl_ch_magb[aarx][symbol*frame_parms->N_RB_DL*12];
    rxdataF128        = (simde__m128i *)&rxdataF_ext[aarx][symbol*frame_parms->N_RB_DL*12];
    rxdataF_comp128   = (simde__m128i *)&rxdataF_comp[aarx][symbol*frame_parms->N_RB_DL*12];

    for (rb=0; rb<nb_rb; rb++) {
      // combine TX channels using precoder from pmi
#ifdef DEBUG_DLSCH_DEMOD
      printf("mode 6 prec: rb %d, pmi->%d\n",rb,pmi_ext[rb]);
#endif
      prec2A_TM56_128(pmi_ext[rb],&dl_ch0_128[0],&dl_ch1_128[0]);
      prec2A_TM56_128(pmi_ext[rb],&dl_ch0_128[1],&dl_ch1_128[1]);

      if (pilots==0) {
        prec2A_TM56_128(pmi_ext[rb],&dl_ch0_128[2],&dl_ch1_128[2]);
      }

      if (mod_order>2) {
        // get channel amplitude if not QPSK
        mmtmpD0 = simde_mm_madd_epi16(dl_ch0_128[0],dl_ch0_128[0]);
        mmtmpD0 = simde_mm_srai_epi32(mmtmpD0,output_shift);
        mmtmpD1 = simde_mm_madd_epi16(dl_ch0_128[1],dl_ch0_128[1]);
        mmtmpD1 = simde_mm_srai_epi32(mmtmpD1,output_shift);
        mmtmpD0 = simde_mm_packs_epi32(mmtmpD0,mmtmpD1);
        dl_ch_mag128[0] = simde_mm_unpacklo_epi16(mmtmpD0,mmtmpD0);
        dl_ch_mag128b[0] = dl_ch_mag128[0];
        dl_ch_mag128[0] = simde_mm_mulhi_epi16(dl_ch_mag128[0],QAM_amp128);
        dl_ch_mag128[0] = simde_mm_slli_epi16(dl_ch_mag128[0],1);
        //print_shorts("dl_ch_mag128[0]=",&dl_ch_mag128[0]);
        //print_shorts("dl_ch_mag128[0]=",&dl_ch_mag128[0]);
        dl_ch_mag128[1] = simde_mm_unpackhi_epi16(mmtmpD0,mmtmpD0);
        dl_ch_mag128b[1] = dl_ch_mag128[1];
        dl_ch_mag128[1] = simde_mm_mulhi_epi16(dl_ch_mag128[1],QAM_amp128);
        dl_ch_mag128[1] = simde_mm_slli_epi16(dl_ch_mag128[1],1);

        if (pilots==0) {
          mmtmpD0 = simde_mm_madd_epi16(dl_ch0_128[2],dl_ch0_128[2]);
          mmtmpD0 = simde_mm_srai_epi32(mmtmpD0,output_shift);
          mmtmpD1 = simde_mm_packs_epi32(mmtmpD0,mmtmpD0);
          dl_ch_mag128[2] = simde_mm_unpacklo_epi16(mmtmpD1,mmtmpD1);
          dl_ch_mag128b[2] = dl_ch_mag128[2];
          dl_ch_mag128[2] = simde_mm_mulhi_epi16(dl_ch_mag128[2],QAM_amp128);
          dl_ch_mag128[2] = simde_mm_slli_epi16(dl_ch_mag128[2],1);
        }

        dl_ch_mag128b[0] = simde_mm_mulhi_epi16(dl_ch_mag128b[0],QAM_amp128b);
        dl_ch_mag128b[0] = simde_mm_slli_epi16(dl_ch_mag128b[0],1);
        //print_shorts("dl_ch_mag128b[0]=",&dl_ch_mag128b[0]);
        dl_ch_mag128b[1] = simde_mm_mulhi_epi16(dl_ch_mag128b[1],QAM_amp128b);
        dl_ch_mag128b[1] = simde_mm_slli_epi16(dl_ch_mag128b[1],1);

        if (pilots==0) {
          dl_ch_mag128b[2] = simde_mm_mulhi_epi16(dl_ch_mag128b[2],QAM_amp128b);
          dl_ch_mag128b[2] = simde_mm_slli_epi16(dl_ch_mag128b[2],1);
        }
      }

      // MF multiply by conjugated channel
      mmtmpD0 = simde_mm_madd_epi16(dl_ch0_128[0],rxdataF128[0]);
      //        print_ints("re",&mmtmpD0);
      // mmtmpD0 contains real part of 4 consecutive outputs (32-bit)
      mmtmpD1 = simde_mm_shufflelo_epi16(dl_ch0_128[0], SIMDE_MM_SHUFFLE(2,3,0,1));
      mmtmpD1 = simde_mm_shufflehi_epi16(mmtmpD1, SIMDE_MM_SHUFFLE(2,3,0,1));
      mmtmpD1 = simde_mm_sign_epi16(mmtmpD1,*(simde__m128i *)&conjugate[0]);
      //        print_ints("im",&mmtmpD1);
      mmtmpD1 = simde_mm_madd_epi16(mmtmpD1,rxdataF128[0]);
      // mmtmpD1 contains imag part of 4 consecutive outputs (32-bit)
      mmtmpD0 = simde_mm_srai_epi32(mmtmpD0,output_shift);
      //        print_ints("re(shift)",&mmtmpD0);
      mmtmpD1 = simde_mm_srai_epi32(mmtmpD1,output_shift);
      //        print_ints("im(shift)",&mmtmpD1);
      mmtmpD2 = simde_mm_unpacklo_epi32(mmtmpD0,mmtmpD1);
      mmtmpD3 = simde_mm_unpackhi_epi32(mmtmpD0,mmtmpD1);
      //        print_ints("c0",&mmtmpD2);
      //        print_ints("c1",&mmtmpD3);
      rxdataF_comp128[0] = simde_mm_packs_epi32(mmtmpD2,mmtmpD3);
      //        print_shorts("rx:",rxdataF128);
      //        print_shorts("ch:",dl_ch128);
      //        print_shorts("pack:",rxdataF_comp128);
      // multiply by conjugated channel
      mmtmpD0 = simde_mm_madd_epi16(dl_ch0_128[1],rxdataF128[1]);
      // mmtmpD0 contains real part of 4 consecutive outputs (32-bit)
      mmtmpD1 = simde_mm_shufflelo_epi16(dl_ch0_128[1], SIMDE_MM_SHUFFLE(2,3,0,1));
      mmtmpD1 = simde_mm_shufflehi_epi16(mmtmpD1, SIMDE_MM_SHUFFLE(2,3,0,1));
      mmtmpD1 = simde_mm_sign_epi16(mmtmpD1,*(simde__m128i *)conjugate);
      mmtmpD1 = simde_mm_madd_epi16(mmtmpD1,rxdataF128[1]);
      // mmtmpD1 contains imag part of 4 consecutive outputs (32-bit)
      mmtmpD0 = simde_mm_srai_epi32(mmtmpD0,output_shift);
      mmtmpD1 = simde_mm_srai_epi32(mmtmpD1,output_shift);
      mmtmpD2 = simde_mm_unpacklo_epi32(mmtmpD0,mmtmpD1);
      mmtmpD3 = simde_mm_unpackhi_epi32(mmtmpD0,mmtmpD1);
      rxdataF_comp128[1] = simde_mm_packs_epi32(mmtmpD2,mmtmpD3);
      //  print_shorts("rx:",rxdataF128+1);
      //  print_shorts("ch:",dl_ch128+1);
      //  print_shorts("pack:",rxdataF_comp128+1);

      if (pilots==0) {
        // multiply by conjugated channel
        mmtmpD0 = simde_mm_madd_epi16(dl_ch0_128[2],rxdataF128[2]);
        // mmtmpD0 contains real part of 4 consecutive outputs (32-bit)
        mmtmpD1 = simde_mm_shufflelo_epi16(dl_ch0_128[2], SIMDE_MM_SHUFFLE(2,3,0,1));
        mmtmpD1 = simde_mm_shufflehi_epi16(mmtmpD1, SIMDE_MM_SHUFFLE(2,3,0,1));
        mmtmpD1 = simde_mm_sign_epi16(mmtmpD1,*(simde__m128i *)conjugate);
        mmtmpD1 = simde_mm_madd_epi16(mmtmpD1,rxdataF128[2]);
        // mmtmpD1 contains imag part of 4 consecutive outputs (32-bit)
        mmtmpD0 = simde_mm_srai_epi32(mmtmpD0,output_shift);
        mmtmpD1 = simde_mm_srai_epi32(mmtmpD1,output_shift);
        mmtmpD2 = simde_mm_unpacklo_epi32(mmtmpD0,mmtmpD1);
        mmtmpD3 = simde_mm_unpackhi_epi32(mmtmpD0,mmtmpD1);
        rxdataF_comp128[2] = simde_mm_packs_epi32(mmtmpD2,mmtmpD3);
        //  print_shorts("rx:",rxdataF128+2);
        //  print_shorts("ch:",dl_ch128+2);
        //        print_shorts("pack:",rxdataF_comp128+2);
        dl_ch0_128+=3;
        dl_ch1_128+=3;
        dl_ch_mag128+=3;
        dl_ch_mag128b+=3;
        rxdataF128+=3;
        rxdataF_comp128+=3;
      } else {
        dl_ch0_128+=2;
        dl_ch1_128+=2;
        dl_ch_mag128+=2;
        dl_ch_mag128b+=2;
        rxdataF128+=2;
        rxdataF_comp128+=2;
      }
    }

    Nre = (pilots==0) ? 12 : 8;
    precoded_signal_strength += ((signal_energy_nodc(&dl_ch_estimates_ext[aarx][symbol*frame_parms->N_RB_DL*Nre],
                                  (nb_rb*Nre))) - (measurements->n0_power[aarx]));
  } // rx_antennas

  measurements->precoded_cqi_dB[eNB_id][0] = dB_fixed2(precoded_signal_strength,measurements->n0_power_tot);
  //printf("eNB_id %d, symbol %d: precoded CQI %d dB\n",eNB_id,symbol,
  //   measurements->precoded_cqi_dB[eNB_id][0]);

}

void precode_channel_est(int32_t **dl_ch_estimates_ext,
                         LTE_DL_FRAME_PARMS *frame_parms,
                         LTE_UE_PDSCH *pdsch_vars,
                         unsigned char symbol,
                         unsigned short nb_rb,
                         MIMO_mode_t mimo_mode) {
  unsigned short rb;
  simde__m128i *dl_ch0_128,*dl_ch1_128;
  unsigned char aarx=0,symbol_mod,pilots=0;
  unsigned char *pmi_ext = pdsch_vars->pmi_ext;
  symbol_mod = (symbol>=(7-frame_parms->Ncp)) ? symbol-(7-frame_parms->Ncp) : symbol;

  if ((symbol_mod == 0) || (symbol_mod == (4-frame_parms->Ncp)))
    pilots=1;

  for (aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++) {
    dl_ch0_128          = (simde__m128i *)&dl_ch_estimates_ext[aarx][symbol*frame_parms->N_RB_DL*12]; // this is h11
    dl_ch1_128          = (simde__m128i *)&dl_ch_estimates_ext[2+aarx][symbol*frame_parms->N_RB_DL*12]; // this is h12

    for (rb=0; rb<nb_rb; rb++) {
      if (mimo_mode==LARGE_CDD) {
        prec2A_TM3_128(&dl_ch0_128[0],&dl_ch1_128[0]);
        prec2A_TM3_128(&dl_ch0_128[1],&dl_ch1_128[1]);

        if (pilots==0) {
          prec2A_TM3_128(&dl_ch0_128[2],&dl_ch1_128[2]);
        }
      } else if (mimo_mode==DUALSTREAM_UNIFORM_PRECODING1) {
        prec2A_TM4_128(0,&dl_ch0_128[0],&dl_ch1_128[0]);
        prec2A_TM4_128(0,&dl_ch0_128[1],&dl_ch1_128[1]);

        if (pilots==0) {
          prec2A_TM4_128(0,&dl_ch0_128[2],&dl_ch1_128[2]);
        }
      } else if (mimo_mode==DUALSTREAM_UNIFORM_PRECODINGj) {
        prec2A_TM4_128(1,&dl_ch0_128[0],&dl_ch1_128[0]);
        prec2A_TM4_128(1,&dl_ch0_128[1],&dl_ch1_128[1]);

        if (pilots==0) {
          prec2A_TM4_128(1,&dl_ch0_128[2],&dl_ch1_128[2]);
        }
      } else if (mimo_mode==DUALSTREAM_PUSCH_PRECODING) {
        prec2A_TM4_128(pmi_ext[rb],&dl_ch0_128[0],&dl_ch1_128[0]);
        prec2A_TM4_128(pmi_ext[rb],&dl_ch0_128[1],&dl_ch1_128[1]);

        if (pilots==0) {
          prec2A_TM4_128(pmi_ext[rb],&dl_ch0_128[2],&dl_ch1_128[2]);
        }
      } else {
        LOG_E(PHY,"Unknown MIMO mode\n");
        return;
      }

      if (pilots==0) {
        dl_ch0_128+=3;
        dl_ch1_128+=3;
      } else {
        dl_ch0_128+=2;
        dl_ch1_128+=2;
      }
    }
  }
}

void dlsch_channel_compensation_TM34(LTE_DL_FRAME_PARMS *frame_parms,
                                     LTE_UE_PDSCH *pdsch_vars,
                                     PHY_MEASUREMENTS *measurements,
                                     int eNB_id,
                                     unsigned char symbol,
                                     unsigned char mod_order0,
                                     unsigned char mod_order1,
                                     int harq_pid,
                                     int round,
                                     MIMO_mode_t mimo_mode,
                                     unsigned short nb_rb,
                                     unsigned short mmse_flag,
                                     unsigned char output_shift0,
                                     unsigned char output_shift1) {
  unsigned short rb,Nre;
  simde__m128i *dl_ch0_128,*dl_ch1_128,*dl_ch_mag0_128,*dl_ch_mag1_128,*dl_ch_mag0_128b,*dl_ch_mag1_128b,*rxdataF128,*rxdataF_comp0_128,*rxdataF_comp1_128;
  unsigned char aarx=0,symbol_mod,pilots=0;
  int precoded_signal_strength0=0,precoded_signal_strength1=0;
  int rx_power_correction;
  int **rxdataF_ext           = pdsch_vars->rxdataF_ext;
  int **dl_ch_estimates_ext   = pdsch_vars->dl_ch_estimates_ext;
  int **dl_ch_mag0            = pdsch_vars->dl_ch_mag0;
  int **dl_ch_mag1            = pdsch_vars->dl_ch_mag1[harq_pid][round];
  int **dl_ch_magb0           = pdsch_vars->dl_ch_magb0;
  int **dl_ch_magb1           = pdsch_vars->dl_ch_magb1[harq_pid][round];
  int **rxdataF_comp0         = pdsch_vars->rxdataF_comp0;
  int **rxdataF_comp1         = pdsch_vars->rxdataF_comp1[harq_pid][round];
  unsigned char *pmi_ext      = pdsch_vars->pmi_ext;
  simde__m128i mmtmpD0,mmtmpD1,mmtmpD2,mmtmpD3,QAM_amp0_128={0},QAM_amp1_128={0};
  symbol_mod = (symbol>=(7-frame_parms->Ncp)) ? symbol-(7-frame_parms->Ncp) : symbol;

  if ((symbol_mod == 0) || (symbol_mod == (4-frame_parms->Ncp)))
    pilots=1;

  rx_power_correction = 1;
  // printf("comp prec: symbol %d, pilots %d\n",symbol, pilots);
  simde__m128i  QAM_amp0_128b = simde_mm_setzero_si128();

  if (mod_order0 == 4) {
    QAM_amp0_128  = simde_mm_set1_epi16(QAM16_n1);
  } else if (mod_order0 == 6) {
    QAM_amp0_128  = simde_mm_set1_epi16(QAM64_n1);
    QAM_amp0_128b = simde_mm_set1_epi16(QAM64_n2);
  }

  simde__m128i  QAM_amp1_128b = simde_mm_setzero_si128();

  if (mod_order1 == 4) {
    QAM_amp1_128  = simde_mm_set1_epi16(QAM16_n1);
  } else if (mod_order1 == 6) {
    QAM_amp1_128  = simde_mm_set1_epi16(QAM64_n1);
    QAM_amp1_128b = simde_mm_set1_epi16(QAM64_n2);
  }

  for (aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++) {
    dl_ch0_128          = (simde__m128i *)&dl_ch_estimates_ext[aarx][symbol*frame_parms->N_RB_DL*12]; // this is h11
    dl_ch1_128          = (simde__m128i *)&dl_ch_estimates_ext[2+aarx][symbol*frame_parms->N_RB_DL*12]; // this is h12
    dl_ch_mag0_128      = (simde__m128i *)&dl_ch_mag0[aarx][symbol*frame_parms->N_RB_DL*12]; //responsible for x1
    dl_ch_mag0_128b     = (simde__m128i *)&dl_ch_magb0[aarx][symbol*frame_parms->N_RB_DL*12];//responsible for x1
    dl_ch_mag1_128      = (simde__m128i *)&dl_ch_mag1[aarx][symbol*frame_parms->N_RB_DL*12];   //responsible for x2. always coming from tx2
    dl_ch_mag1_128b     = (simde__m128i *)&dl_ch_magb1[aarx][symbol*frame_parms->N_RB_DL*12];  //responsible for x2. always coming from tx2
    rxdataF128          = (simde__m128i *)&rxdataF_ext[aarx][symbol*frame_parms->N_RB_DL*12]; //received signal on antenna of interest h11*x1+h12*x2
    rxdataF_comp0_128   = (simde__m128i *)&rxdataF_comp0[aarx][symbol*frame_parms->N_RB_DL*12]; //result of multipl with MF x1 on antenna of interest
    rxdataF_comp1_128   = (simde__m128i *)&rxdataF_comp1[aarx][symbol*frame_parms->N_RB_DL*12]; //result of multipl with MF x2 on antenna of interest

    for (rb=0; rb<nb_rb; rb++) {
      if (mmse_flag == 0) {
        // combine TX channels using precoder from pmi
        if (mimo_mode==LARGE_CDD) {
          prec2A_TM3_128(&dl_ch0_128[0],&dl_ch1_128[0]);
          prec2A_TM3_128(&dl_ch0_128[1],&dl_ch1_128[1]);

          if (pilots==0) {
            prec2A_TM3_128(&dl_ch0_128[2],&dl_ch1_128[2]);
          }
        } else if (mimo_mode==DUALSTREAM_UNIFORM_PRECODING1) {
          prec2A_TM4_128(0,&dl_ch0_128[0],&dl_ch1_128[0]);
          prec2A_TM4_128(0,&dl_ch0_128[1],&dl_ch1_128[1]);

          if (pilots==0) {
            prec2A_TM4_128(0,&dl_ch0_128[2],&dl_ch1_128[2]);
          }
        } else if (mimo_mode==DUALSTREAM_UNIFORM_PRECODINGj) {
          prec2A_TM4_128(1,&dl_ch0_128[0],&dl_ch1_128[0]);
          prec2A_TM4_128(1,&dl_ch0_128[1],&dl_ch1_128[1]);

          if (pilots==0) {
            prec2A_TM4_128(1,&dl_ch0_128[2],&dl_ch1_128[2]);
          }
        } else if (mimo_mode==DUALSTREAM_PUSCH_PRECODING) {
          prec2A_TM4_128(pmi_ext[rb],&dl_ch0_128[0],&dl_ch1_128[0]);
          prec2A_TM4_128(pmi_ext[rb],&dl_ch0_128[1],&dl_ch1_128[1]);

          if (pilots==0) {
            prec2A_TM4_128(pmi_ext[rb],&dl_ch0_128[2],&dl_ch1_128[2]);
          }
        } else {
          LOG_E(PHY,"Unknown MIMO mode\n");
          return;
        }
      }

      if (mod_order0>2) {
        // get channel amplitude if not QPSK
        mmtmpD0 = simde_mm_madd_epi16(dl_ch0_128[0],dl_ch0_128[0]);
        mmtmpD0 = simde_mm_srai_epi32(mmtmpD0,output_shift0);
        mmtmpD1 = simde_mm_madd_epi16(dl_ch0_128[1],dl_ch0_128[1]);
        mmtmpD1 = simde_mm_srai_epi32(mmtmpD1,output_shift0);
        mmtmpD0 = simde_mm_packs_epi32(mmtmpD0,mmtmpD1);
        dl_ch_mag0_128[0] = simde_mm_unpacklo_epi16(mmtmpD0,mmtmpD0);
        dl_ch_mag0_128b[0] = dl_ch_mag0_128[0];
        dl_ch_mag0_128[0] = simde_mm_mulhi_epi16(dl_ch_mag0_128[0],QAM_amp0_128);
        dl_ch_mag0_128[0] = simde_mm_slli_epi16(dl_ch_mag0_128[0],1);
        //  print_shorts("dl_ch_mag0_128[0]=",&dl_ch_mag0_128[0]);
        dl_ch_mag0_128[1] = simde_mm_unpackhi_epi16(mmtmpD0,mmtmpD0);
        dl_ch_mag0_128b[1] = dl_ch_mag0_128[1];
        dl_ch_mag0_128[1] = simde_mm_mulhi_epi16(dl_ch_mag0_128[1],QAM_amp0_128);
        dl_ch_mag0_128[1] = simde_mm_slli_epi16(dl_ch_mag0_128[1],1);

        if (pilots==0) {
          mmtmpD0 = simde_mm_madd_epi16(dl_ch0_128[2],dl_ch0_128[2]);
          mmtmpD0 = simde_mm_srai_epi32(mmtmpD0,output_shift0);
          mmtmpD1 = simde_mm_packs_epi32(mmtmpD0,mmtmpD0);
          dl_ch_mag0_128[2] = simde_mm_unpacklo_epi16(mmtmpD1,mmtmpD1);
          dl_ch_mag0_128b[2] = dl_ch_mag0_128[2];
          dl_ch_mag0_128[2] = simde_mm_mulhi_epi16(dl_ch_mag0_128[2],QAM_amp0_128);
          dl_ch_mag0_128[2] = simde_mm_slli_epi16(dl_ch_mag0_128[2],1);
        }

        dl_ch_mag0_128b[0] = simde_mm_mulhi_epi16(dl_ch_mag0_128b[0],QAM_amp0_128b);
        dl_ch_mag0_128b[0] = simde_mm_slli_epi16(dl_ch_mag0_128b[0],1);
        // print_shorts("dl_ch_mag0_128b[0]=",&dl_ch_mag0_128b[0]);
        dl_ch_mag0_128b[1] = simde_mm_mulhi_epi16(dl_ch_mag0_128b[1],QAM_amp0_128b);
        dl_ch_mag0_128b[1] = simde_mm_slli_epi16(dl_ch_mag0_128b[1],1);

        if (pilots==0) {
          dl_ch_mag0_128b[2] = simde_mm_mulhi_epi16(dl_ch_mag0_128b[2],QAM_amp0_128b);
          dl_ch_mag0_128b[2] = simde_mm_slli_epi16(dl_ch_mag0_128b[2],1);
        }
      }

      if (mod_order1>2) {
        // get channel amplitude if not QPSK
        mmtmpD0 = simde_mm_madd_epi16(dl_ch1_128[0],dl_ch1_128[0]);
        mmtmpD0 = simde_mm_srai_epi32(mmtmpD0,output_shift1);
        mmtmpD1 = simde_mm_madd_epi16(dl_ch1_128[1],dl_ch1_128[1]);
        mmtmpD1 = simde_mm_srai_epi32(mmtmpD1,output_shift1);
        mmtmpD0 = simde_mm_packs_epi32(mmtmpD0,mmtmpD1);
        dl_ch_mag1_128[0] = simde_mm_unpacklo_epi16(mmtmpD0,mmtmpD0);
        dl_ch_mag1_128b[0] = dl_ch_mag1_128[0];
        dl_ch_mag1_128[0] = simde_mm_mulhi_epi16(dl_ch_mag1_128[0],QAM_amp1_128);
        dl_ch_mag1_128[0] = simde_mm_slli_epi16(dl_ch_mag1_128[0],1);
        // print_shorts("dl_ch_mag1_128[0]=",&dl_ch_mag1_128[0]);
        dl_ch_mag1_128[1] = simde_mm_unpackhi_epi16(mmtmpD0,mmtmpD0);
        dl_ch_mag1_128b[1] = dl_ch_mag1_128[1];
        dl_ch_mag1_128[1] = simde_mm_mulhi_epi16(dl_ch_mag1_128[1],QAM_amp1_128);
        dl_ch_mag1_128[1] = simde_mm_slli_epi16(dl_ch_mag1_128[1],1);

        if (pilots==0) {
          mmtmpD0 = simde_mm_madd_epi16(dl_ch1_128[2],dl_ch1_128[2]);
          mmtmpD0 = simde_mm_srai_epi32(mmtmpD0,output_shift1);
          mmtmpD1 = simde_mm_packs_epi32(mmtmpD0,mmtmpD0);
          dl_ch_mag1_128[2] = simde_mm_unpacklo_epi16(mmtmpD1,mmtmpD1);
          dl_ch_mag1_128b[2] = dl_ch_mag1_128[2];
          dl_ch_mag1_128[2] = simde_mm_mulhi_epi16(dl_ch_mag1_128[2],QAM_amp1_128);
          dl_ch_mag1_128[2] = simde_mm_slli_epi16(dl_ch_mag1_128[2],1);
        }

        dl_ch_mag1_128b[0] = simde_mm_mulhi_epi16(dl_ch_mag1_128b[0],QAM_amp1_128b);
        dl_ch_mag1_128b[0] = simde_mm_slli_epi16(dl_ch_mag1_128b[0],1);
        // print_shorts("dl_ch_mag1_128b[0]=",&dl_ch_mag1_128b[0]);
        dl_ch_mag1_128b[1] = simde_mm_mulhi_epi16(dl_ch_mag1_128b[1],QAM_amp1_128b);
        dl_ch_mag1_128b[1] = simde_mm_slli_epi16(dl_ch_mag1_128b[1],1);

        if (pilots==0) {
          dl_ch_mag1_128b[2] = simde_mm_mulhi_epi16(dl_ch_mag1_128b[2],QAM_amp1_128b);
          dl_ch_mag1_128b[2] = simde_mm_slli_epi16(dl_ch_mag1_128b[2],1);
        }
      }

      // layer 0
      // MF multiply by conjugated channel
      mmtmpD0 = simde_mm_madd_epi16(dl_ch0_128[0],rxdataF128[0]);
      //  print_ints("re",&mmtmpD0);
      // mmtmpD0 contains real part of 4 consecutive outputs (32-bit)
      mmtmpD1 = simde_mm_shufflelo_epi16(dl_ch0_128[0], SIMDE_MM_SHUFFLE(2,3,0,1));
      mmtmpD1 = simde_mm_shufflehi_epi16(mmtmpD1, SIMDE_MM_SHUFFLE(2,3,0,1));
      mmtmpD1 = simde_mm_sign_epi16(mmtmpD1,*(simde__m128i *)&conjugate[0]);
      mmtmpD1 = simde_mm_madd_epi16(mmtmpD1,rxdataF128[0]);
      // print_ints("im",&mmtmpD1);
      // mmtmpD1 contains imag part of 4 consecutive outputs (32-bit)
      mmtmpD0 = simde_mm_srai_epi32(mmtmpD0,output_shift0);
      // printf("Shift: %d\n",output_shift);
      // print_ints("re(shift)",&mmtmpD0);
      mmtmpD1 = simde_mm_srai_epi32(mmtmpD1,output_shift0);
      // print_ints("im(shift)",&mmtmpD1);
      mmtmpD2 = simde_mm_unpacklo_epi32(mmtmpD0,mmtmpD1);
      mmtmpD3 = simde_mm_unpackhi_epi32(mmtmpD0,mmtmpD1);
      //  print_ints("c0",&mmtmpD2);
      // print_ints("c1",&mmtmpD3);
      rxdataF_comp0_128[0] = simde_mm_packs_epi32(mmtmpD2,mmtmpD3);
      // print_shorts("rx:",rxdataF128);
      // print_shorts("ch:",dl_ch0_128);
      //print_shorts("pack:",rxdataF_comp0_128);
      // multiply by conjugated channel
      mmtmpD0 = simde_mm_madd_epi16(dl_ch0_128[1],rxdataF128[1]);
      // mmtmpD0 contains real part of 4 consecutive outputs (32-bit)
      mmtmpD1 = simde_mm_shufflelo_epi16(dl_ch0_128[1], SIMDE_MM_SHUFFLE(2,3,0,1));
      mmtmpD1 = simde_mm_shufflehi_epi16(mmtmpD1, SIMDE_MM_SHUFFLE(2,3,0,1));
      mmtmpD1 = simde_mm_sign_epi16(mmtmpD1,*(simde__m128i *)conjugate);
      mmtmpD1 = simde_mm_madd_epi16(mmtmpD1,rxdataF128[1]);
      // mmtmpD1 contains imag part of 4 consecutive outputs (32-bit)
      mmtmpD0 = simde_mm_srai_epi32(mmtmpD0,output_shift0);
      mmtmpD1 = simde_mm_srai_epi32(mmtmpD1,output_shift0);
      mmtmpD2 = simde_mm_unpacklo_epi32(mmtmpD0,mmtmpD1);
      mmtmpD3 = simde_mm_unpackhi_epi32(mmtmpD0,mmtmpD1);
      rxdataF_comp0_128[1] = simde_mm_packs_epi32(mmtmpD2,mmtmpD3);
      //  print_shorts("rx:",rxdataF128+1);
      //  print_shorts("ch:",dl_ch0_128+1);
      // print_shorts("pack:",rxdataF_comp0_128+1);

      if (pilots==0) {
        // multiply by conjugated channel
        mmtmpD0 = simde_mm_madd_epi16(dl_ch0_128[2],rxdataF128[2]);
        // mmtmpD0 contains real part of 4 consecutive outputs (32-bit)
        mmtmpD1 = simde_mm_shufflelo_epi16(dl_ch0_128[2], SIMDE_MM_SHUFFLE(2,3,0,1));
        mmtmpD1 = simde_mm_shufflehi_epi16(mmtmpD1, SIMDE_MM_SHUFFLE(2,3,0,1));
        mmtmpD1 = simde_mm_sign_epi16(mmtmpD1,*(simde__m128i *)conjugate);
        mmtmpD1 = simde_mm_madd_epi16(mmtmpD1,rxdataF128[2]);
        // mmtmpD1 contains imag part of 4 consecutive outputs (32-bit)
        mmtmpD0 = simde_mm_srai_epi32(mmtmpD0,output_shift0);
        mmtmpD1 = simde_mm_srai_epi32(mmtmpD1,output_shift0);
        mmtmpD2 = simde_mm_unpacklo_epi32(mmtmpD0,mmtmpD1);
        mmtmpD3 = simde_mm_unpackhi_epi32(mmtmpD0,mmtmpD1);
        rxdataF_comp0_128[2] = simde_mm_packs_epi32(mmtmpD2,mmtmpD3);
        //   print_shorts("rx:",rxdataF128+2);
        //   print_shorts("ch:",dl_ch0_128+2);
        //  print_shorts("pack:",rxdataF_comp0_128+2);
      }

      // layer 1
      // MF multiply by conjugated channel
      mmtmpD0 = simde_mm_madd_epi16(dl_ch1_128[0],rxdataF128[0]);
      //  print_ints("re",&mmtmpD0);
      // mmtmpD0 contains real part of 4 consecutive outputs (32-bit)
      mmtmpD1 = simde_mm_shufflelo_epi16(dl_ch1_128[0], SIMDE_MM_SHUFFLE(2,3,0,1));
      mmtmpD1 = simde_mm_shufflehi_epi16(mmtmpD1, SIMDE_MM_SHUFFLE(2,3,0,1));
      mmtmpD1 = simde_mm_sign_epi16(mmtmpD1,*(simde__m128i *)&conjugate[0]);
      //  print_ints("im",&mmtmpD1);
      mmtmpD1 = simde_mm_madd_epi16(mmtmpD1,rxdataF128[0]);
      // mmtmpD1 contains imag part of 4 consecutive outputs (32-bit)
      mmtmpD0 = simde_mm_srai_epi32(mmtmpD0,output_shift1);
      // print_ints("re(shift)",&mmtmpD0);
      mmtmpD1 = simde_mm_srai_epi32(mmtmpD1,output_shift1);
      // print_ints("im(shift)",&mmtmpD1);
      mmtmpD2 = simde_mm_unpacklo_epi32(mmtmpD0,mmtmpD1);
      mmtmpD3 = simde_mm_unpackhi_epi32(mmtmpD0,mmtmpD1);
      // print_ints("c0",&mmtmpD2);
      // print_ints("c1",&mmtmpD3);
      rxdataF_comp1_128[0] = simde_mm_packs_epi32(mmtmpD2,mmtmpD3);
      // print_shorts("rx:",rxdataF128);
      //  print_shorts("ch:",dl_ch1_128);
      // print_shorts("pack:",rxdataF_comp1_128);
      // multiply by conjugated channel
      mmtmpD0 = simde_mm_madd_epi16(dl_ch1_128[1],rxdataF128[1]);
      // mmtmpD0 contains real part of 4 consecutive outputs (32-bit)
      mmtmpD1 = simde_mm_shufflelo_epi16(dl_ch1_128[1], SIMDE_MM_SHUFFLE(2,3,0,1));
      mmtmpD1 = simde_mm_shufflehi_epi16(mmtmpD1, SIMDE_MM_SHUFFLE(2,3,0,1));
      mmtmpD1 = simde_mm_sign_epi16(mmtmpD1,*(simde__m128i *)conjugate);
      mmtmpD1 = simde_mm_madd_epi16(mmtmpD1,rxdataF128[1]);
      // mmtmpD1 contains imag part of 4 consecutive outputs (32-bit)
      mmtmpD0 = simde_mm_srai_epi32(mmtmpD0,output_shift1);
      mmtmpD1 = simde_mm_srai_epi32(mmtmpD1,output_shift1);
      mmtmpD2 = simde_mm_unpacklo_epi32(mmtmpD0,mmtmpD1);
      mmtmpD3 = simde_mm_unpackhi_epi32(mmtmpD0,mmtmpD1);
      rxdataF_comp1_128[1] = simde_mm_packs_epi32(mmtmpD2,mmtmpD3);
      //  print_shorts("rx:",rxdataF128+1);
      // print_shorts("ch:",dl_ch1_128+1);
      // print_shorts("pack:",rxdataF_comp1_128+1);

      if (pilots==0) {
        // multiply by conjugated channel
        mmtmpD0 = simde_mm_madd_epi16(dl_ch1_128[2],rxdataF128[2]);
        // mmtmpD0 contains real part of 4 consecutive outputs (32-bit)
        mmtmpD1 = simde_mm_shufflelo_epi16(dl_ch1_128[2], SIMDE_MM_SHUFFLE(2,3,0,1));
        mmtmpD1 = simde_mm_shufflehi_epi16(mmtmpD1, SIMDE_MM_SHUFFLE(2,3,0,1));
        mmtmpD1 = simde_mm_sign_epi16(mmtmpD1,*(simde__m128i *)conjugate);
        mmtmpD1 = simde_mm_madd_epi16(mmtmpD1,rxdataF128[2]);
        // mmtmpD1 contains imag part of 4 consecutive outputs (32-bit)
        mmtmpD0 = simde_mm_srai_epi32(mmtmpD0,output_shift1);
        mmtmpD1 = simde_mm_srai_epi32(mmtmpD1,output_shift1);
        mmtmpD2 = simde_mm_unpacklo_epi32(mmtmpD0,mmtmpD1);
        mmtmpD3 = simde_mm_unpackhi_epi32(mmtmpD0,mmtmpD1);
        rxdataF_comp1_128[2] = simde_mm_packs_epi32(mmtmpD2,mmtmpD3);
        //   print_shorts("rx:",rxdataF128+2);
        //  print_shorts("ch:",dl_ch1_128+2);
        //         print_shorts("pack:",rxdataF_comp1_128+2);
        dl_ch0_128+=3;
        dl_ch1_128+=3;
        dl_ch_mag0_128+=3;
        dl_ch_mag1_128+=3;
        dl_ch_mag0_128b+=3;
        dl_ch_mag1_128b+=3;
        rxdataF128+=3;
        rxdataF_comp0_128+=3;
        rxdataF_comp1_128+=3;
      } else {
        dl_ch0_128+=2;
        dl_ch1_128+=2;
        dl_ch_mag0_128+=2;
        dl_ch_mag1_128+=2;
        dl_ch_mag0_128b+=2;
        dl_ch_mag1_128b+=2;
        rxdataF128+=2;
        rxdataF_comp0_128+=2;
        rxdataF_comp1_128+=2;
      }
    } // rb loop

    Nre = (pilots==0) ? 12 : 8;
    precoded_signal_strength0 += ((signal_energy_nodc(&dl_ch_estimates_ext[aarx][symbol*frame_parms->N_RB_DL*Nre],
                                   (nb_rb*Nre))*rx_power_correction) - (measurements->n0_power[aarx]));
    precoded_signal_strength1 += ((signal_energy_nodc(&dl_ch_estimates_ext[aarx+2][symbol*frame_parms->N_RB_DL*Nre],
                                   (nb_rb*Nre))*rx_power_correction) - (measurements->n0_power[aarx]));
  } // rx_antennas

  measurements->precoded_cqi_dB[eNB_id][0] = dB_fixed2(precoded_signal_strength0,measurements->n0_power_tot);
  measurements->precoded_cqi_dB[eNB_id][1] = dB_fixed2(precoded_signal_strength1,measurements->n0_power_tot);
  // printf("eNB_id %d, symbol %d: precoded CQI %d dB\n",eNB_id,symbol,
  //  measurements->precoded_cqi_dB[eNB_id][0]);
  simde_mm_empty();
  simde_m_empty();
}


void dlsch_dual_stream_correlation(LTE_DL_FRAME_PARMS *frame_parms,
                                   unsigned char symbol,
                                   unsigned short nb_rb,
                                   int **dl_ch_estimates_ext,
                                   int **dl_ch_estimates_ext_i,
                                   int **dl_ch_rho_ext,
                                   unsigned char output_shift) {
  unsigned short rb;
  simde__m128i *dl_ch128,*dl_ch128i,*dl_ch_rho128,mmtmpD0,mmtmpD1,mmtmpD2,mmtmpD3;
  unsigned char aarx,symbol_mod,pilots=0;
  //    printf("dlsch_dual_stream_correlation: symbol %d\n",symbol);
  symbol_mod = (symbol>=(7-frame_parms->Ncp)) ? symbol-(7-frame_parms->Ncp) : symbol;

  if ((symbol_mod == 0) || (symbol_mod == (4-frame_parms->Ncp))) {
    pilots=1;
  }

  //  printf("Dual stream correlation (%p)\n",dl_ch_estimates_ext_i);

  for (aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++) {
    dl_ch128          = (simde__m128i *)&dl_ch_estimates_ext[aarx][symbol*frame_parms->N_RB_DL*12];

    if (dl_ch_estimates_ext_i == NULL) // TM3/4
      dl_ch128i         = (simde__m128i *)&dl_ch_estimates_ext[aarx + frame_parms->nb_antennas_rx][symbol*frame_parms->N_RB_DL*12];
    else
      dl_ch128i         = (simde__m128i *)&dl_ch_estimates_ext_i[aarx][symbol*frame_parms->N_RB_DL*12];

    dl_ch_rho128      = (simde__m128i *)&dl_ch_rho_ext[aarx][symbol*frame_parms->N_RB_DL*12];

    for (rb=0; rb<nb_rb; rb++) {
      // multiply by conjugated channel
      mmtmpD0 = simde_mm_madd_epi16(dl_ch128[0],dl_ch128i[0]);
      //      print_ints("re",&mmtmpD0);
      // mmtmpD0 contains real part of 4 consecutive outputs (32-bit)
      mmtmpD1 = simde_mm_shufflelo_epi16(dl_ch128[0], SIMDE_MM_SHUFFLE(2,3,0,1));
      mmtmpD1 = simde_mm_shufflehi_epi16(mmtmpD1, SIMDE_MM_SHUFFLE(2,3,0,1));
      mmtmpD1 = simde_mm_sign_epi16(mmtmpD1,*(simde__m128i *)&conjugate[0]);
      mmtmpD1 = simde_mm_madd_epi16(mmtmpD1,dl_ch128i[0]);
      //      print_ints("im",&mmtmpD1);
      // mmtmpD1 contains imag part of 4 consecutive outputs (32-bit)
      mmtmpD0 = simde_mm_srai_epi32(mmtmpD0,output_shift);
      //      print_ints("re(shift)",&mmtmpD0);
      mmtmpD1 = simde_mm_srai_epi32(mmtmpD1,output_shift);
      //      print_ints("im(shift)",&mmtmpD1);
      mmtmpD2 = simde_mm_unpacklo_epi32(mmtmpD0,mmtmpD1);
      mmtmpD3 = simde_mm_unpackhi_epi32(mmtmpD0,mmtmpD1);
      //      print_ints("c0",&mmtmpD2);
      //      print_ints("c1",&mmtmpD3);
      dl_ch_rho128[0] = simde_mm_packs_epi32(mmtmpD2,mmtmpD3);
      // print_shorts("rho 0:",dl_ch_rho128);
      // multiply by conjugated channel
      mmtmpD0 = simde_mm_madd_epi16(dl_ch128[1],dl_ch128i[1]);
      // mmtmpD0 contains real part of 4 consecutive outputs (32-bit)
      mmtmpD1 = simde_mm_shufflelo_epi16(dl_ch128[1], SIMDE_MM_SHUFFLE(2,3,0,1));
      mmtmpD1 = simde_mm_shufflehi_epi16(mmtmpD1, SIMDE_MM_SHUFFLE(2,3,0,1));
      mmtmpD1 = simde_mm_sign_epi16(mmtmpD1,*(simde__m128i *)conjugate);
      mmtmpD1 = simde_mm_madd_epi16(mmtmpD1,dl_ch128i[1]);
      // mmtmpD1 contains imag part of 4 consecutive outputs (32-bit)
      mmtmpD0 = simde_mm_srai_epi32(mmtmpD0,output_shift);
      mmtmpD1 = simde_mm_srai_epi32(mmtmpD1,output_shift);
      mmtmpD2 = simde_mm_unpacklo_epi32(mmtmpD0,mmtmpD1);
      mmtmpD3 = simde_mm_unpackhi_epi32(mmtmpD0,mmtmpD1);
      dl_ch_rho128[1] =simde_mm_packs_epi32(mmtmpD2,mmtmpD3);

      if (pilots==0) {
        // multiply by conjugated channel
        mmtmpD0 = simde_mm_madd_epi16(dl_ch128[2],dl_ch128i[2]);
        // mmtmpD0 contains real part of 4 consecutive outputs (32-bit)
        mmtmpD1 = simde_mm_shufflelo_epi16(dl_ch128[2], SIMDE_MM_SHUFFLE(2,3,0,1));
        mmtmpD1 = simde_mm_shufflehi_epi16(mmtmpD1, SIMDE_MM_SHUFFLE(2,3,0,1));
        mmtmpD1 = simde_mm_sign_epi16(mmtmpD1,*(simde__m128i *)conjugate);
        mmtmpD1 = simde_mm_madd_epi16(mmtmpD1,dl_ch128i[2]);
        // mmtmpD1 contains imag part of 4 consecutive outputs (32-bit)
        mmtmpD0 = simde_mm_srai_epi32(mmtmpD0,output_shift);
        mmtmpD1 = simde_mm_srai_epi32(mmtmpD1,output_shift);
        mmtmpD2 = simde_mm_unpacklo_epi32(mmtmpD0,mmtmpD1);
        mmtmpD3 = simde_mm_unpackhi_epi32(mmtmpD0,mmtmpD1);
        dl_ch_rho128[2] = simde_mm_packs_epi32(mmtmpD2,mmtmpD3);
        dl_ch128+=3;
        dl_ch128i+=3;
        dl_ch_rho128+=3;
      } else {
        dl_ch128+=2;
        dl_ch128i+=2;
        dl_ch_rho128+=2;
      }
    }
  }

  simde_mm_empty();
  simde_m_empty();
}


void dlsch_detection_mrc(LTE_DL_FRAME_PARMS *frame_parms,
                         int **rxdataF_comp,
                         int **rxdataF_comp_i,
                         int **rho,
                         int **rho_i,
                         int **dl_ch_mag,
                         int **dl_ch_magb,
                         int **dl_ch_mag_i,
                         int **dl_ch_magb_i,
                         unsigned char symbol,
                         unsigned short nb_rb,
                         unsigned char dual_stream_UE) {
  unsigned char aatx;
  int i;
  simde__m128i *rxdataF_comp128_0,*rxdataF_comp128_1,*rxdataF_comp128_i0,*rxdataF_comp128_i1,*dl_ch_mag128_0,*dl_ch_mag128_1,*dl_ch_mag128_0b,*dl_ch_mag128_1b,*rho128_0,*rho128_1,*rho128_i0,*rho128_i1,
          *dl_ch_mag128_i0,*dl_ch_mag128_i1,*dl_ch_mag128_i0b,*dl_ch_mag128_i1b;

  if (frame_parms->nb_antennas_rx>1) {
    for (aatx=0; aatx<frame_parms->nb_antenna_ports_eNB; aatx++) {
      rxdataF_comp128_0   = (simde__m128i *)&rxdataF_comp[(aatx<<1)][symbol*frame_parms->N_RB_DL*12];
      rxdataF_comp128_1   = (simde__m128i *)&rxdataF_comp[(aatx<<1)+1][symbol*frame_parms->N_RB_DL*12];
      dl_ch_mag128_0      = (simde__m128i *)&dl_ch_mag[(aatx<<1)][symbol*frame_parms->N_RB_DL*12];
      dl_ch_mag128_1      = (simde__m128i *)&dl_ch_mag[(aatx<<1)+1][symbol*frame_parms->N_RB_DL*12];
      dl_ch_mag128_0b     = (simde__m128i *)&dl_ch_magb[(aatx<<1)][symbol*frame_parms->N_RB_DL*12];
      dl_ch_mag128_1b     = (simde__m128i *)&dl_ch_magb[(aatx<<1)+1][symbol*frame_parms->N_RB_DL*12];

      // MRC on each re of rb, both on MF output and magnitude (for 16QAM/64QAM llr computation)
      for (i=0; i<nb_rb*3; i++) {
        rxdataF_comp128_0[i] = simde_mm_adds_epi16(simde_mm_srai_epi16(rxdataF_comp128_0[i],1),simde_mm_srai_epi16(rxdataF_comp128_1[i],1));
        dl_ch_mag128_0[i]    = simde_mm_adds_epi16(simde_mm_srai_epi16(dl_ch_mag128_0[i],1),simde_mm_srai_epi16(dl_ch_mag128_1[i],1));
        dl_ch_mag128_0b[i]   = simde_mm_adds_epi16(simde_mm_srai_epi16(dl_ch_mag128_0b[i],1),simde_mm_srai_epi16(dl_ch_mag128_1b[i],1));
        //       print_shorts("mrc comp0:",&rxdataF_comp128_0[i]);
        //       print_shorts("mrc mag0:",&dl_ch_mag128_0[i]);
        //       print_shorts("mrc mag0b:",&dl_ch_mag128_0b[i]);
        //      print_shorts("mrc rho1:",&rho128_1[i]);
      }
    }

    if (rho) {
      rho128_0 = (simde__m128i *) &rho[0][symbol*frame_parms->N_RB_DL*12];
      rho128_1 = (simde__m128i *) &rho[1][symbol*frame_parms->N_RB_DL*12];

      for (i=0; i<nb_rb*3; i++) {
        //      print_shorts("mrc rho0:",&rho128_0[i]);
        //      print_shorts("mrc rho1:",&rho128_1[i]);
        rho128_0[i] = simde_mm_adds_epi16(simde_mm_srai_epi16(rho128_0[i],1),simde_mm_srai_epi16(rho128_1[i],1));
      }
    }

    if (dual_stream_UE == 1) {
      rho128_i0 = (simde__m128i *) &rho_i[0][symbol*frame_parms->N_RB_DL*12];
      rho128_i1 = (simde__m128i *) &rho_i[1][symbol*frame_parms->N_RB_DL*12];
      rxdataF_comp128_i0   = (simde__m128i *)&rxdataF_comp_i[0][symbol*frame_parms->N_RB_DL*12];
      rxdataF_comp128_i1   = (simde__m128i *)&rxdataF_comp_i[1][symbol*frame_parms->N_RB_DL*12];
      dl_ch_mag128_i0      = (simde__m128i *)&dl_ch_mag_i[0][symbol*frame_parms->N_RB_DL*12];
      dl_ch_mag128_i1      = (simde__m128i *)&dl_ch_mag_i[1][symbol*frame_parms->N_RB_DL*12];
      dl_ch_mag128_i0b     = (simde__m128i *)&dl_ch_magb_i[0][symbol*frame_parms->N_RB_DL*12];
      dl_ch_mag128_i1b     = (simde__m128i *)&dl_ch_magb_i[1][symbol*frame_parms->N_RB_DL*12];

      for (i=0; i<nb_rb*3; i++) {
        rxdataF_comp128_i0[i] = simde_mm_adds_epi16(simde_mm_srai_epi16(rxdataF_comp128_i0[i],1),simde_mm_srai_epi16(rxdataF_comp128_i1[i],1));
        rho128_i0[i]           = simde_mm_adds_epi16(simde_mm_srai_epi16(rho128_i0[i],1),simde_mm_srai_epi16(rho128_i1[i],1));
        dl_ch_mag128_i0[i]    = simde_mm_adds_epi16(simde_mm_srai_epi16(dl_ch_mag128_i0[i],1),simde_mm_srai_epi16(dl_ch_mag128_i1[i],1));
        dl_ch_mag128_i0b[i]    = simde_mm_adds_epi16(simde_mm_srai_epi16(dl_ch_mag128_i0b[i],1),simde_mm_srai_epi16(dl_ch_mag128_i1b[i],1));
      }
    }
  }

  simde_mm_empty();
  simde_m_empty();
}

void dlsch_detection_mrc_TM34(LTE_DL_FRAME_PARMS *frame_parms,
                              LTE_UE_PDSCH *pdsch_vars,
                              int harq_pid,
                              int round,
                              unsigned char symbol,
                              unsigned short nb_rb,
                              unsigned char dual_stream_UE) {
  int i;
  simde__m128i *rxdataF_comp128_0,*rxdataF_comp128_1;
  simde__m128i *dl_ch_mag128_0,*dl_ch_mag128_1;
  simde__m128i *dl_ch_mag128_0b,*dl_ch_mag128_1b;
  simde__m128i *rho128_0, *rho128_1;
  int **rxdataF_comp0           = pdsch_vars->rxdataF_comp0;
  int **rxdataF_comp1           = pdsch_vars->rxdataF_comp1[harq_pid][round];
  int **dl_ch_rho_ext           = pdsch_vars->dl_ch_rho_ext[harq_pid][round]; //for second stream
  int **dl_ch_rho2_ext          = pdsch_vars->dl_ch_rho2_ext;
  int **dl_ch_mag0              = pdsch_vars->dl_ch_mag0;
  int **dl_ch_mag1              = pdsch_vars->dl_ch_mag1[harq_pid][round];
  int **dl_ch_magb0             = pdsch_vars->dl_ch_magb0;
  int **dl_ch_magb1             = pdsch_vars->dl_ch_magb1[harq_pid][round];
  rxdataF_comp128_0   = (simde__m128i *)&rxdataF_comp0[0][symbol*frame_parms->N_RB_DL*12];
  rxdataF_comp128_1   = (simde__m128i *)&rxdataF_comp0[1][symbol*frame_parms->N_RB_DL*12];
  dl_ch_mag128_0      = (simde__m128i *)&dl_ch_mag0[0][symbol*frame_parms->N_RB_DL*12];
  dl_ch_mag128_1      = (simde__m128i *)&dl_ch_mag0[1][symbol*frame_parms->N_RB_DL*12];
  dl_ch_mag128_0b     = (simde__m128i *)&dl_ch_magb0[0][symbol*frame_parms->N_RB_DL*12];
  dl_ch_mag128_1b     = (simde__m128i *)&dl_ch_magb0[1][symbol*frame_parms->N_RB_DL*12];
  rho128_0 = (simde__m128i *) &dl_ch_rho2_ext[0][symbol*frame_parms->N_RB_DL*12];
  rho128_1 = (simde__m128i *) &dl_ch_rho2_ext[1][symbol*frame_parms->N_RB_DL*12];

  // MRC on each re of rb, both on MF output and magnitude (for 16QAM/64QAM llr computation)
  for (i=0; i<nb_rb*3; i++) {
    rxdataF_comp128_0[i] = simde_mm_adds_epi16(simde_mm_srai_epi16(rxdataF_comp128_0[i],1),simde_mm_srai_epi16(rxdataF_comp128_1[i],1));
    dl_ch_mag128_0[i]    = simde_mm_adds_epi16(simde_mm_srai_epi16(dl_ch_mag128_0[i],1),simde_mm_srai_epi16(dl_ch_mag128_1[i],1));
    dl_ch_mag128_0b[i]   = simde_mm_adds_epi16(simde_mm_srai_epi16(dl_ch_mag128_0b[i],1),simde_mm_srai_epi16(dl_ch_mag128_1b[i],1));
    rho128_0[i] = simde_mm_adds_epi16(simde_mm_srai_epi16(rho128_0[i],1),simde_mm_srai_epi16(rho128_1[i],1));

    if (frame_parms->nb_antennas_rx>2) {
      simde__m128i *rxdataF_comp128_2 = NULL;
      simde__m128i *rxdataF_comp128_3 = NULL;
      simde__m128i *dl_ch_mag128_2 = NULL;
      simde__m128i *dl_ch_mag128_3 = NULL;
      simde__m128i *dl_ch_mag128_2b = NULL;
      simde__m128i *dl_ch_mag128_3b = NULL;
      simde__m128i *rho128_2 = NULL;
      simde__m128i *rho128_3 = NULL;
      rxdataF_comp128_2   = (simde__m128i *)&rxdataF_comp0[2][symbol*frame_parms->N_RB_DL*12];
      rxdataF_comp128_3   = (simde__m128i *)&rxdataF_comp0[3][symbol*frame_parms->N_RB_DL*12];
      dl_ch_mag128_2      = (simde__m128i *)&dl_ch_mag0[2][symbol*frame_parms->N_RB_DL*12];
      dl_ch_mag128_3      = (simde__m128i *)&dl_ch_mag0[3][symbol*frame_parms->N_RB_DL*12];
      dl_ch_mag128_2b     = (simde__m128i *)&dl_ch_magb0[2][symbol*frame_parms->N_RB_DL*12];
      dl_ch_mag128_3b     = (simde__m128i *)&dl_ch_magb0[3][symbol*frame_parms->N_RB_DL*12];
      rho128_2 = (simde__m128i *) &dl_ch_rho2_ext[2][symbol*frame_parms->N_RB_DL*12];
      rho128_3 = (simde__m128i *) &dl_ch_rho2_ext[3][symbol*frame_parms->N_RB_DL*12];
      /*rxdataF_comp*/
      rxdataF_comp128_2[i] = simde_mm_adds_epi16(simde_mm_srai_epi16(rxdataF_comp128_2[i],1),simde_mm_srai_epi16(rxdataF_comp128_3[i],1));
      rxdataF_comp128_0[i] = simde_mm_adds_epi16(simde_mm_srai_epi16(rxdataF_comp128_0[i],1),simde_mm_srai_epi16(rxdataF_comp128_2[i],1));
      /*dl_ch_mag*/
      dl_ch_mag128_2[i] = simde_mm_adds_epi16(simde_mm_srai_epi16(dl_ch_mag128_2[i],1),simde_mm_srai_epi16(dl_ch_mag128_3[i],1));
      dl_ch_mag128_0[i] = simde_mm_adds_epi16(simde_mm_srai_epi16(dl_ch_mag128_0[i],1),simde_mm_srai_epi16(dl_ch_mag128_2[i],1));
      /*dl_ch_mag*/
      dl_ch_mag128_2b[i] = simde_mm_adds_epi16(simde_mm_srai_epi16(dl_ch_mag128_2b[i],1),simde_mm_srai_epi16(dl_ch_mag128_3b[i],1));
      dl_ch_mag128_0b[i] = simde_mm_adds_epi16(simde_mm_srai_epi16(dl_ch_mag128_0b[i],1),simde_mm_srai_epi16(dl_ch_mag128_2b[i],1));
      /*rho*/
      rho128_2[i] = simde_mm_adds_epi16(simde_mm_srai_epi16(rho128_2[i],1),simde_mm_srai_epi16(rho128_3[i],1));
      rho128_0[i] = simde_mm_adds_epi16(simde_mm_srai_epi16(rho128_0[i],1),simde_mm_srai_epi16(rho128_2[i],1));
    }
  }

  if (dual_stream_UE == 1) {
    simde__m128i *dl_ch_mag128_i0, *dl_ch_mag128_i1;
    simde__m128i *dl_ch_mag128_i0b, *dl_ch_mag128_i1b;
    simde__m128i *rho128_i0, *rho128_i1;
    simde__m128i *rxdataF_comp128_i0, *rxdataF_comp128_i1;
    rxdataF_comp128_i0   = (simde__m128i *)&rxdataF_comp1[0][symbol*frame_parms->N_RB_DL*12];
    rxdataF_comp128_i1   = (simde__m128i *)&rxdataF_comp1[1][symbol*frame_parms->N_RB_DL*12];
    dl_ch_mag128_i0      = (simde__m128i *)&dl_ch_mag1[0][symbol*frame_parms->N_RB_DL*12];
    dl_ch_mag128_i1      = (simde__m128i *)&dl_ch_mag1[1][symbol*frame_parms->N_RB_DL*12];
    dl_ch_mag128_i0b     = (simde__m128i *)&dl_ch_magb1[0][symbol*frame_parms->N_RB_DL*12];
    dl_ch_mag128_i1b     = (simde__m128i *)&dl_ch_magb1[1][symbol*frame_parms->N_RB_DL*12];
    rho128_i0 = (simde__m128i *) &dl_ch_rho_ext[0][symbol*frame_parms->N_RB_DL*12];
    rho128_i1 = (simde__m128i *) &dl_ch_rho_ext[1][symbol*frame_parms->N_RB_DL*12];

    for (i=0; i<nb_rb*3; i++) {
      rxdataF_comp128_i0[i] = simde_mm_adds_epi16(simde_mm_srai_epi16(rxdataF_comp128_i0[i],1),simde_mm_srai_epi16(rxdataF_comp128_i1[i],1));
      dl_ch_mag128_i0[i]    = simde_mm_adds_epi16(simde_mm_srai_epi16(dl_ch_mag128_i0[i],1),simde_mm_srai_epi16(dl_ch_mag128_i1[i],1));
      dl_ch_mag128_i0b[i]    = simde_mm_adds_epi16(simde_mm_srai_epi16(dl_ch_mag128_i0b[i],1),simde_mm_srai_epi16(dl_ch_mag128_i1b[i],1));
      rho128_i0[i]           = simde_mm_adds_epi16(simde_mm_srai_epi16(rho128_i0[i],1),simde_mm_srai_epi16(rho128_i1[i],1));

      if (frame_parms->nb_antennas_rx>2) {
        simde__m128i *rxdataF_comp128_i2 = NULL;
        simde__m128i *rxdataF_comp128_i3 = NULL;
        simde__m128i *dl_ch_mag128_i2 = NULL;
        simde__m128i *dl_ch_mag128_i3 = NULL;
        simde__m128i *dl_ch_mag128_i2b = NULL;
        simde__m128i *dl_ch_mag128_i3b = NULL;
        simde__m128i *rho128_i2 = NULL;
        simde__m128i *rho128_i3 = NULL;
        rxdataF_comp128_i2   = (simde__m128i *)&rxdataF_comp1[2][symbol*frame_parms->N_RB_DL*12];
        rxdataF_comp128_i3   = (simde__m128i *)&rxdataF_comp1[3][symbol*frame_parms->N_RB_DL*12];
        dl_ch_mag128_i2      = (simde__m128i *)&dl_ch_mag1[2][symbol*frame_parms->N_RB_DL*12];
        dl_ch_mag128_i3      = (simde__m128i *)&dl_ch_mag1[3][symbol*frame_parms->N_RB_DL*12];
        dl_ch_mag128_i2b     = (simde__m128i *)&dl_ch_magb1[2][symbol*frame_parms->N_RB_DL*12];
        dl_ch_mag128_i3b     = (simde__m128i *)&dl_ch_magb1[3][symbol*frame_parms->N_RB_DL*12];
        rho128_i2 = (simde__m128i *) &dl_ch_rho_ext[2][symbol*frame_parms->N_RB_DL*12];
        rho128_i3 = (simde__m128i *) &dl_ch_rho_ext[3][symbol*frame_parms->N_RB_DL*12];
        /*rxdataF_comp*/
        rxdataF_comp128_i2[i] = simde_mm_adds_epi16(simde_mm_srai_epi16(rxdataF_comp128_i2[i],1),simde_mm_srai_epi16(rxdataF_comp128_i3[i],1));
        rxdataF_comp128_i0[i] = simde_mm_adds_epi16(simde_mm_srai_epi16(rxdataF_comp128_i0[i],1),simde_mm_srai_epi16(rxdataF_comp128_i2[i],1));
        /*dl_ch_mag*/
        dl_ch_mag128_i2[i] = simde_mm_adds_epi16(simde_mm_srai_epi16(dl_ch_mag128_i2[i],1),simde_mm_srai_epi16(dl_ch_mag128_i3[i],1));
        dl_ch_mag128_i0[i] = simde_mm_adds_epi16(simde_mm_srai_epi16(dl_ch_mag128_i0[i],1),simde_mm_srai_epi16(dl_ch_mag128_i2[i],1));
        /*dl_ch_mag*/
        dl_ch_mag128_i2b[i] = simde_mm_adds_epi16(simde_mm_srai_epi16(dl_ch_mag128_i2b[i],1),simde_mm_srai_epi16(dl_ch_mag128_i3b[i],1));
        dl_ch_mag128_i0b[i] = simde_mm_adds_epi16(simde_mm_srai_epi16(dl_ch_mag128_i0b[i],1),simde_mm_srai_epi16(dl_ch_mag128_i2b[i],1));
        /*rho*/
        rho128_i2[i] = simde_mm_adds_epi16(simde_mm_srai_epi16(rho128_i2[i],1),simde_mm_srai_epi16(rho128_i3[i],1));
        rho128_i0[i] = simde_mm_adds_epi16(simde_mm_srai_epi16(rho128_i0[i],1),simde_mm_srai_epi16(rho128_i2[i],1));
      }
    }
  }

  simde_mm_empty();
  simde_m_empty();
}

void dlsch_scale_channel(int **dl_ch_estimates_ext,
                         LTE_DL_FRAME_PARMS *frame_parms,
                         LTE_UE_DLSCH_t **dlsch_ue,
                         uint8_t symbol,
                         unsigned short nb_rb) {
  short rb, ch_amp;
  unsigned char aatx,aarx,pilots=0,symbol_mod;
  simde__m128i *dl_ch128, ch_amp128;
  symbol_mod = (symbol>=(7-frame_parms->Ncp)) ? symbol-(7-frame_parms->Ncp) : symbol;

  if ((symbol_mod == 0) || (symbol_mod == (4-frame_parms->Ncp))) {
    if (frame_parms->nb_antenna_ports_eNB==1) // 10 out of 12 so don't reduce size
      nb_rb=1+(5*nb_rb/6);
    else
      pilots=1;
  }

  // Determine scaling amplitude based the symbol
  ch_amp = ((pilots) ? (dlsch_ue[0]->sqrt_rho_b) : (dlsch_ue[0]->sqrt_rho_a));
  LOG_D(PHY,"Scaling PDSCH Chest in OFDM symbol %d by %d, pilots %d nb_rb %d NCP %d symbol %d\n",symbol_mod,ch_amp,pilots,nb_rb,frame_parms->Ncp,symbol);
  // printf("Scaling PDSCH Chest in OFDM symbol %d by %d\n",symbol_mod,ch_amp);
  ch_amp128 = simde_mm_set1_epi16(ch_amp); // Q3.13

  for (aatx=0; aatx<frame_parms->nb_antenna_ports_eNB; aatx++) {
    for (aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++) {
      dl_ch128=(simde__m128i *)&dl_ch_estimates_ext[(aatx<<1)+aarx][symbol*frame_parms->N_RB_DL*12];

      for (rb=0; rb<nb_rb; rb++) {
        dl_ch128[0] = simde_mm_mulhi_epi16(dl_ch128[0],ch_amp128);
        dl_ch128[0] = simde_mm_slli_epi16(dl_ch128[0],3);
        dl_ch128[1] = simde_mm_mulhi_epi16(dl_ch128[1],ch_amp128);
        dl_ch128[1] = simde_mm_slli_epi16(dl_ch128[1],3);

        if (pilots) {
          dl_ch128+=2;
        } else {
          dl_ch128[2] = simde_mm_mulhi_epi16(dl_ch128[2],ch_amp128);
          dl_ch128[2] = simde_mm_slli_epi16(dl_ch128[2],3);
          dl_ch128+=3;
        }
      }
    }
  }
}

//compute average channel_level on each (TX,RX) antenna pair
void dlsch_channel_level(int **dl_ch_estimates_ext,
                         LTE_DL_FRAME_PARMS *frame_parms,
                         int32_t *avg,
                         uint8_t symbol,
                         unsigned short nb_rb) {
  //printf("symbol = %d\n", symbol);
  short rb;
  unsigned char aatx,aarx,nre=12,symbol_mod;
  simde__m128i *dl_ch128, avg128D;
  symbol_mod = (symbol>=(7-frame_parms->Ncp)) ? symbol-(7-frame_parms->Ncp) : symbol;

  if (((symbol_mod == 0) || (symbol_mod == (frame_parms->Ncp-1)))&&(frame_parms->nb_antenna_ports_eNB!=1))
    nre=8;
  else if (((symbol_mod == 0) || (symbol_mod == (frame_parms->Ncp-1)))&&(frame_parms->nb_antenna_ports_eNB==1))
    nre=10;
  else
    nre=12;

  //nb_rb*nre = y * 2^x
  int16_t x = factor2(nb_rb*nre);
  int16_t y = (nb_rb*nre)>>x;

  for (aatx=0; aatx<frame_parms->nb_antenna_ports_eNB; aatx++)
    for (aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++) {
      //clear average level
      //printf("aatx = %d, aarx = %d, aatx*frame_parms->nb_antennas_rx + aarx] = %d \n", aatx, aarx, aatx*frame_parms->nb_antennas_rx + aarx);
      avg128D = simde_mm_setzero_si128();
      // 5 is always a symbol with no pilots for both normal and extended prefix
      dl_ch128=(simde__m128i *)&dl_ch_estimates_ext[aatx*2 + aarx][symbol*frame_parms->N_RB_DL*12];

      for (rb=0; rb<nb_rb; rb++) {
        //printf("rb %d : ",rb);
        avg128D = simde_mm_add_epi32(avg128D,simde_mm_srai_epi32(simde_mm_madd_epi16(dl_ch128[0],dl_ch128[0]),x));
        avg128D = simde_mm_add_epi32(avg128D,simde_mm_srai_epi32(simde_mm_madd_epi16(dl_ch128[1],dl_ch128[1]),x));

        // avg128D =
        // simde_mm_add_epi32(avg128D,simde_mm_madd_epi16(dl_ch128[0],simde_mm_srai_epi16(simde_mm_mulhi_epi16(dl_ch128[0],
        // coeff128),15))); avg128D =
        // simde_mm_add_epi32(avg128D,simde_mm_madd_epi16(dl_ch128[1],simde_mm_srai_epi16(simde_mm_mulhi_epi16(dl_ch128[1],
        // coeff128),15)));

        if (((symbol_mod == 0) || (symbol_mod == (frame_parms->Ncp-1)))&&(frame_parms->nb_antenna_ports_eNB!=1)) {
          dl_ch128+=2;
        } else {
          avg128D = simde_mm_add_epi32(avg128D,simde_mm_srai_epi32(simde_mm_madd_epi16(dl_ch128[2],dl_ch128[2]),x));
          // avg128D =
          // simde_mm_add_epi32(avg128D,simde_mm_madd_epi16(dl_ch128[2],simde_mm_srai_epi16(simde_mm_mulhi_epi16(dl_ch128[2],
          // coeff128),15)));
          dl_ch128+=3;
        }

        /*if(rb==0){
          print_shorts("dl_ch128",&dl_ch128[0]);
          print_shorts("dl_ch128",&dl_ch128[1]);
          print_shorts("dl_ch128",&dl_ch128[2]);
        }*/
      }

      avg[aatx*frame_parms->nb_antennas_rx + aarx] =(((int32_t *)&avg128D)[0] +
          ((int32_t *)&avg128D)[1] +
          ((int32_t *)&avg128D)[2] +
          ((int32_t *)&avg128D)[3])/y;
    }

  simde_mm_empty();
  simde_m_empty();
}

void dlsch_channel_level_core(int **dl_ch_estimates_ext,
                              int32_t *avg,
                              int n_tx,
                              int n_rx,
                              int length,
                              int start_point) {
  short ii;
  int aatx,aarx;
  int length_mod8;
  int length2;
  simde__m128i *dl_ch128, avg128D;
  int16_t x = factor2(length);
  int16_t y = (length)>>x;

  for (aatx=0; aatx<n_tx; aatx++)
    for (aarx=0; aarx<n_rx; aarx++) {
      avg128D = simde_mm_setzero_si128();
      dl_ch128=(simde__m128i *)&dl_ch_estimates_ext[aatx*2 + aarx][start_point];
      length_mod8=length&7;

      if (length_mod8 == 0) {
        length2 = length>>3;

        for (ii=0; ii<length2; ii++) {
          avg128D = simde_mm_add_epi32(avg128D,simde_mm_srai_epi32(simde_mm_madd_epi16(dl_ch128[0],dl_ch128[0]),x));
          avg128D = simde_mm_add_epi32(avg128D,simde_mm_srai_epi32(simde_mm_madd_epi16(dl_ch128[1],dl_ch128[1]),x));
          dl_ch128+=2;
        }
      } else {
        printf ("Channel level: Received number of subcarriers is not multiple of 4, \n"
                "need to adapt the code!\n");
      }

      avg[aatx*n_rx + aarx] =(((int32_t *)&avg128D)[0] +
                              ((int32_t *)&avg128D)[1] +
                              ((int32_t *)&avg128D)[2] +
                              ((int32_t *)&avg128D)[3])/y;
      //printf("Channel level [%d]: %d\n",aatx*n_rx + aarx, avg[aatx*n_rx + aarx]);
    }

  simde_mm_empty();
  simde_m_empty();
  /* FIXME This part needs to be adapted like the one above */
}

void dlsch_channel_level_median(int **dl_ch_estimates_ext,
                                int32_t *median,
                                int n_tx,
                                int n_rx,
                                int length,
                                int start_point) {
  short ii;
  int aatx,aarx;
  int length2;
  int max = 0, min=0;
  int norm_pack;
  simde__m128i *dl_ch128, norm128D;

  for (aatx=0; aatx<n_tx; aatx++) {
    for (aarx=0; aarx<n_rx; aarx++) {
      max = 0;
      min = 0;
      norm128D = simde_mm_setzero_si128();
      dl_ch128=(simde__m128i *)&dl_ch_estimates_ext[aatx*2 + aarx][start_point];
      length2 = length>>2;

      for (ii=0; ii<length2; ii++) {
        norm128D = simde_mm_srai_epi32(simde_mm_madd_epi16(dl_ch128[0],dl_ch128[0]), 1);
        //print_ints("norm128D",&norm128D[0]);
        norm_pack = ((int32_t *)&norm128D)[0] +
                    ((int32_t *)&norm128D)[1] +
                    ((int32_t *)&norm128D)[2] +
                    ((int32_t *)&norm128D)[3];

        if (norm_pack > max)
          max = norm_pack;

        if (norm_pack < min)
          min = norm_pack;

        dl_ch128+=1;
      }

      median[aatx*n_rx + aarx]  = (max+min)>>1;
      // printf("Channel level  median [%d]: %d\n",aatx*n_rx + aarx, median[aatx*n_rx + aarx]);
    }
  }

  simde_mm_empty();
  simde_m_empty();
}

void mmse_processing_oai(LTE_UE_PDSCH *pdsch_vars,
                         LTE_DL_FRAME_PARMS *frame_parms,
                         PHY_MEASUREMENTS *measurements,
                         unsigned char first_symbol_flag,
                         MIMO_mode_t mimo_mode,
                         unsigned short mmse_flag,
                         int noise_power,
                         unsigned char symbol,
                         unsigned short nb_rb) {
  int **rxdataF_ext           = pdsch_vars->rxdataF_ext;
  int **dl_ch_estimates_ext   = pdsch_vars->dl_ch_estimates_ext;
  unsigned char *pmi_ext      = pdsch_vars->pmi_ext;
  int avg_00[frame_parms->nb_antenna_ports_eNB*frame_parms->nb_antennas_rx];
  int avg_01[frame_parms->nb_antenna_ports_eNB*frame_parms->nb_antennas_rx];
  int symbol_mod, length, start_point, nre;
  symbol_mod = (symbol>=(7-frame_parms->Ncp)) ? symbol-(7-frame_parms->Ncp) : symbol;

  if (((symbol_mod == 0) || (symbol_mod == (frame_parms->Ncp-1)))&&(frame_parms->nb_antenna_ports_eNB!=1))
    nre=8;
  else if (((symbol_mod == 0) || (symbol_mod == (frame_parms->Ncp-1)))&&(frame_parms->nb_antenna_ports_eNB==1))
    nre=10;
  else
    nre=12;

  length  = nre*nb_rb;
  start_point = symbol*nb_rb*12;
  mmse_processing_core(rxdataF_ext,
                       dl_ch_estimates_ext,
                       noise_power,
                       frame_parms->nb_antenna_ports_eNB,
                       frame_parms->nb_antennas_rx,
                       length,
                       start_point);

  /*dlsch_channel_aver_band(dl_ch_estimates_ext,
                          frame_parms,
                          chan_avg,
                          symbol,
                          nb_rb);


   for (aatx=0; aatx<frame_parms->nb_antenna_ports_eNB; aatx++)
     for (aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++) {
       H[aatx*frame_parms->nb_antennas_rx + aarx] = (float)(chan_avg[aatx*frame_parms->nb_antennas_rx + aarx].r/(32768.0)) + I*(float)(chan_avg[aatx*frame_parms->nb_antennas_rx + aarx].i/(32768.0));
      // printf("H [%d] = (%f, %f) \n", aatx*frame_parms->nb_antennas_rx + aarx, creal(H[aatx*frame_parms->nb_antennas_rx + aarx]), cimag(H[aatx*frame_parms->nb_antennas_rx + aarx]));
   }*/

  if (first_symbol_flag == 1) {
    dlsch_channel_level_TM34(dl_ch_estimates_ext,
                             frame_parms,
                             pmi_ext,
                             avg_00,
                             avg_01,
                             symbol,
                             nb_rb,
                             mmse_flag,
                             mimo_mode);
    avg_00[0] = (log2_approx(avg_00[0])/2) + dlsch_demod_shift+4;// + 2 ;//+ 4;
    avg_01[0] = (log2_approx(avg_01[0])/2) + dlsch_demod_shift+4;// + 2 ;//+ 4;
    pdsch_vars->log2_maxh0 = cmax(avg_00[0],0);
    pdsch_vars->log2_maxh1 = cmax(avg_01[0],0);
  }
}

void mmse_processing_core(int32_t **rxdataF_ext,
                          int32_t **dl_ch_estimates_ext,
                          int noise_power,
                          int n_tx,
                          int n_rx,
                          int length,
                          int start_point) {
  int aatx, aarx, re;
  float imag;
  float real;
  float complex **W_MMSE= malloc(n_tx*n_rx*sizeof(float complex *));

  for (int j=0; j<n_tx*n_rx; j++) {
    W_MMSE[j] = malloc(sizeof(float complex)*length);
  }

  float complex *H=  malloc(n_tx*n_rx*sizeof(float complex));
  float complex *W_MMSE_re=  malloc(n_tx*n_rx*sizeof(float complex));
  float complex **dl_ch_estimates_ext_flcpx = malloc(n_tx*n_rx*sizeof(float complex *));

  for (int j=0; j<n_tx*n_rx; j++) {
    dl_ch_estimates_ext_flcpx[j] = malloc(sizeof(float complex)*length);
  }

  float complex **rxdataF_ext_flcpx = malloc(n_rx*sizeof(float complex *));

  for (int j=0; j<n_rx; j++) {
    rxdataF_ext_flcpx[j] = malloc(sizeof(float complex)*length);
  }

  chan_est_to_float(dl_ch_estimates_ext,
                    dl_ch_estimates_ext_flcpx,
                    n_tx,
                    n_rx,
                    length,
                    start_point);

  for (re=0; re<length; re++) {
    for (aatx=0; aatx<n_tx; aatx++) {
      for (aarx=0; aarx<n_rx; aarx++) {
        imag = cimag(dl_ch_estimates_ext_flcpx[aatx*n_rx + aarx][re]);
        real = creal(dl_ch_estimates_ext_flcpx[aatx*n_rx + aarx][re]);
        H[aatx*n_rx + aarx] = real+ I*imag;
      }
    }

    compute_MMSE(H, n_tx, noise_power, W_MMSE_re);

    for (aatx=0; aatx<n_tx; aatx++) {
      for (aarx=0; aarx<n_rx; aarx++) {
        W_MMSE[aatx*n_rx + aarx][re] = W_MMSE_re[aatx*n_rx + aarx];
      }
    }
  }

  rxdataF_to_float(rxdataF_ext,
                   rxdataF_ext_flcpx,
                   n_rx,
                   length,
                   start_point);
  mult_mmse_rxdataF(W_MMSE,
                    rxdataF_ext_flcpx,
                    n_tx,
                    n_rx,
                    length,
                    start_point);
  mult_mmse_chan_est(W_MMSE,
                     dl_ch_estimates_ext_flcpx,
                     n_tx,
                     n_rx,
                     length,
                     start_point);
  float_to_rxdataF(rxdataF_ext,
                   rxdataF_ext_flcpx,
                   n_tx,
                   n_rx,
                   length,
                   start_point);
  float_to_chan_est(dl_ch_estimates_ext,
                    dl_ch_estimates_ext_flcpx,
                    n_tx,
                    n_rx,
                    length,
                    start_point);
  free(W_MMSE);
  free(H);
  free(W_MMSE_re);
  free(dl_ch_estimates_ext_flcpx);
  free(rxdataF_ext_flcpx);
}


/*THIS FUNCTION TAKES FLOAT_POINT INPUT. SHOULD NOT BE USED WITH OAI*/
void mmse_processing_core_flp(float complex **rxdataF_ext_flcpx,
                              float complex **H,
                              int32_t **rxdataF_ext,
                              int32_t **dl_ch_estimates_ext,
                              float noise_power,
                              int n_tx,
                              int n_rx,
                              int length,
                              int start_point) {
  int aatx, aarx, re;
  float max = 0;
  float one_over_max = 0;
  float complex **W_MMSE= malloc(n_tx*n_rx*sizeof(float complex *));

  for (int j=0; j<n_tx*n_rx; j++) {
    W_MMSE[j] = malloc(sizeof(float complex)*length);
  }

  float complex *H_re=  malloc(n_tx*n_rx*sizeof(float complex));
  float complex *W_MMSE_re=  malloc(n_tx*n_rx*sizeof(float complex));

  for (re=0; re<length; re++) {
    for (aatx=0; aatx<n_tx; aatx++) {
      for (aarx=0; aarx<n_rx; aarx++) {
        H_re[aatx*n_rx + aarx] = H[aatx*n_rx + aarx][re];
#ifdef DEBUG_MMSE

        if (re == 0)
          printf(" H_re[%d]= (%f + i%f)\n", aatx*n_rx + aarx, creal(H_re[aatx*n_rx + aarx]), cimag(H_re[aatx*n_rx + aarx]));

#endif
      }
    }

    compute_MMSE(H_re, n_tx, noise_power, W_MMSE_re);

    for (aatx=0; aatx<n_tx; aatx++) {
      for (aarx=0; aarx<n_rx; aarx++) {
        W_MMSE[aatx*n_rx + aarx][re] = W_MMSE_re[aatx*n_rx + aarx];

        if (fabs(creal(W_MMSE_re[aatx*n_rx + aarx])) > max)
          max = fabs(creal(W_MMSE_re[aatx*n_rx + aarx]));

        if (fabs(cimag(W_MMSE_re[aatx*n_rx + aarx])) > max)
          max = fabs(cimag(W_MMSE_re[aatx*n_rx + aarx]));
      }
    }
  }

  one_over_max = 1.0/max;

  for (re=0; re<length; re++)
    for (aatx=0; aatx<n_tx; aatx++)
      for (aarx=0; aarx<n_rx; aarx++) {
#ifdef DEBUG_MMSE

        if (re == 0)
          printf(" W_MMSE[%d] = (%f + i%f)\n", aatx*n_rx + aarx, creal(W_MMSE[aatx*n_rx + aarx][re]), cimag(W_MMSE[aatx*n_rx + aarx][re]));

#endif
        W_MMSE[aatx*n_rx + aarx][re] = one_over_max*W_MMSE[aatx*n_rx + aarx][re];
#ifdef DEBUG_MMSE

        if (re == 0)
          printf(" AFTER NORM W_MMSE[%d] = (%f + i%f), max = %f \n", aatx*n_rx + aarx, creal(W_MMSE[aatx*n_rx + aarx][re]), cimag(W_MMSE[aatx*n_rx + aarx][re]), max);

#endif
      }

  mult_mmse_rxdataF(W_MMSE,
                    rxdataF_ext_flcpx,
                    n_tx,
                    n_rx,
                    length,
                    start_point);
  mult_mmse_chan_est(W_MMSE,
                     H,
                     n_tx,
                     n_rx,
                     length,
                     start_point);
  float_to_rxdataF(rxdataF_ext,
                   rxdataF_ext_flcpx,
                   n_tx,
                   n_rx,
                   length,
                   start_point);
  float_to_chan_est(dl_ch_estimates_ext,
                    H,
                    n_tx,
                    n_rx,
                    length,
                    start_point);
  free(H_re);
  free(W_MMSE);
  free(W_MMSE_re);
}


void dlsch_channel_aver_band(int **dl_ch_estimates_ext,
                             LTE_DL_FRAME_PARMS *frame_parms,
                             struct complex32 *chan_avg,
                             unsigned char symbol,
                             unsigned short nb_rb) {
  short rb;
  unsigned char aatx,aarx,nre=12,symbol_mod;
  simde__m128i *dl_ch128, avg128D;
  int32_t chan_est_avg[4];
  symbol_mod = (symbol>=(7-frame_parms->Ncp)) ? symbol-(7-frame_parms->Ncp) : symbol;

  if (((symbol_mod == 0) || (symbol_mod == (frame_parms->Ncp-1)))&&(frame_parms->nb_antenna_ports_eNB!=1))
    nre=8;
  else if (((symbol_mod == 0) || (symbol_mod == (frame_parms->Ncp-1)))&&(frame_parms->nb_antenna_ports_eNB==1))
    nre=10;
  else
    nre=12;

  for (aatx=0; aatx<frame_parms->nb_antenna_ports_eNB; aatx++) {
    for (aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++) {
      dl_ch128=(simde__m128i *)&dl_ch_estimates_ext[aatx*frame_parms->nb_antennas_rx + aarx][symbol*frame_parms->N_RB_DL*12];
      avg128D = simde_mm_setzero_si128();
      //  print_shorts("avg128D 1",&avg128D);

      for (rb=0; rb<nb_rb; rb++) {
        /*  printf("symbol %d, ant %d, nre*nrb %d, rb %d \n", symbol, aatx*frame_parms->nb_antennas_rx + aarx, nb_rb*nre, rb);
          print_shorts("aver dl_ch128",&dl_ch128[0]);
          print_shorts("aver dl_ch128",&dl_ch128[1]);
          print_shorts("aver dl_ch128",&dl_ch128[2]);
        avg128D = simde_mm_add_epi16(avg128D, dl_ch128[0]);*/
        //print_shorts("avg128D 2",&avg128D);
        avg128D = simde_mm_add_epi16(avg128D, dl_ch128[1]);
        //  print_shorts("avg128D 3",&avg128D);

        if (((symbol_mod == 0) || (symbol_mod == (frame_parms->Ncp-1)))&&(frame_parms->nb_antenna_ports_eNB!=1)) {
          dl_ch128+=2;
        } else {
          avg128D = simde_mm_add_epi16(avg128D,dl_ch128[2]);
          //  print_shorts("avg128D 4",&avg128D);
          dl_ch128+=3;
        }
      }

      chan_avg[aatx*frame_parms->nb_antennas_rx + aarx].r =(((int16_t *)&avg128D)[0] +
          ((int16_t *)&avg128D)[2] +
          ((int16_t *)&avg128D)[4] +
          ((int16_t *)&avg128D)[6])/(nb_rb*nre);
      //  printf("symb %d chan_avg re [%d] = %d\n", symbol, aatx*frame_parms->nb_antennas_rx + aarx, chan_avg[aatx*frame_parms->nb_antennas_rx + aarx].r);
      chan_avg[aatx*frame_parms->nb_antennas_rx + aarx].i =(((int16_t *)&avg128D)[1] +
          ((int16_t *)&avg128D)[3] +
          ((int16_t *)&avg128D)[5] +
          ((int16_t *)&avg128D)[7])/(nb_rb*nre);
      //  printf("symb %d chan_avg im [%d] = %d\n", symbol, aatx*frame_parms->nb_antennas_rx + aarx, chan_avg[aatx*frame_parms->nb_antennas_rx + aarx].i);
      //printf("symb %d chan_avg im [%d] = %d\n", symbol, aatx*frame_parms->nb_antennas_rx + aarx, chan_avg[aatx*frame_parms->nb_antennas_rx + aarx].i);
      chan_est_avg[aatx*frame_parms->nb_antennas_rx + aarx] = (((int32_t)chan_avg[aatx*frame_parms->nb_antennas_rx + aarx].i)<<16)|(((int32_t)chan_avg[aatx*frame_parms->nb_antennas_rx + aarx].r) & 0xffff);
      //printf("symb %d chan_est_avg [%d] = %d\n", symbol, aatx*frame_parms->nb_antennas_rx + aarx, chan_est_avg[aatx*frame_parms->nb_antennas_rx + aarx]);
      dl_ch128=(simde__m128i *)&dl_ch_estimates_ext[aatx*frame_parms->nb_antennas_rx + aarx][symbol*frame_parms->N_RB_DL*12];

      for (rb=0; rb<nb_rb; rb++) {
        dl_ch128[0] = simde_mm_set1_epi32(chan_est_avg[aatx*frame_parms->nb_antennas_rx + aarx]);
        dl_ch128[1] = simde_mm_set1_epi32(chan_est_avg[aatx*frame_parms->nb_antennas_rx + aarx]);

        if (((symbol_mod == 0) || (symbol_mod == (frame_parms->Ncp-1)))&&(frame_parms->nb_antenna_ports_eNB!=1)) {
          dl_ch128+=2;
        } else {
          dl_ch128[2] = simde_mm_set1_epi32(chan_est_avg[aatx*frame_parms->nb_antennas_rx + aarx]);
          dl_ch128+=3;
        }
      }
    }
  }

  simde_mm_empty();
  simde_m_empty();
}

void rxdataF_to_float(int32_t **rxdataF_ext,
                      float complex **rxdataF_f,
                      int n_rx,
                      int length,
                      int start_point) {
  short re;
  int aarx;
  int16_t imag;
  int16_t real;

  for (aarx=0; aarx<n_rx; aarx++) {
    for (re=0; re<length; re++) {
      imag = (int16_t) (rxdataF_ext[aarx][start_point + re] >> 16);
      real = (int16_t) (rxdataF_ext[aarx][start_point + re] & 0xffff);
      rxdataF_f[aarx][re] = (float)(real/(32768.0)) + I*(float)(imag/(32768.0));
#ifdef DEBUG_MMSE

      if (re==0) {
        printf("rxdataF_to_float: aarx = %d, real= %d, imag = %d\n", aarx, real, imag);
        //printf("rxdataF_to_float: rxdataF_ext[%d][%d] = %d\n", aarx, start_point + re, rxdataF_ext[aarx][start_point + re]);
        //printf("rxdataF_to_float: ant %d, re = %d, rxdataF_f real = %f, rxdataF_f imag = %f \n", aarx, re, creal(rxdataF_f[aarx][re]), cimag(rxdataF_f[aarx][re]));
      }

#endif
    }
  }
}



void chan_est_to_float(int32_t **dl_ch_estimates_ext,
                       float complex **dl_ch_estimates_ext_f,
                       int n_tx,
                       int n_rx,
                       int length,
                       int start_point) {
  short re;
  int aatx,aarx;
  int16_t imag;
  int16_t real;

  for (aatx=0; aatx<n_tx; aatx++) {
    for (aarx=0; aarx<n_rx; aarx++) {
      for (re=0; re<length; re++) {
        imag = (int16_t) (dl_ch_estimates_ext[aatx*n_rx + aarx][start_point + re] >> 16);
        real = (int16_t) (dl_ch_estimates_ext[aatx*n_rx + aarx][start_point+ re] & 0xffff);
        dl_ch_estimates_ext_f[aatx*n_rx + aarx][re] = (float)(real/(32768.0)) + I*(float)(imag/(32768.0));
#ifdef DEBUG_MMSE

        if (re==0) {
          printf("ant %d, re = %d, real = %d, imag = %d \n", aatx*n_rx + aarx, re, real, imag);
          printf("ant %d, re = %d, real = %f, imag = %f \n", aatx*n_rx + aarx, re, creal(dl_ch_estimates_ext_f[aatx*n_rx + aarx][re]), cimag(dl_ch_estimates_ext_f[aatx*n_rx + aarx][re]));
        }

#endif
      }
    }
  }
}

void float_to_chan_est(int32_t **dl_ch_estimates_ext,
                       float complex **dl_ch_estimates_ext_f,
                       int n_tx,
                       int n_rx,
                       int length,
                       int start_point) {
  short re;
  int aarx, aatx;
  int16_t imag;
  int16_t real;

  for (aatx=0; aatx<n_tx; aatx++) {
    for (aarx=0; aarx<n_rx; aarx++) {
      for (re=0; re<length; re++) {
        if (cimag(dl_ch_estimates_ext_f[aatx*n_rx + aarx][re])<-1)
          imag = 0x8000;
        else if (cimag(dl_ch_estimates_ext_f[aatx*n_rx + aarx][re])>=1)
          imag = 0x7FFF;
        else
          imag = cimag(dl_ch_estimates_ext_f[aatx*n_rx + aarx][re])*32768;

        if (creal(dl_ch_estimates_ext_f[aatx*n_rx + aarx][re])<-1)
          real = 0x8000;
        else if (creal(dl_ch_estimates_ext_f[aatx*n_rx + aarx][re])>=1)
          real = 0x7FFF;
        else
          real = creal(dl_ch_estimates_ext_f[aatx*n_rx + aarx][re])*32768;

        dl_ch_estimates_ext[aatx*n_rx + aarx][start_point + re] = (((int32_t)imag)<<16)|((int32_t)real & 0xffff);
#ifdef DEBUG_MMSE

        if (re==0) {
          printf(" float_to_chan_est: chan est real = %f, chan est imag = %f\n",creal(dl_ch_estimates_ext_f[aatx*n_rx + aarx][re]), cimag(dl_ch_estimates_ext_f[aatx*n_rx + aarx][re]));
          printf("float_to_chan_est: real fixed = %d, imag fixed = %d\n", real, imag);
          printf("float_to_chan_est: ant %d, re = %d, dl_ch_estimates_ext = %d \n", aatx*n_rx + aarx, re,  dl_ch_estimates_ext[aatx*n_rx + aarx][start_point + re]);
        }

#endif
      }
    }
  }
}


void float_to_rxdataF(int32_t **rxdataF_ext,
                      float complex **rxdataF_f,
                      int n_tx,
                      int n_rx,
                      int length,
                      int start_point) {
  short re;
  int aarx;
  int16_t imag;
  int16_t real;

  for (aarx=0; aarx<n_rx; aarx++) {
    for (re=0; re<length; re++) {
      if (cimag(rxdataF_f[aarx][re])<-1)
        imag = 0x8000;
      else if (cimag(rxdataF_f[aarx][re])>=1)
        imag = 0x7FFF;
      else
        imag = cimag(rxdataF_f[aarx][re])*32768;

      if (creal(rxdataF_f[aarx][re])<-1)
        real = 0x8000;
      else if (creal(rxdataF_f[aarx][re])>=1)
        real = 0x7FFF;
      else
        real = creal(rxdataF_f[aarx][re])*32768;

      rxdataF_ext[aarx][start_point + re] = (((int32_t)imag)<<16)|(((int32_t)real) & 0xffff);
#ifdef DEBUG_MMSE

      if (re==0) {
        printf(" float_to_rxdataF: real = %f, imag = %f\n",creal(rxdataF_f[aarx][re]), cimag(rxdataF_f[aarx][re]));
        printf("float_to_rxdataF: real fixed = %d, imag fixed = %d\n", real, imag);
        printf("float_to_rxdataF: ant %d, re = %d, rxdataF_ext = %d \n", aarx, re,  rxdataF_ext[aarx][start_point + re]);
      }

#endif
    }
  }
}


void mult_mmse_rxdataF(float complex **Wmmse,
                       float complex **rxdataF_ext_f,
                       int n_tx,
                       int n_rx,
                       int length,
                       int start_point) {
  short re;
  int aarx, aatx;
  float complex *rxdata_re =  malloc(n_rx*sizeof(float complex));
  float complex *rxdata_mmse_re =  malloc(n_rx*sizeof(float complex));
  float complex *Wmmse_re =  malloc(n_tx*n_rx*sizeof(float complex));

  for (re=0; re<length; re++) {
    for (aarx=0; aarx<n_rx; aarx++) {
      rxdata_re[aarx] = rxdataF_ext_f[aarx][re];
#ifdef DEBUG_MMSE

      if (re==0)
        printf("mult_mmse_rxdataF before: rxdata_re[%d] = (%f, %f)\n", aarx, creal(rxdata_re[aarx]), cimag(rxdata_re[aarx]));

#endif
    }

    for (aatx=0; aatx<n_tx; aatx++) {
      for (aarx=0; aarx<n_rx; aarx++) {
        Wmmse_re[aatx*n_rx + aarx] = Wmmse[aatx*n_rx + aarx][re];
      }
    }

    mutl_matrix_matrix_col_based(Wmmse_re, rxdata_re, n_rx, n_tx, n_rx, 1, rxdata_mmse_re);

    for (aarx=0; aarx<n_rx; aarx++) {
      rxdataF_ext_f[aarx][re] = rxdata_mmse_re[aarx];
#ifdef DEBUG_MMSE

      if (re==0)
        printf("mult_mmse_rxdataF after: rxdataF_ext_f[%d] = (%f, %f)\n", aarx, creal(rxdataF_ext_f[aarx][re]), cimag(rxdataF_ext_f[aarx][re]));

#endif
    }
  }

  free(rxdata_re);
  free(rxdata_mmse_re);
  free(Wmmse_re);
}

void mult_mmse_chan_est(float complex **Wmmse,
                        float complex **dl_ch_estimates_ext_f,
                        int n_tx,
                        int n_rx,
                        int length,
                        int start_point) {
  short re;
  int aarx, aatx;
  float complex *chan_est_re =  malloc(n_tx*n_rx*sizeof(float complex));
  float complex *chan_est_mmse_re =  malloc(n_tx*n_rx*sizeof(float complex));
  float complex *Wmmse_re =  malloc(n_tx*n_rx*sizeof(float complex));

  for (re=0; re<length; re++) {
    for (aatx=0; aatx<n_tx; aatx++) {
      for (aarx=0; aarx<n_rx; aarx++) {
        chan_est_re[aatx*n_rx + aarx] = dl_ch_estimates_ext_f[aatx*n_rx + aarx][re];
        Wmmse_re[aatx*n_rx + aarx] = Wmmse[aatx*n_rx + aarx][re];
#ifdef DEBUG_MMSE

        if (re==0)
          printf("mult_mmse_chan_est: chan_est_re[%d] = (%f, %f)\n", aatx*n_rx + aarx, creal(chan_est_re[aatx*n_rx + aarx]), cimag(chan_est_re[aatx*n_rx + aarx]));

#endif
      }
    }

    mutl_matrix_matrix_col_based(Wmmse_re, chan_est_re, n_rx, n_tx, n_rx, n_tx, chan_est_mmse_re);

    for (aatx=0; aatx<n_tx; aatx++) {
      for (aarx=0; aarx<n_rx; aarx++) {
        dl_ch_estimates_ext_f[aatx*n_rx + aarx][re] = chan_est_mmse_re[aatx*n_rx + aarx];
#ifdef DEBUG_MMSE

        if (re==0)
          printf("mult_mmse_chan_est: dl_ch_estimates_ext_f[%d][%d] = (%f, %f)\n", aatx*n_rx + aarx, re, creal(dl_ch_estimates_ext_f[aatx*n_rx + aarx][re]), cimag(dl_ch_estimates_ext_f[aatx*n_rx + aarx][re]));

#endif
      }
    }
  }

  free(Wmmse_re);
  free(chan_est_re);
  free(chan_est_mmse_re);
}





//compute average channel_level of effective (precoded) channel
void dlsch_channel_level_TM34(int **dl_ch_estimates_ext,
                              LTE_DL_FRAME_PARMS *frame_parms,
                              unsigned char *pmi_ext,
                              int *avg_0,
                              int *avg_1,
                              uint8_t symbol,
                              unsigned short nb_rb,
                              unsigned int mmse_flag,
                              MIMO_mode_t mimo_mode) {
  short rb;
  unsigned char aarx,nre=12,symbol_mod;
  simde__m128i *dl_ch0_128,*dl_ch1_128, dl_ch0_128_tmp, dl_ch1_128_tmp, avg_0_128D, avg_1_128D;
  symbol_mod = (symbol>=(7-frame_parms->Ncp)) ? symbol-(7-frame_parms->Ncp) : symbol;
  //clear average level
  // avg_0_128D = simde_mm_setzero_si128();
  // avg_1_128D = simde_mm_setzero_si128();
  avg_0[0] = 0;
  avg_0[1] = 0;
  avg_1[0] = 0;
  avg_1[1] = 0;
  // 5 is always a symbol with no pilots for both normal and extended prefix

  if (((symbol_mod == 0) || (symbol_mod == (frame_parms->Ncp-1)))&&(frame_parms->nb_antenna_ports_eNB!=1))
    nre=8;
  else if (((symbol_mod == 0) || (symbol_mod == (frame_parms->Ncp-1)))&&(frame_parms->nb_antenna_ports_eNB==1))
    nre=10;
  else
    nre=12;

  for (aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++) {
    dl_ch0_128 = (simde__m128i *)&dl_ch_estimates_ext[aarx][symbol*frame_parms->N_RB_DL*12];
    dl_ch1_128 = (simde__m128i *)&dl_ch_estimates_ext[2+aarx][symbol*frame_parms->N_RB_DL*12];
    avg_0_128D = simde_mm_setzero_si128();
    avg_1_128D = simde_mm_setzero_si128();

    for (rb=0; rb<nb_rb; rb++) {
      // printf("rb %d : \n",rb);
      //print_shorts("ch0\n",&dl_ch0_128[0]);
      //print_shorts("ch1\n",&dl_ch1_128[0]);
      dl_ch0_128_tmp = simde_mm_load_si128(&dl_ch0_128[0]);
      dl_ch1_128_tmp = simde_mm_load_si128(&dl_ch1_128[0]);

      if (mmse_flag == 0) {
        if (mimo_mode==LARGE_CDD)
          prec2A_TM3_128(&dl_ch0_128_tmp,&dl_ch1_128_tmp);
        else if (mimo_mode==DUALSTREAM_UNIFORM_PRECODING1)
          prec2A_TM4_128(0,&dl_ch0_128_tmp,&dl_ch1_128_tmp);
        else if (mimo_mode==DUALSTREAM_UNIFORM_PRECODINGj)
          prec2A_TM4_128(1,&dl_ch0_128_tmp,&dl_ch1_128_tmp);
        else if (mimo_mode==DUALSTREAM_PUSCH_PRECODING)
          prec2A_TM4_128(pmi_ext[rb],&dl_ch0_128_tmp,&dl_ch1_128_tmp);
      }

      //      mmtmpD0 = simde_mm_madd_epi16(dl_ch0_128_tmp,dl_ch0_128_tmp);
      avg_0_128D = simde_mm_add_epi32(avg_0_128D,simde_mm_madd_epi16(dl_ch0_128_tmp,dl_ch0_128_tmp));
      avg_1_128D = simde_mm_add_epi32(avg_1_128D,simde_mm_madd_epi16(dl_ch1_128_tmp,dl_ch1_128_tmp));
      dl_ch0_128_tmp = simde_mm_load_si128(&dl_ch0_128[1]);
      dl_ch1_128_tmp = simde_mm_load_si128(&dl_ch1_128[1]);

      if (mmse_flag == 0) {
        if (mimo_mode==LARGE_CDD)
          prec2A_TM3_128(&dl_ch0_128_tmp,&dl_ch1_128_tmp);
        else if (mimo_mode==DUALSTREAM_UNIFORM_PRECODING1)
          prec2A_TM4_128(0,&dl_ch0_128_tmp,&dl_ch1_128_tmp);
        else if (mimo_mode==DUALSTREAM_UNIFORM_PRECODINGj)
          prec2A_TM4_128(1,&dl_ch0_128_tmp,&dl_ch1_128_tmp);
        else if (mimo_mode==DUALSTREAM_PUSCH_PRECODING)
          prec2A_TM4_128(pmi_ext[rb],&dl_ch0_128_tmp,&dl_ch1_128_tmp);
      }

      //      mmtmpD1 = simde_mm_madd_epi16(dl_ch0_128_tmp,dl_ch0_128_tmp);
      avg_0_128D = simde_mm_add_epi32(avg_0_128D,simde_mm_madd_epi16(dl_ch0_128_tmp,dl_ch0_128_tmp));
      avg_1_128D = simde_mm_add_epi32(avg_1_128D,simde_mm_madd_epi16(dl_ch1_128_tmp,dl_ch1_128_tmp));

      if (((symbol_mod == 0) || (symbol_mod == (frame_parms->Ncp-1)))&&(frame_parms->nb_antenna_ports_eNB!=1)) {
        dl_ch0_128+=2;
        dl_ch1_128+=2;
      } else {
        dl_ch0_128_tmp = simde_mm_load_si128(&dl_ch0_128[2]);
        dl_ch1_128_tmp = simde_mm_load_si128(&dl_ch1_128[2]);

        if (mmse_flag == 0) {
          if (mimo_mode==LARGE_CDD)
            prec2A_TM3_128(&dl_ch0_128_tmp,&dl_ch1_128_tmp);
          else if (mimo_mode==DUALSTREAM_UNIFORM_PRECODING1)
            prec2A_TM4_128(0,&dl_ch0_128_tmp,&dl_ch1_128_tmp);
          else if (mimo_mode==DUALSTREAM_UNIFORM_PRECODINGj)
            prec2A_TM4_128(1,&dl_ch0_128_tmp,&dl_ch1_128_tmp);
          else if (mimo_mode==DUALSTREAM_PUSCH_PRECODING)
            prec2A_TM4_128(pmi_ext[rb],&dl_ch0_128_tmp,&dl_ch1_128_tmp);
        }

        //      mmtmpD2 = simde_mm_madd_epi16(dl_ch0_128_tmp,dl_ch0_128_tmp);
        avg_1_128D = simde_mm_add_epi32(avg_1_128D,simde_mm_madd_epi16(dl_ch1_128_tmp,dl_ch1_128_tmp));
        avg_0_128D = simde_mm_add_epi32(avg_0_128D,simde_mm_madd_epi16(dl_ch0_128_tmp,dl_ch0_128_tmp));
        dl_ch0_128+=3;
        dl_ch1_128+=3;
      }
    }

    avg_0[aarx] = (((int *)&avg_0_128D)[0])/(nb_rb*nre) +
                  (((int *)&avg_0_128D)[1])/(nb_rb*nre) +
                  (((int *)&avg_0_128D)[2])/(nb_rb*nre) +
                  (((int *)&avg_0_128D)[3])/(nb_rb*nre);
    //  printf("From Chan_level aver stream 0 %d =%d\n", aarx, avg_0[aarx]);
    avg_1[aarx] = (((int *)&avg_1_128D)[0])/(nb_rb*nre) +
                  (((int *)&avg_1_128D)[1])/(nb_rb*nre) +
                  (((int *)&avg_1_128D)[2])/(nb_rb*nre) +
                  (((int *)&avg_1_128D)[3])/(nb_rb*nre);
    //    printf("From Chan_level aver stream 1 %d =%d\n", aarx, avg_1[aarx]);
  }

  //avg_0[0] = max(avg_0[0],avg_0[1]);
  //avg_1[0] = max(avg_1[0],avg_1[1]);
  //avg_0[0]= max(avg_0[0], avg_1[0]);
  avg_0[0] = avg_0[0] + avg_0[1];
  // printf("From Chan_level aver stream 0 final =%d\n", avg_0[0]);
  avg_1[0] = avg_1[0] + avg_1[1];
  // printf("From Chan_level aver stream 1 final =%d\n", avg_1[0]);
  avg_0[0] = min (avg_0[0], avg_1[0]);
  avg_1[0] = avg_0[0];
  simde_mm_empty();
  simde_m_empty();
}

//compute average channel_level of effective (precoded) channel
void dlsch_channel_level_TM56(int **dl_ch_estimates_ext,
                              LTE_DL_FRAME_PARMS *frame_parms,
                              unsigned char *pmi_ext,
                              int *avg,
                              uint8_t symbol,
                              unsigned short nb_rb) {
  short rb;
  unsigned char aarx,nre=12,symbol_mod;
  simde__m128i *dl_ch0_128,*dl_ch1_128, dl_ch0_128_tmp, dl_ch1_128_tmp,avg128D;
  symbol_mod = (symbol>=(7-frame_parms->Ncp)) ? symbol-(7-frame_parms->Ncp) : symbol;
  //clear average level
  avg128D = simde_mm_setzero_si128();
  avg[0] = 0;
  avg[1] = 0;
  // 5 is always a symbol with no pilots for both normal and extended prefix

  if (((symbol_mod == 0) || (symbol_mod == (frame_parms->Ncp-1)))&&(frame_parms->nb_antenna_ports_eNB!=1))
    nre=8;
  else if (((symbol_mod == 0) || (symbol_mod == (frame_parms->Ncp-1)))&&(frame_parms->nb_antenna_ports_eNB==1))
    nre=10;
  else
    nre=12;

  for (aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++) {
    dl_ch0_128 = (simde__m128i *)&dl_ch_estimates_ext[aarx][symbol*frame_parms->N_RB_DL*12];
    dl_ch1_128 = (simde__m128i *)&dl_ch_estimates_ext[2+aarx][symbol*frame_parms->N_RB_DL*12];

    for (rb=0; rb<nb_rb; rb++) {
      dl_ch0_128_tmp = simde_mm_load_si128(&dl_ch0_128[0]);
      dl_ch1_128_tmp = simde_mm_load_si128(&dl_ch1_128[0]);
      prec2A_TM56_128(pmi_ext[rb],&dl_ch0_128_tmp,&dl_ch1_128_tmp);
      //      mmtmpD0 = simde_mm_madd_epi16(dl_ch0_128_tmp,dl_ch0_128_tmp);
      avg128D = simde_mm_add_epi32(avg128D,simde_mm_madd_epi16(dl_ch0_128_tmp,dl_ch0_128_tmp));
      dl_ch0_128_tmp = simde_mm_load_si128(&dl_ch0_128[1]);
      dl_ch1_128_tmp = simde_mm_load_si128(&dl_ch1_128[1]);
      prec2A_TM56_128(pmi_ext[rb],&dl_ch0_128_tmp,&dl_ch1_128_tmp);
      //      mmtmpD1 = simde_mm_madd_epi16(dl_ch0_128_tmp,dl_ch0_128_tmp);
      avg128D = simde_mm_add_epi32(avg128D,simde_mm_madd_epi16(dl_ch0_128_tmp,dl_ch0_128_tmp));

      if (((symbol_mod == 0) || (symbol_mod == (frame_parms->Ncp-1)))&&(frame_parms->nb_antenna_ports_eNB!=1)) {
        dl_ch0_128+=2;
        dl_ch1_128+=2;
      } else {
        dl_ch0_128_tmp = simde_mm_load_si128(&dl_ch0_128[2]);
        dl_ch1_128_tmp = simde_mm_load_si128(&dl_ch1_128[2]);
        prec2A_TM56_128(pmi_ext[rb],&dl_ch0_128_tmp,&dl_ch1_128_tmp);
        //      mmtmpD2 = simde_mm_madd_epi16(dl_ch0_128_tmp,dl_ch0_128_tmp);
        avg128D = simde_mm_add_epi32(avg128D,simde_mm_madd_epi16(dl_ch0_128_tmp,dl_ch0_128_tmp));
        dl_ch0_128+=3;
        dl_ch1_128+=3;
      }
    }

    avg[aarx] = (((int *)&avg128D)[0])/(nb_rb*nre) +
                (((int *)&avg128D)[1])/(nb_rb*nre) +
                (((int *)&avg128D)[2])/(nb_rb*nre) +
                (((int *)&avg128D)[3])/(nb_rb*nre);
  }

  // choose maximum of the 2 effective channels
  avg[0] = cmax(avg[0],avg[1]);
  simde_mm_empty();
  simde_m_empty();
}

//compute average channel_level for TM7
void dlsch_channel_level_TM7(int **dl_bf_ch_estimates_ext,
                             LTE_DL_FRAME_PARMS *frame_parms,
                             int *avg,
                             uint8_t symbol,
                             unsigned short nb_rb) {
  short rb;
  unsigned char aatx,aarx,nre=12,symbol_mod;
  simde__m128i *dl_ch128,avg128D;
  symbol_mod = (symbol>=(7-frame_parms->Ncp)) ? symbol-(7-frame_parms->Ncp) : symbol;

  for (aatx=0; aatx<frame_parms->nb_antenna_ports_eNB; aatx++)
    for (aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++) {
      //clear average level
      avg128D = simde_mm_setzero_si128();
      // 5 is always a symbol with no pilots for both normal and extended prefix
      dl_ch128=(simde__m128i *)&dl_bf_ch_estimates_ext[(aatx<<1)+aarx][symbol*frame_parms->N_RB_DL*12];

      for (rb=0; rb<nb_rb; rb++) {
        //  printf("rb %d : ",rb);
        //  print_shorts("ch",&dl_ch128[0]);
        avg128D = simde_mm_add_epi32(avg128D,simde_mm_madd_epi16(dl_ch128[0],dl_ch128[0]));
        avg128D = simde_mm_add_epi32(avg128D,simde_mm_madd_epi16(dl_ch128[1],dl_ch128[1]));

        if (((symbol_mod == 0) || (symbol_mod == (frame_parms->Ncp-1)))&&(frame_parms->nb_antenna_ports_eNB!=1)) {
          dl_ch128+=2;
        } else {
          avg128D = simde_mm_add_epi32(avg128D,simde_mm_madd_epi16(dl_ch128[2],dl_ch128[2]));
          dl_ch128+=3;
        }

        /*
          if (rb==0) {
          print_shorts("dl_ch128",&dl_ch128[0]);
          print_shorts("dl_ch128",&dl_ch128[1]);
          print_shorts("dl_ch128",&dl_ch128[2]);
          }
        */
      }

      if (((symbol_mod == 0) || (symbol_mod == (frame_parms->Ncp-1))))
        nre=10;
      else if ((frame_parms->Ncp==0) && (symbol==3 || symbol==6 || symbol==9 || symbol==12))
        nre=9;
      else if ((frame_parms->Ncp==1) && (symbol==4 || symbol==7 || symbol==9))
        nre=8;
      else
        nre=12;

      avg[(aatx<<1)+aarx] = (((int *)&avg128D)[0] +
                             ((int *)&avg128D)[1] +
                             ((int *)&avg128D)[2] +
                             ((int *)&avg128D)[3])/(nb_rb*nre);
      //            printf("Channel level : %d\n",avg[(aatx<<1)+aarx]);
    }

  simde_mm_empty();
  simde_m_empty();
}
//#define ONE_OVER_2_Q15 16384
void dlsch_alamouti(LTE_DL_FRAME_PARMS *frame_parms,
                    int **rxdataF_comp,
                    int **dl_ch_mag,
                    int **dl_ch_magb,
                    unsigned char symbol,
                    unsigned short nb_rb) {
  short *rxF0,*rxF1;
  simde__m128i *ch_mag0,*ch_mag1,*ch_mag0b,*ch_mag1b;
  unsigned char rb,re;
  int jj = (symbol*frame_parms->N_RB_DL*12);
  uint8_t symbol_mod = (symbol>=(7-frame_parms->Ncp)) ? symbol-(7-frame_parms->Ncp) : symbol;
  uint8_t pilots = ((symbol_mod==0)||(symbol_mod==(4-frame_parms->Ncp))) ? 1 : 0;
  // amp = simde_mm_set1_epi16(ONE_OVER_2_Q15);
  //     printf("Doing alamouti!\n");
  rxF0     = (short *)&rxdataF_comp[0][jj]; //tx antenna 0  h0*y
  rxF1     = (short *)&rxdataF_comp[2][jj]; //tx antenna 1  h1*y
  ch_mag0 = (simde__m128i *)&dl_ch_mag[0][jj];
  ch_mag1 = (simde__m128i *)&dl_ch_mag[2][jj];
  ch_mag0b = (simde__m128i *)&dl_ch_magb[0][jj];
  ch_mag1b = (simde__m128i *)&dl_ch_magb[2][jj];

  for (rb=0; rb<nb_rb; rb++) {
    for (re=0; re<((pilots==0)?12:8); re+=2) {
      // Alamouti RX combining
      //      printf("Alamouti: symbol %d, rb %d, re %d: rxF0 (%d,%d,%d,%d), rxF1 (%d,%d,%d,%d)\n",symbol,rb,re,rxF0[0],rxF0[1],rxF0[2],rxF0[3],rxF1[0],rxF1[1],rxF1[2],rxF1[3]);
      rxF0[0] = rxF0[0] + rxF1[2];
      rxF0[1] = rxF0[1] - rxF1[3];
      rxF0[2] = rxF0[2] - rxF1[0];
      rxF0[3] = rxF0[3] + rxF1[1];
      //      printf("Alamouti: rxF0 after (%d,%d,%d,%d)\n",rxF0[0],rxF0[1],rxF0[2],rxF0[3]);
      rxF0+=4;
      rxF1+=4;
    }

    // compute levels for 16QAM or 64 QAM llr unit
    ch_mag0[0] = simde_mm_adds_epi16(ch_mag0[0],ch_mag1[0]);
    ch_mag0[1] = simde_mm_adds_epi16(ch_mag0[1],ch_mag1[1]);
    ch_mag0b[0] = simde_mm_adds_epi16(ch_mag0b[0],ch_mag1b[0]);
    ch_mag0b[1] = simde_mm_adds_epi16(ch_mag0b[1],ch_mag1b[1]);

    // account for 1/sqrt(2) scaling at transmission
    //ch_mag0[0] = simde_mm_srai_epi16(ch_mag0[0],1);
    //ch_mag0[1] = simde_mm_srai_epi16(ch_mag0[1],1);
    //ch_mag0b[0] = simde_mm_srai_epi16(ch_mag0b[0],1);
    //ch_mag0b[1] = simde_mm_srai_epi16(ch_mag0b[1],1);



    if (pilots==0) {
      ch_mag0[2] = simde_mm_adds_epi16(ch_mag0[2],ch_mag1[2]);
      ch_mag0b[2] = simde_mm_adds_epi16(ch_mag0b[2],ch_mag1b[2]);
      //ch_mag0[2] = simde_mm_srai_epi16(ch_mag0[2],1);
      //ch_mag0b[2] = simde_mm_srai_epi16(ch_mag0b[2],1);
      ch_mag0+=3;
      ch_mag1+=3;
      ch_mag0b+=3;
      ch_mag1b+=3;
    } else {
      ch_mag0+=2;
      ch_mag1+=2;
      ch_mag0b+=2;
      ch_mag1b+=2;
    }
  }

  simde_mm_empty();
  simde_m_empty();
}

//==============================================================================================
// Extraction functions
//==============================================================================================

unsigned short dlsch_extract_rbs_single(int **rxdataF,
                                        int **dl_ch_estimates,
                                        int **rxdataF_ext,
                                        int **dl_ch_estimates_ext,
                                        unsigned short pmi,
                                        unsigned char *pmi_ext,
                                        unsigned int *rb_alloc,
                                        unsigned char symbol,
                                        unsigned char subframe,
                                        uint32_t high_speed_flag,
                                        LTE_DL_FRAME_PARMS *frame_parms) {
  unsigned short rb,nb_rb=0;
  unsigned char rb_alloc_ind;
  unsigned char i,aarx,l,nsymb,skip_half=0,sss_symb,pss_symb=0;
  int *dl_ch0,*dl_ch0_ext,*rxF,*rxF_ext;
  unsigned char symbol_mod,pilots=0,j=0,poffset=0;
  symbol_mod = (symbol>=(7-frame_parms->Ncp)) ? symbol-(7-frame_parms->Ncp) : symbol;
  pilots = ((symbol_mod==0)||(symbol_mod==(4-frame_parms->Ncp))) ? 1 : 0;
  l=symbol;
  nsymb = (frame_parms->Ncp==NORMAL) ? 14:12;

  if (frame_parms->frame_type == TDD) {  // TDD
    sss_symb = nsymb-1;
    pss_symb = 2;
  } else {
    sss_symb = (nsymb>>1)-2;
    pss_symb = (nsymb>>1)-1;
  }

  if (symbol_mod==(4-frame_parms->Ncp))
    poffset=3;

  for (aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++) {
    if (high_speed_flag == 1)
      dl_ch0     = &dl_ch_estimates[aarx][5+(symbol*(frame_parms->ofdm_symbol_size))];
    else
      dl_ch0     = &dl_ch_estimates[aarx][5];

    dl_ch0_ext = &dl_ch_estimates_ext[aarx][symbol*(frame_parms->N_RB_DL*12)];
    rxF_ext   = &rxdataF_ext[aarx][symbol*(frame_parms->N_RB_DL*12)];
    rxF       = &rxdataF[aarx][(frame_parms->first_carrier_offset + (symbol*(frame_parms->ofdm_symbol_size)))];

    if ((frame_parms->N_RB_DL&1) == 0)  // even number of RBs
      for (rb=0; rb<frame_parms->N_RB_DL; rb++) {
        if (rb < 32)
          rb_alloc_ind = (rb_alloc[0]>>rb) & 1;
        else if (rb < 64)
          rb_alloc_ind = (rb_alloc[1]>>(rb-32)) & 1;
        else if (rb < 96)
          rb_alloc_ind = (rb_alloc[2]>>(rb-64)) & 1;
        else if (rb < 100)
          rb_alloc_ind = (rb_alloc[3]>>(rb-96)) & 1;
        else
          rb_alloc_ind = 0;

        if (rb_alloc_ind == 1)
          nb_rb++;

        // For second half of RBs skip DC carrier
        if (rb==(frame_parms->N_RB_DL>>1)) {
          rxF       = &rxdataF[aarx][(1 + (symbol*(frame_parms->ofdm_symbol_size)))];
          //dl_ch0++;
        }

        // PBCH
        if ((subframe==0) && (rb>=((frame_parms->N_RB_DL>>1)-3)) && (rb<((frame_parms->N_RB_DL>>1)+3)) && (l>=nsymb>>1) && (l<((nsymb>>1) + 4))) {
          rb_alloc_ind = 0;
        }

        //SSS
        if (((subframe==0)||(subframe==5)) && (rb>=((frame_parms->N_RB_DL>>1)-3)) && (rb<((frame_parms->N_RB_DL>>1)+3)) && (l==sss_symb) ) {
          rb_alloc_ind = 0;
        }

        if (frame_parms->frame_type == FDD) {
          //PSS
          if (((subframe==0)||(subframe==5)) && (rb>=((frame_parms->N_RB_DL>>1)-3)) && (rb<((frame_parms->N_RB_DL>>1)+3)) && (l==pss_symb) ) {
            rb_alloc_ind = 0;
          }
        }

        if ((frame_parms->frame_type == TDD) &&
            (subframe==6)) { //TDD Subframe 6
          if ((rb>=((frame_parms->N_RB_DL>>1)-3)) && (rb<((frame_parms->N_RB_DL>>1)+3)) && (l==pss_symb) ) {
            rb_alloc_ind = 0;
          }
        }

        if (rb_alloc_ind==1) {
          uint32_t tmp = (rb >> 2) << 1;
          *pmi_ext = tmp >= 16 ? 0 : (pmi >> tmp) & 3;
          memcpy(dl_ch0_ext,dl_ch0,12*sizeof(int));

          /*
            printf("rb %d\n",rb);
            for (i=0;i<12;i++)
            printf("(%d %d)",((short *)dl_ch0)[i<<1],((short*)dl_ch0)[1+(i<<1)]);
            printf("\n");
          */
          if (pilots==0) {
            for (i=0; i<12; i++) {
              rxF_ext[i]=rxF[i];
              /*
                printf("%d : (%d,%d)\n",(rxF+i-&rxdataF[aarx][( (symbol*(frame_parms->ofdm_symbol_size)))]),
                ((short*)&rxF[i])[0],((short*)&rxF[i])[1]);*/
            }

            dl_ch0_ext+=12;
            rxF_ext+=12;
          } else {
            j=0;

            for (i=0; i<12; i++) {
              if ((i!=(frame_parms->nushift+poffset)) &&
                  (i!=((frame_parms->nushift+poffset+6)%12))) {
                rxF_ext[j]=rxF[i];
                //            printf("extract rb %d, re %d => (%d,%d)\n",rb,i,*(short *)&rxF_ext[j],*(1+(short*)&rxF_ext[j]));
                dl_ch0_ext[j++]=dl_ch0[i];
              }
            }

            dl_ch0_ext+=10;
            rxF_ext+=10;
          }
        }

        dl_ch0+=12;
        rxF+=12;
      } else { // Odd number of RBs
      for (rb=0; rb<frame_parms->N_RB_DL>>1; rb++) {
#ifdef DEBUG_DLSCH_DEMOD
        printf("dlch_ext %u\n",dl_ch0_ext-&dl_ch_estimates_ext[aarx][0]);
#endif
        skip_half=0;

        if (rb < 32)
          rb_alloc_ind = (rb_alloc[0]>>rb) & 1;
        else if (rb < 64)
          rb_alloc_ind = (rb_alloc[1]>>(rb-32)) & 1;
        else if (rb < 96)
          rb_alloc_ind = (rb_alloc[2]>>(rb-64)) & 1;
        else if (rb < 100)
          rb_alloc_ind = (rb_alloc[3]>>(rb-96)) & 1;
        else
          rb_alloc_ind = 0;

        if (rb_alloc_ind == 1)
          nb_rb++;

        // PBCH
        if ((subframe==0) && (rb>((frame_parms->N_RB_DL>>1)-3)) && (rb<((frame_parms->N_RB_DL>>1)+3)) && (l>=(nsymb>>1)) && (l<((nsymb>>1) + 4))) {
          rb_alloc_ind = 0;
        }

        //PBCH subframe 0, symbols nsymb>>1 ... nsymb>>1 + 3
        if ((subframe==0) && (rb==((frame_parms->N_RB_DL>>1)-3)) && (l>=(nsymb>>1)) && (l<((nsymb>>1) + 4)))
          skip_half=1;
        else if ((subframe==0) && (rb==((frame_parms->N_RB_DL>>1)+3)) && (l>=(nsymb>>1)) && (l<((nsymb>>1) + 4)))
          skip_half=2;

        //SSS

        if (((subframe==0)||(subframe==5)) &&
            (rb>((frame_parms->N_RB_DL>>1)-3)) &&
            (rb<((frame_parms->N_RB_DL>>1)+3)) &&
            (l==sss_symb) ) {
          rb_alloc_ind = 0;
        }

        //SSS
        if (((subframe==0)||(subframe==5)) &&
            (rb==((frame_parms->N_RB_DL>>1)-3)) &&
            (l==sss_symb))
          skip_half=1;
        else if (((subframe==0)||(subframe==5)) &&
                 (rb==((frame_parms->N_RB_DL>>1)+3)) &&
                 (l==sss_symb))
          skip_half=2;

        //PSS in subframe 0/5 if FDD
        if (frame_parms->frame_type == FDD) {  //FDD
          if (((subframe==0)||(subframe==5)) &&
              (rb>((frame_parms->N_RB_DL>>1)-3)) &&
              (rb<((frame_parms->N_RB_DL>>1)+3)) &&
              (l==pss_symb) ) {
            rb_alloc_ind = 0;
          }

          if (((subframe==0)||(subframe==5)) && (rb==((frame_parms->N_RB_DL>>1)-3)) && (l==pss_symb))
            skip_half=1;
          else if (((subframe==0)||(subframe==5)) && (rb==((frame_parms->N_RB_DL>>1)+3)) && (l==pss_symb))
            skip_half=2;
        }

        if ((frame_parms->frame_type == TDD) &&
            (subframe==6)) { //TDD Subframe 6
          if ((rb>((frame_parms->N_RB_DL>>1)-3)) && (rb<((frame_parms->N_RB_DL>>1)+3)) && (l==pss_symb) ) {
            rb_alloc_ind = 0;
          }

          if ((rb==((frame_parms->N_RB_DL>>1)-3)) && (l==pss_symb))
            skip_half=1;
          else if ((rb==((frame_parms->N_RB_DL>>1)+3)) && (l==pss_symb))
            skip_half=2;
        }

        if (rb_alloc_ind==1) {
#ifdef DEBUG_DLSCH_DEMOD
          printf("rb %d/symbol %d (skip_half %d)\n",rb,l,skip_half);
#endif

          if (pilots==0) {
            //      printf("Extracting w/o pilots (symbol %d, rb %d, skip_half %d)\n",l,rb,skip_half);
            if (skip_half==1) {
              memcpy(dl_ch0_ext,dl_ch0,6*sizeof(int));

              for (i=0; i<6; i++) {
                rxF_ext[i]=rxF[i];
#ifdef DEBUG_DLSCH_DEMOD
                printf("extract rb %d, re %d => (%d,%d)\n",rb,i,*(short *)&rxF_ext[i],*(1+(short *)&rxF_ext[i]));
#endif
              }

              dl_ch0_ext+=6;
              rxF_ext+=6;
            } else if (skip_half==2) {
              memcpy(dl_ch0_ext,dl_ch0+6,6*sizeof(int));

              for (i=0; i<6; i++) {
                rxF_ext[i]=rxF[(i+6)];
#ifdef DEBUG_DLSCH_DEMOD
                printf("extract rb %d, re %d => (%d,%d)\n",rb,i,*(short *)&rxF_ext[i],*(1+(short *)&rxF_ext[i]));
#endif
              }

              dl_ch0_ext+=6;
              rxF_ext+=6;
            } else {
              memcpy(dl_ch0_ext,dl_ch0,12*sizeof(int));

              for (i=0; i<12; i++) {
                rxF_ext[i]=rxF[i];
#ifdef DEBUG_DLSCH_DEMOD
                printf("extract rb %d, re %d => (%d,%d)\n",rb,i,*(short *)&rxF_ext[i],*(1+(short *)&rxF_ext[i]));
#endif
              }

              dl_ch0_ext+=12;
              rxF_ext+=12;
            }
          } else {
            //      printf("Extracting with pilots (symbol %d, rb %d, skip_half %d)\n",l,rb,skip_half);
            j=0;

            if (skip_half==1) {
              for (i=0; i<6; i++) {
                if (i!=((frame_parms->nushift+poffset)%6)) {
                  rxF_ext[j]=rxF[i];
#ifdef DEBUG_DLSCH_DEMOD
                  printf("extract rb %d, re %d => (%d,%d)\n",rb,i,*(short *)&rxF_ext[j],*(1+(short *)&rxF_ext[j]));
#endif
                  dl_ch0_ext[j++]=dl_ch0[i];
                }
              }

              rxF_ext+=5;
              dl_ch0_ext+=5;
            } else if (skip_half==2) {
              for (i=0; i<6; i++) {
                if (i!=((frame_parms->nushift+poffset)%6)) {
                  rxF_ext[j]=rxF[(i+6)];
#ifdef DEBUG_DLSCH_DEMOD
                  printf("extract rb %d, re %d => (%d,%d)\n",rb,i,*(short *)&rxF_ext[j],*(1+(short *)&rxF_ext[j]));
#endif
                  dl_ch0_ext[j++]=dl_ch0[i+6];
                }
              }

              dl_ch0_ext+=5;
              rxF_ext+=5;
            } else {
              for (i=0; i<12; i++) {
                if ((i!=(frame_parms->nushift+poffset)) &&
                    (i!=((frame_parms->nushift+poffset+6)%12))) {
                  rxF_ext[j]=rxF[i];
#ifdef DEBUG_DLSCH_DEMOD
                  printf("extract rb %d, re %d => (%d,%d)\n",rb,i,*(short *)&rxF_ext[j],*(1+(short *)&rxF_ext[j]));
#endif
                  dl_ch0_ext[j++]=dl_ch0[i];
                }
              }

              dl_ch0_ext+=10;
              rxF_ext+=10;
            }
          }
        }

        dl_ch0+=12;
        rxF+=12;
      } // first half loop

      // Do middle RB (around DC)
      if (rb < 32)
        rb_alloc_ind = (rb_alloc[0]>>rb) & 1;
      else if (rb < 64)
        rb_alloc_ind = (rb_alloc[1]>>(rb-32)) & 1;
      else if (rb < 96)
        rb_alloc_ind = (rb_alloc[2]>>(rb-64)) & 1;
      else if (rb < 100)
        rb_alloc_ind = (rb_alloc[3]>>(rb-96)) & 1;
      else
        rb_alloc_ind = 0;

      if (rb_alloc_ind == 1)
        nb_rb++;

      // PBCH

      if ((subframe==0) &&
          (l>=(nsymb>>1)) &&
          (l<((nsymb>>1) + 4))) {
        rb_alloc_ind = 0;
      }

      //SSS
      if (((subframe==0)||(subframe==5)) && (l==sss_symb) ) {
        rb_alloc_ind = 0;
      }

      if (frame_parms->frame_type == FDD) {
        //PSS
        if (((subframe==0)||(subframe==5)) && (l==pss_symb) ) {
          rb_alloc_ind = 0;
        }
      }

      //PSS
      if ((frame_parms->frame_type == TDD) &&
          (subframe==6) &&
          (l==pss_symb) ) {
        rb_alloc_ind = 0;
      }

      //  printf("dlch_ext %d\n",dl_ch0_ext-&dl_ch_estimates_ext[aarx][0]);
      //      printf("DC rb %d (%p)\n",rb,rxF);
      if (rb_alloc_ind==1) {
#ifdef DEBUG_DLSCH_DEMOD
        printf("rb %d/symbol %d (skip_half %d)\n",rb,l,skip_half);
#endif

        if (pilots==0) {
          for (i=0; i<6; i++) {
            dl_ch0_ext[i]=dl_ch0[i];
            rxF_ext[i]=rxF[i];
          }

          rxF       = &rxdataF[aarx][((symbol*(frame_parms->ofdm_symbol_size)))];

          for (; i<12; i++) {
            dl_ch0_ext[i]=dl_ch0[i];
            rxF_ext[i]=rxF[(1+i-6)];
          }

          dl_ch0_ext+=12;
          rxF_ext+=12;
        } else { // pilots==1
          j=0;

          for (i=0; i<6; i++) {
            if (i!=((frame_parms->nushift+poffset)%6)) {
              dl_ch0_ext[j]=dl_ch0[i];
              rxF_ext[j++]=rxF[i];
#ifdef DEBUG_DLSCH_DEMOD
              printf("**extract rb %d, re %d => (%d,%d)\n",rb,i,*(short *)&rxF_ext[j-1],*(1+(short *)&rxF_ext[j-1]));
#endif
            }
          }

          rxF       = &rxdataF[aarx][((symbol*(frame_parms->ofdm_symbol_size)))];

          for (; i<12; i++) {
            if (i!=((frame_parms->nushift+6+poffset)%12)) {
              dl_ch0_ext[j]=dl_ch0[i];
              rxF_ext[j++]=rxF[(1+i-6)];
#ifdef DEBUG_DLSCH_DEMOD
              printf("**extract rb %d, re %d => (%d,%d)\n",rb,i,*(short *)&rxF_ext[j-1],*(1+(short *)&rxF_ext[j-1]));
#endif
            }
          }

          dl_ch0_ext+=10;
          rxF_ext+=10;
        } // symbol_mod==0
      } // rballoc==1
      else {
        rxF       = &rxdataF[aarx][((symbol*(frame_parms->ofdm_symbol_size)))];
      }

      dl_ch0+=12;
      rxF+=7;
      rb++;

      for (; rb<frame_parms->N_RB_DL; rb++) {
        //      printf("dlch_ext %d\n",dl_ch0_ext-&dl_ch_estimates_ext[aarx][0]);
        //      printf("rb %d (%p)\n",rb,rxF);
        skip_half=0;

        if (rb < 32)
          rb_alloc_ind = (rb_alloc[0]>>rb) & 1;
        else if (rb < 64)
          rb_alloc_ind = (rb_alloc[1]>>(rb-32)) & 1;
        else if (rb < 96)
          rb_alloc_ind = (rb_alloc[2]>>(rb-64)) & 1;
        else if (rb < 100)
          rb_alloc_ind = (rb_alloc[3]>>(rb-96)) & 1;
        else
          rb_alloc_ind = 0;

        if (rb_alloc_ind == 1)
          nb_rb++;

        // PBCH
        if ((subframe==0) && (rb>((frame_parms->N_RB_DL>>1)-3)) && (rb<((frame_parms->N_RB_DL>>1)+3)) && (l>=nsymb>>1) && (l<((nsymb>>1) + 4))) {
          rb_alloc_ind = 0;
        }

        //PBCH subframe 0, symbols nsymb>>1 ... nsymb>>1 + 3
        if ((subframe==0) && (rb==((frame_parms->N_RB_DL>>1)-3)) && (l>=(nsymb>>1)) && (l<((nsymb>>1) + 4)))
          skip_half=1;
        else if ((subframe==0) && (rb==((frame_parms->N_RB_DL>>1)+3)) && (l>=(nsymb>>1)) && (l<((nsymb>>1) + 4)))
          skip_half=2;

        //SSS
        if (((subframe==0)||(subframe==5)) && (rb>((frame_parms->N_RB_DL>>1)-3)) && (rb<((frame_parms->N_RB_DL>>1)+3)) && (l==sss_symb) ) {
          rb_alloc_ind = 0;
        }

        //SSS
        if (((subframe==0)||(subframe==5)) && (rb==((frame_parms->N_RB_DL>>1)-3)) && (l==sss_symb))
          skip_half=1;
        else if (((subframe==0)||(subframe==5)) && (rb==((frame_parms->N_RB_DL>>1)+3)) && (l==sss_symb))
          skip_half=2;

        if (frame_parms->frame_type == FDD) {
          //PSS
          if (((subframe==0)||(subframe==5)) && (rb>((frame_parms->N_RB_DL>>1)-3)) && (rb<((frame_parms->N_RB_DL>>1)+3)) && (l==pss_symb) ) {
            rb_alloc_ind = 0;
          }

          //PSS

          if (((subframe==0)||(subframe==5)) && (rb==((frame_parms->N_RB_DL>>1)-3)) && (l==pss_symb))
            skip_half=1;
          else if (((subframe==0)||(subframe==5)) && (rb==((frame_parms->N_RB_DL>>1)+3)) && (l==pss_symb))
            skip_half=2;
        }

        if ((frame_parms->frame_type == TDD) &&
            (subframe==6)) { //TDD Subframe 6
          if ((rb>((frame_parms->N_RB_DL>>1)-3)) && (rb<((frame_parms->N_RB_DL>>1)+3)) && (l==pss_symb) ) {
            rb_alloc_ind = 0;
          }

          if ((rb==((frame_parms->N_RB_DL>>1)-3)) && (l==pss_symb))
            skip_half=1;
          else if ((rb==((frame_parms->N_RB_DL>>1)+3)) && (l==pss_symb))
            skip_half=2;
        }

        if (rb_alloc_ind==1) {
#ifdef DEBUG_DLSCH_DEMOD
          printf("rb %d/symbol %d (skip_half %d)\n",rb,l,skip_half);
#endif

          /*
            printf("rb %d\n",rb);
            for (i=0;i<12;i++)
            printf("(%d %d)",((short *)dl_ch0)[i<<1],((short*)dl_ch0)[1+(i<<1)]);
            printf("\n");
          */
          if (pilots==0) {
            //      printf("Extracting w/o pilots (symbol %d, rb %d, skip_half %d)\n",l,rb,skip_half);
            if (skip_half==1) {
              memcpy(dl_ch0_ext,dl_ch0,6*sizeof(int));

              for (i=0; i<6; i++) {
                rxF_ext[i]=rxF[i];
#ifdef DEBUG_DLSCH_DEMOD
                printf("extract rb %d, re %d => (%d,%d)\n",rb,i,*(short *)&rxF_ext[i],*(1+(short *)&rxF_ext[i]));
#endif
              }

              dl_ch0_ext+=6;
              rxF_ext+=6;
            } else if (skip_half==2) {
              memcpy(dl_ch0_ext,dl_ch0+6,6*sizeof(int));

              for (i=0; i<6; i++) {
                rxF_ext[i]=rxF[(i+6)];
#ifdef DEBUG_DLSCH_DEMOD
                printf("extract rb %d, re %d => (%d,%d)\n",rb,i,*(short *)&rxF_ext[i],*(1+(short *)&rxF_ext[i]));
#endif
              }

              dl_ch0_ext+=6;
              rxF_ext+=6;
            } else {
              memcpy(dl_ch0_ext,dl_ch0,12*sizeof(int));

              for (i=0; i<12; i++) {
                rxF_ext[i]=rxF[i];
#ifdef DEBUG_DLSCH_DEMOD
                printf("extract rb %d, re %d => (%d,%d)\n",rb,i,*(short *)&rxF_ext[i],*(1+(short *)&rxF_ext[i]));
#endif
              }

              dl_ch0_ext+=12;
              rxF_ext+=12;
            }
          } else {
            //      printf("Extracting with pilots (symbol %d, rb %d, skip_half %d)\n",l,rb,skip_half);
            j=0;

            if (skip_half==1) {
              for (i=0; i<6; i++) {
                if (i!=((frame_parms->nushift+poffset)%6)) {
                  rxF_ext[j]=rxF[i];
#ifdef DEBUG_DLSCH_DEMOD
                  printf("extract rb %d, re %d => (%d,%d)\n",rb,i,*(short *)&rxF_ext[j],*(1+(short *)&rxF_ext[j]));
#endif
                  dl_ch0_ext[j++]=dl_ch0[i];
                }
              }

              dl_ch0_ext+=5;
              rxF_ext+=5;
            } else if (skip_half==2) {
              for (i=0; i<6; i++) {
                if (i!=((frame_parms->nushift+poffset)%6)) {
                  rxF_ext[j]=rxF[(i+6)];
#ifdef DEBUG_DLSCH_DEMOD
                  printf("extract rb %d, re %d => (%d,%d)\n",rb,i,*(short *)&rxF_ext[j],*(1+(short *)&rxF_ext[j]));
#endif
                  dl_ch0_ext[j++]=dl_ch0[i+6];
                }
              }

              dl_ch0_ext+=5;
              rxF_ext+=5;
            } else {
              for (i=0; i<12; i++) {
                if ((i!=(frame_parms->nushift+poffset)) &&
                    (i!=((frame_parms->nushift+poffset+6)%12))) {
                  rxF_ext[j]=rxF[i];
#ifdef DEBUG_DLSCH_DEMOD
                  printf("extract rb %d, re %d => (%d,%d)\n",rb,i,*(short *)&rxF_ext[j],*(1+(short *)&rxF_ext[j]));
#endif
                  dl_ch0_ext[j++]=dl_ch0[i];
                }
              }

              dl_ch0_ext+=10;
              rxF_ext+=10;
            }
          } // pilots=0
        }

        dl_ch0+=12;
        rxF+=12;
      }
    }
  }

  return(nb_rb/frame_parms->nb_antennas_rx);
}

unsigned short dlsch_extract_rbs_dual(int **rxdataF,
                                      int **dl_ch_estimates,
                                      int **rxdataF_ext,
                                      int **dl_ch_estimates_ext,
                                      unsigned short pmi,
                                      unsigned char *pmi_ext,
                                      unsigned int *rb_alloc,
                                      unsigned char symbol,
                                      unsigned char subframe,
                                      uint32_t high_speed_flag,
                                      LTE_DL_FRAME_PARMS *frame_parms,
                                      MIMO_mode_t mimo_mode) {
  int prb,nb_rb=0;
  int prb_off,prb_off2;
  int rb_alloc_ind,skip_half=0,sss_symb,pss_symb=0,nsymb,l;
  int i,aarx;
  int32_t *dl_ch0,*dl_ch0p,*dl_ch0_ext,*dl_ch1,*dl_ch1p,*dl_ch1_ext,*rxF,*rxF_ext;
  int symbol_mod,pilots=0,j=0;
  unsigned char *pmi_loc;
  symbol_mod = (symbol>=(7-frame_parms->Ncp)) ? symbol-(7-frame_parms->Ncp) : symbol;
  //  printf("extract_rbs: symbol_mod %d\n",symbol_mod);

  if ((symbol_mod == 0) || (symbol_mod == (4-frame_parms->Ncp)))
    pilots=1;

  nsymb = (frame_parms->Ncp==NORMAL) ? 14:12;
  l=symbol;

  if (frame_parms->frame_type == TDD) {  // TDD
    sss_symb = nsymb-1;
    pss_symb = 2;
  } else {
    sss_symb = (nsymb>>1)-2;
    pss_symb = (nsymb>>1)-1;
  }

  for (aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++) {
    if (high_speed_flag==1) {
      dl_ch0     = &dl_ch_estimates[aarx][5+(symbol*(frame_parms->ofdm_symbol_size))];
      dl_ch1     = &dl_ch_estimates[2+aarx][5+(symbol*(frame_parms->ofdm_symbol_size))];
    } else {
      dl_ch0     = &dl_ch_estimates[aarx][5];
      dl_ch1     = &dl_ch_estimates[2+aarx][5];
    }

    pmi_loc = pmi_ext;
    // pointers to extracted RX signals and channel estimates
    rxF_ext    = &rxdataF_ext[aarx][symbol*(frame_parms->N_RB_DL*12)];
    dl_ch0_ext = &dl_ch_estimates_ext[aarx][symbol*(frame_parms->N_RB_DL*12)];
    dl_ch1_ext = &dl_ch_estimates_ext[2+aarx][symbol*(frame_parms->N_RB_DL*12)];

    for (prb=0; prb<frame_parms->N_RB_DL; prb++) {
      skip_half=0;

      if (prb < 32)
        rb_alloc_ind = (rb_alloc[0]>>prb) & 1;
      else if (prb < 64)
        rb_alloc_ind = (rb_alloc[1]>>(prb-32)) & 1;
      else if (prb < 96)
        rb_alloc_ind = (rb_alloc[2]>>(prb-64)) & 1;
      else if (prb < 100)
        rb_alloc_ind = (rb_alloc[3]>>(prb-96)) & 1;
      else
        rb_alloc_ind = 0;

      if (rb_alloc_ind == 1)
        nb_rb++;

      if ((frame_parms->N_RB_DL&1) == 0) {  // even number of RBs

        // PBCH
        if ((subframe==0) &&
            (prb>=((frame_parms->N_RB_DL>>1)-3)) &&
            (prb<((frame_parms->N_RB_DL>>1)+3)) &&
            (l>=(nsymb>>1)) &&
            (l<((nsymb>>1) + 4))) {
          rb_alloc_ind = 0;
          //    printf("symbol %d / rb %d: skipping PBCH REs\n",symbol,prb);
        }

        //SSS

        if (((subframe==0)||(subframe==5)) &&
            (prb>=((frame_parms->N_RB_DL>>1)-3)) &&
            (prb<((frame_parms->N_RB_DL>>1)+3)) &&
            (l==sss_symb) ) {
          rb_alloc_ind = 0;
          //    printf("symbol %d / rb %d: skipping SSS REs\n",symbol,prb);
        }

        //PSS in subframe 0/5 if FDD
        if (frame_parms->frame_type == FDD) {  //FDD
          if (((subframe==0)||(subframe==5)) &&
              (prb>=((frame_parms->N_RB_DL>>1)-3)) &&
              (prb<((frame_parms->N_RB_DL>>1)+3)) &&
              (l==pss_symb) ) {
            rb_alloc_ind = 0;
            //    printf("symbol %d / rb %d: skipping PSS REs\n",symbol,prb);
          }
        }

        if ((frame_parms->frame_type == TDD) &&
            (subframe==6)) { //TDD Subframe 6
          if ((prb>=((frame_parms->N_RB_DL>>1)-3)) &&
              (prb<((frame_parms->N_RB_DL>>1)+3)) &&
              (l==pss_symb) ) {
            rb_alloc_ind = 0;
          }
        }

        if (rb_alloc_ind==1) {              // PRB is allocated
          prb_off      = 12*prb;
          prb_off2     = 1+(12*(prb-(frame_parms->N_RB_DL>>1)));
          dl_ch0p    = dl_ch0+(12*prb);
          dl_ch1p    = dl_ch1+(12*prb);

          if (prb<(frame_parms->N_RB_DL>>1)) {
            rxF      = &rxdataF[aarx][prb_off+
                                      frame_parms->first_carrier_offset +
                                      (symbol*(frame_parms->ofdm_symbol_size))];
          } else {
            rxF      = &rxdataF[aarx][prb_off2+
                                      (symbol*(frame_parms->ofdm_symbol_size))];
          }

          /*
          if (mimo_mode <= PUSCH_PRECODING1)
           *pmi_loc = (pmi>>((prb>>2)<<1))&3;
          else
           *pmi_loc=(pmi>>prb)&1;*/
          *pmi_loc = get_pmi(frame_parms->N_RB_DL,mimo_mode,pmi,prb);
          pmi_loc++;

          if (pilots == 0) {
            memcpy(dl_ch0_ext,dl_ch0p,12*sizeof(int));
            memcpy(dl_ch1_ext,dl_ch1p,12*sizeof(int));
            memcpy(rxF_ext,rxF,12*sizeof(int));
            dl_ch0_ext +=12;
            dl_ch1_ext +=12;
            rxF_ext    +=12;
          } else { // pilots==1
            j=0;

            for (i=0; i<12; i++) {
              if ((i!=frame_parms->nushift) &&
                  (i!=frame_parms->nushift+3) &&
                  (i!=frame_parms->nushift+6) &&
                  (i!=((frame_parms->nushift+9)%12))) {
                rxF_ext[j]=rxF[i];
                //        printf("extract rb %d, re %d => (%d,%d)\n",rb,i,*(short *)&rxF_ext[j],*(1+(short*)&rxF_ext[j]));
                dl_ch0_ext[j]=dl_ch0p[i];
                dl_ch1_ext[j++]=dl_ch1p[i];
              }
            }

            dl_ch0_ext+=8;
            dl_ch1_ext+=8;
            rxF_ext+=8;
          } // pilots==1
        }
      } else {  // Odd number of RBs

        // PBCH
        if ((subframe==0) &&
            (prb>((frame_parms->N_RB_DL>>1)-3)) &&
            (prb<((frame_parms->N_RB_DL>>1)+3)) &&
            (l>=(nsymb>>1)) &&
            (l<((nsymb>>1) + 4))) {
          rb_alloc_ind = 0;
          //    printf("symbol %d / rb %d: skipping PBCH REs\n",symbol,prb);
        }

        //SSS

        if (((subframe==0)||(subframe==5)) &&
            (prb>((frame_parms->N_RB_DL>>1)-3)) &&
            (prb<((frame_parms->N_RB_DL>>1)+3)) &&
            (l==sss_symb) ) {
          rb_alloc_ind = 0;
          //    printf("symbol %d / rb %d: skipping SSS REs\n",symbol,prb);
        }

        //PSS in subframe 0/5 if FDD
        if (frame_parms->frame_type == FDD) {  //FDD
          if (((subframe==0)||(subframe==5)) &&
              (prb>((frame_parms->N_RB_DL>>1)-3)) &&
              (prb<((frame_parms->N_RB_DL>>1)+3)) &&
              (l==pss_symb) ) {
            rb_alloc_ind = 0;
            //    printf("symbol %d / rb %d: skipping PSS REs\n",symbol,prb);
          }
        }

        if ((frame_parms->frame_type == TDD) &&
            ((subframe==1) || (subframe==6))) { //TDD Subframe 1-6
          if ((prb>((frame_parms->N_RB_DL>>1)-3)) &&
              (prb<((frame_parms->N_RB_DL>>1)+3)) &&
              (l==pss_symb) ) {
            rb_alloc_ind = 0;
          }
        }

        if (rb_alloc_ind == 1) {
          skip_half=0;

          //Check if we have to drop half a PRB due to PSS/SSS/PBCH
          // skip_half == 0 means full PRB
          // skip_half == 1 means first half is used (leftmost half-PRB from PSS/SSS/PBCH)
          // skip_half == 2 means second half is used (rightmost half-PRB from PSS/SSS/PBCH)
          //PBCH subframe 0, symbols nsymb>>1 ... nsymb>>1 + 3
          if ((subframe==0) &&
              (prb==((frame_parms->N_RB_DL>>1)-3)) &&
              (l>=(nsymb>>1)) &&
              (l<((nsymb>>1) + 4)))
            skip_half=1;
          else if ((subframe==0) &&
                   (prb==((frame_parms->N_RB_DL>>1)+3)) &&
                   (l>=(nsymb>>1)) &&
                   (l<((nsymb>>1) + 4)))
            skip_half=2;

          //SSS
          if (((subframe==0)||(subframe==5)) &&
              (prb==((frame_parms->N_RB_DL>>1)-3)) &&
              (l==sss_symb))
            skip_half=1;
          else if (((subframe==0)||(subframe==5)) &&
                   (prb==((frame_parms->N_RB_DL>>1)+3)) &&
                   (l==sss_symb))
            skip_half=2;

          //PSS Subframe 0,5
          if (((frame_parms->frame_type == FDD) &&
               (((subframe==0)||(subframe==5)))) ||  //FDD Subframes 0,5
              ((frame_parms->frame_type == TDD) &&
               (((subframe==1) || (subframe==6))))) { //TDD Subframes 1,6
            if ((prb==((frame_parms->N_RB_DL>>1)-3)) &&
                (l==pss_symb))
              skip_half=1;
            else if ((prb==((frame_parms->N_RB_DL>>1)+3)) &&
                     (l==pss_symb))
              skip_half=2;
          }

          prb_off      = 12*prb;
          prb_off2     = 7+(12*(prb-(frame_parms->N_RB_DL>>1)-1));
          dl_ch0p      = dl_ch0+(12*prb);
          dl_ch1p      = dl_ch1+(12*prb);

          if (prb<=(frame_parms->N_RB_DL>>1)) {
            rxF      = &rxdataF[aarx][prb_off+
                                      frame_parms->first_carrier_offset +
                                      (symbol*(frame_parms->ofdm_symbol_size))];
          } else {
            rxF      = &rxdataF[aarx][prb_off2+
                                      (symbol*(frame_parms->ofdm_symbol_size))];
          }

#ifdef DEBUG_DLSCH_DEMOD
          printf("symbol %d / rb %d: alloc %d skip_half %d (rxF %p, rxF_ext %p) prb_off (%d,%d)\n",symbol,prb,rb_alloc_ind,skip_half,rxF,rxF_ext,prb_off,prb_off2);
#endif
          /* if (mimo_mode <= PUSCH_PRECODING1)
            *pmi_loc = (pmi>>((prb>>2)<<1))&3;
           else
            *pmi_loc=(pmi>>prb)&1;
          // printf("symbol_mod %d (pilots %d) rb %d, sb %d, pmi %d (pmi_loc %p,rxF %p, ch00 %p, ch01 %p, rxF_ext %p dl_ch0_ext %p dl_ch1_ext %p)\n",symbol_mod,pilots,prb,prb>>2,*pmi_loc,pmi_loc,rxF,dl_ch0, dl_ch1, rxF_ext,dl_ch0_ext,dl_ch1_ext);
          */
          *pmi_loc = get_pmi(frame_parms->N_RB_DL,mimo_mode,pmi,prb);
          pmi_loc++;

          if (prb != (frame_parms->N_RB_DL>>1)) { // This PRB is not around DC
            if (pilots==0) {
              if (skip_half==1) {
                memcpy(dl_ch0_ext,dl_ch0p,6*sizeof(int32_t));
                memcpy(dl_ch1_ext,dl_ch1p,6*sizeof(int32_t));
                memcpy(rxF_ext,rxF,6*sizeof(int32_t));
#ifdef DEBUG_DLSCH_DEMOD

                for (i=0; i<6; i++)
                  printf("extract rb %d, re %d => (%d,%d)\n",prb,i,*(short *)&rxF_ext[i],*(1+(short *)&rxF_ext[i]));

#endif
                dl_ch0_ext+=6;
                dl_ch1_ext+=6;
                rxF_ext+=6;
              } else if (skip_half==2) {
                memcpy(dl_ch0_ext,dl_ch0p+6,6*sizeof(int32_t));
                memcpy(dl_ch1_ext,dl_ch1p+6,6*sizeof(int32_t));
                memcpy(rxF_ext,rxF+6,6*sizeof(int32_t));
#ifdef DEBUG_DLSCH_DEMOD

                for (i=0; i<6; i++)
                  printf("extract rb %d, re %d => (%d,%d)\n",prb,i,*(short *)&rxF_ext[i],*(1+(short *)&rxF_ext[i]));

#endif
                dl_ch0_ext+=6;
                dl_ch1_ext+=6;
                rxF_ext+=6;
              } else {  // skip_half==0
                memcpy(dl_ch0_ext,dl_ch0p,12*sizeof(int32_t));
                memcpy(dl_ch1_ext,dl_ch1p,12*sizeof(int32_t));
                memcpy(rxF_ext,rxF,12*sizeof(int32_t));
#ifdef DEBUG_DLSCH_DEMOD

                for (i=0; i<12; i++)
                  printf("extract rb %d, re %d => (%d,%d)\n",prb,i,*(short *)&rxF_ext[i],*(1+(short *)&rxF_ext[i]));

#endif
                dl_ch0_ext+=12;
                dl_ch1_ext+=12;
                rxF_ext+=12;
              }
            } else { // pilots=1
              j=0;

              if (skip_half==1) {
                for (i=0; i<6; i++) {
                  if ((i!=frame_parms->nushift) &&
                      (i!=((frame_parms->nushift+3)%6))) {
                    rxF_ext[j]=rxF[i];
#ifdef DEBUG_DLSCH_DEMOD
                    printf("(pilots,skip1)extract rb %d, re %d (%d)=> (%d,%d)\n",prb,i,j,*(short *)&rxF_ext[j],*(1+(short *)&rxF_ext[j]));
#endif
                    dl_ch0_ext[j]=dl_ch0p[i];
                    dl_ch1_ext[j++]=dl_ch1p[i];
                  }
                }

                dl_ch0_ext+=4;
                dl_ch1_ext+=4;
                rxF_ext+=4;
              } else if (skip_half==2) {
                for (i=0; i<6; i++) {
                  if ((i!=frame_parms->nushift) &&
                      (i!=((frame_parms->nushift+3)%6))) {
                    rxF_ext[j]=rxF[(i+6)];
#ifdef DEBUG_DLSCH_DEMOD
                    printf("(pilots,skip2)extract rb %d, re %d (%d) => (%d,%d)\n",prb,i,j,*(short *)&rxF_ext[j],*(1+(short *)&rxF_ext[j]));
#endif
                    dl_ch0_ext[j]=dl_ch0p[i+6];
                    dl_ch1_ext[j++]=dl_ch1p[i+6];
                  }
                }

                dl_ch0_ext+=4;
                dl_ch1_ext+=4;
                rxF_ext+=4;
              } else { //skip_half==0
                for (i=0; i<12; i++) {
                  if ((i!=frame_parms->nushift) &&
                      (i!=frame_parms->nushift+3) &&
                      (i!=frame_parms->nushift+6) &&
                      (i!=((frame_parms->nushift+9)%12))) {
                    rxF_ext[j]=rxF[i];
#ifdef DEBUG_DLSCH_DEMOD
                    printf("(pilots)extract rb %d, re %d => (%d,%d)\n",prb,i,*(short *)&rxF_ext[j],*(1+(short *)&rxF_ext[j]));
#endif
                    dl_ch0_ext[j]  =dl_ch0p[i];
                    dl_ch1_ext[j++]=dl_ch1p[i];
                  }
                }

                dl_ch0_ext+=8;
                dl_ch1_ext+=8;
                rxF_ext+=8;
              } //skip_half==0
            } //pilots==1
          } else {       // Do middle RB (around DC)
            if (pilots==0) {
              memcpy(dl_ch0_ext,dl_ch0p,6*sizeof(int32_t));
              memcpy(dl_ch1_ext,dl_ch1p,6*sizeof(int32_t));
              memcpy(rxF_ext,rxF,6*sizeof(int32_t));
#ifdef DEBUG_DLSCH_DEMOD

              for (i=0; i<6; i++) {
                printf("extract rb %d, re %d => (%d,%d)\n",prb,i,*(short *)&rxF_ext[i],*(1+(short *)&rxF_ext[i]));
              }

#endif
              rxF_ext+=6;
              dl_ch0_ext+=6;
              dl_ch1_ext+=6;
              dl_ch0p+=6;
              dl_ch1p+=6;
              rxF       = &rxdataF[aarx][1+((symbol*(frame_parms->ofdm_symbol_size)))];
              memcpy(dl_ch0_ext,dl_ch0p,6*sizeof(int32_t));
              memcpy(dl_ch1_ext,dl_ch1p,6*sizeof(int32_t));
              memcpy(rxF_ext,rxF,6*sizeof(int32_t));
#ifdef DEBUG_DLSCH_DEMOD

              for (i=0; i<6; i++) {
                printf("extract rb %d, re %d => (%d,%d)\n",prb,i,*(short *)&rxF_ext[i],*(1+(short *)&rxF_ext[i]));
              }

#endif
              rxF_ext+=6;
              dl_ch0_ext+=6;
              dl_ch1_ext+=6;
            } else { // pilots==1
              j=0;

              for (i=0; i<6; i++) {
                if ((i!=frame_parms->nushift) &&
                    (i!=((frame_parms->nushift+3)%6))) {
                  dl_ch0_ext[j]=dl_ch0p[i];
                  dl_ch1_ext[j]=dl_ch1p[i];
                  rxF_ext[j++]=rxF[i];
#ifdef DEBUG_DLSCH_DEMOD
                  printf("(pilots)extract rb %d, re %d (%d) => (%d,%d)\n",prb,i,j,*(short *)&rxF[i],*(1+(short *)&rxF[i]));
#endif
                }
              }

              rxF       = &rxdataF[aarx][1+symbol*(frame_parms->ofdm_symbol_size)];

              for (; i<12; i++) {
                if ((i!=((frame_parms->nushift+6)%12)) &&
                    (i!=((frame_parms->nushift+9)%12))) {
                  dl_ch0_ext[j]=dl_ch0p[i];
                  dl_ch1_ext[j]=dl_ch1p[i];
                  rxF_ext[j++]=rxF[i-6];
#ifdef DEBUG_DLSCH_DEMOD
                  printf("(pilots)extract rb %d, re %d (%d) => (%d,%d)\n",prb,i,j,*(short *)&rxF[1+i-6],*(1+(short *)&rxF[1+i-6]));
#endif
                }
              }

              dl_ch0_ext+=8;
              dl_ch1_ext+=8;
              rxF_ext+=8;
            } //pilots==1
          }  // if Middle PRB
        } // if odd PRB
      } // if rballoc==1
    } // for prb
  } // for aarx

  return(nb_rb/frame_parms->nb_antennas_rx);
}

unsigned short dlsch_extract_rbs_TM7(int **rxdataF,
                                     int **dl_bf_ch_estimates,
                                     int **rxdataF_ext,
                                     int **dl_bf_ch_estimates_ext,
                                     unsigned int *rb_alloc,
                                     unsigned char symbol,
                                     unsigned char subframe,
                                     uint32_t high_speed_flag,
                                     LTE_DL_FRAME_PARMS *frame_parms) {
  unsigned short rb,nb_rb=0;
  unsigned char rb_alloc_ind;
  unsigned char i,aarx,l,nsymb,skip_half=0,sss_symb,pss_symb=0;
  int *dl_ch0,*dl_ch0_ext,*rxF,*rxF_ext;
  unsigned char symbol_mod,pilots=0,uespec_pilots=0,j=0,poffset=0,uespec_poffset=0;
  int8_t uespec_nushift = frame_parms->Nid_cell%3;
  symbol_mod = (symbol>=(7-frame_parms->Ncp)) ? symbol-(7-frame_parms->Ncp) : symbol;
  pilots = ((symbol_mod==0)||(symbol_mod==(4-frame_parms->Ncp))) ? 1 : 0;
  l=symbol;
  nsymb = (frame_parms->Ncp==NORMAL) ? 14:12;

  if (frame_parms->Ncp==0) {
    if (symbol==3 || symbol==6 || symbol==9 || symbol==12)
      uespec_pilots = 1;
  } else {
    if (symbol==4 || symbol==7 || symbol==10)
      uespec_pilots = 1;
  }

  if (frame_parms->frame_type == TDD) {// TDD
    sss_symb = nsymb-1;
    pss_symb = 2;
  } else {
    sss_symb = (nsymb>>1)-2;
    pss_symb = (nsymb>>1)-1;
  }

  if (symbol_mod==(4-frame_parms->Ncp))
    poffset=3;

  if ((frame_parms->Ncp==0 && (symbol==6 ||symbol ==12)) || (frame_parms->Ncp==1 && symbol==7))
    uespec_poffset=2;

  for (aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++) {
    if (high_speed_flag == 1)
      dl_ch0     = &dl_bf_ch_estimates[aarx][symbol*(frame_parms->ofdm_symbol_size)];
    else
      dl_ch0     = &dl_bf_ch_estimates[aarx][0];

    dl_ch0_ext = &dl_bf_ch_estimates_ext[aarx][symbol*(frame_parms->N_RB_DL*12)];
    rxF_ext    = &rxdataF_ext[aarx][symbol*(frame_parms->N_RB_DL*12)];
    rxF        = &rxdataF[aarx][(frame_parms->first_carrier_offset + (symbol*(frame_parms->ofdm_symbol_size)))];

    if ((frame_parms->N_RB_DL&1) == 0)  // even number of RBs
      for (rb=0; rb<frame_parms->N_RB_DL; rb++) {
        if (rb < 32)
          rb_alloc_ind = (rb_alloc[0]>>rb) & 1;
        else if (rb < 64)
          rb_alloc_ind = (rb_alloc[1]>>(rb-32)) & 1;
        else if (rb < 96)
          rb_alloc_ind = (rb_alloc[2]>>(rb-64)) & 1;
        else if (rb < 100)
          rb_alloc_ind = (rb_alloc[3]>>(rb-96)) & 1;
        else
          rb_alloc_ind = 0;

        if (rb_alloc_ind == 1)
          nb_rb++;

        // For second half of RBs skip DC carrier
        if (rb==(frame_parms->N_RB_DL>>1)) {
          rxF       = &rxdataF[aarx][(1 + (symbol*(frame_parms->ofdm_symbol_size)))];
          //dl_ch0++;
        }

        // PBCH
        if ((subframe==0) && (rb>=((frame_parms->N_RB_DL>>1)-3)) && (rb<((frame_parms->N_RB_DL>>1)+3)) && (l>=nsymb>>1) && (l<((nsymb>>1) + 4))) {
          rb_alloc_ind = 0;
        }

        //SSS
        if (((subframe==0)||(subframe==5)) && (rb>=((frame_parms->N_RB_DL>>1)-3)) && (rb<((frame_parms->N_RB_DL>>1)+3)) && (l==sss_symb) ) {
          rb_alloc_ind = 0;
        }

        if (frame_parms->frame_type == FDD) {
          //PSS
          if (((subframe==0)||(subframe==5)) && (rb>=((frame_parms->N_RB_DL>>1)-3)) && (rb<((frame_parms->N_RB_DL>>1)+3)) && (l==pss_symb) ) {
            rb_alloc_ind = 0;
          }
        }

        if ((frame_parms->frame_type == TDD) &&
            (subframe==6)) { //TDD Subframe 6
          if ((rb>=((frame_parms->N_RB_DL>>1)-3)) && (rb<((frame_parms->N_RB_DL>>1)+3)) && (l==pss_symb) ) {
            rb_alloc_ind = 0;
          }
        }

        if (rb_alloc_ind==1) {
          /*
              printf("rb %d\n",rb);
              for (i=0;i<12;i++)
              printf("(%d %d)",((short *)dl_ch0)[i<<1],((short*)dl_ch0)[1+(i<<1)]);
              printf("\n");
          */
          if (pilots==0 && uespec_pilots==0) {
            memcpy(dl_ch0_ext,dl_ch0,12*sizeof(int));

            for (i=0; i<12; i++) {
              rxF_ext[i]=rxF[i];
            }

            dl_ch0_ext+=12;
            rxF_ext+=12;
          } else if(pilots==1 && uespec_pilots==0) {
            j=0;

            for (i=0; i<12; i++) {
              if ((i!=(frame_parms->nushift+poffset)) &&
                  (i!=((frame_parms->nushift+poffset+6)%12))) {
                rxF_ext[j]=rxF[i];
                dl_ch0_ext[j++]=dl_ch0[i];
              }
            }

            dl_ch0_ext+=10;
            rxF_ext+=10;
          } else if (pilots==0 && uespec_pilots==1) {
            j=0;

            for (i=0; i<12; i++) {
              if (frame_parms->Ncp==0) {
                if (i!=uespec_nushift+uespec_poffset && i!=uespec_nushift+uespec_poffset+4 && i!=(uespec_nushift+uespec_poffset+8)%12) {
                  rxF_ext[j] = rxF[i];
                  dl_ch0_ext[j++]=dl_ch0[i];
                }
              } else {
                if (i!=uespec_nushift+uespec_poffset && i!=uespec_nushift+uespec_poffset+3 && i!=uespec_nushift+uespec_poffset+6 && i!=(uespec_nushift+uespec_poffset+9)%12) {
                  rxF_ext[j] = rxF[i];
                  dl_ch0_ext[j++]=dl_ch0[i];
                }
              }
            }

            dl_ch0_ext+=9-frame_parms->Ncp;
            rxF_ext+=9-frame_parms->Ncp;
          } else {
            LOG_E(PHY,"dlsch_extract_rbs_TM7(dl_demodulation.c):pilot or ue spec pilot detection error\n");
            exit(-1);
          }
        }

        dl_ch0+=12;
        rxF+=12;
      } else { // Odd number of RBs
      for (rb=0; rb<frame_parms->N_RB_DL>>1; rb++) {
        skip_half=0;

        if (rb < 32)
          rb_alloc_ind = (rb_alloc[0]>>rb) & 1;
        else if (rb < 64)
          rb_alloc_ind = (rb_alloc[1]>>(rb-32)) & 1;
        else if (rb < 96)
          rb_alloc_ind = (rb_alloc[2]>>(rb-64)) & 1;
        else if (rb < 100)
          rb_alloc_ind = (rb_alloc[3]>>(rb-96)) & 1;
        else
          rb_alloc_ind = 0;

        if (rb_alloc_ind == 1)
          nb_rb++;

        // PBCH
        if ((subframe==0) && (rb>((frame_parms->N_RB_DL>>1)-3)) && (rb<((frame_parms->N_RB_DL>>1)+3)) && (l>=(nsymb>>1)) && (l<((nsymb>>1) + 4))) {
          rb_alloc_ind = 0;
        }

        //PBCH subframe 0, symbols nsymb>>1 ... nsymb>>1 + 3
        if ((subframe==0) && (rb==((frame_parms->N_RB_DL>>1)-3)) && (l>=(nsymb>>1)) && (l<((nsymb>>1) + 4)))
          skip_half=1;
        else if ((subframe==0) && (rb==((frame_parms->N_RB_DL>>1)+3)) && (l>=(nsymb>>1)) && (l<((nsymb>>1) + 4)))
          skip_half=2;

        //SSS

        if (((subframe==0)||(subframe==5)) &&
            (rb>((frame_parms->N_RB_DL>>1)-3)) &&
            (rb<((frame_parms->N_RB_DL>>1)+3)) &&
            (l==sss_symb) ) {
          rb_alloc_ind = 0;
        }

        //SSS
        if (((subframe==0)||(subframe==5)) &&
            (rb==((frame_parms->N_RB_DL>>1)-3)) &&
            (l==sss_symb))
          skip_half=1;
        else if (((subframe==0)||(subframe==5)) &&
                 (rb==((frame_parms->N_RB_DL>>1)+3)) &&
                 (l==sss_symb))
          skip_half=2;

        //PSS in subframe 0/5 if FDD
        if (frame_parms->frame_type == FDD) {  //FDD
          if (((subframe==0)||(subframe==5)) && (rb>((frame_parms->N_RB_DL>>1)-3)) && (rb<((frame_parms->N_RB_DL>>1)+3)) && (l==pss_symb) ) {
            rb_alloc_ind = 0;
          }

          if (((subframe==0)||(subframe==5)) && (rb==((frame_parms->N_RB_DL>>1)-3)) && (l==pss_symb))
            skip_half=1;
          else if (((subframe==0)||(subframe==5)) && (rb==((frame_parms->N_RB_DL>>1)+3)) && (l==pss_symb))
            skip_half=2;
        }

        if ((frame_parms->frame_type == TDD) && ((subframe==1)||(subframe==6))) { //TDD Subframe 1 and 6
          if ((rb>((frame_parms->N_RB_DL>>1)-3)) && (rb<((frame_parms->N_RB_DL>>1)+3)) && (l==pss_symb) ) {
            rb_alloc_ind = 0;
          }

          if ((rb==((frame_parms->N_RB_DL>>1)-3)) && (l==pss_symb))
            skip_half=1;
          else if ((rb==((frame_parms->N_RB_DL>>1)+3)) && (l==pss_symb))
            skip_half=2;
        }

        if (rb_alloc_ind==1) {
#ifdef DEBUG_DLSCH_DEMOD
          printf("rb %d/symbol %d pilots %d, uespec_pilots %d, (skip_half %d)\n",rb,l,pilots,uespec_pilots,skip_half);
#endif

          if (pilots==0 && uespec_pilots==0) {
            //printf("Extracting w/o pilots (symbol %d, rb %d, skip_half %d)\n",l,rb,skip_half);
            if (skip_half==1) {
              memcpy(dl_ch0_ext,dl_ch0,6*sizeof(int));

              for (i=0; i<6; i++) {
                rxF_ext[i]=rxF[i];
#ifdef DEBUG_DLSCH_DEMOD
                printf("extract rb %d, re %d => (%d,%d)\n",rb,i,*(short *)&rxF_ext[i],*(1+(short *)&rxF_ext[i]));
#endif
              }

              dl_ch0_ext+=6;
              rxF_ext+=6;
            } else if (skip_half==2) {
              memcpy(dl_ch0_ext,dl_ch0+6,6*sizeof(int));

              for (i=0; i<6; i++) {
                rxF_ext[i]=rxF[(i+6)];
#ifdef DEBUG_DLSCH_DEMOD
                printf("extract rb %d, re %d => (%d,%d)\n",rb,i,*(short *)&rxF_ext[i],*(1+(short *)&rxF_ext[i]));
#endif
              }

              dl_ch0_ext+=6;
              rxF_ext+=6;
            } else {
              memcpy(dl_ch0_ext,dl_ch0,12*sizeof(int));

              for (i=0; i<12; i++) {
                rxF_ext[i]=rxF[i];
#ifdef DEBUG_DLSCH_DEMOD
                printf("extract symbol %d rb %d, re %d => (%d,%d)\n",symbol,rb,i,*(short *)&rxF[i],*(1+(short *)&rxF[i]));
#endif
              }

              dl_ch0_ext+=12;
              rxF_ext+=12;
            }
          } else if (pilots==1 && uespec_pilots==0) {
            // printf("Extracting with pilots (symbol %d, rb %d, skip_half %d)\n",l,rb,skip_half);
            j=0;

            if (skip_half==1) {
              for (i=0; i<6; i++) {
                if (i!=((frame_parms->nushift+poffset)%6)) {
                  rxF_ext[j]=rxF[i];
                  dl_ch0_ext[j++]=dl_ch0[i];
#ifdef DEBUG_DLSCH_DEMOD
                  printf("extract rb %d, re %d => (%d,%d)\n",rb,i,*(short *)&rxF_ext[i],*(1+(short *)&rxF_ext[i]));
#endif
                }
              }

              dl_ch0_ext+=5;
              rxF_ext+=5;
            } else if (skip_half==2) {
              for (i=0; i<6; i++) {
                if (i!=((frame_parms->nushift+poffset)%6)) {
                  rxF_ext[j]=rxF[(i+6)];
                  dl_ch0_ext[j++]=dl_ch0[i+6];
#ifdef DEBUG_DLSCH_DEMOD
                  printf("extract rb %d, re %d => (%d,%d)\n",rb,i,*(short *)&rxF_ext[i],*(1+(short *)&rxF_ext[i]));
#endif
                }
              }

              dl_ch0_ext+=5;
              rxF_ext+=5;
            } else {
              for (i=0; i<12; i++) {
                if ((i!=(frame_parms->nushift+poffset)) &&
                    (i!=((frame_parms->nushift+poffset+6)%12))) {
                  rxF_ext[j]=rxF[i];
#ifdef DEBUG_DLSCH_DEMOD
                  printf("extract rb %d, re %d => (%d,%d)\n",rb,i,*(short *)&rxF_ext[j],*(1+(short *)&rxF_ext[j]));
#endif
                  dl_ch0_ext[j++]=dl_ch0[i];
                }
              }

              dl_ch0_ext+=10;
              rxF_ext+=10;
            }
          } else if(pilots==0 && uespec_pilots==1) {
            //printf("Extracting with uespec pilots (symbol %d, rb %d, skip_half %d)\n",l,rb,skip_half);
            j=0;

            if (skip_half==1) {
              if (frame_parms->Ncp==0) {
                for (i=0; i<6; i++) {
                  if (i!=uespec_nushift+uespec_poffset && i!=uespec_nushift+uespec_poffset+4 && i!=(uespec_nushift+uespec_poffset+8)%12) {
                    rxF_ext[j]=rxF[i];
                    dl_ch0_ext[j++]=dl_ch0[i];
#ifdef DEBUG_DLSCH_DEMOD
                    printf("extract rb %d, re %d => (%d,%d)\n",rb,i,*(short *)&rxF_ext[i],*(1+(short *)&rxF_ext[i]));
#endif
                  }
                }

                dl_ch0_ext+=6-(uespec_nushift+uespec_poffset<6)-(uespec_nushift+uespec_poffset+4<6)-((uespec_nushift+uespec_poffset+8)%12<6);
                rxF_ext+=6-(uespec_nushift+uespec_poffset<6)-(uespec_nushift+uespec_poffset+4<6)-((uespec_nushift+uespec_poffset+8)%12<6);
              } else {
                for (i=0; i<6; i++) {
                  if (i!=uespec_nushift+uespec_poffset && i!=uespec_nushift+uespec_poffset+3 && i!=uespec_nushift+uespec_poffset+6 && i!=(uespec_nushift+uespec_poffset+9)%12) {
                    rxF_ext[j]=rxF[i];
                    dl_ch0_ext[j++]=dl_ch0[i];
#ifdef DEBUG_DLSCH_DEMOD
                    printf("extract rb %d, re %d => (%d,%d)\n",rb,i,*(short *)&rxF_ext[i],*(1+(short *)&rxF_ext[i]));
#endif
                  }
                }

                dl_ch0_ext+=4;
                rxF_ext+=4;
              }
            } else if (skip_half==2) {
              if(frame_parms->Ncp==0) {
                for (i=0; i<6; i++) {
                  if (i!=uespec_nushift+uespec_poffset && i!=uespec_nushift+uespec_poffset+4 && i!=(uespec_nushift+uespec_poffset+8)%12) {
                    rxF_ext[j]=rxF[(i+6)];
                    dl_ch0_ext[j++]=dl_ch0[i+6];
#ifdef DEBUG_DLSCH_DEMOD
                    printf("extract rb %d, re %d => (%d,%d)\n",rb,i,*(short *)&rxF_ext[i],*(1+(short *)&rxF_ext[i]));
#endif
                  }
                }

                dl_ch0_ext+=6-(uespec_nushift+uespec_poffset>6)-(uespec_nushift+uespec_poffset+4>6)-((uespec_nushift+uespec_poffset+8)%12>6);
                rxF_ext+=6-(uespec_nushift+uespec_poffset>6)-(uespec_nushift+uespec_poffset+4>6)-((uespec_nushift+uespec_poffset+8)%12>6);
              } else {
                for (i=0; i<6; i++) {
                  if (i!=uespec_nushift+uespec_poffset && i!=uespec_nushift+uespec_poffset+3 && i!=uespec_nushift+uespec_poffset+6 && i!=(uespec_nushift+uespec_poffset+9)%12) {
                    rxF_ext[j]=rxF[(i+6)];
                    dl_ch0_ext[j++]=dl_ch0[i+6];
#ifdef DEBUG_DLSCH_DEMOD
                    printf("extract rb %d, re %d => (%d,%d)\n",rb,i,*(short *)&rxF_ext[i],*(1+(short *)&rxF_ext[i]));
#endif
                  }
                }

                dl_ch0_ext+=4;
                rxF_ext+=4;
              }
            } else {
              for (i=0; i<12; i++) {
                if (frame_parms->Ncp==0) {
                  if (i!=uespec_nushift+uespec_poffset && i!=uespec_nushift+uespec_poffset+4 && i!=(uespec_nushift+uespec_poffset+8)%12) {
                    rxF_ext[j] = rxF[i];
                    dl_ch0_ext[j++] = dl_ch0[i];
#ifdef DEBUG_DLSCH_DEMOD
                    printf("extract symbol %d, rb %d, re %d, j %d => (%d,%d)\n",
                           symbol,rb,i,j-1,*(short *)&dl_ch0[j],*(1+(short *)&dl_ch0[i]));
#endif
                  }
                } else {
                  if (i!=uespec_nushift+uespec_poffset && i!=uespec_nushift+uespec_poffset+3 && i!=uespec_nushift+uespec_poffset+6 && i!=(uespec_nushift+uespec_poffset+9)%12) {
                    rxF_ext[j] = rxF[i];
                    dl_ch0_ext[j++]=dl_ch0[i];
#ifdef DEBUG_DLSCH_DEMOD
                    printf("extract rb %d, re %d => (%d,%d)\n",rb,i,*(short *)&rxF_ext[i],*(1+(short *)&rxF_ext[i]));
#endif
                  }
                }
              }

              dl_ch0_ext+=9-frame_parms->Ncp;
              rxF_ext+=9-frame_parms->Ncp;
            }
          } else {
            LOG_E(PHY,"dlsch_extract_rbs_TM7(dl_demodulation.c):pilot or ue spec pilot detection error\n");
            exit(-1);
          }
        }

        dl_ch0+=12;
        rxF+=12;
      } // first half loop

      // Do middle RB (around DC)
      if (rb < 32)
        rb_alloc_ind = (rb_alloc[0]>>rb) & 1;
      else if (rb < 64)
        rb_alloc_ind = (rb_alloc[1]>>(rb-32)) & 1;
      else if (rb < 96)
        rb_alloc_ind = (rb_alloc[2]>>(rb-64)) & 1;
      else if (rb < 100)
        rb_alloc_ind = (rb_alloc[3]>>(rb-96)) & 1;
      else
        rb_alloc_ind = 0;

      if (rb_alloc_ind == 1)
        nb_rb++;

      // PBCH
      if ((subframe==0) && (rb>=((frame_parms->N_RB_DL>>1)-3)) && (rb<((frame_parms->N_RB_DL>>1)+3)) && (l>=(nsymb>>1)) && (l<((nsymb>>1) + 4))) {
        rb_alloc_ind = 0;
      }

      //SSS
      if (((subframe==0)||(subframe==5)) && (rb>=((frame_parms->N_RB_DL>>1)-3)) && (rb<((frame_parms->N_RB_DL>>1)+3)) && (l==sss_symb) ) {
        rb_alloc_ind = 0;
      }

      if (frame_parms->frame_type == FDD) {
        //PSS
        if (((subframe==0)||(subframe==5)) && (rb>=((frame_parms->N_RB_DL>>1)-3)) && (rb<((frame_parms->N_RB_DL>>1)+3)) && (l==pss_symb) ) {
          rb_alloc_ind = 0;
        }
      }

      if ((frame_parms->frame_type == TDD) && ((subframe==1)||(subframe==6))) {
        //PSS
        if ((rb>((frame_parms->N_RB_DL>>1)-3)) && (rb<((frame_parms->N_RB_DL>>1)+3)) && (l==pss_symb) ) {
          rb_alloc_ind = 0;
        }
      }

      //printf("dlch_ext %d\n",dl_ch0_ext-&dl_ch_estimates_ext[aarx][0]);
      //printf("DC rb %d (%p)\n",rb,rxF);
      if (rb_alloc_ind==1) {
        //printf("rb %d/symbol %d (skip_half %d)\n",rb,l,skip_half);
        if (pilots==0 && uespec_pilots==0) {
          for (i=0; i<6; i++) {
            dl_ch0_ext[i]=dl_ch0[i];
            rxF_ext[i]=rxF[i];
#ifdef DEBUG_DLSCH_DEMOD
            printf("extract rb %d, re %d => (%d,%d)\n",rb,i,*(short *)&rxF_ext[i],*(1+(short *)&rxF_ext[i]));
#endif
          }

          rxF       = &rxdataF[aarx][((symbol*(frame_parms->ofdm_symbol_size)))];

          for (; i<12; i++) {
            dl_ch0_ext[i]=dl_ch0[i];
            rxF_ext[i]=rxF[(1+i-6)];
#ifdef DEBUG_DLSCH_DEMOD
            printf("extract rb %d, re %d => (%d,%d)\n",rb,i,*(short *)&rxF_ext[i],*(1+(short *)&rxF_ext[i]));
#endif
          }

          dl_ch0_ext+=12;
          rxF_ext+=12;
        } else if(pilots==1 && uespec_pilots==0) { // pilots==1
          j=0;

          for (i=0; i<6; i++) {
            if (i!=((frame_parms->nushift+poffset)%6)) {
              dl_ch0_ext[j]=dl_ch0[i];
              rxF_ext[j++]=rxF[i];
#ifdef DEBUG_DLSCH_DEMOD
              printf("extract rb %d, re %d => (%d,%d)\n",rb,i,*(short *)&rxF_ext[i],*(1+(short *)&rxF_ext[i]));
#endif
            }
          }

          rxF       = &rxdataF[aarx][((symbol*(frame_parms->ofdm_symbol_size)))];

          for (; i<12; i++) {
            if (i!=((frame_parms->nushift+6+poffset)%12)) {
              dl_ch0_ext[j]=dl_ch0[i];
              rxF_ext[j++]=rxF[(1+i-6)];
#ifdef DEBUG_DLSCH_DEMOD
              printf("extract rb %d, re %d => (%d,%d)\n",rb,i,*(short *)&rxF_ext[i],*(1+(short *)&rxF_ext[i]));
#endif
            }
          }

          dl_ch0_ext+=10;
          rxF_ext+=10;
        } else if(pilots==0 && uespec_pilots==1) {
          j=0;

          for (i=0; i<6; i++) {
            if (frame_parms->Ncp==0) {
              if (i!=uespec_nushift+uespec_poffset && i!=uespec_nushift+uespec_poffset+4 && i!=(uespec_nushift+uespec_poffset+8)%12) {
                dl_ch0_ext[j]=dl_ch0[i];
                rxF_ext[j++] = rxF[i];
#ifdef DEBUG_DLSCH_DEMOD
                printf("extract rb %d, re %d => (%d,%d)\n",rb,i,*(short *)&rxF_ext[i],*(1+(short *)&rxF_ext[i]));
#endif
              }
            } else {
              if (i!=uespec_nushift+uespec_poffset && i!=uespec_nushift+uespec_poffset+3 && i!=uespec_nushift+uespec_poffset+6 && i!=(uespec_nushift+uespec_poffset+9)%12) {
                dl_ch0_ext[j]=dl_ch0[i];
                rxF_ext[j++] = rxF[i];
#ifdef DEBUG_DLSCH_DEMOD
                printf("extract rb %d, re %d => (%d,%d)\n",rb,i,*(short *)&rxF_ext[i],*(1+(short *)&rxF_ext[i]));
#endif
              }
            }
          }

          rxF       = &rxdataF[aarx][((symbol*(frame_parms->ofdm_symbol_size)))];

          for (; i<12; i++) {
            if (frame_parms->Ncp==0) {
              if (i!=uespec_nushift+uespec_poffset && i!=uespec_nushift+uespec_poffset+4 && i!=(uespec_nushift+uespec_poffset+8)%12) {
                dl_ch0_ext[j]=dl_ch0[i];
                rxF_ext[j++]=rxF[(1+i-6)];
#ifdef DEBUG_DLSCH_DEMOD
                printf("extract rb %d, re %d => (%d,%d)\n",rb,i,*(short *)&rxF_ext[i],*(1+(short *)&rxF_ext[i]));
#endif
              }
            } else {
              if (i!=uespec_nushift+uespec_poffset && i!=uespec_nushift+uespec_poffset+3 && i!=uespec_nushift+uespec_poffset+6 && i!=(uespec_nushift+uespec_poffset+9)%12) {
                dl_ch0_ext[j]=dl_ch0[i];
                rxF_ext[j++] = rxF[(1+i-6)];
#ifdef DEBUG_DLSCH_DEMOD
                printf("extract rb %d, re %d => (%d,%d)\n",rb,i,*(short *)&rxF_ext[i],*(1+(short *)&rxF_ext[i]));
#endif
              }
            }
          }

          dl_ch0_ext+=9-frame_parms->Ncp;
          rxF_ext+=9-frame_parms->Ncp;
        }// symbol_mod==0
      } // rballoc==1
      else {
        rxF       = &rxdataF[aarx][((symbol*(frame_parms->ofdm_symbol_size)))];
      }

      dl_ch0+=12;
      rxF+=7;
      rb++;

      for (; rb<frame_parms->N_RB_DL; rb++) {
        //  printf("dlch_ext %d\n",dl_ch0_ext-&dl_ch_estimates_ext[aarx][0]);
        //  printf("rb %d (%p)\n",rb,rxF);
        skip_half=0;

        if (rb < 32)
          rb_alloc_ind = (rb_alloc[0]>>rb) & 1;
        else if (rb < 64)
          rb_alloc_ind = (rb_alloc[1]>>(rb-32)) & 1;
        else if (rb < 96)
          rb_alloc_ind = (rb_alloc[2]>>(rb-64)) & 1;
        else if (rb < 100)
          rb_alloc_ind = (rb_alloc[3]>>(rb-96)) & 1;
        else
          rb_alloc_ind = 0;

        if (rb_alloc_ind==1)
          nb_rb++;

        // PBCH
        if ((subframe==0) && (rb>((frame_parms->N_RB_DL>>1)-3)) && (rb<((frame_parms->N_RB_DL>>1)+3)) && (l>=nsymb>>1) && (l<((nsymb>>1) + 4))) {
          rb_alloc_ind = 0;
        }

        //PBCH subframe 0, symbols nsymb>>1 ... nsymb>>1 + 3
        if ((subframe==0) && (rb==((frame_parms->N_RB_DL>>1)-3)) && (l>=(nsymb>>1)) && (l<((nsymb>>1) + 4)))
          skip_half=1;
        else if ((subframe==0) && (rb==((frame_parms->N_RB_DL>>1)+3)) && (l>=(nsymb>>1)) && (l<((nsymb>>1) + 4)))
          skip_half=2;

        //SSS
        if (((subframe==0)||(subframe==5)) && (rb>((frame_parms->N_RB_DL>>1)-3)) && (rb<((frame_parms->N_RB_DL>>1)+3)) && (l==sss_symb) ) {
          rb_alloc_ind = 0;
        }

        //SSS
        if (((subframe==0)||(subframe==5)) && (rb==((frame_parms->N_RB_DL>>1)-3)) && (l==sss_symb))
          skip_half=1;
        else if (((subframe==0)||(subframe==5)) && (rb==((frame_parms->N_RB_DL>>1)+3)) && (l==sss_symb))
          skip_half=2;

        //PSS
        if (frame_parms->frame_type == FDD) {
          if (((subframe==0)||(subframe==5)) && (rb>((frame_parms->N_RB_DL>>1)-3)) && (rb<((frame_parms->N_RB_DL>>1)+3)) && (l==pss_symb) ) {
            rb_alloc_ind = 0;
          }

          if (((subframe==0)||(subframe==5)) && (rb==((frame_parms->N_RB_DL>>1)-3)) && (l==pss_symb))
            skip_half=1;
          else if (((subframe==0)||(subframe==5)) && (rb==((frame_parms->N_RB_DL>>1)+3)) && (l==pss_symb))
            skip_half=2;
        }

        if ((frame_parms->frame_type == TDD) && ((subframe==1)||(subframe==6))) { //TDD Subframe 1 and 6
          if ((rb>((frame_parms->N_RB_DL>>1)-3)) && (rb<((frame_parms->N_RB_DL>>1)+3)) && (l==pss_symb) ) {
            rb_alloc_ind = 0;
          }

          if ((rb==((frame_parms->N_RB_DL>>1)-3)) && (l==pss_symb))
            skip_half=1;
          else if ((rb==((frame_parms->N_RB_DL>>1)+3)) && (l==pss_symb))
            skip_half=2;
        }

        if (rb_alloc_ind==1) {
#ifdef DEBUG_DLSCH_DEMOD
          printf("rb %d/symbol %d (skip_half %d)\n",rb,l,skip_half);
#endif

          /*
              printf("rb %d\n",rb);
            for (i=0;i<12;i++)
            printf("(%d %d)",((short *)dl_ch0)[i<<1],((short*)dl_ch0)[1+(i<<1)]);
            printf("\n");
          */
          if (pilots==0 && uespec_pilots==0) {
            //printf("Extracting w/o pilots (symbol %d, rb %d, skip_half %d)\n",l,rb,skip_half);
            if (skip_half==1) {
              memcpy(dl_ch0_ext,dl_ch0,6*sizeof(int));

              for (i=0; i<6; i++) {
                rxF_ext[i]=rxF[i];
#ifdef DEBUG_DLSCH_DEMOD
                printf("extract rb %d, re %d => (%d,%d)\n",rb,i,*(short *)&rxF_ext[i],*(1+(short *)&rxF_ext[i]));
#endif
              }

              dl_ch0_ext+=6;
              rxF_ext+=6;
            } else if (skip_half==2) {
              memcpy(dl_ch0_ext,dl_ch0+6,6*sizeof(int));

              for (i=0; i<6; i++) {
                rxF_ext[i]=rxF[i+6];
#ifdef DEBUG_DLSCH_DEMOD
                printf("extract rb %d, re %d => (%d,%d)\n",rb,i,*(short *)&rxF_ext[i],*(1+(short *)&rxF_ext[i]));
#endif
              }

              dl_ch0_ext+=6;
              rxF_ext+=6;
            } else {
              memcpy(dl_ch0_ext,dl_ch0,12*sizeof(int));
              //printf("symbol %d, extract rb %d, => (%d,%d)\n",symbol,rb,*(short *)&dl_ch0[j],*(1+(short*)&dl_ch0[i]));

              for (i=0; i<12; i++) {
                rxF_ext[i]=rxF[i];
#ifdef DEBUG_DLSCH_DEMOD
                printf("extract rb %d, re %d => (%d,%d)\n",rb,i,*(short *)&rxF_ext[i],*(1+(short *)&rxF_ext[i]));
#endif
              }

              dl_ch0_ext+=12;
              rxF_ext+=12;
            }
          } else if (pilots==1 && uespec_pilots==0) {
            //printf("Extracting with pilots (symbol %d, rb %d, skip_half %d)\n",l,rb,skip_half);
            j=0;

            if (skip_half==1) {
              for (i=0; i<6; i++) {
                if (i!=((frame_parms->nushift+poffset)%6)) {
                  rxF_ext[j]=rxF[i];
                  dl_ch0_ext[j++]=dl_ch0[i];
#ifdef DEBUG_DLSCH_DEMOD
                  printf("extract rb %d, re %d => (%d,%d)\n",rb,i,*(short *)&rxF_ext[i],*(1+(short *)&rxF_ext[i]));
#endif
                }
              }

              dl_ch0_ext+=5;
              rxF_ext+=5;
            } else if (skip_half==2) {
              for (i=0; i<6; i++) {
                if (i!=((frame_parms->nushift+poffset)%6)) {
                  rxF_ext[j]=rxF[(i+6)];
                  dl_ch0_ext[j++]=dl_ch0[i+6];
#ifdef DEBUG_DLSCH_DEMOD
                  printf("extract rb %d, re %d => (%d,%d)\n",rb,i,*(short *)&rxF_ext[i],*(1+(short *)&rxF_ext[i]));
#endif
                }
              }

              dl_ch0_ext+=5;
              rxF_ext+=5;
            } else {
              for (i=0; i<12; i++) {
                if ((i!=(frame_parms->nushift+poffset)) &&
                    (i!=((frame_parms->nushift+poffset+6)%12))) {
                  rxF_ext[j]=rxF[i];
#ifdef DEBUG_DLSCH_DEMOD
                  printf("extract rb %d, re %d => (%d,%d)\n",rb,i,*(short *)&rxF_ext[j],*(1+(short *)&rxF_ext[j]));
#endif
                  dl_ch0_ext[j++]=dl_ch0[i];
                }
              }

              dl_ch0_ext+=10;
              rxF_ext+=10;
            }
          } else if(pilots==0 && uespec_pilots==1) {
            j=0;

            if (skip_half==1) {
              if (frame_parms->Ncp==0) {
                for (i=0; i<6; i++) {
                  if (i!=uespec_nushift+uespec_poffset && i!=uespec_nushift+uespec_poffset+4 && i!=(uespec_nushift+uespec_poffset+8)%12) {
                    rxF_ext[j]=rxF[i];
                    dl_ch0_ext[j++]=dl_ch0[i];
#ifdef DEBUG_DLSCH_DEMOD
                    printf("extract rb %d, re %d => (%d,%d)\n",rb,i,*(short *)&rxF_ext[i],*(1+(short *)&rxF_ext[i]));
#endif
                  }
                }

                dl_ch0_ext+=6-(uespec_nushift+uespec_poffset<6)-(uespec_nushift+uespec_poffset+4<6)-((uespec_nushift+uespec_poffset+8)%12<6);
                rxF_ext+=6-(uespec_nushift+uespec_poffset<6)-(uespec_nushift+uespec_poffset+4<6)-((uespec_nushift+uespec_poffset+8)%12<6);
              } else {
                for (i=0; i<6; i++) {
                  if (i!=uespec_nushift+uespec_poffset && i!=uespec_nushift+uespec_poffset+3 && i!=uespec_nushift+uespec_poffset+6 && i!=(uespec_nushift+uespec_poffset+9)%12) {
                    rxF_ext[j]=rxF[i];
                    dl_ch0_ext[j++]=dl_ch0[i];
#ifdef DEBUG_DLSCH_DEMOD
                    printf("extract rb %d, re %d => (%d,%d)\n",rb,i,*(short *)&rxF_ext[i],*(1+(short *)&rxF_ext[i]));
#endif
                  }
                }

                dl_ch0_ext+=4;
                rxF_ext+=4;
              }
            } else if (skip_half==2) {
              if(frame_parms->Ncp==0) {
                for (i=0; i<6; i++) {
                  if (i!=uespec_nushift+uespec_poffset && i!=uespec_nushift+uespec_poffset+4 && i!=(uespec_nushift+uespec_poffset+8)%12) {
                    rxF_ext[j]=rxF[i+6];
                    dl_ch0_ext[j++]=dl_ch0[i+6];
#ifdef DEBUG_DLSCH_DEMOD
                    printf("extract rb %d, re %d => (%d,%d)\n",rb,i,*(short *)&rxF_ext[i],*(1+(short *)&rxF_ext[i]));
#endif
                  }
                }

                dl_ch0_ext+=6-(uespec_nushift+uespec_poffset>6)-(uespec_nushift+uespec_poffset+4>6)-((uespec_nushift+uespec_poffset+8)%12>6);
                rxF_ext+=6-(uespec_nushift+uespec_poffset>6)-(uespec_nushift+uespec_poffset+4>6)-((uespec_nushift+uespec_poffset+8)%12>6);
              } else {
                for (i=0; i<6; i++) {
                  if (i!=uespec_nushift+uespec_poffset && i!=uespec_nushift+uespec_poffset+3 && i!=uespec_nushift+uespec_poffset+6 && i!=(uespec_nushift+uespec_poffset+9)%12) {
                    rxF_ext[j]=rxF[(i+6)];
                    dl_ch0_ext[j++]=dl_ch0[i+6];
#ifdef DEBUG_DLSCH_DEMOD
                    printf("extract rb %d, re %d => (%d,%d)\n",rb,i,*(short *)&rxF_ext[i],*(1+(short *)&rxF_ext[i]));
#endif
                  }
                }

                dl_ch0_ext+=4;
                rxF_ext+=4;
              }
            } else {
              for (i=0; i<12; i++) {
                if (frame_parms->Ncp==0) {
                  if (i!=uespec_nushift+uespec_poffset && i!=uespec_nushift+uespec_poffset+4 && i!=(uespec_nushift+uespec_poffset+8)%12) {
                    rxF_ext[j] = rxF[i];
                    dl_ch0_ext[j++]=dl_ch0[i];
#ifdef DEBUG_DLSCH_DEMOD
                    printf("extract rb %d, re %d => (%d,%d)\n",rb,i,*(short *)&rxF_ext[i],*(1+(short *)&rxF_ext[i]));
#endif
                  }
                } else {
                  if (i!=uespec_nushift+uespec_poffset && i!=uespec_nushift+uespec_poffset+3 && i!=uespec_nushift+uespec_poffset+6 && i!=(uespec_nushift+uespec_poffset+9)%12) {
                    rxF_ext[j] = rxF[i];
                    dl_ch0_ext[j++]=dl_ch0[i];
#ifdef DEBUG_DLSCH_DEMOD
                    printf("extract rb %d, re %d => (%d,%d)\n",rb,i,*(short *)&rxF_ext[i],*(1+(short *)&rxF_ext[i]));
#endif
                  }
                }
              }

              dl_ch0_ext+=9-frame_parms->Ncp;
              rxF_ext+=9-frame_parms->Ncp;
            }
          }// pilots=0
        }

        dl_ch0+=12;
        rxF+=12;
      }
    }
  }
  
  simde_mm_empty();
  simde_m_empty();
  return(nb_rb/frame_parms->nb_antennas_rx);
}

//==============================================================================================

void dump_dlsch2(PHY_VARS_UE *ue,uint8_t eNB_id,uint8_t subframe,unsigned int *coded_bits_per_codeword,int round,  unsigned char harq_pid) {
#define NSYMB  ((ue->frame_parms.Ncp == 0) ? 14 : 12)
  char fname[32],vname[32];
  sprintf(fname,"dlsch%d_rxF_r%d_ext0.m",eNB_id,round);
  sprintf(vname,"dl%d_rxF_r%d_ext0",eNB_id,round);
  LOG_M(fname,vname,ue->pdsch_vars[ue->current_thread_id[subframe]][eNB_id]->rxdataF_ext[0],
        12*(ue->frame_parms.N_RB_DL)*NSYMB,1,1);

  if (ue->frame_parms.nb_antennas_rx >1) {
    sprintf(fname,"dlsch%d_rxF_r%d_ext1.m",eNB_id,round);
    sprintf(vname,"dl%d_rxF_r%d_ext1",eNB_id,round);
    LOG_M(fname,vname,ue->pdsch_vars[ue->current_thread_id[subframe]][eNB_id]->rxdataF_ext[1],
          12*(ue->frame_parms.N_RB_DL)*NSYMB,1,1);
  }

  sprintf(fname,"dlsch%d_ch_r%d_ext00.m",eNB_id,round);
  sprintf(vname,"dl%d_ch_r%d_ext00",eNB_id,round);
  LOG_M(fname,vname,ue->pdsch_vars[ue->current_thread_id[subframe]][eNB_id]->dl_ch_estimates_ext[0],
        12*(ue->frame_parms.N_RB_DL)*NSYMB,1,1);

  if (ue->transmission_mode[eNB_id]==7) {
    sprintf(fname,"dlsch%d_bf_ch_r%d.m",eNB_id,round);
    sprintf(vname,"dl%d_bf_ch_r%d",eNB_id,round);
    LOG_M(fname,vname,ue->pdsch_vars[ue->current_thread_id[subframe]][eNB_id]->dl_bf_ch_estimates[0],512*NSYMB,1,1);
    //LOG_M(fname,vname,phy_vars_ue->lte_ue_pdsch_vars[eNB_id]->dl_bf_ch_estimates[0],512,1,1);
    sprintf(fname,"dlsch%d_bf_ch_r%d_ext00.m",eNB_id,round);
    sprintf(vname,"dl%d_bf_ch_r%d_ext00",eNB_id,round);
    LOG_M(fname,vname,ue->pdsch_vars[ue->current_thread_id[subframe]][eNB_id]->dl_bf_ch_estimates_ext[0],
          12*(ue->frame_parms.N_RB_DL)*NSYMB,1,1);
  }

  if (ue->frame_parms.nb_antennas_rx == 2) {
    sprintf(fname,"dlsch%d_ch_r%d_ext01.m",eNB_id,round);
    sprintf(vname,"dl%d_ch_r%d_ext01",eNB_id,round);
    LOG_M(fname,vname,ue->pdsch_vars[ue->current_thread_id[subframe]][eNB_id]->dl_ch_estimates_ext[1],
          12*(ue->frame_parms.N_RB_DL)*NSYMB,1,1);
  }

  if (ue->frame_parms.nb_antenna_ports_eNB == 2) {
    sprintf(fname,"dlsch%d_ch_r%d_ext10.m",eNB_id,round);
    sprintf(vname,"dl%d_ch_r%d_ext10",eNB_id,round);
    LOG_M(fname,vname,ue->pdsch_vars[ue->current_thread_id[subframe]][eNB_id]->dl_ch_estimates_ext[2],
          12*(ue->frame_parms.N_RB_DL)*NSYMB,1,1);

    if (ue->frame_parms.nb_antennas_rx == 2) {
      sprintf(fname,"dlsch%d_ch_r%d_ext11.m",eNB_id,round);
      sprintf(vname,"dl%d_ch_r%d_ext11",eNB_id,round);
      LOG_M(fname,vname,ue->pdsch_vars[ue->current_thread_id[subframe]][eNB_id]->dl_ch_estimates_ext[3],
            12*(ue->frame_parms.N_RB_DL)*NSYMB,1,1);
    }
  }

  sprintf(fname,"dlsch%d_rxF_r%d_uespec0.m",eNB_id,round);
  sprintf(vname,"dl%d_rxF_r%d_uespec0",eNB_id,round);
  LOG_M(fname,vname,ue->pdsch_vars[ue->current_thread_id[subframe]][eNB_id]->rxdataF_uespec_pilots[0],
        12*(ue->frame_parms.N_RB_DL),1,1);
  /*
    LOG_M("dlsch%d_ch_ext01.m","dl01_ch0_ext",pdsch_vars[eNB_id]->dl_ch_estimates_ext[1],12*N_RB_DL*NSYMB,1,1);
    LOG_M("dlsch%d_ch_ext10.m","dl10_ch0_ext",pdsch_vars[eNB_id]->dl_ch_estimates_ext[2],12*N_RB_DL*NSYMB,1,1);
    LOG_M("dlsch%d_ch_ext11.m","dl11_ch0_ext",pdsch_vars[eNB_id]->dl_ch_estimates_ext[3],12*N_RB_DL*NSYMB,1,1);
  */
  sprintf(fname,"dlsch%d_r%d_rho.m",eNB_id,round);
  sprintf(vname,"dl_rho_r%d_%d",eNB_id,round);
  LOG_M(fname,vname,ue->pdsch_vars[ue->current_thread_id[subframe]][eNB_id]->dl_ch_rho_ext[harq_pid][round][0],
        12*(ue->frame_parms.N_RB_DL)*NSYMB,1,1);
  sprintf(fname,"dlsch%d_r%d_rho2.m",eNB_id,round);
  sprintf(vname,"dl_rho2_r%d_%d",eNB_id,round);
  LOG_M(fname,vname,ue->pdsch_vars[ue->current_thread_id[subframe]][eNB_id]->dl_ch_rho2_ext[0],
        12*(ue->frame_parms.N_RB_DL)*NSYMB,1,1);
  sprintf(fname,"dlsch%d_rxF_r%d_comp0.m",eNB_id,round);
  sprintf(vname,"dl%d_rxF_r%d_comp0",eNB_id,round);
  LOG_M(fname,vname,ue->pdsch_vars[ue->current_thread_id[subframe]][eNB_id]->rxdataF_comp0[0],
        12*(ue->frame_parms.N_RB_DL)*NSYMB,1,1);

  if (ue->frame_parms.nb_antenna_ports_eNB == 2) {
    sprintf(fname,"dlsch%d_rxF_r%d_comp1.m",eNB_id,round);
    sprintf(vname,"dl%d_rxF_r%d_comp1",eNB_id,round);
    LOG_M(fname,vname,ue->pdsch_vars[ue->current_thread_id[subframe]][eNB_id]->rxdataF_comp1[harq_pid][round][0],
          12*(ue->frame_parms.N_RB_DL)*NSYMB,1,1);
  }

  sprintf(fname,"dlsch%d_rxF_r%d_llr.m",eNB_id,round);
  sprintf(vname,"dl%d_r%d_llr",eNB_id,round);
  LOG_M(fname,vname, ue->pdsch_vars[ue->current_thread_id[subframe]][eNB_id]->llr[0],coded_bits_per_codeword[0],1,0);
  sprintf(fname,"dlsch%d_r%d_mag1.m",eNB_id,round);
  sprintf(vname,"dl%d_r%d_mag1",eNB_id,round);
  LOG_M(fname,vname,ue->pdsch_vars[ue->current_thread_id[subframe]][eNB_id]->dl_ch_mag0[0],
        12*(ue->frame_parms.N_RB_DL)*NSYMB,1,1);
  sprintf(fname,"dlsch%d_r%d_mag2.m",eNB_id,round);
  sprintf(vname,"dl%d_r%d_mag2",eNB_id,round);
  LOG_M(fname,vname,ue->pdsch_vars[ue->current_thread_id[subframe]][eNB_id]->dl_ch_magb0[0],
        12*(ue->frame_parms.N_RB_DL)*NSYMB,1,1);
}


