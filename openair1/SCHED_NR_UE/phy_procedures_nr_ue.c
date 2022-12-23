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
#include "PHY/NR_REFSIG/dmrs_nr.h"
#include "PHY/phy_extern_nr_ue.h"
#include "PHY/MODULATION/modulation_UE.h"
#include "PHY/INIT/phy_init.h"
#include "PHY/NR_UE_TRANSPORT/nr_transport_ue.h"
#include "PHY/NR_UE_TRANSPORT/nr_transport_proto_ue.h"
#include "PHY/NR_UE_TRANSPORT/srs_modulation_nr.h"
#include "SCHED_NR/extern.h"
#include "SCHED_NR_UE/phy_sch_processing_time.h"
#include "PHY/NR_UE_ESTIMATION/nr_estimation.h"
#ifdef EMOS
#include "SCHED/phy_procedures_emos.h"
#endif
#include "executables/softmodem-common.h"
#include "executables/nr-uesoftmodem.h"
#include "LAYER2/NR_MAC_UE/mac_proto.h"
#include "SCHED_NR_UE/pucch_uci_ue_nr.h"
#include <openair1/PHY/TOOLS/phy_scope_interface.h>

//#define DEBUG_PHY_PROC
#define NR_PDCCH_SCHED
//#define NR_PDCCH_SCHED_DEBUG
//#define NR_PUCCH_SCHED
//#define NR_PUCCH_SCHED_DEBUG
//#define NR_PDSCH_DEBUG

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

#if defined(OAI_USRP) || defined(OAI_BLADERF) || defined(OAI_LMSSDR) || defined(OAI_ADRV9371_ZC706)
extern uint64_t downlink_frequency[MAX_NUM_CCs][4];
#endif

unsigned int gain_table[31] = {100,112,126,141,158,178,200,224,251,282,316,359,398,447,501,562,631,708,794,891,1000,1122,1258,1412,1585,1778,1995,2239,2512,2818,3162};

void nr_fill_dl_indication(nr_downlink_indication_t *dl_ind,
                           fapi_nr_dci_indication_t *dci_ind,
                           fapi_nr_rx_indication_t *rx_ind,
                           UE_nr_rxtx_proc_t *proc,
                           PHY_VARS_NR_UE *ue,
                           void *phy_data){

  memset((void*)dl_ind, 0, sizeof(nr_downlink_indication_t));

  dl_ind->gNB_index = proc->gNB_id;
  dl_ind->module_id = ue->Mod_id;
  dl_ind->cc_id     = ue->CC_id;
  dl_ind->frame     = proc->frame_rx;
  dl_ind->slot      = proc->nr_slot_rx;
  dl_ind->phy_data  = phy_data;

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
                           PHY_VARS_NR_UE *ue,
                           NR_UE_DLSCH_t *dlsch0,
                           NR_UE_DLSCH_t *dlsch1,
                           uint16_t n_pdus,
                           UE_nr_rxtx_proc_t *proc,
                           void *typeSpecific){

  NR_DL_FRAME_PARMS *frame_parms = &ue->frame_parms;

  if (n_pdus > 1){
    LOG_E(PHY, "In %s: multiple number of DL PDUs not supported yet...\n", __FUNCTION__);
  }

  NR_DL_UE_HARQ_t *dl_harq0 = NULL;

  if ((pdu_type !=  FAPI_NR_RX_PDU_TYPE_SSB) && dlsch0) {
    dl_harq0 = &ue->dl_harq_processes[0][dlsch0->dlsch_config.harq_process_nbr];
    trace_NRpdu(DIRECTION_DOWNLINK,
		dl_harq0->b,
		dlsch0->dlsch_config.TBS / 8,
		WS_C_RNTI,
		dlsch0->rnti,
		proc->frame_rx,
		proc->nr_slot_rx,
		0,0);
  }
  switch (pdu_type){
    case FAPI_NR_RX_PDU_TYPE_SIB:
    case FAPI_NR_RX_PDU_TYPE_RAR:
    case FAPI_NR_RX_PDU_TYPE_DLSCH:
      if(dlsch0) {
        dl_harq0 = &ue->dl_harq_processes[0][dlsch0->dlsch_config.harq_process_nbr];
        rx_ind->rx_indication_body[n_pdus - 1].pdsch_pdu.harq_pid = dlsch0->dlsch_config.harq_process_nbr;
        rx_ind->rx_indication_body[n_pdus - 1].pdsch_pdu.ack_nack = dl_harq0->ack;
        rx_ind->rx_indication_body[n_pdus - 1].pdsch_pdu.pdu = dl_harq0->b;
        rx_ind->rx_indication_body[n_pdus - 1].pdsch_pdu.pdu_length = dlsch0->dlsch_config.TBS / 8;
      }
      if(dlsch1) {
        AssertFatal(1==0,"Second codeword currently not supported\n");
      }
      break;
    case FAPI_NR_RX_PDU_TYPE_SSB:
      rx_ind->rx_indication_body[n_pdus - 1].ssb_pdu.pdu=malloc(sizeof(((fapiPbch_t*)typeSpecific)->decoded_output));
      memcpy(rx_ind->rx_indication_body[n_pdus - 1].ssb_pdu.pdu,
	     ((fapiPbch_t*)typeSpecific)->decoded_output,
	     sizeof(((fapiPbch_t*)typeSpecific)->decoded_output));
      rx_ind->rx_indication_body[n_pdus - 1].ssb_pdu.additional_bits = ((fapiPbch_t*)typeSpecific)->xtra_byte;
      rx_ind->rx_indication_body[n_pdus - 1].ssb_pdu.ssb_index = (frame_parms->ssb_index)&0x7;
      rx_ind->rx_indication_body[n_pdus - 1].ssb_pdu.ssb_length = frame_parms->Lmax;
      rx_ind->rx_indication_body[n_pdus - 1].ssb_pdu.cell_id = frame_parms->Nid_cell;
      rx_ind->rx_indication_body[n_pdus - 1].ssb_pdu.ssb_start_subcarrier = frame_parms->ssb_start_subcarrier;
      rx_ind->rx_indication_body[n_pdus - 1].ssb_pdu.rsrp_dBm = ue->measurements.ssb_rsrp_dBm[frame_parms->ssb_index];
    break;
    case FAPI_NR_CSIRS_IND:
      memcpy(&rx_ind->rx_indication_body[n_pdus - 1].csirs_measurements,
             (fapi_nr_csirs_measurements_t*)typeSpecific,
             sizeof(*(fapi_nr_csirs_measurements_t*)typeSpecific));
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
                            nr_phy_data_tx_t *phy_data) {

  int slot_tx = proc->nr_slot_tx;
  int frame_tx = proc->frame_tx;
  int gNB_id = proc->gNB_id;

  AssertFatal(ue->CC_id == 0, "Transmission on secondary CCs is not supported yet\n");

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_UE_TX,VCD_FUNCTION_IN);

  for(int i=0; i< ue->frame_parms.nb_antennas_tx; ++i)
    memset(ue->common_vars.txdataF[i], 0, sizeof(int)*14*ue->frame_parms.ofdm_symbol_size);

  LOG_D(PHY,"****** start TX-Chain for AbsSubframe %d.%d ******\n", frame_tx, slot_tx);

  start_meas(&ue->phy_proc_tx);

  for (uint8_t harq_pid = 0; harq_pid < NR_MAX_ULSCH_HARQ_PROCESSES; harq_pid++) {
    if (ue->ul_harq_processes[harq_pid].status == ACTIVE)
      nr_ue_ulsch_procedures(ue, harq_pid, frame_tx, slot_tx, gNB_id, phy_data);
  }

  ue_srs_procedures_nr(ue, proc);

  pucch_procedures_ue_nr(ue, proc, phy_data);

  LOG_D(PHY, "Sending Uplink data \n");
  nr_ue_pusch_common_procedures(ue,
                                proc->nr_slot_tx,
                                &ue->frame_parms,
                                ue->frame_parms.nb_antennas_tx);

  nr_ue_prach_procedures(ue, proc);

  LOG_D(PHY,"****** end TX-Chain for AbsSubframe %d.%d ******\n", proc->frame_tx, proc->nr_slot_tx);

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_UE_TX, VCD_FUNCTION_OUT);
  stop_meas(&ue->phy_proc_tx);
}

