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

/*! \file PHY/defs_nr_UE.h
 \brief Top-level constants and data structures definitions for NR UE
 \author Guy De Souza, H. WANG, A. Mico Pereperez
 \date 2018
 \version 0.1
 \company Eurecom
 \email: desouza@eurecom.fr
 \note
 \warning
*/
#ifndef __PHY_DEFS_NR_UE__H__
#define __PHY_DEFS_NR_UE__H__

#ifdef __cplusplus
#include <atomic>
#define _Atomic(X) std::atomic< X >
#endif

#include "defs_nr_common.h"
#include "CODING/nrPolar_tools/nr_polar_pbch_defs.h"

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <math.h>
#include "common_lib.h"
#include "fapi_nr_ue_interface.h"
#include "assertions.h"

#ifdef MEX
  #define msg mexPrintf
#else
    #if ENABLE_RAL
      #include "common/utils/hashtable/hashtable.h"
      #include "COMMON/ral_messages_types.h"
      #include "UTIL/queue.h"
    #endif
    #define msg(aRGS...) LOG_D(PHY, ##aRGS)
#endif
//use msg in the real-time thread context
#define msg_nrt printf
//use msg_nrt in the non real-time context (for initialization, ...)
#ifndef malloc16
    #define malloc16(x) memalign(32,x)
#endif
#define free16(y,x) free(y)
#define bigmalloc malloc
#define bigmalloc16 malloc16
#define openair_free(y,x) free((y))
#define PAGE_SIZE 4096

#ifdef NR_UNIT_TEST
  #define FILE_NAME                " "
  #define LINE_FILE                (0)
  #define NR_TST_PHY_PRINTF(...)   printf(__VA_ARGS__)
#else
  #define FILE_NAME                (__FILE__)
  #define LINE_FILE                (__LINE__)
  #define NR_TST_PHY_PRINTF(...)
#endif

#define PAGE_MASK 0xfffff000
#define virt_to_phys(x) (x)
#define openair_sched_exit() exit(-1)

#define bzero(s,n) (memset((s),0,(n)))
#define cmax(a,b)  ((a>b) ? (a) : (b))
#define cmin(a,b)  ((a<b) ? (a) : (b))
#define cmax3(a,b,c) ((cmax(a,b)>c) ? (cmax(a,b)) : (c))
/// suppress compiler warning for unused arguments
#define UNUSED(x) (void)x;

#include "impl_defs_top.h"
#include "impl_defs_nr.h"
#include "time_meas.h"
#include "PHY/CODING/coding_defs.h"
#include "PHY/TOOLS/tools_defs.h"
#include "common/platform_types.h"
#include "NR_UE_TRANSPORT/nr_transport_ue.h"

#if defined(UPGRADE_RAT_NR)
  #include "PHY/NR_REFSIG/ss_pbch_nr.h"
#endif

#include <pthread.h>
#include "radio/COMMON/common_lib.h"
#include "NR_IF_Module.h"

/// Context data structure for gNB subframe processing
typedef struct {
  /// Component Carrier index
  uint8_t              CC_id;
  /// Last RX timestamp
  openair0_timestamp timestamp_rx;
} UE_nr_proc_t;

typedef enum {
  NR_PBCH_EST=0,
  NR_PDCCH_EST,
  NR_PDSCH_EST,
  NR_SSS_EST,
} NR_CHANNEL_EST_t;

#define debug_msg if (((mac_xface->frame%100) == 0) || (mac_xface->frame < 50)) msg

