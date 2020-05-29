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

/*! \file phy_procedures_nr_ue.c
 * \brief Implementation of UE procedures from 36.213 LTE specifications
 * \author R. Knopp, F. Kaltenberger, N. Nikaein, A. Mico Pereperez, G. Casati
 * \date 2018
 * \version 0.1
 * \company Eurecom
 * \email: knopp@eurecom.fr,florian.kaltenberger@eurecom.fr, navid.nikaein@eurecom.fr, guido.casati@iis.fraunhofer.de
 * \note
 * \warning
 */

#define _GNU_SOURCE

#include "assertions.h"
#include "defs.h"
//#include "PHY/defs.h"
#include "PHY/defs_nr_UE.h"
//#include "PHY/phy_vars_nr_ue.h"
#include "PHY/phy_extern_nr_ue.h"
#include "PHY/MODULATION/modulation_UE.h"
#include "PHY/NR_REFSIG/refsig_defs_ue.h"
#include "PHY/NR_UE_TRANSPORT/nr_transport_ue.h"
#include "PHY/NR_UE_TRANSPORT/nr_transport_proto_ue.h"
//#include "PHY/extern.h"
#include "SCHED_NR_UE/defs.h"
#include "SCHED_NR/extern.h"
#include "SCHED_NR_UE/phy_sch_processing_time.h"
//#include <sched.h>
//#include "targets/RT/USER/nr-softmodem.h"
#include "PHY/NR_UE_ESTIMATION/nr_estimation.h"
#include "PHY/NR_TRANSPORT/nr_dci.h"
#ifdef EMOS
#include "SCHED/phy_procedures_emos.h"
#endif

//#define DEBUG_PHY_PROC

#define NR_PDCCH_SCHED
//#define NR_PDCCH_SCHED_DEBUG
//#define NR_PUCCH_SCHED
//#define NR_PUCCH_SCHED_DEBUG

#ifndef PUCCH
#define PUCCH
#endif

#include "LAYER2/NR_MAC_UE/mac_defs.h"
#include "common/utils/LOG/log.h"

#ifdef EMOS
fifo_dump_emos_UE emos_dump_UE;
#endif

#include "common/utils/LOG/vcd_signal_dumper.h"
#include "UTIL/OPT/opt.h"
#include "intertask_interface.h"
#include "T.h"

#define DLSCH_RB_ALLOC 0x1fbf  // skip DC RB (total 23/25 RBs)
#define DLSCH_RB_ALLOC_12 0x0aaa  // skip DC RB (total 23/25 RBs)

#define NS_PER_SLOT 500000

char nr_mode_string[4][20] = {"NOT SYNCHED","PRACH","RAR","PUSCH"};

extern double cpuf;

/*
int nr_generate_ue_ul_dlsch_params_from_dci(PHY_VARS_NR_UE *ue,
					    uint8_t eNB_id,
					    int frame,
					    uint8_t nr_tti_rx,
					    uint64_t dci_pdu[2],
					    uint16_t rnti,
					    uint8_t dci_length,
					    NR_DCI_format_t dci_format,
					    NR_UE_PDCCH *pdcch_vars,
					    NR_UE_PDSCH *pdsch_vars,
					    NR_UE_DLSCH_t **dlsch,
					    NR_UE_ULSCH_t *ulsch,
					    NR_DL_FRAME_PARMS *frame_parms,
					    PDSCH_CONFIG_DEDICATED *pdsch_config_dedicated,
					    uint8_t beamforming_mode,
					    uint8_t dci_fields_sizes[NBR_NR_DCI_FIELDS][NBR_NR_FORMATS],
					    uint16_t n_RB_ULBWP,
					    uint16_t n_RB_DLBWP,
					    uint16_t crc_scrambled_values[TOTAL_NBR_SCRAMBLED_VALUES],
					    NR_DCI_INFO_EXTRACTED_t *nr_dci_info_extracted);
*/

#if defined(EXMIMO) || defined(OAI_USRP) || defined(OAI_BLADERF) || defined(OAI_LMSSDR) || defined(OAI_ADRV9371_ZC706)
extern uint64_t downlink_frequency[MAX_NUM_CCs][4];
#endif


#if 0
void nr_dump_dlsch(PHY_VARS_NR_UE *ue,UE_nr_rxtx_proc_t *proc,uint8_t eNB_id,uint8_t nr_tti_rx,uint8_t harq_pid)
{
  unsigned int coded_bits_per_codeword;
  uint8_t nsymb = (ue->frame_parms.Ncp == 0) ? 14 : 12;

  coded_bits_per_codeword = get_G(&ue->frame_parms,
                                  ue->dlsch[ue->current_thread_id[nr_tti_rx]][eNB_id][0]->harq_processes[harq_pid]->nb_rb,
                                  ue->dlsch[ue->current_thread_id[nr_tti_rx]][eNB_id][0]->harq_processes[harq_pid]->rb_alloc_even,
                                  ue->dlsch[ue->current_thread_id[nr_tti_rx]][eNB_id][0]->harq_processes[harq_pid]->Qm,
                                  ue->dlsch[ue->current_thread_id[nr_tti_rx]][eNB_id][0]->harq_processes[harq_pid]->Nl,
                                  ue->pdcch_vars[0%RX_NB_TH][eNB_id]->num_pdcch_symbols,
                                  proc->frame_rx,
				  nr_tti_rx,
				  ue->transmission_mode[eNB_id]<7?0:ue->transmission_mode[eNB_id]);

  write_output("rxsigF0.m","rxsF0", ue->common_vars.common_vars_rx_data_per_thread[ue->current_thread_id[nr_tti_rx]].rxdataF[0],2*nsymb*ue->frame_parms.ofdm_symbol_size,2,1);
  write_output("rxsigF0_ext.m","rxsF0_ext", ue->pdsch_vars[ue->current_thread_id[nr_tti_rx]][0]->rxdataF_ext[0],2*nsymb*ue->frame_parms.ofdm_symbol_size,1,1);
  write_output("dlsch00_ch0_ext.m","dl00_ch0_ext", ue->pdsch_vars[ue->current_thread_id[nr_tti_rx]][0]->dl_ch_estimates_ext[0],300*nsymb,1,1);
  /*
    write_output("dlsch01_ch0_ext.m","dl01_ch0_ext",pdsch_vars[0]->dl_ch_estimates_ext[1],300*12,1,1);
    write_output("dlsch10_ch0_ext.m","dl10_ch0_ext",pdsch_vars[0]->dl_ch_estimates_ext[2],300*12,1,1);
    write_output("dlsch11_ch0_ext.m","dl11_ch0_ext",pdsch_vars[0]->dl_ch_estimates_ext[3],300*12,1,1);
    write_output("dlsch_rho.m","dl_rho",pdsch_vars[0]->rho[0],300*12,1,1);
  */
  write_output("dlsch_rxF_comp0.m","dlsch0_rxF_comp0", ue->pdsch_vars[ue->current_thread_id[nr_tti_rx]][0]->rxdataF_comp0[0],300*12,1,1);
  write_output("dlsch_rxF_llr.m","dlsch_llr", ue->pdsch_vars[ue->current_thread_id[nr_tti_rx]][0]->llr[0],coded_bits_per_codeword,1,0);

  write_output("dlsch_mag1.m","dlschmag1",ue->pdsch_vars[ue->current_thread_id[nr_tti_rx]][0]->dl_ch_mag0,300*12,1,1);
  write_output("dlsch_mag2.m","dlschmag2",ue->pdsch_vars[ue->current_thread_id[nr_tti_rx]][0]->dl_ch_magb0,300*12,1,1);
}

void nr_dump_dlsch_SI(PHY_VARS_NR_UE *ue,UE_nr_rxtx_proc_t *proc,uint8_t eNB_id,uint8_t nr_tti_rx)
{
  unsigned int coded_bits_per_codeword;
  uint8_t nsymb = ((ue->frame_parms.Ncp == 0) ? 14 : 12);

  coded_bits_per_codeword = get_G(&ue->frame_parms,
                                  ue->dlsch_SI[eNB_id]->harq_processes[0]->nb_rb,
                                  ue->dlsch_SI[eNB_id]->harq_processes[0]->rb_alloc_even,
                                  2,
                                  1,
                                  ue->pdcch_vars[0%RX_NB_TH][eNB_id]->num_pdcch_symbols,
                                  proc->frame_rx,
				  nr_tti_rx,
				  0);
  LOG_D(PHY,"[UE %d] Dumping dlsch_SI : ofdm_symbol_size %d, nsymb %d, nb_rb %d, mcs %d, nb_rb %d, num_pdcch_symbols %d,G %d\n",
        ue->Mod_id,
	ue->frame_parms.ofdm_symbol_size,
	nsymb,
        ue->dlsch_SI[eNB_id]->harq_processes[0]->nb_rb,
        ue->dlsch_SI[eNB_id]->harq_processes[0]->mcs,
        ue->dlsch_SI[eNB_id]->harq_processes[0]->nb_rb,
        ue->pdcch_vars[0%RX_NB_TH][eNB_id]->num_pdcch_symbols,
        coded_bits_per_codeword);

  write_output("rxsig0.m","rxs0", &ue->common_vars.rxdata[0][nr_tti_rx*ue->frame_parms.samples_per_tti],ue->frame_parms.samples_per_tti,1,1);

  write_output("rxsigF0.m","rxsF0", ue->common_vars.common_vars_rx_data_per_thread[ue->current_thread_id[nr_tti_rx]].rxdataF[0],nsymb*ue->frame_parms.ofdm_symbol_size,1,1);
  write_output("rxsigF0_ext.m","rxsF0_ext", ue->pdsch_vars_SI[0]->rxdataF_ext[0],2*nsymb*ue->frame_parms.ofdm_symbol_size,1,1);
  write_output("dlsch00_ch0_ext.m","dl00_ch0_ext", ue->pdsch_vars_SI[0]->dl_ch_estimates_ext[0],ue->frame_parms.N_RB_DL*12*nsymb,1,1);
  /*
    write_output("dlsch01_ch0_ext.m","dl01_ch0_ext",pdsch_vars[0]->dl_ch_estimates_ext[1],300*12,1,1);
    write_output("dlsch10_ch0_ext.m","dl10_ch0_ext",pdsch_vars[0]->dl_ch_estimates_ext[2],300*12,1,1);
    write_output("dlsch11_ch0_ext.m","dl11_ch0_ext",pdsch_vars[0]->dl_ch_estimates_ext[3],300*12,1,1);
    write_output("dlsch_rho.m","dl_rho",pdsch_vars[0]->rho[0],300*12,1,1);
  */
  write_output("dlsch_rxF_comp0.m","dlsch0_rxF_comp0", ue->pdsch_vars_SI[0]->rxdataF_comp0[0],ue->frame_parms.N_RB_DL*12*nsymb,1,1);
  write_output("dlsch_rxF_llr.m","dlsch_llr", ue->pdsch_vars_SI[0]->llr[0],coded_bits_per_codeword,1,0);

  write_output("dlsch_mag1.m","dlschmag1",ue->pdsch_vars_SI[0]->dl_ch_mag0,300*nsymb,1,1);
  write_output("dlsch_mag2.m","dlschmag2",ue->pdsch_vars_SI[0]->dl_ch_magb0,300*nsymb,1,1);
  sleep(1);
  exit(-1);
}

#if defined(EXMIMO) || defined(OAI_USRP) || defined(OAI_BLADERF) || defined(OAI_LMSSDR) || defined(OAI_ADRV9371_ZC706)
//unsigned int gain_table[31] = {100,112,126,141,158,178,200,224,251,282,316,359,398,447,501,562,631,708,794,891,1000,1122,1258,1412,1585,1778,1995,2239,2512,2818,3162};
/*
  unsigned int get_tx_amp_prach(int power_dBm, int power_max_dBm, int N_RB_UL)
  {

  int gain_dB = power_dBm - power_max_dBm;
  int amp_x_100;

  switch (N_RB_UL) {
  case 6:
  amp_x_100 = AMP;      // PRACH is 6 PRBS so no scale
  break;
  case 15:
  amp_x_100 = 158*AMP;  // 158 = 100*sqrt(15/6)
  break;
  case 25:
  amp_x_100 = 204*AMP;  // 204 = 100*sqrt(25/6)
  break;
  case 50:
  amp_x_100 = 286*AMP;  // 286 = 100*sqrt(50/6)
  break;
  case 75:
  amp_x_100 = 354*AMP;  // 354 = 100*sqrt(75/6)
  break;
  case 100:
  amp_x_100 = 408*AMP;  // 408 = 100*sqrt(100/6)
  break;
  default:
  LOG_E(PHY,"Unknown PRB size %d\n",N_RB_UL);
  //mac_xface->macphy_exit("");
  break;
  }
  if (gain_dB < -30) {
  return(amp_x_100/3162);
  } else if (gain_dB>0)
  return(amp_x_100);
  else
  return(amp_x_100/gain_table[-gain_dB]);  // 245 corresponds to the factor sqrt(25/6)
  }
*/

unsigned int get_tx_amp(int power_dBm, int power_max_dBm, int N_RB_UL, int nb_rb)
{

  int gain_dB = power_dBm - power_max_dBm;
  double gain_lin;

  gain_lin = pow(10,.1*gain_dB);
  if ((nb_rb >0) && (nb_rb <= N_RB_UL)) {
    return((int)(AMP*sqrt(gain_lin*N_RB_UL/(double)nb_rb)));
  }
  else {
    LOG_E(PHY,"Illegal nb_rb/N_RB_UL combination (%d/%d)\n",nb_rb,N_RB_UL);
    //mac_xface->macphy_exit("");
  }
  return(0);
}

#endif

void nr_dump_dlsch_ra(PHY_VARS_NR_UE *ue,UE_nr_rxtx_proc_t *proc,uint8_t eNB_id,uint8_t nr_tti_rx)
{
  unsigned int coded_bits_per_codeword;
  uint8_t nsymb = ((ue->frame_parms.Ncp == 0) ? 14 : 12);

  coded_bits_per_codeword = get_G(&ue->frame_parms,
                                  ue->dlsch_ra[eNB_id]->harq_processes[0]->nb_rb,
                                  ue->dlsch_ra[eNB_id]->harq_processes[0]->rb_alloc_even,
                                  2,
                                  1,
                                  ue->pdcch_vars[0%RX_NB_TH][eNB_id]->num_pdcch_symbols,
                                  proc->frame_rx,
				  nr_tti_rx,
				  0);
  LOG_D(PHY,"[UE %d] Dumping dlsch_ra : nb_rb %d, mcs %d, nb_rb %d, num_pdcch_symbols %d,G %d\n",
        ue->Mod_id,
        ue->dlsch_ra[eNB_id]->harq_processes[0]->nb_rb,
        ue->dlsch_ra[eNB_id]->harq_processes[0]->mcs,
        ue->dlsch_ra[eNB_id]->harq_processes[0]->nb_rb,
        ue->pdcch_vars[0%RX_NB_TH][eNB_id]->num_pdcch_symbols,
        coded_bits_per_codeword);

  write_output("rxsigF0.m","rxsF0", ue->common_vars.common_vars_rx_data_per_thread[ue->current_thread_id[nr_tti_rx]].rxdataF[0],2*12*ue->frame_parms.ofdm_symbol_size,2,1);
  write_output("rxsigF0_ext.m","rxsF0_ext", ue->pdsch_vars_ra[0]->rxdataF_ext[0],2*12*ue->frame_parms.ofdm_symbol_size,1,1);
  write_output("dlsch00_ch0_ext.m","dl00_ch0_ext", ue->pdsch_vars_ra[0]->dl_ch_estimates_ext[0],300*nsymb,1,1);
  /*
    write_output("dlsch01_ch0_ext.m","dl01_ch0_ext",pdsch_vars[0]->dl_ch_estimates_ext[1],300*12,1,1);
    write_output("dlsch10_ch0_ext.m","dl10_ch0_ext",pdsch_vars[0]->dl_ch_estimates_ext[2],300*12,1,1);
    write_output("dlsch11_ch0_ext.m","dl11_ch0_ext",pdsch_vars[0]->dl_ch_estimates_ext[3],300*12,1,1);
    write_output("dlsch_rho.m","dl_rho",pdsch_vars[0]->rho[0],300*12,1,1);
  */
  write_output("dlsch_rxF_comp0.m","dlsch0_rxF_comp0", ue->pdsch_vars_ra[0]->rxdataF_comp0[0],300*nsymb,1,1);
  write_output("dlsch_rxF_llr.m","dlsch_llr", ue->pdsch_vars_ra[0]->llr[0],coded_bits_per_codeword,1,0);

  write_output("dlsch_mag1.m","dlschmag1",ue->pdsch_vars_ra[0]->dl_ch_mag0,300*nsymb,1,1);
  write_output("dlsch_mag2.m","dlschmag2",ue->pdsch_vars_ra[0]->dl_ch_magb0,300*nsymb,1,1);
}

