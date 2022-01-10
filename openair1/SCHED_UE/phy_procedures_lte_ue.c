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

/*! \file phy_procedures_lte_ue.c
 * \brief Implementation of UE procedures from 36.213 LTE specifications / This includes FeMBMS UE procedures from 36.213 v14.2.0 specification
 * \author R. Knopp, F. Kaltenberger, N. Nikaein, J. Morgade
 * \date 2011
 * \version 0.1
 * \company Eurecom
 * \email: {knopp, florian.kaltenberger, navid.nikaein}@eurecom.fr, javier.morgade@ieee.org
 * \note
 * \warning
 */

#define _GNU_SOURCE

#include <sched.h>
#include "assertions.h"
#include "PHY/defs_UE.h"
#include "PHY/phy_extern_ue.h"
//#include "executables/nr-uesoftmodem.h"
#include "targets/RT/USER/lte-softmodem.h"

#include "PHY/LTE_UE_TRANSPORT/transport_proto_ue.h"
#include "SCHED_UE/sched_UE.h"
#include "PHY/MODULATION/modulation_UE.h"
#include "PHY/LTE_ESTIMATION/lte_estimation.h"

#ifndef PUCCH
  #define PUCCH
#endif

#include "LAYER2/MAC/mac.h"
#include "common/utils/LOG/log.h"

#include "common/utils/LOG/vcd_signal_dumper.h"
#include "UTIL/OPT/opt.h"
#include "intertask_interface.h"
#include "PHY/defs_UE.h"

#include "PHY/CODING/coding_extern.h"

#include "T.h"

#include "PHY/TOOLS/tools_defs.h"

#define DLSCH_RB_ALLOC 0x1fbf  // skip DC RB (total 23/25 RBs)
#define DLSCH_RB_ALLOC_12 0x0aaa  // skip DC RB (total 23/25 RBs)

#define NS_PER_SLOT 500000

char mode_string[4][20] = {"NOT SYNCHED","PRACH","RAR","PUSCH"};

extern double cpuf;

void Msg1_transmitted(module_id_t module_idP, uint8_t CC_id, frame_t frameP, uint8_t eNB_id);
void Msg3_transmitted(module_id_t module_idP, uint8_t CC_id, frame_t frameP, uint8_t eNB_id);

extern uint64_t downlink_frequency[MAX_NUM_CCs][4];

void get_dumpparam(PHY_VARS_UE *ue,
                   UE_rxtx_proc_t *proc,
                   uint8_t eNB_id,
                   uint8_t nb_rb,
                   uint32_t *alloc_even,
                   uint8_t subframe,
                   uint32_t Qm,
                   uint32_t Nl,
                   uint32_t tm,
                   uint8_t *nsymb,
                   uint32_t *coded_bits_per_codeword) {
  *nsymb = (ue->frame_parms.Ncp == 0) ? 14 : 12;
  *coded_bits_per_codeword = get_G(&ue->frame_parms,
                                   nb_rb,
                                   alloc_even,
                                   Qm,
                                   Nl,
                                   ue->pdcch_vars[0%RX_NB_TH][eNB_id]->num_pdcch_symbols,
                                   proc->frame_rx,
                                   subframe,
                                   tm);
}

void dump_dlsch(PHY_VARS_UE *ue,
                UE_rxtx_proc_t *proc,
                uint8_t eNB_id,
                uint8_t subframe,
                uint8_t harq_pid) {
  if (LOG_DUMPFLAG(DEBUG_UE_PHYPROC)) {
    unsigned int coded_bits_per_codeword;
    uint8_t nsymb ;
    get_dumpparam(ue, proc, eNB_id,
                  ue->dlsch[ue->current_thread_id[subframe]][eNB_id][0]->harq_processes[harq_pid]->nb_rb,
                  ue->dlsch[ue->current_thread_id[subframe]][eNB_id][0]->harq_processes[harq_pid]->rb_alloc_even,
                  subframe,
                  ue->dlsch[ue->current_thread_id[subframe]][eNB_id][0]->harq_processes[harq_pid]->Qm,
                  ue->dlsch[ue->current_thread_id[subframe]][eNB_id][0]->harq_processes[harq_pid]->Nl,
                  ue->transmission_mode[eNB_id]<7?0:ue->transmission_mode[eNB_id],
                  &nsymb, &coded_bits_per_codeword);
    LOG_M("rxsigF0.m","rxsF0", ue->common_vars.common_vars_rx_data_per_thread[ue->current_thread_id[subframe]].rxdataF[0],2*nsymb*ue->frame_parms.ofdm_symbol_size,2,1);
    LOG_M("rxsigF0_ext.m","rxsF0_ext", ue->pdsch_vars[ue->current_thread_id[subframe]][0]->rxdataF_ext[0],2*nsymb*ue->frame_parms.ofdm_symbol_size,1,1);
    LOG_M("dlsch00_ch0_ext.m","dl00_ch0_ext", ue->pdsch_vars[ue->current_thread_id[subframe]][0]->dl_ch_estimates_ext[0],300*nsymb,1,1);
    /*
      LOG_M("dlsch01_ch0_ext.m","dl01_ch0_ext",pdsch_vars[0]->dl_ch_estimates_ext[1],300*12,1,1);
      LOG_M("dlsch10_ch0_ext.m","dl10_ch0_ext",pdsch_vars[0]->dl_ch_estimates_ext[2],300*12,1,1);
      LOG_M("dlsch11_ch0_ext.m","dl11_ch0_ext",pdsch_vars[0]->dl_ch_estimates_ext[3],300*12,1,1);
      LOG_M("dlsch_rho.m","dl_rho",pdsch_vars[0]->rho[0],300*12,1,1);
    */
    LOG_M("dlsch_rxF_comp0.m","dlsch0_rxF_comp0", ue->pdsch_vars[ue->current_thread_id[subframe]][0]->rxdataF_comp0[0],300*12,1,1);
    LOG_M("dlsch_rxF_llr.m","dlsch_llr", ue->pdsch_vars[ue->current_thread_id[subframe]][0]->llr[0],coded_bits_per_codeword,1,0);
    LOG_M("dlsch_mag1.m","dlschmag1",ue->pdsch_vars[ue->current_thread_id[subframe]][0]->dl_ch_mag0,300*12,1,1);
    LOG_M("dlsch_mag2.m","dlschmag2",ue->pdsch_vars[ue->current_thread_id[subframe]][0]->dl_ch_magb0,300*12,1,1);
  }
}

void dump_dlsch_SI(PHY_VARS_UE *ue,
                   UE_rxtx_proc_t *proc,
                   uint8_t eNB_id,
                   uint8_t subframe) {
  if (LOG_DUMPFLAG(DEBUG_UE_PHYPROC)) {
    unsigned int coded_bits_per_codeword;
    uint8_t nsymb;
    get_dumpparam(ue, proc, eNB_id,
                  ue->dlsch_SI[eNB_id]->harq_processes[0]->nb_rb,
                  ue->dlsch_SI[eNB_id]->harq_processes[0]->rb_alloc_even,
                  subframe,2,1,0,
                  &nsymb, &coded_bits_per_codeword);
    LOG_D(PHY,"[UE %d] Dumping dlsch_SI : ofdm_symbol_size %d, nsymb %d, nb_rb %d, mcs %d, nb_rb %d, num_pdcch_symbols %d,G %d\n",
          ue->Mod_id,
          ue->frame_parms.ofdm_symbol_size,
          nsymb,
          ue->dlsch_SI[eNB_id]->harq_processes[0]->nb_rb,
          ue->dlsch_SI[eNB_id]->harq_processes[0]->mcs,
          ue->dlsch_SI[eNB_id]->harq_processes[0]->nb_rb,
          ue->pdcch_vars[0%RX_NB_TH][eNB_id]->num_pdcch_symbols,
          coded_bits_per_codeword);
    LOG_M("rxsig0.m","rxs0", &ue->common_vars.rxdata[0][subframe*ue->frame_parms.samples_per_tti],ue->frame_parms.samples_per_tti,1,1);
    LOG_M("rxsigF0.m","rxsF0", ue->common_vars.common_vars_rx_data_per_thread[ue->current_thread_id[subframe]].rxdataF[0],nsymb*ue->frame_parms.ofdm_symbol_size,1,1);
    LOG_M("rxsigF0_ext.m","rxsF0_ext", ue->pdsch_vars_SI[0]->rxdataF_ext[0],2*nsymb*ue->frame_parms.ofdm_symbol_size,1,1);
    LOG_M("dlsch00_ch0_ext.m","dl00_ch0_ext", ue->pdsch_vars_SI[0]->dl_ch_estimates_ext[0],ue->frame_parms.N_RB_DL*12*nsymb,1,1);
    /*
      LOG_M("dlsch01_ch0_ext.m","dl01_ch0_ext",pdsch_vars[0]->dl_ch_estimates_ext[1],300*12,1,1);
      LOG_M("dlsch10_ch0_ext.m","dl10_ch0_ext",pdsch_vars[0]->dl_ch_estimates_ext[2],300*12,1,1);
      LOG_M("dlsch11_ch0_ext.m","dl11_ch0_ext",pdsch_vars[0]->dl_ch_estimates_ext[3],300*12,1,1);
      LOG_M("dlsch_rho.m","dl_rho",pdsch_vars[0]->rho[0],300*12,1,1);
    */
    LOG_M("dlsch_rxF_comp0.m","dlsch0_rxF_comp0", ue->pdsch_vars_SI[0]->rxdataF_comp0[0],ue->frame_parms.N_RB_DL*12*nsymb,1,1);
    LOG_M("dlsch_rxF_llr.m","dlsch_llr", ue->pdsch_vars_SI[0]->llr[0],coded_bits_per_codeword,1,0);
    LOG_M("dlsch_mag1.m","dlschmag1",ue->pdsch_vars_SI[0]->dl_ch_mag0,300*nsymb,1,1);
    LOG_M("dlsch_mag2.m","dlschmag2",ue->pdsch_vars_SI[0]->dl_ch_magb0,300*nsymb,1,1);
    sleep(1);
    exit(-1);
  }
}


unsigned int get_tx_amp(int power_dBm,
                        int power_max_dBm,
                        int N_RB_UL,
                        int nb_rb) {
  int gain_dB;
  double gain_lin;

  if ( (power_dBm<=power_max_dBm) && ! IS_SOFTMODEM_RFSIM)
    gain_dB = power_dBm - power_max_dBm;
  else
    gain_dB = 0;

  gain_lin = pow(10,.1*gain_dB);
  AssertFatal((nb_rb >0) && (nb_rb <= N_RB_UL),"Illegal nb_rb/N_RB_UL combination (%d/%d)\n",nb_rb,N_RB_UL);
  LOG_D(PHY," tx gain: %d = %d * sqrt ( pow(10, 0.1*max(0,%d-%d)) * %d/%d ) (gain lin=%f (dB=%d))\n",
        (int)(AMP*sqrt(gain_lin*N_RB_UL/(double)nb_rb)),
        AMP, power_dBm, power_max_dBm,  N_RB_UL, nb_rb, gain_lin, gain_dB);
  return((int)(AMP*sqrt(gain_lin*N_RB_UL/(double)nb_rb)));
}


void dump_dlsch_ra(PHY_VARS_UE *ue,
                   UE_rxtx_proc_t *proc,
                   uint8_t eNB_id,
                   uint8_t subframe) {
  if (LOG_DUMPFLAG(DEBUG_UE_PHYPROC)) {
    unsigned int coded_bits_per_codeword;
    uint8_t nsymb ;
    get_dumpparam(ue, proc, eNB_id,
                  ue->dlsch_SI[eNB_id]->harq_processes[0]->nb_rb,
                  ue->dlsch_SI[eNB_id]->harq_processes[0]->rb_alloc_even,
                  subframe,2,1,0,
                  &nsymb, &coded_bits_per_codeword);
    LOG_D(PHY,"[UE %d] Dumping dlsch_ra : nb_rb %d, mcs %d, nb_rb %d, num_pdcch_symbols %d,G %d\n",
          ue->Mod_id,
          ue->dlsch_ra[eNB_id]->harq_processes[0]->nb_rb,
          ue->dlsch_ra[eNB_id]->harq_processes[0]->mcs,
          ue->dlsch_ra[eNB_id]->harq_processes[0]->nb_rb,
          ue->pdcch_vars[0%RX_NB_TH][eNB_id]->num_pdcch_symbols,
          coded_bits_per_codeword);
    LOG_M("rxsigF0.m","rxsF0", ue->common_vars.common_vars_rx_data_per_thread[ue->current_thread_id[subframe]].rxdataF[0],2*12*ue->frame_parms.ofdm_symbol_size,2,1);
    LOG_M("rxsigF0_ext.m","rxsF0_ext", ue->pdsch_vars_ra[0]->rxdataF_ext[0],2*12*ue->frame_parms.ofdm_symbol_size,1,1);
    LOG_M("dlsch00_ch0_ext.m","dl00_ch0_ext", ue->pdsch_vars_ra[0]->dl_ch_estimates_ext[0],300*nsymb,1,1);
    /*
      LOG_M("dlsch01_ch0_ext.m","dl01_ch0_ext",pdsch_vars[0]->dl_ch_estimates_ext[1],300*12,1,1);
      LOG_M("dlsch10_ch0_ext.m","dl10_ch0_ext",pdsch_vars[0]->dl_ch_estimates_ext[2],300*12,1,1);
      LOG_M("dlsch11_ch0_ext.m","dl11_ch0_ext",pdsch_vars[0]->dl_ch_estimates_ext[3],300*12,1,1);
      LOG_M("dlsch_rho.m","dl_rho",pdsch_vars[0]->rho[0],300*12,1,1);
    */
    LOG_M("dlsch_rxF_comp0.m","dlsch0_rxF_comp0", ue->pdsch_vars_ra[0]->rxdataF_comp0[0],300*nsymb,1,1);
    LOG_M("dlsch_rxF_llr.m","dlsch_llr", ue->pdsch_vars_ra[0]->llr[0],coded_bits_per_codeword,1,0);
    LOG_M("dlsch_mag1.m","dlschmag1",ue->pdsch_vars_ra[0]->dl_ch_mag0,300*nsymb,1,1);
    LOG_M("dlsch_mag2.m","dlschmag2",ue->pdsch_vars_ra[0]->dl_ch_magb0,300*nsymb,1,1);
  }
}

void phy_reset_ue(module_id_t Mod_id,
                  uint8_t CC_id,
                  uint8_t eNB_index) {
  // This flushes ALL DLSCH and ULSCH harq buffers of ALL connected eNBs...add the eNB_index later
  // for more flexibility
  uint8_t i,j,k,s;
  PHY_VARS_UE *ue = PHY_vars_UE_g[Mod_id][CC_id];

  //[NUMBER_OF_RX_THREAD=2][NUMBER_OF_CONNECTED_eNB_MAX][2];
  for(int l=0; l<RX_NB_TH; l++) {
    for(i=0; i<NUMBER_OF_CONNECTED_eNB_MAX; i++) {
      for(j=0; j<2; j++) {
        //DL HARQ
        if(ue->dlsch[l][i][j]) {
          for(k=0; k<NUMBER_OF_HARQ_PID_MAX && ue->dlsch[l][i][j]->harq_processes[k]; k++) {
            ue->dlsch[l][i][j]->harq_processes[k]->status = SCH_IDLE;

            for (s=0; s<10; s++) {
              // reset ACK/NACK bit to DTX for all subframes s = 0..9
              ue->dlsch[l][i][j]->harq_ack[s].ack = 2;
              ue->dlsch[l][i][j]->harq_ack[s].send_harq_status = 0;
              ue->dlsch[l][i][j]->harq_ack[s].vDAI_UL = 0xff;
              ue->dlsch[l][i][j]->harq_ack[s].vDAI_DL = 0xff;
            }
          }
        }
      }

      //UL HARQ
      if(ue->ulsch[i]) {
        for(k=0; k<NUMBER_OF_HARQ_PID_MAX && ue->ulsch[i]->harq_processes[k]; k++) {
          ue->ulsch[i]->harq_processes[k]->status = SCH_IDLE;
          //Set NDIs for all UL HARQs to 0
          //  ue->ulsch[i]->harq_processes[k]->Ndi = 0;
        }
      }

      // flush Msg3 buffer
      ue->ulsch_Msg3_active[i] = 0;
    }
  }
}

void ra_failed(uint8_t Mod_id,
               uint8_t CC_id,
               uint8_t eNB_index) {
  // if contention resolution fails, go back to PRACH
  PHY_vars_UE_g[Mod_id][CC_id]->UE_mode[eNB_index] = PRACH;

  for (int i=0; i <RX_NB_TH_MAX; i++ ) {
    PHY_vars_UE_g[Mod_id][CC_id]->pdcch_vars[i][eNB_index]->crnti_is_temporary = 0;
    PHY_vars_UE_g[Mod_id][CC_id]->pdcch_vars[i][eNB_index]->crnti = 0;
  }

  LOG_E(PHY,"[UE %d] Random-access procedure fails, going back to PRACH, setting SIStatus = 0, discard temporary C-RNTI and State RRC_IDLE\n",Mod_id);
}

void ra_succeeded(uint8_t Mod_id,
                  uint8_t CC_id,
                  uint8_t eNB_index) {
  int i;
  LOG_I(PHY,"[UE %d][RAPROC] Random-access procedure succeeded. Set C-RNTI = Temporary C-RNTI\n",Mod_id);

  for (int i=0; i <RX_NB_TH_MAX; i++ )
    PHY_vars_UE_g[Mod_id][CC_id]->pdcch_vars[i][eNB_index]->crnti_is_temporary = 0;

  PHY_vars_UE_g[Mod_id][CC_id]->ulsch_Msg3_active[eNB_index] = 0;
  PHY_vars_UE_g[Mod_id][CC_id]->UE_mode[eNB_index] = PUSCH;

  for (i=0; i<8; i++) {
    if (PHY_vars_UE_g[Mod_id][CC_id]->ulsch[eNB_index]->harq_processes[i]) {
      PHY_vars_UE_g[Mod_id][CC_id]->ulsch[eNB_index]->harq_processes[i]->status=SCH_IDLE;

      for (int i=0; i <RX_NB_TH_MAX; i++ ) {
        PHY_vars_UE_g[Mod_id][CC_id]->dlsch[i][eNB_index][0]->harq_processes[i]->round=0;
      }

      PHY_vars_UE_g[Mod_id][CC_id]->ulsch[eNB_index]->harq_processes[i]->subframe_scheduling_flag=0;
    }
  }
}

UE_MODE_t get_ue_mode(uint8_t Mod_id,
                      uint8_t CC_id,
                      uint8_t eNB_index) {
  return(PHY_vars_UE_g[Mod_id][CC_id]->UE_mode[eNB_index]);
}

void process_timing_advance_rar(PHY_VARS_UE *ue,
                                UE_rxtx_proc_t *proc,
                                uint16_t timing_advance) {
  ue->timing_advance = timing_advance*4;

  if (LOG_DEBUGFLAG(DEBUG_UE_PHYPROC)) {
    /* TODO: fix this log, what is 'HW timing advance'? */
    /*LOG_I(PHY,"[UE %d] AbsoluteSubFrame %d.%d, received (rar) timing_advance %d, HW timing advance %d\n",ue->Mod_id,proc->frame_rx, proc->subframe_rx, ue->timing_advance);*/
    LOG_UI(PHY,"[UE %d] AbsoluteSubFrame %d.%d, received (rar) timing_advance %d\n",ue->Mod_id,proc->frame_rx, proc->subframe_rx, ue->timing_advance);
  }
}

void process_timing_advance(module_id_t Mod_id,
                            uint8_t CC_id,
                            int16_t timing_advance) {
  //  uint32_t frame = PHY_vars_UE_g[Mod_id]->frame;
  // timing advance has Q1.5 format
  timing_advance = timing_advance - 31;
  PHY_vars_UE_g[Mod_id][CC_id]->timing_advance = PHY_vars_UE_g[Mod_id][CC_id]->timing_advance+timing_advance*4; //this is for 25RB only!!!
  LOG_D(PHY,"[UE %d] Got timing advance %d from MAC, new value %d\n",Mod_id, timing_advance, PHY_vars_UE_g[Mod_id][CC_id]->timing_advance);
}

uint8_t is_SR_TXOp(PHY_VARS_UE *ue,
                   UE_rxtx_proc_t *proc,
                   uint8_t eNB_id) {
  int subframe=proc->subframe_tx;
  LOG_D(PHY,"[UE %d][SR %x] Frame %d subframe %d Checking for SR TXOp (sr_ConfigIndex %d)\n",
        ue->Mod_id,ue->pdcch_vars[ue->current_thread_id[proc->subframe_rx]][eNB_id]->crnti,proc->frame_tx,subframe,
        ue->scheduling_request_config[eNB_id].sr_ConfigIndex);

  if (ue->scheduling_request_config[eNB_id].sr_ConfigIndex <= 4) {        // 5 ms SR period
    if ((subframe%5) == ue->scheduling_request_config[eNB_id].sr_ConfigIndex)
      return(1);
  } else if (ue->scheduling_request_config[eNB_id].sr_ConfigIndex <= 14) { // 10 ms SR period
    if (subframe==(ue->scheduling_request_config[eNB_id].sr_ConfigIndex-5))
      return(1);
  } else if (ue->scheduling_request_config[eNB_id].sr_ConfigIndex <= 34) { // 20 ms SR period
    if ((10*(proc->frame_tx&1)+subframe) == (ue->scheduling_request_config[eNB_id].sr_ConfigIndex-15))
      return(1);
  } else if (ue->scheduling_request_config[eNB_id].sr_ConfigIndex <= 74) { // 40 ms SR period
    if ((10*(proc->frame_tx&3)+subframe) == (ue->scheduling_request_config[eNB_id].sr_ConfigIndex-35))
      return(1);
  } else if (ue->scheduling_request_config[eNB_id].sr_ConfigIndex <= 154) { // 80 ms SR period
    if ((10*(proc->frame_tx&7)+subframe) == (ue->scheduling_request_config[eNB_id].sr_ConfigIndex-75))
      return(1);
  }

  return(0);
}

uint8_t is_cqi_TXOp(PHY_VARS_UE *ue,
                    UE_rxtx_proc_t *proc,
                    uint8_t eNB_id) {
  int subframe = proc->subframe_tx;
  int frame    = proc->frame_tx;
  CQI_REPORTPERIODIC *cqirep = &ue->cqi_report_config[eNB_id].CQI_ReportPeriodic;

  //LOG_I(PHY,"[UE %d][CRNTI %x] AbsSubFrame %d.%d Checking for CQI TXOp (cqi_ConfigIndex %d) isCQIOp %d\n",
  //      ue->Mod_id,ue->pdcch_vars[eNB_id]->crnti,frame,subframe,
  //      cqirep->cqi_PMI_ConfigIndex,
  //      (((10*frame + subframe) % cqirep->Npd) == cqirep->N_OFFSET_CQI));

  if (cqirep->cqi_PMI_ConfigIndex==-1)
    return(0);
  else if (((10*frame + subframe) % cqirep->Npd) == cqirep->N_OFFSET_CQI)
    return(1);
  else
    return(0);
}
uint8_t is_ri_TXOp(PHY_VARS_UE *ue,
                   UE_rxtx_proc_t *proc,
                   uint8_t eNB_id) {
  int subframe = proc->subframe_tx;
  int frame    = proc->frame_tx;
  CQI_REPORTPERIODIC *cqirep = &ue->cqi_report_config[eNB_id].CQI_ReportPeriodic;
  int log2Mri = cqirep->ri_ConfigIndex/161;
  int N_OFFSET_RI = cqirep->ri_ConfigIndex % 161;

  //LOG_I(PHY,"[UE %d][CRNTI %x] AbsSubFrame %d.%d Checking for RI TXOp (ri_ConfigIndex %d) isRIOp %d\n",
  //      ue->Mod_id,ue->pdcch_vars[eNB_id]->crnti,frame,subframe,
  //      cqirep->ri_ConfigIndex,
  //      (((10*frame + subframe + cqirep->N_OFFSET_CQI - N_OFFSET_RI) % (cqirep->Npd<<log2Mri)) == 0));
  if (cqirep->ri_ConfigIndex==-1)
    return(0);
  else if (((10*frame + subframe + cqirep->N_OFFSET_CQI - N_OFFSET_RI) % (cqirep->Npd<<log2Mri)) == 0)
    return(1);
  else
    return(0);
}

void compute_cqi_ri_resources(PHY_VARS_UE *ue,
                              LTE_UE_ULSCH_t *ulsch,
                              uint8_t eNB_id,
                              uint16_t rnti,
                              uint16_t p_rnti,
                              uint16_t cba_rnti,
                              uint8_t cqi_status,
                              uint8_t ri_status) {
  //PHY_MEASUREMENTS *meas = &ue->measurements;
  //uint8_t transmission_mode = ue->transmission_mode[eNB_id];

  //LOG_I(PHY,"compute_cqi_ri_resources O_RI %d O %d uci format %d \n",ulsch->O_RI,ulsch->O,ulsch->uci_format);
  if (cqi_status == 1 || ri_status == 1) {
    ulsch->O = 4;
  }
}

void ue_compute_srs_occasion(PHY_VARS_UE *ue,
                             UE_rxtx_proc_t *proc,
                             uint8_t eNB_id,
                             uint8_t isSubframeSRS) {
  LTE_DL_FRAME_PARMS *frame_parms = &ue->frame_parms;
  int frame_tx    = proc->frame_tx;
  int subframe_tx = proc->subframe_tx;
  SOUNDINGRS_UL_CONFIG_DEDICATED *pSoundingrs_ul_config_dedicated=&ue->soundingrs_ul_config_dedicated[eNB_id];
  uint16_t srsPeriodicity;
  uint16_t srsOffset;
  uint8_t is_pucch2_subframe = 0;
  uint8_t is_sr_an_subframe  = 0;
  // check for SRS opportunity
  pSoundingrs_ul_config_dedicated->srsUeSubframe   = 0;
  pSoundingrs_ul_config_dedicated->srsCellSubframe = isSubframeSRS;

  if (isSubframeSRS) {
    LOG_D(PHY," SrsDedicatedSetup: %d \n",pSoundingrs_ul_config_dedicated->srsConfigDedicatedSetup);

    if(pSoundingrs_ul_config_dedicated->srsConfigDedicatedSetup) {
      compute_srs_pos(frame_parms->frame_type, pSoundingrs_ul_config_dedicated->srs_ConfigIndex, &srsPeriodicity, &srsOffset);
      LOG_D(PHY," srsPeriodicity: %d srsOffset: %d isSubframeSRS %d \n",srsPeriodicity,srsOffset,isSubframeSRS);
      // transmit SRS if the four following constraints are respected:
      // - UE is configured to transmit SRS
      // - SRS are configured in current subframe
      // - UE is configured to send SRS in this subframe
      // 36.213 8.2
      // 1- A UE shall not transmit SRS whenever SRS and PUCCH format 2/2a/2b transmissions happen to coincide in the same subframe
      // 2- A UE shall not transmit SRS whenever SRS transmit
      //    on and PUCCH transmission carrying ACK/NACK and/or
      //    positive SR happen to coincide in the same subframe if the parameter
      //    Simultaneous-AN-and-SRS is FALSE
      // check PUCCH format 2/2a/2b transmissions
      is_pucch2_subframe = is_cqi_TXOp(ue,proc,eNB_id) && (ue->cqi_report_config[eNB_id].CQI_ReportPeriodic.cqi_PMI_ConfigIndex>0);
      is_pucch2_subframe = (is_ri_TXOp(ue,proc,eNB_id) && (ue->cqi_report_config[eNB_id].CQI_ReportPeriodic.ri_ConfigIndex>0)) || is_pucch2_subframe;

      // check ACK/SR transmission
      if(frame_parms->soundingrs_ul_config_common.ackNackSRS_SimultaneousTransmission == FALSE) {
        if(is_SR_TXOp(ue,proc,eNB_id)) {
          uint32_t SR_payload = 0;

          if (ue->mac_enabled==1) {
            int Mod_id = ue->Mod_id;
            int CC_id = ue->CC_id;
            SR_payload = ue_get_SR(Mod_id,
                                   CC_id,
                                   frame_tx,
                                   eNB_id,
                                   ue->pdcch_vars[ue->current_thread_id[proc->subframe_rx]][eNB_id]->crnti,
                                   subframe_tx); // subframe used for meas gap

            if (SR_payload > 0)
              is_sr_an_subframe = 1;
          }
        }

        uint8_t pucch_ack_payload[2];

        if (get_ack(&ue->frame_parms,
                    ue->dlsch[ue->current_thread_id[proc->subframe_rx]][eNB_id][0]->harq_ack,
                    subframe_tx,proc->subframe_rx,pucch_ack_payload,0) > 0) {
          is_sr_an_subframe = 1;
        }
      }

      // check SRS UE opportunity
      if( isSubframeSRS  &&
          (((10*frame_tx+subframe_tx) % srsPeriodicity) == srsOffset)
        ) {
        if ((is_pucch2_subframe == 0) && (is_sr_an_subframe == 0)) {
          pSoundingrs_ul_config_dedicated->srsUeSubframe = 1;
          ue->ulsch[eNB_id]->srs_active   = 1;
          ue->ulsch[eNB_id]->Nsymb_pusch  = 12-(frame_parms->Ncp<<1)- ue->ulsch[eNB_id]->srs_active;
        } else {
          LOG_I(PHY,"DROP UE-SRS-TX for this subframe %d.%d: collision with PUCCH2 or SR/AN: PUCCH2-occasion: %d, SR-AN-occasion[simSRS-SR-AN %d]: %d  \n", frame_tx, subframe_tx, is_pucch2_subframe,
                frame_parms->soundingrs_ul_config_common.ackNackSRS_SimultaneousTransmission, is_sr_an_subframe);
        }
      }
    }

    LOG_D(PHY," srsCellSubframe: %d, srsUeSubframe: %d, Nsymb-pusch: %d \n", pSoundingrs_ul_config_dedicated->srsCellSubframe, pSoundingrs_ul_config_dedicated->srsUeSubframe,
          ue->ulsch[eNB_id]->Nsymb_pusch);
  }
}


void get_cqipmiri_params(PHY_VARS_UE *ue,
                         uint8_t eNB_id) {
  CQI_REPORTPERIODIC *cqirep = &ue->cqi_report_config[eNB_id].CQI_ReportPeriodic;
  int cqi_PMI_ConfigIndex = cqirep->cqi_PMI_ConfigIndex;

  if (ue->frame_parms.frame_type == FDD) {
    if (cqi_PMI_ConfigIndex <= 1) {        // 2 ms CQI_PMI period
      cqirep->Npd = 2;
      cqirep->N_OFFSET_CQI = cqi_PMI_ConfigIndex;
    } else if (cqi_PMI_ConfigIndex <= 6) { // 5 ms CQI_PMI period
      cqirep->Npd = 5;
      cqirep->N_OFFSET_CQI = cqi_PMI_ConfigIndex-2;
    } else if (cqi_PMI_ConfigIndex <=16) { // 10ms CQI_PMI period
      cqirep->Npd = 10;
      cqirep->N_OFFSET_CQI = cqi_PMI_ConfigIndex-7;
    } else if (cqi_PMI_ConfigIndex <= 36) { // 20 ms CQI_PMI period
      cqirep->Npd = 20;
      cqirep->N_OFFSET_CQI = cqi_PMI_ConfigIndex-17;
    } else if (cqi_PMI_ConfigIndex <= 76) { // 40 ms CQI_PMI period
      cqirep->Npd = 40;
      cqirep->N_OFFSET_CQI = cqi_PMI_ConfigIndex-37;
    } else if (cqi_PMI_ConfigIndex <= 156) { // 80 ms CQI_PMI period
      cqirep->Npd = 80;
      cqirep->N_OFFSET_CQI = cqi_PMI_ConfigIndex-77;
    } else if (cqi_PMI_ConfigIndex <= 316) { // 160 ms CQI_PMI period
      cqirep->Npd = 160;
      cqirep->N_OFFSET_CQI = cqi_PMI_ConfigIndex-157;
    } else if (cqi_PMI_ConfigIndex > 317) {
      if (cqi_PMI_ConfigIndex <= 349) { // 32 ms CQI_PMI period
        cqirep->Npd = 32;
        cqirep->N_OFFSET_CQI = cqi_PMI_ConfigIndex-318;
      } else if (cqi_PMI_ConfigIndex <= 413) { // 64 ms CQI_PMI period
        cqirep->Npd = 64;
        cqirep->N_OFFSET_CQI = cqi_PMI_ConfigIndex-350;
      } else if (cqi_PMI_ConfigIndex <= 541) { // 128 ms CQI_PMI period
        cqirep->Npd = 128;
        cqirep->N_OFFSET_CQI = cqi_PMI_ConfigIndex-414;
      }
    }
  } else { // TDD
    if (cqi_PMI_ConfigIndex == 0) {        // all UL subframes
      cqirep->Npd = 1;
      cqirep->N_OFFSET_CQI = 0;
    } else if (cqi_PMI_ConfigIndex <= 6) { // 5 ms CQI_PMI period
      cqirep->Npd = 5;
      cqirep->N_OFFSET_CQI = cqi_PMI_ConfigIndex-1;
    } else if (cqi_PMI_ConfigIndex <=16) { // 10ms CQI_PMI period
      cqirep->Npd = 10;
      cqirep->N_OFFSET_CQI = cqi_PMI_ConfigIndex-6;
    } else if (cqi_PMI_ConfigIndex <= 36) { // 20 ms CQI_PMI period
      cqirep->Npd = 20;
      cqirep->N_OFFSET_CQI = cqi_PMI_ConfigIndex-16;
    } else if (cqi_PMI_ConfigIndex <= 76) { // 40 ms CQI_PMI period
      cqirep->Npd = 40;
      cqirep->N_OFFSET_CQI = cqi_PMI_ConfigIndex-36;
    } else if (cqi_PMI_ConfigIndex <= 156) { // 80 ms CQI_PMI period
      cqirep->Npd = 80;
      cqirep->N_OFFSET_CQI = cqi_PMI_ConfigIndex-76;
    } else if (cqi_PMI_ConfigIndex <= 316) { // 160 ms CQI_PMI period
      cqirep->Npd = 160;
      cqirep->N_OFFSET_CQI = cqi_PMI_ConfigIndex-156;
    }
  }
}

