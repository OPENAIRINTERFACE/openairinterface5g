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

/*! \file PHY/defs_eNB.h
 \brief Top-level defines and structure definitions for eNB
 \author R. Knopp, F. Kaltenberger
 \date 2011
 \version 0.1
 \company Eurecom
 \email: knopp@eurecom.fr,florian.kaltenberger@eurecom.fr
 \note
 \warning
*/
#ifndef __PHY_DEFS_ENB__H__
#define __PHY_DEFS_ENB__H__


#ifndef _GNU_SOURCE
  #define _GNU_SOURCE
#endif

#include <execinfo.h>
#include <getopt.h>
#include <linux/sched.h>
#include <malloc.h>
#include <math.h>
#include <pthread.h>
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/sysinfo.h>
#include <sys/types.h>
#include <unistd.h>

#include "common_lib.h"
#include "defs_common.h"
#include "defs_RU.h"
#include "impl_defs_top.h"
#include "time_meas.h"
//#include "PHY/CODING/coding_defs.h"
#include "PHY/TOOLS/tools_defs.h"
#include "common/platform_types.h"
#include "PHY/LTE_TRANSPORT/transport_common.h"
#include "PHY/LTE_TRANSPORT/transport_eNB.h"
#include "openair2/PHY_INTERFACE/IF_Module.h"
#include "common/openairinterface5g_limits.h"


#define PBCH_A 24
#define MAX_NUM_RU_PER_eNB 64
#define MAX_NUM_RX_PRACH_PREAMBLES 4

typedef struct {
  /// \brief Pointers (dynamic) to the received data in the time domain.
  /// - first index: rx antenna [0..nb_antennas_rx[
  /// - second index: ? [0..2*ofdm_symbol_size*frame_parms->symbols_per_tti[
  int32_t **rxdata;
  /// \brief Pointers (dynamic) to the received data in the frequency domain.
  /// - first index: rx antenna [0..nb_antennas_rx[
  /// - second index: ? [0..2*ofdm_symbol_size*frame_parms->symbols_per_tti[
  int32_t **rxdataF;
  /// \brief holds the transmit data in the frequency domain.
  /// - first index: tx antenna [0..14[ where 14 is the total supported antenna ports.
  /// - second index: sample [0..]
  int32_t **txdataF;
} LTE_eNB_COMMON;

typedef struct {
  uint8_t     num_dci;
  uint8_t     num_pdcch_symbols;
  DCI_ALLOC_t dci_alloc[32];
} LTE_eNB_PDCCH;

typedef struct {
  uint8_t hi;
  uint8_t first_rb;
  uint8_t n_DMRS;
} phich_config_t;

typedef struct {
  uint8_t num_hi;
  phich_config_t config[32];
} LTE_eNB_PHICH;

typedef struct {
  uint8_t     num_dci;
  eDCI_ALLOC_t edci_alloc[32];
} LTE_eNB_EPDCCH;

typedef struct {
  /// number of active MPDCCH allocations
  uint8_t     num_dci;
  /// MPDCCH DCI allocations from MAC
  mDCI_ALLOC_t mdci_alloc[32];
  // MAX SIZE of an EPDCCH set is 16EREGs * 9REs/EREG * 8 PRB pairs = 2304 bits
  uint8_t e[2304];
} LTE_eNB_MPDCCH;


typedef struct {
  /// \brief Hold the channel estimates in frequency domain based on SRS.
  /// - first index: rx antenna id [0..nb_antennas_rx[
  /// - second index: ? [0..ofdm_symbol_size[
  int32_t **srs_ch_estimates;
  /// \brief Hold the channel estimates in time domain based on SRS.
  /// - first index: rx antenna id [0..nb_antennas_rx[
  /// - second index: ? [0..2*ofdm_symbol_size[
  int32_t **srs_ch_estimates_time;
  /// \brief Holds the SRS for channel estimation at the RX.
  /// - first index: rx antenna id [0..nb_antennas_rx[
  /// - second index: ? [0..ofdm_symbol_size[
  int32_t *srs;
} LTE_eNB_SRS;