void phy_reset_ue(uint8_t Mod_id,uint8_t CC_id,uint8_t eNB_index)
{

  // This flushes ALL DLSCH and ULSCH harq buffers of ALL connected eNBs...add the eNB_index later
  // for more flexibility

  uint8_t i,j,k,s;
  PHY_VARS_NR_UE *ue = PHY_vars_UE_g[Mod_id][CC_id];

  //[NUMBER_OF_RX_THREAD=2][NUMBER_OF_CONNECTED_eNB_MAX][2];
  for(int l=0; l<RX_NB_TH; l++) {
    for(i=0; i<NUMBER_OF_CONNECTED_eNB_MAX; i++) {
      for(j=0; j<2; j++) {
	//DL HARQ
	if(ue->dlsch[l][i][j]) {
	  for(k=0; k<NR_MAX_DLSCH_HARQ_PROCESSES && ue->dlsch[l][i][j]->harq_processes[k]; k++) {
	    ue->dlsch[l][i][j]->harq_processes[k]->status = SCH_IDLE;
	    for (s=0; s<10; s++) {
	      // reset ACK/NACK bit to DTX for all nr_tti_rxs s = 0..9
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
	for(k=0; k<NR_MAX_ULSCH_HARQ_PROCESSES && ue->ulsch[i]->harq_processes[k]; k++) {
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

void ra_failed(uint8_t Mod_id,uint8_t CC_id,uint8_t eNB_index)
{

  // if contention resolution fails, go back to PRACH
  PHY_vars_UE_g[Mod_id][CC_id]->UE_mode[eNB_index] = PRACH;
  PHY_vars_UE_g[Mod_id][CC_id]->pdcch_vars[0][eNB_index]->crnti_is_temporary = 0;
  PHY_vars_UE_g[Mod_id][CC_id]->pdcch_vars[0][eNB_index]->crnti = 0;
  PHY_vars_UE_g[Mod_id][CC_id]->pdcch_vars[1][eNB_index]->crnti_is_temporary = 0;
  PHY_vars_UE_g[Mod_id][CC_id]->pdcch_vars[1][eNB_index]->crnti = 0;
  LOG_E(PHY,"[UE %d] Random-access procedure fails, going back to PRACH, setting SIStatus = 0, discard temporary C-RNTI and State RRC_IDLE\n",Mod_id);
  //mac_xface->macphy_exit("");
}

void ra_succeeded(uint8_t Mod_id,uint8_t CC_id,uint8_t eNB_index)
{

  int i;

  LOG_I(PHY,"[UE %d][RAPROC] Random-access procedure succeeded. Set C-RNTI = Temporary C-RNTI\n",Mod_id);

  PHY_vars_UE_g[Mod_id][CC_id]->pdcch_vars[0][eNB_index]->crnti_is_temporary = 0;
  PHY_vars_UE_g[Mod_id][CC_id]->pdcch_vars[1][eNB_index]->crnti_is_temporary = 0;
  PHY_vars_UE_g[Mod_id][CC_id]->ulsch_Msg3_active[eNB_index] = 0;
  PHY_vars_UE_g[Mod_id][CC_id]->UE_mode[eNB_index] = PUSCH;

  for (i=0; i<8; i++) {
    if (PHY_vars_UE_g[Mod_id][CC_id]->ulsch[eNB_index]->harq_processes[i]) {
      PHY_vars_UE_g[Mod_id][CC_id]->ulsch[eNB_index]->harq_processes[i]->status=IDLE;
      PHY_vars_UE_g[Mod_id][CC_id]->dlsch[0][eNB_index][0]->harq_processes[i]->round=0;
      PHY_vars_UE_g[Mod_id][CC_id]->dlsch[1][eNB_index][0]->harq_processes[i]->round=0;
      PHY_vars_UE_g[Mod_id][CC_id]->ulsch[eNB_index]->harq_processes[i]->subframe_scheduling_flag=0;
    }
  }


}

UE_MODE_t get_ue_mode(uint8_t Mod_id,uint8_t CC_id,uint8_t eNB_index)
{

  return(PHY_vars_UE_g[Mod_id][CC_id]->UE_mode[eNB_index]);

}
void nr_process_timing_advance_rar(PHY_VARS_NR_UE *ue,UE_nr_rxtx_proc_t *proc,uint16_t timing_advance) {

  ue->timing_advance = timing_advance*4;


#ifdef DEBUG_PHY_PROC
  /* TODO: fix this log, what is 'HW timing advance'? */
  /*LOG_I(PHY,"[UE %d] AbsoluteSubFrame %d.%d, received (rar) timing_advance %d, HW timing advance %d\n",ue->Mod_id,proc->frame_rx, proc->nr_tti_rx_rx, ue->timing_advance);*/
  LOG_I(PHY,"[UE %d] AbsoluteSubFrame %d.%d, received (rar) timing_advance %d\n",ue->Mod_id,proc->frame_rx, proc->nr_tti_rx, ue->timing_advance);
#endif

}

uint8_t nr_is_SR_TXOp(PHY_VARS_NR_UE *ue,UE_nr_rxtx_proc_t *proc,uint8_t eNB_id)
{

  int nr_tti_tx=proc->nr_tti_tx;

  LOG_D(PHY,"[UE %d][SR %x] Frame %d nr_tti_tx %d Checking for SR TXOp (sr_ConfigIndex %d)\n",
        ue->Mod_id,ue->pdcch_vars[ue->current_thread_id[proc->nr_tti_rx]][eNB_id]->crnti,proc->frame_tx,nr_tti_tx,
        ue->scheduling_request_config[eNB_id].sr_ConfigIndex);

  if (ue->scheduling_request_config[eNB_id].sr_ConfigIndex <= 4) {        // 5 ms SR period
    if ((nr_tti_tx%5) == ue->scheduling_request_config[eNB_id].sr_ConfigIndex)
      return(1);
  } else if (ue->scheduling_request_config[eNB_id].sr_ConfigIndex <= 14) { // 10 ms SR period
    if (nr_tti_tx==(ue->scheduling_request_config[eNB_id].sr_ConfigIndex-5))
      return(1);
  } else if (ue->scheduling_request_config[eNB_id].sr_ConfigIndex <= 34) { // 20 ms SR period
    if ((10*(proc->frame_tx&1)+nr_tti_tx) == (ue->scheduling_request_config[eNB_id].sr_ConfigIndex-15))
      return(1);
  } else if (ue->scheduling_request_config[eNB_id].sr_ConfigIndex <= 74) { // 40 ms SR period
    if ((10*(proc->frame_tx&3)+nr_tti_tx) == (ue->scheduling_request_config[eNB_id].sr_ConfigIndex-35))
      return(1);
  } else if (ue->scheduling_request_config[eNB_id].sr_ConfigIndex <= 154) { // 80 ms SR period
    if ((10*(proc->frame_tx&7)+nr_tti_tx) == (ue->scheduling_request_config[eNB_id].sr_ConfigIndex-75))
      return(1);
  }

  return(0);
}

uint8_t is_cqi_TXOp(PHY_VARS_NR_UE *ue,UE_nr_rxtx_proc_t *proc,uint8_t eNB_id)
{
  int nr_tti_tx = proc->nr_tti_tx;
  int frame    = proc->frame_tx;
  CQI_REPORTPERIODIC *cqirep = &ue->cqi_report_config[eNB_id].CQI_ReportPeriodic;

  //LOG_I(PHY,"[UE %d][CRNTI %x] AbsSubFrame %d.%d Checking for CQI TXOp (cqi_ConfigIndex %d) isCQIOp %d\n",
  //      ue->Mod_id,ue->pdcch_vars[eNB_id]->crnti,frame,nr_tti_rx,
  //      cqirep->cqi_PMI_ConfigIndex,
  //      (((10*frame + nr_tti_tx) % cqirep->Npd) == cqirep->N_OFFSET_CQI));

  if (cqirep->cqi_PMI_ConfigIndex==-1)
    return(0);
  else if (((10*frame + nr_tti_tx) % cqirep->Npd) == cqirep->N_OFFSET_CQI)
    return(1);
  else
    return(0);
}
uint8_t is_ri_TXOp(PHY_VARS_NR_UE *ue,UE_nr_rxtx_proc_t *proc,uint8_t eNB_id)
{


  int nr_tti_tx = proc->nr_tti_tx;
  int frame    = proc->frame_tx;
  CQI_REPORTPERIODIC *cqirep = &ue->cqi_report_config[eNB_id].CQI_ReportPeriodic;
  int log2Mri = cqirep->ri_ConfigIndex/161;
  int N_OFFSET_RI = cqirep->ri_ConfigIndex % 161;

  //LOG_I(PHY,"[UE %d][CRNTI %x] AbsSubFrame %d.%d Checking for RI TXOp (ri_ConfigIndex %d) isRIOp %d\n",
  //      ue->Mod_id,ue->pdcch_vars[eNB_id]->crnti,frame,nr_tti_tx,
  //      cqirep->ri_ConfigIndex,
  //      (((10*frame + nr_tti_tx + cqirep->N_OFFSET_CQI - N_OFFSET_RI) % (cqirep->Npd<<log2Mri)) == 0));
  if (cqirep->ri_ConfigIndex==-1)
    return(0);
  else if (((10*frame + nr_tti_tx + cqirep->N_OFFSET_CQI - N_OFFSET_RI) % (cqirep->Npd<<log2Mri)) == 0)
    return(1);
  else
    return(0);
}

void compute_cqi_ri_resources(PHY_VARS_NR_UE *ue,
                              NR_UE_ULSCH_t *ulsch,
                              uint8_t eNB_id,
                              uint16_t rnti,
                              uint16_t p_rnti,
                              uint16_t cba_rnti,
                              uint8_t cqi_status,
                              uint8_t ri_status)
{
  //PHY_MEASUREMENTS *meas = &ue->measurements;
  //uint8_t transmission_mode = ue->transmission_mode[eNB_id];


  //LOG_I(PHY,"compute_cqi_ri_resources O_RI %d O %d uci format %d \n",ulsch->O_RI,ulsch->O,ulsch->uci_format);
  if (cqi_status == 1 || ri_status == 1)
    {
      ulsch->O = 4;
    }
}

void ue_compute_srs_occasion(PHY_VARS_NR_UE *ue,UE_nr_rxtx_proc_t *proc,uint8_t eNB_id,uint8_t isSubframeSRS)
{

  NR_DL_FRAME_PARMS *frame_parms = &ue->frame_parms;
  int frame_tx    = proc->frame_tx;
  int nr_tti_tx = proc->nr_tti_tx;
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
    if(pSoundingrs_ul_config_dedicated->srsConfigDedicatedSetup)
      {
	nr_compute_srs_pos(frame_parms->frame_type, pSoundingrs_ul_config_dedicated->srs_ConfigIndex, &srsPeriodicity, &srsOffset);

	LOG_D(PHY," srsPeriodicity: %d srsOffset: %d isSubframeSRS %d \n",srsPeriodicity,srsOffset,isSubframeSRS);

	// transmit SRS if the four following constraints are respected:
	// - UE is configured to transmit SRS
	// - SRS are configured in current nr_tti_rx
	// - UE is configured to send SRS in this nr_tti_tx

	// 36.213 8.2
	// 1- A UE shall not transmit SRS whenever SRS and PUCCH format 2/2a/2b transmissions happen to coincide in the same nr_tti_rx
	// 2- A UE shall not transmit SRS whenever SRS transmit
	//    on and PUCCH transmission carrying ACK/NACK and/or
	//    positive SR happen to coincide in the same nr_tti_rx if the parameter
	//    Simultaneous-AN-and-SRS is FALSE

	// check PUCCH format 2/2a/2b transmissions
	is_pucch2_subframe = nr_is_cqi_TXOp(ue,proc,eNB_id) && (ue->cqi_report_config[eNB_id].CQI_ReportPeriodic.cqi_PMI_ConfigIndex>0);
	is_pucch2_subframe = (nr_is_ri_TXOp(ue,proc,eNB_id) && (ue->cqi_report_config[eNB_id].CQI_ReportPeriodic.ri_ConfigIndex>0)) || is_pucch2_subframe;

	// check ACK/SR transmission
	if(frame_parms->soundingrs_ul_config_common.ackNackSRS_SimultaneousTransmission == FALSE)
          {
	    if(nr_is_SR_TXOp(ue,proc,eNB_id))
              {
		uint32_t SR_payload = 0;
		if (ue->mac_enabled==1)
                  {
		    int Mod_id = ue->Mod_id;
		    int CC_id = ue->CC_id;
		    SR_payload = mac_xface->ue_get_SR(Mod_id,
						      CC_id,
						      frame_tx,
						      eNB_id,
						      ue->pdcch_vars[ue->current_thread_id[proc->nr_tti_rx]][eNB_id]->crnti,
						      nr_tti_tx); // nr_tti_tx used for meas gap

		    if (SR_payload > 0)
		      is_sr_an_subframe = 1;
                  }
              }

	    uint8_t pucch_ack_payload[2];
	    if (nr_get_ack(&ue->frame_parms,
			   ue->dlsch[ue->current_thread_id[proc->nr_tti_rx]][eNB_id][0]->harq_ack,
			   nr_tti_tx,proc->nr_tti_rx,pucch_ack_payload,0) > 0)
              {
		is_sr_an_subframe = 1;
              }
          }

	// check SRS UE opportunity
	if( isSubframeSRS  &&
	    (((10*frame_tx+nr_tti_tx) % srsPeriodicity) == srsOffset)
	    )
          {
	    if ((is_pucch2_subframe == 0) && (is_sr_an_subframe == 0))
              {
		pSoundingrs_ul_config_dedicated->srsUeSubframe = 1;
		ue->ulsch[eNB_id]->srs_active   = 1;
		ue->ulsch[eNB_id]->Nsymb_pusch  = 12-(frame_parms->Ncp<<1)- ue->ulsch[eNB_id]->srs_active;
              }
	    else
              {
		LOG_I(PHY,"DROP UE-SRS-TX for this nr_tti_tx %d.%d: collision with PUCCH2 or SR/AN: PUCCH2-occasion: %d, SR-AN-occasion[simSRS-SR-AN %d]: %d  \n", frame_tx, nr_tti_tx, is_pucch2_subframe, frame_parms->soundingrs_ul_config_common.ackNackSRS_SimultaneousTransmission, is_sr_an_subframe);
              }
          }
      }
    LOG_D(PHY," srsCellSubframe: %d, srsUeSubframe: %d, Nsymb-pusch: %d \n", pSoundingrs_ul_config_dedicated->srsCellSubframe, pSoundingrs_ul_config_dedicated->srsUeSubframe, ue->ulsch[eNB_id]->Nsymb_pusch);
  }
}


void nr_get_cqipmiri_params(PHY_VARS_NR_UE *ue,uint8_t eNB_id)
{

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
    }
    else if (cqi_PMI_ConfigIndex > 317) {

      if (cqi_PMI_ConfigIndex <= 349) { // 32 ms CQI_PMI period
	cqirep->Npd = 32;
	cqirep->N_OFFSET_CQI = cqi_PMI_ConfigIndex-318;
      }
      else if (cqi_PMI_ConfigIndex <= 413) { // 64 ms CQI_PMI period
	cqirep->Npd = 64;
	cqirep->N_OFFSET_CQI = cqi_PMI_ConfigIndex-350;
      }
      else if (cqi_PMI_ConfigIndex <= 541) { // 128 ms CQI_PMI period
	cqirep->Npd = 128;
	cqirep->N_OFFSET_CQI = cqi_PMI_ConfigIndex-414;
      }
    }
  }
  else { // TDD
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
                             uint8_t bundling_flag)
{
  if((cqi_status == 0) && (ri_status==0))
    {
      // PUCCH Format 1 1a 1b
      // 1- SR only ==> PUCCH format 1
      // 2- 1bit Ack/Nack with/without SR  ==> PUCCH format 1a
      // 3- 2bits Ack/Nack with/without SR ==> PUCCH format 1b
      if((nb_cw == 1)&&(bundling_flag==bundling))
	{
          return pucch_format1a;
	}
      if((nb_cw == 1)&&(bundling_flag==multiplexing))
	{
          return pucch_format1b;
	}
      if(nb_cw == 2)
	{
          return pucch_format1b;
	}
      if(SR_payload == 1)
	{
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
    }
  else
    {
      // PUCCH Format 2 2a 2b
      // 1- CQI only or RI only  ==> PUCCH format 2
      // 2- CQI or RI + 1bit Ack/Nack for normal CP  ==> PUCCH format 2a
      // 3- CQI or RI + 2bits Ack/Nack for normal CP ==> PUCCH format 2b
      // 4- CQI or RI + Ack/Nack for extended CP ==> PUCCH format 2
      if(nb_cw == 0)
	{
          return pucch_format2;
	}
      if(cyclic_prefix_type == NORMAL)
	{
          if(nb_cw == 1)
	    {
              return pucch_format2a;
	    }
          if(nb_cw == 2)
	    {
              return pucch_format2b;
	    }
	}
      else
	{
          return pucch_format2;
	}
    }
  return pucch_format1a;
}

uint16_t nr_get_n1_pucch(PHY_VARS_NR_UE *ue,
			 UE_nr_rxtx_proc_t *proc,
			 nr_harq_status_t *harq_ack,
			 uint8_t eNB_id,
			 uint8_t *b,
			 uint8_t SR)
{

  NR_DL_FRAME_PARMS *frame_parms=&ue->frame_parms;
  uint8_t nCCE0,nCCE1,nCCE2,nCCE3,harq_ack1,harq_ack0,harq_ack3,harq_ack2;
  ANFBmode_t bundling_flag;
  uint16_t n1_pucch0=0,n1_pucch1=0,n1_pucch2=0,n1_pucch3=0,n1_pucch_inter;
  static uint8_t candidate_dl[9]; // which downlink(s) the current ACK/NACK is associating to
  uint8_t last_dl=0xff; // the last downlink with valid DL-DCI. for calculating the PUCCH resource index
  int sf;
  int M;
  uint8_t ack_counter=0;
  // clear this, important for case where n1_pucch selection is not used
  int nr_tti_tx=proc->nr_tti_tx;

  ue->pucch_sel[nr_tti_tx] = 0;

  if (frame_parms->frame_type == FDD ) { // FDD
    sf = (nr_tti_tx<4)? nr_tti_tx+6 : nr_tti_tx-4;
    LOG_D(PHY,"n1_pucch_UE: nr_tti_tx %d, nCCE %d\n",sf,ue->pdcch_vars[ue->current_thread_id[proc->nr_tti_rx]][eNB_id]->nCCE[sf]);

    if (SR == 0)
      return(frame_parms->pucch_config_common.n1PUCCH_AN + ue->pdcch_vars[ue->current_thread_id[proc->nr_tti_rx]][eNB_id]->nCCE[sf]);
    else
      return(ue->scheduling_request_config[eNB_id].sr_PUCCH_ResourceIndex);
  } else {

    bundling_flag = ue->pucch_config_dedicated[eNB_id].tdd_AckNackFeedbackMode;
#ifdef DEBUG_PHY_PROC

    if (bundling_flag==bundling) {
      LOG_D(PHY,"[UE%d] Frame %d nr_tti_tx %d : nr_get_n1_pucch, bundling, SR %d/%d\n",ue->Mod_id,proc->frame_tx,nr_tti_tx,SR,
            ue->scheduling_request_config[eNB_id].sr_PUCCH_ResourceIndex);
    } else {
      LOG_D(PHY,"[UE%d] Frame %d nr_tti_tx %d : nr_get_n1_pucch, multiplexing, SR %d/%d\n",ue->Mod_id,proc->frame_tx,nr_tti_tx,SR,
            ue->scheduling_request_config[eNB_id].sr_PUCCH_ResourceIndex);
    }

#endif

    switch (frame_parms->tdd_config) {
    case 1:  // DL:S:UL:UL:DL:DL:S:UL:UL:DL

      harq_ack0 = 2; // DTX
      M=1;

      // This is the offset for a particular nr_tti_tx (2,3,4) => (0,2,4)
      if (nr_tti_tx == 2) {  // ACK nr_tti_txs 5,6
        candidate_dl[0] = 6;
        candidate_dl[1] = 5;
        M=2;
      } else if (nr_tti_tx == 3) { // ACK nr_tti_tx 9
        candidate_dl[0] = 9;
      } else if (nr_tti_tx == 7) { // ACK nr_tti_txs 0,1
        candidate_dl[0] = 1;
        candidate_dl[1] = 0;
        M=2;
      } else if (nr_tti_tx == 8) { // ACK nr_tti_txs 4
        candidate_dl[0] = 4;
      } else {
        LOG_E(PHY,"[UE%d] : Frame %d phy_procedures_lte.c: get_n1pucch, illegal tx-nr_tti_tx %d for tdd_config %d\n",
              ue->Mod_id,proc->frame_tx,nr_tti_tx,frame_parms->tdd_config);
        return(0);
      }

      // checking which downlink candidate is the last downlink with valid DL-DCI
      int k;
      for (k=0;k<M;k++) {
        if (harq_ack[candidate_dl[k]].send_harq_status>0) {
          last_dl = candidate_dl[k];
          break;
        }
      }
      if (last_dl >= 10) {
        LOG_E(PHY,"[UE%d] : Frame %d phy_procedures_lte.c: get_n1pucch, illegal rx-nr_tti_tx %d (tx-nr_tti_tx %d) for tdd_config %d\n",
              ue->Mod_id,proc->frame_tx,last_dl,nr_tti_tx,frame_parms->tdd_config);
        return (0);
      }

      LOG_D(PHY,"SFN/SF %d/%d calculating n1_pucch0 from last_dl=%d\n",
	    proc->frame_tx%1024,
	    proc->nr_tti_tx,
	    last_dl);

      // i=0
      nCCE0 = ue->pdcch_vars[ue->current_thread_id[proc->nr_tti_rx]][eNB_id]->nCCE[last_dl];
      n1_pucch0 = nr_get_Np(frame_parms->N_RB_DL,nCCE0,0) + nCCE0+ frame_parms->pucch_config_common.n1PUCCH_AN;

      harq_ack0 = b[0];

      if (harq_ack0!=2) {  // DTX
        if (frame_parms->frame_type == FDD ) {
          if (SR == 0) {  // last paragraph pg 68 from 36.213 (v8.6), m=0
            b[0]=(M==2) ? 1-harq_ack0 : harq_ack0;
            b[1]=harq_ack0;   // in case we use pucch format 1b (subframes 2,7)
	    ue->pucch_sel[nr_tti_tx] = 0;
            return(n1_pucch0);
          } else { // SR and only 0 or 1 ACKs (first 2 entries in Table 7.3-1 of 36.213)
            b[0]=harq_ack0;
	    return(ue->scheduling_request_config[eNB_id].sr_PUCCH_ResourceIndex);
          }
        } else {
          if (SR == 0) {
            b[0] = harq_ack0;
            b[1] = harq_ack0;
            ue->pucch_sel[nr_tti_tx] = 0;
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
      // Note: this doesn't allow using nr_tti_tx 1 for PDSCH transmission!!! (i.e. SF 1 cannot be acked in SF 2)
      // set ACK/NAKs to DTX
      harq_ack1 = 2; // DTX
      harq_ack0 = 2; // DTX
      // This is the offset for a particular nr_tti_rx (2,3,4) => (0,2,4)
      last_dl = (nr_tti_tx-2)<<1;
      // i=0
      nCCE0 = ue->pdcch_vars[ue->current_thread_id[proc->nr_tti_rx]][eNB_id]->nCCE[5+last_dl];
      n1_pucch0 = nr_get_Np(frame_parms->N_RB_DL,nCCE0,0) + nCCE0+ frame_parms->pucch_config_common.n1PUCCH_AN;
      // i=1
      nCCE1 = ue->pdcch_vars[ue->current_thread_id[proc->nr_tti_rx]][eNB_id]->nCCE[(6+last_dl)%10];
      n1_pucch1 = nr_get_Np(frame_parms->N_RB_DL,nCCE1,1) + nCCE1 + frame_parms->pucch_config_common.n1PUCCH_AN;

      // set ACK/NAK to values if not DTX
      if (ue->dlsch[ue->current_thread_id[proc->nr_tti_rx]][eNB_id][0]->harq_ack[(6+last_dl)%10].send_harq_status>0)  // n-6 // nr_tti_tx 6 is to be ACK/NAKed
        harq_ack1 = ue->dlsch[ue->current_thread_id[proc->nr_tti_rx]][eNB_id][0]->harq_ack[(6+last_dl)%10].ack;

      if (ue->dlsch[ue->current_thread_id[proc->nr_tti_rx]][eNB_id][0]->harq_ack[5+last_dl].send_harq_status>0)  // n-6 // nr_tti_tx 5 is to be ACK/NAKed
        harq_ack0 = ue->dlsch[ue->current_thread_id[proc->nr_tti_rx]][eNB_id][0]->harq_ack[5+last_dl].ack;

      LOG_D(PHY,"SFN/SF %d/%d calculating n1_pucch cce0=%d n1_pucch0=%d cce1=%d n1_pucch1=%d\n",
	    proc->frame_tx%1024,
	    proc->nr_tti_tx,
	    nCCE0,n1_pucch0,
	    nCCE1,n1_pucch1);

      if (harq_ack1!=2) { // n-6 // nr_tti_tx 6,8,0 and maybe 5,7,9 is to be ACK/NAKed

        if ((bundling_flag==bundling)&&(SR == 0)) {  // This is for bundling without SR,
          // n1_pucch index takes value of smallest element in set {0,1}
          // i.e. 0 if harq_ack0 is not DTX, otherwise 1
          b[0] = harq_ack1;

          if (harq_ack0!=2)
            b[0]=b[0]&harq_ack0;

          ue->pucch_sel[nr_tti_tx] = 1;
          return(n1_pucch1);

        } else if ((bundling_flag==multiplexing)&&(SR==0)) { // Table 10.1
          if (harq_ack0 == 2)
            harq_ack0 = 0;

          b[1] = harq_ack0;
          b[0] = (harq_ack0!=harq_ack1)?0:1;

          if ((harq_ack0 == 1) && (harq_ack1 == 0)) {
            ue->pucch_sel[nr_tti_tx] = 0;
            return(n1_pucch0);
          } else {
            ue->pucch_sel[nr_tti_tx] = 1;
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
      } else if (harq_ack0!=2) { // n-7  // nr_tti_tx 5,7,9 only is to be ACK/NAKed
        if ((bundling_flag==bundling)&&(SR == 0)) {  // last paragraph pg 68 from 36.213 (v8.6), m=0
          b[0]=harq_ack0;
          ue->pucch_sel[nr_tti_tx] = 0;
          return(n1_pucch0);
        } else if ((bundling_flag==multiplexing)&&(SR==0)) { // Table 10.1 with i=1 set to DTX
          b[0] = harq_ack0;
          b[1] = 1-b[0];
          ue->pucch_sel[nr_tti_tx] = 0;
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
      // Note: this doesn't allow using nr_tti_tx 1 for PDSCH transmission!!! (i.e. SF 1 cannot be acked in SF 2)
      // set ACK/NAKs to DTX
      harq_ack3 = 2; // DTX
      harq_ack2 = 2; // DTX
      harq_ack1 = 2; // DTX
      harq_ack0 = 2; // DTX
      // This is the offset for a particular nr_tti_tx (2,3,4) => (0,2,4)
      //last_dl = (nr_tti_tx-2)<<1;
      if (nr_tti_tx == 2) {
	// i=0
	//nCCE0 = ue->pdcch_vars[ue->current_thread_id[proc->nr_tti_rx]][eNB_id]->nCCE[2+nr_tti_tx];
	nCCE0 = ue->pdcch_vars[ue->current_thread_id[proc->nr_tti_rx]][eNB_id]->nCCE[(8+nr_tti_tx)%10];
	n1_pucch0 = 2*nr_get_Np(frame_parms->N_RB_DL,nCCE0,0) + nCCE0+ frame_parms->pucch_config_common.n1PUCCH_AN;
	// i=1
	nCCE1 = ue->pdcch_vars[ue->current_thread_id[proc->nr_tti_rx]][eNB_id]->nCCE[2+nr_tti_tx];
	n1_pucch1 = nr_get_Np(frame_parms->N_RB_DL,nCCE1,0) + nr_get_Np(frame_parms->N_RB_DL,nCCE1,1) + nCCE1 + frame_parms->pucch_config_common.n1PUCCH_AN;
	// i=2
	nCCE2 = ue->pdcch_vars[ue->current_thread_id[proc->nr_tti_rx]][eNB_id]->nCCE[(8+nr_tti_tx)%10];

	n1_pucch2 = 2*nr_get_Np(frame_parms->N_RB_DL,nCCE2,1) + nCCE2+ frame_parms->pucch_config_common.n1PUCCH_AN;
	// i=3
	//nCCE3 = ue->pdcch_vars[ue->current_thread_id[proc->nr_tti_rx]][eNB_id]->nCCE[(9+nr_tti_tx)%10];
	//n1_pucch3 = nr_get_Np(frame_parms->N_RB_DL,nCCE3,1) + nCCE3 + frame_parms->pucch_config_common.n1PUCCH_AN;

	// set ACK/NAK to values if not DTX
	if (ue->dlsch[ue->current_thread_id[proc->nr_tti_rx]][eNB_id][0]->harq_ack[(8+nr_tti_tx)%10].send_harq_status>0)  // n-6 // nr_tti_tx 6 is to be ACK/NAKed
	  harq_ack0 = ue->dlsch[ue->current_thread_id[proc->nr_tti_rx]][eNB_id][0]->harq_ack[(8+nr_tti_tx)%10].ack;

	if (ue->dlsch[ue->current_thread_id[proc->nr_tti_rx]][eNB_id][0]->harq_ack[2+nr_tti_tx].send_harq_status>0)  // n-6 // nr_tti_tx 5 is to be ACK/NAKed
	  harq_ack1 = ue->dlsch[ue->current_thread_id[proc->nr_tti_rx]][eNB_id][0]->harq_ack[2+nr_tti_tx].ack;

	if (ue->dlsch[ue->current_thread_id[proc->nr_tti_rx]][eNB_id][0]->harq_ack[3+nr_tti_tx].send_harq_status>0)  // n-6 // nr_tti_tx 6 is to be ACK/NAKed
	  harq_ack2 = ue->dlsch[ue->current_thread_id[proc->nr_tti_rx]][eNB_id][0]->harq_ack[3+nr_tti_tx].ack;

	//if (ue->dlsch[ue->current_thread_id[proc->nr_tti_rx]][eNB_id][0]->harq_ack[(9+nr_tti_tx)%10].send_harq_status>0)  // n-6 // nr_tti_tx 5 is to be ACK/NAKed
	//harq_ack3 = ue->dlsch[ue->current_thread_id[proc->nr_tti_rx]][eNB_id][0]->harq_ack[(9+nr_tti_tx)%10].ack;
	//LOG_I(PHY,"SFN/SF %d/%d calculating n1_pucch cce0=%d n1_pucch0=%d cce1=%d n1_pucch1=%d cce2=%d n1_pucch2=%d\n",
	//                      proc->frame_tx%1024,
	//                      proc->nr_tti_tx_tx,
	//                      nCCE0,n1_pucch0,
	//                      nCCE1,n1_pucch1, nCCE2, n1_pucch2);
      }else if (nr_tti_tx == 3) {
	// i=0

	nCCE0 = ue->pdcch_vars[ue->current_thread_id[proc->nr_tti_rx]][eNB_id]->nCCE[4+nr_tti_tx];
	n1_pucch0 = 3*nr_get_Np(frame_parms->N_RB_DL,nCCE0,0) + nCCE0+ frame_parms->pucch_config_common.n1PUCCH_AN;
	// i=1
	nCCE1 = ue->pdcch_vars[ue->current_thread_id[proc->nr_tti_rx]][eNB_id]->nCCE[5+nr_tti_tx];
	n1_pucch1 = 2*nr_get_Np(frame_parms->N_RB_DL,nCCE1,0) + nr_get_Np(frame_parms->N_RB_DL,nCCE1,1) + nCCE1 + frame_parms->pucch_config_common.n1PUCCH_AN;
	// i=2
	nCCE2 = ue->pdcch_vars[ue->current_thread_id[proc->nr_tti_rx]][eNB_id]->nCCE[(6+nr_tti_tx)];
	n1_pucch2 = nr_get_Np(frame_parms->N_RB_DL,nCCE2,0) + 2*nr_get_Np(frame_parms->N_RB_DL,nCCE2,1) + nCCE2+ frame_parms->pucch_config_common.n1PUCCH_AN;
	// i=3
	nCCE3 = ue->pdcch_vars[ue->current_thread_id[proc->nr_tti_rx]][eNB_id]->nCCE[(3+nr_tti_tx)];
	n1_pucch3 = 3*nr_get_Np(frame_parms->N_RB_DL,nCCE3,1) + nCCE3 + frame_parms->pucch_config_common.n1PUCCH_AN;

	// set ACK/NAK to values if not DTX
	if (ue->dlsch[ue->current_thread_id[proc->nr_tti_rx]][eNB_id][0]->harq_ack[4+nr_tti_tx].send_harq_status>0)  // n-6 // nr_tti_tx 6 is to be ACK/NAKed
          harq_ack0 = ue->dlsch[ue->current_thread_id[proc->nr_tti_rx]][eNB_id][0]->harq_ack[4+nr_tti_tx].ack;

	if (ue->dlsch[ue->current_thread_id[proc->nr_tti_rx]][eNB_id][0]->harq_ack[5+nr_tti_tx].send_harq_status>0)  // n-6 // nr_tti_tx 5 is to be ACK/NAKed
          harq_ack1 = ue->dlsch[ue->current_thread_id[proc->nr_tti_rx]][eNB_id][0]->harq_ack[5+nr_tti_tx].ack;

	if (ue->dlsch[ue->current_thread_id[proc->nr_tti_rx]][eNB_id][0]->harq_ack[(6+nr_tti_tx)].send_harq_status>0)  // n-6 // nr_tti_tx 6 is to be ACK/NAKed
          harq_ack2 = ue->dlsch[ue->current_thread_id[proc->nr_tti_rx]][eNB_id][0]->harq_ack[(6+nr_tti_tx)].ack;

	if (ue->dlsch[ue->current_thread_id[proc->nr_tti_rx]][eNB_id][0]->harq_ack[(3+nr_tti_tx)].send_harq_status>0)  // n-6 // nr_tti_tx 5 is to be ACK/NAKed
          harq_ack3 = ue->dlsch[ue->current_thread_id[proc->nr_tti_rx]][eNB_id][0]->harq_ack[(3+nr_tti_tx)].ack;
      }

      //LOG_I(PHY,"SFN/SF %d/%d calculating n1_pucch cce0=%d n1_pucch0=%d harq_ack0=%d cce1=%d n1_pucch1=%d harq_ack1=%d cce2=%d n1_pucch2=%d harq_ack2=%d cce3=%d n1_pucch3=%d harq_ack3=%d bundling_flag=%d\n",
      //                                proc->frame_tx%1024,
      //                                proc->nr_tti_tx,
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

	/*if (nr_tti_tx == 3) {
	  n1_pucch_inter = n1_pucch2;
	  } else if (nr_tti_tx == 2) {
	  n1_pucch_inter = n1_pucch1;
	  }*/

	//LOG_I(PHY,"SFN/SF %d/%d calculating n1_pucch n1_pucch_inter=%d  b[0]=%d b[1]=%d \n",
	//                                           proc->frame_tx%1024,
	//                                           proc->nr_tti_tx,n1_pucch_inter,
	//                                           b[0],b[1]);

	return(n1_pucch_inter);

      } else if ((bundling_flag==multiplexing)&&(SR==0)) { // Table 10.1

	if (nr_tti_tx == 3) {
	  LOG_I(PHY, "sbuframe=%d \n",nr_tti_tx);
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
	} else if (nr_tti_tx == 2) {
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



void ulsch_common_procedures(PHY_VARS_NR_UE *ue, UE_nr_rxtx_proc_t *proc, uint8_t empty_subframe) {

  int aa;
  NR_DL_FRAME_PARMS *frame_parms=&ue->frame_parms;

  int nsymb;
  int nr_tti_tx = proc->nr_tti_tx;
  int frame_tx = proc->frame_tx;
  int ulsch_start;
  int overflow=0;
#if defined(EXMIMO) || defined(OAI_USRP) || defined(OAI_BLADERF) || defined(OAI_LMSSDR) || defined(OAI_ADRV9371_ZC706)
  int k,l;
  int dummy_tx_buffer[frame_parms->samples_per_subframe] __attribute__((aligned(16)));
#endif

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_UE_TX_ULSCH_COMMON,VCD_FUNCTION_IN);
#if UE_TIMING_TRACE
  start_meas(&ue->ofdm_mod_stats);
#endif
  nsymb = (frame_parms->Ncp == 0) ? 14 : 12;

#if defined(EXMIMO) || defined(OAI_USRP) || defined(OAI_BLADERF) || defined(OAI_LMSSDR) || defined(OAI_ADRV9371_ZC706)//this is the EXPRESS MIMO case
  ulsch_start = (ue->rx_offset+nr_tti_tx*frame_parms->samples_per_subframe-
		 ue->hw_timing_advance-
		 ue->timing_advance-
		 ue->N_TA_offset+5);
  //LOG_E(PHY,"ul-signal [nr_tti_rx: %d, ulsch_start %d]\n",nr_tti_tx, ulsch_start);

  if(ulsch_start < 0)
    ulsch_start = ulsch_start + (LTE_NUMBER_OF_SUBFRAMES_PER_FRAME*frame_parms->samples_per_subframe);

  if (ulsch_start > (LTE_NUMBER_OF_SUBFRAMES_PER_FRAME*frame_parms->samples_per_subframe))
    ulsch_start = ulsch_start % (LTE_NUMBER_OF_SUBFRAMES_PER_FRAME*frame_parms->samples_per_subframe);

  //LOG_E(PHY,"ul-signal [nr_tti_rx: %d, ulsch_start %d]\n",nr_tti_tx, ulsch_start);
#else //this is the normal case
  ulsch_start = (frame_parms->samples_per_subframe*nr_tti_tx)-ue->N_TA_offset; //-ue->timing_advance;
#endif //else EXMIMO

  //#if defined(EXMIMO) || defined(OAI_USRP) || defined(OAI_BLADERF) || defined(OAI_LMSSDR) || defined(OAI_ADRV9371_ZC706)
  if (empty_subframe)
    {
      //#if 1
      overflow = ulsch_start - 9*frame_parms->samples_per_subframe;
      for (aa=0; aa<frame_parms->nb_antennas_tx; aa++) {

	if (overflow > 0)
	  {
	    memset(&ue->common_vars.txdata[aa][ulsch_start],0,4*(frame_parms->samples_per_subframe-overflow));
	    memset(&ue->common_vars.txdata[aa][0],0,4*overflow);
	  }
	else
	  {
	    memset(&ue->common_vars.txdata[aa][ulsch_start],0,4*frame_parms->samples_per_subframe);
	  }
      }
      /*#else
	overflow = ulsch_start - 9*frame_parms->samples_per_subframe;
	for (aa=0; aa<frame_parms->nb_antennas_tx; aa++) {
	for (k=ulsch_start; k<cmin(frame_parms->samples_per_subframe*LTE_NUMBER_OF_SUBFRAMES_PER_FRAME,ulsch_start+frame_parms->samples_per_subframe); k++) {
	((short*)ue->common_vars.txdata[aa])[2*k] = 0;
	((short*)ue->common_vars.txdata[aa])[2*k+1] = 0;
	}

	for (k=0; k<overflow; k++) {
	((short*)ue->common_vars.txdata[aa])[2*k] = 0;
	((short*)ue->common_vars.txdata[aa])[2*k+1] = 0;
	}
	}
	endif*/
      return;
    }


  if ((frame_tx%100) == 0)
    LOG_D(PHY,"[UE %d] Frame %d, nr_tti_rx %d: ulsch_start = %d (rxoff %d, HW TA %d, timing advance %d, TA_offset %d\n",
	  ue->Mod_id,frame_tx,nr_tti_tx,
	  ulsch_start,
	  ue->rx_offset,
	  ue->hw_timing_advance,
	  ue->timing_advance,
	  ue->N_TA_offset);


  for (aa=0; aa<frame_parms->nb_antennas_tx; aa++) {
    if (frame_parms->Ncp == 1)
      PHY_ofdm_mod(&ue->common_vars.txdataF[aa][nr_tti_tx*nsymb*frame_parms->ofdm_symbol_size],
#if defined(EXMIMO) || defined(OAI_USRP) || defined(OAI_BLADERF) || defined(OAI_LMSSDR) || defined(OAI_ADRV9371_ZC706)
		   dummy_tx_buffer,
#else
		   &ue->common_vars.txdata[aa][ulsch_start],
#endif
		   frame_parms->ofdm_symbol_size,
		   nsymb,
		   frame_parms->nb_prefix_samples,
		   CYCLIC_PREFIX);
    else
      normal_prefix_mod(&ue->common_vars.txdataF[aa][nr_tti_tx*nsymb*frame_parms->ofdm_symbol_size],
#if defined(EXMIMO) || defined(OAI_USRP) || defined(OAI_BLADERF) || defined(OAI_LMSSDR) || defined(OAI_ADRV9371_ZC706)
			dummy_tx_buffer,
#else
			&ue->common_vars.txdata[aa][ulsch_start],
#endif
			nsymb,
			&ue->frame_parms);


#if defined(EXMIMO) || defined(OAI_USRP) || defined(OAI_BLADERF) || defined(OAI_LMSSDR) || defined(OAI_ADRV9371_ZC706)
    apply_7_5_kHz(ue,dummy_tx_buffer,0);
    apply_7_5_kHz(ue,dummy_tx_buffer,1);
#else
    apply_7_5_kHz(ue,&ue->common_vars.txdata[aa][ulsch_start],0);
    apply_7_5_kHz(ue,&ue->common_vars.txdata[aa][ulsch_start],1);
#endif


#if defined(EXMIMO) || defined(OAI_USRP) || defined(OAI_BLADERF) || defined(OAI_LMSSDR) || defined(OAI_ADRV9371_ZC706)
    overflow = ulsch_start - 9*frame_parms->samples_per_subframe;


    for (k=ulsch_start,l=0; k<cmin(frame_parms->samples_per_subframe*LTE_NUMBER_OF_SUBFRAMES_PER_FRAME,ulsch_start+frame_parms->samples_per_subframe); k++,l++) {
      ((short*)ue->common_vars.txdata[aa])[2*k] = ((short*)dummy_tx_buffer)[2*l]<<4;
      ((short*)ue->common_vars.txdata[aa])[2*k+1] = ((short*)dummy_tx_buffer)[2*l+1]<<4;
    }

    for (k=0; k<overflow; k++,l++) {
      ((short*)ue->common_vars.txdata[aa])[2*k] = ((short*)dummy_tx_buffer)[2*l]<<4;
      ((short*)ue->common_vars.txdata[aa])[2*k+1] = ((short*)dummy_tx_buffer)[2*l+1]<<4;
    }
#if defined(EXMIMO)
    // handle switch before 1st TX nr_tti_rx, guarantee that the slot prior to transmission is switch on
    for (k=ulsch_start - (frame_parms->samples_per_subframe>>1) ; k<ulsch_start ; k++) {
      if (k<0)
	ue->common_vars.txdata[aa][k+frame_parms->samples_per_subframe*LTE_NUMBER_OF_SUBFRAMES_PER_FRAME] &= 0xFFFEFFFE;
      else if (k>(frame_parms->samples_per_subframe*LTE_NUMBER_OF_SUBFRAMES_PER_FRAME))
	ue->common_vars.txdata[aa][k-frame_parms->samples_per_subframe*LTE_NUMBER_OF_SUBFRAMES_PER_FRAME] &= 0xFFFEFFFE;
      else
	ue->common_vars.txdata[aa][k] &= 0xFFFEFFFE;
    }
#endif
#endif
    /*
      only for debug
      LOG_I(PHY,"ul-signal [nr_tti_rx: %d, ulsch_start %d, TA: %d, rxOffset: %d, timing_advance: %d, hw_timing_advance: %d]\n",nr_tti_tx, ulsch_start, ue->N_TA_offset, ue->rx_offset, ue->timing_advance, ue->hw_timing_advance);
      if( (crash == 1) && (nr_tti_tx == 0) )
      {
      LOG_E(PHY,"***** DUMP TX Signal [ulsch_start %d] *****\n",ulsch_start);
      write_output("txBuff.m","txSignal",&ue->common_vars.txdata[aa][ulsch_start],frame_parms->samples_per_subframe,1,1);
      }
    */

  } //nb_antennas_tx

#if UE_TIMING_TRACE
  stop_meas(&ue->ofdm_mod_stats);
#endif

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_UE_TX_ULSCH_COMMON,VCD_FUNCTION_OUT);

}

#endif


void nr_process_timing_advance(module_id_t Mod_id, uint8_t CC_id, uint8_t ta_command, uint8_t mu, uint16_t bwp_ul_NB_RB){

  // 3GPP TS 38.213 p4.2
  // scale by the scs numerology
  int factor_mu = 1 << mu;
  uint16_t bw_scaling;

  // scale the 16 factor in N_TA calculation in 38.213 section 4.2 according to the used FFT size
  switch (bwp_ul_NB_RB) {
    case 32:  bw_scaling =  4; break;
    case 66:  bw_scaling =  8; break;
    case 106: bw_scaling = 16; break;
    case 217: bw_scaling = 32; break;
    case 245: bw_scaling = 32; break;
    case 273: bw_scaling = 32; break;
    default: abort();
  }

  PHY_vars_UE_g[Mod_id][CC_id]->timing_advance += (ta_command - 31) * bw_scaling / factor_mu;

  LOG_D(PHY, "[UE %d] Got timing advance command %u from MAC, new value is %u\n", Mod_id, ta_command, PHY_vars_UE_g[Mod_id][CC_id]->timing_advance);
}

#if 0
void ue_ulsch_uespec_procedures(PHY_VARS_NR_UE *ue,
								UE_nr_rxtx_proc_t *proc,
								uint8_t eNB_id,
								uint8_t abstraction_flag)
{
  int nr_tti_tx=proc->nr_tti_tx;

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_UE_TX_ULSCH_UESPEC,VCD_FUNCTION_IN);

  /* reset harq for tx of current rx slot because it is sure that transmission has already been achieved for this slot */
  set_tx_harq_id(ue->ulsch[ue->current_thread_id[nr_tti_tx]][eNB_id][0], NR_MAX_HARQ_PROCESSES, proc->nr_tti_rx);

#if 0
  int frame_tx=proc->frame_tx;
  int harq_pid;
  /* get harq pid related to this next tx slot */
  harq_pid = get_tx_harq_id(ue->ulsch[ue->current_thread_id[nr_tti_tx]][eNB_id][0], nr_tti_tx);

  int tx_amp;
  unsigned int input_buffer_length;
  int Mod_id = ue->Mod_id;
  int CC_id = ue->CC_id;
  uint8_t Msg3_flag=0;
  uint16_t first_rb, nb_rb;
  uint8_t ulsch_input_buffer[5477] __attribute__ ((aligned(32)));
  uint8_t access_mode;
  uint8_t Nbundled=0;
  uint8_t NbundledCw1=0;
  uint8_t ack_status_cw0=0;
  uint8_t ack_status_cw1=0;
  uint8_t cqi_status = 0;
  uint8_t ri_status  = 0;
  if (ue->mac_enabled == 1) {
    if ((ue->ulsch_Msg3_active[eNB_id] == 1) &&
	(ue->ulsch_Msg3_frame[eNB_id] == frame_tx) &&
	(ue->ulsch_Msg3_subframe[eNB_id] == nr_tti_tx)) { // Initial Transmission of Msg3

      ue->ulsch[eNB_id]->harq_processes[harq_pid]->subframe_scheduling_flag = 1;

      if (ue->ulsch[eNB_id]->harq_processes[harq_pid]->round==0)
    	  generate_ue_ulsch_params_from_rar(ue,
    			                            proc,
											eNB_id);

      ue->ulsch[eNB_id]->power_offset = 14;
      LOG_D(PHY,"[UE  %d][RAPROC] Frame %d: Setting Msg3_flag in nr_tti_rx %d, for harq_pid %d\n",
	    Mod_id,
	    frame_tx,
	    nr_tti_tx,
	    harq_pid);
      Msg3_flag = 1;
    } else {

      /* no pusch has been scheduled on this transmit slot */
      if (harq_pid == NR_MAX_HARQ_PROCESSES) {
	LOG_E(PHY,"[UE%d] Frame %d nr_tti_rx %d ulsch_decoding.c: FATAL ERROR: illegal harq_pid, returning\n",
	      Mod_id,frame_tx, nr_tti_tx);
	//mac_xface->macphy_exit("Error in ulsch_decoding");
	VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_UE_TX, VCD_FUNCTION_OUT);
#if UE_TIMING_TRACE
	stop_meas(&ue->phy_proc_tx);
#endif
	return;
      }

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

    if (20 < ue->ulsch[eNB_id]->harq_processes[harq_pid]->mcs) {
      LOG_D(PHY,"Not supported MCS in OAI mcs=%d\n", ue->ulsch[eNB_id]->harq_processes[harq_pid]->mcs);
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
    LOG_D(PHY,"Generating PUSCH (Abssubframe: %d.%d): harq-Id: %d, round: %d, MaxReTrans: %d \n",frame_tx,nr_tti_tx,harq_pid,ue->ulsch[eNB_id]->harq_processes[harq_pid]->round,ue->ulsch[eNB_id]->Mlimit);
    if (ue->ulsch[eNB_id]->harq_processes[harq_pid]->round >= (ue->ulsch[eNB_id]->Mlimit - 1))
      {
        LOG_D(PHY,"PUSCH MAX Retransmission achieved ==> send last pusch\n");
        ue->ulsch[eNB_id]->harq_processes[harq_pid]->subframe_scheduling_flag = 0;
        ue->ulsch[eNB_id]->harq_processes[harq_pid]->round  = 0;
      }

    ack_status_cw0 = nr_reset_ack(&ue->frame_parms,
				  ue->dlsch[ue->current_thread_id[proc->nr_tti_rx]][eNB_id][0]->harq_ack,
				  nr_tti_tx,
				  proc->nr_tti_rx,
				  ue->ulsch[eNB_id]->o_ACK,
				  &Nbundled,
				  0);
    ack_status_cw1 = nr_reset_ack(&ue->frame_parms,
				  ue->dlsch[ue->current_thread_id[proc->nr_tti_rx]][eNB_id][1]->harq_ack,
				  nr_tti_tx,
				  proc->nr_tti_rx,
				  ue->ulsch[eNB_id]->o_ACK,
				  &NbundledCw1,
				  1);

    //Nbundled = ue->dlsch[ue->current_thread_id[proc->nr_tti_rx]][eNB_id][0]->harq_ack;
    //ue->ulsch[eNB_id]->bundling = Nbundled;

    first_rb = ue->ulsch[eNB_id]->harq_processes[harq_pid]->first_rb;
    nb_rb = ue->ulsch[eNB_id]->harq_processes[harq_pid]->nb_rb;


    // check Periodic CQI/RI reporting
    cqi_status = ((ue->cqi_report_config[eNB_id].CQI_ReportPeriodic.cqi_PMI_ConfigIndex>0)&&
		  (nr_is_cqi_TXOp(ue,proc,eNB_id)==1));

    ri_status = ((ue->cqi_report_config[eNB_id].CQI_ReportPeriodic.ri_ConfigIndex>0) &&
		 (nr_is_ri_TXOp(ue,proc,eNB_id)==1));

    // compute CQI/RI resources
    compute_cqi_ri_resources(ue, ue->ulsch[eNB_id], eNB_id, ue->ulsch[eNB_id]->rnti, P_RNTI, CBA_RNTI, cqi_status, ri_status);

    if (ack_status_cw0 > 0) {

      // check if we received a PDSCH at nr_tti_tx - 4
      // ==> send ACK/NACK on PUSCH
      if (ue->frame_parms.frame_type == FDD)
	{
	  ue->ulsch[eNB_id]->harq_processes[harq_pid]->O_ACK = ack_status_cw0 + ack_status_cw1;
	}


#if T_TRACER
      if(ue->ulsch[eNB_id]->o_ACK[0])
	{
	  LOG_I(PHY,"PUSCH ACK\n");
	  T(T_UE_PHY_DLSCH_UE_ACK, T_INT(eNB_id), T_INT(frame_tx%1024), T_INT(nr_tti_tx), T_INT(Mod_id), T_INT(ue->dlsch[ue->current_thread_id[proc->nr_tti_rx]][eNB_id][0]->rnti),
	    T_INT(ue->dlsch[ue->current_thread_id[proc->nr_tti_rx]][eNB_id][0]->current_harq_pid));
	}
      else
	{
	  LOG_I(PHY,"PUSCH NACK\n");
	  T(T_UE_PHY_DLSCH_UE_NACK, T_INT(eNB_id), T_INT(frame_tx%1024), T_INT(nr_tti_tx), T_INT(Mod_id), T_INT(ue->dlsch[ue->current_thread_id[proc->nr_tti_rx]][eNB_id][0]->rnti),
	    T_INT(ue->dlsch[ue->current_thread_id[proc->nr_tti_rx]][eNB_id][0]->current_harq_pid));
	}
#endif
#ifdef UE_DEBUG_TRACE
      LOG_I(PHY,"[UE  %d][PDSCH %x] AbsSubFrame %d.%d Generating ACK (%d,%d) for %d bits on PUSCH\n",
	    Mod_id,
	    ue->ulsch[eNB_id]->rnti,
	    frame_tx%1024,nr_tti_tx,
	    ue->ulsch[eNB_id]->o_ACK[0],ue->ulsch[eNB_id]->o_ACK[1],
	    ue->ulsch[eNB_id]->harq_processes[harq_pid]->O_ACK);
#endif
    }

    //#ifdef UE_DEBUG_TRACE
    LOG_I(PHY,
	  "[UE  %d][PUSCH %d] AbsSubframe %d.%d Generating PUSCH : first_rb %d, nb_rb %d, round %d, mcs %d, tbs %d, rv %d, "
	  "cyclic_shift %d (cyclic_shift_common %d,n_DMRS2 %d,n_PRS %d), ACK (%d,%d), O_ACK %d, ack_status_cw0 %d ack_status_cw1 %d bundling %d, Nbundled %d, CQI %d, RI %d\n",
          Mod_id,harq_pid,frame_tx%1024,nr_tti_tx,
          first_rb,nb_rb,
          ue->ulsch[eNB_id]->harq_processes[harq_pid]->round,
          ue->ulsch[eNB_id]->harq_processes[harq_pid]->mcs,
	  ue->ulsch[eNB_id]->harq_processes[harq_pid]->TBS,
          ue->ulsch[eNB_id]->harq_processes[harq_pid]->rvidx,
          (ue->frame_parms.pusch_config_common.ul_ReferenceSignalsPUSCH.cyclicShift+
           ue->ulsch[eNB_id]->harq_processes[harq_pid]->n_DMRS2+
           ue->frame_parms.pusch_config_common.ul_ReferenceSignalsPUSCH.nPRS[nr_tti_tx<<1])%12,
          ue->frame_parms.pusch_config_common.ul_ReferenceSignalsPUSCH.cyclicShift,
          ue->ulsch[eNB_id]->harq_processes[harq_pid]->n_DMRS2,
          ue->frame_parms.pusch_config_common.ul_ReferenceSignalsPUSCH.nPRS[nr_tti_tx<<1],
          ue->ulsch[eNB_id]->o_ACK[0],ue->ulsch[eNB_id]->o_ACK[1],
          ue->ulsch[eNB_id]->harq_processes[harq_pid]->O_ACK,
          ack_status_cw0,
          ack_status_cw1,
          ue->ulsch[eNB_id]->bundling, Nbundled,
          cqi_status,
          ri_status);
    //#endif





    if (Msg3_flag == 1) {
      LOG_I(PHY,"[UE  %d][RAPROC] Frame %d, nr_tti_rx %d Generating (RRCConnectionRequest) Msg3 (nb_rb %d, first_rb %d, round %d, rvidx %d) Msg3: %x.%x.%x|%x.%x.%x.%x.%x.%x\n",Mod_id,frame_tx,
	    nr_tti_tx,
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
#if UE_TIMING_TRACE
      start_meas(&ue->ulsch_encoding_stats);
#endif

      if (abstraction_flag==0) {
	if (ulsch_encoding(ue->prach_resources[eNB_id]->Msg3,
			   ue,
			   harq_pid,
			   eNB_id,
			   proc->nr_tti_rx,
			   ue->transmission_mode[eNB_id],0,0)!=0) {
	  LOG_E(PHY,"ulsch_coding.c: FATAL ERROR: returning\n");
	  //mac_xface->macphy_exit("Error in ulsch_coding");
	  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_UE_TX, VCD_FUNCTION_OUT);
#if UE_TIMING_TRACE
	  stop_meas(&ue->phy_proc_tx);
	  printf("------FULL TX PROC : %5.2f ------\n",ue->phy_proc_tx.p_time/(cpuf*1000.0));
#endif
	  return;
	}
      }

#ifdef PHY_ABSTRACTION
      else {
        ulsch_encoding_emul(ue->prach_resources[eNB_id]->Msg3,ue,eNB_id,proc->nr_tti_rx,harq_pid,0);
      }

#endif

#if UE_TIMING_TRACE
      stop_meas(&ue->ulsch_encoding_stats);
#endif
      if (ue->mac_enabled == 1) {
	// signal MAC that Msg3 was sent
	//mac_xface->Msg3_transmitted(Mod_id,
	CC_id,
	  frame_tx,
	  eNB_id);
    }
  } // Msg3_flag==1
  else {
    input_buffer_length = ue->ulsch[eNB_id]->harq_processes[harq_pid]->TBS/8;

    if (ue->mac_enabled==1) {
      //  LOG_D(PHY,"[UE  %d] ULSCH : Searching for MAC SDUs\n",Mod_id);
      if (ue->ulsch[eNB_id]->harq_processes[harq_pid]->round==0) {
	//if (ue->ulsch[eNB_id]->harq_processes[harq_pid]->calibration_flag == 0) {
	access_mode=SCHEDULED_ACCESS;
	//mac_xface->ue_get_sdu(Mod_id,
        CC_id,
	  frame_tx,
	  proc->subframe_tx,
	  nr_tti_tx%(ue->frame_parms.ttis_per_subframe),
	  eNB_id,
	  ulsch_input_buffer,
	  input_buffer_length,
	  &access_mode);
    }

#ifdef DEBUG_PHY_PROC
#ifdef DEBUG_ULSCH
    LOG_D(PHY,"[UE] Frame %d, nr_tti_rx %d : ULSCH SDU (TX harq_pid %d)  (%d bytes) : \n",frame_tx,nr_tti_tx,harq_pid, ue->ulsch[eNB_id]->harq_processes[harq_pid]->TBS>>3);

    for (int i=0; i<ue->ulsch[eNB_id]->harq_processes[harq_pid]->TBS>>3; i++)
      LOG_T(PHY,"%x.",ulsch_input_buffer[i]);

    LOG_T(PHY,"\n");
#endif
#endif
  }
  else {
    unsigned int taus(void);

    for (int i=0; i<input_buffer_length; i++)
      ulsch_input_buffer[i]= (uint8_t)(taus()&0xff);

  }

#if UE_TIMING_TRACE
  start_meas(&ue->ulsch_encoding_stats);
#endif
  if (abstraction_flag==0) {

    if (ulsch_encoding(ulsch_input_buffer,
		       ue,
		       harq_pid,
		       eNB_id,
		       proc->nr_tti_rx,
		       ue->transmission_mode[eNB_id],0,
		       Nbundled)!=0) {
      LOG_E(PHY,"ulsch_coding.c: FATAL ERROR: returning\n");
      VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_UE_TX, VCD_FUNCTION_OUT);
#if UE_TIMING_TRACE
      stop_meas(&ue->phy_proc_tx);
#endif
      return;
    }
  }

#ifdef PHY_ABSTRACTION
  else {
    ulsch_encoding_emul(ulsch_input_buffer,ue,eNB_id,proc->nr_tti_rx,harq_pid,0);
  }

#endif
#if UE_TIMING_TRACE
  stop_meas(&ue->ulsch_encoding_stats);
#endif
}

if (abstraction_flag == 0) {
  if (ue->mac_enabled==1) {
    nr_pusch_power_cntl(ue,proc,eNB_id,1, abstraction_flag);
    ue->tx_power_dBm[nr_tti_tx] = ue->ulsch[eNB_id]->Po_PUSCH;
  }
  else {
    ue->tx_power_dBm[nr_tti_tx] = ue->tx_power_max_dBm;
  }
  ue->tx_total_RE[nr_tti_tx] = nb_rb*12;

#if defined(EXMIMO) || defined(OAI_USRP) || defined(OAI_BLADERF) || defined(OAI_LMSSDR) || defined(OAI_ADRV9371_ZC706)
  tx_amp = nr_get_tx_amp(ue->tx_power_dBm[nr_tti_tx],
		      ue->tx_power_max_dBm,
		      ue->frame_parms.N_RB_UL,
		      nb_rb);
#else
  tx_amp = AMP;
#endif
#if T_TRACER
  T(T_UE_PHY_PUSCH_TX_POWER, T_INT(eNB_id),T_INT(Mod_id), T_INT(frame_tx%1024), T_INT(nr_tti_tx),T_INT(ue->tx_power_dBm[nr_tti_tx]),
    T_INT(tx_amp),T_INT(ue->ulsch[eNB_id]->f_pusch),T_INT(get_nr_PL(Mod_id,0,eNB_id)),T_INT(nb_rb));
#endif

#ifdef UE_DEBUG_TRACE
  LOG_I(PHY,"[UE  %d][PUSCH %d] AbsSubFrame %d.%d, generating PUSCH, Po_PUSCH: %d dBm (max %d dBm), amp %d\n",
	Mod_id,harq_pid,frame_tx%1024,nr_tti_tx,ue->tx_power_dBm[nr_tti_tx],ue->tx_power_max_dBm, tx_amp);
#endif

  if (tx_amp>100)
    tx_amp =100;

  //LOG_I(PHY,"[UE  %d][PUSCH %d] after AbsSubFrame %d.%d, generating PUSCH, Po_PUSCH: %d dBm (max %d dBm), amp %d\n",
  //    Mod_id,harq_pid,frame_tx%1024,nr_tti_tx,ue->tx_power_dBm[nr_tti_tx],ue->tx_power_max_dBm, tx_amp);

      
#if UE_TIMING_TRACE

  start_meas(&ue->ulsch_modulation_stats);
#endif
  ulsch_modulation(ue->common_vars.txdataF,
		   tx_amp,
		   frame_tx,
		   nr_tti_tx,
		   &ue->frame_parms,
		   ue->ulsch[eNB_id]);
  for (int aa=0; aa<1/*frame_parms->nb_antennas_tx*/; aa++)
    generate_drs_pusch(ue,
		       proc,
		       eNB_id,
		       tx_amp,
		       nr_tti_tx,
		       first_rb,
		       nb_rb,
		       aa);
#if UE_TIMING_TRACE
  stop_meas(&ue->ulsch_modulation_stats);
#endif
 }

if (abstraction_flag==1) {
  // clear SR
  ue->sr[nr_tti_tx]=0;
 }
} // subframe_scheduling_flag==1

VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_UE_TX_ULSCH_UESPEC,VCD_FUNCTION_OUT);

#endif

}
#endif

#if 0

void ue_srs_procedures(PHY_VARS_NR_UE *ue,UE_nr_rxtx_proc_t *proc,uint8_t eNB_id,uint8_t abstraction_flag)
{

  //NR_DL_FRAME_PARMS *frame_parms = &ue->frame_parms;
  //int8_t  frame_tx    = proc->frame_tx;
  int8_t  nr_tti_tx = proc->nr_tti_tx;
  int16_t tx_amp;
  int16_t Po_SRS;
  uint8_t nb_rb_srs;

  SOUNDINGRS_UL_CONFIG_DEDICATED *pSoundingrs_ul_config_dedicated=&ue->soundingrs_ul_config_dedicated[eNB_id];
  uint8_t isSrsTxOccasion = pSoundingrs_ul_config_dedicated->srsUeSubframe;

  if(isSrsTxOccasion)
    {
      ue->generate_ul_signal[eNB_id] = 1;
      if (ue->mac_enabled==1)
	{
	  srs_power_cntl(ue,proc,eNB_id, (uint8_t*)(&nb_rb_srs), abstraction_flag);
	  Po_SRS = ue->ulsch[eNB_id]->Po_SRS;
	}
      else
	{
	  Po_SRS = ue->tx_power_max_dBm;
	}

#if defined(EXMIMO) || defined(OAI_USRP) || defined(OAI_BLADERF) || defined(OAI_LMSSDR) || defined(OAI_ADRV9371_ZC706)
      if (ue->mac_enabled==1)
	{
	  tx_amp = nr_get_tx_amp(Po_SRS,
			      ue->tx_power_max_dBm,
			      ue->frame_parms.N_RB_UL,
			      nb_rb_srs);
	}
      else
	{
	  tx_amp = AMP;
	}
#else
      tx_amp = AMP;
#endif
      LOG_D(PHY,"SRS PROC; TX_MAX_POWER %d, Po_SRS %d, NB_RB_UL %d, NB_RB_SRS %d TX_AMPL %d\n",ue->tx_power_max_dBm,
            Po_SRS,
            ue->frame_parms.N_RB_UL,
            nb_rb_srs,
            tx_amp);

      uint16_t nsymb = (ue->frame_parms.Ncp==0) ? 14:12;
      uint16_t symbol_offset = (int)ue->frame_parms.ofdm_symbol_size*((nr_tti_tx*nsymb)+(nsymb-1));
      generate_srs(&ue->frame_parms,
		   &ue->soundingrs_ul_config_dedicated[eNB_id],
		   &ue->common_vars.txdataF[eNB_id][symbol_offset],
		   tx_amp,
		   nr_tti_tx);
    }
}

int16_t get_pucch2_cqi(PHY_VARS_NR_UE *ue,int eNB_id,int *len) {

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
  }
  else { // Mode 1-1 feedback, later
    //LOG_I(PHY,"compute CQI value, TM %d, length 0, Cqi Avg 0 \n", ue->transmission_mode[eNB_id]);
    *len=0;
    // 2-antenna ports RI=1, 6 bits (2 PMI, 4 CQI)

    // 2-antenna ports RI=2, 8 bits (1 PMI, 7 CQI/DIFF CQI)
    return(0);
  }
}


int16_t get_pucch2_ri(PHY_VARS_NR_UE *ue,int eNB_id) {

  return(1);
}


void get_pucch_param(PHY_VARS_NR_UE    *ue,
                     UE_nr_rxtx_proc_t *proc,
                     uint8_t        *ack_payload,
                     PUCCH_FMT_t    format,
                     uint8_t        eNB_id,
                     uint8_t        SR,
                     uint8_t        cqi_report,
                     uint16_t       *pucch_resource,
                     uint8_t        *pucch_payload,
                     uint16_t       *plength)
{

  switch (format) {
  case pucch_format1:
    {
      pucch_resource[0] = ue->scheduling_request_config[eNB_id].sr_PUCCH_ResourceIndex;
      pucch_payload[0]  = 0; // payload is ignored in case of format1
      pucch_payload[1]  = 0; // payload is ignored in case of format1
    }
    break;

  case pucch_format1a:
  case pucch_format1b:
    {
      pucch_resource[0] = nr_get_n1_pucch(ue,
					  proc,
					  ue->dlsch[ue->current_thread_id[proc->nr_tti_rx]][eNB_id][0]->harq_ack,
					  eNB_id,
					  ack_payload,
					  SR);
      pucch_payload[0]  = ack_payload[0];
      pucch_payload[1]  = ack_payload[1];
      //pucch_payload[1]  = 1;
    }
    break;

  case pucch_format2:
    {
      pucch_resource[0]    = ue->cqi_report_config[eNB_id].CQI_ReportPeriodic.cqi_PUCCH_ResourceIndex;
      if(cqi_report)
        {
	  pucch_payload[0] = get_pucch2_cqi(ue,eNB_id,(int*)plength);
        }
      else
        {
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

#ifdef NR_PUCCH_SCHED
void ue_nr_pucch_procedures(PHY_VARS_NR_UE *ue,UE_nr_rxtx_proc_t *proc,uint8_t eNB_id,uint8_t abstraction_flag) {
}
#endif

void ue_pucch_procedures(PHY_VARS_NR_UE *ue,UE_nr_rxtx_proc_t *proc,uint8_t eNB_id,uint8_t abstraction_flag) {


  uint8_t  pucch_ack_payload[2];
  uint16_t pucch_resource;
  ANFBmode_t bundling_flag;
  PUCCH_FMT_t format;

  uint8_t  SR_payload;
  uint8_t  pucch_payload[2];
  uint16_t len;

  NR_DL_FRAME_PARMS *frame_parms = &ue->frame_parms;
  int frame_tx=proc->frame_tx;
  int nr_tti_tx=proc->nr_tti_tx;
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
    specific SRS nr_tti_rx even if the UE does not transmit SRS in that nr_tti_rx
  */

  int harq_pid = nr_subframe2harq_pid(&ue->frame_parms,
				      frame_tx,
				      nr_tti_tx);

  if(ue->ulsch[eNB_id]->harq_processes[harq_pid]->subframe_scheduling_flag)
    {
      LOG_D(PHY,"PUSCH is programmed on this nr_tti_rx [pid %d] AbsSuframe %d.%d ==> Skip PUCCH transmission \n",harq_pid,frame_tx,nr_tti_tx);
      VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_UE_TX_PUCCH,VCD_FUNCTION_OUT);
      return;
    }

  uint8_t isShortenPucch = (pSoundingrs_ul_config_dedicated->srsCellSubframe && frame_parms->soundingrs_ul_config_common.ackNackSRS_SimultaneousTransmission);

  bundling_flag = ue->pucch_config_dedicated[eNB_id].tdd_AckNackFeedbackMode;

  if ((frame_parms->frame_type==FDD) ||
      (bundling_flag==bundling)    ||
      ((frame_parms->frame_type==TDD)&&(frame_parms->tdd_config==1)&&((nr_tti_tx!=2)||(nr_tti_tx!=7)))) {
    format = pucch_format1a;
    LOG_D(PHY,"[UE] PUCCH 1a\n");
  } else {
    format = pucch_format1b;
    LOG_D(PHY,"[UE] PUCCH 1b\n");
  }

  // Part - I
  // Collect feedback that should be transmitted at this nr_tti_rx
  // - SR
  // - ACK/NACK
  // - CQI
  // - RI

  SR_payload = 0;
  if (nr_is_SR_TXOp(ue,proc,eNB_id)==1)
    {
      if (ue->mac_enabled==1) {
	SR_payload = mac_xface->ue_get_SR(Mod_id,
					  CC_id,
					  frame_tx,
					  eNB_id,
					  ue->pdcch_vars[ue->current_thread_id[proc->nr_tti_rx]][eNB_id]->crnti,
					  nr_tti_tx); // nr_tti_rx used for meas gap
      }
      else {
	SR_payload = 1;
      }
    }

  ack_status_cw0 = nr_get_ack(&ue->frame_parms,
			      ue->dlsch[ue->current_thread_id[proc->nr_tti_rx]][eNB_id][0]->harq_ack,
			      nr_tti_tx,
			      proc->nr_tti_rx,
			      pucch_ack_payload,
			      0);

  ack_status_cw1 = nr_get_ack(&ue->frame_parms,
			      ue->dlsch[ue->current_thread_id[proc->nr_tti_rx]][eNB_id][1]->harq_ack,
			      nr_tti_tx,
			      proc->nr_tti_rx,
			      pucch_ack_payload,
			      1);

  nb_cw = ( (ack_status_cw0 != 0) ? 1:0) + ( (ack_status_cw1 != 0) ? 1:0);

  cqi_status = ((ue->cqi_report_config[eNB_id].CQI_ReportPeriodic.cqi_PMI_ConfigIndex>0)&&
		(nr_is_cqi_TXOp(ue,proc,eNB_id)==1));

  ri_status = ((ue->cqi_report_config[eNB_id].CQI_ReportPeriodic.ri_ConfigIndex>0) &&
	       (nr_is_ri_TXOp(ue,proc,eNB_id)==1));

  // Part - II
  // if nothing to report ==> exit function
  if( (nb_cw==0) && (SR_payload==0) && (cqi_status==0) && (ri_status==0) )
    {
      LOG_D(PHY,"PUCCH No feedback AbsSubframe %d.%d SR_payload %d nb_cw %d pucch_ack_payload[0] %d pucch_ack_payload[1] %d cqi_status %d Return \n",
            frame_tx%1024, nr_tti_tx, SR_payload, nb_cw, pucch_ack_payload[0], pucch_ack_payload[1], cqi_status);
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
	frame_tx%1024, nr_tti_tx, SR_payload, nb_cw, ack_status_cw0, ack_status_cw1, pucch_ack_payload[0], pucch_ack_payload[1], cqi_status, ri_status, format, pucch_resource,pucch_payload[0],pucch_payload[1]);

  // Part - IV
  // Generate PUCCH signal
  ue->generate_ul_signal[eNB_id] = 1;

  switch (format) {
  case pucch_format1:
  case pucch_format1a:
  case pucch_format1b:
    {
      if (ue->mac_enabled == 1) {
	Po_PUCCH = nr_pucch_power_cntl(ue,proc,nr_tti_tx,eNB_id,format);
      }
      else {
	Po_PUCCH = ue->tx_power_max_dBm;
      }
      ue->tx_power_dBm[nr_tti_tx] = Po_PUCCH;
      ue->tx_total_RE[nr_tti_tx] = 12;

#if defined(EXMIMO) || defined(OAI_USRP) || defined(OAI_BLADERF) || defined(OAI_LMSSDR) || defined(OAI_ADRV9371_ZC706)
      tx_amp = nr_get_tx_amp(Po_PUCCH,
			  ue->tx_power_max_dBm,
			  ue->frame_parms.N_RB_UL,
			  1);
#else
      tx_amp = AMP;
#endif
#if T_TRACER
      T(T_UE_PHY_PUCCH_TX_POWER, T_INT(eNB_id),T_INT(Mod_id), T_INT(frame_tx%1024), T_INT(nr_tti_tx),T_INT(ue->tx_power_dBm[nr_tti_tx]),
	T_INT(tx_amp),T_INT(ue->dlsch[ue->current_thread_id[proc->nr_tti_rx]][eNB_id][0]->g_pucch),T_INT(get_nr_PL(ue->Mod_id,ue->CC_id,eNB_id)));
#endif

#ifdef UE_DEBUG_TRACE
      if(format == pucch_format1)
	{
          LOG_I(PHY,"[UE  %d][SR %x] AbsSubframe %d.%d Generating PUCCH 1 (SR for PUSCH), an_srs_simultanous %d, shorten_pucch %d, n1_pucch %d, Po_PUCCH %d\n",
		Mod_id,
		ue->dlsch[ue->current_thread_id[proc->nr_tti_rx]][eNB_id][0]->rnti,
		frame_tx%1024, nr_tti_tx,
		frame_parms->soundingrs_ul_config_common.ackNackSRS_SimultaneousTransmission,
		isShortenPucch,
		ue->scheduling_request_config[eNB_id].sr_PUCCH_ResourceIndex,
		Po_PUCCH);
	}
      else
	{
          if (SR_payload>0) {
	    LOG_I(PHY,"[UE  %d][SR %x] AbsSubFrame %d.%d Generating PUCCH %s payload %d,%d (with SR for PUSCH), an_srs_simultanous %d, shorten_pucch %d, n1_pucch %d, Po_PUCCH %d, amp %d\n",
		  Mod_id,
		  ue->dlsch[ue->current_thread_id[proc->nr_tti_rx]][eNB_id][0]->rnti,
		  frame_tx % 1024, nr_tti_tx,
		  (format == pucch_format1a? "1a": (
						    format == pucch_format1b? "1b" : "??")),
		  pucch_ack_payload[0],pucch_ack_payload[1],
		  frame_parms->soundingrs_ul_config_common.ackNackSRS_SimultaneousTransmission,
		  isShortenPucch,
		  pucch_resource,
		  Po_PUCCH,
		  tx_amp);
          } else {
	    LOG_I(PHY,"[UE  %d][PDSCH %x] AbsSubFrame %d.%d rx_offset_diff: %d, Generating PUCCH %s, an_srs_simultanous %d, shorten_pucch %d, n1_pucch %d, b[0]=%d,b[1]=%d (SR_Payload %d), Po_PUCCH %d, amp %d\n",
		  Mod_id,
		  ue->dlsch[ue->current_thread_id[proc->nr_tti_rx]][eNB_id][0]->rnti,
		  frame_tx%1024, nr_tti_tx,ue->rx_offset_diff,
		  (format == pucch_format1a? "1a": (
						    format == pucch_format1b? "1b" : "??")),
		  frame_parms->soundingrs_ul_config_common.ackNackSRS_SimultaneousTransmission,
		  isShortenPucch,
		  pucch_resource,pucch_payload[0],pucch_payload[1],SR_payload,
		  Po_PUCCH,
		  tx_amp);
          }
	}
#endif

#if T_TRACER
      if(pucch_payload[0])
	{
          T(T_UE_PHY_DLSCH_UE_ACK, T_INT(eNB_id), T_INT(frame_tx%1024), T_INT(nr_tti_tx), T_INT(Mod_id), T_INT(ue->dlsch[ue->current_thread_id[proc->nr_tti_rx]][eNB_id][0]->rnti),
	    T_INT(ue->dlsch[ue->current_thread_id[proc->nr_tti_rx]][eNB_id][0]->current_harq_pid));
	}
      else
	{
          T(T_UE_PHY_DLSCH_UE_NACK, T_INT(eNB_id), T_INT(frame_tx%1024), T_INT(nr_tti_tx), T_INT(Mod_id), T_INT(ue->dlsch[ue->current_thread_id[proc->nr_tti_rx]][eNB_id][0]->rnti),
	    T_INT(ue->dlsch[ue->current_thread_id[proc->nr_tti_rx]][eNB_id][0]->current_harq_pid));
	}
#endif

      if (abstraction_flag == 0) {

	generate_pucch1x(ue->common_vars.txdataF,
			 &ue->frame_parms,
			 ue->ncs_cell,
			 format,
			 &ue->pucch_config_dedicated[eNB_id],
			 pucch_resource,
			 isShortenPucch,  // shortened format
			 pucch_payload,
			 tx_amp,
			 nr_tti_tx);

      } else {
#ifdef PHY_ABSTRACTION
	LOG_D(PHY,"Calling generate_pucch_emul ... (ACK %d %d, SR %d)\n",pucch_ack_payload[0],pucch_ack_payload[1],SR_payload);
	generate_pucch_emul(ue,
			    proc,
			    format,
			    ue->frame_parms.pucch_config_common.nCS_AN,
			    pucch_payload,
			    SR_payload);
#endif
      }
    }
    break;


  case pucch_format2:
    {
      if (ue->mac_enabled == 1) {
	Po_PUCCH = nr_pucch_power_cntl(ue,proc,nr_tti_tx,eNB_id,format);
      }
      else {
	Po_PUCCH = ue->tx_power_max_dBm;
      }
      ue->tx_power_dBm[nr_tti_tx] = Po_PUCCH;
      ue->tx_total_RE[nr_tti_tx] = 12;

#if defined(EXMIMO) || defined(OAI_USRP) || defined(OAI_BLADERF) || defined(OAI_LMSSDR) || defined(OAI_ADRV9371_ZC706)
      tx_amp =  nr_get_tx_amp(Po_PUCCH,
			   ue->tx_power_max_dBm,
			   ue->frame_parms.N_RB_UL,
			   1);
#else
      tx_amp = AMP;
#endif
#if T_TRACER
      T(T_UE_PHY_PUCCH_TX_POWER, T_INT(eNB_id),T_INT(Mod_id), T_INT(frame_tx%1024), T_INT(nr_tti_tx),T_INT(ue->tx_power_dBm[nr_tti_tx]),
	T_INT(tx_amp),T_INT(ue->dlsch[ue->current_thread_id[proc->nr_tti_rx]][eNB_id][0]->g_pucch),T_INT(get_nr_PL(ue->Mod_id,ue->CC_id,eNB_id)));
#endif
#ifdef UE_DEBUG_TRACE
      LOG_I(PHY,"[UE  %d][RNTI %x] AbsSubFrame %d.%d Generating PUCCH 2 (RI or CQI), Po_PUCCH %d, isShortenPucch %d, amp %d\n",
	    Mod_id,
	    ue->dlsch[ue->current_thread_id[proc->nr_tti_rx]][eNB_id][0]->rnti,
	    frame_tx%1024, nr_tti_tx,
	    Po_PUCCH,
	    isShortenPucch,
	    tx_amp);
#endif
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
		       nr_tti_tx,
		       ue->pdcch_vars[ue->current_thread_id[proc->nr_tti_rx]][eNB_id]->crnti);
    }
    break;

  case pucch_format2a:
    LOG_D(PHY,"[UE  %d][RNTI %x] AbsSubFrame %d.%d Generating PUCCH 2a (RI or CQI) Ack/Nack 1bit \n",
	  Mod_id,
	  ue->dlsch[ue->current_thread_id[proc->nr_tti_rx]][eNB_id][0]->rnti,
	  frame_tx%1024, nr_tti_tx);
    break;
  case pucch_format2b:
    LOG_D(PHY,"[UE  %d][RNTI %x] AbsSubFrame %d.%d Generating PUCCH 2b (RI or CQI) Ack/Nack 2bits\n",
	  Mod_id,
	  ue->dlsch[ue->current_thread_id[proc->nr_tti_rx]][eNB_id][0]->rnti,
	  frame_tx%1024, nr_tti_tx);
    break;
  default:
    break;
  }

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_UE_TX_PUCCH,VCD_FUNCTION_OUT);

}

#endif


void phy_procedures_nrUE_TX(PHY_VARS_NR_UE *ue,
                            UE_nr_rxtx_proc_t *proc,
                            uint8_t gNB_id,
                            uint8_t thread_id)
{
  //int32_t ulsch_start=0;
  int slot_tx = proc->nr_tti_tx;
  int frame_tx = proc->frame_tx;

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_UE_TX,VCD_FUNCTION_IN);

  LOG_I(PHY,"****** start TX-Chain for AbsSubframe %d.%d ******\n", frame_tx, slot_tx);

#if UE_TIMING_TRACE
  start_meas(&ue->phy_proc_tx);
#endif

  uint8_t harq_pid = 0; //temporary implementation

  nr_ue_ulsch_procedures(ue,
                         harq_pid,
                         frame_tx,
                         slot_tx,
                         thread_id,
                         gNB_id);


/*
  if (ue->UE_mode[eNB_id] == PUSCH) {
    // check if we need to use PUCCH 1a/1b
    ue_pucch_procedures(ue,proc,eNB_id,abstraction_flag);
    // check if we need to use SRS
    ue_srs_procedures(ue,proc,eNB_id,abstraction_flag);
  } // UE_mode==PUSCH
*/

	  LOG_D(PHY, "Sending data \n");
	  nr_ue_pusch_common_procedures(ue,
                                harq_pid,
                                slot_tx,
                                thread_id,
                                gNB_id,
                                &ue->frame_parms);

  //LOG_M("txdata.m","txs",ue->common_vars.txdata[0],1228800,1,1);


/*
  if ((ue->UE_mode[eNB_id] == PRACH) &&
      (ue->frame_parms.prach_config_common.prach_Config_enabled==1)) {

    // check if we have PRACH opportunity

    if (is_prach_subframe(&ue->frame_parms,frame_tx,nr_tti_tx)) {

      ue_prach_procedures(ue,proc,eNB_id,abstraction_flag,mode);
    }
  } // mode is PRACH
  else {
    ue->generate_prach=0;
  }
*/

  LOG_I(PHY,"****** end TX-Chain for AbsSubframe %d.%d ******\n", frame_tx, slot_tx);

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_UE_TX, VCD_FUNCTION_OUT);
#if UE_TIMING_TRACE
  stop_meas(&ue->phy_proc_tx);
#endif

}


void nr_ue_prach_procedures(PHY_VARS_NR_UE *ue,UE_nr_rxtx_proc_t *proc,uint8_t eNB_id,uint8_t abstraction_flag,runmode_t mode) {

  int frame_tx = proc->frame_tx;
  int nr_tti_tx = proc->nr_tti_tx;
  int prach_power;
  uint16_t preamble_tx=50;
  PRACH_RESOURCES_t prach_resources;

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_UE_TX_PRACH, VCD_FUNCTION_IN);

  ue->generate_nr_prach=0;
 if (ue->mac_enabled==0){
    ue->prach_resources[eNB_id] = &prach_resources;
 ue->prach_resources[eNB_id]->ra_PreambleIndex = preamble_tx;
  ue->prach_resources[eNB_id]->ra_TDD_map_index = 0;
 }

  if (ue->mac_enabled==1){

    // ask L2 for RACH transport
    if ((mode != rx_calib_ue) && (mode != rx_calib_ue_med) && (mode != rx_calib_ue_byp) && (mode != no_L2_connect) ) {
      LOG_D(PHY,"Getting PRACH resources\n");
      //ue->prach_resources[eNB_id] = mac_xface->ue_get_rach(ue->Mod_id,ue->CC_id,frame_tx,eNB_id,nr_tti_tx);   
   // LOG_D(PHY,"Got prach_resources for eNB %d address %p, RRCCommon %p\n",eNB_id,ue->prach_resources[eNB_id],UE_mac_inst[ue->Mod_id].radioResourceConfigCommon);  
   // LOG_D(PHY,"Prach resources %p\n",ue->prach_resources[eNB_id]);
  }
}

if (ue->prach_resources[eNB_id]!=NULL) {

  ue->generate_nr_prach=1;
  ue->prach_cnt=0;
#ifdef SMBV
ue->prach_resources[eNB_id]->ra_PreambleIndex = preamble_tx;
#endif

#ifdef OAI_EMU
  ue->prach_PreambleIndex=ue->prach_resources[eNB_id]->ra_PreambleIndex;
#endif

  if (abstraction_flag == 0) {

    LOG_I(PHY,"mode %d\n",mode);

    if ((ue->mac_enabled==1) && (mode != calib_prach_tx)) {

      ue->tx_power_dBm[nr_tti_tx] = ue->prach_resources[eNB_id]->ra_PREAMBLE_RECEIVED_TARGET_POWER+get_nr_PL(ue,eNB_id);
    }
    else {
      ue->tx_power_dBm[nr_tti_tx] = ue->tx_power_max_dBm;
      ue->prach_resources[eNB_id]->ra_PreambleIndex = preamble_tx; 
    }

   LOG_I(PHY,"[UE  %d][RAPROC] Frame %d, nr_tti_rx %d : Generating PRACH, preamble %d,PL %d,  P0_PRACH %d, TARGET_RECEIVED_POWER %d dBm, PRACH TDD Resource index %d, RA-RNTI %d\n",
	  ue->Mod_id,
	  frame_tx,
	  nr_tti_tx,
	  ue->prach_resources[eNB_id]->ra_PreambleIndex,
	  get_nr_PL(ue,eNB_id),
	  ue->tx_power_dBm[nr_tti_tx],
	  ue->prach_resources[eNB_id]->ra_PREAMBLE_RECEIVED_TARGET_POWER,
	  ue->prach_resources[eNB_id]->ra_TDD_map_index,
	  ue->prach_resources[eNB_id]->ra_RNTI);

    ue->tx_total_RE[nr_tti_tx] = 96;

#if defined(EXMIMO) || defined(OAI_USRP) || defined(OAI_BLADERF) || defined(OAI_LMSSDR) || defined(OAI_ADRV9371_ZC706)
    ue->prach_vars[eNB_id]->amp = nr_get_tx_amp(ue->tx_power_dBm[nr_tti_tx],
					     ue->tx_power_max_dBm,
					     ue->frame_parms.N_RB_UL,
					     6);
#else
   ue->prach_vars[eNB_id]->amp = AMP;
#endif
   if ((mode == calib_prach_tx) && (((proc->frame_tx&0xfffe)%100)==0))
      LOG_D(PHY,"[UE  %d][RAPROC] Frame %d, nr_tti_rx %d : PRACH TX power %d dBm, amp %d\n",
	    ue->Mod_id,
	    proc->frame_rx,
	    proc->nr_tti_tx,
	    ue->tx_power_dBm[nr_tti_tx],
	    ue->prach_vars[eNB_id]->amp);


   //       start_meas(&ue->tx_prach);
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_GENERATE_PRACH, VCD_FUNCTION_IN);

//    prach_power = generate_nr_prach(ue,eNB_id,nr_tti_tx,frame_tx);
prach_power = generate_nr_prach(ue,0,9,0); //subframe number hardcoded according to the simulator

    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_GENERATE_PRACH, VCD_FUNCTION_OUT);
    //      stop_meas(&ue->tx_prach);
    LOG_D(PHY,"[UE  %d][RAPROC] PRACH PL %d dB, power %d dBm, digital power %d dB (amp %d)\n",
	  ue->Mod_id,
	  get_nr_PL(ue,eNB_id),
	  ue->tx_power_dBm[nr_tti_tx],
	  dB_fixed(prach_power),
	  ue->prach_vars[eNB_id]->amp);
  }/* else {
    UE_transport_info[ue->Mod_id][ue->CC_id].cntl.prach_flag=1;
    UE_transport_info[ue->Mod_id][ue->CC_id].cntl.prach_id=ue->prach_resources[eNB_id]->ra_PreambleIndex;
  }*/ // commented for compiling as abstraction flag is 0

  if (ue->mac_enabled==1){
    //mac_xface->Msg1_transmitted(ue->Mod_id,ue->CC_id,frame_tx,eNB_id);
 }

LOG_I(PHY,"[UE  %d][RAPROC] Frame %d, nr_tti_rx %d: Generating PRACH (eNB %d) preamble index %d for UL, TX power %d dBm (PL %d dB), l3msg \n",
      ue->Mod_id,frame_tx,nr_tti_tx,eNB_id,
      ue->prach_resources[eNB_id]->ra_PreambleIndex,
      ue->prach_resources[eNB_id]->ra_PREAMBLE_RECEIVED_TARGET_POWER+get_nr_PL(ue,eNB_id),
      get_nr_PL(ue,eNB_id));

}


// if we're calibrating the PRACH kill the pointer to its resources so that the RA protocol doesn't continue
if (mode == calib_prach_tx)
  ue->prach_resources[eNB_id]=NULL;

LOG_D(PHY,"[UE %d] frame %d nr_tti_rx %d : generate_nr_prach %d, prach_cnt %d\n",
      ue->Mod_id,frame_tx,nr_tti_tx,ue->generate_nr_prach,ue->prach_cnt);

ue->prach_cnt++;

if (ue->prach_cnt==3)
  ue->generate_nr_prach=0;

VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_UE_TX_PRACH, VCD_FUNCTION_OUT);
}

/*
void phy_procedures_UE_S_TX(PHY_VARS_NR_UE *ue,uint8_t eNB_id,uint8_t abstraction_flag,relaying_type_t r_type)
{
  int aa;//i,aa;
  NR_DL_FRAME_PARMS *frame_parms=&ue->frame_parms;

  if (abstraction_flag==0) {

    for (aa=0; aa<frame_parms->nb_antennas_tx; aa++) {
#if defined(EXMIMO) //this is the EXPRESS MIMO case
      int i;
      // set the whole tx buffer to RX
      for (i=0; i<LTE_NUMBER_OF_SUBFRAMES_PER_FRAME*frame_parms->samples_per_subframe; i++)
	ue->common_vars.txdata[aa][i] = 0x00010001;

#else //this is the normal case
      memset(&ue->common_vars.txdata[aa][0],0,
	     (LTE_NUMBER_OF_SUBFRAMES_PER_FRAME*frame_parms->samples_per_subframe)*sizeof(int32_t));
#endif //else EXMIMO

    }
  }
}

*/

void nr_ue_measurement_procedures(uint16_t l,    // symbol index of each slot [0..6]
								  PHY_VARS_NR_UE *ue,
								  UE_nr_rxtx_proc_t *proc,
								  uint8_t eNB_id,
								  uint16_t slot, // slot index of each radio frame [0..19]
								  runmode_t mode)
{
  LOG_D(PHY,"ue_measurement_procedures l %u Ncp %d\n",l,ue->frame_parms.Ncp);

  NR_DL_FRAME_PARMS *frame_parms=&ue->frame_parms;
  int nr_tti_rx = proc->nr_tti_rx;
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_MEASUREMENT_PROCEDURES, VCD_FUNCTION_IN);

  if (l==2) {
    // UE measurements on symbol 0
      LOG_D(PHY,"Calling measurements nr_tti_rx %d, rxdata %p\n",nr_tti_rx,ue->common_vars.rxdata);
/*
      nr_ue_measurements(ue,
			  0,
			  0,
			  0,
			  0,
			  nr_tti_rx);
*/			  
			  //(nr_tti_rx*frame_parms->samples_per_tti+ue->rx_offset)%(frame_parms->samples_per_tti*LTE_NUMBER_OF_SUBFRAMES_PER_FRAME)

#if T_TRACER
    if(slot == 0)
      T(T_UE_PHY_MEAS, T_INT(eNB_id),  T_INT(ue->Mod_id), T_INT(proc->frame_rx%1024), T_INT(proc->nr_tti_rx),
	T_INT((int)(10*log10(ue->measurements.rsrp[0])-ue->rx_total_gain_dB)),
	T_INT((int)ue->measurements.rx_rssi_dBm[0]),
	T_INT((int)(ue->measurements.rx_power_avg_dB[0] - ue->measurements.n0_power_avg_dB)),
	T_INT((int)ue->measurements.rx_power_avg_dB[0]),
	T_INT((int)ue->measurements.n0_power_avg_dB),
	T_INT((int)ue->measurements.wideband_cqi_avg[0]),
	T_INT((int)ue->common_vars.freq_offset));
#endif
  }
#if 0
  if (l==(6-ue->frame_parms.Ncp)) {

    // make sure we have signal from PSS/SSS for N0 measurement
    // LOG_I(PHY," l==(6-ue->frame_parms.Ncp) ue_rrc_measurements\n");

    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_RRC_MEASUREMENTS, VCD_FUNCTION_IN);
    ue_rrc_measurements(ue,
			slot,
			abstraction_flag);
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_RRC_MEASUREMENTS, VCD_FUNCTION_OUT);


  }
#endif

  // accumulate and filter timing offset estimation every subframe (instead of every frame)
  if (( slot == 2) && (l==(2-frame_parms->Ncp))) {

    // AGC

    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_GAIN_CONTROL, VCD_FUNCTION_IN);


    //printf("start adjust gain power avg db %d\n", ue->measurements.rx_power_avg_dB[eNB_id]);
    phy_adjust_gain_nr (ue,ue->measurements.rx_power_avg_dB[eNB_id],eNB_id);
    
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_GAIN_CONTROL, VCD_FUNCTION_OUT);

}

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_MEASUREMENT_PROCEDURES, VCD_FUNCTION_OUT);
}