PUCCH_FMT_t get_pucch_format(lte_frame_type_t frame_type,
                             lte_prefix_type_t cyclic_prefix_type,
                             uint8_t SR_payload,
                             uint8_t nb_cw,
                             uint8_t cqi_status,
                             uint8_t ri_status,
                             uint8_t bundling_flag) {
  if((cqi_status == 0) && (ri_status==0)) {
    // PUCCH Format 1 1a 1b
    // 1- SR only ==> PUCCH format 1
    // 2- 1bit Ack/Nack with/without SR  ==> PUCCH format 1a
    // 3- 2bits Ack/Nack with/without SR ==> PUCCH format 1b
    if((nb_cw == 1)&&(bundling_flag==bundling)) {
      return pucch_format1a;
    }

    if((nb_cw == 1)&&(bundling_flag==multiplexing)) {
      return pucch_format1b;
    }

    if(nb_cw == 2) {
      return pucch_format1b;
    }

    if(SR_payload == 1) {
      return pucch_format1;
      /*
      if (frame_type == FDD) {
          return pucch_format1;
      } else if (frame_type == TDD) {
          return pucch_format1b;
      } else {
          AssertFatal(1==0,"Unknown frame_type");
      }*/
    }
  } else {
    // PUCCH Format 2 2a 2b
    // 1- CQI only or RI only  ==> PUCCH format 2
    // 2- CQI or RI + 1bit Ack/Nack for normal CP  ==> PUCCH format 2a
    // 3- CQI or RI + 2bits Ack/Nack for normal CP ==> PUCCH format 2b
    // 4- CQI or RI + Ack/Nack for extended CP ==> PUCCH format 2
    if(nb_cw == 0) {
      return pucch_format2;
    }

    if(cyclic_prefix_type == NORMAL) {
      if(nb_cw == 1) {
        return pucch_format2a;
      }

      if(nb_cw == 2) {
        return pucch_format2b;
      }
    } else {
      return pucch_format2;
    }
  }

  return pucch_format1a;
}
uint16_t get_n1_pucch(PHY_VARS_UE *ue,
                      UE_rxtx_proc_t *proc,
                      harq_status_t *harq_ack,
                      uint8_t eNB_id,
                      uint8_t *b,
                      uint8_t SR) {
  LTE_DL_FRAME_PARMS *frame_parms=&ue->frame_parms;
  uint8_t nCCE0,nCCE1,nCCE2,nCCE3,harq_ack1,harq_ack0,harq_ack3,harq_ack2;
  ANFBmode_t bundling_flag;
  uint16_t n1_pucch0=0,n1_pucch1=0,n1_pucch2=0,n1_pucch3=0,n1_pucch_inter;
  static uint8_t candidate_dl[9]; // which downlink(s) the current ACK/NACK is associating to
  uint8_t last_dl=0xff; // the last downlink with valid DL-DCI. for calculating the PUCCH resource index
  int sf;
  int M;
  uint8_t ack_counter=0;
  // clear this, important for case where n1_pucch selection is not used
  int subframe=proc->subframe_tx;
  ue->pucch_sel[subframe] = 0;

  if (frame_parms->frame_type == FDD ) { // FDD
    sf = (subframe<4)? subframe+6 : subframe-4;
    LOG_D(PHY,"n1_pucch_UE: subframe %d, nCCE %d\n",sf,ue->pdcch_vars[ue->current_thread_id[proc->subframe_rx]][eNB_id]->nCCE[sf]);

    if (SR == 0)
      return(frame_parms->pucch_config_common.n1PUCCH_AN + ue->pdcch_vars[ue->current_thread_id[proc->subframe_rx]][eNB_id]->nCCE[sf]);
    else
      return(ue->scheduling_request_config[eNB_id].sr_PUCCH_ResourceIndex);
  } else {
    bundling_flag = ue->pucch_config_dedicated[eNB_id].tdd_AckNackFeedbackMode;

    if (LOG_DEBUGFLAG(DEBUG_UE_PHYPROC)) {
      if (bundling_flag==bundling) {
        LOG_D(PHY,"[UE%d] Frame %d subframe %d : get_n1_pucch, bundling, SR %d/%d\n",ue->Mod_id,proc->frame_tx,subframe,SR,
              ue->scheduling_request_config[eNB_id].sr_PUCCH_ResourceIndex);
      } else {
        LOG_D(PHY,"[UE%d] Frame %d subframe %d : get_n1_pucch, multiplexing, SR %d/%d\n",ue->Mod_id,proc->frame_tx,subframe,SR,
              ue->scheduling_request_config[eNB_id].sr_PUCCH_ResourceIndex);
      }
    }

    switch (frame_parms->tdd_config) {
      case 1:  // DL:S:UL:UL:DL:DL:S:UL:UL:DL
        harq_ack0 = 2; // DTX
        M=1;

        // This is the offset for a particular subframe (2,3,4) => (0,2,4)
        if (subframe == 2) {  // ACK subframes 5,6
          candidate_dl[0] = 6;
          candidate_dl[1] = 5;
          M=2;
        } else if (subframe == 3) { // ACK subframe 9
          candidate_dl[0] = 9;
        } else if (subframe == 7) { // ACK subframes 0,1
          candidate_dl[0] = 1;
          candidate_dl[1] = 0;
          M=2;
        } else if (subframe == 8) { // ACK subframes 4
          candidate_dl[0] = 4;
        } else {
          LOG_E(PHY,"[UE%d] : Frame %d phy_procedures_lte.c: get_n1pucch, illegal tx-subframe %d for tdd_config %d\n",
                ue->Mod_id,proc->frame_tx,subframe,frame_parms->tdd_config);
          return(0);
        }

        // checking which downlink candidate is the last downlink with valid DL-DCI
        int k;

        for (k=0; k<M; k++) {
          if (harq_ack[candidate_dl[k]].send_harq_status>0) {
            last_dl = candidate_dl[k];
            break;
          }
        }

        if (last_dl >= 10) {
          LOG_E(PHY,"[UE%d] : Frame %d phy_procedures_lte.c: get_n1pucch, illegal rx-subframe %d (tx-subframe %d) for tdd_config %d\n",
                ue->Mod_id,proc->frame_tx,last_dl,subframe,frame_parms->tdd_config);
          return (0);
        }

        LOG_D(PHY,"SFN/SF %d/%d calculating n1_pucch0 from last_dl=%d\n",
              proc->frame_tx%1024,
              proc->subframe_tx,
              last_dl);
        // i=0
        nCCE0 = ue->pdcch_vars[ue->current_thread_id[proc->subframe_rx]][eNB_id]->nCCE[last_dl];
        n1_pucch0 = get_Np(frame_parms->N_RB_DL,nCCE0,0) + nCCE0+ frame_parms->pucch_config_common.n1PUCCH_AN;
        harq_ack0 = b[0];

        if (harq_ack0!=2) {  // DTX
          if (frame_parms->frame_type == FDD ) {
            if (SR == 0) {  // last paragraph pg 68 from 36.213 (v8.6), m=0
              b[0]=(M==2) ? 1-harq_ack0 : harq_ack0;
              b[1]=harq_ack0;   // in case we use pucch format 1b (subframes 2,7)
              ue->pucch_sel[subframe] = 0;
              return(n1_pucch0);
            } else { // SR and only 0 or 1 ACKs (first 2 entries in Table 7.3-1 of 36.213)
              b[0]=harq_ack0;
              return(ue->scheduling_request_config[eNB_id].sr_PUCCH_ResourceIndex);
            }
          } else {
            if (SR == 0) {
              b[0] = harq_ack0;
              b[1] = harq_ack0;
              ue->pucch_sel[subframe] = 0;
              return(n1_pucch0);
            } else {
              b[0] = harq_ack0;
              b[1] = harq_ack0;
              return(ue->scheduling_request_config[eNB_id].sr_PUCCH_ResourceIndex);
            }
          }
        }

        break;

      case 3:  // DL:S:UL:UL:UL:DL:DL:DL:DL:DL
        // in this configuration we have M=2 from pg 68 of 36.213 (v8.6)
        // Note: this doesn't allow using subframe 1 for PDSCH transmission!!! (i.e. SF 1 cannot be acked in SF 2)
        // set ACK/NAKs to DTX
        harq_ack1 = 2; // DTX
        harq_ack0 = 2; // DTX
        // This is the offset for a particular subframe (2,3,4) => (0,2,4)
        last_dl = (subframe-2)<<1;
        // i=0
        nCCE0 = ue->pdcch_vars[ue->current_thread_id[proc->subframe_rx]][eNB_id]->nCCE[5+last_dl];
        n1_pucch0 = get_Np(frame_parms->N_RB_DL,nCCE0,0) + nCCE0+ frame_parms->pucch_config_common.n1PUCCH_AN;
        // i=1
        nCCE1 = ue->pdcch_vars[ue->current_thread_id[proc->subframe_rx]][eNB_id]->nCCE[(6+last_dl)%10];
        n1_pucch1 = get_Np(frame_parms->N_RB_DL,nCCE1,1) + nCCE1 + frame_parms->pucch_config_common.n1PUCCH_AN;

        // set ACK/NAK to values if not DTX
        if (ue->dlsch[ue->current_thread_id[proc->subframe_rx]][eNB_id][0]->harq_ack[(6+last_dl)%10].send_harq_status>0)  // n-6 // subframe 6 is to be ACK/NAKed
          harq_ack1 = ue->dlsch[ue->current_thread_id[proc->subframe_rx]][eNB_id][0]->harq_ack[(6+last_dl)%10].ack;

        if (ue->dlsch[ue->current_thread_id[proc->subframe_rx]][eNB_id][0]->harq_ack[5+last_dl].send_harq_status>0)  // n-6 // subframe 5 is to be ACK/NAKed
          harq_ack0 = ue->dlsch[ue->current_thread_id[proc->subframe_rx]][eNB_id][0]->harq_ack[5+last_dl].ack;

        LOG_D(PHY,"SFN/SF %d/%d calculating n1_pucch cce0=%d n1_pucch0=%d cce1=%d n1_pucch1=%d\n",
              proc->frame_tx%1024,
              proc->subframe_tx,
              nCCE0,n1_pucch0,
              nCCE1,n1_pucch1);

        if (harq_ack1!=2) { // n-6 // subframe 6,8,0 and maybe 5,7,9 is to be ACK/NAKed
          if ((bundling_flag==bundling)&&(SR == 0)) {  // This is for bundling without SR,
            // n1_pucch index takes value of smallest element in set {0,1}
            // i.e. 0 if harq_ack0 is not DTX, otherwise 1
            b[0] = harq_ack1;

            if (harq_ack0!=2)
              b[0]=b[0]&harq_ack0;

            ue->pucch_sel[subframe] = 1;
            return(n1_pucch1);
          } else if ((bundling_flag==multiplexing)&&(SR==0)) { // Table 10.1
            if (harq_ack0 == 2)
              harq_ack0 = 0;

            b[1] = harq_ack0;
            b[0] = (harq_ack0!=harq_ack1)?0:1;

            if ((harq_ack0 == 1) && (harq_ack1 == 0)) {
              ue->pucch_sel[subframe] = 0;
              return(n1_pucch0);
            } else {
              ue->pucch_sel[subframe] = 1;
              return(n1_pucch1);
            }
          } else if (SR==1) { // SR and 0,1,or 2 ACKS, (first 3 entries in Table 7.3-1 of 36.213)
            // this should be number of ACKs (including
            if (harq_ack0 == 2)
              harq_ack0 = 0;

            b[0]= harq_ack1 | harq_ack0;
            b[1]= harq_ack1 ^ harq_ack0;
            return(ue->scheduling_request_config[eNB_id].sr_PUCCH_ResourceIndex);
          }
        } else if (harq_ack0!=2) { // n-7  // subframe 5,7,9 only is to be ACK/NAKed
          if ((bundling_flag==bundling)&&(SR == 0)) {  // last paragraph pg 68 from 36.213 (v8.6), m=0
            b[0]=harq_ack0;
            ue->pucch_sel[subframe] = 0;
            return(n1_pucch0);
          } else if ((bundling_flag==multiplexing)&&(SR==0)) { // Table 10.1 with i=1 set to DTX
            b[0] = harq_ack0;
            b[1] = 1-b[0];
            ue->pucch_sel[subframe] = 0;
            return(n1_pucch0);
          } else if (SR==1) { // SR and only 0 or 1 ACKs (first 2 entries in Table 7.3-1 of 36.213)
            b[0]=harq_ack0;
            b[1]=b[0];
            return(ue->scheduling_request_config[eNB_id].sr_PUCCH_ResourceIndex);
          }
        }

        break;

      case 4:  // DL:S:UL:UL:DL:DL:DL:DL:DL:DL
        // in this configuration we have M=4 from pg 68 of 36.213 (v8.6)
        // Note: this doesn't allow using subframe 1 for PDSCH transmission!!! (i.e. SF 1 cannot be acked in SF 2)
        // set ACK/NAKs to DTX
        harq_ack3 = 2; // DTX
        harq_ack2 = 2; // DTX
        harq_ack1 = 2; // DTX
        harq_ack0 = 2; // DTX

        // This is the offset for a particular subframe (2,3,4) => (0,2,4)
        //last_dl = (subframe-2)<<1;
        if (subframe == 2) {
          // i=0
          //nCCE0 = ue->pdcch_vars[ue->current_thread_id[proc->subframe_rx]][eNB_id]->nCCE[2+subframe];
          nCCE0 = ue->pdcch_vars[ue->current_thread_id[proc->subframe_rx]][eNB_id]->nCCE[(8+subframe)%10];
          n1_pucch0 = 2*get_Np(frame_parms->N_RB_DL,nCCE0,0) + nCCE0+ frame_parms->pucch_config_common.n1PUCCH_AN;
          // i=1
          nCCE1 = ue->pdcch_vars[ue->current_thread_id[proc->subframe_rx]][eNB_id]->nCCE[2+subframe];
          n1_pucch1 = get_Np(frame_parms->N_RB_DL,nCCE1,0) + get_Np(frame_parms->N_RB_DL,nCCE1,1) + nCCE1 + frame_parms->pucch_config_common.n1PUCCH_AN;
          // i=2
          nCCE2 = ue->pdcch_vars[ue->current_thread_id[proc->subframe_rx]][eNB_id]->nCCE[(8+subframe)%10];
          n1_pucch2 = 2*get_Np(frame_parms->N_RB_DL,nCCE2,1) + nCCE2+ frame_parms->pucch_config_common.n1PUCCH_AN;

          // i=3
          //nCCE3 = ue->pdcch_vars[ue->current_thread_id[proc->subframe_rx]][eNB_id]->nCCE[(9+subframe)%10];
          //n1_pucch3 = get_Np(frame_parms->N_RB_DL,nCCE3,1) + nCCE3 + frame_parms->pucch_config_common.n1PUCCH_AN;

          // set ACK/NAK to values if not DTX
          if (ue->dlsch[ue->current_thread_id[proc->subframe_rx]][eNB_id][0]->harq_ack[(8+subframe)%10].send_harq_status>0)  // n-6 // subframe 6 is to be ACK/NAKed
            harq_ack0 = ue->dlsch[ue->current_thread_id[proc->subframe_rx]][eNB_id][0]->harq_ack[(8+subframe)%10].ack;

          if (ue->dlsch[ue->current_thread_id[proc->subframe_rx]][eNB_id][0]->harq_ack[2+subframe].send_harq_status>0)  // n-6 // subframe 5 is to be ACK/NAKed
            harq_ack1 = ue->dlsch[ue->current_thread_id[proc->subframe_rx]][eNB_id][0]->harq_ack[2+subframe].ack;

          if (ue->dlsch[ue->current_thread_id[proc->subframe_rx]][eNB_id][0]->harq_ack[3+subframe].send_harq_status>0)  // n-6 // subframe 6 is to be ACK/NAKed
            harq_ack2 = ue->dlsch[ue->current_thread_id[proc->subframe_rx]][eNB_id][0]->harq_ack[3+subframe].ack;

          //if (ue->dlsch[ue->current_thread_id[proc->subframe_rx]][eNB_id][0]->harq_ack[(9+subframe)%10].send_harq_status>0)  // n-6 // subframe 5 is to be ACK/NAKed
          //harq_ack3 = ue->dlsch[ue->current_thread_id[proc->subframe_rx]][eNB_id][0]->harq_ack[(9+subframe)%10].ack;
          //LOG_I(PHY,"SFN/SF %d/%d calculating n1_pucch cce0=%d n1_pucch0=%d cce1=%d n1_pucch1=%d cce2=%d n1_pucch2=%d\n",
          //                      proc->frame_tx%1024,
          //                      proc->subframe_tx,
          //                      nCCE0,n1_pucch0,
          //                      nCCE1,n1_pucch1, nCCE2, n1_pucch2);
        } else if (subframe == 3) {
          // i=0
          nCCE0 = ue->pdcch_vars[ue->current_thread_id[proc->subframe_rx]][eNB_id]->nCCE[4+subframe];
          n1_pucch0 = 3*get_Np(frame_parms->N_RB_DL,nCCE0,0) + nCCE0+ frame_parms->pucch_config_common.n1PUCCH_AN;
          // i=1
          nCCE1 = ue->pdcch_vars[ue->current_thread_id[proc->subframe_rx]][eNB_id]->nCCE[5+subframe];
          n1_pucch1 = 2*get_Np(frame_parms->N_RB_DL,nCCE1,0) + get_Np(frame_parms->N_RB_DL,nCCE1,1) + nCCE1 + frame_parms->pucch_config_common.n1PUCCH_AN;
          // i=2
          nCCE2 = ue->pdcch_vars[ue->current_thread_id[proc->subframe_rx]][eNB_id]->nCCE[(6+subframe)];
          n1_pucch2 = get_Np(frame_parms->N_RB_DL,nCCE2,0) + 2*get_Np(frame_parms->N_RB_DL,nCCE2,1) + nCCE2+ frame_parms->pucch_config_common.n1PUCCH_AN;
          // i=3
          nCCE3 = ue->pdcch_vars[ue->current_thread_id[proc->subframe_rx]][eNB_id]->nCCE[(3+subframe)];
          n1_pucch3 = 3*get_Np(frame_parms->N_RB_DL,nCCE3,1) + nCCE3 + frame_parms->pucch_config_common.n1PUCCH_AN;

          // set ACK/NAK to values if not DTX
          if (ue->dlsch[ue->current_thread_id[proc->subframe_rx]][eNB_id][0]->harq_ack[4+subframe].send_harq_status>0)  // n-6 // subframe 6 is to be ACK/NAKed
            harq_ack0 = ue->dlsch[ue->current_thread_id[proc->subframe_rx]][eNB_id][0]->harq_ack[4+subframe].ack;

          if (ue->dlsch[ue->current_thread_id[proc->subframe_rx]][eNB_id][0]->harq_ack[5+subframe].send_harq_status>0)  // n-6 // subframe 5 is to be ACK/NAKed
            harq_ack1 = ue->dlsch[ue->current_thread_id[proc->subframe_rx]][eNB_id][0]->harq_ack[5+subframe].ack;

          if (ue->dlsch[ue->current_thread_id[proc->subframe_rx]][eNB_id][0]->harq_ack[(6+subframe)].send_harq_status>0)  // n-6 // subframe 6 is to be ACK/NAKed
            harq_ack2 = ue->dlsch[ue->current_thread_id[proc->subframe_rx]][eNB_id][0]->harq_ack[(6+subframe)].ack;

          if (ue->dlsch[ue->current_thread_id[proc->subframe_rx]][eNB_id][0]->harq_ack[(3+subframe)].send_harq_status>0)  // n-6 // subframe 5 is to be ACK/NAKed
            harq_ack3 = ue->dlsch[ue->current_thread_id[proc->subframe_rx]][eNB_id][0]->harq_ack[(3+subframe)].ack;
        }

        //LOG_I(PHY,"SFN/SF %d/%d calculating n1_pucch cce0=%d n1_pucch0=%d harq_ack0=%d cce1=%d n1_pucch1=%d harq_ack1=%d cce2=%d n1_pucch2=%d harq_ack2=%d cce3=%d n1_pucch3=%d harq_ack3=%d bundling_flag=%d\n",
        //                                proc->frame_tx%1024,
        //                                proc->subframe_tx,
        //                                nCCE0,n1_pucch0,harq_ack0,
        //                                nCCE1,n1_pucch1,harq_ack1, nCCE2, n1_pucch2, harq_ack2,
        //                                nCCE3, n1_pucch3, harq_ack3, bundling_flag);

        if ((bundling_flag==bundling)&&(SR == 0)) {  // This is for bundling without SR,
          b[0] = 1;
          ack_counter = 0;

          if ((harq_ack3!=2) ) {
            b[0] = b[0]&harq_ack3;
            n1_pucch_inter = n1_pucch3;
            ack_counter ++;
          }

          if ((harq_ack0!=2) ) {
            b[0] = b[0]&harq_ack0;
            n1_pucch_inter = n1_pucch0;
            ack_counter ++;
          }

          if ((harq_ack1!=2) ) {
            b[0] = b[0]&harq_ack1;
            n1_pucch_inter = n1_pucch1;
            ack_counter ++;
          }

          if ((harq_ack2!=2) ) {
            b[0] = b[0]&harq_ack2;
            n1_pucch_inter = n1_pucch2;
            ack_counter ++;
          }

          if (ack_counter == 0)
            b[0] = 0;

          /*if (subframe == 3) {
             n1_pucch_inter = n1_pucch2;
          } else if (subframe == 2) {
             n1_pucch_inter = n1_pucch1;
          }*/
          //LOG_I(PHY,"SFN/SF %d/%d calculating n1_pucch n1_pucch_inter=%d  b[0]=%d b[1]=%d \n",
          //                                           proc->frame_tx%1024,
          //                                           proc->subframe_tx,n1_pucch_inter,
          //                                           b[0],b[1]);
          return(n1_pucch_inter);
        } else if ((bundling_flag==multiplexing)&&(SR==0)) { // Table 10.1
          if (subframe == 3) {
            LOG_I(PHY, "sbuframe=%d \n",subframe);

            if ((harq_ack0 == 1) && (harq_ack1 == 1) && (harq_ack2 == 1) && (harq_ack3 == 1)) {
              b[0] = 1;
              b[1] = 1;
              return(n1_pucch1);
            } else if ((harq_ack0 == 1) && (harq_ack1 == 1) && (harq_ack2 == 1) && ((harq_ack3 == 2) || (harq_ack3 == 0))) {
              b[0] = 1;
              b[1] = 0;
              return(n1_pucch1);
            } else if (((harq_ack0 == 0) || (harq_ack0 == 2)) && ((harq_ack1 == 2) || (harq_ack1 == 0)) && (harq_ack2 == 0) && (harq_ack3 == 2)) {
              b[0] = 1;
              b[1] = 1;
              return(n1_pucch2);
            } else if ((harq_ack0 == 1) && (harq_ack1 == 1) && ((harq_ack2 == 2) || (harq_ack2 == 0)) && (harq_ack3 == 1)) {
              b[0] = 1;
              b[1] = 0;
              return(n1_pucch1);
            } else if ((harq_ack0 == 0) && (harq_ack1 == 2) && (harq_ack2 == 2) && (harq_ack3 == 2)) {
              b[0] = 1;
              b[1] = 0;
              return(n1_pucch0);
            } else if ((harq_ack0 == 1) && (harq_ack1 == 1) && ((harq_ack2 == 2) || (harq_ack2 == 0)) && ((harq_ack3 == 2) || (harq_ack3 == 0))) {
              b[0] = 1;
              b[1] = 0;
              return(n1_pucch1);
            } else if ((harq_ack0 == 1) && ((harq_ack1 == 2) || (harq_ack1 == 0)) && (harq_ack2 == 1) && (harq_ack3 == 1)) {
              b[0] = 0;
              b[1] = 1;
              return(n1_pucch3);
            } else if (((harq_ack0 == 0) || (harq_ack0 == 2)) && ((harq_ack1 == 2) || (harq_ack1 == 0)) && ((harq_ack2 == 2) || (harq_ack2 == 0)) && (harq_ack3 == 0)) {
              b[0] = 1;
              b[1] = 1;
              return(n1_pucch3);
            } else if ((harq_ack0 == 1) && ((harq_ack1 == 2) || (harq_ack1 == 0)) && (harq_ack2 == 1) && ((harq_ack3 == 2) || (harq_ack3 == 0))) {
              b[0] = 0;
              b[1] = 1;
              return(n1_pucch2);
            } else if ((harq_ack0 == 1) && ((harq_ack1 == 2) || (harq_ack1 == 0)) && ((harq_ack2 == 2) || (harq_ack2 == 0)) && (harq_ack3 == 1)) {
              b[0] = 0;
              b[1] = 1;
              return(n1_pucch0);
            } else if ((harq_ack0 == 1) && ((harq_ack1 == 2) || (harq_ack1 == 0)) && ((harq_ack2 == 2) || (harq_ack2 == 0)) && ((harq_ack3 == 2) || (harq_ack3 == 0))) {
              b[0] = 0;
              b[1] = 1;
              return(n1_pucch0);
            } else if (((harq_ack0 == 2) || (harq_ack0 == 0)) && (harq_ack1 == 1) && (harq_ack2 == 1) && (harq_ack3 == 1)) {
              b[0] = 0;
              b[1] = 1;
              return(n1_pucch3);
            } else if (((harq_ack0 == 2) || (harq_ack0 == 0)) && (harq_ack1 == 0) && (harq_ack2 == 2) && (harq_ack3 == 2)) {
              b[0] = 0;
              b[1] = 0;
              return(n1_pucch1);
            } else if (((harq_ack0 == 2) || (harq_ack0 == 0)) && (harq_ack1 == 1) && (harq_ack2 == 1) && ((harq_ack3 == 2) || (harq_ack3 == 0))) {
              b[0] = 1;
              b[1] = 0;
              return(n1_pucch2);
            } else if (((harq_ack0 == 2) || (harq_ack0 == 0)) && (harq_ack1 == 1) && ((harq_ack2 == 2) || (harq_ack2 == 0)) && (harq_ack3 == 1)) {
              b[0] = 1;
              b[1] = 0;
              return(n1_pucch3);
            } else if (((harq_ack0 == 2) || (harq_ack0 == 0)) && (harq_ack1 == 1) && ((harq_ack2 == 2) || (harq_ack2 == 0)) && ((harq_ack3 == 2) || (harq_ack3 == 0))) {
              b[0] = 0;
              b[1] = 1;
              return(n1_pucch1);
            } else if (((harq_ack0 == 2) || (harq_ack0 == 0)) && ((harq_ack1 == 2) || (harq_ack1 == 0)) && (harq_ack2 == 1) && (harq_ack3 == 1)) {
              b[0] = 0;
              b[1] = 1;
              return(n1_pucch3);
            } else if (((harq_ack0 == 2) || (harq_ack0 == 0)) && ((harq_ack1 == 2) || (harq_ack1 == 0)) && (harq_ack2 == 1) && ((harq_ack3 == 2) || (harq_ack3 == 0))) {
              b[0] = 0;
              b[1] = 0;
              return(n1_pucch2);
            } else if (((harq_ack0 == 2) || (harq_ack0 == 0)) && ((harq_ack1 == 2) || (harq_ack1 == 0)) && (harq_ack3 == 1) && ((harq_ack2 == 2) || (harq_ack2 == 0))) {
              b[0] = 0;
              b[1] = 0;
              return(n1_pucch3);
            }
          } else if (subframe == 2) {
            if ((harq_ack0 == 1) && (harq_ack1 == 1) && (harq_ack2 == 1)) {
              b[0] = 1;
              b[1] = 1;
              return(n1_pucch2);
            } else if ((harq_ack0 == 1) && (harq_ack1 == 1) && ((harq_ack2 == 2) || (harq_ack2 == 0))) {
              b[0] = 1;
              b[1] = 1;
              return(n1_pucch1);
            } else if ((harq_ack0 == 1) && ((harq_ack1 == 2) || (harq_ack1 == 0)) && (harq_ack2 == 1)) {
              b[0] = 1;
              b[1] = 1;
              return(n1_pucch0);
            } else if ((harq_ack0 == 1) && ((harq_ack1 == 2) || (harq_ack1 == 0)) && ((harq_ack2 == 2) || (harq_ack2 == 0))) {
              b[0] = 0;
              b[1] = 1;
              return(n1_pucch0);
            } else if (((harq_ack0 == 2) || (harq_ack0 == 0)) && (harq_ack1 == 1) && (harq_ack2 == 1)) {
              b[0] = 1;
              b[1] = 0;
              return(n1_pucch2);
            } else if (((harq_ack0 == 2) || (harq_ack0 == 0)) && (harq_ack1 == 1) && ((harq_ack2 == 2) || (harq_ack2 == 0))) {
              b[1] = 0;
              b[0] = 0;
              return(n1_pucch1);
            } else if (((harq_ack0 == 2) || (harq_ack0 == 0)) && ((harq_ack1 == 2) || (harq_ack1 == 0)) && (harq_ack2 == 1)) {
              b[0] = 0;
              b[1] = 0;
              return(n1_pucch2);
            } else if ((harq_ack0 == 2) && (harq_ack1 == 2) && (harq_ack2 == 0)) {
              b[0] = 0;
              b[1] = 1;
              return(n1_pucch2);
            } else if ((harq_ack0 == 2) && (harq_ack1 == 0) && ((harq_ack2 == 2) || (harq_ack2 == 0))) {
              b[0] = 1;
              b[1] = 0;
              return(n1_pucch1);
            } else if ((harq_ack0 == 0) && ((harq_ack1 == 2) || (harq_ack1 == 0)) && ((harq_ack2 == 2) || (harq_ack2 == 0))) {
              b[0] = 1;
              b[1] = 0;
              return(n1_pucch0);
            }
          }
        } else if (SR==1) { // SR and 0,1,or 2 ACKS, (first 3 entries in Table 7.3-1 of 36.213)
          // this should be number of ACKs (including
          ack_counter = 0;

          if (harq_ack0==1)
            ack_counter ++;

          if (harq_ack1==1)
            ack_counter ++;

          if (harq_ack2==1)
            ack_counter ++;

          if (harq_ack3==1)
            ack_counter ++;

          switch (ack_counter) {
            case 0:
              b[0] = 0;
              b[1] = 0;
              break;

            case 1:
              b[0] = 1;
              b[1] = 1;
              break;

            case 2:
              b[0] = 1;
              b[1] = 0;
              break;

            case 3:
              b[0] = 0;
              b[1] = 1;
              break;

            case 4:
              b[0] = 1;
              b[1] = 1;
              break;
          }

          ack_counter = 0;
          return(ue->scheduling_request_config[eNB_id].sr_PUCCH_ResourceIndex);
        }

        break;
    }  // switch tdd_config
  }

  LOG_E(PHY,"[UE%d] : Frame %d phy_procedures_lte.c: get_n1pucch, exit without proper return\n", ue->Mod_id, proc->frame_tx);
  return(-1);
}


