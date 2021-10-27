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

#include "nr/nr_common.h"
#include "assertions.h"
#include "defs.h"
#include "PHY/defs_nr_UE.h"
#include "PHY/phy_extern_nr_ue.h"
#include "PHY/MODULATION/modulation_UE.h"
#include "PHY/NR_REFSIG/refsig_defs_ue.h"
#include "PHY/NR_UE_TRANSPORT/nr_transport_ue.h"
#include "PHY/NR_UE_TRANSPORT/nr_transport_proto_ue.h"
#include "SCHED_NR_UE/defs.h"
#include "SCHED_NR_UE/pucch_uci_ue_nr.h"
#include "SCHED_NR/extern.h"
#include "SCHED_NR_UE/phy_sch_processing_time.h"
#include "PHY/NR_UE_ESTIMATION/nr_estimation.h"
#include "PHY/NR_TRANSPORT/nr_dci.h"
#ifdef EMOS
#include "SCHED/phy_procedures_emos.h"
#endif
#include "executables/softmodem-common.h"
#include "executables/nr-uesoftmodem.h"
#include "LAYER2/NR_MAC_UE/mac_proto.h"
#include "LAYER2/NR_MAC_UE/nr_l1_helpers.h"

//#define DEBUG_PHY_PROC
#define NR_PDCCH_SCHED
//#define NR_PDCCH_SCHED_DEBUG
//#define NR_PUCCH_SCHED
//#define NR_PUCCH_SCHED_DEBUG

#ifndef PUCCH
#define PUCCH
#endif

#include "common/utils/LOG/log.h"

#ifdef EMOS
fifo_dump_emos_UE emos_dump_UE;
#endif

#include "common/utils/LOG/vcd_signal_dumper.h"
#include "UTIL/OPT/opt.h"
#include "intertask_interface.h"
#include "T.h"

char nr_mode_string[NUM_UE_MODE][20] = {"NOT SYNCHED","PRACH","RAR","RA_WAIT_CR", "PUSCH", "RESYNCH"};

const uint8_t nr_rv_round_map_ue[4] = {0, 2, 1, 3};

#if defined(EXMIMO) || defined(OAI_USRP) || defined(OAI_BLADERF) || defined(OAI_LMSSDR) || defined(OAI_ADRV9371_ZC706)
extern uint64_t downlink_frequency[MAX_NUM_CCs][4];
#endif

unsigned int gain_table[31] = {100,112,126,141,158,178,200,224,251,282,316,359,398,447,501,562,631,708,794,891,1000,1122,1258,1412,1585,1778,1995,2239,2512,2818,3162};

void nr_fill_dl_indication(nr_downlink_indication_t *dl_ind,
                           fapi_nr_dci_indication_t *dci_ind,
                           fapi_nr_rx_indication_t *rx_ind,
                           UE_nr_rxtx_proc_t *proc,
                           PHY_VARS_NR_UE *ue,
                           uint8_t gNB_id){

  memset((void*)dl_ind, 0, sizeof(nr_downlink_indication_t));

  dl_ind->gNB_index = gNB_id;
  dl_ind->module_id = ue->Mod_id;
  dl_ind->cc_id     = ue->CC_id;
  dl_ind->frame     = proc->frame_rx;
  dl_ind->slot      = proc->nr_slot_rx;
  dl_ind->thread_id = proc->thread_id;

  if (dci_ind) {

    dl_ind->rx_ind = NULL; //no data, only dci for now
    dl_ind->dci_ind = dci_ind;

  } else if (rx_ind) {

    dl_ind->rx_ind = rx_ind; //  hang on rx_ind instance
    dl_ind->dci_ind = NULL;

  }
}

void nr_fill_rx_indication(fapi_nr_rx_indication_t *rx_ind,
                           uint8_t pdu_type,
                           uint8_t gNB_id,
                           PHY_VARS_NR_UE *ue,
                           NR_UE_DLSCH_t *dlsch0,
                           NR_UE_DLSCH_t *dlsch1,
                           uint16_t n_pdus){


  NR_DL_FRAME_PARMS *frame_parms = &ue->frame_parms;

  if (n_pdus > 1){
    LOG_E(PHY, "In %s: multiple number of DL PDUs not supported yet...\n", __FUNCTION__);
  }

  switch (pdu_type){
    case FAPI_NR_RX_PDU_TYPE_SIB:
      rx_ind->rx_indication_body[n_pdus - 1].pdsch_pdu.harq_pid = dlsch0->current_harq_pid;
      rx_ind->rx_indication_body[n_pdus - 1].pdsch_pdu.ack_nack = dlsch0->harq_processes[dlsch0->current_harq_pid]->ack;
      rx_ind->rx_indication_body[n_pdus - 1].pdsch_pdu.pdu = dlsch0->harq_processes[dlsch0->current_harq_pid]->b;
      rx_ind->rx_indication_body[n_pdus - 1].pdsch_pdu.pdu_length = dlsch0->harq_processes[dlsch0->current_harq_pid]->TBS / 8;
    break;
    case FAPI_NR_RX_PDU_TYPE_DLSCH:
      if(dlsch0) {
        rx_ind->rx_indication_body[n_pdus - 1].pdsch_pdu.harq_pid = dlsch0->current_harq_pid;
        rx_ind->rx_indication_body[n_pdus - 1].pdsch_pdu.ack_nack = dlsch0->harq_processes[dlsch0->current_harq_pid]->ack;
        rx_ind->rx_indication_body[n_pdus - 1].pdsch_pdu.pdu = dlsch0->harq_processes[dlsch0->current_harq_pid]->b;
        rx_ind->rx_indication_body[n_pdus - 1].pdsch_pdu.pdu_length = dlsch0->harq_processes[dlsch0->current_harq_pid]->TBS / 8;
      }
      if(dlsch1) {
        AssertFatal(1==0,"Second codeword currently not supported\n");
      }
      break;
    case FAPI_NR_RX_PDU_TYPE_RAR:
      rx_ind->rx_indication_body[n_pdus - 1].pdsch_pdu.harq_pid = dlsch0->current_harq_pid;
      rx_ind->rx_indication_body[n_pdus - 1].pdsch_pdu.ack_nack = dlsch0->harq_processes[dlsch0->current_harq_pid]->ack;
      rx_ind->rx_indication_body[n_pdus - 1].pdsch_pdu.pdu = dlsch0->harq_processes[dlsch0->current_harq_pid]->b;
      rx_ind->rx_indication_body[n_pdus - 1].pdsch_pdu.pdu_length = dlsch0->harq_processes[dlsch0->current_harq_pid]->TBS / 8;
    break;
    case FAPI_NR_RX_PDU_TYPE_SSB:
      rx_ind->rx_indication_body[n_pdus - 1].ssb_pdu.pdu = ue->pbch_vars[gNB_id]->decoded_output;
      rx_ind->rx_indication_body[n_pdus - 1].ssb_pdu.additional_bits = ue->pbch_vars[gNB_id]->xtra_byte;
      rx_ind->rx_indication_body[n_pdus - 1].ssb_pdu.ssb_index = (frame_parms->ssb_index)&0x7;
      rx_ind->rx_indication_body[n_pdus - 1].ssb_pdu.ssb_length = frame_parms->Lmax;
      rx_ind->rx_indication_body[n_pdus - 1].ssb_pdu.cell_id = frame_parms->Nid_cell;
      rx_ind->rx_indication_body[n_pdus - 1].ssb_pdu.ssb_start_subcarrier = frame_parms->ssb_start_subcarrier;
      rx_ind->rx_indication_body[n_pdus - 1].ssb_pdu.rsrp_dBm = ue->measurements.rsrp_dBm[gNB_id];
    break;
    default:
    break;
  }

  rx_ind->rx_indication_body[n_pdus -1].pdu_type = pdu_type;
  rx_ind->number_pdus = n_pdus;

}

int get_tx_amp_prach(int power_dBm, int power_max_dBm, int N_RB_UL){

  int gain_dB = power_dBm - power_max_dBm, amp_x_100 = -1;

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
  LOG_E(PHY, "Unknown PRB size %d\n", N_RB_UL);
  return (amp_x_100);
  break;
  }
  if (gain_dB < -30) {
    return (amp_x_100/3162);
  } else if (gain_dB > 0)
    return (amp_x_100);
  else
    return (amp_x_100/gain_table[-gain_dB]);  // 245 corresponds to the factor sqrt(25/6)

  return (amp_x_100);
}

UE_MODE_t get_nrUE_mode(uint8_t Mod_id,uint8_t CC_id,uint8_t gNB_id){
  return(PHY_vars_UE_g[Mod_id][CC_id]->UE_mode[gNB_id]);
}

// convert time factor "16 * 64 * T_c / (2^mu)" in N_TA calculation in TS38.213 section 4.2 to samples by multiplying with samples per second
//   16 * 64 * T_c            / (2^mu) * samples_per_second
// = 16 * T_s                 / (2^mu) * samples_per_second
// = 16 * 1 / (15 kHz * 2048) / (2^mu) * (15 kHz * 2^mu * ofdm_symbol_size)
// = 16 * 1 /           2048           *                  ofdm_symbol_size
// = 16 * ofdm_symbol_size / 2048
static inline
uint16_t get_bw_scaling(uint16_t ofdm_symbol_size){
  return 16 * ofdm_symbol_size / 2048;
}

// UL time alignment procedures:
// - If the current tx frame and slot match the TA configuration in ul_time_alignment
//   then timing advance is processed and set to be applied in the next UL transmission
// - Application of timing adjustment according to TS 38.213 p4.2
// todo:
// - handle RAR TA application as per ch 4.2 TS 38.213
void ue_ta_procedures(PHY_VARS_NR_UE *ue, int slot_tx, int frame_tx){

  if (ue->mac_enabled == 1) {

    uint8_t gNB_id = 0;
    NR_UL_TIME_ALIGNMENT_t *ul_time_alignment = &ue->ul_time_alignment[gNB_id];

    if (frame_tx == ul_time_alignment->ta_frame && slot_tx == ul_time_alignment->ta_slot) {

      uint16_t ofdm_symbol_size = ue->frame_parms.ofdm_symbol_size;
      uint16_t bw_scaling = get_bw_scaling(ofdm_symbol_size);

      ue->timing_advance += (ul_time_alignment->ta_command - 31) * bw_scaling;

      LOG_D(PHY, "In %s: [UE %d] [%d.%d] Got timing advance command %u from MAC, new value is %d\n",
        __FUNCTION__,
        ue->Mod_id,
        frame_tx,
        slot_tx,
        ul_time_alignment->ta_command,
        ue->timing_advance);

      ul_time_alignment->ta_frame = -1;
      ul_time_alignment->ta_slot = -1;

    }
  }
}

void phy_procedures_nrUE_TX(PHY_VARS_NR_UE *ue,
                            UE_nr_rxtx_proc_t *proc,
                            uint8_t gNB_id) {

  int slot_tx = proc->nr_slot_tx;
  int frame_tx = proc->frame_tx;

  AssertFatal(ue->CC_id == 0, "Transmission on secondary CCs is not supported yet\n");

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_UE_TX,VCD_FUNCTION_IN);

  memset(ue->common_vars.txdataF[0], 0, sizeof(int)*14*ue->frame_parms.ofdm_symbol_size);

  LOG_D(PHY,"****** start TX-Chain for AbsSubframe %d.%d ******\n", frame_tx, slot_tx);


  start_meas(&ue->phy_proc_tx);

  if (ue->UE_mode[gNB_id] <= PUSCH){

    for (uint8_t harq_pid = 0; harq_pid < ue->ulsch[proc->thread_id][gNB_id][0]->number_harq_processes_for_pusch; harq_pid++) {
      if (ue->ulsch[proc->thread_id][gNB_id][0]->harq_processes[harq_pid]->status == ACTIVE)
        nr_ue_ulsch_procedures(ue, harq_pid, frame_tx, slot_tx, proc->thread_id, gNB_id);
    }

  }

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_UE_TX, VCD_FUNCTION_OUT);
  stop_meas(&ue->phy_proc_tx);


}

