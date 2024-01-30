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
#include "PHY/INIT/nr_phy_init.h"
#include "PHY/NR_REFSIG/ptrs_nr.h"
#include "PHY/NR_UE_TRANSPORT/nr_transport_ue.h"
#include "PHY/NR_UE_TRANSPORT/nr_transport_proto_ue.h"
#include "PHY/NR_UE_TRANSPORT/srs_modulation_nr.h"
#include "SCHED_NR_UE/phy_sch_processing_time.h"
#include "PHY/NR_UE_ESTIMATION/nr_estimation.h"
#ifdef EMOS
#include "SCHED/phy_procedures_emos.h"
#endif
#include "executables/softmodem-common.h"
#include "executables/nr-uesoftmodem.h"
#include "SCHED_NR_UE/pucch_uci_ue_nr.h"
#include <openair1/PHY/TOOLS/phy_scope_interface.h>

//#define DEBUG_PHY_PROC
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

static const unsigned int gain_table[31] = {100,  112,  126,  141,  158,  178,  200,  224,  251, 282,  316,
                                            359,  398,  447,  501,  562,  631,  708,  794,  891, 1000, 1122,
                                            1258, 1412, 1585, 1778, 1995, 2239, 2512, 2818, 3162};

void nr_fill_dl_indication(nr_downlink_indication_t *dl_ind,
                           fapi_nr_dci_indication_t *dci_ind,
                           fapi_nr_rx_indication_t *rx_ind,
                           const UE_nr_rxtx_proc_t *proc,
                           PHY_VARS_NR_UE *ue,
                           void *phy_data)
{
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
                           const UE_nr_rxtx_proc_t *proc,
                           void *typeSpecific,
                           uint8_t *b)
{
  if (n_pdus > 1){
    LOG_E(PHY, "In %s: multiple number of DL PDUs not supported yet...\n", __FUNCTION__);
  }

  NR_DL_UE_HARQ_t *dl_harq0 = NULL;

  if ((pdu_type !=  FAPI_NR_RX_PDU_TYPE_SSB) && dlsch0) {
    int t=WS_C_RNTI;
    if (pdu_type == FAPI_NR_RX_PDU_TYPE_RAR)
      t=WS_RA_RNTI;
    if  (pdu_type == FAPI_NR_RX_PDU_TYPE_SIB)
      t=WS_SI_RNTI;
    dl_harq0 = &ue->dl_harq_processes[0][dlsch0->dlsch_config.harq_process_nbr];
    trace_NRpdu(DIRECTION_DOWNLINK,
		b,
		dlsch0->dlsch_config.TBS / 8,
		t,
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
        rx_ind->rx_indication_body[n_pdus - 1].pdsch_pdu.pdu = b;
        rx_ind->rx_indication_body[n_pdus - 1].pdsch_pdu.pdu_length = dlsch0->dlsch_config.TBS / 8;
      }
      if(dlsch1) {
        AssertFatal(1==0,"Second codeword currently not supported\n");
      }
      break;
    case FAPI_NR_RX_PDU_TYPE_SSB: {
        fapi_nr_ssb_pdu_t *ssb_pdu = &rx_ind->rx_indication_body[n_pdus - 1].ssb_pdu;
        if(typeSpecific) {
          NR_DL_FRAME_PARMS *frame_parms = &ue->frame_parms;
          fapiPbch_t *pbch = (fapiPbch_t *)typeSpecific;
          memcpy(ssb_pdu->pdu, pbch->decoded_output, sizeof(pbch->decoded_output));
          ssb_pdu->additional_bits = pbch->xtra_byte;
          ssb_pdu->ssb_index = (frame_parms->ssb_index) & 0x7;
          ssb_pdu->ssb_length = frame_parms->Lmax;
          ssb_pdu->cell_id = frame_parms->Nid_cell;
          ssb_pdu->ssb_start_subcarrier = frame_parms->ssb_start_subcarrier;
          ssb_pdu->rsrp_dBm = ue->measurements.ssb_rsrp_dBm[frame_parms->ssb_index];
          ssb_pdu->radiolink_monitoring = RLM_in_sync; // TODO to be removed from here
          ssb_pdu->decoded_pdu = true;
        }
        else {
          ssb_pdu->radiolink_monitoring = RLM_out_of_sync; // TODO to be removed from here
          ssb_pdu->decoded_pdu = false;
        }
      }
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

// UL time alignment procedures:
// - If the current tx frame and slot match the TA configuration
//   then timing advance is processed and set to be applied in the next UL transmission
// - Application of timing adjustment according to TS 38.213 p4.2
// todo:
// - handle RAR TA application as per ch 4.2 TS 38.213
void ue_ta_procedures(PHY_VARS_NR_UE *ue, int slot_tx, int frame_tx)
{
  if (frame_tx == ue->ta_frame && slot_tx == ue->ta_slot) {

    uint16_t ofdm_symbol_size = ue->frame_parms.ofdm_symbol_size;

    // convert time factor "16 * 64 * T_c / (2^mu)" in N_TA calculation in TS38.213 section 4.2 to samples by multiplying with samples per second
    //   16 * 64 * T_c            / (2^mu) * samples_per_second
    // = 16 * T_s                 / (2^mu) * samples_per_second
    // = 16 * 1 / (15 kHz * 2048) / (2^mu) * (15 kHz * 2^mu * ofdm_symbol_size)
    // = 16 * 1 /           2048           *                  ofdm_symbol_size
    // = 16 * ofdm_symbol_size / 2048
    uint16_t bw_scaling = 16 * ofdm_symbol_size / 2048;

    ue->timing_advance += (ue->ta_command - 31) * bw_scaling;

    LOG_D(PHY, "[UE %d] [%d.%d] Got timing advance command %u from MAC, new value is %d\n",
          ue->Mod_id,
          frame_tx,
          slot_tx,
          ue->ta_command,
          ue->timing_advance);

    ue->ta_frame = -1;
    ue->ta_slot = -1;
  }
}

void phy_procedures_nrUE_TX(PHY_VARS_NR_UE *ue, const UE_nr_rxtx_proc_t *proc, nr_phy_data_tx_t *phy_data)
{
  const int slot_tx = proc->nr_slot_tx;
  const int frame_tx = proc->frame_tx;
  const int gNB_id = proc->gNB_id;

  AssertFatal(ue->CC_id == 0, "Transmission on secondary CCs is not supported yet\n");

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_UE_TX,VCD_FUNCTION_IN);

  const int samplesF_per_slot = NR_SYMBOLS_PER_SLOT * ue->frame_parms.ofdm_symbol_size;
  c16_t txdataF_buf[ue->frame_parms.nb_antennas_tx * samplesF_per_slot] __attribute__((aligned(32)));
  memset(txdataF_buf, 0, sizeof(txdataF_buf));
  c16_t *txdataF[ue->frame_parms.nb_antennas_tx]; /* workaround to be compatible with current txdataF usage in all tx procedures. */
  for(int i=0; i< ue->frame_parms.nb_antennas_tx; ++i)
    txdataF[i] = &txdataF_buf[i * samplesF_per_slot];

  LOG_D(PHY,"****** start TX-Chain for AbsSubframe %d.%d ******\n", frame_tx, slot_tx);

  start_meas(&ue->phy_proc_tx);

  for (uint8_t harq_pid = 0; harq_pid < NR_MAX_ULSCH_HARQ_PROCESSES; harq_pid++) {
    if (ue->ul_harq_processes[harq_pid].status == ACTIVE) {
      nr_ue_ulsch_procedures(ue, harq_pid, frame_tx, slot_tx, gNB_id, phy_data, (c16_t **)&txdataF);
    }
  }

  ue_srs_procedures_nr(ue, proc, (c16_t **)&txdataF);

  pucch_procedures_ue_nr(ue, proc, phy_data, (c16_t **)&txdataF);

  LOG_D(PHY, "Sending Uplink data \n");
  nr_ue_pusch_common_procedures(ue, proc->nr_slot_tx, &ue->frame_parms, ue->frame_parms.nb_antennas_tx, (c16_t **)txdataF);

  nr_ue_prach_procedures(ue, proc);

  LOG_D(PHY, "****** end TX-Chain for AbsSubframe %d.%d ******\n", proc->frame_tx, proc->nr_slot_tx);

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_UE_TX, VCD_FUNCTION_OUT);
  stop_meas(&ue->phy_proc_tx);
}