void nr_ue_measurement_procedures(uint16_t l,
                                  PHY_VARS_NR_UE *ue,
                                  UE_nr_rxtx_proc_t *proc,
                                  NR_UE_DLSCH_t *dlsch,
                                  uint32_t pdsch_est_size,
                                  int32_t dl_ch_estimates[][pdsch_est_size]) {

  NR_DL_FRAME_PARMS *frame_parms=&ue->frame_parms;
  int frame_rx   = proc->frame_rx;
  int nr_slot_rx = proc->nr_slot_rx;
  int gNB_id = proc->gNB_id;
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_MEASUREMENT_PROCEDURES, VCD_FUNCTION_IN);

  if (l==2) {

    LOG_D(PHY,"Doing UE measurement procedures in symbol l %u Ncp %d nr_slot_rx %d, rxdata %p\n",
      l,
      ue->frame_parms.Ncp,
      nr_slot_rx,
      ue->common_vars.rxdata);

    nr_ue_measurements(ue, proc, dlsch, pdsch_est_size, dl_ch_estimates);

#if T_TRACER
    if(nr_slot_rx == 0)
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
  if (( nr_slot_rx == 2) && (l==(2-frame_parms->Ncp))) {

    // AGC

    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_GAIN_CONTROL, VCD_FUNCTION_IN);


    //printf("start adjust gain power avg db %d\n", ue->measurements.rx_power_avg_dB[gNB_id]);
    phy_adjust_gain_nr (ue,ue->measurements.rx_power_avg_dB[gNB_id],gNB_id);
    
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_GAIN_CONTROL, VCD_FUNCTION_OUT);

}

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_MEASUREMENT_PROCEDURES, VCD_FUNCTION_OUT);
}

static void nr_ue_pbch_procedures(PHY_VARS_NR_UE *ue,
                                  UE_nr_rxtx_proc_t *proc,
                                  int estimateSz,
                                  struct complex16 dl_ch_estimates[][estimateSz],
                                  nr_phy_data_t *phy_data,
                                  c16_t rxdataF[][ue->frame_parms.samples_per_slot_wCP]) {

  int ret = 0;
  DevAssert(ue);

  int frame_rx = proc->frame_rx;
  int nr_slot_rx = proc->nr_slot_rx;
  int gNB_id = proc->gNB_id;

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_PBCH_PROCEDURES, VCD_FUNCTION_IN);

  LOG_D(PHY,"[UE  %d] Frame %d Slot %d, Trying PBCH (NidCell %d, gNB_id %d)\n",ue->Mod_id,frame_rx,nr_slot_rx,ue->frame_parms.Nid_cell,gNB_id);
  fapiPbch_t result;
  ret = nr_rx_pbch(ue,
                   proc,
                   estimateSz,
                   dl_ch_estimates,
                   ue->pbch_vars[gNB_id],
                   &ue->frame_parms,
                   (ue->frame_parms.ssb_index)&7,
                   SISO,
                   phy_data,
                   &result,
                   rxdataF);

  if (ret==0) {

    ue->pbch_vars[gNB_id]->pdu_errors_conseq = 0;

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

int nr_ue_pdcch_procedures(PHY_VARS_NR_UE *ue,
                           UE_nr_rxtx_proc_t *proc,
                           int32_t pdcch_est_size,
                           int32_t pdcch_dl_ch_estimates[][pdcch_est_size],
                           nr_phy_data_t *phy_data,
                           int n_ss,
                           c16_t rxdataF[][ue->frame_parms.samples_per_slot_wCP])
{
  int frame_rx = proc->frame_rx;
  int nr_slot_rx = proc->nr_slot_rx;
  unsigned int dci_cnt=0;
  fapi_nr_dci_indication_t dci_ind = {0};
  nr_downlink_indication_t dl_indication;
  NR_UE_PDCCH_CONFIG *phy_pdcch_config = &phy_data->phy_pdcch_config;

  fapi_nr_dl_config_dci_dl_pdu_rel15_t *rel15 = &phy_pdcch_config->pdcch_config[n_ss];

  start_meas(&ue->dlsch_rx_pdcch_stats);

  /// PDCCH/DCI e-sequence (input to rate matching).
  int32_t pdcch_e_rx_size = NR_MAX_PDCCH_SIZE;
  int16_t pdcch_e_rx[pdcch_e_rx_size];

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_RX_PDCCH, VCD_FUNCTION_IN);
  nr_rx_pdcch(ue, proc, pdcch_est_size, pdcch_dl_ch_estimates, pdcch_e_rx, rel15, rxdataF);
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_RX_PDCCH, VCD_FUNCTION_OUT);
  

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_DCI_DECODING, VCD_FUNCTION_IN);

#ifdef NR_PDCCH_SCHED_DEBUG
  printf("<-NR_PDCCH_PHY_PROCEDURES_LTE_UE (nr_ue_pdcch_procedures)-> Entering function nr_dci_decoding_procedure for search space %d)\n",
	 n_ss);
#endif

  dci_cnt = nr_dci_decoding_procedure(ue, proc, pdcch_e_rx, &dci_ind, rel15);

#ifdef NR_PDCCH_SCHED_DEBUG
  LOG_I(PHY,"<-NR_PDCCH_PHY_PROCEDURES_LTE_UE (nr_ue_pdcch_procedures)-> Ending function nr_dci_decoding_procedure() -> dci_cnt=%u\n",dci_cnt);