#if 0
void restart_phy(PHY_VARS_NR_UE *ue,UE_nr_rxtx_proc_t *proc, uint8_t eNB_id,uint8_t abstraction_flag)
{

  //  uint8_t last_slot;
  uint8_t i;
  LOG_I(PHY,"[UE  %d] frame %d, slot %d, restarting PHY!\n",ue->Mod_id,proc->frame_rx,proc->nr_tti_rx);
  //mac_xface->macphy_exit("restart_phy called");
  //   first_run = 1;

  if (abstraction_flag ==0 ) {
    ue->UE_mode[eNB_id] = NOT_SYNCHED;
  } else {
    ue->UE_mode[eNB_id] = PRACH;
    ue->prach_resources[eNB_id]=NULL;
  }

  proc->frame_rx = -1;
  proc->frame_tx = -1;
  //  ue->synch_wait_cnt=0;
  //  ue->sched_cnt=-1;

  ue->pbch_vars[eNB_id]->pdu_errors_conseq=0;
  ue->pbch_vars[eNB_id]->pdu_errors=0;

  ue->pdcch_vars[0][eNB_id]->dci_errors = 0;
  ue->pdcch_vars[0][eNB_id]->dci_missed = 0;
  ue->pdcch_vars[0][eNB_id]->dci_false  = 0;
  ue->pdcch_vars[0][eNB_id]->dci_received = 0;

  ue->pdcch_vars[1][eNB_id]->dci_errors = 0;
  ue->pdcch_vars[1][eNB_id]->dci_missed = 0;
  ue->pdcch_vars[1][eNB_id]->dci_false  = 0;
  ue->pdcch_vars[1][eNB_id]->dci_received = 0;

  ue->dlsch_errors[eNB_id] = 0;
  ue->dlsch_errors_last[eNB_id] = 0;
  ue->dlsch_received[eNB_id] = 0;
  ue->dlsch_received_last[eNB_id] = 0;
  ue->dlsch_fer[eNB_id] = 0;
  ue->dlsch_SI_received[eNB_id] = 0;
  ue->dlsch_ra_received[eNB_id] = 0;
  ue->dlsch_p_received[eNB_id] = 0;
  ue->dlsch_SI_errors[eNB_id] = 0;
  ue->dlsch_ra_errors[eNB_id] = 0;
  ue->dlsch_p_errors[eNB_id] = 0;

  ue->dlsch_mch_received[eNB_id] = 0;

  for (i=0; i < MAX_MBSFN_AREA ; i ++) {
    ue->dlsch_mch_received_sf[i][eNB_id] = 0;
    ue->dlsch_mcch_received[i][eNB_id] = 0;
    ue->dlsch_mtch_received[i][eNB_id] = 0;
    ue->dlsch_mcch_errors[i][eNB_id] = 0;
    ue->dlsch_mtch_errors[i][eNB_id] = 0;
    ue->dlsch_mcch_trials[i][eNB_id] = 0;
    ue->dlsch_mtch_trials[i][eNB_id] = 0;
  }

  //ue->total_TBS[eNB_id] = 0;
  //ue->total_TBS_last[eNB_id] = 0;
  //ue->bitrate[eNB_id] = 0;
  //ue->total_received_bits[eNB_id] = 0;
}
#endif //(0)