void nr_ue_measurement_procedures(uint16_t l,
                                  PHY_VARS_NR_UE *ue,
                                  const UE_nr_rxtx_proc_t *proc,
                                  NR_UE_DLSCH_t *dlsch,
                                  uint32_t pdsch_est_size,
                                  int32_t dl_ch_estimates[][pdsch_est_size])
{
  NR_DL_FRAME_PARMS *frame_parms=&ue->frame_parms;
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
      T(T_UE_PHY_MEAS,
        T_INT(gNB_id),
        T_INT(ue->Mod_id),
        T_INT(proc->frame_rx % 1024),
        T_INT(nr_slot_rx),
        T_INT((int)(10 * log10(ue->measurements.rsrp[0]) - ue->rx_total_gain_dB)),
        T_INT((int)ue->measurements.rx_rssi_dBm[0]),
        T_INT((int)(ue->measurements.rx_power_avg_dB[0] - ue->measurements.n0_power_avg_dB)),
        T_INT((int)ue->measurements.rx_power_avg_dB[0]),
        T_INT((int)ue->measurements.n0_power_avg_dB),
        T_INT((int)ue->measurements.wideband_cqi_avg[0]));
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

static int nr_ue_pbch_procedures(PHY_VARS_NR_UE *ue,
                                 const UE_nr_rxtx_proc_t *proc,
                                 int estimateSz,
                                 struct complex16 dl_ch_estimates[][estimateSz],
                                 c16_t rxdataF[][ue->frame_parms.samples_per_slot_wCP])
{
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
                   &ue->frame_parms,
                   (ue->frame_parms.ssb_index)&7,
                   SISO,
                   &result,
                   rxdataF);

  if (ret==0) {

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
    LOG_E(PHY, "[UE %d] frame %d, nr_slot_rx %d, Error decoding PBCH!\n", ue->Mod_id, frame_rx, nr_slot_rx);
  }
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_PBCH_PROCEDURES, VCD_FUNCTION_OUT);
  return ret;
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