typedef struct {

  // RRC measurements
  uint32_t rssi;
  int n_adj_cells;
  uint32_t rsrp[7];
  short rsrp_dBm[7];
  int ssb_rsrp_dBm[64];
  // common measurements
  //! estimated noise power (linear)
  unsigned int   n0_power[NB_ANTENNAS_RX];
  //! estimated noise power (dB)
  unsigned short n0_power_dB[NB_ANTENNAS_RX];
  //! total estimated noise power (linear)
  unsigned int   n0_power_tot;
  //! total estimated noise power (dB)
  unsigned short n0_power_tot_dB;
  //! average estimated noise power (linear)
  unsigned int   n0_power_avg;
  //! average estimated noise power (dB)
  unsigned short n0_power_avg_dB;
  //! total estimated noise power (dBm)
  short n0_power_tot_dBm;

  // UE measurements
  //! estimated received spatial signal power (linear)
  fourDimArray_t *rx_spatial_power;
  //! estimated received spatial signal power (dB)
  fourDimArray_t *rx_spatial_power_dB;

  /// estimated received signal power (sum over all TX antennas)
  int            rx_power[NUMBER_OF_CONNECTED_gNB_MAX][NB_ANTENNAS_RX];
  /// estimated received signal power (sum over all TX antennas)
  unsigned short rx_power_dB[NUMBER_OF_CONNECTED_gNB_MAX][NB_ANTENNAS_RX];

  /// estimated received signal power (sum over all TX/RX antennas)
  int            rx_power_tot[NUMBER_OF_CONNECTED_gNB_MAX]; //NEW
  /// estimated received signal power (sum over all TX/RX antennas)
  unsigned short rx_power_tot_dB[NUMBER_OF_CONNECTED_gNB_MAX]; //NEW

  //! estimated received signal power (sum of all TX/RX antennas, time average)
  int            rx_power_avg[NUMBER_OF_CONNECTED_gNB_MAX];
  //! estimated received signal power (sum of all TX/RX antennas, time average, in dB)
  unsigned short rx_power_avg_dB[NUMBER_OF_CONNECTED_gNB_MAX];

  /// SINR (sum of all TX/RX antennas, in dB)
  int            wideband_cqi_tot[NUMBER_OF_CONNECTED_gNB_MAX];
  /// SINR (sum of all TX/RX antennas, time average, in dB)
  int            wideband_cqi_avg[NUMBER_OF_CONNECTED_gNB_MAX];

  //! estimated rssi (dBm)
  short          rx_rssi_dBm[NUMBER_OF_CONNECTED_gNB_MAX];
  //! estimated correlation (wideband linear) between spatial channels (computed in dlsch_demodulation)
  int            rx_correlation[NUMBER_OF_CONNECTED_gNB_MAX][NB_ANTENNAS_RX][NR_MAX_NB_LAYERS*NR_MAX_NB_LAYERS];//
  //! estimated correlation (wideband dB) between spatial channels (computed in dlsch_demodulation)
  int            rx_correlation_dB[NUMBER_OF_CONNECTED_gNB_MAX][2];

  /// Wideband CQI (sum of all RX antennas, in dB, for precoded transmission modes (3,4,5,6), up to 4 spatial streams)
  int            precoded_cqi_dB[NUMBER_OF_CONNECTED_gNB_MAX+1][4];
  /// Subband CQI per RX antenna (= SINR)
  int            subband_cqi[NUMBER_OF_CONNECTED_gNB_MAX][NB_ANTENNAS_RX][NUMBER_OF_SUBBANDS_MAX];
  /// Total Subband CQI  (= SINR)
  int            subband_cqi_tot[NUMBER_OF_CONNECTED_gNB_MAX][NUMBER_OF_SUBBANDS_MAX];
  /// Subband CQI in dB (= SINR dB)
  int            subband_cqi_dB[NUMBER_OF_CONNECTED_gNB_MAX][NB_ANTENNAS_RX][NUMBER_OF_SUBBANDS_MAX];
  /// Total Subband CQI
  int            subband_cqi_tot_dB[NUMBER_OF_CONNECTED_gNB_MAX][NUMBER_OF_SUBBANDS_MAX];
  /// Wideband PMI for each RX antenna
  int            wideband_pmi_re[NUMBER_OF_CONNECTED_gNB_MAX][NB_ANTENNAS_RX];
  /// Wideband PMI for each RX antenna
  int            wideband_pmi_im[NUMBER_OF_CONNECTED_gNB_MAX][NB_ANTENNAS_RX];
  ///Subband PMI for each RX antenna
  int            subband_pmi_re[NUMBER_OF_CONNECTED_gNB_MAX][NUMBER_OF_SUBBANDS_MAX][NB_ANTENNAS_RX];
  ///Subband PMI for each RX antenna
  int            subband_pmi_im[NUMBER_OF_CONNECTED_gNB_MAX][NUMBER_OF_SUBBANDS_MAX][NB_ANTENNAS_RX];
  /// chosen RX antennas (1=Rx antenna 1, 2=Rx antenna 2, 3=both Rx antennas)
  unsigned char           selected_rx_antennas[NUMBER_OF_CONNECTED_gNB_MAX][NUMBER_OF_SUBBANDS_MAX];
  /// Wideband Rank indication
  unsigned char  rank[NUMBER_OF_CONNECTED_gNB_MAX];
  /// Number of RX Antennas
  unsigned char  nb_antennas_rx;
  /// DLSCH error counter
  // short          dlsch_errors;

} PHY_NR_MEASUREMENTS;