void nr_ue_pbch_procedures(uint8_t eNB_id,
			   PHY_VARS_NR_UE *ue,
			   UE_nr_rxtx_proc_t *proc,
			   uint8_t abstraction_flag)
{
  //  int i;
  //int pbch_tx_ant=0;
  //uint8_t pbch_phase;
  int ret = 0;
  //static uint8_t first_run = 1;
  //uint8_t pbch_trials = 0;

  DevAssert(ue);

  int frame_rx = proc->frame_rx;
  int nr_tti_rx = proc->nr_tti_rx;

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_PBCH_PROCEDURES, VCD_FUNCTION_IN);

  //LOG_I(PHY,"[UE  %d] Frame %d, Trying PBCH %d (NidCell %d, eNB_id %d)\n",ue->Mod_id,frame_rx,pbch_phase,ue->frame_parms.Nid_cell,eNB_id);

  ret = nr_rx_pbch(ue, proc,
		   ue->pbch_vars[eNB_id],
		   &ue->frame_parms,
		   eNB_id,
		   (ue->frame_parms.ssb_index)&7,
		   SISO,
		   ue->high_speed_flag);

  if (ret==0) {

    ue->pbch_vars[eNB_id]->pdu_errors_conseq = 0;


#ifdef DEBUG_PHY_PROC
    uint16_t frame_tx;
    LOG_D(PHY,"[UE %d] frame %d, nr_tti_rx %d, Received PBCH (MIB): frame_tx %d. N_RB_DL %d\n",
    ue->Mod_id,
    frame_rx,
    nr_tti_rx,
    frame_tx,
    ue->frame_parms.N_RB_DL);
#endif

  } else {
    LOG_E(PHY,"[UE %d] frame %d, nr_tti_rx %d, Error decoding PBCH!\n",
	  ue->Mod_id,frame_rx, nr_tti_rx);

    /*
    write_output("rxsig0.m","rxs0", ue->common_vars.rxdata[0],ue->frame_parms.samples_per_subframe,1,1);


      write_output("H00.m","h00",&(ue->common_vars.dl_ch_estimates[0][0][0]),((ue->frame_parms.Ncp==0)?7:6)*(ue->frame_parms.ofdm_symbol_size),1,1);
      write_output("H10.m","h10",&(ue->common_vars.dl_ch_estimates[0][2][0]),((ue->frame_parms.Ncp==0)?7:6)*(ue->frame_parms.ofdm_symbol_size),1,1);

      write_output("rxsigF0.m","rxsF0", ue->common_vars.rxdataF[0],8*ue->frame_parms.ofdm_symbol_size,1,1);
      write_output("PBCH_rxF0_ext.m","pbch0_ext",ue->pbch_vars[0]->rxdataF_ext[0],12*4*6,1,1);
      write_output("PBCH_rxF0_comp.m","pbch0_comp",ue->pbch_vars[0]->rxdataF_comp[0],12*4*6,1,1);
      write_output("PBCH_rxF_llr.m","pbch_llr",ue->pbch_vars[0]->llr,(ue->frame_parms.Ncp==0) ? 1920 : 1728,1,4);
      exit(-1);
    */

    ue->pbch_vars[eNB_id]->pdu_errors_conseq++;
    ue->pbch_vars[eNB_id]->pdu_errors++;

    if (ue->pbch_vars[eNB_id]->pdu_errors_conseq>=100) {
      LOG_E(PHY,"More that 100 consecutive PBCH errors! Exiting!\n");
      exit_fun("More that 100 consecutive PBCH errors! Exiting!\n");
    }
  }

  if (frame_rx % 100 == 0) {
    ue->pbch_vars[eNB_id]->pdu_fer = ue->pbch_vars[eNB_id]->pdu_errors - ue->pbch_vars[eNB_id]->pdu_errors_last;
    ue->pbch_vars[eNB_id]->pdu_errors_last = ue->pbch_vars[eNB_id]->pdu_errors;
  }