void ulsch_common_procedures(PHY_VARS_UE *ue,
                             UE_rxtx_proc_t *proc,
                             uint8_t empty_subframe) {
  int aa;
  LTE_DL_FRAME_PARMS *frame_parms=&ue->frame_parms;
  int nsymb;
  int subframe_tx = proc->subframe_tx;
  int ulsch_start;
  int overflow=0;
  int k, l;
  int dummy_tx_buffer[frame_parms->samples_per_tti] __attribute__((aligned(16)));
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_UE_TX_ULSCH_COMMON,VCD_FUNCTION_IN);

  if ( LOG_DEBUGFLAG(UE_TIMING)) {
    start_meas(&ue->ofdm_mod_stats);
  }

  nsymb = (frame_parms->Ncp == 0) ? 14 : 12;

  if (!IS_SOFTMODEM_BASICSIM) {
    ulsch_start = (ue->rx_offset+subframe_tx*frame_parms->samples_per_tti-
                   ue->hw_timing_advance-
                   ue->timing_advance-
                   ue->N_TA_offset+5);

    if(ulsch_start < 0)
      ulsch_start = ulsch_start + (LTE_NUMBER_OF_SUBFRAMES_PER_FRAME*frame_parms->samples_per_tti);

    if (ulsch_start > (LTE_NUMBER_OF_SUBFRAMES_PER_FRAME*frame_parms->samples_per_tti))
      ulsch_start = ulsch_start % (LTE_NUMBER_OF_SUBFRAMES_PER_FRAME*frame_parms->samples_per_tti);
  } else { //this is the simulators case
    ulsch_start = (frame_parms->samples_per_tti*subframe_tx)-ue->N_TA_offset; //-ue->timing_advance;
  }

  if (empty_subframe) {
    overflow = ulsch_start - 9*frame_parms->samples_per_tti;

    for (aa=0; aa<frame_parms->nb_antennas_tx; aa++) {
      if (overflow > 0) {
        memset(&ue->common_vars.txdata[aa][ulsch_start],0,4*(frame_parms->samples_per_tti-overflow));
        memset(&ue->common_vars.txdata[aa][0],0,4*overflow);
      } else {
        memset(&ue->common_vars.txdata[aa][ulsch_start],0,4*frame_parms->samples_per_tti);
      }
    }

    return;
  }

  for (aa=0; aa<frame_parms->nb_antennas_tx; aa++) {
    int *Buff = IS_SOFTMODEM_BASICSIM ? &ue->common_vars.txdata[aa][ulsch_start] :dummy_tx_buffer;

    if (frame_parms->Ncp == 1) {
      PHY_ofdm_mod(&ue->common_vars.txdataF[aa][subframe_tx*nsymb*frame_parms->ofdm_symbol_size],
                   Buff,
                   frame_parms->ofdm_symbol_size,
                   nsymb,
                   frame_parms->nb_prefix_samples,
                   CYCLIC_PREFIX);
    } else {
      normal_prefix_mod(&ue->common_vars.txdataF[aa][subframe_tx*nsymb*frame_parms->ofdm_symbol_size],
                        Buff,
                        nsymb>>1,
                        &ue->frame_parms);
      Buff += (frame_parms->samples_per_tti>>1);
      normal_prefix_mod(&ue->common_vars.txdataF[aa][((subframe_tx*nsymb)+(nsymb>>1))*frame_parms->ofdm_symbol_size],
                        Buff,
                        nsymb>>1,
                        &ue->frame_parms);
    }

    if (IS_SOFTMODEM_BASICSIM) {
      apply_7_5_kHz(ue,&ue->common_vars.txdata[aa][ulsch_start],0);
      apply_7_5_kHz(ue,&ue->common_vars.txdata[aa][ulsch_start],1);
    } else {
      apply_7_5_kHz(ue,dummy_tx_buffer,0);
      apply_7_5_kHz(ue,dummy_tx_buffer,1);
    }

    if (!(IS_SOFTMODEM_BASICSIM) ) {
      overflow = ulsch_start - 9*frame_parms->samples_per_tti;

      for (k=ulsch_start,l=0; k<cmin(frame_parms->samples_per_tti*LTE_NUMBER_OF_SUBFRAMES_PER_FRAME,ulsch_start+frame_parms->samples_per_tti); k++,l++) {
        ((short *)ue->common_vars.txdata[aa])[2*k] = ((short *)dummy_tx_buffer)[2*l];
        ((short *)ue->common_vars.txdata[aa])[2*k+1] = ((short *)dummy_tx_buffer)[2*l+1];
      }

      for (k=0; k<overflow; k++,l++) {
        ((short *)ue->common_vars.txdata[aa])[2*k] = ((short *)dummy_tx_buffer)[2*l];
        ((short *)ue->common_vars.txdata[aa])[2*k+1] = ((short *)dummy_tx_buffer)[2*l+1];
      }

#if defined(EXMIMO)

      // handle switch before 1st TX subframe, guarantee that the slot prior to transmission is switch on
      for (k=ulsch_start - (frame_parms->samples_per_tti>>1) ; k<ulsch_start ; k++) {
        if (k<0)
          ue->common_vars.txdata[aa][k+frame_parms->samples_per_tti*LTE_NUMBER_OF_SUBFRAMES_PER_FRAME] &= 0xFFFEFFFE;
        else if (k>(frame_parms->samples_per_tti*LTE_NUMBER_OF_SUBFRAMES_PER_FRAME))
          ue->common_vars.txdata[aa][k-frame_parms->samples_per_tti*LTE_NUMBER_OF_SUBFRAMES_PER_FRAME] &= 0xFFFEFFFE;
        else
          ue->common_vars.txdata[aa][k] &= 0xFFFEFFFE;
      }

#endif
    }

    /*
    only for debug
    LOG_I(PHY,"ul-signal [subframe: %d, ulsch_start %d, TA: %d, rxOffset: %d, timing_advance: %d, hw_timing_advance: %d]\n",subframe_tx, ulsch_start, ue->N_TA_offset, ue->rx_offset, ue->timing_advance, ue->hw_timing_advance);
    if( (crash == 1) && (subframe_tx == 0) )
    {
      LOG_E(PHY,"***** DUMP TX Signal [ulsch_start %d] *****\n",ulsch_start);
      LOG_M("txBuff.m","txSignal",&ue->common_vars.txdata[aa][ulsch_start],frame_parms->samples_per_tti,1,1);
    }
    */
  } //nb_antennas_tx

  if ( LOG_DEBUGFLAG(UE_TIMING)) {
    stop_meas(&ue->ofdm_mod_stats);
  }

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_UE_TX_ULSCH_COMMON,VCD_FUNCTION_OUT);
}

void ue_prach_procedures(PHY_VARS_UE *ue,
                         UE_rxtx_proc_t *proc,
                         uint8_t eNB_id,
                         uint8_t abstraction_flag,
                         runmode_t mode) {
  int frame_tx = proc->frame_tx;
  int subframe_tx = proc->subframe_tx;
  LOG_USEDINLOG_VAR(int, prach_power);
  PRACH_RESOURCES_t prach_resources_local;
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_UE_TX_PRACH, VCD_FUNCTION_IN);
  ue->generate_prach=0;

  if (ue->mac_enabled==0) {
    ue->prach_resources[eNB_id] = &prach_resources_local;
    prach_resources_local.ra_RNTI = 0xbeef;
    prach_resources_local.ra_PreambleIndex = 0;
  }

  if (ue->mac_enabled==1) {
    // ask L2 for RACH transport
    if ((mode != rx_calib_ue) && (mode != rx_calib_ue_med) && (mode != rx_calib_ue_byp) && (mode != no_L2_connect) ) {
      //LOG_D(PHY,"Getting PRACH resources\n");
      ue->prach_resources[eNB_id] = ue_get_rach(ue->Mod_id,
                                    ue->CC_id,
                                    frame_tx,
                                    eNB_id,
                                    subframe_tx);
      LOG_D(PHY,"Prach resources %p\n",ue->prach_resources[eNB_id]);
    }
  }

  if (ue->prach_resources[eNB_id]!=NULL) {
    ue->generate_prach=1;
    ue->prach_cnt=0;
#ifdef SMBV
    ue->prach_resources[eNB_id]->ra_PreambleIndex = 19;
#endif
    LOG_I(PHY,"mode %d\n",mode);

    if ((ue->mac_enabled==1) && (mode != calib_prach_tx)) {
      ue->tx_power_dBm[subframe_tx] = ue->prach_resources[eNB_id]->ra_PREAMBLE_RECEIVED_TARGET_POWER+get_PL(ue->Mod_id,ue->CC_id,eNB_id);
    } else {
      ue->tx_power_dBm[subframe_tx] = ue->tx_power_max_dBm;
      ue->prach_resources[eNB_id]->ra_PreambleIndex = 19;
    }

    LOG_D(PHY,"[UE  %d][RAPROC] Frame %d, Subframe %d : Generating PRACH, preamble %d,PL %d,  P0_PRACH %d, TARGET_RECEIVED_POWER %d dBm, PRACH TDD Resource index %d, RA-RNTI %d\n",
          ue->Mod_id,
          frame_tx,
          subframe_tx,
          ue->prach_resources[eNB_id]->ra_PreambleIndex,
          get_PL(ue->Mod_id,ue->CC_id,eNB_id),
          ue->tx_power_dBm[subframe_tx],
          ue->prach_resources[eNB_id]->ra_PREAMBLE_RECEIVED_TARGET_POWER,
          ue->prach_resources[eNB_id]->ra_TDD_map_index,
          ue->prach_resources[eNB_id]->ra_RNTI);
    ue->tx_total_RE[subframe_tx] = 96;

    if (IS_SOFTMODEM_BASICSIM) {
      ue->prach_vars[eNB_id]->amp = get_tx_amp(ue->tx_power_dBm[subframe_tx],
                                    ue->tx_power_max_dBm,
                                    ue->frame_parms.N_RB_UL,
                                    6);
    } else {
      ue->prach_vars[eNB_id]->amp = AMP;
    }

    if ((mode == calib_prach_tx) && (((proc->frame_tx&0xfffe)%100)==0))
      LOG_D(PHY,"[UE  %d][RAPROC] Frame %d, Subframe %d : PRACH TX power %d dBm, amp %d\n",
            ue->Mod_id,
            proc->frame_rx,
            proc->subframe_tx,
            ue->tx_power_dBm[subframe_tx],
            ue->prach_vars[eNB_id]->amp);

    //      start_meas(&ue->tx_prach);
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_GENERATE_PRACH, VCD_FUNCTION_IN);
    prach_power = generate_prach(ue,eNB_id,subframe_tx,frame_tx);
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_GENERATE_PRACH, VCD_FUNCTION_OUT);
    //      stop_meas(&ue->tx_prach);
    LOG_I(PHY,"[UE  %d][RAPROC] PRACH PL %d dB, power %d dBm (max %d dBm), digital power %d dB (amp %d)\n",
          ue->Mod_id,
          get_PL(ue->Mod_id,ue->CC_id,eNB_id),
          ue->tx_power_dBm[subframe_tx],
          ue->tx_power_max_dBm,
          dB_fixed(prach_power),
          ue->prach_vars[eNB_id]->amp);

    if (ue->mac_enabled==1) {
      Msg1_transmitted(ue->Mod_id,
                       ue->CC_id,
                       frame_tx,
                       eNB_id);
    }

    LOG_I(PHY,"[UE  %d][RAPROC] Frame %d, subframe %d: Generating PRACH (eNB %d) preamble index %d for UL, TX power %d dBm (PL %d dB), l3msg \n",
          ue->Mod_id,frame_tx,subframe_tx,eNB_id,
          ue->prach_resources[eNB_id]->ra_PreambleIndex,
          ue->prach_resources[eNB_id]->ra_PREAMBLE_RECEIVED_TARGET_POWER+get_PL(ue->Mod_id,ue->CC_id,eNB_id),
          get_PL(ue->Mod_id,ue->CC_id,eNB_id));

    // if we're calibrating the PRACH kill the pointer to its resources so that the RA protocol doesn't continue
    if (mode == calib_prach_tx)
      ue->prach_resources[eNB_id]=NULL;

    LOG_D(PHY,"[UE %d] frame %d subframe %d : generate_prach %d, prach_cnt %d\n",
          ue->Mod_id,frame_tx,subframe_tx,ue->generate_prach,ue->prach_cnt);
    ue->prach_cnt++;

    if (ue->prach_cnt==3)
      ue->generate_prach=0;
  }

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_UE_TX_PRACH, VCD_FUNCTION_OUT);
}

void ue_ulsch_uespec_procedures(PHY_VARS_UE *ue,
                                UE_rxtx_proc_t *proc,
                                uint8_t eNB_id,
                                uint8_t abstraction_flag) {
  int harq_pid;
  int frame_tx=proc->frame_tx;
  int subframe_tx=proc->subframe_tx;
  int Mod_id = ue->Mod_id;
  int CC_id = ue->CC_id;
  uint8_t Msg3_flag=0;
  uint16_t first_rb, nb_rb;
  unsigned int input_buffer_length;
  int i;
  int aa;
  int tx_amp;
  uint8_t ulsch_input_buffer[5477] __attribute__ ((aligned(32)));
  uint8_t access_mode;
  uint8_t Nbundled=0;
  uint8_t NbundledCw1=0;
  uint8_t ack_status_cw0=0;
  uint8_t ack_status_cw1=0;
  uint8_t cqi_status = 0;
  uint8_t ri_status  = 0;
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_UE_TX_ULSCH_UESPEC,VCD_FUNCTION_IN);
  // get harq_pid from subframe relationship
  harq_pid = subframe2harq_pid(&ue->frame_parms,
                               frame_tx,
                               subframe_tx);
  LOG_D(PHY,"Frame %d, Subframe %d : ue_uespec_procedures, harq_pid %d => subframe_scheduling %d\n",
        frame_tx,subframe_tx,harq_pid,
        ue->ulsch[eNB_id]->harq_processes[harq_pid]->subframe_scheduling_flag);

  if (ue->mac_enabled == 1) {
    if ((ue->ulsch_Msg3_active[eNB_id] == 1)       &&
        (ue->ulsch_Msg3_frame[eNB_id] == frame_tx) &&
        (ue->ulsch_Msg3_subframe[eNB_id] == subframe_tx)) { // Initial Transmission of Msg3
      ue->ulsch[eNB_id]->harq_processes[harq_pid]->subframe_scheduling_flag = 1;

      if (ue->ulsch[eNB_id]->harq_processes[harq_pid]->round==0)
        generate_ue_ulsch_params_from_rar(ue,
                                          proc,
                                          eNB_id);

      ue->ulsch[eNB_id]->power_offset = 14;
      LOG_D(PHY,"[UE  %d][RAPROC] Frame %d: Setting Msg3_flag in subframe %d, for harq_pid %d\n",
            Mod_id,
            frame_tx,
            subframe_tx,
            harq_pid);
      Msg3_flag = 1;
    } else {
      AssertFatal(harq_pid!=255,"[UE%d] Frame %d subframe %d ulsch_decoding.c: FATAL ERROR: illegal harq_pid, exiting\n",
                  Mod_id,frame_tx, subframe_tx);
      Msg3_flag=0;
    }
  }

  if (ue->ulsch[eNB_id]->harq_processes[harq_pid]->subframe_scheduling_flag == 1) {
    uint8_t isBad = 0;

    if (ue->frame_parms.N_RB_UL <= ue->ulsch[eNB_id]->harq_processes[harq_pid]->first_rb) {
      LOG_D(PHY,"Invalid PUSCH first_RB=%d for N_RB_UL=%d\n",
            ue->ulsch[eNB_id]->harq_processes[harq_pid]->first_rb,
            ue->frame_parms.N_RB_UL);
      isBad = 1;
    }

    if (ue->frame_parms.N_RB_UL < ue->ulsch[eNB_id]->harq_processes[harq_pid]->nb_rb) {
      LOG_D(PHY,"Invalid PUSCH num_RB=%d for N_RB_UL=%d\n",
            ue->ulsch[eNB_id]->harq_processes[harq_pid]->nb_rb,
            ue->frame_parms.N_RB_UL);
      isBad = 1;
    }

    if (0 > ue->ulsch[eNB_id]->harq_processes[harq_pid]->first_rb) {
      LOG_D(PHY,"Invalid PUSCH first_RB=%d\n",
            ue->ulsch[eNB_id]->harq_processes[harq_pid]->first_rb);
      isBad = 1;
    }

    if (0 >= ue->ulsch[eNB_id]->harq_processes[harq_pid]->nb_rb) {
      LOG_D(PHY,"Invalid PUSCH num_RB=%d\n",
            ue->ulsch[eNB_id]->harq_processes[harq_pid]->nb_rb);
      isBad = 1;
    }

    if (ue->frame_parms.N_RB_UL < (ue->ulsch[eNB_id]->harq_processes[harq_pid]->nb_rb + ue->ulsch[eNB_id]->harq_processes[harq_pid]->first_rb)) {
      LOG_D(PHY,"Invalid PUSCH num_RB=%d + first_RB=%d for N_RB_UL=%d\n",
            ue->ulsch[eNB_id]->harq_processes[harq_pid]->nb_rb,
            ue->ulsch[eNB_id]->harq_processes[harq_pid]->first_rb,
            ue->frame_parms.N_RB_UL);
      isBad = 1;
    }

    if ((0 > ue->ulsch[eNB_id]->harq_processes[harq_pid]->rvidx) ||
        (3 < ue->ulsch[eNB_id]->harq_processes[harq_pid]->rvidx)) {
      LOG_D(PHY,"Invalid PUSCH RV index=%d\n", ue->ulsch[eNB_id]->harq_processes[harq_pid]->rvidx);
      isBad = 1;
    }

    if (isBad) {
      LOG_I(PHY,"Skip PUSCH generation!\n");
      ue->ulsch[eNB_id]->harq_processes[harq_pid]->subframe_scheduling_flag = 0;
    }
  }

  if (ue->ulsch[eNB_id]->harq_processes[harq_pid]->subframe_scheduling_flag == 1) {
    ue->generate_ul_signal[eNB_id] = 1;
    // deactivate service request
    // ue->ulsch[eNB_id]->harq_processes[harq_pid]->subframe_scheduling_flag = 0;
    LOG_D(PHY,"Generating PUSCH (Abssubframe: %d.%d): harq-Id: %d, round: %d, MaxReTrans: %d \n",frame_tx,subframe_tx,harq_pid,ue->ulsch[eNB_id]->harq_processes[harq_pid]->round,
          ue->ulsch[eNB_id]->Mlimit);

    if (ue->ulsch[eNB_id]->harq_processes[harq_pid]->round >= (ue->ulsch[eNB_id]->Mlimit - 1)) {
      //        LOG_D(PHY,"PUSCH MAX Retransmission achieved ==> send last pusch\n");
      ue->ulsch[eNB_id]->harq_processes[harq_pid]->subframe_scheduling_flag = 0;
      ue->ulsch[eNB_id]->harq_processes[harq_pid]->round  = 0;
    }

    ack_status_cw0 = reset_ack(&ue->frame_parms,
                               ue->dlsch[ue->current_thread_id[proc->subframe_rx]][eNB_id][0]->harq_ack,
                               subframe_tx,
                               proc->subframe_rx,
                               ue->ulsch[eNB_id]->o_ACK,
                               &Nbundled,
                               0);
    ack_status_cw1 = reset_ack(&ue->frame_parms,
                               ue->dlsch[ue->current_thread_id[proc->subframe_rx]][eNB_id][1]->harq_ack,
                               subframe_tx,
                               proc->subframe_rx,
                               ue->ulsch[eNB_id]->o_ACK,
                               &NbundledCw1,
                               1);
    //Nbundled = ue->dlsch[ue->current_thread_id[proc->subframe_rx]][eNB_id][0]->harq_ack;
    //ue->ulsch[eNB_id]->bundling = Nbundled;
    first_rb = ue->ulsch[eNB_id]->harq_processes[harq_pid]->first_rb;
    nb_rb = ue->ulsch[eNB_id]->harq_processes[harq_pid]->nb_rb;
    // check Periodic CQI/RI reporting
    cqi_status = ((ue->cqi_report_config[eNB_id].CQI_ReportPeriodic.cqi_PMI_ConfigIndex>0)&&
                  (is_cqi_TXOp(ue,proc,eNB_id)==1));
    ri_status = ((ue->cqi_report_config[eNB_id].CQI_ReportPeriodic.ri_ConfigIndex>0) &&
                 (is_ri_TXOp(ue,proc,eNB_id)==1));
    // compute CQI/RI resources
    compute_cqi_ri_resources(ue, ue->ulsch[eNB_id], eNB_id, ue->ulsch[eNB_id]->rnti, P_RNTI, CBA_RNTI, cqi_status, ri_status);

    if (ack_status_cw0 > 0) {
      // check if we received a PDSCH at subframe_tx - 4
      // ==> send ACK/NACK on PUSCH
      if (ue->frame_parms.frame_type == FDD) {
        ue->ulsch[eNB_id]->harq_processes[harq_pid]->O_ACK = ack_status_cw0 + ack_status_cw1;
      }

      if (ue->ulsch[eNB_id]->o_ACK[0]) {
        T(T_UE_PHY_DLSCH_UE_ACK, T_INT(eNB_id), T_INT(frame_tx%1024), T_INT(subframe_tx), T_INT(ue->dlsch[ue->current_thread_id[proc->subframe_rx]][eNB_id][0]->rnti),
          T_INT(ue->dlsch[ue->current_thread_id[proc->subframe_rx]][eNB_id][0]->current_harq_pid));
      } else {
        T(T_UE_PHY_DLSCH_UE_NACK, T_INT(eNB_id), T_INT(frame_tx%1024), T_INT(subframe_tx), T_INT(ue->dlsch[ue->current_thread_id[proc->subframe_rx]][eNB_id][0]->rnti),
          T_INT(ue->dlsch[ue->current_thread_id[proc->subframe_rx]][eNB_id][0]->current_harq_pid));
      }

      if ( LOG_DEBUGFLAG(DEBUG_UE_PHYPROC)) {
        if(ue->ulsch[eNB_id]->o_ACK[0]) {
          LOG_I(PHY,"PUSCH ACK\n");
        } else {
          LOG_I(PHY,"PUSCH NACK\n");
        }

        LOG_I(PHY,"[UE  %d][PDSCH %x] AbsSubFrame %d.%d Generating ACK (%d,%d) for %d bits on PUSCH\n",
              Mod_id,
              ue->ulsch[eNB_id]->rnti,
              frame_tx%1024,subframe_tx,
              ue->ulsch[eNB_id]->o_ACK[0],ue->ulsch[eNB_id]->o_ACK[1],
              ue->ulsch[eNB_id]->harq_processes[harq_pid]->O_ACK);
      }
    }

    if ( LOG_DEBUGFLAG(DEBUG_UE_PHYPROC)) {
      LOG_D(PHY,
            "[UE  %d][PUSCH %d] AbsSubframe %d.%d Generating PUSCH : first_rb %d, nb_rb %d, round %d, mcs %d, rv %d, "
            "cyclic_shift %d (cyclic_shift_common %d,n_DMRS2 %d,n_PRS %d), ACK (%d,%d), O_ACK %d, ack_status_cw0 %d ack_status_cw1 %d bundling %d, Nbundled %d, CQI %d, RI %d\n",
            Mod_id,harq_pid,frame_tx%1024,subframe_tx,
            first_rb,nb_rb,
            ue->ulsch[eNB_id]->harq_processes[harq_pid]->round,
            ue->ulsch[eNB_id]->harq_processes[harq_pid]->mcs,
            ue->ulsch[eNB_id]->harq_processes[harq_pid]->rvidx,
            (ue->frame_parms.pusch_config_common.ul_ReferenceSignalsPUSCH.cyclicShift+
             ue->ulsch[eNB_id]->harq_processes[harq_pid]->n_DMRS2+
             ue->frame_parms.pusch_config_common.ul_ReferenceSignalsPUSCH.nPRS[subframe_tx<<1])%12,
            ue->frame_parms.pusch_config_common.ul_ReferenceSignalsPUSCH.cyclicShift,
            ue->ulsch[eNB_id]->harq_processes[harq_pid]->n_DMRS2,
            ue->frame_parms.pusch_config_common.ul_ReferenceSignalsPUSCH.nPRS[subframe_tx<<1],
            ue->ulsch[eNB_id]->o_ACK[0],ue->ulsch[eNB_id]->o_ACK[1],
            ue->ulsch[eNB_id]->harq_processes[harq_pid]->O_ACK,
            ack_status_cw0,
            ack_status_cw1,
            ue->ulsch[eNB_id]->bundling, Nbundled,
            cqi_status,
            ri_status);
    }

    if (Msg3_flag == 1) {
      LOG_I(PHY,"[UE  %d][RAPROC] Frame %d, Subframe %d Generating (RRCConnectionRequest) Msg3 (nb_rb %d, first_rb %d, round %d, rvidx %d) Msg3: %x.%x.%x|%x.%x.%x.%x.%x.%x\n",Mod_id,frame_tx,
            subframe_tx,
            ue->ulsch[eNB_id]->harq_processes[harq_pid]->nb_rb,
            ue->ulsch[eNB_id]->harq_processes[harq_pid]->first_rb,
            ue->ulsch[eNB_id]->harq_processes[harq_pid]->round,
            ue->ulsch[eNB_id]->harq_processes[harq_pid]->rvidx,
            ue->prach_resources[eNB_id]->Msg3[0],
            ue->prach_resources[eNB_id]->Msg3[1],
            ue->prach_resources[eNB_id]->Msg3[2],
            ue->prach_resources[eNB_id]->Msg3[3],
            ue->prach_resources[eNB_id]->Msg3[4],
            ue->prach_resources[eNB_id]->Msg3[5],
            ue->prach_resources[eNB_id]->Msg3[6],
            ue->prach_resources[eNB_id]->Msg3[7],
            ue->prach_resources[eNB_id]->Msg3[8]);

      if ( LOG_DEBUGFLAG(UE_TIMING)) {
        start_meas(&ue->ulsch_encoding_stats);
      }

      AssertFatal(ulsch_encoding(ue->prach_resources[eNB_id]->Msg3,
                                 ue,
                                 harq_pid,
                                 eNB_id,
                                 proc->subframe_rx,
                                 ue->transmission_mode[eNB_id],0,0)==0,
                  "ulsch_coding.c: FATAL ERROR: returning\n");

      if ( LOG_DEBUGFLAG(UE_TIMING)) {
        stop_meas(&ue->phy_proc_tx);
        LOG_I(PHY,"------FULL TX PROC : %5.2f ------\n",ue->phy_proc_tx.p_time/(cpuf*1000.0));
        stop_meas(&ue->ulsch_encoding_stats);
      }

      if (ue->mac_enabled == 1) {
        // signal MAC that Msg3 was sent
        Msg3_transmitted(Mod_id,
                         CC_id,
                         frame_tx,
                         eNB_id);
      }

      LOG_I(PHY,"Done Msg3 encoding\n");
    } // Msg3_flag==1
    else {// Msg3_flag==0
      input_buffer_length = ue->ulsch[eNB_id]->harq_processes[harq_pid]->TBS/8;

      if (ue->mac_enabled==1) {
        //  LOG_D(PHY,"[UE  %d] ULSCH : Searching for MAC SDUs\n",Mod_id);
        if (ue->ulsch[eNB_id]->harq_processes[harq_pid]->round==0) {
          //if (ue->ulsch[eNB_id]->harq_processes[harq_pid]->calibration_flag == 0) {
          access_mode=SCHEDULED_ACCESS;
          ue_get_sdu(Mod_id,
                     CC_id,
                     frame_tx,
                     subframe_tx,
                     eNB_id,
                     ulsch_input_buffer,
                     input_buffer_length,
                     &access_mode);
        }

        if (LOG_DEBUGFLAG(DEBUG_UE_PHYPROC)) {
          LOG_D(PHY,"[UE] Frame %d, subframe %d : ULSCH SDU (TX harq_pid %d)  (%d bytes) : \n",frame_tx,subframe_tx,harq_pid, ue->ulsch[eNB_id]->harq_processes[harq_pid]->TBS>>3);

          for (i=0; i<ue->ulsch[eNB_id]->harq_processes[harq_pid]->TBS>>3; i++)
            LOG_T(PHY,"%x.",ulsch_input_buffer[i]);

          LOG_T(PHY,"\n");
        }
      } else {
        unsigned int taus(void);

        for (i=0; i<input_buffer_length; i++)
          ulsch_input_buffer[i]= (uint8_t)(taus()&0xff);
      }

      if ( LOG_DEBUGFLAG(UE_TIMING)) {
        start_meas(&ue->ulsch_encoding_stats);
      }

      if (abstraction_flag==0) {
        if (ulsch_encoding(ulsch_input_buffer,
                           ue,
                           harq_pid,
                           eNB_id,
                           proc->subframe_rx,
                           ue->transmission_mode[eNB_id],0,
                           Nbundled)!=0) {
          LOG_E(PHY,"ulsch_coding.c: FATAL ERROR: returning\n");
          VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_UE_TX, VCD_FUNCTION_OUT);

          if (LOG_DEBUGFLAG(UE_TIMING)) {
            stop_meas(&ue->phy_proc_tx);
          }

          return;
        }
      }

      if(LOG_DEBUGFLAG(UE_TIMING)) {
        stop_meas(&ue->ulsch_encoding_stats);
      }
    }

    if (abstraction_flag == 0) {
      if (ue->mac_enabled==1) {
        pusch_power_cntl(ue,proc,eNB_id,1, abstraction_flag);
        ue->tx_power_dBm[subframe_tx] = ue->ulsch[eNB_id]->Po_PUSCH;
      } else {
        ue->tx_power_dBm[subframe_tx] = ue->tx_power_max_dBm;
      }

      ue->tx_total_RE[subframe_tx] = nb_rb*12;

      if (IS_SOFTMODEM_BASICSIM) {
        tx_amp = AMP;
      } else {
        tx_amp = get_tx_amp(ue->tx_power_dBm[subframe_tx],
                            ue->tx_power_max_dBm,
                            ue->frame_parms.N_RB_UL,
                            nb_rb);
      }

      T(T_UE_PHY_PUSCH_TX_POWER, T_INT(eNB_id), T_INT(frame_tx%1024), T_INT(subframe_tx),T_INT(ue->tx_power_dBm[subframe_tx]),
        T_INT(tx_amp),T_INT(ue->ulsch[eNB_id]->f_pusch),T_INT(get_PL(Mod_id,0,eNB_id)),T_INT(nb_rb));

      if (LOG_DEBUGFLAG(DEBUG_UE_PHYPROC)) {
        LOG_D(PHY,"[UE  %d][PUSCH %d] AbsSubFrame %d.%d, generating PUSCH, Po_PUSCH: %d dBm (max %d dBm), amp %d\n",
              Mod_id,harq_pid,frame_tx%1024,subframe_tx,ue->tx_power_dBm[subframe_tx],ue->tx_power_max_dBm, tx_amp);
      }

      if (LOG_DEBUGFLAG(UE_TIMING)) {
        start_meas(&ue->ulsch_modulation_stats);
      }

      ulsch_modulation(ue->common_vars.txdataF,
                       tx_amp,
                       frame_tx,
                       subframe_tx,
                       &ue->frame_parms,
                       ue->ulsch[eNB_id]);

      for (aa=0; aa<1/*frame_parms->nb_antennas_tx*/; aa++)
        generate_drs_pusch(ue,
                           proc,
                           (LTE_DL_FRAME_PARMS *)NULL,
                           (int32_t **)NULL,
                           eNB_id,
                           tx_amp,
                           subframe_tx,
                           first_rb,
                           nb_rb,
                           aa);

      if (LOG_DEBUGFLAG(UE_TIMING)) {
        stop_meas(&ue->ulsch_modulation_stats);
      }
    }

    if (abstraction_flag==1) {
      // clear SR
      ue->sr[subframe_tx]=0;
    }
  } // subframe_scheduling_flag==1

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_UE_TX_ULSCH_UESPEC,VCD_FUNCTION_OUT);
}

void ue_srs_procedures(PHY_VARS_UE *ue,
                       UE_rxtx_proc_t *proc,
                       uint8_t eNB_id,
                       uint8_t abstraction_flag) {
  //LTE_DL_FRAME_PARMS *frame_parms = &ue->frame_parms;
  //int8_t  frame_tx    = proc->frame_tx;
  int8_t  subframe_tx = proc->subframe_tx;
  int16_t tx_amp;
  int16_t Po_SRS;
  uint8_t nb_rb_srs;
  SOUNDINGRS_UL_CONFIG_DEDICATED *pSoundingrs_ul_config_dedicated=&ue->soundingrs_ul_config_dedicated[eNB_id];
  uint8_t isSrsTxOccasion = pSoundingrs_ul_config_dedicated->srsUeSubframe;

  if(isSrsTxOccasion) {
    ue->generate_ul_signal[eNB_id] = 1;

    if (ue->mac_enabled==1) {
      srs_power_cntl(ue,proc,eNB_id, (uint8_t *)(&nb_rb_srs), abstraction_flag);
      Po_SRS = ue->ulsch[eNB_id]->Po_SRS;
    } else {
      Po_SRS = ue->tx_power_max_dBm;
    }

    if (IS_SOFTMODEM_BASICSIM) {
      tx_amp = AMP;
    } else {
      if (ue->mac_enabled==1) {
        tx_amp = get_tx_amp(Po_SRS,
                            ue->tx_power_max_dBm,
                            ue->frame_parms.N_RB_UL,
                            nb_rb_srs);
      } else {
        tx_amp = AMP;
      }
    }

    LOG_D(PHY,"SRS PROC; TX_MAX_POWER %d, Po_SRS %d, NB_RB_UL %d, NB_RB_SRS %d TX_AMPL %d\n",ue->tx_power_max_dBm,
          Po_SRS,
          ue->frame_parms.N_RB_UL,
          nb_rb_srs,
          tx_amp);
    uint16_t nsymb = (ue->frame_parms.Ncp==0) ? 14:12;
    uint16_t symbol_offset = (int)ue->frame_parms.ofdm_symbol_size*((subframe_tx*nsymb)+(nsymb-1));
    generate_srs(&ue->frame_parms,
                 &ue->soundingrs_ul_config_dedicated[eNB_id],
                 &ue->common_vars.txdataF[eNB_id][symbol_offset],
                 tx_amp,
                 subframe_tx);
  }
}