#endif
  
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_DCI_DECODING, VCD_FUNCTION_OUT);
  //LOG_D(PHY,"[UE  %d][PUSCH] Frame %d nr_slot_rx %d PHICH RX\n",ue->Mod_id,frame_rx,nr_slot_rx);

  for (int i=0; i<dci_cnt; i++) {
    LOG_D(PHY,"[UE  %d] AbsSubFrame %d.%d: DCI %i of %d total DCIs found --> rnti %x : format %d\n",
          ue->Mod_id,frame_rx%1024,nr_slot_rx,
          i + 1,
          dci_cnt,
          dci_ind.dci_list[i].rnti,
          dci_ind.dci_list[i].dci_format);
  }

  dci_ind.number_of_dcis = dci_cnt;

  // fill dl_indication message
  nr_fill_dl_indication(&dl_indication, &dci_ind, NULL, proc, ue, phy_data);
  //  send to mac
  ue->if_inst->dl_indication(&dl_indication, NULL);


  stop_meas(&ue->dlsch_rx_pdcch_stats);
    
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_PDCCH_PROCEDURES, VCD_FUNCTION_OUT);
  return(dci_cnt);
}
#endif // NR_PDCCH_SCHED

int nr_ue_pdsch_procedures(PHY_VARS_NR_UE *ue,
                           UE_nr_rxtx_proc_t *proc,
                           NR_UE_DLSCH_t dlsch[2],
                           int16_t *llr[2],
                           c16_t rxdataF[][ue->frame_parms.samples_per_slot_wCP]) {

  int frame_rx = proc->frame_rx;
  int nr_slot_rx = proc->nr_slot_rx;
  int m;
  int first_symbol_flag=0;

  if (!dlsch[0].active)
    return 0;

  // We handle only one CW now
  if (!(NR_MAX_NB_LAYERS>4)) {
    NR_UE_DLSCH_t *dlsch0 = &dlsch[0];
    int harq_pid = dlsch0->dlsch_config.harq_process_nbr;
    NR_DL_UE_HARQ_t *dlsch0_harq = ue->dl_harq_processes[harq_pid];
    uint16_t BWPStart       = dlsch0->dlsch_config.BWPStart;
    uint16_t pdsch_start_rb = dlsch0->dlsch_config.start_rb;
    uint16_t pdsch_nb_rb    = dlsch0->dlsch_config.number_rbs;
    uint16_t s0             = dlsch0->dlsch_config.start_symbol;
    uint16_t s1             = dlsch0->dlsch_config.number_symbols;
    bool is_SI              = dlsch0->rnti_type == _SI_RNTI_;

    LOG_D(PHY,"[UE %d] nr_slot_rx %d, harq_pid %d (%d), rb_start %d, nb_rb %d, symbol_start %d, nb_symbols %d, DMRS mask %x, Nl %d\n",
          ue->Mod_id,nr_slot_rx,harq_pid,dlsch0_harq->status,pdsch_start_rb,pdsch_nb_rb,s0,s1,dlsch0->dlsch_config.dlDmrsSymbPos, dlsch0->Nl);

    const uint32_t pdsch_est_size = ((ue->frame_parms.symbols_per_slot*ue->frame_parms.ofdm_symbol_size+15)/16)*16;
    __attribute__ ((aligned(32))) int32_t pdsch_dl_ch_estimates[ue->frame_parms.nb_antennas_rx][pdsch_est_size];
    memset(pdsch_dl_ch_estimates, 0, sizeof(int32_t)*ue->frame_parms.nb_antennas_rx*pdsch_est_size);

    for (m = s0; m < (s0 +s1); m++) {
      if (dlsch0->dlsch_config.dlDmrsSymbPos & (1 << m)) {
        for (uint8_t aatx=0; aatx<dlsch0->Nl; aatx++) {//for MIMO Config: it shall loop over no_layers
          LOG_D(PHY,"PDSCH Channel estimation gNB id %d, PDSCH antenna port %d, slot %d, symbol %d\n",0,aatx,nr_slot_rx,m);
          nr_pdsch_channel_estimation(ue,
                                      proc,
                                      is_SI,
                                      get_dmrs_port(aatx,dlsch0->dlsch_config.dmrs_ports),
                                      m,
                                      dlsch0->dlsch_config.nscid,
                                      dlsch0->dlsch_config.dlDmrsScramblingId,
                                      BWPStart,
                                      dlsch0->dlsch_config.dmrsConfigType,
                                      ue->frame_parms.first_carrier_offset+(BWPStart + pdsch_start_rb)*12,
                                      pdsch_nb_rb,
                                      pdsch_est_size,
                                      pdsch_dl_ch_estimates,
                                      rxdataF);
#if 0
          ///LOG_M: the channel estimation
          int nr_frame_rx = proc->frame_rx;
          char filename[100];
          for (uint8_t aarx=0; aarx<ue->frame_parms.nb_antennas_rx; aarx++) {
            sprintf(filename,"PDSCH_CHANNEL_frame%d_slot%d_sym%d_port%d_rx%d.m", nr_frame_rx, nr_slot_rx, m, aatx,aarx);
            int **dl_ch_estimates = ue->pdsch_vars[gNB_id]->dl_ch_estimates;
            LOG_M(filename,"channel_F",&dl_ch_estimates[aatx*ue->frame_parms.nb_antennas_rx+aarx][ue->frame_parms.ofdm_symbol_size*m],ue->frame_parms.ofdm_symbol_size, 1, 1);
          }
#endif
        }
      }
    }
    nr_ue_measurement_procedures(2, ue, proc, &dlsch[0], pdsch_est_size, pdsch_dl_ch_estimates);

    if (ue->chest_time == 1) { // averaging time domain channel estimates
      nr_chest_time_domain_avg(&ue->frame_parms,
                               (int32_t **) pdsch_dl_ch_estimates,
                               dlsch0->dlsch_config.number_symbols,
                               dlsch0->dlsch_config.start_symbol,
                               dlsch0->dlsch_config.dlDmrsSymbPos,
                               pdsch_nb_rb);
    }

    uint16_t first_symbol_with_data = s0;
    uint32_t dmrs_data_re;

    if (dlsch0->dlsch_config.dmrsConfigType == NFAPI_NR_DMRS_TYPE1)
      dmrs_data_re = 12 - 6 * dlsch0->dlsch_config.n_dmrs_cdm_groups;
    else
      dmrs_data_re = 12 - 4 * dlsch0->dlsch_config.n_dmrs_cdm_groups;

    while ((dmrs_data_re == 0) && (dlsch0->dlsch_config.dlDmrsSymbPos & (1 << first_symbol_with_data))) {
      first_symbol_with_data++;
    }

    c16_t ptrs_phase_per_slot[ue->frame_parms.nb_antennas_rx][NR_SYMBOLS_PER_SLOT];
    memset(ptrs_phase_per_slot, 0, ue->frame_parms.nb_antennas_rx*NR_SYMBOLS_PER_SLOT*sizeof(c16_t));

    int32_t ptrs_re_per_slot[ue->frame_parms.nb_antennas_rx][NR_SYMBOLS_PER_SLOT];
    memset(ptrs_re_per_slot, 0, ue->frame_parms.nb_antennas_rx*NR_SYMBOLS_PER_SLOT*sizeof(c16_t));

    uint32_t dl_valid_re[NR_SYMBOLS_PER_SLOT] = {0};
    uint32_t llr_offset[NR_SYMBOLS_PER_SLOT] = {0};

    const uint32_t rx_size = NR_SYMBOLS_PER_SLOT*
                             dlsch[0].dlsch_config.number_rbs*
                             NR_NB_SC_PER_RB;
    __attribute__ ((aligned(32))) int32_t dl_ch_estimates_ext[ue->frame_parms.nb_antennas_rx][rx_size];
    memset(dl_ch_estimates_ext, 0, ue->frame_parms.nb_antennas_rx*rx_size*sizeof(int32_t));

    __attribute__ ((aligned(32))) int32_t rxdataF_ext[ue->frame_parms.nb_antennas_rx][rx_size];
    memset(rxdataF_ext, 0, ue->frame_parms.nb_antennas_rx*rx_size*sizeof(int32_t));

    __attribute__ ((aligned(32))) int32_t rxdataF_comp[ue->frame_parms.nb_antennas_rx][rx_size];
    memset(rxdataF_comp, 0, ue->frame_parms.nb_antennas_rx*rx_size*sizeof(int32_t));

    __attribute__ ((aligned(32))) int32_t dl_ch_mag[ue->frame_parms.nb_antennas_rx][rx_size];
    memset(dl_ch_mag, 0, ue->frame_parms.nb_antennas_rx*rx_size*sizeof(int32_t));

    __attribute__ ((aligned(32))) int32_t dl_ch_magb[ue->frame_parms.nb_antennas_rx][rx_size];
    memset(dl_ch_magb, 0, ue->frame_parms.nb_antennas_rx*rx_size*sizeof(int32_t));

    __attribute__ ((aligned(32))) int32_t dl_ch_magr[ue->frame_parms.nb_antennas_rx][rx_size];
    memset(dl_ch_magr, 0, ue->frame_parms.nb_antennas_rx*rx_size*sizeof(int32_t));

    int32_t log2_maxh = 0;
    start_meas(&ue->rx_pdsch_stats);
    for (m = s0; m < (s1 + s0); m++) {
 
      if (m==first_symbol_with_data)
        first_symbol_flag = 1;
      else
        first_symbol_flag = 0;

      uint8_t slot = 0;
      if(m >= ue->frame_parms.symbols_per_slot>>1)
        slot = 1;
      start_meas(&ue->dlsch_llr_stats_parallelization[slot]);
      // process DLSCH received symbols in the slot
      // symbol by symbol processing (if data/DMRS are multiplexed is checked inside the function)
      if (nr_rx_pdsch(ue,
                      proc,
                      dlsch,
                      m,
                      first_symbol_flag,
                      harq_pid,
                      pdsch_est_size,
                      pdsch_dl_ch_estimates,
                      llr,
                      ptrs_phase_per_slot,
                      ptrs_re_per_slot,
                      dl_valid_re,
                      rx_size,
                      dl_ch_estimates_ext,
                      rxdataF_ext,
                      rxdataF_comp,
                      dl_ch_mag,
                      dl_ch_magb,
                      dl_ch_magr,
                      rxdataF,
                      llr_offset,
                      &log2_maxh) < 0)
        return -1;

      stop_meas(&ue->dlsch_llr_stats_parallelization[slot]);
      if (cpumeas(CPUMEAS_GETSTATE))
        LOG_D(PHY, "[AbsSFN %d.%d] LLR Computation Symbol %d %5.2f \n",frame_rx,nr_slot_rx,m,ue->dlsch_llr_stats_parallelization[slot].p_time/(cpuf*1000.0));
      if(first_symbol_flag) {
        proc->first_symbol_available = 1;
      }
    } // CRNTI active
    stop_meas(&ue->rx_pdsch_stats);

    UEscopeCopy(ue, pdschRxdataF_comp, rxdataF_comp, sizeof(struct complex16), ue->frame_parms.nb_antennas_rx, rx_size);

    if (ue->phy_sim_pdsch_rxdataF_comp)
      memcpy(ue->phy_sim_pdsch_rxdataF_comp, rxdataF_comp, sizeof(int32_t)*rx_size*ue->frame_parms.nb_antennas_rx);
    if (ue->phy_sim_pdsch_rxdataF_ext)
      memcpy(ue->phy_sim_pdsch_rxdataF_ext, rxdataF_ext, sizeof(int32_t)*rx_size*ue->frame_parms.nb_antennas_rx);
    if (ue->phy_sim_pdsch_dl_ch_estimates_ext)
      memcpy(ue->phy_sim_pdsch_dl_ch_estimates_ext, dl_ch_estimates_ext, sizeof(int32_t)*rx_size*ue->frame_parms.nb_antennas_rx);
    if (ue->phy_sim_pdsch_dl_ch_estimates)
      memcpy(ue->phy_sim_pdsch_dl_ch_estimates, dl_ch_estimates_ext, sizeof(int32_t)*rx_size*ue->frame_parms.nb_antennas_rx);
  }
  return 0;
}