typedef struct {
  /// \brief Holds the received data in the frequency domain for the allocated RBs in repeated format.
  /// - first index: rx antenna id [0..nb_antennas_rx[
  /// - second index: ? [0..2*ofdm_symbol_size[
  int32_t **rxdataF_ext;
  /// \brief Holds the received data in the frequency domain for the allocated RBs in normal format.
  /// - first index: rx antenna id [0..nb_antennas_rx[
  /// - second index (definition from phy_init_lte_eNB()): ? [0..12*N_RB_UL*frame_parms->symbols_per_tti[
  int32_t **rxdataF_ext2;
  /// \brief Hold the channel estimates in time domain based on DRS.
  /// - first index: rx antenna id [0..nb_antennas_rx[
  /// - second index: ? [0..4*ofdm_symbol_size[
  int32_t **drs_ch_estimates_time;
  /// \brief Hold the channel estimates in frequency domain based on DRS.
  /// - first index: rx antenna id [0..nb_antennas_rx[
  /// - second index: ? [0..12*N_RB_UL*frame_parms->symbols_per_tti[
  int32_t **drs_ch_estimates;
  /// \brief Holds the compensated signal.
  /// - first index: rx antenna id [0..nb_antennas_rx[
  /// - second index: ? [0..12*N_RB_UL*frame_parms->symbols_per_tti[
  int32_t **rxdataF_comp;
  /// \brief Magnitude of the UL channel estimates. Used for 2nd-bit level thresholds in LLR computation
  /// - first index: rx antenna id [0..nb_antennas_rx[
  /// - second index: ? [0..12*N_RB_UL*frame_parms->symbols_per_tti[
  int32_t **ul_ch_mag;
  /// \brief Magnitude of the UL channel estimates scaled for 3rd bit level thresholds in LLR computation
  /// - first index: rx antenna id [0..nb_antennas_rx[
  /// - second index: ? [0..12*N_RB_UL*frame_parms->symbols_per_tti[
  int32_t **ul_ch_magb;
  /// measured RX power based on DMRS
  int ulsch_power[4];
  /// measured RX power of noise
  int ulsch_noise_power[4];
  /// \brief llr values.
  /// - first index: ? [0..1179743] (hard coded)
  int16_t *llr;
} LTE_eNB_PUSCH;


typedef struct {
  uint8_t pbch_d[96+(3*(16+PBCH_A))];
  uint8_t pbch_w[3*3*(16+PBCH_A)];
  uint8_t pbch_e[1920];
} LTE_eNB_PBCH;


typedef struct {
  /// \brief ?.
  /// first index: ce_level [0..3]
  /// second index: rx antenna [0..63] (hard coded) \note Hard coded array size indexed by \c nb_antennas_rx.
  /// third index: frequency-domain sample [0..ofdm_symbol_size*12[
  int16_t **rxsigF[4];
  /// \brief local buffer to compute prach_ifft (necessary in case of multiple CCs)
  /// first index: ce_level [0..3] (hard coded) \note Hard coded array size indexed by \c nb_antennas_rx.
  /// second index: ? [0..63] (hard coded)
  /// third index: ? [0..63] (hard coded)
  int32_t **prach_ifft[4];

  /// repetition number
  /// indicator of first frame in a group of PRACH repetitions
  int first_frame[4];
  /// current repetition for each CE level
  int repetition_number[4];
} LTE_eNB_PRACH;

#include "time_meas.h"
#include "PHY/CODING/coding_defs.h"
#include "PHY/TOOLS/tools_defs.h"
#include "PHY/LTE_TRANSPORT/transport_eNB.h"


typedef struct {
  struct PHY_VARS_eNB_s *eNB;
  int UE_id;
  int harq_pid;
  int llr8_flag;
  int ret;
} td_params;

/// Context data structure for RX/TX portion of subframe processing
typedef struct {
  /// Component Carrier index
  uint8_t              CC_id;
  /// timestamp transmitted to HW
  openair0_timestamp timestamp_tx;
  openair0_timestamp timestamp_rx;
  /// subframe to act upon for transmission
  int subframe_tx;
  /// subframe to act upon for reception
  int subframe_rx;
  /// frame to act upon for transmission
  int frame_tx;
  /// frame to act upon for reception
  int frame_rx;
  int frame_prach;
  int subframe_prach;
  int frame_prach_br;
  int subframe_prach_br;
  /// \brief Instance count for RXn-TXnp4 processing thread.
  /// \internal This variable is protected by \ref mutex_rxtx.
  int instance_cnt;
  /// pthread structure for RXn-TXnp4 processing thread
  pthread_t pthread;
  /// pthread attributes for RXn-TXnp4 processing thread
  pthread_attr_t attr;
  /// condition variable for tx processing thread
  pthread_cond_t cond;
  /// mutex for RXn-TXnp4 processing thread
  pthread_mutex_t mutex;
  /// scheduling parameters for RXn-TXnp4 thread
  struct sched_param sched_param_rxtx;

  /// \internal This variable is protected by \ref mutex_RUs.
  int instance_cnt_RUs;
  /// condition variable for tx processing thread
  pthread_cond_t cond_RUs;
  /// mutex for RXn-TXnp4 processing thread
  pthread_mutex_t mutex_RUs;
  tpool_t *threadPool;
  int nbDecode;
  notifiedFIFO_t *respDecode;
  pthread_mutex_t mutex_emulateRF;
  int instance_cnt_emulateRF;
  pthread_t pthread_emulateRF;
  pthread_attr_t attr_emulateRF;
  pthread_cond_t cond_emulateRF;
  int first_rx;
} L1_rxtx_proc_t;

