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

/*! \file PHY/defs.h
 \brief Top-level defines and structure definitions
 \author R. Knopp, F. Kaltenberger
 \date 2011
 \version 0.1
 \company Eurecom
 \email: knopp@eurecom.fr,florian.kaltenberger@eurecom.fr
 \note
 \warning
*/
#ifndef __PHY_DEFS__H__
#define __PHY_DEFS__H__

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <math.h>
//#include <complex.h>
#include "assertions.h"
#ifdef MEX
# define msg mexPrintf
#else
# ifdef OPENAIR2
#   if ENABLE_RAL
#     include "collection/hashtable/hashtable.h"
#     include "COMMON/ral_messages_types.h"
#     include "UTIL/queue.h"
#   endif
#   include "log.h"
#   define msg(aRGS...) LOG_D(PHY, ##aRGS)
# else
#   define msg printf
# endif
#endif
//use msg in the real-time thread context
#define msg_nrt printf
//use msg_nrt in the non real-time context (for initialization, ...)
#ifndef malloc16
#  ifdef __AVX2__
#    define malloc16(x) memalign(32,x)
#  else
#    define malloc16(x) memalign(16,x)
#  endif
#endif
#define free16(y,x) free(y)
#define bigmalloc malloc
#define bigmalloc16 malloc16
#define openair_free(y,x) free((y))
#define PAGE_SIZE 4096

//#ifdef SHRLIBDEV
//extern int rxrescale;
//#define RX_IQRESCALELEN rxrescale
//#else
//#define RX_IQRESCALELEN 15
//#endif

//! \brief Allocate \c size bytes of memory on the heap with alignment 16 and zero it afterwards.
//! If no more memory is available, this function will terminate the program with an assertion error.
static inline void* malloc16_clear( size_t size )
{
#ifdef __AVX2__
  void* ptr = memalign(32, size);
#else
  void* ptr = memalign(16, size);
#endif
  DevAssert(ptr);
  memset( ptr, 0, size );
  return ptr;
}



#define PAGE_MASK 0xfffff000
#define virt_to_phys(x) (x)

#define openair_sched_exit() exit(-1)


#define max(a,b)  ((a)>(b) ? (a) : (b))
#define min(a,b)  ((a)<(b) ? (a) : (b))


#define bzero(s,n) (memset((s),0,(n)))

#define cmax(a,b)  ((a>b) ? (a) : (b))
#define cmin(a,b)  ((a<b) ? (a) : (b))

#define cmax3(a,b,c) ((cmax(a,b)>c) ? (cmax(a,b)) : (c))

/// suppress compiler warning for unused arguments
#define UNUSED(x) (void)x;


#include "impl_defs_top.h"
#include "impl_defs_lte.h"

#include "PHY/TOOLS/time_meas.h"
#include "PHY/CODING/defs.h"
#include "PHY/TOOLS/defs.h"
#include "platform_types.h"

#ifdef OPENAIR_LTE

//#include "PHY/LTE_ESTIMATION/defs.h"

#include "PHY/LTE_TRANSPORT/defs.h"
#include <pthread.h>

#define NUM_DCI_MAX 32

#define NUMBER_OF_eNB_SECTORS_MAX 3

#define NB_BANDS_MAX 8

typedef enum {normal_txrx=0,rx_calib_ue=1,rx_calib_ue_med=2,rx_calib_ue_byp=3,debug_prach=4,no_L2_connect=5,calib_prach_tx=6,rx_dump_frame=7,loop_through_memory=8} runmode_t;

enum transmission_access_mode {
  NO_ACCESS=0,
  POSTPONED_ACCESS,
  CANCELED_ACCESS,
  UNKNOWN_ACCESS,
  SCHEDULED_ACCESS,
  CBA_ACCESS
};

typedef struct UE_SCAN_INFO_s {
  /// 10 best amplitudes (linear) for each pss signals
  int32_t amp[3][10];
  /// 10 frequency offsets (kHz) corresponding to best amplitudes, with respect do minimum DL frequency in the band
  int32_t freq_offset_Hz[3][10];
} UE_SCAN_INFO_t;

