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

/*! \file PHY/defs_RU.h
 \brief Top-level defines and structure definitions
 \author R. Knopp, F. Kaltenberger
 \date 2018
 \version 0.1
 \company Eurecom
 \email: knopp@eurecom.fr,florian.kaltenberger@eurecom.fr
 \note
 \warning
*/

#ifndef __PHY_DEFS_RU__H__
#define __PHY_DEFS_RU__H__


#include "common_lib.h"
#include "openairinterface5g_limits.h"
#include "PHY/TOOLS/time_meas.h"
#include "defs_common.h"
#include "nfapi_nr_interface_scf.h"

#define MAX_BANDS_PER_RRU 4
#define MAX_RRU_CONFIG_SIZE 1024



typedef enum {
  normal_txrx=0,
  rx_calib_ue=1,
  rx_calib_ue_med=2,
  rx_calib_ue_byp=3,
  debug_prach=4,
  no_L2_connect=5,
  calib_prach_tx=6,
  rx_dump_frame=7,
  loop_through_memory=8
} runmode_t;

/*! \brief Extension Type */
typedef enum {
  CYCLIC_PREFIX,
  CYCLIC_SUFFIX,
  ZEROS,
  NONE
} Extension_t;

enum transmission_access_mode {
  NO_ACCESS=0,
  POSTPONED_ACCESS,
  CANCELED_ACCESS,
  UNKNOWN_ACCESS,
  SCHEDULED_ACCESS,
  CBA_ACCESS
};

typedef enum {
  eNodeB_3GPP=0,   // classical eNodeB function
  NGFI_RAU_IF5,    // RAU with NGFI IF5
  NGFI_RAU_IF4p5,  // RAU with NFGI IF4p5
  NGFI_RRU_IF5,    // NGFI_RRU (NGFI remote radio-unit,IF5)
  NGFI_RRU_IF4p5,  // NGFI_RRU (NGFI remote radio-unit,IF4p5)
  MBP_RRU_IF5,      // Mobipass RRU
  gNodeB_3GPP
} node_function_t;

typedef enum {
  synch_to_ext_device=0,  // synch to RF or Ethernet device
  synch_to_other,          // synch to another source_(timer, other RU)
  synch_to_mobipass_standalone  // special case for mobipass in standalone mode
} node_timing_t;


typedef struct {
  /// \brief Holds the transmit data in the frequency domain (1 frame).
  /// - first index: rx antenna [0..nb_antennas_rx[
  /// - second index: ? [0..samples_per_frame[
  int32_t **txdata;
  /// \brief holds the transmit data after beamforming in the frequency domain (1 slot).
  /// - first index: tx antenna [0..nb_antennas_tx[
  /// - second index: sample [0..samples_per_slot_woCP]
  int32_t **txdataF_BF;
  /// \brief holds the transmit data before beamforming in the frequency domain (1 frame).
  /// - first index: tx antenna [0..nb_antenna_ports[
  /// - second index: sample [0..samples_per_frame_woCP]
  int32_t **txdataF;
  /// \brief holds the transmit data before beamforming for epdcch/mpdcch
  /// - first index : tx antenna [0..nb_epdcch_antenna_ports[
  /// - second index: sampl [0..]
  int32_t **txdataF_epdcch;
  /// \brief Holds the receive data in the frequency domain.
  /// - first index: rx antenna [0..nb_antennas_rx[
  /// - second index: ? [0..2*ofdm_symbol_size*frame_parms->symbols_per_tti[
  int32_t **rxdata;
  /// \brief Holds the last subframe of received data in time domain after removal of 7.5kHz frequency offset.
  /// - first index: rx antenna [0..nb_antennas_rx[
  /// - second index: sample [0..samples_per_tti[
  int32_t **rxdata_7_5kHz;
  /// \brief Holds the received data in the frequency domain.
  /// - first index: rx antenna [0..nb_antennas_rx[
  /// - second index: ? [0..2*ofdm_symbol_size*frame_parms->symbols_per_tti[
  int32_t **rxdataF;
  /// \brief Holds output of the sync correlator.
  /// - first index: sample [0..samples_per_tti*10[
  uint32_t *sync_corr;
  /// \brief Holds the tdd reciprocity calibration coefficients
  /// - first index: eNB id [0..2] (hard coded)
  /// - second index: tx antenna [0..nb_antennas_tx[
  /// - third index: frequency [0..]
  int32_t **tdd_calib_coeffs;
  /// \brief Anaglogue beam ID for each OFDM symbol (used when beamforming not done in RU)
  /// - first index: antenna port
  /// - second index: beam_id [0.. symbols_per_frame[
  uint8_t **beam_id;
} RU_COMMON;


