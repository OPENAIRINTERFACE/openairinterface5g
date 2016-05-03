/*******************************************************************************
    OpenAirInterface
    Copyright(c) 1999 - 2014 Eurecom

    OpenAirInterface is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.


    OpenAirInterface is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with OpenAirInterface.The full GNU General Public License is
   included in this distribution in the file called "COPYING". If not,
   see <http://www.gnu.org/licenses/>.

  Contact Information
  OpenAirInterface Admin: openair_admin@eurecom.fr
  OpenAirInterface Tech : openair_tech@eurecom.fr
  OpenAirInterface Dev  : openair4g-devel@lists.eurecom.fr

  Address      : Eurecom, Campus SophiaTech, 450 Route des Chappes, CS 50193 - 06904 Biot Sophia Antipolis cedex, FRANCE

*******************************************************************************/

/*! \file PHY/LTE_TRANSPORT/dlsch_demodulation.c
 * \brief Top-level routines for demodulating the PDSCH physical channel from 36-211, V8.6 2009-03
 * \author R. Knopp, F. Kaltenberger,A. Bhamri, S. Aubert
 * \date 2011
 * \version 0.1
 * \company Eurecom
 * \email: knopp@eurecom.fr,florian.kaltenberger@eurecom.fr,ankit.bhamri@eurecom.fr,sebastien.aubert@eurecom.fr
 * \note
 * \warning
 */

#include "PHY/defs.h"
#include "PHY/extern.h"
#include "defs.h"
#include "extern.h"
#include "PHY/sse_intrin.h"


#ifndef USER_MODE
#define NOCYGWIN_STATIC static
#else
#define NOCYGWIN_STATIC
#endif

//#define DEBUG_PHY 1
//#define DEBUG_DLSCH_DEMOD 1

int avg[4]; 

// [MCS][i_mod (0,1,2) = (2,4,6)]
unsigned char offset_mumimo_llr_drange_fix=0;
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