int16_t get_pucch2_cqi(PHY_VARS_UE *ue,
                       int eNB_id,
                       int *len) {
  if ((ue->transmission_mode[eNB_id]<4)||
      (ue->transmission_mode[eNB_id]==7)) { // Mode 1-0 feedback
    // 4-bit CQI message
    /*LOG_I(PHY,"compute CQI value, TM %d, length 4, Cqi Avg %d, value %d \n", ue->transmission_mode[eNB_id],
                    ue->measurements.wideband_cqi_avg[eNB_id],
                    sinr2cqi((double)ue->measurements.wideband_cqi_avg[eNB_id],
                              ue->transmission_mode[eNB_id]));*/
    *len=4;
    return(sinr2cqi((double)ue->measurements.wideband_cqi_avg[eNB_id],
                    ue->transmission_mode[eNB_id]));
  } else { // Mode 1-1 feedback, later
    //LOG_I(PHY,"compute CQI value, TM %d, length 0, Cqi Avg 0 \n", ue->transmission_mode[eNB_id]);
    *len=0;
    // 2-antenna ports RI=1, 6 bits (2 PMI, 4 CQI)
    // 2-antenna ports RI=2, 8 bits (1 PMI, 7 CQI/DIFF CQI)
    return(0);
  }
}


int16_t get_pucch2_ri(PHY_VARS_UE *ue,int eNB_id) {
  return(1);
}


void get_pucch_param(PHY_VARS_UE    *ue,
                     UE_rxtx_proc_t *proc,
                     uint8_t        *ack_payload,
                     PUCCH_FMT_t    format,
                     uint8_t        eNB_id,
                     uint8_t        SR,
                     uint8_t        cqi_report,
                     uint16_t       *pucch_resource,
                     uint8_t        *pucch_payload,
                     uint16_t       *plength) {
  switch (format) {
    case pucch_format1: {
      pucch_resource[0] = ue->scheduling_request_config[eNB_id].sr_PUCCH_ResourceIndex;
      pucch_payload[0]  = 0; // payload is ignored in case of format1
      pucch_payload[1]  = 0; // payload is ignored in case of format1
    }
    break;

    case pucch_format1a:
    case pucch_format1b:
    case pucch_format1b_csA2:
    case pucch_format1b_csA3:
    case pucch_format1b_csA4: {
      pucch_resource[0] = get_n1_pucch(ue,
                                       proc,
                                       ue->dlsch[ue->current_thread_id[proc->subframe_rx]][eNB_id][0]->harq_ack,
                                       eNB_id,
                                       ack_payload,
                                       SR);
      pucch_payload[0]  = ack_payload[0];
      pucch_payload[1]  = ack_payload[1];
      //pucch_payload[1]  = 1;
    }
    break;

    case pucch_format2: {
      pucch_resource[0]    = ue->cqi_report_config[eNB_id].CQI_ReportPeriodic.cqi_PUCCH_ResourceIndex;

      if(cqi_report) {
        pucch_payload[0] = get_pucch2_cqi(ue,eNB_id,(int *)plength);
      } else {
        *plength = 1;
        pucch_payload[0] = get_pucch2_ri(ue,eNB_id);
      }
    }
    break;

    case pucch_format2a:
    case pucch_format2b:
      LOG_E(PHY,"NO Resource available for PUCCH 2a/2b \n");
      break;

    case pucch_format3:
      fprintf(stderr, "PUCCH format 3 not handled\n");
      abort();
  }
}

void ue_pucch_procedures(PHY_VARS_UE *ue,
                         UE_rxtx_proc_t *proc,
                         uint8_t eNB_id,
                         uint8_t abstraction_flag) {
  uint8_t  pucch_ack_payload[2];
  uint16_t pucch_resource;
  ANFBmode_t bundling_flag;
  PUCCH_FMT_t format;
  uint8_t  SR_payload;
  uint8_t  pucch_payload[2];
  uint16_t len;
  LTE_DL_FRAME_PARMS *frame_parms = &ue->frame_parms;
  int frame_tx=proc->frame_tx;
  int subframe_tx=proc->subframe_tx;
  int Mod_id = ue->Mod_id;
  int CC_id = ue->CC_id;
  int tx_amp;
  int16_t Po_PUCCH;
  uint8_t ack_status_cw0=0;
  uint8_t ack_status_cw1=0;
  uint8_t nb_cw=0;
  uint8_t cqi_status=0;
  uint8_t ri_status=0;
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_UE_TX_PUCCH,VCD_FUNCTION_IN);
  SOUNDINGRS_UL_CONFIG_DEDICATED *pSoundingrs_ul_config_dedicated=&ue->soundingrs_ul_config_dedicated[eNB_id];
  // 36.213 8.2
  /*if ackNackSRS_SimultaneousTransmission ==  TRUE and in the cell specific SRS subframes UE shall transmit
    ACK/NACK and SR using the shortened PUCCH format. This shortened PUCCH format shall be used in a cell
    specific SRS subframe even if the UE does not transmit SRS in that subframe
  */
  int harq_pid = subframe2harq_pid(&ue->frame_parms,
                                   frame_tx,
                                   subframe_tx);

  if(ue->ulsch[eNB_id]->harq_processes[harq_pid]->subframe_scheduling_flag) {
    LOG_D(PHY,"PUSCH is programmed on this subframe [pid %d] AbsSuframe %d.%d ==> Skip PUCCH transmission \n",harq_pid,frame_tx,subframe_tx);
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_UE_TX_PUCCH,VCD_FUNCTION_OUT);
    return;
  }

  uint8_t isShortenPucch = (pSoundingrs_ul_config_dedicated->srsCellSubframe && frame_parms->soundingrs_ul_config_common.ackNackSRS_SimultaneousTransmission);
  bundling_flag = ue->pucch_config_dedicated[eNB_id].tdd_AckNackFeedbackMode;
  // Part - I
  // Collect feedback that should be transmitted at this subframe
  // - SR
  // - ACK/NACK
  // - CQI
  // - RI
  SR_payload = 0;

  if (is_SR_TXOp(ue,proc,eNB_id)==1) {
    if (ue->mac_enabled==1) {
      SR_payload = ue_get_SR(Mod_id,
                             CC_id,
                             frame_tx,
                             eNB_id,
                             ue->pdcch_vars[ue->current_thread_id[proc->subframe_rx]][eNB_id]->crnti,
                             subframe_tx); // subframe used for meas gap
    } else {
      SR_payload = 1;
    }
  }

  ack_status_cw0 = get_ack(&ue->frame_parms,
                           ue->dlsch[ue->current_thread_id[proc->subframe_rx]][eNB_id][0]->harq_ack,
                           subframe_tx,
                           proc->subframe_rx,
                           pucch_ack_payload,
                           0);
  ack_status_cw1 = get_ack(&ue->frame_parms,
                           ue->dlsch[ue->current_thread_id[proc->subframe_rx]][eNB_id][1]->harq_ack,
                           subframe_tx,
                           proc->subframe_rx,
                           pucch_ack_payload,
                           1);
  nb_cw = ( (ack_status_cw0 != 0) ? 1:0) + ( (ack_status_cw1 != 0) ? 1:0);
  cqi_status = ((ue->cqi_report_config[eNB_id].CQI_ReportPeriodic.cqi_PMI_ConfigIndex>0)&&
                (is_cqi_TXOp(ue,proc,eNB_id)==1));
  ri_status = ((ue->cqi_report_config[eNB_id].CQI_ReportPeriodic.ri_ConfigIndex>0) &&
               (is_ri_TXOp(ue,proc,eNB_id)==1));

  // Part - II
  // if nothing to report ==> exit function
  if( (nb_cw==0) && (SR_payload==0) && (cqi_status==0) && (ri_status==0) ) {
    LOG_D(PHY,"PUCCH No feedback AbsSubframe %d.%d SR_payload %d nb_cw %d pucch_ack_payload[0] %d pucch_ack_payload[1] %d cqi_status %d Return \n",
          frame_tx%1024, subframe_tx, SR_payload, nb_cw, pucch_ack_payload[0], pucch_ack_payload[1], cqi_status);
    return;
  }

  // Part - III
  // Decide which PUCCH format should be used if needed
  format = get_pucch_format(frame_parms->frame_type,
                            frame_parms->Ncp,
                            SR_payload,
                            nb_cw,
                            cqi_status,
                            ri_status,
                            bundling_flag);
  // Determine PUCCH resources and payload: mandatory for pucch encoding
  get_pucch_param(ue,
                  proc,
                  pucch_ack_payload,
                  format,
                  eNB_id,
                  SR_payload,
                  cqi_status,
                  &pucch_resource,
                  (uint8_t *)&pucch_payload,
                  &len);
  LOG_D(PHY,"PUCCH feedback AbsSubframe %d.%d SR %d NbCW %d (%d %d) AckNack %d.%d CQI %d RI %d format %d pucch_resource %d pucch_payload %d %d \n",
        frame_tx%1024, subframe_tx, SR_payload, nb_cw, ack_status_cw0, ack_status_cw1, pucch_ack_payload[0], pucch_ack_payload[1], cqi_status, ri_status, format, pucch_resource,pucch_payload[0],
        pucch_payload[1]);
  // Part - IV
  // Generate PUCCH signal
  ue->generate_ul_signal[eNB_id] = 1;

  switch (format) {
    case pucch_format1:
    case pucch_format1a:
    case pucch_format1b: {
      if (ue->mac_enabled == 1) {
        Po_PUCCH = pucch_power_cntl(ue,proc,subframe_tx,eNB_id,format);
      } else {
        Po_PUCCH = ue->tx_power_max_dBm;
      }

      ue->tx_power_dBm[subframe_tx] = Po_PUCCH;
      ue->tx_total_RE[subframe_tx] = 12;

      if (IS_SOFTMODEM_BASICSIM) {
        tx_amp = AMP;
      } else {
        tx_amp = get_tx_amp(Po_PUCCH,
                            ue->tx_power_max_dBm,
                            ue->frame_parms.N_RB_UL,
                            1);
      }

      T(T_UE_PHY_PUCCH_TX_POWER, T_INT(eNB_id), T_INT(frame_tx%1024), T_INT(subframe_tx),T_INT(ue->tx_power_dBm[subframe_tx]),
        T_INT(tx_amp),T_INT(ue->dlsch[ue->current_thread_id[proc->subframe_rx]][eNB_id][0]->g_pucch),T_INT(get_PL(ue->Mod_id,ue->CC_id,eNB_id)));

      if(format == pucch_format1) {
        LOG_D(PHY,"[UE  %d][SR %x] AbsSubframe %d.%d Generating PUCCH 1 (SR for PUSCH), an_srs_simultanous %d, shorten_pucch %d, n1_pucch %d, Po_PUCCH %d\n",
              Mod_id,
              ue->dlsch[ue->current_thread_id[proc->subframe_rx]][eNB_id][0]->rnti,
              frame_tx%1024, subframe_tx,
              frame_parms->soundingrs_ul_config_common.ackNackSRS_SimultaneousTransmission,
              isShortenPucch,
              ue->scheduling_request_config[eNB_id].sr_PUCCH_ResourceIndex,
              Po_PUCCH);
      } else {
        if (SR_payload>0) {
          LOG_D(PHY,"[UE  %d][SR %x] AbsSubFrame %d.%d Generating PUCCH %s payload %d,%d (with SR for PUSCH), an_srs_simultanous %d, shorten_pucch %d, n1_pucch %d, Po_PUCCH %d, amp %d\n",
                Mod_id,
                ue->dlsch[ue->current_thread_id[proc->subframe_rx]][eNB_id][0]->rnti,
                frame_tx % 1024, subframe_tx,
                (format == pucch_format1a? "1a": (
                   format == pucch_format1b? "1b" : "??")),
                pucch_ack_payload[0],pucch_ack_payload[1],
                frame_parms->soundingrs_ul_config_common.ackNackSRS_SimultaneousTransmission,
                isShortenPucch,
                pucch_resource,
                Po_PUCCH,
                tx_amp);
        } else {
          LOG_D(PHY,"[UE  %d][PDSCH %x] AbsSubFrame %d.%d rx_offset_diff: %d, Generating PUCCH %s, an_srs_simultanous %d, shorten_pucch %d, n1_pucch %d, b[0]=%d,b[1]=%d (SR_Payload %d), Po_PUCCH %d, amp %d\n",
                Mod_id,
                ue->dlsch[ue->current_thread_id[proc->subframe_rx]][eNB_id][0]->rnti,
                frame_tx%1024, subframe_tx,ue->rx_offset_diff,
                (format == pucch_format1a? "1a": (
                   format == pucch_format1b? "1b" : "??")),
                frame_parms->soundingrs_ul_config_common.ackNackSRS_SimultaneousTransmission,
                isShortenPucch,
                pucch_resource,pucch_payload[0],pucch_payload[1],SR_payload,
                Po_PUCCH,
                tx_amp);
        }
      }

      if (pucch_payload[0]) {
        T(T_UE_PHY_DLSCH_UE_ACK, T_INT(eNB_id), T_INT(frame_tx%1024), T_INT(subframe_tx), T_INT(ue->dlsch[ue->current_thread_id[proc->subframe_rx]][eNB_id][0]->rnti),
          T_INT(ue->dlsch[ue->current_thread_id[proc->subframe_rx]][eNB_id][0]->current_harq_pid));
      } else {
        T(T_UE_PHY_DLSCH_UE_NACK, T_INT(eNB_id), T_INT(frame_tx%1024), T_INT(subframe_tx), T_INT(ue->dlsch[ue->current_thread_id[proc->subframe_rx]][eNB_id][0]->rnti),
          T_INT(ue->dlsch[ue->current_thread_id[proc->subframe_rx]][eNB_id][0]->current_harq_pid));
      }

      generate_pucch1x(ue->common_vars.txdataF,
                       &ue->frame_parms,
                       ue->ncs_cell,
                       format,
                       &ue->pucch_config_dedicated[eNB_id],
                       pucch_resource,
                       isShortenPucch,  // shortened format
                       pucch_payload,
                       tx_amp,
                       subframe_tx);
    }
    break;

    case pucch_format2: {
      if (ue->mac_enabled == 1) {
        Po_PUCCH = pucch_power_cntl(ue,proc,subframe_tx,eNB_id,format);
      } else {
        Po_PUCCH = ue->tx_power_max_dBm;
      }

      ue->tx_power_dBm[subframe_tx] = Po_PUCCH;
      ue->tx_total_RE[subframe_tx] = 12;

      if (IS_SOFTMODEM_BASICSIM) {
        tx_amp = AMP;
      } else {
        tx_amp =  get_tx_amp(Po_PUCCH,
                             ue->tx_power_max_dBm,
                             ue->frame_parms.N_RB_UL,
                             1);
      }

      T(T_UE_PHY_PUCCH_TX_POWER, T_INT(eNB_id), T_INT(frame_tx%1024), T_INT(subframe_tx),T_INT(ue->tx_power_dBm[subframe_tx]),
        T_INT(tx_amp),T_INT(ue->dlsch[ue->current_thread_id[proc->subframe_rx]][eNB_id][0]->g_pucch),T_INT(get_PL(ue->Mod_id,ue->CC_id,eNB_id)));

      if( LOG_DEBUGFLAG(DEBUG_UE_PHYPROC)) {
        LOG_D(PHY,"[UE  %d][RNTI %x] AbsSubFrame %d.%d Generating PUCCH 2 (RI or CQI), Po_PUCCH %d, isShortenPucch %d, amp %d\n",
              Mod_id,
              ue->dlsch[ue->current_thread_id[proc->subframe_rx]][eNB_id][0]->rnti,
              frame_tx%1024, subframe_tx,
              Po_PUCCH,
              isShortenPucch,
              tx_amp);
      }

      generate_pucch2x(ue->common_vars.txdataF,
                       &ue->frame_parms,
                       ue->ncs_cell,
                       format,
                       &ue->pucch_config_dedicated[eNB_id],
                       pucch_resource,
                       pucch_payload,
                       len,          // A
                       0,            // B2 not needed
                       tx_amp,
                       subframe_tx,
                       ue->pdcch_vars[ue->current_thread_id[proc->subframe_rx]][eNB_id]->crnti);
    }
    break;

    case pucch_format2a:
      LOG_D(PHY,"[UE  %d][RNTI %x] AbsSubFrame %d.%d Generating PUCCH 2a (RI or CQI) Ack/Nack 1bit \n",
            Mod_id,
            ue->dlsch[ue->current_thread_id[proc->subframe_rx]][eNB_id][0]->rnti,
            frame_tx%1024, subframe_tx);
      break;

    case pucch_format2b:
      LOG_D(PHY,"[UE  %d][RNTI %x] AbsSubFrame %d.%d Generating PUCCH 2b (RI or CQI) Ack/Nack 2bits\n",
            Mod_id,
            ue->dlsch[ue->current_thread_id[proc->subframe_rx]][eNB_id][0]->rnti,
            frame_tx%1024, subframe_tx);
      break;

    default:
      break;
  }

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_UE_TX_PUCCH,VCD_FUNCTION_OUT);
}

void phy_procedures_UE_SL_TX(PHY_VARS_UE *ue,
                             UE_rxtx_proc_t *proc) {
  int subframe_tx = proc->subframe_tx;
  int frame_tx = proc->frame_tx;
  SLSS_t *slss;
  SLDCH_t *sldch;
  SLSCH_t *slsch;
  LOG_D(PHY,"****** start Sidelink TX-Chain for AbsSubframe %d.%d ******\n", frame_tx, subframe_tx);

  // check for SLBCH/SLSS
  if ((slss = ue_get_slss(ue->Mod_id,ue->CC_id,frame_tx,subframe_tx)) != NULL) generate_slss(ue,slss,frame_tx,subframe_tx);

  // check for SLDCH
  if ((sldch = ue_get_sldch(ue->Mod_id,ue->CC_id,frame_tx,subframe_tx)) != NULL) generate_sldch(ue,sldch,frame_tx,subframe_tx);

  // check for SLSCH
  if ((slsch = ue_get_slsch(ue->Mod_id,ue->CC_id,frame_tx,subframe_tx)) != NULL) generate_slsch(ue,slsch,frame_tx,subframe_tx);
}

void phy_procedures_UE_TX(PHY_VARS_UE *ue,
                          UE_rxtx_proc_t *proc,
                          uint8_t eNB_id,
                          uint8_t abstraction_flag,
                          runmode_t mode) {
  LTE_DL_FRAME_PARMS *frame_parms=&ue->frame_parms;
  //int32_t ulsch_start=0;
  int subframe_tx = proc->subframe_tx;
  int frame_tx = proc->frame_tx;
  unsigned int aa;
  uint8_t isSubframeSRS;
  uint8_t next1_thread_id = ue->current_thread_id[proc->subframe_rx]== (RX_NB_TH-1) ? 0:(ue->current_thread_id[proc->subframe_rx]+1);
  uint8_t next2_thread_id = next1_thread_id== (RX_NB_TH-1) ? 0:(next1_thread_id+1);
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_UE_TX,VCD_FUNCTION_IN);
  LOG_D(PHY,"****** start TX-Chain for AbsSubframe %d.%d ******\n", frame_tx, subframe_tx);
  T(T_UE_PHY_UL_TICK, T_INT(ue->Mod_id), T_INT(frame_tx%1024), T_INT(subframe_tx));
  ue->generate_ul_signal[eNB_id] = 0;

  if ( LOG_DEBUGFLAG(UE_TIMING)) {
    start_meas(&ue->phy_proc_tx);
  }

  ue->tx_power_dBm[subframe_tx]=-127;

  for (aa=0; aa<frame_parms->nb_antennas_tx; aa++) {
    memset(&ue->common_vars.txdataF[aa][subframe_tx*frame_parms->ofdm_symbol_size*frame_parms->symbols_per_tti],
           0,
           frame_parms->ofdm_symbol_size*frame_parms->symbols_per_tti*sizeof(int32_t));
  }

  if (subframe_select(&ue->frame_parms,proc->subframe_tx) == SF_UL ||
      ue->frame_parms.frame_type == FDD) {
    if (ue->UE_mode[eNB_id] > PRACH ) {
      // check cell srs subframe and ue srs subframe. This has an impact on pusch encoding
      isSubframeSRS = is_srs_occasion_common(&ue->frame_parms,proc->frame_tx,proc->subframe_tx);
      ue_compute_srs_occasion(ue,proc,eNB_id,isSubframeSRS);
      ue_ulsch_uespec_procedures(ue,proc,eNB_id,abstraction_flag);
      LOG_D(PHY,"ULPOWERS After ulsch_uespec_procedures : ue->tx_power_dBm[%d]=%d, NPRB %d\n",
            subframe_tx,ue->tx_power_dBm[subframe_tx],ue->tx_total_RE[subframe_tx]);
    }

    if (ue->UE_mode[eNB_id] == PUSCH) {
      // check if we need to use PUCCH 1a/1b
      ue_pucch_procedures(ue,proc,eNB_id,abstraction_flag);
      // check if we need to use SRS
      ue_srs_procedures(ue,proc,eNB_id,abstraction_flag);
    } // UE_mode==PUSCH
  }

  LOG_D(PHY,"doing ulsch_common_procedures (%d.%d): generate_ul_signal %d\n",frame_tx,subframe_tx,
        ue->generate_ul_signal[eNB_id]);
  ulsch_common_procedures(ue,proc, (ue->generate_ul_signal[eNB_id] == 0));

  if ((ue->UE_mode[eNB_id] == PRACH) &&
      (ue->frame_parms.prach_config_common.prach_Config_enabled==1)) {
    // check if we have PRACH opportunity
    if (is_prach_subframe(&ue->frame_parms,frame_tx,subframe_tx)) {
      ue_prach_procedures(ue,proc,eNB_id,abstraction_flag,mode);
    }
  } // mode is PRACH
  else {
    ue->generate_prach=0;
  }

  // reset DL ACK/NACK status
  uint8_t N_bundled = 0;

  if (ue->dlsch[ue->current_thread_id[proc->subframe_rx]][eNB_id][0] != NULL) {
    reset_ack(&ue->frame_parms,
              ue->dlsch[ue->current_thread_id[proc->subframe_rx]][eNB_id][0]->harq_ack,
              subframe_tx,
              proc->subframe_rx,
              ue->ulsch[eNB_id]->o_ACK,
              &N_bundled,
              0);
    reset_ack(&ue->frame_parms,
              ue->dlsch[next1_thread_id][eNB_id][0]->harq_ack,
              subframe_tx,
              proc->subframe_rx,
              ue->ulsch[eNB_id]->o_ACK,
              &N_bundled,
              0);
    reset_ack(&ue->frame_parms,
              ue->dlsch[next2_thread_id][eNB_id][0]->harq_ack,
              subframe_tx,
              proc->subframe_rx,
              ue->ulsch[eNB_id]->o_ACK,
              &N_bundled,
              0);
  }

  if (ue->dlsch_SI[eNB_id] != NULL)
    reset_ack(&ue->frame_parms,
              ue->dlsch_SI[eNB_id]->harq_ack,
              subframe_tx,
              proc->subframe_rx,
              ue->ulsch[eNB_id]->o_ACK,
              &N_bundled,
              0);

  LOG_D(PHY,"****** end TX-Chain for AbsSubframe %d.%d ******\n", frame_tx, subframe_tx);
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_UE_TX, VCD_FUNCTION_OUT);

  if ( LOG_DEBUGFLAG(UE_TIMING)) {
    stop_meas(&ue->phy_proc_tx);
  }
}

void phy_procedures_UE_S_TX(PHY_VARS_UE *ue,
                            uint8_t eNB_id,
                            uint8_t abstraction_flag) {
  int aa;//i,aa;
  LTE_DL_FRAME_PARMS *frame_parms=&ue->frame_parms;

  for (aa=0; aa<frame_parms->nb_antennas_tx; aa++) {
#if defined(EXMIMO) //this is the EXPRESS MIMO case
    int i;

    // set the whole tx buffer to RX
    for (i=0; i<LTE_NUMBER_OF_SUBFRAMES_PER_FRAME*frame_parms->samples_per_tti; i++)
      ue->common_vars.txdata[aa][i] = 0x00010001;

#else //this is the normal case
    //    memset(&ue->common_vars.txdata[aa][0],0,
    //     (LTE_NUMBER_OF_SUBFRAMES_PER_FRAME*frame_parms->samples_per_tti)*sizeof(int32_t));
#endif //else EXMIMO
  }
}

void ue_measurement_procedures(uint16_t l,    // symbol index of each slot [0..6]
                               PHY_VARS_UE *ue,
                               UE_rxtx_proc_t *proc,
                               uint8_t eNB_id,
                               uint16_t slot, // slot index of each radio frame [0..19]
                               uint8_t abstraction_flag,
                               runmode_t mode) {
  //LOG_I(PHY,"ue_measurement_procedures l %d Ncp %d\n",l,ue->frame_parms.Ncp);
  LTE_DL_FRAME_PARMS *frame_parms=&ue->frame_parms;
  int subframe_rx = proc->subframe_rx;
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_MEASUREMENT_PROCEDURES, VCD_FUNCTION_IN);

  if (l==0) {
    // UE measurements on symbol 0
    LOG_D(PHY,"Calling measurements subframe %d, rxdata %p\n",subframe_rx,ue->common_vars.rxdata);
    LOG_D(PHY,"Calling measurements subframe %d, rxdata %p\n",subframe_rx,ue->common_vars.rxdata);
    lte_ue_measurements(ue,
                        (subframe_rx*frame_parms->samples_per_tti+ue->rx_offset)%(frame_parms->samples_per_tti*LTE_NUMBER_OF_SUBFRAMES_PER_FRAME),
                        (subframe_rx == 1) ? 1 : 0,
                        0,
                        0,
                        subframe_rx);

    if(slot == 0)
      T(T_UE_PHY_MEAS, T_INT(eNB_id), T_INT(proc->frame_rx%1024), T_INT(proc->subframe_rx),
        T_INT((int)(10*log10(ue->measurements.rsrp[0])-ue->rx_total_gain_dB)),
        T_INT((int)ue->measurements.rx_rssi_dBm[0]),
        T_INT((int)(ue->measurements.rx_power_avg_dB[0] - ue->measurements.n0_power_avg_dB)),
        T_INT((int)ue->measurements.rx_power_avg_dB[0]),
        T_INT((int)ue->measurements.n0_power_avg_dB),
        T_INT((int)ue->measurements.wideband_cqi_avg[0]),
        T_INT((int)ue->common_vars.freq_offset));
  }

  if (( (slot%2) == 0) && (l==(6-ue->frame_parms.Ncp))) {
    // make sure we have signal from PSS/SSS for N0 measurement
    // LOG_I(PHY," l==(6-ue->frame_parms.Ncp) ue_rrc_measurements\n");
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_RRC_MEASUREMENTS, VCD_FUNCTION_IN);
    ue_rrc_measurements(ue,
                        slot,
                        abstraction_flag);
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_RRC_MEASUREMENTS, VCD_FUNCTION_OUT);
  }

  // accumulate and filter timing offset estimation every subframe (instead of every frame)
  if (( (slot%2) == 0) && (l==(4-frame_parms->Ncp))) {
    // AGC
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_GAIN_CONTROL, VCD_FUNCTION_IN);

    if (IS_SOFTMODEM_BASICSIM)
      phy_adjust_gain (ue,dB_fixed(ue->measurements.rssi),0);

    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_GAIN_CONTROL, VCD_FUNCTION_OUT);
    eNB_id = 0;

    if (ue->no_timing_correction==0)
      lte_adjust_synch(&ue->frame_parms,
                       ue,
                       eNB_id,
                       subframe_rx,
                       0,
                       16384);
  }

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_MEASUREMENT_PROCEDURES, VCD_FUNCTION_OUT);
}



void ue_pbch_procedures(uint8_t eNB_id,
                        PHY_VARS_UE *ue,
                        UE_rxtx_proc_t *proc,
                        uint8_t abstraction_flag) {
  //  int i;
  int pbch_tx_ant=0;
  uint8_t pbch_phase;
  uint16_t frame_tx;
  static uint8_t first_run = 1;
  uint8_t pbch_trials = 0;
  DevAssert(ue);
  int frame_rx = proc->frame_rx;
  int subframe_rx = proc->subframe_rx;
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_PBCH_PROCEDURES, VCD_FUNCTION_IN);
  pbch_phase=(frame_rx%4);

  if (pbch_phase>=4)
    pbch_phase=0;

if((ue->frame_parms.FeMBMS_active == 0)|| is_fembms_cas_subframe(frame_rx,subframe_rx,&ue->frame_parms) || first_run) {

  for (pbch_trials=0; pbch_trials<4; pbch_trials++) {
    //for (pbch_phase=0;pbch_phase<4;pbch_phase++) {
    //LOG_I(PHY,"[UE  %d] Frame %d, Trying PBCH %d (NidCell %d, eNB_id %d)\n",ue->Mod_id,frame_rx,pbch_phase,ue->frame_parms.Nid_cell,eNB_id);
    if(is_fembms_cas_subframe(frame_rx,subframe_rx,&ue->frame_parms) || ue->frame_parms.FeMBMS_active) {
      pbch_tx_ant = rx_pbch_fembms(&ue->common_vars,
                                   ue->pbch_vars[eNB_id],
                                   &ue->frame_parms,
                                   eNB_id,
                                   ue->frame_parms.nb_antenna_ports_eNB==1?SISO:ALAMOUTI,
                                   ue->high_speed_flag,
                                   pbch_phase);
    } else
      pbch_tx_ant = rx_pbch(&ue->common_vars,
                            ue->pbch_vars[eNB_id],
                            &ue->frame_parms,
                            eNB_id,
                            ue->frame_parms.nb_antenna_ports_eNB==1?SISO:ALAMOUTI,
                            ue->high_speed_flag,
                            pbch_phase);

    if ((pbch_tx_ant>0) && (pbch_tx_ant<=4)) {
      break;
    }

    pbch_phase++;

    if (pbch_phase>=4)
      pbch_phase=0;
  }

  if ((pbch_tx_ant>0) && (pbch_tx_ant<=4)) {
    if (opt_enabled) {
      static uint8_t dummy[3];
      dummy[0] = ue->pbch_vars[eNB_id]->decoded_output[2];
      dummy[1] = ue->pbch_vars[eNB_id]->decoded_output[1];
      dummy[2] = ue->pbch_vars[eNB_id]->decoded_output[0];
      trace_pdu( DIRECTION_DOWNLINK, dummy, WS_C_RNTI, ue->Mod_id, 0, 0,
                 frame_rx, subframe_rx, 0, 0);
    }

    if (pbch_tx_ant>2) {
      LOG_W(PHY,"[openair][SCHED][SYNCH] PBCH decoding: pbch_tx_ant>2 not supported\n");
      VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_PBCH_PROCEDURES, VCD_FUNCTION_OUT);
      return;
    }

    ue->pbch_vars[eNB_id]->pdu_errors_conseq = 0;

    if(is_fembms_cas_subframe(frame_rx,subframe_rx,&ue->frame_parms) || ue->frame_parms.FeMBMS_active) {
      frame_tx  = (int)((ue->pbch_vars[eNB_id]->decoded_output[2]&31)<<1);
      frame_tx += ue->pbch_vars[eNB_id]->decoded_output[1]>>7;
      frame_tx = frame_tx<<4;
      frame_tx +=4*pbch_phase;
    } else {
      frame_tx = (((int)(ue->pbch_vars[eNB_id]->decoded_output[2]&0x03))<<8);
      frame_tx += ((int)(ue->pbch_vars[eNB_id]->decoded_output[1]&0xfc));
      frame_tx += pbch_phase;
    }

    if (ue->mac_enabled==1) {
      dl_phy_sync_success(ue->Mod_id,frame_rx,eNB_id,
                          ue->UE_mode[eNB_id]==NOT_SYNCHED ? 1 : 0);
    }

    // if this is the first PBCH after initial synchronization and no timing correction is performed, make L1 state = PRACH
    if (ue->UE_mode[eNB_id]==NOT_SYNCHED && ue->no_timing_correction == 1) ue->UE_mode[eNB_id] = PRACH;

    if (first_run) {
      first_run = 0;
      proc->frame_rx = (proc->frame_rx & 0xFFFFFC00) | (frame_tx & 0x000003FF);
      proc->frame_tx = proc->frame_rx;

      for(int th_id=0; th_id<RX_NB_TH; th_id++) {
        ue->proc.proc_rxtx[th_id].frame_rx = proc->frame_rx;
        ue->proc.proc_rxtx[th_id].frame_tx = proc->frame_tx;
        printf("[UE %d] frame %d, subframe %d: Adjusting frame counter (PBCH ant_tx=%d, frame_tx=%d, phase %d, rx_offset %d) => new frame %d\n",
               ue->Mod_id,
               ue->proc.proc_rxtx[th_id].frame_rx,
               subframe_rx,
               pbch_tx_ant,
               frame_tx,
               pbch_phase,
               ue->rx_offset,
               proc->frame_rx);
      }

      frame_rx = proc->frame_rx;
    } else if (((frame_tx & 0x03FF) != (proc->frame_rx & 0x03FF))) {
      //(pbch_tx_ant != ue->frame_parms.nb_antennas_tx)) {
      LOG_D(PHY,"[UE %d] frame %d, subframe %d: Re-adjusting frame counter (PBCH ant_tx=%d, frame_rx=%d, frame%%1024=%d, phase %d).\n",
            ue->Mod_id,
            proc->frame_rx,
            subframe_rx,
            pbch_tx_ant,
            frame_tx,
            frame_rx & 0x03FF,
            pbch_phase);
      proc->frame_rx = (proc->frame_rx & 0xFFFFFC00) | (frame_tx & 0x000003FF);
      proc->frame_tx = proc->frame_rx;
      frame_rx = proc->frame_rx;

      for(int th_id=0; th_id<RX_NB_TH; th_id++) {
        ue->proc.proc_rxtx[th_id].frame_rx = (proc->frame_rx & 0xFFFFFC00) | (frame_tx & 0x000003FF);
        ue->proc.proc_rxtx[th_id].frame_tx = proc->frame_rx;
      }
    }

    if (LOG_DEBUGFLAG(DEBUG_UE_PHYPROC)) {
      LOG_D(PHY,"[UE %d] frame %d, subframe %d, Received PBCH (MIB): nb_antenna_ports_eNB %d, tx_ant %d, frame_tx %d. N_RB_DL %d, phich_duration %d, phich_resource %d/6!\n",
             ue->Mod_id,
             frame_rx,
             subframe_rx,
             ue->frame_parms.nb_antenna_ports_eNB,
             pbch_tx_ant,
             frame_tx,
             ue->frame_parms.N_RB_DL,
             ue->frame_parms.phich_config_common.phich_duration,
             ue->frame_parms.phich_config_common.phich_resource);
    }
    LOG_D(PHY,"[UE %d] frame %d, subframe %d, Received PBCH (MIB): nb_antenna_ports_eNB %d, tx_ant %d, frame_tx %d. N_RB_DL %d, phich_duration %d, phich_resource %d/6!\n",
             ue->Mod_id,
             frame_rx,
             subframe_rx,
             ue->frame_parms.nb_antenna_ports_eNB,
             pbch_tx_ant,
             frame_tx,
             ue->frame_parms.N_RB_DL,
             ue->frame_parms.phich_config_common.phich_duration,
             ue->frame_parms.phich_config_common.phich_resource);

  } else {
    if (LOG_DUMPFLAG(DEBUG_UE_PHYPROC)) {
      LOG_E(PHY,"[UE %d] frame %d, subframe %d, Error decoding PBCH!\n",
            ue->Mod_id,frame_rx, subframe_rx);
      LOG_I(PHY,"[UE %d] rx_offset %d\n",ue->Mod_id,ue->rx_offset);
      LOG_M("rxsig0.m","rxs0", ue->common_vars.rxdata[0],ue->frame_parms.samples_per_tti,1,1);
      LOG_M("PBCH_rxF0_ext.m","pbch0_ext",ue->pbch_vars[0]->rxdataF_ext[0],12*4*6,1,1);
      LOG_M("PBCH_rxF0_comp.m","pbch0_comp",ue->pbch_vars[0]->rxdataF_comp[0],12*4*6,1,1);
      LOG_M("PBCH_rxF_llr.m","pbch_llr",ue->pbch_vars[0]->llr,(ue->frame_parms.Ncp==0) ? 1920 : 1728,1,4);
      exit(-1);
    }

    ue->pbch_vars[eNB_id]->pdu_errors_conseq++;
    ue->pbch_vars[eNB_id]->pdu_errors++;

    if (ue->mac_enabled == 1) rrc_out_of_sync_ind(ue->Mod_id,frame_rx,eNB_id);
    else AssertFatal(ue->pbch_vars[eNB_id]->pdu_errors_conseq<100,
                       "More that 100 consecutive PBCH errors! Exiting!\n");
  }

  if (frame_rx % 100 == 0) {
    ue->pbch_vars[eNB_id]->pdu_fer = ue->pbch_vars[eNB_id]->pdu_errors - ue->pbch_vars[eNB_id]->pdu_errors_last;
    ue->pbch_vars[eNB_id]->pdu_errors_last = ue->pbch_vars[eNB_id]->pdu_errors;
  }

  if (LOG_DEBUGFLAG(DEBUG_UE_PHYPROC)) {
    LOG_UI(PHY,"[UE %d] frame %d, slot %d, PBCH errors = %d, consecutive errors = %d!\n",
           ue->Mod_id,frame_rx, subframe_rx,
           ue->pbch_vars[eNB_id]->pdu_errors,
           ue->pbch_vars[eNB_id]->pdu_errors_conseq);
  }

} //if((ue->frame_parms.FeMBMS_active == 0)|| is_fembms_cas_subframe(frame_rx,subframe_rx,&ue->frame_parms) || first_run)

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_PBCH_PROCEDURES, VCD_FUNCTION_OUT);
}



