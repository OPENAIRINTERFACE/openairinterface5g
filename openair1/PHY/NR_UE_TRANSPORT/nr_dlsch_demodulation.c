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
#include "PHY/NR_TRANSPORT/nr_transport_proto.h"
#include "nr_transport_proto_ue.h"
#include "PHY/sse_intrin.h"
#include "T.h"
#include "openair1/PHY/NR_UE_ESTIMATION/nr_estimation.h"
#include "openair1/PHY/NR_TRANSPORT/nr_dlsch.h"
#include "PHY/NR_REFSIG/nr_refsig.h"
#include "PHY/NR_REFSIG/dmrs_nr.h"
#include "common/utils/nr/nr_common.h"
#include <complex.h>

/* dynamic shift for LLR computation for TM3/4
 * set as command line argument, see lte-softmodem.c
 * default value: 0
 */
int32_t nr_dlsch_demod_shift = 0;
//int16_t interf_unaw_shift = 13;

//#define DEBUG_HARQ
//#define DEBUG_PHY
//#define DEBUG_DLSCH_DEMOD
//#define DEBUG_PDSCH_RX

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

#define print_ints(s,x) printf("%s = %d %d %d %d\n",s,(x)[0],(x)[1],(x)[2],(x)[3])
#define print_shorts(s,x) printf("%s = [%d+j*%d, %d+j*%d, %d+j*%d, %d+j*%d]\n",s,(x)[0],(x)[1],(x)[2],(x)[3],(x)[4],(x)[5],(x)[6],(x)[7])

/* compute H_h_H matrix inversion up to 4x4 matrices */
uint8_t nr_zero_forcing_rx(int **rxdataF_comp,
                                   int **dl_ch_mag,
                                   int **dl_ch_magb,
                                   int **dl_ch_magr,
                                   int **dl_ch_estimates_ext,
                                   unsigned short nb_rb,
                                   unsigned char n_rx,
                                   unsigned char n_tx,//number of layer
                                   unsigned char mod_order,
                                   int shift,
                                   unsigned char symbol,
                                   int length);

/* Apply layer demapping */
static void nr_dlsch_layer_demapping(int16_t **llr_cw,
                                     uint8_t Nl,
                                     uint8_t mod_order,
                                     uint32_t length,
                                     int32_t codeword_TB0,
                                     int32_t codeword_TB1,
                                     int16_t **llr_layers);

/* compute LLR */
static int nr_dlsch_llr(NR_UE_PDSCH **pdsch_vars,
                        NR_DL_FRAME_PARMS *frame_parms,
                        int32_t **rxdataF_comp_ptr,
                        int32_t **dl_ch_mag_ptr,
                        NR_DL_UE_HARQ_t *dlsch0_harq,
                        NR_DL_UE_HARQ_t *dlsch1_harq,
                        RX_type_t rx_type,
                        unsigned char harq_pid,
                        unsigned char gNB_id,
                        unsigned char gNB_id_i,
                        unsigned char first_symbol_flag,
                        unsigned char symbol,
                        unsigned short nb_rb,
                        int32_t codeword_TB0,
                        int32_t codeword_TB1,
                        uint32_t len,
                        uint8_t nr_slot_rx,
                        uint8_t beamforming_mode);


