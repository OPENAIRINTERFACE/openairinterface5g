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

/*! \file PHY/LTE_TRANSPORT/dlsch_demodulation.c
 * \brief Top-level routines for demodulating the PDSCH physical channel from 38-211, V15.2 2018-06
 * \author H.Wang
 * \date 2018
 * \version 0.1
 * \company Eurecom
 * \note
 * \warning
 */
#include "PHY/defs_nr_UE.h"
#include "PHY/phy_extern_nr_ue.h"
#include "nr_transport_proto_ue.h"
//#include "SCHED/defs.h"
//#include "PHY/defs.h"
//#include "extern.h"
#include "PHY/sse_intrin.h"
#include "T.h"

#ifndef USER_MODE
#define NOCYGWIN_STATIC static
#else
#define NOCYGWIN_STATIC
#endif

/* dynamic shift for LLR computation for TM3/4
 * set as command line argument, see lte-softmodem.c
 * default value: 0
 */
int16_t nr_dlsch_demod_shift = 0;
//int16_t interf_unaw_shift = 13;

//#define DEBUG_HARQ

//#define DEBUG_PHY 1
//#define DEBUG_DLSCH_DEMOD 1



// [MCS][i_mod (0,1,2) = (2,4,6)]
//unsigned char offset_mumimo_llr_drange_fix=0;
//inferference-free case
/*unsigned char interf_unaw_shift_tm4_mcs[29]={5, 3, 4, 3, 3, 2, 1, 1, 2, 0, 1, 1, 1, 1, 0, 0,
                                             1, 1, 1, 1, 0, 2, 1, 0, 1, 0, 1, 0, 0} ;*/

//unsigned char interf_unaw_shift_tm1_mcs[29]={5, 5, 4, 3, 3, 3, 2, 2, 4, 4, 2, 3, 3, 3, 1, 1,
//                                          0, 1, 1, 2, 5, 4, 4, 6, 5, 1, 0, 5, 6} ; // mcs 21, 26, 28 seem to be errorneous

/*
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
//unsigned char offset_mumimo_llr_drange[29][3]= {{0, 6, 5},{0, 4, 5},{0, 4, 5},{0, 5, 4},{0, 5, 6},{0, 5, 3},{0, 4, 4},{0, 4, 4},{0, 3, 3},{0, 1, 2},{1, 1, 0},{1, 3, 2},{3, 4, 1},{2, 0, 0},{2, 2, 2},{1, 1, 1},{2, 1, 0},{2, 1, 1},{1, 0, 1},{1, 0, 1},{0, 0, 0},{1, 0, 0},{0, 0, 0},{0, 1, 0},{1, 0, 0},{0, 0, 0},{0, 0, 0},{0, 0, 0},{0, 0, 0}};


extern void print_shorts(char *s,int16_t *x);


int nr_rx_pdsch(PHY_VARS_NR_UE *ue,
             PDSCH_t type,
             unsigned char eNB_id,
             unsigned char eNB_id_i, //if this == ue->n_connected_eNB, we assume MU interference
             uint32_t frame,
             uint8_t nr_tti_rx,
             unsigned char symbol,
             unsigned char first_symbol_flag,
             RX_type_t rx_type,
             unsigned char i_mod,
             unsigned char harq_pid)
{

  NR_UE_COMMON *common_vars  = &ue->common_vars;
  NR_UE_PDSCH **pdsch_vars;
  NR_DL_FRAME_PARMS *frame_parms    = &ue->frame_parms;
  PHY_NR_MEASUREMENTS *measurements = &ue->measurements;
  NR_UE_DLSCH_t   **dlsch;

  int avg[4];
//  int avg_0[2];
//  int avg_1[2];

#if UE_TIMING_TRACE
  uint8_t slot = 0;
#endif

  unsigned char aatx,aarx;

  unsigned short nb_rb = 0, nb_re =0, round;
  int avgs = 0;// rb;
  NR_DL_UE_HARQ_t *dlsch0_harq,*dlsch1_harq = 0;

  uint8_t beamforming_mode;
  uint32_t *rballoc;

  int32_t **rxdataF_comp_ptr;
  int32_t **dl_ch_mag_ptr;
  int32_t codeword_TB0 = -1;
  int32_t codeword_TB1 = -1;

  //to be updated higher layer
  unsigned short start_rb = 0;
  unsigned short nb_rb_pdsch = 50;
  int16_t  *pllr_symbol_cw0;
  int16_t  *pllr_symbol_cw1;
  //int16_t  *pllr_symbol_cw0_deint;
  //int16_t  *pllr_symbol_cw1_deint;
  uint32_t llr_offset_symbol;
  //uint16_t bundle_L = 2;
  uint8_t l0 =2;
  
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
    pdsch_vars = ue->pdsch_vars[ue->current_thread_id[nr_tti_rx]];
    dlsch = ue->dlsch[ue->current_thread_id[nr_tti_rx]][eNB_id];
    
    
  //set active for testing -> to be removed
  dlsch[0]->harq_processes[harq_pid]->status = ACTIVE;
  dlsch[0]->harq_processes[harq_pid]->Qm = 2;
  dlsch[0]->harq_processes[harq_pid]->mcs = 9;
  dlsch[0]->harq_processes[harq_pid]->Nl=1;
  dlsch[0]->harq_processes[harq_pid]->round=0;
  dlsch[0]->harq_processes[harq_pid]->nb_rb = nb_rb_pdsch;
  frame_parms->nushift = 0;
  
  //printf("status TB0 = %d, status TB1 = %d \n", dlsch[0]->harq_processes[harq_pid]->status, dlsch[1]->harq_processes[harq_pid]->status);
    LOG_D(PHY,"AbsSubframe %d.%d / Sym %d harq_pid %d,  harq status %d.%d \n",
                   frame,nr_tti_rx,symbol,harq_pid,
                   dlsch[0]->harq_processes[harq_pid]->status,
                   dlsch[1]->harq_processes[harq_pid]->status);

    if ((dlsch[0]->harq_processes[harq_pid]->status == ACTIVE) &&
        (dlsch[1]->harq_processes[harq_pid]->status == ACTIVE)){
      codeword_TB0 = dlsch[0]->harq_processes[harq_pid]->codeword;
      codeword_TB1 = dlsch[1]->harq_processes[harq_pid]->codeword;
      dlsch0_harq = dlsch[codeword_TB0]->harq_processes[harq_pid];
      dlsch1_harq = dlsch[codeword_TB1]->harq_processes[harq_pid];
#ifdef DEBUG_HARQ
      printf("[DEMOD] I am assuming both TBs are active\n");
#endif
    }
     else if ((dlsch[0]->harq_processes[harq_pid]->status == ACTIVE) &&
              (dlsch[1]->harq_processes[harq_pid]->status != ACTIVE) ) {
      codeword_TB0 = dlsch[0]->harq_processes[harq_pid]->codeword;
      dlsch0_harq = dlsch[0]->harq_processes[harq_pid];
      dlsch1_harq = NULL;
      codeword_TB1 = -1;
#ifdef DEBUG_HARQ
      printf("[DEMOD] I am assuming only TB0 is active\n");
#endif
    }
     else if ((dlsch[0]->harq_processes[harq_pid]->status != ACTIVE) &&
              (dlsch[1]->harq_processes[harq_pid]->status == ACTIVE) ){
      codeword_TB1 = dlsch[1]->harq_processes[harq_pid]->codeword;
      dlsch0_harq  = dlsch[1]->harq_processes[harq_pid];
      dlsch1_harq  = NULL;
      codeword_TB0 = -1;
#ifdef DEBUG_HARQ
      printf("[DEMOD] I am assuming only TB1 is active, it is in cw %d\n", dlsch0_harq->codeword);
#endif
    }
    else {
      LOG_E(PHY,"[UE][FATAL] Frame %d nr_tti_rx %d: no active DLSCH\n",ue->proc.proc_rxtx[0].frame_rx,nr_tti_rx);
      return(-1);
    }
    beamforming_mode  = ue->transmission_mode[eNB_id]<7?0:ue->transmission_mode[eNB_id];
    break;

  default:
    LOG_E(PHY,"[UE][FATAL] Frame %d nr_tti_rx %d: Unknown PDSCH format %d\n",ue->proc.proc_rxtx[0].frame_rx,nr_tti_rx,type);
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
  if(symbol > ue->frame_parms.symbols_per_tti>>1)
  {
      slot = 1;
  }
#endif

#ifdef DEBUG_HARQ
  printf("Demod  dlsch0_harq->pmi_alloc %d\n",  dlsch0_harq->pmi_alloc);
#endif
#if 0
  if (frame_parms->nb_antenna_ports_eNB>1 && beamforming_mode==0) {
#ifdef DEBUG_DLSCH_MOD
    LOG_I(PHY,"dlsch: using pmi %x (%p), rb_alloc %x\n",pmi2hex_2Ar1(dlsch0_harq->pmi_alloc),dlsch[0],dlsch0_harq->rb_alloc_even[0]);
#endif

#if UE_TIMING_TRACE
    start_meas(&ue->generic_stat_bis[ue->current_thread_id[nr_tti_rx]][slot]);
#endif
    nb_rb = dlsch_extract_rbs_dual(common_vars->common_vars_rx_data_per_thread[ue->current_thread_id[nr_tti_rx]].rxdataF,
                                   common_vars->common_vars_rx_data_per_thread[ue->current_thread_id[nr_tti_rx]].dl_ch_estimates[eNB_id],
                                   pdsch_vars[eNB_id]->rxdataF_ext,
                                   pdsch_vars[eNB_id]->dl_ch_estimates_ext,
                                   dlsch0_harq->pmi_alloc,
                                   pdsch_vars[eNB_id]->pmi_ext,
                                   rballoc,
                                   symbol,
                                   nr_tti_rx,
                                   ue->high_speed_flag,
                                   frame_parms,
                                   dlsch0_harq->mimo_mode);
#ifdef DEBUG_DLSCH_MOD
      printf("dlsch: using pmi %lx, rb_alloc %x, pmi_ext ",pmi2hex_2Ar1(dlsch0_harq->pmi_alloc),*rballoc);
       for (rb=0;rb<nb_rb;rb++)
          printf("%d",pdsch_vars[eNB_id]->pmi_ext[rb]);
       printf("\n");
#endif


   if (rx_type >= rx_IC_single_stream) {
      if (eNB_id_i<ue->n_connected_eNB) // we are in TM5
      nb_rb = dlsch_extract_rbs_dual(common_vars->common_vars_rx_data_per_thread[ue->current_thread_id[nr_tti_rx]].rxdataF,
                                       common_vars->common_vars_rx_data_per_thread[ue->current_thread_id[nr_tti_rx]].dl_ch_estimates[eNB_id_i],
                                       pdsch_vars[eNB_id_i]->rxdataF_ext,
                                       pdsch_vars[eNB_id_i]->dl_ch_estimates_ext,
                                       dlsch0_harq->pmi_alloc,
                                       pdsch_vars[eNB_id_i]->pmi_ext,
                                       rballoc,
                                       symbol,
                                       nr_tti_rx,
                                       ue->high_speed_flag,
                                       frame_parms,
                                       dlsch0_harq->mimo_mode);
      else
        nb_rb = dlsch_extract_rbs_dual(common_vars->common_vars_rx_data_per_thread[ue->current_thread_id[nr_tti_rx]].rxdataF,
                                       common_vars->common_vars_rx_data_per_thread[ue->current_thread_id[nr_tti_rx]].dl_ch_estimates[eNB_id],
                                       pdsch_vars[eNB_id_i]->rxdataF_ext,
                                       pdsch_vars[eNB_id_i]->dl_ch_estimates_ext,
                                       dlsch0_harq->pmi_alloc,
                                       pdsch_vars[eNB_id_i]->pmi_ext,
                                       rballoc,
                                       symbol,
                                       nr_tti_rx,
                                       ue->high_speed_flag,
                                       frame_parms,
                                       dlsch0_harq->mimo_mode);
    }
  } else

#endif
	  if (beamforming_mode==0) { //else if nb_antennas_ports_eNB==1 && beamforming_mode == 0
		  //printf("start nr dlsch extract nr_tti_rx %d thread id %d \n", nr_tti_rx, ue->current_thread_id[nr_tti_rx]);
    nb_rb = nr_dlsch_extract_rbs_single(common_vars->common_vars_rx_data_per_thread[ue->current_thread_id[nr_tti_rx]].rxdataF,
                                     common_vars->common_vars_rx_data_per_thread[ue->current_thread_id[nr_tti_rx]].dl_ch_estimates[eNB_id],
                                     pdsch_vars[eNB_id]->rxdataF_ext,
                                     pdsch_vars[eNB_id]->dl_ch_estimates_ext,
                                     dlsch0_harq->pmi_alloc,
                                     pdsch_vars[eNB_id]->pmi_ext,
                                     rballoc,
                                     symbol,
									 start_rb,
									 nb_rb_pdsch,
                                     nr_tti_rx,
                                     ue->high_speed_flag,
                                     frame_parms);


/*
   if (rx_type==rx_IC_single_stream) {
     if (eNB_id_i<ue->n_connected_eNB)
        nb_rb = dlsch_extract_rbs_single(common_vars->common_vars_rx_data_per_thread[ue->current_thread_id[nr_tti_rx]].rxdataF,
                                         common_vars->common_vars_rx_data_per_thread[ue->current_thread_id[nr_tti_rx]].dl_ch_estimates[eNB_id_i],
                                         pdsch_vars[eNB_id_i]->rxdataF_ext,
                                         pdsch_vars[eNB_id_i]->dl_ch_estimates_ext,
                                         dlsch0_harq->pmi_alloc,
                                         pdsch_vars[eNB_id_i]->pmi_ext,
                                         rballoc,
                                         symbol,
                                         nr_tti_rx,
                                         ue->high_speed_flag,
                                         frame_parms);
      else
        nb_rb = dlsch_extract_rbs_single(common_vars->common_vars_rx_data_per_thread[ue->current_thread_id[nr_tti_rx]].rxdataF,
                                         common_vars->common_vars_rx_data_per_thread[ue->current_thread_id[nr_tti_rx]].dl_ch_estimates[eNB_id],
                                         pdsch_vars[eNB_id_i]->rxdataF_ext,
                                         pdsch_vars[eNB_id_i]->dl_ch_estimates_ext,
                                         dlsch0_harq->pmi_alloc,
                                         pdsch_vars[eNB_id_i]->pmi_ext,
                                         rballoc,
                                         symbol,
                                         nr_tti_rx,
                                         ue->high_speed_flag,
                                         frame_parms);
    }*/
  } /*else if (beamforming_mode==7) { //else if beamforming_mode == 7
    nb_rb = dlsch_extract_rbs_TM7(common_vars->common_vars_rx_data_per_thread[ue->current_thread_id[nr_tti_rx]].rxdataF,
                                  pdsch_vars[eNB_id]->dl_bf_ch_estimates,
                                  pdsch_vars[eNB_id]->rxdataF_ext,
                                  pdsch_vars[eNB_id]->dl_bf_ch_estimates_ext,
                                  rballoc,
                                  symbol,
                                  nr_tti_rx,
                                  ue->high_speed_flag,
                                  frame_parms);

  } else if(beamforming_mode>7) {
    LOG_W(PHY,"dlsch_demodulation: beamforming mode not supported yet.\n");
  }*/

  //printf("nb_rb = %d, eNB_id %d\n",nb_rb,eNB_id);
  if (nb_rb==0) {
    LOG_D(PHY,"dlsch_demodulation.c: nb_rb=0\n");
    return(-1);
  }