typedef struct {
  /// \brief Received frequency-domain signal after extraction.
  /// - first index: ? [0..7] (hard coded) FIXME! accessed via \c nb_antennas_rx
  /// - second index: ? [0..168*N_RB_DL[
  int32_t **rxdataF_ext;
  /// \brief Hold the channel estimates in time domain based on DRS.
  /// - first index: rx antenna id [0..nb_antennas_rx[
  /// - second index: ? [0..4*ofdm_symbol_size[
  int32_t **drs_ch_estimates_time;
  /// \brief Hold the channel estimates in frequency domain based on DRS.
  /// - first index: rx antenna id [0..nb_antennas_rx[
  /// - second index: ? [0..12*N_RB_UL*frame_parms->symbols_per_tti[
  int32_t **drs_ch_estimates;
} RU_CALIBRATION;


typedef struct RU_prec_t_s{
  /// \internal This variable is protected by \ref mutex_feptx_prec
  int instance_cnt_feptx_prec;
  /// pthread struct for RU TX FEP PREC worker thread
  pthread_t pthread_feptx_prec;
  /// pthread attributes for worker feptx prec thread
  pthread_attr_t attr_feptx_prec;
  /// condition varible for RU TX FEP PREC thread
  pthread_cond_t cond_feptx_prec;
  /// mutex for fep PREC TX worker thread
  pthread_mutex_t mutex_feptx_prec;
  int symbol;
  int p;//logical
  int aa;//physical MAX nb_tx
  struct RU_t_s *ru;
  int index;
} RU_prec_t;

typedef struct RU_feptx_t_s{
  /// \internal This variable is protected by \ref mutex_feptx_prec
  int instance_cnt_feptx;
  /// pthread struct for RU TX FEP PREC worker thread
  pthread_t pthread_feptx;
  /// pthread attributes for worker feptx prec thread
  pthread_attr_t attr_feptx;
  /// condition varible for RU TX FEP PREC thread
  pthread_cond_t cond_feptx;
  /// mutex for fep PREC TX worker thread
  pthread_mutex_t mutex_feptx;
  struct RU_t_s *ru;
  int aa;//physical MAX nb_tx
  int half_slot;//first or second half of a slot
  int slot;//current slot
  int symbol;//current symbol
  int nb_antenna_ports;//number of logical port
  int index;
}RU_feptx_t;

typedef struct {
  int frame;
  int slot;
  int fmt;
  int numRA;
  int prachStartSymbol;
  int num_prach_ocas;
} RU_PRACH_list_t;

#define NUMBER_OF_NR_RU_PRACH_MAX 8
#define NUMBER_OF_NR_RU_PRACH_OCCASIONS_MAX 12