typedef struct {
  struct PHY_VARS_eNB_s *eNB;
  LTE_eNB_DLSCH_t *dlsch;
  int G;
  int harq_pid;
  int total_worker;
  int current_worker;
  /// \internal This variable is protected by \ref mutex_te.
  int instance_cnt_te;
  /// pthread attributes for parallel turbo-encoder thread
  pthread_attr_t attr_te;
  /// scheduling parameters for parallel turbo-encoder thread
  struct sched_param sched_param_te;
  /// pthread structure for parallel turbo-encoder thread
  pthread_t pthread_te;
  /// condition variable for parallel turbo-encoder thread
  pthread_cond_t cond_te;
  /// mutex for parallel turbo-encoder thread
  pthread_mutex_t mutex_te;
} te_params;

/// Context data structure for eNB subframe processing
typedef struct L1_proc_t_s {
  /// Component Carrier index
  uint8_t              CC_id;
  /// thread index
  int thread_index;
  /// timestamp received from HW
  openair0_timestamp timestamp_rx;
  /// timestamp to send to "slave rru"
  openair0_timestamp timestamp_tx;
  /// subframe to act upon for reception
  int subframe_rx;
  /// subframe to act upon for PRACH
  int subframe_prach;
  /// subframe to act upon for reception of prach BL/CE UEs
  int subframe_prach_br;
  /// frame to act upon for reception
  int frame_rx;
  /// frame to act upon for transmission
  int frame_tx;
  /// frame to act upon for PRACH
  int frame_prach;
  /// frame to act upon for PRACH BL/CE UEs
  int frame_prach_br;
  /// \internal This variable is protected by \ref mutex_td.
  int instance_cnt_td;
  /// \internal This variable is protected by \ref mutex_te.
  int instance_cnt_te;
  /// \internal This variable is protected by \ref mutex_prach.
  int instance_cnt_prach;
  /// \internal This variable is protected by \ref mutex_prach for BL/CE UEs.
  int instance_cnt_prach_br;
  // instance count for over-the-air eNB synchronization
  int instance_cnt_synch;


  /// \internal This variable is protected by \ref mutex_asynch_rxtx.
  int instance_cnt_asynch_rxtx;
  /// pthread structure for asychronous RX/TX processing thread
  pthread_t pthread_asynch_rxtx;
  /// flag to indicate first RX acquisition
  int first_rx;
  /// flag to indicate first TX transmission
  int first_tx;
  /// pthread attributes for parallel turbo-decoder thread
  pthread_attr_t attr_td;
  /// pthread attributes for parallel turbo-encoder thread
  pthread_attr_t attr_te;
  /// pthread attributes for single eNB processing thread
  pthread_attr_t attr_single;
  /// pthread attributes for prach processing thread
  pthread_attr_t attr_prach;
  /// pthread attributes for prach processing thread BL/CE UEs
  pthread_attr_t attr_prach_br;
  /// pthread attributes for asynchronous RX thread
  pthread_attr_t attr_asynch_rxtx;
  /// scheduling parameters for parallel turbo-decoder thread
  struct sched_param sched_param_td;
  /// scheduling parameters for parallel turbo-encoder thread
  struct sched_param sched_param_te;
  /// scheduling parameters for single eNB thread
  struct sched_param sched_param_single;
  /// scheduling parameters for prach thread
  struct sched_param sched_param_prach;
  /// scheduling parameters for prach thread
  struct sched_param sched_param_prach_br;
  /// scheduling parameters for asynch_rxtx thread
  struct sched_param sched_param_asynch_rxtx;
  /// pthread structure for parallel turbo-decoder thread
  pthread_t pthread_td;
  /// pthread structure for parallel turbo-encoder thread
  pthread_t pthread_te;
  /// pthread structure for PRACH thread
  pthread_t pthread_prach;
  /// pthread structure for PRACH thread BL/CE UEs
  pthread_t pthread_prach_br;
  /// condition variable for parallel turbo-decoder thread
  pthread_cond_t cond_td;
  /// condition variable for parallel turbo-encoder thread
  pthread_cond_t cond_te;
  /// condition variable for PRACH processing thread;
  pthread_cond_t cond_prach;
  /// condition variable for PRACH processing thread BL/CE UEs;
  pthread_cond_t cond_prach_br;
  /// condition variable for asynch RX/TX thread
  pthread_cond_t cond_asynch_rxtx;
  /// mutex for parallel turbo-decoder thread
  pthread_mutex_t mutex_td;
  /// mutex for parallel turbo-encoder thread
  pthread_mutex_t mutex_te;
  /// mutex for PRACH thread
  pthread_mutex_t mutex_prach;
  /// mutex for PRACH thread for BL/CE UEs
  pthread_mutex_t mutex_prach_br;
  /// mutex for asynch RX/TX thread
  pthread_mutex_t mutex_asynch_rxtx;
  /// mutex for RU access to eNB processing (PDSCH/PUSCH)
  pthread_mutex_t mutex_RU;
  /// mutex for eNB processing to access RU TX (PDSCH/PUSCH)
  pthread_mutex_t mutex_RU_tx;
  /// mutex for RU access to eNB processing (PRACH)
  pthread_mutex_t mutex_RU_PRACH;
  /// mutex for RU access to eNB processing (PRACH BR)
  pthread_mutex_t mutex_RU_PRACH_br;
  /// mask for RUs serving eNB (PDSCH/PUSCH)
  int RU_mask[10];
  /// mask for RUs serving eNB (PDSCH/PUSCH)
  int RU_mask_tx;
  /// time measurements for RU arrivals
  struct timespec t[10];
  /// Timing statistics (RU_arrivals)
  time_stats_t ru_arrival_time;
  /// mask for RUs serving eNB (PRACH)
  int RU_mask_prach;
  /// mask for RUs serving eNB (PRACH)
  int RU_mask_prach_br;
  /// parameters for turbo-decoding worker thread
  td_params tdp;
  /// parameters for turbo-encoding worker thread
  te_params tep[3];
  /// set of scheduling variables RXn-TXnp4 threads
  L1_rxtx_proc_t L1_proc,L1_proc_tx;
  /// stats thread pthread descriptor
  pthread_t process_stats_thread;
  /// L1 stats pthread descriptor
  pthread_t L1_stats_thread;
  /// for waking up tx procedure
  RU_proc_t *ru_proc;
  struct PHY_VARS_eNB_s *eNB;
} L1_proc_t;


