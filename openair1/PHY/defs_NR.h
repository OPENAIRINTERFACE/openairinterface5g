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

/*#ifndef __INIT_DEFS_NR__H__
#define __INIT_DEFS_NR__H__

#define _GNU_SOURCE
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <linux/sched.h>
#include <signal.h>
#include <execinfo.h>
#include <getopt.h>
#include <sys/sysinfo.h>
#include <malloc.h>
#include <string.h>
#include <math.h>

#include "types.h"
#include "defs.h"
#include "assertions.h"*/
#include "impl_defs_nr.h"

#define MAX_NUM_SUBCARRIER_SPACING 5

#define NR_SYMBOLS_PER_SLOT 14

#define NR_PSS_LENGTH 127
#define NR_SSS_LENGTH 127

#define ONE_OVER_SQRT2_Q15 23170
#define ONE_OVER_TWO_Q15 16384

#define MOD_TABLE_SIZE_SHORT 6
#define MOD_TABLE_BPSK_OFFSET 1

typedef enum {
  NR_MU_0=0,
  NR_MU_1,
  NR_MU_2,
  NR_MU_3,
  NR_MU_4,
} nr_numerology_index_e;

//NR types temporarily placed here
typedef struct {
  uint32_t subcarrier_spacing;
  /// 3/4 sampling
  uint8_t threequarter_fs;
  /// Size of FFT
  uint16_t ofdm_symbol_size;
  /// Number of prefix samples in all but first symbol of slot
  uint16_t nb_prefix_samples;
  /// Number of prefix samples in first symbol of slot
  uint16_t nb_prefix_samples0;
  /// Carrier offset in FFT buffer for first RE in PRB0
  uint16_t first_carrier_offset;
  /// Number of OFDM/SC-FDMA symbols in one slot
  uint16_t symbols_per_slot;
  /// Number of slots per subframe
  uint16_t slots_per_subframe;
  /// Number of samples in a subframe
  uint32_t samples_per_subframe;
  /// Number of samples in a radio frame
  uint32_t samples_per_frame;
  /// Number of samples in a subframe without CP
  uint32_t samples_per_subframe_wCP;
  /// Number of samples in a radio frame without CP
  uint32_t samples_per_frame_wCP;

} NR_DL_FRAME_PARMS;