int rx_pdsch(PHY_VARS_UE *phy_vars_ue,
             PDSCH_t type,
             unsigned char eNB_id,
             unsigned char eNB_id_i, //if this == phy_vars_ue->n_connected_eNB, we assume MU interference
             uint8_t subframe,
             unsigned char symbol,
             unsigned char first_symbol_flag,
             unsigned char dual_stream_flag,
             unsigned char i_mod,
             unsigned char harq_pid)
{

  LTE_UE_COMMON *lte_ue_common_vars  = &phy_vars_ue->lte_ue_common_vars;
  LTE_UE_PDSCH **lte_ue_pdsch_vars;
  LTE_DL_FRAME_PARMS *frame_parms    = &phy_vars_ue->lte_frame_parms;
  PHY_MEASUREMENTS *phy_measurements = &phy_vars_ue->PHY_measurements;
  LTE_UE_DLSCH_t   **dlsch_ue;

  unsigned char aatx,aarx;
  unsigned short nb_rb;
  int avgs, rb;
  LTE_DL_UE_HARQ_t *dlsch0_harq,*dlsch1_harq = 0;
  uint32_t *rballoc;

  switch (type) {
  case SI_PDSCH:
    lte_ue_pdsch_vars = &phy_vars_ue->lte_ue_pdsch_vars_SI[eNB_id];
    dlsch_ue          = &phy_vars_ue->dlsch_ue_SI[eNB_id];
    dlsch0_harq       = dlsch_ue[0]->harq_processes[harq_pid];
    break;

  case RA_PDSCH:
    lte_ue_pdsch_vars = &phy_vars_ue->lte_ue_pdsch_vars_ra[eNB_id];
    dlsch_ue          = &phy_vars_ue->dlsch_ue_ra[eNB_id];
    dlsch0_harq       = dlsch_ue[0]->harq_processes[harq_pid];
    break;

  case PDSCH:
    lte_ue_pdsch_vars = &phy_vars_ue->lte_ue_pdsch_vars[eNB_id];
    dlsch_ue          = phy_vars_ue->dlsch_ue[eNB_id];
    dlsch0_harq       = dlsch_ue[0]->harq_processes[harq_pid];
    dlsch1_harq       = dlsch_ue[1]->harq_processes[harq_pid];
    break;

  default:
    LOG_E(PHY,"[UE %d][FATAL] Frame %d subframe %d: Unknown PDSCH format %d\n",phy_vars_ue->frame_rx,subframe,type);
    return(-1);
    break;
  }


  if (eNB_id > 2) {
    LOG_W(PHY,"dlsch_demodulation.c: Illegal eNB_id %d\n",eNB_id);
    return(-1);
  }

  if (!lte_ue_common_vars) {
    LOG_W(PHY,"dlsch_demodulation.c: Null lte_ue_common_vars\n");
    return(-1);
  }

  if (!dlsch_ue[0]) {
    LOG_W(PHY,"dlsch_demodulation.c: Null dlsch_ue pointer\n");
    return(-1);
  }

  if (!lte_ue_pdsch_vars) {
    LOG_W(PHY,"dlsch_demodulation.c: Null lte_ue_pdsch_vars pointer\n");
    return(-1);
  }

  if (!frame_parms) {
    LOG_W(PHY,"dlsch_demodulation.c: Null lte_frame_parms\n");
    return(-1);
  }

  
  if (((frame_parms->Ncp == NORMAL) && (symbol>=7)) ||
      ((frame_parms->Ncp == EXTENDED) && (symbol>=6)))
    rballoc = dlsch0_harq->rb_alloc_odd;
  else
    rballoc = dlsch0_harq->rb_alloc_even;

  if (frame_parms->nb_antennas_tx_eNB>1) {
#ifdef DEBUG_DLSCH_MOD
    LOG_I(PHY,"dlsch: using pmi %x (%p), rb_alloc %x\n",pmi2hex_2Ar1(dlsch0_harq->pmi_alloc),dlsch_ue[0],dlsch0_harq->rb_alloc_even[0]);
#endif
    nb_rb = dlsch_extract_rbs_dual(lte_ue_common_vars->rxdataF,
                                   lte_ue_common_vars->dl_ch_estimates[eNB_id],
                                   lte_ue_pdsch_vars[eNB_id]->rxdataF_ext,
                                   lte_ue_pdsch_vars[eNB_id]->dl_ch_estimates_ext,
                                   dlsch0_harq->pmi_alloc,
                                   lte_ue_pdsch_vars[eNB_id]->pmi_ext,
                                   rballoc,
                                   symbol,
                                   subframe,
                                   phy_vars_ue->high_speed_flag,
                                   frame_parms);

    if (dual_stream_flag==1) {
      if (eNB_id_i<phy_vars_ue->n_connected_eNB)
        nb_rb = dlsch_extract_rbs_dual(lte_ue_common_vars->rxdataF,
                                       lte_ue_common_vars->dl_ch_estimates[eNB_id_i],
                                       lte_ue_pdsch_vars[eNB_id_i]->rxdataF_ext,
                                       lte_ue_pdsch_vars[eNB_id_i]->dl_ch_estimates_ext,
                                       dlsch0_harq->pmi_alloc,
                                       lte_ue_pdsch_vars[eNB_id_i]->pmi_ext,
                                       rballoc,
                                       symbol,
                                       subframe,
                                       phy_vars_ue->high_speed_flag,
                                       frame_parms);
      else
        nb_rb = dlsch_extract_rbs_dual(lte_ue_common_vars->rxdataF,
                                       lte_ue_common_vars->dl_ch_estimates[eNB_id],
                                       lte_ue_pdsch_vars[eNB_id_i]->rxdataF_ext,
                                       lte_ue_pdsch_vars[eNB_id_i]->dl_ch_estimates_ext,
                                       dlsch0_harq->pmi_alloc,
                                       lte_ue_pdsch_vars[eNB_id_i]->pmi_ext,
                                       rballoc,
                                       symbol,
                                       subframe,
                                       phy_vars_ue->high_speed_flag,
                                       frame_parms);
    }
  } // if n_tx>1
  else {
    nb_rb = dlsch_extract_rbs_single(lte_ue_common_vars->rxdataF,
                                     lte_ue_common_vars->dl_ch_estimates[eNB_id],
                                     lte_ue_pdsch_vars[eNB_id]->rxdataF_ext,
                                     lte_ue_pdsch_vars[eNB_id]->dl_ch_estimates_ext,
                                     dlsch0_harq->pmi_alloc,
                                     lte_ue_pdsch_vars[eNB_id]->pmi_ext,
                                     rballoc,
                                     symbol,
                                     subframe,
                                     phy_vars_ue->high_speed_flag,
                                     frame_parms);

    if (dual_stream_flag==1) {
      if (eNB_id_i<phy_vars_ue->n_connected_eNB)
        nb_rb = dlsch_extract_rbs_single(lte_ue_common_vars->rxdataF,
                                         lte_ue_common_vars->dl_ch_estimates[eNB_id_i],
                                         lte_ue_pdsch_vars[eNB_id_i]->rxdataF_ext,
                                         lte_ue_pdsch_vars[eNB_id_i]->dl_ch_estimates_ext,
                                         dlsch0_harq->pmi_alloc,
                                         lte_ue_pdsch_vars[eNB_id_i]->pmi_ext,
                                         rballoc,
                                         symbol,
                                         subframe,
                                         phy_vars_ue->high_speed_flag,
                                         frame_parms);
      
        nb_rb = dlsch_extract_rbs_single(lte_ue_common_vars->rxdataF,
                                         lte_ue_common_vars->dl_ch_estimates[eNB_id],
                                         lte_ue_pdsch_vars[eNB_id_i]->rxdataF_ext,
                                         lte_ue_pdsch_vars[eNB_id_i]->dl_ch_estimates_ext,
                                         dlsch0_harq->pmi_alloc,
                                         lte_ue_pdsch_vars[eNB_id_i]->pmi_ext,
                                         rballoc,
                                         symbol,
                                         subframe,
					 phy_vars_ue->high_speed_flag,
                                         frame_parms);
    }
  } //else n_tx>1

  //  printf("nb_rb = %d, eNB_id %d\n",nb_rb,eNB_id);
  if (nb_rb==0) {
    LOG_D(PHY,"dlsch_demodulation.c: nb_rb=0\n");
    return(-1);
  }

  /*
  // DL power control: Scaling of Channel estimates for PDSCH
  dlsch_scale_channel(lte_ue_pdsch_vars[eNB_id]->dl_ch_estimates_ext,
  frame_parms,
  dlsch_ue,
  symbol,
  nb_rb);
  */
  if (first_symbol_flag==1) {
    dlsch_channel_level(lte_ue_pdsch_vars[eNB_id]->dl_ch_estimates_ext,
                        frame_parms,
                        avg,
                        symbol,
                        nb_rb);
#ifdef DEBUG_PHY
    LOG_D(PHY,"[DLSCH] avg[0] %d\n",avg[0]);
#endif

    // the channel gain should be the effective gain of precoding + channel
    // however lets be more conservative and set maxh = nb_tx*nb_rx*max(h_i)
    // in case of precoding we add an additional factor of two for the precoding gain
    avgs = 0;

    for (aatx=0; aatx<frame_parms->nb_antennas_tx_eNB; aatx++)
      for (aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++)
        avgs = cmax(avgs,avg[(aatx<<1)+aarx]);

    //  avgs = cmax(avgs,avg[(aarx<<1)+aatx]);


    lte_ue_pdsch_vars[eNB_id]->log2_maxh = (log2_approx(avgs)/2)+1;
    // + log2_approx(frame_parms->nb_antennas_tx_eNB-1) //-1 because log2_approx counts the number of bits
    //      + log2_approx(frame_parms->nb_antennas_rx-1);

    if ((dlsch0_harq->mimo_mode>=UNIFORM_PRECODING11) &&
        (dlsch0_harq->mimo_mode< DUALSTREAM_UNIFORM_PRECODING1) &&
        (dlsch0_harq->dl_power_off==1)) // we are in TM 6
      lte_ue_pdsch_vars[eNB_id]->log2_maxh++;

    // this version here applies the factor .5 also to the extra terms. however, it does not work so well as the one above
    /* K = Nb_rx         in TM1
       Nb_tx*Nb_rx   in TM2,4,5
       Nb_tx^2*Nb_rx in TM6 */
    /*
      K = frame_parms->nb_antennas_rx*frame_parms->nb_antennas_tx_eNB; //that also covers TM1 since Nb_tx=1
      if ((dlsch0_harq->mimo_mode>=UNIFORM_PRECODING11) &&
      (dlsch0_harq->mimo_mode< DUALSTREAM_UNIFORM_PRECODING1) &&
      (dlsch0_harq->dl_power_off==1)) // we are in TM 6
      K *= frame_parms->nb_antennas_tx_eNB;

      lte_ue_pdsch_vars[eNB_id]->log2_maxh = (log2_approx(K*avgs)/2);
    */

#ifdef DEBUG_PHY
    LOG_D(PHY,"[DLSCH] log2_maxh = %d (%d,%d)\n",lte_ue_pdsch_vars[eNB_id]->log2_maxh,avg[0],avgs);
    LOG_D(PHY,"[DLSCH] mimo_mode = %d\n", dlsch0_harq->mimo_mode);
#endif
  }

  aatx = frame_parms->nb_antennas_tx_eNB;
  aarx = frame_parms->nb_antennas_rx;

  if (dlsch0_harq->mimo_mode<LARGE_CDD) {// SISO or ALAMOUTI

    dlsch_channel_compensation(lte_ue_pdsch_vars[eNB_id]->rxdataF_ext,
                               lte_ue_pdsch_vars[eNB_id]->dl_ch_estimates_ext,
                               lte_ue_pdsch_vars[eNB_id]->dl_ch_mag0,
                               lte_ue_pdsch_vars[eNB_id]->dl_ch_magb0,
                               lte_ue_pdsch_vars[eNB_id]->rxdataF_comp0,
                               (aatx>1) ? lte_ue_pdsch_vars[eNB_id]->rho : NULL,
                               frame_parms,
                               symbol,
                               first_symbol_flag,
                               dlsch0_harq->Qm,
                               nb_rb,
                               lte_ue_pdsch_vars[eNB_id]->log2_maxh,
                               phy_measurements); // log2_maxh+I0_shift
#ifdef DEBUG_PHY

    if (symbol==5)
      write_output("rxF_comp_d.m","rxF_c_d",&lte_ue_pdsch_vars[eNB_id]->rxdataF_comp0[0][symbol*frame_parms->N_RB_DL*12],frame_parms->N_RB_DL*12,1,1);

#endif

    if ((dual_stream_flag==1) &&
        (eNB_id_i<phy_vars_ue->n_connected_eNB)) {
      // get MF output for interfering stream
      dlsch_channel_compensation(lte_ue_pdsch_vars[eNB_id_i]->rxdataF_ext,
                                 lte_ue_pdsch_vars[eNB_id_i]->dl_ch_estimates_ext,
                                 lte_ue_pdsch_vars[eNB_id_i]->dl_ch_mag0,
                                 lte_ue_pdsch_vars[eNB_id_i]->dl_ch_magb0,
                                 lte_ue_pdsch_vars[eNB_id_i]->rxdataF_comp0,
                                 (aatx>1) ? lte_ue_pdsch_vars[eNB_id_i]->rho : NULL,
                                 frame_parms,
                                 symbol,
                                 first_symbol_flag,
                                 i_mod,
                                 nb_rb,
                                 lte_ue_pdsch_vars[eNB_id]->log2_maxh,
                                 phy_measurements); // log2_maxh+I0_shift
#ifdef DEBUG_PHY

      if (symbol == 5) {
        write_output("rxF_comp_d.m","rxF_c_d",&lte_ue_pdsch_vars[eNB_id]->rxdataF_comp0[0][symbol*frame_parms->N_RB_DL*12],frame_parms->N_RB_DL*12,1,1);
        write_output("rxF_comp_i.m","rxF_c_i",&lte_ue_pdsch_vars[eNB_id_i]->rxdataF_comp0[0][symbol*frame_parms->N_RB_DL*12],frame_parms->N_RB_DL*12,1,1);
      }

#endif

      // compute correlation between signal and interference channels
      dlsch_dual_stream_correlation(frame_parms,
                                    symbol,
                                    nb_rb,
                                    lte_ue_pdsch_vars[eNB_id]->dl_ch_estimates_ext,
                                    lte_ue_pdsch_vars[eNB_id_i]->dl_ch_estimates_ext,
                                    lte_ue_pdsch_vars[eNB_id]->dl_ch_rho_ext,
                                    lte_ue_pdsch_vars[eNB_id]->log2_maxh);
    }
  } else if (dlsch0_harq->mimo_mode == LARGE_CDD) { // TM3
    //   LOG_I(PHY,"Running PDSCH RX for TM3\n");
    if (frame_parms->nb_antennas_tx_eNB == 2) {
      if (first_symbol_flag==1) {
        // effective channel of desired user is always stronger than interfering eff. channel
        dlsch_channel_level_TM3(lte_ue_pdsch_vars[eNB_id]->dl_ch_estimates_ext,
                                frame_parms,
                                avg, symbol, nb_rb);

        //  LOG_D(PHY,"llr_offset = %d\n",offset_mumimo_llr_drange[dlsch0_harq->mcs][(dlsch1_harq->mcs>>1)-1]);
        avg[0] = log2_approx(avg[0]) - 13 + offset_mumimo_llr_drange[dlsch0_harq->mcs][(dlsch1_harq->Qm>>1)-1];

        lte_ue_pdsch_vars[eNB_id]->log2_maxh = cmax(avg[0],0);
        //  printf("log2_maxh =%d\n",lte_ue_pdsch_vars[eNB_id]->log2_maxh);
      }

      dlsch_channel_compensation_TM3(frame_parms,
                                     lte_ue_pdsch_vars[eNB_id],
                                     phy_measurements,
                                     eNB_id,
                                     symbol,
                                     dlsch0_harq->Qm,
                                     dlsch1_harq->Qm,
                                     dlsch0_harq->round,
                                     nb_rb,
                                     lte_ue_pdsch_vars[eNB_id]->log2_maxh);
      // compute correlation between signal and interference channels (rho12 and rho21)
      dlsch_dual_stream_correlation(frame_parms,
                                    symbol,
                                    nb_rb,
                                    lte_ue_pdsch_vars[eNB_id]->dl_ch_estimates_ext,
                                    &(lte_ue_pdsch_vars[eNB_id]->dl_ch_estimates_ext[2]),
                                    lte_ue_pdsch_vars[eNB_id]->dl_ch_rho_ext,
                                    lte_ue_pdsch_vars[eNB_id]->log2_maxh);
      dlsch_dual_stream_correlation(frame_parms,
                                    symbol,
                                    nb_rb,
                                    &(lte_ue_pdsch_vars[eNB_id]->dl_ch_estimates_ext[2]),
                                    lte_ue_pdsch_vars[eNB_id]->dl_ch_estimates_ext,
                                    lte_ue_pdsch_vars[eNB_id]->dl_ch_rho2_ext,
                                    lte_ue_pdsch_vars[eNB_id]->log2_maxh);
      //printf("TM3 log2_maxh : %d\n",lte_ue_pdsch_vars[eNB_id]->log2_maxh);

    } else {
      LOG_E(PHY, "only 2 tx antennas supported for TM3\n");
    }
  } else if (dlsch0_harq->mimo_mode<DUALSTREAM_UNIFORM_PRECODING1) { // single-layer precoding (TM5, TM6), potentially TM4 (Single-codeword)
    //    printf("Channel compensation for precoding\n");
    //    if ((dual_stream_flag==1) && (eNB_id_i==NUMBER_OF_CONNECTED_eNB_MAX)) {
    if ((dual_stream_flag==1) && (eNB_id_i==phy_vars_ue->n_connected_eNB)) {  // TM5 two-user

      // Scale the channel estimates for interfering stream

      dlsch_scale_channel(lte_ue_pdsch_vars[eNB_id_i]->dl_ch_estimates_ext,
                          frame_parms,
                          dlsch_ue,
                          symbol,
                          nb_rb);

      /* compute new log2_maxh for effective channel */
      if (first_symbol_flag==1) {
        // effective channel of desired user is always stronger than interfering eff. channel
        dlsch_channel_level_TM56(lte_ue_pdsch_vars[eNB_id]->dl_ch_estimates_ext, frame_parms, lte_ue_pdsch_vars[eNB_id]->pmi_ext, avg, symbol, nb_rb);

        //    LOG_D(PHY,"llr_offset = %d\n",offset_mumimo_llr_drange[dlsch0_harq->mcs][(i_mod>>1)-1]);
        avg[0] = log2_approx(avg[0]) - 13 + offset_mumimo_llr_drange[dlsch0_harq->mcs][(i_mod>>1)-1];

        lte_ue_pdsch_vars[eNB_id]->log2_maxh = cmax(avg[0],0);
        //printf("log1_maxh =%d\n",lte_ue_pdsch_vars[eNB_id]->log2_maxh);
      }

      dlsch_channel_compensation_TM56(lte_ue_pdsch_vars[eNB_id]->rxdataF_ext,
                                      lte_ue_pdsch_vars[eNB_id]->dl_ch_estimates_ext,
                                      lte_ue_pdsch_vars[eNB_id]->dl_ch_mag0,
                                      lte_ue_pdsch_vars[eNB_id]->dl_ch_magb0,
                                      lte_ue_pdsch_vars[eNB_id]->rxdataF_comp0,
                                      lte_ue_pdsch_vars[eNB_id]->pmi_ext,
                                      frame_parms,
                                      phy_measurements,
                                      eNB_id,
                                      symbol,
                                      dlsch0_harq->Qm,
                                      nb_rb,
                                      lte_ue_pdsch_vars[eNB_id]->log2_maxh,
                                      dlsch0_harq->dl_power_off);

      // if interference source is MU interference, assume opposite precoder was used at eNB

      // calculate opposite PMI
      for (rb=0; rb<nb_rb; rb++) {
        switch(lte_ue_pdsch_vars[eNB_id]->pmi_ext[rb]) {
        case 0:
          lte_ue_pdsch_vars[eNB_id_i]->pmi_ext[rb]=1;
          break;

        case 1:
          lte_ue_pdsch_vars[eNB_id_i]->pmi_ext[rb]=0;
          break;

        case 2:
          lte_ue_pdsch_vars[eNB_id_i]->pmi_ext[rb]=3;
          break;

        case 3:
          lte_ue_pdsch_vars[eNB_id_i]->pmi_ext[rb]=2;
          break;
        }

        //  if (rb==0)
        //    printf("pmi %d, pmi_i %d\n",lte_ue_pdsch_vars[eNB_id]->pmi_ext[rb],lte_ue_pdsch_vars[eNB_id_i]->pmi_ext[rb]);

      }

      dlsch_channel_compensation_TM56(lte_ue_pdsch_vars[eNB_id_i]->rxdataF_ext,
                                      lte_ue_pdsch_vars[eNB_id_i]->dl_ch_estimates_ext,
                                      lte_ue_pdsch_vars[eNB_id_i]->dl_ch_mag0,
                                      lte_ue_pdsch_vars[eNB_id_i]->dl_ch_magb0,
                                      lte_ue_pdsch_vars[eNB_id_i]->rxdataF_comp0,
                                      lte_ue_pdsch_vars[eNB_id_i]->pmi_ext,
                                      frame_parms,
                                      phy_measurements,
                                      eNB_id_i,
                                      symbol,
                                      i_mod,
                                      nb_rb,
                                      lte_ue_pdsch_vars[eNB_id]->log2_maxh,
                                      dlsch0_harq->dl_power_off);

#ifdef DEBUG_PHY

      if (symbol==5) {
        write_output("rxF_comp_d.m","rxF_c_d",&lte_ue_pdsch_vars[eNB_id]->rxdataF_comp0[0][symbol*frame_parms->N_RB_DL*12],frame_parms->N_RB_DL*12,1,1);
        write_output("rxF_comp_i.m","rxF_c_i",&lte_ue_pdsch_vars[eNB_id_i]->rxdataF_comp0[0][symbol*frame_parms->N_RB_DL*12],frame_parms->N_RB_DL*12,1,1);
      }

#endif

      dlsch_dual_stream_correlation(frame_parms,
                                    symbol,
                                    nb_rb,
                                    lte_ue_pdsch_vars[eNB_id]->dl_ch_estimates_ext,
                                    lte_ue_pdsch_vars[eNB_id_i]->dl_ch_estimates_ext,
                                    lte_ue_pdsch_vars[eNB_id]->dl_ch_rho_ext,
                                    lte_ue_pdsch_vars[eNB_id]->log2_maxh);

    } else {
      dlsch_channel_compensation_TM56(lte_ue_pdsch_vars[eNB_id]->rxdataF_ext,
                                      lte_ue_pdsch_vars[eNB_id]->dl_ch_estimates_ext,
                                      lte_ue_pdsch_vars[eNB_id]->dl_ch_mag0,
                                      lte_ue_pdsch_vars[eNB_id]->dl_ch_magb0,
                                      lte_ue_pdsch_vars[eNB_id]->rxdataF_comp0,
                                      lte_ue_pdsch_vars[eNB_id]->pmi_ext,
                                      frame_parms,
                                      phy_measurements,
                                      eNB_id,
                                      symbol,
                                      dlsch0_harq->Qm,
                                      nb_rb,
                                      lte_ue_pdsch_vars[eNB_id]->log2_maxh,
                                      1);
    }
  }

  //  printf("MRC\n");
  if (frame_parms->nb_antennas_rx > 1) {
    if (dlsch0_harq->mimo_mode == LARGE_CDD) {
      if (frame_parms->nb_antennas_tx_eNB == 2) {
        dlsch_detection_mrc(frame_parms,
                            lte_ue_pdsch_vars[eNB_id]->rxdataF_comp0,
                            lte_ue_pdsch_vars[eNB_id]->rxdataF_comp1[dlsch0_harq->round],
                            lte_ue_pdsch_vars[eNB_id]->dl_ch_rho_ext,
                            lte_ue_pdsch_vars[eNB_id]->rho,
                            lte_ue_pdsch_vars[eNB_id]->dl_ch_mag0,
                            lte_ue_pdsch_vars[eNB_id]->dl_ch_magb0,
                            lte_ue_pdsch_vars[eNB_id]->dl_ch_mag1,
                            lte_ue_pdsch_vars[eNB_id]->dl_ch_magb1,
                            symbol,
                            nb_rb,
                            dual_stream_flag);
      }
    } else {

      dlsch_detection_mrc(frame_parms,
                          lte_ue_pdsch_vars[eNB_id]->rxdataF_comp0,
                          lte_ue_pdsch_vars[eNB_id_i]->rxdataF_comp0,
                          lte_ue_pdsch_vars[eNB_id]->rho,
                          lte_ue_pdsch_vars[eNB_id]->dl_ch_rho_ext,
                          lte_ue_pdsch_vars[eNB_id]->dl_ch_mag0,
                          lte_ue_pdsch_vars[eNB_id]->dl_ch_magb0,
                          lte_ue_pdsch_vars[eNB_id_i]->dl_ch_mag0,
                          lte_ue_pdsch_vars[eNB_id_i]->dl_ch_magb0,
                          symbol,
                          nb_rb,
                          dual_stream_flag);
    }
  }

  //  printf("Combining");
  if ((dlsch0_harq->mimo_mode == SISO) ||
      ((dlsch0_harq->mimo_mode >= UNIFORM_PRECODING11) &&
       (dlsch0_harq->mimo_mode <= PUSCH_PRECODING0))) {

    /*
      dlsch_siso(frame_parms,
      lte_ue_pdsch_vars[eNB_id]->rxdataF_comp,
      lte_ue_pdsch_vars[eNB_id_i]->rxdataF_comp,
      symbol,
      nb_rb);
    */
  } else if (dlsch0_harq->mimo_mode == ALAMOUTI) {

    dlsch_alamouti(frame_parms,
                   lte_ue_pdsch_vars[eNB_id]->rxdataF_comp0,
                   lte_ue_pdsch_vars[eNB_id]->dl_ch_mag0,
                   lte_ue_pdsch_vars[eNB_id]->dl_ch_magb0,
                   symbol,
                   nb_rb);

  }

  else if (dlsch0_harq->mimo_mode == LARGE_CDD) {

  } else {
    LOG_W(PHY,"dlsch_rx: Unknown MIMO mode\n");
    return (-1);
  }

  //    printf("LLR");

  switch (dlsch0_harq->Qm) {
  case 2 :
    if (dlsch0_harq->mimo_mode != LARGE_CDD) {
      if (dual_stream_flag == 0)
        dlsch_qpsk_llr(frame_parms,
                       lte_ue_pdsch_vars[eNB_id]->rxdataF_comp0,
                       lte_ue_pdsch_vars[eNB_id]->llr[0],
                       symbol,first_symbol_flag,nb_rb,
                       adjust_G2(frame_parms,dlsch0_harq->rb_alloc_even,2,subframe,symbol),
                       lte_ue_pdsch_vars[eNB_id]->llr128);
      else if (i_mod == 2) {
        dlsch_qpsk_qpsk_llr(frame_parms,
                            lte_ue_pdsch_vars[eNB_id]->rxdataF_comp0,
                            lte_ue_pdsch_vars[eNB_id_i]->rxdataF_comp0,
                            lte_ue_pdsch_vars[eNB_id]->dl_ch_rho_ext,
                            lte_ue_pdsch_vars[eNB_id]->llr[0],
                            symbol,first_symbol_flag,nb_rb,
                            adjust_G2(frame_parms,dlsch0_harq->rb_alloc_even,2,subframe,symbol),
                            lte_ue_pdsch_vars[eNB_id]->llr128);
      } else if (i_mod == 4) {
        dlsch_qpsk_16qam_llr(frame_parms,
                             lte_ue_pdsch_vars[eNB_id]->rxdataF_comp0,
                             lte_ue_pdsch_vars[eNB_id_i]->rxdataF_comp0,
                             lte_ue_pdsch_vars[eNB_id_i]->dl_ch_mag0,
                             lte_ue_pdsch_vars[eNB_id]->dl_ch_rho_ext,
                             lte_ue_pdsch_vars[eNB_id]->llr[0],
                             symbol,first_symbol_flag,nb_rb,
                             adjust_G2(frame_parms,dlsch0_harq->rb_alloc_even,2,subframe,symbol),
                             lte_ue_pdsch_vars[eNB_id]->llr128);
      } else {
        dlsch_qpsk_64qam_llr(frame_parms,
                             lte_ue_pdsch_vars[eNB_id]->rxdataF_comp0,
                             lte_ue_pdsch_vars[eNB_id_i]->rxdataF_comp0,
                             lte_ue_pdsch_vars[eNB_id_i]->dl_ch_mag0,
                             lte_ue_pdsch_vars[eNB_id]->dl_ch_rho_ext,
                             lte_ue_pdsch_vars[eNB_id]->llr[0],
                             symbol,first_symbol_flag,nb_rb,
                             adjust_G2(frame_parms,dlsch0_harq->rb_alloc_even,2,subframe,symbol),
                             lte_ue_pdsch_vars[eNB_id]->llr128);

      }
    } else { // TM3
      DevAssert(dlsch1_harq);

      if (dlsch1_harq->Qm == 2) {
        /*  dlsch_qpsk_llr(frame_parms,
	    lte_ue_pdsch_vars[eNB_id]->rxdataF_comp0,
	    lte_ue_pdsch_vars[eNB_id]->llr[0],
	    symbol,first_symbol_flag,nb_rb,
	    adjust_G2(frame_parms,dlsch0_harq->rb_alloc,2,subframe,symbol),
	    lte_ue_pdsch_vars[eNB_id]->llr128);
        */
        dlsch_qpsk_qpsk_llr(frame_parms,
                            lte_ue_pdsch_vars[eNB_id]->rxdataF_comp0,
                            lte_ue_pdsch_vars[eNB_id]->rxdataF_comp1[dlsch0_harq->round],
                            lte_ue_pdsch_vars[eNB_id]->dl_ch_rho_ext,
                            lte_ue_pdsch_vars[eNB_id]->llr[0],
                            symbol,first_symbol_flag,nb_rb,
                            adjust_G2(frame_parms,dlsch0_harq->rb_alloc_even,2,subframe,symbol),
                            lte_ue_pdsch_vars[eNB_id]->llr128);
        dlsch_qpsk_qpsk_llr(frame_parms,
                            lte_ue_pdsch_vars[eNB_id]->rxdataF_comp1[dlsch0_harq->round],
                            lte_ue_pdsch_vars[eNB_id]->rxdataF_comp0,
                            lte_ue_pdsch_vars[eNB_id]->dl_ch_rho2_ext,
                            lte_ue_pdsch_vars[eNB_id]->llr[1],
                            symbol,first_symbol_flag,nb_rb,
                            adjust_G2(frame_parms,dlsch1_harq->rb_alloc_even,2,subframe,symbol),
                            lte_ue_pdsch_vars[eNB_id]->llr128_2ndstream);
      } else if (dlsch1_harq->Qm == 4) {
        dlsch_qpsk_16qam_llr(frame_parms,
                             lte_ue_pdsch_vars[eNB_id]->rxdataF_comp0,
                             lte_ue_pdsch_vars[eNB_id]->rxdataF_comp1[dlsch0_harq->round],
                             lte_ue_pdsch_vars[eNB_id]->dl_ch_mag1,
                             lte_ue_pdsch_vars[eNB_id]->dl_ch_rho_ext,
                             lte_ue_pdsch_vars[eNB_id]->llr[0],
                             symbol,first_symbol_flag,nb_rb,
                             adjust_G2(frame_parms,dlsch0_harq->rb_alloc_even,2,subframe,symbol),
                             lte_ue_pdsch_vars[eNB_id]->llr128);
      } else {
        dlsch_qpsk_64qam_llr(frame_parms,
                             lte_ue_pdsch_vars[eNB_id]->rxdataF_comp0,
                             lte_ue_pdsch_vars[eNB_id]->rxdataF_comp1[dlsch0_harq->round],
                             lte_ue_pdsch_vars[eNB_id]->dl_ch_mag1,
                             lte_ue_pdsch_vars[eNB_id]->dl_ch_rho_ext,
                             lte_ue_pdsch_vars[eNB_id]->llr[0],
                             symbol,first_symbol_flag,nb_rb,
                             adjust_G2(frame_parms,dlsch0_harq->rb_alloc_even,2,subframe,symbol),
                             lte_ue_pdsch_vars[eNB_id]->llr128);

      }
    }

    break;

  case 4 :
    if (dual_stream_flag == 0) {
      dlsch_16qam_llr(frame_parms,
                      lte_ue_pdsch_vars[eNB_id]->rxdataF_comp0,
                      lte_ue_pdsch_vars[eNB_id]->llr[0],
                      lte_ue_pdsch_vars[eNB_id]->dl_ch_mag0,
                      symbol,first_symbol_flag,nb_rb,
                      adjust_G2(frame_parms,dlsch0_harq->rb_alloc_even,4,subframe,symbol),
                      lte_ue_pdsch_vars[eNB_id]->llr128);
    } else if (i_mod == 2) {
      dlsch_16qam_qpsk_llr(frame_parms,
                           lte_ue_pdsch_vars[eNB_id]->rxdataF_comp0,
                           lte_ue_pdsch_vars[eNB_id_i]->rxdataF_comp0,
                           lte_ue_pdsch_vars[eNB_id]->dl_ch_mag0,
                           lte_ue_pdsch_vars[eNB_id]->dl_ch_rho_ext,
                           lte_ue_pdsch_vars[eNB_id]->llr[0],
                           symbol,first_symbol_flag,nb_rb,
                           adjust_G2(frame_parms,dlsch0_harq->rb_alloc_even,2,subframe,symbol),
                           lte_ue_pdsch_vars[eNB_id]->llr128);
    } else if (i_mod == 4) {
      dlsch_16qam_16qam_llr(frame_parms,
                            lte_ue_pdsch_vars[eNB_id]->rxdataF_comp0,
                            lte_ue_pdsch_vars[eNB_id_i]->rxdataF_comp0,
                            lte_ue_pdsch_vars[eNB_id]->dl_ch_mag0,
                            lte_ue_pdsch_vars[eNB_id_i]->dl_ch_mag0,
                            lte_ue_pdsch_vars[eNB_id]->dl_ch_rho_ext,
                            lte_ue_pdsch_vars[eNB_id]->llr[0],
                            symbol,first_symbol_flag,nb_rb,
                            adjust_G2(frame_parms,dlsch0_harq->rb_alloc_even,2,subframe,symbol),
                            lte_ue_pdsch_vars[eNB_id]->llr128);
    } else {
      dlsch_16qam_64qam_llr(frame_parms,
                            lte_ue_pdsch_vars[eNB_id]->rxdataF_comp0,
                            lte_ue_pdsch_vars[eNB_id_i]->rxdataF_comp0,
                            lte_ue_pdsch_vars[eNB_id]->dl_ch_mag0,
                            lte_ue_pdsch_vars[eNB_id_i]->dl_ch_mag0,
                            lte_ue_pdsch_vars[eNB_id]->dl_ch_rho_ext,
                            lte_ue_pdsch_vars[eNB_id]->llr[0],
                            symbol,first_symbol_flag,nb_rb,
                            adjust_G2(frame_parms,dlsch0_harq->rb_alloc_even,2,subframe,symbol),
                            lte_ue_pdsch_vars[eNB_id]->llr128);
    }

    break;

  case 6 :
    if (dual_stream_flag == 0) {
      dlsch_64qam_llr(frame_parms,
                      lte_ue_pdsch_vars[eNB_id]->rxdataF_comp0,
                      lte_ue_pdsch_vars[eNB_id]->llr[0],
                      lte_ue_pdsch_vars[eNB_id]->dl_ch_mag0,
                      lte_ue_pdsch_vars[eNB_id]->dl_ch_magb0,
                      symbol,first_symbol_flag,nb_rb,
                      adjust_G2(frame_parms,dlsch0_harq->rb_alloc_even,6,subframe,symbol),
                      lte_ue_pdsch_vars[eNB_id]->llr128);
    } else if (i_mod == 2) {
      dlsch_64qam_qpsk_llr(frame_parms,
                           lte_ue_pdsch_vars[eNB_id]->rxdataF_comp0,
                           lte_ue_pdsch_vars[eNB_id_i]->rxdataF_comp0,
                           lte_ue_pdsch_vars[eNB_id]->dl_ch_mag0,
                           lte_ue_pdsch_vars[eNB_id]->dl_ch_rho_ext,
                           lte_ue_pdsch_vars[eNB_id]->llr[0],
                           symbol,first_symbol_flag,nb_rb,
                           adjust_G2(frame_parms,dlsch0_harq->rb_alloc_even,2,subframe,symbol),
                           lte_ue_pdsch_vars[eNB_id]->llr128);
    } else if (i_mod == 4) {
      dlsch_64qam_16qam_llr(frame_parms,
                            lte_ue_pdsch_vars[eNB_id]->rxdataF_comp0,
                            lte_ue_pdsch_vars[eNB_id_i]->rxdataF_comp0,
                            lte_ue_pdsch_vars[eNB_id]->dl_ch_mag0,
                            lte_ue_pdsch_vars[eNB_id_i]->dl_ch_mag0,
                            lte_ue_pdsch_vars[eNB_id]->dl_ch_rho_ext,
                            lte_ue_pdsch_vars[eNB_id]->llr[0],
                            symbol,first_symbol_flag,nb_rb,
                            adjust_G2(frame_parms,dlsch0_harq->rb_alloc_even,2,subframe,symbol),
                            lte_ue_pdsch_vars[eNB_id]->llr128);

    } else {
      dlsch_64qam_64qam_llr(frame_parms,
                            lte_ue_pdsch_vars[eNB_id]->rxdataF_comp0,
                            lte_ue_pdsch_vars[eNB_id_i]->rxdataF_comp0,
                            lte_ue_pdsch_vars[eNB_id]->dl_ch_mag0,
                            lte_ue_pdsch_vars[eNB_id_i]->dl_ch_mag0,
                            lte_ue_pdsch_vars[eNB_id]->dl_ch_rho_ext,
                            lte_ue_pdsch_vars[eNB_id]->llr[0],
                            symbol,first_symbol_flag,nb_rb,
                            adjust_G2(frame_parms,dlsch0_harq->rb_alloc_even,2,subframe,symbol),
                            lte_ue_pdsch_vars[eNB_id]->llr128);
    }

    break;

  default:
    LOG_W(PHY,"rx_dlsch.c : Unknown mod_order!!!!\n");
    return(-1);
    break;
  }

  return(0);
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
                                PHY_MEASUREMENTS *phy_measurements)
{

#if defined(__i386) || defined(__x86_64)

  unsigned short rb;
  unsigned char aatx,aarx,symbol_mod,pilots=0;
  __m128i *dl_ch128,*dl_ch128_2,*dl_ch_mag128,*dl_ch_mag128b,*rxdataF128,*rxdataF_comp128,*rho128;
  __m128i mmtmpD0,mmtmpD1,mmtmpD2,mmtmpD3,QAM_amp128,QAM_amp128b;

  symbol_mod = (symbol>=(7-frame_parms->Ncp)) ? symbol-(7-frame_parms->Ncp) : symbol;

  if ((symbol_mod == 0) || (symbol_mod == (4-frame_parms->Ncp))) {

    if (frame_parms->mode1_flag==1) // 10 out of 12 so don't reduce size
      nb_rb=1+(5*nb_rb/6);
    else
      pilots=1;
  }

  for (aatx=0; aatx<frame_parms->nb_antennas_tx_eNB; aatx++) {
    if (mod_order == 4) {
      QAM_amp128 = _mm_set1_epi16(QAM16_n1);  // 2/sqrt(10)
      QAM_amp128b = _mm_setzero_si128();
    } else if (mod_order == 6) {
      QAM_amp128  = _mm_set1_epi16(QAM64_n1); //
      QAM_amp128b = _mm_set1_epi16(QAM64_n2);
    }

    //    printf("comp: rxdataF_comp %p, symbol %d\n",rxdataF_comp[0],symbol);

    for (aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++) {

      dl_ch128          = (__m128i *)&dl_ch_estimates_ext[(aatx<<1)+aarx][symbol*frame_parms->N_RB_DL*12];
      dl_ch_mag128      = (__m128i *)&dl_ch_mag[(aatx<<1)+aarx][symbol*frame_parms->N_RB_DL*12];
      dl_ch_mag128b     = (__m128i *)&dl_ch_magb[(aatx<<1)+aarx][symbol*frame_parms->N_RB_DL*12];
      rxdataF128        = (__m128i *)&rxdataF_ext[aarx][symbol*frame_parms->N_RB_DL*12];
      rxdataF_comp128   = (__m128i *)&rxdataF_comp[(aatx<<1)+aarx][symbol*frame_parms->N_RB_DL*12];


      for (rb=0; rb<nb_rb; rb++) {
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
        phy_measurements->rx_correlation[0][aarx] = signal_energy(&rho[aarx][symbol*frame_parms->N_RB_DL*12],rb*12);
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
    if (frame_parms->mode1_flag==1) { // 10 out of 12 so don't reduce size
      nb_rb=1+(5*nb_rb/6);
    }
    else {
      pilots=1;
    }
  }

  for (aatx=0; aatx<frame_parms->nb_antennas_tx_eNB; aatx++) {
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
	phy_measurements->rx_correlation[0][aarx] = signal_energy(&rho[aarx][symbol*frame_parms->N_RB_DL*12],rb*12);
      }
    }
  }
#endif
}