#ifdef DEBUG_PHY_PROC
  LOG_D(PHY,"[UE %d] frame %d, slot %d, PBCH errors = %d, consecutive errors = %d!\n",
	ue->Mod_id,frame_rx, nr_tti_rx,
	ue->pbch_vars[eNB_id]->pdu_errors,
	ue->pbch_vars[eNB_id]->pdu_errors_conseq);
#endif
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_PBCH_PROCEDURES, VCD_FUNCTION_OUT);
}



unsigned int nr_get_tx_amp(int power_dBm, int power_max_dBm, int N_RB_UL, int nb_rb)
{

  int gain_dB = power_dBm - power_max_dBm;
  double gain_lin;

  gain_lin = pow(10,.1*gain_dB);
  if ((nb_rb >0) && (nb_rb <= N_RB_UL)) {
    return((int)(AMP*sqrt(gain_lin*N_RB_UL/(double)nb_rb)));
  }
  else {
    LOG_E(PHY,"Illegal nb_rb/N_RB_UL combination (%d/%d)\n",nb_rb,N_RB_UL);
    //mac_xface->macphy_exit("");
  }
  return(0);
}

#ifdef NR_PDCCH_SCHED

int nr_ue_pdcch_procedures(uint8_t gNB_id,
			   PHY_VARS_NR_UE *ue,
			   UE_nr_rxtx_proc_t *proc)
{
  int frame_rx = proc->frame_rx;
  int nr_tti_rx = proc->nr_tti_rx;
  unsigned int dci_cnt=0;

  /*
  //  unsigned int dci_cnt=0, i;  //removed for nr_ue_pdcch_procedures and added in the loop for nb_coreset_active
#ifdef NR_PDCCH_SCHED_DEBUG
  printf("<-NR_PDCCH_PHY_PROCEDURES_LTE_UE (nr_ue_pdcch_procedures)-> Entering function nr_ue_pdcch_procedures() \n");
#endif

  int frame_rx = proc->frame_rx;
  int nr_tti_rx = proc->nr_tti_rx;
  NR_DCI_ALLOC_t dci_alloc_rx[8];
  
  //uint8_t next1_thread_id = ue->current_thread_id[nr_tti_rx]== (RX_NB_TH-1) ? 0:(ue->current_thread_id[nr_tti_rx]+1);
  //uint8_t next2_thread_id = next1_thread_id== (RX_NB_TH-1) ? 0:(next1_thread_id+1);
  

  // table dci_fields_sizes_cnt contains dci_fields_sizes for each time a dci is decoded in the slot
  // each element represents the size in bits for each dci field, for each decoded dci -> [dci_cnt-1]
  // each time a dci is decode at dci_cnt, the values of the table dci_fields_sizes[i][j] will be copied at table dci_fields_sizes_cnt[dci_cnt-1][i][j]
  // table dci_fields_sizes_cnt[dci_cnt-1][i][j] will then be used in function nr_extract_dci_info
  uint8_t dci_fields_sizes_cnt[MAX_NR_DCI_DECODED_SLOT][NBR_NR_DCI_FIELDS][NBR_NR_FORMATS];

  int nb_searchspace_active=0;
  NR_UE_PDCCH **pdcch_vars = ue->pdcch_vars[ue->current_thread_id[nr_tti_rx]];
  NR_UE_PDCCH *pdcch_vars2 = ue->pdcch_vars[ue->current_thread_id[nr_tti_rx]][gNB_id];
  // s in TS 38.212 Subclause 10.1, for each active BWP the UE can deal with 10 different search spaces
  // Higher layers have updated the number of searchSpaces with are active in the current slot and this value is stored in variable nb_searchspace_total
  int nb_searchspace_total = pdcch_vars2->nb_search_space;

  pdcch_vars[gNB_id]->crnti = 0x1234; //to be check how to set when using loop memory

  uint16_t c_rnti=pdcch_vars[gNB_id]->crnti;
  uint16_t cs_rnti=0,new_rnti=0,tc_rnti=0;
  uint16_t p_rnti=P_RNTI;
  uint16_t si_rnti=SI_RNTI;
  uint16_t ra_rnti=99;
  uint16_t sp_csi_rnti=0,sfi_rnti=0,int_rnti=0,tpc_pusch_rnti=0,tpc_pucch_rnti=0,tpc_srs_rnti=0; //FIXME
  uint16_t crc_scrambled_values[TOTAL_NBR_SCRAMBLED_VALUES] =
    {c_rnti,cs_rnti,new_rnti,tc_rnti,p_rnti,si_rnti,ra_rnti,sp_csi_rnti,sfi_rnti,int_rnti,tpc_pusch_rnti,tpc_pucch_rnti,tpc_srs_rnti};
  #ifdef NR_PDCCH_SCHED_DEBUG
  printf("<-NR_PDCCH_PHY_PROCEDURES_LTE_UE (nr_ue_pdcch_procedures)-> there is a bug in FAPI to calculate nb_searchspace_total=%d\n",nb_searchspace_total);
  #endif
  if (nb_searchspace_total>1) nb_searchspace_total=1; // to be removed when fixing bug in FAPI
  #ifdef NR_PDCCH_SCHED_DEBUG
  printf("<-NR_PDCCH_PHY_PROCEDURES_LTE_UE (nr_ue_pdcch_procedures)-> there is a bug in FAPI to calculate nb_searchspace_total so we set it to 1...\n");
  printf("<-NR_PDCCH_PHY_PROCEDURES_LTE_UE (nr_ue_pdcch_procedures)-> the number of searchSpaces active in the current slot(%d) is %d) \n",
	 nr_tti_rx,nb_searchspace_total);
  #endif

  //FK: we define dci_ind and dl_indication as local variables, this way the call to the mac should be thread safe
  fapi_nr_dci_indication_t dci_ind;
  nr_downlink_indication_t dl_indication;
  
  // p in TS 38.212 Subclause 10.1, for each active BWP the UE can deal with 3 different CORESETs (including coresetId 0 for common search space)
  //int nb_coreset_total = NR_NBR_CORESET_ACT_BWP;
  unsigned int dci_cnt=0;
  // this table contains 56 (NBR_NR_DCI_FIELDS) elements for each dci field and format described in TS 38.212. Each element represents the size in bits for each dci field
  //uint8_t dci_fields_sizes[NBR_NR_DCI_FIELDS][NBR_NR_FORMATS] = {{0}};
  // this is the UL bandwidth part. FIXME! To be defined where this value comes from
  //  uint16_t n_RB_ULBWP = 106;
  // this is the DL bandwidth part. FIXME! To be defined where this value comes from

  // First we have to identify each searchSpace active at a time and do PDCCH monitoring corresponding to current searchSpace
  // Up to 10 searchSpaces can be configured to UE (s<=10)
  for (nb_searchspace_active=0; nb_searchspace_active<nb_searchspace_total; nb_searchspace_active++){
    int nb_coreset_active=nb_searchspace_active;
    //int do_pdcch_monitoring_current_slot=1; // this variable can be removed and fapi is handling
    
     // The following code has been removed as it is handled by higher layers (fapi)
     //
     // Verify that monitoring is required at the slot nr_tti_rx. We will run pdcch procedure only if do_pdcch_monitoring_current_slot=1
     // For Type0-PDCCH searchspace, we need to calculate the monitoring slot from Tables 13-1 .. 13-15 in TS 38.213 Subsection 13
     //NR_UE_SLOT_PERIOD_OFFSET_t sl_period_offset_mon = pdcch_vars2->searchSpace[nb_searchspace_active].monitoringSlotPeriodicityAndOffset;
     //if (sl_period_offset_mon == nr_sl1) {
     //do_pdcch_monitoring_current_slot=1; // PDCCH monitoring in every slot
     //} else if (nr_tti_rx%(uint16_t)sl_period_offset_mon == pdcch_vars2->searchSpace[nb_searchspace_active].monitoringSlotPeriodicityAndOffset_offset) {
     //do_pdcch_monitoring_current_slot=1; // PDCCH monitoring in every monitoringSlotPeriodicityAndOffset slot with offset
     //}
    
     // FIXME
     // For PDCCH monitoring when overlap with SS/PBCH according to 38.213 v15.1.0 Section 10
     // To be implemented LATER !!!
     
    //int _offset,_index,_M;
    //int searchSpace_id                              = pdcch_vars2->searchSpace[nb_searchspace_active].searchSpaceId;


    #ifdef NR_PDCCH_SCHED_DEBUG
      printf("<-NR_PDCCH_PHY_PROCEDURES_LTE_UE (nr_ue_pdcch_procedures)-> nb_searchspace_active=%d do_pdcch_monitoring_current_slot=%d (to be removed)\n",
              nb_searchspace_active,
              do_pdcch_monitoring_current_slot);
    #endif

//    if (do_pdcch_monitoring_current_slot) {
      // the searchSpace indicates that we need to monitor PDCCH in current nr_tti_rx
      // get the parameters describing the current SEARCHSPACE
      // the CORESET id applicable to the current SearchSpace
      //int searchSpace_coreset_id                      = pdcch_vars2->searchSpace[nb_searchspace_active].controlResourceSetId;
      // FIXME this variable is a bit string (14 bits) identifying every OFDM symbol in a slot.
      // at the moment we will not take into consideration this variable and we will consider that the OFDM symbol offset is always the first OFDM in a symbol
      uint16_t symbol_within_slot_mon                 = pdcch_vars2->searchSpace[nb_searchspace_active].monitoringSymbolWithinSlot;
      // get the remaining parameters describing the current SEARCHSPACE:     // FIXME! To be defined where we get this information from
      //NR_UE_SEARCHSPACE_nbrCAND_t num_cand_L1         = pdcch_vars2->searchSpace[nb_searchspace_active].nrofCandidates_aggrlevel1;
      //NR_UE_SEARCHSPACE_nbrCAND_t num_cand_L2         = pdcch_vars2->searchSpace[nb_searchspace_active].nrofCandidates_aggrlevel2;
      //NR_UE_SEARCHSPACE_nbrCAND_t num_cand_L4         = pdcch_vars2->searchSpace[nb_searchspace_active].nrofCandidates_aggrlevel4;
      //NR_UE_SEARCHSPACE_nbrCAND_t num_cand_L8         = pdcch_vars2->searchSpace[nb_searchspace_active].nrofCandidates_aggrlevel8;
      //NR_UE_SEARCHSPACE_nbrCAND_t num_cand_L16        = pdcch_vars2->searchSpace[nb_searchspace_active].nrofCandidates_aggrlevel16;
                                                                                                  // FIXME! A table of five enum elements
      // searchSpaceType indicates whether this is a common search space or a UE-specific search space
      //int searchSpaceType                             = pdcch_vars2->searchSpace[nb_searchspace_active].searchSpaceType.type;
      NR_SEARCHSPACE_TYPE_t searchSpaceType                             = ue_specific;//common;
      #ifdef NR_PDCCH_SCHED_DEBUG
        printf("<-NR_PDCCH_PHY_PROCEDURES_LTE_UE (nr_ue_pdcch_procedures)-> searchSpaceType=%d is hardcoded THIS HAS TO BE FIXED!!!\n",
                searchSpaceType);
      #endif

      //while ((searchSpace_coreset_id != pdcch_vars2->coreset[nb_coreset_active].controlResourceSetId) && (nb_coreset_active<nb_coreset_total)) {
        // we need to identify the CORESET associated to the active searchSpace
        //nb_coreset_active++;
      if (nb_coreset_active >= nb_coreset_total) return 0; // the coreset_id could not be found. There is a problem
      }


    
     //we do not need these parameters yet
    
     // get the parameters describing the current CORESET
     //int coreset_duration                                      = pdcch_vars2->coreset[nb_coreset_active].duration;
     //uint64_t coreset_freq_dom                                 = pdcch_vars2->coreset[nb_coreset_active].frequencyDomainResources;
     //int coreset_shift_index                                   = pdcch_vars2->coreset[nb_coreset_active].cce_reg_mappingType.shiftIndex;
    // NR_UE_CORESET_REG_bundlesize_t coreset_bundlesize         = pdcch_vars2->coreset[nb_coreset_active].cce_reg_mappingType.reg_bundlesize;
    // NR_UE_CORESET_interleaversize_t coreset_interleaversize   = pdcch_vars2->coreset[nb_coreset_active].cce_reg_mappingType.interleaversize;
    // NR_UE_CORESET_precoder_granularity_t precoder_granularity = pdcch_vars2->coreset[nb_coreset_active].precoderGranularity;
    // int tci_statesPDCCH                                       = pdcch_vars2->coreset[nb_coreset_active].tciStatesPDCCH;
    // int tci_present                                           = pdcch_vars2->coreset[nb_coreset_active].tciPresentInDCI;
    // uint16_t pdcch_DMRS_scrambling_id                         = pdcch_vars2->coreset[nb_coreset_active].pdcchDMRSScramblingID;
    

    // A set of PDCCH candidates for a UE to monitor is defined in terms of PDCCH search spaces.
    // Searchspace types:
    // Type0-PDCCH  common search space for a DCI format with CRC scrambled by a SI-RNTI
    // number of consecutive resource blocks and a number of consecutive symbols for
    // the control resource set of the Type0-PDCCH common search space from
    // the four most significant bits of RMSI-PDCCH-Config as described in Tables 13-1 through 13-10
    // and determines PDCCH monitoring occasions
    // from the four least significant bits of RMSI-PDCCH-Config,
    // included in MasterInformationBlock, as described in Tables 13-11 through 13-15
    // Type0A-PDCCH common search space for a DCI format with CRC scrambled by a SI-RNTI
    // Type1-PDCCH  common search space for a DCI format with CRC scrambled by a RA-RNTI, or a TC-RNTI, or a C-RNTI
    // Type2-PDCCH  common search space for a DCI format with CRC scrambled by a P-RNTI
    // Type3-PDCCH  common search space for a DCI format with CRC scrambled by INT-RNTI, or SFI-RNTI,
    // or TPC-PUSCH-RNTI, or TPC-PUCCH-RNTI, or TPC-SRS-RNTI, or C-RNTI, or CS-RNTI(s), or SP-CSI-RNTI



    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_PDCCH_PROCEDURES, VCD_FUNCTION_IN);
#if UE_TIMING_TRACE
      start_meas(&ue->dlsch_rx_pdcch_stats);
#endif

      VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_RX_PDCCH, VCD_FUNCTION_IN);
#ifdef NR_PDCCH_SCHED_DEBUG
      printf("<-NR_PDCCH_PHY_PROCEDURES_LTE_UE (nr_ue_pdcch_procedures)-> Entering function nr_rx_pdcch with gNB_id=%d (nb_coreset_active=%d, (symbol_within_slot_mon&0x3FFF)=%d, searchSpaceType=%d)\n",
                  gNB_id,nb_coreset_active,(symbol_within_slot_mon&0x3FFF),
                  searchSpaceType);
#endif
        nr_rx_pdcch(ue,
                    proc->frame_rx,
                    nr_tti_rx,
                    gNB_id,
                    //(ue->frame_parms.mode1_flag == 1) ? SISO : ALAMOUTI,
                    SISO,
                    ue->high_speed_flag,
                    ue->is_secondary_ue,
                    nb_coreset_active,
                    (symbol_within_slot_mon&0x3FFF),
                    searchSpaceType);
#ifdef NR_PDCCH_SCHED_DEBUG
          printf("<-NR_PDCCH_PHY_PROCEDURES_LTE_UE (nr_ue_pdcch_procedures)-> Ending function nr_rx_pdcch(nb_coreset_active=%d, (symbol_within_slot_mon&0x3FFF)=%d, searchSpaceType=%d)\n",
                  nb_coreset_active,(symbol_within_slot_mon&0x3FFF),
                  searchSpaceType);
#endif

  */
  
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_RX_PDCCH, VCD_FUNCTION_IN);
  nr_rx_pdcch(ue,
	      proc->frame_rx,
	      nr_tti_rx);  
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_RX_PDCCH, VCD_FUNCTION_OUT);
  

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_DCI_DECODING, VCD_FUNCTION_IN);