typedef struct {
  //unsigned int   rx_power[NUMBER_OF_CONNECTED_eNB_MAX][NB_ANTENNAS_RX];     //! estimated received signal power (linear)
  //unsigned short rx_power_dB[NUMBER_OF_CONNECTED_eNB_MAX][NB_ANTENNAS_RX];  //! estimated received signal power (dB)
  //unsigned short rx_avg_power_dB[NUMBER_OF_CONNECTED_eNB_MAX];              //! estimated avg received signal power (dB)

  // common measurements
  //! estimated noise power (linear)
  unsigned int   n0_power[MAX_NUM_RU_PER_eNB];
  //! estimated noise power (dB)
  unsigned short n0_power_dB[MAX_NUM_RU_PER_eNB];
  //! total estimated noise power (linear)
  unsigned int   n0_power_tot;
  //! estimated avg noise power (dB)
  unsigned short n0_power_tot_dB;
  //! estimated avg noise power (dB)
  short n0_power_tot_dBm;
  //! estimated avg noise power per RB per RX ant (lin)
  unsigned short n0_subband_power[MAX_NUM_RU_PER_eNB][100];
  //! estimated avg noise power per RB per RX ant (dB)
  unsigned short n0_subband_power_dB[MAX_NUM_RU_PER_eNB][100];
  //! estimated avg noise power per RB (dB)
  short n0_subband_power_tot_dB[100];
  //! estimated avg noise power per RB (dBm)
  short n0_subband_power_tot_dBm[100];
  //! etimated avg noise power over all RB (dB)
  short n0_subband_power_avg_dB;
  // eNB measurements (per user)
  //! estimated received spatial signal power (linear)
  unsigned int   rx_spatial_power[NUMBER_OF_SRS_MAX][2][2];
  //! estimated received spatial signal power (dB)
  unsigned short rx_spatial_power_dB[NUMBER_OF_SRS_MAX][2][2];
  //! estimated rssi (dBm)
  short          rx_rssi_dBm[NUMBER_OF_SRS_MAX];
  //! estimated correlation (wideband linear) between spatial channels (computed in dlsch_demodulation)
  int            rx_correlation[NUMBER_OF_SRS_MAX][2];
  //! estimated correlation (wideband dB) between spatial channels (computed in dlsch_demodulation)
  int            rx_correlation_dB[NUMBER_OF_SRS_MAX][2];

  /// Wideband CQI (= SINR)
  int            wideband_cqi[NUMBER_OF_SRS_MAX][MAX_NUM_RU_PER_eNB];
  /// Wideband CQI in dB (= SINR dB)
  int            wideband_cqi_dB[NUMBER_OF_SRS_MAX][MAX_NUM_RU_PER_eNB];
  /// Wideband CQI (sum of all RX antennas, in dB)
  char           wideband_cqi_tot[NUMBER_OF_SRS_MAX];
  /// Subband CQI per RX antenna and RB (= SINR)
  int            subband_cqi[NUMBER_OF_SRS_MAX][MAX_NUM_RU_PER_eNB][100];
  /// Total Subband CQI and RB (= SINR)
  int            subband_cqi_tot[NUMBER_OF_UE_MAX][100];
  /// Subband CQI in dB and RB (= SINR dB)
  int            subband_cqi_dB[NUMBER_OF_SRS_MAX][MAX_NUM_RU_PER_eNB][100];
  /// Total Subband CQI and RB
  int            subband_cqi_tot_dB[NUMBER_OF_SRS_MAX][100];
  /// PRACH background noise level
  int            prach_I0;
  /// PUCCH background noise level
  int            n0_pucch_dB;
} PHY_MEASUREMENTS_eNB;