bool nr_ue_dlsch_procedures(PHY_VARS_NR_UE *ue,
                            UE_nr_rxtx_proc_t *proc,
                            NR_UE_DLSCH_t dlsch[2],
                            int16_t* llr[2]) {

  if (dlsch[0].active == false) {
    LOG_E(PHY, "DLSCH should be active when calling this function\n");
    return 1;
  }

  int gNB_id = proc->gNB_id;
  bool dec = false;
  int harq_pid = dlsch[0].dlsch_config.harq_process_nbr;
  int frame_rx = proc->frame_rx;
  int nr_slot_rx = proc->nr_slot_rx;
  uint32_t ret = UINT32_MAX, ret1 = UINT32_MAX;
  NR_DL_UE_HARQ_t *dl_harq0 = &ue->dl_harq_processes[0][harq_pid];
  NR_DL_UE_HARQ_t *dl_harq1 = &ue->dl_harq_processes[1][harq_pid];
  uint16_t dmrs_len = get_num_dmrs(dlsch[0].dlsch_config.dlDmrsSymbPos);
  nr_downlink_indication_t dl_indication;
  fapi_nr_rx_indication_t *rx_ind = calloc(1, sizeof(*rx_ind));
  uint16_t number_pdus = 1;
  // params for UL time alignment procedure
  NR_UL_TIME_ALIGNMENT_t *ul_time_alignment = &ue->ul_time_alignment[gNB_id];

  uint8_t is_cw0_active = dl_harq0->status;
  uint8_t is_cw1_active = dl_harq1->status;
  uint16_t nb_symb_sch = dlsch[0].dlsch_config.number_symbols;
  uint16_t start_symbol = dlsch[0].dlsch_config.start_symbol;
  uint8_t dmrs_type = dlsch[0].dlsch_config.dmrsConfigType;

  uint8_t nb_re_dmrs;
  if (dmrs_type==NFAPI_NR_DMRS_TYPE1) {
    nb_re_dmrs = 6*dlsch[0].dlsch_config.n_dmrs_cdm_groups;
  }
  else {
    nb_re_dmrs = 4*dlsch[0].dlsch_config.n_dmrs_cdm_groups;
  }

  LOG_D(PHY,"AbsSubframe %d.%d Start LDPC Decoder for CW0 [harq_pid %d] ? %d \n", frame_rx%1024, nr_slot_rx, harq_pid, is_cw0_active);
  LOG_D(PHY,"AbsSubframe %d.%d Start LDPC Decoder for CW1 [harq_pid %d] ? %d \n", frame_rx%1024, nr_slot_rx, harq_pid, is_cw1_active);

  // exit dlsch procedures as there are no active dlsch
  if (is_cw0_active != ACTIVE && is_cw1_active != ACTIVE)
    return false;

  // start ldpc decode for CW 0
  dl_harq0->G = nr_get_G(dlsch[0].dlsch_config.number_rbs,
                         nb_symb_sch,
                         nb_re_dmrs,
                         dmrs_len,
                         dlsch[0].dlsch_config.qamModOrder,
                         dlsch[0].Nl);

  start_meas(&ue->dlsch_unscrambling_stats);
  nr_dlsch_unscrambling(llr[0],
                        dl_harq0->G,
                        0,
                        ue->frame_parms.Nid_cell,
                        dlsch[0].rnti);
    

  stop_meas(&ue->dlsch_unscrambling_stats);

  start_meas(&ue->dlsch_decoding_stats);

  ret = nr_dlsch_decoding(ue,
                          proc,
                          gNB_id,
                          llr[0],
                          &ue->frame_parms,
                          &dlsch[0],
                          dl_harq0,
                          frame_rx,
                          nb_symb_sch,
                          nr_slot_rx,
                          harq_pid);

  LOG_T(PHY,"dlsch decoding, ret = %d\n", ret);


  if(ret<ue->max_ldpc_iterations+1)
    dec = true;

  int ind_type = -1;
  switch(dlsch[0].rnti_type) {
    case _RA_RNTI_:
      ind_type = FAPI_NR_RX_PDU_TYPE_RAR;
      break;

    case _SI_RNTI_:
      ind_type = FAPI_NR_RX_PDU_TYPE_SIB;
      break;

    case _C_RNTI_:
      ind_type = FAPI_NR_RX_PDU_TYPE_DLSCH;
      break;

    default:
      AssertFatal(true, "Invalid DLSCH type %d\n", dlsch[0].rnti_type);
      break;
  }

  nr_fill_dl_indication(&dl_indication, NULL, rx_ind, proc, ue, NULL);
  nr_fill_rx_indication(rx_ind, ind_type, ue, &dlsch[0], NULL, number_pdus, proc, NULL);

  LOG_D(PHY, "In %s DL PDU length in bits: %d, in bytes: %d \n", __FUNCTION__, dlsch[0].dlsch_config.TBS, dlsch[0].dlsch_config.TBS / 8);

  stop_meas(&ue->dlsch_decoding_stats);
  if (cpumeas(CPUMEAS_GETSTATE))  {
    LOG_D(PHY, " --> Unscrambling for CW0 %5.3f\n",
          (ue->dlsch_unscrambling_stats.p_time)/(cpuf*1000.0));
    LOG_D(PHY, "AbsSubframe %d.%d --> LDPC Decoding for CW0 %5.3f\n",
          frame_rx%1024, nr_slot_rx,(ue->dlsch_decoding_stats.p_time)/(cpuf*1000.0));
  }

  if(is_cw1_active) {
    // start ldpc decode for CW 1
    dl_harq1->G = nr_get_G(dlsch[1].dlsch_config.number_rbs,
                           nb_symb_sch,
                           nb_re_dmrs,
                           dmrs_len,
                           dlsch[1].dlsch_config.qamModOrder,
                           dlsch[1].Nl);
    start_meas(&ue->dlsch_unscrambling_stats);
    nr_dlsch_unscrambling(llr[1],
                          dl_harq1->G,
                          0,
                          ue->frame_parms.Nid_cell,
                          dlsch[1].rnti);
    stop_meas(&ue->dlsch_unscrambling_stats);

    start_meas(&ue->dlsch_decoding_stats);

    ret1 = nr_dlsch_decoding(ue,
                             proc,
                             gNB_id,
                             llr[1],
                             &ue->frame_parms,
                             &dlsch[1],
                             dl_harq1,
                             frame_rx,
                             nb_symb_sch,
                             nr_slot_rx,
                             harq_pid);
    LOG_T(PHY,"CW dlsch decoding, ret1 = %d\n", ret1);

    stop_meas(&ue->dlsch_decoding_stats);
    if (cpumeas(CPUMEAS_GETSTATE)) {
      LOG_D(PHY, " --> Unscrambling for CW1 %5.3f\n",
            (ue->dlsch_unscrambling_stats.p_time)/(cpuf*1000.0));
      LOG_D(PHY, "AbsSubframe %d.%d --> ldpc Decoding for CW1 %5.3f\n",
            frame_rx%1024, nr_slot_rx,(ue->dlsch_decoding_stats.p_time)/(cpuf*1000.0));
      }
  LOG_D(PHY, "harq_pid: %d, TBS expected dlsch1: %d \n", harq_pid, dlsch[1].dlsch_config.TBS);
  }
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
    NR_BWP_Id_t dl_bwp = mac->DL_BWP_Id;
    NR_BWP_Id_t ul_bwp = mac->UL_BWP_Id;

    NR_PUSCH_TimeDomainResourceAllocationList_t *pusch_TimeDomainAllocationList = NULL;
    if(ul_bwp){
      if (mac->ULbwp[ul_bwp-1] &&
          mac->ULbwp[ul_bwp-1]->bwp_Dedicated &&
          mac->ULbwp[ul_bwp-1]->bwp_Dedicated->pusch_Config &&
          mac->ULbwp[ul_bwp-1]->bwp_Dedicated->pusch_Config->choice.setup &&
          mac->ULbwp[ul_bwp-1]->bwp_Dedicated->pusch_Config->choice.setup->pusch_TimeDomainAllocationList) {
        pusch_TimeDomainAllocationList = mac->ULbwp[ul_bwp-1]->bwp_Dedicated->pusch_Config->choice.setup->pusch_TimeDomainAllocationList->choice.setup;
      }
      else if (mac->ULbwp[ul_bwp-1] &&
               mac->ULbwp[ul_bwp-1]->bwp_Common &&
               mac->ULbwp[ul_bwp-1]->bwp_Common->pusch_ConfigCommon &&
               mac->ULbwp[ul_bwp-1]->bwp_Common->pusch_ConfigCommon->choice.setup &&
               mac->ULbwp[ul_bwp-1]->bwp_Common->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList) {
        pusch_TimeDomainAllocationList = mac->ULbwp[ul_bwp-1]->bwp_Common->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList;
      }
    }
    else if (mac->scc_SIB &&
             mac->scc_SIB->uplinkConfigCommon &&
             mac->scc_SIB->uplinkConfigCommon->initialUplinkBWP.pusch_ConfigCommon &&
             mac->scc_SIB->uplinkConfigCommon->initialUplinkBWP.pusch_ConfigCommon->choice.setup &&
             mac->scc_SIB->uplinkConfigCommon->initialUplinkBWP.pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList) {
      pusch_TimeDomainAllocationList = mac->scc_SIB->uplinkConfigCommon->initialUplinkBWP.pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList;
    }
    long mapping_type_ul = pusch_TimeDomainAllocationList ? pusch_TimeDomainAllocationList->list.array[0]->mappingType : NR_PUSCH_TimeDomainResourceAllocation__mappingType_typeA;

    NR_PDSCH_Config_t *pdsch_Config = NULL;
    NR_PDSCH_TimeDomainResourceAllocationList_t *pdsch_TimeDomainAllocationList = NULL;
    if(dl_bwp){
      pdsch_Config = (mac->DLbwp[dl_bwp-1] && mac->DLbwp[dl_bwp-1]->bwp_Dedicated->pdsch_Config->choice.setup) ? mac->DLbwp[dl_bwp-1]->bwp_Dedicated->pdsch_Config->choice.setup : NULL;
      if (mac->DLbwp[dl_bwp-1] && mac->DLbwp[dl_bwp-1]->bwp_Dedicated->pdsch_Config->choice.setup->pdsch_TimeDomainAllocationList)
        pdsch_TimeDomainAllocationList = pdsch_Config->pdsch_TimeDomainAllocationList->choice.setup;
      else if (mac->DLbwp[dl_bwp-1] && mac->DLbwp[dl_bwp-1]->bwp_Common->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList)
        pdsch_TimeDomainAllocationList = mac->DLbwp[dl_bwp-1]->bwp_Common->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList;
    }
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
    const double d_2_2 = pusch_d_2_2_timing_capability_1[numerology][1];

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
  return dec;
}