void nr_ue_measurement_procedures(uint16_t l,
                                  PHY_VARS_NR_UE *ue,
                                  UE_nr_rxtx_proc_t *proc,
                                  uint8_t gNB_id,
                                  uint16_t slot){

  NR_DL_FRAME_PARMS *frame_parms=&ue->frame_parms;
  int frame_rx   = proc->frame_rx;
  int nr_slot_rx = proc->nr_slot_rx;
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_MEASUREMENT_PROCEDURES, VCD_FUNCTION_IN);

  if (l==2) {

    LOG_D(PHY,"Doing UE measurement procedures in symbol l %u Ncp %d nr_slot_rx %d, rxdata %p\n",
      l,
      ue->frame_parms.Ncp,
      nr_slot_rx,
      ue->common_vars.rxdata);

    nr_ue_measurements(ue, proc, nr_slot_rx);

#if T_TRACER
    if(slot == 0)
      T(T_UE_PHY_MEAS, T_INT(gNB_id),  T_INT(ue->Mod_id), T_INT(frame_rx%1024), T_INT(nr_slot_rx),
	T_INT((int)(10*log10(ue->measurements.rsrp[0])-ue->rx_total_gain_dB)),
	T_INT((int)ue->measurements.rx_rssi_dBm[0]),
	T_INT((int)(ue->measurements.rx_power_avg_dB[0] - ue->measurements.n0_power_avg_dB)),
	T_INT((int)ue->measurements.rx_power_avg_dB[0]),
	T_INT((int)ue->measurements.n0_power_avg_dB),
	T_INT((int)ue->measurements.wideband_cqi_avg[0]),
	T_INT((int)ue->common_vars.freq_offset));
#endif
  }

  // accumulate and filter timing offset estimation every subframe (instead of every frame)
  if (( slot == 2) && (l==(2-frame_parms->Ncp))) {

    // AGC

    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_GAIN_CONTROL, VCD_FUNCTION_IN);


    //printf("start adjust gain power avg db %d\n", ue->measurements.rx_power_avg_dB[gNB_id]);
    phy_adjust_gain_nr (ue,ue->measurements.rx_power_avg_dB[gNB_id],gNB_id);
    
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_GAIN_CONTROL, VCD_FUNCTION_OUT);

}

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_MEASUREMENT_PROCEDURES, VCD_FUNCTION_OUT);
}

void nr_ue_pbch_procedures(uint8_t gNB_id,
			   PHY_VARS_NR_UE *ue,
			   UE_nr_rxtx_proc_t *proc,
			   uint8_t abstraction_flag)
{
  int ret = 0;

  DevAssert(ue);

  int frame_rx = proc->frame_rx;
  int nr_slot_rx = proc->nr_slot_rx;

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_PBCH_PROCEDURES, VCD_FUNCTION_IN);

  LOG_D(PHY,"[UE  %d] Frame %d Slot %d, Trying PBCH (NidCell %d, gNB_id %d)\n",ue->Mod_id,frame_rx,nr_slot_rx,ue->frame_parms.Nid_cell,gNB_id);

  ret = nr_rx_pbch(ue, proc,
                   ue->pbch_vars[gNB_id],
                   &ue->frame_parms,
                   gNB_id,
                   (ue->frame_parms.ssb_index)&7,
                   SISO);

  if (ret==0) {

    ue->pbch_vars[gNB_id]->pdu_errors_conseq = 0;

    // Switch to PRACH state if it is first PBCH after initial synch and no timing correction is performed
    if (ue->UE_mode[gNB_id] == NOT_SYNCHED && ue->no_timing_correction == 1){
      if (get_softmodem_params()->do_ra) {
        ue->UE_mode[gNB_id] = PRACH;
        ue->prach_resources[gNB_id]->sync_frame = frame_rx;
        ue->prach_resources[gNB_id]->init_msg1 = 0;
      } else {
        ue->UE_mode[gNB_id] = PUSCH;
      }
    }

#ifdef DEBUG_PHY_PROC
    uint16_t frame_tx;
    LOG_D(PHY,"[UE %d] frame %d, nr_slot_rx %d, Received PBCH (MIB): frame_tx %d. N_RB_DL %d\n",
    ue->Mod_id,
    frame_rx,
    nr_slot_rx,
    frame_tx,
    ue->frame_parms.N_RB_DL);
#endif

  } else {
    LOG_E(PHY,"[UE %d] frame %d, nr_slot_rx %d, Error decoding PBCH!\n",
	  ue->Mod_id,frame_rx, nr_slot_rx);
    /*FILE *fd;
    if ((fd = fopen("rxsig_frame0.dat","w")) != NULL) {
                  fwrite((void *)&ue->common_vars.rxdata[0][0],
                         sizeof(int32_t),
                         ue->frame_parms.samples_per_frame,
                         fd);
                  LOG_I(PHY,"Dummping Frame ... bye bye \n");
                  fclose(fd);
                  exit(0);
                }*/

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

    ue->pbch_vars[gNB_id]->pdu_errors_conseq++;
    ue->pbch_vars[gNB_id]->pdu_errors++;

    if (ue->pbch_vars[gNB_id]->pdu_errors_conseq>=100) {
      if (get_softmodem_params()->non_stop) {
        LOG_E(PHY,"More that 100 consecutive PBCH errors! Going back to Sync mode!\n");
        ue->lost_sync = 1;
      } else {
        LOG_E(PHY,"More that 100 consecutive PBCH errors! Exiting!\n");
        exit_fun("More that 100 consecutive PBCH errors! Exiting!\n");
      }
    }
  }

  if (frame_rx % 100 == 0) {
    ue->pbch_vars[gNB_id]->pdu_fer = ue->pbch_vars[gNB_id]->pdu_errors - ue->pbch_vars[gNB_id]->pdu_errors_last;
    ue->pbch_vars[gNB_id]->pdu_errors_last = ue->pbch_vars[gNB_id]->pdu_errors;
  }

#ifdef DEBUG_PHY_PROC
  LOG_D(PHY,"[UE %d] frame %d, slot %d, PBCH errors = %d, consecutive errors = %d!\n",
	ue->Mod_id,frame_rx, nr_slot_rx,
	ue->pbch_vars[gNB_id]->pdu_errors,
	ue->pbch_vars[gNB_id]->pdu_errors_conseq);
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
                           UE_nr_rxtx_proc_t *proc,
                           int n_ss)
{
  int frame_rx = proc->frame_rx;
  int nr_slot_rx = proc->nr_slot_rx;
  unsigned int dci_cnt=0;
  fapi_nr_dci_indication_t *dci_ind = calloc(1, sizeof(*dci_ind));
  nr_downlink_indication_t dl_indication;

  NR_UE_PDCCH *pdcch_vars = ue->pdcch_vars[proc->thread_id][gNB_id];
  fapi_nr_dl_config_dci_dl_pdu_rel15_t *rel15 = &pdcch_vars->pdcch_config[n_ss];

  /*
  //  unsigned int dci_cnt=0, i;  //removed for nr_ue_pdcch_procedures and added in the loop for nb_coreset_active
#ifdef NR_PDCCH_SCHED_DEBUG
  printf("<-NR_PDCCH_PHY_PROCEDURES_LTE_UE (nr_ue_pdcch_procedures)-> Entering function nr_ue_pdcch_procedures() \n");
#endif

  int frame_rx = proc->frame_rx;
  int nr_slot_rx = proc->nr_slot_rx;
  NR_DCI_ALLOC_t dci_alloc_rx[8];
  
  //uint8_t next1_thread_id = proc->thread_id== (RX_NB_TH-1) ? 0:(proc->thread_id+1);
  //uint8_t next2_thread_id = next1_thread_id== (RX_NB_TH-1) ? 0:(next1_thread_id+1);
  

  // table dci_fields_sizes_cnt contains dci_fields_sizes for each time a dci is decoded in the slot
  // each element represents the size in bits for each dci field, for each decoded dci -> [dci_cnt-1]
  // each time a dci is decode at dci_cnt, the values of the table dci_fields_sizes[i][j] will be copied at table dci_fields_sizes_cnt[dci_cnt-1][i][j]
  // table dci_fields_sizes_cnt[dci_cnt-1][i][j] will then be used in function nr_extract_dci_info
  uint8_t dci_fields_sizes_cnt[MAX_NR_DCI_DECODED_SLOT][NBR_NR_DCI_FIELDS][NBR_NR_FORMATS];

  int nb_searchspace_active=0;
  NR_UE_PDCCH **pdcch_vars = ue->pdcch_vars[proc->thread_id];
  NR_UE_PDCCH *pdcch_vars2 = ue->pdcch_vars[proc->thread_id][gNB_id];
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
	 nr_slot_rx,nb_searchspace_total);
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
     // Verify that monitoring is required at the slot nr_slot_rx. We will run pdcch procedure only if do_pdcch_monitoring_current_slot=1
     // For Type0-PDCCH searchspace, we need to calculate the monitoring slot from Tables 13-1 .. 13-15 in TS 38.213 Subsection 13
     //NR_UE_SLOT_PERIOD_OFFSET_t sl_period_offset_mon = pdcch_vars2->searchSpace[nb_searchspace_active].monitoringSlotPeriodicityAndOffset;
     //if (sl_period_offset_mon == nr_sl1) {
     //do_pdcch_monitoring_current_slot=1; // PDCCH monitoring in every slot
     //} else if (nr_slot_rx%(uint16_t)sl_period_offset_mon == pdcch_vars2->searchSpace[nb_searchspace_active].monitoringSlotPeriodicityAndOffset_offset) {
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
      // the searchSpace indicates that we need to monitor PDCCH in current nr_slot_rx
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
    start_meas(&ue->dlsch_rx_pdcch_stats);

      VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_RX_PDCCH, VCD_FUNCTION_IN);
#ifdef NR_PDCCH_SCHED_DEBUG
      printf("<-NR_PDCCH_PHY_PROCEDURES_LTE_UE (nr_ue_pdcch_procedures)-> Entering function nr_rx_pdcch with gNB_id=%d (nb_coreset_active=%d, (symbol_within_slot_mon&0x3FFF)=%d, searchSpaceType=%d)\n",
                  gNB_id,nb_coreset_active,(symbol_within_slot_mon&0x3FFF),
                  searchSpaceType);
#endif
        nr_rx_pdcch(ue,
                    frame_rx,
                    nr_slot_rx,
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
  nr_rx_pdcch(ue, proc, rel15);
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_RX_PDCCH, VCD_FUNCTION_OUT);
  

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_DCI_DECODING, VCD_FUNCTION_IN);

#ifdef NR_PDCCH_SCHED_DEBUG
  printf("<-NR_PDCCH_PHY_PROCEDURES_LTE_UE (nr_ue_pdcch_procedures)-> Entering function nr_dci_decoding_procedure for search space %d)\n",
	 n_ss);
#endif

  dci_cnt = nr_dci_decoding_procedure(ue, proc, dci_ind, rel15);

#ifdef NR_PDCCH_SCHED_DEBUG
  LOG_I(PHY,"<-NR_PDCCH_PHY_PROCEDURES_LTE_UE (nr_ue_pdcch_procedures)-> Ending function nr_dci_decoding_procedure() -> dci_cnt=%u\n",dci_cnt);
#endif
  
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_DCI_DECODING, VCD_FUNCTION_OUT);
  //LOG_D(PHY,"[UE  %d][PUSCH] Frame %d nr_slot_rx %d PHICH RX\n",ue->Mod_id,frame_rx,nr_slot_rx);

  for (int i=0; i<dci_cnt; i++) {
    LOG_D(PHY,"[UE  %d] AbsSubFrame %d.%d, Mode %s: DCI %i of %d total DCIs found --> rnti %x : format %d\n",
      ue->Mod_id,frame_rx%1024,nr_slot_rx,nr_mode_string[ue->UE_mode[gNB_id]],
      i + 1,
      dci_cnt,
      dci_ind->dci_list[i].rnti,
      dci_ind->dci_list[i].dci_format);
  }
  ue->pdcch_vars[proc->thread_id][gNB_id]->dci_received += dci_cnt;

  dci_ind->number_of_dcis = dci_cnt;
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
				     nr_slot_rx,
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
    nr_fill_dl_indication(&dl_indication, dci_ind, NULL, proc, ue, gNB_id);
    //  send to mac
    ue->if_inst->dl_indication(&dl_indication, NULL);

  stop_meas(&ue->dlsch_rx_pdcch_stats);
    
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_PDCCH_PROCEDURES, VCD_FUNCTION_OUT);
  return(dci_cnt);
}
#endif // NR_PDCCH_SCHED

int nr_ue_pdsch_procedures(PHY_VARS_NR_UE *ue, UE_nr_rxtx_proc_t *proc, int gNB_id, PDSCH_t pdsch, NR_UE_DLSCH_t *dlsch0, NR_UE_DLSCH_t *dlsch1) {

  int frame_rx = proc->frame_rx;
  int nr_slot_rx = proc->nr_slot_rx;
  int m;
  int i_mod,gNB_id_i,dual_stream_UE;
  int first_symbol_flag=0;

  if (!dlsch0)
    return 0;
  if (dlsch0->active == 0)
    return 0;

  if (!dlsch1)  {
    int harq_pid = dlsch0->current_harq_pid;
    NR_DL_UE_HARQ_t *dlsch0_harq = dlsch0->harq_processes[harq_pid];
    uint16_t BWPStart       = dlsch0_harq->BWPStart;
    uint16_t pdsch_start_rb = dlsch0_harq->start_rb;
    uint16_t pdsch_nb_rb    = dlsch0_harq->nb_rb;
    uint16_t s0             = dlsch0_harq->start_symbol;
    uint16_t s1             = dlsch0_harq->nb_symbols;
    bool is_SI              = dlsch0->rnti_type == _SI_RNTI_;

    LOG_D(PHY,"[UE %d] PDSCH type %d active in nr_slot_rx %d, harq_pid %d (%d), rb_start %d, nb_rb %d, symbol_start %d, nb_symbols %d, DMRS mask %x\n",ue->Mod_id,pdsch,nr_slot_rx,harq_pid,dlsch0_harq->status,pdsch_start_rb,pdsch_nb_rb,s0,s1,dlsch0_harq->dlDmrsSymbPos);

    for (m = s0; m < (s0 +s1); m++) {
      if (dlsch0_harq->dlDmrsSymbPos & (1 << m)) {
        for (uint8_t aatx=0; aatx<dlsch0_harq->Nl; aatx++) {//for MIMO Config: it shall loop over no_layers
          nr_pdsch_channel_estimation(ue,
                                      proc,
                                      gNB_id,
                                      is_SI,
                                      nr_slot_rx,
                                      aatx /*p*/,
                                      m,
                                      BWPStart,
                                      ue->frame_parms.first_carrier_offset+(BWPStart + pdsch_start_rb)*12,
                                      pdsch_nb_rb);
          LOG_D(PHY,"PDSCH Channel estimation gNB id %d, PDSCH antenna port %d, slot %d, symbol %d\n",0,aatx,nr_slot_rx,m);