int ue_pdcch_procedures(uint8_t eNB_id,
                        PHY_VARS_UE *ue,
                        UE_rxtx_proc_t *proc,
                        uint8_t abstraction_flag) {
  unsigned int dci_cnt=0, i;
  int frame_rx = proc->frame_rx;
  int subframe_rx = proc->subframe_rx;
  DCI_ALLOC_t dci_alloc_rx[8];
  uint8_t next1_thread_id = ue->current_thread_id[subframe_rx]== (RX_NB_TH-1) ? 0:(ue->current_thread_id[subframe_rx]+1);
  uint8_t next2_thread_id = next1_thread_id== (RX_NB_TH-1) ? 0:(next1_thread_id+1);
  LOG_D(PHY,"DCI Decoding procedure in %d.%d\n",frame_rx,subframe_rx);
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_PDCCH_PROCEDURES, VCD_FUNCTION_IN);

  if (LOG_DEBUGFLAG(UE_TIMING)) {
    start_meas(&ue->dlsch_rx_pdcch_stats);
  }

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_RX_PDCCH, VCD_FUNCTION_IN);
  rx_pdcch(ue,
           proc->frame_rx,
           subframe_rx,
           eNB_id,
           ue->frame_parms.nb_antenna_ports_eNB==1?SISO:ALAMOUTI,
           ue->high_speed_flag);
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_RX_PDCCH, VCD_FUNCTION_OUT);
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_DCI_DECODING, VCD_FUNCTION_IN);

  /*printf("Decode SIB frame param aggregation + DCI %d %d, num_pdcch_symbols %d\n",
   ue->pdcch_vars[ue->current_thread_id[subframe_rx]][eNB_id]->agregationLevel,
   ue->pdcch_vars[ue->current_thread_id[subframe_rx]][eNB_id]->dciFormat,
   ue->pdcch_vars[ue->current_thread_id[subframe_rx]][eNB_id]->num_pdcch_symbols);
  */
  //agregation level == FF means no configuration on
  if(ue->pdcch_vars[ue->current_thread_id[subframe_rx]][eNB_id]->agregationLevel == 0xFF || ue->decode_SIB) {
    // search all possible dcis
    dci_cnt = dci_decoding_procedure(ue,
                                     dci_alloc_rx,
                                     (ue->UE_mode[eNB_id] < PUSCH)? 1 : 0,  // if we're in PUSCH don't listen to common search space,
                                     // later when we need paging or RA during connection, update this ...
                                     eNB_id,subframe_rx);
  } else {
    // search only preconfigured dcis
    // search C RNTI dci
    dci_cnt = dci_CRNTI_decoding_procedure(ue,
                                           dci_alloc_rx,
                                           ue->pdcch_vars[ue->current_thread_id[subframe_rx]][eNB_id]->dciFormat,
                                           ue->pdcch_vars[ue->current_thread_id[subframe_rx]][eNB_id]->agregationLevel,
                                           eNB_id,
                                           subframe_rx);
  }

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_DCI_DECODING, VCD_FUNCTION_OUT);
  //LOG_D(PHY,"[UE  %d][PUSCH] Frame %d subframe %d PHICH RX\n",ue->Mod_id,frame_rx,subframe_rx);

  if (is_phich_subframe(&ue->frame_parms,subframe_rx)) {
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_RX_PHICH, VCD_FUNCTION_IN);
    rx_phich(ue,proc,
             subframe_rx,eNB_id);
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_RX_PHICH, VCD_FUNCTION_OUT);
  }

  uint8_t *nCCE_current = &ue->pdcch_vars[ue->current_thread_id[subframe_rx]][eNB_id]->nCCE[subframe_rx];
  uint8_t *nCCE_dest = &ue->pdcch_vars[next1_thread_id][eNB_id]->nCCE[subframe_rx];
  uint8_t *nCCE_dest1 = &ue->pdcch_vars[next2_thread_id][eNB_id]->nCCE[subframe_rx];
  memcpy(nCCE_dest, nCCE_current, sizeof(uint8_t));
  memcpy(nCCE_dest1, nCCE_current, sizeof(uint8_t));
  LOG_D(PHY,"current_thread %d next1_thread %d next2_thread %d \n", ue->current_thread_id[subframe_rx], next1_thread_id, next2_thread_id);
  LOG_D(PHY,"[UE  %d] AbsSubFrame %d.%d, Mode %s: DCI found %i --> rnti %x / crnti %x : format %d\n",
        ue->Mod_id,frame_rx%1024,subframe_rx,mode_string[ue->UE_mode[eNB_id]],
        dci_cnt,
        dci_alloc_rx[0].rnti,
        ue->pdcch_vars[ue->current_thread_id[subframe_rx]][eNB_id]->crnti,
        dci_alloc_rx[0].format );
  ue->pdcch_vars[ue->current_thread_id[subframe_rx]][eNB_id]->dci_received += dci_cnt;

  for (i=0; i<dci_cnt; i++) {
    if ((ue->UE_mode[eNB_id]>PRACH) &&
        (dci_alloc_rx[i].rnti == ue->pdcch_vars[ue->current_thread_id[subframe_rx]][eNB_id]->crnti) &&
        (dci_alloc_rx[i].format != format0)) {
      LOG_D(PHY,"[UE  %d][DCI][PDSCH %x] AbsSubframe %d.%d: format %d, num_pdcch_symbols %d, nCCE %d, total CCEs %d\n",
            ue->Mod_id,dci_alloc_rx[i].rnti,
            frame_rx%1024,subframe_rx,
            dci_alloc_rx[i].format,
            ue->pdcch_vars[ue->current_thread_id[subframe_rx]][eNB_id]->num_pdcch_symbols,
            ue->pdcch_vars[ue->current_thread_id[subframe_rx]][eNB_id]->nCCE[subframe_rx],
            get_nCCE(3,&ue->frame_parms,get_mi(&ue->frame_parms,0)));

      //dump_dci(&ue->frame_parms, &dci_alloc_rx[i]);

      if ((ue->UE_mode[eNB_id] > PRACH) &&
          (generate_ue_dlsch_params_from_dci(frame_rx,
                                             subframe_rx,
                                             (void *)&dci_alloc_rx[i].dci_pdu,
                                             ue->pdcch_vars[ue->current_thread_id[subframe_rx]][eNB_id]->crnti,
                                             dci_alloc_rx[i].format,
                                             ue->pdcch_vars[ue->current_thread_id[subframe_rx]][eNB_id],
                                             ue->pdsch_vars[ue->current_thread_id[subframe_rx]][eNB_id],
                                             ue->dlsch[ue->current_thread_id[subframe_rx]][eNB_id],
                                             &ue->frame_parms,
                                             ue->pdsch_config_dedicated,
                                             SI_RNTI,
                                             0,
                                             P_RNTI,
                                             ue->transmission_mode[eNB_id]<7?0:ue->transmission_mode[eNB_id],
                                             ue->pdcch_vars[0%RX_NB_TH][eNB_id]->crnti_is_temporary? ue->pdcch_vars[ue->current_thread_id[subframe_rx]][eNB_id]->crnti: 0)==0)) {
        // update TPC for PUCCH
        if((dci_alloc_rx[i].format == format1)   ||
            (dci_alloc_rx[i].format == format1A) ||
            (dci_alloc_rx[i].format == format1B) ||
            (dci_alloc_rx[i].format == format2)  ||
            (dci_alloc_rx[i].format == format2A) ||
            (dci_alloc_rx[i].format == format2B)) {
          //ue->dlsch[ue->current_thread_id[subframe_rx]][eNB_id][0]->g_pucch += ue->dlsch[ue->current_thread_id[subframe_rx]][eNB_id][0]->harq_processes[ue->dlsch[ue->current_thread_id[subframe_rx]][eNB_id][0]->current_harq_pid]->delta_PUCCH;
          int32_t delta_pucch = ue->dlsch[ue->current_thread_id[subframe_rx]][eNB_id][0]->harq_processes[ue->dlsch[ue->current_thread_id[subframe_rx]][eNB_id][0]->current_harq_pid]->delta_PUCCH;

          for(int th_id=0; th_id<RX_NB_TH; th_id++) {
            ue->dlsch[th_id][eNB_id][0]->g_pucch += delta_pucch;
          }

          LOG_D(PHY,"update TPC for PUCCH %d.%d / pid %d delta_PUCCH %d g_pucch %d %d \n",frame_rx, subframe_rx,ue->dlsch[ue->current_thread_id[subframe_rx]][eNB_id][0]->current_harq_pid,
                delta_pucch,
                ue->dlsch[0][eNB_id][0]->g_pucch,
                ue->dlsch[1][eNB_id][0]->g_pucch
                //ue->dlsch[2][eNB_id][0]->g_pucch
               );
        }

        ue->dlsch_received[eNB_id]++;

        if ( LOG_DEBUGFLAG(DEBUG_UE_PHYPROC)) {
          LOG_D(PHY,"[UE  %d] Generated UE DLSCH C_RNTI format %d\n",ue->Mod_id,dci_alloc_rx[i].format);
          dump_dci(&ue->frame_parms, &dci_alloc_rx[i]);
          LOG_D(PHY,"[UE %d] *********** dlsch->active in subframe %d=> %d\n",ue->Mod_id,subframe_rx,ue->dlsch[ue->current_thread_id[subframe_rx]][eNB_id][0]->active);
        }

        // we received a CRNTI, so we're in PUSCH
        if (ue->UE_mode[eNB_id] != PUSCH) {
          if (LOG_DEBUGFLAG(DEBUG_UE_PHYPROC)) {
            LOG_D(PHY,"[UE  %d] Frame %d, subframe %d: Received DCI with CRNTI %x => Mode PUSCH\n",ue->Mod_id,frame_rx,subframe_rx,ue->pdcch_vars[subframe_rx&1][eNB_id]->crnti);
          }

          //dump_dci(&ue->frame_parms, &dci_alloc_rx[i]);
          ue->UE_mode[eNB_id] = PUSCH;
        }
      } else {
        LOG_E(PHY,"[UE  %d] Frame %d, subframe %d: Problem in DCI!\n",ue->Mod_id,frame_rx,subframe_rx);
        dump_dci(&ue->frame_parms, &dci_alloc_rx[i]);
      }
    } else if ((dci_alloc_rx[i].rnti == SI_RNTI /*|| dci_alloc_rx[i].rnti == 0xfff9*/) &&
               ((dci_alloc_rx[i].format == format1A) || (dci_alloc_rx[i].format == format1C))) {
      if (LOG_DEBUGFLAG(DEBUG_UE_PHYPROC)) {
        LOG_D(PHY,"[UE  %d] subframe %d: Found rnti %x, format 1%s, dci_cnt %d\n",ue->Mod_id,subframe_rx,dci_alloc_rx[i].rnti,dci_alloc_rx[i].format==format1A?"A":"C",i);
      }

      if (generate_ue_dlsch_params_from_dci(frame_rx,
                                            subframe_rx,
                                            (void *)&dci_alloc_rx[i].dci_pdu,
                                            //dci_alloc_rx[i].rnti,//SI_RNTI (to enable MBMS dedicated SI-RNTI = 0xfff9
                                            SI_RNTI,
                                            dci_alloc_rx[i].format,
                                            ue->pdcch_vars[ue->current_thread_id[subframe_rx]][eNB_id],
                                            ue->pdsch_vars_SI[eNB_id],
                                            &ue->dlsch_SI[eNB_id],
                                            &ue->frame_parms,
                                            ue->pdsch_config_dedicated,
                                            SI_RNTI,
                                            0,
                                            P_RNTI,
                                            ue->transmission_mode[eNB_id]<7?0:ue->transmission_mode[eNB_id],
                                            0)==0) {
        ue->dlsch_SI_received[eNB_id]++;
        LOG_D(PHY,"[UE  %d] Frame %d, subframe %d : Generate UE DLSCH SI_RNTI format 1%s\n",ue->Mod_id,frame_rx,subframe_rx,dci_alloc_rx[i].format==format1A?"A":"C");
        //dump_dci(&ue->frame_parms, &dci_alloc_rx[i]);
      }
    } else if ((dci_alloc_rx[i].rnti == P_RNTI) &&
               ((dci_alloc_rx[i].format == format1A) || (dci_alloc_rx[i].format == format1C))) {
      if (LOG_DEBUGFLAG(DEBUG_UE_PHYPROC)) {
        LOG_D(PHY,"[UE  %d] subframe %d: Found rnti %x, format 1%s, dci_cnt %d\n",ue->Mod_id,subframe_rx,dci_alloc_rx[i].rnti,dci_alloc_rx[i].format==format1A?"A":"C",i);
      }

      if (generate_ue_dlsch_params_from_dci(frame_rx,
                                            subframe_rx,
                                            (void *)&dci_alloc_rx[i].dci_pdu,
                                            P_RNTI,
                                            dci_alloc_rx[i].format,
                                            ue->pdcch_vars[ue->current_thread_id[subframe_rx]][eNB_id],
                                            ue->pdsch_vars_p[eNB_id],
                                            &ue->dlsch_SI[eNB_id],
                                            &ue->frame_parms,
                                            ue->pdsch_config_dedicated,
                                            SI_RNTI,
                                            0,
                                            P_RNTI,
                                            ue->transmission_mode[eNB_id]<7?0:ue->transmission_mode[eNB_id],
                                            0)==0) {
        ue->dlsch_p_received[eNB_id]++;
        LOG_D(PHY,"[UE  %d] Frame %d, subframe %d : Generate UE DLSCH P_RNTI format 1%s\n",ue->Mod_id,frame_rx,subframe_rx,dci_alloc_rx[i].format==format1A?"A":"C");
        //dump_dci(&ue->frame_parms, &dci_alloc_rx[i]);
      }
    } else if ((ue->prach_resources[eNB_id]) &&
               (dci_alloc_rx[i].rnti == ue->prach_resources[eNB_id]->ra_RNTI) &&
               ((dci_alloc_rx[i].format == format1A) || (dci_alloc_rx[i].format == format1C))) {
      if (LOG_DEBUGFLAG(DEBUG_UE_PHYPROC)) {
        LOG_D(PHY,"[UE  %d][RAPROC] subframe %d: Found RA rnti %x, format 1%s, dci_cnt %d\n",ue->Mod_id,subframe_rx,dci_alloc_rx[i].rnti,dci_alloc_rx[i].format==format1A?"A":"C",i);
      }

      if (generate_ue_dlsch_params_from_dci(frame_rx,
                                            subframe_rx,
                                            (void *)&dci_alloc_rx[i].dci_pdu,
                                            ue->prach_resources[eNB_id]->ra_RNTI,
                                            dci_alloc_rx[i].format,
                                            ue->pdcch_vars[ue->current_thread_id[subframe_rx]][eNB_id],
                                            ue->pdsch_vars_ra[eNB_id],
                                            &ue->dlsch_ra[eNB_id],
                                            &ue->frame_parms,
                                            ue->pdsch_config_dedicated,
                                            SI_RNTI,
                                            ue->prach_resources[eNB_id]->ra_RNTI,
                                            P_RNTI,
                                            ue->transmission_mode[eNB_id]<7?0:ue->transmission_mode[eNB_id],
                                            0)==0) {
        ue->dlsch_ra_received[eNB_id]++;

        if (LOG_DEBUGFLAG(DEBUG_UE_PHYPROC)) {
          LOG_D(PHY,"[UE  %d] Generate UE DLSCH RA_RNTI format 1A, rb_alloc %x, dlsch_ra[eNB_id] %p\n",
                ue->Mod_id,ue->dlsch_ra[eNB_id]->harq_processes[0]->rb_alloc_even[0],ue->dlsch_ra[eNB_id]);
        }
      }
    } else if( (dci_alloc_rx[i].rnti == ue->pdcch_vars[ue->current_thread_id[subframe_rx]][eNB_id]->crnti) &&
               (dci_alloc_rx[i].format == format0)) {
      if (LOG_DEBUGFLAG(DEBUG_UE_PHYPROC)) {
        LOG_D(PHY,"[UE  %d][PUSCH] Frame %d subframe %d: Found rnti %x, format 0, dci_cnt %d\n",
              ue->Mod_id,frame_rx,subframe_rx,dci_alloc_rx[i].rnti,i);
      }

      ue->ulsch_no_allocation_counter[eNB_id] = 0;
      //dump_dci(&ue->frame_parms,&dci_alloc_rx[i]);

      if ((ue->UE_mode[eNB_id] > PRACH) &&
          (generate_ue_ulsch_params_from_dci((void *)&dci_alloc_rx[i].dci_pdu,
                                             ue->pdcch_vars[ue->current_thread_id[subframe_rx]][eNB_id]->crnti,
                                             subframe_rx,
                                             format0,
                                             ue,
                                             proc,
                                             SI_RNTI,
                                             0,
                                             P_RNTI,
                                             CBA_RNTI,
                                             eNB_id,
                                             0)==0)) {
        int harq_pid = subframe2harq_pid(&ue->frame_parms,
                                         pdcch_alloc2ul_frame(&ue->frame_parms,proc->frame_rx,proc->subframe_rx),
                                         pdcch_alloc2ul_subframe(&ue->frame_parms,proc->subframe_rx));
        T(T_UE_PHY_ULSCH_UE_DCI, T_INT(eNB_id), T_INT(proc->frame_rx%1024), T_INT(proc->subframe_rx),
          T_INT(dci_alloc_rx[i].rnti),
          T_INT(harq_pid),
          T_INT(ue->ulsch[eNB_id]->harq_processes[harq_pid]->mcs),
          T_INT(ue->ulsch[eNB_id]->harq_processes[harq_pid]->round),
          T_INT(ue->ulsch[eNB_id]->harq_processes[harq_pid]->first_rb),
          T_INT(ue->ulsch[eNB_id]->harq_processes[harq_pid]->nb_rb),
          T_INT(ue->ulsch[eNB_id]->harq_processes[harq_pid]->TBS));

        if (LOG_DEBUGFLAG(DEBUG_UE_PHYPROC)) {
          LOG_USEDINLOG_VAR(int8_t,harq_pid) = subframe2harq_pid(&ue->frame_parms,
                                               pdcch_alloc2ul_frame(&ue->frame_parms,proc->frame_rx,proc->subframe_rx),
                                               pdcch_alloc2ul_subframe(&ue->frame_parms,proc->subframe_rx));
          LOG_D(PHY,"[UE  %d] Generate UE ULSCH C_RNTI format 0 (subframe %d)\n",ue->Mod_id,subframe_rx);
        }
      }
    } else if( (dci_alloc_rx[i].rnti == ue->ulsch[eNB_id]->cba_rnti[0]) &&
               (dci_alloc_rx[i].format == format0)) {
      // UE could belong to more than one CBA group
      // ue->Mod_id%ue->ulsch[eNB_id]->num_active_cba_groups]
      if (LOG_DEBUGFLAG(DEBUG_UE_PHYPROC)) {
        LOG_D(PHY,"[UE  %d][PUSCH] Frame %d subframe %d: Found cba rnti %x, format 0, dci_cnt %d\n",
              ue->Mod_id,frame_rx,subframe_rx,dci_alloc_rx[i].rnti,i);
        /*
        if (((frame_rx%100) == 0) || (frame_rx < 20))
        dump_dci(&ue->frame_parms, &dci_alloc_rx[i]);
        */
      }

      ue->ulsch_no_allocation_counter[eNB_id] = 0;
      //dump_dci(&ue->frame_parms,&dci_alloc_rx[i]);

      if ((ue->UE_mode[eNB_id] > PRACH) &&
          (generate_ue_ulsch_params_from_dci((void *)&dci_alloc_rx[i].dci_pdu,
                                             ue->ulsch[eNB_id]->cba_rnti[0],
                                             subframe_rx,
                                             format0,
                                             ue,
                                             proc,
                                             SI_RNTI,
                                             0,
                                             P_RNTI,
                                             CBA_RNTI,
                                             eNB_id,
                                             0)==0)) {
        if (LOG_DEBUGFLAG(DEBUG_UE_PHYPROC)) {
          LOG_D(PHY,"[UE  %d] Generate UE ULSCH CBA_RNTI format 0 (subframe %d)\n",ue->Mod_id,subframe_rx);
        }

        ue->ulsch[eNB_id]->num_cba_dci[(subframe_rx+4)%10]++;
      }
    } else {
      if (LOG_DEBUGFLAG(DEBUG_UE_PHYPROC)) {
        LOG_D(PHY,"[UE  %d] frame %d, subframe %d: received DCI %d with RNTI=%x (C-RNTI:%x, CBA_RNTI %x) and format %d!\n",ue->Mod_id,frame_rx,subframe_rx,i,dci_alloc_rx[i].rnti,
              ue->pdcch_vars[ue->current_thread_id[subframe_rx]][eNB_id]->crnti,
              ue->ulsch[eNB_id]->cba_rnti[0],
              dci_alloc_rx[i].format);
        //      dump_dci(&ue->frame_parms, &dci_alloc_rx[i]);
      }
    }
  }

  if (LOG_DEBUGFLAG(UE_TIMING)) {
    stop_meas(&ue->dlsch_rx_pdcch_stats);
  }

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_PDCCH_PROCEDURES, VCD_FUNCTION_OUT);
  return(0);
}


void ue_pmch_procedures(PHY_VARS_UE *ue,
                        UE_rxtx_proc_t *proc,
                        int eNB_id,
                        int abstraction_flag,
                        uint8_t fembms_flag) {
  int subframe_rx = proc->subframe_rx;
  int frame_rx = proc->frame_rx;
  int pmch_mcs=-1;
  int CC_id = ue->CC_id;
  uint8_t sync_area=255;
  uint8_t mcch_active;
  int l;
  int ret=0;

  if (is_pmch_subframe(frame_rx,subframe_rx,&ue->frame_parms) || fembms_flag) {
    // LOG_D(PHY,"ue calling pmch subframe ..\n ");
    LOG_D(PHY,"[UE %d] Frame %d, subframe %d: Querying for PMCH demodulation\n",
          ue->Mod_id,frame_rx,subframe_rx);
    if(fembms_flag)
    pmch_mcs = ue_query_mch_fembms(ue->Mod_id,
                            CC_id,
                            frame_rx,
                            subframe_rx,
                            eNB_id,
                            &sync_area,
                            &mcch_active);
    else
    pmch_mcs = ue_query_mch(ue->Mod_id,
                            CC_id,
                            frame_rx,
                            subframe_rx,
                            eNB_id,
                            &sync_area,
                            &mcch_active);

    if (pmch_mcs>=0) {
      LOG_D(PHY,"[UE %d] Frame %d, subframe %d: Programming PMCH demodulation for mcs %d\n",ue->Mod_id,frame_rx,subframe_rx,pmch_mcs);
      fill_UE_dlsch_MCH(ue,pmch_mcs,1,0,0);

      if(fembms_flag /*subframe_rx == 3 || subframe_rx == 2*/) {
        VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_SLOT_FEP_MBSFN_KHZ_1DOT25, VCD_FUNCTION_IN);
        slot_fep_mbsfn_khz_1dot25(ue,subframe_rx,0);
        VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_SLOT_FEP_MBSFN_KHZ_1DOT25, VCD_FUNCTION_OUT);
      } else {
        VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_SLOT_FEP_MBSFN, VCD_FUNCTION_IN);

        for (l=2; l<12; l++) {
          slot_fep_mbsfn(ue,
                         l,
                         subframe_rx,
                         0,0);//ue->rx_offset,0);
        }

        VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_SLOT_FEP_MBSFN, VCD_FUNCTION_OUT);
      }

      if(fembms_flag /*subframe_rx == 3 || subframe_rx == 2*/) {
        VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_RX_PMCH_KHZ_1DOT25, VCD_FUNCTION_IN);
        rx_pmch_khz_1dot25(ue,0,subframe_rx,pmch_mcs);
        VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_RX_PMCH_KHZ_1DOT25, VCD_FUNCTION_OUT);
      } else {
        VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_RX_PMCH, VCD_FUNCTION_IN);

        for (l=2; l<12; l++) {
          rx_pmch(ue,
                  0,
                  subframe_rx,
                  l);
        }

        VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_RX_PMCH, VCD_FUNCTION_OUT);
      }

      VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_DLSCH_PMCH_DECODING, VCD_FUNCTION_IN);
      ue->dlsch_MCH[0]->harq_processes[0]->Qm = get_Qm(pmch_mcs);

      if(fembms_flag /*subframe_rx == 3 || subframe_rx == 2*/)
        ue->dlsch_MCH[0]->harq_processes[0]->G = get_G_khz_1dot25(&ue->frame_parms,
            ue->dlsch_MCH[0]->harq_processes[0]->nb_rb,
            ue->dlsch_MCH[0]->harq_processes[0]->rb_alloc_even,
            ue->dlsch_MCH[0]->harq_processes[0]->Qm,
            1,
            2,
            frame_rx,
            subframe_rx,
            0);
      else
        ue->dlsch_MCH[0]->harq_processes[0]->G = get_G(&ue->frame_parms,
            ue->dlsch_MCH[0]->harq_processes[0]->nb_rb,
            ue->dlsch_MCH[0]->harq_processes[0]->rb_alloc_even,
            ue->dlsch_MCH[0]->harq_processes[0]->Qm,
            1,
            2,
            frame_rx,
            subframe_rx,
            0);

      dlsch_unscrambling(&ue->frame_parms,1,ue->dlsch_MCH[0],
                         ue->dlsch_MCH[0]->harq_processes[0]->G,
                         ue->pdsch_vars_MCH[ue->current_thread_id[subframe_rx]][0]->llr[0],0,subframe_rx<<1);
      LOG_D(PHY,"start turbo decode for MCH %d.%d --> nb_rb %d \n", frame_rx, subframe_rx, ue->dlsch_MCH[0]->harq_processes[0]->nb_rb);
      LOG_D(PHY,"start turbo decode for MCH %d.%d --> rb_alloc_even %x \n", frame_rx, subframe_rx, (unsigned int)((intptr_t)ue->dlsch_MCH[0]->harq_processes[0]->rb_alloc_even));
      LOG_D(PHY,"start turbo decode for MCH %d.%d --> Qm %d \n", frame_rx, subframe_rx, ue->dlsch_MCH[0]->harq_processes[0]->Qm);
      LOG_D(PHY,"start turbo decode for MCH %d.%d --> Nl %d \n", frame_rx, subframe_rx, ue->dlsch_MCH[0]->harq_processes[0]->Nl);
      LOG_D(PHY,"start turbo decode for MCH %d.%d --> G  %d \n", frame_rx, subframe_rx, ue->dlsch_MCH[0]->harq_processes[0]->G);
      LOG_D(PHY,"start turbo decode for MCH %d.%d --> Kmimo  %d \n", frame_rx, subframe_rx, ue->dlsch_MCH[0]->Kmimo);

      ret = dlsch_decoding(ue,
                           ue->pdsch_vars_MCH[ue->current_thread_id[subframe_rx]][0]->llr[0],
                           &ue->frame_parms,
                           ue->dlsch_MCH[0],
                           ue->dlsch_MCH[0]->harq_processes[0],
                           frame_rx,
                           subframe_rx,
                           0,
                           0,1);
      VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_DLSCH_PMCH_DECODING, VCD_FUNCTION_OUT);

      if (mcch_active == 1)
        ue->dlsch_mcch_trials[sync_area][0]++;
      else
        ue->dlsch_mtch_trials[sync_area][0]++;

      if (ret == (1+ue->dlsch_MCH[0]->max_turbo_iterations)) {
        if (mcch_active == 1)
          ue->dlsch_mcch_errors[sync_area][0]++;
        else
          ue->dlsch_mtch_errors[sync_area][0]++;

        LOG_E(PHY,"[UE %d] Frame %d, subframe %d: PMCH in error (%d,%d), not passing to L2 (TBS %d, iter %d,G %d)\n",
              ue->Mod_id,
              frame_rx,subframe_rx,
              ue->dlsch_mcch_errors[sync_area][0],
              ue->dlsch_mtch_errors[sync_area][0],
              ue->dlsch_MCH[0]->harq_processes[0]->TBS>>3,
              ue->dlsch_MCH[0]->max_turbo_iterations,
              ue->dlsch_MCH[0]->harq_processes[0]->G);
        // dump_mch(ue,0,ue->dlsch_MCH[0]->harq_processes[0]->G,subframe_rx);
        //if(subframe_rx == 3){
        //dump_mch(ue,0,ue->dlsch_MCH[0]->harq_processes[0]->G,subframe_rx);
        //exit_fun("nothing to add");
        //}

        if (LOG_DEBUGFLAG(DEBUG_UE_PHYPROC)) {
          for (int i=0; i<ue->dlsch_MCH[0]->harq_processes[0]->TBS>>3; i++) {
            LOG_T(PHY,"%02x.",ue->dlsch_MCH[0]->harq_processes[0]->c[0][i]);
          }

          LOG_T(PHY,"\n");
        }

        //  if (subframe_rx==9)
        //  mac_xface->macphy_exit("Why are we exiting here?");
      } else { // decoding successful
        LOG_D(PHY,"[UE %d] Frame %d, subframe %d: PMCH OK (%d,%d), passing to L2 (TBS %d, iter %d,G %d) ret %d\n",
              ue->Mod_id,
              frame_rx,subframe_rx,
              ue->dlsch_mcch_errors[sync_area][0],
              ue->dlsch_mtch_errors[sync_area][0],
              ue->dlsch_MCH[0]->harq_processes[0]->TBS>>3,
              ue->dlsch_MCH[0]->max_turbo_iterations,
              ue->dlsch_MCH[0]->harq_processes[0]->G,ret);
        ue_send_mch_sdu(ue->Mod_id,
                        CC_id,
                        frame_rx,
                        ue->dlsch_MCH[0]->harq_processes[0]->b,
                        ue->dlsch_MCH[0]->harq_processes[0]->TBS>>3,
                        eNB_id,// not relevant in eMBMS context
                        sync_area);
        //dump_mch(ue,0,ue->dlsch_MCH[0]->harq_processes[0]->G,subframe_rx);
        //exit_fun("nothing to add");

        if (mcch_active == 1)
          ue->dlsch_mcch_received[sync_area][0]++;
        else
          ue->dlsch_mtch_received[sync_area][0]++;

        if (ue->dlsch_mch_received_sf[subframe_rx%5][0] == 1 ) {
          ue->dlsch_mch_received_sf[subframe_rx%5][0]=0;
        } else {
          ue->dlsch_mch_received[0]+=1;
          ue->dlsch_mch_received_sf[subframe_rx][0]=1;
        }
      } // decoding sucessful
    } // pmch_mcs>=0
  } // is_pmch_subframe=true
}