int phy_procedures_nrUE_RX(PHY_VARS_NR_UE *ue,
                           UE_nr_rxtx_proc_t *proc,
                           nr_phy_data_t *phy_data) {

  int frame_rx = proc->frame_rx;
  int nr_slot_rx = proc->nr_slot_rx;
  int gNB_id = proc->gNB_id;
  fapi_nr_config_request_t *cfg = &ue->nrUE_config;
  NR_DL_FRAME_PARMS *fp = &ue->frame_parms;
  NR_UE_PDCCH_CONFIG *phy_pdcch_config = &phy_data->phy_pdcch_config;
  nr_ue_dlsch_init(phy_data->dlsch, NR_MAX_NB_LAYERS>4 ? 2:1, ue->max_ldpc_iterations);
  
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_UE_RX, VCD_FUNCTION_IN);
  start_meas(&ue->phy_proc_rx);

  LOG_D(PHY," ****** start RX-Chain for Frame.Slot %d.%d ******  \n",
        frame_rx%1024, nr_slot_rx);

  const uint32_t rxdataF_sz = ue->frame_parms.samples_per_slot_wCP;
  __attribute__ ((aligned(32))) c16_t rxdataF[ue->frame_parms.nb_antennas_rx][rxdataF_sz];
  // checking if current frame is compatible with SSB periodicity
  if (cfg->ssb_table.ssb_period == 0 ||
      !(frame_rx%(1<<(cfg->ssb_table.ssb_period-1)))){

    const int estimateSz = fp->symbols_per_slot * fp->ofdm_symbol_size;
    // loop over SSB blocks
    for(int ssb_index=0; ssb_index<fp->Lmax; ssb_index++) {
      uint32_t curr_mask = cfg->ssb_table.ssb_mask_list[ssb_index/32].ssb_mask;
      // check if if current SSB is transmitted
      if ((curr_mask >> (31-(ssb_index%32))) &0x01) {
        int ssb_start_symbol = nr_get_ssb_start_symbol(fp, ssb_index);
        int ssb_slot = ssb_start_symbol/fp->symbols_per_slot;
        int ssb_slot_2 = (cfg->ssb_table.ssb_period == 0) ? ssb_slot+(fp->slots_per_frame>>1) : -1;

        if (ssb_slot == nr_slot_rx ||
            ssb_slot_2 == nr_slot_rx) {

          VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_SLOT_FEP_PBCH, VCD_FUNCTION_IN);
          LOG_D(PHY," ------  PBCH ChannelComp/LLR: frame.slot %d.%d ------  \n", frame_rx%1024, nr_slot_rx);

          __attribute__ ((aligned(32))) struct complex16 dl_ch_estimates[fp->nb_antennas_rx][estimateSz];
          __attribute__ ((aligned(32))) struct complex16 dl_ch_estimates_time[fp->nb_antennas_rx][fp->ofdm_symbol_size];

          for (int i=1; i<4; i++) {
            nr_slot_fep(ue,
                        proc,
                        (ssb_start_symbol+i)%(fp->symbols_per_slot),
                        rxdataF);

            start_meas(&ue->dlsch_channel_estimation_stats);
            nr_pbch_channel_estimation(ue,
                                       estimateSz,
                                       dl_ch_estimates,
                                       dl_ch_estimates_time,
                                       proc,
                                       (ssb_start_symbol+i)%(fp->symbols_per_slot),
                                       i-1,
                                       ssb_index&7,
                                       ssb_slot_2 == nr_slot_rx,
                                       rxdataF);
            stop_meas(&ue->dlsch_channel_estimation_stats);
          }

          nr_ue_ssb_rsrp_measurements(ue, ssb_index, proc, rxdataF);

          // resetting ssb index for PBCH detection if there is a stronger SSB index
          if(ue->measurements.ssb_rsrp_dBm[ssb_index] > ue->measurements.ssb_rsrp_dBm[fp->ssb_index])
            fp->ssb_index = ssb_index;

          if(ssb_index == fp->ssb_index) {

            LOG_D(PHY," ------  Decode MIB: frame.slot %d.%d ------  \n", frame_rx%1024, nr_slot_rx);
            nr_ue_pbch_procedures(ue, proc, estimateSz, dl_ch_estimates, phy_data, rxdataF);

            if (ue->no_timing_correction==0) {
              LOG_D(PHY,"start adjust sync slot = %d no timing %d\n", nr_slot_rx, ue->no_timing_correction);
              nr_adjust_synch_ue(fp,
                                 ue,
                                 gNB_id,
                                 fp->ofdm_symbol_size,
                                 dl_ch_estimates_time,
                                 frame_rx,
                                 nr_slot_rx,
                                 0,
                                 16384);
            }
          }
          LOG_D(PHY, "Doing N0 measurements in %s\n", __FUNCTION__);
          nr_ue_rrc_measurements(ue, proc, rxdataF);
          VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_SLOT_FEP_PBCH, VCD_FUNCTION_OUT);
        }
      }
    }
  }

  // Check for PRS slot - section 7.4.1.7.4 in 3GPP rel16 38.211
  for(int gNB_id = 0; gNB_id < ue->prs_active_gNBs; gNB_id++)
  {
    for(int rsc_id = 0; rsc_id < ue->prs_vars[gNB_id]->NumPRSResources; rsc_id++)
    {
      prs_config_t *prs_config = &ue->prs_vars[gNB_id]->prs_resource[rsc_id].prs_cfg;
      for (int i = 0; i < prs_config->PRSResourceRepetition; i++)
      {
        if( (((frame_rx*fp->slots_per_frame + nr_slot_rx) - (prs_config->PRSResourceSetPeriod[1] + prs_config->PRSResourceOffset) + prs_config->PRSResourceSetPeriod[0])%prs_config->PRSResourceSetPeriod[0]) == i*prs_config->PRSResourceTimeGap)
        {
          for(int j = prs_config->SymbolStart; j < (prs_config->SymbolStart+prs_config->NumPRSSymbols); j++)
          {
            nr_slot_fep(ue,
                        proc,
                        (j%fp->symbols_per_slot),
                        rxdataF);
          }
          nr_prs_channel_estimation(rsc_id,
                                    i,
                                    ue,
                                    proc,
                                    fp,
                                    rxdataF);
        }
      } // for i
    } // for rsc_id
  } // for gNB_id

  if ((frame_rx%64 == 0) && (nr_slot_rx==0)) {
    LOG_I(NR_PHY,"============================================\n");
    // fixed text + 8 HARQs rounds à 10 ("999999999/") + NULL
    // if we use 999999999 HARQs, that should be sufficient for at least 138 hours
    const size_t harq_output_len = 31 + 10 * 8 + 1;
    char output[harq_output_len];
    char *p = output;
    const char *end = output + harq_output_len;
    p += snprintf(p, end - p, "Harq round stats for Downlink: %d", ue->dl_stats[0]);
    for (int round = 1; round < 16 && (round < 3 || ue->dl_stats[round] != 0); ++round)
      p += snprintf(p, end - p,"/%d", ue->dl_stats[round]);
    LOG_I(NR_PHY,"%s/0\n", output);

    LOG_I(NR_PHY,"============================================\n");
  }