typedef struct {
  uint16_t rnti;
  int frame;
  int round_trials[8];
  int total_bytes_tx;
  int total_bytes_rx;
  int current_G;
  int current_TBS;
  int current_Qm;
  int current_mcs;
  int current_RI;
  int timing_offset;
  int ulsch_power[4];
  int ulsch_noise_power[4];
} eNB_SCH_STATS_t;

typedef struct {
  uint16_t rnti;
  int frame;
  int pucch1_trials;
  int pucch1_thres;
  int current_pucch1_stat_pos;
  int current_pucch1_stat_neg;
  int pucch1_positive_SR;
  int pucch1_low_stat[4];
  int pucch1_high_stat[4];
  int pucch1_phase;
  int pucch1a_trials;
  int current_pucch1a_stat_re;
  int current_pucch1a_stat_im;
  int pucch1ab_DTX;
  int pucch1b_trials;
  int current_pucch1b_stat_re;
  int current_pucch1b_stat_im;
  int pucch3_trials;
  int current_pucch3_stat;
} eNB_UCI_STATS_t;

/// Top-level PHY Data Structure for eNB
typedef struct PHY_VARS_eNB_s {
  /// Module ID indicator for this instance
  module_id_t          Mod_id;
  uint8_t              CC_id;
  uint8_t              configured;
  L1_proc_t            proc;
  int                  single_thread_flag;
  int                  abstraction_flag;
  int                  num_RU;
  RU_t                 *RU_list[MAX_NUM_RU_PER_eNB];
  /// Ethernet parameters for northbound midhaul interface
  eth_params_t         eth_params_n;
  /// Ethernet parameters for fronthaul interface
  eth_params_t         eth_params;
  int                  rx_total_gain_dB;
  int                  (*start_if)(struct RU_t_s *ru,struct PHY_VARS_eNB_s *eNB);
  uint8_t              local_flag;
  LTE_DL_FRAME_PARMS   frame_parms;
  PHY_MEASUREMENTS_eNB measurements;
  IF_Module_t          *if_inst;
  UL_IND_t             UL_INFO;
  pthread_mutex_t      UL_INFO_mutex;
  /// NFAPI RX ULSCH information
  nfapi_rx_indication_pdu_t  rx_pdu_list[NFAPI_RX_IND_MAX_PDU];
  /// NFAPI RX ULSCH CRC information
  nfapi_crc_indication_pdu_t crc_pdu_list[NFAPI_CRC_IND_MAX_PDU];
  /// NFAPI HARQ information
  nfapi_harq_indication_pdu_t harq_pdu_list[NFAPI_HARQ_IND_MAX_PDU];
  /// NFAPI SR information
  nfapi_sr_indication_pdu_t sr_pdu_list[NFAPI_SR_IND_MAX_PDU];
  /// NFAPI CQI information
  nfapi_cqi_indication_pdu_t cqi_pdu_list[NFAPI_CQI_IND_MAX_PDU];
  /// NFAPI CQI information (raw component)
  nfapi_cqi_indication_raw_pdu_t cqi_raw_pdu_list[NFAPI_CQI_IND_MAX_PDU];
  /// NFAPI PRACH information
  nfapi_preamble_pdu_t preamble_list[MAX_NUM_RX_PRACH_PREAMBLES];
  /// NFAPI PRACH information BL/CE UEs
  nfapi_preamble_pdu_t preamble_list_br[MAX_NUM_RX_PRACH_PREAMBLES];
  Sched_Rsp_t          Sched_INFO;
  LTE_eNB_PDCCH        pdcch_vars[2];
  LTE_eNB_PHICH        phich_vars[2];
  LTE_eNB_EPDCCH       epdcch_vars[2];
  LTE_eNB_MPDCCH       mpdcch_vars[2];
  LTE_eNB_PRACH        prach_vars_br;
  LTE_eNB_COMMON       common_vars;
  LTE_eNB_UCI          uci_vars[NUMBER_OF_UCI_MAX];
  LTE_eNB_SRS          srs_vars[NUMBER_OF_SRS_MAX];
  LTE_eNB_PBCH         pbch;
  LTE_eNB_PUSCH       *pusch_vars[NUMBER_OF_ULSCH_MAX];
  LTE_eNB_PRACH        prach_vars;
  LTE_eNB_DLSCH_t     *dlsch[NUMBER_OF_DLSCH_MAX][2];   // Num active DLSCH contexts times two spatial streams
  LTE_eNB_ULSCH_t     *ulsch[NUMBER_OF_ULSCH_MAX];      // Num active ULSCH contexts
  LTE_eNB_DLSCH_t     *dlsch_SI,*dlsch_ra,*dlsch_p;
  LTE_eNB_DLSCH_t     *dlsch_MCH;
  LTE_eNB_DLSCH_t     *dlsch_PCH;
  LTE_eNB_UE_stats     UE_stats[NUMBER_OF_UE_MAX];
  LTE_eNB_UE_stats    *UE_stats_ptr[NUMBER_OF_UE_MAX];

  /// cell-specific reference symbols
  uint32_t         lte_gold_table[20][2][14];

  /// UE-specific reference symbols (p=5), TM 7
  uint32_t         lte_gold_uespec_port5_table[NUMBER_OF_DLSCH_MAX][20][38];

  /// UE-specific reference symbols (p=7...14), TM 8/9/10
  uint32_t         lte_gold_uespec_table[2][20][2][21];

  /// mbsfn reference symbols
  uint32_t         lte_gold_mbsfn_table[10][3][42];
  /// mbsfn reference symbols
  uint32_t         lte_gold_mbsfn_khz_1dot25_table[10][150];

  // PRACH energy detection parameters
  /// Detection threshold for LTE PRACH
  int              prach_DTX_threshold;
  /// Detection threshold for LTE-M PRACH per CE-level
  int              prach_DTX_threshold_emtc[4];
  /// counter to average prach energh over first 100 prach opportunities
  int              prach_energy_counter;
  // PUCCH1 energy detection parameters
  int              pucch1_DTX_threshold;
  // PUCCH1 energy detection parameters for eMTC per CE-level
  int              pucch1_DTX_threshold_emtc[4];
  // PUCCH1a/b energy detection parameters
  int              pucch1ab_DTX_threshold;
  // PUCCH1a/b energy detection parameters for eMTC per CE-level
  int              pucch1ab_DTX_threshold_emtc[4];

  uint32_t X_u[64][839];
  uint32_t X_u_br[4][64][839];
  uint8_t pbch_configured;
  uint8_t pbch_pdu[4]; //PBCH_PDU_SIZE
  char eNB_generate_rar;

  /// indicator that eNB signal generation uses DTX (i.e. signal is cleared in each subframe
  int use_DTX;
  int32_t **subframe_mask;
  /// Indicator set to 0 after first SR
  uint8_t first_sr[NUMBER_OF_UE_MAX];

  uint32_t max_peak_val;
  int max_eNB_id, max_sync_pos;

  /// \brief sinr for all subcarriers of the current link (used only for abstraction).
  /// first index: ? [0..N_RB_DL*12[
  double *sinr_dB;

  /// N0 (used for abstraction)
  double N0;

  unsigned char first_run_timing_advance[NUMBER_OF_UE_MAX];
  unsigned char first_run_I0_measurements;


  unsigned char    is_secondary_eNB; // primary by default
  unsigned char    is_init_sync;     /// Flag to tell if initial synchronization is performed. This affects how often the secondary eNB will listen to the PSS from the primary system.
  unsigned char    has_valid_precoder; /// Flag to tell if secondary eNB has channel estimates to create NULL-beams from, and this B/F vector is created.
  unsigned char    PeNB_id;          /// id of Primary eNB

  /// hold the precoder for NULL beam to the primary user
  int              **dl_precoder_SeNB[3];
  char             log2_maxp; /// holds the maximum channel/precoder coefficient

  /// if ==0 enables phy only test mode
  int mac_enabled;


  // PDSCH Varaibles
  PDSCH_CONFIG_DEDICATED pdsch_config_dedicated[NUMBER_OF_UE_MAX];

  // PUSCH Varaibles
  PUSCH_CONFIG_DEDICATED pusch_config_dedicated[NUMBER_OF_UE_MAX];

  // PUCCH variables
  PUCCH_CONFIG_DEDICATED pucch_config_dedicated[NUMBER_OF_UE_MAX];

  // UL-POWER-Control
  UL_POWER_CONTROL_DEDICATED ul_power_control_dedicated[NUMBER_OF_UE_MAX];

  // TPC
  TPC_PDCCH_CONFIG tpc_pdcch_config_pucch[NUMBER_OF_UE_MAX];
  TPC_PDCCH_CONFIG tpc_pdcch_config_pusch[NUMBER_OF_UE_MAX];

  // CQI reporting
  CQI_REPORT_CONFIG cqi_report_config[NUMBER_OF_UE_MAX];

  // SRS Variables
  SOUNDINGRS_UL_CONFIG_DEDICATED soundingrs_ul_config_dedicated[NUMBER_OF_UE_MAX];
  uint8_t ncs_cell[20][7];

  // Scheduling Request Config
  SCHEDULING_REQUEST_CONFIG scheduling_request_config[NUMBER_OF_UE_MAX];

  // Transmission mode per UE
  uint8_t transmission_mode[NUMBER_OF_UE_MAX];

  /// cba_last successful reception for each group, used for collision detection
  uint8_t cba_last_reception[4];

  // Pointers for active physicalConfigDedicated to be applied in current subframe
  struct LTE_PhysicalConfigDedicated *physicalConfigDedicated[NUMBER_OF_UE_MAX];


  uint32_t rb_mask_ul[4];

  /// Information regarding TM5
  MU_MIMO_mode mu_mimo_mode[NUMBER_OF_UE_MAX];

  /// statistics for DLSCH measurement collection
  eNB_SCH_STATS_t dlsch_stats[NUMBER_OF_SCH_STATS_MAX];
  /// statistics for ULSCH measurement collection
  eNB_SCH_STATS_t ulsch_stats[NUMBER_OF_SCH_STATS_MAX];
  /// statis for UCI (PUCCH) measurement collection
  eNB_UCI_STATS_t uci_stats[NUMBER_OF_SCH_STATS_MAX];
  /// target_ue_dl_mcs : only for debug purposes
  uint32_t target_ue_dl_mcs;
  /// target_ue_ul_mcs : only for debug purposes
  uint32_t target_ue_ul_mcs;
  /// target_ue_dl_rballoc : only for debug purposes
  uint32_t ue_dl_rb_alloc;
  /// target ul PRBs : only for debug
  uint32_t ue_ul_nb_rb;

  ///check for Total Transmissions
  uint32_t check_for_total_transmissions;

  ///check for MU-MIMO Transmissions
  uint32_t check_for_MUMIMO_transmissions;

  ///check for SU-MIMO Transmissions
  uint32_t check_for_SUMIMO_transmissions;

  ///check for FULL MU-MIMO Transmissions
  uint32_t  FULL_MUMIMO_transmissions;

  /// Counter for total bitrate, bits and throughput in downlink
  uint32_t total_dlsch_bitrate;
  uint32_t total_transmitted_bits;
  uint32_t total_system_throughput;

  int hw_timing_advance;

  time_stats_t phy_proc_tx;
  time_stats_t phy_proc_rx;
  time_stats_t rx_prach;

  time_stats_t ofdm_mod_stats;
  time_stats_t dlsch_common_and_dci;
  time_stats_t dlsch_ue_specific;
  time_stats_t dlsch_encoding_stats;
  time_stats_t dlsch_modulation_stats;
  time_stats_t dlsch_scrambling_stats;
  time_stats_t dlsch_rate_matching_stats;
  time_stats_t dlsch_turbo_encoding_preperation_stats;
  time_stats_t dlsch_turbo_encoding_segmentation_stats;
  time_stats_t dlsch_turbo_encoding_stats;
  time_stats_t dlsch_interleaving_stats;

  time_stats_t rx_dft_stats;
  time_stats_t ulsch_channel_estimation_stats;
  time_stats_t ulsch_freq_offset_estimation_stats;
  time_stats_t ulsch_decoding_stats;
  time_stats_t ulsch_demodulation_stats;
  time_stats_t ulsch_rate_unmatching_stats;
  time_stats_t ulsch_turbo_decoding_stats;
  time_stats_t ulsch_deinterleaving_stats;
  time_stats_t ulsch_demultiplexing_stats;
  time_stats_t ulsch_llr_stats;
  time_stats_t ulsch_tc_init_stats;
  time_stats_t ulsch_tc_alpha_stats;
  time_stats_t ulsch_tc_beta_stats;
  time_stats_t ulsch_tc_gamma_stats;
  time_stats_t ulsch_tc_ext_stats;
  time_stats_t ulsch_tc_intl1_stats;
  time_stats_t ulsch_tc_intl2_stats;

  int32_t pucch1_stats_cnt[NUMBER_OF_UE_MAX][10];
  int32_t pucch1_stats[NUMBER_OF_UE_MAX][10*1024];
  int32_t pucch1_stats_thres[NUMBER_OF_UE_MAX][10*1024];
  int32_t pucch1ab_stats_cnt[NUMBER_OF_UE_MAX][10];
  int32_t pucch1ab_stats[NUMBER_OF_UE_MAX][2*10*1024];
  int32_t pusch_stats_rb[NUMBER_OF_UE_MAX][10240];
  int32_t pusch_stats_round[NUMBER_OF_UE_MAX][10240];
  int32_t pusch_stats_mcs[NUMBER_OF_UE_MAX][10240];
  int32_t pusch_stats_bsr[NUMBER_OF_UE_MAX][10240];
  int32_t pusch_stats_BO[NUMBER_OF_UE_MAX][10240];
  int32_t pusch_signal_threshold;
} PHY_VARS_eNB;