typedef struct {
  bool active[2];
  fapi_nr_ul_config_pucch_pdu pucch_pdu[2];
} NR_UE_PUCCH;

typedef struct {
  /// \brief Holds the transmit data in time domain.
  /// For IFFT_FPGA this points to the same memory as PHY_vars->tx_vars[a].TX_DMA_BUFFER.
  /// - first index: tx antenna [0..nb_antennas_tx[
  /// - second index: sample [0..FRAME_LENGTH_COMPLEX_SAMPLES[
  c16_t **txData;

  /// \brief Holds the received data in time domain.
  /// Should point to the same memory as PHY_vars->rx_vars[a].RX_DMA_BUFFER.
  /// - first index: rx antenna [0..nb_antennas_rx[
  /// - second index: sample [0..2*FRAME_LENGTH_COMPLEX_SAMPLES+2048[
  c16_t **rxdata;

  /// estimated frequency offset (in radians) for all subcarriers
  int32_t freq_offset;
  /// nid2 is the PSS value, the PCI (physical cell id) will be: 3*NID1 (SSS value) + NID2 (PSS value)
  int32_t nid2;
} NR_UE_COMMON;

#define NR_PRS_IDFT_OVERSAMP_FACTOR 1  // IDFT oversampling factor for NR PRS channel estimates in time domain, ALLOWED value 16x, and 1x is default(ie. IDFT size is frame_params->ofdm_symbol_size)
typedef struct {
  prs_config_t prs_cfg;
  int32_t reserved;
  prs_meas_t **prs_meas;
} NR_PRS_RESOURCE_t;

typedef struct {
  uint8_t NumPRSResources;
  NR_PRS_RESOURCE_t prs_resource[NR_MAX_PRS_RESOURCES_PER_SET];
} NR_UE_PRS;

#define NR_PDCCH_DEFS_NR_UE
#define NR_NBR_CORESET_ACT_BWP      3  // The number of CoreSets per BWP is limited to 3 (including initial CORESET: ControlResourceId 0)
#define NR_NBR_SEARCHSPACE_ACT_BWP  10 // The number of SearchSpaces per BWP is limited to 10 (including initial SEARCHSPACE: SearchSpaceId 0)
#ifdef NR_PDCCH_DEFS_NR_UE

#define MAX_NR_DCI_DECODED_SLOT     10    // This value is not specified

typedef enum {
  _format_0_0_found = 0,
  _format_0_1_found = 1,
  _format_1_0_found = 2,
  _format_1_1_found = 3,
  _format_2_0_found = 4,
  _format_2_1_found = 5,
  _format_2_2_found = 6,
  _format_2_3_found = 7
} format_found_t;