#if ENABLE_RAL
typedef struct ral_threshold_phy_s {
  SLIST_ENTRY(ral_threshold_phy_s) ral_thresholds;
  ral_threshold_t                  threshold;
  ral_th_action_t                  th_action;
  ral_link_param_t                 link_param;
  long                             timer_id;
} ral_threshold_phy_t;
#endif

/// Context data structure for eNB subframe processing
typedef struct {
  /// Component Carrier index
  uint8_t              CC_id;
  /// subframe index
  int subframe;
  /// subframe to act upon for transmission
  int subframe_tx;
  /// subframe to act upon for reception
  int subframe_rx;
  /// frame to act upon for transmission
  int frame_tx;
  /// frame to act upon for reception
  int frame_rx;
  /// \brief Instance count for tx processing thread.
  /// \internal This variable is protected by \ref mutex_tx.
  int instance_cnt_tx;
  /// \brief Instance count for rx processing thread.
  /// \internal This variable is protected by \ref mutex_rx.
  int instance_cnt_rx;
  /// pthread structure for tx processing thread
  pthread_t pthread_tx;
  /// pthread structure for rx processing thread
  pthread_t pthread_rx;
  /// condition variable for tx processing thread
  pthread_cond_t cond_tx;
  /// condition variable for rx processing thread
  pthread_cond_t cond_rx;
  /// mutex for tx processing thread
  pthread_mutex_t mutex_tx;
  /// mutex for tx processing thread
  pthread_mutex_t mutex_rx;
} eNB_proc_t;

//! \brief Number of eNB TX and RX threads.
//! This number must be equal to the number of LTE subframes (10). Each thread is responsible for a single subframe.
#define NUM_ENB_THREADS 10