typedef struct RU_proc_t_s {
  /// Pointer to associated RU descriptor
  struct RU_t_s *ru;
  /// timestamp received from HW
  openair0_timestamp timestamp_rx;
  /// timestamp to send to "slave rru"
  openair0_timestamp timestamp_tx;
  /// subframe (LTE) / slot (NR) to act upon for reception
  int tti_rx;
  /// subframe (LTE) / slot (NR) to act upon for transmission
  int tti_tx;
  /// slot to pass to feptx worker thread
  int slot_feptx;
  /// subframe to act upon for reception of prach
  int subframe_prach;
  /// subframe to act upon for reception of prach BL/CE UEs
  int subframe_prach_br;
  /// frame to act upon for reception
  int frame_rx;
  /// frame to act upon for transmission
  int frame_tx;
  /// unwrapped frame count
  int frame_tx_unwrap;
  /// frame to act upon for reception of prach
  int frame_prach;
  /// frame to act upon for reception of prach
  int frame_prach_br;
  /// frame offset for slave RUs (to correct for frame asynchronism at startup)
  int frame_offset;
  /// \brief Instance count for FH processing thread.
  /// \internal This variable is protected by \ref mutex_FH.
  int instance_cnt_FH;
  int instance_cnt_FH1;
  /// \internal This variable is protected by \ref mutex_prach.
  int instance_cnt_prach;
  /// \internal This variable is protected by \ref mutex_prach.
  int instance_cnt_prach_br;
  /// \internal This variable is protected by \ref mutex_synch.
  int instance_cnt_synch;
  /// \internal This variable is protected by \ref mutex_eNBs.
  int instance_cnt_eNBs;
  int instance_cnt_gNBs;
  /// \brief Instance count for rx processing thread.
  /// \internal This variable is protected by \ref mutex_asynch_rxtx.
  int instance_cnt_asynch_rxtx;
  /// \internal This variable is protected by \ref mutex_fep
  int instance_cnt_fep;
  /// \internal This variable is protected by \ref mutex_feptx
  int instance_cnt_feptx;
  /// \internal This variable is protected by \ref mutex_ru_thread
  int instance_cnt_ru;
  /// This varible is protected by \ref mutex_emulatedRF
  int instance_cnt_emulateRF;
  /// pthread structure for RU FH processing thread
  pthread_t pthread_FH;
  pthread_t pthread_FH1;
  /// pthread structure for RU control thread
  pthread_t pthread_ctrl;
  /// pthread structure for RU prach processing thread
  pthread_t pthread_prach;
  /// pthread structure for RU prach processing thread BL/CE UEs
  pthread_t pthread_prach_br;
  /// pthread struct for RU synch thread
  pthread_t pthread_synch;
  /// pthread struct for RU RX FEP worker thread
  pthread_t pthread_fep;
  /// pthread struct for RU TX FEP worker thread
  pthread_t pthread_feptx;
  /// pthread struct for emulated RF
  pthread_t pthread_emulateRF;
  /// pthread structure for asychronous RX/TX processing thread
  pthread_t pthread_asynch_rxtx;
  /// flag to indicate first RX acquisition
  int first_rx;
  /// flag to indicate first TX transmission
  int first_tx;
  /// pthread attributes for RU FH processing thread
  pthread_attr_t attr_FH;
  pthread_attr_t attr_FH1;
  /// pthread attributes for RU control thread
  pthread_attr_t attr_ctrl;
  /// pthread attributes for RU prach
  pthread_attr_t attr_prach;
  /// pthread attributes for RU prach BL/CE UEs
  pthread_attr_t attr_prach_br;
  /// pthread attributes for RU synch thread
  pthread_attr_t attr_synch;
  /// pthread attributes for asynchronous RX thread
  pthread_attr_t attr_asynch_rxtx;
  /// pthread attributes for worker fep thread
  pthread_attr_t attr_fep;
  /// pthread attributes for worker feptx thread
  pthread_attr_t attr_feptx;
  /// pthread attributes for emulated RF
  pthread_attr_t attr_emulateRF;
  /// scheduling parameters for RU FH thread
  struct sched_param sched_param_FH;
  struct sched_param sched_param_FH1;
  /// scheduling parameters for RU prach thread
  struct sched_param sched_param_prach;
  /// scheduling parameters for RU prach thread BL/CE UEs
  struct sched_param sched_param_prach_br;
  /// scheduling parameters for RU synch thread
  struct sched_param sched_param_synch;
  /// scheduling parameters for asynch_rxtx thread
  struct sched_param sched_param_asynch_rxtx;
  /// condition variable for RU FH thread
  pthread_cond_t cond_FH;
  pthread_cond_t cond_FH1;
  /// condition variable for RU prach thread
  pthread_cond_t cond_prach;
  /// condition variable for RU prach thread BL/CE UEs
  pthread_cond_t cond_prach_br;
  /// condition variable for RU synch thread
  pthread_cond_t cond_synch;
  /// condition variable for asynch RX/TX thread
  pthread_cond_t cond_asynch_rxtx;
  /// condition varible for RU RX FEP thread
  pthread_cond_t cond_fep;
  /// condition varible for RU TX FEP thread
  pthread_cond_t cond_feptx;
  /// condition varible for emulated RF
  pthread_cond_t cond_emulateRF;
  /// condition variable for eNB signal
  pthread_cond_t cond_eNBs;
  /// condition variable for gNB signal
  pthread_cond_t cond_gNBs;
  /// condition variable for ru_thread
  pthread_cond_t cond_ru_thread;
  /// mutex for RU FH
  pthread_mutex_t mutex_FH;
  pthread_mutex_t mutex_FH1;
  /// mutex for RU prach
  pthread_mutex_t mutex_prach;
  /// mutex for RU prach BL/CE UEs
  pthread_mutex_t mutex_prach_br;
  /// mutex for RU synch
  pthread_mutex_t mutex_synch;
  /// mutex for eNB signal
  pthread_mutex_t mutex_eNBs;
  /// mutex for eNB signal
  pthread_mutex_t mutex_gNBs;
  /// mutex for asynch RX/TX thread
  pthread_mutex_t mutex_asynch_rxtx;
  /// mutex for fep RX worker thread
  pthread_mutex_t mutex_fep;
  /// mutex for fep TX worker thread
  pthread_mutex_t mutex_feptx;
  /// mutex for ru_thread
  pthread_mutex_t mutex_ru;
  /// mutex for emulated RF thread
  pthread_mutex_t mutex_emulateRF;
  /// symbol mask for IF4p5 reception per subframe
  uint32_t symbol_mask[10];
  /// time measurements for each subframe
  struct timespec t[10];
  /// number of slave threads
  int num_slaves;
  /// array of pointers to slaves
  struct RU_proc_t_s **slave_proc;
#ifdef PHY_TX_THREAD
  /// pthread structure for PRACH thread
  pthread_t pthread_phy_tx;
  pthread_mutex_t mutex_phy_tx;
  pthread_cond_t cond_phy_tx;
  /// \internal This variable is protected by \ref mutex_phy_tx.
  int instance_cnt_phy_tx;
  /// frame to act upon for transmission
  int frame_phy_tx;
  /// subframe to act upon for transmission
  int subframe_phy_tx;
  /// timestamp to send to "slave rru"
  openair0_timestamp timestamp_phy_tx;
  /// pthread structure for RF TX thread
  pthread_t pthread_rf_tx;
  pthread_mutex_t mutex_rf_tx;
  pthread_cond_t cond_rf_tx;
  /// \internal This variable is protected by \ref mutex_rf_tx.
  int instance_cnt_rf_tx;
#endif
#if defined(PRE_SCD_THREAD)
  pthread_t pthread_pre_scd;
  /// condition variable for time processing thread
  pthread_cond_t cond_pre_scd;
  /// mutex for time thread
  pthread_mutex_t mutex_pre_scd;
  int instance_pre_scd;
#endif
  /// pipeline ready state
  int ru_rx_ready;
  int ru_tx_ready;
  int emulate_rf_busy;

  /// structure for precoding thread
  RU_prec_t prec[16];
  /// structure for feptx thread
  RU_feptx_t feptx[16];
  /// mask for checking process finished
  int feptx_mask;
} RU_proc_t;