#if 0
          ///LOG_M: the channel estimation
          int nr_frame_rx = proc->frame_rx;
          char filename[100];
          for (uint8_t aarx=0; aarx<ue->frame_parms.nb_antennas_rx; aarx++) {
            sprintf(filename,"PDSCH_CHANNEL_frame%d_slot%d_sym%d_port%d_rx%d.m", nr_frame_rx, nr_slot_rx, m, aatx,aarx);
            int **dl_ch_estimates = ue->pdsch_vars[proc->thread_id][gNB_id]->dl_ch_estimates;
            LOG_M(filename,"channel_F",&dl_ch_estimates[aatx*ue->frame_parms.nb_antennas_rx+aarx][ue->frame_parms.ofdm_symbol_size*m],ue->frame_parms.ofdm_symbol_size, 1, 1);
          }
#endif
        }
      }
    }

    uint16_t first_symbol_with_data = s0;
    uint32_t dmrs_data_re;

    if (dlsch0_harq->dmrsConfigType == NFAPI_NR_DMRS_TYPE1)
      dmrs_data_re = 12 - 6 * dlsch0_harq->n_dmrs_cdm_groups;
    else
      dmrs_data_re = 12 - 4 * dlsch0_harq->n_dmrs_cdm_groups;

    while ((dmrs_data_re == 0) && (dlsch0_harq->dlDmrsSymbPos & (1 << first_symbol_with_data))) {
      first_symbol_with_data++;
    }

    for (m = s0; m < (s1 + s0); m++) {
 
      dual_stream_UE = 0;
      gNB_id_i = gNB_id+1;
      i_mod = 0;
      if (( m==first_symbol_with_data ) && (m<4))
        first_symbol_flag = 1;
      else
        first_symbol_flag = 0;

      uint8_t slot = 0;
      if(m >= ue->frame_parms.symbols_per_slot>>1)
        slot = 1;
      start_meas(&ue->dlsch_llr_stats_parallelization[proc->thread_id][slot]);
      // process DLSCH received symbols in the slot
      // symbol by symbol processing (if data/DMRS are multiplexed is checked inside the function)
      if (pdsch == PDSCH || pdsch == SI_PDSCH || pdsch == RA_PDSCH) {
        if (nr_rx_pdsch(ue,
                        proc,
                        pdsch,
                        gNB_id,
                        gNB_id_i,
                        frame_rx,
                        nr_slot_rx,
                        m,
                        first_symbol_flag,
                        dual_stream_UE,
                        i_mod,
                        harq_pid) < 0)
          return -1;
      } else AssertFatal(1==0,"Not RA_PDSCH, SI_PDSCH or PDSCH\n");

      stop_meas(&ue->dlsch_llr_stats_parallelization[proc->thread_id][slot]);
#if PHYSIM
      printf("[AbsSFN %d.%d] LLR Computation Symbol %d %5.2f \n",frame_rx,nr_slot_rx,m,ue->dlsch_llr_stats_parallelization[proc->thread_id][slot].p_time/(cpuf*1000.0));
#else
      LOG_D(PHY, "[AbsSFN %d.%d] LLR Computation Symbol %d %5.2f \n",frame_rx,nr_slot_rx,m,ue->dlsch_llr_stats_parallelization[proc->thread_id][slot].p_time/(cpuf*1000.0));
#endif

      if(first_symbol_flag) {
        proc->first_symbol_available = 1;
      }
    } // CRNTI active
  }
  return 0;
}

bool nr_ue_dlsch_procedures(PHY_VARS_NR_UE *ue,
                            UE_nr_rxtx_proc_t *proc,
                            int gNB_id,
                            PDSCH_t pdsch,
                            NR_UE_DLSCH_t *dlsch0,
                            NR_UE_DLSCH_t *dlsch1,
                            int *dlsch_errors,
                            uint8_t dlsch_parallel) {

  if (dlsch0==NULL)
    AssertFatal(0,"dlsch0 should be defined at this level \n");
  bool dec = false;
  int harq_pid = dlsch0->current_harq_pid;
  int frame_rx = proc->frame_rx;
  int nr_slot_rx = proc->nr_slot_rx;
  uint32_t ret = UINT32_MAX, ret1 = UINT32_MAX;
  NR_UE_PDSCH *pdsch_vars;
  uint16_t dmrs_len = get_num_dmrs(dlsch0->harq_processes[dlsch0->current_harq_pid]->dlDmrsSymbPos);
  nr_downlink_indication_t dl_indication;
  fapi_nr_rx_indication_t *rx_ind = calloc(1, sizeof(*rx_ind));
  uint16_t number_pdus = 1;
  // params for UL time alignment procedure
  NR_UL_TIME_ALIGNMENT_t *ul_time_alignment = &ue->ul_time_alignment[gNB_id];

  uint8_t is_cw0_active = dlsch0->harq_processes[harq_pid]->status;
  uint16_t nb_symb_sch = dlsch0->harq_processes[harq_pid]->nb_symbols;
  uint16_t start_symbol = dlsch0->harq_processes[harq_pid]->start_symbol;
  uint8_t dmrs_type = dlsch0->harq_processes[harq_pid]->dmrsConfigType;

  uint8_t nb_re_dmrs;
  if (dmrs_type==NFAPI_NR_DMRS_TYPE1) {
    nb_re_dmrs = 6*dlsch0->harq_processes[harq_pid]->n_dmrs_cdm_groups;
  }
  else {
    nb_re_dmrs = 4*dlsch0->harq_processes[harq_pid]->n_dmrs_cdm_groups;
  }

  uint8_t is_cw1_active = 0;
  if(dlsch1)
    is_cw1_active = dlsch1->harq_processes[harq_pid]->status;

  LOG_D(PHY,"AbsSubframe %d.%d Start LDPC Decoder for CW0 [harq_pid %d] ? %d \n", frame_rx%1024, nr_slot_rx, harq_pid, is_cw0_active);
  LOG_D(PHY,"AbsSubframe %d.%d Start LDPC Decoder for CW1 [harq_pid %d] ? %d \n", frame_rx%1024, nr_slot_rx, harq_pid, is_cw1_active);

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
    case RA_PDSCH:
    case P_PDSCH:
    case PDSCH:
      pdsch_vars = ue->pdsch_vars[proc->thread_id][gNB_id];
      break;
    case PMCH:
    case PDSCH1:
      LOG_E(PHY,"Illegal PDSCH %d for ue_pdsch_procedures\n",pdsch);
      pdsch_vars = NULL;
      return false;
      break;
    default:
      pdsch_vars = NULL;
      return false;
      break;

    }
    if (frame_rx < *dlsch_errors)
      *dlsch_errors=0;

    if (pdsch == RA_PDSCH) {
      if (ue->prach_resources[gNB_id]!=NULL)
        dlsch0->rnti = ue->prach_resources[gNB_id]->ra_RNTI;
      else {
        LOG_E(PHY,"[UE %d] Frame %d, nr_slot_rx %d: FATAL, prach_resources is NULL\n", ue->Mod_id, frame_rx, nr_slot_rx);
        //mac_xface->macphy_exit("prach_resources is NULL");
        return false;
      }
    }

    // exit dlsch procedures as there are no active dlsch
    if (is_cw0_active != ACTIVE && is_cw1_active != ACTIVE)
      return false;

    // start ldpc decode for CW 0
    dlsch0->harq_processes[harq_pid]->G = nr_get_G(dlsch0->harq_processes[harq_pid]->nb_rb,
                                                   nb_symb_sch,
                                                   nb_re_dmrs,
                                                   dmrs_len,
                                                   dlsch0->harq_processes[harq_pid]->Qm,
                                                   dlsch0->harq_processes[harq_pid]->Nl);

      start_meas(&ue->dlsch_unscrambling_stats);
      nr_dlsch_unscrambling(pdsch_vars->llr[0],
                            dlsch0->harq_processes[harq_pid]->G,
                            0,
                            ue->frame_parms.Nid_cell,
                            dlsch0->rnti);
      

      stop_meas(&ue->dlsch_unscrambling_stats);