/// Top-level PHY Data Structure for eNB
typedef struct PHY_VARS_eNB_s {
  /// Module ID indicator for this instance
  module_id_t          Mod_id;
  uint8_t              CC_id;
  eNB_proc_t           proc[NUM_ENB_THREADS];
  uint8_t              local_flag;
  uint32_t             rx_total_gain_eNB_dB;
  LTE_DL_FRAME_PARMS   lte_frame_parms;
  PHY_MEASUREMENTS_eNB PHY_measurements_eNB[NUMBER_OF_eNB_SECTORS_MAX]; /// Measurement variables
  LTE_eNB_COMMON       lte_eNB_common_vars;
  LTE_eNB_SRS          lte_eNB_srs_vars[NUMBER_OF_UE_MAX];
  LTE_eNB_PBCH         lte_eNB_pbch;
  /// \brief ?.
  /// - first index: UE [0..NUMBER_OF_UE_MAX[ (hard coded)
  /// - second index: UE [0..NUMBER_OF_UE_MAX[
  LTE_eNB_PUSCH       *lte_eNB_pusch_vars[NUMBER_OF_UE_MAX];
  LTE_eNB_PRACH        lte_eNB_prach_vars;
  LTE_eNB_DLSCH_t     *dlsch_eNB[NUMBER_OF_UE_MAX][2];   // Nusers times two spatial streams
  // old: LTE_eNB_DLSCH_t  **dlsch_eNB[2];   // Nusers times two spatial streams
  LTE_eNB_ULSCH_t     *ulsch_eNB[NUMBER_OF_UE_MAX+1];      // Nusers + number of RA
  LTE_eNB_DLSCH_t     *dlsch_eNB_SI,*dlsch_eNB_ra;
  LTE_eNB_DLSCH_t     *dlsch_eNB_MCH;
  LTE_eNB_UE_stats     eNB_UE_stats[NUMBER_OF_UE_MAX];
  LTE_eNB_UE_stats    *eNB_UE_stats_ptr[NUMBER_OF_UE_MAX];

  /// cell-specific reference symbols
  uint32_t         lte_gold_table[20][2][14];

  /// UE-specific reference symbols (p=7...14), TM 8/9/10
  uint32_t         lte_gold_uespec_table[2][20][2][21];

  /// mbsfn reference symbols
  uint32_t         lte_gold_mbsfn_table[10][3][42];

  uint32_t X_u[64][839];

  uint8_t pbch_pdu[4]; //PBCH_PDU_SIZE
  char eNB_generate_rar;

  /// Indicator set to 0 after first SR
  uint8_t first_sr[NUMBER_OF_UE_MAX];

  uint32_t max_peak_val;
  int max_eNB_id, max_sync_pos;

  int              N_TA_offset; ///timing offset used in TDD

  /// \brief sinr for all subcarriers of the current link (used only for abstraction).
  /// first index: ? [0..N_RB_DL*12[
  double *sinr_dB;

  /// N0 (used for abstraction)
  double N0;

  unsigned char first_run_timing_advance[NUMBER_OF_UE_MAX];
  unsigned char first_run_I0_measurements;

  unsigned char cooperation_flag; // for cooperative communication

  unsigned char    is_secondary_eNB; // primary by default
  unsigned char    is_init_sync;     /// Flag to tell if initial synchronization is performed. This affects how often the secondary eNB will listen to the PSS from the primary system.
  unsigned char    has_valid_precoder; /// Flag to tell if secondary eNB has channel estimates to create NULL-beams from, and this B/F vector is created.
  unsigned char    PeNB_id;          /// id of Primary eNB
  int              rx_offset;        /// Timing offset (used if is_secondary_eNB)

  /// hold the precoder for NULL beam to the primary user
  int              **dl_precoder_SeNB[3];
  char             log2_maxp; /// holds the maximum channel/precoder coefficient

  /// if ==0 enables phy only test mode
  int mac_enabled;

  /// For emulation only (used by UE abstraction to retrieve DCI)
  uint8_t num_common_dci[2];                         // num_dci in even/odd subframes
  uint8_t num_ue_spec_dci[2];                         // num_dci in even/odd subframes
  DCI_ALLOC_t dci_alloc[2][NUM_DCI_MAX]; // dci_alloc from even/odd subframes


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

  time_stats_t phy_proc;
  time_stats_t phy_proc_tx;
  time_stats_t phy_proc_rx;
  /*
  time_stats_t phy_proc_sf[10]; // for each subframe
  time_stats_t phy_proc_tx_sf[10];
  time_stats_t phy_proc_rx_sf[10];
  */
  time_stats_t rx_prach;

  time_stats_t ofdm_mod_stats;
  time_stats_t dlsch_encoding_stats;
  time_stats_t dlsch_modulation_stats;
  time_stats_t dlsch_scrambling_stats;
  time_stats_t dlsch_rate_matching_stats;
  time_stats_t dlsch_turbo_encoding_stats;
  time_stats_t dlsch_interleaving_stats;

  time_stats_t ofdm_demod_stats;
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
#if ENABLE_RAL
  hash_table_t    *ral_thresholds_timed;
  SLIST_HEAD(ral_thresholds_gen_poll_enb_s, ral_threshold_phy_t) ral_thresholds_gen_polled[RAL_LINK_PARAM_GEN_MAX];
  SLIST_HEAD(ral_thresholds_lte_poll_enb_s, ral_threshold_phy_t) ral_thresholds_lte_polled[RAL_LINK_PARAM_LTE_MAX];
#endif

} PHY_VARS_eNB;

#define debug_msg if (((mac_xface->frame%100) == 0) || (mac_xface->frame < 50)) msg
//#define debug_msg msg

/*
typedef enum {
  max_gain=0,med_gain,byp_gain
} rx_gain_t;
*/