struct turboReqId {
    uint16_t rnti;
    uint16_t frame;
    uint8_t  subframe;
    uint8_t  codeblock;
    uint16_t spare;
} __attribute__((packed));

union turboReqUnion {
    struct turboReqId s;
    uint64_t p;
};

typedef struct TurboDecode_s {
    PHY_VARS_eNB *eNB;
    decoder_if_t *function;
    uint8_t decoded_bytes[3+1768] __attribute__((aligned(32)));
    int UEid;
    int harq_pid;
    int frame;
    int subframe;
    int Fbits;
    int Kr;
    LTE_UL_eNB_HARQ_t *ulsch_harq;
    int nbSegments;
    int segment_r;
    int r_offset;
    int offset;
    int maxIterations;
    int decodeIterations;
} turboDecode_t;

#define TURBO_SIMD_SOFTBITS   96+12+3+3*6144
typedef struct turboEncode_s {
  uint8_t * input;
  int Kr_bytes;
  int filler;
  unsigned int G;
  int r;
  int harq_pid;
  int round;
  int r_offset;
  LTE_eNB_DLSCH_t *dlsch;
  time_stats_t *rm_stats;
  time_stats_t *te_stats;
  time_stats_t *i_stats;
} turboEncode_t;


#endif /* __PHY_DEFS_ENB__H__ */