#if 0
      LOG_I(PHY," ------ start ldpc decoder for AbsSubframe %d.%d / %d  ------  \n", frame_rx, nr_slot_rx, harq_pid);
      LOG_I(PHY,"start ldpc decode for CW 0 for AbsSubframe %d.%d / %d --> nb_rb %d \n", frame_rx, nr_slot_rx, harq_pid, dlsch0->harq_processes[harq_pid]->nb_rb);
      LOG_I(PHY,"start ldpc decode for CW 0 for AbsSubframe %d.%d / %d  --> rb_alloc_even %x \n", frame_rx, nr_slot_rx, harq_pid, dlsch0->harq_processes[harq_pid]->rb_alloc_even);
      LOG_I(PHY,"start ldpc decode for CW 0 for AbsSubframe %d.%d / %d  --> Qm %d \n", frame_rx, nr_slot_rx, harq_pid, dlsch0->harq_processes[harq_pid]->Qm);
      LOG_I(PHY,"start ldpc decode for CW 0 for AbsSubframe %d.%d / %d  --> Nl %d \n", frame_rx, nr_slot_rx, harq_pid, dlsch0->harq_processes[harq_pid]->Nl);
      LOG_I(PHY,"start ldpc decode for CW 0 for AbsSubframe %d.%d / %d  --> G  %d \n", frame_rx, nr_slot_rx, harq_pid, dlsch0->harq_processes[harq_pid]->G);
      LOG_I(PHY,"start ldpc decode for CW 0 for AbsSubframe %d.%d / %d  --> Kmimo  %d \n", frame_rx, nr_slot_rx, harq_pid, dlsch0->Kmimo);
      LOG_I(PHY,"start ldpc decode for CW 0 for AbsSubframe %d.%d / %d  --> Pdcch Sym  %d \n", frame_rx, nr_slot_rx, harq_pid, ue->pdcch_vars[proc->thread_id][gNB_id]->num_pdcch_symbols);
#endif


   start_meas(&ue->dlsch_decoding_stats[proc->thread_id]);

    if( dlsch_parallel) {
      ret = nr_dlsch_decoding_mthread(ue,
                                      proc,
                                      gNB_id,
                                      pdsch_vars->llr[0],
                                      &ue->frame_parms,
                                      dlsch0,
                                      dlsch0->harq_processes[harq_pid],
                                      frame_rx,
                                      nb_symb_sch,
                                      nr_slot_rx,
                                      harq_pid,
                                      pdsch==PDSCH?1:0,
                                      dlsch0->harq_processes[harq_pid]->TBS>256?1:0);

      LOG_T(PHY,"dlsch decoding is parallelized, ret = %d\n", ret);
    }
    else {
      ret = nr_dlsch_decoding(ue,
                              proc,
                              gNB_id,
                              pdsch_vars->llr[0],
                              &ue->frame_parms,
                              dlsch0,
                              dlsch0->harq_processes[harq_pid],
                              frame_rx,
                              nb_symb_sch,
                              nr_slot_rx,
                              harq_pid,
                              pdsch==PDSCH?1:0,
                              dlsch0->harq_processes[harq_pid]->TBS>256?1:0);
      LOG_T(PHY,"Sequential dlsch decoding , ret = %d\n", ret);
    }

    if(ret<dlsch0->max_ldpc_iterations+1)
      dec = true;

    switch (pdsch) {
      case RA_PDSCH:
        nr_fill_dl_indication(&dl_indication, NULL, rx_ind, proc, ue, gNB_id);
        nr_fill_rx_indication(rx_ind, FAPI_NR_RX_PDU_TYPE_RAR, gNB_id, ue, dlsch0, NULL, number_pdus);
        ue->UE_mode[gNB_id] = RA_RESPONSE;
        break;
      case PDSCH:
        nr_fill_dl_indication(&dl_indication, NULL, rx_ind, proc, ue, gNB_id);
        nr_fill_rx_indication(rx_ind, FAPI_NR_RX_PDU_TYPE_DLSCH, gNB_id, ue, dlsch0, NULL, number_pdus);
        break;
      case SI_PDSCH:
        nr_fill_dl_indication(&dl_indication, NULL, rx_ind, proc, ue, gNB_id);
        nr_fill_rx_indication(rx_ind, FAPI_NR_RX_PDU_TYPE_SIB, gNB_id, ue, dlsch0, NULL, number_pdus);
        break;
      default:
        break;
    }

    LOG_D(PHY, "In %s DL PDU length in bits: %d, in bytes: %d \n", __FUNCTION__, dlsch0->harq_processes[harq_pid]->TBS, dlsch0->harq_processes[harq_pid]->TBS / 8);




      stop_meas(&ue->dlsch_decoding_stats[proc->thread_id]);
#if PHYSIM
    printf(" --> Unscrambling for CW0 %5.3f\n",
           (ue->dlsch_unscrambling_stats.p_time)/(cpuf*1000.0));
    printf("AbsSubframe %d.%d --> LDPC Decoding for CW0 %5.3f\n",
           frame_rx%1024, nr_slot_rx,(ue->dlsch_decoding_stats[proc->thread_id].p_time)/(cpuf*1000.0));
#else
    LOG_I(PHY, " --> Unscrambling for CW0 %5.3f\n",
          (ue->dlsch_unscrambling_stats.p_time)/(cpuf*1000.0));
    LOG_I(PHY, "AbsSubframe %d.%d --> LDPC Decoding for CW0 %5.3f\n",
          frame_rx%1024, nr_slot_rx,(ue->dlsch_decoding_stats[proc->thread_id].p_time)/(cpuf*1000.0));