#if UE_TIMING_TRACE
    stop_meas(&ue->generic_stat_bis[ue->current_thread_id[nr_tti_rx]][slot]);
#if DISABLE_LOG_X
    printf("[AbsSFN %d.%d] Slot%d Symbol %d Flag %d type %d: Pilot/Data extraction %5.2f \n",frame,nr_tti_rx,slot,
            symbol,ue->high_speed_flag,type,symbol,ue->generic_stat_bis[ue->current_thread_id[nr_tti_rx]][slot].p_time/(cpuf*1000.0));
#else
    LOG_I(PHY, "[AbsSFN %d.%d] Slot%d Symbol %d Flag %d type %d: Pilot/Data extraction  %5.2f \n",frame,nr_tti_rx,slot,symbol,
            ue->high_speed_flag,type,ue->generic_stat_bis[ue->current_thread_id[nr_tti_rx]][slot].p_time/(cpuf*1000.0));
#endif
#endif

#if UE_TIMING_TRACE
    start_meas(&ue->generic_stat_bis[ue->current_thread_id[nr_tti_rx]][slot]);
#endif
  aatx = frame_parms->nb_antenna_ports_eNB;
  aarx = frame_parms->nb_antennas_rx;

  nr_dlsch_scale_channel(pdsch_vars[eNB_id]->dl_ch_estimates_ext,
                      frame_parms,
                      dlsch,
                      symbol,
                      nb_rb);

  if ((dlsch0_harq->mimo_mode<DUALSTREAM_UNIFORM_PRECODING1) &&
      (rx_type==rx_IC_single_stream) &&
      (eNB_id_i==ue->n_connected_eNB) &&
      (dlsch0_harq->dl_power_off==0)
     )  // TM5 two-user
  {
    nr_dlsch_scale_channel(pdsch_vars[eNB_id_i]->dl_ch_estimates_ext,
                        frame_parms,
                        dlsch,
                        symbol,
                        nb_rb);
  }

#if UE_TIMING_TRACE
    stop_meas(&ue->generic_stat_bis[ue->current_thread_id[nr_tti_rx]][slot]);
#if DISABLE_LOG_X
    printf("[AbsSFN %d.%d] Slot%d Symbol %d: Channel Scale %5.2f \n",frame,nr_tti_rx,slot,symbol,ue->generic_stat_bis[ue->current_thread_id[nr_tti_rx]][slot].p_time/(cpuf*1000.0));
#else
    LOG_I(PHY, "[AbsSFN %d.%d] Slot%d Symbol %d: Channel Scale  %5.2f \n",frame,nr_tti_rx,slot,symbol,ue->generic_stat_bis[ue->current_thread_id[nr_tti_rx]][slot].p_time/(cpuf*1000.0));
#endif
#endif

#if UE_TIMING_TRACE
    start_meas(&ue->generic_stat_bis[ue->current_thread_id[nr_tti_rx]][slot]);