#endif
typedef struct {
  int nb_search_space;
  fapi_nr_dl_config_dci_dl_pdu_rel15_t pdcch_config[FAPI_NR_MAX_SS];
} NR_UE_PDCCH_CONFIG;

#define NR_PSBCH_MAX_NB_CARRIERS 132
#define NR_PSBCH_MAX_NB_MOD_SYMBOLS 99
#define NR_PSBCH_DMRS_LENGTH 297 // in mod symbols
#define NR_PSBCH_DMRS_LENGTH_DWORD 20 // ceil(2(QPSK)*NR_PBCH_DMRS_LENGTH/32)

/* NR Sidelink PSBCH payload fields
   TODO: This will be removed in the future and
   filled in by the upper layers once developed. */
typedef struct {
  uint32_t coverageIndicator : 1;
  uint32_t tddConfig : 12;
  uint32_t DFN : 10;
  uint32_t slotIndex : 7;
  uint32_t reserved : 2;
} PSBCH_payload;

#define PBCH_A 24

typedef struct {
  int16_t amp;
  bool active;
  fapi_nr_ul_config_prach_pdu prach_pdu;
} NR_UE_PRACH;

typedef struct {
  bool active;
  fapi_nr_dl_config_csiim_pdu_rel15_t csiim_config_pdu;
} NR_UE_CSI_IM;

typedef struct {
  bool active;
  fapi_nr_dl_config_csirs_pdu_rel15_t csirs_config_pdu;
} NR_UE_CSI_RS;

typedef struct {
  bool active;
  fapi_nr_ul_config_srs_pdu srs_config_pdu;
} NR_UE_SRS;

// structure used for multiple SSB detection
typedef struct NR_UE_SSB {
  uint8_t i_ssb;   // i_ssb between 0 and 7 (it corresponds to ssb_index only for Lmax=4,8)
  uint8_t n_hf;    // n_hf = 0,1 for Lmax =4 or n_hf = 0 for Lmax =8,64
  uint32_t metric; // metric to order SSB hypothesis
  uint32_t c_re;
  uint32_t c_im;
  struct NR_UE_SSB *next_ssb;
} NR_UE_SSB;

typedef struct UE_NR_SCAN_INFO_s {
  /// 10 best amplitudes (linear) for each pss signals
  int32_t amp[3][10];
  /// 10 frequency offsets (kHz) corresponding to best amplitudes, with respect do minimum DL frequency in the band
  int32_t freq_offset_Hz[3][10];
} UE_NR_SCAN_INFO_t;