#ifdef NR_PDCCH_SCHED

  LOG_D(PHY," ------ --> PDCCH ChannelComp/LLR Frame.slot %d.%d ------  \n", frame_rx%1024, nr_slot_rx);
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_SLOT_FEP_PDCCH, VCD_FUNCTION_IN);

  uint8_t nb_symb_pdcch = phy_pdcch_config->nb_search_space > 0 ? phy_pdcch_config->pdcch_config[0].coreset.duration : 0;
  for (uint16_t l=0; l<nb_symb_pdcch; l++) {

    start_meas(&ue->ofdm_demod_stats);
    nr_slot_fep(ue,
                proc,
                l,
                rxdataF);
  }

    // Hold the channel estimates in frequency domain.
  int32_t pdcch_est_size = ((((fp->symbols_per_slot*(fp->ofdm_symbol_size+LTE_CE_FILTER_LENGTH))+15)/16)*16);
  __attribute__ ((aligned(16))) int32_t pdcch_dl_ch_estimates[4*fp->nb_antennas_rx][pdcch_est_size];

  uint8_t dci_cnt = 0;
  for(int n_ss = 0; n_ss<phy_pdcch_config->nb_search_space; n_ss++) {
    for (uint16_t l=0; l<nb_symb_pdcch; l++) {

      // note: this only works if RBs for PDCCH are contigous!

      nr_pdcch_channel_estimation(ue,
                                  proc,
                                  l,
                                  &phy_pdcch_config->pdcch_config[n_ss].coreset,
                                  fp->first_carrier_offset,
                                  phy_pdcch_config->pdcch_config[n_ss].BWPStart,
                                  pdcch_est_size,
                                  pdcch_dl_ch_estimates,
                                  rxdataF);

      stop_meas(&ue->ofdm_demod_stats);

    }
    dci_cnt = dci_cnt + nr_ue_pdcch_procedures(ue, proc, pdcch_est_size, pdcch_dl_ch_estimates, phy_data, n_ss, rxdataF);
  }
  phy_pdcch_config->nb_search_space = 0;
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_SLOT_FEP_PDCCH, VCD_FUNCTION_OUT);

  NR_UE_DLSCH_t *dlsch = &phy_data->dlsch[0];
  if (dci_cnt > 0) {

    LOG_D(PHY,"[UE %d] Frame %d, nr_slot_rx %d: found %d DCIs\n", ue->Mod_id, frame_rx, nr_slot_rx, dci_cnt);

    if (dlsch->active) {
      VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_SLOT_FEP_PDSCH, VCD_FUNCTION_IN);
      uint16_t nb_symb_sch = dlsch[0].dlsch_config.number_symbols;
      uint16_t start_symb_sch = dlsch[0].dlsch_config.start_symbol;

      LOG_D(PHY," ------ --> PDSCH ChannelComp/LLR Frame.slot %d.%d ------  \n", frame_rx%1024, nr_slot_rx);
      //to update from pdsch config

      for (uint16_t m=start_symb_sch;m<(nb_symb_sch+start_symb_sch) ; m++){
        nr_slot_fep(ue,
                    proc,
                    m,  //to be updated from higher layer
                    rxdataF);
      }
      VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_SLOT_FEP_PDSCH, VCD_FUNCTION_OUT);
    }
  } else {
    LOG_D(PHY,"[UE %d] Frame %d, nr_slot_rx %d: No DCIs found\n", ue->Mod_id, frame_rx, nr_slot_rx);
  }

