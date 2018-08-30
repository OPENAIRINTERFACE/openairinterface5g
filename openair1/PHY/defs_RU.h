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


#define MAX_BANDS_PER_RRU 4
#define MAX_NUM_RU_PER_gNB MAX_NUM_RU_PER_eNB


#ifdef OCP_FRAMEWORK
#include <enums.h>
#else
typedef enum {normal_txrx=0,rx_calib_ue=1,rx_calib_ue_med=2,rx_calib_ue_byp=3,debug_prach=4,no_L2_connect=5,calib_prach_tx=6,rx_dump_frame=7,loop_through_memory=8} runmode_t;

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
  CBA_ACCESS};

typedef enum  {
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
#endif


typedef struct {
  /// \brief Holds the transmit data in the frequency domain.
  /// - first index: rx antenna [0..nb_antennas_rx[
  /// - second index: ? [0..2*ofdm_symbol_size*frame_parms->symbols_per_tti[
  int32_t **txdata;
  /// \brief holds the transmit data after beamforming in the frequency domain.
  /// - first index: tx antenna [0..nb_antennas_tx[
  /// - second index: sample [0..]
  int32_t **txdataF_BF;
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
} RU_COMMON;


typedef struct RU_proc_t_s {
  /// Pointer to associated RU descriptor
  struct RU_t_s *ru;
  /// timestamp received from HW
  openair0_timestamp timestamp_rx;
  /// timestamp to send to "slave rru"
  openair0_timestamp timestamp_tx;
  /// subframe to act upon for reception
  int subframe_rx;
  /// subframe to act upon for transmission
  int subframe_tx;
  /// subframe to act upon for reception of prach
  int subframe_prach;
#if (RRC_VERSION >= MAKE_VERSION(14, 0, 0))
  /// subframe to act upon for reception of prach BL/CE UEs
  int subframe_prach_br;
#endif
  /// frame to act upon for reception
  int frame_rx;
  /// frame to act upon for transmission
  int frame_tx;
  /// unwrapped frame count
  int frame_tx_unwrap;
  /// frame to act upon for reception of prach
  int frame_prach;
#if (RRC_VERSION >= MAKE_VERSION(14, 0, 0))
  /// frame to act upon for reception of prach
  int frame_prach_br;
#endif
  /// frame offset for slave RUs (to correct for frame asynchronism at startup)
  int frame_offset;
  /// \brief Instance count for FH processing thread.
  /// \internal This variable is protected by \ref mutex_FH.
  int instance_cnt_FH;
  int instance_cnt_FH1;
  /// \internal This variable is protected by \ref mutex_prach.
  int instance_cnt_prach;
#if (RRC_VERSION >= MAKE_VERSION(14, 0, 0))
  /// \internal This variable is protected by \ref mutex_prach.
  int instance_cnt_prach_br;
#endif
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
  /// This varible is protected by \ref mutex_emulatedRF
  int instance_cnt_emulateRF;
  /// pthread structure for RU FH processing thread
  pthread_t pthread_FH;
  pthread_t pthread_FH1;
  /// pthread structure for RU prach processing thread
  pthread_t pthread_prach;
#if (RRC_VERSION >= MAKE_VERSION(14, 0, 0))
  /// pthread structure for RU prach processing thread BL/CE UEs
  pthread_t pthread_prach_br;
#endif
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
  /// pthread attributes for RU prach
  pthread_attr_t attr_prach;
#if (RRC_VERSION >= MAKE_VERSION(14, 0, 0))
  /// pthread attributes for RU prach BL/CE UEs
  pthread_attr_t attr_prach_br;
#endif
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
#if (RRC_VERSION >= MAKE_VERSION(14, 0, 0))
  /// scheduling parameters for RU prach thread BL/CE UEs
  struct sched_param sched_param_prach_br;
#endif
  /// scheduling parameters for RU synch thread
  struct sched_param sched_param_synch;
  /// scheduling parameters for asynch_rxtx thread
  struct sched_param sched_param_asynch_rxtx;
  /// condition variable for RU FH thread
  pthread_cond_t cond_FH;
  pthread_cond_t cond_FH1;
  /// condition variable for RU prach thread
  pthread_cond_t cond_prach;
#if (RRC_VERSION >= MAKE_VERSION(14, 0, 0))
  /// condition variable for RU prach thread BL/CE UEs
  pthread_cond_t cond_prach_br;
#endif
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
  /// mutex for RU FH
  pthread_mutex_t mutex_FH;
  pthread_mutex_t mutex_FH1;
  /// mutex for RU prach
  pthread_mutex_t mutex_prach;
#if (RRC_VERSION >= MAKE_VERSION(14, 0, 0))
  /// mutex for RU prach BL/CE UEs
  pthread_mutex_t mutex_prach_br;
#endif
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
  /// mutex for emulated RF thread
  pthread_mutex_t mutex_emulateRF;
  /// symbol mask for IF4p5 reception per subframe
  uint32_t symbol_mask[10];
  /// number of slave threads
  int                  num_slaves;
  /// array of pointers to slaves
  struct RU_proc_t_s           **slave_proc;
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

typedef struct RU_t_s{
  /// index of this ru
  uint32_t idx;
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
  /// flag to indicate the RU is in synch with a master reference
  int in_synch;
  /// timing offset
  int rx_offset;        
  /// flag to indicate the RU is a slave to another source
  int is_slave;
  /// Total gain of receive chain
  uint32_t             rx_total_gain_dB;
  /// number of bands that this device can support
  int num_bands;
  /// band list
  int band[MAX_BANDS_PER_RRU];
  /// number of RX paths on device
  int nb_rx;
  /// number of TX paths on device
  int nb_tx;
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
  /// Frame parameters
  struct LTE_DL_FRAME_PARMS *frame_parms;
  struct NR_DL_FRAME_PARMS *nr_frame_parms;
  ///timing offset used in TDD
  int              N_TA_offset; 
  /// RF device descriptor
  openair0_device rfdevice;
  /// HW configuration
  openair0_config_t openair0_cfg;
  /// Number of NBs using this RU
  int num_eNB;
  int num_gNB;
  /// list of NBs using this RU
  struct PHY_VARS_eNB_s *eNB_list[NUMBER_OF_eNB_MAX];
  struct PHY_VARS_gNB_s *gNB_list[NUMBER_OF_eNB_MAX];
  /// Mapping of antenna ports to RF chain index
  openair0_rf_map      rf_map;
  /// IF device descriptor
  openair0_device ifdevice;
  /// Pointer for ifdevice buffer struct
  if_buffer_t ifbuffer;
  /// if prach processing is to be performed in RU
  int                  do_prach;
  /// function pointer to synchronous RX fronthaul function (RRU,3GPP_eNB/3GPP_gNB)
  void                 (*fh_south_in)(struct RU_t_s *ru,int *frame, int *subframe);
  /// function pointer to synchronous TX fronthaul function
  void                 (*fh_south_out)(struct RU_t_s *ru);
  /// function pointer to synchronous RX fronthaul function (RRU)
  void                 (*fh_north_in)(struct RU_t_s *ru,int *frame, int *subframe);
  /// function pointer to synchronous RX fronthaul function (RRU)
  void                 (*fh_north_out)(struct RU_t_s *ru);
  /// function pointer to asynchronous fronthaul interface
  void                 (*fh_north_asynch_in)(struct RU_t_s *ru,int *frame, int *subframe);
  /// function pointer to asynchronous fronthaul interface
  void                 (*fh_south_asynch_in)(struct RU_t_s *ru,int *frame, int *subframe);
  /// function pointer to initialization function for radio interface
  int                  (*start_rf)(struct RU_t_s *ru);
  /// function pointer to release function for radio interface
  int                  (*stop_rf)(struct RU_t_s *ru);
  /// function pointer to initialization function for radio interface
  int                  (*start_if)(struct RU_t_s *ru,struct PHY_VARS_eNB_s *eNB);
  /// function pointer to RX front-end processing routine (DFTs/prefix removal or NULL)
  void                 (*feprx)(struct RU_t_s *ru);
  /// function pointer to TX front-end processing routine (IDFTs and prefix removal or NULL)
  void                 (*feptx_ofdm)(struct RU_t_s *ru);
  /// function pointer to TX front-end processing routine (PRECODING)
  void                 (*feptx_prec)(struct RU_t_s *ru);
  /// function pointer to wakeup routine in lte-enb/nr-gnb.
  int (*wakeup_rxtx)(struct PHY_VARS_eNB_s *eNB, struct RU_t_s *ru);
  int (*nr_wakeup_rxtx)(struct PHY_VARS_gNB_s *gNB, struct RU_t_s *ru);
  /// function pointer to wakeup routine in lte-enb/nr-gnb.
  void (*wakeup_prach_eNB)(struct PHY_VARS_eNB_s *eNB,struct RU_t_s *ru,int frame,int subframe);

  void (*wakeup_prach_gNB)(struct PHY_VARS_gNB_s *gNB,struct RU_t_s *ru,int frame,int subframe);
#if (RRC_VERSION >= MAKE_VERSION(14, 0, 0))
  /// function pointer to wakeup routine in lte-enb.
  void (*wakeup_prach_eNB_br)(struct PHY_VARS_eNB_s *eNB,struct RU_t_s *ru,int frame,int subframe);
  #endif

  /// function pointer to NB entry routine
  void (*eNB_top)(struct PHY_VARS_eNB_s *eNB, int frame_rx, int subframe_rx, char *string, struct RU_t_s *ru);
  void (*gNB_top)(struct PHY_VARS_gNB_s *gNB, int frame_rx, int subframe_rx, char *string, struct RU_t_s *ru);

  /// Timing statistics
  time_stats_t ofdm_demod_stats;
  /// Timing statistics (TX)
  time_stats_t ofdm_mod_stats;
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
  /// Timong statistics (Compression)
  time_stats_t compression;
  /// Timing statistics (Fronthaul transport)
  time_stats_t transport;
  /// RX and TX buffers for precoder output
  RU_COMMON            common;
  /// beamforming weight vectors per eNB
  int32_t **beam_weights[NUMBER_OF_eNB_MAX+1][15];
  /// beamforming weight vectors per eNB
  int32_t **nrbeam_weights[NUMBER_OF_gNB_MAX+1][16];
  /// received frequency-domain signal for PRACH (IF4p5 RRU) 
  int16_t              **prach_rxsigF;
  /// received frequency-domain signal for PRACH BR (IF4p5 RRU) 
  int16_t              **prach_rxsigF_br[4];
  /// sequence number for IF5
  uint8_t seqno;
  /// initial timestamp used as an offset make first real timestamp 0
  openair0_timestamp   ts_offset;
  /// process scheduling variables
  RU_proc_t            proc;
  /// stats thread pthread descriptor
  pthread_t            ru_stats_thread;

} RU_t;



#define MAX_RRU_CONFIG_SIZE 1024
typedef enum {
  RAU_tick=0,
  RRU_capabilities=1,
  RRU_config=2,
  RRU_MSG_max_num=3
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

#define MAX_BANDS_PER_RRU 4

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
  /// TX attenation w.r.t. max
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
#if (RRC_VERSION >= MAKE_VERSION(14, 0, 0))
  int emtc_prach_CElevel_enable[MAX_BANDS_PER_RRU][4];
  /// emtc_prach_FreqOffset for IF4p5 per CE Level
  int emtc_prach_FreqOffset[MAX_BANDS_PER_RRU][4];
  /// emtc_prach_ConfigIndex for IF4p5 per CE Level
  int emtc_prach_ConfigIndex[MAX_BANDS_PER_RRU][4];
#endif
} RRU_config_t;

#endif //__PHY_DEFS_RU__H__