/// Top-level PHY Data Structure for UE
typedef struct PHY_VARS_NR_UE_s {
  /// \brief Module ID indicator for this instance
  uint8_t Mod_id;
  /// \brief Component carrier ID for this PHY instance
  uint8_t CC_id;
  /// \brief Mapping of CC_id antennas to cards
  openair0_rf_map      rf_map;
  /// \brief Indicator that UE should perform band scanning
  int UE_scan;
  /// \brief Indicator that UE should perform coarse scanning around carrier
  int UE_scan_carrier;
  /// \brief Indicator that UE should enable estimation and compensation of frequency offset
  int UE_fo_compensation;
  /// IF frequency for RF
  uint64_t if_freq;
  /// UL IF frequency offset for RF
  int if_freq_off;
  /// \brief Indicator that UE is synchronized to a gNB
  int is_synchronized;
  /// \brief Indicator that UE is synchronized to a SyncRef UE on Sidelink
  int is_synchronized_sl;
  /// \brief Target gNB Nid_cell when UE is resynchronizing
  int target_Nid_cell;
  /// \brief Indicator that UE is an SynchRef UE
  int sync_ref;
  /// Data structure for UE process scheduling
  UE_nr_proc_t proc;
  /// Flag to indicate the UE shouldn't do timing correction at all
  int no_timing_correction;
  /// \brief Total gain of the TX chain (16-bit baseband I/Q to antenna)
  uint32_t tx_total_gain_dB;
  /// \brief Total gain of the RX chain (antenna to baseband I/Q) This is a function of rx_gain_mode (and the corresponding gain) and the rx_gain of the card.
  uint32_t rx_total_gain_dB;
  /// \brief Total gains with maximum RF gain stage (ExpressMIMO2/Lime)
  uint32_t rx_gain_max[4];
  /// \brief Total gains with medium RF gain stage (ExpressMIMO2/Lime)
  uint32_t rx_gain_med[4];
  /// \brief Total gains with bypassed RF gain stage (ExpressMIMO2/Lime)
  uint32_t rx_gain_byp[4];
  /// \brief Current transmit power
  int16_t tx_power_dBm[NR_MAX_SLOTS_PER_FRAME];
  /// \brief Total number of REs in current transmission
  int tx_total_RE[NR_MAX_SLOTS_PER_FRAME];
  /// \brief Maximum transmit power
  int8_t tx_power_max_dBm;
  /// \brief Number of gNB seen by UE
  uint8_t n_connected_gNB;
  /// \brief indicator that Handover procedure has been initiated
  uint8_t ho_initiated;
  /// \brief indicator that Handover procedure has been triggered
  uint8_t ho_triggered;
  /// threshold for false dci detection
  int dci_thres;
  /// \brief Measurement variables.
  PHY_NR_MEASUREMENTS measurements;
  NR_DL_FRAME_PARMS  frame_parms;
  /// \brief Frame parame before ho used to recover if ho fails.
  NR_DL_FRAME_PARMS  frame_parms_before_ho;
  NR_UE_COMMON    common_vars;

  nr_ue_if_module_t *if_inst;

  fapi_nr_config_request_t nrUE_config;
  nr_synch_request_t synch_request;

  NR_UE_PRACH     *prach_vars[NUMBER_OF_CONNECTED_gNB_MAX];
  NR_UE_CSI_IM    *csiim_vars[NUMBER_OF_CONNECTED_gNB_MAX];
  NR_UE_CSI_RS    *csirs_vars[NUMBER_OF_CONNECTED_gNB_MAX];
  NR_UE_SRS       *srs_vars[NUMBER_OF_CONNECTED_gNB_MAX];
  NR_UE_PRS       *prs_vars[NR_MAX_PRS_COMB_SIZE];
  uint8_t          prs_active_gNBs;
  NR_DL_UE_HARQ_t  dl_harq_processes[2][NR_MAX_DLSCH_HARQ_PROCESSES];
  NR_UL_UE_HARQ_t  ul_harq_processes[NR_MAX_ULSCH_HARQ_PROCESSES];
  
  //Paging parameters
  uint32_t              IMSImod1024;
  uint32_t              PF;
  uint32_t              PO;

#if defined(UPGRADE_RAT_NR)

  /// demodulation reference signal for NR PBCH
  uint32_t dmrs_pbch_bitmap_nr[DMRS_PBCH_I_SSB][DMRS_PBCH_N_HF][DMRS_BITMAP_SIZE];

#endif


  /// PBCH DMRS sequence
  uint32_t nr_gold_pbch[2][64][NR_PBCH_DMRS_LENGTH_DWORD];

  /// PDSCH DMRS
  uint32_t ****nr_gold_pdsch[NUMBER_OF_CONNECTED_eNB_MAX];

  // Scrambling IDs used in PDSCH DMRS
  uint16_t scramblingID_dlsch[2];

  // Scrambling IDs used in PUSCH DMRS
  uint16_t scramblingID_ulsch[2];

  /// PDCCH DMRS
  uint32_t ***nr_gold_pdcch[NUMBER_OF_CONNECTED_eNB_MAX];

  // Scrambling IDs used in PDCCH DMRS
  uint16_t scramblingID_pdcch;

  /// PUSCH DMRS sequence
  uint32_t ****nr_gold_pusch_dmrs;

  // PRS sequence per gNB, per resource
  uint32_t *****nr_gold_prs;

  c16_t X_u[64][839];

  // flag to activate PRB based averaging of channel estimates
  // when off, defaults to frequency domain interpolation
  int chest_freq;
  int chest_time;

  UE_NR_SCAN_INFO_t scan_info[NB_BANDS_MAX];

  /// \brief ?.
  /// - first index: gNB [0..NUMBER_OF_CONNECTED_gNB_MAX[ (hard coded)
  uint32_t total_TBS[NUMBER_OF_CONNECTED_gNB_MAX];
  /// \brief ?.
  /// - first index: gNB [0..NUMBER_OF_CONNECTED_gNB_MAX[ (hard coded)
  uint32_t total_TBS_last[NUMBER_OF_CONNECTED_gNB_MAX];
  /// \brief ?.
  /// - first index: gNB [0..NUMBER_OF_CONNECTED_gNB_MAX[ (hard coded)
  uint32_t bitrate[NUMBER_OF_CONNECTED_gNB_MAX];
  /// \brief ?.
  /// - first index: gNB [0..NUMBER_OF_CONNECTED_gNB_MAX[ (hard coded)
  uint32_t total_received_bits[NUMBER_OF_CONNECTED_gNB_MAX];
  int dlsch_errors[NUMBER_OF_CONNECTED_gNB_MAX];
  int dlsch_errors_last[NUMBER_OF_CONNECTED_gNB_MAX];
  int dlsch_received[NUMBER_OF_CONNECTED_gNB_MAX];
  int dlsch_received_last[NUMBER_OF_CONNECTED_gNB_MAX];
  int dlsch_fer[NUMBER_OF_CONNECTED_gNB_MAX];
  int dlsch_SI_received[NUMBER_OF_CONNECTED_gNB_MAX];
  int dlsch_SI_errors[NUMBER_OF_CONNECTED_gNB_MAX];
  int dlsch_ra_received[NUMBER_OF_CONNECTED_gNB_MAX];
  int dlsch_ra_errors[NUMBER_OF_CONNECTED_gNB_MAX];
  int dlsch_p_received[NUMBER_OF_CONNECTED_gNB_MAX];
  int dlsch_p_errors[NUMBER_OF_CONNECTED_gNB_MAX];
  int dlsch_mch_received_sf[MAX_MBSFN_AREA][NUMBER_OF_CONNECTED_gNB_MAX];
  int dlsch_mch_received[NUMBER_OF_CONNECTED_gNB_MAX];
  int dlsch_mcch_received[MAX_MBSFN_AREA][NUMBER_OF_CONNECTED_gNB_MAX];
  int dlsch_mtch_received[MAX_MBSFN_AREA][NUMBER_OF_CONNECTED_gNB_MAX];
  int dlsch_mcch_errors[MAX_MBSFN_AREA][NUMBER_OF_CONNECTED_gNB_MAX];
  int dlsch_mtch_errors[MAX_MBSFN_AREA][NUMBER_OF_CONNECTED_gNB_MAX];
  int dlsch_mcch_trials[MAX_MBSFN_AREA][NUMBER_OF_CONNECTED_gNB_MAX];
  int dlsch_mtch_trials[MAX_MBSFN_AREA][NUMBER_OF_CONNECTED_gNB_MAX];
  int current_dlsch_cqi[NUMBER_OF_CONNECTED_gNB_MAX];
  uint8_t               decode_SIB;
  uint8_t               decode_MIB;
  uint8_t               init_sync_frame;
  /// temporary offset during cell search prior to MIB decoding
  int              ssb_offset;
  uint16_t         symbol_offset;  /// offset in terms of symbols for detected ssb in sync
  int              rx_offset;      /// Timing offset
  int              rx_offset_diff; /// Timing adjustment for ofdm symbol0 on HW USRP
  int64_t          max_pos_fil;    /// Timing offset IIR filter
  bool             apply_timing_offset;     /// Do time sync for current frame
  int              time_sync_cell;

  /// Timing Advance updates variables
  /// Timing advance update computed from the TA command signalled from gNB
  int timing_advance;
  int N_TA_offset; ///timing offset used in TDD
  int ta_frame;
  int ta_slot;
  int ta_command;

  /// Flag to tell if UE is secondary user (cognitive mode)
  unsigned char    is_secondary_ue;
  /// Flag to tell if secondary gNB has channel estimates to create NULL-beams from.
  unsigned char    has_valid_precoder;
  /// hold the precoder for NULL beam to the primary gNB
  int              **ul_precoder_S_UE;
  /// holds the maximum channel/precoder coefficient
  char             log2_maxp;

  /// Flag to initialize averaging of PHY measurements
  int init_averaging;

  /// \brief sinr for all subcarriers of the current link (used only for abstraction).
  /// - first index: ? [0..12*N_RB_DL[
  double *sinr_dB;

  /// sinr_effective used for CQI calulcation
  double sinr_eff;

  /// N0 (used for abstraction)
  double N0;

  uint8_t max_ldpc_iterations;

  /// SRS variables
  nr_srs_info_t *nr_srs_info;

  /// CSI variables
  nr_csi_info_t *nr_csi_info;

  //#if defined(UPGRADE_RAT_NR)
#if 1
  SystemInformationBlockType1_nr_t systemInformationBlockType1_nr;
#endif

  //#if defined(UPGRADE_RAT_NR)
#if 1
  scheduling_request_config_t scheduling_request_config_nr[NUMBER_OF_CONNECTED_gNB_MAX];

#endif

  time_stats_t phy_proc;
  time_stats_t phy_proc_tx;
  time_stats_t phy_proc_rx;

  time_stats_t ue_ul_indication_stats;

  uint32_t use_ia_receiver;

  time_stats_t ofdm_mod_stats;
  time_stats_t ulsch_encoding_stats;
  time_stats_t ulsch_ldpc_encoding_stats;
  time_stats_t ulsch_modulation_stats;
  time_stats_t ulsch_segmentation_stats;
  time_stats_t ulsch_rate_matching_stats;
  time_stats_t ulsch_interleaving_stats;
  time_stats_t ulsch_multiplexing_stats;

  time_stats_t ue_front_end_stat;
  time_stats_t ue_front_end_per_slot_stat[LTE_SLOTS_PER_SUBFRAME];
  time_stats_t pdcch_procedures_stat;
  time_stats_t pdsch_procedures_stat;
  time_stats_t pdsch_procedures_per_slot_stat[LTE_SLOTS_PER_SUBFRAME];
  time_stats_t dlsch_procedures_stat;

  time_stats_t rx_pdsch_stats;
  time_stats_t ofdm_demod_stats;
  time_stats_t dlsch_rx_pdcch_stats;
  time_stats_t rx_dft_stats;
  time_stats_t dlsch_channel_estimation_stats;
  time_stats_t dlsch_freq_offset_estimation_stats;
  time_stats_t dlsch_decoding_stats;
  time_stats_t dlsch_demodulation_stats;
  time_stats_t dlsch_rate_unmatching_stats;
  time_stats_t dlsch_ldpc_decoding_stats;
  time_stats_t dlsch_deinterleaving_stats;
  time_stats_t dlsch_llr_stats;
  time_stats_t dlsch_llr_stats_parallelization[LTE_SLOTS_PER_SUBFRAME];
  time_stats_t dlsch_unscrambling_stats;
  time_stats_t dlsch_rate_matching_stats;
  time_stats_t dlsch_ldpc_encoding_stats;
  time_stats_t dlsch_interleaving_stats;
  time_stats_t dlsch_tc_init_stats;
  time_stats_t dlsch_tc_alpha_stats;
  time_stats_t dlsch_tc_beta_stats;
  time_stats_t dlsch_tc_gamma_stats;
  time_stats_t dlsch_tc_ext_stats;
  time_stats_t dlsch_tc_intl1_stats;
  time_stats_t dlsch_tc_intl2_stats;
  time_stats_t tx_prach;

  /// RF and Interface devices per CC
  openair0_device rfdevice;

#if ENABLE_RAL
  hash_table_t    *ral_thresholds_timed;
  SLIST_HEAD(ral_thresholds_gen_poll_s, ral_threshold_phy_t) ral_thresholds_gen_polled[RAL_LINK_PARAM_GEN_MAX];
  SLIST_HEAD(ral_thresholds_lte_poll_s, ral_threshold_phy_t) ral_thresholds_lte_polled[RAL_LINK_PARAM_LTE_MAX];
#endif
  int dl_errors;
  _Atomic(int) dl_stats[16];
  void* scopeData;
  // Pointers to hold PDSCH data only for phy simulators
  void *phy_sim_rxdataF;
  void *phy_sim_pdsch_llr;
  void *phy_sim_pdsch_rxdataF_ext;
  void *phy_sim_pdsch_rxdataF_comp;
  void *phy_sim_pdsch_dl_ch_estimates;
  void *phy_sim_pdsch_dl_ch_estimates_ext;
  uint8_t *phy_sim_dlsch_b;
  notifiedFIFO_t phy_config_ind;
  notifiedFIFO_t tx_resume_ind_fifo[NR_MAX_SLOTS_PER_FRAME];
} PHY_VARS_NR_UE;