#endif
  if (first_symbol_flag==1) {
    if (beamforming_mode==0){
      if (dlsch0_harq->mimo_mode<LARGE_CDD) {
        nr_dlsch_channel_level(pdsch_vars[eNB_id]->dl_ch_estimates_ext,
                           frame_parms,
                           avg,
                           symbol,
                           nb_rb);
        avgs = 0;
        for (aatx=0;aatx<frame_parms->nb_antenna_ports_eNB;aatx++)
          for (aarx=0;aarx<frame_parms->nb_antennas_rx;aarx++)
            avgs = cmax(avgs,avg[(aatx<<1)+aarx]);

        pdsch_vars[eNB_id]->log2_maxh = (log2_approx(avgs)/2)+1;
     }/*
     else if ((dlsch0_harq->mimo_mode == LARGE_CDD) ||
           ((dlsch0_harq->mimo_mode >=DUALSTREAM_UNIFORM_PRECODING1) &&
            (dlsch0_harq->mimo_mode <=DUALSTREAM_PUSCH_PRECODING)))
     {
      dlsch_channel_level_TM34(pdsch_vars[eNB_id]->dl_ch_estimates_ext,
                                 frame_parms,
                                 pdsch_vars[eNB_id]->pmi_ext,
                                 avg_0,
                                 avg_1,
                                 symbol,
                                 nb_rb,
                                 dlsch0_harq->mimo_mode);

      LOG_D(PHY,"Channel Level TM34  avg_0 %d, avg_1 %d, rx_type %d, rx_standard %d, dlsch_demod_shift %d \n", avg_0[0],
              avg_1[0], rx_type, rx_standard, dlsch_demod_shift);
        if (rx_type>rx_standard) {
          avg_0[0] = (log2_approx(avg_0[0])/2) + dlsch_demod_shift;// + 2 ;//+ 4;
          avg_1[0] = (log2_approx(avg_1[0])/2) + dlsch_demod_shift;// + 2 ;//+ 4;
          pdsch_vars[eNB_id]->log2_maxh0 = cmax(avg_0[0],0);
          pdsch_vars[eNB_id]->log2_maxh1 = cmax(avg_1[0],0);
         // printf("dlsch_demod_shift  %d\n", dlsch_demod_shift);
         }
          else {
          avg_0[0] = (log2_approx(avg_0[0])/2) - 13 + interf_unaw_shift;
          avg_1[0] = (log2_approx(avg_1[0])/2) - 13 + interf_unaw_shift;
          pdsch_vars[eNB_id]->log2_maxh0 = cmax(avg_0[0],0);
          pdsch_vars[eNB_id]->log2_maxh1 = cmax(avg_1[0],0);
        }
      }
      else if (dlsch0_harq->mimo_mode<DUALSTREAM_UNIFORM_PRECODING1) {// single-layer precoding (TM5, TM6)
        if ((rx_type==rx_IC_single_stream) && (eNB_id_i==ue->n_connected_eNB) && (dlsch0_harq->dl_power_off==0)) {
            dlsch_channel_level_TM56(pdsch_vars[eNB_id]->dl_ch_estimates_ext,
                                frame_parms,
                                pdsch_vars[eNB_id]->pmi_ext,
                                avg,
                                symbol,
                                nb_rb);
            avg[0] = log2_approx(avg[0]) - 13 + offset_mumimo_llr_drange[dlsch0_harq->mcs][(i_mod>>1)-1];
            pdsch_vars[eNB_id]->log2_maxh = cmax(avg[0],0);

        }
        else if (dlsch0_harq->dl_power_off==1) { //TM6

          nr_dlsch_channel_level(pdsch_vars[eNB_id]->dl_ch_estimates_ext,
                                   frame_parms,
                                   avg,
                                   symbol,
                                   nb_rb);

          avgs = 0;
          for (aatx=0;aatx<frame_parms->nb_antenna_ports_eNB;aatx++)
            for (aarx=0;aarx<frame_parms->nb_antennas_rx;aarx++)
              avgs = cmax(avgs,avg[(aatx<<1)+aarx]);

          pdsch_vars[eNB_id]->log2_maxh = (log2_approx(avgs)/2) + 1;
          pdsch_vars[eNB_id]->log2_maxh++;

        }
      }

    }
    else if (beamforming_mode==7)
       dlsch_channel_level_TM7(pdsch_vars[eNB_id]->dl_bf_ch_estimates_ext,
                              frame_parms,
                              avg,
                              symbol,
                              nb_rb);*/
    }
//#ifdef UE_DEBUG_TRACE
    LOG_I(PHY,"[DLSCH] AbsSubframe %d.%d log2_maxh = %d [log2_maxh0 %d log2_maxh1 %d] (%d,%d)\n",
            frame%1024,nr_tti_rx, pdsch_vars[eNB_id]->log2_maxh,
                                                 pdsch_vars[eNB_id]->log2_maxh0,
                                                 pdsch_vars[eNB_id]->log2_maxh1,
                                                 avg[0],avgs);
    //LOG_D(PHY,"[DLSCH] mimo_mode = %d\n", dlsch0_harq->mimo_mode);
//#endif

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
    if (type == PDSCH)
    {
      T(T_UE_PHY_PDSCH_ENERGY, T_INT(eNB_id),  T_INT(0), T_INT(frame%1024), T_INT(nr_tti_rx),
                               T_INT(avg[0]), T_INT(avg[1]),    T_INT(avg[2]),             T_INT(avg[3]));
    }
#endif

#if UE_TIMING_TRACE
    stop_meas(&ue->generic_stat_bis[ue->current_thread_id[nr_tti_rx]][slot]);
#if DISABLE_LOG_X
    printf("[AbsSFN %d.%d] Slot%d Symbol %d first_symbol_flag %d: Channel Level %5.2f \n",frame,nr_tti_rx,slot,symbol,first_symbol_flag,ue->generic_stat_bis[ue->current_thread_id[nr_tti_rx]][slot].p_time/(cpuf*1000.0));
#else
    LOG_I(PHY, "[AbsSFN %d.%d] Slot%d Symbol %d first_symbol_flag %d: Channel Level  %5.2f \n",frame,nr_tti_rx,slot,symbol,first_symbol_flag,ue->generic_stat_bis[ue->current_thread_id[nr_tti_rx]][slot].p_time/(cpuf*1000.0));
#endif
#endif


#if UE_TIMING_TRACE
    start_meas(&ue->generic_stat_bis[ue->current_thread_id[nr_tti_rx]][slot]);
#endif
// Now channel compensation
  if (dlsch0_harq->mimo_mode<LARGE_CDD) {
    nr_dlsch_channel_compensation(pdsch_vars[eNB_id]->rxdataF_ext,
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
 /*if (symbol == 5) {
     write_output("rxF_comp_d.m","rxF_c_d",&pdsch_vars[eNB_id]->rxdataF_comp0[0][symbol*frame_parms->N_RB_DL*12],frame_parms->N_RB_DL*12,1,1);
 } */

    /*if ((rx_type==rx_IC_single_stream) &&
        (eNB_id_i<ue->n_connected_eNB)) {
         nr_dlsch_channel_compensation(pdsch_vars[eNB_id_i]->rxdataF_ext,
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
#ifdef DEBUG_PHY
      if (symbol == 5) {
        write_output("rxF_comp_d.m","rxF_c_d",&pdsch_vars[eNB_id]->rxdataF_comp0[0][symbol*frame_parms->N_RB_DL*12],frame_parms->N_RB_DL*12,1,1);
        write_output("rxF_comp_i.m","rxF_c_i",&pdsch_vars[eNB_id_i]->rxdataF_comp0[0][symbol*frame_parms->N_RB_DL*12],frame_parms->N_RB_DL*12,1,1);
      }
#endif

      dlsch_dual_stream_correlation(frame_parms,
                                    symbol,
                                    nb_rb,
                                    pdsch_vars[eNB_id]->dl_ch_estimates_ext,
                                    pdsch_vars[eNB_id_i]->dl_ch_estimates_ext,
                                    pdsch_vars[eNB_id]->dl_ch_rho_ext[harq_pid][round],
                                    pdsch_vars[eNB_id]->log2_maxh);
    }*/
  }

#if 0
  else if ((dlsch0_harq->mimo_mode == LARGE_CDD) || ((dlsch0_harq->mimo_mode >=DUALSTREAM_UNIFORM_PRECODING1) &&
            (dlsch0_harq->mimo_mode <=DUALSTREAM_PUSCH_PRECODING))){
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
                                     pdsch_vars[eNB_id]->log2_maxh0,
                                     pdsch_vars[eNB_id]->log2_maxh1);
  /*   if (symbol == 5) {
     write_output("rxF_comp_d00.m","rxF_c_d00",&pdsch_vars[eNB_id]->rxdataF_comp0[0][symbol*frame_parms->N_RB_DL*12],frame_parms->N_RB_DL*12,1,1);// should be QAM
     write_output("rxF_comp_d01.m","rxF_c_d01",&pdsch_vars[eNB_id]->rxdataF_comp0[1][symbol*frame_parms->N_RB_DL*12],frame_parms->N_RB_DL*12,1,1);//should be almost 0
     write_output("rxF_comp_d10.m","rxF_c_d10",&pdsch_vars[eNB_id]->rxdataF_comp1[harq_pid][round][0][symbol*frame_parms->N_RB_DL*12],frame_parms->N_RB_DL*12,1,1);//should be almost 0
     write_output("rxF_comp_d11.m","rxF_c_d11",&pdsch_vars[eNB_id]->rxdataF_comp1[harq_pid][round][1][symbol*frame_parms->N_RB_DL*12],frame_parms->N_RB_DL*12,1,1);//should be QAM
        } */
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
  /*     if (symbol == 5) {
     write_output("rho0_0.m","rho0_0",&pdsch_vars[eNB_id]->dl_ch_rho_ext[harq_pid][round][0][symbol*frame_parms->N_RB_DL*12],frame_parms->N_RB_DL*12,1,1);// should be QAM
     write_output("rho2_0.m","rho2_0",&pdsch_vars[eNB_id]->dl_ch_rho2_ext[0][symbol*frame_parms->N_RB_DL*12],frame_parms->N_RB_DL*12,1,1);//should be almost 0
     write_output("rho0_1.m.m","rho0_1",&pdsch_vars[eNB_id]->dl_ch_rho_ext[harq_pid][round][1][symbol*frame_parms->N_RB_DL*12],frame_parms->N_RB_DL*12,1,1);//should be almost 0
     write_output("rho2_1.m","rho2_1",&pdsch_vars[eNB_id]->dl_ch_rho2_ext[1][symbol*frame_parms->N_RB_DL*12],frame_parms->N_RB_DL*12,1,1);//should be QAM
        } */

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
#ifdef DEBUG_PHY
      if (symbol==5) {
        write_output("rxF_comp_d.m","rxF_c_d",&pdsch_vars[eNB_id]->rxdataF_comp0[0][symbol*frame_parms->N_RB_DL*12],frame_parms->N_RB_DL*12,1,1);
       write_output("rxF_comp_i.m","rxF_c_i",&pdsch_vars[eNB_id_i]->rxdataF_comp0[0][symbol*frame_parms->N_RB_DL*12],frame_parms->N_RB_DL*12,1,1);
      }
#endif
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

      nr_dlsch_channel_compensation(pdsch_vars[eNB_id]->rxdataF_ext,
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
#endif

#if UE_TIMING_TRACE
    stop_meas(&ue->generic_stat_bis[ue->current_thread_id[nr_tti_rx]][slot]);
#if DISABLE_LOG_X
    printf("[AbsSFN %d.%d] Slot%d Symbol %d log2_maxh %d channel_level %d: Channel Comp %5.2f \n",frame,nr_tti_rx,slot,symbol,pdsch_vars[eNB_id]->log2_maxh,proc->channel_level,ue->generic_stat_bis[ue->current_thread_id[nr_tti_rx]][slot].p_time/(cpuf*1000.0));
#else
    LOG_I(PHY, "[AbsSFN %d.%d] Slot%d Symbol %d log2_maxh %d channel_level %d: Channel Comp  %5.2f \n",frame,nr_tti_rx,slot,symbol,pdsch_vars[eNB_id]->log2_maxh,proc->channel_level,ue->generic_stat_bis[ue->current_thread_id[nr_tti_rx]][slot].p_time/(cpuf*1000.0));
#endif
#endif
// MRC
#if UE_TIMING_TRACE
    start_meas(&ue->generic_stat_bis[ue->current_thread_id[nr_tti_rx]][slot]);
#endif

#if 0
    if (frame_parms->nb_antennas_rx > 1) {
    if ((dlsch0_harq->mimo_mode == LARGE_CDD) ||
        ((dlsch0_harq->mimo_mode >=DUALSTREAM_UNIFORM_PRECODING1) &&
         (dlsch0_harq->mimo_mode <=DUALSTREAM_PUSCH_PRECODING))){  // TM3 or TM4
      if (frame_parms->nb_antenna_ports_eNB == 2) {
        dlsch_detection_mrc_TM34(frame_parms,
                                 pdsch_vars[eNB_id],
                                 harq_pid,
                                 dlsch0_harq->round,
                                 symbol,
                                 nb_rb,
                                 1);
    /*   if (symbol == 5) {
     write_output("rho0_mrc.m","rho0_0",&pdsch_vars[eNB_id]->dl_ch_rho_ext[harq_pid][round][0][symbol*frame_parms->N_RB_DL*12],frame_parms->N_RB_DL*12,1,1);// should be QAM
     write_output("rho2_mrc.m","rho2_0",&pdsch_vars[eNB_id]->dl_ch_rho2_ext[0][symbol*frame_parms->N_RB_DL*12],frame_parms->N_RB_DL*12,1,1);//should be almost 0
        } */
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
#endif

      //printf("start compute LLR\n");
  if ((dlsch0_harq->mimo_mode == LARGE_CDD) ||
      ((dlsch0_harq->mimo_mode >=DUALSTREAM_UNIFORM_PRECODING1) &&
       (dlsch0_harq->mimo_mode <=DUALSTREAM_PUSCH_PRECODING)))  {
    rxdataF_comp_ptr = pdsch_vars[eNB_id]->rxdataF_comp1[harq_pid][round];
    dl_ch_mag_ptr = pdsch_vars[eNB_id]->dl_ch_mag1[harq_pid][round];
  }
  else {
    rxdataF_comp_ptr = pdsch_vars[eNB_id_i]->rxdataF_comp0;
    dl_ch_mag_ptr = pdsch_vars[eNB_id_i]->dl_ch_mag0;
    //i_mod should have been passed as a parameter
  }
  
#if UE_TIMING_TRACE
    stop_meas(&ue->generic_stat_bis[ue->current_thread_id[nr_tti_rx]][slot]);
#if DISABLE_LOG_X
    printf("[AbsSFN %d.%d] Slot%d Symbol %d: Channel Combine %5.2f \n",frame,nr_tti_rx,slot,symbol,ue->generic_stat_bis[ue->current_thread_id[nr_tti_rx]][slot].p_time/(cpuf*1000.0));
#else
    LOG_I(PHY, "[AbsSFN %d.%d] Slot%d Symbol %d: Channel Combine  %5.2f \n",frame,nr_tti_rx,slot,symbol,ue->generic_stat_bis[ue->current_thread_id[nr_tti_rx]][slot].p_time/(cpuf*1000.0));
#endif
#endif

#if UE_TIMING_TRACE
    start_meas(&ue->generic_stat_bis[ue->current_thread_id[nr_tti_rx]][slot]);
#endif
  //printf("LLR dlsch0_harq->Qm %d rx_type %d cw0 %d cw1 %d symbol %d \n",dlsch0_harq->Qm,rx_type,codeword_TB0,codeword_TB1,symbol);
  // compute LLRs
  // -> // compute @pointer where llrs should filled for this ofdm-symbol

  pdsch_vars[eNB_id]->llr_offset[l0-1] = 0;
  llr_offset_symbol = pdsch_vars[eNB_id]->llr_offset[symbol-1];
  //pllr_symbol_cw0_deint  = (int8_t*)pdsch_vars[eNB_id]->llr[0];
  //pllr_symbol_cw1_deint  = (int8_t*)pdsch_vars[eNB_id]->llr[1];
  pllr_symbol_cw0 = pdsch_vars[eNB_id]->llr[0];
  pllr_symbol_cw1 = pdsch_vars[eNB_id]->llr[1];
  pllr_symbol_cw0 += llr_offset_symbol;
  pllr_symbol_cw1 += llr_offset_symbol;
  
  nb_re= (symbol==l0)? (nb_rb*6):(nb_rb*12);
  pdsch_vars[eNB_id]->llr_offset[symbol] = nb_re*dlsch0_harq->Qm + llr_offset_symbol;
 
  /*LOG_I(PHY,"compute LLRs [symbol %d] NbRB %d Qm %d LLRs-Length %d LLR-Offset %d @LLR Buff %x @LLR Buff(symb) %x\n",
             symbol,
             nb_rb,dlsch0_harq->Qm,
             pdsch_vars[eNB_id]->llr_length[symbol],
             pdsch_vars[eNB_id]->llr_offset[symbol],
             (int16_t*)pdsch_vars[eNB_id]->llr[0],
             pllr_symbol_cw0);*/
             
             /*printf("compute LLRs [symbol %d] NbRB %d Qm %d LLRs-Length %d LLR-Offset %d @LLR Buff %p @LLR Buff(symb) %p\n",
             symbol,
             nb_rb,dlsch0_harq->Qm,
             pdsch_vars[eNB_id]->llr_length[symbol],
             pdsch_vars[eNB_id]->llr_offset[symbol],
             pdsch_vars[eNB_id]->llr[0],
             pllr_symbol_cw0);*/

  switch (dlsch0_harq->Qm) {
  case 2 :
    if ((rx_type==rx_standard) || (codeword_TB1 == -1)) {
        nr_dlsch_qpsk_llr(frame_parms,
                       pdsch_vars[eNB_id]->rxdataF_comp0,
                       pllr_symbol_cw0,
                       symbol,
                       first_symbol_flag,
                       nb_rb,
                       beamforming_mode);

    } else if (codeword_TB0 == -1){

        nr_dlsch_qpsk_llr(frame_parms,
                       pdsch_vars[eNB_id]->rxdataF_comp0,
                       pllr_symbol_cw1,
                       symbol,
                       first_symbol_flag,
                       nb_rb,
                       beamforming_mode);
    }
      else if (rx_type >= rx_IC_single_stream) {
        if (dlsch1_harq->Qm == 2) {
          nr_dlsch_qpsk_qpsk_llr(frame_parms,
                              pdsch_vars[eNB_id]->rxdataF_comp0,
                              rxdataF_comp_ptr,
                              pdsch_vars[eNB_id]->dl_ch_rho2_ext,
                              pdsch_vars[eNB_id]->llr[0],
                              symbol,first_symbol_flag,nb_rb,
                              adjust_G2(frame_parms,dlsch0_harq->rb_alloc_even,2,nr_tti_rx,symbol),
                              pdsch_vars[eNB_id]->llr128);
          if (rx_type==rx_IC_dual_stream) {
            nr_dlsch_qpsk_qpsk_llr(frame_parms,
                                rxdataF_comp_ptr,
                                pdsch_vars[eNB_id]->rxdataF_comp0,
                                pdsch_vars[eNB_id]->dl_ch_rho_ext[harq_pid][round],
                                pdsch_vars[eNB_id]->llr[1],
                                symbol,first_symbol_flag,nb_rb,
                                adjust_G2(frame_parms,dlsch1_harq->rb_alloc_even,2,nr_tti_rx,symbol),
                                pdsch_vars[eNB_id]->llr128_2ndstream);
          }
        }
        else if (dlsch1_harq->Qm == 4) {
          nr_dlsch_qpsk_16qam_llr(frame_parms,
                               pdsch_vars[eNB_id]->rxdataF_comp0,
                               rxdataF_comp_ptr,//i
                               dl_ch_mag_ptr,//i
                               pdsch_vars[eNB_id]->dl_ch_rho2_ext,
                               pdsch_vars[eNB_id]->llr[0],
                               symbol,first_symbol_flag,nb_rb,
                               adjust_G2(frame_parms,dlsch0_harq->rb_alloc_even,2,nr_tti_rx,symbol),
                               pdsch_vars[eNB_id]->llr128);
          if (rx_type==rx_IC_dual_stream) {
            nr_dlsch_16qam_qpsk_llr(frame_parms,
                                 rxdataF_comp_ptr,
                                 pdsch_vars[eNB_id]->rxdataF_comp0,//i
                                 dl_ch_mag_ptr,
                                 pdsch_vars[eNB_id]->dl_ch_rho_ext[harq_pid][round],
                                 pdsch_vars[eNB_id]->llr[1],
                                 symbol,first_symbol_flag,nb_rb,
                                 adjust_G2(frame_parms,dlsch1_harq->rb_alloc_even,4,nr_tti_rx,symbol),
                                 pdsch_vars[eNB_id]->llr128_2ndstream);
          }
        }
        else {
          nr_dlsch_qpsk_64qam_llr(frame_parms,
                               pdsch_vars[eNB_id]->rxdataF_comp0,
                               rxdataF_comp_ptr,//i
                               dl_ch_mag_ptr,//i
                               pdsch_vars[eNB_id]->dl_ch_rho2_ext,
                               pdsch_vars[eNB_id]->llr[0],
                               symbol,first_symbol_flag,nb_rb,
                               adjust_G2(frame_parms,dlsch0_harq->rb_alloc_even,2,nr_tti_rx,symbol),
                               pdsch_vars[eNB_id]->llr128);
          if (rx_type==rx_IC_dual_stream) {
            nr_dlsch_64qam_qpsk_llr(frame_parms,
                                 rxdataF_comp_ptr,
                                 pdsch_vars[eNB_id]->rxdataF_comp0,//i
                                 dl_ch_mag_ptr,
                                 pdsch_vars[eNB_id]->dl_ch_rho_ext[harq_pid][round],
                                 pdsch_vars[eNB_id]->llr[1],
                                 symbol,first_symbol_flag,nb_rb,
                                 adjust_G2(frame_parms,dlsch1_harq->rb_alloc_even,6,nr_tti_rx,symbol),
                                 pdsch_vars[eNB_id]->llr128_2ndstream);
          }
        }
      }
    break;
  case 4 :
    if ((rx_type==rx_standard ) || (codeword_TB1 == -1)) {
      nr_dlsch_16qam_llr(frame_parms,
                      pdsch_vars[eNB_id]->rxdataF_comp0,
                      pdsch_vars[eNB_id]->llr[0],
                      pdsch_vars[eNB_id]->dl_ch_mag0,
                      symbol,first_symbol_flag,nb_rb,
                      pdsch_vars[eNB_id]->llr128,
                      beamforming_mode);
    } else if (codeword_TB0 == -1){
      nr_dlsch_16qam_llr(frame_parms,
                      pdsch_vars[eNB_id]->rxdataF_comp0,
                      pdsch_vars[eNB_id]->llr[1],
                      pdsch_vars[eNB_id]->dl_ch_mag0,
                      symbol,first_symbol_flag,nb_rb,
                      pdsch_vars[eNB_id]->llr128_2ndstream,
                      beamforming_mode);
    }
    else if (rx_type >= rx_IC_single_stream) {
      if (dlsch1_harq->Qm == 2) {
        nr_dlsch_16qam_qpsk_llr(frame_parms,
                             pdsch_vars[eNB_id]->rxdataF_comp0,
                             rxdataF_comp_ptr,//i
                             pdsch_vars[eNB_id]->dl_ch_mag0,
                             pdsch_vars[eNB_id]->dl_ch_rho2_ext,
                             pdsch_vars[eNB_id]->llr[0],
                             symbol,first_symbol_flag,nb_rb,
                             adjust_G2(frame_parms,dlsch0_harq->rb_alloc_even,4,nr_tti_rx,symbol),
                             pdsch_vars[eNB_id]->llr128);
        if (rx_type==rx_IC_dual_stream) {
          nr_dlsch_qpsk_16qam_llr(frame_parms,
                               rxdataF_comp_ptr,
                               pdsch_vars[eNB_id]->rxdataF_comp0,//i
                               pdsch_vars[eNB_id]->dl_ch_mag0,//i
                               pdsch_vars[eNB_id]->dl_ch_rho_ext[harq_pid][round],
                               pdsch_vars[eNB_id]->llr[1],
                               symbol,first_symbol_flag,nb_rb,
                               adjust_G2(frame_parms,dlsch1_harq->rb_alloc_even,2,nr_tti_rx,symbol),
                               pdsch_vars[eNB_id]->llr128_2ndstream);
        }
      }
      else if (dlsch1_harq->Qm == 4) {
        nr_dlsch_16qam_16qam_llr(frame_parms,
                              pdsch_vars[eNB_id]->rxdataF_comp0,
                              rxdataF_comp_ptr,//i
                              pdsch_vars[eNB_id]->dl_ch_mag0,
                              dl_ch_mag_ptr,//i
                              pdsch_vars[eNB_id]->dl_ch_rho2_ext,
                              pdsch_vars[eNB_id]->llr[0],
                              symbol,first_symbol_flag,nb_rb,
                              adjust_G2(frame_parms,dlsch0_harq->rb_alloc_even,4,nr_tti_rx,symbol),
                              pdsch_vars[eNB_id]->llr128);
        if (rx_type==rx_IC_dual_stream) {
          nr_dlsch_16qam_16qam_llr(frame_parms,
                                rxdataF_comp_ptr,
                                pdsch_vars[eNB_id]->rxdataF_comp0,//i
                                dl_ch_mag_ptr,
                                pdsch_vars[eNB_id]->dl_ch_mag0,//i
                                pdsch_vars[eNB_id]->dl_ch_rho_ext[harq_pid][round],
                                pdsch_vars[eNB_id]->llr[1],
                                symbol,first_symbol_flag,nb_rb,
                                adjust_G2(frame_parms,dlsch1_harq->rb_alloc_even,4,nr_tti_rx,symbol),
                                pdsch_vars[eNB_id]->llr128_2ndstream);
        }
      }
      else {
        nr_dlsch_16qam_64qam_llr(frame_parms,
                              pdsch_vars[eNB_id]->rxdataF_comp0,
                              rxdataF_comp_ptr,//i
                              pdsch_vars[eNB_id]->dl_ch_mag0,
                              dl_ch_mag_ptr,//i
                              pdsch_vars[eNB_id]->dl_ch_rho2_ext,
                              pdsch_vars[eNB_id]->llr[0],
                              symbol,first_symbol_flag,nb_rb,
                              adjust_G2(frame_parms,dlsch0_harq->rb_alloc_even,4,nr_tti_rx,symbol),
                              pdsch_vars[eNB_id]->llr128);
        if (rx_type==rx_IC_dual_stream) {
          nr_dlsch_64qam_16qam_llr(frame_parms,
                                rxdataF_comp_ptr,
                                pdsch_vars[eNB_id]->rxdataF_comp0,
                                dl_ch_mag_ptr,
                                pdsch_vars[eNB_id]->dl_ch_mag0,
                                pdsch_vars[eNB_id]->dl_ch_rho_ext[harq_pid][round],
                                pdsch_vars[eNB_id]->llr[1],
                                symbol,first_symbol_flag,nb_rb,
                                adjust_G2(frame_parms,dlsch1_harq->rb_alloc_even,6,nr_tti_rx,symbol),
                                pdsch_vars[eNB_id]->llr128_2ndstream);
        }
      }
    }
    break;
  case 6 :
    if ((rx_type==rx_standard) || (codeword_TB1 == -1))  {
      nr_dlsch_64qam_llr(frame_parms,
                      pdsch_vars[eNB_id]->rxdataF_comp0,
                      (int16_t*)pllr_symbol_cw0,
                      pdsch_vars[eNB_id]->dl_ch_mag0,
                      pdsch_vars[eNB_id]->dl_ch_magb0,
                      symbol,first_symbol_flag,nb_rb,
                      pdsch_vars[eNB_id]->llr_offset[symbol],
                      beamforming_mode);
    } else if (codeword_TB0 == -1){
      nr_dlsch_64qam_llr(frame_parms,
                      pdsch_vars[eNB_id]->rxdataF_comp0,
                      pllr_symbol_cw1,
                      pdsch_vars[eNB_id]->dl_ch_mag0,
                      pdsch_vars[eNB_id]->dl_ch_magb0,
                      symbol,first_symbol_flag,nb_rb,
                      pdsch_vars[eNB_id]->llr_offset[symbol],
                      beamforming_mode);
    }
    else if (rx_type >= rx_IC_single_stream) {
      if (dlsch1_harq->Qm == 2) {
        nr_dlsch_64qam_qpsk_llr(frame_parms,
                             pdsch_vars[eNB_id]->rxdataF_comp0,
                             rxdataF_comp_ptr,//i
                             pdsch_vars[eNB_id]->dl_ch_mag0,
                             pdsch_vars[eNB_id]->dl_ch_rho2_ext,
                             pdsch_vars[eNB_id]->llr[0],
                             symbol,first_symbol_flag,nb_rb,
                             adjust_G2(frame_parms,dlsch0_harq->rb_alloc_even,6,nr_tti_rx,symbol),
                             pdsch_vars[eNB_id]->llr128);
        if (rx_type==rx_IC_dual_stream) {
          nr_dlsch_qpsk_64qam_llr(frame_parms,
                               rxdataF_comp_ptr,
                               pdsch_vars[eNB_id]->rxdataF_comp0,//i
                               pdsch_vars[eNB_id]->dl_ch_mag0,
                               pdsch_vars[eNB_id]->dl_ch_rho_ext[harq_pid][round],
                               pdsch_vars[eNB_id]->llr[1],
                               symbol,first_symbol_flag,nb_rb,
                               adjust_G2(frame_parms,dlsch1_harq->rb_alloc_even,2,nr_tti_rx,symbol),
                               pdsch_vars[eNB_id]->llr128_2ndstream);
        }
      }
      else if (dlsch1_harq->Qm == 4) {
        nr_dlsch_64qam_16qam_llr(frame_parms,
                              pdsch_vars[eNB_id]->rxdataF_comp0,
                              rxdataF_comp_ptr,//i
                              pdsch_vars[eNB_id]->dl_ch_mag0,
                              dl_ch_mag_ptr,//i
                              pdsch_vars[eNB_id]->dl_ch_rho2_ext,
                              pdsch_vars[eNB_id]->llr[0],
                              symbol,first_symbol_flag,nb_rb,
                              adjust_G2(frame_parms,dlsch0_harq->rb_alloc_even,6,nr_tti_rx,symbol),
                              pdsch_vars[eNB_id]->llr128);
        if (rx_type==rx_IC_dual_stream) {
          nr_dlsch_16qam_64qam_llr(frame_parms,
                                rxdataF_comp_ptr,
                                pdsch_vars[eNB_id]->rxdataF_comp0,//i
                                dl_ch_mag_ptr,
                                pdsch_vars[eNB_id]->dl_ch_mag0,//i
                                pdsch_vars[eNB_id]->dl_ch_rho_ext[harq_pid][round],
                                pdsch_vars[eNB_id]->llr[1],
                                symbol,first_symbol_flag,nb_rb,
                                adjust_G2(frame_parms,dlsch1_harq->rb_alloc_even,4,nr_tti_rx,symbol),
                                pdsch_vars[eNB_id]->llr128_2ndstream);
        }
      }
      else {
        nr_dlsch_64qam_64qam_llr(frame_parms,
                              pdsch_vars[eNB_id]->rxdataF_comp0,
                              rxdataF_comp_ptr,//i
                              pdsch_vars[eNB_id]->dl_ch_mag0,
                              dl_ch_mag_ptr,//i
                              pdsch_vars[eNB_id]->dl_ch_rho2_ext,
                              (int16_t*)pllr_symbol_cw0,
                              symbol,first_symbol_flag,nb_rb,
                              adjust_G2(frame_parms,dlsch0_harq->rb_alloc_even,6,nr_tti_rx,symbol),
                              pdsch_vars[eNB_id]->llr_offset[symbol]);
        if (rx_type==rx_IC_dual_stream) {
          nr_dlsch_64qam_64qam_llr(frame_parms,
                                rxdataF_comp_ptr,
                                pdsch_vars[eNB_id]->rxdataF_comp0,//i
                                dl_ch_mag_ptr,
                                pdsch_vars[eNB_id]->dl_ch_mag0,//i
                                pdsch_vars[eNB_id]->dl_ch_rho_ext[harq_pid][round],
                                pllr_symbol_cw1,
                                symbol,first_symbol_flag,nb_rb,
                                adjust_G2(frame_parms,dlsch1_harq->rb_alloc_even,6,nr_tti_rx,symbol),
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
        nr_dlsch_qpsk_llr(frame_parms,
                       pdsch_vars[eNB_id]->rxdataF_comp0,
                       pllr_symbol_cw0,
                       symbol,first_symbol_flag,nb_rb,
                       beamforming_mode);
    }
    break;
  case 4:
    if (rx_type==rx_standard) {
      nr_dlsch_16qam_llr(frame_parms,
                      pdsch_vars[eNB_id]->rxdataF_comp0,
                      pdsch_vars[eNB_id]->llr[0],
                      pdsch_vars[eNB_id]->dl_ch_mag0,
                      symbol,first_symbol_flag,nb_rb,
                      pdsch_vars[eNB_id]->llr128,
                      beamforming_mode);
    }
    break;
  case 6 :
    if (rx_type==rx_standard) {
      nr_dlsch_64qam_llr(frame_parms,
                      pdsch_vars[eNB_id]->rxdataF_comp0,
                      pllr_symbol_cw0,
                      pdsch_vars[eNB_id]->dl_ch_mag0,
                      pdsch_vars[eNB_id]->dl_ch_magb0,
                      symbol,first_symbol_flag,nb_rb,
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

  //nr_dlsch_deinterleaving(symbol,bundle_L,(int16_t*)pllr_symbol_cw0,(int16_t*)pllr_symbol_cw0_deint, nb_rb_pdsch);
  
#if UE_TIMING_TRACE
    stop_meas(&ue->generic_stat_bis[ue->current_thread_id[nr_tti_rx]][slot]);
#if DISABLE_LOG_X
    printf("[AbsSFN %d.%d] Slot%d Symbol %d: LLR Computation %5.2f \n",frame,nr_tti_rx,slot,symbol,ue->generic_stat_bis[ue->current_thread_id[nr_tti_rx]][slot].p_time/(cpuf*1000.0));
#else
    LOG_I(PHY, "[AbsSFN %d.%d] Slot%d Symbol %d: LLR Computation  %5.2f \n",frame,nr_tti_rx,slot,symbol,ue->generic_stat_bis[ue->current_thread_id[nr_tti_rx]][slot].p_time/(cpuf*1000.0));
#endif
#endif
// Please keep it: useful for debugging
#if 0
  if( (symbol == 13) && (nr_tti_rx==0) && (dlsch0_harq->Qm == 6) /*&& (nb_rb==25)*/)
  {
      LOG_E(PHY,"Dump Phy Chan Est \n");
      if(1)
      {
#if 1
      write_output("rxdataF0.m"    , "rxdataF0",             &common_vars->common_vars_rx_data_per_thread[ue->current_thread_id[nr_tti_rx]].rxdataF[0][0],14*frame_parms->ofdm_symbol_size,1,1);
      //write_output("rxdataF1.m"    , "rxdataF1",             &common_vars->common_vars_rx_data_per_thread[ue->current_thread_id[nr_tti_rx]].rxdataF[0][0],14*frame_parms->ofdm_symbol_size,1,1);
      write_output("dl_ch_estimates00.m", "dl_ch_estimates00",   &common_vars->common_vars_rx_data_per_thread[ue->current_thread_id[nr_tti_rx]].dl_ch_estimates[eNB_id][0][0],14*frame_parms->ofdm_symbol_size,1,1);
      //write_output("dl_ch_estimates01.m", "dl_ch_estimates01",   &common_vars->common_vars_rx_data_per_thread[ue->current_thread_id[nr_tti_rx]].dl_ch_estimates[eNB_id][1][0],14*frame_parms->ofdm_symbol_size,1,1);
      //write_output("dl_ch_estimates10.m", "dl_ch_estimates10",   &common_vars->common_vars_rx_data_per_thread[ue->current_thread_id[nr_tti_rx]].dl_ch_estimates[eNB_id][2][0],14*frame_parms->ofdm_symbol_size,1,1);
      //write_output("dl_ch_estimates11.m", "dl_ch_estimates11",   &common_vars->common_vars_rx_data_per_thread[ue->current_thread_id[nr_tti_rx]].dl_ch_estimates[eNB_id][3][0],14*frame_parms->ofdm_symbol_size,1,1);


      //write_output("rxdataF_ext00.m"    , "rxdataF_ext00",       &pdsch_vars[eNB_id]->rxdataF_ext[0][0],14*frame_parms->N_RB_DL*12,1,1);
      //write_output("rxdataF_ext01.m"    , "rxdataF_ext01",       &pdsch_vars[eNB_id]->rxdataF_ext[1][0],14*frame_parms->N_RB_DL*12,1,1);
      //write_output("rxdataF_ext10.m"    , "rxdataF_ext10",       &pdsch_vars[eNB_id]->rxdataF_ext[2][0],14*frame_parms->N_RB_DL*12,1,1);
      //write_output("rxdataF_ext11.m"    , "rxdataF_ext11",       &pdsch_vars[eNB_id]->rxdataF_ext[3][0],14*frame_parms->N_RB_DL*12,1,1);
      write_output("dl_ch_estimates_ext00.m", "dl_ch_estimates_ext00", &pdsch_vars[eNB_id]->dl_ch_estimates_ext[0][0],14*frame_parms->N_RB_DL*12,1,1);
      //write_output("dl_ch_estimates_ext01.m", "dl_ch_estimates_ext01", &pdsch_vars[eNB_id]->dl_ch_estimates_ext[1][0],14*frame_parms->N_RB_DL*12,1,1);
      //write_output("dl_ch_estimates_ext10.m", "dl_ch_estimates_ext10", &pdsch_vars[eNB_id]->dl_ch_estimates_ext[2][0],14*frame_parms->N_RB_DL*12,1,1);
      //write_output("dl_ch_estimates_ext11.m", "dl_ch_estimates_ext11", &pdsch_vars[eNB_id]->dl_ch_estimates_ext[3][0],14*frame_parms->N_RB_DL*12,1,1);
      write_output("rxdataF_comp00.m","rxdataF_comp00",              &pdsch_vars[eNB_id]->rxdataF_comp0[0][0],14*frame_parms->N_RB_DL*12,1,1);
      //write_output("rxdataF_comp01.m","rxdataF_comp01",              &pdsch_vars[eNB_id]->rxdataF_comp0[1][0],14*frame_parms->N_RB_DL*12,1,1);
      //write_output("rxdataF_comp10.m","rxdataF_comp10",              &pdsch_vars[eNB_id]->rxdataF_comp1[harq_pid][round][0][0],14*frame_parms->N_RB_DL*12,1,1);
      //write_output("rxdataF_comp11.m","rxdataF_comp11",              &pdsch_vars[eNB_id]->rxdataF_comp1[harq_pid][round][1][0],14*frame_parms->N_RB_DL*12,1,1);
#endif
      write_output("llr0.m","llr0",  &pdsch_vars[eNB_id]->llr[0][0],(14*nb_rb*12*dlsch1_harq->Qm) - 4*(nb_rb*4*dlsch1_harq->Qm),1,0);
      //write_output("llr1.m","llr1",  &pdsch_vars[eNB_id]->llr[1][0],(14*nb_rb*12*dlsch1_harq->Qm) - 4*(nb_rb*4*dlsch1_harq->Qm),1,0);


      AssertFatal(0," ");
      }

  }
#endif

#if T_TRACER
  T(T_UE_PHY_PDSCH_IQ, T_INT(eNB_id), T_INT(ue->Mod_id), T_INT(frame%1024),
    T_INT(nr_tti_rx), T_INT(nb_rb),
    T_INT(frame_parms->N_RB_UL), T_INT(frame_parms->symbols_per_tti),
    T_BUFFER(&pdsch_vars[eNB_id]->rxdataF_comp0[eNB_id][0],
             2 * /* ulsch[UE_id]->harq_processes[harq_pid]->nb_rb */ frame_parms->N_RB_UL *12*frame_parms->symbols_per_tti*2));
#endif
  return(0);

}

void nr_dlsch_deinterleaving(uint8_t symbol,
							uint16_t L,
							uint16_t *llr,
							uint16_t *llr_deint,
							uint16_t nb_rb_pdsch)
{

  uint32_t bundle_idx, N_bundle, R, C, r,c;
  int32_t m,k;
  uint8_t nb_re;

  R=2;
  N_bundle = nb_rb_pdsch/L;
  C=N_bundle/R;

  uint32_t *bundle_deint = malloc(N_bundle*sizeof(uint32_t));

  printf("N_bundle %d L %d nb_rb_pdsch %d\n",N_bundle, L,nb_rb_pdsch);

  if (symbol==2)
	  nb_re = 6;
  else
	  nb_re = 12;


  AssertFatal(llr!=NULL,"nr_dlsch_deinterleaving: FATAL llr is Null\n");


  for (c =0; c< C; c++){
	  for (r=0; r<R;r++){
		  bundle_idx = r*C+c;
		  bundle_deint[bundle_idx] = c*R+r;
		  //printf("c %u r %u bundle_idx %u bundle_deinter %u\n", c, r, bundle_idx, bundle_deint[bundle_idx]);
	  }
  }

  for (k=0; k<N_bundle;k++)
  {
	  for (m=0; m<nb_re*L;m++){
		  llr_deint[bundle_deint[k]*nb_re*L+m]= llr[k*nb_re*L+m];
		  //printf("k %d m %d bundle_deint %d llr_deint %d\n", k, m, bundle_deint[k], llr_deint[bundle_deint[k]*nb_re*L+m]);
	  }
  }
}

//==============================================================================================
// Pre-processing for LLR computation
//==============================================================================================

void nr_dlsch_channel_compensation(int **rxdataF_ext,
                                int **dl_ch_estimates_ext,
                                int **dl_ch_mag,
                                int **dl_ch_magb,
                                int **rxdataF_comp,
                                int **rho,
                                NR_DL_FRAME_PARMS *frame_parms,
                                unsigned char symbol,
                                uint8_t first_symbol_flag,
                                unsigned char mod_order,
                                unsigned short nb_rb,
                                unsigned char output_shift,
                                PHY_NR_MEASUREMENTS *measurements)
{

#if defined(__i386) || defined(__x86_64)

  unsigned short rb;
  unsigned char aatx,aarx,pilots=0;
  __m128i *dl_ch128,*dl_ch128_2,*dl_ch_mag128,*dl_ch_mag128b,*rxdataF128,*rxdataF_comp128,*rho128;
  __m128i mmtmpD0,mmtmpD1,mmtmpD2,mmtmpD3,QAM_amp128,QAM_amp128b;
  QAM_amp128b = _mm_setzero_si128();

  if (symbol == 2){
      pilots=1;
  }


  for (aatx=0; aatx<frame_parms->nb_antenna_ports_eNB; aatx++) {
    if (mod_order == 4) {
      QAM_amp128 = _mm_set1_epi16(QAM16_n1);  // 2/sqrt(10)
      QAM_amp128b = _mm_setzero_si128();
    } else if (mod_order == 6) {
      QAM_amp128  = _mm_set1_epi16(QAM64_n1); //
      QAM_amp128b = _mm_set1_epi16(QAM64_n2);
    }

    //    printf("comp: rxdataF_comp %p, symbol %d\n",rxdataF_comp[0],symbol);

    for (aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++) {

      dl_ch128          = (__m128i *)&dl_ch_estimates_ext[(aatx<<1)+aarx][symbol*nb_rb*12];
      dl_ch_mag128      = (__m128i *)&dl_ch_mag[(aatx<<1)+aarx][symbol*nb_rb*12];
      dl_ch_mag128b     = (__m128i *)&dl_ch_magb[(aatx<<1)+aarx][symbol*nb_rb*12];
      rxdataF128        = (__m128i *)&rxdataF_ext[aarx][symbol*nb_rb*12];
      rxdataF_comp128   = (__m128i *)&rxdataF_comp[(aatx<<1)+aarx][symbol*nb_rb*12];


      for (rb=0; rb<nb_rb; rb++) {
        if (mod_order>2) {
          // get channel amplitude if not QPSK

          mmtmpD0 = _mm_madd_epi16(dl_ch128[0],dl_ch128[0]);
          mmtmpD0 = _mm_srai_epi32(mmtmpD0,(output_shift-2));

          mmtmpD1 = _mm_madd_epi16(dl_ch128[1],dl_ch128[1]);
          mmtmpD1 = _mm_srai_epi32(mmtmpD1,(output_shift-2));

          mmtmpD0 = _mm_packs_epi32(mmtmpD0,mmtmpD1);

          // store channel magnitude here in a new field of dlsch

          dl_ch_mag128[0] = _mm_unpacklo_epi16(mmtmpD0,mmtmpD0);
          dl_ch_mag128b[0] = dl_ch_mag128[0];
          dl_ch_mag128[0] = _mm_mulhi_epi16(dl_ch_mag128[0],QAM_amp128);
          dl_ch_mag128[0] = _mm_slli_epi16(dl_ch_mag128[0],1);
    //print_ints("Re(ch):",(int16_t*)&mmtmpD0);
    //print_shorts("QAM_amp:",(int16_t*)&QAM_amp128);
    //print_shorts("mag:",(int16_t*)&dl_ch_mag128[0]);
          dl_ch_mag128[1] = _mm_unpackhi_epi16(mmtmpD0,mmtmpD0);
          dl_ch_mag128b[1] = dl_ch_mag128[1];
          dl_ch_mag128[1] = _mm_mulhi_epi16(dl_ch_mag128[1],QAM_amp128);
          dl_ch_mag128[1] = _mm_slli_epi16(dl_ch_mag128[1],1);

          if (pilots==0) {
            mmtmpD0 = _mm_madd_epi16(dl_ch128[2],dl_ch128[2]);
            mmtmpD0 = _mm_srai_epi32(mmtmpD0,output_shift);
            mmtmpD1 = _mm_packs_epi32(mmtmpD0,mmtmpD0);

            dl_ch_mag128[2] = _mm_unpacklo_epi16(mmtmpD1,mmtmpD1);
            dl_ch_mag128b[2] = dl_ch_mag128[2];

            dl_ch_mag128[2] = _mm_mulhi_epi16(dl_ch_mag128[2],QAM_amp128);
            dl_ch_mag128[2] = _mm_slli_epi16(dl_ch_mag128[2],1);
          }

          dl_ch_mag128b[0] = _mm_mulhi_epi16(dl_ch_mag128b[0],QAM_amp128b);
          dl_ch_mag128b[0] = _mm_slli_epi16(dl_ch_mag128b[0],1);


          dl_ch_mag128b[1] = _mm_mulhi_epi16(dl_ch_mag128b[1],QAM_amp128b);
          dl_ch_mag128b[1] = _mm_slli_epi16(dl_ch_mag128b[1],1);

          if (pilots==0) {
            dl_ch_mag128b[2] = _mm_mulhi_epi16(dl_ch_mag128b[2],QAM_amp128b);
            dl_ch_mag128b[2] = _mm_slli_epi16(dl_ch_mag128b[2],1);
          }
        }

        // multiply by conjugated channel
        mmtmpD0 = _mm_madd_epi16(dl_ch128[0],rxdataF128[0]);
        //  print_ints("re",&mmtmpD0);

        // mmtmpD0 contains real part of 4 consecutive outputs (32-bit)
        mmtmpD1 = _mm_shufflelo_epi16(dl_ch128[0],_MM_SHUFFLE(2,3,0,1));
        mmtmpD1 = _mm_shufflehi_epi16(mmtmpD1,_MM_SHUFFLE(2,3,0,1));
        mmtmpD1 = _mm_sign_epi16(mmtmpD1,*(__m128i*)&conjugate[0]);
        //  print_ints("im",&mmtmpD1);
        mmtmpD1 = _mm_madd_epi16(mmtmpD1,rxdataF128[0]);
        // mmtmpD1 contains imag part of 4 consecutive outputs (32-bit)
        mmtmpD0 = _mm_srai_epi32(mmtmpD0,output_shift);
        //  print_ints("re(shift)",&mmtmpD0);
        mmtmpD1 = _mm_srai_epi32(mmtmpD1,output_shift);
        //  print_ints("im(shift)",&mmtmpD1);
        mmtmpD2 = _mm_unpacklo_epi32(mmtmpD0,mmtmpD1);
        mmtmpD3 = _mm_unpackhi_epi32(mmtmpD0,mmtmpD1);
        //        print_ints("c0",&mmtmpD2);
        //  print_ints("c1",&mmtmpD3);
        rxdataF_comp128[0] = _mm_packs_epi32(mmtmpD2,mmtmpD3);
        //  print_shorts("rx:",rxdataF128);
        //  print_shorts("ch:",dl_ch128);
        //  print_shorts("pack:",rxdataF_comp128);

        // multiply by conjugated channel
        mmtmpD0 = _mm_madd_epi16(dl_ch128[1],rxdataF128[1]);
        // mmtmpD0 contains real part of 4 consecutive outputs (32-bit)
        mmtmpD1 = _mm_shufflelo_epi16(dl_ch128[1],_MM_SHUFFLE(2,3,0,1));
        mmtmpD1 = _mm_shufflehi_epi16(mmtmpD1,_MM_SHUFFLE(2,3,0,1));
        mmtmpD1 = _mm_sign_epi16(mmtmpD1,*(__m128i*)conjugate);
        mmtmpD1 = _mm_madd_epi16(mmtmpD1,rxdataF128[1]);
        // mmtmpD1 contains imag part of 4 consecutive outputs (32-bit)
        mmtmpD0 = _mm_srai_epi32(mmtmpD0,output_shift);
        mmtmpD1 = _mm_srai_epi32(mmtmpD1,output_shift);
        mmtmpD2 = _mm_unpacklo_epi32(mmtmpD0,mmtmpD1);
        mmtmpD3 = _mm_unpackhi_epi32(mmtmpD0,mmtmpD1);

        rxdataF_comp128[1] = _mm_packs_epi32(mmtmpD2,mmtmpD3);
        //  print_shorts("rx:",rxdataF128+1);
        //  print_shorts("ch:",dl_ch128+1);
        //  print_shorts("pack:",rxdataF_comp128+1);

        if (pilots==0) {
          // multiply by conjugated channel
          mmtmpD0 = _mm_madd_epi16(dl_ch128[2],rxdataF128[2]);
          // mmtmpD0 contains real part of 4 consecutive outputs (32-bit)
          mmtmpD1 = _mm_shufflelo_epi16(dl_ch128[2],_MM_SHUFFLE(2,3,0,1));
          mmtmpD1 = _mm_shufflehi_epi16(mmtmpD1,_MM_SHUFFLE(2,3,0,1));
          mmtmpD1 = _mm_sign_epi16(mmtmpD1,*(__m128i*)conjugate);
          mmtmpD1 = _mm_madd_epi16(mmtmpD1,rxdataF128[2]);
          // mmtmpD1 contains imag part of 4 consecutive outputs (32-bit)
          mmtmpD0 = _mm_srai_epi32(mmtmpD0,output_shift);
          mmtmpD1 = _mm_srai_epi32(mmtmpD1,output_shift);
          mmtmpD2 = _mm_unpacklo_epi32(mmtmpD0,mmtmpD1);
          mmtmpD3 = _mm_unpackhi_epi32(mmtmpD0,mmtmpD1);

          rxdataF_comp128[2] = _mm_packs_epi32(mmtmpD2,mmtmpD3);
          //  print_shorts("rx:",rxdataF128+2);
          //  print_shorts("ch:",dl_ch128+2);
          //        print_shorts("pack:",rxdataF_comp128+2);

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
      rho128        = (__m128i *)&rho[aarx][symbol*frame_parms->N_RB_DL*12];
      dl_ch128      = (__m128i *)&dl_ch_estimates_ext[aarx][symbol*frame_parms->N_RB_DL*12];
      dl_ch128_2    = (__m128i *)&dl_ch_estimates_ext[2+aarx][symbol*frame_parms->N_RB_DL*12];

      for (rb=0; rb<nb_rb; rb++) {
        // multiply by conjugated channel
        mmtmpD0 = _mm_madd_epi16(dl_ch128[0],dl_ch128_2[0]);
        //  print_ints("re",&mmtmpD0);

        // mmtmpD0 contains real part of 4 consecutive outputs (32-bit)
        mmtmpD1 = _mm_shufflelo_epi16(dl_ch128[0],_MM_SHUFFLE(2,3,0,1));
        mmtmpD1 = _mm_shufflehi_epi16(mmtmpD1,_MM_SHUFFLE(2,3,0,1));
        mmtmpD1 = _mm_sign_epi16(mmtmpD1,*(__m128i*)&conjugate[0]);
        //  print_ints("im",&mmtmpD1);
        mmtmpD1 = _mm_madd_epi16(mmtmpD1,dl_ch128_2[0]);
        // mmtmpD1 contains imag part of 4 consecutive outputs (32-bit)
        mmtmpD0 = _mm_srai_epi32(mmtmpD0,output_shift);
        //  print_ints("re(shift)",&mmtmpD0);
        mmtmpD1 = _mm_srai_epi32(mmtmpD1,output_shift);
        //  print_ints("im(shift)",&mmtmpD1);
        mmtmpD2 = _mm_unpacklo_epi32(mmtmpD0,mmtmpD1);
        mmtmpD3 = _mm_unpackhi_epi32(mmtmpD0,mmtmpD1);
        //        print_ints("c0",&mmtmpD2);
        //  print_ints("c1",&mmtmpD3);
        rho128[0] = _mm_packs_epi32(mmtmpD2,mmtmpD3);

        //print_shorts("rx:",dl_ch128_2);
        //print_shorts("ch:",dl_ch128);
        //print_shorts("pack:",rho128);

        // multiply by conjugated channel
        mmtmpD0 = _mm_madd_epi16(dl_ch128[1],dl_ch128_2[1]);
        // mmtmpD0 contains real part of 4 consecutive outputs (32-bit)
        mmtmpD1 = _mm_shufflelo_epi16(dl_ch128[1],_MM_SHUFFLE(2,3,0,1));
        mmtmpD1 = _mm_shufflehi_epi16(mmtmpD1,_MM_SHUFFLE(2,3,0,1));
        mmtmpD1 = _mm_sign_epi16(mmtmpD1,*(__m128i*)conjugate);
        mmtmpD1 = _mm_madd_epi16(mmtmpD1,dl_ch128_2[1]);
        // mmtmpD1 contains imag part of 4 consecutive outputs (32-bit)
        mmtmpD0 = _mm_srai_epi32(mmtmpD0,output_shift);
        mmtmpD1 = _mm_srai_epi32(mmtmpD1,output_shift);
        mmtmpD2 = _mm_unpacklo_epi32(mmtmpD0,mmtmpD1);
        mmtmpD3 = _mm_unpackhi_epi32(mmtmpD0,mmtmpD1);


        rho128[1] =_mm_packs_epi32(mmtmpD2,mmtmpD3);
        //print_shorts("rx:",dl_ch128_2+1);
        //print_shorts("ch:",dl_ch128+1);
        //print_shorts("pack:",rho128+1);
        // multiply by conjugated channel
        mmtmpD0 = _mm_madd_epi16(dl_ch128[2],dl_ch128_2[2]);
        // mmtmpD0 contains real part of 4 consecutive outputs (32-bit)
        mmtmpD1 = _mm_shufflelo_epi16(dl_ch128[2],_MM_SHUFFLE(2,3,0,1));
        mmtmpD1 = _mm_shufflehi_epi16(mmtmpD1,_MM_SHUFFLE(2,3,0,1));
        mmtmpD1 = _mm_sign_epi16(mmtmpD1,*(__m128i*)conjugate);
        mmtmpD1 = _mm_madd_epi16(mmtmpD1,dl_ch128_2[2]);
        // mmtmpD1 contains imag part of 4 consecutive outputs (32-bit)
        mmtmpD0 = _mm_srai_epi32(mmtmpD0,output_shift);
        mmtmpD1 = _mm_srai_epi32(mmtmpD1,output_shift);
        mmtmpD2 = _mm_unpacklo_epi32(mmtmpD0,mmtmpD1);
        mmtmpD3 = _mm_unpackhi_epi32(mmtmpD0,mmtmpD1);

        rho128[2] = _mm_packs_epi32(mmtmpD2,mmtmpD3);
        //print_shorts("rx:",dl_ch128_2+2);
        //print_shorts("ch:",dl_ch128+2);
        //print_shorts("pack:",rho128+2);

        dl_ch128+=3;
        dl_ch128_2+=3;
        rho128+=3;

      }

      if (first_symbol_flag==1) {
        measurements->rx_correlation[0][aarx] = signal_energy(&rho[aarx][symbol*nb_rb*12],rb*12);
      }
    }
  }

  _mm_empty();
  _m_empty();

#elif defined(__arm__)


  unsigned short rb;
  unsigned char aatx,aarx,symbol_mod,pilots=0;

  int16x4_t *dl_ch128,*dl_ch128_2,*rxdataF128;
  int32x4_t mmtmpD0,mmtmpD1,mmtmpD0b,mmtmpD1b;
  int16x8_t *dl_ch_mag128,*dl_ch_mag128b,mmtmpD2,mmtmpD3,mmtmpD4;
  int16x8_t QAM_amp128,QAM_amp128b;
  int16x4x2_t *rxdataF_comp128,*rho128;

  int16_t conj[4]__attribute__((aligned(16))) = {1,-1,1,-1};
  int32x4_t output_shift128 = vmovq_n_s32(-(int32_t)output_shift);

  symbol_mod = (symbol>=(7-frame_parms->Ncp)) ? symbol-(7-frame_parms->Ncp) : symbol;

  if ((symbol_mod == 0) || (symbol_mod == (4-frame_parms->Ncp))) {
    if (frame_parms->nb_antenna_ports_eNB==1) { // 10 out of 12 so don't reduce size
      nb_rb=1+(5*nb_rb/6);
    }
    else {
      pilots=1;
    }
  }

  for (aatx=0; aatx<frame_parms->nb_antenna_ports_eNB; aatx++) {
    if (mod_order == 4) {
      QAM_amp128  = vmovq_n_s16(QAM16_n1);  // 2/sqrt(10)
      QAM_amp128b = vmovq_n_s16(0);
    } else if (mod_order == 6) {
      QAM_amp128  = vmovq_n_s16(QAM64_n1); //
      QAM_amp128b = vmovq_n_s16(QAM64_n2);
    }
    //    printf("comp: rxdataF_comp %p, symbol %d\n",rxdataF_comp[0],symbol);

    for (aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++) {
      dl_ch128          = (int16x4_t*)&dl_ch_estimates_ext[(aatx<<1)+aarx][symbol*frame_parms->N_RB_DL*12];
      dl_ch_mag128      = (int16x8_t*)&dl_ch_mag[(aatx<<1)+aarx][symbol*frame_parms->N_RB_DL*12];
      dl_ch_mag128b     = (int16x8_t*)&dl_ch_magb[(aatx<<1)+aarx][symbol*frame_parms->N_RB_DL*12];
      rxdataF128        = (int16x4_t*)&rxdataF_ext[aarx][symbol*frame_parms->N_RB_DL*12];
      rxdataF_comp128   = (int16x4x2_t*)&rxdataF_comp[(aatx<<1)+aarx][symbol*frame_parms->N_RB_DL*12];

      for (rb=0; rb<nb_rb; rb++) {
  if (mod_order>2) {
    // get channel amplitude if not QPSK
    mmtmpD0 = vmull_s16(dl_ch128[0], dl_ch128[0]);
    // mmtmpD0 = [ch0*ch0,ch1*ch1,ch2*ch2,ch3*ch3];
    mmtmpD0 = vqshlq_s32(vqaddq_s32(mmtmpD0,vrev64q_s32(mmtmpD0)),output_shift128);
    // mmtmpD0 = [ch0*ch0 + ch1*ch1,ch0*ch0 + ch1*ch1,ch2*ch2 + ch3*ch3,ch2*ch2 + ch3*ch3]>>output_shift128 on 32-bits
    mmtmpD1 = vmull_s16(dl_ch128[1], dl_ch128[1]);
    mmtmpD1 = vqshlq_s32(vqaddq_s32(mmtmpD1,vrev64q_s32(mmtmpD1)),output_shift128);
    mmtmpD2 = vcombine_s16(vmovn_s32(mmtmpD0),vmovn_s32(mmtmpD1));
    // mmtmpD2 = [ch0*ch0 + ch1*ch1,ch0*ch0 + ch1*ch1,ch2*ch2 + ch3*ch3,ch2*ch2 + ch3*ch3,ch4*ch4 + ch5*ch5,ch4*ch4 + ch5*ch5,ch6*ch6 + ch7*ch7,ch6*ch6 + ch7*ch7]>>output_shift128 on 16-bits
    mmtmpD0 = vmull_s16(dl_ch128[2], dl_ch128[2]);
    mmtmpD0 = vqshlq_s32(vqaddq_s32(mmtmpD0,vrev64q_s32(mmtmpD0)),output_shift128);
    mmtmpD1 = vmull_s16(dl_ch128[3], dl_ch128[3]);
    mmtmpD1 = vqshlq_s32(vqaddq_s32(mmtmpD1,vrev64q_s32(mmtmpD1)),output_shift128);
    mmtmpD3 = vcombine_s16(vmovn_s32(mmtmpD0),vmovn_s32(mmtmpD1));
    if (pilots==0) {
      mmtmpD0 = vmull_s16(dl_ch128[4], dl_ch128[4]);
      mmtmpD0 = vqshlq_s32(vqaddq_s32(mmtmpD0,vrev64q_s32(mmtmpD0)),output_shift128);
      mmtmpD1 = vmull_s16(dl_ch128[5], dl_ch128[5]);
      mmtmpD1 = vqshlq_s32(vqaddq_s32(mmtmpD1,vrev64q_s32(mmtmpD1)),output_shift128);
      mmtmpD4 = vcombine_s16(vmovn_s32(mmtmpD0),vmovn_s32(mmtmpD1));
    }

    dl_ch_mag128b[0] = vqdmulhq_s16(mmtmpD2,QAM_amp128b);
    dl_ch_mag128b[1] = vqdmulhq_s16(mmtmpD3,QAM_amp128b);
    dl_ch_mag128[0] = vqdmulhq_s16(mmtmpD2,QAM_amp128);
    dl_ch_mag128[1] = vqdmulhq_s16(mmtmpD3,QAM_amp128);

    if (pilots==0) {
      dl_ch_mag128b[2] = vqdmulhq_s16(mmtmpD4,QAM_amp128b);
      dl_ch_mag128[2]  = vqdmulhq_s16(mmtmpD4,QAM_amp128);
    }
  }

  mmtmpD0 = vmull_s16(dl_ch128[0], rxdataF128[0]);
  //mmtmpD0 = [Re(ch[0])Re(rx[0]) Im(ch[0])Im(ch[0]) Re(ch[1])Re(rx[1]) Im(ch[1])Im(ch[1])]
  mmtmpD1 = vmull_s16(dl_ch128[1], rxdataF128[1]);
  //mmtmpD1 = [Re(ch[2])Re(rx[2]) Im(ch[2])Im(ch[2]) Re(ch[3])Re(rx[3]) Im(ch[3])Im(ch[3])]
  mmtmpD0 = vcombine_s32(vpadd_s32(vget_low_s32(mmtmpD0),vget_high_s32(mmtmpD0)),
             vpadd_s32(vget_low_s32(mmtmpD1),vget_high_s32(mmtmpD1)));
  //mmtmpD0 = [Re(ch[0])Re(rx[0])+Im(ch[0])Im(ch[0]) Re(ch[1])Re(rx[1])+Im(ch[1])Im(ch[1]) Re(ch[2])Re(rx[2])+Im(ch[2])Im(ch[2]) Re(ch[3])Re(rx[3])+Im(ch[3])Im(ch[3])]

  mmtmpD0b = vmull_s16(vrev32_s16(vmul_s16(dl_ch128[0],*(int16x4_t*)conj)), rxdataF128[0]);
  //mmtmpD0 = [-Im(ch[0])Re(rx[0]) Re(ch[0])Im(rx[0]) -Im(ch[1])Re(rx[1]) Re(ch[1])Im(rx[1])]
  mmtmpD1b = vmull_s16(vrev32_s16(vmul_s16(dl_ch128[1],*(int16x4_t*)conj)), rxdataF128[1]);
  //mmtmpD0 = [-Im(ch[2])Re(rx[2]) Re(ch[2])Im(rx[2]) -Im(ch[3])Re(rx[3]) Re(ch[3])Im(rx[3])]
  mmtmpD1 = vcombine_s32(vpadd_s32(vget_low_s32(mmtmpD0b),vget_high_s32(mmtmpD0b)),
             vpadd_s32(vget_low_s32(mmtmpD1b),vget_high_s32(mmtmpD1b)));
  //mmtmpD1 = [-Im(ch[0])Re(rx[0])+Re(ch[0])Im(rx[0]) -Im(ch[1])Re(rx[1])+Re(ch[1])Im(rx[1]) -Im(ch[2])Re(rx[2])+Re(ch[2])Im(rx[2]) -Im(ch[3])Re(rx[3])+Re(ch[3])Im(rx[3])]

  mmtmpD0 = vqshlq_s32(mmtmpD0,output_shift128);
  mmtmpD1 = vqshlq_s32(mmtmpD1,output_shift128);
  rxdataF_comp128[0] = vzip_s16(vmovn_s32(mmtmpD0),vmovn_s32(mmtmpD1));
  mmtmpD0 = vmull_s16(dl_ch128[2], rxdataF128[2]);
  mmtmpD1 = vmull_s16(dl_ch128[3], rxdataF128[3]);
  mmtmpD0 = vcombine_s32(vpadd_s32(vget_low_s32(mmtmpD0),vget_high_s32(mmtmpD0)),
             vpadd_s32(vget_low_s32(mmtmpD1),vget_high_s32(mmtmpD1)));
  mmtmpD0b = vmull_s16(vrev32_s16(vmul_s16(dl_ch128[2],*(int16x4_t*)conj)), rxdataF128[2]);
  mmtmpD1b = vmull_s16(vrev32_s16(vmul_s16(dl_ch128[3],*(int16x4_t*)conj)), rxdataF128[3]);
  mmtmpD1 = vcombine_s32(vpadd_s32(vget_low_s32(mmtmpD0b),vget_high_s32(mmtmpD0b)),
             vpadd_s32(vget_low_s32(mmtmpD1b),vget_high_s32(mmtmpD1b)));
  mmtmpD0 = vqshlq_s32(mmtmpD0,output_shift128);
  mmtmpD1 = vqshlq_s32(mmtmpD1,output_shift128);
  rxdataF_comp128[1] = vzip_s16(vmovn_s32(mmtmpD0),vmovn_s32(mmtmpD1));

  if (pilots==0) {
    mmtmpD0 = vmull_s16(dl_ch128[4], rxdataF128[4]);
    mmtmpD1 = vmull_s16(dl_ch128[5], rxdataF128[5]);
    mmtmpD0 = vcombine_s32(vpadd_s32(vget_low_s32(mmtmpD0),vget_high_s32(mmtmpD0)),
         vpadd_s32(vget_low_s32(mmtmpD1),vget_high_s32(mmtmpD1)));

    mmtmpD0b = vmull_s16(vrev32_s16(vmul_s16(dl_ch128[4],*(int16x4_t*)conj)), rxdataF128[4]);
    mmtmpD1b = vmull_s16(vrev32_s16(vmul_s16(dl_ch128[5],*(int16x4_t*)conj)), rxdataF128[5]);
    mmtmpD1 = vcombine_s32(vpadd_s32(vget_low_s32(mmtmpD0b),vget_high_s32(mmtmpD0b)),
         vpadd_s32(vget_low_s32(mmtmpD1b),vget_high_s32(mmtmpD1b)));


    mmtmpD0 = vqshlq_s32(mmtmpD0,output_shift128);
    mmtmpD1 = vqshlq_s32(mmtmpD1,output_shift128);
    rxdataF_comp128[2] = vzip_s16(vmovn_s32(mmtmpD0),vmovn_s32(mmtmpD1));


    dl_ch128+=6;
    dl_ch_mag128+=3;
    dl_ch_mag128b+=3;
    rxdataF128+=6;
    rxdataF_comp128+=3;

  } else { // we have a smaller PDSCH in symbols with pilots so skip last group of 4 REs and increment less
    dl_ch128+=4;
    dl_ch_mag128+=2;
    dl_ch_mag128b+=2;
    rxdataF128+=4;
    rxdataF_comp128+=2;
  }
      }
    }
  }

  if (rho) {
    for (aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++) {
      rho128        = (int16x4x2_t*)&rho[aarx][symbol*frame_parms->N_RB_DL*12];
      dl_ch128      = (int16x4_t*)&dl_ch_estimates_ext[aarx][symbol*frame_parms->N_RB_DL*12];
      dl_ch128_2    = (int16x4_t*)&dl_ch_estimates_ext[2+aarx][symbol*frame_parms->N_RB_DL*12];
      for (rb=0; rb<nb_rb; rb++) {
  mmtmpD0 = vmull_s16(dl_ch128[0], dl_ch128_2[0]);
  mmtmpD1 = vmull_s16(dl_ch128[1], dl_ch128_2[1]);
  mmtmpD0 = vcombine_s32(vpadd_s32(vget_low_s32(mmtmpD0),vget_high_s32(mmtmpD0)),
             vpadd_s32(vget_low_s32(mmtmpD1),vget_high_s32(mmtmpD1)));
  mmtmpD0b = vmull_s16(vrev32_s16(vmul_s16(dl_ch128[0],*(int16x4_t*)conj)), dl_ch128_2[0]);
  mmtmpD1b = vmull_s16(vrev32_s16(vmul_s16(dl_ch128[1],*(int16x4_t*)conj)), dl_ch128_2[1]);
  mmtmpD1 = vcombine_s32(vpadd_s32(vget_low_s32(mmtmpD0b),vget_high_s32(mmtmpD0b)),
             vpadd_s32(vget_low_s32(mmtmpD1b),vget_high_s32(mmtmpD1b)));

  mmtmpD0 = vqshlq_s32(mmtmpD0,output_shift128);
  mmtmpD1 = vqshlq_s32(mmtmpD1,output_shift128);
  rho128[0] = vzip_s16(vmovn_s32(mmtmpD0),vmovn_s32(mmtmpD1));

  mmtmpD0 = vmull_s16(dl_ch128[2], dl_ch128_2[2]);
  mmtmpD1 = vmull_s16(dl_ch128[3], dl_ch128_2[3]);
  mmtmpD0 = vcombine_s32(vpadd_s32(vget_low_s32(mmtmpD0),vget_high_s32(mmtmpD0)),
             vpadd_s32(vget_low_s32(mmtmpD1),vget_high_s32(mmtmpD1)));
  mmtmpD0b = vmull_s16(vrev32_s16(vmul_s16(dl_ch128[2],*(int16x4_t*)conj)), dl_ch128_2[2]);
  mmtmpD1b = vmull_s16(vrev32_s16(vmul_s16(dl_ch128[3],*(int16x4_t*)conj)), dl_ch128_2[3]);
  mmtmpD1 = vcombine_s32(vpadd_s32(vget_low_s32(mmtmpD0b),vget_high_s32(mmtmpD0b)),
             vpadd_s32(vget_low_s32(mmtmpD1b),vget_high_s32(mmtmpD1b)));

  mmtmpD0 = vqshlq_s32(mmtmpD0,output_shift128);
  mmtmpD1 = vqshlq_s32(mmtmpD1,output_shift128);
  rho128[1] = vzip_s16(vmovn_s32(mmtmpD0),vmovn_s32(mmtmpD1));

  mmtmpD0 = vmull_s16(dl_ch128[0], dl_ch128_2[0]);
  mmtmpD1 = vmull_s16(dl_ch128[1], dl_ch128_2[1]);
  mmtmpD0 = vcombine_s32(vpadd_s32(vget_low_s32(mmtmpD0),vget_high_s32(mmtmpD0)),
             vpadd_s32(vget_low_s32(mmtmpD1),vget_high_s32(mmtmpD1)));
  mmtmpD0b = vmull_s16(vrev32_s16(vmul_s16(dl_ch128[4],*(int16x4_t*)conj)), dl_ch128_2[4]);
  mmtmpD1b = vmull_s16(vrev32_s16(vmul_s16(dl_ch128[5],*(int16x4_t*)conj)), dl_ch128_2[5]);
  mmtmpD1 = vcombine_s32(vpadd_s32(vget_low_s32(mmtmpD0b),vget_high_s32(mmtmpD0b)),
             vpadd_s32(vget_low_s32(mmtmpD1b),vget_high_s32(mmtmpD1b)));

  mmtmpD0 = vqshlq_s32(mmtmpD0,output_shift128);
  mmtmpD1 = vqshlq_s32(mmtmpD1,output_shift128);
  rho128[2] = vzip_s16(vmovn_s32(mmtmpD0),vmovn_s32(mmtmpD1));


  dl_ch128+=6;
  dl_ch128_2+=6;
  rho128+=3;
      }

      if (first_symbol_flag==1) {
  measurements->rx_correlation[0][aarx] = signal_energy(&rho[aarx][symbol*frame_parms->N_RB_DL*12],rb*12);
      }
    }
  }
#endif
}


void nr_dlsch_scale_channel(int **dl_ch_estimates_ext,
                         NR_DL_FRAME_PARMS *frame_parms,
                         NR_UE_DLSCH_t **dlsch_ue,
                         uint8_t symbol,
                         unsigned short nb_rb)
{

#if defined(__x86_64__)||defined(__i386__)

  short rb, ch_amp;
  unsigned char aatx,aarx,pilots=0,symbol_mod;
  __m128i *dl_ch128, ch_amp128;

  
  if (symbol==2){
	  nb_rb = nb_rb>>1;
	  pilots=1;
  }

  // Determine scaling amplitude based the symbol

ch_amp = 1024*8; //((pilots) ? (dlsch_ue[0]->sqrt_rho_b) : (dlsch_ue[0]->sqrt_rho_a));

    LOG_D(PHY,"Scaling PDSCH Chest in OFDM symbol %d by %d, pilots %d nb_rb %d NCP %d symbol %d\n",symbol_mod,ch_amp,pilots,nb_rb,frame_parms->Ncp,symbol);
   // printf("Scaling PDSCH Chest in OFDM symbol %d by %d\n",symbol_mod,ch_amp);

  ch_amp128 = _mm_set1_epi16(ch_amp); // Q3.13

  for (aatx=0; aatx<frame_parms->nb_antenna_ports_eNB; aatx++) {
    for (aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++) {

      dl_ch128=(__m128i *)&dl_ch_estimates_ext[(aatx<<1)+aarx][symbol*nb_rb*12];

      for (rb=0;rb<nb_rb;rb++) {

        dl_ch128[0] = _mm_mulhi_epi16(dl_ch128[0],ch_amp128);
        dl_ch128[0] = _mm_slli_epi16(dl_ch128[0],3);

        dl_ch128[1] = _mm_mulhi_epi16(dl_ch128[1],ch_amp128);
        dl_ch128[1] = _mm_slli_epi16(dl_ch128[1],3);

        if (pilots) {
          dl_ch128+=2;
        } else {
          dl_ch128[2] = _mm_mulhi_epi16(dl_ch128[2],ch_amp128);
          dl_ch128[2] = _mm_slli_epi16(dl_ch128[2],3);
          dl_ch128+=3;

        }
      }
    }
  }

#elif defined(__arm__)

#endif
}


//compute average channel_level on each (TX,RX) antenna pair
void nr_dlsch_channel_level(int **dl_ch_estimates_ext,
                         NR_DL_FRAME_PARMS *frame_parms,
                         int32_t *avg,
                         uint8_t symbol,
                         unsigned short nb_rb)
{

#if defined(__x86_64__)||defined(__i386__)

  short rb;
  unsigned char aatx,aarx,nre=12;
  __m128i *dl_ch128, avg128D;

    if (symbol==2) //assume start symbol 2
      nre=6;
    else
      nre=12;

  //nb_rb*nre = y * 2^x
  int16_t x = factor2(nb_rb*nre);
  int16_t y = (nb_rb*nre)>>x;
  //printf("nb_rb*nre = %d = %d * 2^(%d)\n",nb_rb*nre,y,x);

  for (aatx=0; aatx<frame_parms->nb_antenna_ports_eNB; aatx++)
    for (aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++) {
      //clear average level
      avg128D = _mm_setzero_si128();
      // 5 is always a symbol with no pilots for both normal and extended prefix

      dl_ch128=(__m128i *)&dl_ch_estimates_ext[(aatx<<1)+aarx][symbol*nb_rb*12];

      for (rb=0;rb<nb_rb;rb++) {
        //      printf("rb %d : ",rb);
        //      print_shorts("ch",&dl_ch128[0]);
	avg128D = _mm_add_epi32(avg128D,_mm_srai_epi16(_mm_madd_epi16(dl_ch128[0],dl_ch128[0]),x));
	avg128D = _mm_add_epi32(avg128D,_mm_srai_epi16(_mm_madd_epi16(dl_ch128[1],dl_ch128[1]),x));

        //avg128D = _mm_add_epi32(avg128D,_mm_madd_epi16(dl_ch128[0],_mm_srai_epi16(_mm_mulhi_epi16(dl_ch128[0], coeff128),15)));
        //avg128D = _mm_add_epi32(avg128D,_mm_madd_epi16(dl_ch128[1],_mm_srai_epi16(_mm_mulhi_epi16(dl_ch128[1], coeff128),15)));

	    /*if (((symbol_mod == 0) || (symbol_mod == (frame_parms->Ncp-1)))&&(frame_parms->nb_antenna_ports_eNB!=1)) {
          dl_ch128+=2;
        }
        else {*/
	  avg128D = _mm_add_epi32(avg128D,_mm_srai_epi16(_mm_madd_epi16(dl_ch128[2],dl_ch128[2]),x));
          //avg128D = _mm_add_epi32(avg128D,_mm_madd_epi16(dl_ch128[2],_mm_srai_epi16(_mm_mulhi_epi16(dl_ch128[2], coeff128),15)));
          dl_ch128+=3;
       // }
        /*
          if (rb==0) {
          print_shorts("dl_ch128",&dl_ch128[0]);
          print_shorts("dl_ch128",&dl_ch128[1]);
          print_shorts("dl_ch128",&dl_ch128[2]);
          }
        */
      }

      avg[(aatx<<1)+aarx] =(((int32_t*)&avg128D)[0] +
                            ((int32_t*)&avg128D)[1] +
                            ((int32_t*)&avg128D)[2] +
			      ((int32_t*)&avg128D)[3])/y;
                //  printf("Channel level : %d\n",avg[(aatx<<1)+aarx]);
    }

  _mm_empty();
  _m_empty();

#elif defined(__arm__)

  short rb;
  unsigned char aatx,aarx,nre=12,symbol_mod;
  int32x4_t avg128D;
  int16x4_t *dl_ch128;

  symbol_mod = (symbol>=(7-frame_parms->Ncp)) ? symbol-(7-frame_parms->Ncp) : symbol;

  for (aatx=0; aatx<frame_parms->nb_antenna_ports_eNB; aatx++)
    for (aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++) {
      //clear average level
      avg128D = vdupq_n_s32(0);
      // 5 is always a symbol with no pilots for both normal and extended prefix

      dl_ch128=(int16x4_t *)&dl_ch_estimates_ext[(aatx<<1)+aarx][symbol*frame_parms->N_RB_DL*12];

      for (rb=0; rb<nb_rb; rb++) {
        //  printf("rb %d : ",rb);
        //  print_shorts("ch",&dl_ch128[0]);
        avg128D = vqaddq_s32(avg128D,vmull_s16(dl_ch128[0],dl_ch128[0]));
        avg128D = vqaddq_s32(avg128D,vmull_s16(dl_ch128[1],dl_ch128[1]));
        avg128D = vqaddq_s32(avg128D,vmull_s16(dl_ch128[2],dl_ch128[2]));
        avg128D = vqaddq_s32(avg128D,vmull_s16(dl_ch128[3],dl_ch128[3]));

        if (((symbol_mod == 0) || (symbol_mod == (frame_parms->Ncp-1)))&&(frame_parms->nb_antenna_ports_eNB!=1)) {
          dl_ch128+=4;
        } else {
          avg128D = vqaddq_s32(avg128D,vmull_s16(dl_ch128[4],dl_ch128[4]));
          avg128D = vqaddq_s32(avg128D,vmull_s16(dl_ch128[5],dl_ch128[5]));
          dl_ch128+=6;
        }

        /*
          if (rb==0) {
          print_shorts("dl_ch128",&dl_ch128[0]);
          print_shorts("dl_ch128",&dl_ch128[1]);
          print_shorts("dl_ch128",&dl_ch128[2]);
          }
        */
      }

      if (symbol==2) //assume start symbol 2
          nre=6;
      else
          nre=12;

      avg[(aatx<<1)+aarx] = (((int32_t*)&avg128D)[0] +
                             ((int32_t*)&avg128D)[1] +
                             ((int32_t*)&avg128D)[2] +
                             ((int32_t*)&avg128D)[3])/(nb_rb*nre);

      //            printf("Channel level : %d\n",avg[(aatx<<1)+aarx]);
    }


#endif
}

//==============================================================================================
// Extraction functions
//==============================================================================================

unsigned short nr_dlsch_extract_rbs_single(int **rxdataF,
                                        int **dl_ch_estimates,
                                        int **rxdataF_ext,
                                        int **dl_ch_estimates_ext,
                                        unsigned short pmi,
                                        unsigned char *pmi_ext,
                                        unsigned int *rb_alloc,
                                        unsigned char symbol,
										unsigned short start_rb,
										unsigned short nb_rb_pdsch,
                                        unsigned char nr_tti_rx,
                                        uint32_t high_speed_flag,
                                        NR_DL_FRAME_PARMS *frame_parms) {



  unsigned short k,rb;
  unsigned char i,aarx; //,nsymb,sss_symb,pss_symb=0,l;
  int *dl_ch0,*dl_ch0_ext,*rxF,*rxF_ext;



  unsigned char pilots=0,j=0;

  //symbol_mod = (symbol>=(7-frame_parms->Ncp)) ? symbol-(7-frame_parms->Ncp) : symbol;
  pilots = (symbol==2) ? 1 : 0; //to updated from config
  //l=symbol;
  //nsymb = (frame_parms->Ncp==NORMAL) ? 14:12;
  k = frame_parms->first_carrier_offset + 516; //0

 /* if (frame_parms->frame_type == TDD) {  // TDD
    sss_symb = nsymb-1;
    pss_symb = 2;
  } else {
    sss_symb = (nsymb>>1)-2;
    pss_symb = (nsymb>>1)-1;
  }*/


  for (aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++) {

    if (high_speed_flag == 1)
      dl_ch0     = &dl_ch_estimates[aarx][(2*(frame_parms->ofdm_symbol_size))];
    else
      dl_ch0     = &dl_ch_estimates[aarx][0];

    dl_ch0_ext = &dl_ch_estimates_ext[aarx][symbol*(nb_rb_pdsch*12)];

    rxF_ext   = &rxdataF_ext[aarx][symbol*(nb_rb_pdsch*12)];
    rxF       = &rxdataF[aarx][(k+(symbol*(frame_parms->ofdm_symbol_size)))];
    
    //printf("extract single addr %p pilots %d symbol %d, l %d RB_DL %d nushift %d k %d\n", rxF, pilots, symbol, l, frame_parms->N_RB_DL,frame_parms->nushift,k);

    //if ((frame_parms->N_RB_DL&1) == 0){  // even number of RBs

      for (rb=start_rb;rb<nb_rb_pdsch;rb++) {

        // For second half of RBs skip DC carrier
        if (k>=frame_parms->ofdm_symbol_size) {
          rxF       = &rxdataF[aarx][(symbol*(frame_parms->ofdm_symbol_size))];
          k=k-(frame_parms->ofdm_symbol_size);
        }

        //*pmi_ext = (pmi>>((rb>>2)<<1))&3;
        //memcpy(dl_ch0_ext,dl_ch0,12*sizeof(int));

          /*
            printf("rb %d\n",rb);
            for (i=0;i<12;i++)
            printf("(%d %d)",((short *)dl_ch0)[i<<1],((short*)dl_ch0)[1+(i<<1)]);
            printf("\n");
          */
        if (pilots==0) {
            for (i=0; i<12; i++) {
              rxF_ext[i]=rxF[i];
              dl_ch0_ext[i]=dl_ch0[i];
              /*printf("rb %ld %p: (%d,%d)\n",(rxF+i-&rxdataF[aarx][( (symbol*(frame_parms->ofdm_symbol_size)))]),rxF,
                ((short*)&rxF[i])[0],((short*)&rxF[i])[1]);
              printf("rb %ld %p: dl_ch (%d,%d)\n",(rxF+i-&rxdataF[aarx][( (symbol*(frame_parms->ofdm_symbol_size)))]),rxF,
                ((short*)&dl_ch0[i])[0],((short*)&dl_ch0[i])[1]);
                printf("re count %d\n",rb*12+i);*/
            }

            dl_ch0_ext+=12;
            rxF_ext+=12;
        } else {
            j=0;

            for (i=0; i<12; i++) {
              if ((i&1)!=frame_parms->nushift){
                rxF_ext[j]=rxF[i];
                     //printf("extract rb %d, re %d => rxF (%d,%d) rxF ext (%d,%d) addr %p\n",rb,i,*(short *)&rxF[i],*(1+(short*)&rxF[i]),*(short *)&rxF_ext[j],*(1+(short*)&rxF_ext[j]),rxF_ext);
                dl_ch0_ext[j]=dl_ch0[i];
					//printf("rb %ld %p: dl_ch (%d,%d)\n",(rxF+i-&rxdataF[aarx][( (symbol*(frame_parms->ofdm_symbol_size)))]),rxF,
                //((short*)&dl_ch0[i])[0],((short*)&dl_ch0[i])[1]);
                j++;
              }
            }

            dl_ch0_ext+=6;
            rxF_ext+=6;
        }

        dl_ch0+=12;
        rxF+=12;
        k+=12;
      }
  }


  return(nb_rb_pdsch/frame_parms->nb_antennas_rx);
}

//==============================================================================================

#ifdef USER_MODE


void dump_dlsch2(PHY_VARS_UE *ue,uint8_t eNB_id,uint8_t nr_tti_rx,unsigned int *coded_bits_per_codeword,int round,  unsigned char harq_pid)
{
  unsigned int nsymb = (ue->frame_parms.Ncp == 0) ? 14 : 12;
  char fname[32],vname[32];
  int N_RB_DL=ue->frame_parms.N_RB_DL;

  sprintf(fname,"dlsch%d_rxF_r%d_ext0.m",eNB_id,round);
  sprintf(vname,"dl%d_rxF_r%d_ext0",eNB_id,round);
  write_output(fname,vname,ue->pdsch_vars[ue->current_thread_id[nr_tti_rx]][eNB_id]->rxdataF_ext[0],12*N_RB_DL*nsymb,1,1);

  if (ue->frame_parms.nb_antennas_rx >1) {
    sprintf(fname,"dlsch%d_rxF_r%d_ext1.m",eNB_id,round);
    sprintf(vname,"dl%d_rxF_r%d_ext1",eNB_id,round);
    write_output(fname,vname,ue->pdsch_vars[ue->current_thread_id[nr_tti_rx]][eNB_id]->rxdataF_ext[1],12*N_RB_DL*nsymb,1,1);
  }

  sprintf(fname,"dlsch%d_ch_r%d_ext00.m",eNB_id,round);
  sprintf(vname,"dl%d_ch_r%d_ext00",eNB_id,round);
  write_output(fname,vname,ue->pdsch_vars[ue->current_thread_id[nr_tti_rx]][eNB_id]->dl_ch_estimates_ext[0],12*N_RB_DL*nsymb,1,1);

  if (ue->transmission_mode[eNB_id]==7){
    sprintf(fname,"dlsch%d_bf_ch_r%d.m",eNB_id,round);
    sprintf(vname,"dl%d_bf_ch_r%d",eNB_id,round);
    write_output(fname,vname,ue->pdsch_vars[ue->current_thread_id[nr_tti_rx]][eNB_id]->dl_bf_ch_estimates[0],512*nsymb,1,1);
    //write_output(fname,vname,phy_vars_ue->lte_ue_pdsch_vars[eNB_id]->dl_bf_ch_estimates[0],512,1,1);

    sprintf(fname,"dlsch%d_bf_ch_r%d_ext00.m",eNB_id,round);
    sprintf(vname,"dl%d_bf_ch_r%d_ext00",eNB_id,round);
    write_output(fname,vname,ue->pdsch_vars[ue->current_thread_id[nr_tti_rx]][eNB_id]->dl_bf_ch_estimates_ext[0],12*N_RB_DL*nsymb,1,1);
  }

  if (ue->frame_parms.nb_antennas_rx == 2) {
    sprintf(fname,"dlsch%d_ch_r%d_ext01.m",eNB_id,round);
    sprintf(vname,"dl%d_ch_r%d_ext01",eNB_id,round);
    write_output(fname,vname,ue->pdsch_vars[ue->current_thread_id[nr_tti_rx]][eNB_id]->dl_ch_estimates_ext[1],12*N_RB_DL*nsymb,1,1);
  }

  if (ue->frame_parms.nb_antenna_ports_eNB == 2) {
    sprintf(fname,"dlsch%d_ch_r%d_ext10.m",eNB_id,round);
    sprintf(vname,"dl%d_ch_r%d_ext10",eNB_id,round);
    write_output(fname,vname,ue->pdsch_vars[ue->current_thread_id[nr_tti_rx]][eNB_id]->dl_ch_estimates_ext[2],12*N_RB_DL*nsymb,1,1);

    if (ue->frame_parms.nb_antennas_rx == 2) {
      sprintf(fname,"dlsch%d_ch_r%d_ext11.m",eNB_id,round);
      sprintf(vname,"dl%d_ch_r%d_ext11",eNB_id,round);
      write_output(fname,vname,ue->pdsch_vars[ue->current_thread_id[nr_tti_rx]][eNB_id]->dl_ch_estimates_ext[3],12*N_RB_DL*nsymb,1,1);
    }
  }

  sprintf(fname,"dlsch%d_rxF_r%d_uespec0.m",eNB_id,round);
  sprintf(vname,"dl%d_rxF_r%d_uespec0",eNB_id,round);
  write_output(fname,vname,ue->pdsch_vars[ue->current_thread_id[nr_tti_rx]][eNB_id]->rxdataF_uespec_pilots[0],12*N_RB_DL,1,1);

  /*
    write_output("dlsch%d_ch_ext01.m","dl01_ch0_ext",pdsch_vars[eNB_id]->dl_ch_estimates_ext[1],12*N_RB_DL*nsymb,1,1);
    write_output("dlsch%d_ch_ext10.m","dl10_ch0_ext",pdsch_vars[eNB_id]->dl_ch_estimates_ext[2],12*N_RB_DL*nsymb,1,1);
    write_output("dlsch%d_ch_ext11.m","dl11_ch0_ext",pdsch_vars[eNB_id]->dl_ch_estimates_ext[3],12*N_RB_DL*nsymb,1,1);
  */
  sprintf(fname,"dlsch%d_r%d_rho.m",eNB_id,round);
  sprintf(vname,"dl_rho_r%d_%d",eNB_id,round);

  write_output(fname,vname,ue->pdsch_vars[ue->current_thread_id[nr_tti_rx]][eNB_id]->dl_ch_rho_ext[harq_pid][round][0],12*N_RB_DL*nsymb,1,1);

  sprintf(fname,"dlsch%d_r%d_rho2.m",eNB_id,round);
  sprintf(vname,"dl_rho2_r%d_%d",eNB_id,round);

  write_output(fname,vname,ue->pdsch_vars[ue->current_thread_id[nr_tti_rx]][eNB_id]->dl_ch_rho2_ext[0],12*N_RB_DL*nsymb,1,1);

  sprintf(fname,"dlsch%d_rxF_r%d_comp0.m",eNB_id,round);
  sprintf(vname,"dl%d_rxF_r%d_comp0",eNB_id,round);
  write_output(fname,vname,ue->pdsch_vars[ue->current_thread_id[nr_tti_rx]][eNB_id]->rxdataF_comp0[0],12*N_RB_DL*nsymb,1,1);
  if (ue->frame_parms.nb_antenna_ports_eNB == 2) {
    sprintf(fname,"dlsch%d_rxF_r%d_comp1.m",eNB_id,round);
    sprintf(vname,"dl%d_rxF_r%d_comp1",eNB_id,round);
    write_output(fname,vname,ue->pdsch_vars[ue->current_thread_id[nr_tti_rx]][eNB_id]->rxdataF_comp1[harq_pid][round][0],12*N_RB_DL*nsymb,1,1);
  }

  sprintf(fname,"dlsch%d_rxF_r%d_llr.m",eNB_id,round);
  sprintf(vname,"dl%d_r%d_llr",eNB_id,round);
  write_output(fname,vname, ue->pdsch_vars[ue->current_thread_id[nr_tti_rx]][eNB_id]->llr[0],coded_bits_per_codeword[0],1,0);
  sprintf(fname,"dlsch%d_r%d_mag1.m",eNB_id,round);
  sprintf(vname,"dl%d_r%d_mag1",eNB_id,round);
  write_output(fname,vname,ue->pdsch_vars[ue->current_thread_id[nr_tti_rx]][eNB_id]->dl_ch_mag0[0],12*N_RB_DL*nsymb,1,1);
  sprintf(fname,"dlsch%d_r%d_mag2.m",eNB_id,round);
  sprintf(vname,"dl%d_r%d_mag2",eNB_id,round);
  write_output(fname,vname,ue->pdsch_vars[ue->current_thread_id[nr_tti_rx]][eNB_id]->dl_ch_magb0[0],12*N_RB_DL*nsymb,1,1);

  //  printf("log2_maxh = %d\n",ue->pdsch_vars[eNB_id]->log2_maxh);
}
#endif

#ifdef DEBUG_DLSCH_DEMOD
/*
void print_bytes(char *s,__m128i *x)
{

  char *tempb = (char *)x;

  printf("%s  : %d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",s,
         tempb[0],tempb[1],tempb[2],tempb[3],tempb[4],tempb[5],tempb[6],tempb[7],
         tempb[8],tempb[9],tempb[10],tempb[11],tempb[12],tempb[13],tempb[14],tempb[15]
         );

}

void print_shorts(char *s,__m128i *x)
{

  short *tempb = (short *)x;
  printf("%s  : %d,%d,%d,%d,%d,%d,%d,%d\n",s,
         tempb[0],tempb[1],tempb[2],tempb[3],tempb[4],tempb[5],tempb[6],tempb[7]);

}

void print_shorts2(char *s,__m64 *x)
{

  short *tempb = (short *)x;
  printf("%s  : %d,%d,%d,%d\n",s,
         tempb[0],tempb[1],tempb[2],tempb[3]);

}

void print_ints(char *s,__m128i *x)
{

  int *tempb = (int *)x;
  printf("%s  : %d,%d,%d,%d\n",s,
         tempb[0],tempb[1],tempb[2],tempb[3]);

}*/
#endif