#endif

    if(is_cw1_active) {
      // start ldpc decode for CW 1
      dlsch1->harq_processes[harq_pid]->G = nr_get_G(dlsch1->harq_processes[harq_pid]->nb_rb,
                                                     nb_symb_sch,
                                                     nb_re_dmrs,
                                                     dmrs_len,
                                                     dlsch1->harq_processes[harq_pid]->Qm,
                                                     dlsch1->harq_processes[harq_pid]->Nl);
      start_meas(&ue->dlsch_unscrambling_stats);
      nr_dlsch_unscrambling(pdsch_vars->llr[1],
                            dlsch1->harq_processes[harq_pid]->G,
                            0,
                            ue->frame_parms.Nid_cell,
                            dlsch1->rnti);
      stop_meas(&ue->dlsch_unscrambling_stats);

#if 0
          LOG_I(PHY,"start ldpc decode for CW 1 for AbsSubframe %d.%d / %d --> nb_rb %d \n", frame_rx, nr_slot_rx, harq_pid, dlsch1->harq_processes[harq_pid]->nb_rb);
          LOG_I(PHY,"start ldpc decode for CW 1 for AbsSubframe %d.%d / %d  --> rb_alloc_even %x \n", frame_rx, nr_slot_rx, harq_pid, dlsch1->harq_processes[harq_pid]->rb_alloc_even);
          LOG_I(PHY,"start ldpc decode for CW 1 for AbsSubframe %d.%d / %d  --> Qm %d \n", frame_rx, nr_slot_rx, harq_pid, dlsch1->harq_processes[harq_pid]->Qm);
          LOG_I(PHY,"start ldpc decode for CW 1 for AbsSubframe %d.%d / %d  --> Nl %d \n", frame_rx, nr_slot_rx, harq_pid, dlsch1->harq_processes[harq_pid]->Nl);
          LOG_I(PHY,"start ldpc decode for CW 1 for AbsSubframe %d.%d / %d  --> G  %d \n", frame_rx, nr_slot_rx, harq_pid, dlsch1->harq_processes[harq_pid]->G);
          LOG_I(PHY,"start ldpc decode for CW 1 for AbsSubframe %d.%d / %d  --> Kmimo  %d \n", frame_rx, nr_slot_rx, harq_pid, dlsch1->Kmimo);
          LOG_I(PHY,"start ldpc decode for CW 1 for AbsSubframe %d.%d / %d  --> Pdcch Sym  %d \n", frame_rx, nr_slot_rx, harq_pid, ue->pdcch_vars[proc->thread_id][gNB_id]->num_pdcch_symbols);
#endif

      start_meas(&ue->dlsch_decoding_stats[proc->thread_id]);


      if(dlsch_parallel) {
        ret1 = nr_dlsch_decoding_mthread(ue,
                                         proc,
                                         gNB_id,
                                         pdsch_vars->llr[1],
                                         &ue->frame_parms,
                                         dlsch1,
                                         dlsch1->harq_processes[harq_pid],
                                         frame_rx,
                                         nb_symb_sch,
				         nr_slot_rx,
                                         harq_pid,
                                         pdsch==PDSCH?1:0,
                                         dlsch1->harq_processes[harq_pid]->TBS>256?1:0);
        LOG_T(PHY,"CW dlsch decoding is parallelized, ret1 = %d\n", ret1);
      }
      else {
        ret1 = nr_dlsch_decoding(ue,
                                 proc,
                                 gNB_id,
                                 pdsch_vars->llr[1],
                                 &ue->frame_parms,
                                 dlsch1,
                                 dlsch1->harq_processes[harq_pid],
                                 frame_rx,
                                 nb_symb_sch,
                                 nr_slot_rx,
                                 harq_pid,
                                 pdsch==PDSCH?1:0,//proc->decoder_switch,
                                 dlsch1->harq_processes[harq_pid]->TBS>256?1:0);
        LOG_T(PHY,"CWW sequential dlsch decoding, ret1 = %d\n", ret1);
      }


    stop_meas(&ue->dlsch_decoding_stats[proc->thread_id]);
#if PHYSIM
      printf(" --> Unscrambling for CW1 %5.3f\n",
             (ue->dlsch_unscrambling_stats.p_time)/(cpuf*1000.0));
      printf("AbsSubframe %d.%d --> ldpc Decoding for CW1 %5.3f\n",
             frame_rx%1024, nr_slot_rx,(ue->dlsch_decoding_stats[proc->thread_id].p_time)/(cpuf*1000.0));
#else
      LOG_D(PHY, " --> Unscrambling for CW1 %5.3f\n",
            (ue->dlsch_unscrambling_stats.p_time)/(cpuf*1000.0));
      LOG_D(PHY, "AbsSubframe %d.%d --> ldpc Decoding for CW1 %5.3f\n",
            frame_rx%1024, nr_slot_rx,(ue->dlsch_decoding_stats[proc->thread_id].p_time)/(cpuf*1000.0));
#endif


      LOG_D(PHY,"AbsSubframe %d.%d --> ldpc Decoding for CW1 %5.3f\n",
            frame_rx%1024, nr_slot_rx,(ue->dlsch_decoding_stats[proc->thread_id].p_time)/(cpuf*1000.0));

      LOG_D(PHY, "harq_pid: %d, TBS expected dlsch1: %d \n", harq_pid, dlsch1->harq_processes[harq_pid]->TBS);
    }

    LOG_D(PHY," ------ end ldpc decoder for AbsSubframe %d.%d ------ decoded in %d \n", frame_rx, nr_slot_rx, ret);

    //  send to mac
    if (ue->if_inst && ue->if_inst->dl_indication) {
      ue->if_inst->dl_indication(&dl_indication, ul_time_alignment);
    }

    if (ue->mac_enabled == 1) { // TODO: move this from PHY to MAC layer!

      /* Time Alignment procedure
      // - UE processing capability 1
      // - Setting the TA update to be applied after the reception of the TA command
      // - Timing adjustment computed according to TS 38.213 section 4.2
      // - Durations of N1 and N2 symbols corresponding to PDSCH and PUSCH are
      //   computed according to sections 5.3 and 6.4 of TS 38.214 */
      const int numerology = ue->frame_parms.numerology_index;
      const int ofdm_symbol_size = ue->frame_parms.ofdm_symbol_size;
      const int nb_prefix_samples = ue->frame_parms.nb_prefix_samples;
      const int samples_per_subframe = ue->frame_parms.samples_per_subframe;
      const int slots_per_frame = ue->frame_parms.slots_per_frame;
      const int slots_per_subframe = ue->frame_parms.slots_per_subframe;

      const double tc_factor = 1.0 / samples_per_subframe;
      const uint16_t bw_scaling = get_bw_scaling(ofdm_symbol_size);

      const int Ta_max = 3846; // Max value of 12 bits TA Command
      const double N_TA_max = Ta_max * bw_scaling * tc_factor;

      NR_UE_MAC_INST_t *mac = get_mac_inst(0);

      NR_PUSCH_TimeDomainResourceAllocationList_t *pusch_TimeDomainAllocationList = NULL;
      if (mac->ULbwp[0] &&
          mac->ULbwp[0]->bwp_Dedicated &&
          mac->ULbwp[0]->bwp_Dedicated->pusch_Config &&
          mac->ULbwp[0]->bwp_Dedicated->pusch_Config->choice.setup &&
          mac->ULbwp[0]->bwp_Dedicated->pusch_Config->choice.setup->pusch_TimeDomainAllocationList) {
        pusch_TimeDomainAllocationList = mac->ULbwp[0]->bwp_Dedicated->pusch_Config->choice.setup->pusch_TimeDomainAllocationList->choice.setup;
      }
      else if (mac->ULbwp[0] &&
               mac->ULbwp[0]->bwp_Common &&
               mac->ULbwp[0]->bwp_Common->pusch_ConfigCommon &&
               mac->ULbwp[0]->bwp_Common->pusch_ConfigCommon->choice.setup &&
               mac->ULbwp[0]->bwp_Common->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList) {
        pusch_TimeDomainAllocationList = mac->ULbwp[0]->bwp_Common->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList;
      }
      else if (mac->scc_SIB &&
               mac->scc_SIB->uplinkConfigCommon &&
               mac->scc_SIB->uplinkConfigCommon->initialUplinkBWP.pusch_ConfigCommon &&
               mac->scc_SIB->uplinkConfigCommon->initialUplinkBWP.pusch_ConfigCommon->choice.setup &&
               mac->scc_SIB->uplinkConfigCommon->initialUplinkBWP.pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList) {
        pusch_TimeDomainAllocationList = mac->scc_SIB->uplinkConfigCommon->initialUplinkBWP.pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList;
      }
      long mapping_type_ul = pusch_TimeDomainAllocationList ? pusch_TimeDomainAllocationList->list.array[0]->mappingType : NR_PUSCH_TimeDomainResourceAllocation__mappingType_typeA;

      NR_PDSCH_Config_t *pdsch_Config = (mac->DLbwp[0] && mac->DLbwp[0]->bwp_Dedicated->pdsch_Config->choice.setup) ? mac->DLbwp[0]->bwp_Dedicated->pdsch_Config->choice.setup : NULL;
      NR_PDSCH_TimeDomainResourceAllocationList_t *pdsch_TimeDomainAllocationList = NULL;
      if (mac->DLbwp[0] && mac->DLbwp[0]->bwp_Dedicated->pdsch_Config->choice.setup->pdsch_TimeDomainAllocationList)
        pdsch_TimeDomainAllocationList = pdsch_Config->pdsch_TimeDomainAllocationList->choice.setup;
      else if (mac->DLbwp[0] && mac->DLbwp[0]->bwp_Common->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList)
        pdsch_TimeDomainAllocationList = mac->DLbwp[0]->bwp_Common->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList;
      else if (mac->scc_SIB && mac->scc_SIB->downlinkConfigCommon.initialDownlinkBWP.pdsch_ConfigCommon->choice.setup)
        pdsch_TimeDomainAllocationList = mac->scc_SIB->downlinkConfigCommon.initialDownlinkBWP.pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList;
      long mapping_type_dl = pdsch_TimeDomainAllocationList ? pdsch_TimeDomainAllocationList->list.array[0]->mappingType : NR_PDSCH_TimeDomainResourceAllocation__mappingType_typeA;

      NR_DMRS_DownlinkConfig_t *NR_DMRS_dlconfig = NULL;
      if (pdsch_Config) {
        if (mapping_type_dl == NR_PDSCH_TimeDomainResourceAllocation__mappingType_typeA)
          NR_DMRS_dlconfig = (NR_DMRS_DownlinkConfig_t *)pdsch_Config->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup;
        else
          NR_DMRS_dlconfig = (NR_DMRS_DownlinkConfig_t *)pdsch_Config->dmrs_DownlinkForPDSCH_MappingTypeB->choice.setup;
      }

      pdsch_dmrs_AdditionalPosition_t add_pos_dl = pdsch_dmrs_pos2;
      if (NR_DMRS_dlconfig && NR_DMRS_dlconfig->dmrs_AdditionalPosition)
        add_pos_dl = *NR_DMRS_dlconfig->dmrs_AdditionalPosition;

      /* PDSCH decoding time N_1 for processing capability 1 */
      int N_1;

      if (add_pos_dl == pdsch_dmrs_pos0)
        N_1 = pdsch_N_1_capability_1[numerology][1];
      else if (add_pos_dl == pdsch_dmrs_pos1 || add_pos_dl == pdsch_dmrs_pos2)
        N_1 = pdsch_N_1_capability_1[numerology][2];
      else
        N_1 = pdsch_N_1_capability_1[numerology][3];

      /* PUSCH preapration time N_2 for processing capability 1 */
      const int N_2 = pusch_N_2_timing_capability_1[numerology][1];

      /* d_1_1 depending on the number of PDSCH symbols allocated */
      const int d = 0; // TODO number of overlapping symbols of the scheduling PDCCH and the scheduled PDSCH
      int d_1_1 = 0;
      if (mapping_type_dl == NR_PDSCH_TimeDomainResourceAllocation__mappingType_typeA)
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
      int d_2_1;
      if (mapping_type_ul == NR_PUSCH_TimeDomainResourceAllocation__mappingType_typeB && start_symbol != 0)
        d_2_1 = 0;
      else
        d_2_1 = 1;

      /* d_2_2 */
      const double d_2_2 = 0.0; // set to 0 because there is only 1 BWP: TODO this should corresponds to the switching time as defined in TS 38.133

      /* N_t_1 time duration in msec of N_1 symbols corresponding to a PDSCH reception time
      // N_t_2 time duration in msec of N_2 symbols corresponding to a PUSCH preparation time */
      double N_t_1 = (N_1 + d_1_1) * (ofdm_symbol_size + nb_prefix_samples) * tc_factor;
      double N_t_2 = (N_2 + d_2_1) * (ofdm_symbol_size + nb_prefix_samples) * tc_factor;
      if (N_t_2 < d_2_2) N_t_2 = d_2_2;

      /* Time alignment procedure */
      // N_t_1 + N_t_2 + N_TA_max must be in msec
      const double t_subframe = 1.0; // subframe duration of 1 msec
      const int ul_tx_timing_adjustment = 1 + (int)ceil(slots_per_subframe*(N_t_1 + N_t_2 + N_TA_max + 0.5)/t_subframe);

      if (ul_time_alignment->apply_ta == 1){
        ul_time_alignment->ta_slot = (nr_slot_rx + ul_tx_timing_adjustment) % slots_per_frame;
        if (nr_slot_rx + ul_tx_timing_adjustment > slots_per_frame){
          ul_time_alignment->ta_frame = (frame_rx + 1) % 1024;
        } else {
          ul_time_alignment->ta_frame = frame_rx;
        }
        // reset TA flag
        ul_time_alignment->apply_ta = 0;
        LOG_D(PHY,"Frame %d slot %d -- Starting UL time alignment procedures. TA update will be applied at frame %d slot %d\n",
             frame_rx, nr_slot_rx, ul_time_alignment->ta_frame, ul_time_alignment->ta_slot);
      }
    }
  }
  return dec;
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

  uint8_t pilot1;

  proc->instance_cnt_slot1_dl_processing=-1;
  proc->nr_slot_rx = proc->sub_frame_start * ue->frame_parms.slots_per_subframe;

  char threadname[256];
  sprintf(threadname,"UE_thread_slot1_dl_processing_%d", proc->sub_frame_start);

  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  if ( (proc->sub_frame_start+1)%RX_NB_TH == 0 && threads.slot1_proc_one != -1 )
    CPU_SET(threads.slot1_proc_one, &cpuset);
  if ( RX_NB_TH > 1 && (proc->sub_frame_start+1)%RX_NB_TH == 1 && threads.slot1_proc_two != -1 )
    CPU_SET(threads.slot1_proc_two, &cpuset);
  if ( RX_NB_TH > 2 && (proc->sub_frame_start+1)%RX_NB_TH == 2 && threads.slot1_proc_three != -1 )
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

    int frame_rx            = proc->frame_rx;
    uint8_t subframe_rx         = proc->nr_slot_rx / ue->frame_parms.slots_per_subframe;
    uint8_t next_subframe_rx    = (1 + subframe_rx) % NR_NUMBER_OF_SUBFRAMES_PER_FRAME;
    uint8_t next_subframe_slot0 = next_subframe_rx * ue->frame_parms.slots_per_subframe;

    uint8_t slot1  = proc->nr_slot_rx + 1;
    uint8_t pilot0 = 0;

    //printf("AbsSubframe %d.%d execute dl slot1 processing \n", frame_rx, nr_slot_rx);

    if (ue->frame_parms.Ncp == 0) {  // normal prefix
      pilot1 = 4;
    } else { // extended prefix
      pilot1 = 3;
    }

    /**** Slot1 FE Processing ****/

    start_meas(&ue->ue_front_end_per_slot_stat[proc->thread_id][1]);

    // I- start dl slot1 processing
    // do first symbol of next downlink nr_slot_rx for channel estimation
    /*
    // 1- perform FFT for pilot ofdm symbols first (ofdmSym0 next nr_slot_rx ofdmSym11)
    if (nr_subframe_select(&ue->frame_parms,next_nr_slot_rx) != SF_UL)
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

	  start_meas(&ue->ofdm_demod_stats);
	  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_SLOT_FEP, VCD_FUNCTION_IN);
	  //printf("AbsSubframe %d.%d FFT slot %d, symbol %d\n", frame_rx,nr_slot_rx,slot1,l);
	  front_end_fft(ue,
                        l,
                        slot1,
                        0,
                        0);
	  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_SLOT_FEP, VCD_FUNCTION_OUT);
	  stop_meas(&ue->ofdm_demod_stats);
	}
      } // for l=1..l2

    if (nr_subframe_select(&ue->frame_parms,next_nr_slot_rx) != SF_UL)
      {
	//printf("AbsSubframe %d.%d FFT slot %d, symbol %d\n", frame_rx,nr_slot_rx,next_subframe_slot0,pilot0);
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
	//printf("AbsSubframe %d.%d ChanEst slot %d, symbol %d\n", frame_rx,nr_slot_rx,slot1,l);
	front_end_chanEst(ue,
			  l,
			  slot1,
			  0);
	ue_measurement_procedures(l-1,ue,proc,0,slot1,0,ue->mode);
      }
    //printf("AbsSubframe %d.%d ChanEst slot %d, symbol %d\n", frame_rx,nr_slot_rx,next_subframe_slot0,pilot0);
    front_end_chanEst(ue,
		      pilot0,
		      next_subframe_slot0,
		      0);

    if ( (nr_slot_rx == 0) && (ue->decode_MIB == 1))
      {
	ue_pbch_procedures(0,ue,proc,0);
      }

    proc->chan_est_slot1_available = 1;
    //printf("Set available slot 1channelEst to 1 AbsSubframe %d.%d \n",frame_rx,nr_slot_rx);
    //printf(" [slot1 dl processing] ==> FFT/CHanEst Done for AbsSubframe %d.%d \n", proc->frame_rx, proc->nr_slot_rx);

    //printf(" [slot1 dl processing] ==> Start LLR Comuptation slot1 for AbsSubframe %d.%d \n", proc->frame_rx, proc->nr_slot_rx);



    stop_meas(&ue->ue_front_end_per_slot_stat[proc->thread_id][1]);
#if PHYSIM
    printf("[AbsSFN %d.%d] Slot1: FFT + Channel Estimate + Pdsch Proc Slot0 %5.2f \n",frame_rx,nr_slot_rx,ue->ue_front_end_per_slot_stat[proc->thread_id][1].p_time/(cpuf*1000.0));
#else
    LOG_D(PHY, "[AbsSFN %d.%d] Slot1: FFT + Channel Estimate + Pdsch Proc Slot0 %5.2f \n",frame_rx,nr_slot_rx,ue->ue_front_end_per_slot_stat[proc->thread_id][1].p_time/(cpuf*1000.0));
#endif


    //wait until pdcch is decoded
    uint32_t wait = 0;
    while(proc->dci_slot0_available == 0)
      {
        usleep(1);
        wait++;
      }
    //printf("[slot1 dl processing] AbsSubframe %d.%d LLR Computation Start wait DCI %d\n",frame_rx,nr_slot_rx,wait);

    /**** Pdsch Procedure Slot1 ****/
    // start slot1 thread for Pdsch Procedure (slot1)
    // do procedures for C-RNTI
    //printf("AbsSubframe %d.%d Pdsch Procedure (slot1)\n",frame_rx,nr_slot_rx);


    start_meas(&ue->pdsch_procedures_per_slot_stat[proc->thread_id][1]);

    // start slave thread for Pdsch Procedure (slot1)
    // do procedures for C-RNTI
    uint8_t gNB_id = 0;

    if (ue->dlsch[proc->thread_id][gNB_id][0]->active == 1) {
      //wait until first ofdm symbol is processed
      //wait = 0;
      //while(proc->first_symbol_available == 0)
      //{
      //    usleep(1);
      //    wait++;
      //}
      //printf("[slot1 dl processing] AbsSubframe %d.%d LLR Computation Start wait First Ofdm Sym %d\n",frame_rx,nr_slot_rx,wait);

      //VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PDSCH_PROC, VCD_FUNCTION_IN);
      ue_pdsch_procedures(ue,
			  proc,
			  gNB_id,
			  PDSCH,
			  ue->dlsch[proc->thread_id][gNB_id][0],
			  NULL,
			  (ue->frame_parms.symbols_per_slot>>1),
			  ue->frame_parms.symbols_per_slot-1,
			  abstraction_flag);
      LOG_D(PHY," ------ end PDSCH ChannelComp/LLR slot 0: AbsSubframe %d.%d ------  \n", frame_rx%1024, nr_slot_rx);
      LOG_D(PHY," ------ --> PDSCH Turbo Decoder slot 0/1: AbsSubframe %d.%d ------  \n", frame_rx%1024, nr_slot_rx);
    }

    // do procedures for SI-RNTI
    if ((ue->dlsch_SI[gNB_id]) && (ue->dlsch_SI[gNB_id]->active == 1)) {
      ue_pdsch_procedures(ue,
			  proc,
			  gNB_id,
			  SI_PDSCH,
			  ue->dlsch_SI[gNB_id],
			  NULL,
			  (ue->frame_parms.symbols_per_slot>>1),
			  ue->frame_parms.symbols_per_slot-1,
			  abstraction_flag);
    }

    // do procedures for P-RNTI
    if ((ue->dlsch_p[gNB_id]) && (ue->dlsch_p[gNB_id]->active == 1)) {
      ue_pdsch_procedures(ue,
			  proc,
			  gNB_id,
			  P_PDSCH,
			  ue->dlsch_p[gNB_id],
			  NULL,
			  (ue->frame_parms.symbols_per_slot>>1),
			  ue->frame_parms.symbols_per_slot-1,
			  abstraction_flag);
    }
    // do procedures for RA-RNTI
    if ((ue->dlsch_ra[gNB_id]) && (ue->dlsch_ra[gNB_id]->active == 1) && (UE_mode != PUSCH)) {
      ue_pdsch_procedures(ue,
			  proc,
			  gNB_id,
			  RA_PDSCH,
			  ue->dlsch_ra[gNB_id],
			  NULL,
			  (ue->frame_parms.symbols_per_slot>>1),
			  ue->frame_parms.symbols_per_slot-1,
			  abstraction_flag);
    }

    proc->llr_slot1_available=1;
    //printf("Set available LLR slot1 to 1 AbsSubframe %d.%d \n",frame_rx,nr_slot_rx);

    stop_meas(&ue->pdsch_procedures_per_slot_stat[proc->thread_id][1]);