typedef struct {
  openair0_timestamp timestamp_tx;
  int gNB_id;
  /// NR slot index within frame_tx [0 .. slots_per_frame - 1] to act upon for transmission
  int nr_slot_tx;
  int rx_slot_type;
  /// NR slot index within frame_rx [0 .. slots_per_frame - 1] to act upon for transmission
  int nr_slot_rx;
  int tx_slot_type;
  //#endif
  /// frame to act upon for transmission
  int frame_tx;
  /// frame to act upon for reception
  int frame_rx;
} UE_nr_rxtx_proc_t;

typedef struct nr_phy_data_tx_s {
  NR_UE_ULSCH_t ulsch;
  NR_UE_PUCCH pucch_vars;
} nr_phy_data_tx_t;

typedef struct nr_phy_data_s {
  NR_UE_PDCCH_CONFIG phy_pdcch_config;
  NR_UE_DLSCH_t dlsch[2];
} nr_phy_data_t;
/* this structure is used to pass both UE phy vars and
 * proc to the function UE_thread_rxn_txnp4
 */
typedef struct nr_rxtx_thread_data_s {
  UE_nr_rxtx_proc_t proc;
  PHY_VARS_NR_UE    *UE;
  int writeBlockSize;
  nr_phy_data_t phy_data;
  int tx_wait_for_dlsch;
} nr_rxtx_thread_data_t;

typedef struct LDPCDecode_ue_s {
  PHY_VARS_NR_UE *phy_vars_ue;
  NR_DL_UE_HARQ_t *harq_process;
  t_nrLDPC_dec_params decoderParms;
  NR_UE_DLSCH_t *dlsch;
  short* dlsch_llr;
  int dlsch_id;
  int harq_pid;
  int rv_index;
  int A;
  int E;
  int Kc;
  int Qm;
  int Kr_bytes;
  int nbSegments;
  int segment_r;
  int r_offset;
  int offset;
  int Tbslbrm;
  int decodeIterations;
  time_stats_t ts_deinterleave;
  time_stats_t ts_rate_unmatch;
  time_stats_t ts_ldpc_decode;
  UE_nr_rxtx_proc_t proc;
} ldpcDecode_ue_t;

#include "SIMULATION/ETH_TRANSPORT/defs.h"
#endif