typedef enum {
  LOCAL_RF        =0,
  REMOTE_IF5      =1,
  REMOTE_MBP_IF5  =2,
  REMOTE_IF4p5    =3,
  REMOTE_IF1pp    =4,
  MAX_RU_IF_TYPES =5
                   //EMULATE_RF      =6
} RU_if_south_t;


typedef enum {
  RU_IDLE       = 0,
  RU_CONFIG     = 1,
  RU_READY      = 2,
  RU_RUN        = 3,
  RU_ERROR      = 4,
  RU_SYNC       = 5,
  RU_CHECK_SYNC = 6
} rru_state_t;


/// Some commands to RRU. Not sure we should do it like this !
typedef enum {
  EMPTY            = 0,
  STOP_RU          = 1,
  RU_FRAME_RESYNCH = 2,
  WAIT_RESYNCH     = 3
} rru_cmd_t;


typedef struct RU_t_s {
  /// index of this ru
  uint32_t idx;
  /// pointer to first RU
  struct RU_t_s *ru0;
  /// pointer to ru_mask
  uint64_t *ru_mask;
  /// pointer to ru_mutex
  pthread_mutex_t *ru_mutex;
  /// pointer to ru_cond
  pthread_cond_t *ru_cond;
  /// Pointer to configuration file
  char *rf_config_file;
  /// southbound interface
  RU_if_south_t if_south;
  /// timing
  node_timing_t if_timing;
  /// function
  node_function_t function;
  /// Ethernet parameters for fronthaul interface
  eth_params_t eth_params;
  /// flag to indicate RF emulation mode
  int emulate_rf;
  /// numerology index
  int numerology;
  /// flag to indicate basicsim operation
  int basicsim;
  /// flag to indicate the RU is in sync with a master reference
  int in_synch;
  /// timing offset
  int rx_offset;
  /// south in counter
  int south_in_cnt;
  /// south out counter
  int south_out_cnt;
  /// north in counter
  int north_in_cnt;
  /// north out counter
  int north_out_cnt;
  /// flag to indicate the RU is a slave to another source
  int is_slave;
  /// flag to indicate if the RU has to perform OTA sync
  int ota_sync_enable;
  /// flag to indicate that the RU should generate the DMRS sequence in slot 2 (subframe 1) for OTA synchronization and calibration
  int generate_dmrs_sync;
  /// flag to indicate if the RU has a control channel
  int has_ctrl_prt;
  /// counter to delay start of processing of RU until HW settles
  int wait_cnt;
  /// counter to delay start of slave RUs until stable synchronization
  int wait_check;
  /// Total gain of receive chain
  uint32_t rx_total_gain_dB;
  /// number of bands that this device can support
  int num_bands;
  /// band list
  int band[MAX_BANDS_PER_RRU];
  /// number of RX paths on device
  int nb_rx;
  /// number of TX paths on device
  int nb_tx;
  /// number of logical antennas at TX beamformer input
  int nb_log_antennas;
  /// maximum PDSCH RS EPRE
  int max_pdschReferenceSignalPower;
  /// maximum RX gain
  int max_rxgain;
  /// Attenuation of RX paths on device
  int att_rx;
  /// Attenuation of TX paths on device
  int att_tx;
  /// flag to indicate precoding operation in RU
  int do_precoding;
  /// FAPI confiuration
  nfapi_nr_config_request_scf_t  config;
  /// Frame parameters
  struct LTE_DL_FRAME_PARMS *frame_parms;
  struct NR_DL_FRAME_PARMS *nr_frame_parms;
  ///timing offset used in TDD
  int N_TA_offset;
  /// SF extension used in TDD (unit: number of samples at 30.72MHz) (this is an expert option)
  int sf_extension;
  /// "end of burst delay" used in TDD (unit: number of samples at 30.72MHz) (this is an expert option)
  int end_of_burst_delay;
  /// RF device descriptor
  openair0_device rfdevice;
  /// HW configuration
  openair0_config_t openair0_cfg;
  /// Number of NBs using this RU
  int num_eNB;
  int num_gNB;
  /// list of NBs using this RU
  struct PHY_VARS_eNB_s *eNB_list[NUMBER_OF_eNB_MAX];
  struct PHY_VARS_gNB_s *gNB_list[NUMBER_OF_gNB_MAX];
  /// Mapping of antenna ports to RF chain index
  openair0_rf_map rf_map;
  /// IF device descriptor
  openair0_device ifdevice;
  /// Pointer for ifdevice buffer struct
  if_buffer_t ifbuffer;
  /// if prach processing is to be performed in RU
  int do_prach;
  /// function pointer to synchronous RX fronthaul function (RRU,3GPP_eNB/3GPP_gNB)
  void (*fh_south_in)(struct RU_t_s *ru, int *frame, int *subframe);
  /// function pointer to synchronous TX fronthaul function
  void (*fh_south_out)(struct RU_t_s *ru, int frame_tx, int tti_tx, uint64_t timestamp_tx);
  /// function pointer to synchronous RX fronthaul function (RRU)
  void (*fh_north_in)(struct RU_t_s *ru, int *frame, int *subframe);
  /// function pointer to synchronous RX fronthaul function (RRU)
  void (*fh_north_out)(struct RU_t_s *ru);
  /// function pointer to asynchronous fronthaul interface
  void (*fh_north_asynch_in)(struct RU_t_s *ru, int *frame, int *subframe);
  /// function pointer to asynchronous fronthaul interface
  void (*fh_south_asynch_in)(struct RU_t_s *ru, int *frame, int *subframe);
  /// function pointer to initialization function for radio interface
  int (*start_rf)(struct RU_t_s *ru);
  /// function pointer to release function for radio interface
  int (*stop_rf)(struct RU_t_s *ru);
  /// function pointer to initialization function for radio interface
  int (*start_if)(struct RU_t_s *ru, struct PHY_VARS_eNB_s *eNB);
  int (*nr_start_if)(struct RU_t_s *ru, struct PHY_VARS_gNB_s *gNB);
  /// function pointer to RX front-end processing routine (DFTs/prefix removal or NULL)
  void (*feprx)(struct RU_t_s *ru, int subframe);
  /// function pointer to TX front-end processing routine (IDFTs and prefix removal or NULL)
  void (*feptx_ofdm)(struct RU_t_s *ru, int frame_tx, int tti_tx);
  /// function pointer to TX front-end processing routine (PRECODING)
  void (*feptx_prec)(struct RU_t_s *ru, int frame_tx, int tti_tx);
  /// function pointer to wakeup routine in lte-enb/nr-gnb.
  int (*wakeup_rxtx)(struct PHY_VARS_eNB_s *eNB, struct RU_t_s *ru);
  int (*nr_wakeup_rxtx)(struct PHY_VARS_gNB_s *gNB, struct RU_t_s *ru);
  /// function pointer to wakeup routine in lte-enb/nr-gnb.
  void (*wakeup_prach_eNB)(struct PHY_VARS_eNB_s *eNB, struct RU_t_s *ru, int frame, int subframe);
  void (*wakeup_prach_gNB)(struct PHY_VARS_gNB_s *gNB, struct RU_t_s *ru, int frame, int subframe);
  /// function pointer to wakeup routine in lte-enb.
  void (*wakeup_prach_eNB_br)(struct PHY_VARS_eNB_s *eNB, struct RU_t_s *ru, int frame, int subframe);
  /// function pointer to start a thread of tx write for USRP.
  int (*start_write_thread)(struct RU_t_s *ru);

  /// function pointer to NB entry routine
  void (*eNB_top)(struct PHY_VARS_eNB_s *eNB, int frame_rx, int subframe_rx, char *string, struct RU_t_s *ru);
  void (*gNB_top)(struct PHY_VARS_gNB_s *gNB, int frame_rx, int slot_rx, char *string, struct RU_t_s *ru);

  /// Timing data copy statistics (TX)
  time_stats_t txdataF_copy_stats;
  /// Timing statistics (TX)
  time_stats_t precoding_stats;
  /// Timing statistics
  time_stats_t ofdm_demod_stats;
  /// Timing statistics (TX)
  time_stats_t ofdm_mod_stats;
  /// Timing statistics (TX)
  time_stats_t ofdm_total_stats;
  /// Timing wait statistics
  time_stats_t ofdm_demod_wait_stats;
  /// Timing wakeup statistics
  time_stats_t ofdm_demod_wakeup_stats;
  /// Timing wait statistics (TX)
  time_stats_t ofdm_mod_wait_stats;
  /// Timing wakeup statistics (TX)
  time_stats_t ofdm_mod_wakeup_stats;
  /// Timing statistics (RX Fronthaul + Compression)
  time_stats_t rx_fhaul;
  /// Timing statistics (TX Fronthaul + Compression)
  time_stats_t tx_fhaul;
  /// Timing statistics (Compression)
  time_stats_t compression;
  /// Timing statistics (Fronthaul transport)
  time_stats_t transport;
  /// RX and TX buffers for precoder output
  RU_COMMON common;
  RU_CALIBRATION calibration;
  /// beamforming weight list size
  int nb_bfw;
  /// beamforming weight list of values
  int32_t *bw_list[NUMBER_OF_eNB_MAX+1];
  /// beamforming weight vectors
  int32_t **beam_weights[NUMBER_OF_eNB_MAX+1][15];
  /// prach commands
  RU_PRACH_list_t prach_list[NUMBER_OF_NR_RU_PRACH_MAX];
  /// mutex for prach_list access
  pthread_mutex_t prach_list_mutex;
  /// received frequency-domain signal for PRACH (IF4p5 RRU) 
  int16_t **prach_rxsigF[NUMBER_OF_NR_RU_PRACH_OCCASIONS_MAX];
  /// received frequency-domain signal for PRACH BR (IF4p5 RRU)
  int16_t **prach_rxsigF_br[4];
  /// sequence number for IF5
  uint8_t seqno;
  /// initial timestamp used as an offset make first real timestamp 0
  openair0_timestamp ts_offset;
  /// Current state of the RU
  rru_state_t state;
  /// Command to do
  rru_cmd_t cmd;
  /// value to be passed using command
  uint16_t cmdval;
  /// process scheduling variables
  RU_proc_t proc;
  /// stats thread pthread descriptor
  pthread_t ru_stats_thread;
  /// OTA synchronization signal
  int16_t *dmrssync;
  /// OTA synchronization correlator output
  uint64_t *dmrs_corr;
  /// sleep time in us for delaying L1 wakeup
  int wakeup_L1_sleeptime;
  /// maximum number of sleeps
  int wakeup_L1_sleep_cnt_max;
  /// DL IF frequency in Hz
  uint64_t if_frequency;
  /// UL IF frequency offset to DL IF frequency in Hz
  int if_freq_offset;
} RU_t;