/// Top-level PHY Data Structure for gNB
typedef struct PHY_VARS_gNB_s {
  /// Module ID indicator for this instance
  module_id_t          Mod_id;
  uint8_t              CC_id;
  uint8_t              configured;
  gNB_proc_t           proc;
  int                  single_thread_flag;
  int                  abstraction_flag;
  int                  num_RU;
  RU_t                 *RU_list[MAX_NUM_RU_PER_eNB];
  /// Ethernet parameters for northbound midhaul interface
  eth_params_t         eth_params_n;
  /// Ethernet parameters for fronthaul interface
  eth_params_t         eth_params;
  int                  rx_total_gain_dB;
  int                  (*td)(struct PHY_VARS_gNB_s *eNB,int UE_id,int harq_pid,int llr8_flag);
  int                  (*te)(struct PHY_VARS_gNB_s *,uint8_t *,uint8_t,LTE_eNB_DLSCH_t *,int,uint8_t,time_stats_t *,time_stats_t *,time_stats_t *);
  int                  (*start_if)(struct RU_t_s *ru,struct PHY_VARS_gNB_s *eNB);
  uint8_t              local_flag;
  nfapi_config_request_t  gNB_config;
  NR_DL_FRAME_PARMS   frame_parms;
  PHY_MEASUREMENTS_gNB measurements;
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

  Sched_Rsp_t          Sched_INFO;
  LTE_eNB_PDCCH        pdcch_vars[2];
  LTE_eNB_PHICH        phich_vars[2];

  NR_gNB_COMMON       common_vars;
  LTE_eNB_UCI          uci_vars[NUMBER_OF_UE_MAX];
  LTE_eNB_SRS          srs_vars[NUMBER_OF_UE_MAX];
  LTE_eNB_PBCH         pbch;
  LTE_eNB_PUSCH       *pusch_vars[NUMBER_OF_UE_MAX];
  LTE_eNB_PRACH        prach_vars;
  LTE_eNB_DLSCH_t     *dlsch[NUMBER_OF_UE_MAX][2];   // Nusers times two spatial streams
  LTE_eNB_ULSCH_t     *ulsch[NUMBER_OF_UE_MAX+1];      // Nusers + number of RA
  LTE_eNB_DLSCH_t     *dlsch_SI,*dlsch_ra,*dlsch_p;
  LTE_eNB_DLSCH_t     *dlsch_MCH;
  LTE_eNB_DLSCH_t     *dlsch_PCH;
  LTE_eNB_UE_stats     UE_stats[NUMBER_OF_UE_MAX];
  LTE_eNB_UE_stats    *UE_stats_ptr[NUMBER_OF_UE_MAX];

  uint8_t pbch_configured;
  uint8_t pbch_pdu[4]; //PBCH_PDU_SIZE
  char gNB_generate_rar;

  /// Indicator set to 0 after first SR
  uint8_t first_sr[NUMBER_OF_UE_MAX];

  uint32_t max_peak_val;

  /// \brief sinr for all subcarriers of the current link (used only for abstraction).
  /// first index: ? [0..N_RB_DL*12[
  double *sinr_dB;

  /// N0 (used for abstraction)
  double N0;

  unsigned char first_run_timing_advance[NUMBER_OF_UE_MAX];
  unsigned char first_run_I0_measurements;

  
  unsigned char    is_secondary_gNB; // primary by default
  unsigned char    is_init_sync;     /// Flag to tell if initial synchronization is performed. This affects how often the secondary eNB will listen to the PSS from the primary system.
  unsigned char    has_valid_precoder; /// Flag to tell if secondary eNB has channel estimates to create NULL-beams from, and this B/F vector is created.
  unsigned char    PgNB_id;          /// id of Primary eNB

  /// hold the precoder for NULL beam to the primary user
  int              **dl_precoder_SgNB[3];
  char             log2_maxp; /// holds the maximum channel/precoder coefficient

  /// if ==0 enables phy only test mode
  int mac_enabled;
  /// counter to average prach energh over first 100 prach opportunities
  int prach_energy_counter;

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
  struct PhysicalConfigDedicated *physicalConfigDedicated[NUMBER_OF_UE_MAX];


  uint32_t rb_mask_ul[4];

  /// Information regarding TM5
  MU_MIMO_mode mu_mimo_mode[NUMBER_OF_UE_MAX];


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

  time_stats_t phy_proc;
  time_stats_t phy_proc_tx;
  time_stats_t phy_proc_rx;
  time_stats_t rx_prach;

  time_stats_t ofdm_mod_stats;
  time_stats_t dlsch_encoding_stats;
  time_stats_t dlsch_modulation_stats;
  time_stats_t dlsch_scrambling_stats;
  time_stats_t dlsch_rate_matching_stats;
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

#ifdef LOCALIZATION
  /// time state for localization
  time_stats_t localization_stats;
#endif

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
} PHY_VARS_gNB;

/// Context data structure for RX/TX portion of subframe processing
typedef struct {
  /// Component Carrier index
  uint8_t              CC_id;
  /// timestamp transmitted to HW
  openair0_timestamp timestamp_tx;
  /// subframe to act upon for transmission
  int subframe_tx;
  /// subframe to act upon for reception
  int subframe_rx;
  /// frame to act upon for transmission
  int frame_tx;
  /// frame to act upon for reception
  int frame_rx;
  /// \brief Instance count for RXn-TXnp4 processing thread.
  /// \internal This variable is protected by \ref mutex_rxtx.
  int instance_cnt_rxtx;
  /// pthread structure for RXn-TXnp4 processing thread
  pthread_t pthread_rxtx;
  /// pthread attributes for RXn-TXnp4 processing thread
  pthread_attr_t attr_rxtx;
  /// condition variable for tx processing thread
  pthread_cond_t cond_rxtx;
  /// mutex for RXn-TXnp4 processing thread
  pthread_mutex_t mutex_rxtx;
  /// scheduling parameters for RXn-TXnp4 thread
  struct sched_param sched_param_rxtx;
} gNB_rxtx_proc_t;