#ifdef NR_PDCCH_SCHED_DEBUG
  printf("<-NR_PDCCH_PHY_PROCEDURES_LTE_UE (nr_ue_pdcch_procedures)-> Entering function nr_dci_decoding_procedure with (nb_searchspace_active=%d)\n",
	 pdcch_vars->nb_search_space);
#endif

  fapi_nr_dci_indication_t dci_ind;
  nr_downlink_indication_t dl_indication;
  memset((void*)&dci_ind,0,sizeof(dci_ind));
  memset((void*)&dl_indication,0,sizeof(dl_indication));
  dci_cnt = nr_dci_decoding_procedure(ue,
				      proc->frame_rx,
				      nr_tti_rx,
				      &dci_ind);

#ifdef NR_PDCCH_SCHED_DEBUG
  LOG_I(PHY,"<-NR_PDCCH_PHY_PROCEDURES_LTE_UE (nr_ue_pdcch_procedures)-> Ending function nr_dci_decoding_procedure() -> dci_cnt=%u\n",dci_cnt);
#endif
  
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_DCI_DECODING, VCD_FUNCTION_OUT);
  //LOG_D(PHY,"[UE  %d][PUSCH] Frame %d nr_tti_rx %d PHICH RX\n",ue->Mod_id,frame_rx,nr_tti_rx);

  for (int i=0; i<dci_cnt; i++) {
    LOG_D(PHY,"[UE  %d] AbsSubFrame %d.%d, Mode %s: DCI found %i --> rnti %x : format %d\n",
	  ue->Mod_id,frame_rx%1024,nr_tti_rx,nr_mode_string[ue->UE_mode[gNB_id]],
	  i,
	  dci_ind.dci_list[i].rnti,
	  dci_ind.dci_list[i].dci_format);
  }
  ue->pdcch_vars[ue->current_thread_id[nr_tti_rx]][gNB_id]->dci_received += dci_cnt;

  dci_ind.number_of_dcis = dci_cnt;
    /*
    for (int i=0; i<dci_cnt; i++) {
      
	memset(&dci_ind.dci_list[i].dci,0,sizeof(fapi_nr_dci_pdu_rel15_t));
	
	dci_ind.dci_list[i].rnti = dci_alloc_rx[i].rnti;
	dci_ind.dci_list[i].dci_format = dci_alloc_rx[i].format;
	dci_ind.dci_list[i].n_CCE = dci_alloc_rx[i].firstCCE;
	dci_ind.dci_list[i].N_CCE = (int)dci_alloc_rx[i].L;
	
	status = nr_extract_dci_info(ue,
				     gNB_id,
				     ue->frame_parms.frame_type,
				     dci_alloc_rx[i].dci_length,
				     dci_alloc_rx[i].rnti,
				     dci_alloc_rx[i].dci_pdu,
				     &dci_ind.dci_list[i].dci,
				     dci_fields_sizes_cnt[i],
				     dci_alloc_rx[i].format,
				     nr_tti_rx,
				     pdcch_vars2->n_RB_BWP[nb_searchspace_active],
				     pdcch_vars2->n_RB_BWP[nb_searchspace_active],
				     crc_scrambled_values);
	
	if(status == 0) {
	  LOG_W(PHY,"<-NR_PDCCH_PHY_PROCEDURES_UE (nr_ue_pdcch_procedures)-> bad DCI %d !!! \n",dci_alloc_rx[i].format);
	  return(-1);
	}
	
	LOG_D(PHY,"<-NR_PDCCH_PHY_PROCEDURES_UE (nr_ue_pdcch_procedures)-> Ending function nr_extract_dci_info()\n");
	

        
      } // end for loop dci_cnt
    */

    // fill dl_indication message
    dl_indication.module_id = ue->Mod_id;
    dl_indication.cc_id = ue->CC_id;
    dl_indication.gNB_index = gNB_id;
    dl_indication.frame = frame_rx;
    dl_indication.slot = nr_tti_rx;
    dl_indication.rx_ind = NULL; //no data, only dci for now
    dl_indication.dci_ind = &dci_ind; 
    
    //  send to mac
    ue->if_inst->dl_indication(&dl_indication, NULL);

#if UE_TIMING_TRACE
  stop_meas(&ue->dlsch_rx_pdcch_stats);
#endif
    
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_PDCCH_PROCEDURES, VCD_FUNCTION_OUT);
  return(dci_cnt);
}
#endif // NR_PDCCH_SCHED




#if 0

       if (generate_ue_dlsch_params_from_dci(frame_rx,
       nr_tti_rx,
       (DCI1A_5MHz_TDD_1_6_t *)&dci_alloc_rx[i].dci_pdu,
       ue->prach_resources[eNB_id]->ra_RNTI,
       format1A,
       ue->pdcch_vars[ue->current_thread_id[nr_tti_rx]][eNB_id],
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

       #ifdef DEBUG_PHY_PROC
       LOG_D(PHY,"[UE  %d] Generate UE DLSCH RA_RNTI format 1A, rb_alloc %x, dlsch_ra[eNB_id] %p\n",
       ue->Mod_id,ue->dlsch_ra[eNB_id]->harq_processes[0]->rb_alloc_even[0],ue->dlsch_ra[eNB_id]);
       #endif
       }
       } else if( (dci_alloc_rx[i].rnti == ue->pdcch_vars[ue->current_thread_id[nr_tti_rx]][eNB_id]->crnti) &&
       (dci_alloc_rx[i].format == format0)) {

       #ifdef DEBUG_PHY_PROC
       LOG_D(PHY,"[UE  %d][PUSCH] Frame %d nr_tti_rx %d: Found rnti %x, format 0, dci_cnt %d\n",
       ue->Mod_id,frame_rx,nr_tti_rx,dci_alloc_rx[i].rnti,i);
       #endif

       ue->ulsch_no_allocation_counter[eNB_id] = 0;
       //dump_dci(&ue->frame_parms,&dci_alloc_rx[i]);

       if ((ue->UE_mode[eNB_id] > PRACH) &&
       (generate_ue_ulsch_params_from_dci((void *)&dci_alloc_rx[i].dci_pdu,
       ue->pdcch_vars[ue->current_thread_id[nr_tti_rx]][eNB_id]->crnti,
       nr_tti_rx,
       format0,
       ue,
       proc,
       SI_RNTI,
       0,
       P_RNTI,
       CBA_RNTI,
       eNB_id,
       0)==0)) {
       #if T_TRACER
       NR_DL_FRAME_PARMS *frame_parms = &ue->frame_parms;
       uint8_t harq_pid = subframe2harq_pid(frame_parms,
       pdcch_alloc2ul_frame(frame_parms,proc->frame_rx,proc->nr_tti_rx),
       pdcch_alloc2ul_subframe(frame_parms,proc->nr_tti_rx));

       T(T_UE_PHY_ULSCH_UE_DCI, T_INT(eNB_id), T_INT(proc->frame_rx%1024), T_INT(proc->nr_tti_rx), T_INT(ue->Mod_id),
       T_INT(dci_alloc_rx[i].rnti), T_INT(harq_pid),
       T_INT(ue->ulsch[eNB_id]->harq_processes[harq_pid]->mcs),
       T_INT(ue->ulsch[eNB_id]->harq_processes[harq_pid]->round),
       T_INT(ue->ulsch[eNB_id]->harq_processes[harq_pid]->first_rb),
       T_INT(ue->ulsch[eNB_id]->harq_processes[harq_pid]->nb_rb),
       T_INT(ue->ulsch[eNB_id]->harq_processes[harq_pid]->TBS));
       #endif
       #ifdef DEBUG_PHY_PROC
       LOG_D(PHY,"[UE  %d] Generate UE ULSCH C_RNTI format 0 (nr_tti_rx %d)\n",ue->Mod_id,nr_tti_rx);
       #endif

       }
       } else if( (dci_alloc_rx[i].rnti == ue->ulsch[eNB_id]->cba_rnti[0]) &&
       (dci_alloc_rx[i].format == format0)) {
       // UE could belong to more than one CBA group
       // ue->Mod_id%ue->ulsch[eNB_id]->num_active_cba_groups]
       #ifdef DEBUG_PHY_PROC
       LOG_D(PHY,"[UE  %d][PUSCH] Frame %d nr_tti_rx %d: Found cba rnti %x, format 0, dci_cnt %d\n",
       ue->Mod_id,frame_rx,nr_tti_rx,dci_alloc_rx[i].rnti,i);

       if (((frame_rx%100) == 0) || (frame_rx < 20))
       dump_dci(&ue->frame_parms, &dci_alloc_rx[i]);

       #endif

       ue->ulsch_no_allocation_counter[eNB_id] = 0;
       //dump_dci(&ue->frame_parms,&dci_alloc_rx[i]);

       if ((ue->UE_mode[eNB_id] > PRACH) &&
       (generate_ue_ulsch_params_from_dci((void *)&dci_alloc_rx[i].dci_pdu,
       ue->ulsch[eNB_id]->cba_rnti[0],
       nr_tti_rx,
       format0,
       ue,
       proc,
       SI_RNTI,
       0,
       P_RNTI,
       CBA_RNTI,
       eNB_id,
       0)==0)) {

       #ifdef DEBUG_PHY_PROC
       LOG_D(PHY,"[UE  %d] Generate UE ULSCH CBA_RNTI format 0 (nr_tti_rx %d)\n",ue->Mod_id,nr_tti_rx);
       #endif
       ue->ulsch[eNB_id]->num_cba_dci[(nr_tti_rx+4)%10]++;
       }
       }

       else {
       #ifdef DEBUG_PHY_PROC
       LOG_D(PHY,"[UE  %d] frame %d, nr_tti_rx %d: received DCI %d with RNTI=%x (C-RNTI:%x, CBA_RNTI %x) and format %d!\n",ue->Mod_id,frame_rx,nr_tti_rx,i,dci_alloc_rx[i].rnti,
       ue->pdcch_vars[ue->current_thread_id[nr_tti_rx]][eNB_id]->crnti,
       ue->ulsch[eNB_id]->cba_rnti[0],
       dci_alloc_rx[i].format);

       //      dump_dci(&ue->frame_parms, &dci_alloc_rx[i]);
       #endif
       }*/

    } // end for loop dci_cnt
#if UE_TIMING_TRACE
    stop_meas(&ue->dlsch_rx_pdcch_stats);
#endif
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_PDCCH_PROCEDURES, VCD_FUNCTION_OUT);

    //    } // end if do_pdcch_monitoring_current_slot
  } // end for loop nb_searchspace_active
  return(0);
}



#endif