#if defined(__x86_64__) || defined(__i386__)

void prec2A_TM56_128(unsigned char pmi,__m128i *ch0,__m128i *ch1)
{

  __m128i amp;
  amp = _mm_set1_epi16(ONE_OVER_SQRT2_Q15);

  switch (pmi) {

  case 0 :   // +1 +1
    //    print_shorts("phase 0 :ch0",ch0);
    //    print_shorts("phase 0 :ch1",ch1);
    ch0[0] = _mm_adds_epi16(ch0[0],ch1[0]);
    break;

  case 1 :   // +1 -1
    //    print_shorts("phase 1 :ch0",ch0);
    //    print_shorts("phase 1 :ch1",ch1);
    ch0[0] = _mm_subs_epi16(ch0[0],ch1[0]);
    //    print_shorts("phase 1 :ch0-ch1",ch0);
    break;

  case 2 :   // +1 +j
    ch1[0] = _mm_sign_epi16(ch1[0],*(__m128i*)&conjugate[0]);
    ch1[0] = _mm_shufflelo_epi16(ch1[0],_MM_SHUFFLE(2,3,0,1));
    ch1[0] = _mm_shufflehi_epi16(ch1[0],_MM_SHUFFLE(2,3,0,1));
    ch0[0] = _mm_subs_epi16(ch0[0],ch1[0]);

    break;   // +1 -j

  case 3 :
    ch1[0] = _mm_sign_epi16(ch1[0],*(__m128i*)&conjugate[0]);
    ch1[0] = _mm_shufflelo_epi16(ch1[0],_MM_SHUFFLE(2,3,0,1));
    ch1[0] = _mm_shufflehi_epi16(ch1[0],_MM_SHUFFLE(2,3,0,1));
    ch0[0] = _mm_adds_epi16(ch0[0],ch1[0]);
    break;
  }

  ch0[0] = _mm_mulhi_epi16(ch0[0],amp);
  ch0[0] = _mm_slli_epi16(ch0[0],1);

  _mm_empty();
  _m_empty();
}

#elif defined(__arm__)
void prec2A_TM56_128(unsigned char pmi,int16x8_t* ch0,int16x8_t* ch1) {

  int16x8_t amp;
  int16_t conj[8]__attribute__((aligned(16))) = {1,-1,1,-1,1,-1,1,-1};

  amp = vmovq_n_s16(ONE_OVER_SQRT2_Q15);

  switch (pmi) {
  case 0 :   // +1 +1
    //    print_shorts("phase 0 :ch0",ch0);
    //    print_shorts("phase 0 :ch1",ch1);
    ch0[0] = vqaddq_s16(ch0[0],ch1[0]);
    break;
      
  case 1 :   // +1 -1
    //    print_shorts("phase 1 :ch0",ch0);
    //    print_shorts("phase 1 :ch1",ch1);
    ch0[0] = vqsubq_s16(ch0[0],ch1[0]);
    //    print_shorts("phase 1 :ch0-ch1",ch0);
    break;
      
  case 2 :   // +1 +j
    ch1[0] = vrev32q_s16(vmulq_s16(ch1[0],*(int16x8_t*)conj));
    ch0[0] = vqsubq_s16(ch0[0],ch1[0]);
    break;   // +1 -j
      
  case 3 :
    ch1[0] = vrev32q_s16(vmulq_s16(ch1[0],*(int16x8_t*)conj));
    ch0[0] = vqaddq_s16(ch0[0],ch1[0]);
    break;
  }
    
  ch0[0] = vqdmulhq_s16(ch0[0],amp);
}

#endif

// precoding is stream 0 .5(1,1)  .5(1,-1) .5(1,1)  .5(1,-1)
//              stream 1 .5(1,-1) .5(1,1)  .5(1,-1) .5(1,1)
// store "precoded" channel for stream 0 in ch0, stream 1 in ch1

short TM3_prec[8]__attribute__((aligned(16))) = {1,1,-1,-1,1,1,-1,-1} ;

#if defined(__x86_64__) || defined(__i386__)

static inline void prec2A_TM3_128(__m128i *ch0,__m128i *ch1) __attribute__((always_inline));
static inline void prec2A_TM3_128(__m128i *ch0,__m128i *ch1)
{

  //  __m128i amp = _mm_set1_epi16(ONE_OVER_SQRT2_Q15);

  __m128i tmp0,tmp1;


  //  print_shorts("prec2A_TM3 ch0 (before):",ch0);
  //  print_shorts("prec2A_TM3 ch1 (before):",ch1);

  tmp0 = ch0[0];
  tmp1  = _mm_sign_epi16(ch1[0],((__m128i*)&TM3_prec)[0]);
  //  print_shorts("prec2A_TM3 ch1*s (mid):",(__m128i*)TM3_prec);

  ch0[0] = _mm_adds_epi16(ch0[0],tmp1);
  ch1[0] = _mm_subs_epi16(tmp0,tmp1);


  //  print_shorts("prec2A_TM3 ch0 (mid):",&tmp0);
  //  print_shorts("prec2A_TM3 ch1 (mid):",ch1);


  ch0[0] = _mm_srai_epi16(ch0[0],1);
  ch1[0] = _mm_srai_epi16(ch1[0],1);

  //  print_shorts("prec2A_TM3 ch0 (after):",ch0);
  //  print_shorts("prec2A_TM3 ch1 (after):",ch1);

}

#elif defined(__arm__)

void prec2A_TM3_128(int16x8_t* ch0, int16x8_t* ch1)
{

  int16x8_t tmp0,tmp1;

  tmp0 = ch0[0];
  tmp1   = vmulq_s16(ch1[0],((int16x8_t*)&TM3_prec)[0]);
  ch0[0] = vhaddq_s16(ch0[0],tmp1);
  ch1[0] = vhsubq_s16(tmp0,tmp1);
}

#endif

// pmi = 0 => stream 0 (1,1), stream 1 (1,-1)
// pmi = 1 => stream 0 (1,j), stream 2 (1,-j)

#if defined(__x86_64__) || defined(__i386__)

void prec2A_TM4_128(int pmi,__m128i *ch0,__m128i *ch1)
{

  __m128i amp;
  amp = _mm_set1_epi16(ONE_OVER_SQRT2_Q15);
  __m128i tmp0,tmp1;

  if (pmi == 0) {
    ch0[0] = _mm_adds_epi16(ch0[0],ch1[0]);
    ch1[0] = _mm_subs_epi16(ch0[0],ch1[0]);
  } else {
    tmp0 = ch0[0];
    tmp1   = _mm_sign_epi16(ch1[0],*(__m128i*)&conjugate[0]);
    tmp1   = _mm_shufflelo_epi16(tmp1,_MM_SHUFFLE(2,3,0,1));
    tmp1   = _mm_shufflehi_epi16(tmp1,_MM_SHUFFLE(2,3,0,1));
    ch0[0] = _mm_adds_epi16(tmp0,tmp1);
    ch1[0] = _mm_subs_epi16(tmp0,tmp1);
  }

  ch0[0] = _mm_mulhi_epi16(ch0[0],amp);
  ch0[0] = _mm_slli_epi16(ch0[0],1);
  ch1[0] = _mm_mulhi_epi16(ch1[0],amp);
  ch1[0] = _mm_slli_epi16(ch1[0],1);

}

#elif defined(__arm__)

void prec2A_TM4_128(int pmi,int16x8_t *ch0,int16x8_t *ch1)
{
  int16x8_t amp;
  int16x8_t tmp0,tmp1;
  int16_t conj[8]__attribute__((aligned(16))) = {1,-1,1,-1,1,-1,1,-1};

  amp = vmovq_n_s16(ONE_OVER_SQRT2_Q15);

  if (pmi == 0) {
    ch0[0] = vqaddq_s16(ch0[0],ch1[0]);
    ch1[0] = vqsubq_s16(ch0[0],ch1[0]);
  } else {
    tmp0 = ch0[0];
    tmp1 = vrev32q_s16(vmulq_s16(ch1[0],*(int16x8_t*)conj));    
    ch0[0] = vqaddq_s16(tmp0,tmp1);
    ch1[0] = vqsubq_s16(tmp0,tmp1);
  }

  ch0[0] = vqdmulhq_s16(ch0[0],amp);
  ch1[0] = vqdmulhq_s16(ch1[0],amp);
}
#endif

void dlsch_channel_compensation_TM56(int **rxdataF_ext,
                                     int **dl_ch_estimates_ext,
                                     int **dl_ch_mag,
                                     int **dl_ch_magb,
                                     int **rxdataF_comp,
                                     unsigned char *pmi_ext,
                                     LTE_DL_FRAME_PARMS *frame_parms,
                                     PHY_MEASUREMENTS *phy_measurements,
                                     int eNB_id,
                                     unsigned char symbol,
                                     unsigned char mod_order,
                                     unsigned short nb_rb,
                                     unsigned char output_shift,
                                     unsigned char dl_power_off)
{

#if defined(__x86_64__) || defined(__i386__)

  unsigned short rb,Nre;
  __m128i *dl_ch0_128,*dl_ch1_128,*dl_ch_mag128,*dl_ch_mag128b,*rxdataF128,*rxdataF_comp128;
  unsigned char aarx=0,symbol_mod,pilots=0;
  int precoded_signal_strength=0;
  __m128i mmtmpD0,mmtmpD1,mmtmpD2,mmtmpD3,QAM_amp128,QAM_amp128b;

  symbol_mod = (symbol>=(7-frame_parms->Ncp)) ? symbol-(7-frame_parms->Ncp) : symbol;

  if ((symbol_mod == 0) || (symbol_mod == (4-frame_parms->Ncp)))
    pilots=1;


  //printf("comp prec: symbol %d, pilots %d\n",symbol, pilots);

  if (mod_order == 4) {
    QAM_amp128 = _mm_set1_epi16(QAM16_n1);
    QAM_amp128b = _mm_setzero_si128();
  } else if (mod_order == 6) {
    QAM_amp128  = _mm_set1_epi16(QAM64_n1);
    QAM_amp128b = _mm_set1_epi16(QAM64_n2);
  }

  for (aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++) {

    dl_ch0_128          = (__m128i *)&dl_ch_estimates_ext[aarx][symbol*frame_parms->N_RB_DL*12];
    dl_ch1_128          = (__m128i *)&dl_ch_estimates_ext[2+aarx][symbol*frame_parms->N_RB_DL*12];


    dl_ch_mag128      = (__m128i *)&dl_ch_mag[aarx][symbol*frame_parms->N_RB_DL*12];
    dl_ch_mag128b     = (__m128i *)&dl_ch_magb[aarx][symbol*frame_parms->N_RB_DL*12];
    rxdataF128        = (__m128i *)&rxdataF_ext[aarx][symbol*frame_parms->N_RB_DL*12];
    rxdataF_comp128   = (__m128i *)&rxdataF_comp[aarx][symbol*frame_parms->N_RB_DL*12];


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

        mmtmpD0 = _mm_madd_epi16(dl_ch0_128[0],dl_ch0_128[0]);
        mmtmpD0 = _mm_srai_epi32(mmtmpD0,output_shift);

        mmtmpD1 = _mm_madd_epi16(dl_ch0_128[1],dl_ch0_128[1]);
        mmtmpD1 = _mm_srai_epi32(mmtmpD1,output_shift);

        mmtmpD0 = _mm_packs_epi32(mmtmpD0,mmtmpD1);

        dl_ch_mag128[0] = _mm_unpacklo_epi16(mmtmpD0,mmtmpD0);
        dl_ch_mag128b[0] = dl_ch_mag128[0];
        dl_ch_mag128[0] = _mm_mulhi_epi16(dl_ch_mag128[0],QAM_amp128);
        dl_ch_mag128[0] = _mm_slli_epi16(dl_ch_mag128[0],1);

        //print_shorts("dl_ch_mag128[0]=",&dl_ch_mag128[0]);

        dl_ch_mag128[1] = _mm_unpackhi_epi16(mmtmpD0,mmtmpD0);
        dl_ch_mag128b[1] = dl_ch_mag128[1];
        dl_ch_mag128[1] = _mm_mulhi_epi16(dl_ch_mag128[1],QAM_amp128);
        dl_ch_mag128[1] = _mm_slli_epi16(dl_ch_mag128[1],1);

        if (pilots==0) {
          mmtmpD0 = _mm_madd_epi16(dl_ch0_128[2],dl_ch0_128[2]);
          mmtmpD0 = _mm_srai_epi32(mmtmpD0,output_shift);

          mmtmpD1 = _mm_packs_epi32(mmtmpD0,mmtmpD0);

          dl_ch_mag128[2] = _mm_unpacklo_epi16(mmtmpD1,mmtmpD1);
          dl_ch_mag128b[2] = dl_ch_mag128[2];

          dl_ch_mag128[2] = _mm_mulhi_epi16(dl_ch_mag128[2],QAM_amp128);
          dl_ch_mag128[2] = _mm_slli_epi16(dl_ch_mag128[2],1);
        }

        dl_ch_mag128b[0] = _mm_mulhi_epi16(dl_ch_mag128b[0],QAM_amp128b);
        dl_ch_mag128b[0] = _mm_slli_epi16(dl_ch_mag128b[0],1);

        //print_shorts("dl_ch_mag128b[0]=",&dl_ch_mag128b[0]);

        dl_ch_mag128b[1] = _mm_mulhi_epi16(dl_ch_mag128b[1],QAM_amp128b);
        dl_ch_mag128b[1] = _mm_slli_epi16(dl_ch_mag128b[1],1);

        if (pilots==0) {
          dl_ch_mag128b[2] = _mm_mulhi_epi16(dl_ch_mag128b[2],QAM_amp128b);
          dl_ch_mag128b[2] = _mm_slli_epi16(dl_ch_mag128b[2],1);
        }
      }

      // MF multiply by conjugated channel
      mmtmpD0 = _mm_madd_epi16(dl_ch0_128[0],rxdataF128[0]);
      //  print_ints("re",&mmtmpD0);

      // mmtmpD0 contains real part of 4 consecutive outputs (32-bit)
      mmtmpD1 = _mm_shufflelo_epi16(dl_ch0_128[0],_MM_SHUFFLE(2,3,0,1));
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
      mmtmpD0 = _mm_madd_epi16(dl_ch0_128[1],rxdataF128[1]);
      // mmtmpD0 contains real part of 4 consecutive outputs (32-bit)
      mmtmpD1 = _mm_shufflelo_epi16(dl_ch0_128[1],_MM_SHUFFLE(2,3,0,1));
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
        mmtmpD0 = _mm_madd_epi16(dl_ch0_128[2],rxdataF128[2]);
        // mmtmpD0 contains real part of 4 consecutive outputs (32-bit)
        mmtmpD1 = _mm_shufflelo_epi16(dl_ch0_128[2],_MM_SHUFFLE(2,3,0,1));
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
						     (nb_rb*Nre))) - (phy_measurements->n0_power[aarx]));
  } // rx_antennas

  phy_measurements->precoded_cqi_dB[eNB_id][0] = dB_fixed2(precoded_signal_strength,phy_measurements->n0_power_tot);

  //printf("eNB_id %d, symbol %d: precoded CQI %d dB\n",eNB_id,symbol,
  //   phy_measurements->precoded_cqi_dB[eNB_id][0]);

  _mm_empty();
  _m_empty();