void copy_harq_proc_struct(LTE_DL_UE_HARQ_t *harq_processes_dest,
                           LTE_DL_UE_HARQ_t *current_harq_processes) {
  harq_processes_dest->B              = current_harq_processes->B              ;
  harq_processes_dest->C              = current_harq_processes->C              ;
  harq_processes_dest->Cminus         = current_harq_processes->Cminus         ;
  harq_processes_dest->Cplus          = current_harq_processes->Cplus          ;
  harq_processes_dest->DCINdi         = current_harq_processes->DCINdi         ;
  harq_processes_dest->F              = current_harq_processes->F              ;
  harq_processes_dest->G              = current_harq_processes->G              ;
  harq_processes_dest->Kminus         = current_harq_processes->Kminus         ;
  harq_processes_dest->Kplus          = current_harq_processes->Kplus          ;
  harq_processes_dest->Nl             = current_harq_processes->Nl             ;
  harq_processes_dest->Qm             = current_harq_processes->Qm             ;
  harq_processes_dest->TBS            = current_harq_processes->TBS            ;
  harq_processes_dest->b              = current_harq_processes->b              ;
  harq_processes_dest->codeword       = current_harq_processes->codeword       ;
  harq_processes_dest->delta_PUCCH    = current_harq_processes->delta_PUCCH    ;
  harq_processes_dest->dl_power_off   = current_harq_processes->dl_power_off   ;
  harq_processes_dest->first_tx       = current_harq_processes->first_tx       ;
  harq_processes_dest->mcs            = current_harq_processes->mcs            ;
  harq_processes_dest->mimo_mode      = current_harq_processes->mimo_mode      ;
  harq_processes_dest->nb_rb          = current_harq_processes->nb_rb          ;
  harq_processes_dest->pmi_alloc      = current_harq_processes->pmi_alloc      ;
  harq_processes_dest->rb_alloc_even[0]  = current_harq_processes->rb_alloc_even[0] ;
  harq_processes_dest->rb_alloc_even[1]  = current_harq_processes->rb_alloc_even[1] ;
  harq_processes_dest->rb_alloc_even[2]  = current_harq_processes->rb_alloc_even[2] ;
  harq_processes_dest->rb_alloc_even[3]  = current_harq_processes->rb_alloc_even[3] ;
  harq_processes_dest->rb_alloc_odd[0]  = current_harq_processes->rb_alloc_odd[0]  ;
  harq_processes_dest->rb_alloc_odd[1]  = current_harq_processes->rb_alloc_odd[1]  ;
  harq_processes_dest->rb_alloc_odd[2]  = current_harq_processes->rb_alloc_odd[2]  ;
  harq_processes_dest->rb_alloc_odd[3]  = current_harq_processes->rb_alloc_odd[3]  ;
  harq_processes_dest->round          = current_harq_processes->round          ;
  harq_processes_dest->rvidx          = current_harq_processes->rvidx          ;
  harq_processes_dest->status         = current_harq_processes->status         ;
  harq_processes_dest->vrb_type       = current_harq_processes->vrb_type       ;
}

void copy_ack_struct(harq_status_t *harq_ack_dest, harq_status_t *current_harq_ack) {
  memcpy(harq_ack_dest, current_harq_ack, sizeof(harq_status_t));
}

void ue_pdsch_procedures(PHY_VARS_UE *ue,
                         UE_rxtx_proc_t *proc,
                         int eNB_id,
                         PDSCH_t pdsch,
                         LTE_UE_DLSCH_t *dlsch0,
                         LTE_UE_DLSCH_t *dlsch1,
                         int s0,
                         int s1,
                         int abstraction_flag) {
  int subframe_rx = proc->subframe_rx;
  int m;
  int harq_pid;
  int i_mod,eNB_id_i,dual_stream_UE;
  int first_symbol_flag=0;

  if (dlsch0 && dlsch0->active == 0)
    return;

  for (m=s0; m<=s1; m++) {
    if (dlsch0 && (!dlsch1))  {
      harq_pid = dlsch0->current_harq_pid;
      LOG_D(PHY,"[UE %d] PDSCH active in subframe %d, harq_pid %d Symbol %d\n",ue->Mod_id,subframe_rx,harq_pid,m);

      if ((pdsch==PDSCH) &&
          (ue->transmission_mode[eNB_id] == 5) &&
          (dlsch0->harq_processes[harq_pid]->dl_power_off==0) &&
          (ue->use_ia_receiver ==1)) {
        dual_stream_UE = 1;
        eNB_id_i = ue->n_connected_eNB;
        i_mod =  dlsch0->harq_processes[harq_pid]->Qm;
      } else if((pdsch==PDSCH) && (ue->transmission_mode[eNB_id]==3)) {
        dual_stream_UE = rx_IC_dual_stream;
        eNB_id_i       = eNB_id;
        i_mod          = 0;
      } else {
        dual_stream_UE = 0;
        eNB_id_i = eNB_id+1;
        i_mod = 0;
      }

      //TM7 UE specific channel estimation here!!!
      if (ue->transmission_mode[eNB_id]==7) {
        if (ue->frame_parms.Ncp==0) {
          if ((m==3) || (m==6) || (m==9) || (m==12))
            //LOG_D(PHY,"[UE %d] dlsch->active in subframe %d => %d, l=%d\n",phy_vars_ue->Mod_id,subframe_rx,phy_vars_ue->dlsch_ue[eNB_id][0]->active, l);
            lte_dl_bf_channel_estimation(ue,eNB_id,0,subframe_rx*2+(m>6?1:0),5,m);
        } else {
          LOG_E(PHY,"[UE %d]Beamforming channel estimation not supported yet for TM7 extented CP.\n",ue->Mod_id);
        }
      }

      if ((m==s0) && (m<4))
        first_symbol_flag = 1;
      else
        first_symbol_flag = 0;

      if (LOG_DEBUGFLAG(UE_TIMING)) {
        uint8_t slot = 0;

        if(m >= ue->frame_parms.symbols_per_tti>>1)
          slot = 1;

        start_meas(&ue->dlsch_llr_stats_parallelization[ue->current_thread_id[subframe_rx]][slot]);
      }

      // process DLSCH received in first slot
      rx_pdsch(ue,
               pdsch,
               eNB_id,
               eNB_id_i,
               proc->frame_rx,
               subframe_rx,  // subframe,
               m,
               first_symbol_flag,
               dual_stream_UE,
               i_mod,
               dlsch0->current_harq_pid);

      if (LOG_DEBUGFLAG(UE_TIMING)) {
        uint8_t slot = 0;

        if(m >= ue->frame_parms.symbols_per_tti>>1)
          slot = 1;

        stop_meas(&ue->dlsch_llr_stats_parallelization[ue->current_thread_id[subframe_rx]][slot]);
        LOG_D(PHY, "[AbsSFN %d.%d] LLR Computation Symbol %d %5.2f \n",proc->frame_rx,subframe_rx,m,ue->dlsch_llr_stats_parallelization[ue->current_thread_id[subframe_rx]][slot].p_time/(cpuf*1000.0));
      }

      if(first_symbol_flag) {
        proc->first_symbol_available = 1;
      }
    } // CRNTI active
  }
}

void process_rar(PHY_VARS_UE *ue,
                 UE_rxtx_proc_t *proc,
                 int eNB_id,
                 runmode_t mode,
                 int abstraction_flag) {
  int frame_rx = proc->frame_rx;
  int subframe_rx = proc->subframe_rx;
  int timing_advance;
  LTE_UE_DLSCH_t *dlsch0 = ue->dlsch_ra[eNB_id];
  int harq_pid = 0;
  uint8_t *rar;
  uint8_t next1_thread_id = ue->current_thread_id[subframe_rx]== (RX_NB_TH-1) ? 0:(ue->current_thread_id[subframe_rx]+1);
  uint8_t next2_thread_id = next1_thread_id== (RX_NB_TH-1) ? 0:(next1_thread_id+1);
  LOG_D(PHY,"[UE  %d][RAPROC] Frame %d subframe %d Received RAR  mode %d\n",
        ue->Mod_id,
        frame_rx,
        subframe_rx, ue->UE_mode[eNB_id]);

  if (ue->mac_enabled == 1) {
    if ((ue->UE_mode[eNB_id] != PUSCH) &&
        (ue->prach_resources[eNB_id]->Msg3!=NULL)) {
      LOG_D(PHY,"[UE  %d][RAPROC] Frame %d subframe %d Invoking MAC for RAR (current preamble %d)\n",
            ue->Mod_id,frame_rx,
            subframe_rx,
            ue->prach_resources[eNB_id]->ra_PreambleIndex);
      timing_advance = ue_process_rar(ue->Mod_id,
                                      ue->CC_id,
                                      frame_rx,
                                      ue->prach_resources[eNB_id]->ra_RNTI,
                                      dlsch0->harq_processes[0]->b,
                                      &ue->pdcch_vars[ue->current_thread_id[subframe_rx]][eNB_id]->crnti,
                                      ue->prach_resources[eNB_id]->ra_PreambleIndex,
                                      dlsch0->harq_processes[0]->b); // alter the 'b' buffer so it contains only the selected RAR header and RAR payload
      ue->pdcch_vars[next1_thread_id][eNB_id]->crnti = ue->pdcch_vars[ue->current_thread_id[subframe_rx]][eNB_id]->crnti;
      ue->pdcch_vars[next2_thread_id][eNB_id]->crnti = ue->pdcch_vars[ue->current_thread_id[subframe_rx]][eNB_id]->crnti;

      if (timing_advance!=0xffff) {
        LOG_D(PHY,"[UE  %d][RAPROC] Frame %d subframe %d Got rnti %x and timing advance %d from RAR\n",
              ue->Mod_id,
              frame_rx,
              subframe_rx,
              ue->pdcch_vars[ue->current_thread_id[subframe_rx]][eNB_id]->crnti,
              timing_advance);
        // remember this c-rnti is still a tc-rnti
        ue->pdcch_vars[ue->current_thread_id[subframe_rx]][eNB_id]->crnti_is_temporary = 1;
        //timing_advance = 0;
        process_timing_advance_rar(ue,proc,timing_advance);

        if (mode!=debug_prach) {
          ue->ulsch_Msg3_active[eNB_id]=1;
          get_Msg3_alloc(&ue->frame_parms,
                         subframe_rx,
                         frame_rx,
                         &ue->ulsch_Msg3_frame[eNB_id],
                         &ue->ulsch_Msg3_subframe[eNB_id]);
          LOG_D(PHY,"[UE  %d][RAPROC] Got Msg3_alloc Frame %d subframe %d: Msg3_frame %d, Msg3_subframe %d\n",
                ue->Mod_id,
                frame_rx,
                subframe_rx,
                ue->ulsch_Msg3_frame[eNB_id],
                ue->ulsch_Msg3_subframe[eNB_id]);
          harq_pid = subframe2harq_pid(&ue->frame_parms,
                                       ue->ulsch_Msg3_frame[eNB_id],
                                       ue->ulsch_Msg3_subframe[eNB_id]);
          ue->ulsch[eNB_id]->harq_processes[harq_pid]->round = 0;
          ue->UE_mode[eNB_id] = RA_RESPONSE;
          //      ue->Msg3_timer[eNB_id] = 10;
          ue->ulsch[eNB_id]->power_offset = 6;
          ue->ulsch_no_allocation_counter[eNB_id] = 0;
        }
      } else { // PRACH preamble doesn't match RAR
        LOG_W(PHY,"[UE  %d][RAPROC] Received RAR preamble (%d) doesn't match !!!\n",
              ue->Mod_id,
              ue->prach_resources[eNB_id]->ra_PreambleIndex);
      }
    } // mode != PUSCH
  } else {
    rar = dlsch0->harq_processes[0]->b+1;
    timing_advance = ((((uint16_t)(rar[0]&0x7f))<<4) + (rar[1]>>4));
    process_timing_advance_rar(ue,proc,timing_advance);
  }
}

void ue_dlsch_procedures(PHY_VARS_UE *ue,
                         UE_rxtx_proc_t *proc,
                         int eNB_id,
                         PDSCH_t pdsch,
                         LTE_UE_DLSCH_t *dlsch0,
                         LTE_UE_DLSCH_t *dlsch1,
                         int *dlsch_errors,
                         runmode_t mode,
                         int abstraction_flag) {
  int harq_pid;
  int frame_rx = proc->frame_rx;
  int subframe_rx = proc->subframe_rx;
  int ret=0, ret1=0;
  int CC_id = ue->CC_id;
  LTE_UE_PDSCH *pdsch_vars;
  uint8_t is_cw0_active = 0;
  uint8_t is_cw1_active = 0;

  if (dlsch0==NULL)
    AssertFatal(0,"dlsch0 should be defined at this level \n");

  harq_pid = dlsch0->current_harq_pid;
  is_cw0_active = dlsch0->harq_processes[harq_pid]->status;

  if(dlsch1)
    is_cw1_active = dlsch1->harq_processes[harq_pid]->status;

  LOG_D(PHY,"AbsSubframe %d.%d Start Turbo Decoder for CW0 [harq_pid %d] ? %d \n", frame_rx%1024, subframe_rx, harq_pid, is_cw0_active);
  LOG_D(PHY,"AbsSubframe %d.%d Start Turbo Decoder for CW1 [harq_pid %d] ? %d \n", frame_rx%1024, subframe_rx, harq_pid, is_cw1_active);

  if(is_cw0_active && is_cw1_active) {
    dlsch0->Kmimo = 2;
    dlsch1->Kmimo = 2;
  } else {
    dlsch0->Kmimo = 1;
  }

  if (1) {
    switch (pdsch) {
      case SI_PDSCH:
        pdsch_vars = ue->pdsch_vars_SI[eNB_id];
        break;

      case RA_PDSCH:
        pdsch_vars = ue->pdsch_vars_ra[eNB_id];
        break;

      case P_PDSCH:
        pdsch_vars = ue->pdsch_vars_p[eNB_id];
        break;

      case PDSCH:
        pdsch_vars = ue->pdsch_vars[ue->current_thread_id[subframe_rx]][eNB_id];
        break;

      case PMCH:
      case PDSCH1:
        LOG_E(PHY,"Illegal PDSCH %d for ue_pdsch_procedures\n",pdsch);
        pdsch_vars = NULL;
        return;
        break;

      default:
        pdsch_vars = NULL;
        return;
        break;
    }

    if (frame_rx < *dlsch_errors)
      *dlsch_errors=0;

    if (pdsch==RA_PDSCH) {
      if (ue->prach_resources[eNB_id]!=NULL)
        dlsch0->rnti = ue->prach_resources[eNB_id]->ra_RNTI;
      else {
        LOG_E(PHY,"[UE %d] Frame %d, subframe %d: FATAL, prach_resources is NULL\n",ue->Mod_id,frame_rx,subframe_rx);
        AssertFatal(1==0,"prach_resources is NULL");
      }
    }

    // start turbo decode for CW 0
    dlsch0->harq_processes[harq_pid]->G = get_G(&ue->frame_parms,
                                          dlsch0->harq_processes[harq_pid]->nb_rb,
                                          dlsch0->harq_processes[harq_pid]->rb_alloc_even,
                                          dlsch0->harq_processes[harq_pid]->Qm,
                                          dlsch0->harq_processes[harq_pid]->Nl,
                                          ue->pdcch_vars[ue->current_thread_id[subframe_rx]][eNB_id]->num_pdcch_symbols,
                                          frame_rx,
                                          subframe_rx,
                                          ue->transmission_mode[eNB_id]<7?0:ue->transmission_mode[eNB_id]);

    if (LOG_DEBUGFLAG(UE_TIMING)) {
      start_meas(&ue->dlsch_unscrambling_stats);
    }

    dlsch_unscrambling(&ue->frame_parms,
                       0,
                       dlsch0,
                       dlsch0->harq_processes[harq_pid]->G,
                       pdsch_vars->llr[0],
                       0,
                       subframe_rx<<1);

    if (LOG_DEBUGFLAG(UE_TIMING)) {
      stop_meas(&ue->dlsch_unscrambling_stats);
    }

    LOG_D(PHY," ------ start turbo decoder for AbsSubframe %d.%d / %d  ------  \n", frame_rx, subframe_rx, harq_pid);
    LOG_D(PHY,"start turbo decode for CW 0 for AbsSubframe %d.%d / %d --> nb_rb %d \n", frame_rx, subframe_rx, harq_pid, dlsch0->harq_processes[harq_pid]->nb_rb);
    LOG_D(PHY,"start turbo decode for CW 0 for AbsSubframe %d.%d / %d  --> rb_alloc_even %x \n", frame_rx, subframe_rx, harq_pid,
          (unsigned int)((intptr_t)dlsch0->harq_processes[harq_pid]->rb_alloc_even));
    LOG_D(PHY,"start turbo decode for CW 0 for AbsSubframe %d.%d / %d  --> Qm %d \n", frame_rx, subframe_rx, harq_pid, dlsch0->harq_processes[harq_pid]->Qm);
    LOG_D(PHY,"start turbo decode for CW 0 for AbsSubframe %d.%d / %d  --> Nl %d \n", frame_rx, subframe_rx, harq_pid, dlsch0->harq_processes[harq_pid]->Nl);
    LOG_D(PHY,"start turbo decode for CW 0 for AbsSubframe %d.%d / %d  --> G  %d \n", frame_rx, subframe_rx, harq_pid, dlsch0->harq_processes[harq_pid]->G);
    LOG_D(PHY,"start turbo decode for CW 0 for AbsSubframe %d.%d / %d  --> Kmimo  %d \n", frame_rx, subframe_rx, harq_pid, dlsch0->Kmimo);
    LOG_D(PHY,"start turbo decode for CW 0 for AbsSubframe %d.%d / %d  --> Pdcch Sym  %d \n", frame_rx, subframe_rx, harq_pid,
          ue->pdcch_vars[ue->current_thread_id[subframe_rx]][eNB_id]->num_pdcch_symbols);

    if (LOG_DEBUGFLAG(UE_TIMING)) {
      start_meas(&ue->dlsch_decoding_stats[ue->current_thread_id[subframe_rx]]);
    }

    ret = dlsch_decoding(ue,
                         pdsch_vars->llr[0],
                         &ue->frame_parms,
                         dlsch0,
                         dlsch0->harq_processes[harq_pid],
                         frame_rx,
                         subframe_rx,
                         harq_pid,
                         pdsch==PDSCH?1:0,
                         dlsch0->harq_processes[harq_pid]->TBS>256?1:0);

    if (LOG_DEBUGFLAG(UE_TIMING)) {
      stop_meas(&ue->dlsch_decoding_stats[ue->current_thread_id[subframe_rx]]);
      LOG_I(PHY, " --> Unscrambling for CW0 %5.3f\n",
            (ue->dlsch_unscrambling_stats.p_time)/(cpuf*1000.0));
      LOG_I(PHY, "AbsSubframe %d.%d --> Turbo Decoding for CW0 %5.3f\n",
            frame_rx%1024, subframe_rx,(ue->dlsch_decoding_stats[ue->current_thread_id[subframe_rx]].p_time)/(cpuf*1000.0));
    }

    if(is_cw1_active) {
      // start turbo decode for CW 1
      dlsch1->harq_processes[harq_pid]->G = get_G(&ue->frame_parms,
                                            dlsch1->harq_processes[harq_pid]->nb_rb,
                                            dlsch1->harq_processes[harq_pid]->rb_alloc_even,
                                            dlsch1->harq_processes[harq_pid]->Qm,
                                            dlsch1->harq_processes[harq_pid]->Nl,
                                            ue->pdcch_vars[ue->current_thread_id[subframe_rx]][eNB_id]->num_pdcch_symbols,
                                            frame_rx,
                                            subframe_rx,
                                            ue->transmission_mode[eNB_id]<7?0:ue->transmission_mode[eNB_id]);

      if (LOG_DEBUGFLAG(UE_TIMING)) {
        start_meas(&ue->dlsch_unscrambling_stats);
      }

      dlsch_unscrambling(&ue->frame_parms,
                         0,
                         dlsch1,
                         dlsch1->harq_processes[harq_pid]->G,
                         pdsch_vars->llr[1],
                         1,
                         subframe_rx<<1);

      if (LOG_DEBUGFLAG(UE_TIMING)) {
        stop_meas(&ue->dlsch_unscrambling_stats);
      }

      LOG_D(PHY,"start turbo decode for CW 1 for AbsSubframe %d.%d / %d --> nb_rb %d \n", frame_rx, subframe_rx, harq_pid, dlsch1->harq_processes[harq_pid]->nb_rb);
      LOG_D(PHY,"start turbo decode for CW 1 for AbsSubframe %d.%d / %d  --> rb_alloc_even %x \n", frame_rx, subframe_rx, harq_pid, (uint16_t)((intptr_t)dlsch1->harq_processes[harq_pid]->rb_alloc_even));
      LOG_D(PHY,"start turbo decode for CW 1 for AbsSubframe %d.%d / %d  --> Qm %d \n", frame_rx, subframe_rx, harq_pid, dlsch1->harq_processes[harq_pid]->Qm);
      LOG_D(PHY,"start turbo decode for CW 1 for AbsSubframe %d.%d / %d  --> Nl %d \n", frame_rx, subframe_rx, harq_pid, dlsch1->harq_processes[harq_pid]->Nl);
      LOG_D(PHY,"start turbo decode for CW 1 for AbsSubframe %d.%d / %d  --> G  %d \n", frame_rx, subframe_rx, harq_pid, dlsch1->harq_processes[harq_pid]->G);
      LOG_D(PHY,"start turbo decode for CW 1 for AbsSubframe %d.%d / %d  --> Kmimo  %d \n", frame_rx, subframe_rx, harq_pid, dlsch1->Kmimo);
      LOG_D(PHY,"start turbo decode for CW 1 for AbsSubframe %d.%d / %d  --> Pdcch Sym  %d \n", frame_rx, subframe_rx, harq_pid,
            ue->pdcch_vars[ue->current_thread_id[subframe_rx]][eNB_id]->num_pdcch_symbols);

      if (LOG_DEBUGFLAG(UE_TIMING)) {
        start_meas(&ue->dlsch_decoding_stats[ue->current_thread_id[subframe_rx]]);
      }

      ret1 = dlsch_decoding(ue,
                            pdsch_vars->llr[1],
                            &ue->frame_parms,
                            dlsch1,
                            dlsch1->harq_processes[harq_pid],
                            frame_rx,
                            subframe_rx,
                            harq_pid,
                            pdsch==PDSCH?1:0,
                            dlsch1->harq_processes[harq_pid]->TBS>256?1:0);

      if (LOG_DEBUGFLAG(UE_TIMING)) {
        stop_meas(&ue->dlsch_decoding_stats[ue->current_thread_id[subframe_rx]]);
        LOG_I(PHY, " --> Unscrambling for CW1 %5.3f\n",
              (ue->dlsch_unscrambling_stats.p_time)/(cpuf*1000.0));
        LOG_I(PHY, "AbsSubframe %d.%d --> Turbo Decoding for CW1 %5.3f\n",
              frame_rx%1024, subframe_rx,(ue->dlsch_decoding_stats[ue->current_thread_id[subframe_rx]].p_time)/(cpuf*1000.0));
      }

      LOG_D(PHY,"AbsSubframe %d.%d --> Turbo Decoding for CW1 %5.3f\n",
            frame_rx%1024, subframe_rx,(ue->dlsch_decoding_stats[ue->current_thread_id[subframe_rx]].p_time)/(cpuf*1000.0));
    }

    LOG_D(PHY," ------ end turbo decoder for AbsSubframe %d.%d ------  \n", frame_rx, subframe_rx);

    // Check CRC for CW 0
    if (ret == (1+dlsch0->max_turbo_iterations)) {
      *dlsch_errors=*dlsch_errors+1;

      if(dlsch0->rnti != 0xffff) {
        LOG_D(PHY,"[UE  %d][PDSCH %x/%d] AbsSubframe %d.%d : DLSCH CW0 in error (rv %d,round %d, mcs %d,TBS %d)\n",
              ue->Mod_id,dlsch0->rnti,
              harq_pid,frame_rx,subframe_rx,
              dlsch0->harq_processes[harq_pid]->rvidx,
              dlsch0->harq_processes[harq_pid]->round,
              dlsch0->harq_processes[harq_pid]->mcs,
              dlsch0->harq_processes[harq_pid]->TBS);
      }
    } else {
      if(dlsch0->rnti != 0xffff) {
        LOG_D(PHY,"[UE  %d][PDSCH %x/%d] AbsSubframe %d.%d : Received DLSCH CW0 (rv %d,round %d, mcs %d,TBS %d)\n",
              ue->Mod_id,dlsch0->rnti,
              harq_pid,frame_rx,subframe_rx,
              dlsch0->harq_processes[harq_pid]->rvidx,
              dlsch0->harq_processes[harq_pid]->round,
              dlsch0->harq_processes[harq_pid]->mcs,
              dlsch0->harq_processes[harq_pid]->TBS);
      }

      if ( LOG_DEBUGFLAG(DEBUG_UE_PHYPROC)) {
        int j;
        LOG_D(PHY,"dlsch harq_pid %d (rx): \n",dlsch0->current_harq_pid);

        for (j=0; j<dlsch0->harq_processes[dlsch0->current_harq_pid]->TBS>>3; j++)
          LOG_T(PHY,"%x.",dlsch0->harq_processes[dlsch0->current_harq_pid]->b[j]);

        LOG_T(PHY,"\n");
      }

      if (ue->mac_enabled == 1) {
        switch (pdsch) {
          case PDSCH:
            ue_send_sdu(ue->Mod_id,
                        CC_id,
                        frame_rx,
                        subframe_rx,
                        dlsch0->harq_processes[dlsch0->current_harq_pid]->b,
                        dlsch0->harq_processes[dlsch0->current_harq_pid]->TBS>>3,
                        eNB_id);
            break;

          case SI_PDSCH:
            if(subframe_rx == 0) {
              ue_decode_si_mbms(ue->Mod_id,
                                CC_id,
                                frame_rx,
                                eNB_id,
                                ue->dlsch_SI[eNB_id]->harq_processes[0]->b,
                                ue->dlsch_SI[eNB_id]->harq_processes[0]->TBS>>3);
            } else {
              ue_decode_si(ue->Mod_id,
                           CC_id,
                           frame_rx,
                           eNB_id,
                           ue->dlsch_SI[eNB_id]->harq_processes[0]->b,
                           ue->dlsch_SI[eNB_id]->harq_processes[0]->TBS>>3);
            }
            break;

          case P_PDSCH:
            ue_decode_p(ue->Mod_id,
                        CC_id,
                        frame_rx,
                        eNB_id,
                        ue->dlsch_SI[eNB_id]->harq_processes[0]->b,
                        ue->dlsch_SI[eNB_id]->harq_processes[0]->TBS>>3);
            break;

          case RA_PDSCH:
            process_rar(ue,proc,eNB_id,mode,abstraction_flag);
            break;

          case PDSCH1:
            LOG_E(PHY,"Shouldn't have PDSCH1 yet, come back later\n");
            AssertFatal(1==0,"exiting");
            break;

          case PMCH:
            LOG_E(PHY,"Shouldn't have PMCH here\n");
            AssertFatal(1==0,"exiting");
            break;
        }
      }

      ue->total_TBS[eNB_id] =  ue->total_TBS[eNB_id] +
                               dlsch0->harq_processes[dlsch0->current_harq_pid]->TBS;
      ue->total_received_bits[eNB_id] = ue->total_TBS[eNB_id] +
                                        dlsch0->harq_processes[dlsch0->current_harq_pid]->TBS;
    }

    // Check CRC for CW 1
    if(is_cw1_active) {
      if (ret1 == (1+dlsch0->max_turbo_iterations)) {
        LOG_D(PHY,"[UE  %d][PDSCH %x/%d] Frame %d subframe %d DLSCH CW1 in error (rv %d,mcs %d,TBS %d)\n",
              ue->Mod_id,dlsch0->rnti,
              harq_pid,frame_rx,subframe_rx,
              dlsch0->harq_processes[harq_pid]->rvidx,
              dlsch0->harq_processes[harq_pid]->mcs,
              dlsch0->harq_processes[harq_pid]->TBS);
      } else {
        LOG_D(PHY,"[UE  %d][PDSCH %x/%d] Frame %d subframe %d: Received DLSCH CW1 (rv %d,mcs %d,TBS %d)\n",
              ue->Mod_id,dlsch0->rnti,
              harq_pid,frame_rx,subframe_rx,
              dlsch0->harq_processes[harq_pid]->rvidx,
              dlsch0->harq_processes[harq_pid]->mcs,
              dlsch0->harq_processes[harq_pid]->TBS);

        if (ue->mac_enabled == 1) {
          switch (pdsch) {
            case PDSCH:
              if(is_cw1_active)
                ue_send_sdu(ue->Mod_id,
                            CC_id,
                            frame_rx,
                            subframe_rx,
                            dlsch1->harq_processes[dlsch1->current_harq_pid]->b,
                            dlsch1->harq_processes[dlsch1->current_harq_pid]->TBS>>3,
                            eNB_id);

              break;

            case SI_PDSCH:
            case P_PDSCH:
            case RA_PDSCH:
            case PDSCH1:
            case PMCH:
              AssertFatal(0,"exiting");
              break;
          }
        }
      }
    }

    if (LOG_DEBUGFLAG(DEBUG_UE_PHYPROC)) {
      LOG_D(PHY,"[UE  %d][PDSCH %x/%d] Frame %d subframe %d: PDSCH/DLSCH decoding iter %d (mcs %d, rv %d, TBS %d)\n",
            ue->Mod_id,
            dlsch0->rnti,harq_pid,
            frame_rx,subframe_rx,ret,
            dlsch0->harq_processes[harq_pid]->mcs,
            dlsch0->harq_processes[harq_pid]->rvidx,
            dlsch0->harq_processes[harq_pid]->TBS);

      if (frame_rx%100==0) {
        LOG_D(PHY,"[UE  %d][PDSCH %x] Frame %d subframe %d dlsch_errors %d, dlsch_received %d, dlsch_fer %d, current_dlsch_cqi %d\n",
              ue->Mod_id,dlsch0->rnti,
              frame_rx,subframe_rx,
              ue->dlsch_errors[eNB_id],
              ue->dlsch_received[eNB_id],
              ue->dlsch_fer[eNB_id],
              ue->measurements.wideband_cqi_tot[eNB_id]);
      }
    } /*LOG_DEBUGFLAG(DEBUG_UE_PHYPROC) */
  }
}

/*!
 * \brief This is the UE synchronize thread.
 * It performs band scanning and synchonization.
 * \param arg is a pointer to a \ref PHY_VARS_UE structure.
 * \returns a pointer to an int. The storage is not on the heap and must not be freed.
 */