#if PHYSIM
    printf("[AbsSFN %d.%d] Slot1: LLR Computation %5.2f \n",frame_rx,nr_slot_rx,ue->pdsch_procedures_per_slot_stat[proc->thread_id][1].p_time/(cpuf*1000.0));
#else
    LOG_D(PHY, "[AbsSFN %d.%d] Slot1: LLR Computation %5.2f \n",frame_rx,nr_slot_rx,ue->pdsch_procedures_per_slot_stat[proc->thread_id][1].p_time/(cpuf*1000.0));
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

int is_ssb_in_slot(fapi_nr_config_request_t *config, int frame, int slot, NR_DL_FRAME_PARMS *fp)
{
  int mu = fp->numerology_index;
  //uint8_t half_frame_index = fp->half_frame_bit;
  //uint8_t i_ssb = fp->ssb_index;
  uint8_t Lmax = fp->Lmax;

  if (!(frame%(1<<(config->ssb_table.ssb_period-1)))){

    if(Lmax <= 8) {
      if(slot <=3 && (((config->ssb_table.ssb_mask_list[0].ssb_mask << 2*slot)&0x80000000) == 0x80000000 || ((config->ssb_table.ssb_mask_list[0].ssb_mask << (2*slot +1))&0x80000000) == 0x80000000))
      return 1;
      else return 0;
    
    }
    else if(Lmax == 64) {
      if (mu == NR_MU_3){

        if (slot>=0 && slot <= 7){
          if(((config->ssb_table.ssb_mask_list[0].ssb_mask << 2*slot)&0x80000000) == 0x80000000 || ((config->ssb_table.ssb_mask_list[0].ssb_mask << (2*slot +1))&0x80000000) == 0x80000000)
          return 1;
          else return 0;
        }
      else if (slot>=10 && slot <=17){
         if(((config->ssb_table.ssb_mask_list[0].ssb_mask << 2*(slot-2))&0x80000000) == 0x80000000 || ((config->ssb_table.ssb_mask_list[0].ssb_mask << (2*(slot-2) +1))&0x80000000) == 0x80000000)
         return 1;
         else return 0;
      }
      else if (slot>=20 && slot <=27){
         if(((config->ssb_table.ssb_mask_list[1].ssb_mask << 2*(slot-20))&0x80000000) == 0x80000000 || ((config->ssb_table.ssb_mask_list[1].ssb_mask << (2*(slot-20) +1))&0x80000000) == 0x80000000)
         return 1;
         else return 0;
      }
      else if (slot>=30 && slot <=37){
         if(((config->ssb_table.ssb_mask_list[1].ssb_mask << 2*(slot-22))&0x80000000) == 0x80000000 || ((config->ssb_table.ssb_mask_list[1].ssb_mask << (2*(slot-22) +1))&0x80000000) == 0x80000000)
         return 1;
         else return 0;
       }
      else return 0;

    }


    else if (mu == NR_MU_4) {
         AssertFatal(0==1, "not implemented for mu =  %d yet\n", mu);
    }
    else AssertFatal(0==1, "Invalid numerology index %d for the synchronization block\n", mu);
   }
   else AssertFatal(0==1, "Invalid Lmax %u for the synchronization block\n", Lmax);
  }
  else return 0;

}

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
                           uint8_t gNB_id,
                           uint8_t dlsch_parallel,
                           notifiedFIFO_t *txFifo
                           )
{                                         
  int frame_rx = proc->frame_rx;
  int nr_slot_rx = proc->nr_slot_rx;
  int slot_pbch;
  int slot_ssb;
  NR_UE_PDCCH *pdcch_vars  = ue->pdcch_vars[proc->thread_id][0];
  fapi_nr_config_request_t *cfg = &ue->nrUE_config;

  uint8_t nb_symb_pdcch = pdcch_vars->nb_search_space > 0 ? pdcch_vars->pdcch_config[0].coreset.duration : 0;
  uint8_t dci_cnt = 0;
  NR_DL_FRAME_PARMS *fp = &ue->frame_parms;
  
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_UE_RX, VCD_FUNCTION_IN);

  LOG_D(PHY," ****** start RX-Chain for Frame.Slot %d.%d (energy %d dB)******  \n",
        frame_rx%1024, nr_slot_rx,
        dB_fixed(signal_energy(ue->common_vars.common_vars_rx_data_per_thread[proc->thread_id].rxdataF[0],2048*14)));

  /*
  uint8_t next1_thread_id = proc->thread_id== (RX_NB_TH-1) ? 0:(proc->thread_id+1);
  uint8_t next2_thread_id = next1_thread_id== (RX_NB_TH-1) ? 0:(next1_thread_id+1);
  */

  int coreset_nb_rb=0;
  int coreset_start_rb=0;

  if (pdcch_vars->nb_search_space > 0)
    get_coreset_rballoc(pdcch_vars->pdcch_config[0].coreset.frequency_domain_resource,&coreset_nb_rb,&coreset_start_rb);

  slot_pbch = is_pbch_in_slot(cfg, frame_rx, nr_slot_rx, fp);
  slot_ssb  = is_ssb_in_slot(cfg, frame_rx, nr_slot_rx, fp);

  // looking for pbch only in slot where it is supposed to be
  if (slot_ssb) {
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_SLOT_FEP_PBCH, VCD_FUNCTION_IN);
    LOG_D(PHY," ------  PBCH ChannelComp/LLR: frame.slot %d.%d ------  \n", frame_rx%1024, nr_slot_rx);
    for (int i=1; i<4; i++) {

      nr_slot_fep(ue,
                  proc,
                  (ue->symbol_offset+i)%(fp->symbols_per_slot),
                  nr_slot_rx);


      start_meas(&ue->dlsch_channel_estimation_stats);
      nr_pbch_channel_estimation(ue,proc,gNB_id,nr_slot_rx,(ue->symbol_offset+i)%(fp->symbols_per_slot),i-1,(fp->ssb_index)&7,fp->half_frame_bit);
      stop_meas(&ue->dlsch_channel_estimation_stats);
    }

    nr_ue_rsrp_measurements(ue, gNB_id, proc, nr_slot_rx, 0);

    if ((ue->decode_MIB == 1) && slot_pbch) {

      LOG_D(PHY," ------  Decode MIB: frame.slot %d.%d ------  \n", frame_rx%1024, nr_slot_rx);
      nr_ue_pbch_procedures(gNB_id, ue, proc, 0);

      if (ue->no_timing_correction==0) {
        LOG_D(PHY,"start adjust sync slot = %d no timing %d\n", nr_slot_rx, ue->no_timing_correction);
        nr_adjust_synch_ue(fp,
                           ue,
                           gNB_id,
                           frame_rx,
                           nr_slot_rx,
                           0,
                           16384);
      }

      LOG_D(PHY, "Doing N0 measurements in %s\n", __FUNCTION__);
      nr_ue_rrc_measurements(ue, proc, nr_slot_rx);
      VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_SLOT_FEP_PBCH, VCD_FUNCTION_OUT);
    }
  }

  if ((frame_rx%64 == 0) && (nr_slot_rx==0)) {
    LOG_I(NR_PHY,"============================================\n");
    LOG_I(NR_PHY,"Harq round stats for Downlink: %d/%d/%d/%d DLSCH errors: %d\n",ue->dl_stats[0],ue->dl_stats[1],ue->dl_stats[2],ue->dl_stats[3],ue->dl_stats[4]);
    LOG_I(NR_PHY,"============================================\n");
  }