/// Top-level PHY Data Structure for UE
typedef struct {
  /// \brief Module ID indicator for this instance
  uint8_t Mod_id;
  /// \brief Component carrier ID for this PHY instance
  uint8_t CC_id;
  //uint8_t local_flag;
  /// \brief Indicator of current run mode of UE (normal_txrx, rx_calib_ue, no_L2_connect, debug_prach)
  runmode_t mode;
  /// \brief Indicator that UE should perform band scanning
  int UE_scan;
  /// \brief Indicator that UE should perform coarse scanning around carrier
  int UE_scan_carrier;
  /// \brief Indicator that UE is synchronized to an eNB
  int is_synchronized;
  /// \brief Instance count of TX processing thread (-1 means ready, 0 means busy)
  int instance_cnt_tx;
  /// \brief Instance count of RX processing thread (-1 means ready, 0 means busy)
  int instance_cnt_rx;
  /// \brief Instance count of initial synchronization thread (-1 means ready, 0 means busy).
  /// Protected by mutex \ref mutex_synch and condition \ref cond_synch.
  int instance_cnt_synch;
  /// \brief Condition variable for TX processing thread
  pthread_cond_t cond_tx;
  /// \brief Condition variable for RX processing thread
  pthread_cond_t cond_rx;
  /// \brief Condition variable for initial synchronization thread.
  /// The corresponding mutex is \ref mutex_synch.
  pthread_cond_t cond_synch;
  /// \brief Mutex for TX processing thread
  pthread_mutex_t mutex_tx;
  /// \brief Mutex for RX processing thread
  pthread_mutex_t mutex_rx;
  /// \brief Mutex for initial synchronization thread.
  /// Used to protect \ref instance_cnt_synch.
  /// \sa cond_synch
  pthread_mutex_t mutex_synch;
  /// \brief Pthread structure for RX processing thread
  pthread_t       thread_rx;
  /// \brief Pthread structure for TX processing thread
  pthread_t       thread_tx;
  /// \brief Pthread structure to RX processing thread
  pthread_t       thread_synch;
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
  int8_t tx_power_dBm;
  /// \brief Total number of REs in current transmission
  int tx_total_RE;
  /// \brief Maximum transmit power
  int8_t tx_power_max_dBm;
  /// \brief Frame counters for TX and RX processing
  uint32_t frame_rx,frame_tx;
  /// \brief Slot counters for TX and RX processing
  uint32_t slot_tx,slot_rx;
  /// \brief Number of eNB seen by UE
  uint8_t n_connected_eNB;
  /// \brief indicator that Handover procedure has been initiated
  uint8_t ho_initiated;
  /// \brief indicator that Handover procedure has been triggered
  uint8_t ho_triggered;
  /// \brief Measurement variables.
  PHY_MEASUREMENTS PHY_measurements;
  LTE_DL_FRAME_PARMS  lte_frame_parms;
  /// \brief Frame parame before ho used to recover if ho fails.
  LTE_DL_FRAME_PARMS  lte_frame_parms_before_ho;
  LTE_UE_COMMON    lte_ue_common_vars;

  LTE_UE_PDSCH     *lte_ue_pdsch_vars[NUMBER_OF_CONNECTED_eNB_MAX+1];
  LTE_UE_PDSCH_FLP *lte_ue_pdsch_vars_flp[NUMBER_OF_CONNECTED_eNB_MAX+1];
  LTE_UE_PDSCH     *lte_ue_pdsch_vars_SI[NUMBER_OF_CONNECTED_eNB_MAX+1];
  LTE_UE_PDSCH     *lte_ue_pdsch_vars_ra[NUMBER_OF_CONNECTED_eNB_MAX+1];
  LTE_UE_PDSCH     *lte_ue_pdsch_vars_MCH[NUMBER_OF_CONNECTED_eNB_MAX];
  LTE_UE_PBCH      *lte_ue_pbch_vars[NUMBER_OF_CONNECTED_eNB_MAX];
  LTE_UE_PDCCH     *lte_ue_pdcch_vars[NUMBER_OF_CONNECTED_eNB_MAX];
  LTE_UE_PRACH     *lte_ue_prach_vars[NUMBER_OF_CONNECTED_eNB_MAX];
  LTE_UE_DLSCH_t   *dlsch_ue[NUMBER_OF_CONNECTED_eNB_MAX][2];
  LTE_UE_ULSCH_t   *ulsch_ue[NUMBER_OF_CONNECTED_eNB_MAX];
  LTE_UE_DLSCH_t   *dlsch_ue_col[NUMBER_OF_CONNECTED_eNB_MAX][2];
  LTE_UE_DLSCH_t   *ulsch_ue_col[NUMBER_OF_CONNECTED_eNB_MAX];
  LTE_UE_DLSCH_t   *dlsch_ue_SI[NUMBER_OF_CONNECTED_eNB_MAX],*dlsch_ue_ra[NUMBER_OF_CONNECTED_eNB_MAX];
  LTE_UE_DLSCH_t   *dlsch_ue_MCH[NUMBER_OF_CONNECTED_eNB_MAX];
  // This is for SIC in the UE, to store the reencoded data
  LTE_eNB_DLSCH_t  *dlsch_eNB[NUMBER_OF_CONNECTED_eNB_MAX];

  // For abstraction-purposes only
  uint8_t               sr[10];
  uint8_t               pucch_sel[10];
  uint8_t               pucch_payload[22];

  UE_MODE_t        UE_mode[NUMBER_OF_CONNECTED_eNB_MAX];
  /// cell-specific reference symbols
  uint32_t lte_gold_table[7][20][2][14];

  /// ue-specific reference symbols
  uint32_t         lte_gold_uespec_table[2][20][2][21];

  /// mbsfn reference symbols
  uint32_t lte_gold_mbsfn_table[10][3][42];

  uint32_t X_u[64][839];

  uint32_t high_speed_flag;
  uint32_t perfect_ce;
  int16_t ch_est_alpha;
  UE_SCAN_INFO_t scan_info[NB_BANDS_MAX];

  char ulsch_no_allocation_counter[NUMBER_OF_CONNECTED_eNB_MAX];

  unsigned char ulsch_ue_Msg3_active[NUMBER_OF_CONNECTED_eNB_MAX];
  uint32_t  ulsch_ue_Msg3_frame[NUMBER_OF_CONNECTED_eNB_MAX];
  unsigned char ulsch_ue_Msg3_subframe[NUMBER_OF_CONNECTED_eNB_MAX];
  //  unsigned char Msg3_timer[NUMBER_OF_CONNECTED_eNB_MAX];
  //unsigned char *Msg3_ptr[NUMBER_OF_CONNECTED_eNB_MAX];
  PRACH_RESOURCES_t *prach_resources[NUMBER_OF_CONNECTED_eNB_MAX];
  int turbo_iterations, turbo_cntl_iterations;
  /// \brief ?.
  /// - first index: eNB [0..NUMBER_OF_CONNECTED_eNB_MAX[ (hard coded)
  uint32_t total_TBS[NUMBER_OF_CONNECTED_eNB_MAX];
  /// \brief ?.
  /// - first index: eNB [0..NUMBER_OF_CONNECTED_eNB_MAX[ (hard coded)
  uint32_t total_TBS_last[NUMBER_OF_CONNECTED_eNB_MAX];
  /// \brief ?.
  /// - first index: eNB [0..NUMBER_OF_CONNECTED_eNB_MAX[ (hard coded)
  uint32_t bitrate[NUMBER_OF_CONNECTED_eNB_MAX];
  /// \brief ?.
  /// - first index: eNB [0..NUMBER_OF_CONNECTED_eNB_MAX[ (hard coded)
  uint32_t total_received_bits[NUMBER_OF_CONNECTED_eNB_MAX];
  int dlsch_errors[NUMBER_OF_CONNECTED_eNB_MAX];
  int dlsch_errors_last[NUMBER_OF_CONNECTED_eNB_MAX];
  int dlsch_received[NUMBER_OF_CONNECTED_eNB_MAX];
  int dlsch_received_last[NUMBER_OF_CONNECTED_eNB_MAX];
  int dlsch_fer[NUMBER_OF_CONNECTED_eNB_MAX];
  int dlsch_SI_received[NUMBER_OF_CONNECTED_eNB_MAX];
  int dlsch_SI_errors[NUMBER_OF_CONNECTED_eNB_MAX];
  int dlsch_ra_received[NUMBER_OF_CONNECTED_eNB_MAX];
  int dlsch_ra_errors[NUMBER_OF_CONNECTED_eNB_MAX];
  int dlsch_mch_received_sf[MAX_MBSFN_AREA][NUMBER_OF_CONNECTED_eNB_MAX];
  int dlsch_mch_received[NUMBER_OF_CONNECTED_eNB_MAX];
  int dlsch_mcch_received[MAX_MBSFN_AREA][NUMBER_OF_CONNECTED_eNB_MAX];
  int dlsch_mtch_received[MAX_MBSFN_AREA][NUMBER_OF_CONNECTED_eNB_MAX];
  int dlsch_mcch_errors[MAX_MBSFN_AREA][NUMBER_OF_CONNECTED_eNB_MAX];
  int dlsch_mtch_errors[MAX_MBSFN_AREA][NUMBER_OF_CONNECTED_eNB_MAX];
  int dlsch_mcch_trials[MAX_MBSFN_AREA][NUMBER_OF_CONNECTED_eNB_MAX];
  int dlsch_mtch_trials[MAX_MBSFN_AREA][NUMBER_OF_CONNECTED_eNB_MAX];
  int current_dlsch_cqi[NUMBER_OF_CONNECTED_eNB_MAX];
  unsigned char first_run_timing_advance[NUMBER_OF_CONNECTED_eNB_MAX];
  uint8_t               generate_prach;
  uint8_t               prach_cnt;
  uint8_t               prach_PreambleIndex;
  //  uint8_t               prach_timer;
  int              rx_offset; /// Timing offset
  int              timing_advance; ///timing advance signalled from eNB
  int              N_TA_offset; ///timing offset used in TDD
  /// Flag to tell if UE is secondary user (cognitive mode)
  unsigned char    is_secondary_ue;
  /// Flag to tell if secondary eNB has channel estimates to create NULL-beams from.
  unsigned char    has_valid_precoder;
  /// hold the precoder for NULL beam to the primary eNB
  int              **ul_precoder_S_UE;
  /// holds the maximum channel/precoder coefficient
  char             log2_maxp;

  /// if ==0 enables phy only test mode
  int mac_enabled;

  /// Flag to initialize averaging of PHY measurements
  int init_averaging;

  /// \brief sinr for all subcarriers of the current link (used only for abstraction).
  /// - first index: ? [0..12*N_RB_DL[
  double *sinr_dB;

  /// \brief sinr for all subcarriers of first symbol for the CQI Calculation.
  /// - first index: ? [0..12*N_RB_DL[
  double *sinr_CQI_dB;

  /// sinr_effective used for CQI calulcation
  double sinr_eff;

  /// N0 (used for abstraction)
  double N0;

  /// PDSCH Varaibles
  PDSCH_CONFIG_DEDICATED pdsch_config_dedicated[NUMBER_OF_CONNECTED_eNB_MAX];

  /// PUSCH Varaibles
  PUSCH_CONFIG_DEDICATED pusch_config_dedicated[NUMBER_OF_CONNECTED_eNB_MAX];

  /// PUSCH contention-based access vars
  PUSCH_CA_CONFIG_DEDICATED  pusch_ca_config_dedicated[NUMBER_OF_eNB_MAX]; // lola

  /// PUCCH variables

  PUCCH_CONFIG_DEDICATED pucch_config_dedicated[NUMBER_OF_CONNECTED_eNB_MAX];

  uint8_t ncs_cell[20][7];

  /// UL-POWER-Control
  UL_POWER_CONTROL_DEDICATED ul_power_control_dedicated[NUMBER_OF_CONNECTED_eNB_MAX];

  /// TPC
  TPC_PDCCH_CONFIG tpc_pdcch_config_pucch[NUMBER_OF_CONNECTED_eNB_MAX];
  TPC_PDCCH_CONFIG tpc_pdcch_config_pusch[NUMBER_OF_CONNECTED_eNB_MAX];

  /// CQI reporting
  CQI_REPORT_CONFIG cqi_report_config[NUMBER_OF_CONNECTED_eNB_MAX];

  /// SRS Variables
  SOUNDINGRS_UL_CONFIG_DEDICATED soundingrs_ul_config_dedicated[NUMBER_OF_CONNECTED_eNB_MAX];

  /// Scheduling Request Config
  SCHEDULING_REQUEST_CONFIG scheduling_request_config[NUMBER_OF_CONNECTED_eNB_MAX];

  /// Transmission mode per eNB
  uint8_t transmission_mode[NUMBER_OF_CONNECTED_eNB_MAX];

  time_stats_t phy_proc;
  time_stats_t phy_proc_tx;
  time_stats_t phy_proc_rx;

  time_stats_t ofdm_mod_stats;
  time_stats_t ulsch_encoding_stats;
  time_stats_t ulsch_modulation_stats;
  time_stats_t ulsch_segmentation_stats;
  time_stats_t ulsch_rate_matching_stats;
  time_stats_t ulsch_turbo_encoding_stats;
  time_stats_t ulsch_interleaving_stats;
  time_stats_t ulsch_multiplexing_stats;

  time_stats_t ofdm_demod_stats;
  time_stats_t dlsch_rx_pdcch_stats;
  time_stats_t rx_dft_stats;
  time_stats_t dlsch_channel_estimation_stats;
  time_stats_t dlsch_freq_offset_estimation_stats;
  time_stats_t dlsch_decoding_stats;
  time_stats_t dlsch_demodulation_stats;
  time_stats_t dlsch_rate_unmatching_stats;
  time_stats_t dlsch_turbo_decoding_stats;
  time_stats_t dlsch_deinterleaving_stats;
  time_stats_t dlsch_llr_stats;
  time_stats_t dlsch_unscrambling_stats;
  time_stats_t dlsch_rate_matching_stats;
  time_stats_t dlsch_turbo_encoding_stats;
  time_stats_t dlsch_interleaving_stats;
  time_stats_t dlsch_tc_init_stats;
  time_stats_t dlsch_tc_alpha_stats;
  time_stats_t dlsch_tc_beta_stats;
  time_stats_t dlsch_tc_gamma_stats;
  time_stats_t dlsch_tc_ext_stats;
  time_stats_t dlsch_tc_intl1_stats;
  time_stats_t dlsch_tc_intl2_stats;
  time_stats_t tx_prach;

#if ENABLE_RAL
  hash_table_t    *ral_thresholds_timed;
  SLIST_HEAD(ral_thresholds_gen_poll_s, ral_threshold_phy_t) ral_thresholds_gen_polled[RAL_LINK_PARAM_GEN_MAX];
  SLIST_HEAD(ral_thresholds_lte_poll_s, ral_threshold_phy_t) ral_thresholds_lte_polled[RAL_LINK_PARAM_LTE_MAX];
#endif
} PHY_VARS_UE;

/// Top-level PHY Data Structure for RN
typedef struct {
  /// Module ID indicator for this instance
  uint8_t Mod_id;
  uint32_t frame;
  // phy_vars_eNB
  // phy_vars ue
  // cuurently only used to store and forward the PMCH
  uint8_t mch_avtive[10];
  uint8_t sync_area[10]; // num SF
  LTE_UE_DLSCH_t   *dlsch_rn_MCH[10];

} PHY_VARS_RN;

#include "PHY/INIT/defs.h"
#include "PHY/LTE_REFSIG/defs.h"
#include "PHY/MODULATION/defs.h"
#include "PHY/LTE_TRANSPORT/proto.h"
#include "PHY/LTE_ESTIMATION/defs.h"

#include "SIMULATION/ETH_TRANSPORT/defs.h"
#endif //OPENAIR_LTE


#endif //  __PHY_DEFS__H__