#endif //NR_PDCCH_SCHED

  start_meas(&ue->generic_stat);
  // do procedures for C-RNTI
  int ret_pdsch = 0;
  if (dlsch[0].active) {

    uint8_t nb_re_dmrs;
    if (dlsch[0].dlsch_config.dmrsConfigType == NFAPI_NR_DMRS_TYPE1) {
      nb_re_dmrs = 6*dlsch[0].dlsch_config.n_dmrs_cdm_groups;
    }
    else {
      nb_re_dmrs = 4*dlsch[0].dlsch_config.n_dmrs_cdm_groups;
    }
    uint16_t dmrs_len = get_num_dmrs(dlsch[0].dlsch_config.dlDmrsSymbPos);

    const uint32_t rx_llr_size = nr_get_G(dlsch[0].dlsch_config.number_rbs,
                                          dlsch[0].dlsch_config.number_symbols,
                                          nb_re_dmrs,
                                          dmrs_len,
                                          dlsch[0].dlsch_config.qamModOrder,
                                          dlsch[0].Nl);
    const uint32_t rx_llr_buf_sz = ((rx_llr_size+15)/16)*16;
    const uint32_t nb_codewords = NR_MAX_NB_LAYERS > 4 ? 2 : 1;
    int16_t* llr[2];
    for (int i=0; i<nb_codewords; i++)
      llr[i] = (int16_t *)malloc16_clear(rx_llr_buf_sz*sizeof(int16_t));

    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PDSCH_PROC_C, VCD_FUNCTION_IN);
    ret_pdsch = nr_ue_pdsch_procedures(ue,
                                       proc,
                                       dlsch,
                                       llr,
                                       rxdataF);

    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PDSCH_PROC_C, VCD_FUNCTION_OUT);

    UEscopeCopy(ue, pdschLlr, llr[0], sizeof(int16_t), 1, rx_llr_size);

    LOG_D(PHY, "DLSCH data reception at nr_slot_rx: %d\n", nr_slot_rx);
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PDSCH_PROC, VCD_FUNCTION_IN);

    start_meas(&ue->dlsch_procedures_stat);

    if (ret_pdsch >= 0)
      nr_ue_dlsch_procedures(ue, proc, dlsch, llr);

    stop_meas(&ue->dlsch_procedures_stat);
    if (cpumeas(CPUMEAS_GETSTATE)) {
      LOG_D(PHY, "[SFN %d] Slot1:       Pdsch Proc %5.2f\n",nr_slot_rx,ue->pdsch_procedures_stat.p_time/(cpuf*1000.0));
      LOG_D(PHY, "[SFN %d] Slot0 Slot1: Dlsch Proc %5.2f\n",nr_slot_rx,ue->dlsch_procedures_stat.p_time/(cpuf*1000.0));
    }

    if (ue->phy_sim_rxdataF)
      memcpy(ue->phy_sim_rxdataF, rxdataF, sizeof(int32_t)*rxdataF_sz*ue->frame_parms.nb_antennas_rx);
    if (ue->phy_sim_pdsch_llr)
      memcpy(ue->phy_sim_pdsch_llr, llr[0], sizeof(int16_t)*rx_llr_buf_sz);

    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PDSCH_PROC, VCD_FUNCTION_OUT);
    for (int i=0; i<nb_codewords; i++)
      free(llr[i]);
  }

  // do procedures for CSI-IM
  if ((ue->csiim_vars[gNB_id]) && (ue->csiim_vars[gNB_id]->active == 1)) {
    int l_csiim[4] = {-1, -1, -1, -1};
    for(int symb_idx = 0; symb_idx < 4; symb_idx++) {
      bool nr_slot_fep_done = false;
      for (int symb_idx2 = 0; symb_idx2 < symb_idx; symb_idx2++) {
        if (l_csiim[symb_idx2] == ue->csiim_vars[gNB_id]->csiim_config_pdu.l_csiim[symb_idx]) {
          nr_slot_fep_done = true;
        }
      }
      l_csiim[symb_idx] = ue->csiim_vars[gNB_id]->csiim_config_pdu.l_csiim[symb_idx];
      if(nr_slot_fep_done == false) {
        nr_slot_fep(ue, proc, ue->csiim_vars[gNB_id]->csiim_config_pdu.l_csiim[symb_idx], rxdataF);
      }
    }
    nr_ue_csi_im_procedures(ue, proc, rxdataF);
    ue->csiim_vars[gNB_id]->active = 0;
  }

  // do procedures for CSI-RS
  if ((ue->csirs_vars[gNB_id]) && (ue->csirs_vars[gNB_id]->active == 1)) {
    for(int symb = 0; symb < NR_SYMBOLS_PER_SLOT; symb++) {
      if(is_csi_rs_in_symbol(ue->csirs_vars[gNB_id]->csirs_config_pdu,symb)) {
        nr_slot_fep(ue, proc, symb, rxdataF);
      }
    }
    nr_ue_csi_rs_procedures(ue, proc, rxdataF);
    ue->csirs_vars[gNB_id]->active = 0;
  }

  start_meas(&ue->generic_stat);

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
  if (cpumeas(CPUMEAS_GETSTATE))
    LOG_D(PHY,"after tubo until end of Rx %5.2f \n",ue->generic_stat.p_time/(cpuf*1000.0));