#ifdef NR_PDCCH_SCHED

  LOG_D(PHY," ------ --> PDCCH ChannelComp/LLR Frame.slot %d.%d ------  \n", frame_rx%1024, nr_slot_rx);
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_SLOT_FEP_PDCCH, VCD_FUNCTION_IN);

  for (uint16_t l=0; l<nb_symb_pdcch; l++) {

    start_meas(&ue->ofdm_demod_stats);
    nr_slot_fep(ue,
                proc,
                l,
                nr_slot_rx);
  }

  dci_cnt = 0;
  for(int n_ss = 0; n_ss<pdcch_vars->nb_search_space; n_ss++) {
    for (uint16_t l=0; l<nb_symb_pdcch; l++) {

      // note: this only works if RBs for PDCCH are contigous!
      LOG_D(PHY, "pdcch_channel_estimation: first_carrier_offset %d, BWPStart %d, coreset_start_rb %d\n",
            fp->first_carrier_offset, pdcch_vars->pdcch_config[n_ss].BWPStart, coreset_start_rb);

      if (coreset_nb_rb > 0)
        nr_pdcch_channel_estimation(ue,
                                    proc,
                                    gNB_id,
                                    nr_slot_rx,
                                    l,
                                    fp->first_carrier_offset+(pdcch_vars->pdcch_config[n_ss].BWPStart + coreset_start_rb)*12,
                                    coreset_nb_rb);

      stop_meas(&ue->ofdm_demod_stats);

    }
    dci_cnt = dci_cnt + nr_ue_pdcch_procedures(gNB_id, ue, proc, n_ss);
  }
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_SLOT_FEP_PDCCH, VCD_FUNCTION_OUT);

  if (dci_cnt > 0) {

    LOG_D(PHY,"[UE %d] Frame %d, nr_slot_rx %d: found %d DCIs\n", ue->Mod_id, frame_rx, nr_slot_rx, dci_cnt);

    NR_UE_DLSCH_t *dlsch = NULL;
    if (ue->dlsch[proc->thread_id][gNB_id][0]->active == 1){
      dlsch = ue->dlsch[proc->thread_id][gNB_id][0];
    } else if (ue->dlsch_SI[0]->active == 1){
      dlsch = ue->dlsch_SI[0];
    } else if (ue->dlsch_ra[0]->active == 1){
      dlsch = ue->dlsch_ra[0];
    }

    if (dlsch) {
      VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_SLOT_FEP_PDSCH, VCD_FUNCTION_IN);
      uint8_t harq_pid = dlsch->current_harq_pid;
      NR_DL_UE_HARQ_t *dlsch0_harq = dlsch->harq_processes[harq_pid];
      uint16_t nb_symb_sch = dlsch0_harq->nb_symbols;
      uint16_t start_symb_sch = dlsch0_harq->start_symbol;

      LOG_D(PHY," ------ --> PDSCH ChannelComp/LLR Frame.slot %d.%d ------  \n", frame_rx%1024, nr_slot_rx);
      //to update from pdsch config

      for (uint16_t m=start_symb_sch;m<(nb_symb_sch+start_symb_sch) ; m++){
        nr_slot_fep(ue,
                    proc,
                    m,  //to be updated from higher layer
                    nr_slot_rx);
      }
      VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_SLOT_FEP_PDSCH, VCD_FUNCTION_OUT);
    }
  } else {
    LOG_D(PHY,"[UE %d] Frame %d, nr_slot_rx %d: No DCIs found\n", ue->Mod_id, frame_rx, nr_slot_rx);
  }

#endif //NR_PDCCH_SCHED

  // Start PUSCH processing here. It runs in parallel with PDSCH processing
  notifiedFIFO_elt_t *newElt = newNotifiedFIFO_elt(sizeof(nr_rxtx_thread_data_t), proc->nr_slot_tx,txFifo,processSlotTX);
  nr_rxtx_thread_data_t *curMsg=(nr_rxtx_thread_data_t *)NotifiedFifoData(newElt);
  curMsg->proc = *proc;
  curMsg->UE = ue;
  curMsg->ue_sched_mode = ONLY_PUSCH;
  pushTpool(&(get_nrUE_params()->Tpool), newElt);
  start_meas(&ue->generic_stat);
  // do procedures for C-RNTI
  int ret_pdsch = 0;
  if (ue->dlsch[proc->thread_id][gNB_id][0]->active == 1) {

    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PDSCH_PROC_C, VCD_FUNCTION_IN);
    ret_pdsch = nr_ue_pdsch_procedures(ue,
                                       proc,
                                       gNB_id,
                                       PDSCH,
                                       ue->dlsch[proc->thread_id][gNB_id][0],
                                       NULL);

    nr_ue_measurement_procedures(2, ue, proc, gNB_id, nr_slot_rx);
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PDSCH_PROC_C, VCD_FUNCTION_OUT);
  }

  // do procedures for SI-RNTI
  if ((ue->dlsch_SI[gNB_id]) && (ue->dlsch_SI[gNB_id]->active == 1)) {
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PDSCH_PROC_SI, VCD_FUNCTION_IN);
    nr_ue_pdsch_procedures(ue,
                           proc,
                           gNB_id,
                           SI_PDSCH,
                           ue->dlsch_SI[gNB_id],
                           NULL);
    
    nr_ue_dlsch_procedures(ue,
                           proc,
                           gNB_id,
                           SI_PDSCH,
                           ue->dlsch_SI[gNB_id],
                           NULL,
                           &ue->dlsch_SI_errors[gNB_id],
                           dlsch_parallel);

    // deactivate dlsch once dlsch proc is done
    ue->dlsch_SI[gNB_id]->active = 0;

    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PDSCH_PROC_SI, VCD_FUNCTION_OUT);
  }

  // do procedures for P-RNTI
  if ((ue->dlsch_p[gNB_id]) && (ue->dlsch_p[gNB_id]->active == 1)) {
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PDSCH_PROC_P, VCD_FUNCTION_IN);
    nr_ue_pdsch_procedures(ue,
                           proc,
                           gNB_id,
                           P_PDSCH,
                           ue->dlsch_p[gNB_id],
                           NULL);

    nr_ue_dlsch_procedures(ue,
                           proc,
                           gNB_id,
                           P_PDSCH,
                           ue->dlsch_p[gNB_id],
                           NULL,
                           &ue->dlsch_p_errors[gNB_id],
                           dlsch_parallel);

    // deactivate dlsch once dlsch proc is done
    ue->dlsch_p[gNB_id]->active = 0;
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PDSCH_PROC_P, VCD_FUNCTION_OUT);
  }

  // do procedures for RA-RNTI
  if ((ue->dlsch_ra[gNB_id]) && (ue->dlsch_ra[gNB_id]->active == 1) && (ue->UE_mode[gNB_id] != PUSCH)) {
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PDSCH_PROC_RA, VCD_FUNCTION_IN);
    nr_ue_pdsch_procedures(ue,
                           proc,
                           gNB_id,
                           RA_PDSCH,
                           ue->dlsch_ra[gNB_id],
                           NULL);

    nr_ue_dlsch_procedures(ue,
                           proc,
                           gNB_id,
                           RA_PDSCH,
                           ue->dlsch_ra[gNB_id],
                           NULL,
                           &ue->dlsch_ra_errors[gNB_id],
                           dlsch_parallel);

    // deactivate dlsch once dlsch proc is done
    ue->dlsch_ra[gNB_id]->active = 0;

    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PDSCH_PROC_RA, VCD_FUNCTION_OUT);
  }
    
  // do procedures for C-RNTI
  if (ue->dlsch[proc->thread_id][gNB_id][0]->active == 1) {

    LOG_D(PHY, "DLSCH data reception at nr_slot_rx: %d \n \n", nr_slot_rx);
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PDSCH_PROC, VCD_FUNCTION_IN);

    start_meas(&ue->dlsch_procedures_stat[proc->thread_id]);

    if (ret_pdsch >= 0)
      nr_ue_dlsch_procedures(ue,
			   proc,
			   gNB_id,
			   PDSCH,
			   ue->dlsch[proc->thread_id][gNB_id][0],
			   ue->dlsch[proc->thread_id][gNB_id][1],
			   &ue->dlsch_errors[gNB_id],
			   dlsch_parallel);

  stop_meas(&ue->dlsch_procedures_stat[proc->thread_id]);
#if PHYSIM
  printf("[SFN %d] Slot1:       Pdsch Proc %5.2f\n",nr_slot_rx,ue->pdsch_procedures_stat[proc->thread_id].p_time/(cpuf*1000.0));
  printf("[SFN %d] Slot0 Slot1: Dlsch Proc %5.2f\n",nr_slot_rx,ue->dlsch_procedures_stat[proc->thread_id].p_time/(cpuf*1000.0));
#else
  LOG_D(PHY, "[SFN %d] Slot1:       Pdsch Proc %5.2f\n",nr_slot_rx,ue->pdsch_procedures_stat[proc->thread_id].p_time/(cpuf*1000.0));
  LOG_D(PHY, "[SFN %d] Slot0 Slot1: Dlsch Proc %5.2f\n",nr_slot_rx,ue->dlsch_procedures_stat[proc->thread_id].p_time/(cpuf*1000.0));
#endif



  // deactivate dlsch once dlsch proc is done
  ue->dlsch[proc->thread_id][gNB_id][0]->active = 0;

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PDSCH_PROC, VCD_FUNCTION_OUT);

 }

start_meas(&ue->generic_stat);