typedef enum {
  RAU_tick=0,
  RRU_capabilities=1,
  RRU_config=2,
  RRU_config_ok=3,
  RRU_start=4,
  RRU_stop=5,
  RRU_sync_ok=6,
  RRU_frame_resynch=7,
  RRU_MSG_max_num=8,
  RRU_check_sync = 9,
  RRU_config_update=10,
  RRU_config_update_ok=11
} rru_config_msg_type_t;


typedef struct RRU_CONFIG_msg_s {
  rru_config_msg_type_t type;
  ssize_t len;
  uint8_t msg[MAX_RRU_CONFIG_SIZE];
} RRU_CONFIG_msg_t;


typedef enum {
  OAI_IF5_only      =0,
  OAI_IF4p5_only    =1,
  OAI_IF5_and_IF4p5 =2,
  MBP_IF5           =3,
  MAX_FH_FMTs       =4
} FH_fmt_options_t;


typedef struct RRU_capabilities_s {
  /// Fronthaul format
  FH_fmt_options_t FH_fmt;
  /// number of EUTRA bands (<=4) supported by RRU
  uint8_t          num_bands;
  /// EUTRA band list supported by RRU
  uint8_t          band_list[MAX_BANDS_PER_RRU];
  /// Number of concurrent bands (component carriers)
  uint8_t          num_concurrent_bands;
  /// Maximum TX EPRE of each band
  int8_t           max_pdschReferenceSignalPower[MAX_BANDS_PER_RRU];
  /// Maximum RX gain of each band
  uint8_t          max_rxgain[MAX_BANDS_PER_RRU];
  /// Number of RX ports of each band
  uint8_t          nb_rx[MAX_BANDS_PER_RRU];
  /// Number of TX ports of each band
  uint8_t          nb_tx[MAX_BANDS_PER_RRU];
  /// max DL bandwidth (1,6,15,25,50,75,100)
  uint8_t          N_RB_DL[MAX_BANDS_PER_RRU];
  /// max UL bandwidth (1,6,15,25,50,75,100)
  uint8_t          N_RB_UL[MAX_BANDS_PER_RRU];
} RRU_capabilities_t;