int nr_ue_pdcch_procedures(PHY_VARS_NR_UE *ue,
                           const UE_nr_rxtx_proc_t *proc,
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
  ue->if_inst->dl_indication(&dl_indication);

  stop_meas(&ue->dlsch_rx_pdcch_stats);

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_PDCCH_PROCEDURES, VCD_FUNCTION_OUT);
  return(dci_cnt);
}

static int nr_ue_pdsch_procedures(PHY_VARS_NR_UE *ue,
                                  const UE_nr_rxtx_proc_t *proc,
                                  NR_UE_DLSCH_t dlsch[2],
                                  int16_t *llr[2],
                                  c16_t rxdataF[][ue->frame_parms.samples_per_slot_wCP])
{
  int frame_rx = proc->frame_rx;
  int nr_slot_rx = proc->nr_slot_rx;
  int m;
  int first_symbol_flag=0;

  // We handle only one CW now
  if (!(NR_MAX_NB_LAYERS>4)) {
    NR_UE_DLSCH_t *dlsch0 = &dlsch[0];
    int harq_pid = dlsch0->dlsch_config.harq_process_nbr;
    NR_DL_UE_HARQ_t *dlsch0_harq = &ue->dl_harq_processes[0][harq_pid];
    uint16_t BWPStart       = dlsch0->dlsch_config.BWPStart;
    uint16_t pdsch_start_rb = dlsch0->dlsch_config.start_rb;
    uint16_t pdsch_nb_rb    = dlsch0->dlsch_config.number_rbs;
    uint16_t s0             = dlsch0->dlsch_config.start_symbol;
    uint16_t s1             = dlsch0->dlsch_config.number_symbols;

    AssertFatal(dlsch0->dlsch_config.resource_alloc == 1,
                "DLSCH resource allocation type0 not supported at PHY\n");

    LOG_D(PHY,"[UE %d] nr_slot_rx %d, harq_pid %d (%d), BWP start %d, rb_start %d, nb_rb %d, symbol_start %d, nb_symbols %d, DMRS mask %x, Nl %d\n",
          ue->Mod_id,nr_slot_rx,harq_pid,dlsch0_harq->status,BWPStart,pdsch_start_rb,pdsch_nb_rb,s0,s1,dlsch0->dlsch_config.dlDmrsSymbPos, dlsch0->Nl);

    const uint32_t pdsch_est_size = ((ue->frame_parms.symbols_per_slot * ue->frame_parms.ofdm_symbol_size + 15) / 16) * 16;
    __attribute__((aligned(32))) int32_t pdsch_dl_ch_estimates[ue->frame_parms.nb_antennas_rx * dlsch0->Nl][pdsch_est_size];
    memset(pdsch_dl_ch_estimates, 0, sizeof(int32_t) * ue->frame_parms.nb_antennas_rx * dlsch0->Nl * pdsch_est_size);

    c16_t ptrs_phase_per_slot[ue->frame_parms.nb_antennas_rx][NR_SYMBOLS_PER_SLOT];
    memset(ptrs_phase_per_slot, 0, sizeof(ptrs_phase_per_slot));

    int32_t ptrs_re_per_slot[ue->frame_parms.nb_antennas_rx][NR_SYMBOLS_PER_SLOT];
    memset(ptrs_re_per_slot, 0, sizeof(ptrs_re_per_slot));

    const uint32_t rx_size_symbol = dlsch[0].dlsch_config.number_rbs * NR_NB_SC_PER_RB;
    __attribute__((aligned(32))) int32_t rxdataF_comp[dlsch[0].Nl][ue->frame_parms.nb_antennas_rx][rx_size_symbol * NR_SYMBOLS_PER_SLOT];
    memset(rxdataF_comp, 0, sizeof(rxdataF_comp));

    for (m = s0; m < (s0 +s1); m++) {
      if (dlsch0->dlsch_config.dlDmrsSymbPos & (1 << m)) {
        for (uint8_t aatx=0; aatx<dlsch0->Nl; aatx++) {//for MIMO Config: it shall loop over no_layers
          LOG_D(PHY,"PDSCH Channel estimation gNB id %d, PDSCH antenna port %d, slot %d, symbol %d\n",0,aatx,nr_slot_rx,m);
          nr_pdsch_channel_estimation(ue,
                                      proc,
                                      get_dmrs_port(aatx,dlsch0->dlsch_config.dmrs_ports),
                                      m,
                                      dlsch0->dlsch_config.nscid,
                                      dlsch0->dlsch_config.dlDmrsScramblingId,
                                      BWPStart,
                                      dlsch0->dlsch_config.dmrsConfigType,
                                      dlsch0->dlsch_config.rb_offset,
                                      ue->frame_parms.first_carrier_offset+(BWPStart + pdsch_start_rb)*12,
                                      pdsch_nb_rb,
                                      pdsch_est_size,
                                      pdsch_dl_ch_estimates,
                                      ue->frame_parms.samples_per_slot_wCP, rxdataF);
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

    uint32_t dl_valid_re[NR_SYMBOLS_PER_SLOT] = {0};
    uint32_t llr_offset[NR_SYMBOLS_PER_SLOT] = {0};

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
                      dl_valid_re,
                      rxdataF,
                      llr_offset,
                      &log2_maxh,
                      rx_size_symbol,
                      ue->frame_parms.nb_antennas_rx,
                      rxdataF_comp,
                      ptrs_phase_per_slot,
                      ptrs_re_per_slot)
          < 0)
        return -1;

      stop_meas(&ue->dlsch_llr_stats_parallelization[slot]);
      if (cpumeas(CPUMEAS_GETSTATE))
        LOG_D(PHY,
              "[AbsSFN %d.%d] LLR Computation Symbol %d %5.2f \n",
              frame_rx,
              nr_slot_rx,
              m,
              ue->dlsch_llr_stats_parallelization[slot].p_time / (cpuf * 1000.0));
    } // CRNTI active
    stop_meas(&ue->rx_pdsch_stats);
  }
  return 0;
}