/* Main Function */
int nr_rx_pdsch(PHY_VARS_NR_UE *ue,
                UE_nr_rxtx_proc_t *proc,
                PDSCH_t type,
                unsigned char gNB_id,
                unsigned char gNB_id_i,
                uint32_t frame,
                uint8_t nr_slot_rx,
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

  int avg[16];
//  int avg_0[2];
//  int avg_1[2];

  uint8_t slot = 0;

  unsigned char aatx=0,aarx=0;

  int avgs = 0;// rb;
  NR_DL_UE_HARQ_t *dlsch0_harq, *dlsch1_harq = NULL;

  uint8_t beamforming_mode = 0;

  int32_t **rxdataF_comp_ptr;
  int32_t **dl_ch_mag_ptr;
  int32_t codeword_TB0 = -1;
  int32_t codeword_TB1 = -1;

  //to be updated higher layer
  unsigned short start_rb = 0;
  unsigned short nb_rb_pdsch = 50;
  //int16_t  *pllr_symbol_cw0_deint;
  //int16_t  *pllr_symbol_cw1_deint;
  //uint16_t bundle_L = 2;
  int32_t median[16];
  uint32_t nb_re_pdsch;
  uint16_t startSymbIdx=0;
  uint16_t nbSymb=0;
  uint16_t pduBitmap=0x0;

  switch (type) {
  case SI_PDSCH:
    pdsch_vars = ue->pdsch_vars[proc->thread_id];
    dlsch = &ue->dlsch_SI[gNB_id];
    dlsch0_harq = dlsch[0]->harq_processes[harq_pid];

    break;

  case RA_PDSCH:
    pdsch_vars = ue->pdsch_vars[proc->thread_id];
    dlsch = &ue->dlsch_ra[gNB_id];
    dlsch0_harq = dlsch[0]->harq_processes[harq_pid];

    break;

  case PDSCH:
    pdsch_vars = ue->pdsch_vars[proc->thread_id];
    dlsch = ue->dlsch[proc->thread_id][gNB_id];
    dlsch0_harq = dlsch[0]->harq_processes[harq_pid];
    if (NR_MAX_NB_LAYERS>4)
      dlsch1_harq = dlsch[1]->harq_processes[harq_pid];
    break;

  default:
    LOG_E(PHY, "[UE][FATAL] nr_slot_rx %d: Unknown PDSCH format %d\n", nr_slot_rx, type);
    return -1;
    break;
  }

  if (dlsch0_harq && dlsch1_harq){

    LOG_D(PHY,"AbsSubframe %d.%d / Sym %d harq_pid %d, harq status %d.%d \n", frame, nr_slot_rx, symbol, harq_pid, dlsch0_harq->status, dlsch1_harq->status);

    if ((dlsch0_harq->status == ACTIVE) && (dlsch1_harq->status == ACTIVE)){
      codeword_TB0 = dlsch0_harq->codeword;
      codeword_TB1 = dlsch1_harq->codeword;
      dlsch0_harq = dlsch[codeword_TB0]->harq_processes[harq_pid];
      dlsch1_harq = dlsch[codeword_TB1]->harq_processes[harq_pid];

      #ifdef DEBUG_HARQ
        printf("[DEMOD] I am assuming both TBs are active, in cw0 %d and cw1 %d \n", codeword_TB0, codeword_TB1);
      #endif

    } else if ((dlsch0_harq->status == ACTIVE) && (dlsch1_harq->status != ACTIVE) ) {
      codeword_TB0 = dlsch0_harq->codeword;
      dlsch0_harq = dlsch[codeword_TB0]->harq_processes[harq_pid];
      dlsch1_harq = NULL;

      #ifdef DEBUG_HARQ
        printf("[DEMOD] I am assuming only TB0 is active, in cw %d \n", codeword_TB0);
      #endif

    } else if ((dlsch0_harq->status != ACTIVE) && (dlsch1_harq->status == ACTIVE)){
      codeword_TB1 = dlsch1_harq->codeword;
      dlsch0_harq  = NULL;
      dlsch1_harq  = dlsch[codeword_TB1]->harq_processes[harq_pid];

      #ifdef DEBUG_HARQ
        printf("[DEMOD] I am assuming only TB1 is active, it is in cw %d\n", codeword_TB1);
      #endif

      LOG_E(PHY, "[UE][FATAL] DLSCH: TB0 not active and TB1 active case is not supported\n");
      return -1;

    } else {
      LOG_E(PHY,"[UE][FATAL] nr_slot_rx %d: no active DLSCH\n", nr_slot_rx);
      return(-1);
    }
  } else if (dlsch0_harq) {
    if (dlsch0_harq->status == ACTIVE) {
      codeword_TB0 = dlsch0_harq->codeword;
      dlsch0_harq = dlsch[0]->harq_processes[harq_pid];

      #ifdef DEBUG_HARQ
        printf("[DEMOD] I am assuming only TB0 is active\n");
      #endif
    } else {
      LOG_E(PHY,"[UE][FATAL] nr_slot_rx %d: no active DLSCH\n", nr_slot_rx);
      return (-1);
    }
  } else {
    LOG_E(PHY, "Done\n");
    return -1;
  }

  #ifdef DEBUG_HARQ
    printf("[DEMOD] MIMO mode = %d\n", dlsch0_harq->mimo_mode);
    printf("[DEMOD] cw for TB0 = %d, cw for TB1 = %d\n", codeword_TB0, codeword_TB1);
  #endif

  start_rb = dlsch0_harq->start_rb;
  nb_rb_pdsch =  dlsch0_harq->nb_rb;

  DevAssert(dlsch0_harq);


  if (gNB_id > 2) {
    LOG_W(PHY, "In %s: Illegal gNB_id %d\n", __FUNCTION__, gNB_id);
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

  if(symbol > ue->frame_parms.symbols_per_slot>>1)
  {
      slot = 1;
  }

  uint8_t pilots = (dlsch0_harq->dlDmrsSymbPos >> symbol) & 1;
  uint8_t config_type = dlsch0_harq->dmrsConfigType;
  //----------------------------------------------------------
  //--------------------- RBs extraction ---------------------
  //----------------------------------------------------------
  start_meas(&ue->generic_stat_bis[proc->thread_id][slot]);
  nr_dlsch_extract_rbs(common_vars->common_vars_rx_data_per_thread[proc->thread_id].rxdataF,
                       pdsch_vars[gNB_id]->dl_ch_estimates,
                       pdsch_vars[gNB_id]->rxdataF_ext,
                       pdsch_vars[gNB_id]->dl_ch_estimates_ext,
                       symbol,
                       pilots,
                       config_type,
                       start_rb + dlsch0_harq->BWPStart,
                       nb_rb_pdsch,
                       dlsch0_harq->n_dmrs_cdm_groups,
                       dlsch0_harq->Nl,
                       frame_parms,
                       dlsch0_harq->dlDmrsSymbPos,
                       ue->chest_time);
  stop_meas(&ue->generic_stat_bis[proc->thread_id][slot]);
  if (cpumeas(CPUMEAS_GETSTATE))
    LOG_D(PHY, "[AbsSFN %u.%d] Slot%d Symbol %d type %d: Pilot/Data extraction %5.2f \n",
	  frame,nr_slot_rx,slot,symbol,type,ue->generic_stat_bis[proc->thread_id][slot].p_time/(cpuf*1000.0));

  int nl = dlsch0_harq->Nl;
  int n_rx = frame_parms->nb_antennas_rx;
  nb_re_pdsch = (pilots==1)? ((config_type==NFAPI_NR_DMRS_TYPE1)?nb_rb_pdsch*(12-6*dlsch0_harq->n_dmrs_cdm_groups): nb_rb_pdsch*(12-4*dlsch0_harq->n_dmrs_cdm_groups)) : (nb_rb_pdsch*12);
  //----------------------------------------------------------
  //--------------------- Channel Scaling --------------------
  //----------------------------------------------------------
  start_meas(&ue->generic_stat_bis[proc->thread_id][slot]);
  nr_dlsch_scale_channel(pdsch_vars[gNB_id]->dl_ch_estimates_ext,
                         frame_parms,
                         nl,
                         n_rx,
                         dlsch,
                         symbol,
                         pilots,
                         nb_re_pdsch,
                         nb_rb_pdsch);
  stop_meas(&ue->generic_stat_bis[proc->thread_id][slot]);

  if (cpumeas(CPUMEAS_GETSTATE))
    LOG_D(PHY, "[AbsSFN %u.%d] Slot%d Symbol %d: Channel Scale  %5.2f \n",
          frame,nr_slot_rx,slot,symbol,ue->generic_stat_bis[proc->thread_id][slot].p_time/(cpuf*1000.0));

  //----------------------------------------------------------
  //--------------------- Channel Level Calc. ----------------
  //----------------------------------------------------------
  start_meas(&ue->generic_stat_bis[proc->thread_id][slot]);
  if (first_symbol_flag==1) {
    nr_dlsch_channel_level(pdsch_vars[gNB_id]->dl_ch_estimates_ext,
                           frame_parms,
                           nl,
                           avg,
                           symbol,
                           nb_re_pdsch,
                           nb_rb_pdsch);
    avgs = 0;
    for (aatx=0;aatx<nl;aatx++)
      for (aarx=0;aarx<n_rx;aarx++) {
        //LOG_I(PHY, "nb_rb %d len %d avg_%d_%d Power per SC is %d\n",nb_rb, len,aarx, aatx,avg[aatx*n_rx+aarx]);
        avgs = cmax(avgs,avg[(aatx*n_rx)+aarx]);
        //LOG_I(PHY, "avgs Power per SC is %d\n", avgs);
        median[(aatx*n_rx)+aarx] = avg[(aatx*n_rx)+aarx];
      }
    if (dlsch0_harq->Nl > 1) {
      nr_dlsch_channel_level_median(pdsch_vars[gNB_id]->dl_ch_estimates_ext,
                                    median,
                                    nl,
                                    n_rx,
                                    nb_re_pdsch,
                                    symbol*nb_rb_pdsch*12);
      for (aatx = 0; aatx < nl; aatx++) {
        for (aarx = 0; aarx < n_rx; aarx++) {
          avgs = cmax(avgs, median[aatx*n_rx + aarx]);
        }
      }
    }
    pdsch_vars[gNB_id]->log2_maxh = (log2_approx(avgs)/2) + 1;
    //LOG_I(PHY, "avgs Power per SC is %d lg2_maxh %d\n", avgs,  pdsch_vars[gNB_id]->log2_maxh);
    LOG_D(PHY,"[DLSCH] AbsSubframe %d.%d log2_maxh = %d [log2_maxh0 %d log2_maxh1 %d] (%d,%d)\n",
          frame%1024,
          nr_slot_rx,
          pdsch_vars[gNB_id]->log2_maxh,
          pdsch_vars[gNB_id]->log2_maxh0,
          pdsch_vars[gNB_id]->log2_maxh1,
          avg[0],
          avgs);
  }
  stop_meas(&ue->generic_stat_bis[proc->thread_id][slot]);

#if T_TRACER
  if (type == PDSCH)
  {
    T(T_UE_PHY_PDSCH_ENERGY, T_INT(gNB_id),  T_INT(0), T_INT(frame%1024), T_INT(nr_slot_rx),
                             T_INT(avg[0]), T_INT(avg[1]),    T_INT(avg[2]),             T_INT(avg[3]));
  }
#endif

  if (cpumeas(CPUMEAS_GETSTATE))
    LOG_D(PHY, "[AbsSFN %u.%d] Slot%d Symbol %d first_symbol_flag %d: Channel Level  %5.2f \n",frame,nr_slot_rx,slot,symbol,first_symbol_flag,ue->generic_stat_bis[proc->thread_id][slot].p_time/(cpuf*1000.0));

  //----------------------------------------------------------
  //--------------------- channel compensation ---------------
  //----------------------------------------------------------
  // Disable correlation measurement for optimizing UE
  start_meas(&ue->generic_stat_bis[proc->thread_id][slot]);
  nr_dlsch_channel_compensation(pdsch_vars[gNB_id]->rxdataF_ext,
                                pdsch_vars[gNB_id]->dl_ch_estimates_ext,
                                pdsch_vars[gNB_id]->dl_ch_mag0,
                                pdsch_vars[gNB_id]->dl_ch_magb0,
                                pdsch_vars[gNB_id]->dl_ch_magr0,
                                pdsch_vars[gNB_id]->rxdataF_comp0,
                                NULL,//NULL:disable meas. pdsch_vars[gNB_id]->rho:enable meas.
                                frame_parms,
                                nl,
                                symbol,
                                nb_re_pdsch,
                                first_symbol_flag,
                                dlsch0_harq->Qm,
                                nb_rb_pdsch,
                                pdsch_vars[gNB_id]->log2_maxh,
                                measurements); // log2_maxh+I0_shift
    stop_meas(&ue->generic_stat_bis[proc->thread_id][slot]);
    if (cpumeas(CPUMEAS_GETSTATE))
      LOG_D(PHY, "[AbsSFN %u.%d] Slot%d Symbol %d log2_maxh %d channel_level %d: Channel Comp  %5.2f \n", frame, nr_slot_rx, slot, symbol, pdsch_vars[gNB_id]->log2_maxh, proc->channel_level, ue->generic_stat_bis[proc->thread_id][slot].p_time/(cpuf*1000.0));

    start_meas(&ue->generic_stat_bis[proc->thread_id][slot]);

  if (n_rx > 1) {
    nr_dlsch_detection_mrc(pdsch_vars[gNB_id]->rxdataF_comp0,
                           (nl>1)? pdsch_vars[gNB_id]->rho : NULL,
                           pdsch_vars[gNB_id]->dl_ch_mag0,
                           pdsch_vars[gNB_id]->dl_ch_magb0,
                           pdsch_vars[gNB_id]->dl_ch_magr0,
                           nl,
                           n_rx,
                           symbol,
                           nb_rb_pdsch,
                           nb_re_pdsch);
    if (nl >= 2)//Apply zero forcing for 2, 3, and 4 Tx layers
      nr_zero_forcing_rx(pdsch_vars[gNB_id]->rxdataF_comp0,
                                 pdsch_vars[gNB_id]->dl_ch_mag0,
                                 pdsch_vars[gNB_id]->dl_ch_magb0,
                                 pdsch_vars[gNB_id]->dl_ch_magr0,
                                 pdsch_vars[gNB_id]->dl_ch_estimates_ext,
                                 nb_rb_pdsch,
                                 n_rx,
                                 nl,
                                 dlsch0_harq->Qm,
                                 pdsch_vars[gNB_id]->log2_maxh,
                                 symbol,
                                 nb_re_pdsch);
  }
  stop_meas(&ue->generic_stat_bis[proc->thread_id][slot]);

  //printf("start compute LLR\n");
  rxdataF_comp_ptr = pdsch_vars[gNB_id_i]->rxdataF_comp0;
  dl_ch_mag_ptr = pdsch_vars[gNB_id_i]->dl_ch_mag0;
  
  if (cpumeas(CPUMEAS_GETSTATE))
    LOG_D(PHY, "[AbsSFN %u.%d] Slot%d Symbol %d: Channel Combine and zero forcing %5.2f \n",frame,nr_slot_rx,slot,symbol,ue->generic_stat_bis[proc->thread_id][slot].p_time/(cpuf*1000.0));

  start_meas(&ue->generic_stat_bis[proc->thread_id][slot]);
  /* Store the valid DL RE's */
  pdsch_vars[gNB_id]->dl_valid_re[symbol-1] = nb_re_pdsch;

  if(dlsch0_harq->status == ACTIVE) {
    startSymbIdx = dlsch0_harq->start_symbol;
    nbSymb = dlsch0_harq->nb_symbols;
    pduBitmap = dlsch0_harq->pduBitmap;
  }
  if(dlsch1_harq) {
    startSymbIdx = dlsch1_harq->start_symbol;
    nbSymb = dlsch1_harq->nb_symbols;
    pduBitmap = dlsch1_harq->pduBitmap;
  }

  /* Check for PTRS bitmap and process it respectively */
  if((pduBitmap & 0x1) && (type == PDSCH)) {
    nr_pdsch_ptrs_processing(ue,
			     pdsch_vars,
			     frame_parms,
			     dlsch0_harq,
			     dlsch1_harq,
			     gNB_id,
			     nr_slot_rx,
			     symbol,
			     (nb_rb_pdsch*12),
			     dlsch[0]->rnti,rx_type);
    pdsch_vars[gNB_id]->dl_valid_re[symbol-1] -= pdsch_vars[gNB_id]->ptrs_re_per_slot[0][symbol];
  }

  /* at last symbol in a slot calculate LLR's for whole slot */
  if(symbol == (startSymbIdx + nbSymb -1)) {
    for(uint8_t i =startSymbIdx; i < (startSymbIdx+nbSymb);i++) {
      /* re evaluating the first symbol flag as LLR's are done in symbol loop  */
      if(i == startSymbIdx && i < 3) {
	first_symbol_flag =1;
      }
      else {
	first_symbol_flag=0;
      }
      /* Calculate LLR's for each symbol */
      nr_dlsch_llr(pdsch_vars, frame_parms,
		   rxdataF_comp_ptr, dl_ch_mag_ptr,
		   dlsch0_harq, dlsch1_harq,
		   rx_type, harq_pid,
		   gNB_id, gNB_id_i,
		   first_symbol_flag,
		   i, nb_rb_pdsch,
		   codeword_TB0, codeword_TB1,
		   pdsch_vars[gNB_id]->dl_valid_re[i-1],
		   nr_slot_rx, beamforming_mode);
    }

    int dmrs_type = dlsch[0]->harq_processes[harq_pid]->dmrsConfigType;
    uint8_t nb_re_dmrs;
    uint16_t dmrs_len = get_num_dmrs(dlsch[0]->harq_processes[harq_pid]->dlDmrsSymbPos);
    if (dmrs_type==NFAPI_NR_DMRS_TYPE1) {
      nb_re_dmrs = 6*dlsch[0]->harq_processes[harq_pid]->n_dmrs_cdm_groups;
    } else {
      nb_re_dmrs = 4*dlsch[0]->harq_processes[harq_pid]->n_dmrs_cdm_groups;
    }
    dlsch[0]->harq_processes[harq_pid]->G = nr_get_G(dlsch[0]->harq_processes[harq_pid]->nb_rb,
						     dlsch[0]->harq_processes[harq_pid]->nb_symbols,
						     nb_re_dmrs,
						     dmrs_len,
						     dlsch[0]->harq_processes[harq_pid]->Qm,
						     dlsch[0]->harq_processes[harq_pid]->Nl);
    nr_dlsch_layer_demapping(pdsch_vars[gNB_id]->llr,
			     dlsch[0]->harq_processes[harq_pid]->Nl,
			     dlsch[0]->harq_processes[harq_pid]->Qm,
			     dlsch[0]->harq_processes[harq_pid]->G,
			     codeword_TB0,
			     codeword_TB1,
			     pdsch_vars[gNB_id]->layer_llr);
  }

  stop_meas(&ue->generic_stat_bis[proc->thread_id][slot]);
  if (cpumeas(CPUMEAS_GETSTATE))
    LOG_D(PHY, "[AbsSFN %u.%d] Slot%d Symbol %d: LLR Computation  %5.2f \n",frame,nr_slot_rx,slot,symbol,ue->generic_stat_bis[proc->thread_id][slot].p_time/(cpuf*1000.0));

  // Please keep it: useful for debugging
#ifdef DEBUG_PDSCH_RX
  char filename[50];
  uint8_t aa = 0;

  snprintf(filename, 50, "rxdataF0_symb_%d_nr_slot_rx_%d.m", symbol, nr_slot_rx);
  write_output(filename, "rxdataF0", &common_vars->common_vars_rx_data_per_thread[proc->thread_id].rxdataF[0][0], NR_SYMBOLS_PER_SLOT*frame_parms->ofdm_symbol_size, 1, 1);

  snprintf(filename, 50, "dl_ch_estimates0%d_symb_%d_nr_slot_rx_%d.m", aa, symbol, nr_slot_rx);
  write_output(filename, "dl_ch_estimates", &pdsch_vars[gNB_id]->dl_ch_estimates[aa][0], NR_SYMBOLS_PER_SLOT*frame_parms->ofdm_symbol_size, 1, 1);

  snprintf(filename, 50, "rxdataF_ext0%d_symb_%d_nr_slot_rx_%d.m", aa, symbol, nr_slot_rx);
  write_output(filename, "rxdataF_ext", &pdsch_vars[gNB_id]->rxdataF_ext[aa][0], NR_SYMBOLS_PER_SLOT*frame_parms->N_RB_DL*NR_NB_SC_PER_RB, 1, 1);

  snprintf(filename, 50, "dl_ch_estimates_ext0%d_symb_%d_nr_slot_rx_%d.m", aa, symbol, nr_slot_rx);
  write_output(filename, "dl_ch_estimates_ext00", &pdsch_vars[gNB_id]->dl_ch_estimates_ext[aa][0], NR_SYMBOLS_PER_SLOT*frame_parms->N_RB_DL*NR_NB_SC_PER_RB, 1, 1);

  snprintf(filename, 50, "rxdataF_comp0%d_symb_%d_nr_slot_rx_%d.m", aa, symbol, nr_slot_rx);
  write_output(filename, "rxdataF_comp00", &pdsch_vars[gNB_id]->rxdataF_comp0[aa][0], NR_SYMBOLS_PER_SLOT*frame_parms->N_RB_DL*NR_NB_SC_PER_RB, 1, 1);
/*
  for (int i=0; i < 2; i++){
    snprintf(filename, 50,  "llr%d_symb_%d_nr_slot_rx_%d.m", i, symbol, nr_slot_rx);
    write_output(filename,"llr",  &pdsch_vars[gNB_id]->llr[i][0], (NR_SYMBOLS_PER_SLOT*nb_rb_pdsch*NR_NB_SC_PER_RB*dlsch1_harq->Qm) - 4*(nb_rb_pdsch*4*dlsch1_harq->Qm), 1, 0);
  }
*/
#endif

#if T_TRACER
  T(T_UE_PHY_PDSCH_IQ, T_INT(gNB_id), T_INT(ue->Mod_id), T_INT(frame%1024),
    T_INT(nr_slot_rx), T_INT(nb_rb_pdsch),
    T_INT(frame_parms->N_RB_UL), T_INT(frame_parms->symbols_per_slot),
    T_BUFFER(&pdsch_vars[gNB_id]->rxdataF_comp0[gNB_id][0], 2 * /* ulsch[UE_id]->harq_processes[harq_pid]->nb_rb */ frame_parms->N_RB_UL *12*frame_parms->symbols_per_slot*2));
#endif
  return(0);

}

void nr_dlsch_deinterleaving(uint8_t symbol,
                             uint8_t start_symbol,
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

  printf("N_bundle %u L %d nb_rb_pdsch %d\n",N_bundle, L,nb_rb_pdsch);

  if (symbol==start_symbol)
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
  free(bundle_deint);
}

//==============================================================================================
// Pre-processing for LLR computation
//==============================================================================================

void nr_dlsch_channel_compensation(int **rxdataF_ext,
                                int **dl_ch_estimates_ext,
                                int **dl_ch_mag,
                                int **dl_ch_magb,
                                int **dl_ch_magr,
                                int **rxdataF_comp,
                                int ***rho,
                                NR_DL_FRAME_PARMS *frame_parms,
                                uint8_t nb_aatx,
                                unsigned char symbol,
                                int length,
                                uint8_t first_symbol_flag,
                                unsigned char mod_order,
                                unsigned short nb_rb,
                                unsigned char output_shift,
                                PHY_NR_MEASUREMENTS *measurements)
{

#if defined(__i386) || defined(__x86_64)

  unsigned short rb;
  unsigned char aatx,aarx,atx;
  __m128i *dl_ch128,*dl_ch128_2,*dl_ch_mag128,*dl_ch_mag128b,*dl_ch_mag128r,*rxdataF128,*rxdataF_comp128,*rho128;
  __m128i mmtmpD0,mmtmpD1,mmtmpD2,mmtmpD3,QAM_amp128={0},QAM_amp128b={0},QAM_amp128r={0};

  uint32_t nb_rb_0 = length/12 + ((length%12)?1:0);
  for (aatx=0; aatx<nb_aatx; aatx++) {
    if (mod_order == 4) {
      QAM_amp128 = _mm_set1_epi16(QAM16_n1);  // 2/sqrt(10)
      QAM_amp128b = _mm_setzero_si128();
      QAM_amp128r = _mm_setzero_si128();
    } else if (mod_order == 6) {
      QAM_amp128  = _mm_set1_epi16(QAM64_n1); //
      QAM_amp128b = _mm_set1_epi16(QAM64_n2);
      QAM_amp128r = _mm_setzero_si128();
    } else if (mod_order == 8) {
      QAM_amp128 = _mm_set1_epi16(QAM256_n1);
      QAM_amp128b = _mm_set1_epi16(QAM256_n2);
      QAM_amp128r = _mm_set1_epi16(QAM256_n3);
    }

    //    printf("comp: rxdataF_comp %p, symbol %d\n",rxdataF_comp[0],symbol);

    for (aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++) {

      dl_ch128          = (__m128i *)&dl_ch_estimates_ext[(aatx*frame_parms->nb_antennas_rx)+aarx][symbol*nb_rb*12];
      dl_ch_mag128      = (__m128i *)&dl_ch_mag[(aatx*frame_parms->nb_antennas_rx)+aarx][symbol*nb_rb*12];
      dl_ch_mag128b     = (__m128i *)&dl_ch_magb[(aatx*frame_parms->nb_antennas_rx)+aarx][symbol*nb_rb*12];
      dl_ch_mag128r     = (__m128i *)&dl_ch_magr[(aatx*frame_parms->nb_antennas_rx)+aarx][symbol*nb_rb*12];
      rxdataF128        = (__m128i *)&rxdataF_ext[aarx][symbol*nb_rb*12];
      rxdataF_comp128   = (__m128i *)&rxdataF_comp[(aatx*frame_parms->nb_antennas_rx)+aarx][symbol*nb_rb*12];

      for (rb=0; rb<nb_rb_0; rb++) {
        if (mod_order>2) {
          // get channel amplitude if not QPSK

          mmtmpD0 = _mm_madd_epi16(dl_ch128[0],dl_ch128[0]);
          mmtmpD0 = _mm_srai_epi32(mmtmpD0,output_shift);

          mmtmpD1 = _mm_madd_epi16(dl_ch128[1],dl_ch128[1]);
          mmtmpD1 = _mm_srai_epi32(mmtmpD1,output_shift);

          mmtmpD0 = _mm_packs_epi32(mmtmpD0,mmtmpD1); //|H[0]|^2 |H[1]|^2 |H[2]|^2 |H[3]|^2 |H[4]|^2 |H[5]|^2 |H[6]|^2 |H[7]|^2

          // store channel magnitude here in a new field of dlsch

          dl_ch_mag128[0] = _mm_unpacklo_epi16(mmtmpD0,mmtmpD0);
          dl_ch_mag128b[0] = dl_ch_mag128[0];
          dl_ch_mag128r[0] = dl_ch_mag128[0];
          dl_ch_mag128[0] = _mm_mulhi_epi16(dl_ch_mag128[0],QAM_amp128);
          dl_ch_mag128[0] = _mm_slli_epi16(dl_ch_mag128[0],1);

          dl_ch_mag128b[0] = _mm_mulhi_epi16(dl_ch_mag128b[0],QAM_amp128b);
          dl_ch_mag128b[0] = _mm_slli_epi16(dl_ch_mag128b[0],1);

          dl_ch_mag128r[0] = _mm_mulhi_epi16(dl_ch_mag128r[0],QAM_amp128r);
          dl_ch_mag128r[0] = _mm_slli_epi16(dl_ch_mag128r[0],1);

    //print_ints("Re(ch):",(int16_t*)&mmtmpD0);
    //print_shorts("QAM_amp:",(int16_t*)&QAM_amp128);
    //print_shorts("mag:",(int16_t*)&dl_ch_mag128[0]);
          dl_ch_mag128[1] = _mm_unpackhi_epi16(mmtmpD0,mmtmpD0);
          dl_ch_mag128b[1] = dl_ch_mag128[1];
          dl_ch_mag128r[1] = dl_ch_mag128[1];
          dl_ch_mag128[1] = _mm_mulhi_epi16(dl_ch_mag128[1],QAM_amp128);
          dl_ch_mag128[1] = _mm_slli_epi16(dl_ch_mag128[1],1);

          dl_ch_mag128b[1] = _mm_mulhi_epi16(dl_ch_mag128b[1],QAM_amp128b);
          dl_ch_mag128b[1] = _mm_slli_epi16(dl_ch_mag128b[1],1);

          dl_ch_mag128r[1] = _mm_mulhi_epi16(dl_ch_mag128r[1],QAM_amp128r);
          dl_ch_mag128r[1] = _mm_slli_epi16(dl_ch_mag128r[1],1);

          mmtmpD0 = _mm_madd_epi16(dl_ch128[2],dl_ch128[2]);//[H_I(0)^2+H_Q(0)^2 H_I(1)^2+H_Q(1)^2 H_I(2)^2+H_Q(2)^2 H_I(3)^2+H_Q(3)^2]
          mmtmpD0 = _mm_srai_epi32(mmtmpD0,output_shift);
          mmtmpD1 = _mm_packs_epi32(mmtmpD0,mmtmpD0);//[|H(0)|^2 |H(1)|^2 |H(2)|^2 |H(3)|^2 |H(0)|^2 |H(1)|^2 |H(2)|^2 |H(3)|^2]

          dl_ch_mag128[2] = _mm_unpacklo_epi16(mmtmpD1,mmtmpD1);//[|H(0)|^2 |H(0)|^2 |H(1)|^2 |H(1)|^2 |H(2)|^2 |H(2)|^2 |H(3)|^2 |H(3)|^2]
          dl_ch_mag128b[2] = dl_ch_mag128[2];
          dl_ch_mag128r[2] = dl_ch_mag128[2];

          dl_ch_mag128[2] = _mm_mulhi_epi16(dl_ch_mag128[2],QAM_amp128);
          dl_ch_mag128[2] = _mm_slli_epi16(dl_ch_mag128[2],1);

          dl_ch_mag128b[2] = _mm_mulhi_epi16(dl_ch_mag128b[2],QAM_amp128b);
          dl_ch_mag128b[2] = _mm_slli_epi16(dl_ch_mag128b[2],1);

          dl_ch_mag128r[2] = _mm_mulhi_epi16(dl_ch_mag128r[2],QAM_amp128r);
          dl_ch_mag128r[2] = _mm_slli_epi16(dl_ch_mag128r[2],1);
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

#ifdef DEBUG_DLSCH_DEMOD
        printf("%%arx%d atx%d rb_index %d symbol %d shift %d\n",aarx,aatx,rb,symbol,output_shift);
        printf("rx_%d(%d,:)",aarx+1,rb+1);
        print_shorts("  ",(int16_t *)&rxdataF128[0]);
        printf("ch_%d%d(%d,:)",aarx+1,aatx+1,rb+1);
        print_shorts("  ",(int16_t *)&dl_ch128[0]);
        printf("rx_comp_%d%d(%d,:)",aarx+1,aatx+1,rb+1);
        print_shorts("  ",(int16_t *)&rxdataF_comp128[0]);
#endif

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
#ifdef DEBUG_DLSCH_DEMOD
        print_shorts("rx:",(int16_t*)&rxdataF128[1]);
        print_shorts("ch:",(int16_t*)&dl_ch128[1]);
        print_shorts("pack:",(int16_t*)&rxdataF_comp128[1]);
#endif

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
#ifdef DEBUG_DLSCH_DEMOD
        print_shorts("rx:",(int16_t*)&rxdataF128[2]);
        print_shorts("ch:",(int16_t*)&dl_ch128[2]);
        print_shorts("pack:",(int16_t*)&rxdataF_comp128[2]);
#endif

        dl_ch128+=3;
        dl_ch_mag128+=3;
        dl_ch_mag128b+=3;
        dl_ch_mag128r+=3;
        rxdataF128+=3;
        rxdataF_comp128+=3;
      }
    }
  }
  if (rho) {
    //we compute the Tx correlation matrix for each Rx antenna
    //As an example the 2x2 MIMO case requires
    //rho[aarx][nb_aatx*nb_aatx] = [cov(H_aarx_0,H_aarx_0) cov(H_aarx_0,H_aarx_1)
    //                              cov(H_aarx_1,H_aarx_0) cov(H_aarx_1,H_aarx_1)], aarx=0,...,nb_antennas_rx-1

    //int avg_rho_re[frame_parms->nb_antennas_rx][nb_aatx*nb_aatx];
    //int avg_rho_im[frame_parms->nb_antennas_rx][nb_aatx*nb_aatx];

    for (aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++) {

      for (aatx=0; aatx<nb_aatx; aatx++) {

        for (atx=0; atx<nb_aatx; atx++) {
          //avg_rho_re[aarx][aatx*nb_aatx+atx] = 0;
          //avg_rho_im[aarx][aatx*nb_aatx+atx] = 0;
          rho128        = (__m128i *)&rho[aarx][aatx*nb_aatx+atx][symbol*nb_rb*12];
          dl_ch128      = (__m128i *)&dl_ch_estimates_ext[aatx*frame_parms->nb_antennas_rx+aarx][symbol*nb_rb*12];
          dl_ch128_2    = (__m128i *)&dl_ch_estimates_ext[atx*frame_parms->nb_antennas_rx+aarx][symbol*nb_rb*12];

          for (rb=0; rb<nb_rb_0; rb++) {
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

            /*avg_rho_re[aarx][aatx*nb_aatx+atx] +=(((int16_t*)&rho128[0])[0]+
              ((int16_t*)&rho128[0])[2] +
              ((int16_t*)&rho128[0])[4] +
              ((int16_t*)&rho128[0])[6])/16;*/
            /*avg_rho_im[aarx][aatx*nb_aatx+atx] +=(((int16_t*)&rho128[0])[1]+
              ((int16_t*)&rho128[0])[3] +
              ((int16_t*)&rho128[0])[5] +
              ((int16_t*)&rho128[0])[7])/16;*/

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
            /*avg_rho_re[aarx][aatx*nb_aatx+atx] +=(((int16_t*)&rho128[1])[0]+
              ((int16_t*)&rho128[1])[2] +
              ((int16_t*)&rho128[1])[4] +
              ((int16_t*)&rho128[1])[6])/16;*/
            /*avg_rho_im[aarx][aatx*nb_aatx+atx] +=(((int16_t*)&rho128[1])[1]+
              ((int16_t*)&rho128[1])[3] +
              ((int16_t*)&rho128[1])[5] +
              ((int16_t*)&rho128[1])[7])/16;*/

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

            /*avg_rho_re[aarx][aatx*nb_aatx+atx] +=(((int16_t*)&rho128[2])[0]+
              ((int16_t*)&rho128[2])[2] +
              ((int16_t*)&rho128[2])[4] +
              ((int16_t*)&rho128[2])[6])/16;*/
            /*avg_rho_im[aarx][aatx*nb_aatx+atx] +=(((int16_t*)&rho128[2])[1]+
              ((int16_t*)&rho128[2])[3] +
              ((int16_t*)&rho128[2])[5] +
              ((int16_t*)&rho128[2])[7])/16;*/

            dl_ch128+=3;
            dl_ch128_2+=3;
            rho128+=3;
          }
          if (first_symbol_flag==1) {
            //rho_nm = H_arx_n.conj(H_arx_m)
            //rho_rx_corr[arx][nm] = |H_arx_n|^2.|H_arx_m|^2 &rho[aarx][aatx*nb_aatx+atx][symbol*nb_rb*12]
            measurements->rx_correlation[0][aarx][aatx*nb_aatx+atx] = signal_energy(&rho[aarx][aatx*nb_aatx+atx][symbol*nb_rb*12],length);
            //avg_rho_re[aarx][aatx*nb_aatx+atx] = 16*avg_rho_re[aarx][aatx*nb_aatx+atx]/length;
            //avg_rho_im[aarx][aatx*nb_aatx+atx] = 16*avg_rho_im[aarx][aatx*nb_aatx+atx]/length;
            //printf("rho[rx]%d tx%d tx%d = Re: %d Im: %d\n",aarx, aatx,atx, avg_rho_re[aarx][aatx*nb_aatx+atx], avg_rho_im[aarx][aatx*nb_aatx+atx]);
            //printf("rho_corr[rx]%d tx%d tx%d = %d ...\n",aarx, aatx,atx, measurements->rx_correlation[0][aarx][aatx*nb_aatx+atx]);
          }
        }
      }
    }
  }
  _mm_empty();
  _m_empty();

#elif defined(__arm__)

  unsigned short rb;
  unsigned char aatx,aarx,symbol_mod;

  int16x4_t *dl_ch128,*dl_ch128_2,*rxdataF128;
  int32x4_t mmtmpD0,mmtmpD1,mmtmpD0b,mmtmpD1b;
  int16x8_t *dl_ch_mag128,*dl_ch_mag128b,mmtmpD2,mmtmpD3,mmtmpD4;
  int16x8_t QAM_amp128,QAM_amp128b;
  int16x4x2_t *rxdataF_comp128,*rho128;

  int16_t conj[4]__attribute__((aligned(16))) = {1,-1,1,-1};
  int32x4_t output_shift128 = vmovq_n_s32(-(int32_t)output_shift);

  symbol_mod = (symbol>=(7-frame_parms->Ncp)) ? symbol-(7-frame_parms->Ncp) : symbol;

  uint32_t nb_rb_0 = length/12 + ((length%12)?1:0);

  for (aatx=0; aatx<frame_parms->nb_antenna_ports_gNB; aatx++) {
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

      for (rb=0; rb<nb_rb_0; rb++) {
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

    mmtmpD0 = vmull_s16(dl_ch128[4], dl_ch128[4]);
    mmtmpD0 = vqshlq_s32(vqaddq_s32(mmtmpD0,vrev64q_s32(mmtmpD0)),output_shift128);
    mmtmpD1 = vmull_s16(dl_ch128[5], dl_ch128[5]);
    mmtmpD1 = vqshlq_s32(vqaddq_s32(mmtmpD1,vrev64q_s32(mmtmpD1)),output_shift128);
    mmtmpD4 = vcombine_s16(vmovn_s32(mmtmpD0),vmovn_s32(mmtmpD1));

    dl_ch_mag128b[0] = vqdmulhq_s16(mmtmpD2,QAM_amp128b);
    dl_ch_mag128b[1] = vqdmulhq_s16(mmtmpD3,QAM_amp128b);
    dl_ch_mag128[0] = vqdmulhq_s16(mmtmpD2,QAM_amp128);
    dl_ch_mag128[1] = vqdmulhq_s16(mmtmpD3,QAM_amp128);

    dl_ch_mag128b[2] = vqdmulhq_s16(mmtmpD4,QAM_amp128b);
    dl_ch_mag128[2]  = vqdmulhq_s16(mmtmpD4,QAM_amp128);
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

      }
    }
  }

  if (rho) {
    for (aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++) {
      rho128        = (int16x4x2_t*)&rho[aarx][symbol*frame_parms->N_RB_DL*12];
      dl_ch128      = (int16x4_t*)&dl_ch_estimates_ext[aarx][symbol*frame_parms->N_RB_DL*12];
      dl_ch128_2    = (int16x4_t*)&dl_ch_estimates_ext[2+aarx][symbol*frame_parms->N_RB_DL*12];
      for (rb=0; rb<nb_rb_0; rb++) {
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

void nr_dlsch_channel_compensation_core(int **rxdataF_ext,
                                     int **dl_ch_estimates_ext,
                                     int **dl_ch_mag,
                                     int **dl_ch_magb,
                                     int **rxdataF_comp,
                                     int ***rho,
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
  __m128i *dl_ch128,*dl_ch_mag128,*dl_ch_mag128b, *dl_ch128_2, *rxdataF128,*rxdataF_comp128,*rho128;
  __m128i mmtmpD0,mmtmpD1,mmtmpD2,mmtmpD3,QAM_amp128={0},QAM_amp128b={0};
  int aatx = 0, aarx = 0;

  for (aatx=0; aatx<n_tx; aatx++) {

    if (mod_order == 4) {
      QAM_amp128 = _mm_set1_epi16(QAM16_n1);  // 2/sqrt(10)
      QAM_amp128b = _mm_setzero_si128();
    } else if (mod_order == 6) {
      QAM_amp128  = _mm_set1_epi16(QAM64_n1); //
      QAM_amp128b = _mm_set1_epi16(QAM64_n2);
    }

    for (aarx=0; aarx<n_rx; aarx++) {
      /* TODO: hack to be removed. There is crash for 1 antenna case, so
       * for 1 antenna case, I put back the value 2 as it was before
       * Elena's commit.
       */
      int x = n_rx > 1 ? n_rx : 2;

    dl_ch128          = (__m128i *)&dl_ch_estimates_ext[aatx*x + aarx][start_point];
    dl_ch_mag128      = (__m128i *)&dl_ch_mag[aatx*x + aarx][start_point];
    dl_ch_mag128b     = (__m128i *)&dl_ch_magb[aatx*x + aarx][start_point];
    rxdataF128        = (__m128i *)&rxdataF_ext[aarx][start_point];
    rxdataF_comp128   = (__m128i *)&rxdataF_comp[aatx*x + aarx][start_point];

      length_mod8 = length&7;
      if (length_mod8 == 0){
        length2 = length>>3;

        for (ii=0; ii<length2; ++ii) {
          if (mod_order>2) {
            // get channel amplitude if not QPSK

            mmtmpD0 = _mm_madd_epi16(dl_ch128[0],dl_ch128[0]);
            mmtmpD0 = _mm_srai_epi32(mmtmpD0,output_shift);

            mmtmpD1 = _mm_madd_epi16(dl_ch128[1],dl_ch128[1]);
            mmtmpD1 = _mm_srai_epi32(mmtmpD1,output_shift);

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

            dl_ch_mag128b[0] = _mm_mulhi_epi16(dl_ch_mag128b[0],QAM_amp128b);
            dl_ch_mag128b[0] = _mm_slli_epi16(dl_ch_mag128b[0],1);

            dl_ch_mag128b[1] = _mm_mulhi_epi16(dl_ch_mag128b[1],QAM_amp128b);
            dl_ch_mag128b[1] = _mm_slli_epi16(dl_ch_mag128b[1],1);

          }

          // multiply by conjugated channel
          mmtmpD0 = _mm_madd_epi16(dl_ch128[0],rxdataF128[0]);

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
          //print_shorts("pack:",rxdataF_comp128+1);

          dl_ch128+=2;
          dl_ch_mag128+=2;
          dl_ch_mag128b+=2;
          rxdataF128+=2;
          rxdataF_comp128+=2;
        }
      }else {
        printf ("Channel Compensation: Received number of subcarriers is not multiple of 8, \n"
                 "need to adapt the code!\n");
      }
    }
  }

/*This part of code makes sense only for processing in 2x2 blocks*/
  if (rho) {


    for (aarx=0; aarx<n_rx; aarx++) {
      rho128        = (__m128i *)&rho[aarx][0][start_point];
      dl_ch128      = (__m128i *)&dl_ch_estimates_ext[aarx][start_point];
      dl_ch128_2    = (__m128i *)&dl_ch_estimates_ext[2+aarx][start_point];

      if (length_mod8 == 0){
        length2 = length>>3;

        for (ii=0; ii<length2; ++ii) {
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
          dl_ch128+=2;
          dl_ch128_2+=2;
          rho128+=2;
        }
      }else {
        printf ("Channel Compensation: Received number of subcarriers is not multiple of 8, \n"
                 "need to adapt the code!\n");
      }
    }
  }
  _mm_empty();
  _m_empty();
}


void nr_dlsch_scale_channel(int **dl_ch_estimates_ext,
			    NR_DL_FRAME_PARMS *frame_parms,
			    uint8_t n_tx,
			    uint8_t n_rx,
			    NR_UE_DLSCH_t **dlsch_ue,
			    uint8_t symbol,
			    uint8_t pilots,
			    uint32_t len,
			    unsigned short nb_rb)
{

#if defined(__x86_64__)||defined(__i386__)

  short rb, ch_amp;
  unsigned char aatx,aarx;
  __m128i *dl_ch128, ch_amp128;

  uint32_t nb_rb_0 = len/12 + ((len%12)?1:0);

  // Determine scaling amplitude based the symbol

  ch_amp = 1024*8; //((pilots) ? (dlsch_ue[0]->sqrt_rho_b) : (dlsch_ue[0]->sqrt_rho_a));

  LOG_D(PHY,"Scaling PDSCH Chest in OFDM symbol %d by %d, pilots %d nb_rb %d NCP %d symbol %d\n",symbol,ch_amp,pilots,nb_rb,frame_parms->Ncp,symbol);
  // printf("Scaling PDSCH Chest in OFDM symbol %d by %d\n",symbol_mod,ch_amp);

  ch_amp128 = _mm_set1_epi16(ch_amp); // Q3.13

  for (aatx=0; aatx<n_tx; aatx++) {
    for (aarx=0; aarx<n_rx; aarx++) {

      dl_ch128=(__m128i *)&dl_ch_estimates_ext[(aatx*n_rx)+aarx][symbol*nb_rb*12];

      for (rb=0;rb<nb_rb_0;rb++) {

        dl_ch128[0] = _mm_mulhi_epi16(dl_ch128[0],ch_amp128);
        dl_ch128[0] = _mm_slli_epi16(dl_ch128[0],3);

        dl_ch128[1] = _mm_mulhi_epi16(dl_ch128[1],ch_amp128);
        dl_ch128[1] = _mm_slli_epi16(dl_ch128[1],3);

        dl_ch128[2] = _mm_mulhi_epi16(dl_ch128[2],ch_amp128);
        dl_ch128[2] = _mm_slli_epi16(dl_ch128[2],3);
        dl_ch128+=3;

      }
    }
  }

#elif defined(__arm__)

#endif
}


//compute average channel_level on each (TX,RX) antenna pair
void nr_dlsch_channel_level(int **dl_ch_estimates_ext,
			    NR_DL_FRAME_PARMS *frame_parms,
			    uint8_t n_tx,
			    int32_t *avg,
			    uint8_t symbol,
			    uint32_t len,
			    unsigned short nb_rb)
{

#if defined(__x86_64__)||defined(__i386__)

  short rb;
  unsigned char aatx,aarx;
  __m128i *dl_ch128, avg128D;

  //nb_rb*nre = y * 2^x
  int16_t x = factor2(len);
  //x = (x>4) ? 4 : x;
  int16_t y = (len)>>x;
  //printf("len = %d = %d * 2^(%d)\n",len,y,x);
  uint32_t nb_rb_0 = len/12 + ((len%12)?1:0);

  AssertFatal(y!=0,"Cannot divide by zero: in function %s of file %s\n", __func__, __FILE__);

  for (aatx=0; aatx<n_tx; aatx++)
    for (aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++) {
      //clear average level
      avg128D = _mm_setzero_si128();

      dl_ch128=(__m128i *)&dl_ch_estimates_ext[(aatx*frame_parms->nb_antennas_rx)+aarx][symbol*nb_rb*12];

      for (rb=0;rb<nb_rb_0;rb++) {
        avg128D = _mm_add_epi32(avg128D,_mm_srai_epi32(_mm_madd_epi16(dl_ch128[0],dl_ch128[0]),x));
        avg128D = _mm_add_epi32(avg128D,_mm_srai_epi32(_mm_madd_epi16(dl_ch128[1],dl_ch128[1]),x));
        avg128D = _mm_add_epi32(avg128D,_mm_srai_epi32(_mm_madd_epi16(dl_ch128[2],dl_ch128[2]),x));
        dl_ch128+=3;
      }

      avg[(aatx*frame_parms->nb_antennas_rx)+aarx] =(((int32_t*)&avg128D)[0] +
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
  uint32_t nb_rb_0 = len/12 + ((len%12)?1:0);
  for (aatx=0; aatx<frame_parms->nb_antenna_ports_gNB; aatx++)
    for (aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++) {
      //clear average level
      avg128D = vdupq_n_s32(0);
      // 5 is always a symbol with no pilots for both normal and extended prefix

      dl_ch128=(int16x4_t *)&dl_ch_estimates_ext[(aatx<<1)+aarx][symbol*frame_parms->N_RB_DL*12];

      for (rb=0; rb<nb_rb_0; rb++) {
        //  printf("rb %d : ",rb);
        //  print_shorts("ch",&dl_ch128[0]);
        avg128D = vqaddq_s32(avg128D,vmull_s16(dl_ch128[0],dl_ch128[0]));
        avg128D = vqaddq_s32(avg128D,vmull_s16(dl_ch128[1],dl_ch128[1]));
        avg128D = vqaddq_s32(avg128D,vmull_s16(dl_ch128[2],dl_ch128[2]));
        avg128D = vqaddq_s32(avg128D,vmull_s16(dl_ch128[3],dl_ch128[3]));

        if (((symbol_mod == 0) || (symbol_mod == (frame_parms->Ncp-1)))&&(frame_parms->nb_antenna_ports_gNB!=1)) {
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

void nr_dlsch_channel_level_median(int **dl_ch_estimates_ext,
                                int32_t *median,
                                int n_tx,
                                int n_rx,
                                int length,
                                int start_point)
{

#if defined(__x86_64__)||defined(__i386__)

  short ii;
  int aatx,aarx;
  int length2;
  int max = 0, min=0;
  int norm_pack;
  __m128i *dl_ch128, norm128D;

  for (aatx=0; aatx<n_tx; aatx++) {
    for (aarx=0; aarx<n_rx; aarx++) {
      max = median[aatx*n_rx + aarx];//initialize the med point for max
      min = median[aatx*n_rx + aarx];//initialize the med point for min
      norm128D = _mm_setzero_si128();

      dl_ch128=(__m128i *)&dl_ch_estimates_ext[aatx*n_rx + aarx][start_point];

      length2 = length>>2;//length = number of REs, hence length2=nb_REs*(32/128) in SIMD loop

      for (ii=0;ii<length2;ii++) {
        norm128D = _mm_srai_epi32( _mm_madd_epi16(dl_ch128[0],dl_ch128[0]), 2);//[|H_0|²/4 |H_1|²/4 |H_2|²/4 |H_3|²/4]
        //print_ints("norm128D",&norm128D[0]);

        norm_pack = ((int32_t*)&norm128D)[0] +
            ((int32_t*)&norm128D)[1] +
            ((int32_t*)&norm128D)[2] +
            ((int32_t*)&norm128D)[3];// compute the sum

        if (norm_pack > max)
          max = norm_pack;//store values more than max
        if (norm_pack < min)
          min = norm_pack;//store values less than min
        dl_ch128+=1;
      }

      median[aatx*n_rx + aarx]  = (max+min)>>1;
      //printf("Channel level  median [%d]: %d max = %d min = %d\n",aatx*n_rx + aarx, median[aatx*n_rx + aarx],max,min);
      }
  }

  _mm_empty();
  _m_empty();

#elif defined(__arm__)

  short rb;
  unsigned char aatx,aarx,nre=12,symbol_mod;
  int32x4_t norm128D;
  int16x4_t *dl_ch128;

  for (aatx=0; aatx<frame_parms->nb_antenna_ports_gNB; aatx++){
    for (aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++) {
      max = 0;
      min = 0;
      norm128D = vdupq_n_s32(0);

      dl_ch128=(int16x4_t *)&dl_ch_estimates_ext[aatx*n_rx + aarx][start_point];

      length_mod8=length&3;
      length2 = length>>2;

      for (ii=0;ii<length2;ii++) {
        norm128D = vshrq_n_u32(vmull_s16(dl_ch128[0],dl_ch128[0]), 1);
        norm_pack = ((int32_t*)&norm128D)[0] +
                    ((int32_t*)&norm128D)[1] +
                    ((int32_t*)&norm128D)[2] +
                    ((int32_t*)&norm128D)[3];

        if (norm_pack > max)
          max = norm_pack;
        if (norm_pack < min)
          min = norm_pack;

          dl_ch128+=1;
      }

        median[aatx*n_rx + aarx]  = (max+min)>>1;

      //printf("Channel level  median [%d]: %d\n",aatx*n_rx + aarx, median[aatx*n_rx + aarx]);
      }
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
                                           unsigned char symbol,
                                           uint8_t pilots,
                                           uint8_t config_type,
                                           unsigned short start_rb,
                                           unsigned short nb_rb_pdsch,
                                           uint8_t n_dmrs_cdm_groups,
                                           NR_DL_FRAME_PARMS *frame_parms,
                                           uint16_t dlDmrsSymbPos,
                                           int chest_time_type)
{
  if (config_type == NFAPI_NR_DMRS_TYPE1) {
    AssertFatal(n_dmrs_cdm_groups == 1 || n_dmrs_cdm_groups == 2,
                "n_dmrs_cdm_groups %d is illegal\n",n_dmrs_cdm_groups);
  } else {
    AssertFatal(n_dmrs_cdm_groups == 1 || n_dmrs_cdm_groups == 2 || n_dmrs_cdm_groups == 3,
                "n_dmrs_cdm_groups %d is illegal\n",n_dmrs_cdm_groups);
  }

  const unsigned short start_re = (frame_parms->first_carrier_offset + start_rb * NR_NB_SC_PER_RB) % frame_parms->ofdm_symbol_size;
  int8_t validDmrsEst;

  if (chest_time_type == 0)
    validDmrsEst = get_valid_dmrs_idx_for_channel_est(dlDmrsSymbPos,symbol);
  else
    validDmrsEst = get_next_dmrs_symbol_in_slot(dlDmrsSymbPos,0,14); // get first dmrs symbol index

  for (unsigned char aarx = 0; aarx < frame_parms->nb_antennas_rx; aarx++) {

    int32_t *dl_ch0     = &dl_ch_estimates[aarx][validDmrsEst * frame_parms->ofdm_symbol_size];
    int32_t *dl_ch0_ext = &dl_ch_estimates_ext[aarx][symbol * nb_rb_pdsch * NR_NB_SC_PER_RB];
    int32_t *rxF_ext    = &rxdataF_ext[aarx][symbol * nb_rb_pdsch * NR_NB_SC_PER_RB];
    int32_t *rxF        = &rxdataF[aarx][symbol * frame_parms->ofdm_symbol_size];

    if (pilots == 0) { //data symbol only
      if (start_re + nb_rb_pdsch * NR_NB_SC_PER_RB <= frame_parms->ofdm_symbol_size) {
        memcpy((void*)rxF_ext, (void*)&rxF[start_re], nb_rb_pdsch * NR_NB_SC_PER_RB * sizeof(int32_t));
      } else {
        int neg_length = frame_parms->ofdm_symbol_size - start_re;
        int pos_length = nb_rb_pdsch * NR_NB_SC_PER_RB - neg_length;

        memcpy((void*)rxF_ext, (void*)&rxF[start_re], neg_length * sizeof(int32_t));
        memcpy((void*)&rxF_ext[neg_length], (void*)rxF, pos_length * sizeof(int32_t));
      }
      memcpy((void*)dl_ch0_ext, (void*)dl_ch0, nb_rb_pdsch * NR_NB_SC_PER_RB * sizeof(int32_t));
    }
    else if (config_type == NFAPI_NR_DMRS_TYPE1){
      if (n_dmrs_cdm_groups == 1) { //data is multiplexed
        unsigned short k = start_re;
        for (unsigned short j = 0; j < 6*nb_rb_pdsch; j += 3) {
          rxF_ext[j]      = rxF[k+1];
          rxF_ext[j+1]    = rxF[k+3];
          rxF_ext[j+2]    = rxF[k+5];
          dl_ch0_ext[j]   = dl_ch0[1];
          dl_ch0_ext[j+1] = dl_ch0[3];
          dl_ch0_ext[j+2] = dl_ch0[5];
          dl_ch0 += 6;
          k += 6;
          if (k >= frame_parms->ofdm_symbol_size)
            k -= frame_parms->ofdm_symbol_size;
        }
      }
    }
    else {//NFAPI_NR_DMRS_TYPE2
      if (n_dmrs_cdm_groups == 1) { //data is multiplexed
        unsigned short k = start_re;
        for (unsigned short j = 0; j < 8*nb_rb_pdsch; j += 4) {
          rxF_ext[j]      = rxF[k+2];
          rxF_ext[j+1]    = rxF[k+3];
          rxF_ext[j+2]    = rxF[k+4];
          rxF_ext[j+3]    = rxF[k+5];
          dl_ch0_ext[j]   = dl_ch0[2];
          dl_ch0_ext[j+1] = dl_ch0[3];
          dl_ch0_ext[j+2] = dl_ch0[4];
          dl_ch0_ext[j+3] = dl_ch0[5];
          dl_ch0 += 6;
          k += 6;
          if (k >= frame_parms->ofdm_symbol_size)
            k -= frame_parms->ofdm_symbol_size;
        }
      }
      else if (n_dmrs_cdm_groups == 2) { //data is multiplexed
        unsigned short k = start_re;
        for (unsigned short j = 0; j < 4*nb_rb_pdsch; j += 2) {
          rxF_ext[j]      = rxF[k+4];
          rxF_ext[j+1]    = rxF[k+5];
          dl_ch0_ext[j]   = dl_ch0[4];
          dl_ch0_ext[j+1] = dl_ch0[5];
          dl_ch0 += 6;
          k += 6;
          if (k >= frame_parms->ofdm_symbol_size)
            k -= frame_parms->ofdm_symbol_size;
        }
      }
    }
  }

  return nb_rb_pdsch;
}

void nr_dlsch_extract_rbs(int **rxdataF,
                          int **dl_ch_estimates,
                          int **rxdataF_ext,
                          int **dl_ch_estimates_ext,
                          unsigned char symbol,
                          uint8_t pilots,
                          uint8_t config_type,
                          unsigned short start_rb,
                          unsigned short nb_rb_pdsch,
                          uint8_t n_dmrs_cdm_groups,
                          uint8_t Nl,
                          NR_DL_FRAME_PARMS *frame_parms,
                          uint16_t dlDmrsSymbPos,
                          int chest_time_type)
{

  unsigned short k,rb;
  unsigned char nushift,j,i,aarx,aatx;
  int *dl_ch0,*dl_ch0_ext,*rxF,*rxF_ext;
  int8_t validDmrsEst = 0; //store last DMRS Symbol index

  if (config_type==NFAPI_NR_DMRS_TYPE1) {
    AssertFatal(n_dmrs_cdm_groups == 1 || n_dmrs_cdm_groups == 2,
                "n_dmrs_cdm_groups %d is illegal\n",n_dmrs_cdm_groups);
    nushift = n_dmrs_cdm_groups -1;//delta in Table 7.4.1.1.2-1
  } else {
    AssertFatal(n_dmrs_cdm_groups == 1 || n_dmrs_cdm_groups == 2 || n_dmrs_cdm_groups == 3,
                "n_dmrs_cdm_groups %d is illegal\n",n_dmrs_cdm_groups);
    nushift = (n_dmrs_cdm_groups -1)<<1;//delta in Table 7.4.1.1.2-2
  }

  if (chest_time_type == 0)
    validDmrsEst = get_valid_dmrs_idx_for_channel_est(dlDmrsSymbPos,symbol);
  else
    validDmrsEst = get_next_dmrs_symbol_in_slot(dlDmrsSymbPos,0,14); // get first dmrs symbol index

  for (aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++) {

    k = frame_parms->first_carrier_offset + NR_NB_SC_PER_RB*start_rb;
    if (k>=frame_parms->ofdm_symbol_size)
      k = k-frame_parms->ofdm_symbol_size;

    rxF_ext   = &rxdataF_ext[aarx][symbol*(nb_rb_pdsch*12)];
    rxF       = &rxdataF[aarx][(k+(symbol*(frame_parms->ofdm_symbol_size)))];

    for (aatx=0; aatx<Nl; aatx++) {

      dl_ch0     = &dl_ch_estimates[(aatx*frame_parms->nb_antennas_rx)+aarx][(validDmrsEst*(frame_parms->ofdm_symbol_size))];
      dl_ch0_ext = &dl_ch_estimates_ext[(aatx*frame_parms->nb_antennas_rx)+aarx][symbol*(nb_rb_pdsch*NR_NB_SC_PER_RB)];

      for (rb = 0; rb < nb_rb_pdsch; rb++)
      {
        if (pilots==0) {//data symbol only
          if (aatx==0) {//Extract Rx signal only
            memcpy((void*)rxF_ext,(void*)rxF,12*sizeof(*rxF_ext));
            rxF_ext+=12;
          }
          memcpy((void*)dl_ch0_ext,(void*)dl_ch0,12*sizeof(*dl_ch0_ext));//Extract Channel Estimate
          dl_ch0_ext+=12;
        }
        else {//the symbol contains DMRS
          j=0;
          if (config_type==NFAPI_NR_DMRS_TYPE1) {
            if (nushift == 0) {//data is multiplexed
              for (i = (1-nushift); i<12; i+=2) {
                if (aatx==0) rxF_ext[j]=rxF[i];
                dl_ch0_ext[j]=dl_ch0[i];
                j++;
              }
              dl_ch0_ext+=6;
              if (aatx==0) rxF_ext+=6;
            }
          }
          else {//NFAPI_NR_DMRS_TYPE2
            for (i = (2+nushift); i<6; i++) {
              if (aatx==0) rxF_ext[j]=rxF[i];
              dl_ch0_ext[j]=dl_ch0[i];
              j++;
            }
            for (i = (8+nushift); i<12; i++) {
              if (aatx==0) rxF_ext[j]=rxF[i];
              dl_ch0_ext[j]=dl_ch0[i];
              j++;
            }
            dl_ch0_ext+= j;
            if (aatx==0) rxF_ext+= j;
          }
        }

        dl_ch0+=12;
        if (aatx==0) {
          rxF+=12;
          k+=12;
          if (k>=frame_parms->ofdm_symbol_size) {
            k=k-(frame_parms->ofdm_symbol_size);
            rxF       = &rxdataF[aarx][k+(symbol*(frame_parms->ofdm_symbol_size))];
          }
        }
      }//rb
    }//aatx
  }//aarx
}

void nr_dlsch_detection_mrc(int **rxdataF_comp,
                            int ***rho,
                            int **dl_ch_mag,
                            int **dl_ch_magb,
                            int **dl_ch_magr,
                            short n_tx,
                            short n_rx,
                            unsigned char symbol,
                            unsigned short nb_rb,
                            int length) {
#if defined(__x86_64__)||defined(__i386__)
  unsigned char aatx, aarx;
  int i;
  __m128i *rxdataF_comp128_0,*rxdataF_comp128_1,*dl_ch_mag128_0,*dl_ch_mag128_1,*dl_ch_mag128_0b,*dl_ch_mag128_1b,*dl_ch_mag128_0r,*dl_ch_mag128_1r;

  uint32_t nb_rb_0 = length/12 + ((length%12)?1:0);

  if (n_rx>1) {
    for (aatx=0; aatx<n_tx; aatx++) {
      rxdataF_comp128_0   = (__m128i *)&rxdataF_comp[(aatx*n_rx)][symbol*nb_rb*12];//aarx=0
      dl_ch_mag128_0      = (__m128i *)&dl_ch_mag[(aatx*n_rx)][symbol*nb_rb*12];//aarx=0
      dl_ch_mag128_0b     = (__m128i *)&dl_ch_magb[(aatx*n_rx)][symbol*nb_rb*12];
      dl_ch_mag128_0r     = (__m128i *)&dl_ch_magr[(aatx*n_rx)][symbol*nb_rb*12];
      for (aarx=1; aarx<n_rx; aarx++) {
        rxdataF_comp128_1   = (__m128i *)&rxdataF_comp[(aatx*n_rx)+aarx][symbol*nb_rb*12];// aarx=1,..., n_rx-1
        dl_ch_mag128_1      = (__m128i *)&dl_ch_mag[(aatx*n_rx)+aarx][symbol*nb_rb*12];
        dl_ch_mag128_1b     = (__m128i *)&dl_ch_magb[(aatx*n_rx)+aarx][symbol*nb_rb*12];
        dl_ch_mag128_1r     = (__m128i *)&dl_ch_magr[(aatx*n_rx)+aarx][symbol*nb_rb*12];

        // MRC on each re of rb, both on MF output and magnitude (for 16QAM/64QAM/256 llr computation)
        for (i=0; i<nb_rb_0*3; i++) {
          rxdataF_comp128_0[i] = _mm_adds_epi16(rxdataF_comp128_0[i],rxdataF_comp128_1[i]);
          dl_ch_mag128_0[i]    = _mm_adds_epi16(dl_ch_mag128_0[i],dl_ch_mag128_1[i]);
          dl_ch_mag128_0b[i]   = _mm_adds_epi16(dl_ch_mag128_0b[i],dl_ch_mag128_1b[i]);
          dl_ch_mag128_0r[i]   = _mm_adds_epi16(dl_ch_mag128_0r[i],dl_ch_mag128_1r[i]);
        }
      }
    }
#ifdef DEBUG_DLSCH_DEMOD
    for (i=0; i<nb_rb_0*3; i++) {
    printf("symbol%d RB %d\n",symbol,i/3);
    rxdataF_comp128_0   = (__m128i *)&rxdataF_comp[0][symbol*nb_rb*12];
    rxdataF_comp128_1   = (__m128i *)&rxdataF_comp[n_rx][symbol*nb_rb*12];
    print_shorts("tx 1 mrc_re/mrc_Im:",(int16_t*)&rxdataF_comp128_0[i]);
    print_shorts("tx 2 mrc_re/mrc_Im:",(int16_t*)&rxdataF_comp128_1[i]);
    // printf("mrc mag0 = %d = %d \n",((int16_t*)&dl_ch_mag128_0[0])[0],((int16_t*)&dl_ch_mag128_0[0])[1]);
    // printf("mrc mag0b = %d = %d \n",((int16_t*)&dl_ch_mag128_0b[0])[0],((int16_t*)&dl_ch_mag128_0b[0])[1]);
    }
#endif
    if (rho) {
      /*rho128_0 = (__m128i *) &rho[0][symbol*frame_parms->N_RB_DL*12];
      rho128_1 = (__m128i *) &rho[1][symbol*frame_parms->N_RB_DL*12];
      for (i=0; i<nb_rb_0*3; i++) {
        //      print_shorts("mrc rho0:",&rho128_0[i]);
        //      print_shorts("mrc rho1:",&rho128_1[i]);
        rho128_0[i] = _mm_adds_epi16(_mm_srai_epi16(rho128_0[i],1),_mm_srai_epi16(rho128_1[i],1));
      }*/
      }
    _mm_empty();
    _m_empty();
  }
#endif
}

/* Zero Forcing Rx function: nr_a_sum_b()
 * Compute the complex addition x=x+y
 *
 * */
void nr_a_sum_b(__m128i *input_x,
                __m128i *input_y,
                unsigned short nb_rb)
{
  unsigned short rb;

  for (rb=0; rb<nb_rb; rb++) {

    input_x[0] = _mm_adds_epi16(input_x[0],input_y[0]);
    input_x[1] = _mm_adds_epi16(input_x[1],input_y[1]);
    input_x[2] = _mm_adds_epi16(input_x[2],input_y[2]);

    input_x+=3;
    input_y+=3;
  }
  _mm_empty();
  _m_empty();
}

/* Zero Forcing Rx function: nr_a_mult_b()
 * Compute the complex Multiplication c=a*b
 *
 * */
void nr_a_mult_b(int *a,
                 int *b,
                 int32_t *c,
                 unsigned short nb_rb,
                 unsigned char output_shift0)
{
  //This function is used to compute complex multiplications
  short nr_conjugate[8]__attribute__((aligned(16))) = {1,-1,1,-1,1,-1,1,-1};
  unsigned short rb;
  __m128i *a_128,*b_128, *c_128, mmtmpD0,mmtmpD1,mmtmpD2,mmtmpD3;

  a_128 = (__m128i *)a;
  b_128 = (__m128i *)b;

  c_128 = (__m128i *)c;

  for (rb=0; rb<3*nb_rb; rb++) {
    // the real part
    mmtmpD0 = _mm_sign_epi16(a_128[0],*(__m128i*)&nr_conjugate[0]);
    mmtmpD0 = _mm_madd_epi16(mmtmpD0,b_128[0]); //Re: (a_re*b_re - a_im*b_im)

    // the imag part
    mmtmpD1 = _mm_shufflelo_epi16(a_128[0],_MM_SHUFFLE(2,3,0,1));
    mmtmpD1 = _mm_shufflehi_epi16(mmtmpD1,_MM_SHUFFLE(2,3,0,1));
    mmtmpD1 = _mm_madd_epi16(mmtmpD1,b_128[0]);//Im: (x_im*y_re + x_re*y_im)

    mmtmpD0 = _mm_srai_epi32(mmtmpD0,output_shift0);
    mmtmpD1 = _mm_srai_epi32(mmtmpD1,output_shift0);
    mmtmpD2 = _mm_unpacklo_epi32(mmtmpD0,mmtmpD1);
    mmtmpD3 = _mm_unpackhi_epi32(mmtmpD0,mmtmpD1);

    c_128[0] = _mm_packs_epi32(mmtmpD2,mmtmpD3);

    /*printf("\n Computing mult \n");
    print_shorts("a:",(int16_t*)&a_128[0]);
    print_shorts("b:",(int16_t*)&b_128[0]);
    print_shorts("pack:",(int16_t*)&c_128[0]);*/

    a_128+=1;
    b_128+=1;
    c_128+=1;
  }
  _mm_empty();
  _m_empty();
}

/* Zero Forcing Rx function: nr_element_sign()
 * Compute b=sign*a
 *
 * */
void nr_element_sign(int32_t *a,//a
                     int32_t *b,//b
                     unsigned short nb_rb,
                     int32_t sign)
{
  int16_t nr_sign[8]__attribute__((aligned(16))) = {-1,-1,-1,-1,-1,-1,-1,-1} ;
  unsigned short rb;
  __m128i *a_128,*b_128;

  a_128 = (__m128i *)a;
  b_128 = (__m128i *)b;

  for (rb=0; rb<3*nb_rb; rb++) {

    if (sign < 0)
      b_128[0] = _mm_sign_epi16(a_128[0],*(__m128i*)&nr_sign[0]);
    else
      b_128[0] = a_128[0];

#ifdef DEBUG_DLSCH_DEMOD
     printf("\n Out \n");
     //print_ints("det_re_128:",(int32_t*)&det_re_128);
     //print_ints("det_im_128:",(int32_t*)&det_im_128);
     print_shorts("b:",(int32_t*)&b_128[0]);
#endif
    a_128+=1;
    b_128+=1;
  }
  _mm_empty();
  _m_empty();
}

/* Zero Forcing Rx function: nr_det_4x4()
 * Compute the matrix determinant for 4x4 Matrix
 *
 * */
void nr_determin(int32_t **a44,//
                 int32_t *ad_bc,//ad-bc
                 int32_t size,
                 unsigned short nb_rb,
                 int32_t sign,
                 int32_t shift0){

  int32_t outtemp[12*nb_rb] __attribute__((aligned(32)));
  int32_t outtemp1[12*nb_rb] __attribute__((aligned(32)));
  int32_t **sub_matrix;
  sub_matrix = (int32_t **)malloc16_clear( (size-1)*(size-1)*sizeof(int32_t *) );
  for (int rtx=0;rtx<(size-1);rtx++) {//row
    for (int ctx=0;ctx<(size-1);ctx++) {//column
      sub_matrix[ctx*(size-1)+rtx] = (int32_t *)malloc16_clear( 12*nb_rb*sizeof(int32_t) );
    }
  }
  int16_t k,rr[size-1],cc[size-1];

  if(size==1) {
    nr_element_sign(a44[0],//a
                    ad_bc,//b
                    nb_rb,
                    sign);
  } else {

    for (int rtx=0;rtx<size;rtx++) {//row calculation for determin
      int ctx=0;
      //find the submatrix row and column indices
      k=0;
      for(int rrtx=0;rrtx<size;rrtx++)
        if(rrtx != rtx) rr[k++] = rrtx;
      k=0;
      for(int cctx=0;cctx<size;cctx++)
        if(cctx != ctx) cc[k++] = cctx;
      //fill out the sub matrix corresponds to this element
       for (int ridx=0;ridx<(size-1);ridx++)
         for (int cidx=0;cidx<(size-1);cidx++)
           sub_matrix[cidx*(size-1)+ridx]= (int32_t *)&a44[cc[cidx]*size+rr[ridx]][0];

       nr_determin(sub_matrix,//a33
                   outtemp,
                   size-1,
                   nb_rb,
                   ((rtx&1)==1?-1:1)*((ctx&1)==1?-1:1)*sign,
                   shift0);
       nr_a_mult_b(a44[ctx*size+rtx],
                   outtemp,
                   rtx==0? ad_bc:outtemp1,
                   nb_rb,
                   shift0);

       if (rtx != 0)
         nr_a_sum_b((__m128i *)ad_bc,
                    (__m128i *)outtemp1,
                    nb_rb);
    }
  }
  _mm_empty();
  _m_empty();
}

double complex nr_determin_cpx(double complex *a44_cpx,//
                               int32_t size,//size
                               int32_t sign){
  double complex outtemp, outtemp1;
  //Allocate the submatrix elements
  double complex sub_matrix[(size-1)*(size-1)];
  int16_t k,rr[size-1],cc[size-1];

  if(size==1) {
    return((double complex)a44_cpx[0]*sign);
  }else {
    outtemp1 = 0;
    for (int rtx=0;rtx<size;rtx++) {//row calculation for determin
      int ctx=0;
      //find the submatrix row and column indices
      k=0;
      for(int rrtx=0;rrtx<size;rrtx++)
        if(rrtx != rtx) rr[k++] = rrtx;
      k=0;
      for(int cctx=0;cctx<size;cctx++)
        if(cctx != ctx) cc[k++] = cctx;
      //fill out the sub matrix corresponds to this element
       for (int ridx=0;ridx<(size-1);ridx++)
         for (int cidx=0;cidx<(size-1);cidx++)
           sub_matrix[cidx*(size-1)+ridx]= a44_cpx[cc[cidx]*size+rr[ridx]];

       outtemp = nr_determin_cpx(sub_matrix,//a33
                             size-1,
                             ((rtx&1)==1?-1:1)*((ctx&1)==1?-1:1)*sign);
       outtemp1 += a44_cpx[ctx*size+rtx]*outtemp;
    }

    return((double complex)outtemp1);
  }
}

/* Zero Forcing Rx function: nr_matrix_inverse()
 * Compute the matrix inverse and determinant up to 4x4 Matrix
 *
 * */
uint8_t nr_matrix_inverse(int32_t **a44,//Input matrix//conjH_H_elements[0]
                          int32_t **inv_H_h_H,//Inverse
                          int32_t *ad_bc,//determin
                          int32_t size,
                          unsigned short nb_rb,
                          int32_t flag,//fixed point or floating flag
                          int32_t shift0){

  int16_t k,rr[size-1],cc[size-1];

  if(flag) {//fixed point SIMD calc.
    //Allocate the submatrix elements
    int32_t **sub_matrix;
    sub_matrix = (int32_t **)malloc16_clear( (size-1)*(size-1)*sizeof(int32_t *) );
    for (int rtx=0;rtx<(size-1);rtx++) {//row
      for (int ctx=0;ctx<(size-1);ctx++) {//column
        sub_matrix[ctx*(size-1)+rtx] = (int32_t *)malloc16_clear( 12*nb_rb*sizeof(int32_t) );
      }
    }

    //Compute Matrix determinant
    nr_determin(a44,//
                ad_bc,//determinant
                size,//size
                nb_rb,
                +1,
                shift0);
    //print_shorts("nr_det_",(int16_t*)&ad_bc[0]);

    //Compute Inversion of the H^*H matrix
    /* For 2x2 MIMO matrix, we compute
     * *        |(conj_H_00xH_00+conj_H_10xH_10)   (conj_H_00xH_01+conj_H_10xH_11)|
     * * H_h_H= |                                                                 |
     * *        |(conj_H_01xH_00+conj_H_11xH_10)   (conj_H_01xH_01+conj_H_11xH_11)|
     * *
     * *inv(H_h_H) =(1/det)*[d  -b
     * *                     -c  a]
     * **************************************************************************/
    for (int rtx=0;rtx<size;rtx++) {//row
      k=0;
      for(int rrtx=0;rrtx<size;rrtx++)
        if(rrtx != rtx) rr[k++] = rrtx;
      for (int ctx=0;ctx<size;ctx++) {//column
        k=0;
        for(int cctx=0;cctx<size;cctx++)
          if(cctx != ctx) cc[k++] = cctx;

        //fill out the sub matrix corresponds to this element
        for (int ridx=0;ridx<(size-1);ridx++)
          for (int cidx=0;cidx<(size-1);cidx++)
            sub_matrix[cidx*(size-1)+ridx]= (int32_t *)&a44[cc[cidx]*size+rr[ridx]][0];

        nr_determin(sub_matrix,
                    inv_H_h_H[rtx*size+ctx],//out transpose
                    size-1,//size
                    nb_rb,
                    ((rtx&1)==1?-1:1)*((ctx&1)==1?-1:1),
                    shift0);
        //printf("H_h_H(r%d,c%d)=%d+j%d --> inv_H_h_H(%d,%d) = %d+j%d \n",rtx,ctx,((short *)a44[ctx*size+rtx])[0],((short *)a44[ctx*size+rtx])[1],ctx,rtx,((short *)inv_H_h_H[rtx*size+ctx])[0],((short *)inv_H_h_H[rtx*size+ctx])[1]);
      }
    }
    _mm_empty();
    _m_empty();
  }
  else {//floating point calc.
    //Allocate the submatrix elements
    double complex sub_matrix_cpx[(size-1)*(size-1)];
    //Convert the IQ samples (in Q15 format) to float complex
    double complex a44_cpx[size*size];
    double complex inv_H_h_H_cpx[size*size];
    double complex determin_cpx;
    for (int i=0; i<12*nb_rb; i++) {

      //Convert Q15 to floating point
      for (int rtx=0;rtx<size;rtx++) {//row
        for (int ctx=0;ctx<size;ctx++) {//column
          a44_cpx[ctx*size+rtx]= ((double)((short *)a44[ctx*size+rtx])[(i<<1)])/(1<<(shift0-1)) + I*((double)((short *)a44[ctx*size+rtx])[(i<<1)+1])/(1<<(shift0-1));
          //if (i<4) printf("a44_cpx(%d,%d)= ((FP %d))%lf+(FP %d)j%lf \n",ctx,rtx,((short *)a44[ctx*size+rtx])[(i<<1)],creal(a44_cpx[ctx*size+rtx]),((short *)a44[ctx*size+rtx])[(i<<1)+1],cimag(a44_cpx[ctx*size+rtx]));
        }
      }
      //Compute Matrix determinant (copy real value only)
      determin_cpx = nr_determin_cpx(a44_cpx,//
                                     size,//size
                                     +1);
      //if (i<4) printf("order %d nr_det_cpx = %lf+j%lf \n",log2_approx(creal(determin_cpx)),creal(determin_cpx),cimag(determin_cpx));

      //Round and convert to Q15 (Out in the same format as Fixed point).
      if (creal(determin_cpx)>0) {//determin of the symmetric matrix is real part only
        ((short*) ad_bc)[i<<1] = (short) ((creal(determin_cpx)*(1<<(shift0)))+0.5);//
        //((short*) ad_bc)[(i<<1)+1] = (short) ((cimag(determin_cpx)*(1<<(shift0)))+0.5);//
      } else {
        ((short*) ad_bc)[i<<1] = (short) ((creal(determin_cpx)*(1<<(shift0)))-0.5);//
        //((short*) ad_bc)[(i<<1)+1] = (short) ((cimag(determin_cpx)*(1<<(shift0)))-0.5);//
      }
      //if (i<4) printf("nr_det_FP= %d+j%d \n",((short*) ad_bc)[i<<1],((short*) ad_bc)[(i<<1)+1]);
      //Compute Inversion of the H^*H matrix (normalized output divide by determinant)
      for (int rtx=0;rtx<size;rtx++) {//row
        k=0;
        for(int rrtx=0;rrtx<size;rrtx++)
          if(rrtx != rtx) rr[k++] = rrtx;
        for (int ctx=0;ctx<size;ctx++) {//column
          k=0;
          for(int cctx=0;cctx<size;cctx++)
            if(cctx != ctx) cc[k++] = cctx;

          //fill out the sub matrix corresponds to this element
          for (int ridx=0;ridx<(size-1);ridx++)
            for (int cidx=0;cidx<(size-1);cidx++)
              sub_matrix_cpx[cidx*(size-1)+ridx]= a44_cpx[cc[cidx]*size+rr[ridx]];

          inv_H_h_H_cpx[rtx*size+ctx] = nr_determin_cpx(sub_matrix_cpx,//
                                                        size-1,//size
                                                        ((rtx&1)==1?-1:1)*((ctx&1)==1?-1:1));
          //if (i==0) printf("H_h_H(r%d,c%d)=%lf+j%lf --> inv_H_h_H(%d,%d) = %lf+j%lf \n",rtx,ctx,creal(a44_cpx[ctx*size+rtx]),cimag(a44_cpx[ctx*size+rtx]),ctx,rtx,creal(inv_H_h_H_cpx[rtx*size+ctx]),cimag(inv_H_h_H_cpx[rtx*size+ctx]));

          if (creal(inv_H_h_H_cpx[rtx*size+ctx])>0)
            ((short *) inv_H_h_H[rtx*size+ctx])[i<<1] = (short) ((creal(inv_H_h_H_cpx[rtx*size+ctx])*(1<<(shift0-1)))+0.5);//Convert to Q 18
          else
            ((short *) inv_H_h_H[rtx*size+ctx])[i<<1] = (short) ((creal(inv_H_h_H_cpx[rtx*size+ctx])*(1<<(shift0-1)))-0.5);//

          if (cimag(inv_H_h_H_cpx[rtx*size+ctx])>0)
            ((short *) inv_H_h_H[rtx*size+ctx])[(i<<1)+1] = (short) ((cimag(inv_H_h_H_cpx[rtx*size+ctx])*(1<<(shift0-1)))+0.5);//
          else
            ((short *) inv_H_h_H[rtx*size+ctx])[(i<<1)+1] = (short) ((cimag(inv_H_h_H_cpx[rtx*size+ctx])*(1<<(shift0-1)))-0.5);//

          //if (i<4) printf("inv_H_h_H_FP(%d,%d)= %d+j%d \n",ctx,rtx, ((short *) inv_H_h_H[rtx*size+ctx])[i<<1],((short *) inv_H_h_H[rtx*size+ctx])[(i<<1)+1]);
        }
      }
    }
  }
  return(0);
}

/* Zero Forcing Rx function: nr_conjch0_mult_ch1()
 *
 *
 * */
void nr_conjch0_mult_ch1(int *ch0,
                         int *ch1,
                         int32_t *ch0conj_ch1,
                         unsigned short nb_rb,
                         unsigned char output_shift0)
{
  //This function is used to compute multiplications in H_hermitian * H matrix
  short nr_conjugate[8]__attribute__((aligned(16))) = {-1,1,-1,1,-1,1,-1,1};
  unsigned short rb;
  __m128i *dl_ch0_128,*dl_ch1_128, *ch0conj_ch1_128, mmtmpD0,mmtmpD1,mmtmpD2,mmtmpD3;

  dl_ch0_128 = (__m128i *)ch0;
  dl_ch1_128 = (__m128i *)ch1;

  ch0conj_ch1_128 = (__m128i *)ch0conj_ch1;

  for (rb=0; rb<3*nb_rb; rb++) {

    mmtmpD0 = _mm_madd_epi16(dl_ch0_128[0],dl_ch1_128[0]);
    mmtmpD1 = _mm_shufflelo_epi16(dl_ch0_128[0],_MM_SHUFFLE(2,3,0,1));
    mmtmpD1 = _mm_shufflehi_epi16(mmtmpD1,_MM_SHUFFLE(2,3,0,1));
    mmtmpD1 = _mm_sign_epi16(mmtmpD1,*(__m128i*)&nr_conjugate[0]);
    mmtmpD1 = _mm_madd_epi16(mmtmpD1,dl_ch1_128[0]);
    mmtmpD0 = _mm_srai_epi32(mmtmpD0,output_shift0);
    mmtmpD1 = _mm_srai_epi32(mmtmpD1,output_shift0);
    mmtmpD2 = _mm_unpacklo_epi32(mmtmpD0,mmtmpD1);
    mmtmpD3 = _mm_unpackhi_epi32(mmtmpD0,mmtmpD1);

    ch0conj_ch1_128[0] = _mm_packs_epi32(mmtmpD2,mmtmpD3);

    /*printf("\n Computing conjugates \n");
    print_shorts("ch0:",(int16_t*)&dl_ch0_128[0]);
    print_shorts("ch1:",(int16_t*)&dl_ch1_128[0]);
    print_shorts("pack:",(int16_t*)&ch0conj_ch1_128[0]);*/

    dl_ch0_128+=1;
    dl_ch1_128+=1;
    ch0conj_ch1_128+=1;
  }
  _mm_empty();
  _m_empty();
}

/* Zero Forcing Rx function: up to 4 layers
 *
 *
 * */
uint8_t nr_zero_forcing_rx(int **rxdataF_comp,
                           int **dl_ch_mag,
                           int **dl_ch_magb,
                           int **dl_ch_magr,
                           int **dl_ch_estimates_ext,
                           unsigned short nb_rb,
                           unsigned char n_rx,
                           unsigned char n_tx,//number of layer
                           unsigned char mod_order,
                           int shift,
                           unsigned char symbol,
                           int length)
{
  int *ch0r, *ch0c;
  int32_t *** conjH_H_elements;
  uint32_t nb_rb_0 = length/12 + ((length%12)?1:0);
  int32_t determ_fin[12*nb_rb_0] __attribute__((aligned(32)));

  ///Allocate H^*H matrix elements and sub elements
  conjH_H_elements        = (int32_t ***)malloc16_clear( n_rx*sizeof(int32_t **) );
  for (int aarx=0;aarx<n_rx;aarx++) {
    conjH_H_elements[aarx] = (int32_t **)malloc16_clear( n_tx*n_tx*sizeof(int32_t) );
    for (int rtx=0;rtx<n_tx;rtx++) {//row
      for (int ctx=0;ctx<n_tx;ctx++) {//column
        conjH_H_elements[aarx][ctx*n_tx+rtx] = (int32_t *)malloc16_clear( 12*nb_rb_0*sizeof(int32_t *) );
      }
    }
  }

  //Compute H^*H matrix elements and sub elements:(1/2^log2_maxh)*conjH_H_elements
  for (int rtx=0;rtx<n_tx;rtx++) {//row
    for (int ctx=0;ctx<n_tx;ctx++) {//column
      for (int aarx=0;aarx<n_rx;aarx++)  {
        ch0r = (int *)&dl_ch_estimates_ext[rtx*n_rx+aarx][symbol*nb_rb*12];//[]//conjch00,01,02,03
        ch0c = (int *)&dl_ch_estimates_ext[ctx*n_rx+aarx][symbol*nb_rb*12];//[aatx*n_rx+aarx]//ch00: 01,02,03
        nr_conjch0_mult_ch1(ch0r,
                            ch0c,
                            conjH_H_elements[aarx][ctx*n_tx+rtx],
                            nb_rb_0,
                            shift);
        if (aarx !=0) nr_a_sum_b((__m128i *)conjH_H_elements[0][ctx*n_tx+rtx],
                                 (__m128i *)conjH_H_elements[aarx][ctx*n_tx+rtx],
                                 nb_rb_0);
      }
    }
  }

  //Compute the inverse and determinant of the H^*H matrix
  //Allocate the inverse matrix
  int32_t ** inv_H_h_H;
  inv_H_h_H = (int32_t **)malloc16_clear( n_tx*n_tx*sizeof(int32_t *) );
  for (int rtx=0;rtx<n_tx;rtx++) {//row
    for (int ctx=0;ctx<n_tx;ctx++) {//column
      inv_H_h_H[ctx*n_tx+rtx] = (int32_t *)malloc16_clear( 12*nb_rb_0*sizeof(int32_t) );
    }
  }
  int fp_flag = 1;//0: float point calc 1: Fixed point calc
  nr_matrix_inverse(conjH_H_elements[0],//Input matrix
                    inv_H_h_H,//Inverse
                    determ_fin,//determin
                    n_tx,//size
                    nb_rb_0,
                    fp_flag,//fixed point flag
                    shift-(fp_flag==1?2:0));//the out put is Q15

  // multiply Matrix inversion pf H_h_H by the rx signal vector
  int32_t outtemp[12*nb_rb_0] __attribute__((aligned(32)));
  int32_t **rxdataF_zforcing;
  //Allocate rxdataF for zforcing out
  rxdataF_zforcing        = (int32_t **)malloc16_clear( n_tx*sizeof(int32_t *) );
   for (int rtx=0;rtx<n_tx;rtx++) {//row
     rxdataF_zforcing[rtx] = (int32_t *)malloc16_clear( 12*nb_rb_0*sizeof(int32_t) );
   }

  for (int rtx=0;rtx<n_tx;rtx++) {//Output Layers row
    // loop over Layers rtx=0,...,N_Layers-1
      for (int ctx=0;ctx<n_tx;ctx++) {//column multi
        //printf("Computing r_%d c_%d\n",rtx,ctx);
        //print_shorts(" H_h_H=",(int16_t*)&conjH_H_elements[ctx*n_tx+rtx][0][0]);
        //print_shorts(" Inv_H_h_H=",(int16_t*)&inv_H_h_H[ctx*n_tx+rtx][0]);
        nr_a_mult_b(inv_H_h_H[ctx*n_tx+rtx],
                    (int *)&rxdataF_comp[ctx*n_rx][symbol*nb_rb*12],
                    outtemp,
                    nb_rb_0,
                    shift-(fp_flag==1?2:0));
        nr_a_sum_b((__m128i *)rxdataF_zforcing[rtx],
                   (__m128i *)outtemp,
                   nb_rb_0);//a =a + b
        }
#ifdef DEBUG_DLSCH_DEMOD
    printf("Computing layer_%d \n",rtx);;
    print_shorts(" Rx signal:=",(int16_t*)&rxdataF_zforcing[rtx][0]);
    print_shorts(" Rx signal:=",(int16_t*)&rxdataF_zforcing[rtx][4]);
    print_shorts(" Rx signal:=",(int16_t*)&rxdataF_zforcing[rtx][8]);
#endif
    }

  //Copy zero_forcing out to output array
  for (int rtx=0;rtx<n_tx;rtx++)
    nr_element_sign(rxdataF_zforcing[rtx],
                    (int *)&rxdataF_comp[rtx*n_rx][symbol*nb_rb*12],
                    nb_rb_0,
                    +1);

  //Update LLR thresholds with the Matrix determinant
  __m128i *dl_ch_mag128_0=NULL,*dl_ch_mag128b_0=NULL,*dl_ch_mag128r_0=NULL,*determ_fin_128;
  __m128i mmtmpD2,mmtmpD3;
  __m128i QAM_amp128={0},QAM_amp128b={0},QAM_amp128r={0};
  short nr_realpart[8]__attribute__((aligned(16))) = {1,0,1,0,1,0,1,0};
  determ_fin_128      = (__m128i *)&determ_fin[0];

  if (mod_order>2) {
    if (mod_order == 4) {
      QAM_amp128 = _mm_set1_epi16(QAM16_n1);  //2/sqrt(10)
      QAM_amp128b = _mm_setzero_si128();
      QAM_amp128r = _mm_setzero_si128();
    } else if (mod_order == 6) {
      QAM_amp128  = _mm_set1_epi16(QAM64_n1); //4/sqrt{42}
      QAM_amp128b = _mm_set1_epi16(QAM64_n2); //2/sqrt{42}
      QAM_amp128r = _mm_setzero_si128();
    } else if (mod_order == 8) {
      QAM_amp128 = _mm_set1_epi16(QAM256_n1); //8/sqrt{170}
      QAM_amp128b = _mm_set1_epi16(QAM256_n2);//4/sqrt{170}
      QAM_amp128r = _mm_set1_epi16(QAM256_n3);//2/sqrt{170}
    }
    dl_ch_mag128_0      = (__m128i *)&dl_ch_mag[0][symbol*nb_rb*12];
    dl_ch_mag128b_0     = (__m128i *)&dl_ch_magb[0][symbol*nb_rb*12];
    dl_ch_mag128r_0     = (__m128i *)&dl_ch_magr[0][symbol*nb_rb*12];

    for (int rb=0; rb<3*nb_rb_0; rb++) {
      //for symmetric H_h_H matrix, the determinant is only real values
        mmtmpD2 = _mm_sign_epi16(determ_fin_128[0],*(__m128i*)&nr_realpart[0]);//set imag part to 0
        mmtmpD3 = _mm_shufflelo_epi16(mmtmpD2,_MM_SHUFFLE(2,3,0,1));
        mmtmpD3 = _mm_shufflehi_epi16(mmtmpD3,_MM_SHUFFLE(2,3,0,1));
        mmtmpD2 = _mm_add_epi16(mmtmpD2,mmtmpD3);

        dl_ch_mag128_0[0] = mmtmpD2;
        dl_ch_mag128b_0[0] = mmtmpD2;
        dl_ch_mag128r_0[0] = mmtmpD2;

        dl_ch_mag128_0[0] = _mm_mulhi_epi16(dl_ch_mag128_0[0],QAM_amp128);
        dl_ch_mag128_0[0] = _mm_slli_epi16(dl_ch_mag128_0[0],1);

        dl_ch_mag128b_0[0] = _mm_mulhi_epi16(dl_ch_mag128b_0[0],QAM_amp128b);
        dl_ch_mag128b_0[0] = _mm_slli_epi16(dl_ch_mag128b_0[0],1);
        dl_ch_mag128r_0[0] = _mm_mulhi_epi16(dl_ch_mag128r_0[0],QAM_amp128r);
        dl_ch_mag128r_0[0] = _mm_slli_epi16(dl_ch_mag128r_0[0],1);


      determ_fin_128 += 1;
      dl_ch_mag128_0 += 1;
      dl_ch_mag128b_0 += 1;
      dl_ch_mag128r_0 += 1;
    }
  }

  _mm_empty();
  _m_empty();
  return(0);
}

static void nr_dlsch_layer_demapping(int16_t **llr_cw,
				     uint8_t Nl,
				     uint8_t mod_order,
				     uint32_t length,
				     int32_t codeword_TB0,
				     int32_t codeword_TB1,
				     int16_t **llr_layers) {

  switch (Nl) {
    case 1:
      if (codeword_TB1 == -1)
        memcpy((void*)llr_cw[0], (void*)llr_layers[0], (length)*sizeof(int16_t));
      else if (codeword_TB0 == -1)
        memcpy((void*)llr_cw[1], (void*)llr_layers[0], (length)*sizeof(int16_t));

    break;

    case 2:
    case 3:
    case 4:
      for (int i=0; i<(length/Nl/mod_order); i++){
        for (int l=0; l<Nl; l++) {
          for (int m=0; m<mod_order; m++){
            if (codeword_TB1 == -1)
              llr_cw[0][Nl*mod_order*i+l*mod_order+m] = llr_layers[l][i*mod_order+m];//i:0 -->0 1 2 3
            else if (codeword_TB0 == -1)
              llr_cw[1][Nl*mod_order*i+l*mod_order+m] = llr_layers[l][i*mod_order+m];//i:0 -->0 1 2 3
            //if (i<4) printf("length%d: llr_layers[l%d][m%d]=%d: \n",length,l,m,llr_layers[l][i*mod_order+m]);
            }
          }
        }
    break;

  default:
  AssertFatal(0, "Not supported number of layers %d\n", Nl);
  }
}

static int nr_dlsch_llr(NR_UE_PDSCH **pdsch_vars,
                        NR_DL_FRAME_PARMS *frame_parms,
                        int32_t **rxdataF_comp_ptr,
                        int32_t **dl_ch_mag_ptr,
                        NR_DL_UE_HARQ_t *dlsch0_harq,
                        NR_DL_UE_HARQ_t *dlsch1_harq,
                        RX_type_t rx_type,
                        unsigned char harq_pid,
                        unsigned char gNB_id,
                        unsigned char gNB_id_i,
                        unsigned char first_symbol_flag,
                        unsigned char symbol,
                        unsigned short nb_rb,
                        int32_t codeword_TB0,
                        int32_t codeword_TB1,
                        uint32_t len,
                        uint8_t nr_slot_rx,
                        uint8_t beamforming_mode)
{
  uint32_t llr_offset_symbol;
  
  if (first_symbol_flag==1)
    pdsch_vars[gNB_id]->llr_offset[symbol-1] = 0;
  llr_offset_symbol = pdsch_vars[gNB_id]->llr_offset[symbol-1];

  pdsch_vars[gNB_id]->llr_offset[symbol] = len*dlsch0_harq->Qm + llr_offset_symbol;
 
  /*LOG_I(PHY,"compute LLRs [symbol %d] NbRB %d Qm %d LLRs-Length %d LLR-Offset %d @LLR Buff %x @LLR Buff(symb) %x\n",
    symbol,
    nb_rb,dlsch0_harq->Qm,
    pdsch_vars[gNB_id]->llr_length[symbol],
    pdsch_vars[gNB_id]->llr_offset[symbol],
    (int16_t*)pdsch_vars[gNB_id]->llr[0],
    pllr_symbol_cw0);*/

  switch (dlsch0_harq->Qm) {
  case 2 :
    switch (rx_type) {
      case rx_standard :
        for(int l =0; l<dlsch0_harq->Nl; l++)
          nr_dlsch_qpsk_llr(frame_parms,
                            pdsch_vars[gNB_id]->rxdataF_comp0[l*frame_parms->nb_antennas_rx],
                            pdsch_vars[gNB_id]->layer_llr[l]+llr_offset_symbol,
                            symbol,
                            len,
                            first_symbol_flag,
                            nb_rb,
                            beamforming_mode);
        break;
      case rx_IC_single_stream ://From LTE Code!!
        //not implemented yet
        break;
      case rx_IC_dual_stream :
        //not implemented yet
        break;
      case rx_SIC_dual_stream :
        //not implemented yet
        break;
    }
    break;
  case 4 :
    switch (rx_type) {
      case rx_standard :
        for(int l =0; l<dlsch0_harq->Nl; l++)
          nr_dlsch_16qam_llr(frame_parms,
                             pdsch_vars[gNB_id]->rxdataF_comp0[l*frame_parms->nb_antennas_rx],
                             pdsch_vars[gNB_id]->layer_llr[l]+llr_offset_symbol,
                             pdsch_vars[gNB_id]->dl_ch_mag0[0],
                             symbol,
                             len,
                             first_symbol_flag,
                             nb_rb,
                             beamforming_mode);
        break;
      case rx_IC_single_stream ://not implemented yet
        break;
      case rx_IC_dual_stream ://not implemented yet
        break;
      case rx_SIC_dual_stream ://not implemented yet
        break;
    }
    break;
  case 6 :
    switch (rx_type) {
      case rx_standard :
        for(int l =0; l<dlsch0_harq->Nl; l++)
          nr_dlsch_64qam_llr(frame_parms,
                             pdsch_vars[gNB_id]->rxdataF_comp0[l*frame_parms->nb_antennas_rx],
                             pdsch_vars[gNB_id]->layer_llr[l]+llr_offset_symbol,
                             pdsch_vars[gNB_id]->dl_ch_mag0[0],
                             pdsch_vars[gNB_id]->dl_ch_magb0[0],
                             symbol,
                             len,
                             first_symbol_flag,
                             nb_rb,
                             beamforming_mode);
        break;
      case rx_IC_single_stream ://not implemented yet
        break;
      case rx_IC_dual_stream ://not implemented yet
        break;
      case rx_SIC_dual_stream ://not implemented yet
        break;
    }

    break;
  case 8:
    switch (rx_type) {
      case rx_standard :
        for(int l =0; l<dlsch0_harq->Nl; l++)
          nr_dlsch_256qam_llr(frame_parms,
                              pdsch_vars[gNB_id]->rxdataF_comp0[l*frame_parms->nb_antennas_rx],
                              pdsch_vars[gNB_id]->layer_llr[l]+llr_offset_symbol,
                              pdsch_vars[gNB_id]->dl_ch_mag0[0],
                              pdsch_vars[gNB_id]->dl_ch_magb0[0],
                              pdsch_vars[gNB_id]->dl_ch_magr0[0],
                              symbol,
                              len,
                              first_symbol_flag,
                              nb_rb,
                              beamforming_mode);
        break;
      case rx_IC_single_stream ://not implemented yet
        break;
      case rx_IC_dual_stream ://not implemented yet
        break;
      case rx_SIC_dual_stream ://not implemented yet
        break;
    }
    break;
  default:
    LOG_W(PHY,"rx_dlsch.c : Unknown mod_order!!!!\n");
    return(-1);
    break;
  }

  if (dlsch1_harq) {
    switch (dlsch1_harq->Qm) {
    case 2 :
      if (rx_type==rx_standard) {
        nr_dlsch_qpsk_llr(frame_parms,
                          pdsch_vars[gNB_id]->rxdataF_comp0[0],
                          pdsch_vars[gNB_id]->layer_llr[0]+llr_offset_symbol,
                          symbol,
                          len,
                          first_symbol_flag,
                          nb_rb,
                          beamforming_mode);
      }
      break;
    case 4:
      if (rx_type==rx_standard) {
        nr_dlsch_16qam_llr(frame_parms,
                           pdsch_vars[gNB_id]->rxdataF_comp0[0],
                           pdsch_vars[gNB_id]->layer_llr[0]+llr_offset_symbol,
                           pdsch_vars[gNB_id]->dl_ch_mag0[0],
                           symbol,
                           len,
                           first_symbol_flag,
                           nb_rb,
                           beamforming_mode);
      }
      break;
    case 6 :
      if (rx_type==rx_standard) {
        nr_dlsch_64qam_llr(frame_parms,
                           pdsch_vars[gNB_id]->rxdataF_comp0[0],
                           pdsch_vars[gNB_id]->layer_llr[0]+llr_offset_symbol,
                           pdsch_vars[gNB_id]->dl_ch_mag0[0],
                           pdsch_vars[gNB_id]->dl_ch_magb0[0],
                           symbol,
                           len,
                           first_symbol_flag,
                           nb_rb,
                           beamforming_mode);
        }
        break;
    case 8 :
      if (rx_type==rx_standard) {
        nr_dlsch_256qam_llr(frame_parms,
                            pdsch_vars[gNB_id]->rxdataF_comp0[0],
                            pdsch_vars[gNB_id]->layer_llr[0]+llr_offset_symbol,
                            pdsch_vars[gNB_id]->dl_ch_mag0[0],
                            pdsch_vars[gNB_id]->dl_ch_magb0[0],
                            pdsch_vars[gNB_id]->dl_ch_magr0[0],
                            symbol,
                            len,
                            first_symbol_flag,
                            nb_rb,
                            beamforming_mode);
      }
        break;
      default:
        LOG_W(PHY,"rx_dlsch.c : Unknown mod_order!!!!\n");
        return(-1);
        break;
    }
  }
  return 0;
}
//==============================================================================================


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