typedef struct RRU_config_s {
  /// Fronthaul format
  RU_if_south_t FH_fmt;
  /// number of EUTRA bands (<=4) configured in RRU
  uint8_t num_bands;
  /// EUTRA band list configured in RRU
  uint8_t band_list[MAX_BANDS_PER_RRU];
  /// TDD configuration (0-6)
  uint8_t tdd_config[MAX_BANDS_PER_RRU];
  /// TDD special subframe configuration (0-10)
  uint8_t tdd_config_S[MAX_BANDS_PER_RRU];
  /// TX frequency
  uint32_t tx_freq[MAX_BANDS_PER_RRU];
  /// RX frequency
  uint32_t rx_freq[MAX_BANDS_PER_RRU];
  /// TX attenuation w.r.t. max
  uint8_t att_tx[MAX_BANDS_PER_RRU];
  /// RX attenuation w.r.t. max
  uint8_t att_rx[MAX_BANDS_PER_RRU];
  /// DL bandwidth
  uint8_t N_RB_DL[MAX_BANDS_PER_RRU];
  /// UL bandwidth
  uint8_t N_RB_UL[MAX_BANDS_PER_RRU];
  /// 3/4 sampling rate
  uint8_t threequarter_fs[MAX_BANDS_PER_RRU];
  /// prach_FreqOffset for IF4p5
  int prach_FreqOffset[MAX_BANDS_PER_RRU];
  /// prach_ConfigIndex for IF4p5
  int prach_ConfigIndex[MAX_BANDS_PER_RRU];
  int emtc_prach_CElevel_enable[MAX_BANDS_PER_RRU][4];
  /// emtc_prach_FreqOffset for IF4p5 per CE Level
  int emtc_prach_FreqOffset[MAX_BANDS_PER_RRU][4];
  /// emtc_prach_ConfigIndex for IF4p5 per CE Level
  int emtc_prach_ConfigIndex[MAX_BANDS_PER_RRU][4];
  /// mutex for async RX/TX thread
  pthread_mutex_t mutex_asynch_rxtx;
  /// mutex for RU access to eNB processing (PDSCH/PUSCH)
  pthread_mutex_t mutex_RU;
  /// mutex for RU access to eNB processing (PRACH)
  pthread_mutex_t mutex_RU_PRACH;
  /// mutex for RU access to eNB processing (PRACH BR)
  pthread_mutex_t mutex_RU_PRACH_br;
  /// mask for RUs serving eNB (PDSCH/PUSCH)
  int RU_mask[10];
  /// time measurements for RU arrivals
  struct timespec t[10];
  /// Timing statistics (RU_arrivals)
  time_stats_t ru_arrival_time;
  /// mask for RUs serving eNB (PRACH)
  int RU_mask_prach;
  /// embms mbsfn sf config
  int num_MBSFN_config;
  /// embms mbsfn sf config
  MBSFN_config_t MBSFN_config[8];
} RRU_config_t;

typedef struct processingData_RU {
  int frame_tx;
  int slot_tx;
  openair0_timestamp timestamp_tx;
  RU_t *ru;
} processingData_RU_t;
#endif //__PHY_DEFS_RU__H__