#elif defined(__arm__)

  uint32_t rb,Nre;
  uint32_t aarx,symbol_mod,pilots=0;
  
  int16x4_t *dl_ch0_128,*dl_ch1_128,*rxdataF128;
  int16x8_t *dl_ch0_128b,*dl_ch1_128b;
  int32x4_t mmtmpD0,mmtmpD1,mmtmpD0b,mmtmpD1b;
  int16x8_t *dl_ch_mag128,*dl_ch_mag128b,mmtmpD2,mmtmpD3,mmtmpD4,*rxdataF_comp128;
  int16x8_t QAM_amp128,QAM_amp128b;
  
  int16_t conj[4]__attribute__((aligned(16))) = {1,-1,1,-1};
  int32x4_t output_shift128 = vmovq_n_s32(-(int32_t)output_shift);
  int32_t precoded_signal_strength=0;
  
  symbol_mod = (symbol>=(7-frame_parms->Ncp)) ? symbol-(7-frame_parms->Ncp) : symbol;  
  if ((symbol_mod == 0) || (symbol_mod == (4-frame_parms->Ncp))) {
    if (frame_parms->mode1_flag==1) // 10 out of 12 so don't reduce size
      { nb_rb=1+(5*nb_rb/6); }
    
    else
      { pilots=1; }
  }
  
  
  if (mod_order == 4) {
    QAM_amp128  = vmovq_n_s16(QAM16_n1);  // 2/sqrt(10)
    QAM_amp128b = vmovq_n_s16(0);
    
  } else if (mod_order == 6) {
    QAM_amp128  = vmovq_n_s16(QAM64_n1); //
    QAM_amp128b = vmovq_n_s16(QAM64_n2);
  }
  
  //    printf("comp: rxdataF_comp %p, symbol %d\n",rxdataF_comp[0],symbol);
  
  for (aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++) {
    
    
    
    dl_ch0_128          = (int16x4_t*)&dl_ch_estimates_ext[aarx][symbol*frame_parms->N_RB_DL*12];
    dl_ch1_128          = (int16x4_t*)&dl_ch_estimates_ext[2+aarx][symbol*frame_parms->N_RB_DL*12];
    dl_ch0_128b         = (int16x8_t*)&dl_ch_estimates_ext[aarx][symbol*frame_parms->N_RB_DL*12];
    dl_ch1_128b         = (int16x8_t*)&dl_ch_estimates_ext[2+aarx][symbol*frame_parms->N_RB_DL*12];
    dl_ch_mag128        = (int16x8_t*)&dl_ch_mag[aarx][symbol*frame_parms->N_RB_DL*12];
    dl_ch_mag128b       = (int16x8_t*)&dl_ch_magb[aarx][symbol*frame_parms->N_RB_DL*12];
    rxdataF128          = (int16x4_t*)&rxdataF_ext[aarx][symbol*frame_parms->N_RB_DL*12];
    rxdataF_comp128     = (int16x8_t*)&rxdataF_comp[aarx][symbol*frame_parms->N_RB_DL*12];
    
    for (rb=0; rb<nb_rb; rb++) {
#ifdef DEBUG_DLSCH_DEMOD
      printf("mode 6 prec: rb %d, pmi->%d\n",rb,pmi_ext[rb]);
#endif
      prec2A_TM56_128(pmi_ext[rb],&dl_ch0_128b[0],&dl_ch1_128b[0]);
      prec2A_TM56_128(pmi_ext[rb],&dl_ch0_128b[1],&dl_ch1_128b[1]);
      
      if (pilots==0) {
	prec2A_TM56_128(pmi_ext[rb],&dl_ch0_128b[2],&dl_ch1_128b[2]);
      }
      
      if (mod_order>2) {
	// get channel amplitude if not QPSK
	mmtmpD0 = vmull_s16(dl_ch0_128[0], dl_ch0_128[0]);
	// mmtmpD0 = [ch0*ch0,ch1*ch1,ch2*ch2,ch3*ch3];
	mmtmpD0 = vqshlq_s32(vqaddq_s32(mmtmpD0,vrev64q_s32(mmtmpD0)),output_shift128);
	// mmtmpD0 = [ch0*ch0 + ch1*ch1,ch0*ch0 + ch1*ch1,ch2*ch2 + ch3*ch3,ch2*ch2 + ch3*ch3]>>output_shift128 on 32-bits
	mmtmpD1 = vmull_s16(dl_ch0_128[1], dl_ch0_128[1]);
	mmtmpD1 = vqshlq_s32(vqaddq_s32(mmtmpD1,vrev64q_s32(mmtmpD1)),output_shift128);
	mmtmpD2 = vcombine_s16(vmovn_s32(mmtmpD0),vmovn_s32(mmtmpD1));
	// mmtmpD2 = [ch0*ch0 + ch1*ch1,ch0*ch0 + ch1*ch1,ch2*ch2 + ch3*ch3,ch2*ch2 + ch3*ch3,ch4*ch4 + ch5*ch5,ch4*ch4 + ch5*ch5,ch6*ch6 + ch7*ch7,ch6*ch6 + ch7*ch7]>>output_shift128 on 16-bits 
	mmtmpD0 = vmull_s16(dl_ch0_128[2], dl_ch0_128[2]);
	mmtmpD0 = vqshlq_s32(vqaddq_s32(mmtmpD0,vrev64q_s32(mmtmpD0)),output_shift128);
	mmtmpD1 = vmull_s16(dl_ch0_128[3], dl_ch0_128[3]);
	mmtmpD1 = vqshlq_s32(vqaddq_s32(mmtmpD1,vrev64q_s32(mmtmpD1)),output_shift128);
	mmtmpD3 = vcombine_s16(vmovn_s32(mmtmpD0),vmovn_s32(mmtmpD1));
	if (pilots==0) {
	  mmtmpD0 = vmull_s16(dl_ch0_128[4], dl_ch0_128[4]);
	  mmtmpD0 = vqshlq_s32(vqaddq_s32(mmtmpD0,vrev64q_s32(mmtmpD0)),output_shift128);
	  mmtmpD1 = vmull_s16(dl_ch0_128[5], dl_ch0_128[5]);
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
      mmtmpD0 = vmull_s16(dl_ch0_128[0], rxdataF128[0]);
      //mmtmpD0 = [Re(ch[0])Re(rx[0]) Im(ch[0])Im(ch[0]) Re(ch[1])Re(rx[1]) Im(ch[1])Im(ch[1])] 
      mmtmpD1 = vmull_s16(dl_ch0_128[1], rxdataF128[1]);
      //mmtmpD1 = [Re(ch[2])Re(rx[2]) Im(ch[2])Im(ch[2]) Re(ch[3])Re(rx[3]) Im(ch[3])Im(ch[3])] 
      mmtmpD0 = vcombine_s32(vpadd_s32(vget_low_s32(mmtmpD0),vget_high_s32(mmtmpD0)),
			     vpadd_s32(vget_low_s32(mmtmpD1),vget_high_s32(mmtmpD1)));
      //mmtmpD0 = [Re(ch[0])Re(rx[0])+Im(ch[0])Im(ch[0]) Re(ch[1])Re(rx[1])+Im(ch[1])Im(ch[1]) Re(ch[2])Re(rx[2])+Im(ch[2])Im(ch[2]) Re(ch[3])Re(rx[3])+Im(ch[3])Im(ch[3])] 
      
      mmtmpD0b = vmull_s16(vrev32_s16(vmul_s16(dl_ch0_128[0],*(int16x4_t*)conj)), rxdataF128[0]);
      //mmtmpD0 = [-Im(ch[0])Re(rx[0]) Re(ch[0])Im(rx[0]) -Im(ch[1])Re(rx[1]) Re(ch[1])Im(rx[1])]
      mmtmpD1b = vmull_s16(vrev32_s16(vmul_s16(dl_ch0_128[1],*(int16x4_t*)conj)), rxdataF128[1]);
      //mmtmpD0 = [-Im(ch[2])Re(rx[2]) Re(ch[2])Im(rx[2]) -Im(ch[3])Re(rx[3]) Re(ch[3])Im(rx[3])]
      mmtmpD1 = vcombine_s32(vpadd_s32(vget_low_s32(mmtmpD0b),vget_high_s32(mmtmpD0b)),
			     vpadd_s32(vget_low_s32(mmtmpD1b),vget_high_s32(mmtmpD1b)));
      //mmtmpD1 = [-Im(ch[0])Re(rx[0])+Re(ch[0])Im(rx[0]) -Im(ch[1])Re(rx[1])+Re(ch[1])Im(rx[1]) -Im(ch[2])Re(rx[2])+Re(ch[2])Im(rx[2]) -Im(ch[3])Re(rx[3])+Re(ch[3])Im(rx[3])]
      
      mmtmpD0 = vqshlq_s32(mmtmpD0,output_shift128);
      mmtmpD1 = vqshlq_s32(mmtmpD1,output_shift128);
      rxdataF_comp128[0] = vcombine_s16(vmovn_s32(mmtmpD0),vmovn_s32(mmtmpD1));
      
      mmtmpD0 = vmull_s16(dl_ch0_128[2], rxdataF128[2]);
      mmtmpD1 = vmull_s16(dl_ch0_128[3], rxdataF128[3]);
      mmtmpD0 = vcombine_s32(vpadd_s32(vget_low_s32(mmtmpD0),vget_high_s32(mmtmpD0)),
			     vpadd_s32(vget_low_s32(mmtmpD1),vget_high_s32(mmtmpD1)));
      
      mmtmpD0b = vmull_s16(vrev32_s16(vmul_s16(dl_ch0_128[2],*(int16x4_t*)conj)), rxdataF128[2]);
      mmtmpD1b = vmull_s16(vrev32_s16(vmul_s16(dl_ch0_128[3],*(int16x4_t*)conj)), rxdataF128[3]);
      mmtmpD1 = vcombine_s32(vpadd_s32(vget_low_s32(mmtmpD0b),vget_high_s32(mmtmpD0b)),
			     vpadd_s32(vget_low_s32(mmtmpD1b),vget_high_s32(mmtmpD1b)));
      
      mmtmpD0 = vqshlq_s32(mmtmpD0,output_shift128);
      mmtmpD1 = vqshlq_s32(mmtmpD1,output_shift128);
      rxdataF_comp128[1] = vcombine_s16(vmovn_s32(mmtmpD0),vmovn_s32(mmtmpD1));
      
      if (pilots==0) {
	mmtmpD0 = vmull_s16(dl_ch0_128[4], rxdataF128[4]);
	mmtmpD1 = vmull_s16(dl_ch0_128[5], rxdataF128[5]);
	mmtmpD0 = vcombine_s32(vpadd_s32(vget_low_s32(mmtmpD0),vget_high_s32(mmtmpD0)),
			       vpadd_s32(vget_low_s32(mmtmpD1),vget_high_s32(mmtmpD1)));
	
	mmtmpD0b = vmull_s16(vrev32_s16(vmul_s16(dl_ch0_128[4],*(int16x4_t*)conj)), rxdataF128[4]);
	mmtmpD1b = vmull_s16(vrev32_s16(vmul_s16(dl_ch0_128[5],*(int16x4_t*)conj)), rxdataF128[5]);
	mmtmpD1 = vcombine_s32(vpadd_s32(vget_low_s32(mmtmpD0b),vget_high_s32(mmtmpD0b)),
			       vpadd_s32(vget_low_s32(mmtmpD1b),vget_high_s32(mmtmpD1b)));
	
	
	mmtmpD0 = vqshlq_s32(mmtmpD0,output_shift128);
	mmtmpD1 = vqshlq_s32(mmtmpD1,output_shift128);
	rxdataF_comp128[2] = vcombine_s16(vmovn_s32(mmtmpD0),vmovn_s32(mmtmpD1));
	
	
	dl_ch0_128+=6;
	dl_ch1_128+=6;
	dl_ch_mag128+=3;
	dl_ch_mag128b+=3;
	rxdataF128+=6;
	rxdataF_comp128+=3;
	
      } else { // we have a smaller PDSCH in symbols with pilots so skip last group of 4 REs and increment less
	dl_ch0_128+=4;
	dl_ch1_128+=4;
	dl_ch_mag128+=2;
	dl_ch_mag128b+=2;
	rxdataF128+=4;
	rxdataF_comp128+=2;
      }
    }
    
    Nre = (pilots==0) ? 12 : 8;
    
    precoded_signal_strength += ((signal_energy_nodc(&dl_ch_estimates_ext[aarx][symbol*frame_parms->N_RB_DL*Nre],
						     (nb_rb*Nre))) - (phy_measurements->n0_power[aarx]));
    // rx_antennas
  }
  phy_measurements->precoded_cqi_dB[eNB_id][0] = dB_fixed2(precoded_signal_strength,phy_measurements->n0_power_tot);
  
#endif
}

void dlsch_channel_compensation_TM3(LTE_DL_FRAME_PARMS *frame_parms,
                                    LTE_UE_PDSCH *lte_ue_pdsch_vars,
                                    PHY_MEASUREMENTS *phy_measurements,
                                    int eNB_id,
                                    unsigned char symbol,
                                    unsigned char mod_order0,
                                    unsigned char mod_order1,
                                    int round,
                                    unsigned short nb_rb,
                                    unsigned char output_shift)
{

#if defined(__x86_64__) || defined(__i386__)

  unsigned short rb,Nre;
  __m128i *dl_ch0_128,*dl_ch1_128,*dl_ch_mag0_128,*dl_ch_mag1_128,*dl_ch_mag0_128b,*dl_ch_mag1_128b,*rxdataF128,*rxdataF_comp0_128,*rxdataF_comp1_128;
  unsigned char aarx=0,symbol_mod,pilots=0;
  int precoded_signal_strength0=0,precoded_signal_strength1=0;

  int **rxdataF_ext           = lte_ue_pdsch_vars->rxdataF_ext;
  int **dl_ch_estimates_ext   = lte_ue_pdsch_vars->dl_ch_estimates_ext;
  int **dl_ch_mag0            = lte_ue_pdsch_vars->dl_ch_mag0;
  int **dl_ch_mag1            = lte_ue_pdsch_vars->dl_ch_mag1;
  int **dl_ch_magb0           = lte_ue_pdsch_vars->dl_ch_magb0;
  int **dl_ch_magb1           = lte_ue_pdsch_vars->dl_ch_magb1;
  int **rxdataF_comp0         = lte_ue_pdsch_vars->rxdataF_comp0;
  int **rxdataF_comp1         = lte_ue_pdsch_vars->rxdataF_comp1[round]; //?
  __m128i mmtmpD0,mmtmpD1,mmtmpD2,mmtmpD3,QAM_amp0_128,QAM_amp0_128b,QAM_amp1_128,QAM_amp1_128b;


  symbol_mod = (symbol>=(7-frame_parms->Ncp)) ? symbol-(7-frame_parms->Ncp) : symbol;

  if ((symbol_mod == 0) || (symbol_mod == (4-frame_parms->Ncp)))
    pilots=1;


  //printf("comp prec: symbol %d, pilots %d\n",symbol, pilots);

  if (mod_order0 == 4) {
    QAM_amp0_128  = _mm_set1_epi16(QAM16_n1);
    QAM_amp0_128b = _mm_setzero_si128();
  } else if (mod_order0 == 6) {
    QAM_amp0_128  = _mm_set1_epi16(QAM64_n1);
    QAM_amp0_128b = _mm_set1_epi16(QAM64_n2);
  }

  if (mod_order1 == 4) {
    QAM_amp1_128  = _mm_set1_epi16(QAM16_n1);
    QAM_amp1_128b = _mm_setzero_si128();
  } else if (mod_order1 == 6) {
    QAM_amp1_128  = _mm_set1_epi16(QAM64_n1);
    QAM_amp1_128b = _mm_set1_epi16(QAM64_n2);
  }

  for (aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++) {

    dl_ch0_128          = (__m128i *)&dl_ch_estimates_ext[aarx][symbol*frame_parms->N_RB_DL*12];    // hr,0
    dl_ch1_128          = (__m128i *)&dl_ch_estimates_ext[2+aarx][symbol*frame_parms->N_RB_DL*12];  // hr,1


    dl_ch_mag0_128      = (__m128i *)&dl_ch_mag0[aarx][symbol*frame_parms->N_RB_DL*12];
    dl_ch_mag0_128b     = (__m128i *)&dl_ch_magb0[aarx][symbol*frame_parms->N_RB_DL*12];
    dl_ch_mag1_128      = (__m128i *)&dl_ch_mag1[aarx][symbol*frame_parms->N_RB_DL*12];
    dl_ch_mag1_128b     = (__m128i *)&dl_ch_magb1[aarx][symbol*frame_parms->N_RB_DL*12];
    rxdataF128          = (__m128i *)&rxdataF_ext[aarx][symbol*frame_parms->N_RB_DL*12];           // yr
    rxdataF_comp0_128   = (__m128i *)&rxdataF_comp0[aarx][symbol*frame_parms->N_RB_DL*12];         // yr,0 = yr * conj(hr,0)
    rxdataF_comp1_128   = (__m128i *)&rxdataF_comp1[aarx][symbol*frame_parms->N_RB_DL*12];         // yr,1 = yr * conj(hr,1)


    for (rb=0; rb<nb_rb; rb++) {
      // combine TX channels using precoder from pmi

      prec2A_TM3_128(&dl_ch0_128[0],&dl_ch1_128[0]);
      prec2A_TM3_128(&dl_ch0_128[1],&dl_ch1_128[1]);

      if (pilots==0) {
        prec2A_TM3_128(&dl_ch0_128[2],&dl_ch1_128[2]);
      }

      if (mod_order0>2) {
        // get channel amplitude if not QPSK

        mmtmpD0 = _mm_madd_epi16(dl_ch0_128[0],dl_ch0_128[0]);
        mmtmpD0 = _mm_srai_epi32(mmtmpD0,output_shift);

        mmtmpD1 = _mm_madd_epi16(dl_ch0_128[1],dl_ch0_128[1]);
        mmtmpD1 = _mm_srai_epi32(mmtmpD1,output_shift);

        mmtmpD0 = _mm_packs_epi32(mmtmpD0,mmtmpD1);

        dl_ch_mag0_128[0] = _mm_unpacklo_epi16(mmtmpD0,mmtmpD0);
        dl_ch_mag0_128b[0] = dl_ch_mag0_128[0];
        dl_ch_mag0_128[0] = _mm_mulhi_epi16(dl_ch_mag0_128[0],QAM_amp0_128);
        dl_ch_mag0_128[0] = _mm_slli_epi16(dl_ch_mag0_128[0],1);

        //  print_shorts("dl_ch_mag0_128[0]=",&dl_ch_mag0_128[0]);

        dl_ch_mag0_128[1] = _mm_unpackhi_epi16(mmtmpD0,mmtmpD0);
        dl_ch_mag0_128b[1] = dl_ch_mag0_128[1];
        dl_ch_mag0_128[1] = _mm_mulhi_epi16(dl_ch_mag0_128[1],QAM_amp0_128);
        dl_ch_mag0_128[1] = _mm_slli_epi16(dl_ch_mag0_128[1],1);

        if (pilots==0) {
          mmtmpD0 = _mm_madd_epi16(dl_ch0_128[2],dl_ch0_128[2]);
          mmtmpD0 = _mm_srai_epi32(mmtmpD0,output_shift);

          mmtmpD1 = _mm_packs_epi32(mmtmpD0,mmtmpD0);

          dl_ch_mag0_128[2] = _mm_unpacklo_epi16(mmtmpD1,mmtmpD1);
          dl_ch_mag0_128b[2] = dl_ch_mag0_128[2];

          dl_ch_mag0_128[2] = _mm_mulhi_epi16(dl_ch_mag0_128[2],QAM_amp0_128);
          dl_ch_mag0_128[2] = _mm_slli_epi16(dl_ch_mag0_128[2],1);
        }

        dl_ch_mag0_128b[0] = _mm_mulhi_epi16(dl_ch_mag0_128b[0],QAM_amp0_128b);
        dl_ch_mag0_128b[0] = _mm_slli_epi16(dl_ch_mag0_128b[0],1);

        //print_shorts("dl_ch_mag128b[0]=",&dl_ch_mag128b[0]);

        dl_ch_mag0_128b[1] = _mm_mulhi_epi16(dl_ch_mag0_128b[1],QAM_amp0_128b);
        dl_ch_mag0_128b[1] = _mm_slli_epi16(dl_ch_mag0_128b[1],1);

        if (pilots==0) {
          dl_ch_mag0_128b[2] = _mm_mulhi_epi16(dl_ch_mag0_128b[2],QAM_amp0_128b);
          dl_ch_mag0_128b[2] = _mm_slli_epi16(dl_ch_mag0_128b[2],1);
        }
      }

      if (mod_order1>2) {
        // get channel amplitude if not QPSK

        mmtmpD0 = _mm_madd_epi16(dl_ch1_128[0],dl_ch1_128[0]);
        mmtmpD0 = _mm_srai_epi32(mmtmpD0,output_shift);

        mmtmpD1 = _mm_madd_epi16(dl_ch1_128[1],dl_ch1_128[1]);
        mmtmpD1 = _mm_srai_epi32(mmtmpD1,output_shift);

        mmtmpD0 = _mm_packs_epi32(mmtmpD0,mmtmpD1);

        dl_ch_mag1_128[0] = _mm_unpacklo_epi16(mmtmpD0,mmtmpD0);
        dl_ch_mag1_128b[0] = dl_ch_mag1_128[0];
        dl_ch_mag1_128[0] = _mm_mulhi_epi16(dl_ch_mag1_128[0],QAM_amp1_128);
        dl_ch_mag1_128[0] = _mm_slli_epi16(dl_ch_mag1_128[0],1);

        //print_shorts("dl_ch_mag128[0]=",&dl_ch_mag128[0]);

        dl_ch_mag1_128[1] = _mm_unpackhi_epi16(mmtmpD0,mmtmpD0);
        dl_ch_mag1_128b[1] = dl_ch_mag1_128[1];
        dl_ch_mag1_128[1] = _mm_mulhi_epi16(dl_ch_mag1_128[1],QAM_amp1_128);
        dl_ch_mag1_128[1] = _mm_slli_epi16(dl_ch_mag1_128[1],1);

        if (pilots==0) {
          mmtmpD0 = _mm_madd_epi16(dl_ch1_128[2],dl_ch1_128[2]);
          mmtmpD0 = _mm_srai_epi32(mmtmpD0,output_shift);

          mmtmpD1 = _mm_packs_epi32(mmtmpD0,mmtmpD0);

          dl_ch_mag1_128[2] = _mm_unpacklo_epi16(mmtmpD1,mmtmpD1);
          dl_ch_mag1_128b[2] = dl_ch_mag1_128[2];

          dl_ch_mag1_128[2] = _mm_mulhi_epi16(dl_ch_mag1_128[2],QAM_amp1_128);
          dl_ch_mag1_128[2] = _mm_slli_epi16(dl_ch_mag1_128[2],1);
        }

        dl_ch_mag1_128b[0] = _mm_mulhi_epi16(dl_ch_mag1_128b[0],QAM_amp1_128b);
        dl_ch_mag1_128b[0] = _mm_slli_epi16(dl_ch_mag1_128b[0],1);

        //print_shorts("dl_ch_mag128b[0]=",&dl_ch_mag128b[0]);

        dl_ch_mag1_128b[1] = _mm_mulhi_epi16(dl_ch_mag1_128b[1],QAM_amp1_128b);
        dl_ch_mag1_128b[1] = _mm_slli_epi16(dl_ch_mag1_128b[1],1);

        if (pilots==0) {
          dl_ch_mag1_128b[2] = _mm_mulhi_epi16(dl_ch_mag1_128b[2],QAM_amp1_128b);
          dl_ch_mag1_128b[2] = _mm_slli_epi16(dl_ch_mag1_128b[2],1);
        }
      }

      // layer 0
      // MF multiply by conjugated channel
      mmtmpD0 = _mm_madd_epi16(dl_ch0_128[0],rxdataF128[0]);
      //print_ints("re",&mmtmpD0);

      // mmtmpD0 contains real part of 4 consecutive outputs (32-bit)
      mmtmpD1 = _mm_shufflelo_epi16(dl_ch0_128[0],_MM_SHUFFLE(2,3,0,1));
      mmtmpD1 = _mm_shufflehi_epi16(mmtmpD1,_MM_SHUFFLE(2,3,0,1));
      mmtmpD1 = _mm_sign_epi16(mmtmpD1,*(__m128i*)&conjugate[0]);
      mmtmpD1 = _mm_madd_epi16(mmtmpD1,rxdataF128[0]);
      //      print_ints("im",&mmtmpD1);
      // mmtmpD1 contains imag part of 4 consecutive outputs (32-bit)
      mmtmpD0 = _mm_srai_epi32(mmtmpD0,output_shift);
      //      printf("Shift: %d\n",output_shift);
      //      print_ints("re(shift)",&mmtmpD0);
      mmtmpD1 = _mm_srai_epi32(mmtmpD1,output_shift);
      //      print_ints("im(shift)",&mmtmpD1);
      mmtmpD2 = _mm_unpacklo_epi32(mmtmpD0,mmtmpD1);
      mmtmpD3 = _mm_unpackhi_epi32(mmtmpD0,mmtmpD1);
      //      print_ints("c0",&mmtmpD2);
      //      print_ints("c1",&mmtmpD3);
      rxdataF_comp0_128[0] = _mm_packs_epi32(mmtmpD2,mmtmpD3);
      //      print_shorts("rx:",rxdataF128);
      //      print_shorts("ch:",dl_ch0_128);
      //      print_shorts("pack:",rxdataF_comp0_128);

      // multiply by conjugated channel
      mmtmpD0 = _mm_madd_epi16(dl_ch0_128[1],rxdataF128[1]);
      // mmtmpD0 contains real part of 4 consecutive outputs (32-bit)
      mmtmpD1 = _mm_shufflelo_epi16(dl_ch0_128[1],_MM_SHUFFLE(2,3,0,1));
      mmtmpD1 = _mm_shufflehi_epi16(mmtmpD1,_MM_SHUFFLE(2,3,0,1));
      mmtmpD1 = _mm_sign_epi16(mmtmpD1,*(__m128i*)conjugate);
      mmtmpD1 = _mm_madd_epi16(mmtmpD1,rxdataF128[1]);
      // mmtmpD1 contains imag part of 4 consecutive outputs (32-bit)
      mmtmpD0 = _mm_srai_epi32(mmtmpD0,output_shift);
      mmtmpD1 = _mm_srai_epi32(mmtmpD1,output_shift);
      mmtmpD2 = _mm_unpacklo_epi32(mmtmpD0,mmtmpD1);
      mmtmpD3 = _mm_unpackhi_epi32(mmtmpD0,mmtmpD1);

      rxdataF_comp0_128[1] = _mm_packs_epi32(mmtmpD2,mmtmpD3);
      //  print_shorts("rx:",rxdataF128+1);
      //  print_shorts("ch:",dl_ch0_128+1);
      //  print_shorts("pack:",rxdataF_comp128+1);

      if (pilots==0) {
        // multiply by conjugated channel
        mmtmpD0 = _mm_madd_epi16(dl_ch0_128[2],rxdataF128[2]);
        // mmtmpD0 contains real part of 4 consecutive outputs (32-bit)
        mmtmpD1 = _mm_shufflelo_epi16(dl_ch0_128[2],_MM_SHUFFLE(2,3,0,1));
        mmtmpD1 = _mm_shufflehi_epi16(mmtmpD1,_MM_SHUFFLE(2,3,0,1));
        mmtmpD1 = _mm_sign_epi16(mmtmpD1,*(__m128i*)conjugate);
        mmtmpD1 = _mm_madd_epi16(mmtmpD1,rxdataF128[2]);
        // mmtmpD1 contains imag part of 4 consecutive outputs (32-bit)
        mmtmpD0 = _mm_srai_epi32(mmtmpD0,output_shift);
        mmtmpD1 = _mm_srai_epi32(mmtmpD1,output_shift);
        mmtmpD2 = _mm_unpacklo_epi32(mmtmpD0,mmtmpD1);
        mmtmpD3 = _mm_unpackhi_epi32(mmtmpD0,mmtmpD1);

        rxdataF_comp0_128[2] = _mm_packs_epi32(mmtmpD2,mmtmpD3);
        //  print_shorts("rx:",rxdataF128+2);
        //  print_shorts("ch:",dl_ch0_128+2);
        //        print_shorts("pack:",rxdataF_comp128+2);

      } else {

      }


      // layer 1
      // MF multiply by conjugated channel
      mmtmpD0 = _mm_madd_epi16(dl_ch1_128[0],rxdataF128[0]);
      //  print_ints("re",&mmtmpD0);

      // mmtmpD0 contains real part of 4 consecutive outputs (32-bit)
      mmtmpD1 = _mm_shufflelo_epi16(dl_ch1_128[0],_MM_SHUFFLE(2,3,0,1));
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
      rxdataF_comp1_128[0] = _mm_packs_epi32(mmtmpD2,mmtmpD3);
      //  print_shorts("rx:",rxdataF128);
      //  print_shorts("ch:",dl_ch128);
      //  print_shorts("pack:",rxdataF_comp128);

      // multiply by conjugated channel
      mmtmpD0 = _mm_madd_epi16(dl_ch1_128[1],rxdataF128[1]);
      // mmtmpD0 contains real part of 4 consecutive outputs (32-bit)
      mmtmpD1 = _mm_shufflelo_epi16(dl_ch1_128[1],_MM_SHUFFLE(2,3,0,1));
      mmtmpD1 = _mm_shufflehi_epi16(mmtmpD1,_MM_SHUFFLE(2,3,0,1));
      mmtmpD1 = _mm_sign_epi16(mmtmpD1,*(__m128i*)conjugate);
      mmtmpD1 = _mm_madd_epi16(mmtmpD1,rxdataF128[1]);
      // mmtmpD1 contains imag part of 4 consecutive outputs (32-bit)
      mmtmpD0 = _mm_srai_epi32(mmtmpD0,output_shift);
      mmtmpD1 = _mm_srai_epi32(mmtmpD1,output_shift);
      mmtmpD2 = _mm_unpacklo_epi32(mmtmpD0,mmtmpD1);
      mmtmpD3 = _mm_unpackhi_epi32(mmtmpD0,mmtmpD1);

      rxdataF_comp1_128[1] = _mm_packs_epi32(mmtmpD2,mmtmpD3);
      //  print_shorts("rx:",rxdataF128+1);
      //  print_shorts("ch:",dl_ch128+1);
      //  print_shorts("pack:",rxdataF_comp128+1);

      if (pilots==0) {
        // multiply by conjugated channel
        mmtmpD0 = _mm_madd_epi16(dl_ch1_128[2],rxdataF128[2]);
        // mmtmpD0 contains real part of 4 consecutive outputs (32-bit)
        mmtmpD1 = _mm_shufflelo_epi16(dl_ch1_128[2],_MM_SHUFFLE(2,3,0,1));
        mmtmpD1 = _mm_shufflehi_epi16(mmtmpD1,_MM_SHUFFLE(2,3,0,1));
        mmtmpD1 = _mm_sign_epi16(mmtmpD1,*(__m128i*)conjugate);
        mmtmpD1 = _mm_madd_epi16(mmtmpD1,rxdataF128[2]);
        // mmtmpD1 contains imag part of 4 consecutive outputs (32-bit)
        mmtmpD0 = _mm_srai_epi32(mmtmpD0,output_shift);
        mmtmpD1 = _mm_srai_epi32(mmtmpD1,output_shift);
        mmtmpD2 = _mm_unpacklo_epi32(mmtmpD0,mmtmpD1);
        mmtmpD3 = _mm_unpackhi_epi32(mmtmpD0,mmtmpD1);

        rxdataF_comp1_128[2] = _mm_packs_epi32(mmtmpD2,mmtmpD3);
        //  print_shorts("rx:",rxdataF128+2);
        //  print_shorts("ch:",dl_ch128+2);
        //        print_shorts("pack:",rxdataF_comp128+2);

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
  }
  
  Nre = (pilots==0) ? 12 : 8;

  precoded_signal_strength0 += ((signal_energy_nodc(&dl_ch_estimates_ext[aarx][symbol*frame_parms->N_RB_DL*Nre],
                                 (nb_rb*Nre))) - (phy_measurements->n0_power[aarx]));

  precoded_signal_strength1 += ((signal_energy_nodc(&dl_ch_estimates_ext[aarx+2][symbol*frame_parms->N_RB_DL*Nre],
                                 (nb_rb*Nre))) - (phy_measurements->n0_power[aarx]));

  phy_measurements->precoded_cqi_dB[eNB_id][0] = dB_fixed2(precoded_signal_strength0,phy_measurements->n0_power_tot);
  phy_measurements->precoded_cqi_dB[eNB_id][1] = dB_fixed2(precoded_signal_strength1,phy_measurements->n0_power_tot);

  //printf("eNB_id %d, symbol %d: precoded CQI %d dB\n",eNB_id,symbol,
  //   phy_measurements->precoded_cqi_dB[eNB_id][0]);

  _mm_empty();
  _m_empty();

#elif defined(__arm__)

  unsigned short rb,Nre;
  unsigned char aarx,symbol_mod,pilots=0;
  int precoded_signal_strength0=0,precoded_signal_strength1=0;  
  int16x4_t *dl_ch0_128,*rxdataF128;
  int16x4_t *dl_ch1_128;
  int16x8_t *dl_ch0_128b,*dl_ch1_128b;
  
  int32x4_t mmtmpD0,mmtmpD1,mmtmpD0b,mmtmpD1b;
  int16x8_t *dl_ch_mag0_128,*dl_ch_mag0_128b,*dl_ch_mag1_128,*dl_ch_mag1_128b,mmtmpD2,mmtmpD3,mmtmpD4,*rxdataF_comp0_128,*rxdataF_comp1_128;
  int16x8_t QAM_amp0_128,QAM_amp0_128b,QAM_amp1_128,QAM_amp1_128b;
  int32x4_t output_shift128 = vmovq_n_s32(-(int32_t)output_shift);
  
  int **rxdataF_ext           = lte_ue_pdsch_vars->rxdataF_ext;
  int **dl_ch_estimates_ext   = lte_ue_pdsch_vars->dl_ch_estimates_ext;
  int **dl_ch_mag0            = lte_ue_pdsch_vars->dl_ch_mag0;
  int **dl_ch_mag1            = lte_ue_pdsch_vars->dl_ch_mag1;
  int **dl_ch_magb0           = lte_ue_pdsch_vars->dl_ch_magb0;
  int **dl_ch_magb1           = lte_ue_pdsch_vars->dl_ch_magb1;
  int **rxdataF_comp0         = lte_ue_pdsch_vars->rxdataF_comp0;
  int **rxdataF_comp1         = lte_ue_pdsch_vars->rxdataF_comp1[round]; //?
  
  int16_t conj[4]__attribute__((aligned(16))) = {1,-1,1,-1};
  
  symbol_mod = (symbol>=(7-frame_parms->Ncp)) ? symbol-(7-frame_parms->Ncp) : symbol;
  
  if ((symbol_mod == 0) || (symbol_mod == (4-frame_parms->Ncp))) {
    if (frame_parms->mode1_flag==1) // 10 out of 12 so don't reduce size
      { nb_rb=1+(5*nb_rb/6); }
    
    else
      { pilots=1; }
  }
  
  
  if (mod_order0 == 4) {
    QAM_amp0_128  = vmovq_n_s16(QAM16_n1);  // 2/sqrt(10)
    QAM_amp0_128b = vmovq_n_s16(0);
    
  } else if (mod_order0 == 6) {
    QAM_amp0_128  = vmovq_n_s16(QAM64_n1); //
    QAM_amp0_128b = vmovq_n_s16(QAM64_n2);
  }
  
  if (mod_order1 == 4) {
    QAM_amp1_128  = vmovq_n_s16(QAM16_n1);  // 2/sqrt(10)
    QAM_amp1_128b = vmovq_n_s16(0);
    
  } else if (mod_order1 == 6) {
    QAM_amp1_128  = vmovq_n_s16(QAM64_n1); //
    QAM_amp1_128b = vmovq_n_s16(QAM64_n2);
  }
    
  //    printf("comp: rxdataF_comp %p, symbol %d\n",rxdataF_comp[0],symbol);
    
  for (aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++) {
      
      
      
    dl_ch0_128          = (int16x4_t*)&dl_ch_estimates_ext[aarx][symbol*frame_parms->N_RB_DL*12];
    dl_ch1_128          = (int16x4_t*)&dl_ch_estimates_ext[2+aarx][symbol*frame_parms->N_RB_DL*12];
    dl_ch0_128b          = (int16x8_t*)&dl_ch_estimates_ext[aarx][symbol*frame_parms->N_RB_DL*12];
    dl_ch1_128b          = (int16x8_t*)&dl_ch_estimates_ext[2+aarx][symbol*frame_parms->N_RB_DL*12];
    dl_ch_mag0_128      = (int16x8_t*)&dl_ch_mag0[aarx][symbol*frame_parms->N_RB_DL*12];
    dl_ch_mag0_128b     = (int16x8_t*)&dl_ch_magb0[aarx][symbol*frame_parms->N_RB_DL*12];
    dl_ch_mag1_128      = (int16x8_t*)&dl_ch_mag1[aarx][symbol*frame_parms->N_RB_DL*12];
    dl_ch_mag1_128b     = (int16x8_t*)&dl_ch_magb1[aarx][symbol*frame_parms->N_RB_DL*12];
    rxdataF128          = (int16x4_t*)&rxdataF_ext[aarx][symbol*frame_parms->N_RB_DL*12];
    rxdataF_comp0_128   = (int16x8_t*)&rxdataF_comp0[aarx][symbol*frame_parms->N_RB_DL*12];
    rxdataF_comp1_128   = (int16x8_t*)&rxdataF_comp1[aarx][symbol*frame_parms->N_RB_DL*12];
      
    for (rb=0; rb<nb_rb; rb++) {
#ifdef DEBUG_DLSCH_DEMOD
      printf("mode 6 prec: rb %d, pmi->%d\n",rb,pmi_ext[rb]);
#endif

      prec2A_TM3_128(&dl_ch0_128b[0],&dl_ch1_128b[0]);
      prec2A_TM3_128(&dl_ch0_128b[1],&dl_ch1_128b[1]);
	
      if (pilots==0) {
	prec2A_TM3_128(&dl_ch0_128b[2],&dl_ch1_128b[2]);
      }

	
      if (mod_order0>2) {
	// get channel amplitude if not QPSK
	mmtmpD0 = vmull_s16(dl_ch0_128[0], dl_ch0_128[0]);
	// mmtmpD0 = [ch0*ch0,ch1*ch1,ch2*ch2,ch3*ch3];
	mmtmpD0 = vqshlq_s32(vqaddq_s32(mmtmpD0,vrev64q_s32(mmtmpD0)),output_shift128);
	// mmtmpD0 = [ch0*ch0 + ch1*ch1,ch0*ch0 + ch1*ch1,ch2*ch2 + ch3*ch3,ch2*ch2 + ch3*ch3]>>output_shift128 on 32-bits
	mmtmpD1 = vmull_s16(dl_ch0_128[1], dl_ch0_128[1]);
	mmtmpD1 = vqshlq_s32(vqaddq_s32(mmtmpD1,vrev64q_s32(mmtmpD1)),output_shift128);
	mmtmpD2 = vcombine_s16(vmovn_s32(mmtmpD0),vmovn_s32(mmtmpD1));
	// mmtmpD2 = [ch0*ch0 + ch1*ch1,ch0*ch0 + ch1*ch1,ch2*ch2 + ch3*ch3,ch2*ch2 + ch3*ch3,ch4*ch4 + ch5*ch5,ch4*ch4 + ch5*ch5,ch6*ch6 + ch7*ch7,ch6*ch6 + ch7*ch7]>>output_shift128 on 16-bits 
	mmtmpD0 = vmull_s16(dl_ch0_128[2], dl_ch0_128[2]);
	mmtmpD0 = vqshlq_s32(vqaddq_s32(mmtmpD0,vrev64q_s32(mmtmpD0)),output_shift128);
	mmtmpD1 = vmull_s16(dl_ch0_128[3], dl_ch0_128[3]);
	mmtmpD1 = vqshlq_s32(vqaddq_s32(mmtmpD1,vrev64q_s32(mmtmpD1)),output_shift128);
	mmtmpD3 = vcombine_s16(vmovn_s32(mmtmpD0),vmovn_s32(mmtmpD1));
	  
	if (pilots==0) {
	  mmtmpD0 = vmull_s16(dl_ch0_128[4], dl_ch0_128[4]);
	  mmtmpD0 = vqshlq_s32(vqaddq_s32(mmtmpD0,vrev64q_s32(mmtmpD0)),output_shift128);
	  mmtmpD1 = vmull_s16(dl_ch0_128[5], dl_ch0_128[5]);
	  mmtmpD1 = vqshlq_s32(vqaddq_s32(mmtmpD1,vrev64q_s32(mmtmpD1)),output_shift128);
	  mmtmpD4 = vcombine_s16(vmovn_s32(mmtmpD0),vmovn_s32(mmtmpD1));
	    
	    
	}
	  
	dl_ch_mag0_128b[0] = vqdmulhq_s16(mmtmpD2,QAM_amp0_128b);
	dl_ch_mag0_128b[1] = vqdmulhq_s16(mmtmpD3,QAM_amp0_128b);
	dl_ch_mag0_128[0] = vqdmulhq_s16(mmtmpD2,QAM_amp0_128);
	dl_ch_mag0_128[1] = vqdmulhq_s16(mmtmpD3,QAM_amp0_128);
	  
	  
	if (pilots==0) {
	  dl_ch_mag0_128b[2] = vqdmulhq_s16(mmtmpD4,QAM_amp0_128b);
	  dl_ch_mag0_128[2]  = vqdmulhq_s16(mmtmpD4,QAM_amp0_128);
	}
      }

      if (mod_order1>2) {
	// get channel amplitude if not QPSK
	mmtmpD0 = vmull_s16(dl_ch1_128[0], dl_ch1_128[0]);
	// mmtmpD0 = [ch0*ch0,ch1*ch1,ch2*ch2,ch3*ch3];
	mmtmpD0 = vqshlq_s32(vqaddq_s32(mmtmpD0,vrev64q_s32(mmtmpD0)),output_shift128);
	// mmtmpD0 = [ch0*ch0 + ch1*ch1,ch0*ch0 + ch1*ch1,ch2*ch2 + ch3*ch3,ch2*ch2 + ch3*ch3]>>output_shift128 on 32-bits
	mmtmpD1 = vmull_s16(dl_ch1_128[1], dl_ch1_128[1]);
	mmtmpD1 = vqshlq_s32(vqaddq_s32(mmtmpD1,vrev64q_s32(mmtmpD1)),output_shift128);
	mmtmpD2 = vcombine_s16(vmovn_s32(mmtmpD0),vmovn_s32(mmtmpD1));
	// mmtmpD2 = [ch0*ch0 + ch1*ch1,ch0*ch0 + ch1*ch1,ch2*ch2 + ch3*ch3,ch2*ch2 + ch3*ch3,ch4*ch4 + ch5*ch5,ch4*ch4 + ch5*ch5,ch6*ch6 + ch7*ch7,ch6*ch6 + ch7*ch7]>>output_shift128 on 16-bits 
	mmtmpD0 = vmull_s16(dl_ch1_128[2], dl_ch1_128[2]);
	mmtmpD0 = vqshlq_s32(vqaddq_s32(mmtmpD0,vrev64q_s32(mmtmpD0)),output_shift128);
	mmtmpD1 = vmull_s16(dl_ch1_128[3], dl_ch1_128[3]);
	mmtmpD1 = vqshlq_s32(vqaddq_s32(mmtmpD1,vrev64q_s32(mmtmpD1)),output_shift128);
	mmtmpD3 = vcombine_s16(vmovn_s32(mmtmpD0),vmovn_s32(mmtmpD1));
	  
	if (pilots==0) {
	  mmtmpD0 = vmull_s16(dl_ch1_128[4], dl_ch1_128[4]);
	  mmtmpD0 = vqshlq_s32(vqaddq_s32(mmtmpD0,vrev64q_s32(mmtmpD0)),output_shift128);
	  mmtmpD1 = vmull_s16(dl_ch1_128[5], dl_ch1_128[5]);
	  mmtmpD1 = vqshlq_s32(vqaddq_s32(mmtmpD1,vrev64q_s32(mmtmpD1)),output_shift128);
	  mmtmpD4 = vcombine_s16(vmovn_s32(mmtmpD0),vmovn_s32(mmtmpD1));
	    
	    
	}
	  
	dl_ch_mag1_128b[0] = vqdmulhq_s16(mmtmpD2,QAM_amp1_128b);
	dl_ch_mag1_128b[1] = vqdmulhq_s16(mmtmpD3,QAM_amp1_128b);
	dl_ch_mag1_128[0] = vqdmulhq_s16(mmtmpD2,QAM_amp1_128);
	dl_ch_mag1_128[1] = vqdmulhq_s16(mmtmpD3,QAM_amp1_128);
	  
	  
	if (pilots==0) {
	  dl_ch_mag1_128b[2] = vqdmulhq_s16(mmtmpD4,QAM_amp1_128b);
	  dl_ch_mag1_128[2]  = vqdmulhq_s16(mmtmpD4,QAM_amp1_128);
	}
      }
	
      mmtmpD0 = vmull_s16(dl_ch0_128[0], rxdataF128[0]);
      //mmtmpD0 = [Re(ch[0])Re(rx[0]) Im(ch[0])Im(ch[0]) Re(ch[1])Re(rx[1]) Im(ch[1])Im(ch[1])] 
      mmtmpD1 = vmull_s16(dl_ch0_128[1], rxdataF128[1]);
      //mmtmpD1 = [Re(ch[2])Re(rx[2]) Im(ch[2])Im(ch[2]) Re(ch[3])Re(rx[3]) Im(ch[3])Im(ch[3])] 
      mmtmpD0 = vcombine_s32(vpadd_s32(vget_low_s32(mmtmpD0),vget_high_s32(mmtmpD0)),
			     vpadd_s32(vget_low_s32(mmtmpD1),vget_high_s32(mmtmpD1)));
      //mmtmpD0 = [Re(ch[0])Re(rx[0])+Im(ch[0])Im(ch[0]) Re(ch[1])Re(rx[1])+Im(ch[1])Im(ch[1]) Re(ch[2])Re(rx[2])+Im(ch[2])Im(ch[2]) Re(ch[3])Re(rx[3])+Im(ch[3])Im(ch[3])] 
	
      mmtmpD0b = vmull_s16(vrev32_s16(vmul_s16(dl_ch0_128[0],*(int16x4_t*)conj)), rxdataF128[0]);
      //mmtmpD0 = [-Im(ch[0])Re(rx[0]) Re(ch[0])Im(rx[0]) -Im(ch[1])Re(rx[1]) Re(ch[1])Im(rx[1])]
      mmtmpD1b = vmull_s16(vrev32_s16(vmul_s16(dl_ch0_128[1],*(int16x4_t*)conj)), rxdataF128[1]);
      //mmtmpD0 = [-Im(ch[2])Re(rx[2]) Re(ch[2])Im(rx[2]) -Im(ch[3])Re(rx[3]) Re(ch[3])Im(rx[3])]
      mmtmpD1 = vcombine_s32(vpadd_s32(vget_low_s32(mmtmpD0b),vget_high_s32(mmtmpD0b)),
			     vpadd_s32(vget_low_s32(mmtmpD1b),vget_high_s32(mmtmpD1b)));
      //mmtmpD1 = [-Im(ch[0])Re(rx[0])+Re(ch[0])Im(rx[0]) -Im(ch[1])Re(rx[1])+Re(ch[1])Im(rx[1]) -Im(ch[2])Re(rx[2])+Re(ch[2])Im(rx[2]) -Im(ch[3])Re(rx[3])+Re(ch[3])Im(rx[3])]
	
      mmtmpD0 = vqshlq_s32(mmtmpD0,output_shift128);
      mmtmpD1 = vqshlq_s32(mmtmpD1,output_shift128);
      rxdataF_comp0_128[0] = vcombine_s16(vmovn_s32(mmtmpD0),vmovn_s32(mmtmpD1));
	
      mmtmpD0 = vmull_s16(dl_ch0_128[2], rxdataF128[2]);
      mmtmpD1 = vmull_s16(dl_ch0_128[3], rxdataF128[3]);
      mmtmpD0 = vcombine_s32(vpadd_s32(vget_low_s32(mmtmpD0),vget_high_s32(mmtmpD0)),
			     vpadd_s32(vget_low_s32(mmtmpD1),vget_high_s32(mmtmpD1)));
      
      mmtmpD0b = vmull_s16(vrev32_s16(vmul_s16(dl_ch0_128[2],*(int16x4_t*)conj)), rxdataF128[2]);
      mmtmpD1b = vmull_s16(vrev32_s16(vmul_s16(dl_ch0_128[3],*(int16x4_t*)conj)), rxdataF128[3]);
      mmtmpD1 = vcombine_s32(vpadd_s32(vget_low_s32(mmtmpD0b),vget_high_s32(mmtmpD0b)),
			     vpadd_s32(vget_low_s32(mmtmpD1b),vget_high_s32(mmtmpD1b)));
	
      mmtmpD0 = vqshlq_s32(mmtmpD0,output_shift128);
      mmtmpD1 = vqshlq_s32(mmtmpD1,output_shift128);
      rxdataF_comp0_128[1] = vcombine_s16(vmovn_s32(mmtmpD0),vmovn_s32(mmtmpD1));
	
      // second stream
      mmtmpD0 = vmull_s16(dl_ch1_128[0], rxdataF128[0]);
      mmtmpD1 = vmull_s16(dl_ch1_128[1], rxdataF128[1]);
      mmtmpD0 = vcombine_s32(vpadd_s32(vget_low_s32(mmtmpD0),vget_high_s32(mmtmpD0)),
			     vpadd_s32(vget_low_s32(mmtmpD1),vget_high_s32(mmtmpD1)));
      mmtmpD0b = vmull_s16(vrev32_s16(vmul_s16(dl_ch0_128[0],*(int16x4_t*)conj)), rxdataF128[0]);
	
      mmtmpD1b = vmull_s16(vrev32_s16(vmul_s16(dl_ch0_128[1],*(int16x4_t*)conj)), rxdataF128[1]);
      //mmtmpD0 = [-Im(ch[2])Re(rx[2]) Re(ch[2])Im(rx[2]) -Im(ch[3])Re(rx[3]) Re(ch[3])Im(rx[3])]
      mmtmpD1 = vcombine_s32(vpadd_s32(vget_low_s32(mmtmpD0b),vget_high_s32(mmtmpD0b)),
			     vpadd_s32(vget_low_s32(mmtmpD1b),vget_high_s32(mmtmpD1b)));
      //mmtmpD1 = [-Im(ch[0])Re(rx[0])+Re(ch[0])Im(rx[0]) -Im(ch[1])Re(rx[1])+Re(ch[1])Im(rx[1]) -Im(ch[2])Re(rx[2])+Re(ch[2])Im(rx[2]) -Im(ch[3])Re(rx[3])+Re(ch[3])Im(rx[3])]
	
      mmtmpD0 = vqshlq_s32(mmtmpD0,output_shift128);
      mmtmpD1 = vqshlq_s32(mmtmpD1,output_shift128);
      rxdataF_comp1_128[0] = vcombine_s16(vmovn_s32(mmtmpD0),vmovn_s32(mmtmpD1));
	
      mmtmpD0 = vmull_s16(dl_ch1_128[2], rxdataF128[2]);
      mmtmpD1 = vmull_s16(dl_ch1_128[3], rxdataF128[3]);
      mmtmpD0 = vcombine_s32(vpadd_s32(vget_low_s32(mmtmpD0),vget_high_s32(mmtmpD0)),
			     vpadd_s32(vget_low_s32(mmtmpD1),vget_high_s32(mmtmpD1)));
	
      mmtmpD0b = vmull_s16(vrev32_s16(vmul_s16(dl_ch0_128[2],*(int16x4_t*)conj)), rxdataF128[2]);
      mmtmpD1b = vmull_s16(vrev32_s16(vmul_s16(dl_ch0_128[3],*(int16x4_t*)conj)), rxdataF128[3]);
      mmtmpD1 = vcombine_s32(vpadd_s32(vget_low_s32(mmtmpD0b),vget_high_s32(mmtmpD0b)),
			     vpadd_s32(vget_low_s32(mmtmpD1b),vget_high_s32(mmtmpD1b)));
	
      mmtmpD0 = vqshlq_s32(mmtmpD0,output_shift128);
      mmtmpD1 = vqshlq_s32(mmtmpD1,output_shift128);
      rxdataF_comp1_128[1] = vcombine_s16(vmovn_s32(mmtmpD0),vmovn_s32(mmtmpD1));
     
      if (pilots==0) {
	mmtmpD0 = vmull_s16(dl_ch0_128[4], rxdataF128[4]);
	mmtmpD1 = vmull_s16(dl_ch0_128[5], rxdataF128[5]);
	mmtmpD0 = vcombine_s32(vpadd_s32(vget_low_s32(mmtmpD0),vget_high_s32(mmtmpD0)),
			       vpadd_s32(vget_low_s32(mmtmpD1),vget_high_s32(mmtmpD1)));
	  
	mmtmpD0b = vmull_s16(vrev32_s16(vmul_s16(dl_ch0_128[4],*(int16x4_t*)conj)), rxdataF128[4]);
	mmtmpD1b = vmull_s16(vrev32_s16(vmul_s16(dl_ch0_128[5],*(int16x4_t*)conj)), rxdataF128[5]);
	mmtmpD1 = vcombine_s32(vpadd_s32(vget_low_s32(mmtmpD0b),vget_high_s32(mmtmpD0b)),
			       vpadd_s32(vget_low_s32(mmtmpD1b),vget_high_s32(mmtmpD1b)));
	  
	  
	mmtmpD0 = vqshlq_s32(mmtmpD0,output_shift128);
	mmtmpD1 = vqshlq_s32(mmtmpD1,output_shift128);
	rxdataF_comp0_128[2] = vcombine_s16(vmovn_s32(mmtmpD0),vmovn_s32(mmtmpD1));
	mmtmpD0 = vmull_s16(dl_ch1_128[4], rxdataF128[4]);
	mmtmpD1 = vmull_s16(dl_ch1_128[5], rxdataF128[5]);
	mmtmpD0 = vcombine_s32(vpadd_s32(vget_low_s32(mmtmpD0),vget_high_s32(mmtmpD0)),
			       vpadd_s32(vget_low_s32(mmtmpD1),vget_high_s32(mmtmpD1)));
	  
	mmtmpD0b = vmull_s16(vrev32_s16(vmul_s16(dl_ch1_128[4],*(int16x4_t*)conj)), rxdataF128[4]);
	mmtmpD1b = vmull_s16(vrev32_s16(vmul_s16(dl_ch1_128[5],*(int16x4_t*)conj)), rxdataF128[5]);
	mmtmpD1 = vcombine_s32(vpadd_s32(vget_low_s32(mmtmpD0b),vget_high_s32(mmtmpD0b)),
			       vpadd_s32(vget_low_s32(mmtmpD1b),vget_high_s32(mmtmpD1b)));
	  
	  
	mmtmpD0 = vqshlq_s32(mmtmpD0,output_shift128);
	mmtmpD1 = vqshlq_s32(mmtmpD1,output_shift128);
	rxdataF_comp1_128[2] = vcombine_s16(vmovn_s32(mmtmpD0),vmovn_s32(mmtmpD1));
      }
    }
      
      
      
    Nre = (pilots==0) ? 12 : 8;

    // rx_antennas
  }


  Nre = (pilots==0) ? 12 : 8;

  precoded_signal_strength0 += ((signal_energy_nodc(&dl_ch_estimates_ext[aarx][symbol*frame_parms->N_RB_DL*Nre],
                                 (nb_rb*Nre))) - (phy_measurements->n0_power[aarx]));

  precoded_signal_strength1 += ((signal_energy_nodc(&dl_ch_estimates_ext[aarx+2][symbol*frame_parms->N_RB_DL*Nre],
                                 (nb_rb*Nre))) - (phy_measurements->n0_power[aarx]));


  phy_measurements->precoded_cqi_dB[eNB_id][0] = dB_fixed2(precoded_signal_strength0,phy_measurements->n0_power_tot);
  phy_measurements->precoded_cqi_dB[eNB_id][1] = dB_fixed2(precoded_signal_strength1,phy_measurements->n0_power_tot);

#endif
}

void dlsch_dual_stream_correlation(LTE_DL_FRAME_PARMS *frame_parms,
                                   unsigned char symbol,
                                   unsigned short nb_rb,
                                   int **dl_ch_estimates_ext,
                                   int **dl_ch_estimates_ext_i,
                                   int **dl_ch_rho_ext,
                                   unsigned char output_shift)
{

#if defined(__x86_64__)||defined(__i386__)

  unsigned short rb;
  __m128i *dl_ch128,*dl_ch128i,*dl_ch_rho128,mmtmpD0,mmtmpD1,mmtmpD2,mmtmpD3;
  unsigned char aarx,symbol_mod,pilots=0;

  //    printf("dlsch_dual_stream_correlation: symbol %d\n",symbol);

  symbol_mod = (symbol>=(7-frame_parms->Ncp)) ? symbol-(7-frame_parms->Ncp) : symbol;

  if ((symbol_mod == 0) || (symbol_mod == (4-frame_parms->Ncp))) {
    pilots=1;
  }

  //  printf("Dual stream correlation (%p)\n",dl_ch_estimates_ext_i);

  for (aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++) {

    dl_ch128          = (__m128i *)&dl_ch_estimates_ext[aarx][symbol*frame_parms->N_RB_DL*12];

    if (dl_ch_estimates_ext_i == NULL) // TM3/4
      dl_ch128i         = (__m128i *)&dl_ch_estimates_ext[2+aarx][symbol*frame_parms->N_RB_DL*12];
    else
      dl_ch128i         = (__m128i *)&dl_ch_estimates_ext_i[aarx][symbol*frame_parms->N_RB_DL*12];

    dl_ch_rho128      = (__m128i *)&dl_ch_rho_ext[aarx][symbol*frame_parms->N_RB_DL*12];


    for (rb=0; rb<nb_rb; rb++) {
      // multiply by conjugated channel
      mmtmpD0 = _mm_madd_epi16(dl_ch128[0],dl_ch128i[0]);
      //      print_ints("re",&mmtmpD0);
      // mmtmpD0 contains real part of 4 consecutive outputs (32-bit)
      mmtmpD1 = _mm_shufflelo_epi16(dl_ch128[0],_MM_SHUFFLE(2,3,0,1));
      mmtmpD1 = _mm_shufflehi_epi16(mmtmpD1,_MM_SHUFFLE(2,3,0,1));
      mmtmpD1 = _mm_sign_epi16(mmtmpD1,*(__m128i*)&conjugate[0]);
      mmtmpD1 = _mm_madd_epi16(mmtmpD1,dl_ch128i[0]);
      //      print_ints("im",&mmtmpD1);
      // mmtmpD1 contains imag part of 4 consecutive outputs (32-bit)
      mmtmpD0 = _mm_srai_epi32(mmtmpD0,output_shift);
      //      print_ints("re(shift)",&mmtmpD0);
      mmtmpD1 = _mm_srai_epi32(mmtmpD1,output_shift);
      //      print_ints("im(shift)",&mmtmpD1);
      mmtmpD2 = _mm_unpacklo_epi32(mmtmpD0,mmtmpD1);
      mmtmpD3 = _mm_unpackhi_epi32(mmtmpD0,mmtmpD1);
      //      print_ints("c0",&mmtmpD2);
      //      print_ints("c1",&mmtmpD3);
      dl_ch_rho128[0] = _mm_packs_epi32(mmtmpD2,mmtmpD3);

      // multiply by conjugated channel
      mmtmpD0 = _mm_madd_epi16(dl_ch128[1],dl_ch128i[1]);
      // mmtmpD0 contains real part of 4 consecutive outputs (32-bit)
      mmtmpD1 = _mm_shufflelo_epi16(dl_ch128[1],_MM_SHUFFLE(2,3,0,1));
      mmtmpD1 = _mm_shufflehi_epi16(mmtmpD1,_MM_SHUFFLE(2,3,0,1));
      mmtmpD1 = _mm_sign_epi16(mmtmpD1,*(__m128i*)conjugate);
      mmtmpD1 = _mm_madd_epi16(mmtmpD1,dl_ch128i[1]);
      // mmtmpD1 contains imag part of 4 consecutive outputs (32-bit)
      mmtmpD0 = _mm_srai_epi32(mmtmpD0,output_shift);
      mmtmpD1 = _mm_srai_epi32(mmtmpD1,output_shift);
      mmtmpD2 = _mm_unpacklo_epi32(mmtmpD0,mmtmpD1);
      mmtmpD3 = _mm_unpackhi_epi32(mmtmpD0,mmtmpD1);
      dl_ch_rho128[1] =_mm_packs_epi32(mmtmpD2,mmtmpD3);

      if (pilots==0) {
        // multiply by conjugated channel
        mmtmpD0 = _mm_madd_epi16(dl_ch128[2],dl_ch128i[2]);
        // mmtmpD0 contains real part of 4 consecutive outputs (32-bit)
        mmtmpD1 = _mm_shufflelo_epi16(dl_ch128[2],_MM_SHUFFLE(2,3,0,1));
        mmtmpD1 = _mm_shufflehi_epi16(mmtmpD1,_MM_SHUFFLE(2,3,0,1));
        mmtmpD1 = _mm_sign_epi16(mmtmpD1,*(__m128i*)conjugate);
        mmtmpD1 = _mm_madd_epi16(mmtmpD1,dl_ch128i[2]);
        // mmtmpD1 contains imag part of 4 consecutive outputs (32-bit)
        mmtmpD0 = _mm_srai_epi32(mmtmpD0,output_shift);
        mmtmpD1 = _mm_srai_epi32(mmtmpD1,output_shift);
        mmtmpD2 = _mm_unpacklo_epi32(mmtmpD0,mmtmpD1);
        mmtmpD3 = _mm_unpackhi_epi32(mmtmpD0,mmtmpD1);
        dl_ch_rho128[2] = _mm_packs_epi32(mmtmpD2,mmtmpD3);

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

  _mm_empty();
  _m_empty();

#elif defined(__arm__)

#endif
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
                         unsigned char dual_stream_UE)
{

#if defined(__x86_64__)||defined(__i386__)

  unsigned char aatx;
  int i;
  __m128i *rxdataF_comp128_0,*rxdataF_comp128_1,*rxdataF_comp128_i0,*rxdataF_comp128_i1,*dl_ch_mag128_0,*dl_ch_mag128_1,*dl_ch_mag128_0b,*dl_ch_mag128_1b,*rho128_0,*rho128_1,*rho128_i0,*rho128_i1,
    *dl_ch_mag128_i0,*dl_ch_mag128_i1,*dl_ch_mag128_i0b,*dl_ch_mag128_i1b;

  if (frame_parms->nb_antennas_rx>1) {

    for (aatx=0; aatx<frame_parms->nb_antennas_tx_eNB; aatx++) {

      rxdataF_comp128_0   = (__m128i *)&rxdataF_comp[(aatx<<1)][symbol*frame_parms->N_RB_DL*12];
      rxdataF_comp128_1   = (__m128i *)&rxdataF_comp[(aatx<<1)+1][symbol*frame_parms->N_RB_DL*12];
      dl_ch_mag128_0      = (__m128i *)&dl_ch_mag[(aatx<<1)][symbol*frame_parms->N_RB_DL*12];
      dl_ch_mag128_1      = (__m128i *)&dl_ch_mag[(aatx<<1)+1][symbol*frame_parms->N_RB_DL*12];
      dl_ch_mag128_0b     = (__m128i *)&dl_ch_magb[(aatx<<1)][symbol*frame_parms->N_RB_DL*12];
      dl_ch_mag128_1b     = (__m128i *)&dl_ch_magb[(aatx<<1)+1][symbol*frame_parms->N_RB_DL*12];

      // MRC on each re of rb, both on MF output and magnitude (for 16QAM/64QAM llr computation)
      for (i=0; i<nb_rb*3; i++) {
        rxdataF_comp128_0[i] = _mm_adds_epi16(_mm_srai_epi16(rxdataF_comp128_0[i],1),_mm_srai_epi16(rxdataF_comp128_1[i],1));
        dl_ch_mag128_0[i]    = _mm_adds_epi16(_mm_srai_epi16(dl_ch_mag128_0[i],1),_mm_srai_epi16(dl_ch_mag128_1[i],1));
        dl_ch_mag128_0b[i]   = _mm_adds_epi16(_mm_srai_epi16(dl_ch_mag128_0b[i],1),_mm_srai_epi16(dl_ch_mag128_1b[i],1));
      }
    }

    if (rho) {
      rho128_0 = (__m128i *) &rho[0][symbol*frame_parms->N_RB_DL*12];
      rho128_1 = (__m128i *) &rho[1][symbol*frame_parms->N_RB_DL*12];

      for (i=0; i<nb_rb*3; i++) {
        //  print_shorts("mrc rho0:",&rho128_0[i]);
        //  print_shorts("mrc rho1:",&rho128_1[i]);
        rho128_0[i] = _mm_adds_epi16(_mm_srai_epi16(rho128_0[i],1),_mm_srai_epi16(rho128_1[i],1));
      }
    }


    if (dual_stream_UE == 1) {
      rho128_i0 = (__m128i *) &rho_i[0][symbol*frame_parms->N_RB_DL*12];
      rho128_i1 = (__m128i *) &rho_i[1][symbol*frame_parms->N_RB_DL*12];
      rxdataF_comp128_i0   = (__m128i *)&rxdataF_comp_i[0][symbol*frame_parms->N_RB_DL*12];
      rxdataF_comp128_i1   = (__m128i *)&rxdataF_comp_i[1][symbol*frame_parms->N_RB_DL*12];
      dl_ch_mag128_i0      = (__m128i *)&dl_ch_mag_i[0][symbol*frame_parms->N_RB_DL*12];
      dl_ch_mag128_i1      = (__m128i *)&dl_ch_mag_i[1][symbol*frame_parms->N_RB_DL*12];
      dl_ch_mag128_i0b     = (__m128i *)&dl_ch_magb_i[0][symbol*frame_parms->N_RB_DL*12];
      dl_ch_mag128_i1b     = (__m128i *)&dl_ch_magb_i[1][symbol*frame_parms->N_RB_DL*12];

      for (i=0; i<nb_rb*3; i++) {
        rxdataF_comp128_i0[i] = _mm_adds_epi16(_mm_srai_epi16(rxdataF_comp128_i0[i],1),_mm_srai_epi16(rxdataF_comp128_i1[i],1));
        rho128_i0[i]           = _mm_adds_epi16(_mm_srai_epi16(rho128_i0[i],1),_mm_srai_epi16(rho128_i1[i],1));

        dl_ch_mag128_i0[i]    = _mm_adds_epi16(_mm_srai_epi16(dl_ch_mag128_i0[i],1),_mm_srai_epi16(dl_ch_mag128_i1[i],1));
        dl_ch_mag128_i0b[i]    = _mm_adds_epi16(_mm_srai_epi16(dl_ch_mag128_i0b[i],1),_mm_srai_epi16(dl_ch_mag128_i1b[i],1));
      }
    }
  }

  _mm_empty();
  _m_empty();

#elif defined(__arm__)

  unsigned char aatx;
  int i;
  int16x8_t *rxdataF_comp128_0,*rxdataF_comp128_1,*rxdataF_comp128_i0,*rxdataF_comp128_i1,*dl_ch_mag128_0,*dl_ch_mag128_1,*dl_ch_mag128_0b,*dl_ch_mag128_1b,*rho128_0,*rho128_1,*rho128_i0,*rho128_i1,*dl_ch_mag128_i0,*dl_ch_mag128_i1,*dl_ch_mag128_i0b,*dl_ch_mag128_i1b;

  if (frame_parms->nb_antennas_rx>1) {

    for (aatx=0; aatx<frame_parms->nb_antennas_tx_eNB; aatx++) {

      rxdataF_comp128_0   = (int16x8_t *)&rxdataF_comp[(aatx<<1)][symbol*frame_parms->N_RB_DL*12];
      rxdataF_comp128_1   = (int16x8_t *)&rxdataF_comp[(aatx<<1)+1][symbol*frame_parms->N_RB_DL*12];
      dl_ch_mag128_0      = (int16x8_t *)&dl_ch_mag[(aatx<<1)][symbol*frame_parms->N_RB_DL*12];
      dl_ch_mag128_1      = (int16x8_t *)&dl_ch_mag[(aatx<<1)+1][symbol*frame_parms->N_RB_DL*12];
      dl_ch_mag128_0b     = (int16x8_t *)&dl_ch_magb[(aatx<<1)][symbol*frame_parms->N_RB_DL*12];
      dl_ch_mag128_1b     = (int16x8_t *)&dl_ch_magb[(aatx<<1)+1][symbol*frame_parms->N_RB_DL*12];

      // MRC on each re of rb, both on MF output and magnitude (for 16QAM/64QAM llr computation)
      for (i=0; i<nb_rb*3; i++) {
        rxdataF_comp128_0[i] = vhaddq_s16(rxdataF_comp128_0[i],rxdataF_comp128_1[i]);
        dl_ch_mag128_0[i]    = vhaddq_s16(dl_ch_mag128_0[i],dl_ch_mag128_1[i]);
        dl_ch_mag128_0b[i]   = vhaddq_s16(dl_ch_mag128_0b[i],dl_ch_mag128_1b[i]);
      }
    }

    if (rho) {
      rho128_0 = (int16x8_t *) &rho[0][symbol*frame_parms->N_RB_DL*12];
      rho128_1 = (int16x8_t *) &rho[1][symbol*frame_parms->N_RB_DL*12];

      for (i=0; i<nb_rb*3; i++) {
        //  print_shorts("mrc rho0:",&rho128_0[i]);
        //  print_shorts("mrc rho1:",&rho128_1[i]);
        rho128_0[i] = vhaddq_s16(rho128_0[i],rho128_1[i]);
      }
    }


    if (dual_stream_UE == 1) {
      rho128_i0 = (int16x8_t *) &rho_i[0][symbol*frame_parms->N_RB_DL*12];
      rho128_i1 = (int16x8_t *) &rho_i[1][symbol*frame_parms->N_RB_DL*12];
      rxdataF_comp128_i0   = (int16x8_t *)&rxdataF_comp_i[0][symbol*frame_parms->N_RB_DL*12];
      rxdataF_comp128_i1   = (int16x8_t *)&rxdataF_comp_i[1][symbol*frame_parms->N_RB_DL*12];

      dl_ch_mag128_i0      = (int16x8_t *)&dl_ch_mag_i[0][symbol*frame_parms->N_RB_DL*12];
      dl_ch_mag128_i1      = (int16x8_t *)&dl_ch_mag_i[1][symbol*frame_parms->N_RB_DL*12];
      dl_ch_mag128_i0b     = (int16x8_t *)&dl_ch_magb_i[0][symbol*frame_parms->N_RB_DL*12];
      dl_ch_mag128_i1b     = (int16x8_t *)&dl_ch_magb_i[1][symbol*frame_parms->N_RB_DL*12];

      for (i=0; i<nb_rb*3; i++) {
        rxdataF_comp128_i0[i] = vhaddq_s16(rxdataF_comp128_i0[i],rxdataF_comp128_i1[i]);
        rho128_i0[i]          = vhaddq_s16(rho128_i0[i],rho128_i1[i]);

        dl_ch_mag128_i0[i]    = vhaddq_s16(dl_ch_mag128_i0[i],dl_ch_mag128_i1[i]);
        dl_ch_mag128_i0b[i]   = vhaddq_s16(dl_ch_mag128_i0b[i],dl_ch_mag128_i1b[i]);
      }
    }
  }

#endif
}

void dlsch_scale_channel(int **dl_ch_estimates_ext,
                         LTE_DL_FRAME_PARMS *frame_parms,
                         LTE_UE_DLSCH_t **dlsch_ue,
                         uint8_t symbol,
                         unsigned short nb_rb)
{

#if defined(__x86_64__)||defined(__i386__)

  short rb, ch_amp;
  unsigned char aatx,aarx,pilots=0,symbol_mod;
  __m128i *dl_ch128, ch_amp128;

  symbol_mod = (symbol>=(7-frame_parms->Ncp)) ? symbol-(7-frame_parms->Ncp) : symbol;

  if ((symbol_mod == 0) || (symbol_mod == (4-frame_parms->Ncp))) {
    if (frame_parms->mode1_flag==1) // 10 out of 12 so don't reduce size
      nb_rb=1+(5*nb_rb/6);
    else
      pilots=1;
  }

  // Determine scaling amplitude based the symbol
  ch_amp = ((pilots) ? (dlsch_ue[0]->sqrt_rho_b) : (dlsch_ue[0]->sqrt_rho_a));

  //  LOG_D(PHY,"Scaling PDSCH Chest in OFDM symbol %d by %d\n",symbol_mod,ch_amp);

  ch_amp128 = _mm_set1_epi16(ch_amp); // Q3.13

  for (aatx=0; aatx<frame_parms->nb_antennas_tx_eNB; aatx++) {
    for (aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++) {

      dl_ch128=(__m128i *)&dl_ch_estimates_ext[(aatx<<1)+aarx][symbol*frame_parms->N_RB_DL*12];

      for (rb=0; rb<nb_rb; rb++) {
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
void dlsch_channel_level(int **dl_ch_estimates_ext,
                         LTE_DL_FRAME_PARMS *frame_parms,
                         int *avg,
                         uint8_t symbol,
                         unsigned short nb_rb)
{

#if defined(__x86_64__)||defined(__i386__)

  short rb;
  unsigned char aatx,aarx,nre=12,symbol_mod;
  __m128i *dl_ch128,avg128D;

  symbol_mod = (symbol>=(7-frame_parms->Ncp)) ? symbol-(7-frame_parms->Ncp) : symbol;

  for (aatx=0; aatx<frame_parms->nb_antennas_tx_eNB; aatx++)
    for (aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++) {
      //clear average level
      avg128D = _mm_setzero_si128();
      // 5 is always a symbol with no pilots for both normal and extended prefix

      dl_ch128=(__m128i *)&dl_ch_estimates_ext[(aatx<<1)+aarx][symbol*frame_parms->N_RB_DL*12];

      for (rb=0; rb<nb_rb; rb++) {
        //  printf("rb %d : ",rb);
        //  print_shorts("ch",&dl_ch128[0]);
        avg128D = _mm_add_epi32(avg128D,_mm_madd_epi16(dl_ch128[0],dl_ch128[0]));
        avg128D = _mm_add_epi32(avg128D,_mm_madd_epi16(dl_ch128[1],dl_ch128[1]));

        if (((symbol_mod == 0) || (symbol_mod == (frame_parms->Ncp-1)))&&(frame_parms->mode1_flag==0)) {
          dl_ch128+=2;
        } else {
          avg128D = _mm_add_epi32(avg128D,_mm_madd_epi16(dl_ch128[2],dl_ch128[2]));
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

      if (((symbol_mod == 0) || (symbol_mod == (frame_parms->Ncp-1)))&&(frame_parms->mode1_flag==0))
        nre=8;
      else if (((symbol_mod == 0) || (symbol_mod == (frame_parms->Ncp-1)))&&(frame_parms->mode1_flag==1))
        nre=10;
      else
        nre=12;

      avg[(aatx<<1)+aarx] = (((int*)&avg128D)[0] +
                             ((int*)&avg128D)[1] +
                             ((int*)&avg128D)[2] +
                             ((int*)&avg128D)[3])/(nb_rb*nre);

      //            printf("Channel level : %d\n",avg[(aatx<<1)+aarx]);
    }

  _mm_empty();
  _m_empty();

#elif defined(__arm__)

  short rb;
  unsigned char aatx,aarx,nre=12,symbol_mod;
  int32x4_t avg128D;
  int16x4_t *dl_ch128;

  symbol_mod = (symbol>=(7-frame_parms->Ncp)) ? symbol-(7-frame_parms->Ncp) : symbol;

  for (aatx=0; aatx<frame_parms->nb_antennas_tx_eNB; aatx++)
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

        if (((symbol_mod == 0) || (symbol_mod == (frame_parms->Ncp-1)))&&(frame_parms->mode1_flag==0)) {
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

      if (((symbol_mod == 0) || (symbol_mod == (frame_parms->Ncp-1)))&&(frame_parms->mode1_flag==0))
        nre=8;
      else if (((symbol_mod == 0) || (symbol_mod == (frame_parms->Ncp-1)))&&(frame_parms->mode1_flag==1))
        nre=10;
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

//compute average channel_level of effective (precoded) channel
void dlsch_channel_level_TM3(int **dl_ch_estimates_ext,
                             LTE_DL_FRAME_PARMS *frame_parms,
                             int *avg,
                             uint8_t symbol,
                             unsigned short nb_rb)
{
#if defined(__x86_64__)||defined(__i386__)

  short rb;
  unsigned char aarx,nre=12,symbol_mod;
  __m128i *dl_ch0_128,*dl_ch1_128, dl_ch0_128_tmp, dl_ch1_128_tmp,avg128D;

  symbol_mod = (symbol>=(7-frame_parms->Ncp)) ? symbol-(7-frame_parms->Ncp) : symbol;

  //clear average level
  avg128D = _mm_setzero_si128();
  avg[0] = 0;
  avg[1] = 0;
  // 5 is always a symbol with no pilots for both normal and extended prefix

  if (((symbol_mod == 0) || (symbol_mod == (frame_parms->Ncp-1)))&&(frame_parms->mode1_flag==0))
    nre=8;
  else if (((symbol_mod == 0) || (symbol_mod == (frame_parms->Ncp-1)))&&(frame_parms->mode1_flag==1))
    nre=10;
  else
    nre=12;

  for (aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++) {
    dl_ch0_128 = (__m128i *)&dl_ch_estimates_ext[aarx][symbol*frame_parms->N_RB_DL*12];
    dl_ch1_128 = (__m128i *)&dl_ch_estimates_ext[2+aarx][symbol*frame_parms->N_RB_DL*12];

    for (rb=0; rb<nb_rb; rb++) {

      dl_ch0_128_tmp = _mm_load_si128(&dl_ch0_128[0]);
      dl_ch1_128_tmp = _mm_load_si128(&dl_ch1_128[0]);

      prec2A_TM3_128(&dl_ch0_128_tmp,&dl_ch1_128_tmp);
      //      mmtmpD0 = _mm_madd_epi16(dl_ch0_128_tmp,dl_ch0_128_tmp);
      avg128D = _mm_add_epi32(avg128D,_mm_madd_epi16(dl_ch0_128_tmp,dl_ch0_128_tmp));

      dl_ch0_128_tmp = _mm_load_si128(&dl_ch0_128[1]);
      dl_ch1_128_tmp = _mm_load_si128(&dl_ch1_128[1]);

      prec2A_TM3_128(&dl_ch0_128_tmp,&dl_ch1_128_tmp);
      //      mmtmpD1 = _mm_madd_epi16(dl_ch0_128_tmp,dl_ch0_128_tmp);
      avg128D = _mm_add_epi32(avg128D,_mm_madd_epi16(dl_ch0_128_tmp,dl_ch0_128_tmp));

      if (((symbol_mod == 0) || (symbol_mod == (frame_parms->Ncp-1)))&&(frame_parms->mode1_flag==0)) {
        dl_ch0_128+=2;
        dl_ch1_128+=2;
      } else {
        dl_ch0_128_tmp = _mm_load_si128(&dl_ch0_128[2]);
        dl_ch1_128_tmp = _mm_load_si128(&dl_ch1_128[2]);

        prec2A_TM3_128(&dl_ch0_128_tmp,&dl_ch1_128_tmp);
        //  mmtmpD2 = _mm_madd_epi16(dl_ch0_128_tmp,dl_ch0_128_tmp);
        avg128D = _mm_add_epi32(avg128D,_mm_madd_epi16(dl_ch0_128_tmp,dl_ch0_128_tmp));

        dl_ch0_128+=3;
        dl_ch1_128+=3;
      }
    }

    avg[aarx] = (((int*)&avg128D)[0])/(nb_rb*nre) +
      (((int*)&avg128D)[1])/(nb_rb*nre) +
      (((int*)&avg128D)[2])/(nb_rb*nre) +
      (((int*)&avg128D)[3])/(nb_rb*nre);
  }

  // choose maximum of the 2 effective channels
  avg[0] = cmax(avg[0],avg[1]);

  _mm_empty();
  _m_empty();

#elif defined(__arm__)

#endif
}

//compute average channel_level of effective (precoded) channel
void dlsch_channel_level_TM56(int **dl_ch_estimates_ext,
                              LTE_DL_FRAME_PARMS *frame_parms,
                              unsigned char *pmi_ext,
                              int *avg,
                              uint8_t symbol,
                              unsigned short nb_rb)
{

#if defined(__x86_64__)||defined(__i386__)

  short rb;
  unsigned char aarx,nre=12,symbol_mod;
  __m128i *dl_ch0_128,*dl_ch1_128, dl_ch0_128_tmp, dl_ch1_128_tmp,avg128D;

  symbol_mod = (symbol>=(7-frame_parms->Ncp)) ? symbol-(7-frame_parms->Ncp) : symbol;

  //clear average level
  avg128D = _mm_setzero_si128();
  avg[0] = 0;
  avg[1] = 0;
  // 5 is always a symbol with no pilots for both normal and extended prefix

  if (((symbol_mod == 0) || (symbol_mod == (frame_parms->Ncp-1)))&&(frame_parms->mode1_flag==0))
    nre=8;
  else if (((symbol_mod == 0) || (symbol_mod == (frame_parms->Ncp-1)))&&(frame_parms->mode1_flag==1))
    nre=10;
  else
    nre=12;

  for (aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++) {
    dl_ch0_128 = (__m128i *)&dl_ch_estimates_ext[aarx][symbol*frame_parms->N_RB_DL*12];
    dl_ch1_128 = (__m128i *)&dl_ch_estimates_ext[2+aarx][symbol*frame_parms->N_RB_DL*12];

    for (rb=0; rb<nb_rb; rb++) {

      dl_ch0_128_tmp = _mm_load_si128(&dl_ch0_128[0]);
      dl_ch1_128_tmp = _mm_load_si128(&dl_ch1_128[0]);

      prec2A_TM56_128(pmi_ext[rb],&dl_ch0_128_tmp,&dl_ch1_128_tmp);
      //      mmtmpD0 = _mm_madd_epi16(dl_ch0_128_tmp,dl_ch0_128_tmp);
      avg128D = _mm_add_epi32(avg128D,_mm_madd_epi16(dl_ch0_128_tmp,dl_ch0_128_tmp));

      dl_ch0_128_tmp = _mm_load_si128(&dl_ch0_128[1]);
      dl_ch1_128_tmp = _mm_load_si128(&dl_ch1_128[1]);

      prec2A_TM56_128(pmi_ext[rb],&dl_ch0_128_tmp,&dl_ch1_128_tmp);
      //      mmtmpD1 = _mm_madd_epi16(dl_ch0_128_tmp,dl_ch0_128_tmp);
      avg128D = _mm_add_epi32(avg128D,_mm_madd_epi16(dl_ch0_128_tmp,dl_ch0_128_tmp));

      if (((symbol_mod == 0) || (symbol_mod == (frame_parms->Ncp-1)))&&(frame_parms->mode1_flag==0)) {
        dl_ch0_128+=2;
        dl_ch1_128+=2;
      } else {
        dl_ch0_128_tmp = _mm_load_si128(&dl_ch0_128[2]);
        dl_ch1_128_tmp = _mm_load_si128(&dl_ch1_128[2]);

        prec2A_TM56_128(pmi_ext[rb],&dl_ch0_128_tmp,&dl_ch1_128_tmp);
        //  mmtmpD2 = _mm_madd_epi16(dl_ch0_128_tmp,dl_ch0_128_tmp);
        avg128D = _mm_add_epi32(avg128D,_mm_madd_epi16(dl_ch0_128_tmp,dl_ch0_128_tmp));

        dl_ch0_128+=3;
        dl_ch1_128+=3;
      }
    }

    avg[aarx] = (((int*)&avg128D)[0])/(nb_rb*nre) +
      (((int*)&avg128D)[1])/(nb_rb*nre) +
      (((int*)&avg128D)[2])/(nb_rb*nre) +
      (((int*)&avg128D)[3])/(nb_rb*nre);
  }

  // choose maximum of the 2 effective channels
  avg[0] = cmax(avg[0],avg[1]);

  _mm_empty();
  _m_empty();

#elif defined(__arm__)


#endif
}


void dlsch_alamouti(LTE_DL_FRAME_PARMS *frame_parms,
                    int **rxdataF_comp,
                    int **dl_ch_mag,
                    int **dl_ch_magb,
                    unsigned char symbol,
                    unsigned short nb_rb)
{

#if defined(__x86_64__)||defined(__i386__)

  short *rxF0,*rxF1;
  __m128i *ch_mag0,*ch_mag1,*ch_mag0b,*ch_mag1b, amp, *rxF0_128;
  unsigned char rb,re;
  int jj = (symbol*frame_parms->N_RB_DL*12);
  uint8_t symbol_mod = (symbol>=(7-frame_parms->Ncp)) ? symbol-(7-frame_parms->Ncp) : symbol;
  uint8_t pilots = ((symbol_mod==0)||(symbol_mod==(4-frame_parms->Ncp))) ? 1 : 0;
  rxF0_128 = (__m128i*) &rxdataF_comp[0][jj];

  amp = _mm_set1_epi16(ONE_OVER_SQRT2_Q15);

  //    printf("Doing alamouti!\n");
  rxF0     = (short*)&rxdataF_comp[0][jj];  //tx antenna 0  h0*y
  rxF1     = (short*)&rxdataF_comp[2][jj];  //tx antenna 1  h1*y
  ch_mag0 = (__m128i *)&dl_ch_mag[0][jj];
  ch_mag1 = (__m128i *)&dl_ch_mag[2][jj];
  ch_mag0b = (__m128i *)&dl_ch_magb[0][jj];
  ch_mag1b = (__m128i *)&dl_ch_magb[2][jj];
 
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
    ch_mag0[0] = _mm_adds_epi16(ch_mag0[0],ch_mag1[0]);
    ch_mag0[1] = _mm_adds_epi16(ch_mag0[1],ch_mag1[1]);

    ch_mag0b[0] = _mm_adds_epi16(ch_mag0b[0],ch_mag1b[0]);
    ch_mag0b[1] = _mm_adds_epi16(ch_mag0b[1],ch_mag1b[1]);

    // account for 1/sqrt(2) scaling at transmission
    ch_mag0[0] = _mm_srai_epi16(ch_mag0[0],1);
    ch_mag0[1] = _mm_srai_epi16(ch_mag0[1],1);
    ch_mag0b[0] = _mm_srai_epi16(ch_mag0b[0],1);
    ch_mag0b[1] = _mm_srai_epi16(ch_mag0b[1],1);

    rxF0_128[0] = _mm_mulhi_epi16(rxF0_128[0],amp);
    rxF0_128[0] = _mm_slli_epi16(rxF0_128[0],1);
    rxF0_128[1] = _mm_mulhi_epi16(rxF0_128[1],amp);
    rxF0_128[1] = _mm_slli_epi16(rxF0_128[1],1);

    if (pilots==0) {
      ch_mag0[2] = _mm_adds_epi16(ch_mag0[2],ch_mag1[2]);
      ch_mag0b[2] = _mm_adds_epi16(ch_mag0b[2],ch_mag1b[2]);

      ch_mag0[2] = _mm_srai_epi16(ch_mag0[2],1);
      ch_mag0b[2] = _mm_srai_epi16(ch_mag0b[2],1);

      rxF0_128[2] = _mm_mulhi_epi16(rxF0_128[2],amp);
      rxF0_128[2] = _mm_slli_epi16(rxF0_128[2],1);

      ch_mag0+=3;
      ch_mag1+=3;
      ch_mag0b+=3;
      ch_mag1b+=3;
      rxF0_128+=3;
    } else {
      ch_mag0+=2;
      ch_mag1+=2;
      ch_mag0b+=2;
      ch_mag1b+=2;
      rxF0_128+=2;
    }
  }

  _mm_empty();
  _m_empty();

#elif defined(__arm__)

#endif
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
                                        LTE_DL_FRAME_PARMS *frame_parms)
{


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
          *pmi_ext = (pmi>>((rb>>2)<<1))&3;
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

      }
    else {  // Odd number of RBs
      for (rb=0; rb<frame_parms->N_RB_DL>>1; rb++) {
#ifdef DEBUG_DLSCH_DEMOD
	printf("dlch_ext %d\n",dl_ch0_ext-&dl_ch_estimates_ext[aarx][0]);
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
	    //	    printf("Extracting w/o pilots (symbol %d, rb %d, skip_half %d)\n",l,rb,skip_half);
            if (skip_half==1) {
              memcpy(dl_ch0_ext,dl_ch0,6*sizeof(int));

              for (i=0; i<6; i++) {
	        rxF_ext[i]=rxF[i];
#ifdef DEBUG_DLSCH_DEMOD
		printf("extract rb %d, re %d => (%d,%d)\n",rb,i,*(short *)&rxF_ext[i],*(1+(short*)&rxF_ext[i]));
#endif
	      }
              dl_ch0_ext+=6;
              rxF_ext+=6;
            } else if (skip_half==2) {
              memcpy(dl_ch0_ext,dl_ch0+6,6*sizeof(int));

              for (i=0; i<6; i++) {
                rxF_ext[i]=rxF[(i+6)];
#ifdef DEBUG_DLSCH_DEMOD
		printf("extract rb %d, re %d => (%d,%d)\n",rb,i,*(short *)&rxF_ext[i],*(1+(short*)&rxF_ext[i]));
#endif
	      }
              dl_ch0_ext+=6;
              rxF_ext+=6;
            } else {
              memcpy(dl_ch0_ext,dl_ch0,12*sizeof(int));

              for (i=0; i<12; i++) {
                rxF_ext[i]=rxF[i];
#ifdef DEBUG_DLSCH_DEMOD
                printf("extract rb %d, re %d => (%d,%d)\n",rb,i,*(short *)&rxF_ext[i],*(1+(short*)&rxF_ext[i]));
#endif
	      }
              dl_ch0_ext+=12;
              rxF_ext+=12;
            }
          } else {
	    //	    printf("Extracting with pilots (symbol %d, rb %d, skip_half %d)\n",l,rb,skip_half);
            j=0;

            if (skip_half==1) {
              for (i=0; i<6; i++) {
                if (i!=((frame_parms->nushift+poffset)%6)) {
                  rxF_ext[j]=rxF[i];
#ifdef DEBUG_DLSCH_DEMOD
                  printf("extract rb %d, re %d => (%d,%d)\n",rb,i,*(short *)&rxF_ext[j],*(1+(short*)&rxF_ext[j]));
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
		  printf("extract rb %d, re %d => (%d,%d)\n",rb,i,*(short *)&rxF_ext[j],*(1+(short*)&rxF_ext[j]));
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
		  printf("extract rb %d, re %d => (%d,%d)\n",rb,i,*(short *)&rxF_ext[j],*(1+(short*)&rxF_ext[j]));
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
              printf("**extract rb %d, re %d => (%d,%d)\n",rb,i,*(short *)&rxF_ext[j-1],*(1+(short*)&rxF_ext[j-1]));
#endif
            }
          }

          rxF       = &rxdataF[aarx][((symbol*(frame_parms->ofdm_symbol_size)))];

          for (; i<12; i++) {
            if (i!=((frame_parms->nushift+6+poffset)%12)) {
              dl_ch0_ext[j]=dl_ch0[i];
              rxF_ext[j++]=rxF[(1+i-6)];
#ifdef DEBUG_DLSCH_DEMOD
              printf("**extract rb %d, re %d => (%d,%d)\n",rb,i,*(short *)&rxF_ext[j-1],*(1+(short*)&rxF_ext[j-1]));
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
	    //	    printf("Extracting w/o pilots (symbol %d, rb %d, skip_half %d)\n",l,rb,skip_half);
            if (skip_half==1) {
              memcpy(dl_ch0_ext,dl_ch0,6*sizeof(int));

              for (i=0; i<6; i++) {
                rxF_ext[i]=rxF[i];
#ifdef DEBUG_DLSCH_DEMOD	 
		printf("extract rb %d, re %d => (%d,%d)\n",rb,i,*(short *)&rxF_ext[i],*(1+(short*)&rxF_ext[i]));
#endif
	      }
              dl_ch0_ext+=6;
              rxF_ext+=6;

            } else if (skip_half==2) {
              memcpy(dl_ch0_ext,dl_ch0+6,6*sizeof(int));

              for (i=0; i<6; i++) {
                rxF_ext[i]=rxF[(i+6)];
#ifdef DEBUG_DLSCH_DEMOD
		printf("extract rb %d, re %d => (%d,%d)\n",rb,i,*(short *)&rxF_ext[i],*(1+(short*)&rxF_ext[i]));
#endif
	      }
              dl_ch0_ext+=6;
              rxF_ext+=6;

            } else {
              memcpy(dl_ch0_ext,dl_ch0,12*sizeof(int));

              for (i=0; i<12; i++) {
                rxF_ext[i]=rxF[i];
#ifdef DEBUG_DLSCH_DEMOD
		printf("extract rb %d, re %d => (%d,%d)\n",rb,i,*(short *)&rxF_ext[i],*(1+(short*)&rxF_ext[i]));
#endif
	      }
              dl_ch0_ext+=12;
              rxF_ext+=12;
            }
          } else {
	    //	    printf("Extracting with pilots (symbol %d, rb %d, skip_half %d)\n",l,rb,skip_half);
            j=0;

            if (skip_half==1) {
              for (i=0; i<6; i++) {
                if (i!=((frame_parms->nushift+poffset)%6)) {
                  rxF_ext[j]=rxF[i];
#ifdef DEBUG_DLSCH_DEMOD
		  printf("extract rb %d, re %d => (%d,%d)\n",rb,i,*(short *)&rxF_ext[j],*(1+(short*)&rxF_ext[j]));
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
		  printf("extract rb %d, re %d => (%d,%d)\n",rb,i,*(short *)&rxF_ext[j],*(1+(short*)&rxF_ext[j]));
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
		  printf("extract rb %d, re %d => (%d,%d)\n",rb,i,*(short *)&rxF_ext[j],*(1+(short*)&rxF_ext[j]));
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
                                      LTE_DL_FRAME_PARMS *frame_parms)
{


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
	  //	printf("symbol %d / rb %d: skipping PBCH REs\n",symbol,prb);
	}
	
	//SSS
	
	if (((subframe==0)||(subframe==5)) &&
	    (prb>=((frame_parms->N_RB_DL>>1)-3)) &&
	    (prb<((frame_parms->N_RB_DL>>1)+3)) &&
	    (l==sss_symb) ) {
	  rb_alloc_ind = 0;
	  //	printf("symbol %d / rb %d: skipping SSS REs\n",symbol,prb);
	}
	
	
	
	//PSS in subframe 0/5 if FDD
	if (frame_parms->frame_type == FDD) {  //FDD
	  if (((subframe==0)||(subframe==5)) && 
	      (prb>=((frame_parms->N_RB_DL>>1)-3)) && 
	      (prb<((frame_parms->N_RB_DL>>1)+3)) && 
	      (l==pss_symb) ) {
	    rb_alloc_ind = 0;
	    //	  printf("symbol %d / rb %d: skipping PSS REs\n",symbol,prb);
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
	  if (prb<(frame_parms->N_RB_DL>>1)){
	    rxF      = &rxdataF[aarx][prb_off+
				      frame_parms->first_carrier_offset + 
				      (symbol*(frame_parms->ofdm_symbol_size))];
	  }
	  else {
	    rxF      = &rxdataF[aarx][prb_off2+
				      (symbol*(frame_parms->ofdm_symbol_size))];
	  }
          *pmi_loc = (pmi>>((prb>>2)<<1))&3;
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
	  //	printf("symbol %d / rb %d: skipping PBCH REs\n",symbol,prb);
	}
	
	//SSS
	
	if (((subframe==0)||(subframe==5)) &&
	    (prb>((frame_parms->N_RB_DL>>1)-3)) &&
	    (prb<((frame_parms->N_RB_DL>>1)+3)) &&
	    (l==sss_symb) ) {
	  rb_alloc_ind = 0;
	  //	printf("symbol %d / rb %d: skipping SSS REs\n",symbol,prb);
	}
	
	
	
	//PSS in subframe 0/5 if FDD
	if (frame_parms->frame_type == FDD) {  //FDD
	  if (((subframe==0)||(subframe==5)) && 
	      (prb>((frame_parms->N_RB_DL>>1)-3)) && 
	      (prb<((frame_parms->N_RB_DL>>1)+3)) && 
	      (l==pss_symb) ) {
	    rb_alloc_ind = 0;
	    //	  printf("symbol %d / rb %d: skipping PSS REs\n",symbol,prb);
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
	  
	  if (prb<=(frame_parms->N_RB_DL>>1)){
	    rxF      = &rxdataF[aarx][prb_off+
				      frame_parms->first_carrier_offset + 
				      (symbol*(frame_parms->ofdm_symbol_size))];
	  }
	  else {
	    rxF      = &rxdataF[aarx][prb_off2+ 
				      (symbol*(frame_parms->ofdm_symbol_size))];
	  }
#ifdef DEBUG_DLSCH_DEMOD
	  printf("symbol %d / rb %d: alloc %d skip_half %d (rxF %p, rxF_ext %p) prb_off (%d,%d)\n",symbol,prb,rb_alloc_ind,skip_half,rxF,rxF_ext,prb_off,prb_off2);
#endif
          *pmi_loc = (pmi>>((prb>>2)<<1))&3;
          //    printf("symbol_mod %d (pilots %d) rb %d, sb %d, pmi %d (pmi_loc %p,rxF %p, ch00 %p, ch01 %p, rxF_ext %p dl_ch0_ext %p dl_ch1_ext %p)\n",symbol_mod,pilots,rb,rb>>2,*pmi_loc,pmi_loc,rxF,dl_ch0, dl_ch1, rxF_ext,dl_ch0_ext,dl_ch1_ext);

          pmi_loc++;

	  if (prb != (frame_parms->N_RB_DL>>1)) { // This PRB is not around DC
	    if (pilots==0) {
	      if (skip_half==1) {
		memcpy(dl_ch0_ext,dl_ch0p,6*sizeof(int32_t));
		memcpy(dl_ch1_ext,dl_ch1p,6*sizeof(int32_t));
		memcpy(rxF_ext,rxF,6*sizeof(int32_t));
#ifdef DEBUG_DLSCH_DEMOD
		for (i=0;i<6;i++)
		  printf("extract rb %d, re %d => (%d,%d)\n",prb,i,*(short *)&rxF_ext[i],*(1+(short*)&rxF_ext[i]));
#endif
		dl_ch0_ext+=6;
		dl_ch1_ext+=6;
		rxF_ext+=6;
	      } else if (skip_half==2) {
		memcpy(dl_ch0_ext,dl_ch0p+6,6*sizeof(int32_t));
		memcpy(dl_ch1_ext,dl_ch1p+6,6*sizeof(int32_t));
		memcpy(rxF_ext,rxF+6,6*sizeof(int32_t));
#ifdef DEBUG_DLSCH_DEMOD
		for (i=0;i<6;i++)
		  printf("extract rb %d, re %d => (%d,%d)\n",prb,i,*(short *)&rxF_ext[i],*(1+(short*)&rxF_ext[i]));
#endif
		dl_ch0_ext+=6;
		dl_ch1_ext+=6;
		rxF_ext+=6;
	      } else {  // skip_half==0
		memcpy(dl_ch0_ext,dl_ch0p,12*sizeof(int32_t));
		memcpy(dl_ch1_ext,dl_ch1p,12*sizeof(int32_t));
		memcpy(rxF_ext,rxF,12*sizeof(int32_t));
#ifdef DEBUG_DLSCH_DEMOD
		for (i=0;i<12;i++)
		  printf("extract rb %d, re %d => (%d,%d)\n",prb,i,*(short *)&rxF_ext[i],*(1+(short*)&rxF_ext[i]));
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
		    printf("(pilots,skip1)extract rb %d, re %d (%d)=> (%d,%d)\n",prb,i,j,*(short *)&rxF_ext[j],*(1+(short*)&rxF_ext[j]));
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
		    printf("(pilots,skip2)extract rb %d, re %d (%d) => (%d,%d)\n",prb,i,j,*(short *)&rxF_ext[j],*(1+(short*)&rxF_ext[j]));
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
		    printf("(pilots)extract rb %d, re %d => (%d,%d)\n",prb,i,*(short *)&rxF_ext[j],*(1+(short*)&rxF_ext[j]));
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
		printf("extract rb %d, re %d => (%d,%d)\n",prb,i,*(short *)&rxF_ext[i],*(1+(short*)&rxF_ext[i]));
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
		printf("extract rb %d, re %d => (%d,%d)\n",prb,i,*(short *)&rxF_ext[i],*(1+(short*)&rxF_ext[i]));
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
		  printf("(pilots)extract rb %d, re %d (%d) => (%d,%d)\n",prb,i,j,*(short *)&rxF[i],*(1+(short*)&rxF[i]));
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
		  printf("(pilots)extract rb %d, re %d (%d) => (%d,%d)\n",prb,i,j,*(short *)&rxF[1+i-6],*(1+(short*)&rxF[1+i-6]));
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

//==============================================================================================
// Auxiliary functions
//==============================================================================================

#ifdef USER_MODE

void dump_dlsch2(PHY_VARS_UE *phy_vars_ue,uint8_t eNB_id,uint16_t coded_bits_per_codeword,int round)
{

  unsigned int nsymb = (phy_vars_ue->lte_frame_parms.Ncp == 0) ? 14 : 12;
  char fname[32],vname[32];
  int N_RB_DL=phy_vars_ue->lte_frame_parms.N_RB_DL;

  sprintf(fname,"dlsch%d_rxF_r%d_ext0.m",eNB_id,round);
  sprintf(vname,"dl%d_rxF_r%d_ext0",eNB_id,round);
  write_output(fname,vname,phy_vars_ue->lte_ue_pdsch_vars[eNB_id]->rxdataF_ext[0],12*N_RB_DL*nsymb,1,1);

  if (phy_vars_ue->lte_frame_parms.nb_antennas_rx >1) {
    sprintf(fname,"dlsch%d_rxF_r%d_ext1.m",eNB_id,round);
    sprintf(vname,"dl%d_rxF_r%d_ext1",eNB_id,round);
    write_output(fname,vname,phy_vars_ue->lte_ue_pdsch_vars[eNB_id]->rxdataF_ext[1],12*N_RB_DL*nsymb,1,1);
  }

  sprintf(fname,"dlsch%d_ch_r%d_ext00.m",eNB_id,round);
  sprintf(vname,"dl%d_ch_r%d_ext00",eNB_id,round);
  write_output(fname,vname,phy_vars_ue->lte_ue_pdsch_vars[eNB_id]->dl_ch_estimates_ext[0],12*N_RB_DL*nsymb,1,1);

  if (phy_vars_ue->lte_frame_parms.nb_antennas_rx == 2) {
    sprintf(fname,"dlsch%d_ch_r%d_ext01.m",eNB_id,round);
    sprintf(vname,"dl%d_ch_r%d_ext01",eNB_id,round);
    write_output(fname,vname,phy_vars_ue->lte_ue_pdsch_vars[eNB_id]->dl_ch_estimates_ext[1],12*N_RB_DL*nsymb,1,1);
  }

  if (phy_vars_ue->lte_frame_parms.nb_antennas_tx_eNB == 2) {
    sprintf(fname,"dlsch%d_ch_r%d_ext10.m",eNB_id,round);
    sprintf(vname,"dl%d_ch_r%d_ext10",eNB_id,round);
    write_output(fname,vname,phy_vars_ue->lte_ue_pdsch_vars[eNB_id]->dl_ch_estimates_ext[2],12*N_RB_DL*nsymb,1,1);

    if (phy_vars_ue->lte_frame_parms.nb_antennas_rx == 2) {
      sprintf(fname,"dlsch%d_ch_r%d_ext11.m",eNB_id,round);
      sprintf(vname,"dl%d_ch_r%d_ext11",eNB_id,round);
      write_output(fname,vname,phy_vars_ue->lte_ue_pdsch_vars[eNB_id]->dl_ch_estimates_ext[3],12*N_RB_DL*nsymb,1,1);
    }
  }

  /*
    write_output("dlsch%d_ch_ext01.m","dl01_ch0_ext",lte_ue_pdsch_vars[eNB_id]->dl_ch_estimates_ext[1],12*N_RB_DL*nsymb,1,1);
    write_output("dlsch%d_ch_ext10.m","dl10_ch0_ext",lte_ue_pdsch_vars[eNB_id]->dl_ch_estimates_ext[2],12*N_RB_DL*nsymb,1,1);
    write_output("dlsch%d_ch_ext11.m","dl11_ch0_ext",lte_ue_pdsch_vars[eNB_id]->dl_ch_estimates_ext[3],12*N_RB_DL*nsymb,1,1);
  */
  sprintf(fname,"dlsch%d_r%d_rho.m",eNB_id,round);
  sprintf(vname,"dl_rho_r%d_%d",eNB_id,round);
  write_output(fname,vname,phy_vars_ue->lte_ue_pdsch_vars[eNB_id]->dl_ch_rho_ext[0],12*N_RB_DL*nsymb,1,1);

  sprintf(fname,"dlsch%d_rxF_r%d_comp0.m",eNB_id,round);
  sprintf(vname,"dl%d_rxF_r%d_comp0",eNB_id,round);
  write_output(fname,vname,phy_vars_ue->lte_ue_pdsch_vars[eNB_id]->rxdataF_comp0[0],12*N_RB_DL*nsymb,1,1);
  if (phy_vars_ue->lte_frame_parms.nb_antennas_tx_eNB == 2) {
    sprintf(fname,"dlsch%d_rxF_r%d_comp1.m",eNB_id,round);
    sprintf(vname,"dl%d_rxF_r%d_comp1",eNB_id,round);
    write_output(fname,vname,phy_vars_ue->lte_ue_pdsch_vars[eNB_id]->rxdataF_comp1[0][round],12*N_RB_DL*nsymb,1,1);
  }

  sprintf(fname,"dlsch%d_rxF_r%d_llr.m",eNB_id,round);
  sprintf(vname,"dl%d_r%d_llr",eNB_id,round);
  write_output(fname,vname, phy_vars_ue->lte_ue_pdsch_vars[eNB_id]->llr[0],coded_bits_per_codeword,1,0);
  sprintf(fname,"dlsch%d_r%d_mag1.m",eNB_id,round);
  sprintf(vname,"dl%d_r%d_mag1",eNB_id,round);
  write_output(fname,vname,phy_vars_ue->lte_ue_pdsch_vars[eNB_id]->dl_ch_mag0[0],12*N_RB_DL*nsymb,1,1);
  sprintf(fname,"dlsch%d_r%d_mag2.m",eNB_id,round);
  sprintf(vname,"dl%d_r%d_mag2",eNB_id,round);
  write_output(fname,vname,phy_vars_ue->lte_ue_pdsch_vars[eNB_id]->dl_ch_magb0[0],12*N_RB_DL*nsymb,1,1);

  //  printf("log2_maxh = %d\n",phy_vars_ue->lte_ue_pdsch_vars[eNB_id]->log2_maxh);
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