#ifdef UE_SLOT_PARALLELISATION
#define FIFO_PRIORITY   40
void *UE_thread_slot1_dl_processing(void *arg) {
  static __thread int UE_dl_slot1_processing_retval;
  struct rx_tx_thread_data *rtd = arg;
  UE_rxtx_proc_t *proc = rtd->proc;
  PHY_VARS_UE    *ue   = rtd->UE;
  int frame_rx;
  uint8_t subframe_rx;
  uint8_t pilot0;
  uint8_t pilot1;
  uint8_t slot1;
  uint8_t next_subframe_rx;
  uint8_t next_subframe_slot0;
  proc->instance_cnt_slot1_dl_processing=-1;
  proc->subframe_rx=proc->sub_frame_start;
  char threadname[256];
  sprintf(threadname,"UE_thread_slot1_dl_processing_%d", proc->sub_frame_start);
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);

  if ( (proc->sub_frame_start+1)%RX_NB_TH == 0 && threads.slot1_proc_one != -1 )
    CPU_SET(threads.slot1_proc_one, &cpuset);

  if ( (proc->sub_frame_start+1)%RX_NB_TH == 1 && threads.slot1_proc_two != -1 )
    CPU_SET(threads.slot1_proc_two, &cpuset);

  // cppcheck-suppress moduloAlwaysTrueFalse
  if ( (proc->sub_frame_start+1)%RX_NB_TH == 2 && threads.slot1_proc_three != -1 )
    CPU_SET(threads.slot1_proc_three, &cpuset);

  init_thread(900000,1000000, FIFO_PRIORITY-1, &cpuset,
              threadname);

  while (!oai_exit) {
    if (pthread_mutex_lock(&proc->mutex_slot1_dl_processing) != 0) {
      LOG_E( PHY, "[SCHED][UE] error locking mutex for UE slot1 dl processing\n" );
      exit_fun("nothing to add");
    }

    while (proc->instance_cnt_slot1_dl_processing < 0) {
      // most of the time, the thread is waiting here
      pthread_cond_wait( &proc->cond_slot1_dl_processing, &proc->mutex_slot1_dl_processing );
    }

    if (pthread_mutex_unlock(&proc->mutex_slot1_dl_processing) != 0) {
      LOG_E( PHY, "[SCHED][UE] error unlocking mutex for UE slot1 dl processing \n" );
      exit_fun("nothing to add");
    }

    /*for(int th_idx=0; th_idx< RX_NB_TH; th_idx++)
    {
    frame_rx    = ue->proc.proc_rxtx[0].frame_rx;
    subframe_rx = ue->proc.proc_rxtx[0].subframe_rx;
    printf("AbsSubframe %d.%d execute dl slot1 processing \n", frame_rx, subframe_rx);
    }*/
    frame_rx    = proc->frame_rx;
    subframe_rx = proc->subframe_rx;
    next_subframe_rx    = (1+subframe_rx)%10;
    next_subframe_slot0 = next_subframe_rx<<1;
    slot1  = (subframe_rx<<1) + 1;
    pilot0 = 0;

    //printf("AbsSubframe %d.%d execute dl slot1 processing \n", frame_rx, subframe_rx);

    if (ue->frame_parms.Ncp == 0) {  // normal prefix
      pilot1 = 4;
    } else { // extended prefix
      pilot1 = 3;
    }

    /**** Slot1 FE Processing ****/
    if (LOG_DEBUGFLAG(UE_TIMING)) {
      start_meas(&ue->ue_front_end_per_slot_stat[ue->current_thread_id[subframe_rx]][1]);
    }

    // I- start dl slot1 processing
    // do first symbol of next downlink subframe for channel estimation
    /*
    // 1- perform FFT for pilot ofdm symbols first (ofdmSym0 next subframe ofdmSym11)
    if (subframe_select(&ue->frame_parms,next_subframe_rx) != SF_UL)
    {
        front_end_fft(ue,
                pilot0,
                next_subframe_slot0,
                0,
                0);
    }

    front_end_fft(ue,
            pilot1,
            slot1,
            0,
            0);
     */
    // 1- perform FFT
    for (int l=1; l<ue->frame_parms.symbols_per_tti>>1; l++) {
      //if( (l != pilot0) && (l != pilot1))
      {
        if (LOG_DEBUGFLAG(UE_TIMING)) {
          start_meas(&ue->ofdm_demod_stats);
        }

        VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_SLOT_FEP, VCD_FUNCTION_IN);
        //printf("AbsSubframe %d.%d FFT slot %d, symbol %d\n", frame_rx,subframe_rx,slot1,l);
        front_end_fft(ue,
                      l,
                      slot1,
                      0,
                      0);
        VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_SLOT_FEP, VCD_FUNCTION_OUT);

        if (LOG_DEBUGFLAG(UE_TIMING)) {
          stop_meas(&ue->ofdm_demod_stats);
        }
      }
    } // for l=1..l2

    if (subframe_select(&ue->frame_parms,next_subframe_rx) != SF_UL) {
      //printf("AbsSubframe %d.%d FFT slot %d, symbol %d\n", frame_rx,subframe_rx,next_subframe_slot0,pilot0);
      front_end_fft(ue,
                    pilot0,
                    next_subframe_slot0,
                    0,
                    0);
    }

    // 2- perform Channel Estimation for slot1
    for (int l=1; l<ue->frame_parms.symbols_per_tti>>1; l++) {
      if(l == pilot1) {
        //wait until channel estimation for pilot0/slot1 is available
        uint32_t wait = 0;

        while(proc->chan_est_pilot0_slot1_available == 0) {
          usleep(1);
          wait++;
        }

        //printf("[slot1 dl processing] ChanEst symbol %d slot %d wait%d\n",l,slot1,wait);
      }

      //printf("AbsSubframe %d.%d ChanEst slot %d, symbol %d\n", frame_rx,subframe_rx,slot1,l);
      front_end_chanEst(ue,
                        l,
                        slot1,
                        0);
      ue_measurement_procedures(l-1,ue,proc,0,1+(subframe_rx<<1),0,ue->mode);
    }

    //printf("AbsSubframe %d.%d ChanEst slot %d, symbol %d\n", frame_rx,subframe_rx,next_subframe_slot0,pilot0);
    front_end_chanEst(ue,
                      pilot0,
                      next_subframe_slot0,
                      0);

    if ( (subframe_rx == 0) && (ue->decode_MIB == 1)) {
      ue_pbch_procedures(0,ue,proc,0);
    }

    proc->chan_est_slot1_available = 1;
    //printf("Set available slot 1channelEst to 1 AbsSubframe %d.%d \n",frame_rx,subframe_rx);
    //printf(" [slot1 dl processing] ==> FFT/CHanEst Done for AbsSubframe %d.%d \n", proc->frame_rx, proc->subframe_rx);

    //printf(" [slot1 dl processing] ==> Start LLR Comuptation slot1 for AbsSubframe %d.%d \n", proc->frame_rx, proc->subframe_rx);

    if ( LOG_DEBUGFLAG(UE_TIMING)) {
      stop_meas(&ue->ue_front_end_per_slot_stat[ue->current_thread_id[subframe_rx]][1]);
      LOG_D(PHY, "[AbsSFN %d.%d] Slot1: FFT + Channel Estimate + Pdsch Proc Slot0 %5.2f \n",frame_rx,subframe_rx,ue->ue_front_end_per_slot_stat[ue->current_thread_id[subframe_rx]][1].p_time/(cpuf*1000.0));
    }

    //wait until pdcch is decoded
    uint32_t wait = 0;

    while(proc->dci_slot0_available == 0) {
      usleep(1);
      wait++;
    }

    //printf("[slot1 dl processing] AbsSubframe %d.%d LLR Computation Start wait DCI %d\n",frame_rx,subframe_rx,wait);

    /**** Pdsch Procedure Slot1 ****/
    // start slot1 thread for Pdsch Procedure (slot1)
    // do procedures for C-RNTI
    //printf("AbsSubframe %d.%d Pdsch Procedure (slot1)\n",frame_rx,subframe_rx);

    if ( LOG_DEBUGFLAG(UE_TIMING)) {
      start_meas(&ue->pdsch_procedures_per_slot_stat[ue->current_thread_id[subframe_rx]][1]);
    }

    // start slave thread for Pdsch Procedure (slot1)
    // do procedures for C-RNTI
    uint8_t eNB_id = 0;
    uint8_t abstraction_flag = 0;

    if (ue->dlsch[ue->current_thread_id[subframe_rx]][eNB_id][0]->active == 1) {
      //wait until first ofdm symbol is processed
      //wait = 0;
      //while(proc->first_symbol_available == 0)
      //{
      //    usleep(1);
      //    wait++;
      //}
      //printf("[slot1 dl processing] AbsSubframe %d.%d LLR Computation Start wait First Ofdm Sym %d\n",frame_rx,subframe_rx,wait);
      //VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PDSCH_PROC, VCD_FUNCTION_IN);
      ue_pdsch_procedures(ue,
                          proc,
                          eNB_id,
                          PDSCH,
                          ue->dlsch[ue->current_thread_id[subframe_rx]][eNB_id][0],
                          NULL,
                          (ue->frame_parms.symbols_per_tti>>1),
                          ue->frame_parms.symbols_per_tti-1,
                          abstraction_flag);
      LOG_D(PHY," ------ end PDSCH ChannelComp/LLR slot 0: AbsSubframe %d.%d ------  \n", frame_rx%1024, subframe_rx);
      LOG_D(PHY," ------ --> PDSCH Turbo Decoder slot 0/1: AbsSubframe %d.%d ------  \n", frame_rx%1024, subframe_rx);
    }

    // do procedures for SI-RNTI
    if ((ue->dlsch_SI[eNB_id]) && (ue->dlsch_SI[eNB_id]->active == 1)) {
      ue_pdsch_procedures(ue,
                          proc,
                          eNB_id,
                          SI_PDSCH,
                          ue->dlsch_SI[eNB_id],
                          NULL,
                          (ue->frame_parms.symbols_per_tti>>1),
                          ue->frame_parms.symbols_per_tti-1,
                          abstraction_flag);
    }

    // do procedures for P-RNTI
    if ((ue->dlsch_p[eNB_id]) && (ue->dlsch_p[eNB_id]->active == 1)) {
      ue_pdsch_procedures(ue,
                          proc,
                          eNB_id,
                          P_PDSCH,
                          ue->dlsch_p[eNB_id],
                          NULL,
                          (ue->frame_parms.symbols_per_tti>>1),
                          ue->frame_parms.symbols_per_tti-1,
                          abstraction_flag);
    }

    // do procedures for RA-RNTI
    if ((ue->dlsch_ra[eNB_id]) && (ue->dlsch_ra[eNB_id]->active == 1)) {
      ue_pdsch_procedures(ue,
                          proc,
                          eNB_id,
                          RA_PDSCH,
                          ue->dlsch_ra[eNB_id],
                          NULL,
                          (ue->frame_parms.symbols_per_tti>>1),
                          ue->frame_parms.symbols_per_tti-1,
                          abstraction_flag);
    }

    proc->llr_slot1_available=1;
    //printf("Set available LLR slot1 to 1 AbsSubframe %d.%d \n",frame_rx,subframe_rx);

    if ( LOG_DEBUGFLAG(UE_TIMING)) {
      stop_meas(&ue->pdsch_procedures_per_slot_stat[ue->current_thread_id[subframe_rx]][1]);
      LOG_D(PHY, "[AbsSFN %d.%d] Slot1: LLR Computation %5.2f \n",frame_rx,subframe_rx,ue->pdsch_procedures_per_slot_stat[ue->current_thread_id[subframe_rx]][1].p_time/(cpuf*1000.0));
    }

    if (pthread_mutex_lock(&proc->mutex_slot1_dl_processing) != 0) {
      LOG_E( PHY, "[SCHED][UE] error locking mutex for UE RXTX\n" );
      exit_fun("noting to add");
    }

    proc->instance_cnt_slot1_dl_processing--;

    if (pthread_mutex_unlock(&proc->mutex_slot1_dl_processing) != 0) {
      LOG_E( PHY, "[SCHED][UE] error unlocking mutex for UE FEP Slo1\n" );
      exit_fun("noting to add");
    }
  }

  // thread finished
  free(arg);
  return &UE_dl_slot1_processing_retval;
}
#endif

#ifdef UE_SLOT_PARALLELISATION
int phy_procedures_slot_parallelization_UE_RX(PHY_VARS_UE *ue,
    UE_rxtx_proc_t *proc,
    uint8_t eNB_id,
    uint8_t abstraction_flag,
    uint8_t do_pdcch_flag,
    runmode_t mode) {
  int l,l2;
  int pmch_flag=0;
  int frame_rx = proc->frame_rx;
  int subframe_rx = proc->subframe_rx;
  uint8_t pilot0;
  uint8_t pilot1;
  uint8_t slot0;
  uint8_t slot1;
  uint8_t first_ofdm_sym;
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_UE_RX, VCD_FUNCTION_IN);
  T(T_UE_PHY_DL_TICK, T_INT(ue->Mod_id), T_INT(frame_rx%1024), T_INT(subframe_rx));
  T(T_UE_PHY_INPUT_SIGNAL, T_INT(ue->Mod_id), T_INT(frame_rx%1024), T_INT(subframe_rx), T_INT(0),
    T_BUFFER(&ue->common_vars.rxdata[0][subframe_rx*ue->frame_parms.samples_per_tti],
             ue->frame_parms.samples_per_tti * 4));

  // start timers
  if (LOG_DEBUGFLAG(DEBUG_UE_PHYPROC)) {
    LOG_D(PHY," ****** start RX-Chain for AbsSubframe %d.%d ******  \n", frame_rx%1024, subframe_rx);
  }

  if (LOG_DEBUGFLAG(UE_TIMING)) {
    start_meas(&ue->phy_proc_rx[ue->current_thread_id[subframe_rx]]);
    start_meas(&ue->ue_front_end_stat[ue->current_thread_id[subframe_rx]]);
  }

  pmch_flag = is_pmch_subframe(frame_rx,subframe_rx,&ue->frame_parms) ? 1 : 0;

  if (do_pdcch_flag) {
    // deactivate reception until we scan pdcch
    if (ue->dlsch[ue->current_thread_id[subframe_rx]][eNB_id][0])
      ue->dlsch[ue->current_thread_id[subframe_rx]][eNB_id][0]->active = 0;

    if (ue->dlsch[ue->current_thread_id[subframe_rx]][eNB_id][1])
      ue->dlsch[ue->current_thread_id[subframe_rx]][eNB_id][1]->active = 0;

    if (ue->dlsch_SI[eNB_id])
      ue->dlsch_SI[eNB_id]->active = 0;

    if (ue->dlsch_p[eNB_id])
      ue->dlsch_p[eNB_id]->active = 0;

    if (ue->dlsch_ra[eNB_id])
      ue->dlsch_ra[eNB_id]->active = 0;
  }

  if ( LOG_DEBUG_FLAG(DEBUG_UE_PHYPROC)) {
    LOG_D(PHY,"[UE %d] Frame %d subframe %d: Doing phy_procedures_UE_RX\n",
          ue->Mod_id,frame_rx, subframe_rx);
  }

  if (subframe_select(&ue->frame_parms,subframe_rx) == SF_S) { // S-subframe, do first 5 symbols only
    l2 = 4;
  } else if (pmch_flag == 1) { // do first 2 symbols only
    l2 = 1;
  } else { // normal subframe, last symbol to be processed is the first of the second slot
    l2 = (ue->frame_parms.symbols_per_tti/2)-1;
  }

  int prev_subframe_rx = (subframe_rx - 1)<0? 9: (subframe_rx - 1);

  if (subframe_select(&ue->frame_parms,prev_subframe_rx) != SF_DL) {
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    // RX processing of symbols l=0...l2
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    first_ofdm_sym = 0;
  } else {
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    // RX processing of symbols l=1...l2 (l=0 is done in last scheduling epoch)
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    first_ofdm_sym = 1;
  }

  slot0  = (subframe_rx<<1);
  slot1  = (subframe_rx<<1) + 1;
  pilot0 = 0;

  if (ue->frame_parms.Ncp == 0) {  // normal prefix
    pilot1 = 4;
  } else { // extended prefix
    pilot1 = 3;
  }

  //LOG_I(PHY,"Set available channelEst to 0 AbsSubframe %d.%d \n",frame_rx,subframe_rx);
  //LOG_I(PHY,"Set available llrs slot1 to 0 AbsSubframe %d.%d \n",frame_rx,subframe_rx);
  //LOG_I(PHY,"Set available dci info slot0 to 0 AbsSubframe %d.%d \n",frame_rx,subframe_rx);
  proc->chan_est_pilot0_slot1_available=0;
  proc->llr_slot1_available=0;
  proc->dci_slot0_available=0;
  proc->first_symbol_available=0;
  proc->chan_est_slot1_available=0;
  //proc->channel_level=0;

  if (pthread_mutex_lock(&proc->mutex_slot1_dl_processing) != 0) {
    LOG_E( PHY, "[SCHED][UE %d][Slot0] error locking mutex for UE slot1 dl processing\n",ue->Mod_id );
    exit_fun("nothing to add");
  }

  proc->instance_cnt_slot1_dl_processing++;

  if (proc->instance_cnt_slot1_dl_processing == 0) {
    LOG_D(PHY,"unblock slot1 dl processing thread blocked on instance_cnt_slot1_dl_processing : %d \n", proc->instance_cnt_slot1_dl_processing );

    if (pthread_cond_signal(&proc->cond_slot1_dl_processing) != 0) {
      LOG_E( PHY, "[SCHED][UE %d][Slot0] ERROR pthread_cond_signal for UE slot1 processing thread\n", ue->Mod_id);
      exit_fun("nothing to add");
    }

    if (pthread_mutex_unlock(&proc->mutex_slot1_dl_processing) != 0) {
      LOG_E( PHY, "[SCHED][UE %d][Slot0] error unlocking mutex for UE slot1 dl processing \n",ue->Mod_id );
      exit_fun("nothing to add");
    }
  } else {
    LOG_E( PHY, "[SCHED][UE %d] UE RX thread busy (IC %d)!!\n", ue->Mod_id, proc->instance_cnt_slot1_dl_processing);

    if (proc->instance_cnt_slot1_dl_processing > 2)
      exit_fun("instance_cnt_slot1_dl_processing > 2");
  }

  //AssertFatal(pthread_cond_signal(&proc->cond_slot1_dl_processing) ==0 ,"");
  AssertFatal(pthread_mutex_unlock(&proc->mutex_slot1_dl_processing) ==0,"");

  /**** Slot0 FE Processing ****/
  // I- start main thread for FFT/ChanEst symbol: 0/1 --> 7
  if ( LOG_DEBUGFLAG(UE_TIMING)) {
    start_meas(&ue->ue_front_end_per_slot_stat[ue->current_thread_id[subframe_rx]][0]);
  }

  // 1- perform FFT for pilot ofdm symbols first (ofdmSym7 ofdmSym4 or (ofdmSym6 ofdmSym3))
  //printf("AbsSubframe %d.%d FFT slot %d, symbol %d\n", frame_rx,subframe_rx,slot1,pilot0);
  front_end_fft(ue,
                pilot0,
                slot1,
                0,
                0);
  //printf("AbsSubframe %d.%d FFT slot %d, symbol %d\n", frame_rx,subframe_rx,slot0,pilot1);
  front_end_fft(ue,
                pilot1,
                slot0,
                0,
                0);
  //printf("AbsSubframe %d.%d ChanEst slot %d, symbol %d\n", frame_rx,subframe_rx,slot0,pilot1);
  front_end_chanEst(ue,
                    pilot1,
                    slot0,
                    0);
  //printf("AbsSubframe %d.%d ChanEst slot %d, symbol %d\n", frame_rx,subframe_rx,slot1,pilot0);
  front_end_chanEst(ue,
                    pilot0,
                    slot1,
                    0);
  proc->chan_est_pilot0_slot1_available = 1;
  //printf("Set available channelEst to 1 AbsSubframe %d.%d \n",frame_rx,subframe_rx);

  // 2- perform FFT for other ofdm symbols other than pilots
  for (l=first_ofdm_sym; l<=l2; l++) {
    if( (l != pilot0) && (l != pilot1)) {
      //printf("AbsSubframe %d.%d FFT slot %d, symbol %d\n", frame_rx,subframe_rx,slot0,l);
      if (LOG_DEBUGFLAG(UE_TIMING)) {
        start_meas(&ue->ofdm_demod_stats);
      }

      VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_SLOT_FEP, VCD_FUNCTION_IN);
      front_end_fft(ue,
                    l,
                    slot0,
                    0,
                    0);
      VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_SLOT_FEP, VCD_FUNCTION_OUT);

      if (LOG_DEBUGFLAG(UE_TIMING)) {
        stop_meas(&ue->ofdm_demod_stats);
      }
    }
  } // for l=1..l2

  // 3- perform Channel Estimation for slot0
  for (l=first_ofdm_sym; l<=l2; l++) {
    if( (l != pilot0) && (l != pilot1)) {
      //printf("AbsSubframe %d.%d ChanEst slot %d, symbol %d\n", frame_rx,subframe_rx,slot0,l);
      front_end_chanEst(ue,
                        l,
                        slot0,
                        0);
    }

    ue_measurement_procedures(l-1,ue,proc,eNB_id,(subframe_rx<<1),abstraction_flag,mode);
  }

  if (do_pdcch_flag) {
    if (LOG_DEBUGFLAG(UE_TIMING)) {
      start_meas(&ue->pdcch_procedures_stat[ue->current_thread_id[subframe_rx]]);
    }

    if (ue_pdcch_procedures(eNB_id,ue,proc,abstraction_flag) == -1) {
      LOG_E(PHY,"[UE  %d] Frame %d, subframe %d: Error in pdcch procedures\n",ue->Mod_id,frame_rx,subframe_rx);

      if (LOG_DEBUGFLAG(UE_TIMING)) {
        LOG_D(PHY, "[AbsSFN %d.%d] Slot0: PDCCH %5.2f \n",frame_rx,subframe_rx,ue->pdcch_procedures_stat[ue->current_thread_id[subframe_rx]].p_time/(cpuf*1000.0));
      }

      //proc->dci_slot0_available = 1;
      return(-1);
    }

    //proc->dci_slot0_available=1;
    if (LOG_DEBUGFLAG(UE_TIMING)) {
      stop_meas(&ue->pdcch_procedures_stat[ue->current_thread_id[subframe_rx]]);
      LOG_D(PHY, "[AbsSFN %d.%d] Slot0: PDCCH %5.2f \n",frame_rx,subframe_rx,ue->pdcch_procedures_stat[ue->current_thread_id[subframe_rx]].p_time/(cpuf*1000.0));
    }
  }

  //printf("num_pdcch_symbols %d\n",ue->pdcch_vars[ue->current_thread_id[subframe_rx]][eNB_id]->num_pdcch_symbols);

  // first slot has been processed (FFTs + Channel Estimation, PCFICH/PHICH/PDCCH)
  if (LOG_DEBUGFLAG(UE_TIMING)) {
    stop_meas(&ue->ue_front_end_per_slot_stat[ue->current_thread_id[subframe_rx]][0]);
    LOG_D(PHY, "[AbsSFN %d.%d] Slot0: FFT + Channel Estimate + PCFICH/PHICH/PDCCH %5.2f \n",frame_rx,subframe_rx,
          ue->ue_front_end_per_slot_stat[ue->current_thread_id[subframe_rx]][0].p_time/(cpuf*1000.0));
  }

  //wait until slot1 FE is done
  uint32_t wait = 0;

  while(proc->chan_est_slot1_available == 0) {
    usleep(1);
    wait++;
  }

  if (LOG_DEBUGFLAG(UE_TIMING)) {
    stop_meas(&ue->ue_front_end_stat[ue->current_thread_id[subframe_rx]]);
    LOG_D(PHY, "[AbsSFN %d.%d] FULL FE Processing %5.2f \n",frame_rx,subframe_rx,ue->ue_front_end_per_slot_stat[ue->current_thread_id[subframe_rx]][0].p_time/(cpuf*1000.0));
  }

  /**** End Subframe FE Processing ****/

  //Trigger LLR parallelized for Slot 1
  //proc->dci_slot0_available=1;
  //printf("Set available dci slot0 to 1 AbsSubframe %d.%d \n",frame_rx%1024,subframe_rx);

  /**** Pdsch Procedure Slot0 ****/
  // start main thread for Pdsch Procedure (slot0)
  // do procedures for C-RNTI
  //printf("AbsSubframe %d.%d Pdsch Procedure (slot0)\n",frame_rx%1024,subframe_rx);
  //printf("AbsSubframe %d.%d Pdsch Procedure PDSCH Active %d \n",frame_rx%1024,subframe_rx, ue->dlsch[ue->current_thread_id[subframe_rx]][0][0]->active);

  if (LOG_DEBUGFLAG(UE_TIMING)) {
    start_meas(&ue->pdsch_procedures_stat[ue->current_thread_id[subframe_rx]]);
  }

  if (LOG_DEBUGFLAG(UE_TIMING)) {
    start_meas(&ue->pdsch_procedures_per_slot_stat[ue->current_thread_id[subframe_rx]][0]);
  }

  if (ue->dlsch[ue->current_thread_id[subframe_rx]][eNB_id][0]->active == 1) {
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PDSCH_PROC, VCD_FUNCTION_IN);
    ue_pdsch_procedures(ue,
                        proc,
                        eNB_id,
                        PDSCH,
                        ue->dlsch[ue->current_thread_id[subframe_rx]][eNB_id][0],
                        NULL,
                        ue->pdcch_vars[ue->current_thread_id[subframe_rx]][eNB_id]->num_pdcch_symbols,
                        (ue->frame_parms.symbols_per_tti>>1)-1,
                        abstraction_flag);
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PDSCH_PROC, VCD_FUNCTION_OUT);
  }

  // do procedures for SI-RNTI
  if ((ue->dlsch_SI[eNB_id]) && (ue->dlsch_SI[eNB_id]->active == 1)) {
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PDSCH_PROC_SI, VCD_FUNCTION_IN);
    ue_pdsch_procedures(ue,
                        proc,
                        eNB_id,
                        SI_PDSCH,
                        ue->dlsch_SI[eNB_id],
                        NULL,
                        ue->pdcch_vars[ue->current_thread_id[subframe_rx]][eNB_id]->num_pdcch_symbols,
                        (ue->frame_parms.symbols_per_tti>>1)-1,
                        abstraction_flag);
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PDSCH_PROC_SI, VCD_FUNCTION_OUT);
  }

  // do procedures for SI-RNTI
  if ((ue->dlsch_p[eNB_id]) && (ue->dlsch_p[eNB_id]->active == 1)) {
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PDSCH_PROC_P, VCD_FUNCTION_IN);
    ue_pdsch_procedures(ue,
                        proc,
                        eNB_id,
                        P_PDSCH,
                        ue->dlsch_p[eNB_id],
                        NULL,
                        ue->pdcch_vars[ue->current_thread_id[subframe_rx]][eNB_id]->num_pdcch_symbols,
                        (ue->frame_parms.symbols_per_tti>>1)-1,
                        abstraction_flag);
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PDSCH_PROC_P, VCD_FUNCTION_OUT);
  }

  // do procedures for RA-RNTI
  if ((ue->dlsch_ra[eNB_id]) && (ue->dlsch_ra[eNB_id]->active == 1)) {
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PDSCH_PROC_RA, VCD_FUNCTION_IN);
    ue_pdsch_procedures(ue,
                        proc,
                        eNB_id,
                        RA_PDSCH,
                        ue->dlsch_ra[eNB_id],
                        NULL,
                        ue->pdcch_vars[ue->current_thread_id[subframe_rx]][eNB_id]->num_pdcch_symbols,
                        (ue->frame_parms.symbols_per_tti>>1)-1,
                        abstraction_flag);
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PDSCH_PROC_RA, VCD_FUNCTION_OUT);
  }

  // LLR linear
  proc->dci_slot0_available=1;
  //printf("Set available dci slot0 to 1 AbsSubframe %d.%d \n",frame_rx%1024,subframe_rx);

  if (LOG_DEBUGFLAG(UE_TIMING)) {
    stop_meas(&ue->pdsch_procedures_per_slot_stat[ue->current_thread_id[subframe_rx]][0]);
    LOG_D(PHY, "[AbsSFN %d.%d] Slot0: LLR Computation %5.2f \n",frame_rx,subframe_rx,ue->pdsch_procedures_per_slot_stat[ue->current_thread_id[subframe_rx]][0].p_time/(cpuf*1000.0));
  }

  //wait until LLR Slot1 is done
  wait = 0;

  while(proc->llr_slot1_available == 0) {
    usleep(1);
    wait++;
  }

  if (LOG_DEBUGFLAG(UE_TIMING)) {
    stop_meas(&ue->pdsch_procedures_stat[ue->current_thread_id[subframe_rx]]);
    LOG_D(PHY, "[AbsSFN %d.%d] Full LLR Computation %5.2f \n",frame_rx,subframe_rx,ue->pdsch_procedures_stat[ue->current_thread_id[subframe_rx]].p_time/(cpuf*1000.0));
  }

  //=====================================================================//
  if (LOG_DEBUGFLAG(UE_TIMING)) {
    start_meas(&ue->dlsch_procedures_stat[ue->current_thread_id[subframe_rx]]);
  }

  LOG_D(PHY,"==> Start Turbo Decoder active dlsch %d SI %d RA %d \n",ue->dlsch[ue->current_thread_id[subframe_rx]][eNB_id][0]->active,
        ue->dlsch_SI[eNB_id]->active,
        //ue->dlsch_p[eNB_id]->active,
        ue->dlsch_ra[eNB_id]->active);

  // Start Turbo decoder
  if (ue->dlsch[ue->current_thread_id[subframe_rx]][eNB_id][0]->active == 1) {
    //VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PDSCH_PROC, VCD_FUNCTION_IN);
    ue_dlsch_procedures(ue,
                        proc,
                        eNB_id,
                        PDSCH,
                        ue->dlsch[ue->current_thread_id[subframe_rx]][eNB_id][0],
                        ue->dlsch[ue->current_thread_id[subframe_rx]][eNB_id][1],
                        &ue->dlsch_errors[eNB_id],
                        mode,
                        abstraction_flag);
    //VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PDSCH_PROC, VCD_FUNCTION_OUT);
  }

  // do procedures for SI-RNTI
  if ((ue->dlsch_SI[eNB_id]) && (ue->dlsch_SI[eNB_id]->active == 1)) {
    ue_dlsch_procedures(ue,
                        proc,
                        eNB_id,
                        SI_PDSCH,
                        ue->dlsch_SI[eNB_id],
                        NULL,
                        &ue->dlsch_SI_errors[eNB_id],
                        mode,
                        abstraction_flag);
    ue->dlsch_SI[eNB_id]->active = 0;
  }

  // do procedures for P-RNTI
  if ((ue->dlsch_p[eNB_id]) && (ue->dlsch_p[eNB_id]->active == 1)) {
    ue_dlsch_procedures(ue,
                        proc,
                        eNB_id,
                        P_PDSCH,
                        ue->dlsch_p[eNB_id],
                        NULL,
                        &ue->dlsch_p_errors[eNB_id],
                        mode,
                        abstraction_flag);
    ue->dlsch_p[eNB_id]->active = 0;
  }

  // do procedures for RA-RNTI
  if ((ue->dlsch_ra[eNB_id]) && (ue->dlsch_ra[eNB_id]->active == 1)) {
    ue_dlsch_procedures(ue,
                        proc,
                        eNB_id,
                        RA_PDSCH,
                        ue->dlsch_ra[eNB_id],
                        NULL,
                        &ue->dlsch_ra_errors[eNB_id],
                        mode,
                        abstraction_flag);
    ue->dlsch_ra[eNB_id]->active = 0;
  }

  if (LOG_DEBUGFLAG(UE_TIMING)) {
    stop_meas(&ue->dlsch_procedures_stat[ue->current_thread_id[subframe_rx]]);
    LOG_D(PHY, "[AbsSFN %d.%d] Channel Decoder: %5.2f \n",frame_rx,subframe_rx,ue->dlsch_procedures_stat[ue->current_thread_id[subframe_rx]].p_time/(cpuf*1000.0));
  }

  // duplicate harq structure
  uint8_t          current_harq_pid        = ue->dlsch[ue->current_thread_id[subframe_rx]][eNB_id][0]->current_harq_pid;
  LTE_DL_UE_HARQ_t *current_harq_processes = ue->dlsch[ue->current_thread_id[subframe_rx]][eNB_id][0]->harq_processes[current_harq_pid];
  harq_status_t    *current_harq_ack       = &ue->dlsch[ue->current_thread_id[subframe_rx]][eNB_id][0]->harq_ack[subframe_rx];

  // For Debug parallelisation
  //if (current_harq_ack->ack == 0) {
  //printf("[slot0 dl processing][End of Channel Decoding] AbsSubframe %d.%d Decode Fail for HarqId%d Round%d\n",frame_rx,subframe_rx,current_harq_pid,current_harq_processes->round);
  //}
  for(uint8_t rx_th_idx=1; rx_th_idx<RX_NB_TH; rx_th_idx++) {
    LTE_DL_UE_HARQ_t *harq_processes_dest  = ue->dlsch[ue->current_thread_id[(subframe_rx+rx_th_idx)%10]][eNB_id][0]->harq_processes[current_harq_pid];
    harq_status_t    *harq_ack_dest        = &ue->dlsch[ue->current_thread_id[(subframe_rx+rx_th_idx)%10]][eNB_id][0]->harq_ack[subframe_rx];
    copy_harq_proc_struct(harq_processes_dest, current_harq_processes);
    copy_ack_struct(harq_ack_dest, current_harq_ack);
  }

  /*
  LTE_DL_UE_HARQ_t *harq_processes_dest    = ue->dlsch[(subframe_rx+1)%RX_NB_TH][eNB_id][0]->harq_processes[current_harq_pid];
  LTE_DL_UE_HARQ_t *harq_processes_dest1    = ue->dlsch[(subframe_rx+2)%RX_NB_TH][eNB_id][0]->harq_processes[current_harq_pid];

  harq_status_t *current_harq_ack = &ue->dlsch[subframe_rx%RX_NB_TH][eNB_id][0]->harq_ack[subframe_rx];
  harq_status_t *harq_ack_dest    = &ue->dlsch[(subframe_rx+1)%RX_NB_TH][eNB_id][0]->harq_ack[subframe_rx];
  harq_status_t *harq_ack_dest1    = &ue->dlsch[(subframe_rx+2)%RX_NB_TH][eNB_id][0]->harq_ack[subframe_rx];

  copy_harq_proc_struct(harq_processes_dest, current_harq_processes);
  copy_ack_struct(harq_ack_dest, current_harq_ack);

  copy_harq_proc_struct(harq_processes_dest1, current_harq_processes);
  copy_ack_struct(harq_ack_dest1, current_harq_ack);
  */
  if (subframe_rx==9) {
    if (frame_rx % 10 == 0) {
      if ((ue->dlsch_received[eNB_id] - ue->dlsch_received_last[eNB_id]) != 0)
        ue->dlsch_fer[eNB_id] = (100*(ue->dlsch_errors[eNB_id] - ue->dlsch_errors_last[eNB_id]))/(ue->dlsch_received[eNB_id] - ue->dlsch_received_last[eNB_id]);

      ue->dlsch_errors_last[eNB_id] = ue->dlsch_errors[eNB_id];
      ue->dlsch_received_last[eNB_id] = ue->dlsch_received[eNB_id];
    }

    ue->bitrate[eNB_id] = (ue->total_TBS[eNB_id] - ue->total_TBS_last[eNB_id])*100;
    ue->total_TBS_last[eNB_id] = ue->total_TBS[eNB_id];
    LOG_D(PHY,"[UE %d] Calculating bitrate Frame %d: total_TBS = %d, total_TBS_last = %d, bitrate %f kbits\n",
          ue->Mod_id,frame_rx,ue->total_TBS[eNB_id],
          ue->total_TBS_last[eNB_id],(float) ue->bitrate[eNB_id]/1000.0);
#if UE_AUTOTEST_TRACE

    if ((frame_rx % 100 == 0)) {
      LOG_I(PHY,"[UE  %d] AUTOTEST Metric : UE_DLSCH_BITRATE = %5.2f kbps (frame = %d) \n", ue->Mod_id, (float) ue->bitrate[eNB_id]/1000.0, frame_rx);
    }

#endif
  }

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_UE_RX, VCD_FUNCTION_OUT);

  if (LOG_DEBUGFLAG(UE_TIMING)) {
    stop_meas(&ue->phy_proc_rx[ue->current_thread_id[subframe_rx]]);
    LOG_I(PHY, "------FULL RX PROC [AbsSFN %d.%d]: %5.2f ------\n",frame_rx,subframe_rx,ue->phy_proc_rx[ue->current_thread_id[subframe_rx]].p_time/(cpuf*1000.0));
  }

  LOG_D(PHY," ****** end RX-Chain  for AbsSubframe %d.%d ******  \n", frame_rx%1024, subframe_rx);
  return (0);
}
#endif /*UE_SLOT_PARALLELISATION */