// This function release the Tx working thread for one pending information, like dlsch ACK/NACK
static void send_dl_done_to_tx_thread(notifiedFIFO_t *nf, int rx_slot)
{
  if (nf) {
    notifiedFIFO_elt_t *newElt = newNotifiedFIFO_elt(sizeof(int), 0, NULL, NULL);
    // We put rx slot only for tracing purpose
    int *msgData = (int *) NotifiedFifoData(newElt);
    *msgData = rx_slot;
    pushNotifiedFIFO(nf, newElt);
  }
}

static bool nr_ue_dlsch_procedures(PHY_VARS_NR_UE *ue, const UE_nr_rxtx_proc_t *proc, NR_UE_DLSCH_t dlsch[2], int16_t *llr[2])
{
  if (dlsch[0].active == false) {
    LOG_E(PHY, "DLSCH should be active when calling this function\n");
    return true;
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
  fapi_nr_rx_indication_t rx_ind = {0};
  uint16_t number_pdus = 1;

  uint8_t is_cw0_active = dl_harq0->status;
  uint8_t is_cw1_active = dl_harq1->status;
  uint16_t nb_symb_sch = dlsch[0].dlsch_config.number_symbols;
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
  if (is_cw0_active != ACTIVE && is_cw1_active != ACTIVE) {
    // don't wait anymore
    LOG_E(NR_PHY, "Internal error  nr_ue_dlsch_procedure() called but no active cw on slot %d, harq %d\n", nr_slot_rx, harq_pid);
    const int ack_nack_slot = (proc->nr_slot_rx + dlsch[0].dlsch_config.k1_feedback) % ue->frame_parms.slots_per_frame;
    send_dl_done_to_tx_thread(ue->tx_resume_ind_fifo + ack_nack_slot, proc->nr_slot_rx);
    return false;
  }

  start_meas(&ue->dlsch_unscrambling_stats);
  nr_dlsch_unscrambling(llr[0],
                        dl_harq0->G,
                        0,
                        ue->frame_parms.Nid_cell,
                        dlsch[0].rnti);
    

  stop_meas(&ue->dlsch_unscrambling_stats);

  start_meas(&ue->dlsch_decoding_stats);

  // create memory to store decoder output
  int a_segments = MAX_NUM_NR_DLSCH_SEGMENTS_PER_LAYER*NR_MAX_NB_LAYERS;  //number of segments to be allocated
  int num_rb = dlsch[0].dlsch_config.number_rbs;
  if (num_rb != 273) {
    a_segments = a_segments*num_rb;
    a_segments = (a_segments/273)+1;
  }
  uint32_t dlsch_bytes = a_segments*1056;  // allocated bytes per segment
  __attribute__ ((aligned(32))) uint8_t p_b[dlsch_bytes];

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
                          harq_pid,
                          dlsch_bytes,
                          p_b);

  LOG_T(PHY,"dlsch decoding, ret = %d\n", ret);


  if(ret<ue->max_ldpc_iterations+1)
    dec = true;

  int ind_type = -1;
  switch(dlsch[0].rnti_type) {
    case TYPE_RA_RNTI_:
      ind_type = FAPI_NR_RX_PDU_TYPE_RAR;
      break;

    case TYPE_SI_RNTI_:
      ind_type = FAPI_NR_RX_PDU_TYPE_SIB;
      break;

    case TYPE_C_RNTI_:
      ind_type = FAPI_NR_RX_PDU_TYPE_DLSCH;
      break;

    default:
      AssertFatal(true, "Invalid DLSCH type %d\n", dlsch[0].rnti_type);
      break;
  }

  nr_fill_dl_indication(&dl_indication, NULL, &rx_ind, proc, ue, NULL);
  nr_fill_rx_indication(&rx_ind, ind_type, ue, &dlsch[0], NULL, number_pdus, proc, NULL, p_b);

  LOG_D(PHY, "DL PDU length in bits: %d, in bytes: %d \n", dlsch[0].dlsch_config.TBS, dlsch[0].dlsch_config.TBS / 8);

  stop_meas(&ue->dlsch_decoding_stats);
  if (cpumeas(CPUMEAS_GETSTATE))  {
    LOG_D(PHY, " --> Unscrambling for CW0 %5.3f\n",
          (ue->dlsch_unscrambling_stats.p_time)/(cpuf*1000.0));
    LOG_D(PHY, "AbsSubframe %d.%d --> LDPC Decoding for CW0 %5.3f\n",
          frame_rx%1024, nr_slot_rx,(ue->dlsch_decoding_stats.p_time)/(cpuf*1000.0));
  }

  if(is_cw1_active) {
    // start ldpc decode for CW 1
    fapi_nr_dl_config_dlsch_pdu_rel15_t *dlsch_config = &dlsch[1].dlsch_config;
    uint32_t unav_res = 0;
    if(dlsch_config->pduBitmap & 0x1) {
      uint16_t ptrsSymbPos = 0;
      set_ptrs_symb_idx(&ptrsSymbPos,
                        dlsch_config->number_symbols,
                        dlsch_config->start_symbol,
                        1 << dlsch_config->PTRSTimeDensity,
                        dlsch_config->dlDmrsSymbPos);
      int n_ptrs = (dlsch_config->number_rbs + dlsch_config->PTRSFreqDensity - 1) / dlsch_config->PTRSFreqDensity;
      int ptrsSymbPerSlot = get_ptrs_symbols_in_slot(ptrsSymbPos, dlsch_config->start_symbol, dlsch_config->number_symbols);
      unav_res = n_ptrs * ptrsSymbPerSlot;
    }
    dl_harq1->G = nr_get_G(dlsch_config->number_rbs,
                           nb_symb_sch,
                           nb_re_dmrs,
                           dmrs_len,
                           unav_res,
                           dlsch_config->qamModOrder,
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
                             harq_pid,
                             dlsch_bytes,
                             p_b);
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
    ue->if_inst->dl_indication(&dl_indication);
  }

  // DLSCH decoding finished! don't wait anymore in Tx process, we know if we should answer ACK/NACK PUCCH
  if (dlsch[0].rnti_type == TYPE_C_RNTI_) {
    const int ack_nack_slot = (proc->nr_slot_rx + dlsch[0].dlsch_config.k1_feedback) % ue->frame_parms.slots_per_frame;
    send_dl_done_to_tx_thread(ue->tx_resume_ind_fifo + ack_nack_slot, proc->nr_slot_rx);
  }

  if (ue->phy_sim_dlsch_b)
    memcpy(ue->phy_sim_dlsch_b, p_b, dlsch_bytes);

  return dec;
}