#if 0
void copy_harq_proc_struct(NR_DL_UE_HARQ_t *harq_processes_dest, NR_DL_UE_HARQ_t *current_harq_processes)
{

  harq_processes_dest->B             = current_harq_processes->B             ;
  harq_processes_dest->C             = current_harq_processes->C             ;
  harq_processes_dest->DCINdi         = current_harq_processes->DCINdi       ;
  harq_processes_dest->F             = current_harq_processes->F             ;
  harq_processes_dest->G             = current_harq_processes->G             ;
  harq_processes_dest->K             = current_harq_processes->K             ;
  harq_processes_dest->Nl             = current_harq_processes->Nl             ;
  harq_processes_dest->Qm             = current_harq_processes->Qm             ;
  harq_processes_dest->TBS            = current_harq_processes->TBS            ;
  harq_processes_dest->b              = current_harq_processes->b              ;
  harq_processes_dest->codeword       = current_harq_processes->codeword       ;
  harq_processes_dest->delta_PUCCH    = current_harq_processes->delta_PUCCH    ;
  harq_processes_dest->dl_power_off   = current_harq_processes->dl_power_off   ;
  harq_processes_dest->first_tx       = current_harq_processes->first_tx       ;
  harq_processes_dest->mcs            = current_harq_processes->mcs            ;
  harq_processes_dest->mcs_table      = current_harq_processes->mcs_table      ;
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
#endif

/*void copy_ack_struct(nr_harq_status_t *harq_ack_dest, nr_harq_status_t *current_harq_ack)
  {
  memcpy(harq_ack_dest, current_harq_ack, sizeof(nr_harq_status_t));
  }*/

void nr_ue_pdsch_procedures(PHY_VARS_NR_UE *ue, UE_nr_rxtx_proc_t *proc, int eNB_id, PDSCH_t pdsch, NR_UE_DLSCH_t *dlsch0, NR_UE_DLSCH_t *dlsch1) {

  int nr_tti_rx = proc->nr_tti_rx;
  int m;
  int i_mod,eNB_id_i,dual_stream_UE;
  int first_symbol_flag=0;

  if (!dlsch0)
  	return;
  if (dlsch0->active == 0)
    return;

  if (!dlsch1)  {
    int harq_pid = dlsch0->current_harq_pid;
    uint16_t BWPStart       = dlsch0->harq_processes[harq_pid]->BWPStart;
    //    uint16_t BWPSize        = dlsch0->harq_processes[harq_pid]->BWPSize;
    uint16_t pdsch_start_rb = dlsch0->harq_processes[harq_pid]->start_rb;
    uint16_t pdsch_nb_rb =  dlsch0->harq_processes[harq_pid]->nb_rb;
    uint16_t s0 =  dlsch0->harq_processes[harq_pid]->start_symbol;
    uint16_t s1 =  dlsch0->harq_processes[harq_pid]->nb_symbols;

    LOG_D(PHY,"[UE %d] PDSCH type %d active in nr_tti_rx %d, harq_pid %d, rb_start %d, nb_rb %d, symbol_start %d, nb_symbols %d, DMRS mask %x\n",ue->Mod_id,pdsch,nr_tti_rx,harq_pid,pdsch_start_rb,pdsch_nb_rb,s0,s1,dlsch0->harq_processes[harq_pid]->dlDmrsSymbPos);

    // do channel estimation for first DMRS only
    for (m = s0; m < 3; m++) {
      if (((1<<m)&dlsch0->harq_processes[harq_pid]->dlDmrsSymbPos) > 0) {
	nr_pdsch_channel_estimation(ue,
				    0 /*eNB_id*/,
				    nr_tti_rx,
				    0 /*p*/,
				    m,
				    ue->frame_parms.first_carrier_offset+(BWPStart + pdsch_start_rb)*12,
				    pdsch_nb_rb);
	LOG_D(PHY,"Channel Estimation in symbol %d\n",m);
	break;
      }
    }
    for (m = s0; m < (s1 + s0); m++) {
 
      dual_stream_UE = 0;
      eNB_id_i = eNB_id+1;
      i_mod = 0;

      if ((m==s0) && (m<3))
	first_symbol_flag = 1;
      else
	first_symbol_flag = 0;
#if UE_TIMING_TRACE
      uint8_t slot = 0;
      if(m >= ue->frame_parms.symbols_per_slot>>1)
        slot = 1;
      start_meas(&ue->dlsch_llr_stats_parallelization[ue->current_thread_id[nr_tti_rx]][slot]);
#endif
      // process DLSCH received in first slot
      nr_rx_pdsch(ue,
	       pdsch,
	       eNB_id,
	       eNB_id_i,
	       proc->frame_rx,
	       nr_tti_rx,  // nr_tti_rx,
	       m,
	       first_symbol_flag,
	       dual_stream_UE,
	       i_mod,
	       dlsch0->current_harq_pid);
#if UE_TIMING_TRACE
      stop_meas(&ue->dlsch_llr_stats_parallelization[ue->current_thread_id[nr_tti_rx]][slot]);
#if DISABLE_LOG_X
      printf("[AbsSFN %d.%d] LLR Computation Symbol %d %5.2f \n",proc->frame_rx,nr_tti_rx,m,ue->dlsch_llr_stats_parallelization[ue->current_thread_id[nr_tti_rx]][slot].p_time/(cpuf*1000.0));
#else
      LOG_D(PHY, "[AbsSFN %d.%d] LLR Computation Symbol %d %5.2f \n",proc->frame_rx,nr_tti_rx,m,ue->dlsch_llr_stats_parallelization[ue->current_thread_id[nr_tti_rx]][slot].p_time/(cpuf*1000.0));
#endif
#endif

      if(first_symbol_flag)
	{
          proc->first_symbol_available = 1;
	}
    } // CRNTI active
  }
}
#if 0
void process_rar(PHY_VARS_NR_UE *ue, UE_nr_rxtx_proc_t *proc, int eNB_id, runmode_t mode, int abstraction_flag) {

  int frame_rx = proc->frame_rx;
  int nr_tti_rx = proc->nr_tti_rx;
  int timing_advance;
  NR_UE_DLSCH_t *dlsch0 = ue->dlsch_ra[eNB_id];
  int harq_pid = 0;
  uint8_t *rar;
  /*
  uint8_t next1_thread_id = ue->current_thread_id[nr_tti_rx]== (RX_NB_TH-1) ? 0:(ue->current_thread_id[nr_tti_rx]+1);
  uint8_t next2_thread_id = next1_thread_id== (RX_NB_TH-1) ? 0:(next1_thread_id+1);
  */

  LOG_D(PHY,"[UE  %d][RAPROC] Frame %d nr_tti_rx %d Received RAR  mode %d\n",
	ue->Mod_id,
	frame_rx,
	nr_tti_rx, ue->UE_mode[eNB_id]);


  if (ue->mac_enabled == 1) {
    if ((ue->UE_mode[eNB_id] != PUSCH) &&
	(ue->prach_resources[eNB_id]->Msg3!=NULL)) {
      LOG_D(PHY,"[UE  %d][RAPROC] Frame %d nr_tti_rx %d Invoking MAC for RAR (current preamble %d)\n",
	    ue->Mod_id,frame_rx,
	    nr_tti_rx,
	    ue->prach_resources[eNB_id]->ra_PreambleIndex);
      
      /*      timing_advance = mac_xface->ue_process_rar(ue->Mod_id,
	      ue->CC_id,
	      frame_rx,
	      ue->prach_resources[eNB_id]->ra_RNTI,
	      dlsch0->harq_processes[0]->b,
	      &ue->pdcch_vars[ue->current_thread_id[nr_tti_rx]][eNB_id]->crnti,
	      ue->prach_resources[eNB_id]->ra_PreambleIndex,
	      dlsch0->harq_processes[0]->b); // alter the 'b' buffer so it contains only the selected RAR header and RAR payload
      */
      /*
      ue->pdcch_vars[next1_thread_id][eNB_id]->crnti = ue->pdcch_vars[ue->current_thread_id[nr_tti_rx]][eNB_id]->crnti;
      ue->pdcch_vars[next2_thread_id][eNB_id]->crnti = ue->pdcch_vars[ue->current_thread_id[nr_tti_rx]][eNB_id]->crnti;
      */

      if (timing_advance!=0xffff) {

	LOG_D(PHY,"[UE  %d][RAPROC] Frame %d nr_tti_rx %d Got rnti %x and timing advance %d from RAR\n",
              ue->Mod_id,
              frame_rx,
              nr_tti_rx,
              ue->pdcch_vars[ue->current_thread_id[nr_tti_rx]][eNB_id]->crnti,
              timing_advance);

	// remember this c-rnti is still a tc-rnti

	ue->pdcch_vars[ue->current_thread_id[nr_tti_rx]][eNB_id]->crnti_is_temporary = 1;
	      
	//timing_advance = 0;
	nr_process_timing_advance_rar(ue,proc,timing_advance);
	      
	if (mode!=debug_prach) {
	  ue->ulsch_Msg3_active[eNB_id]=1;
	  nr_get_Msg3_alloc(&ue->frame_parms,
			    nr_tti_rx,
			    frame_rx,
			    &ue->ulsch_Msg3_frame[eNB_id],
			    &ue->ulsch_Msg3_subframe[eNB_id]);
	  
	  LOG_D(PHY,"[UE  %d][RAPROC] Got Msg3_alloc Frame %d nr_tti_rx %d: Msg3_frame %d, Msg3_subframe %d\n",
		ue->Mod_id,
		frame_rx,
		nr_tti_rx,
		ue->ulsch_Msg3_frame[eNB_id],
		ue->ulsch_Msg3_subframe[eNB_id]);
	  harq_pid = nr_subframe2harq_pid(&ue->frame_parms,
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
  }
  else {
    rar = dlsch0->harq_processes[0]->b+1;
    timing_advance = ((((uint16_t)(rar[0]&0x7f))<<4) + (rar[1]>>4));
    nr_process_timing_advance_rar(ue,proc,timing_advance);
  }

}
#endif

void nr_ue_dlsch_procedures(PHY_VARS_NR_UE *ue,
       UE_nr_rxtx_proc_t *proc,
       int eNB_id,
       PDSCH_t pdsch,
       NR_UE_DLSCH_t *dlsch0,
       NR_UE_DLSCH_t *dlsch1,
       int *dlsch_errors,
       runmode_t mode) {

  int harq_pid;
  int frame_rx = proc->frame_rx;
  int nr_tti_rx = proc->nr_tti_rx;
  int ret=0, ret1=0;
  NR_UE_PDSCH *pdsch_vars;
  uint8_t is_cw0_active = 0;
  uint8_t is_cw1_active = 0;
  //nfapi_nr_config_request_t *cfg = &ue->nrUE_config;
  //uint8_t dmrs_type = cfg->pdsch_config.dmrs_type.value;
  uint8_t nb_re_dmrs = 6; //(dmrs_type==NFAPI_NR_DMRS_TYPE1)?6:4;
  uint16_t length_dmrs = 1; //cfg->pdsch_config.dmrs_max_length.value;
  uint16_t nb_symb_sch = 9;
  nr_downlink_indication_t dl_indication;
  fapi_nr_rx_indication_t rx_ind;
  // params for UL time alignment procedure
  NR_UL_TIME_ALIGNMENT_t *ul_time_alignment = &ue->ul_time_alignment[eNB_id];
  uint16_t slots_per_frame = ue->frame_parms.slots_per_frame;
  uint16_t slots_per_subframe = ue->frame_parms.slots_per_subframe;
  uint8_t numerology = ue->frame_parms.numerology_index, mapping_type_ul, mapping_type_dl;
  int ul_tx_timing_adjustment, N_TA_max, factor_mu, N_t_1, N_t_2, N_1, N_2, d_1_1 = 0, d_2_1, d;
  uint8_t d_2_2 = 0;// set to 0 because there is only 1 BWP
                    // TODO this should corresponds to the switching time as defined in
                    // TS 38.133
  uint16_t ofdm_symbol_size = ue->frame_parms.ofdm_symbol_size;
  uint16_t nb_prefix_samples = ue->frame_parms.nb_prefix_samples;
  uint32_t t_subframe = 1; // subframe duration of 1 msec
  uint16_t bw_scaling, start_symbol;
  float tc_factor;

  if (dlsch0==NULL)
    AssertFatal(0,"dlsch0 should be defined at this level \n");


  harq_pid = dlsch0->current_harq_pid;
  is_cw0_active = dlsch0->harq_processes[harq_pid]->status;
  nb_symb_sch = dlsch0->harq_processes[harq_pid]->nb_symbols;
  start_symbol = dlsch0->harq_processes[harq_pid]->start_symbol;



  if(dlsch1)
    is_cw1_active = dlsch1->harq_processes[harq_pid]->status;

  LOG_D(PHY,"AbsSubframe %d.%d Start LDPC Decoder for CW0 [harq_pid %d] ? %d \n", frame_rx%1024, nr_tti_rx, harq_pid, is_cw0_active);
  LOG_D(PHY,"AbsSubframe %d.%d Start LDPC Decoder for CW1 [harq_pid %d] ? %d \n", frame_rx%1024, nr_tti_rx, harq_pid, is_cw1_active);

  if(is_cw0_active && is_cw1_active)
    {
      dlsch0->Kmimo = 2;
      dlsch1->Kmimo = 2;
    }
  else
    {
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
      pdsch_vars = ue->pdsch_vars[ue->current_thread_id[nr_tti_rx]][eNB_id];
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
	LOG_E(PHY,"[UE %d] Frame %d, nr_tti_rx %d: FATAL, prach_resources is NULL\n",ue->Mod_id,frame_rx,nr_tti_rx);
	//mac_xface->macphy_exit("prach_resources is NULL");
	return;
      }
    }


      // start ldpc decode for CW 0
      dlsch0->harq_processes[harq_pid]->G = nr_get_G(dlsch0->harq_processes[harq_pid]->nb_rb,
						     nb_symb_sch,
						     nb_re_dmrs,
						     length_dmrs,
						     dlsch0->harq_processes[harq_pid]->Qm,
						     dlsch0->harq_processes[harq_pid]->Nl);
#if UE_TIMING_TRACE
      start_meas(&ue->dlsch_unscrambling_stats);
#endif
      nr_dlsch_unscrambling(pdsch_vars->llr[0],
    		  	  	  	    dlsch0->harq_processes[harq_pid]->G,
							0,
							ue->frame_parms.Nid_cell,
							dlsch0->rnti);
      

#if UE_TIMING_TRACE
      stop_meas(&ue->dlsch_unscrambling_stats);
#endif

#if 0
      LOG_I(PHY," ------ start ldpc decoder for AbsSubframe %d.%d / %d  ------  \n", frame_rx, nr_tti_rx, harq_pid);
      LOG_I(PHY,"start ldpc decode for CW 0 for AbsSubframe %d.%d / %d --> nb_rb %d \n", frame_rx, nr_tti_rx, harq_pid, dlsch0->harq_processes[harq_pid]->nb_rb);
      LOG_I(PHY,"start ldpc decode for CW 0 for AbsSubframe %d.%d / %d  --> rb_alloc_even %x \n", frame_rx, nr_tti_rx, harq_pid, dlsch0->harq_processes[harq_pid]->rb_alloc_even);
      LOG_I(PHY,"start ldpc decode for CW 0 for AbsSubframe %d.%d / %d  --> Qm %d \n", frame_rx, nr_tti_rx, harq_pid, dlsch0->harq_processes[harq_pid]->Qm);
      LOG_I(PHY,"start ldpc decode for CW 0 for AbsSubframe %d.%d / %d  --> Nl %d \n", frame_rx, nr_tti_rx, harq_pid, dlsch0->harq_processes[harq_pid]->Nl);
      LOG_I(PHY,"start ldpc decode for CW 0 for AbsSubframe %d.%d / %d  --> G  %d \n", frame_rx, nr_tti_rx, harq_pid, dlsch0->harq_processes[harq_pid]->G);
      LOG_I(PHY,"start ldpc decode for CW 0 for AbsSubframe %d.%d / %d  --> Kmimo  %d \n", frame_rx, nr_tti_rx, harq_pid, dlsch0->Kmimo);
      LOG_I(PHY,"start ldpc decode for CW 0 for AbsSubframe %d.%d / %d  --> Pdcch Sym  %d \n", frame_rx, nr_tti_rx, harq_pid, ue->pdcch_vars[ue->current_thread_id[nr_tti_rx]][eNB_id]->num_pdcch_symbols);
#endif

#if UE_TIMING_TRACE
      start_meas(&ue->dlsch_decoding_stats[ue->current_thread_id[nr_tti_rx]]);
#endif

#ifdef UE_DLSCH_PARALLELISATION
		 ret = nr_dlsch_decoding_mthread(ue,
			   proc,
			   eNB_id,
			   pdsch_vars->llr[0],
			   &ue->frame_parms,
			   dlsch0,
			   dlsch0->harq_processes[harq_pid],
			   frame_rx,
			   nb_symb_sch,
			   nr_tti_rx,
			   harq_pid,
			   pdsch==PDSCH?1:0,
			   dlsch0->harq_processes[harq_pid]->TBS>256?1:0);
		 LOG_T(PHY,"UE_DLSCH_PARALLELISATION is defined, ret = %d\n", ret);
#else
      ret = nr_dlsch_decoding(ue,
			   pdsch_vars->llr[0],
			   &ue->frame_parms,
			   dlsch0,
			   dlsch0->harq_processes[harq_pid],
			   frame_rx,
			   nb_symb_sch,
			   nr_tti_rx,
			   harq_pid,
			   pdsch==PDSCH?1:0,
			   dlsch0->harq_processes[harq_pid]->TBS>256?1:0);
      LOG_T(PHY,"UE_DLSCH_PARALLELISATION is NOT defined, ret = %d\n", ret);
      //printf("start cW0 dlsch decoding\n");
#endif

#if UE_TIMING_TRACE
      stop_meas(&ue->dlsch_decoding_stats[ue->current_thread_id[nr_tti_rx]]);
#if DISABLE_LOG_X
      printf(" --> Unscrambling for CW0 %5.3f\n",
              (ue->dlsch_unscrambling_stats.p_time)/(cpuf*1000.0));
      printf("AbsSubframe %d.%d --> LDPC Decoding for CW0 %5.3f\n",
              frame_rx%1024, nr_tti_rx,(ue->dlsch_decoding_stats[ue->current_thread_id[nr_tti_rx]].p_time)/(cpuf*1000.0));
#else
      LOG_I(PHY, " --> Unscrambling for CW0 %5.3f\n",
              (ue->dlsch_unscrambling_stats.p_time)/(cpuf*1000.0));
      LOG_I(PHY, "AbsSubframe %d.%d --> LDPC Decoding for CW0 %5.3f\n",
              frame_rx%1024, nr_tti_rx,(ue->dlsch_decoding_stats[ue->current_thread_id[nr_tti_rx]].p_time)/(cpuf*1000.0));
#endif

#endif
      if(is_cw1_active)
      {
          // start ldpc decode for CW 1
          dlsch1->harq_processes[harq_pid]->G = nr_get_G(dlsch1->harq_processes[harq_pid]->nb_rb,
							 nb_symb_sch,
							 nb_re_dmrs,
							 length_dmrs,
							 dlsch1->harq_processes[harq_pid]->Qm,
							 dlsch1->harq_processes[harq_pid]->Nl);
#if UE_TIMING_TRACE
          start_meas(&ue->dlsch_unscrambling_stats);
#endif
          nr_dlsch_unscrambling(pdsch_vars->llr[1],
              		  	  	  	  	dlsch1->harq_processes[harq_pid]->G,
                                    0,
          							ue->frame_parms.Nid_cell,
          							dlsch1->rnti);
#if UE_TIMING_TRACE
          stop_meas(&ue->dlsch_unscrambling_stats);
#endif

#if 0
          LOG_I(PHY,"start ldpc decode for CW 1 for AbsSubframe %d.%d / %d --> nb_rb %d \n", frame_rx, nr_tti_rx, harq_pid, dlsch1->harq_processes[harq_pid]->nb_rb);
          LOG_I(PHY,"start ldpc decode for CW 1 for AbsSubframe %d.%d / %d  --> rb_alloc_even %x \n", frame_rx, nr_tti_rx, harq_pid, dlsch1->harq_processes[harq_pid]->rb_alloc_even);
          LOG_I(PHY,"start ldpc decode for CW 1 for AbsSubframe %d.%d / %d  --> Qm %d \n", frame_rx, nr_tti_rx, harq_pid, dlsch1->harq_processes[harq_pid]->Qm);
          LOG_I(PHY,"start ldpc decode for CW 1 for AbsSubframe %d.%d / %d  --> Nl %d \n", frame_rx, nr_tti_rx, harq_pid, dlsch1->harq_processes[harq_pid]->Nl);
          LOG_I(PHY,"start ldpc decode for CW 1 for AbsSubframe %d.%d / %d  --> G  %d \n", frame_rx, nr_tti_rx, harq_pid, dlsch1->harq_processes[harq_pid]->G);
          LOG_I(PHY,"start ldpc decode for CW 1 for AbsSubframe %d.%d / %d  --> Kmimo  %d \n", frame_rx, nr_tti_rx, harq_pid, dlsch1->Kmimo);
          LOG_I(PHY,"start ldpc decode for CW 1 for AbsSubframe %d.%d / %d  --> Pdcch Sym  %d \n", frame_rx, nr_tti_rx, harq_pid, ue->pdcch_vars[ue->current_thread_id[nr_tti_rx]][eNB_id]->num_pdcch_symbols);
#endif

#if UE_TIMING_TRACE
          start_meas(&ue->dlsch_decoding_stats[ue->current_thread_id[nr_tti_rx]]);
#endif

#ifdef UE_DLSCH_PARALLELISATION
          ret1 = nr_dlsch_decoding_mthread(ue,
        		  	  	  	proc,
        		  	  	    eNB_id,
                            pdsch_vars->llr[1],
                            &ue->frame_parms,
                            dlsch1,
                            dlsch1->harq_processes[harq_pid],
                            frame_rx,
                            nb_symb_sch,
                            nr_tti_rx,
                            harq_pid,
                            pdsch==PDSCH?1:0,
                            dlsch1->harq_processes[harq_pid]->TBS>256?1:0);
          LOG_T(PHY,"UE_DLSCH_PARALLELISATION is defined, ret1 = %d\n", ret1);
#else
          ret1 = nr_dlsch_decoding(ue,
                  pdsch_vars->llr[1],
                  &ue->frame_parms,
                  dlsch1,
                  dlsch1->harq_processes[harq_pid],
                  frame_rx,
				  nb_symb_sch,
                  nr_tti_rx,
                  harq_pid,
                  pdsch==PDSCH?1:0,//proc->decoder_switch,
                  dlsch1->harq_processes[harq_pid]->TBS>256?1:0);
          LOG_T(PHY,"UE_DLSCH_PARALLELISATION is NOT defined, ret1 = %d\n", ret1);
          printf("start cw1 dlsch decoding\n");
#endif

#if UE_TIMING_TRACE
          stop_meas(&ue->dlsch_decoding_stats[ue->current_thread_id[nr_tti_rx]]);
#if DISABLE_LOG_X
          printf(" --> Unscrambling for CW1 %5.3f\n",
                  (ue->dlsch_unscrambling_stats.p_time)/(cpuf*1000.0));
          printf("AbsSubframe %d.%d --> ldpc Decoding for CW1 %5.3f\n",
                  frame_rx%1024, nr_tti_rx,(ue->dlsch_decoding_stats[ue->current_thread_id[nr_tti_rx]].p_time)/(cpuf*1000.0));
#else
          LOG_D(PHY, " --> Unscrambling for CW1 %5.3f\n",
                  (ue->dlsch_unscrambling_stats.p_time)/(cpuf*1000.0));
          LOG_D(PHY, "AbsSubframe %d.%d --> ldpc Decoding for CW1 %5.3f\n",
                  frame_rx%1024, nr_tti_rx,(ue->dlsch_decoding_stats[ue->current_thread_id[nr_tti_rx]].p_time)/(cpuf*1000.0));
#endif

#endif
          LOG_I(PHY,"AbsSubframe %d.%d --> ldpc Decoding for CW1 %5.3f\n",
                  frame_rx%1024, nr_tti_rx,(ue->dlsch_decoding_stats[ue->current_thread_id[nr_tti_rx]].p_time)/(cpuf*1000.0));
      }

      LOG_D(PHY," ------ end ldpc decoder for AbsSubframe %d.%d ------  \n", frame_rx, nr_tti_rx);

      LOG_D(PHY, "harq_pid: %d, TBS expected dlsch0: %d, TBS expected dlsch1: %d  \n",harq_pid, dlsch0->harq_processes[harq_pid]->TBS, dlsch1->harq_processes[harq_pid]->TBS);
      
      if(ret<dlsch0->max_ldpc_iterations+1){
      // fill dl_indication message
      dl_indication.module_id = ue->Mod_id;
      dl_indication.cc_id = ue->CC_id;
      dl_indication.gNB_index = eNB_id;
      dl_indication.frame = frame_rx;
      dl_indication.slot = nr_tti_rx;

      dl_indication.rx_ind = &rx_ind; //  hang on rx_ind instance
      dl_indication.proc=proc;

      //dl_indication.rx_ind->number_pdus
      rx_ind.rx_indication_body[0].pdu_type = FAPI_NR_RX_PDU_TYPE_DLSCH;
      rx_ind.rx_indication_body[0].pdsch_pdu.pdu = dlsch0->harq_processes[harq_pid]->b;
      rx_ind.rx_indication_body[0].pdsch_pdu.pdu_length = dlsch0->harq_processes[harq_pid]->TBS>>3;
      LOG_D(PHY, "PDU length in bits: %d, in bytes: %d \n", dlsch0->harq_processes[harq_pid]->TBS, rx_ind.rx_indication_body[0].pdsch_pdu.pdu_length);
      rx_ind.number_pdus = 1;

      //ue->dl_indication.rx_ind = &dlsch1->harq_processes[harq_pid]->b; //no data, only dci for now
      dl_indication.dci_ind = NULL; //&ue->dci_ind;
      //  send to mac
      if (ue->if_inst && ue->if_inst->dl_indication)
      ue->if_inst->dl_indication(&dl_indication, ul_time_alignment);
      }

      // TODO CRC check for CW0

      // Check CRC for CW 0
      /*if (ret == (1+dlsch0->max_turbo_iterations)) {
        *dlsch_errors=*dlsch_errors+1;
        if(dlsch0->rnti != 0xffff){
          LOG_D(PHY,"[UE  %d][PDSCH %x/%d] AbsSubframe %d.%d : DLSCH CW0 in error (rv %d,round %d, mcs %d,TBS %d)\n",
          ue->Mod_id,dlsch0->rnti,
          harq_pid,frame_rx,subframe_rx,
          dlsch0->harq_processes[harq_pid]->rvidx,
          dlsch0->harq_processes[harq_pid]->round,
          dlsch0->harq_processes[harq_pid]->mcs,
          dlsch0->harq_processes[harq_pid]->TBS);
        }
      } else {
        if(dlsch0->rnti != 0xffff){
          LOG_D(PHY,"[UE  %d][PDSCH %x/%d] AbsSubframe %d.%d : Received DLSCH CW0 (rv %d,round %d, mcs %d,TBS %d)\n",
          ue->Mod_id,dlsch0->rnti,
          harq_pid,frame_rx,subframe_rx,
          dlsch0->harq_processes[harq_pid]->rvidx,
          dlsch0->harq_processes[harq_pid]->round,
          dlsch0->harq_processes[harq_pid]->mcs,
          dlsch0->harq_processes[harq_pid]->TBS);
        }
        if ( LOG_DEBUGFLAG(DEBUG_UE_PHYPROC)){
          int j;
          LOG_D(PHY,"dlsch harq_pid %d (rx): \n",dlsch0->current_harq_pid);

          for (j=0; j<dlsch0->harq_processes[dlsch0->current_harq_pid]->TBS>>3; j++)
            LOG_T(PHY,"%x.",dlsch0->harq_processes[dlsch0->current_harq_pid]->b[j]);
          LOG_T(PHY,"\n");
      }*/

      if (ue->mac_enabled == 1) {

        // scale the 16 factor in N_TA calculation in 38.213 section 4.2 according to the used FFT size
        switch (ue->frame_parms.N_RB_DL) {
          case 32:  bw_scaling =  4; break;
          case 66:  bw_scaling =  8; break;
          case 106: bw_scaling = 16; break;
          case 217: bw_scaling = 32; break;
          case 245: bw_scaling = 32; break;
          case 273: bw_scaling = 32; break;
          default: abort();
        }

        /* Time Alignment procedure
        // - UE processing capability 1
        // - Setting the TA update to be applied after the reception of the TA command
        // - Timing adjustment computed according to TS 38.213 section 4.2
        // - Durations of N1 and N2 symbols corresponding to PDSCH and PUSCH are
        //   computed according to sections 5.3 and 6.4 of TS 38.214 */
        factor_mu = 1 << numerology;
        N_TA_max = 3846 * bw_scaling / factor_mu;

        /* PDSCH decoding time N_1 for processing capability 1 */
        if (ue->dmrs_DownlinkConfig.pdsch_dmrs_AdditionalPosition == pdsch_dmrs_pos0)
          N_1 = pdsch_N_1_capability_1[numerology][1];
        else if (ue->dmrs_DownlinkConfig.pdsch_dmrs_AdditionalPosition == pdsch_dmrs_pos1 || ue->dmrs_DownlinkConfig.pdsch_dmrs_AdditionalPosition == 2) // TODO set to pdsch_dmrs_pos2 when available
          N_1 = pdsch_N_1_capability_1[numerology][2];
        else
          N_1 = pdsch_N_1_capability_1[numerology][3];

        /* PUSCH preapration time N_2 for processing capability 1 */
        N_2 = pusch_N_2_timing_capability_1[numerology][1];
        mapping_type_dl = ue->PDSCH_Config.pdsch_TimeDomainResourceAllocation[0]->mappingType;
        mapping_type_ul = ue->pusch_config.pusch_TimeDomainResourceAllocation[0]->mappingType;

        /* d_1_1 depending on the number of PDSCH symbols allocated */
        d = 0; // TODO number of overlapping symbols of the scheduling PDCCH and the scheduled PDSCH
        if (mapping_type_dl == typeA)
         if (nb_symb_sch + start_symbol < 7)
          d_1_1 = 7 - (nb_symb_sch + start_symbol);
         else
          d_1_1 = 0;
        else // mapping type B
          switch (nb_symb_sch){
            case 7: d_1_1 = 0; break;
            case 4: d_1_1 = 3; break;
            case 2: d_1_1 = 3 + d; break;
            default: break;
          }

        /* d_2_1 */
        if (mapping_type_ul == typeB && start_symbol != 0)
          d_2_1 = 0;
        else
          d_2_1 = 1;

        /* N_t_1 time duration in msec of N_1 symbols corresponding to a PDSCH reception time
        // N_t_2 time duration in msec of N_2 symbols corresponding to a PUSCH preparation time */
        N_t_1 = (N_1 + d_1_1) * (ofdm_symbol_size + nb_prefix_samples) / factor_mu;
        N_t_2 = (N_2 + d_2_1) * (ofdm_symbol_size + nb_prefix_samples) / factor_mu;
        if (N_t_2 < d_2_2) N_t_2 = d_2_2;

        /* Time alignment procedure */
        // N_t_1 + N_t_2 + N_TA_max is in unit of Ts, therefore must be converted to Tc
        // N_t_1 + N_t_2 + N_TA_max must be in msec
        tc_factor = 64 * 0.509 * 10e-7;
        ul_tx_timing_adjustment = 1 + ceil(slots_per_subframe*((N_t_1 + N_t_2 + N_TA_max)*tc_factor + 0.5)/t_subframe);

        if (ul_time_alignment->apply_ta == 1){
          ul_time_alignment->ta_slot = (nr_tti_rx + ul_tx_timing_adjustment) % slots_per_frame;
          if (nr_tti_rx + ul_tx_timing_adjustment > slots_per_frame){
            ul_time_alignment->ta_frame = (frame_rx + 1) % 1024;
          } else {
            ul_time_alignment->ta_frame = frame_rx;
          }
          // reset TA flag
          ul_time_alignment->apply_ta = 0;
          LOG_D(PHY,"Frame %d slot %d -- Starting UL time alignment procedures. TA update will be applied at frame %d slot %d\n", frame_rx, nr_tti_rx, ul_time_alignment->ta_frame, ul_time_alignment->ta_slot);
        }
      }

      /*ue->total_TBS[eNB_id] =  ue->total_TBS[eNB_id] + dlsch0->harq_processes[dlsch0->current_harq_pid]->TBS;
      ue->total_received_bits[eNB_id] = ue->total_TBS[eNB_id] + dlsch0->harq_processes[dlsch0->current_harq_pid]->TBS;
    }*/

    // TODO CRC check for CW1

  }
}


/*!
 * \brief This is the UE synchronize thread.
 * It performs band scanning and synchonization.
 * \param arg is a pointer to a \ref PHY_VARS_NR_UE structure.
 * \returns a pointer to an int. The storage is not on the heap and must not be freed.
 */
#ifdef UE_SLOT_PARALLELISATION
#define FIFO_PRIORITY   40
void *UE_thread_slot1_dl_processing(void *arg) {

  static __thread int UE_dl_slot1_processing_retval;
  struct rx_tx_thread_data *rtd = arg;
  UE_nr_rxtx_proc_t *proc = rtd->proc;
  PHY_VARS_NR_UE    *ue   = rtd->UE;

  int frame_rx;
  uint8_t nr_tti_rx;
  uint8_t pilot0;
  uint8_t pilot1;
  uint8_t slot1;

  uint8_t next_nr_tti_rx;
  uint8_t next_subframe_slot0;

  proc->instance_cnt_slot1_dl_processing=-1;
  proc->nr_tti_rx=proc->sub_frame_start;

  char threadname[256];
  sprintf(threadname,"UE_thread_slot1_dl_processing_%d", proc->sub_frame_start);

  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  if ( (proc->sub_frame_start+1)%RX_NB_TH == 0 && threads.slot1_proc_one != -1 )
    CPU_SET(threads.slot1_proc_one, &cpuset);
  if ( (proc->sub_frame_start+1)%RX_NB_TH == 1 && threads.slot1_proc_two != -1 )
    CPU_SET(threads.slot1_proc_two, &cpuset);
  if ( (proc->sub_frame_start+1)%RX_NB_TH == 2 && threads.slot1_proc_three != -1 )
    CPU_SET(threads.slot1_proc_three, &cpuset);

  init_thread(900000,1000000 , FIFO_PRIORITY-1, &cpuset,
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
      nr_tti_rx = ue->proc.proc_rxtx[0].nr_tti_rx;
      printf("AbsSubframe %d.%d execute dl slot1 processing \n", frame_rx, nr_tti_rx);
      }*/
    frame_rx    = proc->frame_rx;
    nr_tti_rx = proc->nr_tti_rx;
    next_nr_tti_rx    = (1+nr_tti_rx)%10;
    next_subframe_slot0 = next_nr_tti_rx<<1;

    slot1  = (nr_tti_rx<<1) + 1;
    pilot0 = 0;

    //printf("AbsSubframe %d.%d execute dl slot1 processing \n", frame_rx, nr_tti_rx);

    if (ue->frame_parms.Ncp == 0) {  // normal prefix
      pilot1 = 4;
    } else { // extended prefix
      pilot1 = 3;
    }

    /**** Slot1 FE Processing ****/
#if UE_TIMING_TRACE
    start_meas(&ue->ue_front_end_per_slot_stat[ue->current_thread_id[nr_tti_rx]][1]);
#endif
    // I- start dl slot1 processing
    // do first symbol of next downlink nr_tti_rx for channel estimation
    /*
    // 1- perform FFT for pilot ofdm symbols first (ofdmSym0 next nr_tti_rx ofdmSym11)
    if (nr_subframe_select(&ue->frame_parms,next_nr_tti_rx) != SF_UL)
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
    for (int l=1; l<ue->frame_parms.symbols_per_slot>>1; l++)
      {
	//if( (l != pilot0) && (l != pilot1))
	{
#if UE_TIMING_TRACE
	  start_meas(&ue->ofdm_demod_stats);
#endif
	  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_SLOT_FEP, VCD_FUNCTION_IN);
	  //printf("AbsSubframe %d.%d FFT slot %d, symbol %d\n", frame_rx,nr_tti_rx,slot1,l);
	  front_end_fft(ue,
                        l,
                        slot1,
                        0,
                        0);
	  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_SLOT_FEP, VCD_FUNCTION_OUT);
#if UE_TIMING_TRACE
	  stop_meas(&ue->ofdm_demod_stats);
#endif
	}
      } // for l=1..l2

    if (nr_subframe_select(&ue->frame_parms,next_nr_tti_rx) != SF_UL)
      {
	//printf("AbsSubframe %d.%d FFT slot %d, symbol %d\n", frame_rx,nr_tti_rx,next_subframe_slot0,pilot0);
	front_end_fft(ue,
		      pilot0,
		      next_subframe_slot0,
		      0,
		      0);
      }

    // 2- perform Channel Estimation for slot1
    for (int l=1; l<ue->frame_parms.symbols_per_slot>>1; l++)
      {
	if(l == pilot1)
	  {
	    //wait until channel estimation for pilot0/slot1 is available
	    uint32_t wait = 0;
	    while(proc->chan_est_pilot0_slot1_available == 0)
	      {
		usleep(1);
		wait++;
	      }
	    //printf("[slot1 dl processing] ChanEst symbol %d slot %d wait%d\n",l,slot1,wait);
	  }
	//printf("AbsSubframe %d.%d ChanEst slot %d, symbol %d\n", frame_rx,nr_tti_rx,slot1,l);
	front_end_chanEst(ue,
			  l,
			  slot1,
			  0);
	ue_measurement_procedures(l-1,ue,proc,0,1+(nr_tti_rx<<1),0,ue->mode);
      }
    //printf("AbsSubframe %d.%d ChanEst slot %d, symbol %d\n", frame_rx,nr_tti_rx,next_subframe_slot0,pilot0);
    front_end_chanEst(ue,
		      pilot0,
		      next_subframe_slot0,
		      0);

    if ( (nr_tti_rx == 0) && (ue->decode_MIB == 1))
      {
	ue_pbch_procedures(0,ue,proc,0);
      }

    proc->chan_est_slot1_available = 1;
    //printf("Set available slot 1channelEst to 1 AbsSubframe %d.%d \n",frame_rx,nr_tti_rx);
    //printf(" [slot1 dl processing] ==> FFT/CHanEst Done for AbsSubframe %d.%d \n", proc->frame_rx, proc->nr_tti_rx);

    //printf(" [slot1 dl processing] ==> Start LLR Comuptation slot1 for AbsSubframe %d.%d \n", proc->frame_rx, proc->nr_tti_rx);


#if UE_TIMING_TRACE
    stop_meas(&ue->ue_front_end_per_slot_stat[ue->current_thread_id[nr_tti_rx]][1]);
#if DISABLE_LOG_X
    printf("[AbsSFN %d.%d] Slot1: FFT + Channel Estimate + Pdsch Proc Slot0 %5.2f \n",frame_rx,nr_tti_rx,ue->ue_front_end_per_slot_stat[ue->current_thread_id[nr_tti_rx]][1].p_time/(cpuf*1000.0));
#else
    LOG_D(PHY, "[AbsSFN %d.%d] Slot1: FFT + Channel Estimate + Pdsch Proc Slot0 %5.2f \n",frame_rx,nr_tti_rx,ue->ue_front_end_per_slot_stat[ue->current_thread_id[nr_tti_rx]][1].p_time/(cpuf*1000.0));
#endif
#endif


    //wait until pdcch is decoded
    uint32_t wait = 0;
    while(proc->dci_slot0_available == 0)
      {
        usleep(1);
        wait++;
      }
    //printf("[slot1 dl processing] AbsSubframe %d.%d LLR Computation Start wait DCI %d\n",frame_rx,nr_tti_rx,wait);


    /**** Pdsch Procedure Slot1 ****/
    // start slot1 thread for Pdsch Procedure (slot1)
    // do procedures for C-RNTI
    //printf("AbsSubframe %d.%d Pdsch Procedure (slot1)\n",frame_rx,nr_tti_rx);


#if UE_TIMING_TRACE
    start_meas(&ue->pdsch_procedures_per_slot_stat[ue->current_thread_id[nr_tti_rx]][1]);
#endif
    // start slave thread for Pdsch Procedure (slot1)
    // do procedures for C-RNTI
    uint8_t eNB_id = 0;

    if (ue->dlsch[ue->current_thread_id[nr_tti_rx]][eNB_id][0]->active == 1) {
      //wait until first ofdm symbol is processed
      //wait = 0;
      //while(proc->first_symbol_available == 0)
      //{
      //    usleep(1);
      //    wait++;
      //}
      //printf("[slot1 dl processing] AbsSubframe %d.%d LLR Computation Start wait First Ofdm Sym %d\n",frame_rx,nr_tti_rx,wait);

      //VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PDSCH_PROC, VCD_FUNCTION_IN);
      ue_pdsch_procedures(ue,
			  proc,
			  eNB_id,
			  PDSCH,
			  ue->dlsch[ue->current_thread_id[nr_tti_rx]][eNB_id][0],
			  NULL,
			  (ue->frame_parms.symbols_per_slot>>1),
			  ue->frame_parms.symbols_per_slot-1,
			  abstraction_flag);
      LOG_D(PHY," ------ end PDSCH ChannelComp/LLR slot 0: AbsSubframe %d.%d ------  \n", frame_rx%1024, nr_tti_rx);
      LOG_D(PHY," ------ --> PDSCH Turbo Decoder slot 0/1: AbsSubframe %d.%d ------  \n", frame_rx%1024, nr_tti_rx);
    }

    // do procedures for SI-RNTI
    if ((ue->dlsch_SI[eNB_id]) && (ue->dlsch_SI[eNB_id]->active == 1)) {
      ue_pdsch_procedures(ue,
			  proc,
			  eNB_id,
			  SI_PDSCH,
			  ue->dlsch_SI[eNB_id],
			  NULL,
			  (ue->frame_parms.symbols_per_slot>>1),
			  ue->frame_parms.symbols_per_slot-1,
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
			  (ue->frame_parms.symbols_per_slot>>1),
			  ue->frame_parms.symbols_per_slot-1,
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
			  (ue->frame_parms.symbols_per_slot>>1),
			  ue->frame_parms.symbols_per_slot-1,
			  abstraction_flag);
    }

    proc->llr_slot1_available=1;
    //printf("Set available LLR slot1 to 1 AbsSubframe %d.%d \n",frame_rx,nr_tti_rx);

#if UE_TIMING_TRACE
    stop_meas(&ue->pdsch_procedures_per_slot_stat[ue->current_thread_id[nr_tti_rx]][1]);
#if DISABLE_LOG_X
    printf("[AbsSFN %d.%d] Slot1: LLR Computation %5.2f \n",frame_rx,nr_tti_rx,ue->pdsch_procedures_per_slot_stat[ue->current_thread_id[nr_tti_rx]][1].p_time/(cpuf*1000.0));
#else
    LOG_D(PHY, "[AbsSFN %d.%d] Slot1: LLR Computation %5.2f \n",frame_rx,nr_tti_rx,ue->pdsch_procedures_per_slot_stat[ue->current_thread_id[nr_tti_rx]][1].p_time/(cpuf*1000.0));
#endif
#endif


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


int is_pbch_in_slot(fapi_nr_config_request_t *config, int frame, int slot, NR_DL_FRAME_PARMS *fp)  {

  int ssb_slot_decoded = (fp->ssb_index>>1) + ((fp->ssb_index>>4)<<1); //slot in which the decoded SSB can be found

  if (config->ssb_table.ssb_period == 0) {  
    // check for pbch in corresponding slot each half frame
    if (fp->half_frame_bit)
      return(slot == ssb_slot_decoded || slot == ssb_slot_decoded - fp->slots_per_frame/2);
    else
      return(slot == ssb_slot_decoded || slot == ssb_slot_decoded + fp->slots_per_frame/2);
  }
  else {
    // if the current frame is supposed to contain ssb
    if (!(frame%(1<<(config->ssb_table.ssb_period-1))))
      return(slot == ssb_slot_decoded);
    else
      return 0;
  }
}


int phy_procedures_nrUE_RX(PHY_VARS_NR_UE *ue,
                           UE_nr_rxtx_proc_t *proc,
                           uint8_t eNB_id,
                           uint8_t do_pdcch_flag,
                           runmode_t mode)
{
  int frame_rx = proc->frame_rx;
  int nr_tti_rx = proc->nr_tti_rx;
  int slot_pbch;
  NR_UE_PDCCH *pdcch_vars  = ue->pdcch_vars[ue->current_thread_id[nr_tti_rx]][0];
  NR_UE_DLSCH_t   **dlsch = ue->dlsch[ue->current_thread_id[nr_tti_rx]][eNB_id];
  fapi_nr_config_request_t *cfg = &ue->nrUE_config;
  uint8_t harq_pid = ue->dlsch[ue->current_thread_id[nr_tti_rx]][eNB_id][0]->current_harq_pid;
  NR_DL_UE_HARQ_t *dlsch0_harq = dlsch[0]->harq_processes[harq_pid];
  uint16_t nb_symb_sch = dlsch0_harq->nb_symbols;
  uint16_t start_symb_sch = dlsch0_harq->start_symbol;
  uint8_t nb_symb_pdcch = pdcch_vars->nb_search_space > 0 ? pdcch_vars->pdcch_config[0].coreset.duration : 0;
  uint8_t dci_cnt = 0;
  NR_DL_FRAME_PARMS *fp = &ue->frame_parms;
  
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_UE_RX, VCD_FUNCTION_IN);
  
  LOG_D(PHY," ****** start RX-Chain for Frame.Slot %d.%d ******  \n", frame_rx%1024, nr_tti_rx);

  /*
  uint8_t next1_thread_id = ue->current_thread_id[nr_tti_rx]== (RX_NB_TH-1) ? 0:(ue->current_thread_id[nr_tti_rx]+1);
  uint8_t next2_thread_id = next1_thread_id== (RX_NB_TH-1) ? 0:(next1_thread_id+1);
  */


  int coreset_nb_rb=0;
  int coreset_start_rb=0;
  if (pdcch_vars->nb_search_space > 0)
    get_coreset_rballoc(pdcch_vars->pdcch_config[0].coreset.frequency_domain_resource,&coreset_nb_rb,&coreset_start_rb);
  
  slot_pbch = is_pbch_in_slot(cfg, frame_rx, nr_tti_rx, fp);

  // looking for pbch only in slot where it is supposed to be
  if ((ue->decode_MIB == 1) && slot_pbch)
    {
      LOG_I(PHY," ------  PBCH ChannelComp/LLR: frame.slot %d.%d ------  \n", frame_rx%1024, nr_tti_rx);

      for (int i=1; i<4; i++) {

	nr_slot_fep(ue,
		    (ue->symbol_offset+i)%(fp->symbols_per_slot),
		    nr_tti_rx,
		    0,
		    0);

#if UE_TIMING_TRACE
  	start_meas(&ue->dlsch_channel_estimation_stats);
#endif
   	nr_pbch_channel_estimation(ue,0,nr_tti_rx,(ue->symbol_offset+i)%(fp->symbols_per_slot),i-1,(fp->ssb_index)&7,fp->half_frame_bit);
#if UE_TIMING_TRACE
  	stop_meas(&ue->dlsch_channel_estimation_stats);
#endif
      
      }
      nr_ue_pbch_procedures(eNB_id,ue,proc,0);

      if (ue->no_timing_correction==0) {
        LOG_I(PHY,"start adjust sync slot = %d no timing %d\n", nr_tti_rx, ue->no_timing_correction);
        nr_adjust_synch_ue(fp,
      		           ue,
  			   eNB_id,
  			   nr_tti_rx,
  			   0,
  			   16384);
      }
    }

#ifdef NR_PDCCH_SCHED
  nr_gold_pdcch(ue, 0, 2);

  LOG_D(PHY," ------ --> PDCCH ChannelComp/LLR Frame.slot %d.%d ------  \n", frame_rx%1024, nr_tti_rx);
  for (uint16_t l=0; l<nb_symb_pdcch; l++) {
    
#if UE_TIMING_TRACE
    start_meas(&ue->ofdm_demod_stats);
#endif
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_SLOT_FEP, VCD_FUNCTION_IN);
    nr_slot_fep(ue,
    		    l,
				nr_tti_rx,
				0,
				0);

    // note: this only works if RBs for PDCCH are contigous!
    LOG_D(PHY,"pdcch_channel_estimation: first_carrier_offset %d, BWPStart %d, coreset_start_rb %d\n",
	  fp->first_carrier_offset,pdcch_vars->pdcch_config[0].BWPStart,coreset_start_rb);
    if (coreset_nb_rb > 0)
      nr_pdcch_channel_estimation(ue,
				  0,
				  nr_tti_rx,
				  l,
				  fp->first_carrier_offset+(pdcch_vars->pdcch_config[0].BWPStart + coreset_start_rb)*12,
				  coreset_nb_rb);
    
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_SLOT_FEP, VCD_FUNCTION_OUT);
#if UE_TIMING_TRACE
    stop_meas(&ue->ofdm_demod_stats);
#endif
    
    //printf("phy procedure pdcch start measurement l =%d\n",l);
    //nr_ue_measurement_procedures(l,ue,proc,eNB_id,(nr_tti_rx),mode);
      
  }

  dci_cnt = nr_ue_pdcch_procedures(eNB_id, ue, proc);

  if (dci_cnt > 0) {

    LOG_D(PHY,"[UE  %d] Frame %d, nr_tti_rx %d: found %d DCIs\n",ue->Mod_id,frame_rx,nr_tti_rx,dci_cnt);

  } else {
    LOG_D(PHY,"[UE  %d] Frame %d, nr_tti_rx %d: No DCIs found\n",ue->Mod_id,frame_rx,nr_tti_rx);
  }
#endif //NR_PDCCH_SCHED

  
  if (dci_cnt > 0){
    LOG_D(PHY," ------ --> PDSCH ChannelComp/LLR Frame.slot %d.%d ------  \n", frame_rx%1024, nr_tti_rx);
    //to update from pdsch config
    start_symb_sch = dlsch0_harq->start_symbol;
    int symb_dmrs=-1;
    for (int i=0;i<4;i++) if (((1<<i)&dlsch0_harq->dlDmrsSymbPos) > 0) {symb_dmrs=i;break;}
    AssertFatal(symb_dmrs>=0,"no dmrs in 0..3\n");
    LOG_D(PHY,"Initializing dmrs for symb %d DMRS mask %x\n",symb_dmrs,dlsch0_harq->dlDmrsSymbPos);
    nr_gold_pdsch(ue,symb_dmrs,0, 1);

    nb_symb_sch = dlsch0_harq->nb_symbols;
    
    for (uint16_t m=start_symb_sch;m<(nb_symb_sch+start_symb_sch) ; m++){
      nr_slot_fep(ue,
		  m,  //to be updated from higher layer
		  nr_tti_rx,
		  0,
		  0);
 
      
    }
    //set active for testing, to be removed
    ue->dlsch[ue->current_thread_id[nr_tti_rx]][eNB_id][0]->active = 1;
  }
  else
    ue->dlsch[ue->current_thread_id[nr_tti_rx]][eNB_id][0]->active = 0;

#if UE_TIMING_TRACE
  start_meas(&ue->generic_stat);
#endif
  // do procedures for C-RNTI
  if (ue->dlsch[ue->current_thread_id[nr_tti_rx]][eNB_id][0]->active == 1) {
    //VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PDSCH_PROC, VCD_FUNCTION_IN);
    nr_ue_pdsch_procedures(ue,
			   proc,
			   eNB_id,
			   PDSCH,
			   ue->dlsch[ue->current_thread_id[nr_tti_rx]][eNB_id][0],
			   NULL);
			   
    //printf("phy procedure pdsch start measurement\n"); 
    nr_ue_measurement_procedures(2,ue,proc,eNB_id,nr_tti_rx,mode);

    /*
    write_output("rxF.m","rxF",&ue->common_vars.common_vars_rx_data_per_thread[ue->current_thread_id[nr_tti_rx]].rxdataF[0][0],fp->ofdm_symbol_size*14,1,1);
    write_output("rxF_ch.m","rxFch",&ue->pdsch_vars[ue->current_thread_id[nr_tti_rx]][eNB_id]->dl_ch_estimates[0][0],fp->ofdm_symbol_size*14,1,1);
    write_output("rxF_ch_ext.m","rxFche",&ue->pdsch_vars[ue->current_thread_id[nr_tti_rx]][eNB_id]->dl_ch_estimates_ext[0][2*50*12],50*12,1,1);
    write_output("rxF_ext.m","rxFe",&ue->pdsch_vars[ue->current_thread_id[nr_tti_rx]][eNB_id]->rxdataF_ext[0][0],50*12*14,1,1);
    write_output("rxF_comp.m","rxFc",&ue->pdsch_vars[ue->current_thread_id[nr_tti_rx]][eNB_id]->rxdataF_comp0[0][0],fp->N_RB_DL*12*14,1,1);
    write_output("rxF_llr.m","rxFllr",ue->pdsch_vars[ue->current_thread_id[nr_tti_rx]][eNB_id]->llr[0],(nb_symb_sch-1)*50*12+50*6,1,0);
    */
    
    //VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PDSCH_PROC, VCD_FUNCTION_OUT);
  }

  // do procedures for SI-RNTI
  if ((ue->dlsch_SI[eNB_id]) && (ue->dlsch_SI[eNB_id]->active == 1)) {
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PDSCH_PROC_SI, VCD_FUNCTION_IN);
    nr_ue_pdsch_procedures(ue,
			   proc,
			   eNB_id,
			   SI_PDSCH,
			   ue->dlsch_SI[eNB_id],
			   NULL);
    
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PDSCH_PROC_SI, VCD_FUNCTION_OUT);
  }

  // do procedures for SI-RNTI
  if ((ue->dlsch_p[eNB_id]) && (ue->dlsch_p[eNB_id]->active == 1)) {
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PDSCH_PROC_P, VCD_FUNCTION_IN);
    nr_ue_pdsch_procedures(ue,
			   proc,
			   eNB_id,
			   P_PDSCH,
			   ue->dlsch_p[eNB_id],
			   NULL);

    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PDSCH_PROC_P, VCD_FUNCTION_OUT);
  }

  // do procedures for RA-RNTI
  if ((ue->dlsch_ra[eNB_id]) && (ue->dlsch_ra[eNB_id]->active == 1)) {
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PDSCH_PROC_RA, VCD_FUNCTION_IN);
    nr_ue_pdsch_procedures(ue,
			   proc,
			   eNB_id,
			   RA_PDSCH,
			   ue->dlsch_ra[eNB_id],
			   NULL);

    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PDSCH_PROC_RA, VCD_FUNCTION_OUT);
  }
    
  // do procedures for C-RNTI
  if (ue->dlsch[ue->current_thread_id[nr_tti_rx]][eNB_id][0]->active == 1) {
    
    LOG_D(PHY, "DLSCH data reception at nr_tti_rx: %d \n \n", nr_tti_rx);
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PDSCH_PROC, VCD_FUNCTION_IN);