/// Context data structure for eNB subframe processing
typedef struct gNB_proc_t_s {
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
  /// frame to act upon for reception
  int frame_rx;
  /// frame to act upon for transmission
  int frame_tx;
  /// frame to act upon for PRACH
  int frame_prach;
  /// \internal This variable is protected by \ref mutex_td.
  int instance_cnt_td;
  /// \internal This variable is protected by \ref mutex_te.
  int instance_cnt_te;
  /// \internal This variable is protected by \ref mutex_prach.
  int instance_cnt_prach;

  // instance count for over-the-air gNB synchronization
  int instance_cnt_synch;
  /// \internal This variable is protected by \ref mutex_asynch_rxtx.
  int instance_cnt_asynch_rxtx;
  /// pthread structure for eNB single processing thread
  pthread_t pthread_single;
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
  /// scheduling parameters for asynch_rxtx thread
  struct sched_param sched_param_asynch_rxtx;
  /// pthread structure for parallel turbo-decoder thread
  pthread_t pthread_td;
  /// pthread structure for parallel turbo-encoder thread
  pthread_t pthread_te;
  /// pthread structure for PRACH thread
  pthread_t pthread_prach;
  /// condition variable for parallel turbo-decoder thread
  pthread_cond_t cond_td;
  /// condition variable for parallel turbo-encoder thread
  pthread_cond_t cond_te;
  /// condition variable for PRACH processing thread;
  pthread_cond_t cond_prach;
  /// condition variable for asynch RX/TX thread
  pthread_cond_t cond_asynch_rxtx;
  /// mutex for parallel turbo-decoder thread
  pthread_mutex_t mutex_td;
  /// mutex for parallel turbo-encoder thread
  pthread_mutex_t mutex_te;
  /// mutex for PRACH thread
  pthread_mutex_t mutex_prach;
  /// mutex for asynch RX/TX thread
  pthread_mutex_t mutex_asynch_rxtx;
  /// mutex for RU access to eNB processing (PDSCH/PUSCH)
  pthread_mutex_t mutex_RU;
  /// mutex for RU access to eNB processing (PRACH)
  pthread_mutex_t mutex_RU_PRACH;
  /// mutex for RU access to eNB processing (PRACH BR)
  pthread_mutex_t mutex_RU_PRACH_br;
  /// mask for RUs serving eNB (PDSCH/PUSCH)
  int RU_mask;
  /// mask for RUs serving eNB (PRACH)
  int RU_mask_prach;
  /// parameters for turbo-decoding worker thread
  td_params tdp;
  /// parameters for turbo-encoding worker thread
  te_params tep;
  /// set of scheduling variables RXn-TXnp4 threads
  gNB_rxtx_proc_t proc_rxtx[2];
} gNB_proc_t;

typedef struct {
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
  // gNB measurements (per user)
  //! estimated received spatial signal power (linear)
  unsigned int   rx_spatial_power[NUMBER_OF_UE_MAX][2][2];
  //! estimated received spatial signal power (dB)
  unsigned short rx_spatial_power_dB[NUMBER_OF_UE_MAX][2][2];
  //! estimated rssi (dBm)
  short          rx_rssi_dBm[NUMBER_OF_UE_MAX];
  //! estimated correlation (wideband linear) between spatial channels (computed in dlsch_demodulation)
  int            rx_correlation[NUMBER_OF_UE_MAX][2];
  //! estimated correlation (wideband dB) between spatial channels (computed in dlsch_demodulation)
  int            rx_correlation_dB[NUMBER_OF_UE_MAX][2];

  /// Wideband CQI (= SINR)
  int            wideband_cqi[NUMBER_OF_UE_MAX][MAX_NUM_RU_PER_eNB];
  /// Wideband CQI in dB (= SINR dB)
  int            wideband_cqi_dB[NUMBER_OF_UE_MAX][MAX_NUM_RU_PER_eNB];
  /// Wideband CQI (sum of all RX antennas, in dB)
  char           wideband_cqi_tot[NUMBER_OF_UE_MAX];
  /// Subband CQI per RX antenna and RB (= SINR)
  int            subband_cqi[NUMBER_OF_UE_MAX][MAX_NUM_RU_PER_eNB][100];
  /// Total Subband CQI and RB (= SINR)
  int            subband_cqi_tot[NUMBER_OF_UE_MAX][100];
  /// Subband CQI in dB and RB (= SINR dB)
  int            subband_cqi_dB[NUMBER_OF_UE_MAX][MAX_NUM_RU_PER_eNB][100];
  /// Total Subband CQI and RB
  int            subband_cqi_tot_dB[NUMBER_OF_UE_MAX][100];
  /// PRACH background noise level
  int            prach_I0;
} PHY_MEASUREMENTS_gNB;


#endif