void pbch_pdcch_processing(PHY_VARS_NR_UE *ue, const UE_nr_rxtx_proc_t *proc, nr_phy_data_t *phy_data)
{
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
            const int pbchSuccess = nr_ue_pbch_procedures(ue, proc, estimateSz, dl_ch_estimates, rxdataF);

            if (ue->no_timing_correction==0 && pbchSuccess == 0) {
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
            ue->apply_timing_offset = true;
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
          nr_prs_channel_estimation(gNB_id, rsc_id, i, ue, proc, fp, rxdataF);
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
    LOG_I(NR_PHY,"%s\n", output);

    LOG_I(NR_PHY,"============================================\n");
  }

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
  LOG_D(PHY,"[UE %d] Frame %d, nr_slot_rx %d: found %d DCIs\n", ue->Mod_id, frame_rx, nr_slot_rx, dci_cnt);
  phy_pdcch_config->nb_search_space = 0;
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_SLOT_FEP_PDCCH, VCD_FUNCTION_OUT);
}

void pdsch_processing(PHY_VARS_NR_UE *ue, const UE_nr_rxtx_proc_t *proc, nr_phy_data_t *phy_data)
{
  int frame_rx = proc->frame_rx;
  int nr_slot_rx = proc->nr_slot_rx;
  int gNB_id = proc->gNB_id;

  NR_UE_DLSCH_t *dlsch = &phy_data->dlsch[0];
  time_stats_t meas = {0};
  start_meas(&meas);
  // do procedures for C-RNTI

  const uint32_t rxdataF_sz = ue->frame_parms.samples_per_slot_wCP;
  __attribute__ ((aligned(32))) c16_t rxdataF[ue->frame_parms.nb_antennas_rx][rxdataF_sz];
  if (dlsch[0].active) {
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_SLOT_FEP_PDSCH, VCD_FUNCTION_IN);
    fapi_nr_dl_config_dlsch_pdu_rel15_t *dlsch_config = &dlsch[0].dlsch_config;
    uint16_t nb_symb_sch = dlsch_config->number_symbols;
    uint16_t start_symb_sch = dlsch_config->start_symbol;

    LOG_D(PHY," ------ --> PDSCH ChannelComp/LLR Frame.slot %d.%d ------  \n", frame_rx%1024, nr_slot_rx);
    //to update from pdsch config

    for (uint16_t m=start_symb_sch;m<(nb_symb_sch+start_symb_sch) ; m++){
      nr_slot_fep(ue,
                  proc,
                  m,  //to be updated from higher layer
                  rxdataF);
    }
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_SLOT_FEP_PDSCH, VCD_FUNCTION_OUT);

    uint8_t nb_re_dmrs;
    if (dlsch_config->dmrsConfigType == NFAPI_NR_DMRS_TYPE1) {
      nb_re_dmrs = 6 * dlsch_config->n_dmrs_cdm_groups;
    }
    else {
      nb_re_dmrs = 4 * dlsch_config->n_dmrs_cdm_groups;
    }
    uint16_t dmrs_len = get_num_dmrs(dlsch_config->dlDmrsSymbPos);
    uint32_t unav_res = 0;
    if(dlsch_config->pduBitmap & 0x1) {
      uint16_t ptrsSymbPos = 0;
      set_ptrs_symb_idx(&ptrsSymbPos,
                        dlsch_config->number_symbols,
                        dlsch_config->start_symbol,
                        1 << dlsch_config->PTRSTimeDensity,
                        dlsch_config->dlDmrsSymbPos);
      int n_ptrs = (dlsch_config->number_rbs + dlsch_config->PTRSFreqDensity - 1) / dlsch_config->PTRSFreqDensity;
      int ptrsSymbPerSlot = get_ptrs_symbols_in_slot(ptrsSymbPos, dlsch_config->start_symbol, dlsch_config->number_symbols);
      unav_res = n_ptrs * ptrsSymbPerSlot;
    }
    NR_DL_UE_HARQ_t *dlsch0_harq = &ue->dl_harq_processes[0][dlsch_config->harq_process_nbr];
    dlsch0_harq->G = nr_get_G(dlsch_config->number_rbs,
                              dlsch_config->number_symbols,
                              nb_re_dmrs,
                              dmrs_len,
                              unav_res,
                              dlsch_config->qamModOrder,
                              dlsch[0].Nl);
    const uint32_t rx_llr_buf_sz = ((dlsch0_harq->G + 15) / 16) * 16;
    const uint32_t nb_codewords = NR_MAX_NB_LAYERS > 4 ? 2 : 1;
    int16_t* llr[2];
    for (int i = 0; i < nb_codewords; i++)
      llr[i] = (int16_t *)malloc16_clear(rx_llr_buf_sz * sizeof(int16_t));

    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PDSCH_PROC_C, VCD_FUNCTION_IN);
    // it returns -1 in case of internal failure, or 0 in case of normal result
    int ret_pdsch = nr_ue_pdsch_procedures(ue, proc, dlsch, llr, rxdataF);

    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PDSCH_PROC_C, VCD_FUNCTION_OUT);

    UEscopeCopy(ue, pdschLlr, llr[0], sizeof(int16_t), 1, dlsch0_harq->G, 0);

    LOG_D(PHY, "DLSCH data reception at nr_slot_rx: %d\n", nr_slot_rx);
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PDSCH_PROC, VCD_FUNCTION_IN);

    start_meas(&ue->dlsch_procedures_stat);

    if (ret_pdsch >= 0)
      nr_ue_dlsch_procedures(ue, proc, dlsch, llr);
    else {
      LOG_E(NR_PHY, "Demodulation impossible, internal error\n");
      send_dl_done_to_tx_thread(
          ue->tx_resume_ind_fifo + (proc->nr_slot_rx + dlsch_config->k1_feedback) % ue->frame_parms.slots_per_frame,
          proc->nr_slot_rx);
      LOG_W(NR_PHY, "nr_ue_pdsch_procedures failed in slot %d\n", proc->nr_slot_rx);
    }

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

  start_meas(&meas);

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

  stop_meas(&meas);
  if (cpumeas(CPUMEAS_GETSTATE))
    LOG_D(PHY, "after ldpc decode until end of Rx %5.2f \n", meas.p_time / (cpuf * 1000.0));

#ifdef EMOS
  phy_procedures_emos_UE_RX(ue,slot,gNB_id);
#endif


  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_UE_RX, VCD_FUNCTION_OUT);

  stop_meas(&ue->phy_proc_rx);
  if (cpumeas(CPUMEAS_GETSTATE))
    LOG_D(PHY, "------FULL RX PROC [SFN %d]: %5.2f ------\n",nr_slot_rx,ue->phy_proc_rx.p_time/(cpuf*1000.0));

  LOG_D(PHY," ****** end RX-Chain  for AbsSubframe %d.%d ******  \n", frame_rx%1024, nr_slot_rx);
  UEscopeCopy(ue, commonRxdataF, rxdataF, sizeof(int32_t), ue->frame_parms.nb_antennas_rx, rxdataF_sz, 0);
}


// todo:
// - power control as per 38.213 ch 7.4
void nr_ue_prach_procedures(PHY_VARS_NR_UE *ue, const UE_nr_rxtx_proc_t *proc)
{
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