#if UE_TIMING_TRACE
    start_meas(&ue->dlsch_procedures_stat[ue->current_thread_id[nr_tti_rx]]);
#endif

    nr_ue_dlsch_procedures(ue,
			   proc,
			   eNB_id,
			   PDSCH,
			   ue->dlsch[ue->current_thread_id[nr_tti_rx]][eNB_id][0],
			   ue->dlsch[ue->current_thread_id[nr_tti_rx]][eNB_id][1],
			   &ue->dlsch_errors[eNB_id],
			   mode);


#if UE_TIMING_TRACE
  stop_meas(&ue->dlsch_procedures_stat[ue->current_thread_id[nr_tti_rx]]);
#if DISABLE_LOG_X
  printf("[SFN %d] Slot1:       Pdsch Proc %5.2f\n",nr_tti_rx,ue->pdsch_procedures_stat[ue->current_thread_id[nr_tti_rx]].p_time/(cpuf*1000.0));
  printf("[SFN %d] Slot0 Slot1: Dlsch Proc %5.2f\n",nr_tti_rx,ue->dlsch_procedures_stat[ue->current_thread_id[nr_tti_rx]].p_time/(cpuf*1000.0));
#else
  LOG_D(PHY, "[SFN %d] Slot1:       Pdsch Proc %5.2f\n",nr_tti_rx,ue->pdsch_procedures_stat[ue->current_thread_id[nr_tti_rx]].p_time/(cpuf*1000.0));
  LOG_D(PHY, "[SFN %d] Slot0 Slot1: Dlsch Proc %5.2f\n",nr_tti_rx,ue->dlsch_procedures_stat[ue->current_thread_id[nr_tti_rx]].p_time/(cpuf*1000.0));
#endif

#endif

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PDSCH_PROC, VCD_FUNCTION_OUT);

 }
#if UE_TIMING_TRACE
start_meas(&ue->generic_stat);
#endif

#if 0

  if(nr_tti_rx==5 &&  ue->dlsch[ue->current_thread_id[nr_tti_rx]][eNB_id][0]->harq_processes[ue->dlsch[ue->current_thread_id[nr_tti_rx]][eNB_id][0]->current_harq_pid]->nb_rb > 20){
       //write_output("decoder_llr.m","decllr",dlsch_llr,G,1,0);
       //write_output("llr.m","llr",  &ue->pdsch_vars[eNB_id]->llr[0][0],(14*nb_rb*12*dlsch1_harq->Qm) - 4*(nb_rb*4*dlsch1_harq->Qm),1,0);

       write_output("rxdataF0_current.m"    , "rxdataF0", &ue->common_vars.common_vars_rx_data_per_thread[ue->current_thread_id[nr_tti_rx]].rxdataF[0][0],14*fp->ofdm_symbol_size,1,1);
       //write_output("rxdataF0_previous.m"    , "rxdataF0_prev_sss", &ue->common_vars.common_vars_rx_data_per_thread[next_thread_id].rxdataF[0][0],14*fp->ofdm_symbol_size,1,1);

       //write_output("rxdataF0_previous.m"    , "rxdataF0_prev", &ue->common_vars.common_vars_rx_data_per_thread[next_thread_id].rxdataF[0][0],14*fp->ofdm_symbol_size,1,1);

       write_output("dl_ch_estimates.m", "dl_ch_estimates_sfn5", &ue->common_vars.common_vars_rx_data_per_thread[ue->current_thread_id[nr_tti_rx]].dl_ch_estimates[0][0][0],14*fp->ofdm_symbol_size,1,1);
       write_output("dl_ch_estimates_ext.m", "dl_ch_estimatesExt_sfn5", &ue->pdsch_vars[ue->current_thread_id[nr_tti_rx]][0]->dl_ch_estimates_ext[0][0],14*fp->N_RB_DL*12,1,1);
       write_output("rxdataF_comp00.m","rxdataF_comp00",         &ue->pdsch_vars[ue->current_thread_id[nr_tti_rx]][0]->rxdataF_comp0[0][0],14*fp->N_RB_DL*12,1,1);
       //write_output("magDLFirst.m", "magDLFirst", &phy_vars_ue->pdsch_vars[ue->current_thread_id[nr_tti_rx]][0]->dl_ch_mag0[0][0],14*fp->N_RB_DL*12,1,1);
       //write_output("magDLSecond.m", "magDLSecond", &phy_vars_ue->pdsch_vars[ue->current_thread_id[nr_tti_rx]][0]->dl_ch_magb0[0][0],14*fp->N_RB_DL*12,1,1);

       AssertFatal (0,"");
  }
#endif

  // do procedures for SI-RNTI
  if ((ue->dlsch_SI[eNB_id]) && (ue->dlsch_SI[eNB_id]->active == 1)) {
    nr_ue_pdsch_procedures(ue,
			   proc,
			   eNB_id,
			   SI_PDSCH,
			   ue->dlsch_SI[eNB_id],
			   NULL);

    /*ue_dlsch_procedures(ue,
      proc,
      eNB_id,
      SI_PDSCH,
      ue->dlsch_SI[eNB_id],
      NULL,
      &ue->dlsch_SI_errors[eNB_id],
      mode,
      abstraction_flag);
    ue->dlsch_SI[eNB_id]->active = 0;*/
  }

  // do procedures for P-RNTI
  if ((ue->dlsch_p[eNB_id]) && (ue->dlsch_p[eNB_id]->active == 1)) {
    nr_ue_pdsch_procedures(ue,
			   proc,
			   eNB_id,
			   P_PDSCH,
			   ue->dlsch_p[eNB_id],
			   NULL);


    /*ue_dlsch_procedures(ue,
      proc,
      eNB_id,
      P_PDSCH,
      ue->dlsch_p[eNB_id],
      NULL,
      &ue->dlsch_p_errors[eNB_id],
      mode,
      abstraction_flag);*/
    ue->dlsch_p[eNB_id]->active = 0;
  }
  // do procedures for RA-RNTI
  if ((ue->dlsch_ra[eNB_id]) && (ue->dlsch_ra[eNB_id]->active == 1)) {
    nr_ue_pdsch_procedures(ue,
			   proc,
			   eNB_id,
			   RA_PDSCH,
			   ue->dlsch_ra[eNB_id],
			   NULL);

    /*ue_dlsch_procedures(ue,
      proc,
      eNB_id,
      RA_PDSCH,
      ue->dlsch_ra[eNB_id],
      NULL,
      &ue->dlsch_ra_errors[eNB_id],
      mode,
      abstraction_flag);*/
    ue->dlsch_ra[eNB_id]->active = 0;
  }

  // duplicate harq structure
  /*
  uint8_t          current_harq_pid        = ue->dlsch[ue->current_thread_id[nr_tti_rx]][eNB_id][0]->current_harq_pid;
  NR_DL_UE_HARQ_t *current_harq_processes = ue->dlsch[ue->current_thread_id[nr_tti_rx]][eNB_id][0]->harq_processes[current_harq_pid];
  NR_DL_UE_HARQ_t *harq_processes_dest    = ue->dlsch[next1_thread_id][eNB_id][0]->harq_processes[current_harq_pid];
  NR_DL_UE_HARQ_t *harq_processes_dest1    = ue->dlsch[next2_thread_id][eNB_id][0]->harq_processes[current_harq_pid];
  */
  /*nr_harq_status_t *current_harq_ack = &ue->dlsch[ue->current_thread_id[nr_tti_rx]][eNB_id][0]->harq_ack[nr_tti_rx];
  nr_harq_status_t *harq_ack_dest    = &ue->dlsch[next1_thread_id][eNB_id][0]->harq_ack[nr_tti_rx];
  nr_harq_status_t *harq_ack_dest1    = &ue->dlsch[next2_thread_id][eNB_id][0]->harq_ack[nr_tti_rx];
*/

  //copy_harq_proc_struct(harq_processes_dest, current_harq_processes);
//copy_ack_struct(harq_ack_dest, current_harq_ack);

//copy_harq_proc_struct(harq_processes_dest1, current_harq_processes);
//copy_ack_struct(harq_ack_dest1, current_harq_ack);

if (nr_tti_rx==9) {
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

#if UE_TIMING_TRACE
stop_meas(&ue->generic_stat);
printf("after tubo until end of Rx %5.2f \n",ue->generic_stat.p_time/(cpuf*1000.0));
#endif

#ifdef EMOS
phy_procedures_emos_UE_RX(ue,slot,eNB_id);
#endif


VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_UE_RX, VCD_FUNCTION_OUT);

#if UE_TIMING_TRACE
stop_meas(&ue->phy_proc_rx[ue->current_thread_id[nr_tti_rx]]);
#if DISABLE_LOG_X
printf("------FULL RX PROC [SFN %d]: %5.2f ------\n",nr_tti_rx,ue->phy_proc_rx[ue->current_thread_id[nr_tti_rx]].p_time/(cpuf*1000.0));
#else
LOG_D(PHY, "------FULL RX PROC [SFN %d]: %5.2f ------\n",nr_tti_rx,ue->phy_proc_rx[ue->current_thread_id[nr_tti_rx]].p_time/(cpuf*1000.0));
#endif
#endif

//#endif //pdsch

LOG_D(PHY," ****** end RX-Chain  for AbsSubframe %d.%d ******  \n", frame_rx%1024, nr_tti_rx);
return (0);
}


uint8_t nr_is_cqi_TXOp(PHY_VARS_NR_UE *ue,
		            UE_nr_rxtx_proc_t *proc,
					uint8_t gNB_id)
{
  int subframe = proc->subframe_tx;
  int frame    = proc->frame_tx;
  CQI_REPORTPERIODIC *cqirep = &ue->cqi_report_config[gNB_id].CQI_ReportPeriodic;

  //LOG_I(PHY,"[UE %d][CRNTI %x] AbsSubFrame %d.%d Checking for CQI TXOp (cqi_ConfigIndex %d) isCQIOp %d\n",
  //      ue->Mod_id,ue->pdcch_vars[gNB_id]->crnti,frame,subframe,
  //      cqirep->cqi_PMI_ConfigIndex,
  //      (((10*frame + subframe) % cqirep->Npd) == cqirep->N_OFFSET_CQI));

  if (cqirep->cqi_PMI_ConfigIndex==-1)
    return(0);
  else if (((10*frame + subframe) % cqirep->Npd) == cqirep->N_OFFSET_CQI)
    return(1);
  else
    return(0);
}


uint8_t nr_is_ri_TXOp(PHY_VARS_NR_UE *ue,
		           UE_nr_rxtx_proc_t *proc,
				   uint8_t gNB_id)
{
  int subframe = proc->subframe_tx;
  int frame    = proc->frame_tx;
  CQI_REPORTPERIODIC *cqirep = &ue->cqi_report_config[gNB_id].CQI_ReportPeriodic;
  int log2Mri = cqirep->ri_ConfigIndex/161;
  int N_OFFSET_RI = cqirep->ri_ConfigIndex % 161;

  //LOG_I(PHY,"[UE %d][CRNTI %x] AbsSubFrame %d.%d Checking for RI TXOp (ri_ConfigIndex %d) isRIOp %d\n",
  //      ue->Mod_id,ue->pdcch_vars[gNB_id]->crnti,frame,subframe,
  //      cqirep->ri_ConfigIndex,
  //      (((10*frame + subframe + cqirep->N_OFFSET_CQI - N_OFFSET_RI) % (cqirep->Npd<<log2Mri)) == 0));
  if (cqirep->ri_ConfigIndex==-1)
    return(0);
  else if (((10*frame + subframe + cqirep->N_OFFSET_CQI - N_OFFSET_RI) % (cqirep->Npd<<log2Mri)) == 0)
    return(1);
  else
    return(0);
}