void phy_procedures_UE_SL_RX(PHY_VARS_UE *ue,UE_rxtx_proc_t *proc) {
}


int phy_procedures_UE_RX(PHY_VARS_UE *ue,
                         UE_rxtx_proc_t *proc,
                         uint8_t eNB_id,
                         uint8_t abstraction_flag,
                         uint8_t do_pdcch_flag,
                         runmode_t mode) {
  int l,l2;
  int pilot1;
  int pmch_flag=0;
  int frame_rx = proc->frame_rx;
  int subframe_rx = proc->subframe_rx;
  uint8_t next1_thread_id = ue->current_thread_id[subframe_rx]== (RX_NB_TH-1) ? 0:(ue->current_thread_id[subframe_rx]+1);
  uint8_t next2_thread_id = next1_thread_id== (RX_NB_TH-1) ? 0:(next1_thread_id+1);
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_UE_RX, VCD_FUNCTION_IN);
  T(T_UE_PHY_DL_TICK, T_INT(ue->Mod_id), T_INT(frame_rx%1024), T_INT(subframe_rx));
  T(T_UE_PHY_INPUT_SIGNAL, T_INT(ue->Mod_id), T_INT(frame_rx%1024), T_INT(subframe_rx), T_INT(0),
    T_BUFFER(&ue->common_vars.rxdata[0][subframe_rx*ue->frame_parms.samples_per_tti],
             ue->frame_parms.samples_per_tti * 4));

  // start timers
  if ( LOG_DEBUGFLAG(DEBUG_UE_PHYPROC)) {
    LOG_I(PHY," ****** start RX-Chain for AbsSubframe->0 %d.%d ******  \n", frame_rx%1024, subframe_rx);
  }

  if(LOG_DEBUGFLAG(UE_TIMING)) {
    start_meas(&ue->phy_proc_rx[ue->current_thread_id[subframe_rx]]);
    start_meas(&ue->ue_front_end_stat[ue->current_thread_id[subframe_rx]]);
  }

  if(is_fembms_pmch_subframe(frame_rx,subframe_rx,&ue->frame_parms)) {
    ue_pmch_procedures(ue,proc,eNB_id,abstraction_flag,1);
    return 0;
  } else { // this gets closed at end
    if (is_fembms_cas_subframe(frame_rx,subframe_rx,&ue->frame_parms) || is_fembms_nonMBSFN_subframe(frame_rx,subframe_rx,&ue->frame_parms) ) {
      l=0;
      slot_fep(ue,
       l,
       subframe_rx<<1,
       0,
       0,
       0);
    }
    pmch_flag = is_pmch_subframe(frame_rx,subframe_rx,&ue->frame_parms) ? 1 : 0;

    if (do_pdcch_flag) {
      // deactivate reception until we scan pdcch
      if (ue->dlsch[ue->current_thread_id[subframe_rx]][eNB_id][0])
        ue->dlsch[ue->current_thread_id[subframe_rx]][eNB_id][0]->active = 0;

      if (ue->dlsch[ue->current_thread_id[subframe_rx]][eNB_id][1])
        ue->dlsch[ue->current_thread_id[subframe_rx]][eNB_id][1]->active = 0;

      if (ue->dlsch_SI[eNB_id])
        ue->dlsch_SI[eNB_id]->active = 0;

      if (ue->dlsch_p[eNB_id])
        ue->dlsch_p[eNB_id]->active = 0;

      if (ue->dlsch_ra[eNB_id])
        ue->dlsch_ra[eNB_id]->active = 0;
    }

    if (LOG_DEBUGFLAG(DEBUG_UE_PHYPROC)) {
      LOG_D(PHY,"[UE %d] Frame %d subframe %d: Doing phy_procedures_UE_RX\n",
            ue->Mod_id,frame_rx, subframe_rx);
    }

    if (ue->frame_parms.Ncp == 0) {  // normal prefix
      pilot1 = 4;
    } else { // extended prefix
      pilot1 = 3;
    }

    if (subframe_select(&ue->frame_parms,subframe_rx) == SF_S) { // S-subframe, do first 5 symbols only
      l2 = 4;
    } else if (pmch_flag == 1) { // do first 2 symbols only
      l2 = 1;
    } else { // normal subframe, last symbol to be processed is the first of the second slot
      l2 = (ue->frame_parms.symbols_per_tti/2)-1;
    }

    int prev_subframe_rx = (subframe_rx - 1)<0? 9: (subframe_rx - 1);

    if (subframe_select(&ue->frame_parms,prev_subframe_rx) != SF_DL) {
      //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
      // RX processing of symbols l=0...l2
      //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
      l=0;
    } else {
      //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
      // RX processing of symbols l=1...l2 (l=0 is done in last scheduling epoch)
      //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
      l=1;
    }

    LOG_D(PHY," ------ slot 0 Processing: AbsSubframe %d.%d ------  \n", frame_rx%1024, subframe_rx);
    LOG_D(PHY," ------  --> FFT/ChannelEst/PDCCH slot 0: AbsSubframe %d.%d ------  \n", frame_rx%1024, subframe_rx);

    for (; l<=l2; l++) {
      if (abstraction_flag == 0) {
        if (LOG_DEBUGFLAG(UE_TIMING)) {
          start_meas(&ue->ofdm_demod_stats);
        }

        VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_SLOT_FEP, VCD_FUNCTION_IN);
        slot_fep(ue,
                 l,
                 (subframe_rx<<1),
                 0,
                 0,
                 0);
        VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_SLOT_FEP, VCD_FUNCTION_OUT);

        if (LOG_DEBUGFLAG(UE_TIMING)) {
          stop_meas(&ue->ofdm_demod_stats);
        }
      }

      ue_measurement_procedures(l-1,ue,proc,eNB_id,(subframe_rx<<1),abstraction_flag,mode);

      if (do_pdcch_flag) {
        if ((l==pilot1) ||
            ((pmch_flag==1)&&(l==l2)))  {
          LOG_D(PHY,"[UE  %d] Frame %d: Calling pdcch procedures (eNB %d)\n",ue->Mod_id,frame_rx,eNB_id);

          //start_meas(&ue->rx_pdcch_stats[ue->current_thread_id[subframe_rx]]);
          if (ue_pdcch_procedures(eNB_id,ue,proc,abstraction_flag) == -1) {
            LOG_E(PHY,"[UE  %d] Frame %d, subframe %d: Error in pdcch procedures\n",ue->Mod_id,frame_rx,subframe_rx);
            return(-1);
          }

          //stop_meas(&ue->rx_pdcch_stats[ue->current_thread_id[subframe_rx]]);
          //printf("subframe %d n_pdcch_sym %d pdcch procedures  %5.3f \n",
          //        subframe_rx, ue->pdcch_vars[ue->current_thread_id[subframe_rx]][eNB_id]->num_pdcch_symbols,
          //     (ue->rx_pdcch_stats[ue->current_thread_id[subframe_rx]].p_time)/(cpuf*1000.0));
          LOG_D(PHY,"num_pdcch_symbols %d\n",ue->pdcch_vars[ue->current_thread_id[subframe_rx]][eNB_id]->num_pdcch_symbols);
        }
      }
    } // for l=1..l2

    ue_measurement_procedures(l-1,ue,proc,eNB_id,(subframe_rx<<1),abstraction_flag,mode);
    LOG_D(PHY," ------  end FFT/ChannelEst/PDCCH slot 0: AbsSubframe %d.%d ------  \n", frame_rx%1024, subframe_rx);

    // If this is PMCH, call procedures, do channel estimation for first symbol of next DL subframe and return
    if (pmch_flag == 1) {
      ue_pmch_procedures(ue,proc,eNB_id,abstraction_flag,
                         0 );
      int next_subframe_rx = (1+subframe_rx)%10;

      if (subframe_select(&ue->frame_parms,next_subframe_rx) != SF_UL) {
        slot_fep(ue,
                 0,
                 (next_subframe_rx<<1),
                 0,
                 0,
                 0);
      }

      return 0;
    }

    slot_fep(ue,
             0,
             1+(subframe_rx<<1),
             0,
             0,
             0);

    // first slot has been processed (FFTs + Channel Estimation, PCFICH/PHICH/PDCCH)
    if (LOG_DEBUGFLAG(UE_TIMING)) {
      stop_meas(&ue->ue_front_end_stat[ue->current_thread_id[subframe_rx]]);
      LOG_I(PHY, "[SFN %d] Slot0: FFT + Channel Estimate + PCFICH/PHICH/PDCCH %5.2f \n",subframe_rx,ue->ue_front_end_stat[ue->current_thread_id[subframe_rx]].p_time/(cpuf*1000.0));
    }

    LOG_D(PHY," ------ --> PDSCH ChannelComp/LLR slot 0: AbsSubframe %d.%d ------  \n", frame_rx%1024, subframe_rx);

    if (LOG_DEBUGFLAG(UE_TIMING)) {
      start_meas(&ue->generic_stat);
      start_meas(&ue->crnti_procedures_stats);
    }

    // do procedures for C-RNTI
    if (ue->dlsch[ue->current_thread_id[subframe_rx]][eNB_id][0]->active == 1) {
      VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PDSCH_PROC, VCD_FUNCTION_IN);
      ue_pdsch_procedures(ue,
                          proc,
                          eNB_id,
                          PDSCH,
                          ue->dlsch[ue->current_thread_id[subframe_rx]][eNB_id][0],
                          NULL,
                          ue->pdcch_vars[ue->current_thread_id[subframe_rx]][eNB_id]->num_pdcch_symbols,
                          ue->frame_parms.symbols_per_tti>>1,
                          abstraction_flag);
      VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PDSCH_PROC, VCD_FUNCTION_OUT);
    }

    if (LOG_DEBUGFLAG(UE_TIMING)) {
      stop_meas(&ue->crnti_procedures_stats);
    }

    LOG_D(PHY," ------ end PDSCH ChannelComp/LLR slot 0: AbsSubframe %d.%d ------  \n", frame_rx%1024, subframe_rx);

    // do procedures for SI-RNTI
    if ((ue->dlsch_SI[eNB_id]) && (ue->dlsch_SI[eNB_id]->active == 1)) {
      VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PDSCH_PROC_SI, VCD_FUNCTION_IN);
      ue_pdsch_procedures(ue,
                          proc,
                          eNB_id,
                          SI_PDSCH,
                          ue->dlsch_SI[eNB_id],
                          NULL,
                          ue->pdcch_vars[ue->current_thread_id[subframe_rx]][eNB_id]->num_pdcch_symbols,
                          ue->frame_parms.symbols_per_tti>>1,
                          abstraction_flag);
      VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PDSCH_PROC_SI, VCD_FUNCTION_OUT);
    }

    // do procedures for SI-RNTI
    if ((ue->dlsch_p[eNB_id]) && (ue->dlsch_p[eNB_id]->active == 1)) {
      VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PDSCH_PROC_P, VCD_FUNCTION_IN);
      ue_pdsch_procedures(ue,
                          proc,
                          eNB_id,
                          P_PDSCH,
                          ue->dlsch_p[eNB_id],
                          NULL,
                          ue->pdcch_vars[ue->current_thread_id[subframe_rx]][eNB_id]->num_pdcch_symbols,
                          ue->frame_parms.symbols_per_tti>>1,
                          abstraction_flag);
      VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PDSCH_PROC_P, VCD_FUNCTION_OUT);
    }

    // do procedures for RA-RNTI
    if ((ue->dlsch_ra[eNB_id]) && (ue->dlsch_ra[eNB_id]->active == 1)) {
      VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PDSCH_PROC_RA, VCD_FUNCTION_IN);
      ue_pdsch_procedures(ue,
                          proc,
                          eNB_id,
                          RA_PDSCH,
                          ue->dlsch_ra[eNB_id],
                          NULL,
                          ue->pdcch_vars[ue->current_thread_id[subframe_rx]][eNB_id]->num_pdcch_symbols,
                          ue->frame_parms.symbols_per_tti>>1,
                          abstraction_flag);
      VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PDSCH_PROC_RA, VCD_FUNCTION_OUT);
    }

    LOG_D(PHY," ------ slot 1 Processing: AbsSubframe %d.%d ------  \n", frame_rx%1024, subframe_rx);
    LOG_D(PHY," ------  --> FFT/ChannelEst/PDCCH slot 1: AbsSubframe %d.%d ------  \n", frame_rx%1024, subframe_rx);

    if (subframe_select(&ue->frame_parms,subframe_rx) != SF_S) {  // do front-end processing for second slot, and first symbol of next subframe
      for (l=1; l<ue->frame_parms.symbols_per_tti>>1; l++) {
        if (abstraction_flag == 0) {
          if (LOG_DEBUGFLAG(UE_TIMING)) {
            start_meas(&ue->ofdm_demod_stats);
          }

          VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_SLOT_FEP, VCD_FUNCTION_IN);
          slot_fep(ue,
                   l,
                   1+(subframe_rx<<1),
                   0,
                   0,
                   0);
          VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_SLOT_FEP, VCD_FUNCTION_OUT);

          if (LOG_DEBUGFLAG(UE_TIMING)) {
            stop_meas(&ue->ofdm_demod_stats);
          }
        }

        ue_measurement_procedures(l-1,ue,proc,eNB_id,1+(subframe_rx<<1),abstraction_flag,mode);
      } // for l=1..l2

      ue_measurement_procedures(l-1,ue,proc,eNB_id,1+(subframe_rx<<1),abstraction_flag,mode);
      // do first symbol of next downlink subframe for channel estimation
      int next_subframe_rx = (1+subframe_rx)%10;

      if (subframe_select(&ue->frame_parms,next_subframe_rx) != SF_UL) {
        slot_fep(ue,
                 0,
                 (next_subframe_rx<<1),
                 0,
                 0,
                 0);
      }
    } // not an S-subframe

    if(LOG_DEBUGFLAG(UE_TIMING)) {
      stop_meas(&ue->generic_stat);
      LOG_I(PHY, "[SFN %d] Slot1: FFT + Channel Estimate + Pdsch Proc Slot0 %5.2f \n",subframe_rx,ue->generic_stat.p_time/(cpuf*1000.0));
    }

    LOG_D(PHY," ------  end FFT/ChannelEst/PDCCH slot 1: AbsSubframe %d.%d ------  \n", frame_rx%1024, subframe_rx);

    if ( (subframe_rx == 0) && (ue->decode_MIB == 1)) {
      ue_pbch_procedures(eNB_id,ue,proc,abstraction_flag);
    }

    // do procedures for C-RNTI
    LOG_D(PHY," ------ --> PDSCH ChannelComp/LLR slot 0: AbsSubframe %d.%d ------  \n", frame_rx%1024, subframe_rx);

    if (ue->dlsch[ue->current_thread_id[subframe_rx]][eNB_id][0]->active == 1) {
      VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PDSCH_PROC, VCD_FUNCTION_IN);

      if (LOG_DEBUGFLAG(UE_TIMING)) {
        start_meas(&ue->pdsch_procedures_stat[ue->current_thread_id[subframe_rx]]);
      }

      ue_pdsch_procedures(ue,
                          proc,
                          eNB_id,
                          PDSCH,
                          ue->dlsch[ue->current_thread_id[subframe_rx]][eNB_id][0],
                          NULL,
                          1+(ue->frame_parms.symbols_per_tti>>1),
                          ue->frame_parms.symbols_per_tti-1,
                          abstraction_flag);
      LOG_D(PHY," ------ end PDSCH ChannelComp/LLR slot 0: AbsSubframe %d.%d ------  \n", frame_rx%1024, subframe_rx);
      LOG_D(PHY," ------ --> PDSCH Turbo Decoder slot 0/1: AbsSubframe %d.%d ------  \n", frame_rx%1024, subframe_rx);

      if (LOG_DEBUGFLAG(UE_TIMING)) {
        stop_meas(&ue->pdsch_procedures_stat[ue->current_thread_id[subframe_rx]]);
        start_meas(&ue->dlsch_procedures_stat[ue->current_thread_id[subframe_rx]]);
      }

      ue_dlsch_procedures(ue,
                          proc,
                          eNB_id,
                          PDSCH,
                          ue->dlsch[ue->current_thread_id[subframe_rx]][eNB_id][0],
                          ue->dlsch[ue->current_thread_id[subframe_rx]][eNB_id][1],
                          &ue->dlsch_errors[eNB_id],
                          mode,
                          abstraction_flag);

      if (LOG_DEBUGFLAG(UE_TIMING)) {
        stop_meas(&ue->dlsch_procedures_stat[ue->current_thread_id[subframe_rx]]);
        LOG_I(PHY, "[SFN %d] Slot1:       Pdsch Proc %5.2f\n",subframe_rx,ue->pdsch_procedures_stat[ue->current_thread_id[subframe_rx]].p_time/(cpuf*1000.0));
        LOG_I(PHY, "[SFN %d] Slot0 Slot1: Dlsch Proc %5.2f\n",subframe_rx,ue->dlsch_procedures_stat[ue->current_thread_id[subframe_rx]].p_time/(cpuf*1000.0));
      }

      VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PDSCH_PROC, VCD_FUNCTION_OUT);
    }

    if (LOG_DEBUGFLAG(UE_TIMING)) {
      start_meas(&ue->generic_stat);
    }

    if (LOG_DUMPFLAG(DEBUG_UE_PHYPROC)) {
      if(subframe_rx==5 &&  ue->dlsch[ue->current_thread_id[subframe_rx]][eNB_id][0]->harq_processes[ue->dlsch[ue->current_thread_id[subframe_rx]][eNB_id][0]->current_harq_pid]->nb_rb > 20) {
        //LOG_M("decoder_llr.m","decllr",dlsch_llr,G,1,0);
        //LOG_M("llr.m","llr",  &ue->pdsch_vars[eNB_id]->llr[0][0],(14*nb_rb*12*dlsch1_harq->Qm) - 4*(nb_rb*4*dlsch1_harq->Qm),1,0);
        LOG_M("rxdataF0_current.m", "rxdataF0", &ue->common_vars.common_vars_rx_data_per_thread[ue->current_thread_id[subframe_rx]].rxdataF[0][0],14*ue->frame_parms.ofdm_symbol_size,1,1);
        //LOG_M("rxdataF0_previous.m"    , "rxdataF0_prev_sss", &ue->common_vars.common_vars_rx_data_per_thread[next_thread_id].rxdataF[0][0],14*ue->frame_parms.ofdm_symbol_size,1,1);
        //LOG_M("rxdataF0_previous.m"    , "rxdataF0_prev", &ue->common_vars.common_vars_rx_data_per_thread[next_thread_id].rxdataF[0][0],14*ue->frame_parms.ofdm_symbol_size,1,1);
        LOG_M("dl_ch_estimates.m", "dl_ch_estimates_sfn5", &ue->common_vars.common_vars_rx_data_per_thread[ue->current_thread_id[subframe_rx]].dl_ch_estimates[0][0][0],14*ue->frame_parms.ofdm_symbol_size,1,
              1);
        LOG_M("dl_ch_estimates_ext.m", "dl_ch_estimatesExt_sfn5", &ue->pdsch_vars[ue->current_thread_id[subframe_rx]][0]->dl_ch_estimates_ext[0][0],14*ue->frame_parms.N_RB_DL*12,1,1);
        LOG_M("rxdataF_comp00.m","rxdataF_comp00",     &ue->pdsch_vars[ue->current_thread_id[subframe_rx]][0]->rxdataF_comp0[0][0],14*ue->frame_parms.N_RB_DL*12,1,1);
        //LOG_M("magDLFirst.m", "magDLFirst", &phy_vars_ue->pdsch_vars[ue->current_thread_id[subframe_rx]][0]->dl_ch_mag0[0][0],14*frame_parms->N_RB_DL*12,1,1);
        //LOG_M("magDLSecond.m", "magDLSecond", &phy_vars_ue->pdsch_vars[ue->current_thread_id[subframe_rx]][0]->dl_ch_magb0[0][0],14*frame_parms->N_RB_DL*12,1,1);
        AssertFatal (0,"");
      }
    }

    // do procedures for SI-RNTI
    if ((ue->dlsch_SI[eNB_id]) && (ue->dlsch_SI[eNB_id]->active == 1)) {
      ue_pdsch_procedures(ue,
                          proc,
                          eNB_id,
                          SI_PDSCH,
                          ue->dlsch_SI[eNB_id],
                          NULL,
                          1+(ue->frame_parms.symbols_per_tti>>1),
                          ue->frame_parms.symbols_per_tti-1,
                          abstraction_flag);
      ue_dlsch_procedures(ue,
                          proc,
                          eNB_id,
                          SI_PDSCH,
                          ue->dlsch_SI[eNB_id],
                          NULL,
                          &ue->dlsch_SI_errors[eNB_id],
                          mode,
                          abstraction_flag);
      ue->dlsch_SI[eNB_id]->active = 0;
    }

    // do procedures for P-RNTI
    if ((ue->dlsch_p[eNB_id]) && (ue->dlsch_p[eNB_id]->active == 1)) {
      ue_pdsch_procedures(ue,
                          proc,
                          eNB_id,
                          P_PDSCH,
                          ue->dlsch_p[eNB_id],
                          NULL,
                          1+(ue->frame_parms.symbols_per_tti>>1),
                          ue->frame_parms.symbols_per_tti-1,
                          abstraction_flag);
      ue_dlsch_procedures(ue,
                          proc,
                          eNB_id,
                          P_PDSCH,
                          ue->dlsch_p[eNB_id],
                          NULL,
                          &ue->dlsch_p_errors[eNB_id],
                          mode,
                          abstraction_flag);
      ue->dlsch_p[eNB_id]->active = 0;
    }

    // do procedures for RA-RNTI
    if ((ue->dlsch_ra[eNB_id]) && (ue->dlsch_ra[eNB_id]->active == 1)) {
      ue_pdsch_procedures(ue,
                          proc,
                          eNB_id,
                          RA_PDSCH,
                          ue->dlsch_ra[eNB_id],
                          NULL,
                          1+(ue->frame_parms.symbols_per_tti>>1),
                          ue->frame_parms.symbols_per_tti-1,
                          abstraction_flag);
      ue_dlsch_procedures(ue,
                          proc,
                          eNB_id,
                          RA_PDSCH,
                          ue->dlsch_ra[eNB_id],
                          NULL,
                          &ue->dlsch_ra_errors[eNB_id],
                          mode,
                          abstraction_flag);
      ue->dlsch_ra[eNB_id]->active = 0;
    }
  } // This commes from feMBMS subframe filtering !

  // duplicate harq structure
  uint8_t          current_harq_pid        = ue->dlsch[ue->current_thread_id[subframe_rx]][eNB_id][0]->current_harq_pid;
  LTE_DL_UE_HARQ_t *current_harq_processes = ue->dlsch[ue->current_thread_id[subframe_rx]][eNB_id][0]->harq_processes[current_harq_pid];
  LTE_DL_UE_HARQ_t *harq_processes_dest    = ue->dlsch[next1_thread_id][eNB_id][0]->harq_processes[current_harq_pid];
  LTE_DL_UE_HARQ_t *harq_processes_dest1    = ue->dlsch[next2_thread_id][eNB_id][0]->harq_processes[current_harq_pid];
  harq_status_t *current_harq_ack = &ue->dlsch[ue->current_thread_id[subframe_rx]][eNB_id][0]->harq_ack[subframe_rx];
  harq_status_t *harq_ack_dest    = &ue->dlsch[next1_thread_id][eNB_id][0]->harq_ack[subframe_rx];
  harq_status_t *harq_ack_dest1    = &ue->dlsch[next2_thread_id][eNB_id][0]->harq_ack[subframe_rx];
  copy_harq_proc_struct(harq_processes_dest, current_harq_processes);
  copy_ack_struct(harq_ack_dest, current_harq_ack);
  copy_harq_proc_struct(harq_processes_dest1, current_harq_processes);
  copy_ack_struct(harq_ack_dest1, current_harq_ack);

  if (subframe_rx==9) {
    if (frame_rx % 10 == 0) {
      if ((ue->dlsch_received[eNB_id] - ue->dlsch_received_last[eNB_id]) != 0)
        ue->dlsch_fer[eNB_id] = (100*(ue->dlsch_errors[eNB_id] - ue->dlsch_errors_last[eNB_id]))/(ue->dlsch_received[eNB_id] - ue->dlsch_received_last[eNB_id]);

      ue->dlsch_errors_last[eNB_id] = ue->dlsch_errors[eNB_id];
      ue->dlsch_received_last[eNB_id] = ue->dlsch_received[eNB_id];
    }

    ue->bitrate[eNB_id] = (ue->total_TBS[eNB_id] - ue->total_TBS_last[eNB_id])*100;
    ue->total_TBS_last[eNB_id] = ue->total_TBS[eNB_id];
    LOG_D(PHY,"[UE %d] Calculating bitrate Frame %d: total_TBS = %d, total_TBS_last = %d, bitrate %f kbits\n",
          ue->Mod_id,frame_rx,ue->total_TBS[eNB_id],
          ue->total_TBS_last[eNB_id],(float) ue->bitrate[eNB_id]/1000.0);

    if ( LOG_DEBUGFLAG(DEBUG_UE_PHYPROC)) {
      if ((frame_rx % 100 == 0)) {
        LOG_UI(PHY,"[UE  %d] AUTOTEST Metric : UE_DLSCH_BITRATE = %5.2f kbps (frame = %d) \n", ue->Mod_id, (float) ue->bitrate[eNB_id]/1000.0, frame_rx);
      }
    }
  }

  if ( LOG_DEBUGFLAG(UE_TIMING)) {
    stop_meas(&ue->generic_stat);
    LOG_I(PHY,"after tubo until end of Rx %5.2f \n",ue->generic_stat.p_time/(cpuf*1000.0));
  }

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_UE_RX, VCD_FUNCTION_OUT);

  if ( LOG_DEBUGFLAG(UE_TIMING) ) {
    stop_meas(&ue->phy_proc_rx[ue->current_thread_id[subframe_rx]]);
    LOG_I(PHY, "------FULL RX PROC [SFN %d]: %5.2f ------\n",subframe_rx,ue->phy_proc_rx[ue->current_thread_id[subframe_rx]].p_time/(cpuf*1000.0));
  }

  LOG_D(PHY," ****** end RX-Chain  for AbsSubframe %d.%d ******  \n", frame_rx%1024, subframe_rx);
  return (0);
}


void phy_procedures_UE_lte(PHY_VARS_UE *ue,
                           UE_rxtx_proc_t *proc,
                           uint8_t eNB_id,
                           uint8_t abstraction_flag,
                           uint8_t do_pdcch_flag,
                           runmode_t mode) {
  MessageDef   *msg_p;
  int           result;
  int           frame_rx = proc->frame_rx;
  int           frame_tx = proc->frame_tx;
  int           subframe_rx = proc->subframe_rx;
  int           subframe_tx = proc->subframe_tx;
  UE_L2_STATE_t ret;
  int slot;

  if (ue->mac_enabled == 0) {
    ue->UE_mode[eNB_id]=PUSCH;
  }

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_UE_LTE,1);

  if ( LOG_DEBUGFLAG(UE_TIMING)) {
    start_meas(&ue->phy_proc[ue->current_thread_id[subframe_rx]]);
  }
  do {
    // Checks if a message has been sent to PHY sub-task
    itti_poll_msg (TASK_PHY_UE, &msg_p);

    if (msg_p != NULL) {
      switch (ITTI_MSG_ID(msg_p)) {
        case PHY_FIND_CELL_REQ:
          LOG_I(PHY, "[UE ] Received %s\n", ITTI_MSG_NAME (msg_p));
          /* TODO process the message */
          break;

        default:
          LOG_E(PHY, "[UE %ld] Received unexpected message %s\n", ITTI_MSG_DESTINATION_INSTANCE (msg_p), ITTI_MSG_NAME (msg_p));
          break;
      }

      result = itti_free (ITTI_MSG_ORIGIN_ID(msg_p), msg_p);
      AssertFatal (result == EXIT_SUCCESS, "Failed to free memory (%d)!\n", result);
    }
  } while(msg_p != NULL);

  for (slot=0; slot<2; slot++) {
    if ((subframe_select(&ue->frame_parms,subframe_tx)==SF_UL)||
        (ue->frame_parms.frame_type == FDD)) {
      phy_procedures_UE_TX(ue,proc,eNB_id,abstraction_flag,mode);
    }

    if ((subframe_select(&ue->frame_parms,subframe_rx)==SF_DL) ||
        (ue->frame_parms.frame_type == FDD)) {
      phy_procedures_UE_RX(ue,proc,eNB_id,abstraction_flag,do_pdcch_flag,mode);
    }

    if ((subframe_select(&ue->frame_parms,subframe_tx)==SF_S) &&
        (slot==1)) {
      phy_procedures_UE_S_TX(ue,eNB_id,abstraction_flag);
    }

    if ((subframe_select(&ue->frame_parms,subframe_rx)==SF_S) &&
        (slot==0)) {
      phy_procedures_UE_RX(ue,proc,eNB_id,abstraction_flag,do_pdcch_flag,mode);
    }

    if (ue->mac_enabled==1) {
      if (slot==0) {
        //LOG_I(PHY,"[UE %d] Frame %d, subframe %d, star ue_scheduler\n", ue->Mod_id,frame_rx,subframe_tx);
        ret = ue_scheduler(ue->Mod_id,
                           frame_rx,
                           subframe_rx,
                           frame_tx,
                           subframe_tx,
                           subframe_select(&ue->frame_parms,subframe_tx),
                           eNB_id,
                           0/*FIXME CC_id*/);

        if (ret == CONNECTION_LOST) {
          LOG_E(PHY,"[UE %d] Frame %d, subframe %d RRC Connection lost, returning to PRACH\n",ue->Mod_id,
                frame_rx,subframe_tx);
          ue->UE_mode[eNB_id] = PRACH;
          //      mac_xface->macphy_exit("Connection lost");
        } else if (ret == PHY_RESYNCH) {
          LOG_E(PHY,"[UE %d] Frame %d, subframe %d RRC Connection lost, trying to resynch\n",
                ue->Mod_id,
                frame_rx,subframe_tx);
          ue->UE_mode[eNB_id] = RESYNCH;
          //     mac_xface->macphy_exit("Connection lost");
        } else if (ret == PHY_HO_PRACH) {
          LOG_I(PHY,"[UE %d] Frame %d, subframe %d, return to PRACH and perform a contention-free access\n",
                ue->Mod_id,frame_rx,subframe_tx);
          ue->UE_mode[eNB_id] = PRACH;
        }
      }
    }

    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_UE_LTE,0);

    if (LOG_DEBUGFLAG(UE_TIMING)) {
      stop_meas(&ue->phy_proc[ue->current_thread_id[subframe_rx]]);
    }
  } // slot
}