#ifdef EMOS
  phy_procedures_emos_UE_RX(ue,slot,gNB_id);
#endif


  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_UE_RX, VCD_FUNCTION_OUT);

  stop_meas(&ue->phy_proc_rx);
  if (cpumeas(CPUMEAS_GETSTATE))
    LOG_D(PHY, "------FULL RX PROC [SFN %d]: %5.2f ------\n",nr_slot_rx,ue->phy_proc_rx.p_time/(cpuf*1000.0));

  LOG_D(PHY," ****** end RX-Chain  for AbsSubframe %d.%d ******  \n", frame_rx%1024, nr_slot_rx);
  UEscopeCopy(ue, commonRxdataF, rxdataF, sizeof(int32_t), ue->frame_parms.nb_antennas_rx, rxdataF_sz);
  return (0);
}


// todo:
// - power control as per 38.213 ch 7.4
void nr_ue_prach_procedures(PHY_VARS_NR_UE *ue, UE_nr_rxtx_proc_t *proc) {

  int gNB_id = proc->gNB_id;
  int frame_tx = proc->frame_tx, nr_slot_tx = proc->nr_slot_tx, prach_power; // tx_amp
  uint8_t mod_id = ue->Mod_id;

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_UE_TX_PRACH, VCD_FUNCTION_IN);

  if (ue->prach_vars[gNB_id]->active) {

    fapi_nr_ul_config_prach_pdu *prach_pdu = &ue->prach_vars[gNB_id]->prach_pdu;
    ue->tx_power_dBm[nr_slot_tx] = prach_pdu->prach_tx_power;

    LOG_D(PHY, "In %s: [UE %d][RAPROC][%d.%d]: Generating PRACH Msg1 (preamble %d, P0_PRACH %d)\n",
          __FUNCTION__,
          mod_id,
          frame_tx,
          nr_slot_tx,
          prach_pdu->ra_PreambleIndex,
          ue->tx_power_dBm[nr_slot_tx]);

    ue->prach_vars[gNB_id]->amp = AMP;

    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_GENERATE_PRACH, VCD_FUNCTION_IN);

    prach_power = generate_nr_prach(ue, gNB_id, frame_tx, nr_slot_tx);

    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_GENERATE_PRACH, VCD_FUNCTION_OUT);

    LOG_D(PHY, "In %s: [UE %d][RAPROC][%d.%d]: Generated PRACH Msg1 (TX power PRACH %d dBm, digital power %d dBW (amp %d)\n",
      __FUNCTION__,
      mod_id,
      frame_tx,
      nr_slot_tx,
      ue->tx_power_dBm[nr_slot_tx],
      dB_fixed(prach_power),
      ue->prach_vars[gNB_id]->amp);

    ue->prach_vars[gNB_id]->active = false;
  }

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_UE_TX_PRACH, VCD_FUNCTION_OUT);

}