#if 0

  if(nr_slot_rx==5 &&  ue->dlsch[proc->thread_id][gNB_id][0]->harq_processes[ue->dlsch[proc->thread_id][gNB_id][0]->current_harq_pid]->nb_rb > 20){
       //write_output("decoder_llr.m","decllr",dlsch_llr,G,1,0);
       //write_output("llr.m","llr",  &ue->pdsch_vars[proc->thread_id][gNB_id]->llr[0][0],(14*nb_rb*12*dlsch1_harq->Qm) - 4*(nb_rb*4*dlsch1_harq->Qm),1,0);

       write_output("rxdataF0_current.m"    , "rxdataF0", &ue->common_vars.common_vars_rx_data_per_thread[proc->thread_id].rxdataF[0][0],14*fp->ofdm_symbol_size,1,1);
       //write_output("rxdataF0_previous.m"    , "rxdataF0_prev_sss", &ue->common_vars.common_vars_rx_data_per_thread[next_thread_id].rxdataF[0][0],14*fp->ofdm_symbol_size,1,1);

       //write_output("rxdataF0_previous.m"    , "rxdataF0_prev", &ue->common_vars.common_vars_rx_data_per_thread[next_thread_id].rxdataF[0][0],14*fp->ofdm_symbol_size,1,1);

       write_output("dl_ch_estimates.m", "dl_ch_estimates_sfn5", &ue->common_vars.common_vars_rx_data_per_thread[proc->thread_id].dl_ch_estimates[0][0][0],14*fp->ofdm_symbol_size,1,1);
       write_output("dl_ch_estimates_ext.m", "dl_ch_estimatesExt_sfn5", &ue->pdsch_vars[proc->thread_id][gNB_id]->dl_ch_estimates_ext[0][0],14*fp->N_RB_DL*12,1,1);
       write_output("rxdataF_comp00.m","rxdataF_comp00",         &ue->pdsch_vars[proc->thread_id][gNB_id]->rxdataF_comp0[0][0],14*fp->N_RB_DL*12,1,1);
       //write_output("magDLFirst.m", "magDLFirst", &phy_vars_ue->pdsch_vars[proc->thread_id][gNB_id]->dl_ch_mag0[0][0],14*fp->N_RB_DL*12,1,1);
       //write_output("magDLSecond.m", "magDLSecond", &phy_vars_ue->pdsch_vars[proc->thread_id][gNB_id]->dl_ch_magb0[0][0],14*fp->N_RB_DL*12,1,1);

       AssertFatal (0,"");
  }
#endif

  // duplicate harq structure
/*
  uint8_t          current_harq_pid        = ue->dlsch[proc->thread_id][gNB_id][0]->current_harq_pid;
  NR_DL_UE_HARQ_t *current_harq_processes = ue->dlsch[proc->thread_id][gNB_id][0]->harq_processes[current_harq_pid];
  NR_DL_UE_HARQ_t *harq_processes_dest    = ue->dlsch[next1_thread_id][gNB_id][0]->harq_processes[current_harq_pid];
  NR_DL_UE_HARQ_t *harq_processes_dest1    = ue->dlsch[next2_thread_id][gNB_id][0]->harq_processes[current_harq_pid];
  */
  /*nr_harq_status_t *current_harq_ack = &ue->dlsch[proc->thread_id][gNB_id][0]->harq_ack[nr_slot_rx];
  nr_harq_status_t *harq_ack_dest    = &ue->dlsch[next1_thread_id][gNB_id][0]->harq_ack[nr_slot_rx];
  nr_harq_status_t *harq_ack_dest1    = &ue->dlsch[next2_thread_id][gNB_id][0]->harq_ack[nr_slot_rx];
*/

  //copy_harq_proc_struct(harq_processes_dest, current_harq_processes);
//copy_ack_struct(harq_ack_dest, current_harq_ack);

//copy_harq_proc_struct(harq_processes_dest1, current_harq_processes);
//copy_ack_struct(harq_ack_dest1, current_harq_ack);

if (nr_slot_rx==9) {
  if (frame_rx % 10 == 0) {
    if ((ue->dlsch_received[gNB_id] - ue->dlsch_received_last[gNB_id]) != 0)
      ue->dlsch_fer[gNB_id] = (100*(ue->dlsch_errors[gNB_id] - ue->dlsch_errors_last[gNB_id]))/(ue->dlsch_received[gNB_id] - ue->dlsch_received_last[gNB_id]);

    ue->dlsch_errors_last[gNB_id] = ue->dlsch_errors[gNB_id];
    ue->dlsch_received_last[gNB_id] = ue->dlsch_received[gNB_id];
  }


  ue->bitrate[gNB_id] = (ue->total_TBS[gNB_id] - ue->total_TBS_last[gNB_id])*100;
  ue->total_TBS_last[gNB_id] = ue->total_TBS[gNB_id];
  LOG_D(PHY,"[UE %d] Calculating bitrate Frame %d: total_TBS = %d, total_TBS_last = %d, bitrate %f kbits\n",
	ue->Mod_id,frame_rx,ue->total_TBS[gNB_id],
	ue->total_TBS_last[gNB_id],(float) ue->bitrate[gNB_id]/1000.0);

#if UE_AUTOTEST_TRACE
  if ((frame_rx % 100 == 0)) {
    LOG_I(PHY,"[UE  %d] AUTOTEST Metric : UE_DLSCH_BITRATE = %5.2f kbps (frame = %d) \n", ue->Mod_id, (float) ue->bitrate[gNB_id]/1000.0, frame_rx);
  }
#endif

 }

stop_meas(&ue->generic_stat);
#if PHYSIM
printf("after tubo until end of Rx %5.2f \n",ue->generic_stat.p_time/(cpuf*1000.0));
#endif

#ifdef EMOS
phy_procedures_emos_UE_RX(ue,slot,gNB_id);
#endif


VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_UE_RX, VCD_FUNCTION_OUT);

stop_meas(&ue->phy_proc_rx[proc->thread_id]);
#if PHYSIM
printf("------FULL RX PROC [SFN %d]: %5.2f ------\n",nr_slot_rx,ue->phy_proc_rx[proc->thread_id].p_time/(cpuf*1000.0));
#else
LOG_D(PHY, "------FULL RX PROC [SFN %d]: %5.2f ------\n",nr_slot_rx,ue->phy_proc_rx[proc->thread_id].p_time/(cpuf*1000.0));
#endif

//#endif //pdsch

LOG_D(PHY," ****** end RX-Chain  for AbsSubframe %d.%d ******  \n", frame_rx%1024, nr_slot_rx);
return (0);
}


uint8_t nr_is_cqi_TXOp(PHY_VARS_NR_UE *ue,
		            UE_nr_rxtx_proc_t *proc,
					uint8_t gNB_id)
{
  int subframe = proc->nr_slot_tx / ue->frame_parms.slots_per_subframe;
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
  int subframe = proc->nr_slot_tx / ue->frame_parms.slots_per_subframe;
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

// todo:
// - power control as per 38.213 ch 7.4
void nr_ue_prach_procedures(PHY_VARS_NR_UE *ue, UE_nr_rxtx_proc_t *proc, uint8_t gNB_id) {

  int frame_tx = proc->frame_tx, nr_slot_tx = proc->nr_slot_tx, prach_power; // tx_amp
  uint8_t mod_id = ue->Mod_id;
  NR_PRACH_RESOURCES_t * prach_resources = ue->prach_resources[gNB_id];
  AssertFatal(prach_resources != NULL, "ue->prach_resources[%u] == NULL\n", gNB_id);
  uint8_t nr_prach = 0;

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_UE_TX_PRACH, VCD_FUNCTION_IN);

  if (ue->mac_enabled == 0){

    prach_resources->ra_TDD_map_index = 0;
    prach_resources->ra_PREAMBLE_RECEIVED_TARGET_POWER = 10;
    prach_resources->ra_RNTI = 0x1234;
    nr_prach = 1;
    prach_resources->init_msg1 = 1;

  } else {

    nr_prach = nr_ue_get_rach(prach_resources, &ue->prach_vars[0]->prach_pdu, mod_id, ue->CC_id, frame_tx, gNB_id, nr_slot_tx);
    LOG_D(PHY, "In %s:[%d.%d] getting PRACH resources : %d\n", __FUNCTION__, frame_tx, nr_slot_tx,nr_prach);
  }

  if (nr_prach == GENERATE_PREAMBLE) {

    if (ue->mac_enabled == 1) {
      int16_t pathloss = get_nr_PL(mod_id, ue->CC_id, gNB_id);
      int16_t ra_preamble_rx_power = (int16_t)(prach_resources->ra_PREAMBLE_RECEIVED_TARGET_POWER - pathloss + 30);
      ue->tx_power_dBm[nr_slot_tx] = min(nr_get_Pcmax(mod_id), ra_preamble_rx_power);

      LOG_D(PHY, "In %s: [UE %d][RAPROC][%d.%d]: Generating PRACH Msg1 (preamble %d, PL %d dB, P0_PRACH %d, TARGET_RECEIVED_POWER %d dBm, RA-RNTI %x)\n",
        __FUNCTION__,
        mod_id,
        frame_tx,
        nr_slot_tx,
        prach_resources->ra_PreambleIndex,
        pathloss,
        ue->tx_power_dBm[nr_slot_tx],
        prach_resources->ra_PREAMBLE_RECEIVED_TARGET_POWER,
        prach_resources->ra_RNTI);
    }

    ue->prach_vars[gNB_id]->amp = AMP;

    /* #if defined(EXMIMO) || defined(OAI_USRP) || defined(OAI_BLADERF) || defined(OAI_LMSSDR) || defined(OAI_ADRV9371_ZC706)
      tx_amp = get_tx_amp_prach(ue->tx_power_dBm[nr_slot_tx], ue->tx_power_max_dBm, ue->frame_parms.N_RB_UL);
      if (tx_amp != -1)
        ue->prach_vars[gNB_id]->amp = tx_amp;
    #else
      ue->prach_vars[gNB_id]->amp = AMP;
    #endif */

    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_GENERATE_PRACH, VCD_FUNCTION_IN);

    prach_power = generate_nr_prach(ue, gNB_id, nr_slot_tx);

    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_GENERATE_PRACH, VCD_FUNCTION_OUT);

    LOG_D(PHY, "In %s: [UE %d][RAPROC][%d.%d]: Generated PRACH Msg1 (TX power PRACH %d dBm, digital power %d dBW (amp %d)\n",
      __FUNCTION__,
      mod_id,
      frame_tx,
      nr_slot_tx,
      ue->tx_power_dBm[nr_slot_tx],
      dB_fixed(prach_power),
      ue->prach_vars[gNB_id]->amp);

    if (ue->mac_enabled == 1)
      nr_Msg1_transmitted(mod_id, ue->CC_id, frame_tx, gNB_id);

  } else if (nr_prach == WAIT_CONTENTION_RESOLUTION) {
    LOG_D(PHY, "In %s: [UE %d] RA waiting contention resolution\n", __FUNCTION__, mod_id);
    ue->UE_mode[gNB_id] = RA_WAIT_CR;
  } else if (nr_prach == RA_SUCCEEDED) {
    LOG_D(PHY, "In %s: [UE %d] RA completed, setting UE mode to PUSCH\n", __FUNCTION__, mod_id);
    ue->UE_mode[gNB_id] = PUSCH;
  } else if(nr_prach == RA_FAILED){
    LOG_D(PHY, "In %s: [UE %d] RA failed, setting UE mode to PRACH\n", __FUNCTION__, mod_id);
    ue->UE_mode[gNB_id] = PRACH;
  }

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_UE_TX_PRACH, VCD_FUNCTION_OUT);

}
